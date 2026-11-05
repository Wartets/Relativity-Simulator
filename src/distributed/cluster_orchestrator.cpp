#include "relativistic/distributed/cluster_orchestrator.hpp"
#include "relativistic/render/software_compute_engine.hpp"
#include "relativistic/distributed/session_protocol.hpp"
#include "relativistic/core/engine_log.hpp"
#include "relativistic/core/diagnostics/diagnostics_registry.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>

namespace Relativistic::Distributed {

namespace {

constexpr size_t kMaximumEventBurst = 512;
constexpr size_t kStealBatchSize = 8;
constexpr uint32_t kMaximumIntegrityFailures = 32;
constexpr uint32_t kMaximumCompressionIndex = static_cast<uint32_t>(TileCompressionMode::Lz4Block);
constexpr double kOfflineTileTimeoutSeconds = 30.0;
constexpr double kOfflineMinimumTileTimeoutSeconds = 0.25;
constexpr double kOfflineFrameTimeoutSeconds = 900.0;
constexpr auto kPumpInterval = std::chrono::microseconds(200);
constexpr auto kWaitInterval = std::chrono::microseconds(200);
constexpr auto kMaintenanceInterval = std::chrono::milliseconds(500);
constexpr auto kReconnectBackoff = std::chrono::milliseconds(2000);
constexpr auto kHandshakeLifetime = std::chrono::milliseconds(kHandshakeTimeoutMs);

[[nodiscard]] uint32_t random_identifier() noexcept {
	static std::atomic<uint32_t> seed{0x83A142EFU};
	return seed.fetch_add(0x9E3779B9U, std::memory_order_relaxed);
}

template <typename Value>
[[nodiscard]] std::span<const uint8_t> bytes_of(const Value& value) noexcept {
	return std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&value), sizeof(Value));
}

[[nodiscard]] double to_seconds(ClusterTimePoint point) noexcept {
	return std::chrono::duration<double>(point.time_since_epoch()).count();
}

[[nodiscard]] bool split_peer_address(const std::string& text, std::string& host, uint16_t& port) {
	const size_t colon = text.rfind(':');
	if (colon == std::string::npos || colon == 0U) {
		host = text;
		port = 0;
		return !host.empty();
	}
	host = text.substr(0, colon);
	port = static_cast<uint16_t>(std::strtoul(text.c_str() + colon + 1U, nullptr, 10));
	return true;
}

}

bool FrameWindow::store_tile(const TileGeometry& geometry, std::span<const Render::GpuPixelOutput> tile_pixels) const noexcept {
	if (geometry.width == 0U || geometry.height == 0U || tile_pixels.size() < geometry.pixel_count()) {
		return false;
	}
	if (static_cast<uint64_t>(geometry.origin_x) + geometry.width > width || static_cast<uint64_t>(geometry.origin_y) + geometry.height > height) {
		return false;
	}
	const uint64_t window_end = static_cast<uint64_t>(first_row) + row_count;
	const uint32_t y_begin = std::max(geometry.origin_y, first_row);
	const uint32_t y_end = static_cast<uint32_t>(std::min<uint64_t>(static_cast<uint64_t>(geometry.origin_y) + geometry.height, window_end));
	if (y_begin >= y_end) {
		return true;
	}
	if (pixels.size() < static_cast<size_t>(width) * row_count) {
		return false;
	}
	const size_t row_bytes = static_cast<size_t>(geometry.width) * kPixelStrideBytes;
	for (uint32_t y = y_begin; y < y_end; ++y) {
		Render::GpuPixelOutput* destination = pixels.data() + static_cast<size_t>(y - first_row) * width + geometry.origin_x;
		const Render::GpuPixelOutput* source = tile_pixels.data() + static_cast<size_t>(y - geometry.origin_y) * geometry.width;
		std::memcpy(destination, source, row_bytes);
	}
	return true;
}

ClusterOrchestrator::ClusterOrchestrator(const ClusterConfiguration& configuration) noexcept
	: config_(configuration),
	  scheduler_(config_.make_load_balancer_parameters()),
	  local_abi_signature_(local_engine_abi_signature()),
	  route_interactive_(config_.task_routing.route_interactive_viewport),
	  route_offline_(config_.task_routing.route_capture_studio) {
	local_capabilities_ = detect_local_hardware_capabilities(LocalGpuInfo{});
	reseed_local_throughput();
}

ClusterOrchestrator::~ClusterOrchestrator() noexcept {
	shutdown();
}

void ClusterOrchestrator::set_error(const std::string& message) {
	std::lock_guard<std::mutex> lock(error_mutex_);
	last_error_ = message;
}

std::string ClusterOrchestrator::last_error() const {
	std::lock_guard<std::mutex> lock(error_mutex_);
	return last_error_;
}

void ClusterOrchestrator::reseed_local_throughput() noexcept {
	local_throughput_ = ThroughputEstimator{};
	const double weight = std::max(static_cast<double>(local_capabilities_.relative_performance_weight), 0.05);
	local_throughput_.seed(kDefaultSecondsPerKiloRay / weight);
}

LoadBalancerParameters ClusterOrchestrator::make_profile_parameters(DispatchProfile profile) const noexcept {
	LoadBalancerParameters parameters = config_.make_load_balancer_parameters();
	if (profile == DispatchProfile::Offline) {
		parameters.maximum_timeout_seconds = kOfflineTileTimeoutSeconds;
		parameters.minimum_timeout_seconds = kOfflineMinimumTileTimeoutSeconds;
	}
	return parameters;
}

void ClusterOrchestrator::set_local_tile_renderer(LocalTileRenderer renderer) {
	std::lock_guard<std::mutex> lock(renderer_mutex_);
	local_renderer_ = std::move(renderer);
}

void ClusterOrchestrator::clear_local_tile_renderer() noexcept {
	try {
		std::lock_guard<std::mutex> lock(renderer_mutex_);
		local_renderer_ = nullptr;
	} catch (...) {
	}
}

void ClusterOrchestrator::set_local_gpu_info(const LocalGpuInfo& info) noexcept {
	try {
		std::lock_guard<std::mutex> lock(pump_mutex_);
		local_capabilities_ = detect_local_hardware_capabilities(info);
		reseed_local_throughput();
	} catch (...) {
	}
}

uint32_t ClusterOrchestrator::workers_active() const {
	return static_cast<uint32_t>(registry_.schedulable_count());
}

ClusterFrameTelemetry ClusterOrchestrator::last_frame_telemetry() const {
	std::lock_guard<std::mutex> lock(telemetry_mutex_);
	return telemetry_;
}

bool ClusterOrchestrator::start() noexcept {
	if (!config_.cluster.enabled) {
		cluster_active_.store(false, std::memory_order_relaxed);
		return true;
	}
	if (cluster_active_.load(std::memory_order_acquire)) {
		return true;
	}
	try {
		if (!config_.security.has_shared_token) {
			set_error("A cluster token is required to start the master");
			Core::log_error("The cluster master cannot start without a shared token.");
			return false;
		}

		instance_identifier_ = (static_cast<uint64_t>(random_identifier()) << 32U) | random_identifier();
		tile_dispatch_time_.assign(kMaximumSchedulerTiles, ClusterTimePoint{});

		TransportConfiguration transport_config = config_.make_transport_configuration();
		std::optional<TransportEndpoint> listen_endpoint;
		if (config_.cluster.listen_port != 0) {
			listen_endpoint = TransportEndpoint{config_.cluster.listen_interface, config_.cluster.listen_port};
		}
		if (!transport_.open(listen_endpoint, transport_config)) {
			set_error("The network transport could not be opened");
			Core::log_error("Failed to open network transport for master node.");
			return false;
		}

		if (config_.cluster.discovery_multicast_enabled) {
			DiscoveryConfiguration discovery_config = config_.make_discovery_configuration(
				"relativistic-master",
				local_abi_signature_,
				local_capabilities_.relative_performance_weight
			);
			discovery_config.role = (config_.cluster.node_role == ClusterNodeRole::Hybrid) ? ClusterNodeRole::Hybrid : ClusterNodeRole::Master;
			if (!discovery_.start(discovery_config)) {
				Core::log_warning("Cluster discovery is disabled: " + discovery_.last_error());
			}
		}

		next_maintenance_ = ClusterClock::now();
		cluster_active_.store(true, std::memory_order_release);
		pump_thread_ = std::jthread([this](std::stop_token stop) { pump_loop(stop); });
		Core::log_info("Cluster master started on port " + std::to_string(config_.cluster.listen_port) + " with " + std::to_string(config_.static_nodes.size()) + " static node(s).");
		return true;
	} catch (const std::exception& ex) {
		set_error(ex.what());
		Core::log_error(std::string("Cluster master start failed: ") + ex.what());
		return false;
	}
}

void ClusterOrchestrator::shutdown() noexcept {
	if (!cluster_active_.exchange(false, std::memory_order_acq_rel)) {
		return;
	}
	try {
		pump_thread_.request_stop();
		if (pump_thread_.joinable()) {
			pump_thread_.join();
		}
		{
			std::lock_guard<std::mutex> wait_for_dispatch(dispatch_mutex_);
		}
		std::lock_guard<std::mutex> lock(pump_mutex_);
		active_window_ = nullptr;
		discovery_.stop();
		for (auto& [peer, session] : sessions_by_peer_) {
			transport_.disconnect(peer, static_cast<uint32_t>(DisconnectReason::Shutdown), false);
		}
		transport_.flush();
		transport_.close();
		sessions_by_peer_.clear();
		sessions_by_node_.clear();
		discovered_peers_.clear();
		reconnect_after_.clear();
		reclaimed_by_node_loss_.clear();
		registry_.clear();
	} catch (...) {
	}
}

void ClusterOrchestrator::pump_loop(std::stop_token stop) noexcept {
	Core::Diagnostics::set_thread_name("cluster-pump");
	while (!stop.stop_requested()) {
		try {
			std::lock_guard<std::mutex> lock(pump_mutex_);
			service_unlocked(ClusterClock::now());
		} catch (...) {
		}
		std::this_thread::sleep_for(kPumpInterval);
	}
}

void ClusterOrchestrator::service_unlocked(ClusterTimePoint now) {
	process_network_events();
	ping_heartbeats(now);
	detect_timeouts(now);
	if (now >= next_maintenance_) {
		next_maintenance_ = now + kMaintenanceInterval;
		maintain_connections(now);
	}
}

bool ClusterOrchestrator::send_encrypted_packet(
	MasterPeerSession& session,
	PacketType type,
	uint64_t frame_index,
	std::span<const uint8_t> payload,
	TransportChannel channel
) {
	std::lock_guard<std::mutex> lock(session.send_mutex);
	if (session.transmit_sequence >= kSessionRekeyPacketLimit) {
		session.rekey_required.store(true, std::memory_order_release);
		return false;
	}
	thread_local std::vector<uint8_t> ciphertext;
	if (ciphertext.size() < payload.size()) {
		ciphertext.resize(payload.size());
	}
	PacketEnvelope envelope = make_envelope(type, session.session_identifier, session.transmit_sequence, frame_index, payload);
	const std::span<uint8_t, 16> tag(envelope.authentication_tag);
	const std::span<uint8_t> cipher_view(ciphertext.data(), payload.size());
	if (!session.security_channel.encrypt_and_authenticate_packet(envelope.authenticated_header(), payload, cipher_view, tag)) {
		return false;
	}
	++session.transmit_sequence;
	return transport_.send_gather(session.peer_id, channel, bytes_of(envelope), std::span<const uint8_t>(ciphertext.data(), payload.size()));
}

bool ClusterOrchestrator::send_plain_packet(MasterPeerSession& session, PacketType type, std::span<const uint8_t> payload) {
	const PacketEnvelope envelope = make_envelope(type, session.session_identifier, 0U, 0U, payload);
	return transport_.send_gather(session.peer_id, TransportChannel::Control, bytes_of(envelope), payload);
}

void ClusterOrchestrator::close_session(MasterPeerSession& session, DisconnectReason reason) noexcept {
	transport_.disconnect(session.peer_id, static_cast<uint32_t>(reason), true);
}

void ClusterOrchestrator::broadcast_bodies_if_needed(uint32_t frame_index, std::span<const Render::GpuBodyData> bodies) {
	if (bodies.empty() || last_broadcast_catalog_frame_ == frame_index) {
		return;
	}
	last_broadcast_catalog_frame_ = frame_index;

	thread_local std::vector<uint8_t> chunk_buffer;
	const size_t needed = sizeof(BodyCatalogChunkHeader) + kBodiesPerCatalogChunk * sizeof(Render::GpuBodyData);
	if (chunk_buffer.size() < needed) {
		chunk_buffer.resize(needed);
	}

	uint32_t first = 0;
	while (first < bodies.size()) {
		const uint32_t count = static_cast<uint32_t>(std::min<size_t>(kBodiesPerCatalogChunk, bodies.size() - first));
		const size_t bytes = serialize_body_catalog_chunk(frame_index, bodies, first, count, chunk_buffer);
		if (bytes > 0) {
			for (auto& [peer, session] : sessions_by_peer_) {
				if (session->authenticated && session->node_id != 0U) {
					static_cast<void>(send_encrypted_packet(*session, PacketType::BodyCatalogChunk, frame_index, std::span<const uint8_t>(chunk_buffer.data(), bytes), TransportChannel::Control));
				}
			}
		}
		first += count;
	}
}

void ClusterOrchestrator::cancel_remote_frame(uint64_t frame_index) {
	TaskCancelPayload payload{};
	payload.frame_index = static_cast<uint32_t>(frame_index);
	payload.tile_index = 0U;
	payload.scope = static_cast<uint32_t>(TaskCancelScope::WholeFrame);
	for (auto& [peer, session] : sessions_by_peer_) {
		if (session->authenticated && session->node_id != 0U) {
			static_cast<void>(send_encrypted_packet(*session, PacketType::TaskCancel, frame_index, bytes_of(payload), TransportChannel::Control));
		}
	}
}

void ClusterOrchestrator::ping_heartbeats(ClusterTimePoint now) noexcept {
	const auto interval = std::chrono::milliseconds(config_.cluster.heartbeat_interval_ms);
	for (auto& [peer, session] : sessions_by_peer_) {
		if (session->authenticated && session->node_id != 0U && (now - session->last_ping >= interval)) {
			session->last_ping = now;
			session->last_ping_timestamp = to_seconds(now);
			const double timestamp = session->last_ping_timestamp;
			static_cast<void>(send_encrypted_packet(*session, PacketType::HeartbeatPing, 0U, bytes_of(timestamp), TransportChannel::Control));
		}
		if (session->rekey_required.exchange(false, std::memory_order_acq_rel)) {
			close_session(*session, DisconnectReason::RekeyRequired);
		}
	}
}

void ClusterOrchestrator::detect_timeouts(ClusterTimePoint now) noexcept {
	std::vector<uint32_t> expired_nodes;
	registry_.enforce_timeouts(now, config_.cluster.node_timeout_ms, expired_nodes);
	std::vector<ReclaimedTile> reclaimed;
	for (const uint32_t node_id : expired_nodes) {
		scheduler_.reclaim_node(node_id, reclaimed);
		const auto it = sessions_by_node_.find(node_id);
		if (it != sessions_by_node_.end()) {
			transport_.disconnect(it->second->peer_id, static_cast<uint32_t>(DisconnectReason::HandshakeTimeout), true);
			sessions_by_peer_.erase(it->second->peer_id);
			sessions_by_node_.erase(it);
		}
	}
	for (const ReclaimedTile& tile : reclaimed) {
		reclaimed_by_node_loss_.push_back(tile.tile_index);
	}
	for (auto it = sessions_by_peer_.begin(); it != sessions_by_peer_.end();) {
		MasterPeerSession& session = *it->second;
		if (session.node_id == 0U && (now - session.created) > kHandshakeLifetime) {
			transport_.disconnect(session.peer_id, static_cast<uint32_t>(DisconnectReason::HandshakeTimeout), false);
			it = sessions_by_peer_.erase(it);
		} else {
			++it;
		}
	}
}

void ClusterOrchestrator::ensure_connection(const ClusterNodeDescriptor& descriptor, ClusterTimePoint now) {
	if (!descriptor.enabled || descriptor.endpoint.host.empty() || descriptor.endpoint.port == 0U) {
		return;
	}
	uint32_t node_id = 0;
	const auto existing = registry_.find_by_endpoint(descriptor.endpoint);
	if (existing.has_value()) {
		node_id = *existing;
	} else {
		const auto registered = registry_.register_node(descriptor, NodeState::Disconnected, now);
		if (!registered.has_value()) {
			return;
		}
		node_id = *registered;
	}
	bool needs_connection = false;
	registry_.visit(node_id, [&needs_connection](ClusterNode& node) {
		needs_connection = node.state() == NodeState::Disconnected && node.peer_identifier() == kInvalidPeerId && node.descriptor().enabled;
	});
	if (!needs_connection) {
		return;
	}
	const auto backoff = reconnect_after_.find(node_id);
	if (backoff != reconnect_after_.end() && now < backoff->second) {
		return;
	}
	reconnect_after_[node_id] = now + kReconnectBackoff;
	const auto peer = transport_.connect(descriptor.endpoint);
	if (!peer.has_value()) {
		return;
	}
	if (!registry_.bind_peer(node_id, *peer, now)) {
		transport_.disconnect(*peer, static_cast<uint32_t>(DisconnectReason::ProtocolViolation), false);
		return;
	}
	auto session = std::make_shared<MasterPeerSession>();
	session->peer_id = *peer;
	session->node_id = node_id;
	session->initiated = true;
	session->handshaking = true;
	session->identity = CryptoChannel::generate_node_identity_keys(
		(static_cast<uint64_t>(random_identifier()) << 32U) ^ random_identifier() ^ static_cast<uint64_t>(now.time_since_epoch().count())
	);
	session->session_identifier = random_identifier();
	session->created = now;
	sessions_by_peer_[*peer] = session;
	sessions_by_node_[node_id] = session;
}

void ClusterOrchestrator::maintain_connections(ClusterTimePoint now) {
	std::vector<DiscoveryEvent> events;
	discovery_.drain_events(events);
	for (const DiscoveryEvent& event : events) {
		if (event.type == DiscoveryEventType::PeerLost) {
			discovered_peers_.erase(event.peer.instance_identifier);
		} else if (event.peer.role == ClusterNodeRole::Worker || event.peer.role == ClusterNodeRole::Hybrid) {
			discovered_peers_[event.peer.instance_identifier] = event.peer;
		}
	}

	for (const ClusterNodeDescriptor& descriptor : config_.static_nodes) {
		ensure_connection(descriptor, now);
	}
	for (const auto& [identifier, peer] : discovered_peers_) {
		if (peer.abi_signature != local_abi_signature_) {
			continue;
		}
		ClusterNodeDescriptor descriptor{};
		descriptor.name = peer.name;
		descriptor.endpoint = TransportEndpoint{peer.address, peer.service_port};
		descriptor.static_weight = 1.0;
		descriptor.enabled = true;
		ensure_connection(descriptor, now);
	}
}

void ClusterOrchestrator::on_peer_connected(uint32_t peer_id) {
	const auto it = sessions_by_peer_.find(peer_id);
	if (it == sessions_by_peer_.end()) {
		auto session = std::make_shared<MasterPeerSession>();
		session->peer_id = peer_id;
		session->initiated = false;
		session->handshaking = true;
		session->identity = CryptoChannel::generate_node_identity_keys(
			(static_cast<uint64_t>(random_identifier()) << 32U) ^ random_identifier() ^ static_cast<uint64_t>(ClusterClock::now().time_since_epoch().count())
		);
		session->session_identifier = random_identifier();
		session->created = ClusterClock::now();
		sessions_by_peer_[peer_id] = session;
		return;
	}
	MasterPeerSession& session = *it->second;
	if (session.initiated && session.handshaking && !session.authenticated) {
		static_cast<void>(send_plain_packet(session, PacketType::NodeAuthentication, session.identity.public_key));
	}
}

void ClusterOrchestrator::on_peer_disconnected(uint32_t peer_id) {
	const auto it = sessions_by_peer_.find(peer_id);
	if (it != sessions_by_peer_.end()) {
		const std::shared_ptr<MasterPeerSession> session = it->second;
		const auto by_node = sessions_by_node_.find(session->node_id);
		if (by_node != sessions_by_node_.end() && by_node->second == session) {
			sessions_by_node_.erase(by_node);
		}
		sessions_by_peer_.erase(it);
	}
	const auto node_id = registry_.release_peer(peer_id, ClusterClock::now());
	if (node_id.has_value()) {
		std::vector<ReclaimedTile> reclaimed;
		scheduler_.reclaim_node(*node_id, reclaimed);
		for (const ReclaimedTile& tile : reclaimed) {
			reclaimed_by_node_loss_.push_back(tile.tile_index);
		}
	}
}

void ClusterOrchestrator::process_network_events() noexcept {
	thread_local std::vector<uint8_t> receive_buffer(kMaximumTransportPacketBytes);
	thread_local std::vector<uint8_t> plain_buffer(kMaximumTransportPacketBytes);
	TransportEvent event{};
	size_t handled = 0;
	try {
		while (handled < kMaximumEventBurst && transport_.poll(event, receive_buffer, 0U)) {
			++handled;
			switch (event.type) {
				case TransportEventType::Connected:
					on_peer_connected(event.peer_id);
					break;
				case TransportEventType::Disconnected:
					on_peer_disconnected(event.peer_id);
					break;
				case TransportEventType::Received:
					on_packet_received(event, receive_buffer, plain_buffer);
					break;
				default:
					break;
			}
		}
	} catch (...) {
	}
}

void ClusterOrchestrator::on_packet_received(const TransportEvent& event, std::span<const uint8_t> wire, std::span<uint8_t> plain) {
	const auto it = sessions_by_peer_.find(event.peer_id);
	if (it == sessions_by_peer_.end() || event.truncated || event.payload_bytes < sizeof(PacketEnvelope) || event.payload_bytes > wire.size()) {
		return;
	}
	const std::shared_ptr<MasterPeerSession> session = it->second;
	PacketEnvelope envelope{};
	std::memcpy(&envelope, wire.data(), sizeof(PacketEnvelope));
	if (!envelope.is_valid_magic() || envelope.protocol_version != PacketEnvelope::kProtocolVersion) {
		return;
	}
	const size_t payload_bytes = event.payload_bytes - sizeof(PacketEnvelope);
	if (payload_bytes != envelope.payload_bytes || payload_bytes > plain.size()) {
		return;
	}
	const std::span<const uint8_t> cipher(wire.data() + sizeof(PacketEnvelope), payload_bytes);
	if (!session->authenticated) {
		handle_handshake_packet(session, envelope, cipher);
		return;
	}
	const std::span<uint8_t> plaintext(plain.data(), payload_bytes);
	if (!session->security_channel.verify_and_decrypt_packet(envelope.authenticated_header(), envelope.packet_sequence, cipher, envelope.authentication_tag, plaintext)) {
		if (++session->integrity_failures > kMaximumIntegrityFailures) {
			close_session(*session, DisconnectReason::AuthenticationFailed);
		}
		return;
	}
	session->integrity_failures = 0;
	handle_authenticated_packet(session, envelope, std::span<const uint8_t>(plaintext));
}

void ClusterOrchestrator::handle_handshake_packet(const std::shared_ptr<MasterPeerSession>& session, const PacketEnvelope& envelope, std::span<const uint8_t> cipher) {
	if (!session->handshaking || cipher.size() != 32U) {
		return;
	}
	const ClusterNodeRole role = (config_.cluster.node_role == ClusterNodeRole::Hybrid) ? ClusterNodeRole::Hybrid : ClusterNodeRole::Master;
	const std::span<const uint8_t, 32> remote_key(cipher.data(), 32);

	if (envelope.type == PacketType::NodeAuthChallenge && session->initiated) {
		if (!session->security_channel.perform_handshake_client(remote_key, session->identity.private_key, config_.security.cluster_shared_token)) {
			close_session(*session, DisconnectReason::AuthenticationFailed);
			return;
		}
		session->authenticated = true;
		session->handshaking = false;
		AuthenticationProofPayload proof{};
		proof.role = static_cast<uint32_t>(role);
		proof.instance_identifier = instance_identifier_;
		static_cast<void>(send_encrypted_packet(*session, PacketType::NodeAuthResponse, 0U, bytes_of(proof), TransportChannel::Control));
		static_cast<void>(send_encrypted_packet(*session, PacketType::CapabilitiesAdvertisement, 0U, bytes_of(local_capabilities_), TransportChannel::Control));
	} else if (envelope.type == PacketType::NodeAuthentication && !session->initiated) {
		if (!session->security_channel.perform_handshake_server(remote_key, session->identity.private_key, config_.security.cluster_shared_token)) {
			close_session(*session, DisconnectReason::AuthenticationFailed);
			return;
		}
		session->authenticated = true;
		session->handshaking = false;
		static_cast<void>(send_plain_packet(*session, PacketType::NodeAuthChallenge, session->identity.public_key));
		static_cast<void>(send_encrypted_packet(*session, PacketType::CapabilitiesAdvertisement, 0U, bytes_of(local_capabilities_), TransportChannel::Control));
	}
}

void ClusterOrchestrator::handle_authenticated_packet(const std::shared_ptr<MasterPeerSession>& session, const PacketEnvelope& envelope, std::span<const uint8_t> plain) {
	const ClusterTimePoint now = ClusterClock::now();
	switch (envelope.type) {
		case PacketType::HeartbeatPong: {
			const double round_trip = to_seconds(now) - session->last_ping_timestamp;
			registry_.visit(session->node_id, [now, round_trip](ClusterNode& node) {
				node.record_heartbeat(now);
				if (round_trip > 0.0) {
					node.record_round_trip(round_trip);
				}
				if (node.state() == NodeState::Stalled) {
					node.release_in_flight(now);
					static_cast<void>(node.transition_to(NodeState::Ready, now));
				}
			});
			break;
		}
		case PacketType::CapabilitiesAdvertisement: {
			if (plain.size() < sizeof(NodeHardwareCapabilities)) {
				break;
			}
			NodeHardwareCapabilities remote{};
			std::memcpy(&remote, plain.data(), sizeof(NodeHardwareCapabilities));
			if (session->node_id == 0U) {
				ClusterNodeDescriptor descriptor{};
				std::string host;
				uint16_t port = 0;
				static_cast<void>(split_peer_address(transport_.peer_address(session->peer_id), host, port));
				descriptor.name = "worker-" + std::to_string(session->peer_id);
				descriptor.endpoint = TransportEndpoint{host, port};
				descriptor.static_weight = 1.0;
				descriptor.enabled = true;
				const auto registered = registry_.register_node(descriptor, NodeState::Unauthenticated, now);
				if (!registered.has_value()) {
					close_session(*session, DisconnectReason::SessionLimit);
					break;
				}
				session->node_id = *registered;
				sessions_by_node_[*registered] = session;
				static_cast<void>(registry_.bind_peer(*registered, session->peer_id, now));
			}
			HandshakeVerdict verdict = HandshakeVerdict::Accepted;
			const NodeHardwareCapabilities local = local_capabilities_;
			registry_.visit(session->node_id, [&](ClusterNode& node) {
				if (node.state() != NodeState::Unauthenticated) {
					return;
				}
				static_cast<void>(node.begin_handshake(now));
				NodeAdvertisement advertisement{};
				advertisement.hardware = remote;
				advertisement.role = ClusterNodeRole::Worker;
				advertisement.name = node.descriptor().name;
				verdict = node.accept_advertisement(advertisement, local, false, now);
				if (verdict == HandshakeVerdict::Accepted) {
					static_cast<void>(node.transition_to(NodeState::Synchronized, now));
					static_cast<void>(node.mark_ready(now));
				}
			});
			if (verdict != HandshakeVerdict::Accepted) {
				close_session(*session, (verdict == HandshakeVerdict::AbiMismatch) ? DisconnectReason::AbiMismatch : DisconnectReason::InvalidCapabilities);
			}
			break;
		}
		case PacketType::TileResultData: {
			thread_local std::array<Render::GpuPixelOutput, kTilePixelCapacity> tile_pixels;
			const auto decode_start = std::chrono::steady_clock::now();
			TileCodecScratch& scratch = TileCodec::thread_scratch();
			const DecodedTileResult decoded = decode_render_tile_result(plain, tile_pixels, scratch);
			frame_decode_seconds_ += std::chrono::duration<double>(std::chrono::steady_clock::now() - decode_start).count();
			frame_bytes_received_ += plain.size();
			if (decoded.status != TileDecodeStatus::Ok || active_window_ == nullptr || static_cast<uint32_t>(active_frame_) != envelope.frame_index) {
				break;
			}
			const uint32_t tile = decoded.header.tile_index;
			if (tile >= tile_dispatch_time_.size()) {
				break;
			}
			const auto geometry = resolve_tile_geometry(tile, active_window_->width, active_window_->height);
			if (!geometry.has_value() || decoded.header.pixel_count != geometry->pixel_count()) {
				break;
			}
			const TileCompletionResult result = scheduler_.complete_remote(active_frame_, tile, session->node_id);
			if (result == TileCompletionResult::Accepted) {
				static_cast<void>(active_window_->store_tile(*geometry, std::span<const Render::GpuPixelOutput>(tile_pixels.data(), geometry->pixel_count())));
				const double execution_seconds = static_cast<double>(decoded.header.execution_time_us) * 1.0e-6;
				const double round_trip = seconds_between(tile_dispatch_time_[tile], now);
				const auto pixel_count = static_cast<uint32_t>(geometry->pixel_count());
				registry_.visit(session->node_id, [=](ClusterNode& node) {
					node.register_completion(pixel_count, execution_seconds, round_trip, now);
				});
			} else if (result == TileCompletionResult::DuplicateIgnored) {
				registry_.visit(session->node_id, [](ClusterNode& node) { node.register_late_result(); });
			}
			break;
		}
		default:
			break;
	}
}

bool ClusterOrchestrator::prepare_frame_workload(
	uint64_t frame_index,
	uint32_t total_tiles,
	std::span<const uint8_t> tile_importance_mask
) {
	thread_local std::vector<NodeCapacityInput> capacities;
	registry_.collect_capacity_inputs(capacities);
	NodeCapacityInput local{};
	local.node_id = kLocalNodeIdentifier;
	local.is_local = true;
	local.eligible = true;
	local.static_weight = 1.0;
	local.seconds_per_kilo_ray = local_throughput_.seconds_per_kilo_ray();
	local.round_trip_seconds = 0.0;
	capacities.push_back(local);
	return scheduler_.begin_frame(frame_index, total_tiles, tile_importance_mask, capacities);
}

void ClusterOrchestrator::send_remote_tasks(
	uint64_t frame_index,
	const Render::GpuCameraPushConstants& constants,
	bool bodies_empty,
	ClusterTimePoint now,
	std::vector<uint32_t>& undispatched
) {
	const uint32_t width = constants.screen_width;
	const uint32_t height = constants.screen_height;
	const uint8_t flags = bodies_empty ? kTileTaskFlagNoBodies : static_cast<uint8_t>(0U);
	const auto mode = static_cast<TileCompressionMode>(std::min(config_.performance.compression_mode, kMaximumCompressionIndex));

	thread_local std::vector<uint8_t> task_buffer(kRenderTileTaskPayloadBytes);
	RenderTileTaskPayload task{};
	task.camera_constants = constants;
	task.dispatch_timestamp = to_seconds(now);

	const size_t slot_count = scheduler_.slot_count();
	for (size_t slot_index = 0; slot_index < slot_count; ++slot_index) {
		const SlotAllocation slot = scheduler_.slot_allocation(slot_index);
		if (slot.is_local) {
			continue;
		}
		const auto tiles = scheduler_.slot_tiles(slot_index);
		const auto session_it = sessions_by_node_.find(slot.node_id);
		if (session_it == sessions_by_node_.end() || !session_it->second->authenticated) {
			undispatched.insert(undispatched.end(), tiles.begin(), tiles.end());
			continue;
		}
		MasterPeerSession& session = *session_it->second;
		DispatchTiming timing{};
		registry_.visit(slot.node_id, [&timing](ClusterNode& node) { timing = node.dispatch_timing(); });

		for (const uint32_t tile : tiles) {
			const auto header = make_render_tile_task_header(tile, width, height, 0U, flags, mode);
			if (!header.has_value()) {
				undispatched.push_back(tile);
				continue;
			}
			task.header = *header;
			const size_t task_bytes = serialize_render_tile_task(task, task_buffer);
			if (task_bytes == 0U || !send_encrypted_packet(session, PacketType::TaskAssignment, frame_index, std::span<const uint8_t>(task_buffer.data(), task_bytes), TransportChannel::TileData)) {
				undispatched.push_back(tile);
				continue;
			}
			const uint32_t pixel_count = static_cast<uint32_t>(header->tile_width) * header->tile_height;
			if (scheduler_.mark_dispatched(tile, slot.node_id, now, pixel_count, timing)) {
				tile_dispatch_time_[tile] = now;
				registry_.visit(slot.node_id, [now](ClusterNode& node) { static_cast<void>(node.register_dispatch(now)); });
			}
		}
	}
}

bool ClusterOrchestrator::render_tiles_software(
	const Render::GpuCameraPushConstants& constants,
	std::span<const uint32_t> tiles,
	std::span<const Render::GpuBodyData> bodies,
	const TileCompletionSink& sink,
	const std::atomic<bool>* cancel_flag
) {
	if (tiles.empty()) {
		return true;
	}
	std::atomic<bool> failed{false};
	const uint32_t width = constants.screen_width;
	const uint32_t height = constants.screen_height;
	Core::global_render_thread_pool().parallel_for(tiles.size(), [&](size_t begin, size_t end) noexcept {
		thread_local std::array<Render::GpuPixelOutput, kTilePixelCapacity> pixels;
		for (size_t i = begin; i < end; ++i) {
			if (cancel_flag != nullptr && cancel_flag->load(std::memory_order_relaxed)) {
				failed.store(true, std::memory_order_relaxed);
				return;
			}
			const auto geometry = resolve_tile_geometry(tiles[i], width, height);
			if (!geometry.has_value()) {
				failed.store(true, std::memory_order_relaxed);
				continue;
			}
			const Render::SoftwareComputeEngine::TileRegion region{geometry->origin_x, geometry->origin_y, geometry->width, geometry->height};
			const std::span<Render::GpuPixelOutput> output(pixels.data(), geometry->pixel_count());
			if (!Render::SoftwareComputeEngine::dispatch_tile(constants, region, output, bodies, cancel_flag)) {
				failed.store(true, std::memory_order_relaxed);
				continue;
			}
			try {
				sink(tiles[i], output);
			} catch (...) {
				failed.store(true, std::memory_order_relaxed);
			}
		}
	}, 1);
	return !failed.load(std::memory_order_relaxed);
}

bool ClusterOrchestrator::run_local_batch(
	const Render::GpuCameraPushConstants& constants,
	std::span<const uint32_t> tiles,
	std::span<const Render::GpuBodyData> bodies,
	const FrameWindow& window,
	uint64_t frame_index,
	const LocalTileRenderer& renderer,
	const std::atomic<bool>* cancel_flag
) {
	thread_local std::vector<uint32_t> pending;
	pending.clear();
	for (const uint32_t tile : tiles) {
		if (scheduler_.should_compute_local(frame_index, tile)) {
			pending.push_back(tile);
		}
	}
	if (pending.empty()) {
		return true;
	}
	const uint32_t width = constants.screen_width;
	const uint32_t height = constants.screen_height;
	const TileCompletionSink sink = [this, &window, frame_index, width, height](uint32_t tile, std::span<const Render::GpuPixelOutput> pixels) {
		const auto geometry = resolve_tile_geometry(tile, width, height);
		if (geometry.has_value() && scheduler_.try_complete_local(frame_index, tile)) {
			static_cast<void>(window.store_tile(*geometry, pixels));
		}
	};

	uint64_t pixel_total = 0;
	for (const uint32_t tile : pending) {
		const auto geometry = resolve_tile_geometry(tile, width, height);
		if (geometry.has_value()) {
			pixel_total += geometry->pixel_count();
		}
	}

	const auto started = std::chrono::steady_clock::now();
	bool ok = renderer ? renderer(constants, pending, bodies, sink, cancel_flag) : render_tiles_software(constants, pending, bodies, sink, cancel_flag);
	const bool cancelled = cancel_flag != nullptr && cancel_flag->load(std::memory_order_relaxed);
	if (!ok && !cancelled) {
		thread_local std::vector<uint32_t> leftovers;
		leftovers.clear();
		for (const uint32_t tile : pending) {
			if (scheduler_.should_compute_local(frame_index, tile)) {
				leftovers.push_back(tile);
			}
		}
		ok = render_tiles_software(constants, leftovers, bodies, sink, cancel_flag);
	}
	const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
	frame_local_seconds_ += seconds;
	if (ok && pixel_total > 0U) {
		local_throughput_.record(static_cast<uint32_t>(std::min<uint64_t>(pixel_total, 0xFFFFFFFFULL)), seconds);
	}
	return ok;
}

bool ClusterOrchestrator::dispatch_distributed(
	const Render::GpuCameraPushConstants& constants,
	std::span<Render::GpuPixelOutput> framebuffer,
	std::span<const Render::GpuBodyData> bodies,
	const std::atomic<bool>* cancel_flag
) {
	const FrameWindow window{framebuffer, constants.screen_width, constants.screen_height, 0U, constants.screen_height};
	return dispatch_frame(constants, window, bodies, DispatchProfile::Interactive, cancel_flag);
}

bool ClusterOrchestrator::dispatch_frame(
	const Render::GpuCameraPushConstants& constants,
	const FrameWindow& window,
	std::span<const Render::GpuBodyData> bodies,
	DispatchProfile profile,
	const std::atomic<bool>* cancel_flag
) {
	if (!is_cluster_operational()) {
		return false;
	}
	const bool offline = profile == DispatchProfile::Offline;
	if (!(offline ? route_offline_.load(std::memory_order_relaxed) : route_interactive_.load(std::memory_order_relaxed))) {
		return false;
	}
	const uint32_t width = constants.screen_width;
	const uint32_t height = constants.screen_height;
	if (width == 0U || height == 0U || width > kMaximumScreenWidth || height > kMaximumScreenHeight) {
		return false;
	}
	const uint64_t window_end = static_cast<uint64_t>(window.first_row) + window.row_count;
	if (window.width != width || window.height != height || window.row_count == 0U || window.first_row >= height || window_end > height || window.pixels.size() < static_cast<size_t>(width) * window.row_count) {
		return false;
	}
	if (bodies.size() > kMaximumCatalogBodies || registry_.schedulable_count() == 0U) {
		return false;
	}
	std::unique_lock<std::mutex> dispatch_lock(dispatch_mutex_, std::try_to_lock);
	if (!dispatch_lock.owns_lock()) {
		return false;
	}

	const auto frame_start = ClusterClock::now();
	const uint64_t frame_index = global_frame_index_.fetch_add(1, std::memory_order_relaxed);
	const uint32_t columns = tile_grid_columns(width);
	const uint32_t tile_rows = tile_grid_rows(height);
	const uint32_t total_tiles = columns * tile_rows;
	const uint32_t first_tile_row = window.first_row / kTileEdgePixels;
	const uint32_t last_tile_row = static_cast<uint32_t>((window_end - 1U) / kTileEdgePixels);

	thread_local std::vector<uint8_t> importance;
	importance.assign(total_tiles, 0U);
	for (uint32_t row = first_tile_row; row <= last_tile_row; ++row) {
		std::fill_n(importance.begin() + static_cast<std::ptrdiff_t>(row) * columns, columns, kNominalTileCost);
	}

	if (profile != applied_profile_) {
		scheduler_.configure(make_profile_parameters(profile));
		applied_profile_ = profile;
	}

	LocalTileRenderer renderer;
	{
		std::lock_guard<std::mutex> lock(renderer_mutex_);
		renderer = local_renderer_;
	}

	const SchedulerStatistics statistics_before = scheduler_.statistics();
	std::vector<uint32_t> initial_local;
	{
		std::lock_guard<std::mutex> lock(pump_mutex_);
		if (!is_cluster_operational()) {
			return false;
		}
		reclaimed_by_node_loss_.clear();
		frame_bytes_received_ = 0;
		frame_decode_seconds_ = 0.0;
		frame_local_seconds_ = 0.0;
		broadcast_bodies_if_needed(static_cast<uint32_t>(frame_index), bodies);
		if (!prepare_frame_workload(frame_index, total_tiles, importance)) {
			return false;
		}
		active_window_ = &window;
		active_frame_ = frame_index;
		const auto local_tiles = scheduler_.local_tiles();
		initial_local.assign(local_tiles.begin(), local_tiles.end());
		send_remote_tasks(frame_index, constants, bodies.empty(), frame_start, initial_local);
	}
	const double dispatch_seconds = seconds_between(frame_start, ClusterClock::now());

	bool cancelled = false;
	uint32_t reclaimed_count = 0;
	if (!run_local_batch(constants, initial_local, bodies, window, frame_index, renderer, cancel_flag)) {
		cancelled = cancel_flag != nullptr && cancel_flag->load(std::memory_order_relaxed);
	}

	const double frame_timeout = offline ? kOfflineFrameTimeoutSeconds : std::max(static_cast<double>(config_.performance.tile_timeout_ms) * 1.0e-3, 0.004);
	const auto frame_deadline = frame_start + std::chrono::duration_cast<ClusterClock::duration>(std::chrono::duration<double>(frame_timeout));

	std::vector<ReclaimedTile> reclaimed;
	std::vector<uint32_t> retry;
	while (!cancelled && !scheduler_.is_frame_complete()) {
		if ((cancel_flag != nullptr && cancel_flag->load(std::memory_order_relaxed)) || !is_cluster_operational()) {
			cancelled = true;
			break;
		}
		const auto now = ClusterClock::now();
		if (now >= frame_deadline) {
			break;
		}
		retry.clear();
		reclaimed.clear();
		{
			std::lock_guard<std::mutex> lock(pump_mutex_);
			retry.insert(retry.end(), reclaimed_by_node_loss_.begin(), reclaimed_by_node_loss_.end());
			reclaimed_by_node_loss_.clear();
		}
		scheduler_.collect_expired(now, reclaimed);
		scheduler_.steal_speculative(now, kStealBatchSize, reclaimed);
		for (const ReclaimedTile& tile : reclaimed) {
			retry.push_back(tile.tile_index);
			if (tile.reason == ReclaimReason::Timeout) {
				registry_.visit(tile.node_id, [now](ClusterNode& node) { node.register_deadline_miss(now); });
			}
		}
		reclaimed_count += static_cast<uint32_t>(retry.size());
		if (retry.empty()) {
			std::this_thread::sleep_for(kWaitInterval);
			continue;
		}
		if (!run_local_batch(constants, retry, bodies, window, frame_index, renderer, cancel_flag)) {
			cancelled = cancel_flag != nullptr && cancel_flag->load(std::memory_order_relaxed);
		}
	}

	if (!cancelled && !scheduler_.is_frame_complete()) {
		reclaimed.clear();
		retry.clear();
		scheduler_.collect_expired(ClusterClock::now() + std::chrono::hours(1), reclaimed);
		{
			std::lock_guard<std::mutex> lock(pump_mutex_);
			retry.insert(retry.end(), reclaimed_by_node_loss_.begin(), reclaimed_by_node_loss_.end());
			reclaimed_by_node_loss_.clear();
		}
		for (const ReclaimedTile& tile : reclaimed) {
			retry.push_back(tile.tile_index);
		}
		reclaimed_count += static_cast<uint32_t>(retry.size());
		if (!run_local_batch(constants, retry, bodies, window, frame_index, renderer, cancel_flag)) {
			cancelled = cancel_flag != nullptr && cancel_flag->load(std::memory_order_relaxed);
		}
	}

	if (!cancelled && !scheduler_.is_frame_complete()) {
		retry.clear();
		for (const uint32_t tile : scheduler_.active_tiles()) {
			if (scheduler_.should_compute_local(frame_index, tile)) {
				retry.push_back(tile);
			}
		}
		reclaimed_count += static_cast<uint32_t>(retry.size());
		if (!run_local_batch(constants, retry, bodies, window, frame_index, renderer, cancel_flag)) {
			cancelled = cancel_flag != nullptr && cancel_flag->load(std::memory_order_relaxed);
		}
	}

	const bool complete = !cancelled && scheduler_.is_frame_complete();
	double round_trip_mean_ms = 0.0;
	uint64_t bytes_received = 0;
	double decode_seconds = 0.0;
	double local_seconds = 0.0;
	{
		std::lock_guard<std::mutex> lock(pump_mutex_);
		active_window_ = nullptr;
		const auto release_time = ClusterClock::now();
		registry_.visit_all([release_time](ClusterNode& node) { node.release_in_flight(release_time); });
		if (!complete || reclaimed_count > 0U) {
			cancel_remote_frame(frame_index);
		}
		bytes_received = frame_bytes_received_;
		decode_seconds = frame_decode_seconds_;
		local_seconds = frame_local_seconds_;
	}
	{
		const auto snapshots = registry_.snapshots();
		double sum = 0.0;
		uint32_t counted = 0;
		for (const NodeSnapshot& snapshot : snapshots) {
			if (node_accepts_work(snapshot.state)) {
				sum += snapshot.round_trip_mean_seconds;
				++counted;
			}
		}
		round_trip_mean_ms = (counted > 0U) ? (sum / static_cast<double>(counted)) * 1.0e3 : 0.0;
	}

	const SchedulerStatistics statistics_after = scheduler_.statistics();
	ClusterFrameTelemetry telemetry{};
	telemetry.frame_index = frame_index;
	telemetry.tiles_total = static_cast<uint32_t>(scheduler_.active_tiles().size());
	telemetry.tiles_remote = static_cast<uint32_t>(statistics_after.tiles_completed_remote - statistics_before.tiles_completed_remote);
	telemetry.tiles_local = static_cast<uint32_t>(statistics_after.tiles_completed_local - statistics_before.tiles_completed_local);
	telemetry.tiles_reclaimed = reclaimed_count;
	telemetry.workers_active = static_cast<uint32_t>(registry_.schedulable_count());
	telemetry.bytes_received = bytes_received;
	telemetry.dispatch_ms = dispatch_seconds * 1.0e3;
	telemetry.local_render_ms = local_seconds * 1.0e3;
	telemetry.decode_ms = decode_seconds * 1.0e3;
	telemetry.frame_ms = seconds_between(frame_start, ClusterClock::now()) * 1.0e3;
	telemetry.round_trip_mean_ms = round_trip_mean_ms;
	telemetry.complete = complete;
	telemetry.offline = offline;
	{
		std::lock_guard<std::mutex> lock(telemetry_mutex_);
		telemetry_ = telemetry;
	}
	return complete;
}

}

#include "relativistic/distributed/cluster_orchestrator.hpp"
#include "relativistic/render/software_compute_engine.hpp"
#include "relativistic/distributed/session_protocol.hpp"
#include "relativistic/core/engine_log.hpp"
#include "relativistic/core/engine_signature.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace Relativistic::Distributed {

namespace {

[[nodiscard]] uint32_t random_identifier() noexcept {
	static std::atomic<uint32_t> seed{0x83A142EFU};
	return seed.fetch_add(0x9E3779B9U, std::memory_order_relaxed);
}

template <typename Value>
[[nodiscard]] std::span<const uint8_t> bytes_of(const Value& value) noexcept {
	return std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&value), sizeof(Value));
}

}

ClusterOrchestrator::ClusterOrchestrator(const ClusterConfiguration& configuration) noexcept
	: config_(configuration),
	  scheduler_(config_.make_load_balancer_parameters()),
	  local_abi_signature_(local_engine_abi_signature()) {}

ClusterOrchestrator::~ClusterOrchestrator() noexcept {
	shutdown();
}

bool ClusterOrchestrator::start() noexcept {
	if (!config_.cluster.enabled) {
		cluster_active_.store(false, std::memory_order_relaxed);
		return true;
	}

	TransportConfiguration transport_config = config_.make_transport_configuration();
	std::optional<TransportEndpoint> listen_endpoint;
	if (config_.cluster.listen_port != 0) {
		listen_endpoint = TransportEndpoint{config_.cluster.listen_interface, config_.cluster.listen_port};
	}
	if (!transport_.open(listen_endpoint, transport_config)) {
		Core::log_error("Failed to open network transport for master node.");
		return false;
	}

	if (config_.cluster.discovery_multicast_enabled) {
		DiscoveryConfiguration discovery_config = config_.make_discovery_configuration(
			config_.cluster.cluster_name,
			local_abi_signature_,
			1.0f
		);
		static_cast<void>(discovery_.start(discovery_config));
	}

	for (const auto& node_desc : config_.static_nodes) {
		if (!node_desc.enabled || node_desc.endpoint.host.empty()) {
			continue;
		}
		const auto node_id = registry_.register_node(node_desc, NodeState::Disconnected);
		if (node_id.has_value()) {
			const auto maybe_peer = transport_.connect(node_desc.endpoint);
			if (maybe_peer.has_value()) {
				static_cast<void>(registry_.bind_peer(*node_id, *maybe_peer, ClusterClock::now()));
				auto session = std::make_shared<MasterPeerSession>();
				session->peer_id = *maybe_peer;
				session->node_id = *node_id;
				session->identity = CryptoChannel::generate_node_identity_keys();
				session->session_identifier = random_identifier();
				session->handshaking = true;
				sessions_by_peer_[*maybe_peer] = session;
				sessions_by_node_[*node_id] = session;
				static_cast<void>(send_plain_packet(*session, PacketType::NodeAuthentication, session->identity.public_key));
			}
		}
	}

	cluster_active_.store(true, std::memory_order_release);
	return true;
}

void ClusterOrchestrator::shutdown() noexcept {
	if (!cluster_active_.exchange(false, std::memory_order_acq_rel)) {
		return;
	}
	discovery_.stop();
	for (auto& [peer, session] : sessions_by_peer_) {
		transport_.disconnect(peer, static_cast<uint32_t>(DisconnectReason::Shutdown), false);
	}
	transport_.flush();
	transport_.close();
	sessions_by_peer_.clear();
	sessions_by_node_.clear();
	registry_.clear();
}

void ClusterOrchestrator::register_worker_node(const ConnectedWorkerNode& worker) {
	std::lock_guard<std::mutex> lock(workers_mutex_);
	workers_.push_back(worker);
}

void ClusterOrchestrator::unregister_worker_node(uint32_t node_id) {
	std::lock_guard<std::mutex> lock(workers_mutex_);
	std::erase_if(workers_, [node_id](const ConnectedWorkerNode& w) noexcept {
		return w.node_identifier == node_id;
	});
	registry_.unregister_node(node_id);
}

bool ClusterOrchestrator::send_encrypted_packet(
	MasterPeerSession& session,
	PacketType type,
	uint64_t frame_index,
	std::span<const uint8_t> payload,
	TransportChannel channel
) {
	std::lock_guard<std::mutex> lock(session.send_mutex);
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

bool ClusterOrchestrator::send_plain_packet(
	MasterPeerSession& session,
	PacketType type,
	std::span<const uint8_t> payload
) {
	const PacketEnvelope envelope = make_envelope(type, session.session_identifier, 0U, 0U, payload);
	return transport_.send_gather(session.peer_id, TransportChannel::Control, bytes_of(envelope), payload);
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
				if (session->authenticated) {
					static_cast<void>(send_encrypted_packet(*session, PacketType::BodyCatalogChunk, frame_index, std::span<const uint8_t>(chunk_buffer.data(), bytes), TransportChannel::Control));
				}
			}
		}
		first += count;
	}
}

void ClusterOrchestrator::ping_heartbeats_unlocked(std::chrono::steady_clock::time_point current_time) noexcept {
	const auto interval = std::chrono::milliseconds(config_.cluster.heartbeat_interval_ms);
	for (auto& [peer, session] : sessions_by_peer_) {
		if (session->authenticated && (current_time - session->last_ping >= interval)) {
			session->last_ping = current_time;
			session->last_ping_timestamp = std::chrono::duration<double>(current_time.time_since_epoch()).count();
			double ts = session->last_ping_timestamp;
			static_cast<void>(send_encrypted_packet(*session, PacketType::HeartbeatPing, 0U, std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&ts), sizeof(ts)), TransportChannel::Control));
		}
	}
}

void ClusterOrchestrator::detect_timeouts_and_reclaim_tasks(std::chrono::steady_clock::time_point current_time) noexcept {
	std::vector<uint32_t> expired_nodes;
	registry_.enforce_timeouts(current_time, config_.cluster.node_timeout_ms, expired_nodes);
	std::vector<ReclaimedTile> reclaimed;
	for (uint32_t node_id : expired_nodes) {
		scheduler_.reclaim_node(node_id, reclaimed);
		auto it = sessions_by_node_.find(node_id);
		if (it != sessions_by_node_.end()) {
			transport_.disconnect(it->second->peer_id, static_cast<uint32_t>(DisconnectReason::HandshakeTimeout), true);
			sessions_by_peer_.erase(it->second->peer_id);
			sessions_by_node_.erase(it);
		}
	}
	scheduler_.collect_expired(current_time, reclaimed);
}

void ClusterOrchestrator::process_network_events(
	uint64_t current_frame_index,
	std::span<Render::GpuPixelOutput> destination_framebuffer,
	size_t screen_width,
	size_t screen_height
) noexcept {
	TransportEvent event{};
	thread_local std::vector<uint8_t> receive_buffer(kMaximumTransportPacketBytes);
	thread_local std::vector<uint8_t> plain_buffer(kMaximumTransportPacketBytes);
	thread_local std::array<Render::GpuPixelOutput, kTilePixelCapacity> tile_pixels;

	while (transport_.poll(event, receive_buffer, 0U)) {
		if (event.type == TransportEventType::Connected) {
			auto it = sessions_by_peer_.find(event.peer_id);
			if (it == sessions_by_peer_.end()) {
				auto session = std::make_shared<MasterPeerSession>();
				session->peer_id = event.peer_id;
				session->identity = CryptoChannel::generate_node_identity_keys();
				session->session_identifier = random_identifier();
				session->handshaking = true;
				sessions_by_peer_[event.peer_id] = session;
				static_cast<void>(send_plain_packet(*session, PacketType::NodeAuthentication, session->identity.public_key));
			}
		} else if (event.type == TransportEventType::Disconnected) {
			auto it = sessions_by_peer_.find(event.peer_id);
			if (it != sessions_by_peer_.end()) {
				static_cast<void>(registry_.release_peer(event.peer_id, ClusterClock::now()));
				sessions_by_node_.erase(it->second->node_id);
				sessions_by_peer_.erase(it);
			}
		} else if (event.type == TransportEventType::Received) {
			auto it = sessions_by_peer_.find(event.peer_id);
			if (it == sessions_by_peer_.end() || event.payload_bytes < sizeof(PacketEnvelope)) {
				continue;
			}
			MasterPeerSession& session = *it->second;
			PacketEnvelope envelope{};
			std::memcpy(&envelope, receive_buffer.data(), sizeof(PacketEnvelope));
			if (!envelope.is_valid_magic()) {
				continue;
			}
			const size_t payload_bytes = event.payload_bytes - sizeof(PacketEnvelope);
			const std::span<const uint8_t> cipher(receive_buffer.data() + sizeof(PacketEnvelope), payload_bytes);
			const std::span<uint8_t> plain(plain_buffer.data(), payload_bytes);

			if (session.handshaking && !session.authenticated) {
				if (envelope.type == PacketType::NodeAuthChallenge && payload_bytes == 32) {
					std::span<const uint8_t, 32> worker_pub(cipher.data(), 32);
					if (session.security_channel.perform_handshake_client(worker_pub, session.identity.private_key, config_.security.cluster_shared_token)) {
						session.authenticated = true;
						session.handshaking = false;
						AuthenticationProofPayload proof{};
						proof.role = static_cast<uint32_t>(ClusterNodeRole::Master);
						proof.instance_identifier = random_identifier();
						static_cast<void>(send_encrypted_packet(session, PacketType::NodeAuthResponse, 0U, bytes_of(proof), TransportChannel::Control));

						NodeHardwareCapabilities caps{};
						caps.engine_abi_signature = local_abi_signature_;
						caps.logical_cores = std::thread::hardware_concurrency();
						static_cast<void>(send_encrypted_packet(session, PacketType::CapabilitiesAdvertisement, 0U, bytes_of(caps), TransportChannel::Control));
					}
				} else if (envelope.type == PacketType::NodeAuthentication && payload_bytes == 32) {
					std::span<const uint8_t, 32> worker_pub(cipher.data(), 32);
					if (session.security_channel.perform_handshake_server(worker_pub, session.identity.private_key, config_.security.cluster_shared_token)) {
						session.authenticated = true;
						session.handshaking = false;
						static_cast<void>(send_plain_packet(session, PacketType::NodeAuthChallenge, session.identity.public_key));

						NodeHardwareCapabilities caps{};
						caps.engine_abi_signature = local_abi_signature_;
						caps.logical_cores = std::thread::hardware_concurrency();
						static_cast<void>(send_encrypted_packet(session, PacketType::CapabilitiesAdvertisement, 0U, bytes_of(caps), TransportChannel::Control));
					}
				}
			} else if (session.authenticated) {
				if (!session.security_channel.verify_and_decrypt_packet(envelope.authenticated_header(), envelope.packet_sequence, cipher, envelope.authentication_tag, plain)) {
					continue;
				}
				if (envelope.type == PacketType::HeartbeatPong) {
					const auto now = ClusterClock::now();
					const double now_ts = std::chrono::duration<double>(now.time_since_epoch()).count();
					const double rtt = now_ts - session.last_ping_timestamp;
					registry_.visit(session.node_id, [rtt, now](ClusterNode& node) noexcept {
						node.record_heartbeat(now);
						node.record_round_trip(rtt);
					});
				} else if (envelope.type == PacketType::CapabilitiesAdvertisement) {
					if (plain.size() >= sizeof(NodeHardwareCapabilities)) {
						NodeHardwareCapabilities remote_caps{};
						std::memcpy(&remote_caps, plain.data(), sizeof(remote_caps));
						const auto now = ClusterClock::now();
						if (session.node_id == 0) {
							ClusterNodeDescriptor desc{};
							desc.name = "worker-" + std::to_string(session.peer_id);
							desc.endpoint.host = transport_.peer_address(session.peer_id);
							desc.static_weight = static_cast<double>(remote_caps.relative_performance_weight);
							desc.enabled = true;
							const auto maybe_id = registry_.register_node(desc, NodeState::Unauthenticated, now);
							if (maybe_id.has_value()) {
								session.node_id = *maybe_id;
								sessions_by_node_[*maybe_id] = it->second;
								static_cast<void>(registry_.bind_peer(*maybe_id, session.peer_id, now));
							}
						}
						registry_.visit(session.node_id, [remote_caps, now](ClusterNode& node) noexcept {
							NodeAdvertisement adv{};
							adv.hardware = remote_caps;
							adv.role = ClusterNodeRole::Worker;
							NodeHardwareCapabilities local_caps{};
							local_caps.engine_abi_signature = local_engine_abi_signature();
							if (node.state() == NodeState::Unauthenticated) {
								static_cast<void>(node.begin_handshake(now));
							}
							static_cast<void>(node.accept_advertisement(adv, local_caps, false, now));
							static_cast<void>(node.transition_to(NodeState::Synchronized, now));
							static_cast<void>(node.transition_to(NodeState::Ready, now));
						});
					}
				} else if (envelope.type == PacketType::TileResultData) {
					TileCodecScratch& scratch = TileCodec::thread_scratch();
					const DecodedTileResult decoded = decode_render_tile_result(plain, tile_pixels, scratch);
					if (decoded.status == TileDecodeStatus::Ok) {
						const uint32_t tile_idx = decoded.header.tile_index;
						const auto result = scheduler_.complete_remote(envelope.frame_index, tile_idx, session.node_id);
						if (result == TileCompletionResult::Accepted && envelope.frame_index == current_frame_index) {
							const auto geometry = resolve_tile_geometry(tile_idx, static_cast<uint32_t>(screen_width), static_cast<uint32_t>(screen_height));
							if (geometry.has_value()) {
								static_cast<void>(store_tile_pixels(*geometry, tile_pixels, destination_framebuffer, static_cast<uint32_t>(screen_width), static_cast<uint32_t>(screen_height)));
							}
						}
					}
				}
			}
		}
	}
}

void ClusterOrchestrator::prepare_frame_workload(
	uint64_t frame_index,
	uint32_t total_tiles,
	std::span<const uint8_t> tile_importance_mask
) {
	std::vector<NodeCapacityInput> capacities;
	registry_.collect_capacity_inputs(capacities);
	static_cast<void>(scheduler_.begin_frame(frame_index, total_tiles, tile_importance_mask, capacities));
}

size_t ClusterOrchestrator::fetch_remote_tile_results(
	uint64_t current_frame_index,
	std::span<Render::GpuPixelOutput> destination_framebuffer,
	size_t screen_width,
	size_t screen_height
) noexcept {
	const auto now = ClusterClock::now();
	process_network_events(current_frame_index, destination_framebuffer, screen_width, screen_height);
	ping_heartbeats_unlocked(now);
	detect_timeouts_and_reclaim_tasks(now);
	return destination_framebuffer.size();
}

bool ClusterOrchestrator::dispatch_distributed(
	const Render::GpuCameraPushConstants& constants,
	std::span<Render::GpuPixelOutput> framebuffer,
	std::span<const Render::GpuBodyData> bodies,
	const std::atomic<bool>* cancel_flag
) {
	if (!is_cluster_operational() || !config_.task_routing.route_interactive_viewport) {
		return false;
	}

	const uint32_t width = constants.screen_width;
	const uint32_t height = constants.screen_height;
	const uint32_t total_tiles = tile_grid_columns(width) * tile_grid_rows(height);
	const uint64_t frame_index = global_frame_index_.fetch_add(1, std::memory_order_relaxed);

	broadcast_bodies_if_needed(static_cast<uint32_t>(frame_index), bodies);

	thread_local std::vector<uint8_t> importance_mask;
	importance_mask.assign(total_tiles, kNominalTileCost);

	prepare_frame_workload(frame_index, total_tiles, importance_mask);

	const auto now = ClusterClock::now();
	const size_t slot_count = scheduler_.slot_count();

	thread_local std::vector<uint8_t> task_buffer(kRenderTileTaskPayloadBytes);

	for (size_t s = 0; s < slot_count; ++s) {
		const SlotAllocation slot = scheduler_.slot_allocation(s);
		if (slot.is_local) {
			continue;
		}
		auto session_it = sessions_by_node_.find(slot.node_id);
		if (session_it == sessions_by_node_.end() || !session_it->second->authenticated) {
			continue;
		}
		MasterPeerSession& session = *session_it->second;
		const auto tiles = scheduler_.slot_tiles(s);
		for (uint32_t tile_idx : tiles) {
			RenderTileTaskPayload task{};
			const auto hdr = make_render_tile_task_header(tile_idx, width, height, 0, 0, static_cast<TileCompressionMode>(config_.performance.compression_mode));
			if (!hdr.has_value()) {
				continue;
			}
			task.header = *hdr;
			task.dispatch_timestamp = std::chrono::duration<double>(now.time_since_epoch()).count();
			task.camera_constants = constants;

			const size_t task_bytes = serialize_render_tile_task(task, task_buffer);
			if (task_bytes > 0) {
				static_cast<void>(send_encrypted_packet(session, PacketType::TaskAssignment, frame_index, std::span<const uint8_t>(task_buffer.data(), task_bytes), TransportChannel::TileData));
				DispatchTiming timing{};
				timing.deadline_margin_seconds = static_cast<double>(config_.performance.tile_timeout_ms) * 1.0e-3;
				timing.seconds_per_kilo_ray = kDefaultSecondsPerKiloRay;
				static_cast<void>(scheduler_.mark_dispatched(tile_idx, slot.node_id, now, kTilePixelCapacity, timing));
			}
		}
	}

	const auto local_tiles = scheduler_.local_tiles();
	for (uint32_t tile_idx : local_tiles) {
		if (cancel_flag != nullptr && cancel_flag->load(std::memory_order_relaxed)) {
			return false;
		}
		const auto geom = resolve_tile_geometry(tile_idx, width, height);
		if (geom.has_value() && scheduler_.should_compute_local(frame_index, tile_idx)) {
			const Render::SoftwareComputeEngine::TileRegion region{geom->origin_x, geom->origin_y, geom->width, geom->height};
			std::array<Render::GpuPixelOutput, kTilePixelCapacity> local_pix{};
			if (Render::SoftwareComputeEngine::dispatch_tile(constants, region, local_pix, bodies, cancel_flag)) {
				static_cast<void>(store_tile_pixels(*geom, local_pix, framebuffer, width, height));
				static_cast<void>(scheduler_.try_complete_local(frame_index, tile_idx));
			}
		}
	}

	const auto deadline = now + std::chrono::milliseconds(config_.performance.tile_timeout_ms);
	while (!scheduler_.is_frame_complete() && ClusterClock::now() < deadline) {
		if (cancel_flag != nullptr && cancel_flag->load(std::memory_order_relaxed)) {
			return false;
		}
		static_cast<void>(fetch_remote_tile_results(frame_index, framebuffer, width, height));

		std::vector<ReclaimedTile> stolen;
		static_cast<void>(scheduler_.steal_speculative(ClusterClock::now(), 4, stolen));
		for (const auto& stolen_tile : stolen) {
			const auto geom = resolve_tile_geometry(stolen_tile.tile_index, width, height);
			if (geom.has_value() && scheduler_.should_compute_local(frame_index, stolen_tile.tile_index)) {
				const Render::SoftwareComputeEngine::TileRegion region{geom->origin_x, geom->origin_y, geom->width, geom->height};
				std::array<Render::GpuPixelOutput, kTilePixelCapacity> stolen_pix{};
				if (Render::SoftwareComputeEngine::dispatch_tile(constants, region, stolen_pix, bodies, cancel_flag)) {
					static_cast<void>(store_tile_pixels(*geom, stolen_pix, framebuffer, width, height));
					static_cast<void>(scheduler_.try_complete_local(frame_index, stolen_tile.tile_index));
				}
			}
		}

		if (scheduler_.is_frame_complete()) {
			break;
		}
		std::this_thread::sleep_for(std::chrono::microseconds(500));
	}

	if (!scheduler_.is_frame_complete()) {
		std::vector<ReclaimedTile> urgent;
		static_cast<void>(scheduler_.collect_expired(ClusterClock::now() + std::chrono::hours(1), urgent));
		for (const auto& urgent_tile : urgent) {
			if (cancel_flag != nullptr && cancel_flag->load(std::memory_order_relaxed)) {
				return false;
			}
			const auto geom = resolve_tile_geometry(urgent_tile.tile_index, width, height);
			if (geom.has_value() && scheduler_.should_compute_local(frame_index, urgent_tile.tile_index)) {
				const Render::SoftwareComputeEngine::TileRegion region{geom->origin_x, geom->origin_y, geom->width, geom->height};
				std::array<Render::GpuPixelOutput, kTilePixelCapacity> urgent_pix{};
				if (Render::SoftwareComputeEngine::dispatch_tile(constants, region, urgent_pix, bodies, cancel_flag)) {
					static_cast<void>(store_tile_pixels(*geom, urgent_pix, framebuffer, width, height));
					static_cast<void>(scheduler_.try_complete_local(frame_index, urgent_tile.tile_index));
				}
			}
		}
		for (uint32_t tile_idx : scheduler_.active_tiles()) {
			if (!scheduler_.is_frame_complete() && scheduler_.should_compute_local(frame_index, tile_idx)) {
				if (cancel_flag != nullptr && cancel_flag->load(std::memory_order_relaxed)) {
					return false;
				}
				const auto geom = resolve_tile_geometry(tile_idx, width, height);
				if (geom.has_value()) {
					const Render::SoftwareComputeEngine::TileRegion region{geom->origin_x, geom->origin_y, geom->width, geom->height};
					std::array<Render::GpuPixelOutput, kTilePixelCapacity> fallback_pix{};
					if (Render::SoftwareComputeEngine::dispatch_tile(constants, region, fallback_pix, bodies, cancel_flag)) {
						static_cast<void>(store_tile_pixels(*geom, fallback_pix, framebuffer, width, height));
						static_cast<void>(scheduler_.try_complete_local(frame_index, tile_idx));
					}
				}
			}
		}
	}

	return true;
}

}

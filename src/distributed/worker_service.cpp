#include "relativistic/distributed/worker_service.hpp"
#include "relativistic/core/engine_log.hpp"
#include "relativistic/core/pcg64.hpp"
#include "relativistic/core/thread_pool.hpp"
#include "relativistic/distributed/body_catalog.hpp"
#include "relativistic/distributed/cluster_node.hpp"
#include "relativistic/distributed/compression_codecs.hpp"
#include "relativistic/distributed/crypto_channel.hpp"
#include "relativistic/distributed/discovery_service.hpp"
#include "relativistic/distributed/network_transport.hpp"
#include "relativistic/distributed/packet_protocol.hpp"
#include "relativistic/distributed/session_protocol.hpp"
#include "relativistic/distributed/task_payloads.hpp"
#include "relativistic/distributed/tile_renderer.hpp"
#include "relativistic/render/bodies/body_render_guard.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <random>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace Relativistic::Distributed {

namespace {

constexpr size_t kMaximumQueuedTiles = 16384;
constexpr size_t kMaximumSessions = 32;
constexpr uint32_t kMaximumIntegrityFailures = 32;
constexpr uint32_t kPollSliceMs = 10;
constexpr size_t kMaximumEventBurst = 256;
constexpr uint32_t kMaximumIntegrationSteps = 1U << 20U;
constexpr uint32_t kClockConvergenceReplies = 4;
constexpr auto kWaitingJobLifetime = std::chrono::milliseconds(500);

enum class SessionPhase : uint8_t {
	ServerAwaitAuthentication,
	ServerAwaitProof,
	ClientAwaitChallenge,
	ClientAwaitAcknowledgement,
	Established,
	Closing
};

using JobKey = std::tuple<uint32_t, uint32_t, uint32_t>;
using BodyListPtr = std::shared_ptr<const std::vector<Render::GpuBodyData>>;

struct PeerSession;

struct TileJob {
	std::shared_ptr<PeerSession> session{};
	RenderTileTaskPayload payload{};
	uint32_t frame_index{0};
	BodyListPtr bodies{};
	std::atomic<bool> cancel{false};
	std::atomic<bool> released{false};
};

struct WaitingJob {
	std::shared_ptr<TileJob> job{};
	ClusterTimePoint deadline{};
};

struct PeerSession {
	PeerSession(uint32_t id, bool initiator, SessionPhase initial, ClusterTimePoint now)
		: peer_id(id), initiated(initiator), phase(initial), created_at(now), last_heartbeat_sent(now) {}

	const uint32_t peer_id;
	const bool initiated;
	SessionPhase phase;
	CryptoKeyPair identity{};
	CryptoChannel channel{};
	uint32_t session_identifier{0};
	std::mutex send_mutex{};
	uint32_t transmit_sequence{0};
	std::atomic<bool> open{true};
	std::atomic<bool> rekey_required{false};
	std::atomic<uint32_t> in_flight{0};
	bool capabilities_accepted{false};
	uint32_t clock_replies{0};
	uint32_t latest_frame{0};
	bool has_frame{false};
	uint32_t integrity_failures{0};
	ClusterTimePoint created_at;
	ClusterTimePoint established_at{};
	ClusterTimePoint last_heartbeat_sent;
	BodyCatalogStore catalogs{};
	std::vector<WaitingJob> waiting{};
};

template <typename Value>
[[nodiscard]] std::span<const uint8_t> bytes_of(const Value& value) noexcept {
	return std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&value), sizeof(Value));
}

[[nodiscard]] uint64_t entropy_seed() {
	std::random_device device;
	const uint64_t high = device();
	const uint64_t low = device();
	const auto ticks = static_cast<uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count());
	return (high << 32U) ^ low ^ (ticks * 0x9E3779B97F4A7C15ULL);
}

[[nodiscard]] uint32_t random_session_identifier() {
	const auto value = static_cast<uint32_t>(entropy_seed() >> 16U);
	return (value != 0U) ? value : 1U;
}

[[nodiscard]] std::string default_node_name() {
	for (const char* variable : {"HOSTNAME", "COMPUTERNAME"}) {
		const char* value = std::getenv(variable);
		if (value != nullptr && value[0] != '\0') {
			return value;
		}
	}
	return "cluster-worker";
}

}

struct ClusterWorkerService::Impl {
	WorkerOptions options;
	std::string node_name{};
	uint64_t instance_identifier{0};
	ClusterTimePoint epoch{ClusterClock::now()};
	NetworkTransport transport{};
	DiscoveryService discovery{};
	std::unique_ptr<TileRenderer> renderer{};
	std::unique_ptr<Core::ThreadPool> pool{};
	NodeHardwareCapabilities hardware{};
	std::atomic<bool> stop{false};
	const std::atomic<bool>* external_stop{nullptr};
	std::string error{};
	mutable std::mutex error_mutex{};
	std::unordered_map<uint32_t, std::shared_ptr<PeerSession>> sessions{};
	uint32_t outbound_peer{kInvalidPeerId};
	ClusterTimePoint next_connect_attempt{};
	std::vector<uint8_t> plain_buffer{};
	std::mutex jobs_mutex{};
	std::map<JobKey, std::shared_ptr<TileJob>> jobs{};
	std::atomic<uint64_t> sessions_accepted{0};
	std::atomic<uint64_t> tiles_received{0};
	std::atomic<uint64_t> tiles_completed{0};
	std::atomic<uint64_t> tiles_cancelled{0};
	std::atomic<uint64_t> tiles_rejected{0};
	std::atomic<uint64_t> tiles_expired{0};
	std::atomic<uint64_t> tiles_on_gpu{0};
	std::atomic<uint64_t> tiles_on_cpu{0};
	std::atomic<uint64_t> packets_rejected{0};
	std::atomic<uint64_t> catalogs_received{0};
	bool started{false};

	explicit Impl(const WorkerOptions& configured) : options(configured) {}

	~Impl() {
		shutdown();
	}

	void set_error(const std::string& message) {
		std::lock_guard<std::mutex> lock(error_mutex);
		error = message;
	}

	[[nodiscard]] double seconds_since_epoch(ClusterTimePoint point) const noexcept {
		return seconds_between(epoch, point);
	}

	[[nodiscard]] bool stop_requested() const noexcept {
		return stop.load(std::memory_order_relaxed) || (external_stop != nullptr && external_stop->load(std::memory_order_relaxed));
	}

	[[nodiscard]] bool initialize() {
		node_name = options.node_name.empty() ? default_node_name() : options.node_name;
		instance_identifier = entropy_seed();

		TileRendererConfiguration tile_configuration{};
		tile_configuration.backend = options.backend;
		tile_configuration.vulkan_device = options.vulkan_device;
		tile_configuration.embedded_shader = options.embedded_tile_shader;
		renderer = std::make_unique<TileRenderer>(tile_configuration);
		if (!renderer->initialize()) {
			set_error("The tile renderer could not be initialized");
			return false;
		}
		if (options.backend == WorkerBackend::Gpu && !renderer->gpu_available()) {
			Core::log_warning("The GPU backend was requested but no usable device exists; unsupported tiles fall back to the CPU.");
		}

		hardware = detect_local_hardware_capabilities(renderer->gpu_info());
		if (options.performance_weight > 0.0) {
			hardware.relative_performance_weight = static_cast<float>(options.performance_weight);
		}

		const size_t threads = (options.thread_count > 0U) ? options.thread_count : std::max<size_t>(1U, std::thread::hardware_concurrency());
		pool = std::make_unique<Core::ThreadPool>(threads);
		plain_buffer.assign(kMaximumTransportPacketBytes, 0U);

		TransportConfiguration transport_configuration{};
		transport_configuration.maximum_peers = kMaximumSessions;
		std::optional<TransportEndpoint> listen_endpoint;
		if (options.listen_enabled) {
			listen_endpoint = TransportEndpoint{options.listen_interface, options.listen_port};
		}
		if (!transport.open(listen_endpoint, transport_configuration)) {
			set_error("The network transport could not be opened");
			return false;
		}

		if (options.listen_enabled && options.discovery_enabled) {
			DiscoveryConfiguration discovery_configuration{};
			discovery_configuration.multicast_group = options.discovery_group;
			discovery_configuration.port = options.discovery_port;
			discovery_configuration.interface_address = options.discovery_interface;
			discovery_configuration.role = ClusterNodeRole::Worker;
			discovery_configuration.service_port = options.listen_port;
			discovery_configuration.node_name = node_name;
			discovery_configuration.cluster_name = options.cluster_name;
			discovery_configuration.cluster_token = options.auth_token;
			discovery_configuration.abi_signature = hardware.engine_abi_signature;
			discovery_configuration.performance_weight = hardware.relative_performance_weight;
			if (!discovery.start(discovery_configuration)) {
				Core::log_warning("Discovery announcements are disabled: " + discovery.last_error());
			}
		}

		next_connect_attempt = ClusterClock::now();
		started = true;
		Core::log_info("Cluster worker '" + node_name + "' ready: " + std::to_string(threads) + " threads, GPU " + renderer->gpu_description() + ", weight " + std::to_string(static_cast<double>(hardware.relative_performance_weight)) + (options.listen_enabled ? ", listening on port " + std::to_string(options.listen_port) : std::string(", outbound only")));
		return true;
	}

	void shutdown() {
		if (!started && pool == nullptr) {
			return;
		}
		started = false;
		stop.store(true, std::memory_order_relaxed);
		discovery.stop();
		{
			std::lock_guard<std::mutex> lock(jobs_mutex);
			for (auto& entry : jobs) {
				entry.second->cancel.store(true, std::memory_order_relaxed);
			}
		}
		for (auto& [peer, session] : sessions) {
			session->open.store(false, std::memory_order_relaxed);
			transport.disconnect(peer, static_cast<uint32_t>(DisconnectReason::Shutdown), true);
		}
		transport.flush();
		if (pool != nullptr) {
			pool->shutdown();
			pool.reset();
		}
		transport.close();
		sessions.clear();
		jobs.clear();
		renderer.reset();
	}

	[[nodiscard]] bool send_plain(PeerSession& session, PacketType type, std::span<const uint8_t> payload) {
		const PacketEnvelope envelope = make_envelope(type, session.session_identifier, 0U, 0U, payload);
		return transport.send_gather(session.peer_id, TransportChannel::Control, bytes_of(envelope), payload);
	}

	[[nodiscard]] bool send_encrypted(PeerSession& session, PacketType type, uint64_t frame_index, std::span<const uint8_t> plaintext, TransportChannel channel) {
		if (!session.open.load(std::memory_order_acquire)) {
			return false;
		}
		std::lock_guard<std::mutex> lock(session.send_mutex);
		if (session.transmit_sequence >= kSessionRekeyPacketLimit) {
			session.rekey_required.store(true, std::memory_order_release);
			return false;
		}
		thread_local std::vector<uint8_t> ciphertext;
		if (ciphertext.size() < plaintext.size()) {
			ciphertext.resize(plaintext.size());
		}
		PacketEnvelope envelope = make_envelope(type, session.session_identifier, session.transmit_sequence, frame_index, plaintext);
		const std::span<uint8_t, 16> tag(envelope.authentication_tag);
		const std::span<uint8_t> cipher_view(ciphertext.data(), plaintext.size());
		if (!session.channel.encrypt_and_authenticate_packet(envelope.authenticated_header(), plaintext, cipher_view, tag)) {
			return false;
		}
		++session.transmit_sequence;
		return transport.send_gather(session.peer_id, channel, bytes_of(envelope), std::span<const uint8_t>(ciphertext.data(), plaintext.size()));
	}

	void close_session(PeerSession& session, DisconnectReason reason) {
		if (session.phase == SessionPhase::Closing) {
			return;
		}
		session.phase = SessionPhase::Closing;
		session.open.store(false, std::memory_order_release);
		cancel_jobs(session.peer_id, TaskCancelScope::UpToFrame, std::numeric_limits<uint32_t>::max(), 0U);
		release_waiting(session);
		transport.disconnect(session.peer_id, static_cast<uint32_t>(reason), true);
	}

	size_t cancel_jobs(uint32_t peer, TaskCancelScope scope, uint32_t frame, uint32_t tile) {
		JobKey first{peer, frame, tile};
		JobKey last{peer, frame, tile};
		if (scope == TaskCancelScope::WholeFrame) {
			first = JobKey{peer, frame, 0U};
			last = JobKey{peer, frame, std::numeric_limits<uint32_t>::max()};
		} else if (scope == TaskCancelScope::UpToFrame) {
			first = JobKey{peer, 0U, 0U};
			last = JobKey{peer, frame, std::numeric_limits<uint32_t>::max()};
		}
		size_t count = 0;
		std::lock_guard<std::mutex> lock(jobs_mutex);
		for (auto it = jobs.lower_bound(first); it != jobs.end() && it->first <= last; ++it) {
			it->second->cancel.store(true, std::memory_order_relaxed);
			++count;
		}
		return count;
	}

	void release_job(const std::shared_ptr<TileJob>& job) {
		if (job->released.exchange(true, std::memory_order_acq_rel)) {
			return;
		}
		{
			std::lock_guard<std::mutex> lock(jobs_mutex);
			jobs.erase(JobKey{job->session->peer_id, job->frame_index, job->payload.header.tile_index});
		}
		uint32_t current = job->session->in_flight.load(std::memory_order_relaxed);
		while (current > 0U && !job->session->in_flight.compare_exchange_weak(current, current - 1U, std::memory_order_relaxed)) {
		}
	}

	void release_waiting(PeerSession& session) {
		for (WaitingJob& waiting : session.waiting) {
			release_job(waiting.job);
		}
		session.waiting.clear();
	}

	void prune_cancelled_waiting(PeerSession& session) {
		auto it = std::remove_if(session.waiting.begin(), session.waiting.end(), [this](const WaitingJob& waiting) {
			if (!waiting.job->cancel.load(std::memory_order_relaxed)) {
				return false;
			}
			tiles_cancelled.fetch_add(1, std::memory_order_relaxed);
			release_job(waiting.job);
			return true;
		});
		session.waiting.erase(it, session.waiting.end());
	}

	void launch_job(const std::shared_ptr<TileJob>& job) {
		if (pool == nullptr) {
			release_job(job);
			return;
		}
		pool->enqueue([this, job]() {
			execute_job(job);
		});
	}

	void execute_job(const std::shared_ptr<TileJob>& job) {
		if (job->cancel.load(std::memory_order_relaxed) || !job->session->open.load(std::memory_order_acquire)) {
			tiles_cancelled.fetch_add(1, std::memory_order_relaxed);
			release_job(job);
			return;
		}

		thread_local std::array<Render::GpuPixelOutput, kTilePixelCapacity> pixels;
		thread_local std::vector<uint8_t> encoded;

		const std::span<const Render::GpuBodyData> bodies = (job->bodies != nullptr)
			? std::span<const Render::GpuBodyData>(*job->bodies)
			: std::span<const Render::GpuBodyData>{};
		const TileRenderOutcome outcome = renderer->render(job->payload, bodies, std::span<Render::GpuPixelOutput>(pixels), &job->cancel);
		
		if (outcome.cancelled) {
			tiles_cancelled.fetch_add(1, std::memory_order_relaxed);
			release_job(job);
			return;
		}
		if (!outcome.completed) {
			tiles_rejected.fetch_add(1, std::memory_order_relaxed);
			release_job(job);
			return;
		}

		const auto mode = static_cast<TileCompressionMode>(job->payload.header.compression_type);
		const size_t capacity = sizeof(RenderTileResultPayload) + TileCodec::max_compressed_size(mode, outcome.pixel_count);

		if (encoded.size() < capacity) {
			encoded.resize(capacity);
		}

		TileCodecScratch& scratch = TileCodec::thread_scratch();
		CompressionParameters comp_params{};
		comp_params.lz4_acceleration = 1;

		const EncodedTileResult encoded_result = encode_render_tile_result(
			job->frame_index,
			job->payload.header.tile_index,
			std::span<const Render::GpuPixelOutput>(pixels.data(), outcome.pixel_count),
			mode,
			outcome.execution_time_us,
			encoded,
			scratch,
			comp_params
		);

		if (encoded_result.status != CodecStatus::Ok) {
			tiles_rejected.fetch_add(1, std::memory_order_relaxed);
			release_job(job);
			return;
		}

		if (send_encrypted(*job->session, PacketType::TileResultData, job->frame_index, std::span<const uint8_t>(encoded.data(), encoded_result.bytes), TransportChannel::TileData)) {
			tiles_completed.fetch_add(1, std::memory_order_relaxed);
			if (outcome.used_gpu) {
				tiles_on_gpu.fetch_add(1, std::memory_order_relaxed);
			} else {
				tiles_on_cpu.fetch_add(1, std::memory_order_relaxed);
			}
		}
		release_job(job);
	}

	void on_peer_connected(uint32_t peer_id) {
		const bool is_outbound = (peer_id == outbound_peer);
		auto session = std::make_shared<PeerSession>(peer_id, is_outbound, is_outbound ? SessionPhase::ClientAwaitChallenge : SessionPhase::ServerAwaitAuthentication, ClusterClock::now());
		session->identity = CryptoChannel::generate_node_identity_keys();
		if (is_outbound) {
			outbound_peer = kInvalidPeerId;
		}
		sessions.emplace(peer_id, session);
	}

	void on_peer_disconnected(uint32_t peer_id) {
		auto it = sessions.find(peer_id);
		if (it != sessions.end()) {
			close_session(*it->second, DisconnectReason::Shutdown);
			sessions.erase(it);
		}
	}

	void handle_control_packet(PeerSession& session, PacketType type, uint64_t frame_index, std::span<const uint8_t> payload, ClusterTimePoint now) {
		switch (type) {
			case PacketType::HeartbeatPing: {
				static_cast<void>(send_encrypted(session, PacketType::HeartbeatPong, frame_index, payload, TransportChannel::Control));
				break;
			}
			case PacketType::HeartbeatPong: {
				break;
			}
			case PacketType::SynchronizeClock: {
				if (payload.size() >= sizeof(double)) {
					double request_time = 0.0;
					std::memcpy(&request_time, payload.data(), sizeof(double));
					double times[2] = {request_time, seconds_since_epoch(now)};
					static_cast<void>(send_encrypted(session, PacketType::SynchronizeClock, frame_index, bytes_of(times), TransportChannel::Control));
				}
				break;
			}
			case PacketType::CapabilitiesAdvertisement: {
				if (payload.size() >= sizeof(NodeHardwareCapabilities)) {
					NodeHardwareCapabilities master_caps{};
					std::memcpy(&master_caps, payload.data(), sizeof(NodeHardwareCapabilities));
					if (master_caps.engine_abi_signature != hardware.engine_abi_signature) {
						close_session(session, DisconnectReason::AbiMismatch);
					} else {
						session.capabilities_accepted = true;
						static_cast<void>(send_encrypted(session, PacketType::CapabilitiesAdvertisement, 0U, bytes_of(hardware), TransportChannel::Control));
					}
				}
				break;
			}
			case PacketType::TaskCancel: {
				if (payload.size() == sizeof(TaskCancelPayload)) {
					TaskCancelPayload cancel{};
					std::memcpy(&cancel, payload.data(), sizeof(TaskCancelPayload));
					cancel_jobs(session.peer_id, static_cast<TaskCancelScope>(cancel.scope), cancel.frame_index, cancel.tile_index);
				}
				break;
			}
			case PacketType::BodyCatalogChunk: {
				uint32_t completed_frame = 0;
				std::vector<Render::GpuBodyData> completed_bodies;
				const auto result = session.catalogs.ingest(payload, completed_frame, completed_bodies);
				if (result == BodyCatalogStore::IngestResult::Invalid) {
					close_session(session, DisconnectReason::ProtocolViolation);
				} else if (result == BodyCatalogStore::IngestResult::Completed) {
					auto ptr = std::make_shared<const std::vector<Render::GpuBodyData>>(std::move(completed_bodies));
					session.catalogs.publish(completed_frame, std::move(ptr));
					catalogs_received.fetch_add(1, std::memory_order_relaxed);
					
					prune_cancelled_waiting(session);
					auto& waiting = session.waiting;
					for (auto it = waiting.begin(); it != waiting.end();) {
						if (it->job->frame_index <= completed_frame || (it->job->payload.header.flags & kTileTaskFlagReuseLatestBodies) != 0U) {
							it->job->bodies = session.catalogs.find(it->job->frame_index);
							if (it->job->bodies == nullptr) {
								it->job->bodies = session.catalogs.latest();
							}
							launch_job(it->job);
							it = waiting.erase(it);
						} else {
							++it;
						}
					}
				}
				break;
			}
			default:
				break;
		}
	}

	void handle_data_packet(PeerSession& session, PacketType type, uint64_t frame_index, std::span<const uint8_t> payload, ClusterTimePoint now) {
		if (type == PacketType::TaskAssignment) {
			auto job = std::make_shared<TileJob>();
			job->session = sessions[session.peer_id];
			job->frame_index = static_cast<uint32_t>(frame_index);
			if (!deserialize_render_tile_task(payload, job->payload)) {
				tiles_rejected.fetch_add(1, std::memory_order_relaxed);
				return;
			}
			if (session.latest_frame < frame_index || !session.has_frame) {
				session.latest_frame = static_cast<uint32_t>(frame_index);
				session.has_frame = true;
				cancel_jobs(session.peer_id, TaskCancelScope::UpToFrame, session.latest_frame - 1U, 0U);
				prune_cancelled_waiting(session);
			}

			{
				std::lock_guard<std::mutex> lock(jobs_mutex);
				if (jobs.size() >= kMaximumQueuedTiles) {
					tiles_rejected.fetch_add(1, std::memory_order_relaxed);
					return;
				}
				jobs.emplace(JobKey{session.peer_id, job->frame_index, job->payload.header.tile_index}, job);
			}

			tiles_received.fetch_add(1, std::memory_order_relaxed);
			session.in_flight.fetch_add(1, std::memory_order_relaxed);

			const bool needs_bodies = (job->payload.header.flags & kTileTaskFlagNoBodies) == 0U;
			if (needs_bodies) {
				job->bodies = session.catalogs.find(job->frame_index);
				if (job->bodies == nullptr && (job->payload.header.flags & kTileTaskFlagReuseLatestBodies) != 0U) {
					job->bodies = session.catalogs.latest();
				}
				if (job->bodies == nullptr) {
					WaitingJob waiting{};
					waiting.job = job;
					waiting.deadline = now + kWaitingJobLifetime;
					session.waiting.push_back(std::move(waiting));
					return;
				}
			}
			launch_job(job);
		}
	}

	void handle_unauthenticated_packet(PeerSession& session, PacketType type, std::span<const uint8_t> payload, ClusterTimePoint now) {
		static_cast<void>(now);
		if (type == PacketType::NodeAuthentication && session.phase == SessionPhase::ServerAwaitAuthentication) {
			if (payload.size() != 32U) {
				close_session(session, DisconnectReason::AuthenticationFailed);
				return;
			}
			std::span<const uint8_t, 32> client_pub(payload.data(), 32);
			session.session_identifier = random_session_identifier();
			if (!session.channel.perform_handshake_server(client_pub, session.identity.private_key, options.auth_token)) {
				close_session(session, DisconnectReason::AuthenticationFailed);
				return;
			}
			session.phase = SessionPhase::ServerAwaitProof;
			static_cast<void>(send_plain(session, PacketType::NodeAuthChallenge, bytes_of(session.identity.public_key)));

		} else if (type == PacketType::NodeAuthChallenge && session.phase == SessionPhase::ClientAwaitChallenge) {
			if (payload.size() != 32U) {
				close_session(session, DisconnectReason::ProtocolViolation);
				return;
			}
			std::span<const uint8_t, 32> server_pub(payload.data(), 32);
			if (!session.channel.perform_handshake_client(server_pub, session.identity.private_key, options.auth_token)) {
				close_session(session, DisconnectReason::AuthenticationFailed);
				return;
			}
			AuthenticationProofPayload proof{};
			proof.role = static_cast<uint32_t>(ClusterNodeRole::Worker);
			proof.instance_identifier = instance_identifier;
			session.phase = SessionPhase::ClientAwaitAcknowledgement;
			static_cast<void>(send_encrypted(session, PacketType::NodeAuthResponse, 0U, bytes_of(proof), TransportChannel::Control));

		} else {
			close_session(session, DisconnectReason::ProtocolViolation);
		}
	}

	void run_event_loop() {
		while (!stop_requested()) {
			const ClusterTimePoint now = ClusterClock::now();

			if (options.has_master() && outbound_peer == kInvalidPeerId && now >= next_connect_attempt) {
				const auto maybe_peer = transport.connect(TransportEndpoint{options.master_host, options.master_port});
				if (maybe_peer.has_value()) {
					outbound_peer = *maybe_peer;
				}
				next_connect_attempt = now + std::chrono::milliseconds(options.reconnect_interval_ms);
			}

			size_t events_handled = 0;
			TransportEvent event{};
			thread_local std::vector<uint8_t> wire_receive_buffer(kMaximumTransportPacketBytes, 0U);
			const std::span<uint8_t> wire_span(wire_receive_buffer);

			while (events_handled < kMaximumEventBurst && transport.poll(event, wire_span, 0U)) {
				++events_handled;
				if (event.type == TransportEventType::Connected) {
					on_peer_connected(event.peer_id);
					if (auto it = sessions.find(event.peer_id); it != sessions.end() && it->second->initiated) {
						static_cast<void>(send_plain(*it->second, PacketType::NodeAuthentication, bytes_of(it->second->identity.public_key)));
					}
				} else if (event.type == TransportEventType::Disconnected) {
					on_peer_disconnected(event.peer_id);
				} else if (event.type == TransportEventType::Received) {
					auto it = sessions.find(event.peer_id);
					if (it == sessions.end() || event.truncated || event.payload_bytes < sizeof(PacketEnvelope)) {
						continue;
					}
					PeerSession& session = *it->second;
					PacketEnvelope envelope{};
					std::memcpy(&envelope, wire_receive_buffer.data(), sizeof(PacketEnvelope));
					if (!envelope.is_valid_magic()) {
						continue;
					}

					const size_t payload_size = event.payload_bytes - sizeof(PacketEnvelope);
					if (payload_size != envelope.payload_bytes || plain_buffer.size() < payload_size) {
						continue;
					}
					const std::span<const uint8_t> ciphertext(wire_receive_buffer.data() + sizeof(PacketEnvelope), payload_size);
					const std::span<uint8_t> plaintext(plain_buffer.data(), payload_size);
					const std::span<const uint8_t, 16> tag(envelope.authentication_tag);

					if (session.phase == SessionPhase::ServerAwaitAuthentication || session.phase == SessionPhase::ClientAwaitChallenge) {
						handle_unauthenticated_packet(session, envelope.type, ciphertext, now);
					} else {
						if (!session.channel.verify_and_decrypt_packet(envelope.authenticated_header(), envelope.packet_sequence, ciphertext, tag, plaintext)) {
							++session.integrity_failures;
							packets_rejected.fetch_add(1, std::memory_order_relaxed);
							if (session.integrity_failures > kMaximumIntegrityFailures) {
								close_session(session, DisconnectReason::AuthenticationFailed);
							}
							continue;
						}
						session.integrity_failures = 0;

						if (session.phase == SessionPhase::ServerAwaitProof && envelope.type == PacketType::NodeAuthResponse) {
							session.phase = SessionPhase::Established;
							session.established_at = now;
							sessions_accepted.fetch_add(1, std::memory_order_relaxed);
						} else if (session.phase == SessionPhase::ClientAwaitAcknowledgement && (envelope.type == PacketType::NodeAuthResponse || envelope.type == PacketType::CapabilitiesAdvertisement)) {
							session.phase = SessionPhase::Established;
							session.established_at = now;
							sessions_accepted.fetch_add(1, std::memory_order_relaxed);
							static_cast<void>(send_encrypted(session, PacketType::CapabilitiesAdvertisement, 0U, bytes_of(hardware), TransportChannel::Control));
						}
						if (session.phase == SessionPhase::Established) {
							if (event.channel == TransportChannel::TileData) {
								handle_data_packet(session, envelope.type, envelope.frame_index, plaintext, now);
							} else {
								handle_control_packet(session, envelope.type, envelope.frame_index, plaintext, now);
							}
						}
					}
				}
			}

			if (events_handled == 0) {
				std::this_thread::sleep_for(std::chrono::milliseconds(kPollSliceMs));
			}

			for (auto& [peer, session] : sessions) {
				if (session->phase != SessionPhase::Established) {
					if (seconds_since_epoch(now) - seconds_since_epoch(session->created_at) > kHandshakeTimeoutMs / 1000.0) {
						close_session(*session, DisconnectReason::HandshakeTimeout);
					}
					continue;
				}
				if (session->rekey_required.load(std::memory_order_acquire)) {
					close_session(*session, DisconnectReason::RekeyRequired);
					continue;
				}
				
				if (!session->waiting.empty()) {
					prune_cancelled_waiting(*session);
					auto& waiting = session->waiting;
					for (auto it = waiting.begin(); it != waiting.end();) {
						if (now >= it->deadline) {
							tiles_expired.fetch_add(1, std::memory_order_relaxed);
							release_job(it->job);
							it = waiting.erase(it);
						} else {
							++it;
						}
					}
				}
			}
		}
	}
};

ClusterWorkerService::ClusterWorkerService(const WorkerOptions& options)
	: impl_(std::make_unique<Impl>(options)) {}

ClusterWorkerService::~ClusterWorkerService() = default;

void ClusterWorkerService::bind_stop_flag(const std::atomic<bool>* flag) noexcept {
	impl_->external_stop = flag;
}

bool ClusterWorkerService::start() {
	return impl_->initialize();
}

void ClusterWorkerService::run() {
	if (impl_->started) {
		impl_->run_event_loop();
	}
}

void ClusterWorkerService::request_stop() noexcept {
	impl_->stop.store(true, std::memory_order_relaxed);
}

WorkerStatistics ClusterWorkerService::statistics() const noexcept {
	WorkerStatistics stats{};
	stats.sessions_accepted = impl_->sessions_accepted.load(std::memory_order_relaxed);
	stats.tiles_received = impl_->tiles_received.load(std::memory_order_relaxed);
	stats.tiles_completed = impl_->tiles_completed.load(std::memory_order_relaxed);
	stats.tiles_cancelled = impl_->tiles_cancelled.load(std::memory_order_relaxed);
	stats.tiles_rejected = impl_->tiles_rejected.load(std::memory_order_relaxed);
	stats.tiles_expired = impl_->tiles_expired.load(std::memory_order_relaxed);
	stats.tiles_on_gpu = impl_->tiles_on_gpu.load(std::memory_order_relaxed);
	stats.tiles_on_cpu = impl_->tiles_on_cpu.load(std::memory_order_relaxed);
	stats.packets_rejected = impl_->packets_rejected.load(std::memory_order_relaxed);
	stats.catalogs_received = impl_->catalogs_received.load(std::memory_order_relaxed);
	return stats;
}

std::string ClusterWorkerService::last_error() const {
	std::lock_guard<std::mutex> lock(impl_->error_mutex);
	return impl_->error;
}

}

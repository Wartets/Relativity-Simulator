#pragma once

#include "relativistic/render/gpu_types.hpp"
#include "relativistic/distributed/cluster_config.hpp"
#include "relativistic/distributed/cluster_node.hpp"
#include "relativistic/distributed/cluster_telemetry.hpp"
#include "relativistic/distributed/node_registry.hpp"
#include "relativistic/distributed/distribution_scheduler.hpp"
#include "relativistic/distributed/network_transport.hpp"
#include "relativistic/distributed/discovery_service.hpp"
#include "relativistic/distributed/body_catalog.hpp"
#include "relativistic/distributed/compute_kernels.hpp"
#include "relativistic/distributed/compute_payloads.hpp"
#include "relativistic/distributed/task_payloads.hpp"
#include "relativistic/distributed/packet_protocol.hpp"
#include "relativistic/distributed/session_protocol.hpp"
#include "relativistic/distributed/crypto_channel.hpp"
#include "relativistic/core/thread_pool.hpp"
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <span>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace Relativistic::Distributed {

enum class DispatchProfile : uint8_t {
	Interactive = 0,
	Offline = 1
};

struct FrameWindow {
	std::span<Render::GpuPixelOutput> pixels{};
	uint32_t width{0};
	uint32_t height{0};
	uint32_t first_row{0};
	uint32_t row_count{0};

	[[nodiscard]] bool store_tile(const TileGeometry& geometry, std::span<const Render::GpuPixelOutput> tile_pixels) const noexcept;
};

using TileCompletionSink = std::function<void(uint32_t, std::span<const Render::GpuPixelOutput>)>;

using LocalTileRenderer = std::function<bool(
	const Render::GpuCameraPushConstants&,
	std::span<const uint32_t>,
	std::span<const Render::GpuBodyData>,
	const TileCompletionSink&,
	const std::atomic<bool>*
)>;

using LocalNBodyEvaluator = std::function<bool(
	std::span<const NBodyBodyState>,
	uint32_t,
	uint32_t,
	uint32_t,
	double,
	double,
	std::span<NBodyRowResult>
)>;

class ClusterOrchestrator {
private:
	struct MasterPeerSession {
		uint32_t peer_id{kInvalidPeerId};
		uint32_t node_id{0};
		bool authenticated{false};
		bool handshaking{false};
		bool initiated{false};
		CryptoKeyPair identity{};
		CryptoChannel security_channel{};
		uint32_t session_identifier{0};
		uint32_t transmit_sequence{0};
		uint32_t integrity_failures{0};
		std::atomic<bool> rekey_required{false};
		std::mutex send_mutex{};
		ClusterTimePoint created{};
		ClusterTimePoint last_ping{};
		double last_ping_timestamp{0.0};
	};

	struct ComputeJobState {
		uint32_t job_id{0};
		PacketType packet_type{PacketType::SimulationSubStepData};
		uint32_t unit_count{0};
		std::unique_ptr<std::atomic<uint8_t>[]> status{};
		std::unique_ptr<std::atomic<int64_t>[]> deadline_ns{};
		std::vector<uint32_t> owner{};
		std::atomic<uint32_t> completed{0};
		std::atomic<uint32_t> active_applies{0};
		std::atomic<bool> closed{false};
		std::atomic<bool> failed{false};
		std::function<bool(uint32_t, std::span<const uint8_t>)> apply_result{};
	};

	struct ComputeJobSpec {
		PacketType packet_type{PacketType::SimulationSubStepData};
		uint32_t unit_count{0};
		double unit_timeout_seconds{0.1};
		bool allow_remote{true};
		std::function<bool(MasterPeerSession&, uint32_t)> prepare_session{};
		std::function<std::vector<uint8_t>(uint32_t, uint32_t)> build_task{};
		std::function<std::vector<uint8_t>(uint32_t, uint32_t)> execute_local{};
		std::function<bool(uint32_t, std::span<const uint8_t>)> apply_result{};
	};

	ClusterConfiguration config_{};
	NodeRegistry registry_{};
	DistributionScheduler scheduler_;
	NetworkTransport transport_{};
	DiscoveryService discovery_{};

	uint32_t local_abi_signature_{0};
	std::atomic<bool> route_interactive_{true};
	std::atomic<bool> route_offline_{true};
	std::atomic<bool> route_monte_carlo_{true};
	std::atomic<bool> route_nbody_{false};
	std::atomic<bool> route_exact_metrics_{true};
	std::atomic<bool> cluster_active_{false};
	std::atomic<uint64_t> global_frame_index_{0};
	std::atomic<uint32_t> next_compute_job_{0};
	std::mutex compute_mutex_{};
	std::mutex compute_run_mutex_{};
	std::shared_ptr<ComputeJobState> active_compute_{};

	mutable std::mutex pump_mutex_{};
	std::mutex dispatch_mutex_{};
	mutable std::mutex renderer_mutex_{};
	mutable std::mutex telemetry_mutex_{};
	mutable std::mutex error_mutex_{};

	std::unordered_map<uint32_t, std::shared_ptr<MasterPeerSession>> sessions_by_peer_{};
	std::unordered_map<uint32_t, std::shared_ptr<MasterPeerSession>> sessions_by_node_{};
	std::map<uint64_t, DiscoveredPeer> discovered_peers_{};
	std::unordered_map<uint32_t, ClusterTimePoint> reconnect_after_{};
	std::vector<uint32_t> reclaimed_by_node_loss_{};
	std::vector<ClusterTimePoint> tile_dispatch_time_{};
	ClusterTimePoint next_maintenance_{};

	const FrameWindow* active_window_{nullptr};
	uint64_t active_frame_{0};
	uint32_t last_broadcast_catalog_frame_{0xFFFFFFFFU};
	DispatchProfile applied_profile_{DispatchProfile::Interactive};

	uint64_t instance_identifier_{0};
	NodeHardwareCapabilities local_capabilities_{};
	ThroughputEstimator local_throughput_{};
	LocalTileRenderer local_renderer_{};
	LocalNBodyEvaluator local_nbody_evaluator_{};

	uint64_t frame_bytes_received_{0};
	double frame_decode_seconds_{0.0};
	double frame_local_seconds_{0.0};
	ClusterFrameTelemetry telemetry_{};
	std::string last_error_{};

	std::jthread pump_thread_{};

	void set_error(const std::string& message);
	void reseed_local_throughput() noexcept;
	[[nodiscard]] LoadBalancerParameters make_profile_parameters(DispatchProfile profile) const noexcept;

	void pump_loop(std::stop_token stop) noexcept;
	void service_unlocked(ClusterTimePoint now);
	void process_network_events() noexcept;
	void on_peer_connected(uint32_t peer_id);
	void on_peer_disconnected(uint32_t peer_id);
	void on_packet_received(const TransportEvent& event, std::span<const uint8_t> wire, std::span<uint8_t> plain);
	void handle_handshake_packet(const std::shared_ptr<MasterPeerSession>& session, const PacketEnvelope& envelope, std::span<const uint8_t> cipher);
	void handle_authenticated_packet(const std::shared_ptr<MasterPeerSession>& session, const PacketEnvelope& envelope, std::span<const uint8_t> plain);
	void ping_heartbeats(ClusterTimePoint now) noexcept;
	void detect_timeouts(ClusterTimePoint now) noexcept;
	void maintain_connections(ClusterTimePoint now);
	void ensure_connection(const ClusterNodeDescriptor& descriptor, ClusterTimePoint now);
	void close_session(MasterPeerSession& session, DisconnectReason reason) noexcept;

	bool send_encrypted_packet(MasterPeerSession& session, PacketType type, uint64_t frame_index, std::span<const uint8_t> payload, TransportChannel channel);
	bool send_plain_packet(MasterPeerSession& session, PacketType type, std::span<const uint8_t> payload);
	void broadcast_bodies_if_needed(uint32_t frame_index, std::span<const Render::GpuBodyData> bodies);
	void cancel_remote_frame(uint64_t frame_index);
	void send_remote_tasks(uint64_t frame_index, const Render::GpuCameraPushConstants& constants, bool bodies_empty, ClusterTimePoint now, std::vector<uint32_t>& undispatched);

	bool run_compute_job(const ComputeJobSpec& spec, const std::atomic<bool>* cancel_flag);
	void execute_compute_units_locally(ComputeJobState& job, const ComputeJobSpec& spec, std::span<const uint32_t> units, const std::atomic<bool>* cancel_flag);
	bool complete_compute_unit(ComputeJobState& job, uint32_t unit, std::span<const uint8_t> body, bool remote);
	void handle_compute_result(const std::shared_ptr<MasterPeerSession>& session, PacketType type, std::span<const uint8_t> plain);
	bool send_nbody_state(MasterPeerSession& session, uint32_t job_id, std::span<const NBodyBodyState> bodies);

	bool run_local_batch(
		const Render::GpuCameraPushConstants& constants,
		std::span<const uint32_t> tiles,
		std::span<const Render::GpuBodyData> bodies,
		const FrameWindow& window,
		uint64_t frame_index,
		const LocalTileRenderer& renderer,
		const std::atomic<bool>* cancel_flag
	);

public:
	explicit ClusterOrchestrator(const ClusterConfiguration& configuration) noexcept;
	~ClusterOrchestrator() noexcept;

	ClusterOrchestrator(const ClusterOrchestrator&) = delete;
	ClusterOrchestrator& operator=(const ClusterOrchestrator&) = delete;

	bool start() noexcept;
	void shutdown() noexcept;

	void set_local_tile_renderer(LocalTileRenderer renderer);
	void clear_local_tile_renderer() noexcept;
	void set_local_nbody_evaluator(LocalNBodyEvaluator evaluator);
	void clear_local_nbody_evaluator() noexcept;
	void set_local_gpu_info(const LocalGpuInfo& info) noexcept;

	void set_interactive_routing(bool enabled) noexcept { route_interactive_.store(enabled, std::memory_order_relaxed); }
	void set_offline_routing(bool enabled) noexcept { route_offline_.store(enabled, std::memory_order_relaxed); }
	void set_monte_carlo_routing(bool enabled) noexcept { route_monte_carlo_.store(enabled, std::memory_order_relaxed); }
	void set_nbody_routing(bool enabled) noexcept { route_nbody_.store(enabled, std::memory_order_relaxed); }
	void set_exact_metrics_routing(bool enabled) noexcept { route_exact_metrics_.store(enabled, std::memory_order_relaxed); }

	[[nodiscard]] bool interactive_routing_enabled() const noexcept { return route_interactive_.load(std::memory_order_relaxed); }
	[[nodiscard]] bool offline_routing_enabled() const noexcept { return route_offline_.load(std::memory_order_relaxed); }
	[[nodiscard]] bool monte_carlo_routing_enabled() const noexcept { return route_monte_carlo_.load(std::memory_order_relaxed); }
	[[nodiscard]] bool nbody_routing_enabled() const noexcept { return route_nbody_.load(std::memory_order_relaxed); }
	[[nodiscard]] bool exact_metrics_routing_enabled() const noexcept { return route_exact_metrics_.load(std::memory_order_relaxed); }

	[[nodiscard]] bool is_cluster_operational() const noexcept {
		return cluster_active_.load(std::memory_order_relaxed);
	}

	[[nodiscard]] const ClusterConfiguration& configuration() const noexcept {
		return config_;
	}

	[[nodiscard]] NodeRegistry& registry() noexcept {
		return registry_;
	}

	[[nodiscard]] const NodeRegistry& registry() const noexcept {
		return registry_;
	}

	[[nodiscard]] DistributionScheduler& scheduler() noexcept {
		return scheduler_;
	}

	[[nodiscard]] const DistributionScheduler& scheduler() const noexcept {
		return scheduler_;
	}

	[[nodiscard]] uint32_t workers_active() const;
	[[nodiscard]] ClusterFrameTelemetry last_frame_telemetry() const;
	[[nodiscard]] std::string last_error() const;
	[[nodiscard]] std::vector<NodeSnapshot> node_snapshots() const;
	[[nodiscard]] std::vector<DiscoveredPeer> discovered_peers() const;
	bool ping_node(uint32_t node_id) noexcept;

	bool prepare_frame_workload(
		uint64_t frame_index,
		uint32_t total_tiles,
		std::span<const uint8_t> tile_importance_mask
	);

	bool dispatch_frame(
		const Render::GpuCameraPushConstants& constants,
		const FrameWindow& window,
		std::span<const Render::GpuBodyData> bodies,
		DispatchProfile profile,
		const std::atomic<bool>* cancel_flag
	);

	bool dispatch_distributed(
		const Render::GpuCameraPushConstants& constants,
		std::span<Render::GpuPixelOutput> framebuffer,
		std::span<const Render::GpuBodyData> bodies,
		const std::atomic<bool>* cancel_flag
	);

	[[nodiscard]] bool evaluate_nbody_accelerations(
		std::span<const NBodyBodyState> bodies,
		const NBodyEvaluationOptions& options,
		std::span<NBodyRowResult> output,
		const std::atomic<bool>* cancel_flag
	);

	[[nodiscard]] bool run_geodesic_ensemble(
		const GeodesicEnsembleSpec& spec,
		uint32_t samples_per_unit,
		GeodesicEnsemblePartial& result,
		const std::atomic<bool>* cancel_flag
	);

	[[nodiscard]] bool evaluate_stochastic_pce_quadrature(
		const StochasticEvaluationSpec& spec,
		uint32_t points_per_unit,
		std::span<StochasticQuadratureResult> results,
		const std::atomic<bool>* cancel_flag
	);

	static bool render_tiles_software(
		const Render::GpuCameraPushConstants& constants,
		std::span<const uint32_t> tiles,
		std::span<const Render::GpuBodyData> bodies,
		const TileCompletionSink& sink,
		const std::atomic<bool>* cancel_flag
	);
};

}

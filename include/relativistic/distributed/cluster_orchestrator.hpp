#pragma once

#include "relativistic/render/gpu_types.hpp"
#include "relativistic/distributed/cluster_config.hpp"
#include "relativistic/distributed/cluster_node.hpp"
#include "relativistic/distributed/node_registry.hpp"
#include "relativistic/distributed/distribution_scheduler.hpp"
#include "relativistic/distributed/network_transport.hpp"
#include "relativistic/distributed/discovery_service.hpp"
#include "relativistic/distributed/body_catalog.hpp"
#include "relativistic/distributed/task_payloads.hpp"
#include "relativistic/distributed/packet_protocol.hpp"
#include "relativistic/distributed/crypto_channel.hpp"
#include "relativistic/core/thread_pool.hpp"
#include <vector>
#include <span>
#include <memory>
#include <mutex>
#include <atomic>
#include <chrono>
#include <unordered_map>
#include <thread>

namespace Relativistic::Distributed {

struct ConnectedWorkerNode {
	uint32_t node_identifier{0};
	NodeState state{NodeState::Disconnected};
	NodeHardwareCapabilities hardware{};
	CryptoChannel security_channel{};
	double moving_average_rtt_seconds{0.005};
	double variance_rtt_seconds{0.0001};
	double execution_throughput_rays_per_second{1.0e6};
	uint64_t frames_served_count{0};
	uint64_t assigned_tiles_mask{0};
	std::chrono::steady_clock::time_point last_heartbeat_timestamp{};
};

class ClusterOrchestrator {
private:
	struct MasterPeerSession {
		uint32_t peer_id{kInvalidPeerId};
		uint32_t node_id{0};
		bool authenticated{false};
		bool handshaking{false};
		CryptoKeyPair identity{};
		CryptoChannel security_channel{};
		uint32_t session_identifier{0};
		uint32_t transmit_sequence{0};
		std::mutex send_mutex{};
		std::chrono::steady_clock::time_point last_ping{};
		double last_ping_timestamp{0.0};
	};

	ClusterConfiguration config_{};
	NodeRegistry registry_{};
	DistributionScheduler scheduler_{};
	NetworkTransport transport_{};
	DiscoveryService discovery_{};
	BodyCatalogStore catalog_store_{};

	std::vector<ConnectedWorkerNode> workers_{};
	mutable std::mutex workers_mutex_{};
	std::unordered_map<uint32_t, std::shared_ptr<MasterPeerSession>> sessions_by_peer_{};
	std::unordered_map<uint32_t, std::shared_ptr<MasterPeerSession>> sessions_by_node_{};

	std::atomic<bool> cluster_active_{false};
	std::atomic<uint64_t> global_frame_index_{0};
	uint32_t local_abi_signature_{0};
	uint32_t last_broadcast_catalog_frame_{0xFFFFFFFFU};

	std::vector<TileWorkAssignment> pending_tasks_{};
	std::vector<uint8_t> tile_completed_table_{};

	void ping_heartbeats_unlocked(std::chrono::steady_clock::time_point current_time) noexcept;
	void detect_timeouts_and_reclaim_tasks(std::chrono::steady_clock::time_point current_time) noexcept;

	void process_network_events(
		uint64_t current_frame_index,
		std::span<Render::GpuPixelOutput> destination_framebuffer,
		size_t screen_width,
		size_t screen_height
	) noexcept;

	bool send_encrypted_packet(
		MasterPeerSession& session,
		PacketType type,
		uint64_t frame_index,
		std::span<const uint8_t> payload,
		TransportChannel channel
	);

	bool send_plain_packet(
		MasterPeerSession& session,
		PacketType type,
		std::span<const uint8_t> payload
	);

	void broadcast_bodies_if_needed(uint32_t frame_index, std::span<const Render::GpuBodyData> bodies);

public:
	explicit ClusterOrchestrator(const ClusterConfiguration& configuration) noexcept;
	~ClusterOrchestrator() noexcept;

	ClusterOrchestrator(const ClusterOrchestrator&) = delete;
	ClusterOrchestrator& operator=(const ClusterOrchestrator&) = delete;

	bool start() noexcept;
	void shutdown() noexcept;

	void register_worker_node(const ConnectedWorkerNode& worker);
	void unregister_worker_node(uint32_t node_id);

	void prepare_frame_workload(
		uint64_t frame_index,
		uint32_t total_tiles,
		std::span<const uint8_t> tile_importance_mask
	);

	[[nodiscard]] size_t fetch_remote_tile_results(
		uint64_t current_frame_index,
		std::span<Render::GpuPixelOutput> destination_framebuffer,
		size_t screen_width,
		size_t screen_height
	) noexcept;

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

	bool dispatch_distributed(
		const Render::GpuCameraPushConstants& constants,
		std::span<Render::GpuPixelOutput> framebuffer,
		std::span<const Render::GpuBodyData> bodies,
		const std::atomic<bool>* cancel_flag
	);
};

}

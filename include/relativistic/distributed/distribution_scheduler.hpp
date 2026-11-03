#pragma once

#include "relativistic/distributed/cluster_node.hpp"
#include "relativistic/distributed/task_payloads.hpp"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <span>
#include <utility>
#include <vector>

namespace Relativistic::Distributed {

inline constexpr uint32_t kLocalNodeIdentifier = 0;
inline constexpr uint8_t kNominalTileCost = 128;
inline constexpr uint32_t kMaximumSchedulerTiles = tile_grid_columns(kMaximumScreenWidth) * tile_grid_rows(kMaximumScreenHeight);

enum class TileStatus : uint8_t {
	Skipped = 0,
	Queued = 1,
	InFlight = 2,
	Reclaimed = 3,
	CompletedRemote = 4,
	CompletedLocal = 5
};

enum class ReclaimReason : uint8_t {
	Timeout = 0,
	Speculative = 1,
	NodeLost = 2
};

enum class TileInterleaveMode : uint8_t {
	Interleaved = 0,
	Contiguous = 1
};

enum class TileCompletionResult : uint8_t {
	Accepted = 0,
	DuplicateIgnored = 1,
	StaleFrame = 2,
	UnknownTile = 3,
	NotOwner = 4,
	NotDispatched = 5
};

struct LoadBalancerParameters {
	bool dynamic_balancing{true};
	bool work_stealing{true};
	double latency_penalty_alpha{1.0};
	double minimum_timeout_seconds{0.004};
	double maximum_timeout_seconds{0.120};
	double speculative_fraction{0.5};
	TileInterleaveMode interleave_mode{TileInterleaveMode::Interleaved};
};

struct SlotAllocation {
	uint32_t node_id{kLocalNodeIdentifier};
	bool is_local{false};
	double share{0.0};
	uint32_t quota{0};
};

struct ReclaimedTile {
	uint32_t tile_index{0};
	uint32_t node_id{kLocalNodeIdentifier};
	ReclaimReason reason{ReclaimReason::Timeout};
};

struct SchedulerStatistics {
	uint64_t frames_planned{0};
	uint64_t tiles_assigned_local{0};
	uint64_t tiles_assigned_remote{0};
	uint64_t tiles_dispatched{0};
	uint64_t tiles_completed_local{0};
	uint64_t tiles_completed_remote{0};
	uint64_t tiles_reclaimed_timeout{0};
	uint64_t tiles_reclaimed_speculative{0};
	uint64_t tiles_reclaimed_node_lost{0};
	uint64_t late_results_discarded{0};
	uint64_t stale_results_discarded{0};
};

[[nodiscard]] bool compute_capacity_shares(
	std::span<const NodeCapacityInput> nodes,
	double latency_penalty_alpha,
	bool dynamic_balancing,
	std::span<double> shares
) noexcept;

[[nodiscard]] bool compute_tile_quotas(
	std::span<const double> shares,
	uint32_t active_tiles,
	size_t local_slot,
	std::span<uint32_t> quotas
) noexcept;

[[nodiscard]] double estimate_tile_compute_seconds(uint32_t pixel_count, uint8_t cost, double seconds_per_kilo_ray) noexcept;

class DistributionScheduler {
private:
	static constexpr size_t kNoSlot = static_cast<size_t>(-1);

	LoadBalancerParameters parameters_{};
	mutable std::mutex mutex_{};
	std::unique_ptr<std::atomic<uint8_t>[]> status_{};
	std::vector<uint32_t> owner_{};
	std::vector<uint8_t> cost_{};
	std::vector<ClusterTimePoint> dispatch_time_{};
	std::vector<ClusterTimePoint> deadline_{};
	std::vector<uint32_t> active_tiles_{};
	std::vector<uint32_t> in_flight_{};
	std::vector<SlotAllocation> slots_{};
	std::vector<std::vector<uint32_t>> slot_tiles_{};
	std::vector<NodeCapacityInput> capacity_inputs_{};
	std::vector<double> shares_{};
	std::vector<uint32_t> quotas_{};
	std::vector<int64_t> credits_{};
	std::vector<std::pair<ClusterTimePoint, uint32_t>> candidates_{};
	size_t local_slot_{0};

	std::atomic<uint64_t> frame_index_{0};
	std::atomic<uint32_t> tile_count_{0};
	std::atomic<uint32_t> expected_completions_{0};
	std::atomic<uint32_t> completed_tiles_{0};

	std::atomic<uint64_t> frames_planned_{0};
	std::atomic<uint64_t> assigned_local_{0};
	std::atomic<uint64_t> assigned_remote_{0};
	std::atomic<uint64_t> dispatched_{0};
	std::atomic<uint64_t> completed_local_{0};
	std::atomic<uint64_t> completed_remote_{0};
	std::atomic<uint64_t> reclaimed_timeout_{0};
	std::atomic<uint64_t> reclaimed_speculative_{0};
	std::atomic<uint64_t> reclaimed_node_lost_{0};
	std::atomic<uint64_t> late_discarded_{0};
	std::atomic<uint64_t> stale_discarded_{0};

	[[nodiscard]] std::atomic<uint8_t>& status_at(uint32_t tile_index) noexcept { return status_[tile_index]; }
	[[nodiscard]] const std::atomic<uint8_t>& status_at(uint32_t tile_index) const noexcept { return status_[tile_index]; }

	void distribute_interleaved();
	void distribute_contiguous();
	bool try_reclaim_unlocked(uint32_t tile_index, ReclaimReason reason, std::vector<ReclaimedTile>& output);
	void count_reclaim(ReclaimReason reason) noexcept;

public:
	explicit DistributionScheduler(const LoadBalancerParameters& parameters = {});

	DistributionScheduler(const DistributionScheduler&) = delete;
	DistributionScheduler& operator=(const DistributionScheduler&) = delete;

	void configure(const LoadBalancerParameters& parameters);
	[[nodiscard]] LoadBalancerParameters parameters() const;

	[[nodiscard]] bool begin_frame(
		uint64_t frame_index,
		uint32_t total_tiles,
		std::span<const uint8_t> tile_importance,
		std::span<const NodeCapacityInput> nodes
	);

	[[nodiscard]] size_t slot_count() const;
	[[nodiscard]] SlotAllocation slot_allocation(size_t slot) const;
	[[nodiscard]] std::span<const uint32_t> slot_tiles(size_t slot) const noexcept;
	[[nodiscard]] std::span<const uint32_t> local_tiles() const noexcept;
	[[nodiscard]] size_t local_slot() const noexcept { return local_slot_; }
	[[nodiscard]] std::span<const uint32_t> active_tiles() const noexcept { return active_tiles_; }

	[[nodiscard]] bool mark_dispatched(
		uint32_t tile_index,
		uint32_t node_id,
		ClusterTimePoint now,
		uint32_t pixel_count,
		const DispatchTiming& timing
	);

	[[nodiscard]] TileCompletionResult complete_remote(uint64_t frame_index, uint32_t tile_index, uint32_t node_id);
	[[nodiscard]] bool should_compute_local(uint64_t frame_index, uint32_t tile_index) const noexcept;
	[[nodiscard]] bool try_complete_local(uint64_t frame_index, uint32_t tile_index) noexcept;

	size_t collect_expired(ClusterTimePoint now, std::vector<ReclaimedTile>& output);
	size_t reclaim_node(uint32_t node_id, std::vector<ReclaimedTile>& output);
	size_t steal_speculative(ClusterTimePoint now, size_t maximum_count, std::vector<ReclaimedTile>& output);

	[[nodiscard]] TileStatus tile_status(uint32_t tile_index) const noexcept;
	[[nodiscard]] bool is_frame_complete() const noexcept;
	[[nodiscard]] uint32_t remaining_tiles() const noexcept;
	[[nodiscard]] uint32_t in_flight_count() const;
	[[nodiscard]] uint64_t current_frame() const noexcept { return frame_index_.load(std::memory_order_acquire); }
	[[nodiscard]] SchedulerStatistics statistics() const noexcept;
};

}

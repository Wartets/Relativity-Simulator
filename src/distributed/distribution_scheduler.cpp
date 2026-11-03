#include "relativistic/distributed/distribution_scheduler.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace Relativistic::Distributed {

namespace {

[[nodiscard]] constexpr uint8_t code(TileStatus status) noexcept {
	return static_cast<uint8_t>(status);
}

[[nodiscard]] double latency_penalty(double round_trip_seconds, double alpha) noexcept {
	const double ratio = std::max(round_trip_seconds / kLatencyReferenceSeconds, 1.0);
	return std::pow(ratio, -alpha);
}

}

bool compute_capacity_shares(
	std::span<const NodeCapacityInput> nodes,
	double latency_penalty_alpha,
	bool dynamic_balancing,
	std::span<double> shares
) noexcept {
	if (nodes.empty() || shares.size() != nodes.size()) {
		return false;
	}
	const double alpha = std::clamp(std::isfinite(latency_penalty_alpha) ? latency_penalty_alpha : 1.0, 0.0, 4.0);
	double total = 0.0;
	for (size_t i = 0; i < nodes.size(); ++i) {
		const NodeCapacityInput& node = nodes[i];
		double score = 0.0;
		if (node.eligible && std::isfinite(node.static_weight) && node.static_weight > 0.0) {
			score = node.static_weight;
			if (dynamic_balancing) {
				const double execution = (std::isfinite(node.seconds_per_kilo_ray) && node.seconds_per_kilo_ray > 0.0)
					? std::max(node.seconds_per_kilo_ray, 1.0e-9)
					: kDefaultSecondsPerKiloRay;
				const double round_trip = (std::isfinite(node.round_trip_seconds) && node.round_trip_seconds > 0.0) ? node.round_trip_seconds : 0.0;
				score *= (1.0 / execution) * latency_penalty(round_trip, alpha);
			}
		}
		shares[i] = score;
		total += score;
	}
	if (!(total > 0.0) || !std::isfinite(total)) {
		std::fill(shares.begin(), shares.end(), 0.0);
		return false;
	}
	for (double& share : shares) {
		share /= total;
	}
	return true;
}

bool compute_tile_quotas(
	std::span<const double> shares,
	uint32_t active_tiles,
	size_t local_slot,
	std::span<uint32_t> quotas
) noexcept {
	if (shares.empty() || shares.size() != quotas.size() || local_slot >= shares.size()) {
		return false;
	}
	uint64_t assigned = 0;
	for (size_t i = 0; i < shares.size(); ++i) {
		const double scaled = shares[i] * static_cast<double>(active_tiles) + 1.0e-9;
		uint32_t quota = 0;
		if (std::isfinite(scaled) && scaled > 0.0) {
			quota = static_cast<uint32_t>(std::min(std::floor(scaled), static_cast<double>(active_tiles)));
		}
		quotas[i] = quota;
		assigned += quota;
	}
	while (assigned > active_tiles) {
		size_t victim = local_slot;
		uint32_t largest = 0;
		for (size_t i = 0; i < quotas.size(); ++i) {
			if (i != local_slot && quotas[i] > largest) {
				largest = quotas[i];
				victim = i;
			}
		}
		--quotas[victim];
		--assigned;
	}
	quotas[local_slot] += static_cast<uint32_t>(active_tiles - assigned);
	return true;
}

double estimate_tile_compute_seconds(uint32_t pixel_count, uint8_t cost, double seconds_per_kilo_ray) noexcept {
	const double rate = (std::isfinite(seconds_per_kilo_ray) && seconds_per_kilo_ray > 0.0) ? seconds_per_kilo_ray : kDefaultSecondsPerKiloRay;
	const double cost_factor = static_cast<double>(std::max<uint8_t>(cost, 1U)) / static_cast<double>(kNominalTileCost);
	return static_cast<double>(pixel_count) * 1.0e-3 * rate * cost_factor;
}

DistributionScheduler::DistributionScheduler(const LoadBalancerParameters& parameters)
	: parameters_(parameters),
	  status_(std::make_unique<std::atomic<uint8_t>[]>(kMaximumSchedulerTiles)) {
	owner_.assign(kMaximumSchedulerTiles, kLocalNodeIdentifier);
	cost_.assign(kMaximumSchedulerTiles, kNominalTileCost);
	dispatch_time_.assign(kMaximumSchedulerTiles, ClusterTimePoint{});
	deadline_.assign(kMaximumSchedulerTiles, ClusterTimePoint{});
	active_tiles_.reserve(kMaximumSchedulerTiles);
	in_flight_.reserve(kMaximumSchedulerTiles);
}

void DistributionScheduler::configure(const LoadBalancerParameters& parameters) {
	std::lock_guard<std::mutex> lock(mutex_);
	parameters_ = parameters;
	parameters_.latency_penalty_alpha = std::clamp(std::isfinite(parameters_.latency_penalty_alpha) ? parameters_.latency_penalty_alpha : 1.0, 0.0, 4.0);
	parameters_.minimum_timeout_seconds = std::max(parameters_.minimum_timeout_seconds, 1.0e-4);
	parameters_.maximum_timeout_seconds = std::max(parameters_.maximum_timeout_seconds, parameters_.minimum_timeout_seconds);
	parameters_.speculative_fraction = std::clamp(parameters_.speculative_fraction, 0.05, 1.0);
}

LoadBalancerParameters DistributionScheduler::parameters() const {
	std::lock_guard<std::mutex> lock(mutex_);
	return parameters_;
}

void DistributionScheduler::distribute_interleaved() {
	const size_t slot_total = slots_.size();
	credits_.assign(slot_total, 0);
	const auto total_weight = static_cast<int64_t>(active_tiles_.size());
	for (const uint32_t tile : active_tiles_) {
		size_t selected = kNoSlot;
		int64_t best_credit = std::numeric_limits<int64_t>::min();
		for (size_t slot = 0; slot < slot_total; ++slot) {
			if (slots_[slot].quota == 0U) {
				continue;
			}
			credits_[slot] += static_cast<int64_t>(slots_[slot].quota);
			if (credits_[slot] > best_credit) {
				best_credit = credits_[slot];
				selected = slot;
			}
		}
		if (selected == kNoSlot) {
			selected = local_slot_;
		}
		credits_[selected] -= total_weight;
		slot_tiles_[selected].push_back(tile);
	}
}

void DistributionScheduler::distribute_contiguous() {
	size_t cursor = 0;
	for (size_t slot = 0; slot < slots_.size(); ++slot) {
		const size_t count = std::min<size_t>(slots_[slot].quota, active_tiles_.size() - cursor);
		slot_tiles_[slot].assign(active_tiles_.begin() + static_cast<std::ptrdiff_t>(cursor), active_tiles_.begin() + static_cast<std::ptrdiff_t>(cursor + count));
		cursor += count;
	}
}

bool DistributionScheduler::begin_frame(
	uint64_t frame_index,
	uint32_t total_tiles,
	std::span<const uint8_t> tile_importance,
	std::span<const NodeCapacityInput> nodes
) {
	if (total_tiles == 0U || total_tiles > kMaximumSchedulerTiles) {
		return false;
	}
	if (!tile_importance.empty() && tile_importance.size() < total_tiles) {
		return false;
	}

	std::lock_guard<std::mutex> lock(mutex_);

	capacity_inputs_.clear();
	size_t local_entry = kNoSlot;
	for (const NodeCapacityInput& node : nodes) {
		if (node.is_local) {
			if (local_entry == kNoSlot) {
				local_entry = capacity_inputs_.size();
				NodeCapacityInput local = node;
				local.node_id = kLocalNodeIdentifier;
				local.eligible = true;
				capacity_inputs_.push_back(local);
			}
		} else if (node.eligible && node.node_id != kLocalNodeIdentifier) {
			capacity_inputs_.push_back(node);
		}
	}
	if (local_entry == kNoSlot) {
		NodeCapacityInput local{};
		local.node_id = kLocalNodeIdentifier;
		local.is_local = true;
		local.eligible = true;
		local.static_weight = 0.0;
		local_entry = capacity_inputs_.size();
		capacity_inputs_.push_back(local);
	}

	const size_t slot_total = capacity_inputs_.size();
	shares_.assign(slot_total, 0.0);
	if (!compute_capacity_shares(capacity_inputs_, parameters_.latency_penalty_alpha, parameters_.dynamic_balancing, shares_)) {
		std::fill(shares_.begin(), shares_.end(), 0.0);
		shares_[local_entry] = 1.0;
	}

	active_tiles_.clear();
	for (uint32_t tile = 0; tile < total_tiles; ++tile) {
		const uint8_t importance = tile_importance.empty() ? kNominalTileCost : tile_importance[tile];
		if (importance == 0U) {
			status_at(tile).store(code(TileStatus::Skipped), std::memory_order_relaxed);
		} else {
			status_at(tile).store(code(TileStatus::Queued), std::memory_order_relaxed);
			active_tiles_.push_back(tile);
		}
		cost_[tile] = importance;
		owner_[tile] = kLocalNodeIdentifier;
	}

	const auto active_count = static_cast<uint32_t>(active_tiles_.size());
	quotas_.assign(slot_total, 0U);
	if (!compute_tile_quotas(shares_, active_count, local_entry, quotas_)) {
		return false;
	}

	local_slot_ = local_entry;
	slots_.clear();
	slots_.reserve(slot_total);
	if (slot_tiles_.size() < slot_total) {
		slot_tiles_.resize(slot_total);
	}
	for (size_t slot = 0; slot < slot_total; ++slot) {
		slots_.push_back(SlotAllocation{capacity_inputs_[slot].node_id, slot == local_entry, shares_[slot], quotas_[slot]});
		slot_tiles_[slot].clear();
		slot_tiles_[slot].reserve(quotas_[slot]);
	}

	if (parameters_.interleave_mode == TileInterleaveMode::Interleaved) {
		distribute_interleaved();
	} else {
		distribute_contiguous();
	}

	uint64_t remote_assigned = 0;
	for (size_t slot = 0; slot < slot_total; ++slot) {
		for (const uint32_t tile : slot_tiles_[slot]) {
			owner_[tile] = slots_[slot].node_id;
		}
		if (slot != local_slot_) {
			remote_assigned += slot_tiles_[slot].size();
		}
	}

	in_flight_.clear();
	completed_tiles_.store(0, std::memory_order_release);
	expected_completions_.store(active_count, std::memory_order_release);
	tile_count_.store(total_tiles, std::memory_order_release);
	frame_index_.store(frame_index, std::memory_order_release);

	frames_planned_.fetch_add(1, std::memory_order_relaxed);
	assigned_remote_.fetch_add(remote_assigned, std::memory_order_relaxed);
	assigned_local_.fetch_add(active_count - remote_assigned, std::memory_order_relaxed);
	return true;
}

size_t DistributionScheduler::slot_count() const {
	std::lock_guard<std::mutex> lock(mutex_);
	return slots_.size();
}

SlotAllocation DistributionScheduler::slot_allocation(size_t slot) const {
	std::lock_guard<std::mutex> lock(mutex_);
	return (slot < slots_.size()) ? slots_[slot] : SlotAllocation{};
}

std::span<const uint32_t> DistributionScheduler::slot_tiles(size_t slot) const noexcept {
	if (slot >= slots_.size()) {
		return {};
	}
	return slot_tiles_[slot];
}

std::span<const uint32_t> DistributionScheduler::local_tiles() const noexcept {
	return slot_tiles(local_slot_);
}

bool DistributionScheduler::mark_dispatched(
	uint32_t tile_index,
	uint32_t node_id,
	ClusterTimePoint now,
	uint32_t pixel_count,
	const DispatchTiming& timing
) {
	if (node_id == kLocalNodeIdentifier) {
		return false;
	}
	std::lock_guard<std::mutex> lock(mutex_);
	if (tile_index >= tile_count_.load(std::memory_order_acquire) || owner_[tile_index] != node_id) {
		return false;
	}
	uint8_t expected = code(TileStatus::Queued);
	if (!status_at(tile_index).compare_exchange_strong(expected, code(TileStatus::InFlight), std::memory_order_acq_rel)) {
		return false;
	}
	const double margin = (std::isfinite(timing.deadline_margin_seconds) && timing.deadline_margin_seconds > 0.0) ? timing.deadline_margin_seconds : 0.0;
	const double estimate = estimate_tile_compute_seconds(pixel_count, cost_[tile_index], timing.seconds_per_kilo_ray);
	const double timeout = std::clamp(margin + estimate, parameters_.minimum_timeout_seconds, parameters_.maximum_timeout_seconds);
	dispatch_time_[tile_index] = now;
	deadline_[tile_index] = now + std::chrono::duration_cast<ClusterClock::duration>(std::chrono::duration<double>(timeout));
	in_flight_.push_back(tile_index);
	dispatched_.fetch_add(1, std::memory_order_relaxed);
	return true;
}

TileCompletionResult DistributionScheduler::complete_remote(uint64_t frame_index, uint32_t tile_index, uint32_t node_id) {
	if (frame_index != frame_index_.load(std::memory_order_acquire)) {
		stale_discarded_.fetch_add(1, std::memory_order_relaxed);
		return TileCompletionResult::StaleFrame;
	}
	std::lock_guard<std::mutex> lock(mutex_);
	if (tile_index >= tile_count_.load(std::memory_order_acquire)) {
		return TileCompletionResult::UnknownTile;
	}
	if (owner_[tile_index] != node_id) {
		return TileCompletionResult::NotOwner;
	}
	std::atomic<uint8_t>& status = status_at(tile_index);
	uint8_t observed = status.load(std::memory_order_acquire);
	while (observed == code(TileStatus::InFlight) || observed == code(TileStatus::Reclaimed)) {
		if (status.compare_exchange_weak(observed, code(TileStatus::CompletedRemote), std::memory_order_acq_rel, std::memory_order_acquire)) {
			completed_tiles_.fetch_add(1, std::memory_order_acq_rel);
			completed_remote_.fetch_add(1, std::memory_order_relaxed);
			return TileCompletionResult::Accepted;
		}
	}
	if (observed == code(TileStatus::CompletedLocal) || observed == code(TileStatus::CompletedRemote)) {
		late_discarded_.fetch_add(1, std::memory_order_relaxed);
		return TileCompletionResult::DuplicateIgnored;
	}
	return TileCompletionResult::NotDispatched;
}

bool DistributionScheduler::should_compute_local(uint64_t frame_index, uint32_t tile_index) const noexcept {
	if (frame_index != frame_index_.load(std::memory_order_acquire) || tile_index >= tile_count_.load(std::memory_order_acquire)) {
		return false;
	}
	const uint8_t observed = status_at(tile_index).load(std::memory_order_acquire);
	return observed == code(TileStatus::Queued) || observed == code(TileStatus::Reclaimed);
}

bool DistributionScheduler::try_complete_local(uint64_t frame_index, uint32_t tile_index) noexcept {
	if (frame_index != frame_index_.load(std::memory_order_acquire) || tile_index >= tile_count_.load(std::memory_order_acquire)) {
		return false;
	}
	std::atomic<uint8_t>& status = status_at(tile_index);
	uint8_t observed = status.load(std::memory_order_acquire);
	while (observed == code(TileStatus::Queued) || observed == code(TileStatus::Reclaimed)) {
		if (status.compare_exchange_weak(observed, code(TileStatus::CompletedLocal), std::memory_order_acq_rel, std::memory_order_acquire)) {
			completed_tiles_.fetch_add(1, std::memory_order_acq_rel);
			completed_local_.fetch_add(1, std::memory_order_relaxed);
			return true;
		}
	}
	return false;
}

void DistributionScheduler::count_reclaim(ReclaimReason reason) noexcept {
	switch (reason) {
		case ReclaimReason::Timeout:
			reclaimed_timeout_.fetch_add(1, std::memory_order_relaxed);
			break;
		case ReclaimReason::Speculative:
			reclaimed_speculative_.fetch_add(1, std::memory_order_relaxed);
			break;
		case ReclaimReason::NodeLost:
			reclaimed_node_lost_.fetch_add(1, std::memory_order_relaxed);
			break;
	}
}

bool DistributionScheduler::try_reclaim_unlocked(uint32_t tile_index, ReclaimReason reason, std::vector<ReclaimedTile>& output) {
	uint8_t expected = code(TileStatus::InFlight);
	if (!status_at(tile_index).compare_exchange_strong(expected, code(TileStatus::Reclaimed), std::memory_order_acq_rel)) {
		return false;
	}
	output.push_back(ReclaimedTile{tile_index, owner_[tile_index], reason});
	count_reclaim(reason);
	return true;
}

size_t DistributionScheduler::collect_expired(ClusterTimePoint now, std::vector<ReclaimedTile>& output) {
	std::lock_guard<std::mutex> lock(mutex_);
	const size_t before = output.size();
	size_t write = 0;
	for (size_t read = 0; read < in_flight_.size(); ++read) {
		const uint32_t tile = in_flight_[read];
		if (status_at(tile).load(std::memory_order_acquire) != code(TileStatus::InFlight)) {
			continue;
		}
		if (now >= deadline_[tile] && try_reclaim_unlocked(tile, ReclaimReason::Timeout, output)) {
			continue;
		}
		in_flight_[write++] = tile;
	}
	in_flight_.resize(write);
	return output.size() - before;
}

size_t DistributionScheduler::reclaim_node(uint32_t node_id, std::vector<ReclaimedTile>& output) {
	std::lock_guard<std::mutex> lock(mutex_);
	const size_t before = output.size();
	size_t write = 0;
	for (size_t read = 0; read < in_flight_.size(); ++read) {
		const uint32_t tile = in_flight_[read];
		if (status_at(tile).load(std::memory_order_acquire) != code(TileStatus::InFlight)) {
			continue;
		}
		if (owner_[tile] == node_id && try_reclaim_unlocked(tile, ReclaimReason::NodeLost, output)) {
			continue;
		}
		in_flight_[write++] = tile;
	}
	in_flight_.resize(write);
	return output.size() - before;
}

size_t DistributionScheduler::steal_speculative(ClusterTimePoint now, size_t maximum_count, std::vector<ReclaimedTile>& output) {
	if (maximum_count == 0U) {
		return 0;
	}
	std::lock_guard<std::mutex> lock(mutex_);
	if (!parameters_.work_stealing) {
		return 0;
	}
	candidates_.clear();
	for (const uint32_t tile : in_flight_) {
		if (status_at(tile).load(std::memory_order_acquire) != code(TileStatus::InFlight)) {
			continue;
		}
		const double window = seconds_between(dispatch_time_[tile], deadline_[tile]);
		const double elapsed = seconds_between(dispatch_time_[tile], now);
		if (window > 0.0 && elapsed >= parameters_.speculative_fraction * window) {
			candidates_.emplace_back(dispatch_time_[tile], tile);
		}
	}
	std::sort(candidates_.begin(), candidates_.end());
	const size_t before = output.size();
	for (const auto& [dispatched_at, tile] : candidates_) {
		if (output.size() - before >= maximum_count) {
			break;
		}
		try_reclaim_unlocked(tile, ReclaimReason::Speculative, output);
	}
	size_t write = 0;
	for (size_t read = 0; read < in_flight_.size(); ++read) {
		if (status_at(in_flight_[read]).load(std::memory_order_acquire) == code(TileStatus::InFlight)) {
			in_flight_[write++] = in_flight_[read];
		}
	}
	in_flight_.resize(write);
	return output.size() - before;
}

TileStatus DistributionScheduler::tile_status(uint32_t tile_index) const noexcept {
	if (tile_index >= tile_count_.load(std::memory_order_acquire)) {
		return TileStatus::Skipped;
	}
	return static_cast<TileStatus>(status_at(tile_index).load(std::memory_order_acquire));
}

bool DistributionScheduler::is_frame_complete() const noexcept {
	return completed_tiles_.load(std::memory_order_acquire) >= expected_completions_.load(std::memory_order_acquire);
}

uint32_t DistributionScheduler::remaining_tiles() const noexcept {
	const uint32_t expected = expected_completions_.load(std::memory_order_acquire);
	const uint32_t completed = completed_tiles_.load(std::memory_order_acquire);
	return (completed >= expected) ? 0U : (expected - completed);
}

uint32_t DistributionScheduler::in_flight_count() const {
	std::lock_guard<std::mutex> lock(mutex_);
	return static_cast<uint32_t>(std::count_if(in_flight_.begin(), in_flight_.end(), [this](uint32_t tile) {
		return status_at(tile).load(std::memory_order_acquire) == code(TileStatus::InFlight);
	}));
}

SchedulerStatistics DistributionScheduler::statistics() const noexcept {
	SchedulerStatistics result{};
	result.frames_planned = frames_planned_.load(std::memory_order_relaxed);
	result.tiles_assigned_local = assigned_local_.load(std::memory_order_relaxed);
	result.tiles_assigned_remote = assigned_remote_.load(std::memory_order_relaxed);
	result.tiles_dispatched = dispatched_.load(std::memory_order_relaxed);
	result.tiles_completed_local = completed_local_.load(std::memory_order_relaxed);
	result.tiles_completed_remote = completed_remote_.load(std::memory_order_relaxed);
	result.tiles_reclaimed_timeout = reclaimed_timeout_.load(std::memory_order_relaxed);
	result.tiles_reclaimed_speculative = reclaimed_speculative_.load(std::memory_order_relaxed);
	result.tiles_reclaimed_node_lost = reclaimed_node_lost_.load(std::memory_order_relaxed);
	result.late_results_discarded = late_discarded_.load(std::memory_order_relaxed);
	result.stale_results_discarded = stale_discarded_.load(std::memory_order_relaxed);
	return result;
}

}

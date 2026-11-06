#include "support/test_harness.hpp"
#include "relativistic/distributed/cluster_node.hpp"
#include "relativistic/distributed/distribution_scheduler.hpp"
#include "relativistic/distributed/node_registry.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <numeric>
#include <thread>
#include <vector>

using namespace Relativistic;
using namespace Relativistic::Distributed;
using namespace Relativistic::Testing;

int main() {
	TestSuite suite(
		"cluster_scheduler_stealing",
		"Validates capacity-weighted tile partitioning conservation, dynamic load balancing convergence, speculative work stealing on deadlines, and thread-safe scheduler node transitions."
	);

	suite.section("Tile Partition Conservation Across Resolutions");
	{
		DistributionScheduler scheduler;
		const std::vector<std::pair<uint32_t, uint32_t>> resolutions = {
			{3840, 2160},
			{1920, 1080},
			{1280, 720},
			{640, 480}
		};

		std::vector<NodeCapacityInput> nodes(4);
		nodes[0] = NodeCapacityInput{1, false, true, 2.0, 1.0e-3, 0.005};
		nodes[1] = NodeCapacityInput{2, false, true, 1.5, 1.2e-3, 0.015};
		nodes[2] = NodeCapacityInput{3, false, true, 0.8, 2.0e-3, 0.035};
		nodes[3] = NodeCapacityInput{0, true, true, 1.0, 1.0e-3, 0.0};

		for (const auto& [w, h] : resolutions) {
			const uint32_t cols = tile_grid_columns(w);
			const uint32_t rows = tile_grid_rows(h);
			const uint32_t total = cols * rows;

			const bool ok = scheduler.begin_frame(1, total, {}, nodes);
			suite.check(ok, "scheduler begins frame successfully");

			uint32_t sum_quotas = 0;
			for (size_t s = 0; s < scheduler.slot_count(); ++s) {
				const auto alloc = scheduler.slot_allocation(s);
				sum_quotas += alloc.quota;
				suite.check(scheduler.slot_tiles(s).size() == alloc.quota, "slot tile assignment matches allocated quota");
			}
			suite.check(sum_quotas == total, "sum of node tile quotas exactly equals total active tiles");
			suite.check(scheduler.remaining_tiles() == total, "initial remaining tiles equals total");
		}
	}

	suite.section("Capacity Share Convergence & Latency Dampening");
	{
		std::vector<NodeCapacityInput> nodes(3);
		nodes[0] = NodeCapacityInput{1, false, true, 1.0, 1.0e-3, 0.002};
		nodes[1] = NodeCapacityInput{2, false, true, 2.0, 0.5e-3, 0.025};
		nodes[2] = NodeCapacityInput{3, false, true, 1.0, 2.0e-3, 0.080};

		std::vector<double> shares(3, 0.0);
		const double alpha = 1.0;
		const bool ok = compute_capacity_shares(nodes, alpha, true, shares);
		suite.check(ok, "capacity shares compute successfully");

		const double sum = std::accumulate(shares.begin(), shares.end(), 0.0);
		suite.check_near(sum, 1.0, 1.0e-12, "capacity shares are strictly normalized to 1.0");

		auto analytical_score = [](const NodeCapacityInput& n, double a) {
			const double inv_exec = 1.0 / n.seconds_per_kilo_ray;
			const double rtt_ratio = std::max(n.round_trip_seconds / kLatencyReferenceSeconds, 1.0);
			const double penalty = std::pow(rtt_ratio, -a);
			return n.static_weight * inv_exec * penalty;
		};

		const double s0 = analytical_score(nodes[0], alpha);
		const double s1 = analytical_score(nodes[1], alpha);
		const double s2 = analytical_score(nodes[2], alpha);
		const double s_tot = s0 + s1 + s2;

		suite.check_relative(shares[0], s0 / s_tot, 1.0e-10, "node 0 capacity share matches analytical formulation");
		suite.check_relative(shares[1], s1 / s_tot, 1.0e-10, "node 1 capacity share matches analytical formulation");
		suite.check_relative(shares[2], s2 / s_tot, 1.0e-10, "node 2 capacity share matches analytical formulation");
	}

	suite.section("Deadline Timeout Reclamation & Work Stealing");
	{
		LoadBalancerParameters params{};
		params.minimum_timeout_seconds = 0.05;
		params.maximum_timeout_seconds = 0.10;
		params.work_stealing = true;
		params.speculative_fraction = 0.5;

		DistributionScheduler scheduler(params);
		std::vector<NodeCapacityInput> nodes(2);
		nodes[0] = NodeCapacityInput{1, false, true, 1.0, 1.0e-3, 0.010};
		nodes[1] = NodeCapacityInput{0, true, true, 1.0, 1.0e-3, 0.0};

		const uint32_t total_tiles = 100;
		const bool frame_ok = scheduler.begin_frame(42, total_tiles, {}, nodes);
		suite.check(frame_ok, "Scheduler initializes frame with remote and local nodes");

		const ClusterTimePoint t0 = ClusterClock::now();
		DispatchTiming timing{0.05, 1.0e-3};

		const auto remote_tiles = scheduler.slot_tiles(0);
		suite.check(!remote_tiles.empty(), "remote slot receives non-empty tile quota");
		const uint32_t test_tile = remote_tiles.front();

		const bool dispatched = scheduler.mark_dispatched(test_tile, 1, t0, 1024, timing);
		suite.check(dispatched, "mark dispatched succeeds for remote tile");
		suite.check(scheduler.tile_status(test_tile) == TileStatus::InFlight, "tile status is InFlight after dispatch");

		std::vector<ReclaimedTile> reclaimed;
		const size_t early_reclaims = scheduler.collect_expired(t0 + std::chrono::milliseconds(20), reclaimed);
		suite.check(early_reclaims == 0, "no tiles reclaimed prior to deadline expiration");

		const size_t expired_reclaims = scheduler.collect_expired(t0 + std::chrono::milliseconds(150), reclaimed);
		suite.check(expired_reclaims == 1, "stalled tile successfully reclaimed after deadline expiration");
		suite.check(reclaimed.front().tile_index == test_tile, "reclaimed tile index matches target");
		suite.check(reclaimed.front().reason == ReclaimReason::Timeout, "reclaim reason is Timeout");
		suite.check(scheduler.tile_status(test_tile) == TileStatus::Reclaimed, "tile status transitioned to Reclaimed");

		suite.check(scheduler.should_compute_local(42, test_tile), "reclaimed tile qualified for immediate local execution");
		const bool local_done = scheduler.try_complete_local(42, test_tile);
		suite.check(local_done, "local completion of stolen tile succeeds");
		suite.check(scheduler.tile_status(test_tile) == TileStatus::CompletedLocal, "tile status is CompletedLocal");

		const TileCompletionResult late_res = scheduler.complete_remote(42, test_tile, 1);
		suite.check(late_res == TileCompletionResult::DuplicateIgnored, "late remote response ignored without double count");
		suite.check(scheduler.statistics().late_results_discarded == 1, "late discard telemetry incremented");
	}

	suite.section("Concurrent Peer Registry Access");
	{
		NodeRegistry registry;
		std::atomic<bool> start_gate{false};
		std::atomic<size_t> failures{0};

		auto worker = [&](uint32_t thread_id) {
			while (!start_gate.load(std::memory_order_relaxed)) {
				std::this_thread::yield();
			}
			for (size_t i = 0; i < 200; ++i) {
				ClusterNodeDescriptor desc{};
				desc.name = "node_" + std::to_string(thread_id) + "_" + std::to_string(i);
				desc.endpoint.host = "10.0." + std::to_string(thread_id) + "." + std::to_string(i + 1);
				desc.endpoint.port = static_cast<uint16_t>(5000 + i);

				const auto id = registry.register_node(desc);
				if (!id.has_value()) {
					failures.fetch_add(1, std::memory_order_relaxed);
					continue;
				}
				static_cast<void>(registry.bind_peer(*id, static_cast<uint32_t>(thread_id * 1000 + i), ClusterClock::now()));
				registry.visit(*id, [](ClusterNode& n) {
					n.record_round_trip(0.005);
				});
				static_cast<void>(registry.release_peer(static_cast<uint32_t>(thread_id * 1000 + i), ClusterClock::now()));
				static_cast<void>(registry.unregister_node(*id));
			}
		};

		std::vector<std::jthread> threads;
		for (uint32_t t = 0; t < 8; ++t) {
			threads.emplace_back(worker, t);
		}
		start_gate.store(true, std::memory_order_release);
		for (auto& th : threads) {
			th.join();
		}

		suite.check(failures.load() == 0, "no registration failures under concurrent multi-threaded stress");
		suite.check(registry.size() == 0, "all nodes cleaned up symmetrically");
	}

	return suite.finish();
}

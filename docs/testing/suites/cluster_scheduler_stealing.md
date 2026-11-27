# Cluster scheduler and work stealing suite

Executable: `test_cluster_scheduler_stealing` - Source: `tests/test_cluster_scheduler_stealing.cpp` - Results: [cluster_scheduler_stealing.md](../results/cluster_scheduler_stealing.md)

## Purpose

Verifies `DistributionScheduler` and `NodeRegistry` ([ARCHITECTURE.md](../../ARCHITECTURE.md#36-distributed-workload-scheduling--work-stealing)): capacity-weighted tile quota conservation across raster resolutions, analytical convergence of latency-damped capacity shares, deadline-based speculative work stealing, and thread-safe peer registry lifecycle transitions.

## Scenario

Resolutions 3840x2160, 1920x1080, 1280x720, and 640x480 across four nodes; dynamic capacity shares computed with damping exponent $\alpha = 1.0$; load balancing with timeout range $[0.05, 0.10]\,\text{s}$ and 100 tiles; 8 worker threads executing 200 concurrent node registration and deregistration cycles.

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Tile Partition Conservation Across Resolutions | Frame start | Scheduler begins frame successfully |
| Tile Partition Conservation Across Resolutions | Quota conservation | Node quotas sum exactly to total tiles across all resolutions |
| Tile Partition Conservation Across Resolutions | Assignments | Tile count per slot equals allocated quota |
| Capacity Share Convergence & Latency Dampening | Normalization | Capacity shares sum to 1.0 within 1e-12 |
| Capacity Share Convergence & Latency Dampening | Convergence | Node capacity shares match analytical formula within a relative 1e-10 |
| Deadline Timeout Reclamation & Work Stealing | Dispatch | Mark dispatched succeeds and status transitions to InFlight |
| Deadline Timeout Reclamation & Work Stealing | Deadline expiration | Zero reclaims prior to deadline; single reclaim after expiration |
| Deadline Timeout Reclamation & Work Stealing | Reclaim state | Reason is Timeout and status transitions to Reclaimed |
| Deadline Timeout Reclamation & Work Stealing | Local execution | Reclaimed tile qualifies for local compute and completes |
| Deadline Timeout Reclamation & Work Stealing | Telemetry | Late remote result ignored without double count |
| Concurrent Peer Registry Access | Concurrency | Zero registration failures and symmetric cleanup under 8-thread stress |

## Justification

Quota partitioning must be conservative to prevent dropped or orphaned screen tiles. Capacity shares follow the closed-form weight and latency model of [MATHEMATICAL_FORMULATION.md Section 10.1](../../MATHEMATICAL_FORMULATION.md#101-latency-damped-capacity-share-formulation), bounded by 1e-10 relative error.

## Acceptance

All checks pass.

## Recorded results

See [../results/cluster_scheduler_stealing.md](../results/cluster_scheduler_stealing.md).

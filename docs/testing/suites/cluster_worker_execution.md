# Cluster worker execution suite

Executable: `test_cluster_worker_execution` - Source: `tests/test_cluster_worker_execution.cpp` - Results: [cluster_worker_execution.md](../results/cluster_worker_execution.md)

## Purpose

Verifies mathematical parity between `TileRenderer` execution and local scalar dispatch on Kerr black hole spacetimes with accretion disks, cooperative task cancellation latency, and sustained execution stability ([TECHNICAL_MANUAL.md](../../TECHNICAL_MANUAL.md#213-distributed-computing--cluster-engine)).

## Scenario

A 384x384 viewport on a Kerr metric ($M = 1.0, a = 0.94$) with an accretion disk extending to $18 M$, 1024 max steps, step factor 0.45; tile rendered via `TileRenderer` with CPU backend; cooperative cancellation tested via atomic flag set after $200\,\mu\text{s}$; 500 consecutive tiles executed in a sustained loop.

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Geometry & Setup | Tile resolution | Test tile geometry resolves within screen bounds |
| Local Reference Integration | Local dispatch | Local reference tile dispatches successfully |
| Worker Tile Renderer Parity | Initialization | Worker tile renderer initializes and task header is valid |
| Worker Tile Renderer Parity | Outcome | Worker tile completes without error and without cancellation |
| Worker Tile Renderer Parity | Radiance parity | Radiance matches local reference within a relative 1e-12 |
| Worker Tile Renderer Parity | Redshift parity | Redshift matches local reference within a relative 1e-12 |
| Worker Tile Renderer Parity | Status flags | Status flags match local reference on every pixel |
| Task Cancellation Latency | Responsiveness | Cancellation latency remains under 15 ms |
| Sustained Pipeline Stress & Stability | Reliability | All 500 stress tiles complete without memory fault or crash |

## Justification

Tile integration on worker nodes evaluates the same geodesic differential equations as local dispatch; relative discrepancies must not exceed 1e-12. Real-time viewport responsiveness requires cooperative cancellation to halt within 15 ms.

## Measurements

Maximum radiance relative error (`max_radiance_relative_error`), maximum redshift relative error (`max_redshift_relative_error`), cancellation latency (`cancellation_latency`, ms), and worker throughput (`worker_throughput`, MPx/s).

## Acceptance

All checks pass.

## Recorded results

See [../results/cluster_worker_execution.md](../results/cluster_worker_execution.md).

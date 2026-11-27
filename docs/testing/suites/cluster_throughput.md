# Cluster throughput benchmark

Executable: `benchmark_cluster_throughput` - Source: `tests/benchmark_cluster_throughput.cpp` - Results: [cluster_throughput.md](../results/cluster_throughput.md)

## Purpose

Benchmarks distributed geodesic rendering throughput, parallel multi-worker scaling efficiency, and wire codec compression bandwidth across simulated cluster topologies ([ARCHITECTURE.md](../../ARCHITECTURE.md#214-distributed-compute-subsystem--heterogeneous-cluster-architecture)).

## Scenario

A high-spin Kerr black hole ($M = 1.0, a = 0.998$) with a primary accretion disk, 60-degree field of view, observer at $r = 16.0$, 256x256 resolution (512x512 under `--profile full`), 5 warmup iterations, 15 measured iterations; cluster orchestrator running two CPU worker services with 2 threads each; 8192-pixel tile compressed over 5000 iterations for DirectRaw and LZ4Block.

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Correctness & Invariant Validation | Physical validity | All output pixels have finite radiances |
| Correctness & Invariant Validation | Ray classification | Kerr shadow produces absorbed rays and escape rays reach sky |
| Baseline Local Throughput | Operational bound | Local throughput exceeds 0.1 MPx/s |
| Multi-Worker Scaling Simulation | Orchestration | Master orchestrator and worker nodes start successfully |
| Multi-Worker Scaling Simulation | Performance | Cluster dispatch operates with positive throughput (> 0.1 MPx/s) |
| Compression Codec Bandwidth Comparison | LZ4 throughput | Real-time compression throughput exceeds 300 MB/s |

## Justification

Correctness verification is strictly decoupled from timing: non-finite values or zero horizon hits indicate physical or numerical failures. The 0.1 MPx/s baseline and 300 MB/s LZ4 compression thresholds represent operational floors preventing throughput regressions.

## Measurements

Baseline throughput (`baseline_throughput`, MPx/s), baseline frame time (`baseline_frame_time`, ms), cluster throughput (`cluster_throughput`, MPx/s), cluster speedup factor (`cluster_speedup_factor`, ratio), DirectRaw throughput (`codec_direct_raw_throughput`, MB/s), and LZ4 compression throughput (`codec_lz4_compression_throughput`, MB/s).

## Acceptance

All checks pass.

## Recorded results

See [../results/cluster_throughput.md](../results/cluster_throughput.md).

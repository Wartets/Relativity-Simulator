# cluster_throughput

Benchmarks distributed geodesic rendering throughput, parallel scaling efficiency, load balancing, and wire codec performance across simulated cluster topologies.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 9 |
| Failed checks | 0 |
| Duration | 14.14 s |
| Generated | 2026-11-07 22:50:44 UTC |

## Correctness & Invariant Validation (Decoupled from Timing)

| Result | Check | Detail |
| --- | --- | --- |
| PASS | all output pixels have finite physical radiances |  |
| PASS | kerr black hole shadow produces non-zero absorbed rays |  |
| PASS | escape rays reach celestial background |  |

## Baseline Local Throughput

| Result | Check | Detail |
| --- | --- | --- |
| PASS | baseline throughput exceeds operational threshold |  |

## Multi-Worker Scaling Simulation (1, 2, 4, 8 Workers)

| Result | Check | Detail |
| --- | --- | --- |
| PASS | cluster orchestrator starts successfully |  |
| PASS | worker node started successfully |  |
| PASS | worker node started successfully |  |
| PASS | cluster dispatch operates with positive throughput |  |

## Compression Codec Bandwidth Comparison

| Result | Check | Detail |
| --- | --- | --- |
| PASS | lz4 real-time compression exceeds 300 MB/s |  |

## Measurements

| Section | Name | Value | Unit |
| --- | --- | --- | --- |
| Baseline Local Throughput | baseline_throughput | 0.217896617 | MPx/s |
| Baseline Local Throughput | baseline_frame_time | 300.766487 | ms |
| Multi-Worker Scaling Simulation (1, 2, 4, 8 Workers) | cluster_throughput | 0.241370668 | MPx/s |
| Multi-Worker Scaling Simulation (1, 2, 4, 8 Workers) | cluster_speedup_factor | 1.10773022 | ratio |
| Compression Codec Bandwidth Comparison | codec_direct_raw_throughput | 9674922.6 | MB/s |
| Compression Codec Bandwidth Comparison | codec_lz4_compression_throughput | 7654623.39 | MB/s |


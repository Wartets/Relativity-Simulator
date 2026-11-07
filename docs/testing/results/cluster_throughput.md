# cluster_throughput

Benchmarks distributed geodesic rendering throughput, parallel scaling efficiency, load balancing, and wire codec performance across simulated cluster topologies.

| Property | Value |
| --- | --- |
| Status | FAILED |
| Checks | 6 |
| Failed checks | 1 |
| Duration | 9.882 s |
| Generated | 2026-11-07 18:01:35 UTC |

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
| PASS | cluster dispatch operates with positive throughput |  |

## Compression Codec Bandwidth Comparison

| Result | Check | Detail |
| --- | --- | --- |
| FAIL | lz4 real-time compression exceeds 300 MB/s |  |

## Measurements

| Section | Name | Value | Unit |
| --- | --- | --- | --- |
| Baseline Local Throughput | baseline_throughput | 0.21383682 | MPx/s |
| Baseline Local Throughput | baseline_frame_time | 306.476687 | ms |
| Multi-Worker Scaling Simulation (1, 2, 4, 8 Workers) | cluster_throughput | 1638400 | MPx/s |
| Multi-Worker Scaling Simulation (1, 2, 4, 8 Workers) | cluster_speedup_factor | 7661917.17 | ratio |
| Compression Codec Bandwidth Comparison | codec_direct_raw_throughput | 22745.8002 | MB/s |
| Compression Codec Bandwidth Comparison | codec_lz4_compression_throughput | 100.734047 | MB/s |


# engine_performance

Large-scale performance and capability benchmark of the rendering engine. It sweeps spacetime metrics, celestial body counts, secondary black hole counts, performance presets and space skipping on the CPU and GPU compute paths, and reports frame times, throughput, ray statistics, scaling exponents and capability envelopes.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 4 |
| Failed checks | 0 |
| Duration | 16.44 s |
| Generated | 2026-10-19 22:14:46 UTC |

## Notes

- Detailed reports written to D:/Documents/Github/Relativity-Simulator/docs/testing/results/performance

## Environment

| Result | Check | Detail |
| --- | --- | --- |
| PASS | at least one metric scenario is selected |  |
| PASS | the scenario matrix is not empty | cases=16 |

## Per-case sanity

| Result | Check | Detail |
| --- | --- | --- |
| PASS | all 16 executed cases satisfy the sanity invariants | violations=0 |

## CPU and GPU image parity

| Result | Check | Detail |
| --- | --- | --- |
| PASS | CPU and GPU renderings of the reference scene do not diverge grossly | value=0.00105612 limit=0.25 |

## Measurements

| Section | Name | Value | Unit |
| --- | --- | --- | --- |
| Environment | hardware_threads | 12 | threads |
| Environment | gpu_available | 1 | boolean |
| Environment | scenario_count | 16 | cases |
| Environment | benchmark_wall_time | 15.3513994 | s |
| CPU and GPU image parity | mean_absolute_rgb_difference_cpu_vs_gpu | 0.00105611965 | linear sRGB units |
| CPU and GPU image parity | parity_frame_cpu_time | 70.8818 | ms |
| CPU and GPU image parity | parity_frame_gpu_time | 3.0477 | ms |


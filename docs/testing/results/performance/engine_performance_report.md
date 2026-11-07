# Engine performance report

| Property | Value |
| --- | --- |
| Generated | 2026-11-07 18:10:26 UTC |
| Profile | quick |
| Cases executed | 16 |
| Total duration | 15.323 s |
| Hardware threads | 12 |
| GPU compute available | yes |
| GPU device | NVIDIA GeForce GTX 1060 |
| Base width at scale 1.0 | 160 px |
| Timed repetitions | 1 |
| Ray step cap | 1024 |
| Projection mode | 0 |

## Capability envelope on Schwarzschild

Largest total body count whose median frame time stays within the target, with no secondary black hole and no space skipping.

| Path | Preset | Resolution | >= 30 FPS | >= 10 FPS | >= 1 FPS |
| --- | --- | --- | --- | --- | --- |
| CPU | 1 | 80x45 | none | 0 | 16 |
| GPU | 1 | 80x45 | 16 | 16 | 16 |

## Metric cost without bodies

Median frame time per metric, no bodies, no secondary black hole, no space skipping.

| Metric | Preset | CPU ms | GPU ms | GPU speedup |
| --- | --- | --- | --- | --- |
| Schwarzschild | 1 | 93.594 | 2.7285 | 34.3x |
| Kerr | 1 | 128.29 | 8.5599 | 14.99x |

## Body count scaling

Exponent of the power law fitted to frame time against (1 + total bodies); 1 means linear growth.

| Metric | Path | Preset | Exponent | Time at lowest count (ms) | Time at highest count (ms) |
| --- | --- | --- | --- | --- | --- |

## Secondary black hole cost

Additional median frame time per secondary black hole, with no celestial bodies.

| Metric | Path | Preset | Black holes | Baseline ms | Loaded ms | Cost per black hole (ms) |
| --- | --- | --- | --- | --- | --- | --- |
| Schwarzschild | CPU | 1 | 1 | 93.594 | 2854.4 | 2760.8 |
| Schwarzschild | GPU | 1 | 1 | 2.7285 | 8.5111 | 5.7826 |
| Kerr | CPU | 1 | 1 | 128.29 | 183.67 | 55.374 |
| Kerr | GPU | 1 | 1 | 8.5599 | 123.83 | 115.27 |

## Space skipping gain

| Metric | Path | Preset | Bodies | Black holes | Without ms | With ms | Gain |
| --- | --- | --- | --- | --- | --- | --- | --- |

## Throughput leaderboard

| Rank | Case | Resolution | MPix/s | Median ms |
| --- | --- | --- | --- | --- |
| 1 | Schwarzschild bodies=0 black_holes=0 preset=1 skip=off path=gpu | 80x45 | 1.3194 | 2.7285 |
| 2 | Schwarzschild bodies=0 black_holes=1 preset=1 skip=off path=gpu | 80x45 | 0.42298 | 8.5111 |
| 3 | Schwarzschild bodies=16 black_holes=0 preset=1 skip=off path=gpu | 80x45 | 0.4214 | 8.5429 |
| 4 | Kerr bodies=0 black_holes=0 preset=1 skip=off path=gpu | 80x45 | 0.42057 | 8.5599 |
| 5 | Schwarzschild bodies=16 black_holes=1 preset=1 skip=off path=gpu | 80x45 | 0.26478 | 13.596 |
| 6 | Schwarzschild bodies=0 black_holes=0 preset=1 skip=off path=cpu | 80x45 | 0.038464 | 93.594 |
| 7 | Schwarzschild bodies=16 black_holes=0 preset=1 skip=off path=cpu | 80x45 | 0.030698 | 117.27 |
| 8 | Kerr bodies=0 black_holes=1 preset=1 skip=off path=gpu | 80x45 | 0.029072 | 123.83 |
| 9 | Kerr bodies=0 black_holes=0 preset=1 skip=off path=cpu | 80x45 | 0.028061 | 128.29 |
| 10 | Kerr bodies=0 black_holes=1 preset=1 skip=off path=cpu | 80x45 | 0.019601 | 183.67 |

## Slowest cases

| Rank | Case | Resolution | Median ms | Average iterations |
| --- | --- | --- | --- | --- |
| 1 | Schwarzschild bodies=16 black_holes=1 preset=1 skip=off path=cpu | 80x45 | 3050.3 | 9.5456 |
| 2 | Schwarzschild bodies=0 black_holes=1 preset=1 skip=off path=cpu | 80x45 | 2854.4 | 13.289 |
| 3 | Kerr bodies=16 black_holes=0 preset=1 skip=off path=cpu | 80x45 | 216.64 | 12.197 |
| 4 | Kerr bodies=16 black_holes=0 preset=1 skip=off path=gpu | 80x45 | 207.66 | 12.197 |
| 5 | Kerr bodies=16 black_holes=1 preset=1 skip=off path=cpu | 80x45 | 204.25 | 12.294 |
| 6 | Kerr bodies=16 black_holes=1 preset=1 skip=off path=gpu | 80x45 | 203.05 | 12.294 |
| 7 | Kerr bodies=0 black_holes=1 preset=1 skip=off path=cpu | 80x45 | 183.67 | 12.378 |
| 8 | Kerr bodies=0 black_holes=0 preset=1 skip=off path=cpu | 80x45 | 128.29 | 12.378 |
| 9 | Kerr bodies=0 black_holes=1 preset=1 skip=off path=gpu | 80x45 | 123.83 | 12.378 |
| 10 | Schwarzschild bodies=16 black_holes=0 preset=1 skip=off path=cpu | 80x45 | 117.27 | 12.866 |

## Complete results

| Metric | Bodies | BH | Preset | Skip | Path | Resolution | Steps | Median ms | Min ms | Max ms | Stddev ms | MPix/s | Avg iter | Horizon % | Disk % | Body % | Saturated % |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Schwarzschild | 0 | 0 | 1 | off | CPU | 80x45 | 1024 | 93.594 | 93.594 | 93.594 | 0 | 0.038464 | 12.385 | 5.889 | 73.39 | 0 | 0 |
| Schwarzschild | 0 | 0 | 1 | off | GPU | 80x45 | 1024 | 2.7285 | 2.7285 | 2.7285 | 0 | 1.3194 | 13.272 | 5.889 | 1.472 | 0 | 1.472 |
| Schwarzschild | 0 | 1 | 1 | off | CPU | 80x45 | 1024 | 2854.4 | 2854.4 | 2854.4 | 0 | 0.0012612 | 13.289 | 5.861 | 1.556 | 0 | 1.556 |
| Schwarzschild | 0 | 1 | 1 | off | GPU | 80x45 | 1024 | 8.5111 | 8.5111 | 8.5111 | 0 | 0.42298 | 13.311 | 5.917 | 1.528 | 0 | 1.528 |
| Schwarzschild | 16 | 0 | 1 | off | CPU | 80x45 | 1024 | 117.27 | 117.27 | 117.27 | 0 | 0.030698 | 12.866 | 5.889 | 15.58 | 2.25 | 3.722 |
| Schwarzschild | 16 | 0 | 1 | off | GPU | 80x45 | 1024 | 8.5429 | 8.5429 | 8.5429 | 0 | 0.4214 | 13.064 | 5.889 | 3.611 | 2.278 | 3.75 |
| Schwarzschild | 16 | 1 | 1 | off | CPU | 80x45 | 1024 | 3050.3 | 3050.3 | 3050.3 | 0 | 0.0011802 | 9.5456 | 5.111 | 35.39 | 0.7778 | 35.97 |
| Schwarzschild | 16 | 1 | 1 | off | GPU | 80x45 | 1024 | 13.596 | 13.596 | 13.596 | 0 | 0.26478 | 11.418 | 6.389 | 17.22 | 1.333 | 18.25 |
| Kerr | 0 | 0 | 1 | off | CPU | 80x45 | 1024 | 128.29 | 128.29 | 128.29 | 0 | 0.028061 | 12.378 | 1.861 | 3.472 | 0 | 3.472 |
| Kerr | 0 | 0 | 1 | off | GPU | 80x45 | 1024 | 8.5599 | 8.5599 | 8.5599 | 0 | 0.42057 | 12.378 | 1.861 | 3.472 | 0 | 3.472 |
| Kerr | 0 | 1 | 1 | off | CPU | 80x45 | 1024 | 183.67 | 183.67 | 183.67 | 0 | 0.019601 | 12.378 | 1.861 | 3.472 | 0 | 3.472 |
| Kerr | 0 | 1 | 1 | off | CPU (fallback) | 80x45 | 1024 | 123.83 | 123.83 | 123.83 | 0 | 0.029072 | 12.378 | 1.861 | 3.472 | 0 | 3.472 |
| Kerr | 16 | 0 | 1 | off | CPU | 80x45 | 1024 | 216.64 | 216.64 | 216.64 | 0 | 0.016617 | 12.197 | 1.861 | 3.806 | 2.167 | 5.639 |
| Kerr | 16 | 0 | 1 | off | CPU (fallback) | 80x45 | 1024 | 207.66 | 207.66 | 207.66 | 0 | 0.017336 | 12.197 | 1.861 | 3.806 | 2.167 | 5.639 |
| Kerr | 16 | 1 | 1 | off | CPU | 80x45 | 1024 | 204.25 | 204.25 | 204.25 | 0 | 0.017626 | 12.294 | 1.861 | 4.222 | 1.111 | 4.583 |
| Kerr | 16 | 1 | 1 | off | CPU (fallback) | 80x45 | 1024 | 203.05 | 203.05 | 203.05 | 0 | 0.01773 | 12.294 | 1.861 | 4.222 | 1.111 | 4.583 |

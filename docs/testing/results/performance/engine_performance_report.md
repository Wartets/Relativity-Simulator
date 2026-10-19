# Engine performance report

| Property | Value |
| --- | --- |
| Generated | 2026-10-19 22:14:46 UTC |
| Profile | quick |
| Cases executed | 16 |
| Total duration | 15.351 s |
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
| Schwarzschild | 1 | 69.379 | 2.5586 | 27.12x |
| Kerr | 1 | 135.79 | 4.8662 | 27.91x |

## Body count scaling

Exponent of the power law fitted to frame time against (1 + total bodies); 1 means linear growth.

| Metric | Path | Preset | Exponent | Time at lowest count (ms) | Time at highest count (ms) |
| --- | --- | --- | --- | --- | --- |

## Secondary black hole cost

Additional median frame time per secondary black hole, with no celestial bodies.

| Metric | Path | Preset | Black holes | Baseline ms | Loaded ms | Cost per black hole (ms) |
| --- | --- | --- | --- | --- | --- | --- |
| Schwarzschild | CPU | 1 | 1 | 69.379 | 2924 | 2854.6 |
| Schwarzschild | GPU | 1 | 1 | 2.5586 | 5.6123 | 3.0537 |
| Kerr | CPU | 1 | 1 | 135.79 | 129.86 | -5.9297 |
| Kerr | GPU | 1 | 1 | 4.8662 | 144.41 | 139.54 |

## Space skipping gain

| Metric | Path | Preset | Bodies | Black holes | Without ms | With ms | Gain |
| --- | --- | --- | --- | --- | --- | --- | --- |

## Throughput leaderboard

| Rank | Case | Resolution | MPix/s | Median ms |
| --- | --- | --- | --- | --- |
| 1 | Schwarzschild bodies=0 black_holes=0 preset=1 skip=off path=gpu | 80x45 | 1.407 | 2.5586 |
| 2 | Schwarzschild bodies=16 black_holes=0 preset=1 skip=off path=gpu | 80x45 | 0.77167 | 4.6652 |
| 3 | Kerr bodies=0 black_holes=0 preset=1 skip=off path=gpu | 80x45 | 0.7398 | 4.8662 |
| 4 | Schwarzschild bodies=0 black_holes=1 preset=1 skip=off path=gpu | 80x45 | 0.64145 | 5.6123 |
| 5 | Schwarzschild bodies=16 black_holes=1 preset=1 skip=off path=gpu | 80x45 | 0.59318 | 6.069 |
| 6 | Schwarzschild bodies=0 black_holes=0 preset=1 skip=off path=cpu | 80x45 | 0.051889 | 69.379 |
| 7 | Schwarzschild bodies=16 black_holes=0 preset=1 skip=off path=cpu | 80x45 | 0.030032 | 119.87 |
| 8 | Kerr bodies=0 black_holes=1 preset=1 skip=off path=cpu | 80x45 | 0.027722 | 129.86 |
| 9 | Kerr bodies=0 black_holes=0 preset=1 skip=off path=cpu | 80x45 | 0.026511 | 135.79 |
| 10 | Kerr bodies=0 black_holes=1 preset=1 skip=off path=gpu | 80x45 | 0.024929 | 144.41 |

## Slowest cases

| Rank | Case | Resolution | Median ms | Average iterations |
| --- | --- | --- | --- | --- |
| 1 | Schwarzschild bodies=16 black_holes=1 preset=1 skip=off path=cpu | 80x45 | 3183.1 | 9.5456 |
| 2 | Schwarzschild bodies=0 black_holes=1 preset=1 skip=off path=cpu | 80x45 | 2924 | 13.289 |
| 3 | Kerr bodies=16 black_holes=1 preset=1 skip=off path=cpu | 80x45 | 257.16 | 12.294 |
| 4 | Kerr bodies=16 black_holes=1 preset=1 skip=off path=gpu | 80x45 | 228.69 | 12.294 |
| 5 | Kerr bodies=16 black_holes=0 preset=1 skip=off path=gpu | 80x45 | 224.67 | 12.197 |
| 6 | Kerr bodies=16 black_holes=0 preset=1 skip=off path=cpu | 80x45 | 185.51 | 12.197 |
| 7 | Kerr bodies=0 black_holes=1 preset=1 skip=off path=gpu | 80x45 | 144.41 | 12.378 |
| 8 | Kerr bodies=0 black_holes=0 preset=1 skip=off path=cpu | 80x45 | 135.79 | 12.378 |
| 9 | Kerr bodies=0 black_holes=1 preset=1 skip=off path=cpu | 80x45 | 129.86 | 12.378 |
| 10 | Schwarzschild bodies=16 black_holes=0 preset=1 skip=off path=cpu | 80x45 | 119.87 | 12.866 |

## Complete results

| Metric | Bodies | BH | Preset | Skip | Path | Resolution | Steps | Median ms | Min ms | Max ms | Stddev ms | MPix/s | Avg iter | Horizon % | Disk % | Body % | Saturated % |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Schwarzschild | 0 | 0 | 1 | off | CPU | 80x45 | 1024 | 69.379 | 69.379 | 69.379 | 0 | 0.051889 | 12.385 | 5.889 | 73.39 | 0 | 0 |
| Schwarzschild | 0 | 0 | 1 | off | GPU | 80x45 | 1024 | 2.5586 | 2.5586 | 2.5586 | 0 | 1.407 | 13.272 | 5.889 | 1.472 | 0 | 1.472 |
| Schwarzschild | 0 | 1 | 1 | off | CPU | 80x45 | 1024 | 2924 | 2924 | 2924 | 0 | 0.0012312 | 13.289 | 5.861 | 1.556 | 0 | 1.556 |
| Schwarzschild | 0 | 1 | 1 | off | GPU | 80x45 | 1024 | 5.6123 | 5.6123 | 5.6123 | 0 | 0.64145 | 13.311 | 5.917 | 1.528 | 0 | 1.528 |
| Schwarzschild | 16 | 0 | 1 | off | CPU | 80x45 | 1024 | 119.87 | 119.87 | 119.87 | 0 | 0.030032 | 12.866 | 5.889 | 15.58 | 2.25 | 3.722 |
| Schwarzschild | 16 | 0 | 1 | off | GPU | 80x45 | 1024 | 4.6652 | 4.6652 | 4.6652 | 0 | 0.77167 | 13.064 | 5.889 | 3.611 | 2.278 | 3.75 |
| Schwarzschild | 16 | 1 | 1 | off | CPU | 80x45 | 1024 | 3183.1 | 3183.1 | 3183.1 | 0 | 0.001131 | 9.5456 | 5.111 | 35.39 | 0.7778 | 35.97 |
| Schwarzschild | 16 | 1 | 1 | off | GPU | 80x45 | 1024 | 6.069 | 6.069 | 6.069 | 0 | 0.59318 | 11.418 | 6.389 | 17.22 | 1.333 | 18.25 |
| Kerr | 0 | 0 | 1 | off | CPU | 80x45 | 1024 | 135.79 | 135.79 | 135.79 | 0 | 0.026511 | 12.378 | 1.861 | 3.472 | 0 | 3.472 |
| Kerr | 0 | 0 | 1 | off | GPU | 80x45 | 1024 | 4.8662 | 4.8662 | 4.8662 | 0 | 0.7398 | 12.378 | 1.861 | 3.472 | 0 | 3.472 |
| Kerr | 0 | 1 | 1 | off | CPU | 80x45 | 1024 | 129.86 | 129.86 | 129.86 | 0 | 0.027722 | 12.378 | 1.861 | 3.472 | 0 | 3.472 |
| Kerr | 0 | 1 | 1 | off | CPU (fallback) | 80x45 | 1024 | 144.41 | 144.41 | 144.41 | 0 | 0.024929 | 12.378 | 1.861 | 3.472 | 0 | 3.472 |
| Kerr | 16 | 0 | 1 | off | CPU | 80x45 | 1024 | 185.51 | 185.51 | 185.51 | 0 | 0.019406 | 12.197 | 1.861 | 3.806 | 2.167 | 5.639 |
| Kerr | 16 | 0 | 1 | off | CPU (fallback) | 80x45 | 1024 | 224.67 | 224.67 | 224.67 | 0 | 0.016024 | 12.197 | 1.861 | 3.806 | 2.167 | 5.639 |
| Kerr | 16 | 1 | 1 | off | CPU | 80x45 | 1024 | 257.16 | 257.16 | 257.16 | 0 | 0.013999 | 12.294 | 1.861 | 4.222 | 1.111 | 4.583 |
| Kerr | 16 | 1 | 1 | off | CPU (fallback) | 80x45 | 1024 | 228.69 | 228.69 | 228.69 | 0 | 0.015742 | 12.294 | 1.861 | 4.222 | 1.111 | 4.583 |

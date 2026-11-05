# Engine performance report

| Property | Value |
| --- | --- |
| Generated | 2026-11-05 09:48:47 UTC |
| Profile | quick |
| Cases executed | 16 |
| Total duration | 13.769 s |
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
| Schwarzschild | 1 | 91.787 | 36.961 | 2.483x |
| Kerr | 1 | 108.34 | 8.8904 | 12.19x |

## Body count scaling

Exponent of the power law fitted to frame time against (1 + total bodies); 1 means linear growth.

| Metric | Path | Preset | Exponent | Time at lowest count (ms) | Time at highest count (ms) |
| --- | --- | --- | --- | --- | --- |

## Secondary black hole cost

Additional median frame time per secondary black hole, with no celestial bodies.

| Metric | Path | Preset | Black holes | Baseline ms | Loaded ms | Cost per black hole (ms) |
| --- | --- | --- | --- | --- | --- | --- |
| Schwarzschild | CPU | 1 | 1 | 91.787 | 2377.9 | 2286.1 |
| Schwarzschild | GPU | 1 | 1 | 36.961 | 81.48 | 44.519 |
| Kerr | CPU | 1 | 1 | 108.34 | 130.56 | 22.224 |
| Kerr | GPU | 1 | 1 | 8.8904 | 132.22 | 123.33 |

## Space skipping gain

| Metric | Path | Preset | Bodies | Black holes | Without ms | With ms | Gain |
| --- | --- | --- | --- | --- | --- | --- | --- |

## Throughput leaderboard

| Rank | Case | Resolution | MPix/s | Median ms |
| --- | --- | --- | --- | --- |
| 1 | Schwarzschild bodies=16 black_holes=0 preset=1 skip=off path=gpu | 80x45 | 0.67202 | 5.357 |
| 2 | Kerr bodies=0 black_holes=0 preset=1 skip=off path=gpu | 80x45 | 0.40493 | 8.8904 |
| 3 | Schwarzschild bodies=16 black_holes=1 preset=1 skip=off path=gpu | 80x45 | 0.33698 | 10.683 |
| 4 | Schwarzschild bodies=0 black_holes=0 preset=1 skip=off path=gpu | 80x45 | 0.097399 | 36.961 |
| 5 | Schwarzschild bodies=0 black_holes=1 preset=1 skip=off path=gpu | 80x45 | 0.044183 | 81.48 |
| 6 | Schwarzschild bodies=0 black_holes=0 preset=1 skip=off path=cpu | 80x45 | 0.039221 | 91.787 |
| 7 | Kerr bodies=0 black_holes=0 preset=1 skip=off path=cpu | 80x45 | 0.03323 | 108.34 |
| 8 | Schwarzschild bodies=16 black_holes=0 preset=1 skip=off path=cpu | 80x45 | 0.031181 | 115.46 |
| 9 | Kerr bodies=0 black_holes=1 preset=1 skip=off path=cpu | 80x45 | 0.027573 | 130.56 |
| 10 | Kerr bodies=0 black_holes=1 preset=1 skip=off path=gpu | 80x45 | 0.027227 | 132.22 |

## Slowest cases

| Rank | Case | Resolution | Median ms | Average iterations |
| --- | --- | --- | --- | --- |
| 1 | Schwarzschild bodies=16 black_holes=1 preset=1 skip=off path=cpu | 80x45 | 2492.7 | 9.5456 |
| 2 | Schwarzschild bodies=0 black_holes=1 preset=1 skip=off path=cpu | 80x45 | 2377.9 | 13.289 |
| 3 | Kerr bodies=16 black_holes=1 preset=1 skip=off path=cpu | 80x45 | 267.64 | 12.294 |
| 4 | Kerr bodies=16 black_holes=1 preset=1 skip=off path=gpu | 80x45 | 264.82 | 12.294 |
| 5 | Kerr bodies=16 black_holes=0 preset=1 skip=off path=gpu | 80x45 | 213.94 | 12.197 |
| 6 | Kerr bodies=16 black_holes=0 preset=1 skip=off path=cpu | 80x45 | 195.38 | 12.197 |
| 7 | Kerr bodies=0 black_holes=1 preset=1 skip=off path=gpu | 80x45 | 132.22 | 12.378 |
| 8 | Kerr bodies=0 black_holes=1 preset=1 skip=off path=cpu | 80x45 | 130.56 | 12.378 |
| 9 | Schwarzschild bodies=16 black_holes=0 preset=1 skip=off path=cpu | 80x45 | 115.46 | 12.866 |
| 10 | Kerr bodies=0 black_holes=0 preset=1 skip=off path=cpu | 80x45 | 108.34 | 12.378 |

## Complete results

| Metric | Bodies | BH | Preset | Skip | Path | Resolution | Steps | Median ms | Min ms | Max ms | Stddev ms | MPix/s | Avg iter | Horizon % | Disk % | Body % | Saturated % |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Schwarzschild | 0 | 0 | 1 | off | CPU | 80x45 | 1024 | 91.787 | 91.787 | 91.787 | 0 | 0.039221 | 12.385 | 5.889 | 73.39 | 0 | 0 |
| Schwarzschild | 0 | 0 | 1 | off | GPU | 80x45 | 1024 | 36.961 | 36.961 | 36.961 | 0 | 0.097399 | 13.272 | 5.889 | 1.472 | 0 | 1.472 |
| Schwarzschild | 0 | 1 | 1 | off | CPU | 80x45 | 1024 | 2377.9 | 2377.9 | 2377.9 | 0 | 0.0015139 | 13.289 | 5.861 | 1.556 | 0 | 1.556 |
| Schwarzschild | 0 | 1 | 1 | off | GPU | 80x45 | 1024 | 81.48 | 81.48 | 81.48 | 0 | 0.044183 | 13.311 | 5.917 | 1.528 | 0 | 1.528 |
| Schwarzschild | 16 | 0 | 1 | off | CPU | 80x45 | 1024 | 115.46 | 115.46 | 115.46 | 0 | 0.031181 | 12.866 | 5.889 | 15.58 | 2.25 | 3.722 |
| Schwarzschild | 16 | 0 | 1 | off | GPU | 80x45 | 1024 | 5.357 | 5.357 | 5.357 | 0 | 0.67202 | 13.064 | 5.889 | 3.611 | 2.278 | 3.75 |
| Schwarzschild | 16 | 1 | 1 | off | CPU | 80x45 | 1024 | 2492.7 | 2492.7 | 2492.7 | 0 | 0.0014442 | 9.5456 | 5.111 | 35.39 | 0.7778 | 35.97 |
| Schwarzschild | 16 | 1 | 1 | off | GPU | 80x45 | 1024 | 10.683 | 10.683 | 10.683 | 0 | 0.33698 | 11.418 | 6.389 | 17.22 | 1.333 | 18.25 |
| Kerr | 0 | 0 | 1 | off | CPU | 80x45 | 1024 | 108.34 | 108.34 | 108.34 | 0 | 0.03323 | 12.378 | 1.861 | 3.472 | 0 | 3.472 |
| Kerr | 0 | 0 | 1 | off | GPU | 80x45 | 1024 | 8.8904 | 8.8904 | 8.8904 | 0 | 0.40493 | 12.378 | 1.861 | 3.472 | 0 | 3.472 |
| Kerr | 0 | 1 | 1 | off | CPU | 80x45 | 1024 | 130.56 | 130.56 | 130.56 | 0 | 0.027573 | 12.378 | 1.861 | 3.472 | 0 | 3.472 |
| Kerr | 0 | 1 | 1 | off | CPU (fallback) | 80x45 | 1024 | 132.22 | 132.22 | 132.22 | 0 | 0.027227 | 12.378 | 1.861 | 3.472 | 0 | 3.472 |
| Kerr | 16 | 0 | 1 | off | CPU | 80x45 | 1024 | 195.38 | 195.38 | 195.38 | 0 | 0.018425 | 12.197 | 1.861 | 3.806 | 2.167 | 5.639 |
| Kerr | 16 | 0 | 1 | off | CPU (fallback) | 80x45 | 1024 | 213.94 | 213.94 | 213.94 | 0 | 0.016827 | 12.197 | 1.861 | 3.806 | 2.167 | 5.639 |
| Kerr | 16 | 1 | 1 | off | CPU | 80x45 | 1024 | 267.64 | 267.64 | 267.64 | 0 | 0.013451 | 12.294 | 1.861 | 4.222 | 1.111 | 4.583 |
| Kerr | 16 | 1 | 1 | off | CPU (fallback) | 80x45 | 1024 | 264.82 | 264.82 | 264.82 | 0 | 0.013594 | 12.294 | 1.861 | 4.222 | 1.111 | 4.583 |

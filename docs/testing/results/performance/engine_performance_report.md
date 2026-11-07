# Engine performance report

| Property | Value |
| --- | --- |
| Generated | 2026-11-06 12:47:29 UTC |
| Profile | quick |
| Cases executed | 16 |
| Total duration | 13.18 s |
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
| CPU | 1 | 80x45 | none | 16 | 16 |
| GPU | 1 | 80x45 | 16 | 16 | 16 |

## Metric cost without bodies

Median frame time per metric, no bodies, no secondary black hole, no space skipping.

| Metric | Preset | CPU ms | GPU ms | GPU speedup |
| --- | --- | --- | --- | --- |
| Schwarzschild | 1 | 71.85 | 2.5649 | 28.01x |
| Kerr | 1 | 92.529 | 8.2045 | 11.28x |

## Body count scaling

Exponent of the power law fitted to frame time against (1 + total bodies); 1 means linear growth.

| Metric | Path | Preset | Exponent | Time at lowest count (ms) | Time at highest count (ms) |
| --- | --- | --- | --- | --- | --- |

## Secondary black hole cost

Additional median frame time per secondary black hole, with no celestial bodies.

| Metric | Path | Preset | Black holes | Baseline ms | Loaded ms | Cost per black hole (ms) |
| --- | --- | --- | --- | --- | --- | --- |
| Schwarzschild | CPU | 1 | 1 | 71.85 | 2559 | 2487.2 |
| Schwarzschild | GPU | 1 | 1 | 2.5649 | 8.8452 | 6.2803 |
| Kerr | CPU | 1 | 1 | 92.529 | 103.11 | 10.579 |
| Kerr | GPU | 1 | 1 | 8.2045 | 108.65 | 100.45 |

## Space skipping gain

| Metric | Path | Preset | Bodies | Black holes | Without ms | With ms | Gain |
| --- | --- | --- | --- | --- | --- | --- | --- |

## Throughput leaderboard

| Rank | Case | Resolution | MPix/s | Median ms |
| --- | --- | --- | --- | --- |
| 1 | Schwarzschild bodies=0 black_holes=0 preset=1 skip=off path=gpu | 80x45 | 1.4036 | 2.5649 |
| 2 | Schwarzschild bodies=16 black_holes=0 preset=1 skip=off path=gpu | 80x45 | 0.51141 | 7.0393 |
| 3 | Kerr bodies=0 black_holes=0 preset=1 skip=off path=gpu | 80x45 | 0.43878 | 8.2045 |
| 4 | Schwarzschild bodies=0 black_holes=1 preset=1 skip=off path=gpu | 80x45 | 0.407 | 8.8452 |
| 5 | Schwarzschild bodies=16 black_holes=1 preset=1 skip=off path=gpu | 80x45 | 0.26919 | 13.373 |
| 6 | Schwarzschild bodies=16 black_holes=0 preset=1 skip=off path=cpu | 80x45 | 0.050728 | 70.966 |
| 7 | Schwarzschild bodies=0 black_holes=0 preset=1 skip=off path=cpu | 80x45 | 0.050105 | 71.85 |
| 8 | Kerr bodies=0 black_holes=0 preset=1 skip=off path=cpu | 80x45 | 0.038907 | 92.529 |
| 9 | Kerr bodies=0 black_holes=1 preset=1 skip=off path=cpu | 80x45 | 0.034915 | 103.11 |
| 10 | Kerr bodies=0 black_holes=1 preset=1 skip=off path=gpu | 80x45 | 0.033133 | 108.65 |

## Slowest cases

| Rank | Case | Resolution | Median ms | Average iterations |
| --- | --- | --- | --- | --- |
| 1 | Schwarzschild bodies=0 black_holes=1 preset=1 skip=off path=cpu | 80x45 | 2559 | 13.289 |
| 2 | Schwarzschild bodies=16 black_holes=1 preset=1 skip=off path=cpu | 80x45 | 2373.4 | 9.5456 |
| 3 | Kerr bodies=16 black_holes=0 preset=1 skip=off path=gpu | 80x45 | 268.72 | 12.197 |
| 4 | Kerr bodies=16 black_holes=1 preset=1 skip=off path=gpu | 80x45 | 245.63 | 12.294 |
| 5 | Kerr bodies=16 black_holes=1 preset=1 skip=off path=cpu | 80x45 | 224.44 | 12.294 |
| 6 | Kerr bodies=16 black_holes=0 preset=1 skip=off path=cpu | 80x45 | 166.86 | 12.197 |
| 7 | Kerr bodies=0 black_holes=1 preset=1 skip=off path=gpu | 80x45 | 108.65 | 12.378 |
| 8 | Kerr bodies=0 black_holes=1 preset=1 skip=off path=cpu | 80x45 | 103.11 | 12.378 |
| 9 | Kerr bodies=0 black_holes=0 preset=1 skip=off path=cpu | 80x45 | 92.529 | 12.378 |
| 10 | Schwarzschild bodies=0 black_holes=0 preset=1 skip=off path=cpu | 80x45 | 71.85 | 12.385 |

## Complete results

| Metric | Bodies | BH | Preset | Skip | Path | Resolution | Steps | Median ms | Min ms | Max ms | Stddev ms | MPix/s | Avg iter | Horizon % | Disk % | Body % | Saturated % |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Schwarzschild | 0 | 0 | 1 | off | CPU | 80x45 | 1024 | 71.85 | 71.85 | 71.85 | 0 | 0.050105 | 12.385 | 5.889 | 73.39 | 0 | 0 |
| Schwarzschild | 0 | 0 | 1 | off | GPU | 80x45 | 1024 | 2.5649 | 2.5649 | 2.5649 | 0 | 1.4036 | 13.272 | 5.889 | 1.472 | 0 | 1.472 |
| Schwarzschild | 0 | 1 | 1 | off | CPU | 80x45 | 1024 | 2559 | 2559 | 2559 | 0 | 0.0014068 | 13.289 | 5.861 | 1.556 | 0 | 1.556 |
| Schwarzschild | 0 | 1 | 1 | off | GPU | 80x45 | 1024 | 8.8452 | 8.8452 | 8.8452 | 0 | 0.407 | 13.311 | 5.917 | 1.528 | 0 | 1.528 |
| Schwarzschild | 16 | 0 | 1 | off | CPU | 80x45 | 1024 | 70.966 | 70.966 | 70.966 | 0 | 0.050728 | 12.866 | 5.889 | 15.58 | 2.25 | 3.722 |
| Schwarzschild | 16 | 0 | 1 | off | GPU | 80x45 | 1024 | 7.0393 | 7.0393 | 7.0393 | 0 | 0.51141 | 13.064 | 5.889 | 3.611 | 2.278 | 3.75 |
| Schwarzschild | 16 | 1 | 1 | off | CPU | 80x45 | 1024 | 2373.4 | 2373.4 | 2373.4 | 0 | 0.0015168 | 9.5456 | 5.111 | 35.39 | 0.7778 | 35.97 |
| Schwarzschild | 16 | 1 | 1 | off | GPU | 80x45 | 1024 | 13.373 | 13.373 | 13.373 | 0 | 0.26919 | 11.418 | 6.389 | 17.22 | 1.333 | 18.25 |
| Kerr | 0 | 0 | 1 | off | CPU | 80x45 | 1024 | 92.529 | 92.529 | 92.529 | 0 | 0.038907 | 12.378 | 1.861 | 3.472 | 0 | 3.472 |
| Kerr | 0 | 0 | 1 | off | GPU | 80x45 | 1024 | 8.2045 | 8.2045 | 8.2045 | 0 | 0.43878 | 12.378 | 1.861 | 3.472 | 0 | 3.472 |
| Kerr | 0 | 1 | 1 | off | CPU | 80x45 | 1024 | 103.11 | 103.11 | 103.11 | 0 | 0.034915 | 12.378 | 1.861 | 3.472 | 0 | 3.472 |
| Kerr | 0 | 1 | 1 | off | CPU (fallback) | 80x45 | 1024 | 108.65 | 108.65 | 108.65 | 0 | 0.033133 | 12.378 | 1.861 | 3.472 | 0 | 3.472 |
| Kerr | 16 | 0 | 1 | off | CPU | 80x45 | 1024 | 166.86 | 166.86 | 166.86 | 0 | 0.021575 | 12.197 | 1.861 | 3.806 | 2.167 | 5.639 |
| Kerr | 16 | 0 | 1 | off | CPU (fallback) | 80x45 | 1024 | 268.72 | 268.72 | 268.72 | 0 | 0.013397 | 12.197 | 1.861 | 3.806 | 2.167 | 5.639 |
| Kerr | 16 | 1 | 1 | off | CPU | 80x45 | 1024 | 224.44 | 224.44 | 224.44 | 0 | 0.01604 | 12.294 | 1.861 | 4.222 | 1.111 | 4.583 |
| Kerr | 16 | 1 | 1 | off | CPU (fallback) | 80x45 | 1024 | 245.63 | 245.63 | 245.63 | 0 | 0.014656 | 12.294 | 1.861 | 4.222 | 1.111 | 4.583 |

# Engine performance report

| Property | Value |
| --- | --- |
| Generated | 2026-10-19 12:52:40 UTC |
| Profile | quick |
| Cases executed | 16 |
| Total duration | 14.966 s |
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
| Schwarzschild | 1 | 76.157 | 2.5737 | 29.59x |
| Kerr | 1 | 104.88 | 5.2776 | 19.87x |

## Body count scaling

Exponent of the power law fitted to frame time against (1 + total bodies); 1 means linear growth.

| Metric | Path | Preset | Exponent | Time at lowest count (ms) | Time at highest count (ms) |
| --- | --- | --- | --- | --- | --- |

## Secondary black hole cost

Additional median frame time per secondary black hole, with no celestial bodies.

| Metric | Path | Preset | Black holes | Baseline ms | Loaded ms | Cost per black hole (ms) |
| --- | --- | --- | --- | --- | --- | --- |
| Schwarzschild | CPU | 1 | 1 | 76.157 | 2628.1 | 2552 |
| Schwarzschild | GPU | 1 | 1 | 2.5737 | 5.6546 | 3.0809 |
| Kerr | CPU | 1 | 1 | 104.88 | 124.88 | 19.995 |
| Kerr | GPU | 1 | 1 | 5.2776 | 126.04 | 120.77 |

## Space skipping gain

| Metric | Path | Preset | Bodies | Black holes | Without ms | With ms | Gain |
| --- | --- | --- | --- | --- | --- | --- | --- |

## Throughput leaderboard

| Rank | Case | Resolution | MPix/s | Median ms |
| --- | --- | --- | --- | --- |
| 1 | Schwarzschild bodies=0 black_holes=0 preset=1 skip=off path=gpu | 80x45 | 1.3988 | 2.5737 |
| 2 | Schwarzschild bodies=16 black_holes=0 preset=1 skip=off path=gpu | 80x45 | 0.76279 | 4.7195 |
| 3 | Kerr bodies=0 black_holes=0 preset=1 skip=off path=gpu | 80x45 | 0.68213 | 5.2776 |
| 4 | Schwarzschild bodies=0 black_holes=1 preset=1 skip=off path=gpu | 80x45 | 0.63665 | 5.6546 |
| 5 | Schwarzschild bodies=16 black_holes=1 preset=1 skip=off path=gpu | 80x45 | 0.58703 | 6.1326 |
| 6 | Schwarzschild bodies=0 black_holes=0 preset=1 skip=off path=cpu | 80x45 | 0.047271 | 76.157 |
| 7 | Schwarzschild bodies=16 black_holes=0 preset=1 skip=off path=cpu | 80x45 | 0.039833 | 90.378 |
| 8 | Kerr bodies=0 black_holes=0 preset=1 skip=off path=cpu | 80x45 | 0.034324 | 104.88 |
| 9 | Kerr bodies=0 black_holes=1 preset=1 skip=off path=cpu | 80x45 | 0.028828 | 124.88 |
| 10 | Kerr bodies=0 black_holes=1 preset=1 skip=off path=gpu | 80x45 | 0.028561 | 126.04 |

## Slowest cases

| Rank | Case | Resolution | Median ms | Average iterations |
| --- | --- | --- | --- | --- |
| 1 | Schwarzschild bodies=16 black_holes=1 preset=1 skip=off path=cpu | 80x45 | 3128.9 | 9.5456 |
| 2 | Schwarzschild bodies=0 black_holes=1 preset=1 skip=off path=cpu | 80x45 | 2628.1 | 13.289 |
| 3 | Kerr bodies=16 black_holes=1 preset=1 skip=off path=cpu | 80x45 | 242.26 | 12.294 |
| 4 | Kerr bodies=16 black_holes=0 preset=1 skip=off path=gpu | 80x45 | 229.78 | 12.197 |
| 5 | Kerr bodies=16 black_holes=1 preset=1 skip=off path=gpu | 80x45 | 225.1 | 12.294 |
| 6 | Kerr bodies=16 black_holes=0 preset=1 skip=off path=cpu | 80x45 | 198.75 | 12.197 |
| 7 | Kerr bodies=0 black_holes=1 preset=1 skip=off path=gpu | 80x45 | 126.04 | 12.378 |
| 8 | Kerr bodies=0 black_holes=1 preset=1 skip=off path=cpu | 80x45 | 124.88 | 12.378 |
| 9 | Kerr bodies=0 black_holes=0 preset=1 skip=off path=cpu | 80x45 | 104.88 | 12.378 |
| 10 | Schwarzschild bodies=16 black_holes=0 preset=1 skip=off path=cpu | 80x45 | 90.378 | 12.866 |

## Complete results

| Metric | Bodies | BH | Preset | Skip | Path | Resolution | Steps | Median ms | Min ms | Max ms | Stddev ms | MPix/s | Avg iter | Horizon % | Disk % | Body % | Saturated % |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Schwarzschild | 0 | 0 | 1 | off | CPU | 80x45 | 1024 | 76.157 | 76.157 | 76.157 | 0 | 0.047271 | 12.385 | 5.889 | 73.39 | 0 | 0 |
| Schwarzschild | 0 | 0 | 1 | off | GPU | 80x45 | 1024 | 2.5737 | 2.5737 | 2.5737 | 0 | 1.3988 | 13.272 | 5.889 | 1.472 | 0 | 1.472 |
| Schwarzschild | 0 | 1 | 1 | off | CPU | 80x45 | 1024 | 2628.1 | 2628.1 | 2628.1 | 0 | 0.0013698 | 13.289 | 5.861 | 1.556 | 0 | 1.556 |
| Schwarzschild | 0 | 1 | 1 | off | GPU | 80x45 | 1024 | 5.6546 | 5.6546 | 5.6546 | 0 | 0.63665 | 13.311 | 5.917 | 1.528 | 0 | 1.528 |
| Schwarzschild | 16 | 0 | 1 | off | CPU | 80x45 | 1024 | 90.378 | 90.378 | 90.378 | 0 | 0.039833 | 12.866 | 5.889 | 15.58 | 2.25 | 3.722 |
| Schwarzschild | 16 | 0 | 1 | off | GPU | 80x45 | 1024 | 4.7195 | 4.7195 | 4.7195 | 0 | 0.76279 | 13.064 | 5.889 | 3.611 | 2.278 | 3.75 |
| Schwarzschild | 16 | 1 | 1 | off | CPU | 80x45 | 1024 | 3128.9 | 3128.9 | 3128.9 | 0 | 0.0011506 | 9.5456 | 5.111 | 35.39 | 0.7778 | 35.97 |
| Schwarzschild | 16 | 1 | 1 | off | GPU | 80x45 | 1024 | 6.1326 | 6.1326 | 6.1326 | 0 | 0.58703 | 11.418 | 6.389 | 17.22 | 1.333 | 18.25 |
| Kerr | 0 | 0 | 1 | off | CPU | 80x45 | 1024 | 104.88 | 104.88 | 104.88 | 0 | 0.034324 | 12.378 | 1.861 | 3.472 | 0 | 3.472 |
| Kerr | 0 | 0 | 1 | off | GPU | 80x45 | 1024 | 5.2776 | 5.2776 | 5.2776 | 0 | 0.68213 | 12.378 | 1.861 | 3.472 | 0 | 3.472 |
| Kerr | 0 | 1 | 1 | off | CPU | 80x45 | 1024 | 124.88 | 124.88 | 124.88 | 0 | 0.028828 | 12.378 | 1.861 | 3.472 | 0 | 3.472 |
| Kerr | 0 | 1 | 1 | off | CPU (fallback) | 80x45 | 1024 | 126.04 | 126.04 | 126.04 | 0 | 0.028561 | 12.378 | 1.861 | 3.472 | 0 | 3.472 |
| Kerr | 16 | 0 | 1 | off | CPU | 80x45 | 1024 | 198.75 | 198.75 | 198.75 | 0 | 0.018113 | 12.197 | 1.861 | 3.806 | 2.167 | 5.639 |
| Kerr | 16 | 0 | 1 | off | CPU (fallback) | 80x45 | 1024 | 229.78 | 229.78 | 229.78 | 0 | 0.015667 | 12.197 | 1.861 | 3.806 | 2.167 | 5.639 |
| Kerr | 16 | 1 | 1 | off | CPU | 80x45 | 1024 | 242.26 | 242.26 | 242.26 | 0 | 0.01486 | 12.294 | 1.861 | 4.222 | 1.111 | 4.583 |
| Kerr | 16 | 1 | 1 | off | CPU (fallback) | 80x45 | 1024 | 225.1 | 225.1 | 225.1 | 0 | 0.015993 | 12.294 | 1.861 | 4.222 | 1.111 | 4.583 |

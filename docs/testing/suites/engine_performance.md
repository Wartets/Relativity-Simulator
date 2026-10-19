# Engine performance benchmark

Executable: `benchmark_engine_performance` - Source: `tests/benchmark_engine_performance.cpp` - Reports: [engine_performance.md](../results/engine_performance.md) and [performance/engine_performance_report.md](../results/performance/engine_performance_report.md)

## Purpose

Documents what the engine can do and what it needs: frame time, throughput, ray statistics and scaling for every combination of spacetime, scene complexity and performance setting, on both execution paths.

## Dimensions

| Dimension | quick | standard | full |
| --- | --- | --- | --- |
| Metrics | Schwarzschild, Kerr | 10 (Flat, Schwarzschild, Kerr, Reissner-Nordstrom, Kerr-Newman, Schwarzschild-de Sitter, Morris-Thorne, Alcubierre, Eddington-Finkelstein, TeVeS) | all 16 |
| Celestial bodies | 0, 16 | 0, 10, 100 | 0, 1, 10, 100, 500, 1000 |
| Secondary black holes | 0, 1 | 0, 1, 2 | 0, 1, 2, 4, 8 |
| Performance presets | 1 | 0 to 3 | 0 to 5 |
| Space skipping | off | off | off and on |
| Paths | CPU and GPU | CPU and GPU | CPU and GPU |
| Base width | 160 px | 256 px | 320 px |
| Ray step cap | 1024 | 4096 | none |
| Timed repetitions | 1 | 2 | 3 |
| Time budget per case | 60 s | 180 s | 900 s |

The render resolution of a case is the base width times the preset render scale, with a 16:9 aspect. Presets provide ray steps, tolerances and body level-of-detail settings through the orchestrator. Secondary black holes are skipped for metrics that ignore them (Morris-Thorne, Alcubierre).

## Scene construction

Scenes are deterministic. Celestial bodies are scattered between 8 and 70 gravitational radii with cycling geometry, surface texture, atmosphere and preset values; secondary black holes sit between 20 and 60 radii with random spins. The observer is at r = 30 with a 50 degree pinhole field of view looking at the origin.

## Method

Each case is prepared through the orchestrator, warmed up with one discarded frame, then timed over the configured repetitions through the headless pipeline dispatch. The pipeline telemetry reports the effective path, so GPU fallbacks are visible in the results. One pipeline is kept per resolution.

## Sanity checks

For every case: the frame buffer is complete, no pixel is non-finite, alpha is one, frame time is positive, horizon metrics produce horizon pixels, and flat spacetime without secondary sources produces none. When a GPU is available, the suite also compares CPU and GPU images of a reference scene and fails above a mean absolute RGB difference of 0.25.

## Reported analyses

- Capability envelope on Schwarzschild: largest body count sustaining 30, 10 and 1 frames per second per path and preset.
- Cost of each metric and GPU speed-up.
- Power-law exponent of frame time against body count.
- Cost per secondary black hole.
- Space skipping gain.
- Throughput leaderboard, slowest cases, and the complete result table.

## Running

See the options in `benchmark_engine_performance --help`. Results of the last run are regenerated at every execution; the full profile can take hours, depending on hardware.

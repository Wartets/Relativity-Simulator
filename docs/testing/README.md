# Testing guide

This directory is the reference for every automated test of the engine: what each suite verifies, how to run it, the acceptance criteria, and the results of the last recorded run.

## Layout

| Path | Content |
| --- | --- |
| `tests/` | Test sources, one executable per suite |
| `tests/support/test_harness.hpp` | Shared harness: checks, measurements, console output, report generation |
| `tests/support/engine_scenario.hpp` | Deterministic scenario builder used by the engine benchmarks |
| `docs/testing/suites/` | In-depth description of each maintained suite |
| `docs/testing/results/` | Reports written by the suites at the end of every run |
| `docs/testing/test_catalog.md` | Catalogue of every registered test with its migration status |
| `docs/testing/CONTRIBUTING_TESTS.md` | Rules for writing and documenting tests |

## Running

```
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Long benchmark:

```
cmake --build build --target run_engine_performance_benchmark
```

The performance binary also accepts options directly:

```
benchmark_engine_performance --profile standard --path both --repetitions 3
benchmark_engine_performance --metrics Kerr,Schwarzschild --bodies 0,100,1000 --black-holes 0,4 --presets 1,3
benchmark_engine_performance --list
```

## Results workflow

Every migrated suite writes `<suite>.md` and `<suite>.json` into the directory named by the `RELATIVISTIC_TEST_REPORT_DIR` environment variable. CTest sets it to `docs/testing/results`, so a normal `ctest` run refreshes the documented results in place. Without the variable the reports go to `./test_reports`.

The performance benchmark additionally writes `performance/engine_performance_report.md`, `.json` and `.csv`.

Commit refreshed results together with the change that motivated the run.

## Maintained suites

| Suite | Executable | Description |
| --- | --- | --- |
| Static metrics | `test_static_metrics` | [suites/static_metrics.md](suites/static_metrics.md) |
| SIMD geodesic bundle | `benchmark_simd_bundle` | [suites/simd_bundle.md](suites/simd_bundle.md) |
| Projection modes | `test_projection_modes` | [suites/projection_modes.md](suites/projection_modes.md) |
| Camera navigation modes | `test_camera_navigation_modes` | [suites/camera_navigation_modes.md](suites/camera_navigation_modes.md) |
| Engine performance | `benchmark_engine_performance` | [suites/engine_performance.md](suites/engine_performance.md) |

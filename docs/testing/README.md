# Testing guide

This directory is the reference for every automated test of the engine: what each suite verifies, how to run it, the acceptance criteria, and the results of the last recorded run.

## Layout

| Path | Content |
| --- | --- |
| `tests/` | Test sources, one executable per suite |
| `tests/support/test_harness.hpp` | Shared harness: checks, measurements, console output, report generation |
| `tests/support/engine_scenario.hpp` | Deterministic scenario builder used by the engine benchmarks |
| `tests/support/metric_checks.hpp` | Shared metric invariant checks: inversion, symmetry, signature, connection symmetry and scale-aware analytic against finite-difference connection |
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
| Scheduler | `test_scheduler` | [suites/scheduler.md](suites/scheduler.md) |
| REPL parser | `test_repl_parser` | [suites/repl_parser.md](suites/repl_parser.md) |
| SPSC queue | `test_spsc_queue` | [suites/spsc_queue.md](suites/spsc_queue.md) |
| Thread pool | `test_thread_pool` | [suites/thread_pool.md](suites/thread_pool.md) |
| PCG64 generator | `test_pcg64` | [suites/pcg64.md](suites/pcg64.md) |
| Memory arena | `test_memory_arena` | [suites/memory_arena.md](suites/memory_arena.md) |
| Scenario serializer | `test_scenario_serializer` | [suites/scenario_serializer.md](suites/scenario_serializer.md) |
| Interval arithmetic | `test_interval_arithmetic` | [suites/interval_arithmetic.md](suites/interval_arithmetic.md) |
| Horizon regime | `test_horizon_regime` | [suites/horizon_regime.md](suites/horizon_regime.md) |
| Double-single arithmetic | `test_double_single` | [suites/double_single.md](suites/double_single.md) |
| Post-Newtonian conservation | `test_pn_conservative` | [suites/pn_conservative.md](suites/pn_conservative.md) |
| Post-Newtonian radiation reaction | `test_pn_radiation_reaction` | [suites/pn_radiation_reaction.md](suites/pn_radiation_reaction.md) |
| Post-Newtonian spin precession | `test_pn_spin_precession` | [suites/pn_spin_precession.md](suites/pn_spin_precession.md) |
| Gravitational waves | `test_gravitational_waves` | [suites/gravitational_waves.md](suites/gravitational_waves.md) |
| Hulse-Taylor orbital decay | `test_hulse_taylor_orbital_decay` | [suites/hulse_taylor_orbital_decay.md](suites/hulse_taylor_orbital_decay.md) |
| Hyperbolic motion | `test_hyperbolic_motion` | [suites/hyperbolic_motion.md](suites/hyperbolic_motion.md) |
| Polynomial chaos | `test_polynomial_chaos` | [suites/polynomial_chaos.md](suites/polynomial_chaos.md) |
| Uncertain quantity | `test_uncertain_quantity` | [suites/uncertain_quantity.md](suites/uncertain_quantity.md) |
| Zonotope propagation | `test_zonotope_propagation` | [suites/zonotope_propagation.md](suites/zonotope_propagation.md) |
| SPK reader | `test_spk_reader` | [suites/spk_reader.md](suites/spk_reader.md) |
| HORIZONS parser | `test_horizons_parser` | [suites/horizons_parser.md](suites/horizons_parser.md) |
| Frame transforms | `test_frame_transforms` | [suites/frame_transforms.md](suites/frame_transforms.md) |
| Ephemeris and Keplerian elements | `test_ephemeris_kepler` | [suites/ephemeris_kepler.md](suites/ephemeris_kepler.md) |
| FITS exporter | `test_fits_exporter` | [suites/fits_exporter.md](suites/fits_exporter.md) |
| VTK exporter | `test_vtk_exporter` | [suites/vtk_exporter.md](suites/vtk_exporter.md) |
| Dark matter profiles | `test_dark_matter_profiles` | [suites/dark_matter_profiles.md](suites/dark_matter_profiles.md) |
| MOND rotation curves | `test_mond_rotation_curves` | [suites/mond_rotation_curves.md](suites/mond_rotation_curves.md) |
| f(R) chameleon | `test_f_r_chameleon` | [suites/f_r_chameleon.md](suites/f_r_chameleon.md) |
| Spectral shift and beaming | `test_spectral_shift_beaming` | [suites/spectral_shift_beaming.md](suites/spectral_shift_beaming.md) |
| Camera projections | `test_camera_projections` | [suites/camera_projections.md](suites/camera_projections.md) |
| SIMD vectors | `test_simd_vec` | [suites/simd_vec.md](suites/simd_vec.md) |
| Kerr metrics | `test_kerr_metrics` | [suites/kerr_metrics.md](suites/kerr_metrics.md) |
| Vacuum and cosmological metrics | `test_vacuum_cosmo_metrics` | [suites/vacuum_cosmo_metrics.md](suites/vacuum_cosmo_metrics.md) |
| Exotic metrics | `test_exotic_metrics` | [suites/exotic_metrics.md](suites/exotic_metrics.md) |
| TeVeS metric | `test_teves_metric` | [suites/teves_metric.md](suites/teves_metric.md) |
| Symplectic Gauss-Legendre | `test_symplectic_gauss_legendre` | [suites/symplectic_gauss_legendre.md](suites/symplectic_gauss_legendre.md) |
| High-order integrators | `test_high_order_integrators` | [suites/high_order_integrators.md](suites/high_order_integrators.md) |
| Hermite 4th-order | `test_hermite4_aarseth` | [suites/hermite4_aarseth.md](suites/hermite4_aarseth.md) |
| Kerr shadow | `test_kerr_shadow_eht` | [suites/kerr_shadow_eht.md](suites/kerr_shadow_eht.md) |
| Spherical harmonics | `test_spherical_harmonics` | [suites/spherical_harmonics.md](suites/spherical_harmonics.md) |
| Tidal and rotational gravity | `test_tidal_rotational_gravity` | [suites/tidal_rotational_gravity.md](suites/tidal_rotational_gravity.md) |
| Container serializer | `test_hdf5_serializer` | [suites/hdf5_serializer.md](suites/hdf5_serializer.md) |
| Terrell-Penrose optics | `test_terrell_penrose` | [suites/terrell_penrose.md](suites/terrell_penrose.md) |
| CIE spectral pipeline | `test_cie_spectral_pipeline` | [suites/cie_spectral_pipeline.md](suites/cie_spectral_pipeline.md) |
| Rocket dynamics | `test_rocket_dynamics` | [suites/rocket_dynamics.md](suites/rocket_dynamics.md) |
| Novikov-Thorne disk | `test_novikov_thorne_disk` | [suites/novikov_thorne_disk.md](suites/novikov_thorne_disk.md) |
| Fishbone-Moncrief torus | `test_fishbone_moncrief_torus` | [suites/fishbone_moncrief_torus.md](suites/fishbone_moncrief_torus.md) |
| Uncertainty metrology | `test_uncertainty_metrology` | [suites/uncertainty_metrology.md](suites/uncertainty_metrology.md) |

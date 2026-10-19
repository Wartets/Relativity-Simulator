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

## Test catalog

| Test / Executable | Suite | Category | Documentation |
| --- | --- | --- | --- |
| `benchmark_engine_performance_quick` / `benchmark_engine_performance` | Engine performance | Engine performance | [suites/engine_performance.md](suites/engine_performance.md) |
| `benchmark_simd_bundle` | SIMD geodesic bundle | Numerical performance | [suites/simd_bundle.md](suites/simd_bundle.md) |
| `test_barnes_hut_octree` | Barnes-Hut octree | Dark matter and N-body | [suites/barnes_hut_octree.md](suites/barnes_hut_octree.md) |
| `test_bssn_gauge_wave` | BSSN gauge wave | Numerical relativity | [suites/bssn_gauge_wave.md](suites/bssn_gauge_wave.md) |
| `test_camera_navigation_modes` | Camera navigation modes | Observer | [suites/camera_navigation_modes.md](suites/camera_navigation_modes.md) |
| `test_camera_projections` | Camera projections | Observer | [suites/camera_projections.md](suites/camera_projections.md) |
| `test_carter_constant` | Carter constant | Metrics and geodesics | [suites/carter_constant.md](suites/carter_constant.md) |
| `test_christoffel_solver` | Christoffel solver | Tensor and SIMD math | [suites/christoffel_solver.md](suites/christoffel_solver.md) |
| `test_cie_spectral_pipeline` | CIE spectral pipeline | Rendering and optics | [suites/cie_spectral_pipeline.md](suites/cie_spectral_pipeline.md) |
| `test_concurrent_command_injection` | Concurrent command injection | Core infrastructure | [suites/concurrent_command_injection.md](suites/concurrent_command_injection.md) |
| `test_covariance_propagation` | Covariance propagation | Uncertainty quantification | [suites/covariance_propagation.md](suites/covariance_propagation.md) |
| `test_dark_matter_field` | Dark matter field | Dark matter and N-body | [suites/dark_matter_field.md](suites/dark_matter_field.md) |
| `test_dark_matter_profiles` | Dark matter profiles | Dark matter and N-body | [suites/dark_matter_profiles.md](suites/dark_matter_profiles.md) |
| `test_default_startup_scenario` | Default startup scenario | Input and output | [suites/default_startup_scenario.md](suites/default_startup_scenario.md) |
| `test_double_single` | Double-single arithmetic | Tensor and SIMD math | [suites/double_single.md](suites/double_single.md) |
| `test_eos_tov` | Equation of state and TOV | Hydrodynamics and radiation | [suites/eos_tov.md](suites/eos_tov.md) |
| `test_ephemeris_kepler` | Ephemeris and Keplerian elements | Ephemerides and frames | [suites/ephemeris_kepler.md](suites/ephemeris_kepler.md) |
| `test_exotic_metrics` | Exotic metrics | Metrics and geodesics | [suites/exotic_metrics.md](suites/exotic_metrics.md) |
| `test_f_r_chameleon` | f(R) chameleon | Modified gravity | [suites/f_r_chameleon.md](suites/f_r_chameleon.md) |
| `test_fishbone_moncrief_torus` | Fishbone-Moncrief torus | Hydrodynamics and radiation | [suites/fishbone_moncrief_torus.md](suites/fishbone_moncrief_torus.md) |
| `test_fits_exporter` | FITS exporter | Input and output | [suites/fits_exporter.md](suites/fits_exporter.md) |
| `test_frame_transforms` | Frame transforms | Ephemerides and frames | [suites/frame_transforms.md](suites/frame_transforms.md) |
| `test_galaxy_collision_nbody` | Galaxy collision | Dark matter and N-body | [suites/galaxy_collision_nbody.md](suites/galaxy_collision_nbody.md) |
| `test_geodesic_bundle` | Geodesic bundle | Tensor and SIMD math | [suites/geodesic_bundle.md](suites/geodesic_bundle.md) |
| `test_gravitational_waves` | Gravitational waves | Post-Newtonian dynamics | [suites/gravitational_waves.md](suites/gravitational_waves.md) |
| `test_hdf5_serializer` | Container serializer | Input and output | [suites/hdf5_serializer.md](suites/hdf5_serializer.md) |
| `test_hermite4_aarseth` | Hermite 4th-order | Integrators | [suites/hermite4_aarseth.md](suites/hermite4_aarseth.md) |
| `test_high_order_integrators` | High-order integrators | Integrators | [suites/high_order_integrators.md](suites/high_order_integrators.md) |
| `test_horizon_crossing` | Horizon crossing | Metrics and geodesics | [suites/horizon_crossing.md](suites/horizon_crossing.md) |
| `test_horizon_regime` | Horizon regime | Metrics | [suites/horizon_regime.md](suites/horizon_regime.md) |
| `test_horizons_parser` | HORIZONS parser | Ephemerides and frames | [suites/horizons_parser.md](suites/horizons_parser.md) |
| `test_hulse_taylor_orbital_decay` | Hulse-Taylor orbital decay | Gravity physics validation | [suites/hulse_taylor_orbital_decay.md](suites/hulse_taylor_orbital_decay.md) |
| `test_hyperbolic_motion` | Hyperbolic motion | Gravity physics validation | [suites/hyperbolic_motion.md](suites/hyperbolic_motion.md) |
| `test_interval_arithmetic` | Interval arithmetic | Uncertainty quantification | [suites/interval_arithmetic.md](suites/interval_arithmetic.md) |
| `test_invariant_preservation` | Invariant preservation | Metrics and geodesics | [suites/invariant_preservation.md](suites/invariant_preservation.md) |
| `test_kerr_metrics` | Kerr metrics | Metrics and geodesics | [suites/kerr_metrics.md](suites/kerr_metrics.md) |
| `test_kerr_schild_geodesics` | Kerr-Schild geodesics | Metrics and geodesics | [suites/kerr_schild_geodesics.md](suites/kerr_schild_geodesics.md) |
| `test_kerr_shadow_eht` | Kerr shadow | Gravity physics validation | [suites/kerr_shadow_eht.md](suites/kerr_shadow_eht.md) |
| `test_memory_arena` | Memory arena | Core infrastructure | [suites/memory_arena.md](suites/memory_arena.md) |
| `test_mercury_precession` | Mercury precession | Gravity physics validation | [suites/mercury_precession.md](suites/mercury_precession.md) |
| `test_mond_rotation_curves` | MOND rotation curves | Dark matter and N-body | [suites/mond_rotation_curves.md](suites/mond_rotation_curves.md) |
| `test_nodal_precession_leo` | Nodal precession | Gravity physics validation | [suites/nodal_precession_leo.md](suites/nodal_precession_leo.md) |
| `test_novikov_thorne_disk` | Novikov-Thorne disk | Hydrodynamics and radiation | [suites/novikov_thorne_disk.md](suites/novikov_thorne_disk.md) |
| `test_observer_tetrad` | Observer tetrad | Rendering and optics | [suites/observer_tetrad.md](suites/observer_tetrad.md) |
| `test_pcg64` | PCG64 generator | Core infrastructure | [suites/pcg64.md](suites/pcg64.md) |
| `test_pn_conservative` | Post-Newtonian conservation | Post-Newtonian dynamics | [suites/pn_conservative.md](suites/pn_conservative.md) |
| `test_pn_radiation_reaction` | Post-Newtonian radiation reaction | Post-Newtonian dynamics | [suites/pn_radiation_reaction.md](suites/pn_radiation_reaction.md) |
| `test_pn_spin_precession` | Post-Newtonian spin precession | Post-Newtonian dynamics | [suites/pn_spin_precession.md](suites/pn_spin_precession.md) |
| `test_polarized_radiative_transfer` | Polarized radiative transfer | Hydrodynamics and radiation | [suites/polarized_radiative_transfer.md](suites/polarized_radiative_transfer.md) |
| `test_polynomial_chaos` | Polynomial chaos | Uncertainty quantification | [suites/polynomial_chaos.md](suites/polynomial_chaos.md) |
| `test_projection_modes` | Projection modes | Observer | [suites/projection_modes.md](suites/projection_modes.md) |
| `test_relativistic_hydro` | Relativistic hydrodynamics | Hydrodynamics and radiation | [suites/relativistic_hydro.md](suites/relativistic_hydro.md) |
| `test_repl_parser` | REPL parser | Core infrastructure | [suites/repl_parser.md](suites/repl_parser.md) |
| `test_replay_loop` | Record and replay | Core infrastructure | [suites/replay_loop.md](suites/replay_loop.md) |
| `test_rocket_dynamics` | Rocket dynamics | Rendering and optics | [suites/rocket_dynamics.md](suites/rocket_dynamics.md) |
| `test_scenario_serializer` | Scenario serializer | Input and output | [suites/scenario_serializer.md](suites/scenario_serializer.md) |
| `test_scheduler` | Scheduler | Core infrastructure | [suites/scheduler.md](suites/scheduler.md) |
| `test_schwarzschild_shadow_render` | Schwarzschild shadow render | Rendering and optics | [suites/schwarzschild_shadow_render.md](suites/schwarzschild_shadow_render.md) |
| `test_shapiro_delay` | Shapiro delay | Gravity physics validation | [suites/shapiro_delay.md](suites/shapiro_delay.md) |
| `test_simd_vec` | SIMD vectors | Tensor and SIMD math | [suites/simd_vec.md](suites/simd_vec.md) |
| `test_solar_deflection` | Solar light deflection | Gravity physics validation | [suites/solar_deflection.md](suites/solar_deflection.md) |
| `test_spectral_shift_beaming` | Spectral shift and beaming | Rendering and optics | [suites/spectral_shift_beaming.md](suites/spectral_shift_beaming.md) |
| `test_spherical_harmonics` | Spherical harmonics | Post-Newtonian dynamics | [suites/spherical_harmonics.md](suites/spherical_harmonics.md) |
| `test_spk_reader` | SPK reader | Ephemerides and frames | [suites/spk_reader.md](suites/spk_reader.md) |
| `test_spsc_queue` | SPSC queue | Core infrastructure | [suites/spsc_queue.md](suites/spsc_queue.md) |
| `test_static_metrics` | Static metrics | Metrics | [suites/static_metrics.md](suites/static_metrics.md) |
| `test_symplectic_gauss_legendre` | Symplectic Gauss-Legendre | Integrators | [suites/symplectic_gauss_legendre.md](suites/symplectic_gauss_legendre.md) |
| `test_tensor_algebra` | Tensor algebra | Tensor and SIMD math | [suites/tensor_algebra.md](suites/tensor_algebra.md) |
| `test_terrell_penrose` | Terrell-Penrose optics | Rendering and optics | [suites/terrell_penrose.md](suites/terrell_penrose.md) |
| `test_teves_metric` | TeVeS metric | Metrics and geodesics | [suites/teves_metric.md](suites/teves_metric.md) |
| `test_thread_pool` | Thread pool | Core infrastructure | [suites/thread_pool.md](suites/thread_pool.md) |
| `test_tidal_rotational_gravity` | Tidal and rotational gravity | Post-Newtonian dynamics | [suites/tidal_rotational_gravity.md](suites/tidal_rotational_gravity.md) |
| `test_ui_multi_window` | Interface control | Interface | [suites/ui_multi_window.md](suites/ui_multi_window.md) |
| `test_uncertain_quantity` | Uncertain quantity | Uncertainty quantification | [suites/uncertain_quantity.md](suites/uncertain_quantity.md) |
| `test_uncertainty_metrology` | Uncertainty metrology | Uncertainty quantification | [suites/uncertainty_metrology.md](suites/uncertainty_metrology.md) |
| `test_uncertainty_pce` | Polynomial chaos propagation | Uncertainty quantification | [suites/uncertainty_pce.md](suites/uncertainty_pce.md) |
| `test_vacuum_cosmo_metrics` | Vacuum and cosmological metrics | Metrics and geodesics | [suites/vacuum_cosmo_metrics.md](suites/vacuum_cosmo_metrics.md) |
| `test_vtk_exporter` | VTK exporter | Input and output | [suites/vtk_exporter.md](suites/vtk_exporter.md) |
| `test_vulkan_compute_pipeline` | Compute pipeline | Rendering and optics | [suites/vulkan_compute_pipeline.md](suites/vulkan_compute_pipeline.md) |
| `test_zonotope_propagation` | Zonotope propagation | Uncertainty quantification | [suites/zonotope_propagation.md](suites/zonotope_propagation.md) |

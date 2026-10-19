# Test catalogue

Status values: `Maintained` (uses the shared harness, documented in `suites/`, results in `results/`), `Legacy` (registered, not yet audited or migrated to the harness).

## Maintained

| Test | Category | Documentation |
| --- | --- | --- |
| `test_static_metrics` | Metrics | [suites/static_metrics.md](suites/static_metrics.md) |
| `test_horizon_regime` | Metrics | [suites/horizon_regime.md](suites/horizon_regime.md) |
| `test_f_r_chameleon` | Modified gravity | [suites/f_r_chameleon.md](suites/f_r_chameleon.md) |
| `benchmark_simd_bundle` | Numerical performance | [suites/simd_bundle.md](suites/simd_bundle.md) |
| `test_double_single` | Tensor and SIMD math | [suites/double_single.md](suites/double_single.md) |
| `test_projection_modes` | Observer | [suites/projection_modes.md](suites/projection_modes.md) |
| `test_camera_projections` | Observer | [suites/camera_projections.md](suites/camera_projections.md) |
| `test_camera_navigation_modes` | Observer | [suites/camera_navigation_modes.md](suites/camera_navigation_modes.md) |
| `test_hyperbolic_motion` | Gravity physics validation | [suites/hyperbolic_motion.md](suites/hyperbolic_motion.md) |
| `test_hulse_taylor_orbital_decay` | Gravity physics validation | [suites/hulse_taylor_orbital_decay.md](suites/hulse_taylor_orbital_decay.md) |
| `benchmark_engine_performance_quick` | Engine performance | [suites/engine_performance.md](suites/engine_performance.md) |
| `test_scheduler` | Core infrastructure | [suites/scheduler.md](suites/scheduler.md) |
| `test_repl_parser` | Core infrastructure | [suites/repl_parser.md](suites/repl_parser.md) |
| `test_spsc_queue` | Core infrastructure | [suites/spsc_queue.md](suites/spsc_queue.md) |
| `test_thread_pool` | Core infrastructure | [suites/thread_pool.md](suites/thread_pool.md) |
| `test_pcg64` | Core infrastructure | [suites/pcg64.md](suites/pcg64.md) |
| `test_memory_arena` | Core infrastructure | [suites/memory_arena.md](suites/memory_arena.md) |
| `test_pn_conservative` | Post-Newtonian dynamics | [suites/pn_conservative.md](suites/pn_conservative.md) |
| `test_pn_radiation_reaction` | Post-Newtonian dynamics | [suites/pn_radiation_reaction.md](suites/pn_radiation_reaction.md) |
| `test_pn_spin_precession` | Post-Newtonian dynamics | [suites/pn_spin_precession.md](suites/pn_spin_precession.md) |
| `test_gravitational_waves` | Post-Newtonian dynamics | [suites/gravitational_waves.md](suites/gravitational_waves.md) |
| `test_dark_matter_profiles` | Dark matter and N-body | [suites/dark_matter_profiles.md](suites/dark_matter_profiles.md) |
| `test_mond_rotation_curves` | Dark matter and N-body | [suites/mond_rotation_curves.md](suites/mond_rotation_curves.md) |
| `test_ephemeris_kepler` | Ephemerides and frames | [suites/ephemeris_kepler.md](suites/ephemeris_kepler.md) |
| `test_spk_reader` | Ephemerides and frames | [suites/spk_reader.md](suites/spk_reader.md) |
| `test_horizons_parser` | Ephemerides and frames | [suites/horizons_parser.md](suites/horizons_parser.md) |
| `test_frame_transforms` | Ephemerides and frames | [suites/frame_transforms.md](suites/frame_transforms.md) |
| `test_scenario_serializer` | Input and output | [suites/scenario_serializer.md](suites/scenario_serializer.md) |
| `test_fits_exporter` | Input and output | [suites/fits_exporter.md](suites/fits_exporter.md) |
| `test_vtk_exporter` | Input and output | [suites/vtk_exporter.md](suites/vtk_exporter.md) |
| `test_spectral_shift_beaming` | Rendering and optics | [suites/spectral_shift_beaming.md](suites/spectral_shift_beaming.md) |
| `test_interval_arithmetic` | Uncertainty quantification | [suites/interval_arithmetic.md](suites/interval_arithmetic.md) |
| `test_polynomial_chaos` | Uncertainty quantification | [suites/polynomial_chaos.md](suites/polynomial_chaos.md) |
| `test_uncertain_quantity` | Uncertainty quantification | [suites/uncertain_quantity.md](suites/uncertain_quantity.md) |
| `test_zonotope_propagation` | Uncertainty quantification | [suites/zonotope_propagation.md](suites/zonotope_propagation.md) |
| `test_uncertainty_metrology` | Uncertainty quantification | [suites/uncertainty_metrology.md](suites/uncertainty_metrology.md) |
| `test_simd_vec` | Tensor and SIMD math | [suites/simd_vec.md](suites/simd_vec.md) |
| `test_kerr_metrics` | Metrics and geodesics | [suites/kerr_metrics.md](suites/kerr_metrics.md) |
| `test_vacuum_cosmo_metrics` | Metrics and geodesics | [suites/vacuum_cosmo_metrics.md](suites/vacuum_cosmo_metrics.md) |
| `test_exotic_metrics` | Metrics and geodesics | [suites/exotic_metrics.md](suites/exotic_metrics.md) |
| `test_teves_metric` | Metrics and geodesics | [suites/teves_metric.md](suites/teves_metric.md) |
| `test_symplectic_gauss_legendre` | Integrators | [suites/symplectic_gauss_legendre.md](suites/symplectic_gauss_legendre.md) |
| `test_high_order_integrators` | Integrators | [suites/high_order_integrators.md](suites/high_order_integrators.md) |
| `test_hermite4_aarseth` | Integrators | [suites/hermite4_aarseth.md](suites/hermite4_aarseth.md) |
| `test_kerr_shadow_eht` | Gravity physics validation | [suites/kerr_shadow_eht.md](suites/kerr_shadow_eht.md) |
| `test_spherical_harmonics` | Post-Newtonian dynamics | [suites/spherical_harmonics.md](suites/spherical_harmonics.md) |
| `test_tidal_rotational_gravity` | Post-Newtonian dynamics | [suites/tidal_rotational_gravity.md](suites/tidal_rotational_gravity.md) |
| `test_hdf5_serializer` | Input and output | [suites/hdf5_serializer.md](suites/hdf5_serializer.md) |
| `test_terrell_penrose` | Rendering and optics | [suites/terrell_penrose.md](suites/terrell_penrose.md) |
| `test_cie_spectral_pipeline` | Rendering and optics | [suites/cie_spectral_pipeline.md](suites/cie_spectral_pipeline.md) |
| `test_rocket_dynamics` | Rendering and optics | [suites/rocket_dynamics.md](suites/rocket_dynamics.md) |
| `test_novikov_thorne_disk` | Hydrodynamics and radiation | [suites/novikov_thorne_disk.md](suites/novikov_thorne_disk.md) |
| `test_fishbone_moncrief_torus` | Hydrodynamics and radiation | [suites/fishbone_moncrief_torus.md](suites/fishbone_moncrief_torus.md) |

## Legacy

Descriptions are derived from the registered test names and must be confirmed during each audit.

| Category | Tests |
| --- | --- |
| Core infrastructure | `test_concurrent_command_injection`, `test_replay_loop` |
| Tensor and SIMD math | `test_tensor_algebra`, `test_geodesic_bundle`, `test_christoffel_solver` |
| Metrics and geodesics | `test_carter_constant`, `test_kerr_schild_geodesics`, `test_invariant_preservation`, `test_horizon_crossing` |
| Gravity physics validation | `test_solar_deflection`, `test_mercury_precession`, `test_shapiro_delay`, `test_nodal_precession_leo` |
| Dark matter and N-body | `test_dark_matter_field`, `test_barnes_hut_octree`, `test_galaxy_collision_nbody` |
| Input and output | `test_default_startup_scenario` |
| Rendering and optics | `test_vulkan_compute_pipeline`, `test_schwarzschild_shadow_render`, `test_observer_tetrad` |
| Hydrodynamics and radiation | `test_relativistic_hydro`, `test_eos_tov`, `test_polarized_radiative_transfer` |
| Uncertainty quantification | `test_covariance_propagation`, `test_uncertainty_pce` |
| Numerical relativity | `test_bssn_gauge_wave` |
| Interface | `test_ui_multi_window` |

Legacy tests rely on `assert`, which is compiled out when `NDEBUG` is defined. Migrating them to the harness therefore also restores their effect in optimized builds.

## Migration procedure

1. Run the test and record why it fails or what it covers.
2. Port it to `support/test_harness.hpp` following [CONTRIBUTING_TESTS.md](CONTRIBUTING_TESTS.md).
3. Add a page in `docs/testing/suites/` and move the entry to the Maintained table.
4. Commit the regenerated report from `docs/testing/results/`.

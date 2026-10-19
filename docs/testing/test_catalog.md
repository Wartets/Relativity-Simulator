# Test catalogue

Status values: `Maintained` (uses the shared harness, documented in `suites/`, results in `results/`), `Legacy` (registered, not yet audited or migrated to the harness).

## Maintained

| Test | Category | Documentation |
| --- | --- | --- |
| `test_static_metrics` | Metrics | [suites/static_metrics.md](suites/static_metrics.md) |
| `test_horizon_regime` | Metrics | [suites/horizon_regime.md](suites/horizon_regime.md) |
| `benchmark_simd_bundle` | Numerical performance | [suites/simd_bundle.md](suites/simd_bundle.md) |
| `test_double_single` | Tensor and SIMD math | [suites/double_single.md](suites/double_single.md) |
| `test_projection_modes` | Observer | [suites/projection_modes.md](suites/projection_modes.md) |
| `test_camera_navigation_modes` | Observer | [suites/camera_navigation_modes.md](suites/camera_navigation_modes.md) |
| `benchmark_engine_performance_quick` | Engine performance | [suites/engine_performance.md](suites/engine_performance.md) |
| `test_scheduler` | Core infrastructure | [suites/scheduler.md](suites/scheduler.md) |
| `test_repl_parser` | Core infrastructure | [suites/repl_parser.md](suites/repl_parser.md) |
| `test_spsc_queue` | Core infrastructure | [suites/spsc_queue.md](suites/spsc_queue.md) |
| `test_thread_pool` | Core infrastructure | [suites/thread_pool.md](suites/thread_pool.md) |
| `test_pcg64` | Core infrastructure | [suites/pcg64.md](suites/pcg64.md) |
| `test_memory_arena` | Core infrastructure | [suites/memory_arena.md](suites/memory_arena.md) |
| `test_scenario_serializer` | Input and output | [suites/scenario_serializer.md](suites/scenario_serializer.md) |
| `test_interval_arithmetic` | Uncertainty quantification | [suites/interval_arithmetic.md](suites/interval_arithmetic.md) |

## Legacy

Descriptions are derived from the registered test names and must be confirmed during each audit.

| Category | Tests |
| --- | --- |
| Core infrastructure | `test_concurrent_command_injection`, `test_replay_loop` |
| Tensor and SIMD math | `test_tensor_algebra`, `test_simd_vec`, `test_geodesic_bundle`, `test_christoffel_solver` |
| Metrics and geodesics | `test_vacuum_cosmo_metrics`, `test_exotic_metrics`, `test_kerr_metrics`, `test_carter_constant`, `test_kerr_schild_geodesics`, `test_invariant_preservation`, `test_horizon_crossing`, `test_teves_metric`, `test_f_r_chameleon` |
| Integrators | `test_symplectic_gauss_legendre`, `test_high_order_integrators`, `test_hermite4_aarseth` |
| Gravity physics validation | `test_solar_deflection`, `test_mercury_precession`, `test_shapiro_delay`, `test_hulse_taylor_orbital_decay`, `test_nodal_precession_leo`, `test_kerr_shadow_eht`, `test_hyperbolic_motion` |
| Post-Newtonian dynamics | `test_pn_conservative`, `test_pn_spin_precession`, `test_pn_radiation_reaction`, `test_gravitational_waves`, `test_spherical_harmonics`, `test_tidal_rotational_gravity` |
| Dark matter and N-body | `test_dark_matter_profiles`, `test_dark_matter_field`, `test_barnes_hut_octree`, `test_mond_rotation_curves`, `test_galaxy_collision_nbody` |
| Ephemerides and frames | `test_ephemeris_kepler`, `test_spk_reader`, `test_horizons_parser`, `test_frame_transforms` |
| Input and output | `test_fits_exporter`, `test_hdf5_serializer`, `test_vtk_exporter`, `test_default_startup_scenario` |
| Rendering and optics | `test_vulkan_compute_pipeline`, `test_schwarzschild_shadow_render`, `test_spectral_shift_beaming`, `test_terrell_penrose`, `test_cie_spectral_pipeline`, `test_observer_tetrad`, `test_camera_projections`, `test_rocket_dynamics` |
| Hydrodynamics and radiation | `test_relativistic_hydro`, `test_eos_tov`, `test_novikov_thorne_disk`, `test_fishbone_moncrief_torus`, `test_polarized_radiative_transfer` |
| Uncertainty quantification | `test_zonotope_propagation`, `test_covariance_propagation`, `test_polynomial_chaos`, `test_uncertain_quantity`, `test_uncertainty_metrology`, `test_uncertainty_pce` |
| Numerical relativity | `test_bssn_gauge_wave` |
| Interface | `test_ui_multi_window` |

## Migration procedure

1. Run the test and record why it fails or what it covers.
2. Port it to `support/test_harness.hpp` following [CONTRIBUTING_TESTS.md](CONTRIBUTING_TESTS.md).
3. Add a page in `docs/testing/suites/` and move the entry to the Maintained table.
4. Commit the regenerated report from `docs/testing/results/`.

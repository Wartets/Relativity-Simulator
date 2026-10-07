# Project Structure

```text
Relativity-Simulator/
├── .github/
│   └── worlflows/
│       └── ci.yml
│   
│
├── apps/
│   ├── engine_cli/
│   │   └── main.cpp
│   │
│   └── headless_exporter/
│       └── main.cpp
│   
│
├── assets/
│   ├── earth/
│   │   └── solarsystemscope/
│   │       ├── 1k_earth_daymap.png
│   │       ├── 1k_earth_nightmap.png
│   │       ├── 2k_earth_daymap.png
│   │       ├── 2k_earth_nightmap.png
│   │       └── LICENCE.txt
│   │   
│   │
│   ├── media/
│   │   ├── interface_preview.png
│   │   ├── schwarzschild_infall.gif
│   │   ├── schwarzschild_orbit_passby.gif
│   │   └── schwarzschild_preview.png
│   │
│   └── sky/
│       ├── ambientcg/
│       │   ├── NightSkyHDRI001_1K/
│       │   │   └── NightSkyHDRI001_1K_TONEMAPPED.jpg
│       │   │
│       │   ├── NightSkyHDRI001_2K/
│       │   │   └── NightSkyHDRI001_2K_TONEMAPPED.jpg
│       │   │
│       │   ├── NightSkyHDRI001_4K/
│       │   │   └── NightSkyHDRI001_4K_TONEMAPPED.jpg
│       │   │
│       │   ├── NightSkyHDRI008_1K/
│       │   │   └── NightSkyHDRI008_1K_TONEMAPPED.jpg
│       │   │
│       │   ├── NightSkyHDRI008_2K/
│       │   │   └── NightSkyHDRI008_2K_TONEMAPPED.jpg
│       │   │
│       │   ├── NightSkyHDRI008_4K/
│       │   │   └── NightSkyHDRI008_4K_TONEMAPPED.jpg
│       │   │
│       │   └── LICENCE.txt
│       │
│       └── eso/
│           ├── eso0932a.tif
│           └── LICENCE.txt
│       
│   
│
├── docs/
│   ├── ARCHITECTURE.md
│   ├── CLI_REFERENCE.md
│   ├── DESCRIPTION.md
│   ├── FILE_FORMATS.md
│   ├── INDEX.md
│   ├── MATHEMATICAL_FORMULATION.md
│   ├── TECHNICAL_MANUAL.md
│   └── TREE.md
│
├── include/
│   └── relativistic/
│       ├── capture/
│       │   ├── camera_path.hpp
│       │   ├── capture_coordinator.hpp
│       │   ├── capture_post_process.hpp
│       │   ├── capture_progress.hpp
│       │   ├── easing.hpp
│       │   ├── expression.hpp
│       │   ├── motion_script.hpp
│       │   ├── motion_script_file.hpp
│       │   ├── path_preview.hpp
│       │   ├── path_preview_builder.hpp
│       │   ├── physics_recorder.hpp
│       │   └── script_events.hpp
│       │
│       ├── core/
│       │   ├── math/
│       │   │   ├── christoffel.hpp
│       │   │   ├── four_vector_bundle.hpp
│       │   │   ├── geodesic_bundle.hpp
│       │   │   ├── riemann.hpp
│       │   │   ├── tensor.hpp
│       │   │   └── tensor_ops.hpp
│       │   │
│       │   ├── constants.hpp
│       │   ├── deterministic_replay.hpp
│       │   ├── engine_log.hpp
│       │   ├── engine_signature.hpp
│       │   ├── memory_arena.hpp
│       │   ├── pcg64.hpp
│       │   ├── physical_constants_engine.hpp
│       │   ├── schwarzschild_null_integrator.hpp
│       │   ├── sha256.hpp
│       │   ├── simd.hpp
│       │   ├── simd_math.hpp
│       │   ├── spsc_queue.hpp
│       │   ├── system_console.hpp
│       │   └── thread_pool.hpp
│       │
│       ├── dark_matter/
│       │   ├── barnes_hut.hpp
│       │   ├── dark_matter_profiles.hpp
│       │   └── galaxy_model.hpp
│       │
│       ├── dynamics/
│       │   ├── pn/
│       │   │   ├── pn_acceleration.hpp
│       │   │   ├── pn_body.hpp
│       │   │   ├── pn_gravitational_waves.hpp
│       │   │   ├── pn_integrator.hpp
│       │   │   ├── pn_nbody_system.hpp
│       │   │   ├── pn_orders.hpp
│       │   │   └── pn_spin_precession.hpp
│       │   │
│       │   ├── body_surface_layers.hpp
│       │   ├── bulk_body_actions.hpp
│       │   ├── hulse_taylor_pulsar.hpp
│       │   ├── interaction_compatibility.hpp
│       │   ├── interaction_config.hpp
│       │   └── interaction_solver.hpp
│       │
│       ├── gravimetry/
│       │   ├── legendre_table.hpp
│       │   ├── orbital_precession.hpp
│       │   ├── spherical_harmonics.hpp
│       │   └── tidal_perturbations.hpp
│       │
│       ├── hydro/
│       │   ├── solvers/
│       │   │   ├── con2prim.hpp
│       │   │   ├── constrained_transport.hpp
│       │   │   ├── grhd_solver.hpp
│       │   │   ├── reconstruction.hpp
│       │   │   ├── riemann_solvers.hpp
│       │   │   └── tov_solver.hpp
│       │   │
│       │   ├── eos.hpp
│       │   ├── fishbone_moncrief.hpp
│       │   ├── hydro_types.hpp
│       │   └── novikov_thorne.hpp
│       │
│       ├── integrators/
│       │   ├── cash_karp.hpp
│       │   ├── geodesic_state.hpp
│       │   ├── hermite4_aarseth.hpp
│       │   ├── horizon_manager.hpp
│       │   ├── rk45_adaptive.hpp
│       │   ├── step_controller.hpp
│       │   ├── symplectic_gauss_legendre.hpp
│       │   └── vernier9.hpp
│       │
│       ├── io/
│       │   ├── capture/
│       │   │   ├── capture_settings_io.hpp
│       │   │   ├── capture_studio_settings.hpp
│       │   │   ├── capture_target_settings.hpp
│       │   │   ├── recording_settings.hpp
│       │   │   ├── screenshot_capture_settings.hpp
│       │   │   ├── screenshot_exporter.hpp
│       │   │   └── video_capture_settings.hpp
│       │   │
│       │   ├── image/
│       │   │   ├── image_codecs.hpp
│       │   │   ├── image_format.hpp
│       │   │   └── image_stream_writers.hpp
│       │   │
│       │   ├── scenario/
│       │   │   ├── scenario_locator.hpp
│       │   │   └── scenario_serializer.hpp
│       │   │
│       │   ├── ephemeris_types.hpp
│       │   ├── fits_exporter.hpp
│       │   ├── frame_transforms.hpp
│       │   ├── hdf5_serializer.hpp
│       │   ├── horizons_parser.hpp
│       │   ├── spk_reader.hpp
│       │   ├── telemetry_table.hpp
│       │   ├── user_settings.hpp
│       │   └── vtk_exporter.hpp
│       │
│       ├── metrics/
│       │   ├── bssn/
│       │   │   ├── bssn_constraints.hpp
│       │   │   ├── bssn_evolution.hpp
│       │   │   ├── bssn_grid.hpp
│       │   │   ├── bssn_interpolation.hpp
│       │   │   └── bssn_metric.hpp
│       │   │
│       │   ├── alcubierre.hpp
│       │   ├── bardeen_shadow.hpp
│       │   ├── eddington_finkelstein.hpp
│       │   ├── flat_minkowski.hpp
│       │   ├── flrw.hpp
│       │   ├── kerr.hpp
│       │   ├── kerr_de_sitter.hpp
│       │   ├── kerr_invariants.hpp
│       │   ├── kerr_newman.hpp
│       │   ├── kerr_schild.hpp
│       │   ├── morris_thorne.hpp
│       │   ├── painleve_gullstrand.hpp
│       │   ├── reissner_nordstrom.hpp
│       │   ├── schwarzschild.hpp
│       │   ├── schwarzschild_de_sitter.hpp
│       │   ├── schwarzschild_isotropic.hpp
│       │   ├── spacetime_concept.hpp
│       │   └── subsidiary_source_field.hpp
│       │
│       ├── modified_gravity/
│       │   ├── f_r_gravity.hpp
│       │   ├── mond.hpp
│       │   └── teves.hpp
│       │
│       ├── observer/
│       │   ├── camera_collision.hpp
│       │   ├── camera_projections.hpp
│       │   ├── direction_projection.hpp
│       │   ├── observer_tetrad.hpp
│       │   ├── planet_orbit.hpp
│       │   ├── rocket_dynamics.hpp
│       │   ├── surface_geometry.hpp
│       │   ├── surface_walker.hpp
│       │   └── tetrad_transport.hpp
│       │
│       ├── optics/
│       │   ├── textures/
│       │   │   ├── earth_texture_catalog.hpp
│       │   │   ├── earth_texture_image.hpp
│       │   │   ├── sky_panorama_catalog.hpp
│       │   │   ├── sky_panorama_codecs.hpp
│       │   │   └── sky_panorama_image.hpp
│       │   │
│       │   ├── carter_ray_classifier.hpp
│       │   ├── cie_observer.hpp
│       │   ├── disk_thermal_profile.hpp
│       │   ├── inverse_compton.hpp
│       │   ├── maxwell_juttner.hpp
│       │   ├── polarized_radiative_transfer.hpp
│       │   ├── radiative_processes.hpp
│       │   ├── spectral_shift.hpp
│       │   ├── spectrum.hpp
│       │   ├── stokes_vector.hpp
│       │   ├── terrell_penrose.hpp
│       │   └── tonemapping.hpp
│       │
│       ├── orchestrator/
│       │   ├── command.hpp
│       │   ├── performance_profiler.hpp
│       │   ├── repl.hpp
│       │   ├── scheduler.hpp
│       │   ├── session_state.hpp
│       │   └── simulation_orchestrator.hpp
│       │
│       ├── render/
│       │   ├── bodies/
│       │   │   ├── body_lighting.hpp
│       │   │   ├── body_surface_shading.hpp
│       │   │   ├── earth_surface_shading.hpp
│       │   │   ├── earth_terminator.hpp
│       │   │   └── earth_texture_requirements.hpp
│       │   │
│       │   ├── accretion_disk_model.hpp
│       │   ├── accretion_disk_settings.hpp
│       │   ├── double_single.hpp
│       │   ├── geodesic_compute_pipeline.hpp
│       │   ├── gpu_types.hpp
│       │   ├── software_compute_engine.hpp
│       │   ├── vulkan_compute_executor.hpp
│       │   └── vulkan_context.hpp
│       │
│       ├── ui/
│       │   ├── hud/
│       │   │   ├── hud_layout_config.hpp
│       │   │   ├── hud_manager_window.hpp
│       │   │   └── hud_preferences.hpp
│       │   │
│       │   ├── schematic/
│       │   │   ├── schematic_primary_source_overlay.hpp
│       │   │   ├── schematic_view_config.hpp
│       │   │   └── schematic_view_renderer.hpp
│       │   │
│       │   ├── spatial_reference/
│       │   │   ├── spatial_reference_config.hpp
│       │   │   ├── spatial_reference_editor.hpp
│       │   │   └── spatial_reference_renderer.hpp
│       │   │
│       │   ├── accretion_disk_editor.hpp
│       │   ├── body_manager_window.hpp
│       │   ├── camera_control_config.hpp
│       │   ├── capture_studio_window.hpp
│       │   ├── capture_widgets.hpp
│       │   ├── compatibility_notes.hpp
│       │   ├── constants_window.hpp
│       │   ├── control_panel_window.hpp
│       │   ├── input_actions.hpp
│       │   ├── interactive_camera_controller.hpp
│       │   ├── interface_persistence.hpp
│       │   ├── keybind_settings_window.hpp
│       │   ├── log_console_window.hpp
│       │   ├── motion_script_editor.hpp
│       │   ├── numeric_slider_utils.hpp
│       │   ├── performance_analysis_window.hpp
│       │   ├── performance_settings_window.hpp
│       │   ├── scenario_selector_window.hpp
│       │   ├── secondary_view_window.hpp
│       │   ├── secondary_viewport_manager.hpp
│       │   ├── spectrograph_window.hpp
│       │   ├── telemetry_window.hpp
│       │   ├── tooltip_utils.hpp
│       │   ├── ui_manager.hpp
│       │   ├── viewport_primary_window.hpp
│       │   ├── visual_diagnostics_window.hpp
│       │   └── window_chrome.hpp
│       │
│       ├── uncertainty/
│       │   ├── covariance.hpp
│       │   ├── interval.hpp
│       │   ├── metrology.hpp
│       │   ├── monte_carlo_bundle.hpp
│       │   ├── orbital_wrapping_benchmark.hpp
│       │   ├── pce_geodesic.hpp
│       │   ├── polynomial_chaos.hpp
│       │   ├── uncertain_quantity.hpp
│       │   ├── uncertainty_types.hpp
│       │   ├── variational_geodesic.hpp
│       │   └── zonotope.hpp
│       │
│       └── units/
│           ├── expression_engine.hpp
│           ├── unit_aware_widgets.hpp
│           └── unit_system.hpp
│       
│   
│
├── scenarios/
│   ├── alcubierre_warp_bubble.yaml
│   ├── annihilation_matter_antimatter.yaml
│   ├── binary_black_hole_inspiral.yaml
│   ├── collision_cascade_demo.yaml
│   ├── cometary_flyby_high_eccentricity.yaml
│   ├── electromagnetic_binary_charges.yaml
│   ├── extremal_kerr_near_horizon.yaml
│   ├── flrw_cosmological_expansion.yaml
│   ├── galaxy_collision.yaml
│   ├── grand_tour_multiplanet_system.yaml
│   ├── hulse_taylor_pulsar.yaml
│   ├── kerr_accretion_disk.yaml
│   ├── kerr_black_hole.yaml
│   ├── kerr_newman_charged_rotating.yaml
│   ├── morris_thorne_wormhole.yaml
│   ├── reissner_nordstrom_charged.yaml
│   ├── relativistic_shock_tube.yaml
│   ├── schwarzschild_accretion.yaml
│   ├── schwarzschild_de_sitter_lambda.yaml
│   ├── solar_system_mercury.yaml
│   ├── thermodynamic_radiative_cooling.yaml
│   └── tidal_disruption_fragmentation.yaml
│
├── shaders/
│   ├── geodesic_tracer_ds.comp
│   ├── geodesic_tracer_fp64.comp
│   └── monte_carlo_shadow.comp
│
├── tests/
│   ├── benchmark_simd_bundle.cpp
│   ├── test_barnes_hut_octree.cpp
│   ├── test_bssn_gauge_wave.cpp
│   ├── test_camera_projections.cpp
│   ├── test_carter_constant.cpp
│   ├── test_christoffel_solver.cpp
│   ├── test_cie_spectral_pipeline.cpp
│   ├── test_concurrent_command_injection.cpp
│   ├── test_covariance_propagation.cpp
│   ├── test_dark_matter_profiles.cpp
│   ├── test_default_startup_scenario.cpp
│   ├── test_double_single.cpp
│   ├── test_eos_tov.cpp
│   ├── test_ephemeris_kepler.cpp
│   ├── test_exotic_metrics.cpp
│   ├── test_f_r_chameleon.cpp
│   ├── test_fishbone_moncrief_torus.cpp
│   ├── test_fits_exporter.cpp
│   ├── test_frame_transforms.cpp
│   ├── test_galaxy_collision_nbody.cpp
│   ├── test_geodesic_bundle.cpp
│   ├── test_gravitational_waves.cpp
│   ├── test_hdf5_serializer.cpp
│   ├── test_hermite4_aarseth.cpp
│   ├── test_high_order_integrators.cpp
│   ├── test_horizon_crossing.cpp
│   ├── test_horizons_parser.cpp
│   ├── test_hulse_taylor_orbital_decay.cpp
│   ├── test_hyperbolic_motion.cpp
│   ├── test_interval_arithmetic.cpp
│   ├── test_invariant_preservation.cpp
│   ├── test_kerr_metrics.cpp
│   ├── test_kerr_schild_geodesics.cpp
│   ├── test_kerr_shadow_eht.cpp
│   ├── test_memory_arena.cpp
│   ├── test_mercury_precession.cpp
│   ├── test_mond_rotation_curves.cpp
│   ├── test_nodal_precession_leo.cpp
│   ├── test_novikov_thorne_disk.cpp
│   ├── test_observer_tetrad.cpp
│   ├── test_pcg64.cpp
│   ├── test_pn_conservative.cpp
│   ├── test_pn_radiation_reaction.cpp
│   ├── test_pn_spin_precession.cpp
│   ├── test_polarized_radiative_transfer.cpp
│   ├── test_polynomial_chaos.cpp
│   ├── test_relativistic_hydro.cpp
│   ├── test_repl_parser.cpp
│   ├── test_replay_loop.cpp
│   ├── test_rocket_dynamics.cpp
│   ├── test_scenario_serializer.cpp
│   ├── test_scheduler.cpp
│   ├── test_schwarzschild_shadow_render.cpp
│   ├── test_shapiro_delay.cpp
│   ├── test_simd_vec.cpp
│   ├── test_solar_deflection.cpp
│   ├── test_spectral_shift_beaming.cpp
│   ├── test_spherical_harmonics.cpp
│   ├── test_spk_reader.cpp
│   ├── test_spsc_queue.cpp
│   ├── test_static_metrics.cpp
│   ├── test_symplectic_gauss_legendre.cpp
│   ├── test_tensor_algebra.cpp
│   ├── test_terrell_penrose.cpp
│   ├── test_teves_metric.cpp
│   ├── test_thread_pool.cpp
│   ├── test_tidal_rotational_gravity.cpp
│   ├── test_ui_multi_window.cpp
│   ├── test_uncertain_quantity.cpp
│   ├── test_uncertainty_metrology.cpp
│   ├── test_uncertainty_pce.cpp
│   ├── test_vacuum_cosmo_metrics.cpp
│   ├── test_vtk_exporter.cpp
│   ├── test_vulkan_compute_pipeline.cpp
│   └── test_zonotope_propagation.cpp
│
├── .gitattributes
├── .gitignore
├── CMakeLists.txt
├── CODE_OF_CONDUCT.md
├── CONTRIBUTING.md
├── LICENSE
└── README.md
```

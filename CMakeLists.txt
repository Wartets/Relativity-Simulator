cmake_minimum_required(VERSION 3.25)

project(RelativisticEngine
	VERSION 0.0.1
	DESCRIPTION "Relativistic Physics and Raytracing Engine"
	LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
	add_compile_options(
		-Wall
		-Wextra
		-Wpedantic
		-Werror
		-Wconversion
		-Wshadow
		-Wnon-virtual-dtor
		-Wold-style-cast
		-Wcast-align
		-Wunused
		-Woverloaded-virtual
		-Wnull-dereference
		-Wdouble-promotion
		-Wformat=2
		-frounding-math
		-fno-math-errno
	)
	if (NOT MSVC)
		add_compile_options(-march=native)
	endif()
elseif (MSVC)
	add_compile_options(
		/W4
		/WX
		/permissive-
		/fp:strict
		/Zc:__cplusplus
		/utf-8
	)
endif()

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_library(relativistic_core INTERFACE)
target_include_directories(relativistic_core INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:include>
)
target_compile_features(relativistic_core INTERFACE cxx_std_23)

include(CTest)
enable_testing()

add_executable(test_memory_arena tests/test_memory_arena.cpp)
target_link_libraries(test_memory_arena PRIVATE relativistic_core)
add_test(NAME test_memory_arena COMMAND test_memory_arena)

add_executable(test_pcg64 tests/test_pcg64.cpp)
target_link_libraries(test_pcg64 PRIVATE relativistic_core)
add_test(NAME test_pcg64 COMMAND test_pcg64)

add_executable(test_tensor_algebra tests/test_tensor_algebra.cpp)
target_link_libraries(test_tensor_algebra PRIVATE relativistic_core)
add_test(NAME test_tensor_algebra COMMAND test_tensor_algebra)

add_executable(test_simd_vec tests/test_simd_vec.cpp)
target_link_libraries(test_simd_vec PRIVATE relativistic_core)
add_test(NAME test_simd_vec COMMAND test_simd_vec)

add_executable(test_geodesic_bundle tests/test_geodesic_bundle.cpp)
target_link_libraries(test_geodesic_bundle PRIVATE relativistic_core)
add_test(NAME test_geodesic_bundle COMMAND test_geodesic_bundle)

add_executable(benchmark_simd_bundle tests/benchmark_simd_bundle.cpp)
target_link_libraries(benchmark_simd_bundle PRIVATE relativistic_core)
add_test(NAME benchmark_simd_bundle COMMAND benchmark_simd_bundle)

add_executable(test_christoffel_solver tests/test_christoffel_solver.cpp)
target_link_libraries(test_christoffel_solver PRIVATE relativistic_core)
add_test(NAME test_christoffel_solver COMMAND test_christoffel_solver)

add_executable(test_replay_loop tests/test_replay_loop.cpp)
target_link_libraries(test_replay_loop PRIVATE relativistic_core)
add_test(NAME test_replay_loop COMMAND test_replay_loop)

add_executable(test_solar_deflection tests/test_solar_deflection.cpp)
target_link_libraries(test_solar_deflection PRIVATE relativistic_core)
add_test(NAME test_solar_deflection COMMAND test_solar_deflection)

add_executable(test_static_metrics tests/test_static_metrics.cpp)
target_link_libraries(test_static_metrics PRIVATE relativistic_core)
add_test(NAME test_static_metrics COMMAND test_static_metrics)

add_executable(test_invariant_preservation tests/test_invariant_preservation.cpp)
target_link_libraries(test_invariant_preservation PRIVATE relativistic_core)
add_test(NAME test_invariant_preservation COMMAND test_invariant_preservation)

add_executable(test_horizon_crossing tests/test_horizon_crossing.cpp)
target_link_libraries(test_horizon_crossing PRIVATE relativistic_core)
add_test(NAME test_horizon_crossing COMMAND test_horizon_crossing)

add_executable(test_kerr_metrics tests/test_kerr_metrics.cpp)
target_link_libraries(test_kerr_metrics PRIVATE relativistic_core)
add_test(NAME test_kerr_metrics COMMAND test_kerr_metrics)

add_executable(test_carter_constant tests/test_carter_constant.cpp)
target_link_libraries(test_carter_constant PRIVATE relativistic_core)
add_test(NAME test_carter_constant COMMAND test_carter_constant)

add_executable(test_kerr_schild_geodesics tests/test_kerr_schild_geodesics.cpp)
target_link_libraries(test_kerr_schild_geodesics PRIVATE relativistic_core)
add_test(NAME test_kerr_schild_geodesics COMMAND test_kerr_schild_geodesics)

add_executable(test_vacuum_cosmo_metrics tests/test_vacuum_cosmo_metrics.cpp)
target_link_libraries(test_vacuum_cosmo_metrics PRIVATE relativistic_core)
add_test(NAME test_vacuum_cosmo_metrics COMMAND test_vacuum_cosmo_metrics)

add_executable(test_exotic_metrics tests/test_exotic_metrics.cpp)
target_link_libraries(test_exotic_metrics PRIVATE relativistic_core)
add_test(NAME test_exotic_metrics COMMAND test_exotic_metrics)

add_executable(test_symplectic_gauss_legendre tests/test_symplectic_gauss_legendre.cpp)
target_link_libraries(test_symplectic_gauss_legendre PRIVATE relativistic_core)
add_test(NAME test_symplectic_gauss_legendre COMMAND test_symplectic_gauss_legendre)

add_executable(test_high_order_integrators tests/test_high_order_integrators.cpp)
target_link_libraries(test_high_order_integrators PRIVATE relativistic_core)
add_test(NAME test_high_order_integrators COMMAND test_high_order_integrators)

add_executable(test_hermite4_aarseth tests/test_hermite4_aarseth.cpp)
target_link_libraries(test_hermite4_aarseth PRIVATE relativistic_core)
add_test(NAME test_hermite4_aarseth COMMAND test_hermite4_aarseth)

add_executable(test_mercury_precession tests/test_mercury_precession.cpp)
target_link_libraries(test_mercury_precession PRIVATE relativistic_core)
add_test(NAME test_mercury_precession COMMAND test_mercury_precession)

add_executable(test_shapiro_delay tests/test_shapiro_delay.cpp)
target_link_libraries(test_shapiro_delay PRIVATE relativistic_core)
add_test(NAME test_shapiro_delay COMMAND test_shapiro_delay)

add_executable(test_spsc_queue tests/test_spsc_queue.cpp)
target_link_libraries(test_spsc_queue PRIVATE relativistic_core)
add_test(NAME test_spsc_queue COMMAND test_spsc_queue)

add_executable(test_repl_parser tests/test_repl_parser.cpp)
target_link_libraries(test_repl_parser PRIVATE relativistic_core)
add_test(NAME test_repl_parser COMMAND test_repl_parser)

add_executable(test_scheduler tests/test_scheduler.cpp)
target_link_libraries(test_scheduler PRIVATE relativistic_core)
add_test(NAME test_scheduler COMMAND test_scheduler)

add_executable(test_concurrent_command_injection tests/test_concurrent_command_injection.cpp)
target_link_libraries(test_concurrent_command_injection PRIVATE relativistic_core)
add_test(NAME test_concurrent_command_injection COMMAND test_concurrent_command_injection)

add_executable(test_ephemeris_kepler tests/test_ephemeris_kepler.cpp)
target_link_libraries(test_ephemeris_kepler PRIVATE relativistic_core)
add_test(NAME test_ephemeris_kepler COMMAND test_ephemeris_kepler)

add_executable(test_spk_reader tests/test_spk_reader.cpp)
target_link_libraries(test_spk_reader PRIVATE relativistic_core)
add_test(NAME test_spk_reader COMMAND test_spk_reader)

add_executable(test_horizons_parser tests/test_horizons_parser.cpp)
target_link_libraries(test_horizons_parser PRIVATE relativistic_core)
add_test(NAME test_horizons_parser COMMAND test_horizons_parser)

add_executable(test_frame_transforms tests/test_frame_transforms.cpp)
target_link_libraries(test_frame_transforms PRIVATE relativistic_core)
add_test(NAME test_frame_transforms COMMAND test_frame_transforms)

add_executable(test_scenario_serializer tests/test_scenario_serializer.cpp)
target_link_libraries(test_scenario_serializer PRIVATE relativistic_core)
add_test(NAME test_scenario_serializer COMMAND test_scenario_serializer)

add_executable(test_fits_exporter tests/test_fits_exporter.cpp)
target_link_libraries(test_fits_exporter PRIVATE relativistic_core)
add_test(NAME test_fits_exporter COMMAND test_fits_exporter)

add_executable(test_hdf5_serializer tests/test_hdf5_serializer.cpp)
target_link_libraries(test_hdf5_serializer PRIVATE relativistic_core)
add_test(NAME test_hdf5_serializer COMMAND test_hdf5_serializer)

add_executable(test_vtk_exporter tests/test_vtk_exporter.cpp)
target_link_libraries(test_vtk_exporter PRIVATE relativistic_core)
add_test(NAME test_vtk_exporter COMMAND test_vtk_exporter)

add_executable(test_double_single tests/test_double_single.cpp)
target_link_libraries(test_double_single PRIVATE relativistic_core)
add_test(NAME test_double_single COMMAND test_double_single)

add_executable(test_observer_tetrad tests/test_observer_tetrad.cpp)
target_link_libraries(test_observer_tetrad PRIVATE relativistic_core)
add_test(NAME test_observer_tetrad COMMAND test_observer_tetrad)

add_executable(test_vulkan_compute_pipeline tests/test_vulkan_compute_pipeline.cpp)
target_link_libraries(test_vulkan_compute_pipeline PRIVATE relativistic_core)
add_test(NAME test_vulkan_compute_pipeline COMMAND test_vulkan_compute_pipeline)

add_executable(test_schwarzschild_shadow_render tests/test_schwarzschild_shadow_render.cpp)
target_link_libraries(test_schwarzschild_shadow_render PRIVATE relativistic_core)
add_test(NAME test_schwarzschild_shadow_render COMMAND test_schwarzschild_shadow_render)

add_executable(test_spectral_shift_beaming tests/test_spectral_shift_beaming.cpp)
target_link_libraries(test_spectral_shift_beaming PRIVATE relativistic_core)
add_test(NAME test_spectral_shift_beaming COMMAND test_spectral_shift_beaming)

add_executable(test_terrell_penrose tests/test_terrell_penrose.cpp)
target_link_libraries(test_terrell_penrose PRIVATE relativistic_core)
add_test(NAME test_terrell_penrose COMMAND test_terrell_penrose)

add_executable(test_cie_spectral_pipeline tests/test_cie_spectral_pipeline.cpp)
target_link_libraries(test_cie_spectral_pipeline PRIVATE relativistic_core)
add_test(NAME test_cie_spectral_pipeline COMMAND test_cie_spectral_pipeline)

add_executable(test_kerr_shadow_eht tests/test_kerr_shadow_eht.cpp)
target_link_libraries(test_kerr_shadow_eht PRIVATE relativistic_core)
add_test(NAME test_kerr_shadow_eht COMMAND test_kerr_shadow_eht)

add_executable(engine_cli apps/engine_cli/main.cpp)
target_link_libraries(engine_cli PRIVATE relativistic_core)

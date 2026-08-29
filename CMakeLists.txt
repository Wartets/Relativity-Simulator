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

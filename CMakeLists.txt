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

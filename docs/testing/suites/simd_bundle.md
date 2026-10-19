# SIMD geodesic bundle benchmark

Executable: `benchmark_simd_bundle` - Source: `tests/benchmark_simd_bundle.cpp` - Results: [benchmark_simd_bundle.md](../results/benchmark_simd_bundle.md)

## Purpose

Validates `GeodesicBundle::step_rk4_schwarzschild` against an independent scalar RK4 integrator of the full four-dimensional Schwarzschild geodesic equations, and measures bundle throughput.

## Scenario

16384 null rays start at r = 25 M in the equatorial plane with impact parameters from 3 to 13 M. Each ray advances 100 RK4 steps of 0.01 affine units. The scalar reference implements the same Christoffel symbols, horizon threshold (1.001 r_s), escape radius (50 r_s) and guards as the bundle.

## Checks

| Check | Criterion |
| --- | --- |
| Positions | Relative deviation from the scalar reference below 1e-9 for every ray, for 4-wide and 8-wide bundles |
| Momenta | Same criterion on all four momentum components |
| Affine parameter | Matches the reference to 1e-12 |
| Lane masks | Active, horizon and celestial states identical to the reference |
| Null condition | `g_mu_nu p^mu p^nu` stays below 1e-7 for every active ray |

## Measurements

Best of five timings for the scalar reference and both bundle widths, speed-up ratios and throughput in millions of ray-steps per second. Measurements are informational; the bundle currently executes lane loops, so ratios near one are expected until intrinsics are used.

## History

The previous benchmark could not pass: the bundle did not integrate the time component of the momentum and froze `p0 = 1/f`, and the old reference used a different escape radius. The bundle now integrates `dp0 = -2 Gamma^0_01 p0 p1` and uses the evolving `p0` in the radial equation.

## Results

See [../results/benchmark_simd_bundle.md](../results/benchmark_simd_bundle.md).

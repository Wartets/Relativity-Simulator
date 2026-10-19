# Geodesic bundle suite

Executable: `test_geodesic_bundle` - Source: `tests/test_geodesic_bundle.cpp` - Results: [geodesic_bundle.md](../results/geodesic_bundle.md)

## Purpose

Verifies the structure-of-arrays ray bundles of [TECHNICAL_MANUAL.md](../../TECHNICAL_MANUAL.md#21-static-tensor--simd-algebra-layer) and the four-vector helpers they use. Lane-by-lane agreement with a scalar reference is covered by [simd_bundle.md](simd_bundle.md).

## Scenario

Four-wide vectors for the Minkowski product, the Lorentz factor and the velocity addition; an eight-lane double bundle of rays from r = 20 with impact parameters 4 to 7.5, evolved for up to 500 Runge-Kutta steps. The allocation trap is active during the computation only.

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Four-vector algebra | Minkowski product | `-u0 v0 + u.v` in every lane within 1e-15 |
| Four-vector algebra | Lorentz factor | 1.25, 1.25, 1/0.6 and 1 within 1e-14 |
| Four-vector algebra | Velocity addition | 0.5 c and -0.5 c give 0 along x (second lane) and the expected first lane within 1e-14 |
| Ray bundle | Initial state | All eight lanes active |
| Ray bundle | Evolution | Every lane keeps a positive radius |
| Allocation discipline | Heap | Zero allocations |

## Justification

The expected values are closed forms of special relativity. A positive radius after the evolution excludes numerical blow up of any lane.

## Acceptance

All checks pass.

## Recorded results

See [../results/geodesic_bundle.md](../results/geodesic_bundle.md).

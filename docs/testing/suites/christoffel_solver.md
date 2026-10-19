# Christoffel solver suite

Executable: `test_christoffel_solver` - Source: `tests/test_christoffel_solver.cpp` - Results: [christoffel_solver.md](../results/christoffel_solver.md)

## Purpose

Verifies `compute_christoffel_numerical` of [MATHEMATICAL_FORMULATION.md](../../MATHEMATICAL_FORMULATION.md#21-christoffel-symbols-of-the-second-kind) at fourth, sixth and eighth order against analytic connections, and the dispatcher `compute_christoffel` of [ARCHITECTURE.md](../../ARCHITECTURE.md#21-tensor-algebra--curvature-evaluation-layer).

## Scenario

Minkowski at (10, 5, 2, -3) and Schwarzschild (M = 1) at four points with r from 3 to 50. The global `operator new` is trapped during the computation.

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Flat spacetime | Orders 4, 6, 8 and dispatch | Absolute error below 1e-15 |
| Schwarzschild spacetime | Lower-index symmetry | Below 1e-15 for every order |
| Schwarzschild spacetime | Fourth order | Relative error below 1e-6 |
| Schwarzschild spacetime | Sixth order | Relative error below 1e-9 |
| Schwarzschild spacetime | Eighth order | Relative error below 1e-12, absolute below 1e-12 |
| Schwarzschild spacetime | Ordering | Sixth-order error never above fourth-order error |
| Schwarzschild spacetime | Dispatch | Exactly the analytic connection when available |
| Schwarzschild spacetime | Wrapper | `NumericalMetricWrapper` reproduces the connection within a relative 1e-12 |
| Allocation discipline | Heap | Zero allocations |

## Justification

The step is scaled as `h0 max(1, |x|)`, with truncation error `O(h^n)` and rounding error `O(eps/h)`; the quoted tolerances sit above both for the tested radii. The relative error only considers components larger than 1e-14.

## Limits

The relative criterion excludes tiny components; the scale-aware row criterion is exercised in [static_metrics.md](static_metrics.md).

## Acceptance

All checks pass.

## Recorded results

See [../results/christoffel_solver.md](../results/christoffel_solver.md).

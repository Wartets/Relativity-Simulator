# BSSN gauge wave suite

Executable: `test_bssn_gauge_wave` - Source: `tests/test_bssn_gauge_wave.cpp` - Results: [bssn_gauge_wave.md](../results/bssn_gauge_wave.md)

## Purpose

Verifies the stability of `BssnGrid`, `BssnEvolution` and `BssnConstraints` ([DESCRIPTION.md](../../DESCRIPTION.md#44-31-numerical-relativity-grids-bssn)). The grid is not used by the renderer ([TECHNICAL_MANUAL.md](../../TECHNICAL_MANUAL.md#metric-usage-in-softwarecomputeengine)).

## Scenario

A 64 by 4 by 4 grid with unit extent in each direction, gauge wave `H = 1 - 0.1 sin(2 pi x)` with lapse `sqrt(H)`, conformal metric `diag(H^(2/3), H^(-1/3), H^(-1/3))`, `phi = ln(H)/12` and extrinsic curvature `H'/(2 H^(3/2))`, evolved for 1000 fourth-order Runge-Kutta steps of 0.25 dx.

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Initial data | Constraint | The initial Hamiltonian constraint is finite |
| Evolution | Constraint | Finite at every checkpoint (every 100 steps) |
| Evolution | Lapse | Finite and positive everywhere at the end |
| Evolution | Growth | The peak Hamiltonian norm never exceeds 100 times the initial norm (floor 1e-6) |

## Justification

The gauge wave is an exact vacuum solution, so a correct evolution keeps the constraint at its discretization level. A growth beyond two orders of magnitude, a non-finite value or a collapse of the lapse indicates a numerical instability.

## Limits

The suite measures stability, not the convergence order, and applies a bound chosen as a stability limit rather than a derived tolerance.

## Measurements

Initial, intermediate and peak Hamiltonian constraint norms.

## Acceptance

All checks pass.

## Recorded results

See [../results/bssn_gauge_wave.md](../results/bssn_gauge_wave.md).

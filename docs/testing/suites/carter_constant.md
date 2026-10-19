# Carter constant suite

Executable: `test_carter_constant` - Source: `tests/test_carter_constant.cpp` - Results: [carter_constant.md](../results/carter_constant.md)

## Purpose

Verifies the conservation of the Kerr constants of motion computed by `compute_kerr_invariants_bl` ([TECHNICAL_MANUAL.md](../../TECHNICAL_MANUAL.md#22-spacetime-metric-implementations)) along a geodesic of the Dormand-Prince integrator ([ARCHITECTURE.md](../../ARCHITECTURE.md#23-differential-solvers--geodesic-integrators)).

## Scenario

Kerr with M = 1 and a = 0.85, a timelike particle at (r, theta) = (10, pi/3) with u_r = 0, u_theta = 0.004, u_phi = 0.033, rtol 1e-13, atol 1e-15, steps from 0.02 to 0.08, 20000 steps sampled every 100.

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Initial conditions | Normalization | The quadratic for the temporal velocity has a real root |
| Initial conditions | Carter constant and energy | Positive |
| Conservation | Steps | Every adaptive step succeeds |
| Conservation | Carter constant | Relative drift below 1e-12 |
| Conservation | Energy | Relative drift below 1e-12 |
| Conservation | Axial angular momentum | Relative drift below 1e-12 |

## Justification

The three quantities are exact integrals of the Kerr geodesic flow, so their drift measures the integration error alone; the tolerance follows from the local tolerance of 1e-13 accumulated over the run.

## Measurements

The three relative drifts.

## Acceptance

All checks pass.

## Recorded results

See [../results/carter_constant.md](../results/carter_constant.md).

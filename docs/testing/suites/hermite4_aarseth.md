# Hermite 4th-order suite

Executable: `test_hermite4_aarseth` - Source: `tests/test_hermite4_aarseth.cpp` - Results: [hermite4_aarseth.md](../results/hermite4_aarseth.md)

## Purpose

Verifies the predictor-corrector integrator with Aarseth step selection of [ARCHITECTURE.md](../../ARCHITECTURE.md#23-differential-solvers--geodesic-integrators).

## Scenario

Circular Schwarzschild orbit at r = 10 M, eta = 0.02, step bounds 1e-6 and 1, one thousand variable steps starting from 0.02.

## Checks

| Check | Criterion |
| --- | --- |
| Accumulated proper time | Positive |
| Step count | Exactly 1000 steps recorded |
| Orbital radius | Stays at 10 within 1e-4 |

## Justification

The radius of a circular geodesic is constant, so its drift quantifies the truncation error; the step count verifies that the statistics follow every call.

## Acceptance

All checks pass.

## Recorded results

See [../results/hermite4_aarseth.md](../results/hermite4_aarseth.md).

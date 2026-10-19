# Hyperbolic motion suite

Executable: `test_hyperbolic_motion` - Source: `tests/test_hyperbolic_motion.cpp` - Results: [hyperbolic_motion.md](../results/hyperbolic_motion.md)

## Purpose

Validates the relativistic rocket of [DESCRIPTION.md](../../DESCRIPTION.md#81-observer-definition--comobile-orthonormal-tetrads) against the closed-form solution of constant proper acceleration in flat spacetime.

## Scenario

Proper accelerations of 0.1, 1, 9.81, 50 and 100 and coordinate times of 0.1, 1, 5 and 20, in coordinate-time flow with a step of 5e-4, starting at rest.

## Checks

| Check | Criterion |
| --- | --- |
| Proper time | Worst relative error over the 20 cases below 1e-7 |
| Velocity | Worst relative error below 1e-7 |
| Position | Worst relative error below 1e-6 |
| Lorentz factor | Worst relative error below 1e-7 |

## Measurements

The four worst relative errors.

## Acceptance

All checks pass. See also the closely related rocket checks in the legacy `test_rocket_dynamics` listed in [../test_catalog.md](../test_catalog.md).

## Recorded results

See [../results/hyperbolic_motion.md](../results/hyperbolic_motion.md).

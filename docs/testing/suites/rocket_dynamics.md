# Rocket dynamics suite

Executable: `test_rocket_dynamics` - Source: `tests/test_rocket_dynamics.cpp` - Results: [rocket_dynamics.md](../results/rocket_dynamics.md)

## Purpose

Verifies the relativistic rocket of [DESCRIPTION.md](../../DESCRIPTION.md#81-observer-definition--comobile-orthonormal-tetrads). The parameter sweep against the hyperbolic solution is in [hyperbolic_motion.md](hyperbolic_motion.md).

## Scenario

A rocket at rest in flat spacetime with a proper acceleration of 2 (10000 proper-time steps of 1e-4), with a proper acceleration of 1.5 in coordinate-time flow (2000 steps of 1e-3), and a yaw rate of pi/2 for one unit of proper time.

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Proper-time flow | Coordinate time | sinh(a tau) / a within 1e-7 |
| Proper-time flow | Proper time, velocity, position, Lorentz factor | Hyperbolic solution within 1e-9, 1e-9, 1e-7 and 1e-8 |
| Coordinate-time flow | Elapsed time | 2 within 1e-4 |
| Coordinate-time flow | Velocity | Hyperbolic solution within 1e-6 |
| Spatial rotation | Orthonormality | The tetrad stays orthonormal within 1e-12 |
| Spatial rotation | Quarter turn | The first spatial leg moves from x to the z axis within 1e-12 |

## Justification

A quarter-turn rotation is exact for the tetrad update, so the transformed leg must align with the target axis to rounding precision.

## Acceptance

All checks pass.

## Recorded results

See [../results/rocket_dynamics.md](../results/rocket_dynamics.md).

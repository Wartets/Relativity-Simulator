# Mercury precession suite

Executable: `test_mercury_precession` - Source: `tests/test_mercury_precession.cpp` - Results: [mercury_precession.md](../results/mercury_precession.md)

## Purpose

Reproduces the perihelion advance benchmark of [DESCRIPTION.md](../../DESCRIPTION.md#12-verification-criteria--benchmark-protocols) with the implicit sixth-order integrator of [ARCHITECTURE.md](../../ARCHITECTURE.md#23-differential-solvers--geodesic-integrators).

## Scenario

Geometric units, M = 1, a = 1000 r_s, e = 0.20563. The angular momentum and energy follow from the turning points. The step is one twelve-hundredth of the Newtonian period, and 100 perihelion passages are detected by the sign change of the radial velocity with linear interpolation.

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Exact precession | Reference | Exact precession per revolution positive |
| Numerical integration | Convergence | Every implicit step converges |
| Numerical integration | Passages | Exactly 100 perihelion passages detected |
| Comparison | Total precession | Relative error below 1e-8 against the elliptic-integral result |

## Justification

The exact precession per revolution `4K(k)/sqrt(r_s (u3 - u1)) - 2 pi` follows from the Schwarzschild orbit equation with `K` computed by the arithmetic-geometric mean. The integrator is of order 6, so the accumulated error over 100 orbits stays below 1e-8.

## Measurements

Theoretical and numerical precession in radians.

## Acceptance

All checks pass.

## Recorded results

See [../results/mercury_precession.md](../results/mercury_precession.md).

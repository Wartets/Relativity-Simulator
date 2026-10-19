# Shapiro delay suite

Executable: `test_shapiro_delay` - Source: `tests/test_shapiro_delay.cpp` - Results: [shapiro_delay.md](../results/shapiro_delay.md)

## Purpose

Reproduces the Shapiro delay benchmark of [DESCRIPTION.md](../../DESCRIPTION.md#12-verification-criteria--benchmark-protocols) by comparing the geodesic integration with an independent quadrature.

## Scenario

Geometric units, M = 1, closest approach 1000, endpoints at r = 50000 and r = 75000, step 5 (0.5 inside 1.5 r0) with the sixth-order implicit scheme. The reference is a 500-panel four-point Gauss-Legendre quadrature of the regularized exact flight time.

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Geodesic integration | Convergence | Every implicit step converges |
| Geodesic integration | Endpoint | The final radius equals the probe radius within 1e-6 |
| Reference quadrature | Flight time | Relative difference below 1e-11 against the quadrature |

## Justification

The substitution `r = r0 + w^2` removes the square-root singularity at the turning point, so the quadrature converges at the Gauss rate; both computations are exact to rounding, hence the 1e-11 bound.

## Measurements

One-way and round-trip delays (theory and numerical), exact and integrated flight times.

## Acceptance

All checks pass.

## Recorded results

See [../results/shapiro_delay.md](../results/shapiro_delay.md).

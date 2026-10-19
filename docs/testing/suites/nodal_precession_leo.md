# Nodal precession suite

Executable: `test_nodal_precession_leo` - Source: `tests/test_nodal_precession_leo.cpp` - Results: [nodal_precession_leo.md](../results/nodal_precession_leo.md)

## Purpose

Verifies `OrbitalPrecessionAnalytic` and the zonal gravity of `SphericalHarmonicsGravityModel` ([ARCHITECTURE.md](../../ARCHITECTURE.md#24-post-newtonian-dynamics--gravimetry)).

## Scenario

Earth with GM = 3.986004418e14, R = 6378137 m, J2 = 1.08262668355e-3. Analytic checks at 800 km altitude with e = 0.001; numerical orbit at 700 km altitude, inclination 98.2 degrees, 40 orbits with a Runge-Kutta step of 2 s and a node regression of the ascending node.

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Analytical precession formulas | Sun-synchronous orbit | Drift equals one revolution per tropical year within 1e-12 rad/day |
| Analytical precession formulas | Equatorial rate | Equals `-3/2 n J2 (R/p)^2` within 1e-15 |
| Analytical precession formulas | Polar rate | Below 1e-15 |
| Analytical precession formulas | Critical inclination | Apsidal rate below 1e-14 |
| Analytical precession formulas | J4 | Positive and below 1e-6 |
| Numerical nodal drift | Node count | At least 30 |
| Numerical nodal drift | Rate | Regression slope within 2 percent of the analytic J2 rate |

## Justification

The secular theory of J2 gives the nodal rate; the osculating node oscillates with short-period terms, so a regression over many crossings is compared at 2 percent.

## Measurements

Theoretical and numerical nodal rates in rad/day.

## Acceptance

All checks pass.

## Recorded results

See [../results/nodal_precession_leo.md](../results/nodal_precession_leo.md).

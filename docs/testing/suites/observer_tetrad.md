# Observer tetrad suite

Executable: `test_observer_tetrad` - Source: `tests/test_observer_tetrad.cpp` - Results: [observer_tetrad.md](../results/observer_tetrad.md)

## Purpose

Verifies `ObserverTetrad` and `FermiWalkerTransport` of [DESCRIPTION.md](../../DESCRIPTION.md#8-observer-kinematics--coordinate-transport) against the classical precession effects.

## Scenario

Stationary and ZAMO tetrads in Schwarzschild (r = 6 M) and Kerr (a = 0.9 M); a gyroscope on a circle of radius 10 at v = 0.6 c; Gravity Probe B at r = 7020137 m for two orbits of 10000 steps; a static observer at r = 15 in Kerr with a = 0.95 for 2000 steps of 0.05.

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Tetrad orthonormality | Schwarzschild stationary and ZAMO, Kerr ZAMO | Orthonormal within 1e-12, 1e-12 and 1e-10 |
| Thomas precession | Angle after one orbit | `-2 pi (gamma - 1)` within a relative 1e-4 |
| Thomas precession | Tetrad | Orthonormality residual below 1e-10 |
| Gravity Probe B geodetic precession | Theory | Between 6.55 and 6.65 arcsec per year |
| Gravity Probe B geodetic precession | Transport | Within 1e-4 arcsec per year of the theory; residual below 1e-10 |
| Lense-Thirring frame dragging | Rate | Within a relative 1e-4 of the connection prediction; residual below 1e-9 |
| Fermi-Walker against parallel transport | Identity | Derivatives with zero acceleration identical within 1e-15 |

## Justification

The Thomas angle follows from the Lorentz boost composition, the geodetic rate is the Schwarzschild result `(3/2) (GM)^(3/2)/(c^2 r^(5/2))`, and with no acceleration Fermi-Walker transport reduces to parallel transport.

## Measurements

Thomas angle, annual precessions and gyroscope rates.

## Acceptance

All checks pass.

## Recorded results

See [../results/observer_tetrad.md](../results/observer_tetrad.md).

# symplectic_gauss_legendre

Verifies the implicit Gauss-Legendre integrators: exact straight-line propagation in flat spacetime with the fourth-order scheme, and conservation of the normalization, energy and angular momentum of a circular Schwarzschild orbit over 2000 steps with the sixth-order scheme.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 8 |
| Failed checks | 0 |
| Duration | 0.4163 s |
| Generated | 2026-10-19 22:21:19 UTC |

## Flat spacetime

| Result | Check | Detail |
| --- | --- | --- |
| PASS | all one hundred implicit steps converge |  |
| PASS | the x coordinate advances by u^x times the proper time | actual=2 expected=2 abs_error=2.132e-14 tolerance=1e-12 |
| PASS | the y coordinate advances by u^y times the proper time | actual=4 expected=4 abs_error=4.263e-14 tolerance=1e-12 |
| PASS | the z coordinate advances by u^z times the proper time | actual=6 expected=6 abs_error=3.464e-14 tolerance=1e-12 |

## Circular Schwarzschild orbit

| Result | Check | Detail |
| --- | --- | --- |
| PASS | all two thousand implicit steps converge |  |
| PASS | the four-velocity normalization drifts by less than 1e-11 | value=9.68114e-14 limit=1e-11 |
| PASS | the conserved energy drifts by less than 1e-11 | value=8.88178e-15 limit=1e-11 |
| PASS | the conserved angular momentum drifts by less than 1e-11 | value=5.18696e-13 limit=1e-11 |

## Measurements

| Section | Name | Value | Unit |
| --- | --- | --- | --- |
| Circular Schwarzschild orbit | normalization_drift | 9.68114477e-14 | dimensionless |
| Circular Schwarzschild orbit | energy_drift | 8.8817842e-15 | dimensionless |
| Circular Schwarzschild orbit | angular_momentum_drift | 5.18696197e-13 | dimensionless |


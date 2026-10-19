# rocket_dynamics

Verifies the relativistic rocket in flat spacetime: constant proper acceleration against the closed-form hyperbolic solution in proper-time flow, the coordinate-time flow mode, and an exact quarter-turn rotation that preserves the orthonormality of the tetrad.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 10 |
| Failed checks | 0 |
| Duration | 2.765 s |
| Generated | 2026-10-19 22:21:09 UTC |

## Proper-time flow

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the coordinate time equals sinh(a tau) / a | actual=1.81343020392 expected=1.81343020392 abs_error=3.262e-13 tolerance=1e-07 |
| PASS | the proper time matches the hyperbolic solution | actual=1 expected=1 abs_error=8.671e-14 tolerance=1e-09 |
| PASS | the velocity matches the hyperbolic solution | actual=0.964027580076 expected=0.964027580076 abs_error=1.554e-15 tolerance=1e-09 |
| PASS | the position matches the hyperbolic solution | actual=1.38109784554 expected=1.38109784554 abs_error=6.439e-15 tolerance=1e-07 |
| PASS | the Lorentz factor matches the hyperbolic solution | actual=3.76219569108 expected=3.76219569108 abs_error=8.216e-14 tolerance=1e-08 |

## Coordinate-time flow

| Result | Check | Detail |
| --- | --- | --- |
| PASS | two thousand steps of 0.001 advance the coordinate time by 2 | actual=2 expected=2 abs_error=1.095e-13 tolerance=0.0001 |
| PASS | the velocity matches the hyperbolic solution in coordinate-time flow | actual=0.948683298051 expected=0.948683298051 abs_error=1.91e-14 tolerance=1e-06 |

## Spatial rotation

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the rotated tetrad remains orthonormal |  |
| PASS | a quarter turn removes the x component of the first spatial leg | actual=6.12323399574e-17 expected=0 abs_error=6.123e-17 tolerance=1e-12 |
| PASS | a quarter turn aligns the first spatial leg with the z axis | actual=1 expected=1 abs_error=0 tolerance=1e-12 |


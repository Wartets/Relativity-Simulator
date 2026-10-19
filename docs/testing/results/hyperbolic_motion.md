# hyperbolic_motion

Verifies the relativistic rocket against the closed-form hyperbolic motion of constant proper acceleration in flat spacetime: proper time, velocity, position and Lorentz factor over a grid of accelerations and coordinate times.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 4 |
| Failed checks | 0 |
| Duration | 56.18 s |
| Generated | 2026-10-19 22:20:25 UTC |

## Constant proper acceleration sweep

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the proper time matches the hyperbolic solution within 1e-7 over the whole grid | value=4.05928e-08 limit=1e-07 |
| PASS | the velocity matches the hyperbolic solution within 1e-7 over the whole grid | value=1.4592e-11 limit=1e-07 |
| PASS | the position matches the hyperbolic solution within 1e-6 over the whole grid | value=1.61728e-09 limit=1e-06 |
| PASS | the Lorentz factor matches the hyperbolic solution within 1e-7 over the whole grid | value=9.89374e-08 limit=1e-07 |

## Measurements

| Section | Name | Value | Unit |
| --- | --- | --- | --- |
| Constant proper acceleration sweep | worst_relative_proper_time_error | 4.0592812e-08 | dimensionless |
| Constant proper acceleration sweep | worst_relative_velocity_error | 1.45920252e-11 | dimensionless |
| Constant proper acceleration sweep | worst_relative_position_error | 1.61727905e-09 | dimensionless |
| Constant proper acceleration sweep | worst_relative_gamma_error | 9.89374344e-08 | dimensionless |


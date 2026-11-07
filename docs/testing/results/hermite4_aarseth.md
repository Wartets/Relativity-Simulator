# hermite4_aarseth

Verifies the fourth-order Hermite predictor-corrector with Aarseth step selection on a circular Schwarzschild orbit: one thousand variable steps are recorded, the accumulated step is positive and the orbital radius stays constant.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 3 |
| Failed checks | 0 |
| Duration | 0.06484 s |
| Generated | 2026-11-06 12:54:39 UTC |

## Circular orbit

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the accumulated proper time is positive |  |
| PASS | exactly one thousand steps are recorded |  |
| PASS | the orbital radius stays at 10 within 1e-4 | actual=10 expected=10 abs_error=0 tolerance=0.0001 |

## Measurements

| Section | Name | Value | Unit |
| --- | --- | --- | --- |
| Circular orbit | accumulated_proper_time | 0.020999 | simulation time |


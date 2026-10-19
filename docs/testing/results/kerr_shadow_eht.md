# kerr_shadow_eht

Verifies the Bardeen shadow of the Kerr metric: the area of the Schwarzschild shadow against pi times 27 M^2, and the overlap ratio of the computed boundary with the analytic curve for spins from 0 to 0.9999 and inclinations of 17 and 90 degrees.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 6 |
| Failed checks | 0 |
| Duration | 0.1005 s |
| Generated | 2026-10-19 22:20:32 UTC |

## Schwarzschild shadow area

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the shadow area equals 27 pi M^2 within a relative 1e-5 | actual=84.8228621187 expected=84.8230016469 rel_error=1.645e-06 tolerance=1e-05 |

## Shadow overlap benchmark

| Result | Check | Detail |
| --- | --- | --- |
| PASS | a = 0, inclination 90 deg: the overlap with the analytic shadow exceeds 99.999 percent |  |
| PASS | a = 0.5, inclination 90 deg: the overlap with the analytic shadow exceeds 99.999 percent |  |
| PASS | a = 0.9, inclination 90 deg: the overlap with the analytic shadow exceeds 99.999 percent |  |
| PASS | a = 0.9999, inclination 90 deg: the overlap with the analytic shadow exceeds 99.999 percent |  |
| PASS | a = 0.9, inclination 17 deg: the overlap with the analytic shadow exceeds 99.999 percent |  |

## Measurements

| Section | Name | Value | Unit |
| --- | --- | --- | --- |
| Shadow overlap benchmark | overlap_ratio_a = 0, inclination 90 deg | 1 | dimensionless |
| Shadow overlap benchmark | overlap_ratio_a = 0.5, inclination 90 deg | 1 | dimensionless |
| Shadow overlap benchmark | overlap_ratio_a = 0.9, inclination 90 deg | 1 | dimensionless |
| Shadow overlap benchmark | overlap_ratio_a = 0.9999, inclination 90 deg | 1 | dimensionless |
| Shadow overlap benchmark | overlap_ratio_a = 0.9, inclination 17 deg | 1 | dimensionless |


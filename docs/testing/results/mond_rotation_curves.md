# mond_rotation_curves

Verifies the MOND framework: the Newtonian limit at high acceleration, the deep-MOND limit at low acceleration, the asymptotically flat rotation velocity of a point mass, the baryonic Tully-Fisher relation v^4 = G M a0, and the simple interpolation function.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 7 |
| Failed checks | 0 |
| Duration | 0.001017 s |
| Generated | 2026-10-19 22:20:50 UTC |

## Acceleration limits

| Result | Check | Detail |
| --- | --- | --- |
| PASS | at high acceleration the MOND acceleration equals the Newtonian value within 1 percent | actual=1.00000072e-07 expected=1e-07 rel_error=7.2e-07 tolerance=0.01 |
| PASS | at low acceleration the MOND acceleration follows sqrt(a_N a0) within 5 percent | actual=3.46482337813e-12 expected=3.46410161514e-12 rel_error=0.0002084 tolerance=0.05 |

## Rotation curve of a point mass

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the circular velocity at 30 kpc is within 5 percent of the flat value | actual=203014.958436 expected=199768.267515 rel_error=0.01625 tolerance=0.05 |
| PASS | the circular velocity at 50 kpc is within 3 percent of the flat value | actual=200931.754497 expected=199768.267515 rel_error=0.005824 tolerance=0.03 |
| PASS | the circular velocity at 100 kpc is within 2 percent of the flat value | actual=200058.530615 expected=199768.267515 rel_error=0.001453 tolerance=0.02 |

## Baryonic Tully-Fisher relation

| Result | Check | Detail |
| --- | --- | --- |
| PASS | v^4 equals G M a0 within 5 percent at 100 kpc | actual=1.60187380204e+21 expected=1.59259743852e+21 rel_error=0.005825 tolerance=0.05 |

## Simple interpolation function

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the simple interpolation reaches the flat velocity within 5 percent at 100 kpc | actual=205221.309719 expected=199768.267515 rel_error=0.0273 tolerance=0.05 |

## Measurements

| Section | Name | Value | Unit |
| --- | --- | --- | --- |
| Rotation curve of a point mass | asymptotic_flat_velocity | 199768.268 | m/s |


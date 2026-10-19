# nodal_precession_leo

Verifies the secular J2 perturbation of low Earth orbits: the sun-synchronous inclination, the equatorial and polar nodal rates, the vanishing apsidal drift at the critical inclination, the size of the J4 correction, and the nodal drift of a numerically integrated 98.2 degree orbit against the analytic rate.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 8 |
| Failed checks | 0 |
| Duration | 0.9657 s |
| Generated | 2026-10-19 22:20:51 UTC |

## Analytical precession formulas

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the sun-synchronous inclination yields a nodal drift of one revolution per tropical year | actual=0.0172027912086 expected=0.0172027912086 abs_error=0 tolerance=1e-12 |
| PASS | the equatorial nodal rate equals -3/2 n J2 (R/p)^2 | actual=-1.33102535733e-06 expected=-1.33102535733e-06 abs_error=0 tolerance=1e-15 |
| PASS | a polar orbit has no J2 nodal drift | value=8.15018e-23 limit=1e-15 |
| PASS | the apsidal drift vanishes at the critical inclination | value=2.95547e-22 limit=1e-14 |
| PASS | the J4 term changes the nodal rate |  |
| PASS | the J4 correction stays small against the J2 rate | value=5.38425e-10 limit=1e-06 |

## Numerical nodal drift

| Result | Check | Detail |
| --- | --- | --- |
| PASS | at least thirty ascending node crossings are detected | nodes=40 |
| PASS | the regression slope of the node longitude matches the analytic J2 rate within 2 percent | actual=2.0027060219e-07 expected=1.99396633287e-07 rel_error=0.004383 tolerance=0.02 |

## Measurements

| Section | Name | Value | Unit |
| --- | --- | --- | --- |
| Numerical nodal drift | theoretical_nodal_rate | 0.0172278691 | rad/day |
| Numerical nodal drift | numerical_nodal_rate | 0.01730338 | rad/day |


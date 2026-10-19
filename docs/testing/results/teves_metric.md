# teves_metric

Verifies the TeVeS spacetime metric: satisfaction of the spacetime concept, geometric invariants in geometrized units, positivity of the connection in SI units, and the circular velocity of the metric against the MOND prediction and the baryonic Tully-Fisher relation.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 9 |
| Failed checks | 0 |
| Duration | 0.001205 s |
| Generated | 2026-10-19 22:21:20 UTC |

## Geometrized units

| Result | Check | Detail |
| --- | --- | --- |
| PASS | TeVeS: the inverse metric times the metric reproduces the identity | value=2.22045e-16 limit=1e-12 |
| PASS | TeVeS: the metric tensor is symmetric relative to its largest component | value=0 limit=1e-14 |
| PASS | TeVeS: the metric determinant is negative, which gives a Lorentzian signature |  |
| PASS | TeVeS: the analytic connection is symmetric in its lower indices | value=0 limit=1e-14 |
| PASS | TeVeS: the analytic connection matches eighth-order finite differences relative to the largest symbol of its row | worst_relative=3.423e-14 at (2,3,3) tolerance=0.0001 |

## Galactic scale in SI units

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the SI inverse metric reproduces the identity | value=1.11022e-16 limit=1e-12 |
| PASS | the radial and temporal connection symbols are positive |  |
| PASS | the metric circular velocity at 50 kpc matches the MOND prediction | actual=210807.219996 expected=210806.815004 rel_error=1.921e-06 tolerance=0.0001 |
| PASS | the asymptotic velocity at 250 kpc follows the baryonic Tully-Fisher relation within 2 percent | actual=201933.438361 expected=199768.267515 rel_error=0.01084 tolerance=0.02 |


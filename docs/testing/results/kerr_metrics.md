# kerr_metrics

Verifies the Kerr metric in Boyer-Lindquist and Kerr-Schild forms: horizon and ergosphere radii, regime flags, frame dragging, metric inversion, symmetry, signature, the unit determinant of the Kerr-Schild form and the agreement of the analytic connection with eighth-order finite differences.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 18 |
| Failed checks | 0 |
| Duration | 0.001852 s |
| Generated | 2026-10-19 22:20:30 UTC |

## Boyer-Lindquist horizons and ergosphere

| Result | Check | Detail |
| --- | --- | --- |
| PASS | a / M = 0.8 is classified as sub-extremal |  |
| PASS | the outer horizon equals M + sqrt(M^2 - a^2) | actual=1.6 expected=1.6 abs_error=0 tolerance=1e-14 |
| PASS | the inner horizon equals M - sqrt(M^2 - a^2) | actual=0.4 expected=0.4 abs_error=0 tolerance=1e-14 |
| PASS | the ergosphere touches the outer horizon at the pole | actual=1.6 expected=1.6 abs_error=0 tolerance=1e-14 |
| PASS | the equatorial ergosphere radius equals 2 M | actual=2 expected=2 abs_error=0 tolerance=1e-14 |

## Boyer-Lindquist geometry

| Result | Check | Detail |
| --- | --- | --- |
| PASS | Kerr: the inverse metric times the metric reproduces the identity | value=5.55112e-17 limit=1e-13 |
| PASS | Kerr: the metric tensor is symmetric relative to its largest component | value=0 limit=1e-14 |
| PASS | Kerr: the metric determinant is negative, which gives a Lorentzian signature |  |
| PASS | Kerr: the analytic connection is symmetric in its lower indices | value=0 limit=1e-14 |
| PASS | Kerr: the analytic connection matches eighth-order finite differences relative to the largest symbol of its row | worst_relative=2.616e-13 at (0,0,2) tolerance=1e-08 |
| PASS | the off-diagonal time-azimuth component that encodes frame dragging is present and symmetric |  |
| PASS | the zero-angular-momentum observers co-rotate with the hole |  |

## Kerr-Schild geometry

| Result | Check | Detail |
| --- | --- | --- |
| PASS | Kerr-Schild: the inverse metric times the metric reproduces the identity | value=2.22045e-16 limit=1e-13 |
| PASS | Kerr-Schild: the metric tensor is symmetric relative to its largest component | value=0 limit=1e-14 |
| PASS | Kerr-Schild: the metric determinant is negative, which gives a Lorentzian signature |  |
| PASS | Kerr-Schild: the analytic connection is symmetric in its lower indices | value=0 limit=1e-14 |
| PASS | Kerr-Schild: the analytic connection matches eighth-order finite differences relative to the largest symbol of its row | worst_relative=3.727e-13 at (3,1,1) tolerance=1e-08 |
| PASS | the Kerr-Schild metric determinant equals -1 | actual=-1 expected=-1 abs_error=2.22e-16 tolerance=1e-12 |


# vacuum_cosmo_metrics

Verifies the electrovacuum and cosmological metrics: Reissner-Nordstrom and Kerr-Newman horizons and regimes, Kerr-Newman reduction to Kerr, geometric invariants of Reissner-Nordstrom, Kerr-Newman, Schwarzschild-de Sitter and Kerr-de Sitter, and the FLRW scale factor, redshift and metric inversion.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 35 |
| Failed checks | 0 |
| Duration | 0.004153 s |
| Generated | 2026-10-19 22:22:20 UTC |

## Reissner-Nordstrom horizons

| Result | Check | Detail |
| --- | --- | --- |
| PASS | Q / M = 0.6 is sub-extremal |  |
| PASS | the outer horizon equals M + sqrt(M^2 - Q^2) = 1.8 | actual=1.8 expected=1.8 abs_error=0 tolerance=1e-12 |
| PASS | the inner horizon equals M - sqrt(M^2 - Q^2) = 0.2 | actual=0.2 expected=0.2 abs_error=5.551e-17 tolerance=1e-12 |
| PASS | Q = M is extremal |  |
| PASS | the extremal outer horizon sits at M | actual=1 expected=1 abs_error=0 tolerance=1e-12 |
| PASS | the extremal inner horizon sits at M | actual=1 expected=1 abs_error=0 tolerance=1e-12 |
| PASS | Q > M is hyper-extremal |  |
| PASS | the uncharged photon sphere radius equals 3 M | actual=3 expected=3 abs_error=0 tolerance=1e-12 |

## Reissner-Nordstrom geometry

| Result | Check | Detail |
| --- | --- | --- |
| PASS | Reissner-Nordstrom: the inverse metric times the metric reproduces the identity | value=2.22045e-16 limit=1e-12 |
| PASS | Reissner-Nordstrom: the metric tensor is symmetric relative to its largest component | value=0 limit=1e-14 |
| PASS | Reissner-Nordstrom: the metric determinant is negative, which gives a Lorentzian signature |  |
| PASS | Reissner-Nordstrom: the analytic connection is symmetric in its lower indices | value=0 limit=1e-14 |
| PASS | Reissner-Nordstrom: the analytic connection matches eighth-order finite differences relative to the largest symbol of its row | worst_relative=6.46e-14 at (0,0,1) tolerance=1e-08 |

## Kerr-Newman

| Result | Check | Detail |
| --- | --- | --- |
| PASS | a^2 + Q^2 < M^2 is sub-extremal |  |
| PASS | the outer horizon equals M + sqrt(M^2 - a^2 - Q^2) | actual=1.69282032303 expected=1.69282032303 abs_error=0 tolerance=1e-12 |
| PASS | the uncharged Kerr-Newman metric inverts to the identity | value=5.55112e-17 limit=1e-12 |
| PASS | Kerr-Newman: the inverse metric times the metric reproduces the identity | value=2.22045e-16 limit=1e-12 |
| PASS | Kerr-Newman: the metric tensor is symmetric relative to its largest component | value=0 limit=1e-14 |
| PASS | Kerr-Newman: the metric determinant is negative, which gives a Lorentzian signature |  |
| PASS | Kerr-Newman: the analytic connection is symmetric in its lower indices | value=0 limit=1e-14 |
| PASS | Kerr-Newman: the analytic connection matches eighth-order finite differences relative to the largest symbol of its row | worst_relative=1.113e-13 at (3,2,3) tolerance=1e-07 |

## De Sitter extensions

| Result | Check | Detail |
| --- | --- | --- |
| PASS | Schwarzschild-de Sitter: the inverse metric times the metric reproduces the identity | value=0 limit=1e-12 |
| PASS | Schwarzschild-de Sitter: the metric tensor is symmetric relative to its largest component | value=0 limit=1e-14 |
| PASS | Schwarzschild-de Sitter: the metric determinant is negative, which gives a Lorentzian signature |  |
| PASS | Schwarzschild-de Sitter: the analytic connection is symmetric in its lower indices | value=0 limit=1e-14 |
| PASS | Schwarzschild-de Sitter: the analytic connection matches eighth-order finite differences relative to the largest symbol of its row | worst_relative=1.334e-13 at (3,2,3) tolerance=1e-08 |
| PASS | Kerr-de Sitter: the inverse metric times the metric reproduces the identity | value=3.33067e-16 limit=1e-11 |
| PASS | Kerr-de Sitter: the metric tensor is symmetric relative to its largest component | value=0 limit=1e-14 |
| PASS | Kerr-de Sitter: the metric determinant is negative, which gives a Lorentzian signature |  |
| PASS | Kerr-de Sitter: the analytic connection is symmetric in its lower indices | value=0 limit=1e-14 |
| PASS | Kerr-de Sitter: the analytic connection matches eighth-order finite differences relative to the largest symbol of its row | worst_relative=1.996e-13 at (0,0,2) tolerance=1e-07 |

## FLRW expansion

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the matter-dominated scale factor equals 1 at t0 | actual=1 expected=1 abs_error=0 tolerance=1e-12 |
| PASS | the matter-dominated scale factor grows as t^(2/3), giving 4 at t = 8 | actual=4 expected=4 abs_error=0 tolerance=1e-12 |
| PASS | the cosmological redshift between t = 1 and t = 8 equals 3 | actual=3 expected=3 abs_error=0 tolerance=1e-12 |
| PASS | the FLRW inverse metric reproduces the identity | value=0 limit=1e-12 |


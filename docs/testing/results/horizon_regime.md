# horizon_regime

Verifies the horizon regime analysis: constraint detection per metric, Kerr, Kerr-Newman and Nariai extremality classification, projection onto the sub-extremal domain, guaranteed parameter limits and zone layout geometry.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 30 |
| Failed checks | 0 |
| Duration | 0.001663 s |
| Generated | 2026-10-19 22:19:28 UTC |

## Constraint set parsing

| Result | Check | Detail |
| --- | --- | --- |
| PASS | Kerr constrains the spin only |  |
| PASS | Reissner-Nordstrom constrains the charge only |  |
| PASS | Kerr-Newman constrains spin and charge |  |
| PASS | Schwarzschild-de Sitter constrains the cosmological constant only |  |
| PASS | Schwarzschild has no horizon constraint |  |

## Kerr regime classification

| Result | Check | Detail |
| --- | --- | --- |
| PASS | a / M = 0.5 is sub-extremal |  |
| PASS | the outer horizon equals M + sqrt(M^2 - a^2) | actual=1.86602540378 expected=1.86602540378 abs_error=0 tolerance=1e-12 |
| PASS | a / M = 1 is extremal |  |
| PASS | the extremal horizon sits at M | actual=1 expected=1 abs_error=0 tolerance=1e-12 |
| PASS | a / M = 1.1 is super-extremal |  |
| PASS | a super-extremal state reports the radius at M | actual=1 expected=1 abs_error=0 tolerance=1e-12 |

## Kerr-Newman combined extremality

| Result | Check | Detail |
| --- | --- | --- |
| PASS | a^2 + Q^2 = M^2 is extremal |  |
| PASS | a^2 + Q^2 > M^2 is super-extremal |  |
| PASS | projecting a super-extremal state reports a change |  |
| PASS | the projected state is sub-extremal |  |
| PASS | the projected extremality ratio respects the guarantee factor | value=0.998 limit=0.998 |

## Nariai bound

| Result | Check | Detail |
| --- | --- | --- |
| PASS | Lambda M^2 = 1/18 is sub-extremal |  |
| PASS | Lambda M^2 = 1/9 is the Nariai limit |  |
| PASS | Lambda M^2 = 1/6 exceeds the Nariai limit |  |
| PASS | projecting a super-extremal cosmological state reports a change |  |
| PASS | the projected cosmological state is sub-extremal |  |
| PASS | the projected constant respects the guaranteed maximum | value=0.110889 limit=0.110889 |

## Guaranteed parameter limits

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the guaranteed spin limit lies strictly between zero and the mass |  |
| PASS | constraining an excessive spin returns the guaranteed limit | actual=1.997998999 expected=1.997998999 abs_error=0 tolerance=1e-12 |
| PASS | a positive charge limit remains for a moderate spin |  |
| PASS | the combined spin and charge limit respects the guarantee factor | value=3.992 limit=3.992 |

## Zone layout geometry

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the spin axis has a symmetric layout |  |
| PASS | the spin threshold equals the mass | actual=3 expected=3 abs_error=0 tolerance=1e-12 |
| PASS | the mass axis is asymmetric with the sub-extremal zone above the threshold |  |
| PASS | the mass threshold equals the spin | actual=1.5 expected=1.5 abs_error=0 tolerance=1e-12 |


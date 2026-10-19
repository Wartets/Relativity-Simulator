# invariant_preservation

Verifies that the adaptive integrator preserves the four-velocity normalization: a circular Schwarzschild orbit over fifty thousand steps keeps the invariant residual below 1e-11, and a radial free fall through the horizon in Painleve-Gullstrand coordinates stays below 1e-10.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 5 |
| Failed checks | 0 |
| Duration | 3.323 s |
| Generated | 2026-10-19 22:20:29 UTC |

## Circular timelike orbit

| Result | Check | Detail |
| --- | --- | --- |
| PASS | every adaptive step of the circular orbit succeeds |  |
| PASS | the normalization residual of the circular orbit stays below 1e-11 | value=5.10703e-15 limit=1e-11 |

## Horizon infall in Painleve-Gullstrand coordinates

| Result | Check | Detail |
| --- | --- | --- |
| PASS | every adaptive step of the infall succeeds |  |
| PASS | the infalling particle crosses the horizon |  |
| PASS | the normalization residual of the infall stays below 1e-10 | value=9.32587e-14 limit=1e-10 |

## Measurements

| Section | Name | Value | Unit |
| --- | --- | --- | --- |
| Circular timelike orbit | orbit_invariant_residual | 5.10702591e-15 | dimensionless |
| Horizon infall in Painleve-Gullstrand coordinates | infall_invariant_residual | 9.32587341e-14 | dimensionless |


# exotic_metrics

Verifies the traversable wormhole and the warp bubble: the Morris-Thorne metric at the throat, a radial photon crossing the throat with the adaptive integrator, the shaping function limits of the Alcubierre bubble and the geometric invariants of both metrics.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 21 |
| Failed checks | 0 |
| Duration | 0.01018 s |
| Generated | 2026-10-19 22:19:23 UTC |

## Morris-Thorne throat

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the time component equals -1 for a vanishing redshift potential | actual=-1 expected=-1 abs_error=0 tolerance=1e-12 |
| PASS | the proper radial component equals 1 | actual=1 expected=1 abs_error=0 tolerance=1e-12 |
| PASS | the polar component equals the squared throat radius | actual=4 expected=4 abs_error=0 tolerance=1e-12 |
| PASS | the azimuthal component equals the squared throat radius at the equator | actual=4 expected=4 abs_error=0 tolerance=1e-12 |

## Throat transit

| Result | Check | Detail |
| --- | --- | --- |
| PASS | every adaptive step of the throat transit succeeds |  |
| PASS | the photon passes from the negative to the positive side of the wormhole |  |
| PASS | the photon passes within 0.2 of the throat | value=0.05 limit=0.2 |
| PASS | the photon emerges on the far side up to l = 10 |  |

## Morris-Thorne geometry

| Result | Check | Detail |
| --- | --- | --- |
| PASS | Morris-Thorne: the inverse metric times the metric reproduces the identity | value=2.22045e-16 limit=1e-12 |
| PASS | Morris-Thorne: the metric tensor is symmetric relative to its largest component | value=0 limit=1e-14 |
| PASS | Morris-Thorne: the metric determinant is negative, which gives a Lorentzian signature |  |
| PASS | Morris-Thorne: the analytic connection is symmetric in its lower indices | value=0 limit=1e-14 |
| PASS | Morris-Thorne: the analytic connection matches eighth-order finite differences relative to the largest symbol of its row | worst_relative=1.304e-13 at (1,3,3) tolerance=1e-08 |

## Alcubierre bubble

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the shaping function equals 1 at the bubble centre | actual=1 expected=1 abs_error=0 tolerance=1e-06 |
| PASS | the shaping function vanishes far from the bubble | value=0 limit=1e-06 |
| PASS | the inverse metric reproduces the identity at the centre | value=0 limit=1e-12 |
| PASS | Alcubierre: the inverse metric times the metric reproduces the identity | value=0 limit=1e-12 |
| PASS | Alcubierre: the metric tensor is symmetric relative to its largest component | value=0 limit=1e-14 |
| PASS | Alcubierre: the metric determinant is negative, which gives a Lorentzian signature |  |
| PASS | Alcubierre: the analytic connection is symmetric in its lower indices | value=0 limit=1e-14 |
| PASS | Alcubierre: the analytic connection matches eighth-order finite differences relative to the largest symbol of its row | worst_relative=3.055e-13 at (0,1,1) tolerance=1e-06 |


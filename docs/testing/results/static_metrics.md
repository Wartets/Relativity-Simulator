# static_metrics

Verifies the static and exotic spacetime metrics: metric inversion, symmetry, Lorentzian signature, analytic Christoffel symbols against eighth-order finite differences in geometric and SI units, vacuum curvature invariants, and horizon structure.

| Property | Value |
| --- | --- |
| Status | FAILED |
| Checks | 89 |
| Failed checks | 2 |
| Duration | 0.01282 s |
| Generated | 2026-10-19 12:46:35 UTC |

## Static spherical metrics in geometric units

| Result | Check | Detail |
| --- | --- | --- |
| PASS | Flat Minkowski [geometric] inverse metric times metric equals identity | value=0 limit=1e-12 |
| PASS | Flat Minkowski [geometric] metric tensor is symmetric | value=0 limit=1e-14 |
| PASS | Flat Minkowski [geometric] metric has Lorentzian signature (negative determinant) |  |
| PASS | Flat Minkowski [geometric] analytic Christoffel symbols match eighth-order finite differences | value=0 limit=1e-07 |
| PASS | Flat Minkowski [geometric] Christoffel symbols are symmetric in the lower indices | value=0 limit=1e-14 |
| PASS | Schwarzschild [geometric] inverse metric times metric equals identity | value=2.22045e-16 limit=1e-12 |
| PASS | Schwarzschild [geometric] metric tensor is symmetric | value=0 limit=1e-14 |
| PASS | Schwarzschild [geometric] metric has Lorentzian signature (negative determinant) |  |
| PASS | Schwarzschild [geometric] analytic Christoffel symbols match eighth-order finite differences | value=1.16443e-13 limit=1e-07 |
| PASS | Schwarzschild [geometric] Christoffel symbols are symmetric in the lower indices | value=0 limit=1e-14 |
| PASS | Schwarzschild isotropic [geometric] inverse metric times metric equals identity | value=0 limit=1e-12 |
| PASS | Schwarzschild isotropic [geometric] metric tensor is symmetric | value=0 limit=1e-14 |
| PASS | Schwarzschild isotropic [geometric] metric has Lorentzian signature (negative determinant) |  |
| PASS | Schwarzschild isotropic [geometric] analytic Christoffel symbols match eighth-order finite differences | value=1.47356e-13 limit=1e-07 |
| PASS | Schwarzschild isotropic [geometric] Christoffel symbols are symmetric in the lower indices | value=0 limit=1e-14 |
| PASS | Painleve-Gullstrand [geometric] inverse metric times metric equals identity | value=2.22045e-16 limit=1e-12 |
| PASS | Painleve-Gullstrand [geometric] metric tensor is symmetric | value=0 limit=1e-14 |
| PASS | Painleve-Gullstrand [geometric] metric has Lorentzian signature (negative determinant) |  |
| PASS | Painleve-Gullstrand [geometric] analytic Christoffel symbols match eighth-order finite differences | value=1.16563e-13 limit=1e-07 |
| PASS | Painleve-Gullstrand [geometric] Christoffel symbols are symmetric in the lower indices | value=0 limit=1e-14 |
| PASS | Eddington-Finkelstein [geometric] inverse metric times metric equals identity | value=2.22045e-16 limit=1e-12 |
| PASS | Eddington-Finkelstein [geometric] metric tensor is symmetric | value=0 limit=1e-14 |
| PASS | Eddington-Finkelstein [geometric] metric has Lorentzian signature (negative determinant) |  |
| PASS | Eddington-Finkelstein [geometric] analytic Christoffel symbols match eighth-order finite differences | value=1.16443e-13 limit=1e-07 |
| PASS | Eddington-Finkelstein [geometric] Christoffel symbols are symmetric in the lower indices | value=0 limit=1e-14 |
| PASS | Painleve-Gullstrand metric stays regular at the horizon [geometric] |  |
| PASS | Eddington-Finkelstein metric stays regular at the horizon [geometric] |  |

## Static spherical metrics in SI units

| Result | Check | Detail |
| --- | --- | --- |
| PASS | Flat Minkowski [SI] inverse metric times metric equals identity | value=0 limit=1e-09 |
| PASS | Flat Minkowski [SI] metric tensor is symmetric | value=0 limit=1e-14 |
| PASS | Flat Minkowski [SI] metric has Lorentzian signature (negative determinant) |  |
| PASS | Flat Minkowski [SI] analytic Christoffel symbols match eighth-order finite differences | value=0 limit=1e-06 |
| PASS | Flat Minkowski [SI] Christoffel symbols are symmetric in the lower indices | value=0 limit=1e-14 |
| PASS | Schwarzschild [SI] inverse metric times metric equals identity | value=0 limit=1e-09 |
| PASS | Schwarzschild [SI] metric tensor is symmetric | value=0 limit=1e-14 |
| PASS | Schwarzschild [SI] metric has Lorentzian signature (negative determinant) |  |
| PASS | Schwarzschild [SI] analytic Christoffel symbols match eighth-order finite differences | value=8.38412e-14 limit=1e-06 |
| PASS | Schwarzschild [SI] Christoffel symbols are symmetric in the lower indices | value=0 limit=1e-14 |
| PASS | Schwarzschild isotropic [SI] inverse metric times metric equals identity | value=2.22045e-16 limit=1e-09 |
| PASS | Schwarzschild isotropic [SI] metric tensor is symmetric | value=0 limit=1e-14 |
| PASS | Schwarzschild isotropic [SI] metric has Lorentzian signature (negative determinant) |  |
| PASS | Schwarzschild isotropic [SI] analytic Christoffel symbols match eighth-order finite differences | value=3.45103e-13 limit=1e-06 |
| PASS | Schwarzschild isotropic [SI] Christoffel symbols are symmetric in the lower indices | value=0 limit=1e-14 |
| FAIL | Painleve-Gullstrand [SI] inverse metric times metric equals identity | value=1.49012e-08 limit=1e-09 |
| PASS | Painleve-Gullstrand [SI] metric tensor is symmetric | value=0 limit=1e-14 |
| PASS | Painleve-Gullstrand [SI] metric has Lorentzian signature (negative determinant) |  |
| PASS | Painleve-Gullstrand [SI] analytic Christoffel symbols match eighth-order finite differences | value=8.38412e-14 limit=1e-06 |
| PASS | Painleve-Gullstrand [SI] Christoffel symbols are symmetric in the lower indices | value=0 limit=1e-14 |
| FAIL | Eddington-Finkelstein [SI] inverse metric times metric equals identity | value=2.98023e-08 limit=1e-09 |
| PASS | Eddington-Finkelstein [SI] metric tensor is symmetric | value=0 limit=1e-14 |
| PASS | Eddington-Finkelstein [SI] metric has Lorentzian signature (negative determinant) |  |
| PASS | Eddington-Finkelstein [SI] analytic Christoffel symbols match eighth-order finite differences | value=8.38412e-14 limit=1e-06 |
| PASS | Eddington-Finkelstein [SI] Christoffel symbols are symmetric in the lower indices | value=0 limit=1e-14 |
| PASS | Painleve-Gullstrand metric stays regular at the horizon [SI] |  |
| PASS | Eddington-Finkelstein metric stays regular at the horizon [SI] |  |

## Vacuum curvature invariants in geometric units

| Result | Check | Detail |
| --- | --- | --- |
| PASS | Schwarzschild Ricci tensor vanishes in vacuum | value=1.89212e-11 limit=1e-06 |
| PASS | Schwarzschild Kretschmann invariant equals 48 M^2 / r^6 | actual=4.8e-05 expected=4.8e-05 rel_error=2.843e-13 tolerance=0.0001 |
| PASS | Schwarzschild Ricci scalar vanishes in vacuum | value=3.46928e-13 limit=1e-06 |
| PASS | Painleve-Gullstrand Ricci tensor vanishes in vacuum | value=1.89212e-11 limit=1e-06 |
| PASS | Painleve-Gullstrand Kretschmann invariant equals 48 M^2 / r^6 | actual=4.8e-05 expected=4.8e-05 rel_error=2.855e-13 tolerance=0.0001 |
| PASS | Painleve-Gullstrand Ricci scalar vanishes in vacuum | value=3.48165e-13 limit=1e-06 |
| PASS | Eddington-Finkelstein Ricci tensor vanishes in vacuum | value=1.89212e-11 limit=1e-06 |
| PASS | Eddington-Finkelstein Kretschmann invariant equals 48 M^2 / r^6 | actual=4.8e-05 expected=4.8e-05 rel_error=2.826e-13 tolerance=0.0001 |
| PASS | Eddington-Finkelstein Ricci scalar vanishes in vacuum | value=3.45923e-13 limit=1e-06 |

## Reissner-Nordstrom

| Result | Check | Detail |
| --- | --- | --- |
| PASS | Reissner-Nordstrom [geometric] inverse metric times metric equals identity | value=0 limit=1e-12 |
| PASS | Reissner-Nordstrom [geometric] metric tensor is symmetric | value=0 limit=1e-14 |
| PASS | Reissner-Nordstrom [geometric] metric has Lorentzian signature (negative determinant) |  |
| PASS | Reissner-Nordstrom [geometric] analytic Christoffel symbols match eighth-order finite differences | value=3.0087e-14 limit=1e-07 |
| PASS | Reissner-Nordstrom [geometric] Christoffel symbols are symmetric in the lower indices | value=0 limit=1e-14 |
| PASS | outer horizon radius equals M + sqrt(M^2 - Q^2) | actual=1.8 expected=1.8 abs_error=0 tolerance=1e-12 |
| PASS | inner horizon radius equals M - sqrt(M^2 - Q^2) | actual=0.2 expected=0.2 abs_error=5.551e-17 tolerance=1e-12 |
| PASS | Q/M = 0.6 is classified as subextremal |  |
| PASS | Q/M = 1 is classified as extremal |  |
| PASS | Q/M = 1.2 is classified as hyperextremal |  |
| PASS | uncharged outer photon sphere sits at 3 M | actual=3 expected=3 abs_error=0 tolerance=1e-12 |
| PASS | uncharged outer horizon sits at 2 M | actual=2 expected=2 abs_error=0 tolerance=1e-12 |

## Morris-Thorne wormhole and Alcubierre bubble

| Result | Check | Detail |
| --- | --- | --- |
| PASS | Morris-Thorne [geometric] inverse metric times metric equals identity | value=0 limit=1e-12 |
| PASS | Morris-Thorne [geometric] metric tensor is symmetric | value=0 limit=1e-14 |
| PASS | Morris-Thorne [geometric] metric has Lorentzian signature (negative determinant) |  |
| PASS | Morris-Thorne [geometric] analytic Christoffel symbols match eighth-order finite differences | value=4.96124e-14 limit=1e-07 |
| PASS | Morris-Thorne [geometric] Christoffel symbols are symmetric in the lower indices | value=0 limit=1e-14 |
| PASS | areal radius at the throat equals the throat radius | actual=1 expected=1 abs_error=0 tolerance=1e-14 |
| PASS | areal radius follows sqrt(l^2 + b0^2) | actual=3.16227766017 expected=3.16227766017 abs_error=0 tolerance=1e-14 |
| PASS | Alcubierre [geometric] inverse metric times metric equals identity | value=0 limit=1e-12 |
| PASS | Alcubierre [geometric] metric tensor is symmetric | value=0 limit=1e-14 |
| PASS | Alcubierre [geometric] metric has Lorentzian signature (negative determinant) |  |
| PASS | Alcubierre [geometric] analytic Christoffel symbols match eighth-order finite differences | value=9.20432e-12 limit=1e-07 |
| PASS | Alcubierre [geometric] Christoffel symbols are symmetric in the lower indices | value=0 limit=1e-14 |
| PASS | shaping function is unity at the bubble centre | actual=1 expected=1 abs_error=0 tolerance=1e-09 |
| PASS | shaping function vanishes far from the bubble | actual=0 expected=0 abs_error=0 tolerance=1e-09 |


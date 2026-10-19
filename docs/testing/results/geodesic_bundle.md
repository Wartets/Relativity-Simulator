# geodesic_bundle

Verifies the structure-of-arrays ray bundles and their four-vector algebra: the lane-wise Minkowski product, the Lorentz factor, the relativistic velocity addition, and the evolution of an eight-lane Schwarzschild null bundle, without any dynamic allocation.

| Property | Value |
| --- | --- |
| Status | FAILED |
| Checks | 6 |
| Failed checks | 1 |
| Duration | 0.05103 s |
| Generated | 2026-10-19 22:19:26 UTC |

## Four-vector algebra

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the lane-wise Minkowski product follows -u0 v0 + u.v | value=0 limit=1e-15 |
| PASS | the Lorentz factor equals 1 / sqrt(1 - beta^2) in every lane | value=2.22045e-16 limit=1e-14 |
| PASS | the relativistic addition of 0.5 c and -0.5 c along x yields 0.8 c in the first lane and zero in the second | value=1.11022e-16 limit=1e-14 |

## Ray bundle

| Result | Check | Detail |
| --- | --- | --- |
| PASS | a new eight-lane bundle has all lanes active |  |
| PASS | every lane keeps a positive radius after the Schwarzschild evolution |  |

## Allocation discipline

| Result | Check | Detail |
| --- | --- | --- |
| FAIL | no dynamic allocation occurs in the bundle algebra | allocations=1 |


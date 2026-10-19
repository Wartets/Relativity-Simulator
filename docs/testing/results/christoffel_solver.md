# christoffel_solver

Verifies the finite-difference connection solver at fourth, sixth and eighth order against the analytic connection of Minkowski and Schwarzschild spacetimes: exactness on flat space, lower-index symmetry, error ordering across stencil orders, dispatch to the analytic connection and the numerical wrapper, without dynamic allocation.

| Property | Value |
| --- | --- |
| Status | FAILED |
| Checks | 13 |
| Failed checks | 1 |
| Duration | 0.003012 s |
| Generated | 2026-10-19 22:16:36 UTC |

## Flat spacetime

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the fourth-order connection of Minkowski vanishes | value=0 limit=1e-15 |
| PASS | the sixth-order connection of Minkowski vanishes | value=0 limit=1e-15 |
| PASS | the eighth-order connection of Minkowski vanishes | value=0 limit=1e-15 |
| PASS | the dispatching solver returns the vanishing analytic connection of Minkowski | value=0 limit=1e-15 |

## Schwarzschild spacetime

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the numerical connection is symmetric in its lower indices for every stencil order | value=0 limit=1e-15 |
| PASS | the fourth-order connection agrees with the analytic one within a relative 1e-6 | value=1.61436e-11 limit=1e-06 |
| PASS | the sixth-order connection agrees with the analytic one within a relative 1e-9 | value=1.46263e-12 limit=1e-09 |
| PASS | the eighth-order connection agrees with the analytic one within a relative 1e-12 | value=7.70376e-13 limit=1e-12 |
| PASS | the eighth-order connection agrees with the analytic one within an absolute 1e-12 | value=7.95808e-13 limit=1e-12 |
| PASS | the sixth-order relative error never exceeds the fourth-order relative error |  |
| PASS | the dispatching solver returns exactly the analytic connection when the metric provides one | value=0 limit=0 |
| PASS | the numerical metric wrapper reproduces the analytic connection within a relative 1e-12 | value=7.70376e-13 limit=1e-12 |

## Allocation discipline

| Result | Check | Detail |
| --- | --- | --- |
| FAIL | no dynamic allocation occurs in the connection solver | allocations=2 |


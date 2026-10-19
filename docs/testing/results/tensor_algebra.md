# tensor_algebra

Verifies the static tensor algebra: closed-form 4x4 metric inversion for Minkowski, Schwarzschild, Kerr and a dense indefinite metric, the outer product, index contractions of rank-two and rank-three tensors, and symmetrization, all without a single dynamic allocation.

| Property | Value |
| --- | --- |
| Status | FAILED |
| Checks | 13 |
| Failed checks | 1 |
| Duration | 0.001606 s |
| Generated | 2026-10-19 22:21:19 UTC |

## Metric inversion

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the Minkowski inverse metric times the metric equals the identity | value=0 limit=1e-15 |
| PASS | the Schwarzschild inverse metric times the metric equals the identity | value=0 limit=1e-15 |
| PASS | the Kerr inverse metric times the metric equals the identity | value=1.11022e-16 limit=1e-15 |
| PASS | the dense indefinite inverse metric times the metric equals the identity | value=4.44089e-16 limit=1e-15 |

## Products and contractions

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the outer product of two vectors has rank 2 |  |
| PASS | every outer product component equals the product of the vector components | value=0 limit=1e-16 |
| PASS | contracting both indices of a rank-two tensor yields a scalar |  |
| PASS | the full contraction of an outer product equals the dot product | value=0 limit=1e-16 |
| PASS | contracting two indices of a rank-three tensor yields a vector |  |
| PASS | the partial trace of the connection-like tensor matches the hand computed vector | value=0 limit=1e-16 |

## Symmetrization

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the symmetric part equals the mean of the matrix and its transpose | value=0 limit=1e-16 |
| PASS | the antisymmetric part equals half the difference of the matrix and its transpose | value=0 limit=1e-16 |

## Allocation discipline

| Result | Check | Detail |
| --- | --- | --- |
| FAIL | no dynamic allocation occurs during tensor algebra | allocations=1 |


# uncertain_quantity

Verifies the uncertain quantity wrapper: nominal value and uncertainty propagation of interval sums, enclosure of interval products, and access to the nominal vector of a zonotope-backed quantity.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 5 |
| Failed checks | 0 |
| Duration | 0.0008831 s |
| Generated | 2026-10-19 22:21:21 UTC |

## Interval-backed quantities

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the nominal value of the sum equals the sum of the nominal values | actual=15 expected=15 abs_error=0 tolerance=1e-12 |
| PASS | the uncertainty of the sum equals the sum of the uncertainties | actual=0.7 expected=0.7 abs_error=6.661e-16 tolerance=1e-12 |
| PASS | the nominal value of the product lies in the enclosure around 50 | nominal=50.1 |

## Zonotope-backed quantities

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the first component of the zonotope centre is preserved | actual=1 expected=1 abs_error=0 tolerance=1e-15 |
| PASS | the second component of the zonotope centre is preserved | actual=2 expected=2 abs_error=0 tolerance=1e-15 |


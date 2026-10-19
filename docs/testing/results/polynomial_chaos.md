# polynomial_chaos

Verifies the one-dimensional polynomial chaos expansion: exact mean, variance and standard deviation of a linear function of a Gaussian variable on the Hermite basis, the mean of a quadratic function of a uniform variable on the Legendre basis, and the median extracted from sampled quantiles.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 5 |
| Failed checks | 0 |
| Duration | 0.0114 s |
| Generated | 2026-10-19 22:21:00 UTC |

## Hermite basis with a Gaussian variable

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the mean of 3 + 2 xi equals 3 | actual=3 expected=3 abs_error=1.332e-15 tolerance=1e-10 |
| PASS | the variance of 3 + 2 xi equals 4 | actual=4 expected=4 abs_error=1.776e-15 tolerance=1e-10 |
| PASS | the standard deviation of 3 + 2 xi equals 2 | actual=2 expected=2 abs_error=4.441e-16 tolerance=1e-10 |

## Legendre basis with a uniform variable

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the mean of 1 + 3 xi^2 over a uniform variable on [-1, 1] equals 2 | actual=2 expected=2 abs_error=8.882e-16 tolerance=1e-10 |

## Quantiles

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the median of the sampled Gaussian expansion is close to its mean | actual=3 expected=3 abs_error=1.776e-15 tolerance=0.05 |


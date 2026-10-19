# uncertainty_pce

Verifies the polynomial chaos propagation of a geodesic with an uncertain angular momentum against a ten-million-sample Monte-Carlo ensemble: the mean and variance of the final azimuth agree, and the order-four expansion is at least 400 times faster than the ensemble.

| Property | Value |
| --- | --- |
| Status | FAILED |
| Checks | 7 |
| Failed checks | 2 |
| Duration | 57.09 s |
| Generated | 2026-10-19 22:22:19 UTC |

## Polynomial chaos propagation

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the expansion mean of the final azimuth is finite | value=0 |
| PASS | the expansion variance of the final azimuth is finite | value=0 |

## Monte-Carlo ensemble

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the ensemble mean of the final azimuth is finite | value=0 |
| FAIL | the ensemble variance of the final azimuth is positive |  |

## Statistical agreement

| Result | Check | Detail |
| --- | --- | --- |
| FAIL | the means agree within one percent of the standard deviation | value=nan limit=0.01 |
| PASS | the variances agree within a relative 2 percent | actual=0 expected=0 rel_error=0 tolerance=0.02 |

## Cost

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the order-four expansion is at least 400 times faster than the ten-million-sample ensemble | ratio=12137.5 |

## Measurements

| Section | Name | Value | Unit |
| --- | --- | --- | --- |
| Statistical agreement | pce_mean | 0 | rad |
| Statistical agreement | mc_mean | 0 | rad |
| Statistical agreement | pce_variance | 0 | rad^2 |
| Statistical agreement | mc_variance | 0 | rad^2 |
| Statistical agreement | pce_skewness | 0 | dimensionless |
| Statistical agreement | mc_skewness | 0 | dimensionless |
| Cost | pce_time | 4.603 | ms |
| Cost | monte_carlo_time | 55868.7059 | ms |
| Cost | speedup_ratio | 12137.4551 | dimensionless |


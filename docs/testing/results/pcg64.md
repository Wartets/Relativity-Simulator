# pcg64

Verifies the explicit-state PCG64 generator that underpins deterministic replay: reproducibility, stream independence, output ranges, Gaussian moments, jump-ahead and registry derived sub-engines.

| Property | Value |
| --- | --- |
| Status | FAILED |
| Checks | 9 |
| Failed checks | 1 |
| Duration | 0.01643 s |
| Generated | 2026-10-19 13:43:56 UTC |

## Reproducibility

| Result | Check | Detail |
| --- | --- | --- |
| PASS | identical seed and stream produce identical sequences | mismatches=0 |
| PASS | different streams do not produce coinciding values | matches=0 |

## Output ranges

| Result | Check | Detail |
| --- | --- | --- |
| PASS | uniform doubles stay in [0, 1) |  |
| PASS | ranged uniform doubles stay in [-5, 15) |  |

## Gaussian statistics

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the sample mean of the standard normal distribution is close to zero | actual=0.00163582979463 expected=0 abs_error=0.001636 tolerance=0.05 |
| PASS | the sample variance of the standard normal distribution is close to one | actual=1.00372339735 expected=1 abs_error=0.003723 tolerance=0.05 |

## Jump-ahead and sub-engines

| Result | Check | Detail |
| --- | --- | --- |
| PASS | step_forward equals the same number of sequential draws |  |
| PASS | distinct sub-engine identifiers give distinct streams |  |
| FAIL | the same registry seed and identifier reproduce the same sub-engine |  |

## Measurements

| Section | Name | Value | Unit |
| --- | --- | --- | --- |
| Gaussian statistics | sample_mean | 0.00163582979 | dimensionless |
| Gaussian statistics | sample_variance | 1.0037234 | dimensionless |


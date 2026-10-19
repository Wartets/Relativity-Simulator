# Polynomial chaos suite

Executable: `test_polynomial_chaos` - Source: `tests/test_polynomial_chaos.cpp` - Results: [polynomial_chaos.md](../results/polynomial_chaos.md)

## Purpose

Verifies the spectral projections of [MATHEMATICAL_FORMULATION.md](../../MATHEMATICAL_FORMULATION.md#74-generalized-polynomial-chaos-expansion-gpce) on the Hermite and Legendre bases.

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Hermite basis | Mean of 3 + 2 xi | 3 within 1e-10 |
| Hermite basis | Variance | 4 within 1e-10 |
| Hermite basis | Standard deviation | 2 within 1e-10 |
| Legendre basis | Mean of 1 + 3 xi^2 on [-1, 1] | 2 within 1e-10 |
| Quantiles | Median from 10000 samples | 3 within 0.05 |

## Justification

A linear function of a standard Gaussian is represented exactly by two Hermite terms, and the quadrature is exact for polynomials of low degree. The median tolerance reflects the sampling resolution of the quantile estimator.

## Acceptance

All checks pass.

## Recorded results

See [../results/polynomial_chaos.md](../results/polynomial_chaos.md).

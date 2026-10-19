# Polynomial chaos propagation suite

Executable: `test_uncertainty_pce` - Source: `tests/test_uncertainty_pce.cpp` - Results: [uncertainty_pce.md](../results/uncertainty_pce.md)

## Purpose

Compares `PceGeodesicPropagator` ([MATHEMATICAL_FORMULATION.md](../../MATHEMATICAL_FORMULATION.md#74-generalized-polynomial-chaos-expansion-gpce)) with `MonteCarloGeodesicSampler`.

## Scenario

Schwarzschild M = 1, a geodesic from r = 10 on the equator with a Gaussian angular momentum 0.3 with standard deviation 0.01, integrated over an affine parameter of -10. The expansion has order 4 (six quadrature points); the ensemble has 10^7 samples.

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Polynomial chaos propagation | Moments | Finite mean and variance |
| Monte-Carlo ensemble | Moments | Finite mean and positive variance |
| Statistical agreement | Mean | Difference below 1 percent of the ensemble standard deviation |
| Statistical agreement | Variance | Relative difference below 2 percent |
| Cost | Speed-up | The ensemble time divided by the expansion time exceeds 400 |

## Justification

The standard error of the ensemble mean is 3e-4 of the standard deviation at 10^7 samples, so a 1 percent bound detects any real bias of the expansion. The 400 times ratio is the documented performance objective of the expansion and is the only timing ceiling of this suite.

## Measurements

Means, variances, skewnesses, both execution times and the speed-up ratio.

## Limits

The timing check depends on the hardware; it only fails if the objective stated in [DESCRIPTION.md](../../DESCRIPTION.md#72-uncertainty-propagation-frameworks) is no longer met.

## Acceptance

All checks pass.

## Recorded results

See [../results/uncertainty_pce.md](../results/uncertainty_pce.md).

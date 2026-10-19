# covariance_propagation

Verifies the covariance machinery and the variational geodesic integrator: covariance algebra and eigensystem, block extraction and Gaussian sampling, the flat-space Jacobian, the exact analytic covariance of a free particle, million-particle Monte-Carlo validation in Minkowski, Schwarzschild and Kerr spacetimes within 0.1 percent, and process noise injection.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 24 |
| Failed checks | 0 |
| Duration | 159.7 s |
| Generated | 2026-10-19 22:19:17 UTC |

## Covariance algebra

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the first standard deviation is the root of the first variance | actual=2 expected=2 abs_error=0 tolerance=1e-12 |
| PASS | the second standard deviation is the root of the second variance | actual=3 expected=3 abs_error=0 tolerance=1e-12 |
| PASS | the eigenvalues of a covariance matrix are positive |  |
| PASS | the eigenvalues sum to the trace | actual=13 expected=13 abs_error=0 tolerance=1e-12 |
| PASS | the principal semi-axes are sorted in decreasing order |  |
| PASS | the squared semi-axes sum to the trace | actual=13 expected=13 abs_error=0 tolerance=1e-12 |
| PASS | the propagated variances stay positive |  |
| PASS | the confidence hypervolume is positive |  |

## Submatrix extraction and sampling

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the position block carries the expected variances and correlation | value=0 limit=1e-12 |
| PASS | the spatial block carries the expected variances | value=0 limit=1e-12 |
| PASS | the momentum block carries the expected variances and correlation | value=0 limit=1e-12 |
| PASS | a multivariate Gaussian sample has eight components |  |

## Phase-space Jacobian

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the flat-space Jacobian has the identity in the position-momentum block and zeros elsewhere | value=0 limit=1e-15 |

## Exact flat-space covariance

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the nominal worldline follows the straight-line solution | value=1.20792e-13 limit=1e-12 |
| PASS | the propagated covariance equals the exact Sigma_xx = S0 + lambda^2 Sp, Sigma_xp = lambda Sp, Sigma_pp = Sp | value=7.77156e-16 limit=1e-10 |
| PASS | the flat-space transition matrix has unit determinant | actual=1 expected=1 abs_error=0 tolerance=1e-10 |

## Minkowski Monte-Carlo validation

| Result | Check | Detail |
| --- | --- | --- |
| PASS | Minkowski: the semi-axes of the propagated covariance agree with the million-particle ensemble within the tolerance |  |
| PASS | Minkowski: the largest relative semi-axis error is below 0.1 percent | value=7.1275e-14 limit=0.001 |

## Schwarzschild Monte-Carlo validation

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the transition matrix of a Hamiltonian flow has unit determinant | actual=1 expected=1 abs_error=2.22e-16 tolerance=0.0001 |
| PASS | Schwarzschild: the semi-axes of the propagated covariance agree with the million-particle ensemble within the tolerance |  |
| PASS | Schwarzschild: the largest relative semi-axis error is below 0.1 percent | value=3.37384e-06 limit=0.001 |

## Kerr Monte-Carlo validation

| Result | Check | Detail |
| --- | --- | --- |
| PASS | Kerr: the semi-axes of the propagated covariance agree with the million-particle ensemble within the tolerance |  |
| PASS | Kerr: the largest relative semi-axis error is below 0.1 percent | value=1.33821e-06 limit=0.001 |

## Process noise

| Result | Check | Detail |
| --- | --- | --- |
| PASS | diffusion noise creates a positive variance in every phase-space direction |  |

## Measurements

| Section | Name | Value | Unit |
| --- | --- | --- | --- |
| Minkowski Monte-Carlo validation | Minkowski max_semi_axis_error | 7.1275014e-14 | relative |
| Schwarzschild Monte-Carlo validation | Schwarzschild max_semi_axis_error | 3.3738391e-06 | relative |
| Kerr Monte-Carlo validation | Kerr max_semi_axis_error | 1.33820857e-06 | relative |


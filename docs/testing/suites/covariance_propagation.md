# Covariance propagation suite

Executable: `test_covariance_propagation` - Source: `tests/test_covariance_propagation.cpp` - Results: [covariance_propagation.md](../results/covariance_propagation.md)

## Purpose

Verifies `CovarianceMatrix` and `VariationalGeodesicIntegrator` ([MATHEMATICAL_FORMULATION.md](../../MATHEMATICAL_FORMULATION.md#73-lyapunov-covariance-matrix-propagation)) against an exact solution and one-million-particle ensembles.

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Covariance algebra | Statistics and Lyapunov step | Standard deviations and trace identities within 1e-12; positive variances and hypervolume after a step |
| Submatrix extraction and sampling | Blocks | Position, spatial and momentum blocks within 1e-12; a Gaussian sample of eight components |
| Phase-space Jacobian | Flat structure | Identity coupling and zero blocks within 1e-15 |
| Exact flat-space covariance | Free particle | Worldline within 1e-12; covariance within 1e-10 of `S0 + lambda^2 Sp`, `lambda Sp`, `Sp`; unit determinant within 1e-10 |
| Minkowski, Schwarzschild and Kerr Monte-Carlo validation | Semi-axes | Million-particle ensembles agree within 0.1 percent; Schwarzschild transition determinant within 1e-4 of 1 |
| Process noise | Diffusion | Positive variance in all eight directions |

## Justification

For a free particle the transition matrix is `[[1, lambda], [0, 1]]` per component and the covariance follows from it exactly. For curved spacetimes the Monte-Carlo ensemble is an independent estimate whose sampling error at 1e6 particles stays below the 0.1 percent tolerance.

## Scenario

Seeds `0x123456789ABCDEF0`, `0xABCDEF0123456789` and `0xFEEDBEEFCAFE0001`; Minkowski up to lambda = 2 (step 0.5), Schwarzschild and Kerr (a = 0.9) up to lambda = 0.5 (step 0.05).

## Measurements

Largest relative semi-axis error per spacetime.

## Acceptance

All checks pass.

## Recorded results

See [../results/covariance_propagation.md](../results/covariance_propagation.md).

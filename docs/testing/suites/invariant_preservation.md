# Invariant preservation suite

Executable: `test_invariant_preservation` - Source: `tests/test_invariant_preservation.cpp` - Results: [invariant_preservation.md](../results/invariant_preservation.md)

## Purpose

Verifies that `RK45AdaptiveIntegrator` preserves the four-velocity normalization of [MATHEMATICAL_FORMULATION.md](../../MATHEMATICAL_FORMULATION.md#22-second-order-geodesic-differential-equations) on a long orbit and across a horizon.

## Scenario

A circular orbit at r = 20 M in Schwarzschild over 50000 steps (rtol 1e-10, atol 1e-14, invariant tolerance 1e-12); a radial free fall from r = 8 M in Painleve-Gullstrand coordinates over up to 20000 steps.

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Circular timelike orbit | Steps | Every adaptive step succeeds |
| Circular timelike orbit | Residual | Maximum normalization residual below 1e-11 |
| Horizon infall | Steps | Every adaptive step succeeds |
| Horizon infall | Crossing | The particle crosses the Schwarzschild radius |
| Horizon infall | Residual | Maximum normalization residual below 1e-10 |

## Justification

The integrator projects the state back onto the constraint, so the residual is bounded by the projection tolerance; the interior bound is looser because the metric gradients grow near the singularity.

## Measurements

Both residuals.

## Acceptance

All checks pass.

## Recorded results

See [../results/invariant_preservation.md](../results/invariant_preservation.md).

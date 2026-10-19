# Exotic metrics suite

Executable: `test_exotic_metrics` - Source: `tests/test_exotic_metrics.cpp` - Results: [exotic_metrics.md](../results/exotic_metrics.md)

## Purpose

Verifies the traversable wormhole and the warp bubble of [MATHEMATICAL_FORMULATION.md](../../MATHEMATICAL_FORMULATION.md#18-morris-thorne-traversable-wormhole) and [Section 1.9](../../MATHEMATICAL_FORMULATION.md#19-alcubierre-warp-drive-metric).

## Scenario

A wormhole of throat radius 2 crossed by a radial null ray from l = -10 with the adaptive integrator (rtol 1e-11, atol 1e-13, at most 500 steps), and a warp bubble with v_s = 2, R = 50 and sigma = 0.2.

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Morris-Thorne throat | Metric components | -1, 1, b0^2 and b0^2 within 1e-12 |
| Throat transit | Integration | Every step succeeds |
| Throat transit | Crossing | The coordinate l becomes positive, the closest approach is below 0.2 and the ray reaches l = 10 |
| Morris-Thorne geometry | Shared invariants | Connection within a relative 1e-8 |
| Alcubierre bubble | Shaping function | 1 within 1e-6 at the centre, below 1e-6 at 500 |
| Alcubierre bubble | Inversion at the centre | Within 1e-12 |
| Alcubierre bubble | Shared invariants at the wall | Connection within a relative 1e-6 |

## Justification

At the throat the areal radius equals b0 and the redshift potential vanishes, which fixes the metric components. The shaping function tends to 1 inside and 0 outside the bubble.

## Acceptance

All checks pass.

## Recorded results

See [../results/exotic_metrics.md](../results/exotic_metrics.md).

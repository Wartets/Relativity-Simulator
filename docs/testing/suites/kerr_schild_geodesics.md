# Kerr-Schild geodesics suite

Executable: `test_kerr_schild_geodesics` - Source: `tests/test_kerr_schild_geodesics.cpp` - Results: [kerr_schild_geodesics.md](../results/kerr_schild_geodesics.md)

## Purpose

Verifies timelike geodesics in the horizon-regular Cartesian form of [MATHEMATICAL_FORMULATION.md](../../MATHEMATICAL_FORMULATION.md#13-kerr-metric-rotating-black-hole) with `KerrSchildMetric` and the Dormand-Prince integrator in continuity mode.

## Scenario

M = 1. A particle at (-3, 0, 6) with velocity (0.5, 0, -0.1) around a hole of spin 0.8, 2000 steps (steps 0.01 to 0.05); a radial infall from (4, 0, 0) with velocity -0.4 around a hole of spin 0.7, up to 5000 steps (steps 0.005 to 0.02). Tolerances are rtol 1e-11 and atol 1e-13.

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Polar axis crossing | Normalization | Real temporal velocity |
| Polar axis crossing | Steps | Every adaptive step succeeds |
| Polar axis crossing | Coordinates | Finite throughout |
| Horizon infall | Normalization | Real temporal velocity |
| Horizon infall | Steps | Every adaptive step succeeds |
| Horizon infall | Crossing | The Boyer-Lindquist radius falls below the outer horizon |

## Justification

Cartesian Kerr-Schild coordinates have no polar singularity, so a trajectory through the axis must not break; the metric is regular at the horizon, so the infall must cross it.

## Limits

Conservation laws are covered by [carter_constant.md](carter_constant.md).

## Acceptance

All checks pass.

## Recorded results

See [../results/kerr_schild_geodesics.md](../results/kerr_schild_geodesics.md).

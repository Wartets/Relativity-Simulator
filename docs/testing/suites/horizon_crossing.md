# Horizon crossing suite

Executable: `test_horizon_crossing` - Source: `tests/test_horizon_crossing.cpp` - Results: [horizon_crossing.md](../results/horizon_crossing.md)

## Purpose

Verifies the horizon-regular forms of [MATHEMATICAL_FORMULATION.md](../../MATHEMATICAL_FORMULATION.md#12-schwarzschild-metric) and the boundary modes of [DESCRIPTION.md](../../DESCRIPTION.md#32-strong-gravity--curved-spacetime-phenomena), using the shared invariants of `tests/support/metric_checks.hpp`.

## Scenario

M = 1 and c = G = 1. Metric regularity at 0.1, 0.5, 0.99, 1, 1.01, 2, 5 and 10 Schwarzschild radii. Free fall in Painleve-Gullstrand from 4 r_s, an ingoing photon in Eddington-Finkelstein from 5 r_s with advanced time 10, absorption from 2 r_s, continuity from 1.5 r_s and a trapped ray from 0.5 r_s.

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Regularity (both metrics) | Schwarzschild radius | 2 within 1e-15 |
| Regularity (both metrics) | Inversion | Within 1e-12 |
| Regularity (both metrics) | Finiteness | Analytic and numerical connections finite |
| Regularity (both metrics) | Connection | Relative to the largest symbol of its row, below 1e-6 |
| Painleve-Gullstrand free fall | Steps, finiteness, crossing | All steps succeed, state finite, horizon crossed, statistics report it, final radius below 0.1 r_s |
| Painleve-Gullstrand free fall | Invariants | Temporal velocity within 1e-6 of 1, normalization within 1e-8 of -c^2 |
| Eddington-Finkelstein photon | Steps, crossing | All steps succeed, horizon crossed, statistics report it, final radius below 0.1 r_s |
| Eddington-Finkelstein photon | Invariants | Advanced time within 1e-8, `u^v` within 1e-8 of 0, `u^r` within 1e-8 of -1, null constraint below 1e-12 |
| Horizon boundary modes | Absorption | Terminates, crossed and absorbed, no singularity, final radius at or below r_s |
| Horizon boundary modes | Continuity | Terminates, crossed without absorption, singularity reached, final radius within the threshold |
| Interior trapped region | Initial direction | Radial velocity negative |
| Interior trapped region | Evolution | Steps succeed, radius always below r_s, threshold reached |

## Justification

The connection criterion uses the row-scale rule of [CONTRIBUTING_TESTS.md](../CONTRIBUTING_TESTS.md#determinism-and-tolerances) instead of a clamp to unity, so components far below 1 are not masked. Exact null and radial relations hold in Eddington-Finkelstein because `v` is constant along ingoing radial null rays.

## Acceptance

All checks pass.

## Recorded results

See [../results/horizon_crossing.md](../results/horizon_crossing.md).

# Kerr metrics suite

Executable: `test_kerr_metrics` - Source: `tests/test_kerr_metrics.cpp` - Results: [kerr_metrics.md](../results/kerr_metrics.md)

## Purpose

Verifies the rotating black hole geometry of [MATHEMATICAL_FORMULATION.md](../../MATHEMATICAL_FORMULATION.md#13-kerr-metric-rotating-black-hole) in Boyer-Lindquist and Kerr-Schild form, using the shared invariants of `tests/support/metric_checks.hpp`.

## Scenario

M = 1 and a = 0.8 at (r, theta, phi) = (5, pi/3, 0.2) for Boyer-Lindquist; M = 1 and a = 0.85 at the Cartesian point (3, 2, 1.5) for Kerr-Schild.

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Horizons and ergosphere | Regime flags | Sub-extremal, not extremal, not hyper-extremal |
| Horizons and ergosphere | Horizon radii | M +- sqrt(M^2 - a^2) within 1e-14 |
| Horizons and ergosphere | Ergosphere | Equal to the outer horizon at the pole and to 2 M at the equator within 1e-14 |
| Boyer-Lindquist geometry | Inversion | `g^-1 g` equals the identity within 1e-13 |
| Boyer-Lindquist geometry | Symmetry and signature | Relative asymmetry below 1e-14, negative determinant |
| Boyer-Lindquist geometry | Connection | Lower-index symmetry within 1e-14; analytic symbols equal the eighth-order finite differences within a relative 1e-8 of the largest symbol of each row |
| Boyer-Lindquist geometry | Frame dragging | The g_t_phi component is non-zero and symmetric; the ZAMO angular velocity is positive |
| Kerr-Schild geometry | Same invariants | Same tolerances as above |
| Kerr-Schild geometry | Determinant | Equals -1 within 1e-12 |

## Justification

The Kerr-Schild form is a flat metric plus a rank-one null term, whose determinant is exactly -1. The row-relative connection criterion avoids hiding errors on components far below the largest symbol.

## Limits

One field point is tested per form; polar-axis behavior is covered by [test_kerr_schild_geodesics](../test_catalog.md).

## Acceptance

All checks pass.

## Recorded results

See [../results/kerr_metrics.md](../results/kerr_metrics.md).

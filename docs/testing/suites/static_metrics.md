# Static metrics suite

Executable: `test_static_metrics` - Source: `tests/test_static_metrics.cpp` - Results: [static_metrics.md](../results/static_metrics.md)

## Purpose

Guarantees that every static or exotic spacetime metric provides a consistent metric tensor, inverse metric and analytic Christoffel symbols, in dimensionless and SI unit systems.

## Covered metrics

Flat Minkowski, Schwarzschild, Schwarzschild isotropic, Painleve-Gullstrand, Eddington-Finkelstein, Reissner-Nordstrom (M = 1, Q = 0.6), Morris-Thorne (b0 = 1, Phi0 = 0.3) and Alcubierre (vs = 1.5, R = 100, sigma = 0.1).

## Checks per metric

| Check | Method | Tolerance |
| --- | --- | --- |
| Inversion | Largest absolute deviation of `g^-1 g` from the identity | 1e-12 geometric, 1e-9 SI |
| Symmetry | Largest `g_ij - g_ji` relative to the largest component | 1e-14 |
| Signature | Determinant of the metric is negative | exact |
| Christoffel | Analytic symbols against eighth-order finite differences, relative error per component with a floor of 1e-9 times the largest symbol of the same row | 1e-7 geometric, 1e-6 SI |
| Lower symmetry | `Gamma^s_mn - Gamma^s_nm` | 1e-14 |

The Christoffel comparison is scale aware: the previous suite used `max(1, |value|)` as scale, which hid errors on components much smaller than one in SI units. A failing comparison reports the offending index triple.

## Additional checks

- Vacuum curvature on Schwarzschild, Painleve-Gullstrand and Eddington-Finkelstein at r = 10 M: Ricci tensor and scalar vanish (1e-6 of the curvature scale), and the Kretschmann invariant equals 48 M^2 / r^6 within 1e-4. This validates the Riemann, Ricci and Kretschmann code together with each metric.
- Horizon regularity of Painleve-Gullstrand and Eddington-Finkelstein coordinates in both unit systems.
- Reissner-Nordstrom horizon radii (1.8 and 0.2), photon sphere (3 M for Q = 0), and sub-, extremal and hyper-extremal classification.
- Morris-Thorne areal radius at and away from the throat.
- Alcubierre shaping function limits at the centre and far away.

## Acceptance

All checks pass. The suite is deterministic and takes a few seconds, dominated by the finite-difference Riemann evaluation.

## Results

See [../results/static_metrics.md](../results/static_metrics.md), regenerated at each run.

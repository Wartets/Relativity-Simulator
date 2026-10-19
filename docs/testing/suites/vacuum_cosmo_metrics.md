# Vacuum and cosmological metrics suite

Executable: `test_vacuum_cosmo_metrics` - Source: `tests/test_vacuum_cosmo_metrics.cpp` - Results: [vacuum_cosmo_metrics.md](../results/vacuum_cosmo_metrics.md)

## Purpose

Verifies the electrovacuum and cosmological spacetimes of [MATHEMATICAL_FORMULATION.md](../../MATHEMATICAL_FORMULATION.md#14-reissner-nordström-metric-charged-black-hole) through [Section 1.7](../../MATHEMATICAL_FORMULATION.md#17-flrw-metric-cosmological-spacetime).

## Scenario

| Metric | Parameters | Field point |
| --- | --- | --- |
| Reissner-Nordstrom | M = 1, Q = 0.6 and 0.5 | (5, 1.2, 0.4) |
| Kerr-Newman | M = 1, a = 0.5, Q = 0.3 | (3.5, 1.1, 0.7) |
| Schwarzschild-de Sitter | M = 1, Lambda = 1e-5 | (10, 1.3, 0.5) |
| Kerr-de Sitter | M = 1, a = 0.4, Lambda = 1e-4 | (6, 1.2, 0.3) |
| FLRW | Matter dominated, t0 = 1 | (2, 0.5, 1, 0.2) |

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Reissner-Nordstrom horizons | Radii and regimes | 1.8 and 0.2 for Q = 0.6, both 1 for Q = M, hyper-extremal for Q > M, photon sphere 3 M for Q = 0, all within 1e-12 |
| Reissner-Nordstrom geometry | Shared invariants | Inversion 1e-12, symmetry 1e-14, negative determinant, connection within a relative 1e-8 |
| Kerr-Newman | Horizon and reduction | M + sqrt(M^2 - a^2 - Q^2) within 1e-12; the uncharged metric inverts within 1e-12 |
| Kerr-Newman | Shared invariants | Connection within a relative 1e-7 |
| De Sitter extensions | Shared invariants | Schwarzschild-de Sitter within 1e-8, Kerr-de Sitter inversion 1e-11 and connection 1e-7 |
| FLRW | Expansion | a(1) = 1 and a(8) = 4, redshift 3, inversion 1e-12 |

## Justification

The matter-dominated scale factor grows as t^(2/3), so a(8) = 4 and the redshift between t = 1 and t = 8 is 3.

## Limits

The FLRW connection is not compared against finite differences because it depends explicitly on time.

## Acceptance

All checks pass.

## Recorded results

See [../results/vacuum_cosmo_metrics.md](../results/vacuum_cosmo_metrics.md).

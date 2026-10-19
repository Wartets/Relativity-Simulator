# Symplectic Gauss-Legendre suite

Executable: `test_symplectic_gauss_legendre` - Source: `tests/test_symplectic_gauss_legendre.cpp` - Results: [symplectic_gauss_legendre.md](../results/symplectic_gauss_legendre.md)

## Purpose

Verifies the implicit Gauss-Legendre integrators of [ARCHITECTURE.md](../../ARCHITECTURE.md#23-differential-solvers--geodesic-integrators).

## Scenario

Flat spacetime: 100 steps of 0.1 for a particle with u = (sqrt(1.14), 0.1, 0.2, 0.3), fourth-order scheme. Schwarzschild: circular orbit at r = 10 M with 2000 steps of 0.05, sixth-order scheme.

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Flat spacetime | Convergence | All steps converge |
| Flat spacetime | Position | x, y, z advance exactly by u times the elapsed time, within 1e-12 |
| Circular orbit | Convergence | All steps converge |
| Circular orbit | Invariants | Drift of the normalization, energy and angular momentum below 1e-11 at every step |

The drifts are also recorded as measurements.

## Justification

A symplectic scheme preserves the quadratic invariants of the Hamiltonian system without secular drift, and the linear motion of flat spacetime is reproduced exactly by any consistent Runge-Kutta method.

## Acceptance

All checks pass.

## Recorded results

See [../results/symplectic_gauss_legendre.md](../results/symplectic_gauss_legendre.md).

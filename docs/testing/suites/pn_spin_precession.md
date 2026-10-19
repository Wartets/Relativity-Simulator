# Post-Newtonian spin precession suite

Executable: `test_pn_spin_precession` - Source: `tests/test_pn_spin_precession.cpp` - Results: [pn_spin_precession.md](../results/pn_spin_precession.md)

## Purpose

Verifies `PostNewtonianSpinSolver::compute_spin_precession_binary` against the spin-orbit and spin-spin couplings of [MATHEMATICAL_FORMULATION.md](../../MATHEMATICAL_FORMULATION.md#33-spin-orbit--spin-spin-coupling).

## Scenario

Unit masses, separation (10, 0, 0), relative velocity (0, 0.2, 0), spins (0, 0, 0.5) and (0.1, 0, 0.4), configuration `make_all_enabled`.

## Checks

| Check | Criterion |
| --- | --- |
| Precession direction | The z component of both precession angular velocities is positive for an orbital angular momentum along +z |
| Spin magnitude conservation | `|S . dS/dt|` is below 1e-12 times `|S||dS/dt|` for each body, with an absolute floor of 1e-300 |

## Justification

A precession is a rotation, so the derivative of the spin is orthogonal to the spin and its norm is conserved. The tolerance is relative to the product of the norms so that it does not depend on the unit system.

## Acceptance

All checks pass.

## Recorded results

See [../results/pn_spin_precession.md](../results/pn_spin_precession.md).

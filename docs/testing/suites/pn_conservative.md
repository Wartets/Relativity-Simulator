# Post-Newtonian conservation suite

Executable: `test_pn_conservative` - Source: `tests/test_pn_conservative.cpp` - Results: [pn_conservative.md](../results/pn_conservative.md)

## Purpose

Verifies that the conservative part of the post-Newtonian dynamics ([MATHEMATICAL_FORMULATION.md](../../MATHEMATICAL_FORMULATION.md#31-conservative-1pn-acceleration-einstein-infeld-hoffmann)) conserves energy and orbital angular momentum, and that the integrators of [ARCHITECTURE.md](../../ARCHITECTURE.md#34-orchestrator-state-advance) do not introduce secular drift.

## Scenario

Two bodies of unit mass and unit radius start at a separation of 20 on the x axis with opposite velocities of 0.15 along y, in units where G and c follow `PNOrderConfig`. Each run takes 2000 steps of 0.01.

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| 1PN binary with the Runge-Kutta integrator | Initial energy | Finite |
| 1PN binary with the Runge-Kutta integrator | Energy drift | Relative drift below 1e-4 |
| 1PN binary with the Runge-Kutta integrator | Angular momentum drift | Relative drift of the z component below 1e-4 |
| Newtonian binary with the symplectic integrator | Energy drift | Relative drift below 1e-5 |

## Justification

The fourth-order scheme at a step of 0.01 over a binary of period of order 1e3 leaves truncation errors well below 1e-4. The symplectic scheme bounds the energy error without secular growth, which supports the tighter 1e-5 bound.

## Measurements

The three relative drifts are recorded in the result file.

## Limits

Only the equal-mass, near-circular configuration is covered. Spin and radiation terms are covered by [pn_spin_precession.md](pn_spin_precession.md) and [pn_radiation_reaction.md](pn_radiation_reaction.md).

## Acceptance

All checks pass. The suite is deterministic.

## Recorded results

See [../results/pn_conservative.md](../results/pn_conservative.md) and [../results/pn_conservative.json](../results/pn_conservative.json), regenerated at every run.

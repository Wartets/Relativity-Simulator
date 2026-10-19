# Post-Newtonian radiation reaction suite

Executable: `test_pn_radiation_reaction` - Source: `tests/test_pn_radiation_reaction.cpp` - Results: [pn_radiation_reaction.md](../results/pn_radiation_reaction.md)

## Purpose

Verifies the dissipative 2.5PN term of [MATHEMATICAL_FORMULATION.md](../../MATHEMATICAL_FORMULATION.md#32-25pn-radiation-reaction-acceleration): a binary evolved with radiation reaction only must lose energy while radiating.

## Scenario

Two unit masses at a separation of 15 with opposite velocities of 0.2 along y, configuration `make_2_5pn_radiation_only`, 2000 Runge-Kutta steps of 0.01.

## Checks

| Check | Criterion |
| --- | --- |
| Final energy | Finite |
| Energy loss | The final total energy is lower than the initial total energy |
| Radiated power | The latest quadrupole luminosity is positive |

## Measurements

Initial energy, final energy and radiated power.

## Limits

The suite checks the sign of the dissipation, not its magnitude. The magnitude is validated by [hulse_taylor_orbital_decay.md](hulse_taylor_orbital_decay.md).

## Acceptance

All checks pass.

## Recorded results

See [../results/pn_radiation_reaction.md](../results/pn_radiation_reaction.md).

# Equation of state and TOV suite

Executable: `test_eos_tov` - Source: `tests/test_eos_tov.cpp` - Results: [eos_tov.md](../results/eos_tov.md)

## Purpose

Verifies the equations of state of [MATHEMATICAL_FORMULATION.md](../../MATHEMATICAL_FORMULATION.md#43-equations-of-state-eos) and the Tolman-Oppenheimer-Volkoff solver of [ARCHITECTURE.md](../../ARCHITECTURE.md#26-relativistic-hydrodynamics-grhdgrmhd).

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Ideal gas | Relations | Pressure, specific energy, enthalpy and sound speed within 1e-12; sound speed in (0, Gamma - 1) |
| Synge and Mathews gases | Limits | Cold enthalpy `1 + 5 theta/2` within 1e-4; hot enthalpy `4 theta` within 5 percent; Mathews sound speed in (0, 1/3) when cold and 1/3 within 1e-4 when hot |
| Relativistic degenerate Fermi gas | Identities | Energy density above pressure; enthalpy `sqrt(1 + x^2)` within 1e-12; sound speed within 1e-6; inversions within a relative 1e-8 |
| Tabulated nuclear equation of state | SFHo | Positive pressure, causal sound speed, container round trip within 1e-10 |
| White dwarf | Solution | Converged; mass in (0.5, 1.44); radius in (1000, 20000) km; compactness below 1e-3 |
| Free neutron gas | Maximum mass | Between 0.65 and 0.80 solar masses at a radius between 8 and 12 km |
| Nuclear equations of state | SFHo | Maximum mass in (1.95, 2.15); radius at maximum in (9, 13) km; radius at 1.4 in (10.5, 14) km |
| Nuclear equations of state | Shen and LS220 | Maximum masses in (2.05, 2.30) and (1.95, 2.15), Shen above LS220 |

## Justification

The free neutron gas gives the Oppenheimer-Volkoff mass of 0.71 solar masses, the white dwarf must stay under the Chandrasekhar mass, and observed neutron stars constrain the nuclear tables to about 2 solar masses.

## Measurements

Maximum masses of the free gas, SFHo, Shen and LS220.

## Acceptance

All checks pass.

## Recorded results

See [../results/eos_tov.md](../results/eos_tov.md).

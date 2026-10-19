# Relativistic hydrodynamics suite

Executable: `test_relativistic_hydro` - Source: `tests/test_relativistic_hydro.cpp` - Results: [relativistic_hydro.md](../results/relativistic_hydro.md)

## Purpose

Verifies the GRHD and GRMHD stack of [DESCRIPTION.md](../../DESCRIPTION.md#6-relativistic-hydrodynamics-grhdgrmhd--accretion-systems) and [MATHEMATICAL_FORMULATION.md](../../MATHEMATICAL_FORMULATION.md#4-relativistic-hydrodynamics-grhdgrmhd). Stellar structure and nuclear tables are in [eos_tov.md](eos_tov.md).

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Equation of state | Ideal gas | Pressure, energy, enthalpy and pressure from enthalpy within 1e-12; sound speed in (0, Gamma - 1) |
| Equation of state | Synge | Enthalpy above 1; sound speed squared in (0, 1/3] |
| Conserved to primitive inversion | Hydrodynamic, v = 0.999, magnetized | Round trips within 1e-10, 1e-8 and 1e-6; magnetic field within 1e-12 |
| Spatial reconstruction | WENO5 and MP5 | Smooth profile within 1e-6; bounded across a jump |
| Riemann solvers | Uniform state | HLL, HLLC and HLLD give zero mass flux and a momentum flux equal to the pressure within 1e-12; positive Sod flux |
| Constrained transport | Divergence | Below 1e-12 initially and over fifty updates |
| Relativistic Sod shock tube | Evolution | Undisturbed boundary states within 0.1; shocked velocity in (0.60, 0.80); shocked pressure in (0.01, 5); every cell physical |
| Relativistic Balsara 1 magnetized shock tube | Evolution | Every cell has finite positive density and pressure, subluminal velocity and finite transverse field |

## Justification

Each limit is a closed-form identity of the scheme: exact round trips, preserved uniform states and a divergence-free vector-potential field. The shock tube bounds correspond to the known relativistic Riemann solution.

## Measurements

Maximum divergence of the magnetic field.

## Acceptance

All checks pass.

## Recorded results

See [../results/relativistic_hydro.md](../results/relativistic_hydro.md).

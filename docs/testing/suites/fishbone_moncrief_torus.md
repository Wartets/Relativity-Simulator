# Fishbone-Moncrief torus suite

Executable: `test_fishbone_moncrief_torus` - Source: `tests/test_fishbone_moncrief_torus.cpp` - Results: [fishbone_moncrief_torus.md](../results/fishbone_moncrief_torus.md)

## Purpose

Verifies `FishboneMoncriefTorus` of [DESCRIPTION.md](../../DESCRIPTION.md#63-relativistic-accretion-disks--tori).

## Scenario

| Case | a | r_in | r_center | Gamma | Peak density |
| --- | --- | --- | --- | --- | --- |
| Equilibrium | 0.9 | 4.5 | 8.5 | 4/3 | 1000 |
| Normalization | 0.5 | 5 | 10 | 5/3 | default |
| Polytropic | 0.8 | 4.8 | 9 | 4/3 | 500 |
| Cartesian | 0 | 6 | 12 | 5/3 | 100 |

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Equilibrium and cusp | Angular momentum, potentials, enthalpy | Positive angular momentum, potential at the cusp above that at the centre, maximum enthalpy above 1 |
| Equilibrium and cusp | Centre and cusp | Enthalpy equals the maximum at the centre (1e-12) and 1 at the cusp (1e-12); density equals the configured maximum (relative 1e-10) |
| Equilibrium and cusp | Exterior | Zero density and pressure inside the cusp |
| Four-velocity normalization | u.u | Equals -1 within 1e-10 at every sampled radius inside the torus |
| Polytropic relations | Pressure and enthalpy | P = K rho^Gamma and h = 1 + Gamma/(Gamma-1) P/rho within a relative 1e-10 |
| Cartesian evaluation | Centre and cusp | Density above 99 at the centre and below 1e-10 inside the cusp |

## Justification

A torus with constant specific angular momentum is in hydrostatic equilibrium when the enthalpy is a function of the potential alone, which fixes the cusp and centre values.

## Acceptance

All checks pass.

## Recorded results

See [../results/fishbone_moncrief_torus.md](../results/fishbone_moncrief_torus.md).

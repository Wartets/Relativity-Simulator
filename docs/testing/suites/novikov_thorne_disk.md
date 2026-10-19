# Novikov-Thorne disk suite

Executable: `test_novikov_thorne_disk` - Source: `tests/test_novikov_thorne_disk.cpp` - Results: [novikov_thorne_disk.md](../results/novikov_thorne_disk.md)

## Purpose

Verifies `NovikovThorneDisk` of [MATHEMATICAL_FORMULATION.md](../../MATHEMATICAL_FORMULATION.md#51-novikov-thorne-thin-disk-profile). The ray tracers use a parametrized disk instead ([TECHNICAL_MANUAL.md](../../TECHNICAL_MANUAL.md#metric-usage-in-softwarecomputeengine)).

## Scenario

Solar-mass holes with a = 0, 0.998 r_g and -0.9 r_g; geometrized unit holes with a* in {0, 0.2, 0.5, 0.8, 0.95, -0.3, -0.7}; a hole of mass 10 with a = 5; a ten solar mass hole with a* = 0.5 and an accretion rate of 1e15 kg/s.

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Innermost stable circular orbit | Schwarzschild | 6 r_g within a relative 1e-12 |
| Innermost stable circular orbit | Prograde and retrograde | Between 1 and 1.3 r_g, between 8 and 9.5 r_g |
| Page-Thorne function | Zero at the ISCO | Exactly zero for each spin |
| Page-Thorne function | Closed form against quadrature | Relative deviation below 1e-7 for radii from 1.05 to 50 ISCO radii |
| Radiative efficiency | Schwarzschild | 1 - sqrt(8/9) within 1e-10 |
| Radiative efficiency | a = 0.998 | Between 0.30 and 0.35 |
| Circular four-velocity | Normalization | u.u = -1 within 1e-10 for radii from 1.01 to 20 ISCO radii |
| Flux and temperature profile | Inside the ISCO | Both vanish |
| Flux and temperature profile | Outside | Positive, with a flux maximum between one and three ISCO radii |

## Justification

The Page-Thorne integral has a closed form that is independently obtained by quadrature, and the efficiency 1 - E_ISCO equals 1 - sqrt(8/9) for a non-rotating hole.

## Acceptance

All checks pass.

## Recorded results

See [../results/novikov_thorne_disk.md](../results/novikov_thorne_disk.md).

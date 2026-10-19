# Dark matter profiles suite

Executable: `test_dark_matter_profiles` - Source: `tests/test_dark_matter_profiles.cpp` - Results: [dark_matter_profiles.md](../results/dark_matter_profiles.md)

## Purpose

Verifies the analytic halo models of [DESCRIPTION.md](../../DESCRIPTION.md#53-dark-matter-halos--alternative-gravitational-theories).

## Scenario

Characteristic density 1e7, scale radius 20, gravitational constant 1.

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Navarro-Frenk-White | Density at the scale radius | rho0 / 4, relative 1e-12 |
| Navarro-Frenk-White | Enclosed mass | 4 pi rho0 rs^3 (ln 2 - 1/2), relative 1e-12 |
| Navarro-Frenk-White | Peak velocity | Velocity at 0.9 and 1.1 times the reported radius does not exceed the peak |
| Navarro-Frenk-White | Acceleration | Negative radial component, transverse part below 1e-12 of it |
| Einasto | Density, mass, velocity | Density rho0 at the scale radius, positive mass and velocity |
| Burkert | Density, mass | rho0 / 4 at the scale radius, positive mass |
| Hernquist | Mass and potential | M/4 enclosed and potential -G M / (2 a) at the scale radius, relative 1e-14 |

## Acceptance

All checks pass.

## Recorded results

See [../results/dark_matter_profiles.md](../results/dark_matter_profiles.md).

# Polarized radiative transfer suite

Executable: `test_polarized_radiative_transfer` - Source: `tests/test_polarized_radiative_transfer.cpp` - Results: [polarized_radiative_transfer.md](../results/polarized_radiative_transfer.md)

## Purpose

Verifies the polarimetric chain of [MATHEMATICAL_FORMULATION.md](../../MATHEMATICAL_FORMULATION.md#6-polarized-radiative-transfer). The renderer does not evaluate Stokes transport ([DESCRIPTION.md](../../DESCRIPTION.md#93-continuous-spectral-pipeline--cie-1931-integration)), so the components are validated standalone.

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Stokes vector | Invariants | Valid, intensity 10, fully polarized, angle `0.5 atan2(U, Q)` within 1e-12; a quarter turn exchanges Q and U |
| Bessel functions and Maxwell-Juttner distribution | Bessel | K0, K1 and K2 at 1 in their ranges; `K2 = 2 K1 + K0` within 1e-10 |
| Bessel functions and Maxwell-Juttner distribution | Sampling | Lorentz factors at least 1; sample mean within 0.1 of the analytic mean in (3, 4) |
| Synchrotron emission | Power law | Polarization `(p + 1)/(p + 7/3)` within 1e-12 for p = 3 and 2; absorption polarization equal |
| Thermal bremsstrahlung | Emissivity | Positive emissivity and a Gaunt factor in (0.5, 20) |
| Polarized transfer | Steps | The Delano step is valid with positive intensity and polarization; the Runge-Kutta step has positive intensity |
| Inverse Compton scattering | Cross section and energy | Thomson limit within a relative 1e-3; positive y parameter; packets conserved; mean energy rises |

## Justification

The synchrotron polarization of a power law is a textbook closed form, the Bessel recurrence is exact, and Comptonization by hot electrons must raise the mean photon energy.

## Acceptance

All checks pass.

## Recorded results

See [../results/polarized_radiative_transfer.md](../results/polarized_radiative_transfer.md).

# Spherical harmonics suite

Executable: `test_spherical_harmonics` - Source: `tests/test_spherical_harmonics.cpp` - Results: [spherical_harmonics.md](../results/spherical_harmonics.md)

## Purpose

Verifies the gravimetry layer of [DESCRIPTION.md](../../DESCRIPTION.md#52-spherical-harmonics--high-degree-geodesy).

## Scenario

Legendre functions at sin(phi) = 0.5 up to degree 8; the EGM96 model truncated at degree and order 10 evaluated at (4.5e6, 2.5e6, 4.8e6) m; the Earth, Moon and Jupiter presets.

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Associated Legendre functions | P00, P10, P11, P20, P21, P22 | Closed forms within 1e-14 |
| Associated Legendre functions | dP20/dphi | 3 sin cos within 1e-13 |
| Acceleration against potential gradient | Each axis | Central difference of the potential (step 1 m) equals the analytic acceleration within a relative 1e-8 |
| Planetary models | Zonal coefficients | Earth J2, J3, J4, lunar J2 and Jovian J2 within 1e-12 |

## Justification

The acceleration is the negative gradient of the potential, so an independent finite difference of the potential validates the analytic derivative recurrences.

## Acceptance

All checks pass.

## Recorded results

See [../results/spherical_harmonics.md](../results/spherical_harmonics.md).

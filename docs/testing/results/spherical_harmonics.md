# spherical_harmonics

Verifies the spherical harmonic gravity model: unnormalized associated Legendre functions and their derivative against closed forms, the consistency of the analytic acceleration with the numerical gradient of the potential, and the zonal coefficients of the Earth, Moon and Jupiter models.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 15 |
| Failed checks | 0 |
| Duration | 0.002098 s |
| Generated | 2026-10-19 22:21:17 UTC |

## Associated Legendre functions

| Result | Check | Detail |
| --- | --- | --- |
| PASS | P00 equals 1 | actual=1 expected=1 abs_error=0 tolerance=1e-14 |
| PASS | P10 equals sin(phi) | actual=0.5 expected=0.5 abs_error=5.551e-17 tolerance=1e-14 |
| PASS | P11 equals cos(phi) | actual=0.866025403784 expected=0.866025403784 abs_error=0 tolerance=1e-14 |
| PASS | P20 equals (3 sin^2 - 1) / 2 | actual=-0.125 expected=-0.125 abs_error=5.551e-17 tolerance=1e-14 |
| PASS | P21 equals 3 sin cos | actual=1.29903810568 expected=1.29903810568 abs_error=2.22e-16 tolerance=1e-14 |
| PASS | P22 equals 3 cos^2 | actual=2.25 expected=2.25 abs_error=0 tolerance=1e-14 |
| PASS | the derivative of P20 equals 3 sin cos | actual=1.29903810568 expected=1.29903810568 abs_error=2.22e-16 tolerance=1e-13 |

## Acceleration against potential gradient

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the central-difference gradient of the potential matches the analytic acceleration on axis 0 | actual=-5.13511389494 expected=-5.13511388828 rel_error=1.297e-09 tolerance=1e-08 |
| PASS | the central-difference gradient of the potential matches the analytic acceleration on axis 1 | actual=-2.85296583548 expected=-2.85296583715 rel_error=5.837e-10 tolerance=1e-08 |
| PASS | the central-difference gradient of the potential matches the analytic acceleration on axis 2 | actual=-5.4920726344 expected=-5.49207263435 rel_error=9.315e-12 tolerance=1e-08 |

## Planetary models

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the EGM96 J2 coefficient is loaded | actual=0.00108262668355 expected=0.00108262668355 abs_error=0 tolerance=1e-12 |
| PASS | the EGM96 J3 coefficient is loaded | actual=-2.53265648533e-06 expected=-2.53265648533e-06 abs_error=0 tolerance=1e-12 |
| PASS | the EGM96 J4 coefficient is loaded | actual=-1.61962159137e-06 expected=-1.61962159137e-06 abs_error=0 tolerance=1e-12 |
| PASS | the LP165 lunar J2 coefficient is loaded | actual=0.00020335 expected=0.00020335 abs_error=0 tolerance=1e-12 |
| PASS | the Jupiter J2 coefficient is loaded | actual=0.014696572 expected=0.014696572 abs_error=0 tolerance=1e-12 |


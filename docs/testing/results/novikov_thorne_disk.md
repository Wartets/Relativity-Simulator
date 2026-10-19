# novikov_thorne_disk

Verifies the Novikov-Thorne thin disk: the ISCO radius for Schwarzschild, prograde and retrograde Kerr holes, the closed-form Page-Thorne function against numerical quadrature, the radiative efficiency, the normalization of the circular four-velocity, and the flux and temperature profile.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 11 |
| Failed checks | 0 |
| Duration | 0.3331 s |
| Generated | 2026-10-19 22:20:52 UTC |

## Innermost stable circular orbit

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the Schwarzschild ISCO equals 6 r_g | actual=8860.0181462 expected=8860.0181462 rel_error=0 tolerance=1e-12 |
| PASS | the prograde ISCO of a = 0.998 lies between 1 and 1.3 r_g | isco=1.23697 |
| PASS | the retrograde ISCO of a = -0.9 lies between 8 and 9.5 r_g | isco=8.71735 |

## Page-Thorne function

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the Page-Thorne function vanishes at the ISCO for every sampled spin |  |
| PASS | the closed form agrees with numerical quadrature within a relative 1e-7 | value=9.03299e-14 limit=1e-07 |

## Radiative efficiency

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the Schwarzschild efficiency equals 1 - sqrt(8/9) | actual=0.0571909584179 expected=0.0571909584179 abs_error=0 tolerance=1e-10 |
| PASS | the efficiency of a = 0.998 lies between 0.30 and 0.35 | eta=0.320994 |

## Circular four-velocity

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the circular four-velocity satisfies u.u = -1 across the disk | value=6.66134e-16 limit=1e-10 |

## Flux and temperature profile

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the flux and temperature vanish inside the ISCO |  |
| PASS | the flux and temperature are positive outside the ISCO |  |
| PASS | the flux peaks between one and three ISCO radii | peak_over_isco=1.45 |

## Measurements

| Section | Name | Value | Unit |
| --- | --- | --- | --- |
| Page-Thorne function | worst_closed_form_deviation | 9.03298917e-14 | dimensionless |


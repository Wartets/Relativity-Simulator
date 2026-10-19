# dark_matter_profiles

Verifies the analytic dark matter halo profiles: the NFW density and enclosed mass at the scale radius, the location of the peak circular velocity, the sign and direction of the acceleration, the Einasto and Burkert densities at the scale radius, and the Hernquist enclosed mass and potential.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 12 |
| Failed checks | 0 |
| Duration | 0.001221 s |
| Generated | 2026-10-19 22:19:17 UTC |

## Navarro-Frenk-White

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the density at the scale radius equals a quarter of the characteristic density | actual=2500000 expected=2500000 rel_error=0 tolerance=1e-12 |
| PASS | the enclosed mass at the scale radius equals 4 pi rho0 rs^3 (ln 2 - 1/2) | actual=194172724323 expected=194172724323 rel_error=0 tolerance=1e-12 |
| PASS | the circular velocity at the reported radius is a local maximum |  |
| PASS | the acceleration points toward the halo centre |  |
| PASS | the acceleration has no transverse component on the x axis | value=0 limit=0.000485432 |

## Einasto

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the density at the scale radius equals the characteristic density | actual=10000000 expected=10000000 rel_error=0 tolerance=1e-12 |
| PASS | the enclosed mass is positive |  |
| PASS | the circular velocity is positive |  |

## Burkert

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the density at the scale radius equals a quarter of the characteristic density | actual=2500000 expected=2500000 rel_error=0 tolerance=1e-12 |
| PASS | the enclosed mass is positive |  |

## Hernquist

| Result | Check | Detail |
| --- | --- | --- |
| PASS | a quarter of the mass is enclosed within the scale radius | actual=25000000000 expected=25000000000 rel_error=0 tolerance=1e-14 |
| PASS | the potential at the scale radius equals -G M / (2 a) | actual=-2500000000 expected=-2500000000 rel_error=0 tolerance=1e-14 |


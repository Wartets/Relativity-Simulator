# observer_tetrad

Verifies observer tetrads and their transport: orthonormality of stationary and ZAMO tetrads, the Thomas precession of a circularly moving gyroscope, the Gravity Probe B geodetic precession, the Lense-Thirring precession of a static observer in the Kerr field, and the reduction of Fermi-Walker to parallel transport for geodesic observers.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 11 |
| Failed checks | 0 |
| Duration | 5.309 s |
| Generated | 2026-10-19 22:20:58 UTC |

## Tetrad orthonormality

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the stationary Schwarzschild tetrad is orthonormal within 1e-12 |  |
| PASS | the ZAMO Schwarzschild tetrad is orthonormal within 1e-12 |  |
| PASS | the ZAMO Kerr tetrad is orthonormal within 1e-10 |  |

## Thomas precession

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the gyroscope rotation after one orbit equals -2 pi (gamma - 1) within a relative 1e-4 | actual=-1.57079632679 expected=-1.57079632679 rel_error=1.483e-13 tolerance=0.0001 |
| PASS | the transported tetrad stays orthonormal within 1e-10 | value=6.66134e-16 limit=1e-10 |

## Gravity Probe B geodetic precession

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the theoretical geodetic rate lies between 6.55 and 6.65 arcsec per year |  |
| PASS | the transported gyroscope reproduces the geodetic rate within 1e-4 arcsec per year | actual=6.62101648404 expected=6.62100619022 abs_error=1.029e-05 tolerance=0.0001 |
| PASS | the transported tetrad residual stays below 1e-10 | value=2.79776e-14 limit=1e-10 |

## Lense-Thirring frame dragging

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the gyroscope rate of a static observer matches the frame-dragging prediction within a relative 1e-4 | actual=0.000302359613483 expected=0.000302359613483 rel_error=3.819e-14 tolerance=0.0001 |
| PASS | the transported tetrad residual stays below 1e-9 | value=1.9984e-15 limit=1e-09 |

## Fermi-Walker against parallel transport

| Result | Check | Detail |
| --- | --- | --- |
| PASS | a vanishing acceleration reduces Fermi-Walker transport to parallel transport | value=0 limit=1e-15 |

## Measurements

| Section | Name | Value | Unit |
| --- | --- | --- | --- |
| Thomas precession | thomas_angle | -1.57079633 | rad |
| Gravity Probe B geodetic precession | theoretical_annual_precession | 6.62100619 | arcsec/year |
| Gravity Probe B geodetic precession | measured_annual_precession | 6.62101648 | arcsec/year |
| Lense-Thirring frame dragging | expected_gyroscope_rate | 0.000302359613 | rad per unit time |
| Lense-Thirring frame dragging | measured_gyroscope_rate | 0.000302359613 | rad per unit time |


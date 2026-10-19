# shapiro_delay

Verifies the Shapiro time delay of a radio signal grazing a Schwarzschild mass between r = 50000 and r = 75000 with closest approach 1000: the coordinate flight time integrated with the sixth-order Gauss-Legendre scheme agrees with an independent Gauss-Legendre quadrature of the exact geodesic equation within a relative 1e-11.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 3 |
| Failed checks | 0 |
| Duration | 4.437 s |
| Generated | 2026-10-19 22:21:16 UTC |

## Geodesic integration

| Result | Check | Detail |
| --- | --- | --- |
| PASS | every implicit step converges |  |
| PASS | the signal reaches the probe radius | actual=75000 expected=75000 abs_error=7.713e-10 tolerance=1e-06 |

## Reference quadrature

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the integrated coordinate flight time agrees with the exact quadrature within a relative 1e-11 | actual=125004.549708 expected=125004.549708 rel_error=1.409e-14 tolerance=1e-11 |

## Measurements

| Section | Name | Value | Unit |
| --- | --- | --- | --- |
| Reference quadrature | theoretical_one_way_delay | 21.1982725 | GM/c^3 |
| Reference quadrature | numerical_one_way_delay | 21.2176708 | GM/c^3 |
| Reference quadrature | theoretical_round_trip_delay | 42.4632219 | GM/c^3 |
| Reference quadrature | numerical_round_trip_delay | 42.4353416 | GM/c^3 |
| Reference quadrature | exact_flight_time | 125004.55 | GM/c^3 |
| Reference quadrature | integrated_flight_time | 125004.55 | GM/c^3 |


# solar_deflection

Verifies the gravitational deflection of a light ray grazing the solar limb, integrated with the adaptive Dormand-Prince scheme in SI units from -1000 to +1000 solar radii: the deflection agrees with the first-order result 4GM/(c^2 R) within a relative 1e-4 and with the second-order post-Newtonian result within a relative 1e-5, while the null constraint residual stays below 1e-11.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 5 |
| Failed checks | 0 |
| Duration | 0.04845 s |
| Generated | 2026-10-19 22:21:16 UTC |

## Integration across the solar field

| Result | Check | Detail |
| --- | --- | --- |
| PASS | every adaptive step stays above the minimum step size |  |
| PASS | the ray reaches the far boundary of the integration domain |  |

## Deflection angle

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the deflection matches 4GM/(c^2 R) within a relative 1e-4 | actual=1.75123921758 expected=1.75124328137 rel_error=2.321e-06 tolerance=0.0001 |
| PASS | the deflection matches the second-order post-Newtonian result within a relative 1e-5 | actual=1.75123921758 expected=1.75125051209 rel_error=6.449e-06 tolerance=1e-05 |

## Null constraint

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the null constraint residual stays below 1e-11 along the whole ray | value=7.01497e-12 limit=1e-11 |

## Measurements

| Section | Name | Value | Unit |
| --- | --- | --- | --- |
| Deflection angle | steps_completed | 600 | steps |
| Deflection angle | steps_rejected | 8 | steps |
| Deflection angle | computed_deflection | 1.75123922 | arcsec |
| Deflection angle | first_order_deflection | 1.75124328 | arcsec |
| Deflection angle | second_order_deflection | 1.75125051 | arcsec |
| Null constraint | max_invariant_residual | 7.01497194e-12 | dimensionless |


# frame_transforms

Verifies the reference frame transformations: the ICRF to ecliptic round trip reproduces the original state, the identity Lorentz boost is the identity matrix, and the boost along x has the Lorentz factor on the diagonal and its negative product with the velocity off the diagonal.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 10 |
| Failed checks | 0 |
| Duration | 0.001119 s |
| Generated | 2026-10-19 22:19:24 UTC |

## ICRF and ecliptic round trip

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the round trip restores position component 1 | actual=100000000000 expected=100000000000 rel_error=0 tolerance=1e-12 |
| PASS | the round trip restores velocity component 1 | actual=1000 expected=1000 rel_error=0 tolerance=1e-12 |
| PASS | the round trip restores position component 2 | actual=200000000000 expected=200000000000 rel_error=0 tolerance=1e-12 |
| PASS | the round trip restores velocity component 2 | actual=2000 expected=2000 rel_error=1.137e-16 tolerance=1e-12 |
| PASS | the round trip restores position component 3 | actual=300000000000 expected=300000000000 rel_error=0 tolerance=1e-12 |
| PASS | the round trip restores velocity component 3 | actual=3000 expected=3000 rel_error=0 tolerance=1e-12 |

## Lorentz boost matrices

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the zero boost has a unit time-time component | actual=1 expected=1 abs_error=0 tolerance=1e-15 |
| PASS | the zero boost has a unit x-x component | actual=1 expected=1 abs_error=0 tolerance=1e-15 |
| PASS | the time-time component equals the Lorentz factor | actual=1.25 expected=1.25 abs_error=0 tolerance=1e-12 |
| PASS | the time-x component equals minus gamma times beta | actual=-0.75 expected=-0.75 abs_error=0 tolerance=1e-12 |


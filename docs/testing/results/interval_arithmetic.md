# interval_arithmetic

Verifies the interval arithmetic container: enclosure bounds of the four arithmetic operations, scalar scaling, squaring across zero, square root and the sine of an interval.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 16 |
| Failed checks | 0 |
| Duration | 0.001229 s |
| Generated | 2026-10-19 22:20:26 UTC |

## Arithmetic operations

| Result | Check | Detail |
| --- | --- | --- |
| PASS | sum [2,4]+[1,3] lower bound | actual=3 expected=3 abs_error=0 tolerance=1e-12 |
| PASS | sum [2,4]+[1,3] upper bound | actual=7 expected=7 abs_error=0 tolerance=1e-12 |
| PASS | difference [2,4]-[1,3] lower bound | actual=-1 expected=-1 abs_error=0 tolerance=1e-12 |
| PASS | difference [2,4]-[1,3] upper bound | actual=3 expected=3 abs_error=0 tolerance=1e-12 |
| PASS | product [2,4]*[1,3] lower bound | actual=2 expected=2 abs_error=0 tolerance=1e-12 |
| PASS | product [2,4]*[1,3] upper bound | actual=12 expected=12 abs_error=0 tolerance=1e-12 |
| PASS | quotient [2,4]/[1,3] lower bound | actual=0.666666666667 expected=0.666666666667 abs_error=0 tolerance=1e-12 |
| PASS | quotient [2,4]/[1,3] upper bound | actual=4 expected=4 abs_error=0 tolerance=1e-12 |
| PASS | positive scalar scaling lower bound | actual=5 expected=5 abs_error=0 tolerance=1e-12 |
| PASS | positive scalar scaling upper bound | actual=10 expected=10 abs_error=0 tolerance=1e-12 |

## Elementary functions

| Result | Check | Detail |
| --- | --- | --- |
| PASS | squaring an interval that contains zero has a zero lower bound |  |
| PASS | squaring an interval that contains zero has the largest squared endpoint as upper bound | actual=9 expected=9 abs_error=0 tolerance=1e-12 |
| PASS | square root of [4,16] lower bound | actual=2 expected=2 abs_error=0 tolerance=1e-12 |
| PASS | square root of [4,16] upper bound | actual=4 expected=4 abs_error=0 tolerance=1e-12 |
| PASS | the sine of [0, pi] is non-negative |  |
| PASS | the sine of [0, pi] reaches one | actual=1 expected=1 abs_error=0 tolerance=1e-12 |


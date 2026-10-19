# high_order_integrators

Verifies the embedded Cash-Karp 5(4) and Vernier 9(8) integrators on a circular timelike Schwarzschild orbit: every adaptive step succeeds and the orbital radius remains constant to within the stated tolerance.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 6 |
| Failed checks | 0 |
| Duration | 0.003548 s |
| Generated | 2026-10-19 22:19:27 UTC |

## Cash-Karp 5(4)

| Result | Check | Detail |
| --- | --- | --- |
| PASS | every adaptive step over 50 units of proper time succeeds |  |
| PASS | the integrator accepts at least one step |  |
| PASS | the orbital radius stays at 12 within 1e-6 | actual=12 expected=12 abs_error=2.132e-14 tolerance=1e-06 |

## Vernier 9(8)

| Result | Check | Detail |
| --- | --- | --- |
| PASS | every adaptive step over 100 units of proper time succeeds |  |
| PASS | the integrator accepts at least one step |  |
| PASS | the orbital radius stays at 15 within 1e-7 | actual=15 expected=15 abs_error=3.553e-15 tolerance=1e-07 |


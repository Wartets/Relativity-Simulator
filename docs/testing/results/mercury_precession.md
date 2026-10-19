# mercury_precession

Verifies the perihelion advance of a Mercury-like orbit (a = 1000 r_s, e = 0.20563) integrated for one hundred revolutions with the sixth-order Gauss-Legendre scheme: the accumulated precession matches the exact elliptic-integral result of the Schwarzschild geodesic within a relative 1e-8.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 4 |
| Failed checks | 0 |
| Duration | 17.47 s |
| Generated | 2026-10-19 22:20:50 UTC |

## Exact precession

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the exact precession per revolution is positive |  |

## Numerical integration

| Result | Check | Detail |
| --- | --- | --- |
| PASS | every implicit step converges |  |
| PASS | one hundred perihelion passages are detected | count=100 |

## Comparison

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the accumulated precession matches the exact elliptic-integral result within a relative 1e-8 | actual=0.986412196146 expected=0.986412187728 rel_error=8.534e-09 tolerance=1e-08 |

## Measurements

| Section | Name | Value | Unit |
| --- | --- | --- | --- |
| Comparison | theoretical_precession | 0.986412188 | rad |
| Comparison | numerical_precession | 0.986412196 | rad |


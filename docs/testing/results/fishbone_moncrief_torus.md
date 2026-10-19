# fishbone_moncrief_torus

Verifies the Fishbone-Moncrief torus: the enthalpy and density at the pressure maximum and at the cusp, the exclusion of the region inside the inner edge, the normalization of the fluid four-velocity, the polytropic and enthalpy relations and the Cartesian evaluation.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 13 |
| Failed checks | 0 |
| Duration | 0.001953 s |
| Generated | 2026-10-19 22:19:23 UTC |

## Equilibrium and cusp

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the specific angular momentum is positive |  |
| PASS | the potential at the inner edge exceeds that at the centre |  |
| PASS | the maximum enthalpy exceeds 1 |  |
| PASS | the enthalpy at the pressure maximum equals the maximum enthalpy | actual=1.05282124906 expected=1.05282124906 abs_error=0 tolerance=1e-12 |
| PASS | the density at the pressure maximum equals the configured maximum | actual=1000 expected=1000 rel_error=4.547e-16 tolerance=1e-10 |
| PASS | the enthalpy at the cusp equals 1 | actual=1 expected=1 abs_error=0 tolerance=1e-12 |
| PASS | the region inside the inner edge is empty |  |
| PASS | the pressure maximum lies inside the torus with positive density and pressure |  |

## Four-velocity normalization

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the fluid four-velocity satisfies u.u = -1 inside the torus | value=4.44089e-16 limit=1e-10 |

## Polytropic relations

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the pressure follows P = K rho^Gamma | value=0 limit=1e-10 |
| PASS | the enthalpy follows h = 1 + Gamma / (Gamma - 1) P / rho | value=0 limit=1e-10 |

## Cartesian evaluation

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the Cartesian evaluation at the pressure maximum returns the peak density and a positive pressure |  |
| PASS | the Cartesian evaluation inside the cusp returns a negligible density |  |


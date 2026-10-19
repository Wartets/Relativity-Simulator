# carter_constant

Verifies the conservation of the Kerr constants of motion along a timelike geodesic: the energy, the axial angular momentum and the Carter constant of an inclined orbit around a hole of spin 0.85 drift by less than a relative 1e-12 over twenty thousand adaptive steps.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 7 |
| Failed checks | 0 |
| Duration | 4.258 s |
| Generated | 2026-10-19 22:16:36 UTC |

## Initial conditions

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the timelike normalization admits a real temporal velocity |  |
| PASS | the Carter constant of the inclined orbit is positive |  |
| PASS | the conserved energy is positive |  |

## Conservation over twenty thousand steps

| Result | Check | Detail |
| --- | --- | --- |
| PASS | every adaptive step succeeds |  |
| PASS | the Carter constant drifts by less than a relative 1e-12 | value=1.84014e-13 limit=1e-12 |
| PASS | the energy drifts by less than a relative 1e-12 | value=8.96344e-14 limit=1e-12 |
| PASS | the axial angular momentum drifts by less than a relative 1e-12 | value=2.1844e-13 limit=1e-12 |

## Measurements

| Section | Name | Value | Unit |
| --- | --- | --- | --- |
| Conservation over twenty thousand steps | carter_constant_drift | 1.84014468e-13 | relative |
| Conservation over twenty thousand steps | energy_drift | 8.96344398e-14 | relative |
| Conservation over twenty thousand steps | angular_momentum_drift | 2.18440439e-13 | relative |


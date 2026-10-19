# pn_radiation_reaction

Verifies the 2.5PN radiation reaction of an equal-mass binary: the total energy decreases monotonically in aggregate and the quadrupole radiated power is positive.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 3 |
| Failed checks | 0 |
| Duration | 0.1313 s |
| Generated | 2026-10-19 22:20:59 UTC |

## Radiative energy loss

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the final total energy is finite | value=-0.0270684903714 |
| PASS | the total energy after 2000 steps is lower than the initial energy | initial=-0.0266666666667 final=-0.0270684903714 |
| PASS | the quadrupole radiated power is positive |  |

## Measurements

| Section | Name | Value | Unit |
| --- | --- | --- | --- |
| Radiative energy loss | initial_energy | -0.0266666667 | simulation energy |
| Radiative energy loss | final_energy | -0.0270684904 | simulation energy |
| Radiative energy loss | radiated_power | 1.97396309e-05 | simulation power |


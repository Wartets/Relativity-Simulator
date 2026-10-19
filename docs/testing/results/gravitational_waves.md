# gravitational_waves

Verifies the quadrupole gravitational wave strain of a binary neutron star system at face-on inclination: the plus polarization dominates at orbital phase zero, the cross polarization dominates at a quarter-turn phase, and each suppressed polarization vanishes relative to the dominant one.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 4 |
| Failed checks | 0 |
| Duration | 0.0008358 s |
| Generated | 2026-10-19 22:19:26 UTC |

## Orbital phase zero

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the plus polarization is non-zero at phase zero |  |
| PASS | the cross polarization vanishes relative to the plus polarization at phase zero | value=0 limit=1.70955e-30 |

## Orbital phase of a quarter of a half turn

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the cross polarization is non-zero at phase pi/4 |  |
| PASS | the plus polarization vanishes relative to the cross polarization at phase pi/4 | value=1.0468e-37 limit=1.70955e-30 |

## Measurements

| Section | Name | Value | Unit |
| --- | --- | --- | --- |
| Orbital phase zero | h_plus_phase_zero | 1.70955385e-21 | dimensionless strain |
| Orbital phase of a quarter of a half turn | h_cross_phase_quarter | 1.70955385e-21 | dimensionless strain |


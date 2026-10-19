# galaxy_collision_nbody

Verifies a deterministic collision of two disk-bulge-halo galaxies of 500 particles each evolved for fifteen Barnes-Hut leapfrog steps: the centre of mass drifts by less than 1e19 m and the magnitude of the total angular momentum changes by less than 5 percent.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 5 |
| Failed checks | 0 |
| Duration | 1.477 s |
| Generated | 2026-10-19 22:19:26 UTC |

## Initial conditions

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the two galaxies provide 1000 particles | count=1000 |

## Collision evolution

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the centre of mass drift is finite | value=5.81785512571e+18 |
| PASS | the angular momentum change is finite | value=5.02582327856e-05 |
| PASS | the centre of mass drifts by less than 1e19 m | value=5.81786e+18 limit=1e+19 |
| PASS | the total angular momentum changes by less than 5 percent | value=5.02582e-05 limit=0.05 |

## Measurements

| Section | Name | Value | Unit |
| --- | --- | --- | --- |
| Collision evolution | centre_of_mass_drift | 5.81785513e+18 | m |
| Collision evolution | angular_momentum_change | 5.02582328e-05 | relative |


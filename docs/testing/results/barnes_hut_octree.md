# barnes_hut_octree

Verifies the Barnes-Hut octree on a deterministic Gaussian cloud of 300 softened particles: the tree contains more nodes than particles, the quadrupole-corrected accelerations of twenty particles agree with direct summation within 5 percent of the acceleration magnitude, and ten leapfrog steps keep every particle finite.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 4 |
| Failed checks | 0 |
| Duration | 0.1903 s |
| Generated | 2026-10-19 22:15:08 UTC |

## Tree construction

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the octree holds more nodes than particles | nodes=447 |

## Accuracy against direct summation

| Result | Check | Detail |
| --- | --- | --- |
| PASS | every sampled particle has a non-negligible direct acceleration | compared=20 |
| PASS | the tree acceleration agrees with direct summation within 5 percent of the magnitude | value=0.0454623 limit=0.05 |

## Leapfrog integration

| Result | Check | Detail |
| --- | --- | --- |
| PASS | every position and velocity stays finite after ten leapfrog steps |  |

## Measurements

| Section | Name | Value | Unit |
| --- | --- | --- | --- |
| Accuracy against direct summation | worst_relative_acceleration_error | 0.0454622749 | dimensionless |


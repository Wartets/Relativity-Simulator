# kerr_schild_geodesics

Verifies timelike geodesics in horizon-regular Kerr-Schild coordinates: an orbit crossing the polar axis stays finite for two thousand adaptive steps, and a radial infall crosses the outer horizon without the integration failing.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 6 |
| Failed checks | 0 |
| Duration | 1.121 s |
| Generated | 2026-10-19 22:20:31 UTC |

## Polar axis crossing

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the timelike normalization admits a real temporal velocity near the axis |  |
| PASS | every adaptive step of the polar trajectory succeeds |  |
| PASS | the Cartesian coordinates stay finite while the orbit crosses the polar axis |  |

## Horizon infall

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the timelike normalization admits a real temporal velocity for the radial infall |  |
| PASS | every adaptive step of the infall succeeds |  |
| PASS | the infalling particle crosses the outer horizon in Kerr-Schild coordinates |  |


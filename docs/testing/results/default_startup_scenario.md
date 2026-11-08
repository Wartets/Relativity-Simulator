# default_startup_scenario

Verifies the startup path: the initial camera pose, the resolution of the built-in startup scenario and the fallback rules for empty, missing and valid configurations, the load through the command queue, and the preservation of the scenario and of its camera pose across a view reset, a full reset and a failed load.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 24 |
| Failed checks | 0 |
| Duration | 0.01566 s |
| Generated | 2026-11-08 00:16:36 UTC |

## Initial state

| Result | Check | Detail |
| --- | --- | --- |
| PASS | initial camera: the camera is at (-5, 37, -7) |  |
| PASS | initial camera: the camera attitude is pitch 0, yaw 130, roll 0 |  |
| PASS | the initial spherical radius is synchronized with the position | actual=37.9868398264 expected=37.9868398264 abs_error=0 tolerance=1e-09 |
| PASS | no scenario path exists before the startup load |  |

## Scenario resolution

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the built-in startup scenario is reachable |  |
| PASS | the portable path of the built-in scenario is the canonical one |  |
| PASS | an empty configuration falls back to the built-in scenario |  |
| PASS | a missing configuration falls back to the built-in scenario |  |
| PASS | a valid configuration is kept as configured |  |

## Startup load

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the load command is accepted by the queue |  |
| PASS | the startup scenario loads successfully |  |
| PASS | the active scenario name matches the startup scenario |  |
| PASS | the active scenario path matches the startup scenario |  |

## Resets

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the camera reset command is accepted |  |
| PASS | camera after the view reset: the camera position matches the startup scenario |  |
| PASS | camera after the view reset: the camera attitude matches the startup scenario |  |
| PASS | the reset command is accepted |  |
| PASS | the reset restores the scenario parameters |  |
| PASS | the reset keeps the active scenario |  |
| PASS | the reset pauses the scheduler |  |
| PASS | camera after the full reset: the camera position matches the startup scenario |  |
| PASS | camera after the full reset: the camera attitude matches the startup scenario |  |

## Failed load

| Result | Check | Detail |
| --- | --- | --- |
| PASS | loading a missing scenario fails |  |
| PASS | a failed load keeps the active scenario |  |


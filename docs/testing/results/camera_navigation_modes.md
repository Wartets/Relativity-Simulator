# camera_navigation_modes

Verifies every camera navigation mode of the engine: mode catalogue and command-queue selection, free-fly translation and rotation limits, orientation basis, rocket-mode integration under pause, clamp and non-finite conditions, pose preservation across mode switches, and the interactive controller snap, look-at and surface-walk helpers.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 56 |
| Failed checks | 0 |
| Duration | 0.005744 s |
| Generated | 2026-10-19 22:16:31 UTC |

## Navigation mode catalogue

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the engine exposes exactly six navigation modes |  |
| PASS | Planet Orbit is navigation mode 5 |  |
| PASS | mode 0 (Free Fly 6-DOF) can be selected through the command queue |  |
| PASS | mode 1 (Orbit Center) can be selected through the command queue |  |
| PASS | mode 2 (Spherical) can be selected through the command queue |  |
| PASS | mode 3 (Rocket 6-DOF Thrust) can be selected through the command queue |  |
| PASS | mode 4 (Surface Walk) can be selected through the command queue |  |
| PASS | mode 5 (Planet Orbit) can be selected through the command queue |  |
| PASS | cycling with the interface formula visits every mode and wraps to the first |  |

## Free-fly translation and rotation commands

| Result | Check | Detail |
| --- | --- | --- |
| PASS | finite translation is accepted |  |
| PASS | translation updates x | actual=-4 expected=-4 abs_error=0 tolerance=1e-12 |
| PASS | translation updates y | actual=39 expected=39 abs_error=0 tolerance=1e-12 |
| PASS | translation updates z | actual=-4 expected=-4 abs_error=0 tolerance=1e-12 |
| PASS | spherical radius is synchronised after translation | actual=39.4081209905 expected=39.4081209905 abs_error=0 tolerance=1e-09 |
| PASS | spherical polar angle is synchronised after translation | actual=1.67247335087 expected=1.67247335087 abs_error=0 tolerance=1e-09 |
| PASS | spherical azimuth is synchronised after translation | actual=1.6730030449 expected=1.6730030449 abs_error=0 tolerance=1e-09 |
| PASS | non-finite translation is rejected and leaves the camera untouched |  |
| PASS | positive translation is clamped to the position limit | actual=1e+12 expected=1e+12 abs_error=0 tolerance=0.001 |
| PASS | negative translation is clamped to the position limit | actual=-1e+12 expected=-1e+12 abs_error=0 tolerance=0.001 |
| PASS | pitch is clamped to 89 degrees | actual=89 expected=89 abs_error=0 tolerance=1e-12 |
| PASS | yaw and roll are wrapped to a half turn |  |
| PASS | negative pitch is clamped to -89 degrees | actual=-89 expected=-89 abs_error=0 tolerance=1e-12 |
| PASS | non-finite rotation is rejected |  |
| PASS | field of view is clamped to 175 degrees | actual=175 expected=175 abs_error=0 tolerance=1e-12 |
| PASS | field of view is clamped to 5 degrees | actual=5 expected=5 abs_error=0 tolerance=1e-12 |
| PASS | navigation speed keeps a positive minimum | actual=0.001 expected=0.001 abs_error=0 tolerance=1e-12 |

## Orientation basis

| Result | Check | Detail |
| --- | --- | --- |
| PASS | forward, right and up form an orthonormal frame for every sampled attitude |  |
| PASS | the frame keeps a constant handedness for every sampled attitude |  |

## Rocket mode integration and inertial modes

| Result | Check | Detail |
| --- | --- | --- |
| PASS | rocket mode integrates velocity over simulated time | actual=0.5 expected=0.5 abs_error=1.776e-15 tolerance=1e-09 |
| PASS | rocket mode leaves the transverse position untouched | actual=37 expected=37 abs_error=0 tolerance=1e-12 |
| PASS | rocket mode keeps spherical coordinates synchronised | actual=37.9242666376 expected=37.9242666376 abs_error=0 tolerance=1e-09 |
| PASS | rocket mode does not move while the simulation clock is paused |  |
| PASS | rocket mode clamps runaway positions to the position limit |  |
| PASS | rocket mode zeroes a non-finite velocity |  |
| PASS | Free Fly 6-DOF: residual velocity does not drift the camera in non-thrust modes |  |
| PASS | Orbit Center: residual velocity does not drift the camera in non-thrust modes |  |
| PASS | Spherical: residual velocity does not drift the camera in non-thrust modes |  |
| PASS | Surface Walk: residual velocity does not drift the camera in non-thrust modes |  |
| PASS | Planet Orbit: residual velocity does not drift the camera in non-thrust modes |  |

## Pose preservation across mode switches

| Result | Check | Detail |
| --- | --- | --- |
| PASS | switching to Free Fly 6-DOF does not teleport the camera |  |
| PASS | switching to Orbit Center does not teleport the camera |  |
| PASS | switching to Spherical does not teleport the camera |  |
| PASS | switching to Rocket 6-DOF Thrust does not teleport the camera |  |
| PASS | switching to Surface Walk does not teleport the camera |  |
| PASS | switching to Planet Orbit does not teleport the camera |  |
| PASS | camera reset restores the home position |  |
| PASS | camera reset clears the velocity |  |

## Interactive controller helpers

| Result | Check | Detail |
| --- | --- | --- |
| PASS | equatorial snap places the camera at the requested radius | actual=50 expected=50 abs_error=0 tolerance=1e-06 |
| PASS | equatorial snap places the camera in the equatorial plane | actual=6.12323399574e-17 expected=0 abs_error=6.123e-17 tolerance=1e-06 |
| PASS | polar snap places the camera on the positive polar axis |  |
| PASS | polar snap keeps the requested distance | actual=50.00000001 expected=50 abs_error=1e-08 tolerance=0.01 |
| PASS | ISCO snap produces a finite positive radius |  |
| PASS | look-at aligns the forward axis with the target |  |
| PASS | entering surface walk switches to the surface walk mode |  |
| PASS | the surface walker is never placed inside the body |  |
| PASS | leaving surface walk returns to free fly |  |


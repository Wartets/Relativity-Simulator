# ui_multi_window

Verifies the terminal command path and the interactive camera controller that the multi-window interface relies on: parameter, camera, metric, integrator, render and preset commands applied through the REPL, controller speed and sensitivity accessors, scroll zoom, navigation mode switching and the snap helpers.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 33 |
| Failed checks | 0 |
| Duration | 0.003633 s |
| Generated | 2026-10-19 22:21:21 UTC |

## Parameter commands

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the mass command is accepted |  |
| PASS | the mass is applied | actual=5 expected=5 abs_error=0 tolerance=1e-12 |
| PASS | the spin command is accepted |  |
| PASS | the spin is applied | actual=0.94 expected=0.94 abs_error=0 tolerance=1e-12 |

## Camera commands

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the field of view command is accepted |  |
| PASS | the field of view is applied | actual=45 expected=45 abs_error=0 tolerance=1e-12 |
| PASS | the speed command is accepted |  |
| PASS | the speed is applied | actual=25 expected=25 abs_error=0 tolerance=1e-12 |
| PASS | the move command is accepted |  |
| PASS | the x translation is applied relative to the previous position | actual=5 expected=5 abs_error=0 tolerance=1e-12 |
| PASS | the y translation is applied relative to the previous position | actual=32 expected=32 abs_error=0 tolerance=1e-12 |
| PASS | the z translation is applied relative to the previous position | actual=-5 expected=-5 abs_error=0 tolerance=1e-12 |

## Metric, integrator and render commands

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the metric name is stored |  |
| PASS | the integrator name is stored |  |
| PASS | the render scale command is accepted |  |
| PASS | the render scale is applied | actual=1.5 expected=1.5 abs_error=0 tolerance=1e-12 |
| PASS | the ray step budget is applied |  |
| PASS | editing a render parameter selects the custom preset |  |
| PASS | the preset command is accepted |  |
| PASS | the preset index is applied |  |
| PASS | the Performance preset applies its documented resolution scale of 0.5 | actual=0.5 expected=0.5 abs_error=0 tolerance=1e-12 |
| PASS | the camera mode is applied |  |

## Interactive camera controller

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the move speed accessor returns the stored value | actual=15 expected=15 abs_error=0 tolerance=1e-12 |
| PASS | the mouse sensitivity accessor returns the stored value | actual=0.25 expected=0.25 abs_error=0 tolerance=1e-12 |
| PASS | a scroll step changes the field of view | before=45 after=47.5 |
| PASS | the orbit navigation mode is selected |  |
| PASS | the spherical navigation mode is selected |  |

## Snap helpers

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the equatorial snap places the camera at the requested radius | actual=60 expected=60 abs_error=0 tolerance=1e-06 |
| PASS | the equatorial snap places the camera in the equatorial plane | actual=6.12323399574e-17 expected=0 abs_error=6.123e-17 tolerance=1e-06 |
| PASS | the polar snap places the camera on the positive polar axis |  |
| PASS | the polar snap keeps the requested distance | actual=40.0000000125 expected=40 abs_error=1.25e-08 tolerance=0.01 |
| PASS | the ISCO snap produces a finite positive radius |  |
| PASS | the photon sphere snap produces a finite positive radius |  |


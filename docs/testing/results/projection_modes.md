# projection_modes

Verifies every camera projection mode of the observer: unit-length finite rays over several fields of view, renderer pixel grids, forward centre ray, mirror symmetry, monotonic angular mapping, pinhole edge geometry and the round trip between ray generation and screen projection.

| Property | Value |
| --- | --- |
| Status | FAILED |
| Checks | 68 |
| Failed checks | 8 |
| Duration | 0.03821 s |
| Generated | 2026-10-19 12:45:48 UTC |

## Mode catalogue

| Result | Check | Detail |
| --- | --- | --- |
| PASS | Pinhole is mode 0 |  |
| PASS | Equirectangular 360 is mode 3 |  |
| PASS | Hammer-Aitoff is mode 7 |  |

## Projection mode 0: Pinhole

| Result | Check | Detail |
| --- | --- | --- |
| PASS | Pinhole: sampled rays are finite and unit length | samples=324 invalid=0 worst_deviation=2.22e-16 |
| PASS | Pinhole: every pixel of a 96x54 renderer grid yields a valid ray | invalid=0 |
| PASS | Pinhole: centre pixel looks straight along the forward axis |  |
| PASS | Pinhole: left-right mirror symmetry | value=0 limit=1e-09 |
| PASS | Pinhole: up-down mirror symmetry | value=0 limit=1e-09 |
| PASS | Pinhole: off-axis angle grows monotonically along the horizontal axis |  |
| FAIL | Pinhole: direction-to-screen projection inverts the ray generator | missing=0 worst_error=1.2 |

## Projection mode 1: Auto-Zoom

| Result | Check | Detail |
| --- | --- | --- |
| PASS | Auto-Zoom: sampled rays are finite and unit length | samples=324 invalid=0 worst_deviation=2.22e-16 |
| PASS | Auto-Zoom: every pixel of a 96x54 renderer grid yields a valid ray | invalid=0 |
| PASS | Auto-Zoom: centre pixel looks straight along the forward axis |  |
| PASS | Auto-Zoom: left-right mirror symmetry | value=0 limit=1e-09 |
| PASS | Auto-Zoom: up-down mirror symmetry | value=0 limit=1e-09 |
| PASS | Auto-Zoom: off-axis angle grows monotonically along the horizontal axis |  |
| FAIL | Auto-Zoom: direction-to-screen projection inverts the ray generator | missing=0 worst_error=1.2 |

## Projection mode 2: Fisheye Stereographic

| Result | Check | Detail |
| --- | --- | --- |
| PASS | Fisheye Stereographic: sampled rays are finite and unit length | samples=324 invalid=0 worst_deviation=2.22e-16 |
| PASS | Fisheye Stereographic: every pixel of a 96x54 renderer grid yields a valid ray | invalid=0 |
| PASS | Fisheye Stereographic: centre pixel looks straight along the forward axis |  |
| PASS | Fisheye Stereographic: left-right mirror symmetry | value=0 limit=1e-09 |
| PASS | Fisheye Stereographic: up-down mirror symmetry | value=0 limit=1e-09 |
| PASS | Fisheye Stereographic: off-axis angle grows monotonically along the horizontal axis |  |
| FAIL | Fisheye Stereographic: direction-to-screen projection inverts the ray generator | missing=0 worst_error=1.2 |

## Projection mode 3: Equirectangular 360

| Result | Check | Detail |
| --- | --- | --- |
| PASS | Equirectangular 360: sampled rays are finite and unit length | samples=324 invalid=0 worst_deviation=2.22e-16 |
| PASS | Equirectangular 360: every pixel of a 96x54 renderer grid yields a valid ray | invalid=0 |
| PASS | Equirectangular 360: centre pixel looks straight along the forward axis |  |
| PASS | Equirectangular 360: left-right mirror symmetry | value=0 limit=1e-09 |
| PASS | Equirectangular 360: up-down mirror symmetry | value=3.33067e-16 limit=1e-09 |
| PASS | Equirectangular 360: off-axis angle grows monotonically along the horizontal axis |  |
| FAIL | Equirectangular 360: direction-to-screen projection inverts the ray generator | missing=0 worst_error=1.2 |

## Projection mode 4: Fisheye Equidistant

| Result | Check | Detail |
| --- | --- | --- |
| PASS | Fisheye Equidistant: sampled rays are finite and unit length | samples=324 invalid=0 worst_deviation=2.22e-16 |
| PASS | Fisheye Equidistant: every pixel of a 96x54 renderer grid yields a valid ray | invalid=0 |
| PASS | Fisheye Equidistant: centre pixel looks straight along the forward axis |  |
| PASS | Fisheye Equidistant: left-right mirror symmetry | value=0 limit=1e-09 |
| PASS | Fisheye Equidistant: up-down mirror symmetry | value=0 limit=1e-09 |
| PASS | Fisheye Equidistant: off-axis angle grows monotonically along the horizontal axis |  |
| FAIL | Fisheye Equidistant: direction-to-screen projection inverts the ray generator | missing=0 worst_error=1.2 |

## Projection mode 5: Fisheye Orthographic

| Result | Check | Detail |
| --- | --- | --- |
| PASS | Fisheye Orthographic: sampled rays are finite and unit length | samples=324 invalid=0 worst_deviation=2.22e-16 |
| PASS | Fisheye Orthographic: every pixel of a 96x54 renderer grid yields a valid ray | invalid=0 |
| PASS | Fisheye Orthographic: centre pixel looks straight along the forward axis |  |
| PASS | Fisheye Orthographic: left-right mirror symmetry | value=0 limit=1e-09 |
| PASS | Fisheye Orthographic: up-down mirror symmetry | value=0 limit=1e-09 |
| PASS | Fisheye Orthographic: off-axis angle grows monotonically along the horizontal axis |  |
| FAIL | Fisheye Orthographic: direction-to-screen projection inverts the ray generator | missing=0 worst_error=1.2 |

## Projection mode 6: Panini Cylindrical

| Result | Check | Detail |
| --- | --- | --- |
| PASS | Panini Cylindrical: sampled rays are finite and unit length | samples=324 invalid=0 worst_deviation=2.22e-16 |
| PASS | Panini Cylindrical: every pixel of a 96x54 renderer grid yields a valid ray | invalid=0 |
| PASS | Panini Cylindrical: centre pixel looks straight along the forward axis |  |
| PASS | Panini Cylindrical: left-right mirror symmetry | value=0 limit=1e-09 |
| PASS | Panini Cylindrical: up-down mirror symmetry | value=0 limit=1e-09 |
| PASS | Panini Cylindrical: off-axis angle grows monotonically along the horizontal axis |  |
| FAIL | Panini Cylindrical: direction-to-screen projection inverts the ray generator | missing=0 worst_error=1.209 |

## Projection mode 7: Hammer-Aitoff

| Result | Check | Detail |
| --- | --- | --- |
| PASS | Hammer-Aitoff: sampled rays are finite and unit length | samples=324 invalid=0 worst_deviation=2.22e-16 |
| PASS | Hammer-Aitoff: every pixel of a 96x54 renderer grid yields a valid ray | invalid=0 |
| PASS | Hammer-Aitoff: centre pixel looks straight along the forward axis |  |
| PASS | Hammer-Aitoff: left-right mirror symmetry | value=0 limit=1e-09 |
| PASS | Hammer-Aitoff: up-down mirror symmetry | value=1.38778e-16 limit=1e-09 |
| PASS | Hammer-Aitoff: off-axis angle grows monotonically along the horizontal axis |  |
| FAIL | Hammer-Aitoff: direction-to-screen projection inverts the ray generator | missing=0 worst_error=1.652 |

## Pinhole analytic geometry

| Result | Check | Detail |
| --- | --- | --- |
| PASS | Pinhole: horizontal edge ray subtends half the field of view at fov 30 deg | actual=0.261799387799 expected=0.261799387799 abs_error=4.996e-16 tolerance=1e-12 |
| PASS | Pinhole: vertical edge ray subtends half the field of view at fov 30 deg | actual=0.261799387799 expected=0.261799387799 abs_error=4.996e-16 tolerance=1e-12 |
| PASS | Pinhole: horizontal edge ray subtends half the field of view at fov 60 deg | actual=0.523598775598 expected=0.523598775598 abs_error=1.11e-16 tolerance=1e-12 |
| PASS | Pinhole: vertical edge ray subtends half the field of view at fov 60 deg | actual=0.523598775598 expected=0.523598775598 abs_error=1.11e-16 tolerance=1e-12 |
| PASS | Pinhole: horizontal edge ray subtends half the field of view at fov 90 deg | actual=0.785398163397 expected=0.785398163397 abs_error=0 tolerance=1e-12 |
| PASS | Pinhole: vertical edge ray subtends half the field of view at fov 90 deg | actual=0.785398163397 expected=0.785398163397 abs_error=0 tolerance=1e-12 |
| PASS | Pinhole: horizontal edge ray subtends half the field of view at fov 120 deg | actual=1.0471975512 expected=1.0471975512 abs_error=0 tolerance=1e-12 |
| PASS | Pinhole: vertical edge ray subtends half the field of view at fov 120 deg | actual=1.0471975512 expected=1.0471975512 abs_error=0 tolerance=1e-12 |
| PASS | Pinhole: a direction behind the camera has no screen position |  |


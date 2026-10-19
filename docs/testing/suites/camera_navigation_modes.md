# Camera navigation modes suite

Executable: `test_camera_navigation_modes` - Source: `tests/test_camera_navigation_modes.cpp` - Results: [camera_navigation_modes.md](../results/camera_navigation_modes.md)

## Purpose

Verifies the camera model and the six navigation modes without requiring a window: Free Fly 6-DOF, Orbit Center, Spherical, Rocket 6-DOF Thrust, Surface Walk and Planet Orbit.

## Sections

| Section | Verified behaviour |
| --- | --- |
| Mode catalogue | Six modes, Planet Orbit is mode 5, each mode is selectable through the command queue, cycling visits every mode and wraps |
| Translation and rotation | Exact translation, spherical coordinate synchronisation, rejection of non-finite input, position clamp at 1e12, pitch clamp at +-89 degrees, yaw and roll wrapping, field of view clamp (5 to 175 degrees), minimum navigation speed |
| Orientation basis | Forward, right and up are orthonormal with constant handedness over six sampled attitudes |
| Rocket and inertial modes | Rocket mode integrates velocity under a running clock, stays still when paused, clamps runaway positions, zeroes non-finite velocity; every other mode ignores residual velocity |
| Pose preservation | Switching modes never teleports the camera; camera reset restores the home pose and clears velocity |
| Controller helpers | Equatorial, polar and ISCO snaps, look-at alignment, surface walk entry (never inside the body, mode 4) and exit (mode 0) |

## Limits

Keyboard and mouse driven motion of the Free Fly, Orbit, Spherical and Planet Orbit modes lives in the interactive controller update loop, which requires a window and input state. This suite validates the shared state model, command handling and integration those modes rely on; scripted input tests for the update loop are listed as future work.

## Results

See [../results/camera_navigation_modes.md](../results/camera_navigation_modes.md).

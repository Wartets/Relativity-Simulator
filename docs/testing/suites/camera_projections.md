# Camera projections suite

Executable: `test_camera_projections` - Source: `tests/test_camera_projections.cpp` - Results: [camera_projections.md](../results/camera_projections.md)

## Purpose

Verifies the optical frequency shift, beaming, and projection primitives of [DESCRIPTION.md](../../DESCRIPTION.md#83-optical-projections--field-of-view) and [MATHEMATICAL_FORMULATION.md Section 6.1](../../MATHEMATICAL_FORMULATION.md#61-relativistic-frequency-shift-beaming--frame-invariance).

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Pinhole projection | Centre and corner rays | Forward unit vector at the centre, unit corner ray in the expected quadrant, within 1e-14 |
| Auto-zoom compensation | Tangent ratio at gamma = 50 | Equals sqrt((1-beta)/(1+beta)) within 1e-7 |
| Stereographic fisheye | Edge ray at 220 degrees | Unit length and angle of 110 degrees within 1e-12 |
| Equirectangular panorama | Cardinal directions | Forward, backward, up and down within 1e-14 |
| Rays from a stationary tetrad | Null condition | Squared Minkowski norm below 1e-14 in absolute value |

## Acceptance

All checks pass.

## Recorded results

See [../results/camera_projections.md](../results/camera_projections.md).

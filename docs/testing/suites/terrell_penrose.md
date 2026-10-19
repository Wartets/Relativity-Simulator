# Terrell-Penrose suite

Executable: `test_terrell_penrose` - Source: `tests/test_terrell_penrose.cpp` - Results: [terrell_penrose.md](../results/terrell_penrose.md)

## Purpose

Verifies `TerrellPenroseOptics` of [DESCRIPTION.md](../../DESCRIPTION.md#31-relativistic-kinematics--optical-effects).

## Scenario

Aberration of a transverse direction under beta = (0.5, 0, 0) and of a diagonal direction under beta = (0.2, 0.3, 0.4); a unit sphere at (0, 10, 0) moving at (0.8, 0, 0) observed at t = 10 from the origin.

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Aberration inversion | Axis-aligned boost | Backward aberration restores the direction within 1e-12 |
| Aberration inversion | Oblique boost | Same criterion |
| Moving sphere | Intersection | The ray hits the sphere with a positive Doppler shift |
| Moving sphere | Rotation angle | sin(angle) = 0.8 within 1e-12 |

## Justification

Aberration is a Lorentz transformation of the direction, so the opposite boost inverts it. The Terrell rotation angle of a sphere moving at beta satisfies sin(angle) = beta.

## Acceptance

All checks pass.

## Recorded results

See [../results/terrell_penrose.md](../results/terrell_penrose.md).

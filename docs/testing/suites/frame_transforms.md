# Frame transforms suite

Executable: `test_frame_transforms` - Source: `tests/test_frame_transforms.cpp` - Results: [frame_transforms.md](../results/frame_transforms.md)

## Purpose

Verifies the frame conversions listed in [ARCHITECTURE.md](../../ARCHITECTURE.md#61-ephemeris--orbital-mechanics-ingestion).

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| ICRF and ecliptic round trip | Position and velocity components | Restored within a relative 1e-12 |
| Lorentz boost matrices | Zero boost | Unit time-time and x-x entries |
| Lorentz boost matrices | Boost of 0.6 c along x | Diagonal entry equals gamma = 1.25, off-diagonal entry equals -gamma beta, both within 1e-12 |

## Justification

The ecliptic transform is a fixed rotation by the obliquity at J2000, so its inverse composes to the identity up to rounding. The boost entries follow the standard Lorentz matrix.

## Acceptance

All checks pass.

## Recorded results

See [../results/frame_transforms.md](../results/frame_transforms.md).

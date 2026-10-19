# MOND rotation curves suite

Executable: `test_mond_rotation_curves` - Source: `tests/test_mond_rotation_curves.cpp` - Results: [mond_rotation_curves.md](../results/mond_rotation_curves.md)

## Purpose

Verifies `MondFramework` against the limits of modified Newtonian dynamics ([DESCRIPTION.md](../../DESCRIPTION.md#53-dark-matter-halos--alternative-gravitational-theories)).

## Scenario

Acceleration scale a0 = 1.2e-10 m/s^2 and a point baryonic mass of 1e11 solar masses.

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Acceleration limits | Newtonian limit at 1e-7 m/s^2 | Within 1 percent |
| Acceleration limits | Deep-MOND limit at 1e-13 m/s^2 | Equals sqrt(a_N a0) within 5 percent |
| Rotation curve | 30, 50 and 100 kpc | Within 5, 3 and 2 percent of the asymptotic flat velocity |
| Baryonic Tully-Fisher | v^4 at 100 kpc | Equals G M a0 within 5 percent |
| Simple interpolation | Velocity at 100 kpc | Within 5 percent of the flat velocity |

## Justification

The tolerances shrink with radius because the standard interpolation function approaches the deep-MOND regime slowly.

## Acceptance

All checks pass.

## Recorded results

See [../results/mond_rotation_curves.md](../results/mond_rotation_curves.md).

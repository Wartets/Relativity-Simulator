# Galaxy collision suite

Executable: `test_galaxy_collision_nbody` - Source: `tests/test_galaxy_collision_nbody.cpp` - Results: [galaxy_collision_nbody.md](../results/galaxy_collision_nbody.md)

## Purpose

Verifies the galaxy generator and the collisionless integration of [DESCRIPTION.md](../../DESCRIPTION.md#11-standard-simulation-scenarios) (Galaxy Collision with Dark Matter).

## Scenario

Two galaxies of 150 disk, 50 bulge and 300 halo particles at (-50, 0, 0) kpc and (50, 0, 0) kpc with velocities of (100, 50, 0) and (-100, -50, 0) km/s and spins 0.2 and -0.4, PCG64 seed 12345, opening angle 0.6, softening 500 pc, 15 steps of 5e13 s.

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Initial conditions | Particle count | 1000 |
| Collision evolution | Finiteness | Centre of mass drift and angular momentum change finite |
| Collision evolution | Centre of mass | Drift below 1e19 m |
| Collision evolution | Angular momentum | Relative change of the magnitude below 5 percent |

## Justification

The two galaxies have opposite velocities, so the centre of mass is nearly at rest and the mutual forces conserve the angular momentum up to tree and time-step errors.

## Measurements

Drift of the centre of mass and relative angular momentum change.

## Acceptance

All checks pass.

## Recorded results

See [../results/galaxy_collision_nbody.md](../results/galaxy_collision_nbody.md).

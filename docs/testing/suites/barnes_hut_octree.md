# Barnes-Hut octree suite

Executable: `test_barnes_hut_octree` - Source: `tests/test_barnes_hut_octree.cpp` - Results: [barnes_hut_octree.md](../results/barnes_hut_octree.md)

## Purpose

Verifies `BarnesHutOctree` of [ARCHITECTURE.md](../../ARCHITECTURE.md#25-dark-matter--modified-gravity) against direct summation.

## Scenario

300 particles of mass 1000 with softening 5, positions and velocities from a seeded PCG64 stream (seed 42), opening angle 0.5, quadrupole enabled, G = 1; ten leapfrog steps of 0.01.

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Tree construction | Node count | More nodes than particles |
| Accuracy against direct summation | Coverage | All twenty sampled particles have an acceleration above 1e-6 |
| Accuracy against direct summation | Error | Largest component error below 5 percent of the acceleration magnitude |
| Leapfrog integration | Finiteness | Every position and velocity finite |

## Justification

With opening angle 0.5 and quadrupole corrections the force error is of the order of one percent, so 5 percent leaves a safety margin without hiding regressions.

## Measurements

Worst relative acceleration error.

## Acceptance

All checks pass.

## Recorded results

See [../results/barnes_hut_octree.md](../results/barnes_hut_octree.md).

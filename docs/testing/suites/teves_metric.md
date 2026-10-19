# TeVeS metric suite

Executable: `test_teves_metric` - Source: `tests/test_teves_metric.cpp` - Results: [teves_metric.md](../results/teves_metric.md)

## Purpose

Verifies `TeVeSSpacetimeMetric` of [DESCRIPTION.md](../../DESCRIPTION.md#53-dark-matter-halos--alternative-gravitational-theories) and its consistency with MOND ([mond_rotation_curves.md](mond_rotation_curves.md)).

## Scenario

Geometrized units: M = 10, a0 = 1e-3, k = 0.01 at r = 50. SI units: M = 1e11 solar masses, a0 = 1.2e-10 m/s^2, observed at 50 and 250 kpc.

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Geometrized units | Shared invariants | Inversion 1e-12, symmetry 1e-14, negative determinant, connection within a relative 1e-4 |
| Galactic scale | Inversion | Within 1e-12 in SI units |
| Galactic scale | Connection signs | Both the radial and temporal symbols are positive |
| Galactic scale | Circular velocity at 50 kpc | Equals sqrt(r a_MOND) within a relative 1e-4 |
| Galactic scale | Velocity at 250 kpc | Equals (G M a0)^(1/4) within 2 percent |

The `SpacetimeMetric` concept is enforced at compile time.

## Justification

For a static spacetime the circular velocity follows from the ratio of the radial and azimuthal connection symbols, which must reproduce the interpolated MOND acceleration and the flat asymptotic velocity of the baryonic Tully-Fisher relation.

## Acceptance

All checks pass.

## Recorded results

See [../results/teves_metric.md](../results/teves_metric.md).

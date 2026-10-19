# Tidal and rotational gravity suite

Executable: `test_tidal_rotational_gravity` - Source: `tests/test_tidal_rotational_gravity.cpp` - Results: [tidal_rotational_gravity.md](../results/tidal_rotational_gravity.md)

## Purpose

Verifies `TidalPerturbationModel` of [ARCHITECTURE.md](../../ARCHITECTURE.md#24-post-newtonian-dynamics--gravimetry), coupled with the spherical harmonic model of [spherical_harmonics.md](spherical_harmonics.md).

## Scenario

An Earth model with J2 = 1.08263e-3, the Moon at (3.844e8, 0, 0) m with GM = 4.9028e12 m^3/s^2, and a satellite at (7e6, 0, 0) m.

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Third-body tide | C20 change | Non-zero and below 1e-7 in absolute value |
| Direct tidal acceleration | Axis components | Non-zero along the Earth-Moon axis; transverse components below 1e-20 |
| Rotational flattening | J2 | Within 1e-4 of 1.08e-3 for the Earth rotation rate and a Love number of 0.938 |

## Justification

On the line joining the two bodies the tidal field is purely radial by symmetry. The rotational flattening of the Earth contributes about 1.08e-3 to J2.

## Measurements

The rotational J2 value.

## Acceptance

All checks pass.

## Recorded results

See [../results/tidal_rotational_gravity.md](../results/tidal_rotational_gravity.md).

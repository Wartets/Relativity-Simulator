# Dark matter field suite

Executable: `test_dark_matter_field` - Source: `tests/test_dark_matter_field.cpp` - Results: [dark_matter_field.md](../results/dark_matter_field.md)

## Purpose

Verifies the renderer-side dark matter field ([DESCRIPTION.md](../../DESCRIPTION.md#53-dark-matter-halos--alternative-gravitational-theories)): geometry, lensing, emission, persistence and N-body coupling. The analytic profiles are covered by [dark_matter_profiles.md](dark_matter_profiles.md).

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Profile mathematics | Origin, monotonicity, derivative | Zero at the centre, non-decreasing and non-negative derivative for every profile |
| Triaxial ellipsoidal geometry | Local coordinates and radius | (3, 4, 2) within 1e-12 and `sqrt(9 + 64 + 64)` within 1e-12 |
| Gravitational acceleration | Point mass | Inward, no transverse part below 1e-14, magnitude `M/r^2` within 1e-12, lensing scale multiplies within 1e-12 |
| Photon deflection | Kick | Speed preserved within 1e-14, deflected inward; step limit positive and not above 3.5 |
| Volumetric emission | Switching and truncation | Zero when disabled and beyond the truncation radius, positive in every channel inside |
| Settings, diagnostics and serialization | Diagnostics | Positive normalization, enclosed mass strictly between 0 and the total, positive circular velocity and density |
| Settings, diagnostics and serialization | Round trip | Scalars within 1e-12, name, profile and anchor equal, tint within 1e-6 |
| Anchor resolution and N-body coupling | Anchor | Position follows the anchor plus offset and drift within 1e-4; no self force; external force non-zero |
| Subhalo population | Generator | Six subhalos with masses in range and radii in the configured shell |
| Presets | Presets | Positive mass, scale radius and normalization for each |

## Justification

The expected values follow from the definitions of the point-mass potential, the ellipsoidal radius and the rotation of the direction by the kick, which preserves the norm by construction.

## Acceptance

All checks pass.

## Recorded results

See [../results/dark_matter_field.md](../results/dark_matter_field.md).

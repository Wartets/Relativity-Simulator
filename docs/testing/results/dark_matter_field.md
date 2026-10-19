# dark_matter_field

Verifies the dark matter field of the renderer and the N-body coupling: enclosed mass fractions of every profile, triaxial ellipsoidal geometry, gravitational acceleration and lensing scale, photon deflection and step limit, volumetric emission, settings serialization, anchor resolution, subhalo population generation and presets.

| Property | Value |
| --- | --- |
| Status | FAILED |
| Checks | 37 |
| Failed checks | 2 |
| Duration | 0.002344 s |
| Generated | 2026-10-19 22:19:17 UTC |

## Profile mathematics

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the enclosed mass fraction vanishes at the centre for every profile |  |
| FAIL | the enclosed mass fraction is non-negative and non-decreasing for every profile |  |
| PASS | the derivative of the enclosed fraction is non-negative for every profile |  |

## Triaxial ellipsoidal geometry

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the local x coordinate equals the offset from the halo centre | actual=3 expected=3 abs_error=0 tolerance=1e-12 |
| PASS | the local y coordinate equals the offset from the halo centre | actual=4 expected=4 abs_error=0 tolerance=1e-12 |
| PASS | the local z coordinate equals the offset from the halo centre | actual=2 expected=2 abs_error=0 tolerance=1e-12 |
| PASS | the ellipsoidal radius divides each axis by its axis ratio | actual=11.7046999107 expected=11.7046999107 abs_error=0 tolerance=1e-12 |

## Gravitational acceleration

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the acceleration points toward the point mass |  |
| PASS | the acceleration has no transverse component on the axis | value=0 limit=1e-14 |
| PASS | the magnitude equals M / r^2 | actual=0.4 expected=0.4 abs_error=0 tolerance=1e-12 |
| PASS | the lensing scale multiplies the acceleration when requested | actual=1 expected=1 abs_error=0 tolerance=1e-12 |

## Photon deflection

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the kick rotates the photon direction without changing its speed | actual=1 expected=1 abs_error=1.11e-16 tolerance=1e-14 |
| PASS | the kick bends the photon toward the halo |  |
| PASS | the step limit is positive |  |
| PASS | the step limit does not exceed the step fraction times the distance | value=3.5 limit=3.5 |

## Volumetric emission

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the emission vanishes while visualization is disabled |  |
| PASS | the emission is positive in every channel inside the halo |  |
| PASS | the emission vanishes beyond the truncation radius |  |

## Settings, diagnostics and serialization

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the first halo is stored at index zero |  |
| PASS | the total halo mass equals the stored mass | actual=4 expected=4 abs_error=0 tolerance=1e-15 |
| PASS | the profile normalization is positive |  |
| PASS | the mass enclosed at the scale radius lies strictly between zero and the total mass |  |
| PASS | the circular velocity and the density at the scale radius are positive |  |
| PASS | the enable flag and the halo count survive serialization |  |
| PASS | the field scalars survive serialization |  |
| PASS | the name, profile and anchor of the halo survive serialization |  |
| PASS | every scalar halo parameter survives serialization | value=0 limit=1e-12 |
| PASS | the tint survives serialization at single precision | value=0 limit=1e-06 |

## Anchor resolution and N-body coupling

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the anchored halo is accepted |  |
| PASS | the resolved field keeps the anchor and the body coupling flag |  |
| PASS | the halo follows its anchor body plus the offset advanced by its velocity | value=0 limit=0.0001 |
| PASS | the anchor body feels no force from its own halo |  |
| PASS | a different body feels the halo |  |

## Subhalo population

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the generator spawns the requested number of subhalos |  |
| PASS | every subhalo mass lies in the configured range |  |
| PASS | every subhalo lies in the configured shell |  |

## Presets

| Result | Check | Detail |
| --- | --- | --- |
| FAIL | every preset has a positive mass, scale radius and normalization |  |


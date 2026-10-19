# tidal_rotational_gravity

Verifies the tidal and rotational perturbation models: the lunar tide changes the Earth C20 coefficient by a small non-zero amount, the direct tidal acceleration of a satellite on the Earth-Moon axis is purely radial, and the rotational flattening reproduces the Earth J2.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 6 |
| Failed checks | 0 |
| Duration | 0.0009168 s |
| Generated | 2026-10-19 22:21:21 UTC |

## Third-body tide

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the lunar tide modifies the normalized C20 coefficient |  |
| PASS | the C20 change stays below 1e-7 | value=3.78172e-09 limit=1e-07 |

## Direct tidal acceleration

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the tidal acceleration has a non-zero component along the Earth-Moon axis |  |
| PASS | the transverse y component vanishes on the axis | value=0 limit=1e-20 |
| PASS | the transverse z component vanishes on the axis | value=0 limit=1e-20 |

## Rotational flattening

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the rotational contribution to J2 is within 1e-4 of 1.08e-3 | actual=0.00108226186694 expected=0.00108 abs_error=2.262e-06 tolerance=0.0001 |

## Measurements

| Section | Name | Value | Unit |
| --- | --- | --- | --- |
| Rotational flattening | rotational_j2 | 0.00108226187 | dimensionless |


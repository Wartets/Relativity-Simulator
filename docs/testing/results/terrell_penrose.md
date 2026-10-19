# terrell_penrose

Verifies the Terrell-Penrose optics: the backward aberration inverts the forward aberration for axis-aligned and oblique velocities, a ray intersects a moving sphere with a positive Doppler factor, and the apparent rotation angle satisfies sin(angle) = beta.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 5 |
| Failed checks | 0 |
| Duration | 0.0008141 s |
| Generated | 2026-10-19 22:21:20 UTC |

## Aberration inversion

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the backward aberration restores a transverse direction for a boost along x | value=1.4803e-16 limit=1e-12 |
| PASS | the backward aberration restores a diagonal direction for an oblique boost | value=1.11022e-16 limit=1e-12 |

## Moving sphere

| Result | Check | Detail |
| --- | --- | --- |
| PASS | a ray aimed at the retarded position of the sphere intersects it |  |
| PASS | the Doppler shift at the hit is positive |  |
| PASS | the apparent rotation angle satisfies sin(angle) = beta | actual=0.8 expected=0.8 abs_error=0 tolerance=1e-12 |


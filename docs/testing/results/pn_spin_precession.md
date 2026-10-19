# pn_spin_precession

Verifies the post-Newtonian spin precession of a binary: both precession angular velocities point along the orbital angular momentum and each spin derivative is orthogonal to its spin, so the spin magnitudes are conserved.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 4 |
| Failed checks | 0 |
| Duration | 0.0007347 s |
| Generated | 2026-10-19 22:20:59 UTC |

## Precession direction

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the precession angular velocity of the first body has a positive component along the orbital axis |  |
| PASS | the precession angular velocity of the second body has a positive component along the orbital axis |  |

## Spin magnitude conservation

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the first spin derivative is orthogonal to the first spin relative to their norms | value=0 limit=5e-17 |
| PASS | the second spin derivative is orthogonal to the second spin relative to their norms | value=0 limit=1.89663e-16 |


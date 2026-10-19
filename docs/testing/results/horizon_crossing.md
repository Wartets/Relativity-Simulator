# horizon_crossing

Verifies horizon-regular spacetimes and horizon handling: Painleve-Gullstrand and Eddington-Finkelstein metrics and connections stay regular from 0.1 to 10 Schwarzschild radii, a free-falling particle and an ingoing photon cross the horizon with preserved invariants, absorption and continuity boundary modes behave as configured, and rays inside the horizon cannot escape.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 35 |
| Failed checks | 0 |
| Duration | 0.05795 s |
| Generated | 2026-10-19 22:19:28 UTC |

## Painleve-Gullstrand regularity across the horizon

| Result | Check | Detail |
| --- | --- | --- |
| PASS | Painleve-Gullstrand: the Schwarzschild radius equals 2 M in geometric units | actual=2 expected=2 abs_error=0 tolerance=1e-15 |
| PASS | Painleve-Gullstrand: the inverse metric reproduces the identity at every sampled radius | value=1.77636e-15 limit=1e-12 |
| PASS | Painleve-Gullstrand: the analytic and numerical connections are finite at every sampled radius |  |
| PASS | Painleve-Gullstrand: the analytic connection matches eighth-order finite differences relative to the largest symbol of its row | value=1.03648e-13 limit=1e-06 |

## Eddington-Finkelstein regularity across the horizon

| Result | Check | Detail |
| --- | --- | --- |
| PASS | Eddington-Finkelstein: the Schwarzschild radius equals 2 M in geometric units | actual=2 expected=2 abs_error=0 tolerance=1e-15 |
| PASS | Eddington-Finkelstein: the inverse metric reproduces the identity at every sampled radius | value=2.22045e-16 limit=1e-12 |
| PASS | Eddington-Finkelstein: the analytic and numerical connections are finite at every sampled radius |  |
| PASS | Eddington-Finkelstein: the analytic connection matches eighth-order finite differences relative to the largest symbol of its row | value=1.02228e-13 limit=1e-06 |

## Painleve-Gullstrand timelike free fall

| Result | Check | Detail |
| --- | --- | --- |
| PASS | every adaptive step of the free fall succeeds |  |
| PASS | the state stays finite throughout the fall |  |
| PASS | the particle crosses the Schwarzschild radius |  |
| PASS | the integrator statistics report the horizon crossing |  |
| PASS | the particle reaches the deep interior |  |
| PASS | the temporal velocity of a free fall from rest stays equal to 1 | value=2.22045e-16 limit=1e-06 |
| PASS | the four-velocity normalization stays equal to -c^2 | value=7.10543e-15 limit=1e-08 |

## Eddington-Finkelstein infalling photon

| Result | Check | Detail |
| --- | --- | --- |
| PASS | every adaptive step of the photon succeeds |  |
| PASS | the photon crosses the Schwarzschild radius |  |
| PASS | the integrator statistics report the horizon crossing |  |
| PASS | the photon reaches the deep interior |  |
| PASS | the advanced time of an ingoing null ray stays constant | value=0 limit=1e-08 |
| PASS | the advanced-time velocity of an ingoing null ray stays zero | value=0 limit=1e-08 |
| PASS | the radial velocity of an ingoing null ray stays equal to -1 | value=0 limit=1e-08 |
| PASS | the null constraint holds along the ray | value=0 limit=1e-12 |

## Horizon boundary modes

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the absorbing integration terminates within the step budget |  |
| PASS | absorption mode reports the crossing and the absorption |  |
| PASS | absorption mode does not reach the singularity |  |
| PASS | the absorbed particle ends at or inside the horizon |  |
| PASS | the continuing integration terminates within the step budget |  |
| PASS | continuity mode crosses the horizon without absorption |  |
| PASS | continuity mode reaches the singularity threshold |  |
| PASS | the particle ends inside the singularity threshold |  |

## Interior trapped region

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the initial radial velocity inside the horizon points inward |  |
| PASS | every adaptive step inside the horizon succeeds |  |
| PASS | the radius never exceeds the horizon while inside the trapped region |  |
| PASS | the ray reaches the singularity threshold |  |


# polarized_radiative_transfer

Verifies polarized radiative transfer: Stokes vector invariants and frame rotation, Bessel functions and the Maxwell-Juttner distribution, the polarization degree of power-law synchrotron emission and absorption, thermal bremsstrahlung and its Gaunt factor, the analytic and Runge-Kutta transfer steps, and inverse Compton scattering against the Thomson limit.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 30 |
| Failed checks | 0 |
| Duration | 0.02995 s |
| Generated | 2026-10-19 22:20:59 UTC |

## Stokes vector

| Result | Check | Detail |
| --- | --- | --- |
| PASS | a fully linearly polarized vector is physically valid |  |
| PASS | the intensity is the first component | actual=10 expected=10 abs_error=0 tolerance=1e-12 |
| PASS | the linearly polarized intensity equals sqrt(Q^2 + U^2) | actual=10 expected=10 abs_error=0 tolerance=1e-12 |
| PASS | the fractional linear polarization equals 1 | actual=1 expected=1 abs_error=0 tolerance=1e-12 |
| PASS | the electric vector position angle equals half of atan2(U, Q) | actual=0.463647609001 expected=0.463647609001 abs_error=0 tolerance=1e-12 |
| PASS | the intensity is invariant under a frame rotation | actual=10 expected=10 abs_error=0 tolerance=1e-12 |
| PASS | a quarter-turn rotation of the position angle exchanges Q and U | actual=8 expected=8 abs_error=0 tolerance=1e-12 |
| PASS | a quarter-turn rotation of the position angle changes the sign of the old Q | actual=-6 expected=-6 abs_error=8.882e-16 tolerance=1e-12 |

## Bessel functions and Maxwell-Juttner distribution

| Result | Check | Detail |
| --- | --- | --- |
| PASS | K0(1) lies between 0.4 and 0.5 |  |
| PASS | K1(1) lies between 0.5 and 0.7 |  |
| PASS | K2(1) lies between 1.5 and 1.8 |  |
| PASS | the recurrence K2(x) = 2 K1(x) / x + K0(x) holds at x = 1 | actual=1.62483891986 expected=1.62483891986 abs_error=0 tolerance=1e-10 |
| PASS | the mean Lorentz factor at theta = 1 lies between 3 and 4 |  |
| PASS | every sampled Lorentz factor is at least 1 |  |
| PASS | the sample mean agrees with the analytic mean within 0.1 | actual=3.36094313813 expected=3.37044117743 abs_error=0.009498 tolerance=0.1 |

## Synchrotron emission

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the linear polarization of a power law with p = 3 equals (p + 1) / (p + 7/3) | actual=0.75 expected=0.75 abs_error=0 tolerance=1e-12 |
| PASS | the linear polarization for p = 3 equals 0.75 | actual=0.75 expected=0.75 abs_error=1.11e-16 tolerance=1e-12 |
| PASS | the linear polarization for p = 2 equals 9/13 | actual=0.692307692308 expected=0.692307692308 abs_error=1.11e-16 tolerance=1e-12 |
| PASS | the absorption coefficients are positive |  |
| PASS | the absorption polarization equals the emission polarization for a power law | actual=0.692307692308 expected=0.692307692308 abs_error=0 tolerance=1e-12 |

## Thermal bremsstrahlung

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the bremsstrahlung emissivity is positive |  |
| PASS | the thermal Gaunt factor lies between 0.5 and 20 | gaunt=4.660377 |

## Polarized transfer

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the analytic Delano step yields a physically valid Stokes vector |  |
| PASS | the analytic step builds up positive intensity |  |
| PASS | the analytic step builds up linear polarization |  |
| PASS | the Runge-Kutta step builds up positive intensity |  |

## Inverse Compton scattering

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the Klein-Nishina cross section tends to the Thomson value at low energy within a relative 1e-3 | actual=6.65232568638e-29 expected=6.6524587321e-29 rel_error=2e-05 tolerance=0.001 |
| PASS | the Compton y parameter is positive |  |
| PASS | scattering preserves the number of photon packets |  |
| PASS | scattering off hot electrons raises the mean photon energy |  |


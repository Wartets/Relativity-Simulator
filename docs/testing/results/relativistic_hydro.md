# relativistic_hydro

Verifies the relativistic hydrodynamics stack: ideal gas and Synge equations of state, the conserved to primitive inversion for hydrodynamic, ultra-relativistic and magnetized states, WENO5 and MP5 reconstruction, the HLL, HLLC and HLLD solvers, constrained transport and the relativistic Sod and Balsara 1 shock tubes.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 26 |
| Failed checks | 0 |
| Duration | 3.255 s |
| Generated | 2026-10-19 22:21:04 UTC |

## Equation of state

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the ideal gas pressure equals (Gamma - 1) rho eps | actual=2 expected=2 abs_error=0 tolerance=1e-12 |
| PASS | the specific internal energy is recovered from the pressure | actual=1.5 expected=1.5 abs_error=2.22e-16 tolerance=1e-12 |
| PASS | the specific enthalpy equals 1 + eps + p / rho | actual=3.5 expected=3.5 abs_error=0 tolerance=1e-12 |
| PASS | the sound speed squared is positive and below Gamma - 1 |  |
| PASS | the pressure is recovered from the enthalpy | actual=2 expected=2 abs_error=0 tolerance=1e-12 |
| PASS | the Synge enthalpy exceeds 1 |  |
| PASS | the Synge sound speed squared lies in (0, 1/3] |  |

## Conserved to primitive inversion

| Result | Check | Detail |
| --- | --- | --- |
| PASS | a hydrodynamic state round trips within 1e-10 | value=4.85723e-13 limit=1e-10 |
| PASS | an ultra-relativistic state with v = 0.999 round trips within 1e-8 | value=1.10672e-12 limit=1e-08 |
| PASS | a magnetized state round trips within 1e-6 | value=9.35785e-12 limit=1e-06 |
| PASS | the magnetic field is recovered within 1e-12 | value=0 limit=1e-12 |

## Spatial reconstruction

| Result | Check | Detail |
| --- | --- | --- |
| PASS | WENO5 reconstructs the left face of a smooth profile within 1e-6 | value=1.51636e-07 limit=1e-06 |
| PASS | MP5 reconstructs the left face of a smooth profile within 1e-6 | value=1.51636e-07 limit=1e-06 |
| PASS | WENO5 stays within the data bounds across a discontinuity |  |

## Riemann solvers

| Result | Check | Detail |
| --- | --- | --- |
| PASS | a uniform static state has no mass flux for HLL, HLLC and HLLD | value=0 limit=1e-12 |
| PASS | a uniform static state has a momentum flux equal to the pressure for HLL, HLLC and HLLD | value=0 limit=1e-12 |
| PASS | the HLL momentum flux of the Sod interface is positive |  |
| PASS | the HLLC momentum flux of the Sod interface is positive |  |

## Constrained transport

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the initial field derived from a vector potential is divergence free | value=4.44089e-16 limit=1e-12 |
| PASS | the divergence stays below 1e-12 over fifty updates | value=1.86517e-14 limit=1e-12 |

## Relativistic Sod shock tube

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the left state is undisturbed at t = 0.4 |  |
| PASS | the right density is undisturbed at t = 0.4 |  |
| PASS | the shocked region has a velocity between 0.60 and 0.80 |  |
| PASS | the pressure of the shocked region lies between 0.01 and 5 |  |
| PASS | every cell keeps a finite positive density and pressure and a subluminal velocity |  |

## Relativistic Balsara 1 magnetized shock tube

| Result | Check | Detail |
| --- | --- | --- |
| PASS | every cell keeps a finite positive density and pressure, a subluminal velocity and a finite transverse field |  |

## Measurements

| Section | Name | Value | Unit |
| --- | --- | --- | --- |
| Constrained transport | max_divergence | 1.86517468e-14 | dimensionless |


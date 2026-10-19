# pn_conservative

Verifies the conservative post-Newtonian dynamics: energy and orbital angular momentum conservation of an equal-mass 1PN binary integrated with the fourth-order Runge-Kutta scheme, and energy conservation of a Newtonian binary integrated with the symplectic Forest-Ruth scheme.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 4 |
| Failed checks | 0 |
| Duration | 0.2095 s |
| Generated | 2026-10-19 22:20:58 UTC |

## 1PN binary with the Runge-Kutta integrator

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the initial total energy is finite | value=-0.027398078125 |
| PASS | the relative energy drift over 2000 steps stays below 1e-4 | value=5.49564e-05 limit=0.0001 |
| PASS | the relative drift of the orbital angular momentum stays below 1e-4 | value=6.07919e-08 limit=0.0001 |

## Newtonian binary with the symplectic integrator

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the relative energy drift of the symplectic scheme stays below 1e-5 | value=3.78485e-16 limit=1e-05 |

## Measurements

| Section | Name | Value | Unit |
| --- | --- | --- | --- |
| 1PN binary with the Runge-Kutta integrator | relative_energy_drift_1pn_rk4 | 5.49564291e-05 | dimensionless |
| 1PN binary with the Runge-Kutta integrator | relative_angular_momentum_drift_1pn_rk4 | 6.0791888e-08 | dimensionless |
| Newtonian binary with the symplectic integrator | relative_energy_drift_newtonian_symplectic | 3.78485122e-16 | dimensionless |


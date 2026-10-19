# bssn_gauge_wave

Verifies the BSSN evolution on the one-dimensional gauge wave of amplitude 0.1 on a 64 by 4 by 4 grid evolved for one thousand Runge-Kutta steps of 0.25 dx: the Hamiltonian constraint stays finite at every checkpoint, the lapse stays positive, and the constraint norm grows by less than a factor of 100.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 4 |
| Failed checks | 0 |
| Duration | 81.5 s |
| Generated | 2026-10-19 22:16:30 UTC |

## Initial data

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the initial Hamiltonian constraint is finite |  |

## Evolution

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the Hamiltonian constraint stays finite at every checkpoint |  |
| PASS | the lapse stays finite and positive everywhere | minimum=1.17623 |
| PASS | the constraint norm never exceeds one hundred times its initial value | value=43.5579 limit=313.711 |

## Measurements

| Section | Name | Value | Unit |
| --- | --- | --- | --- |
| Initial data | initial_hamiltonian_linf | 3.13710898 | dimensionless |
| Evolution | hamiltonian_linf_step_100 | 1.10623782 | dimensionless |
| Evolution | hamiltonian_linf_step_200 | 3.51666858 | dimensionless |
| Evolution | hamiltonian_linf_step_300 | 2.84330083 | dimensionless |
| Evolution | hamiltonian_linf_step_400 | 4.32816488 | dimensionless |
| Evolution | hamiltonian_linf_step_500 | 3.83564628 | dimensionless |
| Evolution | hamiltonian_linf_step_600 | 5.70099252 | dimensionless |
| Evolution | hamiltonian_linf_step_700 | 6.89702708 | dimensionless |
| Evolution | hamiltonian_linf_step_800 | 10.1697606 | dimensionless |
| Evolution | hamiltonian_linf_step_900 | 19.9658935 | dimensionless |
| Evolution | hamiltonian_linf_step_1000 | 43.5579401 | dimensionless |
| Evolution | peak_hamiltonian_linf | 43.5579401 | dimensionless |


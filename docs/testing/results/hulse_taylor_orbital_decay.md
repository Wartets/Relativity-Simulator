# hulse_taylor_orbital_decay

Verifies the Hulse-Taylor binary pulsar: the Peters-Mathews orbital period derivative agrees with the observed value, the post-Newtonian system of the pulsar is built with two bodies and a positive gravitational wave luminosity, and the adaptive integrator advances it.

| Property | Value |
| --- | --- |
| Status | FAILED |
| Checks | 5 |
| Failed checks | 1 |
| Duration | 0.06029 s |
| Generated | 2026-10-19 22:19:29 UTC |

## Peters-Mathews period derivative

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the analytical period derivative agrees with the observed value within 1 percent | value=0.00838583 limit=0.01 |

## Post-Newtonian orbital evolution

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the pulsar system contains exactly two bodies |  |
| FAIL | the initial gravitational wave luminosity is positive |  |
| PASS | the integrator recorded at least one step |  |
| PASS | the system time advanced |  |

## Measurements

| Section | Name | Value | Unit |
| --- | --- | --- | --- |
| Peters-Mathews period derivative | peters_mathews_p_dot | -2.40268114e-12 | s/s |
| Peters-Mathews period derivative | observed_p_dot | -2.423e-12 | s/s |
| Peters-Mathews period derivative | relative_difference | 0.00838582868 | dimensionless |
| Post-Newtonian orbital evolution | evolved_time | 59294.8094 | simulation time |


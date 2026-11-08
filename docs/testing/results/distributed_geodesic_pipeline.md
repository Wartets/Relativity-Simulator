# distributed_geodesic_pipeline

Validates distributed geodesic compute pipeline execution, colorimetric and geometric fidelity of reassembled frames against local reference, resilience under simulated worker failure, and distributed physics/uncertainty evaluation.

| Property | Value |
| --- | --- |
| Status | FAILED |
| Checks | 19 |
| Failed checks | 6 |
| Duration | 0.2063 s |
| Generated | 2026-11-08 00:20:30 UTC |

## Pipeline Frame Reassembly & Parity

| Result | Check | Detail |
| --- | --- | --- |
| FAIL | orchestrator dispatch frame succeeds |  |
| FAIL | radiance matches local reference within 1e-5 across all pixels | value=1 limit=1e-05 |
| FAIL | pixel status classifications match exactly |  |
| FAIL | horizon absorbed ray count preserved exactly |  |
| FAIL | celestial hit ray count preserved exactly |  |

## Worker Disconnection Resilience & Work Stealing

| Result | Check | Detail |
| --- | --- | --- |
| FAIL | frame completed reliably despite synthetic disruption |  |
| PASS | zero unrendered or missing pixels in reassembled framebuffer |  |

## Distributed 1PN N-Body Evaluation

| Result | Check | Detail |
| --- | --- | --- |
| PASS | local 1pn nbody evaluation succeeds |  |
| PASS | orchestrator evaluate nbody accelerations succeeds |  |
| PASS | nbody 1pn accelerations match local evaluation within 1e-12 | value=0 limit=1e-12 |

## Distributed Geodesic Uncertainty Ensemble

| Result | Check | Detail |
| --- | --- | --- |
| PASS | local geodesic ensemble runs successfully |  |
| PASS | orchestrator run geodesic ensemble succeeds |  |
| PASS | sample count preserved exactly |  |
| PASS | absorbed count matches local reference |  |
| PASS | escaped count matches local reference |  |
| PASS | mean minimum radius matches reference | actual=50 expected=50 abs_error=0 tolerance=1e-10 |

## Distributed Stochastic PCE Quadrature

| Result | Check | Detail |
| --- | --- | --- |
| PASS | local pce quadrature runs successfully |  |
| PASS | orchestrator pce quadrature succeeds |  |
| PASS | pce quadrature points match local evaluation within 1e-10 | value=0 limit=1e-10 |

## Measurements

| Section | Name | Value | Unit |
| --- | --- | --- | --- |
| Pipeline Frame Reassembly & Parity | max_radiance_absolute_difference | 1 | radiance |
| Distributed 1PN N-Body Evaluation | max_nbody_acceleration_error | 0 | acceleration |
| Distributed Stochastic PCE Quadrature | max_pce_response_error | 0 | response |


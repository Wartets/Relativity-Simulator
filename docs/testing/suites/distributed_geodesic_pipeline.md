# Distributed geodesic pipeline suite

Executable: `test_distributed_geodesic_pipeline` - Source: `tests/test_distributed_geodesic_pipeline.cpp` - Results: [distributed_geodesic_pipeline.md](../results/distributed_geodesic_pipeline.md)

## Purpose

Verifies end-to-end distributed pipeline execution through `ClusterOrchestrator` ([ARCHITECTURE.md](../../ARCHITECTURE.md#214-distributed-compute-subsystem--heterogeneous-cluster-architecture)): full-frame tile dispatch and reassembly against local references, fault-tolerant work stealing during worker disruption, distributed 1PN post-Newtonian N-body acceleration reductions, distributed Monte Carlo geodesic uncertainty ensembles, and distributed polynomial chaos stochastic quadratures.

## Scenario

A 128x128 Schwarzschild black hole scene ($M = 1.0, r_h = 2.0$) with accretion disk; worker disruption tested with 100 ms timeout; 32-body post-Newtonian system with $c = 100.0$; 1000-sample geodesic uncertainty ensemble at $r = 50.0$; 16-point stochastic PCE quadrature of degree 3.

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Pipeline Frame Reassembly & Parity | Dispatch | Orchestrator dispatches and completes frame |
| Pipeline Frame Reassembly & Parity | Radiance fidelity | Radiance matches local reference within 1e-5 |
| Pipeline Frame Reassembly & Parity | Classification | Pixel status flags match local reference across all pixels |
| Pipeline Frame Reassembly & Parity | Invariants | Horizon absorbed and celestial hit ray counts match exactly |
| Worker Disconnection Resilience & Work Stealing | Fault tolerance | Frame completes reliably under disruption |
| Worker Disconnection Resilience & Work Stealing | Coverage | Zero unrendered or missing pixels in reassembled framebuffer |
| Distributed 1PN N-Body Evaluation | Execution | Local and distributed 1PN reductions succeed |
| Distributed 1PN N-Body Evaluation | Precision | Distributed accelerations match local evaluation within 1e-12 |
| Distributed Geodesic Uncertainty Ensemble | Execution | Local and distributed ensembles succeed |
| Distributed Geodesic Uncertainty Ensemble | Statistics | Sample, absorbed, and escaped counts match; mean radius within 1e-10 |
| Distributed Stochastic PCE Quadrature | Execution | Local and distributed quadratures succeed |
| Distributed Stochastic PCE Quadrature | Precision | Quadrature response values match local evaluation within 1e-10 |

## Justification

Reassembled tile boundaries must not introduce radiance discontinuities or misclassified pixels. The distributed parallel reductions for N-body accelerations, Chan-Golub-LeVeque statistical moments, and tensor-grid quadratures are mathematically equivalent to local evaluations, bounded by numerical precision.

## Measurements

Maximum absolute radiance difference (`max_radiance_absolute_difference`), maximum N-body acceleration error (`max_nbody_acceleration_error`), and maximum PCE response error (`max_pce_response_error`).

## Acceptance

All checks pass.

## Recorded results

See [../results/distributed_geodesic_pipeline.md](../results/distributed_geodesic_pipeline.md).

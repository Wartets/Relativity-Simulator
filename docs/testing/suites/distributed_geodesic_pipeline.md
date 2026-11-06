# Test Suite Specification: Distributed Geodesic Pipeline

- **Executable Target:** `test_distributed_geodesic_pipeline`
- **Source File:** `tests/test_distributed_geodesic_pipeline.cpp`
- **Output Report:** `docs/testing/results/distributed_geodesic_pipeline.md`

## Purpose

Validates end-to-end distributed master pipeline execution, colorimetric and geometric fidelity of reassembled frames against local reference, resilience under simulated worker failure, and distributed physics/uncertainty evaluation.

## Checked Invariants & Tolerances

| Check Domain | Expected Value | Tolerance | Justification |
| :--- | :--- | :--- | :--- |
| Reassembled Frame Radiance | Difference < 1e-5 | Absolute 1e-5 | Recomposed tiles match reference frame |
| Pixel Classifications | Exact match | 0 mismatches | Status flag consistency across boundaries |
| Shadow & Sky Invariant | Exact count | 0 rays | Geometric preservation of black hole shadow |
| Worker Disconnection Recovery | 0 missing pixels | Exact | Local fallback recovers 100% of framebuffer |
| 1PN N-Body Accelerations | Error < 1e-12 | Absolute 1e-12 | Distributed pairwise acceleration reduction |
| Geodesic Uncertainty Ensemble | Exact sample counts | Exact | Distributed Monte Carlo statistics reduction |
| Stochastic PCE Quadrature | Error < 1e-10 | Absolute 1e-10 | Multidimensional quadrature evaluation |

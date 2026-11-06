# Test Suite Specification: Cluster Worker Execution

- **Executable Target:** `test_cluster_worker_execution`
- **Source File:** `tests/test_cluster_worker_execution.cpp`
- **Output Report:** `docs/testing/results/cluster_worker_execution.md`

## Purpose

Verifies exact mathematical parity between worker and local tile integration on Kerr black hole spacetimes with accretion disks, cooperative task cancellation response latency, and sustained execution stability.

## Checked Invariants & Tolerances

| Check Domain | Expected Value | Tolerance | Justification |
| :--- | :--- | :--- | :--- |
| Radiance Parity | Relative error < 1e-12 | Relative 1e-12 | Independent evaluation produces identical radiances |
| Redshift Factor Parity | Relative error < 1e-12 | Relative 1e-12 | Exact relativistic Doppler and gravitational shift |
| Status Flags | Match exactly | 0 mismatches | Identical pixel classifications |
| Cancellation Latency | < 15 ms | Upper bound | Real-time viewport responsiveness |
| Sustained Stability | 100% completed | 0 failures | Zero-leak isolation across consecutive tiles |

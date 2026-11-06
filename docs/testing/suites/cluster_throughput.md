# Benchmark Specification: Cluster Throughput & Scaling

- **Executable Target:** `benchmark_cluster_throughput`
- **Source File:** `tests/benchmark_cluster_throughput.cpp`
- **Output Report:** `docs/testing/results/cluster_throughput.md`

## Purpose

Benchmarks distributed geodesic rendering throughput, parallel scaling efficiency, load balancing, and wire codec performance across simulated cluster topologies while strictly decoupling correctness verification from execution timing.

## Measured Metrics

| Metric | Target / Bounds | Unit | Purpose |
| :--- | :--- | :--- | :--- |
| Baseline Throughput | > 0.1 | MPx/s | Single-node baseline performance |
| Cluster Throughput | > Baseline | MPx/s | Distributed aggregate rendering throughput |
| Speedup Factor | > 1.0 | Ratio | Parallel acceleration factor |
| DirectRaw Throughput | Informational | MB/s | Uncompressed memory bandwidth |
| LZ4 Compression Throughput | > 300 | MB/s | Real-time compression rate |

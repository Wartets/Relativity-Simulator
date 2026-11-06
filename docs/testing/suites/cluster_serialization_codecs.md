# Test Suite Specification: Cluster Serialization Codecs

- **Executable Target:** `test_cluster_serialization_codecs`
- **Source File:** `tests/test_cluster_serialization_codecs.cpp`
- **Output Report:** `docs/testing/results/cluster_serialization_codecs.md`

## Purpose

Validates zero-allocation byte shuffling, lossless and near-lossless compression codecs (DirectRaw, QOI-HDR RLE, LZ4Block), SIMD vector sanitization against numerical anomalies, and wire payload encapsulation.

## Checked Invariants & Tolerances

| Check Domain | Expected Value | Tolerance | Justification |
| :--- | :--- | :--- | :--- |
| DirectRaw Identity | Match original | Exact bit identity | Byte-for-byte fidelity across 1024-pixel tile |
| LZ4Block Identity | Match original | Exact bit identity | Byte-shuffled lossless block compression |
| LZ4 Throughput | > 1000 MB/s | Lower bound | Master deserialization bandwidth headroom |
| QOI-HDR Precision | Relative error < 1e-3 | Relative 0.001 | Half-float FP16 spectral quantization bound |
| QOI-HDR Metadata | Exact flags and steps | Exact | Auxiliary radiance fields preserved |
| SIMD Sanitization | Clean post-pass | Clean report | NaNs and infinities neutralized, bounds [0, 1e8] |
| Wire Serialization | Roundtrip match | Exact | Task and result wire headers conform to protocol |

# Container serializer suite

Executable: `test_hdf5_serializer` - Source: `tests/test_hdf5_serializer.cpp` - Results: [hdf5_serializer.md](../results/hdf5_serializer.md)

## Purpose

Verifies the binary container of [FILE_FORMATS.md](../../FILE_FORMATS.md#3-hierarchical-data-format-h5).

## Scenario

A series of 50 metric tensors with a linearly varying diagonal and a worldline of 100 events, written, serialized, deserialized and read back.

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Serialization | Buffer | Non-empty and deserializable |
| Metric series | Dataset | Present with dimensions 50, 4, 4 and fifty tensors |
| Metric series | Values | Every component equal within 1e-15 |
| Worldline | Dataset | `/worldlines/geodesic_001/position` present with one hundred events |
| Worldline | Values | Every component equal within 1e-15 |

## Justification

The payload is stored as raw bytes, so the round trip must reproduce every value bit-exactly on the same host.

## Acceptance

All checks pass.

## Recorded results

See [../results/hdf5_serializer.md](../results/hdf5_serializer.md).

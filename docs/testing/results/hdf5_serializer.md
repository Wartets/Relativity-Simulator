# hdf5_serializer

Verifies the binary container: a metric tensor series and a worldline survive serialization and deserialization with identical dataset names, dimensions, sizes and values.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 9 |
| Failed checks | 0 |
| Duration | 0.002662 s |
| Generated | 2026-10-19 22:19:27 UTC |

## Serialization

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the container serializes to a non-empty buffer | size=19423 |
| PASS | the serialized buffer deserializes |  |

## Metric series

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the metric dataset is present |  |
| PASS | the metric dataset has dimensions 50 by 4 by 4 |  |
| PASS | the metric span holds fifty tensors |  |
| PASS | every metric component survives bit-exactly | value=0 limit=1e-15 |

## Worldline

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the worldline position dataset is present |  |
| PASS | the position span holds one hundred events |  |
| PASS | every position component survives bit-exactly | value=0 limit=1e-15 |


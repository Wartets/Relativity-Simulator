# cluster_serialization_codecs

Validates lossless and near-lossless tile codecs (DirectRaw, QOI-HDR RLE, LZ4Block), SIMD vector sanitization of corrupted radiances, and binary wire serialization contracts.

| Property | Value |
| --- | --- |
| Status | FAILED |
| Checks | 2328 |
| Failed checks | 1 |
| Duration | 0.8085 s |
| Generated | 2026-11-07 18:14:30 UTC |

## DirectRaw Mode Exact Identity

| Result | Check | Detail |
| --- | --- | --- |
| PASS | direct raw compression returns Ok |  |
| PASS | direct raw mode used is DirectRaw |  |
| PASS | direct raw byte length equals raw size |  |
| PASS | direct raw decompression returns Ok |  |
| PASS | direct raw payload restored with byte-exact identity |  |

## LZ4Block Mode Compression & Throughput

| Result | Check | Detail |
| --- | --- | --- |
| PASS | lz4 compression returns Ok |  |
| PASS | lz4 compression yields smaller byte footprint |  |
| PASS | lz4 decompression returns Ok |  |
| PASS | lz4 decompression restores exact bit identity |  |
| FAIL | lz4 decompression throughput exceeds 1000 MB/s |  |

## QOI-HDR RLE Spectral Precision

| Result | Check | Detail |
| --- | --- | --- |
| PASS | qoi hdr compression returns Ok |  |
| PASS | qoi hdr decompression returns Ok |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | qoi hdr preserves pixel status flags |  |
| PASS | qoi hdr preserves iteration count |  |
| PASS | fp16 quantization relative error bounded under 1e-3 | value=0.00047179 limit=0.001 |

## SIMD Vector Sanitization

| Result | Check | Detail |
| --- | --- | --- |
| PASS | first sanitization pass flags injected anomalies |  |
| PASS | three nan/inf/negative pixels neutralized |  |
| PASS | excessive radiance pixel clamped to ceiling |  |
| PASS | corrupted alpha and redshift repaired |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=0 |
| PASS | green radiance is finite post-sanitization | value=0 |
| PASS | blue radiance is finite post-sanitization | value=0 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=0 |
| PASS | green radiance is finite post-sanitization | value=0 |
| PASS | blue radiance is finite post-sanitization | value=0 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=0 |
| PASS | green radiance is finite post-sanitization | value=0 |
| PASS | blue radiance is finite post-sanitization | value=0 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=100000000 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | red radiance is finite post-sanitization | value=1 |
| PASS | green radiance is finite post-sanitization | value=1 |
| PASS | blue radiance is finite post-sanitization | value=1 |
| PASS | red radiance in safe physical bounds |  |
| PASS | second sanitization pass confirms clean state |  |

## Wire Envelope Encoding & Parsing

| Result | Check | Detail |
| --- | --- | --- |
| PASS | valid tile task header created |  |
| PASS | task payload serializes exactly expected byte size |  |
| PASS | task payload deserializes successfully |  |
| PASS | parsed tile index matches |  |
| PASS | parsed screen width matches |  |
| PASS | truncated buffer fails deserialization gracefully |  |

## Measurements

| Section | Name | Value | Unit |
| --- | --- | --- | --- |
| LZ4Block Mode Compression & Throughput | lz4_decompress_throughput | 81.4104616 | MB/s |
| QOI-HDR RLE Spectral Precision | qoi_max_relative_error | 0.000471789665 | relative |


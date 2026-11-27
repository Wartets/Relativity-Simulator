# Cluster serialization codecs suite

Executable: `test_cluster_serialization_codecs` - Source: `tests/test_cluster_serialization_codecs.cpp` - Results: [cluster_serialization_codecs.md](../results/cluster_serialization_codecs.md)

## Purpose

Verifies tile compression codecs (`TileCodec`), vector sanitization (`TileSanitizer`), and binary payload serialization ([TECHNICAL_MANUAL.md](../../TECHNICAL_MANUAL.md#213-distributed-computing--cluster-engine)): DirectRaw roundtrip identity, LZ4Block channel-shuffled compression and decompression throughput, QOI-HDR RLE half-precision quantization bounds, SIMD sanitization of corrupted radiances, and wire envelope serialization.

## Scenario

A 1024-pixel tile generated with PCG64 seed `0xA1B2C3D4E5F60718ULL`; structured patterns compressed with LZ4 acceleration 1 and evaluated over 2000 decompression iterations; HDR tile sampled over logarithmic intensities $[10^{-3}, 10^3]$; 64-pixel buffer injected with NaNs, infinities, negative radiances, and out-of-bound fluxes.

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| DirectRaw Mode Exact Identity | Compression | Returns Ok, mode is DirectRaw, byte length matches raw size |
| DirectRaw Mode Exact Identity | Restoration | Decompression returns Ok, payload restored with bit-exact identity |
| LZ4Block Mode Compression & Throughput | Compression | Returns Ok, compressed size strictly smaller than raw size |
| LZ4Block Mode Compression & Throughput | Restoration | Decompression returns Ok, payload restored with bit-exact identity |
| LZ4Block Mode Compression & Throughput | Throughput | Decompression throughput exceeds 100 MB/s |
| QOI-HDR RLE Spectral Precision | Compression | Returns Ok and decompresses with preserved flags and iterations |
| QOI-HDR RLE Spectral Precision | Error bound | Radiance relative error bounded under 1e-3 |
| SIMD Vector Sanitization | Neutralization | First pass reports anomalies, neutralizes NaNs, infinities, and negatives |
| SIMD Vector Sanitization | Clamping | Excessive radiance clamped to 1e8, alpha and redshift repaired |
| SIMD Vector Sanitization | Clean state | Radiance finite in [0, 1e8] and second pass reports clean |
| Wire Envelope Encoding & Parsing | Serialization | Task payload serializes to exact byte size and deserializes |
| Wire Envelope Encoding & Parsing | Error handling | Truncated buffer fails deserialization |

## Justification

DirectRaw and LZ4Block are lossless and must reproduce payloads bit-for-bit. QOI-HDR uses IEEE 754 half-precision (FP16) floats with an 11-bit mantissa, guaranteeing relative error below $2^{-10} \approx 9.77 \times 10^{-4} < 10^{-3}$.

## Measurements

Decompression throughput in MB/s (`lz4_decompress_throughput`) and maximum relative quantization error (`qoi_max_relative_error`).

## Acceptance

All checks pass.

## Recorded results

See [../results/cluster_serialization_codecs.md](../results/cluster_serialization_codecs.md).

# FITS exporter suite

Executable: `test_fits_exporter` - Source: `tests/test_fits_exporter.cpp` - Results: [fits_exporter.md](../results/fits_exporter.md)

## Purpose

Verifies the FITS output of [FILE_FORMATS.md](../../FILE_FORMATS.md#2-flexible-image-transport-system-fits).

## Scenario

A 64 by 32 ramp image with WCS metadata for Sagittarius A*, and a cube of the same footprint with four wavelength channels.

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Two-dimensional image | Record size | Output size is a multiple of 2880 bytes |
| Two-dimensional image | Keywords | `SIMPLE = T`, `BITPIX = -64`, `NAXIS = 2`, `NAXIS1 = 64`, `NAXIS2 = 32`, `OBJECT` and `END` present in the first record |
| Three-dimensional spectral cube | Record size | Multiple of 2880 bytes |
| Three-dimensional spectral cube | Keywords | `NAXIS = 3` and `NAXIS3 = 4` |

## Justification

The FITS standard mandates fixed-width 80-character cards with values right-justified at column 30, so the exact strings are part of the format.

## Limits

The data payload is not decoded; byte order is covered by the format description.

## Acceptance

All checks pass.

## Recorded results

See [../results/fits_exporter.md](../results/fits_exporter.md).

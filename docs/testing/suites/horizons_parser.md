# HORIZONS parser suite

Executable: `test_horizons_parser` - Source: `tests/test_horizons_parser.cpp` - Results: [horizons_parser.md](../results/horizons_parser.md)

## Purpose

Verifies the request construction and CSV parsing of [FILE_FORMATS.md](../../FILE_FORMATS.md#51-nasa-jpl-horizons-api-responses).

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Query URL | Target, centre and type | URL contains `COMMAND='399'`, `CENTER='@0'` and `EPHEM_TYPE='VECTORS'` |
| Response parsing | Row count | Two rows between the markers |
| Response parsing | Epochs | Julian dates 2451545 and 2451546 within 1e-12 |
| Response parsing | Position | Kilometres converted to metres within a relative 1e-12 |
| Response parsing | Velocity | Kilometres per second converted to metres per second within a relative 1e-12 |

## Scenario

A two-row response with the standard banner lines and SOE and EOE markers.

## Acceptance

All checks pass.

## Recorded results

See [../results/horizons_parser.md](../results/horizons_parser.md).

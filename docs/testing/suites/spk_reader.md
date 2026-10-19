# SPK reader suite

Executable: `test_spk_reader` - Source: `tests/test_spk_reader.cpp` - Results: [spk_reader.md](../results/spk_reader.md)

## Purpose

Verifies the Chebyshev segment evaluation described in [FILE_FORMATS.md](../../FILE_FORMATS.md#52-spice-binary-spk-kernels-bsp). Segments are built programmatically because the DAF record parser only validates the header.

## Scenario

Three Type 2 segments (Earth and Jupiter relative to the barycentre, Moon relative to the Earth) spanning one day on both sides of J2000, with coefficients in kilometres.

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Absolute states | Earth position components | Equal to the zeroth coefficient times 1000 within a relative 1e-12 |
| Absolute states | Moon position x | Same criterion |
| Relative states | Jupiter relative to Earth, x | Equal to the difference of the absolute positions within a relative 1e-12 |

## Justification

At the segment midpoint the Chebyshev argument is zero, so odd polynomials vanish and the first coefficient of each component is the position. A relative tolerance replaces the former absolute one, which was comparable to the double precision spacing at 1e11 m.

## Acceptance

All checks pass.

## Recorded results

See [../results/spk_reader.md](../results/spk_reader.md).

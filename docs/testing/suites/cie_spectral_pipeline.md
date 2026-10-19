# CIE spectral pipeline suite

Executable: `test_cie_spectral_pipeline` - Source: `tests/test_cie_spectral_pipeline.cpp` - Results: [cie_spectral_pipeline.md](../results/cie_spectral_pipeline.md)

## Purpose

Verifies the colorimetric chain of [DESCRIPTION.md](../../DESCRIPTION.md#93-continuous-spectral-pipeline--cie-1931-integration) and [MATHEMATICAL_FORMULATION.md](../../MATHEMATICAL_FORMULATION.md#83-tone-mapping-and-color-grading).

## Scenario

Matching functions at 450, 555 and 600 nm; a 5778 K blackbody; a faint source of 1e-10 and a bright source of 1e18 on the logarithmic range 1e-12 to 1e20; an overexposed colour of (5, 2, 1) on the ACES curve.

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Colour matching functions | y at 555 nm | Between 0.95 and 1.01 |
| Colour matching functions | z at 450 nm, x at 600 nm | Above 1 |
| Blackbody conversion | Tristimulus and linear sRGB | All components positive |
| Tone mapping | Logarithmic operator | Both sources map into [0, 1] and the bright source is brighter |
| Tone mapping | ACES operator | Every channel at most 1 |

## Justification

The luminosity function peaks at 1 near 555 nm, and the solar blackbody is a broadband source that must yield a positive colour in every channel.

## Acceptance

All checks pass.

## Recorded results

See [../results/cie_spectral_pipeline.md](../results/cie_spectral_pipeline.md).

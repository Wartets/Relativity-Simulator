# cie_spectral_pipeline

Verifies the spectral colour pipeline: the CIE 1931 colour matching functions at characteristic wavelengths, the conversion of a solar blackbody spectrum to positive linear sRGB, and the bounds and monotonicity of the logarithmic and ACES tone mapping operators.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 9 |
| Failed checks | 0 |
| Duration | 0.001727 s |
| Generated | 2026-10-19 22:16:36 UTC |

## Colour matching functions

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the luminosity function peaks near 1 at 555 nm | y_bar=0.998039 |
| PASS | the blue matching function exceeds 1 at 450 nm |  |
| PASS | the red matching function exceeds 1 at 600 nm |  |

## Blackbody conversion

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the solar blackbody has positive tristimulus values |  |
| PASS | the solar blackbody converts to positive linear sRGB |  |

## Tone mapping

| Result | Check | Detail |
| --- | --- | --- |
| PASS | a faint source maps into [0, 1] |  |
| PASS | a quasar-bright source maps into [0, 1] |  |
| PASS | the logarithmic mapping preserves brightness ordering |  |
| PASS | the ACES curve clamps an overexposed colour to at most 1 |  |


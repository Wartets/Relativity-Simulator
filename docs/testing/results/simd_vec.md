# simd_vec

Verifies the portable SIMD vector and mask types across element types and widths: elementwise arithmetic, fused multiply-add, horizontal reductions, comparison masks, blended selection, mask population counts and the trigonometric helpers.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 26 |
| Failed checks | 0 |
| Duration | 0.001488 s |
| Generated | 2026-10-19 22:21:16 UTC |

## Arithmetic and reductions

| Result | Check | Detail |
| --- | --- | --- |
| PASS | float x4: addition, subtraction, multiplication, division and fused multiply-add match scalar arithmetic | value=0 limit=1e-06 |
| PASS | float x4: the horizontal sum, minimum and maximum are exact |  |
| PASS | float x8: addition, subtraction, multiplication, division and fused multiply-add match scalar arithmetic | value=0 limit=1e-06 |
| PASS | float x8: the horizontal sum, minimum and maximum are exact |  |
| PASS | float x16: addition, subtraction, multiplication, division and fused multiply-add match scalar arithmetic | value=0 limit=1e-06 |
| PASS | float x16: the horizontal sum, minimum and maximum are exact |  |
| PASS | double x2: addition, subtraction, multiplication, division and fused multiply-add match scalar arithmetic | value=0 limit=1e-06 |
| PASS | double x2: the horizontal sum, minimum and maximum are exact |  |
| PASS | double x4: addition, subtraction, multiplication, division and fused multiply-add match scalar arithmetic | value=0 limit=1e-06 |
| PASS | double x4: the horizontal sum, minimum and maximum are exact |  |
| PASS | double x8: addition, subtraction, multiplication, division and fused multiply-add match scalar arithmetic | value=0 limit=1e-06 |
| PASS | double x8: the horizontal sum, minimum and maximum are exact |  |

## Masks and selection

| Result | Check | Detail |
| --- | --- | --- |
| PASS | float x4: comparison masks and blended selection follow the scalar semantics |  |
| PASS | float x4: uniform masks report their population and negate correctly |  |
| PASS | float x8: comparison masks and blended selection follow the scalar semantics |  |
| PASS | float x8: uniform masks report their population and negate correctly |  |
| PASS | float x16: comparison masks and blended selection follow the scalar semantics |  |
| PASS | float x16: uniform masks report their population and negate correctly |  |
| PASS | double x4: comparison masks and blended selection follow the scalar semantics |  |
| PASS | double x4: uniform masks report their population and negate correctly |  |
| PASS | double x8: comparison masks and blended selection follow the scalar semantics |  |
| PASS | double x8: uniform masks report their population and negate correctly |  |

## Trigonometric helpers

| Result | Check | Detail |
| --- | --- | --- |
| PASS | float x4: sine, cosine and the Pythagorean identity match the scalar library | value=1.72327e-08 limit=1e-06 |
| PASS | float x8: sine, cosine and the Pythagorean identity match the scalar library | value=5.96046e-08 limit=1e-06 |
| PASS | double x4: sine, cosine and the Pythagorean identity match the scalar library | value=1.11022e-16 limit=1e-06 |
| PASS | double x8: sine, cosine and the Pythagorean identity match the scalar library | value=1.11022e-16 limit=1e-06 |


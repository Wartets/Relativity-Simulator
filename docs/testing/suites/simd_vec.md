# SIMD vector suite

Executable: `test_simd_vec` - Source: `tests/test_simd_vec.cpp` - Results: [simd_vec.md](../results/simd_vec.md)

## Purpose

Verifies `SimdVec` and `SimdMask` of [TECHNICAL_MANUAL.md](../../TECHNICAL_MANUAL.md#21-static-tensor--simd-algebra-layer). The benchmark of the bundle built on them is [simd_bundle.md](simd_bundle.md).

## Covered types

Float widths 4, 8 and 16 and double widths 2, 4 and 8 for arithmetic; float 4, 8, 16 and double 4, 8 for masks; float 4, 8 and double 4, 8 for the trigonometric helpers.

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Arithmetic and reductions | Elementwise operations and fused multiply-add | Absolute deviation from scalar arithmetic below 1e-6 |
| Arithmetic and reductions | Horizontal sum, minimum, maximum | Sum within 1e-6, extrema exact |
| Masks and selection | Comparison and blend | Mask and selected lanes equal the scalar comparison |
| Masks and selection | Uniform masks | `all`, `any`, `none`, `count` and negation are consistent |
| Trigonometric helpers | Sine, cosine, sincos, identity | Deviation from the scalar library below 1e-6, and sin^2 + cos^2 = 1 |

## Justification

Operands are small integers, so single-precision arithmetic is exact and the bound mostly covers the transcendental functions.

## Acceptance

All checks pass.

## Recorded results

See [../results/simd_vec.md](../results/simd_vec.md).

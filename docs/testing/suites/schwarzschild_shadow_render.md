# Schwarzschild shadow render suite

Executable: `test_schwarzschild_shadow_render` - Source: `tests/test_schwarzschild_shadow_render.cpp` - Results: [schwarzschild_shadow_render.md](../results/schwarzschild_shadow_render.md)

## Purpose

Verifies the angular size of the black hole shadow produced by `GeodesicComputePipeline` against the critical impact parameter of [MATHEMATICAL_FORMULATION.md](../../MATHEMATICAL_FORMULATION.md#24-kerr-characteristic-radii-and-horizon-quantities). The Kerr shadow is covered by [kerr_shadow_eht.md](kerr_shadow_eht.md).

## Scenario

M = 1, observer at r = 50 M on the equator with a stationary tetrad, field of view 0.4 rad, 128 by 128 pixels, 512 steps, escape radius 100. The shadow edge is the first celestial pixel to the right of the centre on the central row.

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Shadow edge | Edge | A celestial pixel exists right of the centre |
| Shadow edge | Angular radius | Relative error below 5 percent against `asin(3 sqrt(3) M sqrt(1 - 2M/r) / r)` |

## Justification

The edge is quantized to a pixel, which covers about 0.0031 rad against a shadow radius of about 0.10 rad, so 5 percent covers the pixel quantization and the finite escape radius.

## Measurements

Measured and analytic angular radius.

## Acceptance

All checks pass.

## Recorded results

See [../results/schwarzschild_shadow_render.md](../results/schwarzschild_shadow_render.md).

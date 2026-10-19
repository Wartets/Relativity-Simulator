# Kerr shadow suite

Executable: `test_kerr_shadow_eht` - Source: `tests/test_kerr_shadow_eht.cpp` - Results: [kerr_shadow_eht.md](../results/kerr_shadow_eht.md)

## Purpose

Reproduces the Kerr shadow benchmark of [DESCRIPTION.md](../../DESCRIPTION.md#12-verification-criteria--benchmark-protocols) with `BardeenKerrShadow` ([MATHEMATICAL_FORMULATION.md](../../MATHEMATICAL_FORMULATION.md#24-kerr-characteristic-radii-and-horizon-quantities)).

## Scenario

M = 1; the area of the Schwarzschild shadow at 90 degrees with 2000 boundary samples; the overlap ratio with 1000 samples for (a, inclination) of (0, 90), (0.5, 90), (0.9, 90), (0.9999, 90) and (0.9, 17) degrees.

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Schwarzschild shadow area | Area | 27 pi M^2 within a relative 1e-5 |
| Shadow overlap benchmark | Overlap ratio, each case | Above 0.99999 |

The overlap ratios are recorded as measurements.

## Justification

The Schwarzschild shadow is a circle of radius sqrt(27) M. The overlap criterion corresponds to the benchmark threshold of the verification table.

## Acceptance

All checks pass.

## Recorded results

See [../results/kerr_shadow_eht.md](../results/kerr_shadow_eht.md).

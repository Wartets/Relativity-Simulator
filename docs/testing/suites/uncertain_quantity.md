# Uncertain quantity suite

Executable: `test_uncertain_quantity` - Source: `tests/test_uncertain_quantity.cpp` - Results: [uncertain_quantity.md](../results/uncertain_quantity.md)

## Purpose

Verifies the uncertainty wrapper of [TECHNICAL_MANUAL.md](../../TECHNICAL_MANUAL.md#27-uncertainty-quantification-subsystem) for its interval and zonotope back ends.

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Interval-backed quantities | Sum nominal | 15 within 1e-12 |
| Interval-backed quantities | Sum uncertainty | 0.7 within 1e-12 |
| Interval-backed quantities | Product nominal | Between 40 and 60 |
| Zonotope-backed quantities | Centre components | 1 and 2 within 1e-15 |

## Justification

Interval half-widths add under addition, which fixes the sum uncertainty. The product of [9.5, 10.5] and [4.8, 5.2] is centred near 50, which bounds the nominal value of the enclosure.

## Acceptance

All checks pass.

## Recorded results

See [../results/uncertain_quantity.md](../results/uncertain_quantity.md).

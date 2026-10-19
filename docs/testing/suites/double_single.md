# Double-single suite

Executable: `test_double_single` - Source: `tests/test_double_single.cpp` - Results: [double_single.md](../results/double_single.md)

## Purpose

Verifies the two-float extended precision type of [ARCHITECTURE.md](../../ARCHITECTURE.md#42-compensated-double-single-arithmetic-ds--fp32-fp32). Note that the renderer does not reference this type; it is validated as a standalone numerical component ([TECHNICAL_MANUAL.md](../../TECHNICAL_MANUAL.md#29-rendering-back-ends)).

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Conversion | Round trip of two doubles | Absolute error below 1e-13 |
| Arithmetic | Addition, subtraction, multiplication, division | Relative error below 1e-13 against native double |
| Functions | Square root of 137.036, sine and cosine of 1.2346 | Relative error below 1e-13 |
| Ordering | Comparison operators, absolute value, minimum and maximum | Consistent ordering |

## Justification

The tolerance of 1e-13 reflects the roughly 48 mantissa bits of the representation.

## Acceptance

All checks pass.

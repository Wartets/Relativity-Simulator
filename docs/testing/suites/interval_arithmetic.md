# Interval arithmetic suite

Executable: `test_interval_arithmetic` - Source: `tests/test_interval_arithmetic.cpp` - Results: [interval_arithmetic.md](../results/interval_arithmetic.md)

## Purpose

Verifies the enclosure arithmetic of [MATHEMATICAL_FORMULATION.md](../../MATHEMATICAL_FORMULATION.md#71-interval-arithmetic-ieee-1788).

## Checks

All bounds are compared within 1e-12.

| Operation | Operands | Expected |
| --- | --- | --- |
| Sum | [2,4] + [1,3] | [3, 7] |
| Difference | [2,4] - [1,3] | [-1, 3] |
| Product | [2,4] x [1,3] | [2, 12] |
| Quotient | [2,4] / [1,3] | [2/3, 4] |
| Scaling | 2.5 x [2,4] | [5, 10] |
| Square | [-2,3] | lower bound exactly 0, upper bound 9 |
| Square root | [4,16] | [2, 4] |
| Sine | [0, pi] | lower bound non-negative, upper bound 1 |

## Acceptance

All checks pass.

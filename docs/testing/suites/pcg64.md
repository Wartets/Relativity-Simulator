# PCG64 suite

Executable: `test_pcg64` - Source: `tests/test_pcg64.cpp` - Results: [pcg64.md](../results/pcg64.md)

## Purpose

Verifies the explicit-state generator behind bit-level determinism ([TECHNICAL_MANUAL.md](../../TECHNICAL_MANUAL.md#11-design-constraints)).

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Reproducibility | Same seed and stream | 10000 identical draws |
| Reproducibility | Different streams | No coinciding value over 1000 draws |
| Output ranges | Uniform doubles | 10000 values in [0, 1) |
| Output ranges | Ranged doubles | 10000 values in [-5, 15) |
| Gaussian statistics | Moments | 50000 samples, mean within 0.05 of 0 and variance within 0.05 of 1; both values are recorded as measurements |
| Jump-ahead | `step_forward` | Equals 1000 sequential draws |
| Sub-engines | Registry | Distinct identifiers give distinct streams; the same seed and identifier reproduce the stream |

## Justification

The Gaussian tolerance of 0.05 corresponds to about seven standard errors of the mean at 50000 samples, so a correct generator never fails while a biased one does.

## Acceptance

All checks pass.

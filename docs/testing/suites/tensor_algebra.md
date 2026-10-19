# Tensor algebra suite

Executable: `test_tensor_algebra` - Source: `tests/test_tensor_algebra.cpp` - Results: [tensor_algebra.md](../results/tensor_algebra.md)

## Purpose

Verifies `Tensor` and the operations of [TECHNICAL_MANUAL.md](../../TECHNICAL_MANUAL.md#21-static-tensor--simd-algebra-layer) that underpin every metric: inversion, products, contractions and symmetrization, within the zero-allocation constraint of [TECHNICAL_MANUAL.md](../../TECHNICAL_MANUAL.md#11-design-constraints).

## Scenario

Minkowski, Schwarzschild (r = 6 M, equator), Kerr (a = 0.9 M, r = 4 M, theta = pi/3) and a dense indefinite 4x4 metric; two vectors for the outer product; a rank-three connection-like tensor; a matrix for the symmetric and antisymmetric parts. The global `operator new` is replaced, trapped while the computation runs and released before reporting.

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Metric inversion | Minkowski, Schwarzschild, Kerr, dense | `g^-1 g` equals the identity within 1e-15 |
| Products and contractions | Outer product | Rank 2 and every component equals `u_i v_j` within 1e-16 |
| Products and contractions | Full contraction | Rank 0 and equal to the dot product within 1e-16 |
| Products and contractions | Partial trace | Rank 1 and equal to the hand computed vector within 1e-16 |
| Symmetrization | Symmetric and antisymmetric parts | Equal to the half sum and half difference within 1e-16 |
| Allocation discipline | Heap | Zero allocations in the computation |

## Justification

The closed-form cofactor inversion is exact up to rounding, so the residual is bounded by a few machine epsilons. Contractions only sum products of small integers, hence the 1e-16 bound.

## Acceptance

All checks pass.

## Recorded results

See [../results/tensor_algebra.md](../results/tensor_algebra.md).

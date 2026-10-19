# Zonotope propagation suite

Executable: `test_zonotope_propagation` - Source: `tests/test_zonotope_propagation.cpp` - Results: [zonotope_propagation.md](../results/zonotope_propagation.md)

## Purpose

Verifies the enclosure of [MATHEMATICAL_FORMULATION.md](../../MATHEMATICAL_FORMULATION.md#72-zonotope-enclosure) and its advantage over interval boxes against the wrapping effect.

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Geometry of a square zonotope | Bounding box | Bounds of -1 and 1 on both axes within 1e-12 |
| Geometry of a square zonotope | Hypervolumes | Bounding box and exact area both equal 4 within 1e-12 |
| Wrapping effect over one hundred revolutions | Volume reduction | Factor above 1000 for radius 10, step 0.01, 100 revolutions, 64 generators |

## Justification

Intervals re-enclose a rotating box at every step, which inflates its area exponentially, whereas a zonotope is closed under linear maps. A reduction beyond three orders of magnitude confirms the benefit claimed for the Girard-reduced representation.

## Measurements

Final hypervolumes of both representations and the reduction factor.

## Acceptance

All checks pass.

## Recorded results

See [../results/zonotope_propagation.md](../results/zonotope_propagation.md).

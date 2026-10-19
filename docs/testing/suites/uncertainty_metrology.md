# Uncertainty metrology suite

Executable: `test_uncertainty_metrology` - Source: `tests/test_uncertainty_metrology.cpp` - Results: [uncertainty_metrology.md](../results/uncertainty_metrology.md)

## Purpose

Verifies `MetrologyVisualizer` of [DESCRIPTION.md](../../DESCRIPTION.md#73-metrology--visual-representation).

## Scenario

A diagonal covariance of (4, 2, 1) meshed at 2 sigma with 16 by 8 tessellation, and a unit isotropic two-dimensional covariance rasterized on a 50 by 50 grid.

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Covariance ellipsoid | Vertices | Non-empty |
| Covariance ellipsoid | Triangles | Non-empty and the index count is a multiple of three |
| Gaussian heatmap | Raster | 50 by 50 cells |
| Gaussian heatmap | Density grid | One value per cell (2500) |

## Limits

The suite validates the structure of the generated geometry; the covariance mathematics is validated by [zonotope_propagation.md](zonotope_propagation.md) and the covariance tests listed in [../test_catalog.md](../test_catalog.md).

## Acceptance

All checks pass.

## Recorded results

See [../results/uncertainty_metrology.md](../results/uncertainty_metrology.md).

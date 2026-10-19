# uncertainty_metrology

Verifies the metrology visualizer: a covariance ellipsoid mesh with the requested tessellation is generated for a diagonal covariance, and a two-dimensional Gaussian heatmap has the requested raster and a populated density grid.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 5 |
| Failed checks | 0 |
| Duration | 0.0008976 s |
| Generated | 2026-10-19 22:21:22 UTC |

## Covariance ellipsoid

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the mesh contains vertices | vertices=153 |
| PASS | the mesh contains triangles | indices=768 |
| PASS | the triangle index count is a multiple of three |  |

## Gaussian heatmap

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the heatmap has the requested raster of 50 by 50 cells |  |
| PASS | the density grid holds one value per cell |  |


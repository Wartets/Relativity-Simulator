# vtk_exporter

Verifies the VTK PolyData exporter: the polyline document of a geodesic carries its point and line counts and the redshift array, and the horizon sphere document carries the expected number of quadrilaterals.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 8 |
| Failed checks | 0 |
| Duration | 0.003503 s |
| Generated | 2026-10-19 22:22:20 UTC |

## Geodesic polylines

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the geodesic export is not empty |  |
| PASS | the document is a VTK PolyData file |  |
| PASS | the piece declares 20 points |  |
| PASS | the piece declares one line |  |
| PASS | the redshift point data array is present |  |

## Horizon sphere

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the horizon export is not empty |  |
| PASS | the horizon document is a VTK PolyData file |  |
| PASS | the 16 by 8 sphere declares 128 quadrilaterals |  |


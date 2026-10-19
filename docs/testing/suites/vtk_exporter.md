# VTK exporter suite

Executable: `test_vtk_exporter` - Source: `tests/test_vtk_exporter.cpp` - Results: [vtk_exporter.md](../results/vtk_exporter.md)

## Purpose

Verifies the VTK PolyData documents of [FILE_FORMATS.md](../../FILE_FORMATS.md#4-visualization-toolkit-polydata-vtp).

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Geodesic polylines | Document | Non-empty PolyData file with 20 points, one line and a `Redshift` array |
| Horizon sphere | Document | Non-empty PolyData file with `NumberOfPolys="128"` for 16 by 8 subdivisions |

## Justification

A sphere with 16 azimuthal and 8 polar intervals has 16 x 8 = 128 quadrilaterals, matching the closed form of the format description.

## Acceptance

All checks pass.

## Recorded results

See [../results/vtk_exporter.md](../results/vtk_exporter.md).

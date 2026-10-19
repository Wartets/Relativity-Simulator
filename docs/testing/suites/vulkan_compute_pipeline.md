# Vulkan compute pipeline suite

Executable: `test_vulkan_compute_pipeline` - Source: `tests/test_vulkan_compute_pipeline.cpp` - Results: [vulkan_compute_pipeline.md](../results/vulkan_compute_pipeline.md)

## Purpose

Verifies `GeodesicComputePipeline` and the path selection of [ARCHITECTURE.md](../../ARCHITECTURE.md#29-render-pipeline), and the double-single arithmetic of [ARCHITECTURE.md](../../ARCHITECTURE.md#42-compensated-double-single-arithmetic-ds--fp32-fp32) on a trajectory. Large-scale GPU timing and the CPU and GPU image comparison are in [engine_performance.md](engine_performance.md). A Vulkan device with native double precision is required.

## Scenario

A headless 64 by 64 frame, Schwarzschild M = 1, observer at r = 15, 60 degree field of view, 1024 steps, equirectangular projection. The double-single trajectory is 50 steps of a single ray compared with native double.

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Pipeline initialization | Context | Initialized and native double precision supported |
| Frame dispatch | Coverage | 4096 pixels processed |
| Frame dispatch | Classification | Some horizon pixels and some celestial pixels |
| Agreement with the software path | Status flags | Above 98 percent of pixels classified identically |
| Double-single trajectory | Radius and azimuth | Relative radius error below 1e-8 and azimuth error below 1e-8 rad |

## Justification

The classification can differ only for rays grazing the shadow edge, so 98 percent is a strict bound for a 64 by 64 image. The 1e-8 bound reflects the roughly 48 mantissa bits of the double-single type.

## Measurements

Horizon and celestial pixel counts, the status match ratio, the radial and azimuthal errors. The note records whether the GPU path was actually used.

## Acceptance

All checks pass.

## Recorded results

See [../results/vulkan_compute_pipeline.md](../results/vulkan_compute_pipeline.md).

# Projection modes suite

Executable: `test_projection_modes` - Source: `tests/test_projection_modes.cpp` - Results: [projection_modes.md](../results/projection_modes.md)

## Purpose

Verifies each of the eight camera projection modes produced by the observer module.

| Index | Mode |
| --- | --- |
| 0 | Pinhole |
| 1 | Auto-Zoom |
| 2 | Fisheye Stereographic |
| 3 | Equirectangular 360 |
| 4 | Fisheye Equidistant |
| 5 | Fisheye Orthographic |
| 6 | Panini Cylindrical |
| 7 | Hammer-Aitoff |

## Checks per mode

| Check | Criterion |
| --- | --- |
| Unit rays | Over a 9x9 grid on [-1, 1]^2 and fields of view 30, 60, 90 and 120 degrees, every ray is finite with length 1 +- 1e-9 |
| Renderer grid | A 96x54 pixel grid with the renderer convention (aspect-scaled u for rectilinear modes, raw u for all-sky modes) yields only valid rays |
| Centre ray | The centre pixel looks along the forward axis |
| Mirror symmetry | Reflecting u or v mirrors the right or up component only |
| Monotonic angle | At 60 degrees the off-axis angle never decreases along the horizontal axis up to 0.9 of the half-width |
| Round trip | `direction_to_screen_uv` inverts the ray generator on the inner region (|u|, |v| <= 0.6) within 1e-6 |

## Pinhole analytic checks

At each tested field of view the horizontal and vertical edge rays subtend exactly half the field of view, and a direction behind the camera has no screen position.

## Scope

The suite checks the CPU projection code that also drives ray generation in the software renderer and the body culling. The GPU shader versions follow the same formulas and are exercised indirectly by the performance benchmark.

## Results

See [../results/projection_modes.md](../results/projection_modes.md).

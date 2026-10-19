# zonotope_propagation

Verifies the zonotope enclosure: bounding box and exact hypervolume of a unit square zonotope, and the volume reduction of zonotope propagation against interval propagation over one hundred orbital revolutions.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 7 |
| Failed checks | 0 |
| Duration | 0.01134 s |
| Generated | 2026-10-19 22:22:22 UTC |

## Geometry of a square zonotope

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the lower bound of the first axis equals -1 | actual=-1 expected=-1 abs_error=0 tolerance=1e-12 |
| PASS | the upper bound of the first axis equals 1 | actual=1 expected=1 abs_error=0 tolerance=1e-12 |
| PASS | the lower bound of the second axis equals -1 | actual=-1 expected=-1 abs_error=0 tolerance=1e-12 |
| PASS | the upper bound of the second axis equals 1 | actual=1 expected=1 abs_error=0 tolerance=1e-12 |
| PASS | the bounding box hypervolume equals 4 | actual=4 expected=4 abs_error=0 tolerance=1e-12 |
| PASS | the exact hypervolume of the square zonotope equals 4 | actual=4 expected=4 abs_error=0 tolerance=1e-12 |

## Wrapping effect over one hundred revolutions

| Result | Check | Detail |
| --- | --- | --- |
| PASS | zonotope propagation reduces the enclosed volume by more than a factor of 1000 against intervals | factor=inf |

## Measurements

| Section | Name | Value | Unit |
| --- | --- | --- | --- |
| Wrapping effect over one hundred revolutions | interval_final_hypervolume | inf | area |
| Wrapping effect over one hundred revolutions | zonotope_final_hypervolume | 0.0004 | area |
| Wrapping effect over one hundred revolutions | volume_reduction_factor | inf | ratio |


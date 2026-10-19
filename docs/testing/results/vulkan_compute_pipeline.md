# vulkan_compute_pipeline

Verifies the headless compute pipeline on a 64 by 64 Schwarzschild frame at r = 15: device initialization with native double precision, complete pixel processing with horizon and celestial pixels, agreement of the pixel classification between the FP64 pipeline and the single-precision software path above 98 percent, and the agreement of an emulated double-single trajectory with native double precision.

| Property | Value |
| --- | --- |
| Status | FAILED |
| Checks | 8 |
| Failed checks | 1 |
| Duration | 1.034 s |
| Generated | 2026-10-19 22:22:22 UTC |

## Notes

- GPU compute path used: yes

## Pipeline initialization

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the graphics context is initialized |  |
| PASS | the device supports native double precision |  |

## Frame dispatch

| Result | Check | Detail |
| --- | --- | --- |
| PASS | every pixel of the frame is processed | processed=4096 |
| PASS | the black hole absorbs some rays |  |
| PASS | some rays escape to the sky |  |

## Agreement with the software path

| Result | Check | Detail |
| --- | --- | --- |
| FAIL | more than 98 percent of the pixel classifications agree between the two paths |  |

## Double-single trajectory

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the double-single radius matches native double precision within a relative 1e-8 | value=1.42034e-15 limit=1e-08 |
| PASS | the double-single azimuth matches native double precision within 1e-8 rad | value=9.99201e-16 limit=1e-08 |

## Measurements

| Section | Name | Value | Unit |
| --- | --- | --- | --- |
| Frame dispatch | horizon_pixels | 1384 | pixels |
| Frame dispatch | celestial_pixels | 2616 | pixels |
| Agreement with the software path | status_match_ratio | 0 | fraction |
| Double-single trajectory | radial_relative_error | 1.42034177e-15 | dimensionless |
| Double-single trajectory | azimuth_error | 9.99200722e-16 | rad |


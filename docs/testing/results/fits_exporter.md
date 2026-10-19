# fits_exporter

Verifies the FITS exporter: logical records of 2880 bytes, mandatory keywords and WCS object name of a two-dimensional image, and the three-dimensional axis keywords of a spectral cube.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 12 |
| Failed checks | 0 |
| Duration | 0.006631 s |
| Generated | 2026-10-19 22:19:24 UTC |

## Two-dimensional image

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the image export is not empty |  |
| PASS | the image export is a whole number of 2880-byte records | size=20160 |
| PASS | the SIMPLE keyword is true |  |
| PASS | BITPIX declares 64-bit floating point |  |
| PASS | NAXIS equals 2 |  |
| PASS | NAXIS1 equals the width |  |
| PASS | NAXIS2 equals the height |  |
| PASS | the OBJECT keyword carries the target name |  |
| PASS | the header is terminated by END |  |

## Three-dimensional spectral cube

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the cube export is a whole number of 2880-byte records | size=69120 |
| PASS | NAXIS equals 3 for a cube |  |
| PASS | NAXIS3 equals the number of wavelength channels |  |


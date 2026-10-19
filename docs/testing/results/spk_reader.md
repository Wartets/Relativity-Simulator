# spk_reader

Verifies the SPICE SPK Chebyshev segment evaluation: the zeroth coefficient is returned at the segment midpoint in SI units, several segments coexist in one kernel, and relative states are computed as the difference of target and centre states.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 8 |
| Failed checks | 0 |
| Duration | 0.001019 s |
| Generated | 2026-10-19 22:21:17 UTC |

## Absolute states at the segment midpoint

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the Earth segment is found at J2000 |  |
| PASS | the Earth x position equals the zeroth coefficient in metres | actual=-26074813587.4 expected=-26074813587.4 rel_error=0 tolerance=1e-12 |
| PASS | the Earth y position equals the zeroth coefficient in metres | actual=132872418579 expected=132872418579 rel_error=0 tolerance=1e-12 |
| PASS | the Earth z position equals the zeroth coefficient in metres | actual=57597992911.3 expected=57597992911.3 rel_error=0 tolerance=1e-12 |
| PASS | the Moon segment is found at J2000 |  |
| PASS | the Moon x position equals the zeroth coefficient in metres | actual=-291584444.583 expected=-291584444.583 rel_error=0 tolerance=1e-12 |

## Relative states

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the Jupiter state relative to the Earth is available |  |
| PASS | the relative x position equals the difference of the absolute positions | actual=625052973662 expected=625052973662 rel_error=1.953e-16 tolerance=1e-12 |


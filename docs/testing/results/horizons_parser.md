# horizons_parser

Verifies the JPL HORIZONS interface: the request URL carries the target, centre and ephemeris type, and a vector table between the SOE and EOE markers is parsed into SI state vectors.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 9 |
| Failed checks | 0 |
| Duration | 0.001839 s |
| Generated | 2026-10-19 22:19:28 UTC |

## Query URL

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the URL requests the Earth NAIF identifier 399 |  |
| PASS | the URL requests the solar system barycentre as centre |  |
| PASS | the URL requests a vector table |  |

## Response parsing

| Result | Check | Detail |
| --- | --- | --- |
| PASS | two state vectors are parsed | count=2 |
| PASS | the first epoch is the Julian date 2451545 | actual=2451545 expected=2451545 abs_error=0 tolerance=1e-12 |
| PASS | the first x position is converted from kilometres to metres | actual=-26074813587.4 expected=-26074813587.4 rel_error=0 tolerance=1e-12 |
| PASS | the first x velocity is converted from kilometres per second to metres per second | actual=-29839.4467978 expected=-29839.4467978 rel_error=0 tolerance=1e-12 |
| PASS | the second epoch is the Julian date 2451546 | actual=2451546 expected=2451546 abs_error=0 tolerance=1e-12 |
| PASS | the second x position is converted from kilometres to metres | actual=-28651817454.9 expected=-28651817454.9 rel_error=0 tolerance=1e-12 |


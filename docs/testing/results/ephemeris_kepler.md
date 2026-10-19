# ephemeris_kepler

Verifies the ephemeris primitives: calendar to Julian date conversion at J2000 and one year later, and the conversion of the osculating elements of the Earth into a state vector with the expected heliocentric distance and orbital speed.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 7 |
| Failed checks | 0 |
| Duration | 0.001181 s |
| Generated | 2026-10-19 22:19:22 UTC |

## Epoch conversion

| Result | Check | Detail |
| --- | --- | --- |
| PASS | 1 January 2000 at noon is the Julian date 2451545 | actual=2451545 expected=2451545 abs_error=0 tolerance=1e-12 |
| PASS | J2000 corresponds to zero seconds past J2000 | actual=0 expected=0 abs_error=0 tolerance=1e-12 |
| PASS | 1 January 2001 at noon is the Julian date 2451911 | actual=2451911 expected=2451911 abs_error=0 tolerance=1e-12 |

## Keplerian elements of the Earth

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the eccentric anomaly stays within 0.04 rad of the mean anomaly for a small eccentricity | value=0.000410122 limit=0.04 |
| PASS | the true anomaly stays within 0.04 rad of the mean anomaly for a small eccentricity | value=0.000823727 limit=0.04 |
| PASS | the heliocentric distance stays within 2 percent of one astronomical unit |  |
| PASS | the orbital speed lies between 29 and 31 kilometres per second |  |

## Measurements

| Section | Name | Value | Unit |
| --- | --- | --- | --- |
| Keplerian elements of the Earth | heliocentric_distance | 1.47099053e+11 | m |
| Keplerian elements of the Earth | orbital_speed | 30286.4278 | m/s |


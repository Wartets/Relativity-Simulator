# Ephemeris and Keplerian elements suite

Executable: `test_ephemeris_kepler` - Source: `tests/test_ephemeris_kepler.cpp` - Results: [ephemeris_kepler.md](../results/ephemeris_kepler.md)

## Purpose

Verifies `Epoch` and `KeplerianElements` of [DESCRIPTION.md](../../DESCRIPTION.md#101-astronomical-ephemerides--coordinate-frames).

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Epoch conversion | J2000 | Julian date 2451545 and zero seconds past J2000 within 1e-12 |
| Epoch conversion | One year later | Julian date 2451911 within 1e-12 |
| Keplerian elements of the Earth | Anomalies | Eccentric and true anomaly within 0.04 rad of the mean anomaly |
| Keplerian elements of the Earth | State vector | Distance within 2 percent of one astronomical unit and speed between 29 and 31 km/s |

## Justification

For an eccentricity of 0.0167 the difference between mean, eccentric and true anomaly is bounded by about 2e per radian, or 0.034 rad. The distance and speed bounds follow from the perihelion and aphelion values of the Earth.

## Measurements

Heliocentric distance and orbital speed.

## Acceptance

All checks pass.

## Recorded results

See [../results/ephemeris_kepler.md](../results/ephemeris_kepler.md).

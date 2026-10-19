# spectral_shift_beaming

Verifies the spectral shift and beaming relations: the longitudinal and transverse kinematic Doppler factors, the Schwarzschild gravitational redshift between a static emitter and a static observer, the g^3 and g^4 intensity and flux scalings, and the wavelength transformation.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 7 |
| Failed checks | 0 |
| Duration | 0.001044 s |
| Generated | 2026-10-19 22:16:31 UTC |

## Kinematic Doppler factor

| Result | Check | Detail |
| --- | --- | --- |
| PASS | a head-on source is blueshifted by sqrt((1 + beta) / (1 - beta)) | actual=2 expected=2 abs_error=0 tolerance=1e-12 |
| PASS | a receding source is redshifted by sqrt((1 - beta) / (1 + beta)) | actual=0.5 expected=0.5 abs_error=0 tolerance=1e-12 |
| PASS | a transverse source shows pure time dilation 1 / gamma | actual=0.8 expected=0.8 abs_error=0 tolerance=1e-12 |

## Gravitational redshift in the Schwarzschild metric

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the frequency ratio equals sqrt(f(r_emit) / f(r_obs)) | actual=0.824786098842 expected=0.824786098842 abs_error=1.11e-16 tolerance=1e-10 |

## Beaming and wavelength scaling

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the specific intensity scales as g^3 | actual=800 expected=800 abs_error=0 tolerance=1e-12 |
| PASS | the bolometric flux scales as g^4 and equals 800 for g = 2 | actual=800 expected=800 abs_error=0 tolerance=1e-12 |
| PASS | the observed wavelength equals the emitted wavelength divided by g | actual=2.5e-07 expected=2.5e-07 rel_error=0 tolerance=1e-12 |


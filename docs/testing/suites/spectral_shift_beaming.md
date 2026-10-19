# Spectral shift and beaming suite

Executable: `test_spectral_shift_beaming` - Source: `tests/test_spectral_shift_beaming.cpp` - Results: [spectral_shift_beaming.md](../results/spectral_shift_beaming.md)

## Purpose

Verifies the frequency shift and beaming relations of [MATHEMATICAL_FORMULATION.md](../../MATHEMATICAL_FORMULATION.md#61-relativistic-frequency-shift--invariance).

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Kinematic Doppler factor | Head-on, receding, transverse at beta = 0.6 | sqrt((1+b)/(1-b)), sqrt((1-b)/(1+b)) and 1/gamma within 1e-12 |
| Gravitational redshift | Emitter at 6 M, observer at 100 M | sqrt(f(r_e) / f(r_o)) within 1e-10 |
| Beaming and wavelength | g = 2 | Intensity 800 (g^3 scaling of 100), flux 800 (g^4 scaling of 50), wavelength 250 nm |

## Justification

The Schwarzschild ratio follows from conservation of the Killing energy along a null geodesic. The intensity and flux inputs are chosen so that g^3 and g^4 scalings both give 800 when g = 2.

## Acceptance

All checks pass.

## Recorded results

See [../results/spectral_shift_beaming.md](../results/spectral_shift_beaming.md).

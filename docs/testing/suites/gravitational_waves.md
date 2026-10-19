# Gravitational waves suite

Executable: `test_gravitational_waves` - Source: `tests/test_gravitational_waves.cpp` - Results: [gravitational_waves.md](../results/gravitational_waves.md)

## Purpose

Verifies the quadrupole strain polarizations of [MATHEMATICAL_FORMULATION.md](../../MATHEMATICAL_FORMULATION.md#34-gravitational-wave-quadrupole-emission) computed by `GravitationalWaveCalculator::compute_binary_quadrupole_strain`.

## Scenario

Two neutron stars of 1.4 solar masses at a separation of 1e8 m, observed at 1e20 m with zero inclination, at orbital phases 0 and pi/4.

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Orbital phase zero | Plus polarization | Non-zero |
| Orbital phase zero | Cross polarization | Below 1e-9 of the plus amplitude |
| Orbital phase pi/4 | Cross polarization | Non-zero |
| Orbital phase pi/4 | Plus polarization | Below 1e-9 of the cross amplitude |

## Justification

For a face-on binary the plus and cross strains vary as cos and sin of twice the orbital phase, so one polarization vanishes when the other peaks. The tolerance is relative because the absolute strain is of order 1e-22, far below any fixed threshold.

## Measurements

The dominant strain at each phase.

## Acceptance

All checks pass.

## Recorded results

See [../results/gravitational_waves.md](../results/gravitational_waves.md).

# Horizon regime suite

Executable: `test_horizon_regime` - Source: `tests/test_horizon_regime.cpp` - Results: [horizon_regime.md](../results/horizon_regime.md)

## Purpose

Verifies the extremality analysis behind the parameter controls and the horizon zone strip, using the horizon definitions of [MATHEMATICAL_FORMULATION.md](../../MATHEMATICAL_FORMULATION.md#13-kerr-metric-rotating-black-hole).

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Constraint parsing | Flags per metric name | Kerr: spin; Reissner-Nordstrom: charge; Kerr-Newman: both; Schwarzschild-de Sitter: cosmological; Schwarzschild: none |
| Kerr | Classification | a/M = 0.5 sub-extremal, 1 extremal, 1.1 super-extremal |
| Kerr | Outer horizon | M + sqrt(M^2 - a^2) within 1e-12; M for extremal and super-extremal states |
| Kerr-Newman | Combined bound | a^2 + Q^2 = M^2 extremal, above it super-extremal |
| Kerr-Newman | Projection | A super-extremal state is projected to sub-extremal with a ratio at most the guarantee factor |
| Nariai | Classification | Lambda M^2 = 1/18, 1/9, 1/6 give sub-extremal, extremal, super-extremal |
| Nariai | Projection | The projected constant is sub-extremal and at most the guarantee factor / 9 |
| Guaranteed limits | Spin and charge | Limits lie within the physical domain, constraining clamps to the limit, spin squared plus charge squared stays below the guarantee |
| Zone layout | Spin and mass axes | Spin axis symmetric with threshold M; mass axis asymmetric with threshold a |

## Acceptance

All checks pass. Tolerances are 1e-12 for closed-form values and 1e-9 for the guarantee comparisons.

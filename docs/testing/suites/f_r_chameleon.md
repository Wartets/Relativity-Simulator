# f(R) chameleon suite

Executable: `test_f_r_chameleon` - Source: `tests/test_f_r_chameleon.cpp` - Results: [f_r_chameleon.md](../results/f_r_chameleon.md)

## Purpose

Verifies the Hu-Sawicki screening of `FRChameleonModel` ([ARCHITECTURE.md](../../ARCHITECTURE.md#25-dark-matter--modified-gravity)).

## Scenario

f_R0 = 1e-11, n = 1, cosmic density 1e-26 kg/m^3 and dense environment of 5500 kg/m^3.

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Scalaron mass | Density dependence | Dense mass more than 1e6 times the cosmic mass |
| Screened Earth | Thin shell factor | Below 0.1 |
| Screened Earth | Effective coupling | Within 5 percent of Newtonian gravity |
| Unscreened cloud | Thin shell factor | Equal to 1 within 1e-12 |
| Unscreened cloud | Effective coupling | 4/3 of Newtonian gravity, relative 1e-6 |

## Justification

An unscreened body feels the fifth force of strength 1/3, giving a total coupling of 4/3, while a thin shell suppresses it by the shell factor.

## Acceptance

All checks pass.

## Recorded results

See [../results/f_r_chameleon.md](../results/f_r_chameleon.md).

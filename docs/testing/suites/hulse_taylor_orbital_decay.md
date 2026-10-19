# Hulse-Taylor orbital decay suite

Executable: `test_hulse_taylor_orbital_decay` - Source: `tests/test_hulse_taylor_orbital_decay.cpp` - Results: [hulse_taylor_orbital_decay.md](../results/hulse_taylor_orbital_decay.md)

## Purpose

Reproduces the benchmark listed in [DESCRIPTION.md](../../DESCRIPTION.md#12-verification-criteria--benchmark-protocols): the Peters-Mathews period derivative of PSR B1913+16 must match the observed orbital decay.

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Peters-Mathews period derivative | Agreement with observation | Relative difference below 1 percent |
| Post-Newtonian orbital evolution | Body count | Exactly two bodies |
| Post-Newtonian orbital evolution | Luminosity | Initial radiated power positive |
| Post-Newtonian orbital evolution | Evolution | 500 adaptive steps of 10 s record steps and advance the time |

## Justification

The 1 percent criterion is the one the former assertion enforced. The analytic formula is evaluated with the catalogued orbital elements, so the residual reflects the uncertainty of the input masses rather than numerical error.

## Measurements

Analytical and observed derivative, relative difference, evolved time.

## Limits

The suite does not compare the integrated period change with the formula; it verifies that the integrator advances the radiating system.

## Acceptance

All checks pass.

## Recorded results

See [../results/hulse_taylor_orbital_decay.md](../results/hulse_taylor_orbital_decay.md).

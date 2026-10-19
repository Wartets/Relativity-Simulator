# Solar deflection suite

Executable: `test_solar_deflection` - Source: `tests/test_solar_deflection.cpp` - Results: [solar_deflection.md](../results/solar_deflection.md)

## Purpose

Reproduces the solar light deflection benchmark of [DESCRIPTION.md](../../DESCRIPTION.md#12-verification-criteria--benchmark-protocols) with `SchwarzschildMetric` in SI units and the Dormand-Prince integrator.

## Scenario

A null ray starting at x = -1000 R_sun with impact parameter 1 R_sun, integrated to x = +1000 R_sun (rtol 1e-10, atol 1e-14, step 1e7 to 5e10 m).

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Integration across the solar field | Steps | No step falls below the minimum step size |
| Integration across the solar field | Domain | The far boundary is reached |
| Deflection angle | First order | Relative error below 1e-4 against `4GM/(c^2 R)` |
| Deflection angle | Second order | Relative error below 1e-5 against the second-order post-Newtonian value |
| Null constraint | Residual | Maximum residual below 1e-11 |

## Justification

The ray starts at finite distance, so the first-order value is only reached within 1e-4. The second-order coefficient `15 pi/4 - 4` improves it by an order of magnitude. The expected deflection is 1.7512 arcseconds.

## Measurements

Step counts, the computed and analytic deflection in arcseconds.

## Acceptance

All checks pass.

## Recorded results

See [../results/solar_deflection.md](../results/solar_deflection.md).

# f_r_chameleon

Verifies the Hu-Sawicki chameleon model: the scalaron mass grows with the ambient density, the Earth is thin-shell screened so that its effective coupling equals Newtonian gravity, and an unscreened low-density cloud feels the enhanced coupling of 4/3.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 5 |
| Failed checks | 0 |
| Duration | 0.001118 s |
| Generated | 2026-10-19 22:19:23 UTC |

## Scalaron mass

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the scalaron is more than a million times heavier in the dense environment |  |

## Screened Earth

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the thin shell factor of the Earth is below 0.1 | value=0.00206198 limit=0.1 |
| PASS | the effective coupling of the Earth equals Newtonian gravity within 5 percent | actual=6.68806229495e-11 expected=6.6743e-11 rel_error=0.002062 tolerance=0.05 |

## Unscreened cloud

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the thin shell factor of the cloud saturates at one | actual=1 expected=1 abs_error=0 tolerance=1e-12 |
| PASS | the effective coupling of the cloud equals four thirds of Newtonian gravity | actual=8.89906666667e-11 expected=8.89906666667e-11 rel_error=0 tolerance=1e-06 |

## Measurements

| Section | Name | Value | Unit |
| --- | --- | --- | --- |
| Scalaron mass | scalaron_mass_ratio | 4.07890917e+44 | ratio |


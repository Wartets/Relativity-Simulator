# eos_tov

Verifies the equations of state and the stellar structure solver: ideal gas, Synge and Mathews relations and limits, the degenerate Fermi gas identities, the tabulated nuclear table round trip, and Tolman-Oppenheimer-Volkoff stars for a white dwarf, a free neutron gas and the SFHo, Shen and LS220 tables.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 32 |
| Failed checks | 0 |
| Duration | 3.697 s |
| Generated | 2026-10-19 22:19:22 UTC |

## Ideal gas

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the pressure equals (Gamma - 1) rho eps | actual=33.3333333333 expected=33.3333333333 abs_error=0 tolerance=1e-12 |
| PASS | the specific internal energy is recovered | actual=0.05 expected=0.05 abs_error=6.939e-18 tolerance=1e-12 |
| PASS | the specific enthalpy equals 1 + eps + p / rho | actual=1.08333333333 expected=1.08333333333 abs_error=2.22e-16 tolerance=1e-12 |
| PASS | the sound speed squared equals Gamma p / (rho h) | actual=0.0512820512821 expected=0.0512820512821 abs_error=0 tolerance=1e-12 |
| PASS | the sound speed squared is positive and below Gamma - 1 |  |

## Synge and Mathews gases

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the cold Synge enthalpy approaches the non-relativistic limit 1 + 5/2 theta | actual=1.00002500011 expected=1.000025 abs_error=1.125e-10 tolerance=0.0001 |
| PASS | the hot Synge enthalpy approaches the ultra-relativistic limit 4 theta within 5 percent | actual=400.003333296 expected=400 rel_error=8.333e-06 tolerance=0.05 |
| PASS | the cold Mathews sound speed squared lies in (0, 1/3) |  |
| PASS | the hot Mathews sound speed squared approaches 1/3 | actual=0.333333333333 expected=0.333333333333 abs_error=0 tolerance=0.0001 |

## Relativistic degenerate Fermi gas

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the energy density exceeds the pressure |  |
| PASS | the specific enthalpy equals sqrt(1 + x^2) | actual=1.80277563773 expected=1.80277563773 abs_error=0 tolerance=1e-12 |
| PASS | the sound speed squared equals c^2 x^2 / (3 (1 + x^2)) | actual=2.07405041247e+16 expected=2.07405041247e+16 abs_error=0 tolerance=1e-06 |
| PASS | the density is recovered from the pressure within a relative 1e-8 | actual=2.06096422865e+19 expected=2.06096422865e+19 rel_error=0 tolerance=1e-08 |
| PASS | the pressure is recovered from the enthalpy within a relative 1e-8 | actual=5.24169631495e+35 expected=5.24169631495e+35 rel_error=5.631e-16 tolerance=1e-08 |

## Tabulated nuclear equation of state

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the preset carries the SFHo name |  |
| PASS | the pressure at 3e17 kg/m^3 is positive |  |
| PASS | the sound speed squared is positive and causal |  |
| PASS | the table survives the container round trip |  |
| PASS | the pressure survives the round trip within a relative 1e-10 | actual=5.20810310355e+32 expected=5.20810310355e+32 rel_error=0 tolerance=1e-10 |
| PASS | the sound speed survives the round trip within 1e-10 | actual=8.98755178737e+16 expected=8.98755178737e+16 abs_error=0 tolerance=1e-10 |

## White dwarf

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the white dwarf integration converges |  |
| PASS | the white dwarf mass lies between 0.5 and the Chandrasekhar limit 1.44 solar masses | mass=1.339113 |
| PASS | the white dwarf radius lies between 1000 and 20000 km | radius=2453.994131 |
| PASS | the white dwarf compactness is below 1e-3 |  |

## Free neutron gas

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the maximum mass of a free neutron gas lies near the Oppenheimer-Volkoff value of 0.7 solar masses |  |
| PASS | the radius at the maximum mass lies between 8 and 12 km |  |

## Nuclear equations of state

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the SFHo maximum mass lies between 1.95 and 2.15 solar masses |  |
| PASS | the SFHo radius at the maximum mass lies between 9 and 13 km |  |
| PASS | the SFHo radius of a 1.4 solar mass star lies between 10.5 and 14 km |  |
| PASS | the Shen maximum mass lies between 2.05 and 2.30 solar masses |  |
| PASS | the LS220 maximum mass lies between 1.95 and 2.15 solar masses |  |
| PASS | the stiffer Shen equation of state supports a larger maximum mass than LS220 |  |

## Measurements

| Section | Name | Value | Unit |
| --- | --- | --- | --- |
| Free neutron gas | free_neutron_maximum_mass | 0.710126657 | solar masses |
| Nuclear equations of state | sfho_maximum_mass | 1.98177282 | solar masses |
| Nuclear equations of state | shen_maximum_mass | 2.28455006 | solar masses |
| Nuclear equations of state | ls220_maximum_mass | 2.03265575 | solar masses |


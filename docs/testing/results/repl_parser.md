# repl_parser

Verifies the terminal command parser: flow commands, argument validation, every documented parameter key, custom parameters, camera commands, text commands, case-insensitivity and error messages.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 52 |
| Failed checks | 0 |
| Duration | 0.002791 s |
| Generated | 2026-10-19 22:21:04 UTC |

## Simulation flow commands

| Result | Check | Detail |
| --- | --- | --- |
| PASS | pause is recognized |  |
| PASS | commands are case-insensitive and trimmed |  |
| PASS | resume is recognized in any case |  |
| PASS | reset is recognized |  |
| PASS | status is recognized |  |
| PASS | shutdown maps to the shutdown command |  |
| PASS | quit maps to the shutdown command |  |
| PASS | exit maps to the shutdown command |  |
| PASS | step without argument advances one tick |  |
| PASS | step accepts an explicit tick count |  |
| PASS | a negative step count is rejected |  |
| PASS | a non-numeric step count is rejected |  |
| PASS | a zero step count is rejected |  |
| PASS | warp is recognized |  |
| PASS | warp stores its factor | actual=2.5 expected=2.5 abs_error=0 tolerance=1e-12 |
| PASS | non-positive warp factors are rejected |  |
| PASS | warp without argument reports the missing argument |  |
| PASS | tickrate is recognized |  |
| PASS | tickrate stores its frequency | actual=120 expected=120 abs_error=0 tolerance=1e-12 |
| PASS | tick rates outside 10 to 1000 Hz are rejected |  |
| PASS | tickrate without argument is rejected |  |

## Parameter commands

| Result | Check | Detail |
| --- | --- | --- |
| PASS | every documented parameter key resolves to its parameter type | mismatches=0 keys=33 |
| PASS | scientific notation values are parsed | actual=1.989e+30 expected=1.989e+30 rel_error=0 tolerance=1e-12 |
| PASS | negative values are parsed | actual=-0.5 expected=-0.5 abs_error=0 tolerance=1e-12 |
| PASS | the set command and its keys are case-insensitive |  |
| PASS | an unknown key becomes a custom parameter |  |
| PASS | the custom parameter keeps its name |  |
| PASS | the custom parameter keeps its value | actual=42.125 expected=42.125 abs_error=0 tolerance=1e-12 |
| PASS | custom parameter names are truncated to 63 characters |  |
| PASS | set without arguments reports the missing name |  |
| PASS | set without value reports the missing value |  |
| PASS | a non-numeric value is rejected |  |

## Camera commands

| Result | Check | Detail |
| --- | --- | --- |
| PASS | camera reset is recognized |  |
| PASS | camera fov stores the angle |  |
| PASS | fields of view outside 5 to 175 degrees are rejected |  |
| PASS | camera speed requires a positive value |  |
| PASS | camera move stores the three components |  |
| PASS | camera move requires three components |  |
| PASS | camera rotate defaults the roll to zero |  |
| PASS | camera rotate accepts an optional roll |  |
| PASS | an unknown camera sub-command is rejected |  |

## Text commands and error handling

| Result | Check | Detail |
| --- | --- | --- |
| PASS | metric stores the metric name |  |
| PASS | metric without name is rejected |  |
| PASS | integrator stores the scheme name |  |
| PASS | integrator without name is rejected |  |
| PASS | load keeps the complete path, spaces included |  |
| PASS | save requires a path |  |
| PASS | export defaults to every format |  |
| PASS | export stores the requested format |  |
| PASS | a blank line is reported as empty |  |
| PASS | an unknown command is reported |  |
| PASS | a rejected command reports failure |  |


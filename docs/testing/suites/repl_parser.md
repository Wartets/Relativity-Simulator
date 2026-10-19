# REPL parser suite

Executable: `test_repl_parser` - Source: `tests/test_repl_parser.cpp` - Results: [repl_parser.md](../results/repl_parser.md)

## Purpose

Verifies `CommandParser::parse`, the entry point of the terminal described in [CLI_REFERENCE.md](../../CLI_REFERENCE.md#2-interactive-terminal-commands-repl) and [ARCHITECTURE.md](../../ARCHITECTURE.md#51-master-terminal-loop-repl).

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Flow commands | Recognition | `pause`, `resume`, `reset`, `status`, `shutdown`, `quit`, `exit`, in any case and with surrounding spaces |
| Flow commands | `step` | Default of one tick, explicit counts, rejection of negative, zero and non-numeric counts |
| Flow commands | `warp` | Positive factors stored, zero, negative and missing arguments rejected with the documented message |
| Flow commands | `tickrate` | 10 to 1000 Hz accepted, outside values and missing arguments rejected |
| Parameter commands | Key table | 33 keys and aliases resolve to their `ParameterType` |
| Parameter commands | Values | Scientific notation, negative numbers and case-insensitivity |
| Parameter commands | Custom parameters | Unknown keys become `Custom` with name and value; names are truncated to 63 characters |
| Parameter commands | Errors | Missing name, missing value and non-numeric values |
| Camera commands | `reset`, `fov`, `speed`, `move`, `rotate` | Value ranges (5 to 175 degrees), component counts, optional roll, unknown sub-command |
| Text commands | `metric`, `integrator`, `load`, `save`, `export` | Payload fidelity, paths with spaces, default export format `all` |
| Text commands | Errors | Blank line and unknown command messages and failure status |

## Limits

The suite checks parsing only. Application of commands is covered by the camera navigation suite ([camera_navigation_modes.md](camera_navigation_modes.md)) and the legacy command injection test.

## Acceptance

All checks pass. The suite is deterministic.

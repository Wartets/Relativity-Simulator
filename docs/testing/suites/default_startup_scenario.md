# Default startup scenario suite

Executable: `test_default_startup_scenario` - Source: `tests/test_default_startup_scenario.cpp` - Results: [default_startup_scenario.md](../results/default_startup_scenario.md)

## Purpose

Verifies the startup rules of [ARCHITECTURE.md](../../ARCHITECTURE.md#35-scenario-resolution-and-startup) and the reset behaviour of [CLI_REFERENCE.md](../../CLI_REFERENCE.md#25-command-processing-and-performance-presets). The working directory must be the repository root, as set by the CTest registration.

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Initial state | Camera | Position (-5, 37, -7) and attitude (0, 130, 0) within 1e-9; radius `sqrt(1443)` within 1e-9; no scenario path |
| Scenario resolution | Built-in scenario | Reachable, with the canonical portable path |
| Scenario resolution | Fallback | Empty and missing configurations fall back; a valid one is kept |
| Startup load | Load | Command accepted and successful; name "Schwarzschild Black Hole & Accretion Disk"; path equals the built-in file; camera at the startup pose |
| Resets | View reset | The camera returns to the startup pose |
| Resets | Full reset | Spin restored to 0, scenario kept, scheduler paused, camera at the startup pose |
| Failed load | Missing file | The load fails and the active scenario is kept |

## Justification

The startup pose is the home camera of the loaded scenario, so both resets must return to it and neither may drop the scenario.

## Acceptance

All checks pass.

## Recorded results

See [../results/default_startup_scenario.md](../results/default_startup_scenario.md).

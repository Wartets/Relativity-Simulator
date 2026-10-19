# Interface control suite

Executable: `test_ui_multi_window` - Source: `tests/test_ui_multi_window.cpp` - Results: [ui_multi_window.md](../results/ui_multi_window.md)

## Purpose

Verifies the command path of [CLI_REFERENCE.md](../../CLI_REFERENCE.md#2-interactive-terminal-commands-repl) and the `InteractiveCameraController` helpers used by the windows of [ARCHITECTURE.md](../../ARCHITECTURE.md#52-graphical-multi-window-workspace-ui-architecture), without opening a window. The six navigation modes are verified in [camera_navigation_modes.md](camera_navigation_modes.md). The test links `imgui_lib`.

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Parameter commands | Mass and spin | Applied within 1e-12 |
| Camera commands | Field of view, speed | Applied within 1e-12 |
| Camera commands | Move | Translation relative to the previous position within 1e-12 |
| Metric, integrator and render commands | Names | Stored unchanged |
| Metric, integrator and render commands | Render | Scale 1.5 and 4096 steps applied; a render edit selects the custom preset |
| Metric, integrator and render commands | Preset | Preset 1 applies its documented scale of 0.5 ([CLI_REFERENCE.md](../../CLI_REFERENCE.md#25-command-processing-and-performance-presets)); camera mode applied |
| Interactive camera controller | Accessors | Move speed and sensitivity returned |
| Interactive camera controller | Scroll and modes | A scroll step changes the field of view; orbit and spherical modes selectable |
| Snap helpers | Equatorial and polar | Requested radius within 1e-6 and 1e-2; equatorial plane and polar axis reached |
| Snap helpers | ISCO and photon sphere | Finite positive radius |

## Justification

All commands go through the queue and are applied at the next processing step, so each check processes the queue before reading the state.

## Limits

Keyboard and mouse driven updates require a window and are not covered.

## Acceptance

All checks pass.

## Recorded results

See [../results/ui_multi_window.md](../results/ui_multi_window.md).

# Relativistic Engine - Command Line Interface & REPL Reference

## 1. Executable Binaries

The engine provides two standalone executables:
1. `engine_cli`: Master simulation host containing the non-blocking command interpreter & the graphical multi-window instrumentation workspace.
2. `headless_exporter`: Non-interactive batch processor for cluster execution, parameter sweeps, & validation benchmarks.

---

## 2. Interactive Terminal Commands (REPL)

In interactive mode, the engine provides the non-blocking master prompt:
`relativistic> `

Commands can be supplied directly via standard input or queued asynchronously through the inter-thread command bus.

### 2.1. Simulation Flow & State Commands

| Command | Arguments | Description | Example |
| :--- | :--- | :--- | :--- |
| `pause` | None | Halts the progression of logical simulation time. | `pause` |
| `resume` | None | Resumes continuous simulation progression. | `resume` |
| `step` | `[N]` | Advances the simulation by exactly `N` ticks (default: 1) & pauses. | `step 10` |
| `warp` | `<factor>` | Sets the logical time dilation factor ($> 0.0$). | `warp 5.0` |
| `tickrate` | `<Hz>` | Adjusts the scheduler logical frequency (10.0 to 1000.0 Hz). | `tickrate 120.0` |
| `reset` | None | Resets simulation time to zero & restores initial conditions. | `reset` |
| `status` | None | Prints the current state of the scheduler & active parameters. | `status` |
| `quit`, `exit`, `shutdown` | None | Terminates all worker threads & exits the application. | `quit` |

### 2.2. Camera & Observer Navigation Commands

| Command | Arguments | Description | Example |
| :--- | :--- | :--- | :--- |
| `camera reset` | None | Restores the default observer position & orientation. | `camera reset` |
| `camera fov` | `<degrees>` | Sets the camera field of view in degrees (5.0 to 175.0). | `camera fov 65.0` |
| `camera speed` | `<value>` | Sets the translation speed of the observer ($> 0.0$). | `camera speed 25.0` |
| `camera move` | `<dx> <dy> <dz>` | Applies an instantaneous Cartesian displacement. | `camera move 0.0 5.0 0.0` |
| `camera rotate` | `<pitch> <yaw> [roll]` | Applies angular increments in degrees. | `camera rotate -15.0 45.0 0.0` |

### 2.3. Spacetime Metric & Integrator Selection

| Command | Arguments | Description | Example |
| :--- | :--- | :--- | :--- |
| `metric` | `<name>` | Changes the active metric geometry. | `metric Kerr` |
| `integrator` | `<name>` | Selects the numerical differential equation solver. | `integrator Vernier9` |
| `load` | `<path>` | Ingests a declarative YAML scenario configuration. | `load scenarios/kerr_accretion_disk.yaml` |
| `save` | `<path>` | Serializes the active simulation state to a YAML file. | `save scenarios/snapshot.yaml` |
| `export` | `[format]` | Triggers file generation (`fits`, `hdf5`, `vtk`, `all`). | `export fits` |

Supported metric names: `FlatMinkowski` (alias `Minkowski`), `Schwarzschild`, `Kerr`, `KerrSchild`, `ReissnerNordstrom`, `KerrNewman`, `SchwarzschildDeSitter`, `FLRW`, `MorrisThorne`, `Alcubierre`, `BSSN`.

The `metric` command stores the name without validating it. Two resolvers consume it:

- `SimulationOrchestrator::metric_name_to_id` (`include/relativistic/orchestrator/simulation_orchestrator.hpp`) matches the exact names above; any other string resolves to Schwarzschild.
- The viewport (`ViewportPrimaryWindow::get_metric_id_from_name` in `include/relativistic/ui/viewport_primary_window.hpp`) matches substrings in this order: `Minkowski`, `Schwarzschild` together with `de Sitter`, `Schwarzschild`, `Kerr-Newman`, `Kerr`, `Reissner`, `FLRW`, `Morris` or `Wormhole`, `Alcubierre` or `Warp`. The compact names `KerrNewman` and `SchwarzschildDeSitter` therefore resolve to Kerr and Schwarzschild in the viewport. The display names used by the metric cycle action (`Kerr-Newman Charged Rotating`, `Schwarzschild-de Sitter (Lambda)`, `Morris-Thorne Traversable Wormhole`, `Alcubierre Warp Drive Bubble`, `BSSN 3+1 Numerical Grid`, among others) resolve correctly. Names without a match, including `BSSN`, are traced as Schwarzschild.

Scenario files accept both forms; see the validation rules in [FILE_FORMATS.md](FILE_FORMATS.md#12-parsing-validation-and-application-rules). Execution path selection by metric is described in [ARCHITECTURE.md](ARCHITECTURE.md#29-render-pipeline).

Supported integrator names: `RK45`, `CashKarp`, `Vernier9`, `GaussLegendre4`, `GaussLegendre6`, `Hermite4`.

The `integrator` command stores the name without validating it. The N-body integration uses the symplectic Forest-Ruth post-Newtonian integrator when the name contains `Symplectic` or `Gauss`, and the fourth-order Runge-Kutta post-Newtonian integrator otherwise (see [ARCHITECTURE.md](ARCHITECTURE.md#34-orchestrator-state-advance)). The integrator cycle action uses the names `Dormand-Prince RK45 (Adaptive)`, `Cash-Karp 5(4) (Adaptive)`, `Vernier 9(8) High-Order`, `Symplectic Gauss-Legendre 4th`, `Symplectic Gauss-Legendre 6th`, and `Hermite 4th-Order (Aarseth)`.

### 2.4. Parameter Modification Commands (`set`)

The `set` command updates physical properties, optical settings, & solver tolerances:
`set <parameter> <value>`

| Parameter Key | Value Type | Physical / Numerical Meaning | Example |
| :--- | :--- | :--- | :--- |
| `mass` | Float | Central body mass $M$. | `set mass 1.0` |
| `spin` | Float | Central body spin parameter $a$. | `set spin 0.94` |
| `charge` | Float | Central body net electric charge $Q$. | `set charge 0.5` |
| `lambda` | Float | Cosmological constant $\Lambda$. | `set lambda 1.1e-52` |
| `throat` | Float | Morris-Thorne wormhole throat radius $b_0$. | `set throat 5.0` |
| `warp_velocity`, `warp_vel` | Float | Alcubierre metric apparent velocity $v_s$. | `set warp_velocity 2.0` |
| `projection`, `proj` | Integer | Projection (0: Pinhole, 1: AutoZoom, 2: FisheyeStereographic, 3: Equirectangular360, 4: FisheyeEquidistant, 5: FisheyeOrthographic, 6: PaniniCylindrical, 7: HammerAitoff). The default is 3. | `set projection 1` |
| `timeflow`, `time_flow` | Integer | Time frame (0: Proper time $\tau$, 1: Coordinate time $t$). | `set timeflow 0` |
| `speed`, `cameran_speed` | Float | Navigation movement rate. | `set speed 15.0` |
| `fov` | Float | Field of view in degrees. | `set fov 75.0` |
| `exposure` | Float | Exposure compensation in EV units. | `set exposure 1.5` |
| `tonemapper`, `tonemap` | Integer | Operator (0: Linear, 1: ACES, 2: Logarithmic, 3: Reinhard). | `set tonemapper 1` |
| `rtol` | Float | Relative integration tolerance. | `set rtol 1e-12` |
| `atol` | Float | Absolute integration tolerance. | `set atol 1e-15` |
| `min_step` | Float | Minimum integration step size bound. | `set min_step 1e-10` |
| `max_step` | Float | Maximum integration step size bound (clamped to 0.01 to 50). | `set max_step 1.0` |
| `step_factor`, `step_scale` | Float | Step size scaling factor of the adaptive controller (clamped to 0.002 to 0.5). | `set step_factor 0.45` |
| `render_scale`, `scale` | Float | Internal raster resolution scaling factor (0.1 to 2.0). | `set scale 1.0` |
| `ray_steps`, `steps_limit` | Integer | Maximum integration steps per ray (64 to 65536). | `set ray_steps 2048` |
| `performance`, `perf` | Integer | Performance profile preset (0 to 5; 6 denotes Custom and leaves the current values unchanged). Preset contents are listed in [Section 2.5](#25-command-processing-and-performance-presets). | `set performance 2` |
| `camera_mode`, `cam_mode` | Integer | Mode (0: Free Fly, 1: Orbit Center, 2: Spherical, 3: Rocket Thrust). Behaviors are described in [Section 3.2](#32-navigation-modes). | `set camera_mode 0` |
| `sky_star_density` | Float | Celestial star density scaling factor. | `set sky_star_density 1.0` |
| `sky_star_brightness` | Float | Star luminance multiplier. | `set sky_star_brightness 1.0` |
| `sky_nebula` | Float | Galactic band and nebula intensity. | `set sky_nebula 1.0` |
| `sky_grid_opacity` | Float | Coordinate grid opacity. | `set sky_grid_opacity 1.0` |
| `sky_rotation` | Float | Sky sphere rotation angle in degrees. | `set sky_rotation 45.0` |
| `sky_hue` | Float | Sky hue shift in degrees. | `set sky_hue 0.0` |
| `sky_saturation` | Float | Sky color saturation factor. | `set sky_saturation 1.0` |
| `sky_bg_r`, `sky_bg_g`, `sky_bg_b` | Float | Background color components [0.0, 1.0]. | `set sky_bg_r 0.0` |
| `work_distribution`, `tiling` | Integer | Work layout (0: Scanlines, 1: 32x32 Tiles). | `set tiling 1` |
| `force_realloc`, `realloc_texture` | Boolean (0/1) | Forces full GPU texture storage reallocation each frame instead of in-place sub-image updates. | `set force_realloc 1` |
| `space_skip`, `space_skip_enabled` | Boolean (0/1) | Enables analytic ray leaping across the weak-field region beyond the space-skip radius. | `set space_skip 1` |
| `space_skip_radius` | Float | Radius, in units of central mass M, beyond which space-skipping may activate. | `set space_skip_radius 60.0` |
| `pole_precision` | Float | Strength of the automatic step-size damping applied near the coordinate poles. | `set pole_precision 2.5` |
| `tickrate` | Float | Scheduler frequency in Hertz (10.0 to 1000.0). | `set tickrate 60.0` |
| `precision_mode` | Integer | Arithmetic precision of the software renderer (0: native FP64, 1: double-single emulation). Stored as a custom parameter and read by the viewport. | `set precision_mode 1` |
| `<custom_name>` | Float | User-defined custom scalar quantity. Names are truncated to 63 characters and at most 32 distinct names are stored; further names are ignored. | `set dyn_res 1.0` |

### 2.5. Command Processing and Performance Presets

Command words are case-insensitive and arguments are separated by spaces or tabs. `CommandParser::parse` (`include/relativistic/orchestrator/command.hpp`) produces a `Command`, and `MasterTerminalRepl::execute_line` (`include/relativistic/orchestrator/repl.hpp`) pushes it onto the command queue (capacity 1024). When the queue is full the line is rejected with the message `Command queue full`. Commands are applied by `SimulationOrchestrator::process_incoming_commands` and each application pushes a `CommandResult` (message of at most 127 characters) onto the result queue. Text arguments (`metric`, `integrator`, `load`, `save`) are truncated to 255 characters.

Additional processing rules:

- `load <path>` resolves the path as given, then relative to up to four parent directories, then against the `scenarios` directory (see [ARCHITECTURE.md](ARCHITECTURE.md#35-scenario-resolution-and-startup)). The scenario format is described in [FILE_FORMATS.md](FILE_FORMATS.md#1-declarative-scenario-definition-yaml).
- `save <path>` overwrites an existing file.
- `reset` restores physical parameters, camera, constants and scheduler to their initial values, clears all custom parameters, and reloads the active scenario when it was loaded from a file.
- `export` commands are reported as `Unknown or unsupported command` by `SimulationOrchestrator::apply_command`; export generation is not part of the orchestrator command set.
- Any command that modifies a rendering or integration parameter (resolution scale, ray steps, tolerances, step bounds, GPU compute, tiling, render distance, level of detail, motion quality, step controller, space skipping, far-field step scale, adaptive tile prepass, body rendering parameters, overlays, custom parameters) sets the performance preset index to 6 (Custom).

Performance presets applied by `SimulationOrchestrator::apply_performance_preset`:

| Index | Name | Resolution scale | Max ray steps | rtol | atol | Body LOD pixel threshold | Body point pixel threshold | Body noise octaves | Body shadows | Body disk occlusion | Atmosphere intensity | Body low power mode |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| 0 | Potato | 0.25 | 512 | 1e-6 | 1e-10 | 24 | 6 | 1 | Off | Off | 0.0 | On |
| 1 | Performance | 0.5 | 2048 | 1e-8 | 1e-12 | 16 | 4 | 2 | Off | Off | 0.5 | Off |
| 2 | Balanced | 1.0 | 4096 | 1e-9 | 1e-13 | 10 | 2 | 4 | Off | Off | 1.0 | Off |
| 3 | High | 1.25 | 16384 | 1e-10 | 1e-14 | 6 | 1 | 5 | On | On | 1.0 | Off |
| 4 | Ultra | 1.5 | 8192 | 1e-12 | 1e-15 | 4 | 1 | 6 | On | On | 1.25 | Off |
| 5 | Extreme | 2.0 | 16384 | 1e-14 | 1e-17 | 4 | 1 | 6 | On | On | 1.5 | Off |

The captions shown for some presets in the Performance & Engine Optimization window differ from these values; the values above are those applied to the simulation parameters.

---

## 3. Graphical Interface & Interactive Controls

When executed without the `--headless` flag, `engine_cli` initializes a graphical OpenGL context managed through ImGui & ImPlot.

### 3.1. Navigation & Viewport Keybindings

| Key / Input | Action | Mode |
| :--- | :--- | :--- |
| `W` / `Z` | Translate forward | Free Fly / Rocket Thrust / Orbit distance / Spherical radius |
| `S` | Translate backward | Free Fly / Rocket Thrust / Orbit distance / Spherical radius |
| `A` / `Q` | Translate / Orbit left | Free Fly / Orbit / Spherical |
| `D` | Translate / Orbit right | Free Fly / Orbit / Spherical |
| `Space` / `E` | Translate upward | Free Fly |
| `C` / `Ctrl` | Translate downward | Free Fly |
| `J` / `Page Up` | Roll counter-clockwise | Free Fly |
| `K` / `Page Down` | Roll clockwise | Free Fly |
| `Shift` (Hold) | High-speed boost modifier ($4\times$) | All Modes |
| `Ctrl` / `Alt` (Hold) | Precision crawl modifier ($0.2\times$) | All Modes |
| `Right Click` + Drag | Angular look (Pitch / Yaw) | All Modes |
| `Left Click` + Drag | Angular look; both buttons are handled identically and a drag starts after a 4 pixel displacement | All Modes |
| `Mouse Scroll` | Adjust camera field of view in steps of 2.5 degrees (5 to 170 degrees) | All Modes |
| Zoom modifier (Hold) + `Mouse Scroll` | Magnify the displayed image between `min_zoom` and `max_zoom` (1.0 to 8.0), centered on the cursor when `zoom_center_on_cursor` is set; the field of view is unchanged | All Modes |

### 3.2. Navigation Modes

Speeds and sensitivities are stored in `CameraControlConfig` (`include/relativistic/ui/camera_control_config.hpp`) and persisted as described in [FILE_FORMATS.md](FILE_FORMATS.md#10-persistent-configuration-files). The camera pose is stored as Cartesian position and pitch, yaw and roll angles in degrees; the spherical coordinates `radius`, `theta` and `phi` are recomputed after every position change.

| Mode | Behavior |
| :--- | :--- |
| Free Fly (0) | Translation along the forward, right and up axes at 10 units per second by default. Pitch is ignored for movement when `ignore_pitch_roll_for_movement` is set (default). The sprint modifier multiplies the speed by 4 and the crawl modifier by 0.2. Roll rate is 45 degrees per second. |
| Orbit Center (1) | Forward and backward change the orbit distance (2 to 5000), left and right change the yaw, up and down change the pitch (limited to plus or minus 89 degrees). The camera is placed at `target - distance * view_direction`. |
| Spherical (2) | Forward and backward change `radius` (2.5 to 5000), left and right change `phi`, up and down change `theta` (0.01 to pi - 0.01) with angular speed `speed / radius`. The camera always looks at the origin. |
| Rocket Thrust (3) | Keys produce thrust accelerations (main 5, lateral 2, vertical 2) whose magnitude is capped at a proper acceleration of 20. Thrust is applied only while the simulation clock runs (`requires_time_running`). While the clock runs, the camera velocity receives a gravitational deceleration `M / r^2` directed toward the origin and the position is integrated. Roll rate is 15 degrees per second. |

### 3.3. Global Function Shortcuts

| Shortcut | Description |
| :--- | :--- |
| `F1` | Toggle Master Simulation Controls window. |
| `F2` | Apply Multi-Window Detached workspace layout. |
| `F3` | Apply Docked Workspace layout container. |
| `F4` | Apply Viewport Fullscreen Focus mode. |
| `F5` / `P` | Toggle simulation pause / resume state. |
| `F6` | Advance simulation by a single logical tick. |
| `F7` | Reset simulation time and orbital clocks. |
| `F8` | Toggle Celestial Body & N-Body Manager window. |
| `F9` | Cycle through camera navigation modes. |
| `F10` | Toggle Telemetry & Invariants window. |
| `F11` | Toggle the Performance & Engine Optimization window and the Curvature Diagnostics & Tensor Inspector window together. |
| `F12` | Open the Capture Studio window. |
| `F` | Orient camera toward coordinate origin $(0, 0, 0)$. |
| `Home` | Reset camera roll angle to zero. |
| `[` / `]` | Decrease / Increase navigation movement speed by 5%. |
| `Numpad 1` | Snap viewpoint to equatorial front ($r = 50M$). |
| `Numpad 3` | Snap viewpoint to equatorial side ($r = 50M$). |
| `Numpad 7` | Snap viewpoint to north pole ($z = 50M$). |
| `Numpad 9` | Snap viewpoint to south pole ($z = -50M$). |
| `Numpad 5` | Snap viewpoint to ISCO orbital radius. |
| `H` | Toggle HUD master visibility. |
| `B` | Toggle Keybind Settings window. |
| `M` | Cycle the active spacetime metric. |
| `I` | Cycle the active ODE integrator. |
| `V` | Cycle the camera projection mode. |
| `T` | Cycle the HDR tonemapping operator. |
| `G` | Cycle the skybox style. |
| `U` | Toggle GPU compute offload. |
| `N` | Toggle adaptive space-skipping. |
| `L` | Toggle distance-based level of detail. |
| `=` | Increase exposure compensation (EV). |
| `-` | Decrease exposure compensation (EV). |
| `.` | Increase the time warp factor. |
| `,` | Decrease the time warp factor. |
| `Insert` | Quick-save the current scenario. |
| `Delete` | Quick-load the last quick-saved scenario. |
| `` ` `` | Toggle fullscreen viewport mode. |
| `R` | Toggle tiled work distribution. |
| `0` | Cycle the adaptive step-size controller. |
| `9` | Reset performance settings to the balanced preset. |
| `O` | Toggle Scenario Manager & Presets window. |
| `Y` | Toggle Curvature Diagnostics & Tensor Inspector window. |
| `X` | Toggle Radiative Transfer & Spectrograph Monitor window. |
| `;` | Toggle Physical Constants Engine window. |

### 3.4. Additional Rebindable Actions

The following actions are available in the Keybind Settings window. Each action has a primary and a secondary key, conflicts are highlighted, and the actions `ZoomModifier`, `Sprint` and `Crawl` support a Hold or Toggle activation mode. The keyboard layout preset (QWERTY or AZERTY) replaces all bindings with the defaults of that layout.

| Action | Effect |
| :--- | :--- |
| Toggle Capture Studio | Shows or hides the Capture Studio window. |
| Toggle Performance Analysis Window | Shows or hides the Performance Analysis & Profiling window; no default key. |
| Start/Stop Benchmark Capture | Starts a 10 second benchmark capture labeled `Quick Capture`, or cancels the running capture; no default key. |
| Quick Save Benchmark Run | Starts a 60 frame benchmark capture labeled `Quick Save`; no default key. |
| Bulk Invert All Velocities | Negates the velocity of every body. |
| Bulk Scatter Body Positions | Randomizes body positions (`BulkBodyActions::scatter_positions`). |
| Bulk Snap Bodies To Grid | Snaps body positions to a grid of spacing 1.0. |
| Bulk Cull Bodies Outside View | Removes bodies beyond `render_distance_scale * mass` (1e7 when the render distance is unbounded). |
| Bulk Equalize Body Masses | Sets all body masses to the same value. |
| Bulk Average Body Masses | Replaces body masses by their mean. |
| Bulk Zero All Spins | Sets all body spins to zero. |

### 3.5. Main Menu Bar

| Menu | Content |
| :--- | :--- |
| File | Capture Studio, quick screenshot with the current Capture Studio settings, snapshot scenario save (`scenarios/snapshot.yaml`), export entries (`fits`, `hdf5`, `vtk`), exit. |
| Simulation | Pause/resume, single step, reset, camera mode cycle, snap to equatorial position (`r = 50`), snap to ISCO. |
| Capture | Capture Studio tab shortcuts (Screenshot, Sequence, Motion Script, Data Recording, Video Encoding), quick screenshot, start sequence, finish sequence, cancel capture. |
| Window Layouts | Multi-Window Detached, Docked Workspace, Viewport Fullscreen Focus, multi-window viewport separation toggle. |
| View Windows | Visibility toggles of every window, Show All Panels, Focus Viewport Only, Force Viewport Refresh, secondary viewport management. |

The Capture Studio, its sequence modes and its output files are described in [ARCHITECTURE.md](ARCHITECTURE.md#210-capture-subsystem) and [FILE_FORMATS.md](FILE_FORMATS.md#6-image-output-formats).

---

## 4. Headless Batch Exporter (`headless_exporter`)

The `headless_exporter` binary processes declarative simulation scenarios without graphical dependencies.

### 4.1. Command Line Syntax

```bash
./headless_exporter [OPTIONS]
```

### 4.2. Command Line Arguments

| Option | Argument | Description | Default |
| :--- | :--- | :--- | :--- |
| `--scenario` | `<path>` | Path to declarative YAML scenario file. | None |
| `--output-dir` | `<path>` | Output directory for scientific exports. | `./output` |
| `--format` | `<fmt>` | Export format filter: `fits`, `hdf5`, `vtk`, `all`. | `all` |
| `--steps` | `<N>` | Number of discrete integration cycles. | `1000` |
| `--dt` | `<value>` | Discrete physical time step per cycle. | `0.01` |
| `--width` | `<pixels>` | Horizontal raytracing raster resolution. | `1920` |
| `--height` | `<pixels>` | Vertical raytracing raster resolution. | `1080` |
| `--validate-benchmarks` | None | Executes the formal analytical test suite. | Disabled |
| `--verbose` | None | Enables per-step constraint telemetry logging. | Disabled |
| `--help`, `-h` | None | Prints CLI argument syntax & exits. | None |

### 4.3. Execution Examples

#### Running Analytical Validation Suite

```bash
./headless_exporter --validate-benchmarks
```

#### Executing 4K Raytraced Kerr Black Hole Disk Render

```bash
./headless_exporter --scenario scenarios/kerr_accretion_disk.yaml --output-dir ./exports --width 3840 --height 2160 --format fits
```

#### High-Step N-Body Trajectory Generation in HDF5

```bash
./headless_exporter --scenario scenarios/solar_system_mercury.yaml --output-dir ./data --steps 100000 --dt 50.0 --format hdf5
```

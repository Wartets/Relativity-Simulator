# Relativistic Engine - Command Line Interface & REPL Reference

## 1. Executable Binaries

The engine provides two standalone executables:
1. `engine_cli`: Master simulation host containing the non-blocking command interpreter and the graphical multi-window instrumentation workspace.
2. `headless_exporter`: Non-interactive batch processor for cluster execution, parameter sweeps, and validation benchmarks.

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
| `step` | `[N]` | Advances the simulation by exactly `N` ticks (default: 1) and pauses. | `step 10` |
| `warp` | `<factor>` | Sets the logical time dilation factor ($> 0.0$). | `warp 5.0` |
| `tickrate` | `<Hz>` | Adjusts the scheduler logical frequency (10.0 to 1000.0 Hz). | `tickrate 120.0` |
| `reset` | None | Resets simulation time to zero and restores initial conditions. | `reset` |
| `status` | None | Prints the current state of the scheduler and active parameters. | `status` |
| `quit`, `exit`, `shutdown` | None | Terminates all worker threads and exits the application. | `quit` |

### 2.2. Camera & Observer Navigation Commands

| Command | Arguments | Description | Example |
| :--- | :--- | :--- | :--- |
| `camera reset` | None | Restores the default observer position and orientation. | `camera reset` |
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

Supported metric names: `FlatMinkowski`, `Schwarzschild`, `Kerr`, `KerrSchild`, `ReissnerNordstrom`, `KerrNewman`, `SchwarzschildDeSitter`, `FLRW`, `MorrisThorne`, `Alcubierre`, `BSSN`.

Supported integrator names: `RK45`, `CashKarp`, `Vernier9`, `GaussLegendre4`, `GaussLegendre6`, `Hermite4`.

### 2.4. Parameter Modification Commands (`set`)

The `set` command updates physical properties, optical settings, and solver tolerances:
`set <parameter> <value>`

| Parameter Key | Value Type | Physical / Numerical Meaning | Example |
| :--- | :--- | :--- | :--- |
| `mass` | Float | Central body mass $M$. | `set mass 1.0` |
| `spin` | Float | Central body spin parameter $a$. | `set spin 0.94` |
| `charge` | Float | Central body net electric charge $Q$. | `set charge 0.5` |
| `lambda` | Float | Cosmological constant $\Lambda$. | `set lambda 1.1e-52` |
| `throat` | Float | Morris-Thorne wormhole throat radius $b_0$. | `set throat 5.0` |
| `warp_velocity`, `warp_vel` | Float | Alcubierre metric apparent velocity $v_s$. | `set warp_velocity 2.0` |
| `projection`, `proj` | Integer | Projection (0: Pinhole, 1: AutoZoom, 2: Fisheye, 3: 360). | `set projection 1` |
| `timeflow`, `time_flow` | Integer | Time frame (0: Proper time $\tau$, 1: Coordinate time $t$). | `set timeflow 0` |
| `speed`, `cameran_speed` | Float | Navigation movement rate. | `set speed 15.0` |
| `fov` | Float | Field of view in degrees. | `set fov 75.0` |
| `exposure` | Float | Exposure compensation in EV units. | `set exposure 1.5` |
| `tonemapper`, `tonemap` | Integer | Operator (0: Linear, 1: ACES, 2: Logarithmic, 3: Reinhard). | `set tonemapper 1` |
| `rtol` | Float | Relative integration tolerance. | `set rtol 1e-12` |
| `atol` | Float | Absolute integration tolerance. | `set atol 1e-15` |
| `min_step` | Float | Minimum integration step size bound. | `set min_step 1e-10` |
| `max_step` | Float | Maximum integration step size bound. | `set max_step 1.0` |
| `render_scale`, `scale` | Float | Internal raster resolution scaling factor (0.1 to 4.0). | `set scale 1.25` |
| `ray_steps`, `steps_limit` | Integer | Maximum integration steps per ray (64 to 16384). | `set ray_steps 4096` |
| `performance`, `perf` | Integer | Performance profile preset (0 to 4). | `set performance 3` |
| `camera_mode`, `cam_mode` | Integer | Mode (0: Free Fly, 1: Orbit, 2: Spherical, 3: Cockpit). | `set camera_mode 1` |
| `tickrate` | Float | Scheduler frequency in Hertz. | `set tickrate 60.0` |
| `<custom_name>` | Float | User-defined custom scalar quantity. | `set gas_density 1e-4` |

---

## 3. Graphical Interface & Interactive Controls

When executed without the `--headless` flag, `engine_cli` initializes a graphical OpenGL context managed through ImGui and ImPlot.

### 3.1. Navigation & Viewport Keybindings

| Key / Input | Action | Mode |
| :--- | :--- | :--- |
| `W` / `Z` | Translate forward | Free Fly / Cockpit / Orbit zoom |
| `S` | Translate backward | Free Fly / Cockpit / Orbit zoom |
| `A` / `Q` | Translate / Orbit left | Free Fly / Orbit / Spherical |
| `D` | Translate / Orbit right | Free Fly / Orbit / Spherical |
| `Space` / `E` | Translate upward | Free Fly |
| `C` / `Ctrl` | Translate downward | Free Fly |
| `J` / `Page Up` | Roll counter-clockwise | Free Fly |
| `K` / `Page Down` | Roll clockwise | Free Fly |
| `Shift` (Hold) | High-speed boost modifier ($4\times$) | All Modes |
| `Ctrl` / `Alt` (Hold) | Precision crawl modifier ($0.2\times$) | All Modes |
| `Right Click` + Drag | Angular look (Pitch / Yaw) | All Modes |
| `Alt` + `Left Click` + Drag | Alternative angular look | All Modes |
| `Mouse Scroll` | Adjust camera field of view | All Modes |

### 3.2. Global Function Shortcuts

| Shortcut | Description |
| :--- | :--- |
| `F1` | Toggle Master Simulation Controls window. |
| `F2` | Apply Multi-Window Detached workspace layout. |
| `F3` | Apply Docked Workspace layout container. |
| `F4` | Apply Viewport Fullscreen Focus mode. |
| `F5` / `Space` | Toggle simulation pause / resume state. |
| `F6` | Advance simulation by a single logical tick. |
| `F7` | Reset simulation time and orbital clocks. |
| `F9` | Cycle through camera navigation modes. |
| `F10` | Toggle Telemetry & Invariants window. |
| `F11` | Toggle Performance & Diagnostics windows. |
| `F12` | Snap camera to standard equatorial position ($r = 50M$). |
| `F` | Orient camera toward coordinate origin $(0, 0, 0)$. |
| `Home` | Reset camera roll angle to zero. |
| `[` / `]` | Decrease / Increase navigation movement speed by 5%. |
| `Alt` + `1` | Snap viewpoint to equatorial front ($r = 50M$). |
| `Alt` + `3` | Snap viewpoint to equatorial side ($r = 50M$). |
| `Alt` + `7` | Snap viewpoint to north pole ($z = 50M$). |
| `Alt` + `9` | Snap viewpoint to south pole ($z = -50M$). |
| `Alt` + `5` | Snap viewpoint to ISCO orbital radius. |

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
| `--help`, `-h` | None | Prints CLI argument syntax and exits. | None |

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

# Relativistic Engine - Command Line Interface & REPL Reference

## 1. Executable Binaries Overview

The engine builds two primary native executables:
1. `engine_cli`: Master interactive terminal loop with optional multi-window OpenGL/ImGui telemetry display.
2. `headless_exporter`: High-throughput non-interactive batch processor for cluster execution, parameter sweeps, and data generation.

---

## 2. Interactive Terminal Commands (REPL)

When launched in interactive mode, the engine presents the non-blocking master prompt:
`relativistic> `

### 2.1. Simulation Control Commands

| Command | Arguments | Description | Example |
| :--- | :--- | :--- | :--- |
| `pause` | None | Halts the advancement of logical simulation time. | `pause` |
| `resume` | None | Resumes continuous simulation progression. | `resume` |
| `step` | `[N]` | Advances simulation by exactly `N` ticks (default: 1) and pauses. | `step 10` |
| `warp` | `<factor>` | Sets the logical time dilation factor ($> 0.0$). | `warp 5.0` |
| `tickrate` | `<Hz>` | Adjusts the scheduler execution frequency (10.0 to 1000.0 Hz). | `tickrate 120.0` |
| `reset` | None | Resets simulation time to 0.0 and restores default parameters. | `reset` |
| `status` | None | Outputs the full telemetry snapshot of the orchestrator state. | `status` |
| `quit`, `exit`, `shutdown` | None | Safely flushes buffers and terminates all worker threads. | `quit` |

### 2.2. Parameter Modification Commands (`set`)

| Parameter Key | Value Type | Physical Meaning | Example |
| :--- | :--- | :--- | :--- |
| `mass` | Float | Central body / black hole mass $M$ ($M_\odot$ or geometrized). | `set mass 10.0` |
| `spin` | Float | Dimensionless Kerr spin parameter $a \in [-0.9999, 0.9999]$. | `set spin 0.94` |
| `charge` | Float | Reissner-Nordström / Kerr-Newman electrical charge $Q$. | `set charge 0.2` |
| `lambda` | Float | Cosmological constant $\Lambda$ in $\text{m}^{-2}$. | `set lambda 1.1e-52` |
| `throat` | Float | Morris-Thorne wormhole throat radius $b_0$. | `set throat 5.0` |
| `warp_velocity` | Float | Alcubierre metric apparent transport velocity $v_s / c$. | `set warp_velocity 2.5` |
| `projection` | Integer | Camera projection (0: Pinhole, 1: AutoZoom, 2: Fisheye, 3: 360). | `set projection 2` |
| `timeflow` | Integer | Time frame mode (0: Comobile proper $\tau$, 1: Coordinate $t$). | `set timeflow 1` |
| `<custom_name>` | Float | User-defined custom scalar property. | `set gas_density 1.5e-3` |

---

## 3. Headless Batch Exporter (`headless_exporter`)

### 3.1. Syntax & Arguments

```bash
./headless_exporter [OPTIONS]
```

### 3.2. Command Line Flags

- `--scenario <path>`: Specifies the input declarative YAML scenario file.
- `--output-dir <path>`: Sets the target directory for scientific file exports (default: `./output`).
- `--format <fmt>`: Filters export formats: `fits`, `hdf5`, `vtk`, `all` (default: `all`).
- `--steps <N>`: Sets the total number of integration cycles to execute.
- `--dt <value>`: Sets the discrete numerical step size in physical units.
- `--width <pixels>`: Sets the horizontal raytracing raster resolution (e.g. `3840`).
- `--height <pixels>`: Sets the vertical raytracing raster resolution (e.g. `2160`).
- `--validate-benchmarks`: Runs the automated analytical validation test suite and returns exit code 0 on strict convergence.
- `--verbose`: Prints per-step stepsize, constraint residuals, and conservation drift.

### 3.3. Batch Processing Examples

#### Example 1: Run Full Analytical Test Suite

```bash
./headless_exporter --validate-benchmarks
```

#### Example 2: Execute Kerr Novikov-Thorne Scenario with 4K Render

```bash
./headless_exporter --scenario scenarios/kerr_accretion_disk.yaml --output-dir ./results --width 3840 --height 2160 --format all
```

#### Example 3: Long-term Post-Newtonian Solar System Integration

```bash
./headless_exporter --scenario scenarios/solar_system_mercury.yaml --steps 50000 --dt 100.0 --format hdf5
```

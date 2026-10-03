# Relativity-Simulator

Deterministic simulation and ray-tracing engine for General Relativity, Post-Newtonian dynamics, relativistic hydrodynamics, and polarized radiative transfer in curved spacetimes.

---

## Documentation Map

Primary references:

- [System Description](docs/DESCRIPTION.md): scientific scope, runtime modes, validation targets.
- [Technical Architecture](docs/ARCHITECTURE.md): subsystem structure, scheduling, concurrency, precision model.
- [Technical Manual](docs/TECHNICAL_MANUAL.md): component-level implementation and engineering constraints.
- [Mathematical Formulation](docs/MATHEMATICAL_FORMULATION.md): equations, metrics, transport formalisms, PN terms.
- [File Formats & I/O](docs/FILE_FORMATS.md): YAML schema, FITS/HDF5/VTK formats, SPICE/HORIZONS ingestion.
- [CLI & REPL Reference](docs/CLI_REFERENCE.md): interactive commands and headless exporter options.
- [Documentation Index](docs/INDEX.md): index of the documentation set.
- [Source Tree](docs/TREE.md): repository layout.

---

## Key Features

- **Spacetime Models**
  - Analytic metrics: [Minkowski](docs/MATHEMATICAL_FORMULATION.md#11-minkowski-metric-flat-spacetime), [Schwarzschild](docs/MATHEMATICAL_FORMULATION.md#12-schwarzschild-metric), [Kerr](docs/MATHEMATICAL_FORMULATION.md#13-kerr-metric-rotating-black-hole), [Reissner–Nordström](docs/MATHEMATICAL_FORMULATION.md#14-reissner-nordström-metric-charged-black-hole), [Kerr–Newman](docs/MATHEMATICAL_FORMULATION.md#15-kerr-newman-metric-charged-rotating-black-hole), [Schwarzschild–de Sitter (Kottler)](docs/MATHEMATICAL_FORMULATION.md#16-schwarzschild-de-sitter--kottler-metric) ([Catalog & Gauges](docs/DESCRIPTION.md#41-analytical-vacuum--electrovacuum-solutions)).
  - Coordinate forms: [Boyer–Lindquist](docs/MATHEMATICAL_FORMULATION.md#13-kerr-metric-rotating-black-hole), [Kerr–Schild Cartesian](docs/MATHEMATICAL_FORMULATION.md#13-kerr-metric-rotating-black-hole), [Eddington–Finkelstein](docs/MATHEMATICAL_FORMULATION.md#12-schwarzschild-metric), [Painlevé–Gullstrand](docs/MATHEMATICAL_FORMULATION.md#12-schwarzschild-metric), [isotropic](docs/MATHEMATICAL_FORMULATION.md#12-schwarzschild-metric) ([Coordinate Systems](docs/DESCRIPTION.md#42-coordinate-systems--gauges)).
  - Additional models: [FLRW](docs/MATHEMATICAL_FORMULATION.md#17-flrw-metric-cosmological-spacetime), [Morris–Thorne](docs/MATHEMATICAL_FORMULATION.md#18-morris-thorne-traversable-wormhole), [Alcubierre](docs/MATHEMATICAL_FORMULATION.md#19-alcubierre-warp-drive-metric) ([Cosmological & Exotic Spacetimes](docs/DESCRIPTION.md#43-cosmological--exotic-spacetimes)).
  - Image rendering of Schwarzschild–de Sitter, FLRW, Morris–Thorne and Alcubierre uses the Schwarzschild null tracer ([Technical Manual](docs/TECHNICAL_MANUAL.md#metric-usage-in-softwarecomputeengine)).
  - Numerical spacetime support: [3+1 BSSN metric grids](docs/DESCRIPTION.md#44-31-numerical-relativity-grids-bssn) with spatial and temporal interpolation ([Metric Modules](docs/ARCHITECTURE.md#22-spacetime-metric-modules)).
  - Metric invariants and curvature tensors: [Riemann, Ricci, and Kretschmann invariants](docs/MATHEMATICAL_FORMULATION.md#23-riemann-tensor-ricci-tensor--kretschmann-scalar) ([Diagnostics Window](docs/TECHNICAL_MANUAL.md#28-orchestration-scheduling--user-interface)).

- **Geodesics and Integrators**
  - Adaptive explicit integrators: [RK45 Dormand–Prince](docs/ARCHITECTURE.md#23-differential-solvers--geodesic-integrators), [Cash–Karp](docs/ARCHITECTURE.md#23-differential-solvers--geodesic-integrators), [Vernier 9](docs/ARCHITECTURE.md#23-differential-solvers--geodesic-integrators).
  - Symplectic integrators: [Gauss–Legendre orders 4 and 6](docs/ARCHITECTURE.md#23-differential-solvers--geodesic-integrators), [Forest–Ruth post-Newtonian scheme](docs/ARCHITECTURE.md#34-orchestrator-state-advance).
  - Predictor-corrector: [Hermite 4th-order Aarseth scheme](docs/ARCHITECTURE.md#23-differential-solvers--geodesic-integrators).
  - Horizon-aware stepping and adaptive step-control infrastructure ([Mathematical Formulation](docs/MATHEMATICAL_FORMULATION.md#82-step-control-termination-and-space-skipping), [Technical Manual](docs/TECHNICAL_MANUAL.md#23-differential-solvers--integrators)).
  - Image rendering integrates null rays with a fixed-step RK4 scheme; the integrator selected at runtime determines the N-body integration only ([Technical Manual](docs/TECHNICAL_MANUAL.md#runtime-use-of-the-integrator-selection)).

- **Relativistic Dynamics**
  - Post-Newtonian N-body dynamics: conservative [1PN Einstein-Infeld-Hoffmann](docs/MATHEMATICAL_FORMULATION.md#31-conservative-1pn-acceleration-einstein-infeld-hoffmann), [2PN, and 3PN](docs/DESCRIPTION.md#51-post-newtonian-multi-body-dynamics) terms.
  - Dissipative terms: [2.5PN radiation reaction](docs/MATHEMATICAL_FORMULATION.md#32-25pn-radiation-reaction-acceleration) and [3.5PN](docs/DESCRIPTION.md#51-post-newtonian-multi-body-dynamics) radiation dissipation.
  - Spin dynamics: [spin-orbit and spin-spin couplings](docs/MATHEMATICAL_FORMULATION.md#33-spin-orbit--spin-spin-coupling).
  - Dedicated [Hulse–Taylor binary pulsar model](docs/DESCRIPTION.md#113-standard-simulation-scenarios) ([Dynamics Subsystem](docs/TECHNICAL_MANUAL.md#24-post-newtonian-multi-body-subsystem)).
  - Interaction layers: [Coulomb, magnetic dipole, collision, thermodynamic, fragmentation, and annihilation](docs/DESCRIPTION.md#54-body-interactions--spacetime-source-bodies) ([Dynamics Subsystem](docs/TECHNICAL_MANUAL.md#24-post-newtonian-multi-body-subsystem)).
  - [Spacetime-source bodies](docs/DESCRIPTION.md#54-body-interactions--spacetime-source-bodies) with Kerr horizons, horizon absorption, and body merging ([State Advance](docs/ARCHITECTURE.md#34-orchestrator-state-advance)).
  - [Bulk body actions](docs/TECHNICAL_MANUAL.md#24-post-newtonian-multi-body-subsystem) (velocity and spin assignment, grid snapping, scattering, mass equalization, parameter equalization, culling; [Keybinds](docs/CLI_REFERENCE.md#34-additional-rebindable-actions)).

- **Gravimetry and Large-Scale Gravity**
  - [Spherical harmonic gravity expansions](docs/DESCRIPTION.md#52-spherical-harmonics--high-degree-geodesy) up to degree and order 32 ($J_n$, $C_{nm}$, $S_{nm}$); [Architecture](docs/ARCHITECTURE.md#24-post-newtonian-dynamics--gravimetry)).
  - [Analytical orbital precession](docs/ARCHITECTURE.md#24-post-newtonian-dynamics--gravimetry) and [tidal Love number perturbations](docs/ARCHITECTURE.md#24-post-newtonian-dynamics--gravimetry).
  - Dark matter modules: [NFW, Einasto, Burkert, and Hernquist profiles](docs/DESCRIPTION.md#53-dark-matter-halos--alternative-gravitational-theories), [Barnes–Hut octree acceleration](docs/DESCRIPTION.md#53-dark-matter-halos--alternative-gravitational-theories), [MOND, TeVeS, and \(f(R)\) Chameleon models](docs/ARCHITECTURE.md#25-dark-matter--modified-gravity).

- **Relativistic Hydrodynamics and Compact Objects**
  - [GRHD/GRMHD 3+1 conservative formulation](docs/MATHEMATICAL_FORMULATION.md#4-relativistic-hydrodynamics-grhdgrmhd) with [WENO5-JS, WENO5-Z, and MP5 reconstructions](docs/DESCRIPTION.md#61-curved-spacetime-hydrodynamics).
  - Approximate Riemann solvers: [HLL, HLLC, and HLLD](docs/DESCRIPTION.md#61-curved-spacetime-hydrodynamics) ([Architecture](docs/ARCHITECTURE.md#26-relativistic-hydrodynamics-grhdgrmhd)).
  - Divergence control: [constrained transport 2D](docs/DESCRIPTION.md#61-curved-spacetime-hydrodynamics) enforcing \(\nabla \cdot \mathbf{B} = 0\) ([Architecture](docs/ARCHITECTURE.md#26-relativistic-hydrodynamics-grhdgrmhd)).
  - EOS models: [ideal gas, Synge, Mathews, relativistic Fermi gas, polytropic, and tabulated nuclear EOS](docs/DESCRIPTION.md#62-multi-regime-equations-of-state-eos) ([Mathematical Formulation](docs/MATHEMATICAL_FORMULATION.md#43-equations-of-state-eos)).
  - Compact-object models: [Tolman-Oppenheimer-Volkoff (TOV) solver](docs/MATHEMATICAL_FORMULATION.md#43-equations-of-state-eos), [Novikov–Thorne thin disk](docs/MATHEMATICAL_FORMULATION.md#51-novikov-thorne-thin-disk-profile), and [Fishbone–Moncrief torus](docs/DESCRIPTION.md#63-relativistic-accretion-disks--tori).
  - [Conservative-to-primitive inversion solver](docs/ARCHITECTURE.md#26-relativistic-hydrodynamics-grhdgrmhd) ([Technical Manual](docs/TECHNICAL_MANUAL.md#25-relativistic-hydrodynamics-grhdgrmhd--stellar-physics)).

- **Radiative Transfer and Rendering**
  - [Backward null geodesic ray tracing](docs/DESCRIPTION.md#91-backward-null-geodesic-raytracing) in curved spacetimes ([Mathematical Formulation](docs/MATHEMATICAL_FORMULATION.md#8-image-formation-in-the-renderer)).
  - [Polarized transfer in full Stokes representation](docs/MATHEMATICAL_FORMULATION.md#6-polarized-radiative-transfer) \((I,Q,U,V)\) via Delano matrix exponentials; image rendering uses [blackbody disk emission integrated with CIE 1931 matching functions](docs/MATHEMATICAL_FORMULATION.md#82-step-control-termination-and-space-skipping).
  - Radiative processes: [synchrotron emission and absorption, thermal bremsstrahlung, and inverse Compton scattering](docs/DESCRIPTION.md#64-radiative-processes--local-emission) ([Architecture](docs/ARCHITECTURE.md#27-polarized-radiative-transfer--optics)).
  - [Continuous spectral pipeline with CIE 1931 observer convolution](docs/DESCRIPTION.md#93-continuous-spectral-pipeline--cie-1931-integration) ([Mathematical Formulation](docs/MATHEMATICAL_FORMULATION.md#82-step-control-termination-and-space-skipping)).
  - [Vulkan compute pipeline in native FP64](docs/ARCHITECTURE.md#29-render-pipeline) with [CPU SIMD (AVX2/AVX-512/Neon) and scalar fallbacks](docs/TECHNICAL_MANUAL.md#29-rendering-back-ends) in double and single precision.
  - [Ray-traced ellipsoidal celestial bodies](docs/MATHEMATICAL_FORMULATION.md#84-celestial-body-shading) with procedural surface layers, atmospheric rims, central-source lighting, shadows, and relativistic Doppler beaming ([Technical Manual](docs/TECHNICAL_MANUAL.md#29-rendering-back-ends)).
  - [Procedural starfield or equirectangular panorama sky](docs/TECHNICAL_MANUAL.md#29-rendering-back-ends), [four HDR tone mapping operators, and CPU color grading](docs/MATHEMATICAL_FORMULATION.md#83-tone-mapping-and-color-grading).
  - [Schematic projection overlay](docs/ARCHITECTURE.md#212-schematic-view-and-hud) with first-order light deflection ([Mathematical Formulation](docs/MATHEMATICAL_FORMULATION.md#85-schematic-overlay-deflection)) and [up to eight secondary observer viewports](docs/TECHNICAL_MANUAL.md#28-orchestration-scheduling--user-interface).
  - Still frame and sequence image streaming in [ten output formats](docs/FILE_FORMATS.md#6-image-output-formats) ([Capture Subsystem](docs/ARCHITECTURE.md#210-capture-subsystem)).

- **Deterministic Runtime and Reproducibility**
  - [Synchronous input event journal primitive](docs/DESCRIPTION.md#112-synchronous-command-journal--replay) (`EventJournal`), decoupled from the command queue.
  - [Engine signature computation and SHA-256 ABI verification](docs/ARCHITECTURE.md#211-profiling-logging-and-persistence).
  - [Lock-free SPSC command and result queues](docs/ARCHITECTURE.md#32-asynchronous-lock-free-command-queue) and [static worker thread pool](docs/ARCHITECTURE.md#33-thread-pool--raytracing-work-distribution).
  - [Allocation-free hot paths utilizing cache-aligned linear memory arenas](docs/ARCHITECTURE.md#1-architectural-principles--system-paradigms) ([Technical Manual](docs/TECHNICAL_MANUAL.md#11-design-constraints)).

- **Scientific Data and Interoperability**
  - [Declarative YAML scenario parser and serializer](docs/FILE_FORMATS.md#1-declarative-scenario-definition-yaml) with [hierarchical path resolution](docs/ARCHITECTURE.md#35-scenario-resolution-and-startup).
  - [SPICE binary SPK kernel evaluation](docs/FILE_FORMATS.md#52-spice-binary-spk-kernels-bsp), [NASA JPL HORIZONS vector table ingestion](docs/FILE_FORMATS.md#51-nasa-jpl-horizons-api-responses), and [astronomical frame transformations](docs/ARCHITECTURE.md#61-ephemeris--orbital-mechanics-ingestion).
  - [FITS 4.0 2D radiance image and 3D spectral cube export](docs/FILE_FORMATS.md#2-flexible-image-transport-system-fits) with WCS metadata headers ([Serialization](docs/ARCHITECTURE.md#62-scientific-data-serialization)).
  - [Custom binary HDF5-signature container serialization](docs/FILE_FORMATS.md#3-hierarchical-data-format-h5) for worldlines, metric series, and covariance matrices.
  - [VTK XML PolyData (VTP) geometry export](docs/FILE_FORMATS.md#4-visualization-toolkit-polydata-vtp) for rays, trajectories, and event horizon meshes.
  - [Incremental image codecs and stream writers](docs/FILE_FORMATS.md#6-image-output-formats) ([Serialization](docs/ARCHITECTURE.md#62-scientific-data-serialization)).

- **Capture and Experiment Control**
  - [Scriptable multi-segment camera motion paths](docs/ARCHITECTURE.md#210-capture-subsystem) with 21 shape layers, 46 easing curves, and expression evaluations ([Mathematical Formulation](docs/MATHEMATICAL_FORMULATION.md#9-motion-script-mathematics)).
  - [Motion script serialization and scripted timeline events](docs/FILE_FORMATS.md#9-motion-script-and-capture-settings-files) ([Capture Subsystem](docs/ARCHITECTURE.md#210-capture-subsystem)).
  - [Deterministic and real-time capture pipelines](docs/ARCHITECTURE.md#210-capture-subsystem) with banded rendering, sub-frame temporal averaging, and [automated ffmpeg video encoding](docs/FILE_FORMATS.md#7-capture-session-directory).
  - [Multi-channel physics telemetry recording across 54 kinematic and relativistic channels in seven file formats](docs/FILE_FORMATS.md#8-telemetry-recording-files) ([Capture Modules](docs/TECHNICAL_MANUAL.md#210-capture-modules)).
  - [Path preview generation](docs/ARCHITECTURE.md#210-capture-subsystem) for visual trajectory inspection in the schematic overlay ([Capture Modules](docs/TECHNICAL_MANUAL.md#210-capture-modules)).

- **Interactive Workspace**
  - [Primary viewport window](docs/ARCHITECTURE.md#29-render-pipeline) with camera transport toolbar, hold-to-zoom magnification, and a [configurable 25-element HUD](docs/ARCHITECTURE.md#212-schematic-view-and-hud) ([Technical Manual](docs/TECHNICAL_MANUAL.md#28-orchestration-scheduling--user-interface)).
  - [Multi-window docking workspace](docs/ARCHITECTURE.md#52-graphical-multi-window-workspace-ui-architecture) with dedicated interfaces for master simulation controls, scenario selector, celestial body manager, curvature diagnostics, spectrograph, performance optimization, profiling, physical constants, key bindings, log console, and Capture Studio ([Technical Manual](docs/TECHNICAL_MANUAL.md#28-orchestration-scheduling--user-interface)).
  - [Four navigation modes](docs/CLI_REFERENCE.md#32-navigation-modes) (Free-Fly 6-DOF, Orbit Center, Spherical Boyer-Lindquist, Rocket Thrust) and [67 rebindable actions with QWERTY and AZERTY presets](docs/CLI_REFERENCE.md#34-additional-rebindable-actions).
  - [Physical constants engine](docs/ARCHITECTURE.md#213-physical-constants-engine) supporting SI, Planck, and Custom presets, with [independent unit preferences across 19 physical quantities](docs/TECHNICAL_MANUAL.md#211-constants-units-and-expressions) and [dimensional expression parsing](docs/TECHNICAL_MANUAL.md#211-constants-units-and-expressions).
  - [Persistent configuration files](docs/FILE_FORMATS.md#10-persistent-configuration-files) under `config/` ([User Settings](docs/FILE_FORMATS.md#101-configuser_settingscfg), [Performance Profiler](docs/FILE_FORMATS.md#102-configperformance_profilercfg), [Capture Studio Settings](docs/FILE_FORMATS.md#9-motion-script-and-capture-settings-files)).

- **Uncertainty Quantification**
  - [Bounded interval arithmetic adhering to IEEE 1788](docs/MATHEMATICAL_FORMULATION.md#71-interval-arithmetic-ieee-1788) ([Description](docs/DESCRIPTION.md#72-uncertainty-propagation-frameworks)).
  - [Multidimensional zonotope enclosures with Girard order reduction](docs/MATHEMATICAL_FORMULATION.md#72-zonotope-enclosure) ([Description](docs/DESCRIPTION.md#72-uncertainty-propagation-frameworks)).
  - [Continuous Jacobi-Lyapunov covariance matrix differential propagation](docs/MATHEMATICAL_FORMULATION.md#73-lyapunov-covariance-matrix-propagation) ([Description](docs/DESCRIPTION.md#72-uncertainty-propagation-frameworks)).
  - [Generalized polynomial chaos expansions (gPCE) on Hermite and Legendre bases](docs/MATHEMATICAL_FORMULATION.md#74-generalized-polynomial-chaos-expansion-gpce) ([Description](docs/DESCRIPTION.md#72-uncertainty-propagation-frameworks)).

---

## Visual Overview

The visual renders and trajectory sequences below were recorded and generated using the integrated [Capture Studio](docs/ARCHITECTURE.md#210-capture-subsystem) workspace widget.

<p align="center">
  <img src="/assets/media/schwarzschild_preview.png" alt="Curved Spacetime Null Geodesic Ray-Tracing (Schwarzschild Metric, M = 60.00)" width="100%">
</p>

<p align="center">
  <img src="/assets/media/schwarzschild_orbit_passby.gif" alt="Schwarzschild Metric Flyby Trajectory (M = 60.00)" width="49.5%">
  <img src="/assets/media/schwarzschild_infall.gif" alt="Schwarzschild Metric Infall Trajectory (M = 60.00)" width="49.5%">
</p>

---

## Building and Installation

### Prerequisites
- C++23 compiler: GCC 13+, Clang 16+, or MSVC 2022 (v19.36+).
- CMake 3.25+.
- OpenGL and GLFW (via CMake FetchContent).
- Vulkan headers and loader (the compute context includes `vulkan/vulkan.h`); when no compute-capable device is present at run time, the CPU renderer is used.
- `glslangValidator` (optional): used by CMake to compile the compute shader.
- `ffmpeg` (optional, run time): video assembly in the Capture Studio.

### Standard Build (Release with LTO)
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_LTO=ON
cmake --build build --config Release -j $(nproc)
```

### Profile-Guided Optimization (PGO) Build

#### Instrumented build
```bash
cmake -B build-pgo -DCMAKE_BUILD_TYPE=Release -DENABLE_PGO_GENERATE=ON
cmake --build build-pgo --config Release -j $(nproc)
./build-pgo/headless_exporter --validate-benchmarks
```

#### Optimized build using profile data
```bash
cmake -B build-opt -DCMAKE_BUILD_TYPE=Release -DENABLE_PGO_USE=ON
cmake --build build-opt --config Release -j $(nproc)
```

### Running Test Suite
```bash
ctest --test-dir build --output-on-failure
```

### Generating Installation Packages (CPack)
```bash
cd build
cpack -G TGZ
cpack -G DEB  # Linux Debian/Ubuntu
cpack -G RPM  # Linux Fedora/RHEL
cpack -G NSIS # Windows Installer
```

---

## Quickstart

### Interactive REPL Mode
```bash
./build/engine_cli
```
Available interactive commands (`warp`, `step`, `set mass`, `status`, etc.) are documented in the [Interactive Terminal Commands (REPL)](docs/CLI_REFERENCE.md#2-interactive-terminal-commands-repl) and navigation keybindings in [Navigation & Viewport Keybindings](docs/CLI_REFERENCE.md#31-navigation--viewport-keybindings).

### Headless Batch Execution
```bash
./build/headless_exporter --validate-benchmarks
./build/headless_exporter --scenario scenarios/kerr_accretion_disk.yaml --output-dir ./output --width 3840 --height 2160
```
Command-line options are documented in [Headless Batch Exporter](docs/CLI_REFERENCE.md#4-headless-batch-exporter-headless_exporter), declarative scenario definitions in [Declarative Scenario Definition](docs/FILE_FORMATS.md#1-declarative-scenario-definition-yaml), and validation targets in [Verification Criteria & Benchmark Protocols](docs/DESCRIPTION.md#12-verification-criteria--benchmark-protocols).

---

## Third-Party Assets and Image Licenses

Sky and panorama assets:

- ambientCG assets (CC0 1.0): [assets/sky/ambientcg/LICENCE.txt](assets/sky/ambientcg/LICENCE.txt)
- ESO Milky Way panorama (CC BY 4.0): [assets/sky/eso/LICENCE.txt](assets/sky/eso/LICENCE.txt)

Earth and planetary assets:

- Solar System Scope Earth textures (CC BY 4.0): [assets/earth/solarsystemscope/LICENCE.txt](assets/earth/solarsystemscope/LICENCE.txt)

---

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.

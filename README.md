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

---

## Key Features

- **Spacetime Models**
  - Analytic metrics: Minkowski, Schwarzschild, Kerr, Reissner–Nordström, Kerr–Newman, Schwarzschild–de Sitter (Kottler).
  - Coordinate forms: Boyer–Lindquist, Kerr–Schild, Eddington–Finkelstein, Painlevé–Gullstrand, isotropic.
  - Additional models: FLRW, Morris–Thorne, Alcubierre.
  - Numerical spacetime support: 3+1 BSSN metric grids with spatial/temporal interpolation.

- **Geodesics and Integrators**
  - Adaptive explicit integrators: RK45 (Dormand–Prince), Cash–Karp, Vernier 9.
  - Symplectic integrators: Gauss–Legendre (order 4/6), Forest–Ruth / Yoshida.
  - Predictor-corrector: Hermite4 with Aarseth timestep control.
  - Horizon-aware stepping and adaptive step-control infrastructure.

- **Relativistic Dynamics**
  - Post-Newtonian N-body dynamics: conservative 1PN/2PN/3PN terms.
  - Dissipative terms: 2.5PN and 3.5PN radiation reaction.
  - Spin dynamics: spin-orbit and spin-spin couplings.
  - Dedicated Hulse–Taylor pulsar module.
  - Configurable interaction and body-action layers.

- **Gravimetry and Large-Scale Gravity**
  - Spherical harmonic gravity tools (\(J_n\), \(C_{nm}\), \(S_{nm}\)).
  - Orbital precession and tidal perturbation modules.
  - Dark matter modules: halo profiles, composite galaxy models, Barnes–Hut acceleration.

- **Relativistic Hydrodynamics and Compact Objects**
  - GRHD/GRMHD solver stack with WENO5-Z and MP5 reconstructions.
  - Riemann solvers: HLL, HLLC, HLLD.
  - Divergence control: constrained transport.
  - EOS support: analytic and tabulated nuclear EOS.
  - Compact-object models: TOV solver, Novikov–Thorne disk, Fishbone–Moncrief torus.
  - Conservative-to-primitive inversion pipeline.

- **Radiative Transfer and Rendering**
  - Backward ray tracing on curved spacetimes.
  - Polarized transfer in full Stokes form \((I,Q,U,V)\).
  - Processes: synchrotron, bremsstrahlung, inverse Compton.
  - Spectral integration with CIE color matching and display mapping.
  - Output paths for still frames and sequence export.

- **Deterministic Runtime and Reproducibility**
  - Deterministic replay support.
  - Engine signature and SHA-256 provenance utilities.
  - Lock-free SPSC queue primitives and thread-pool execution.
  - Allocation-aware hot paths with linear memory arenas.

- **Scientific Data and Interoperability**
  - Scenario serialization and locator utilities.
  - SPICE/HORIZONS ephemeris parsing and frame transforms.
  - FITS image/spectral cube export with WCS metadata.
  - HDF5 serialization for trajectories and tensor fields.
  - VTK PolyData XML export for external visualization.
  - Image codecs and stream writers.

- **Capture and Experiment Control**
  - Scriptable camera paths with easing and expression support.
  - Motion script I/O and scripted events.
  - Capture coordination, progress tracking, and physics recording.
  - Path preview generation for validation before rendering.

- **Uncertainty Quantification**
  - Interval arithmetic (IEEE 1788).
  - Zonotope propagation and order reduction.
  - Covariance transport along trajectories.
  - Generalized polynomial chaos workflows.

---

## Building and Installation

### Prerequisites
- C++23 compiler: GCC 13+, Clang 16+, or MSVC 2022 (v19.36+).
- CMake 3.25+.
- OpenGL and GLFW (via CMake FetchContent).

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
Run `help` to list available commands (`warp`, `step`, `set mass`, `status`, etc.).

### Headless Batch Execution
```bash
./build/headless_exporter --validate-benchmarks
./build/headless_exporter --scenario scenarios/kerr_accretion_disk.yaml --output-dir ./output --width 3840 --height 2160
```

---

## Third-Party Assets and Image Licenses

Sky and panorama assets:

- ambientCG assets (CC0 1.0): [assets/sky/ambientcg/LICENCE.txt](assets/sky/ambientcg/LICENCE.txt)
- ESO Milky Way panorama (CC BY 4.0): [assets/sky/eso/LICENCE.txt](assets/sky/eso/LICENCE.txt)

---

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.

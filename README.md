# Relativity-Simulator

A deterministic, computing and raytracing engine for General Relativity, Post-Newtonian celestial mechanics, relativistic hydrodynamics, and polarized radiative transfer across curved spacetimes.

---

## Key Features

- **Spacetime Geometries:**
  - Exact electrovacuum metrics: Minkowski, Schwarzschild, Kerr, Reissner-Nordström, Kerr-Newman, Schwarzschild-de Sitter / Kottler.
  - Regular formulations: Kerr-Schild, Eddington-Finkelstein, Painlevé-Gullstrand, Isotropic coordinates.
  - Cosmological & Exotic metrics: FLRW ($k \in \{-1, 0, 1\}$), Morris-Thorne traversable wormholes, Alcubierre warp drive.
  - Numerical Spacetimes: 3+1 BSSN metric grid with tricubic and quintic Hermite temporal interpolation.

- **Differential Integrators:**
  - Adaptive explicit: Runge-Kutta 4/5 (Dormand-Prince), Cash-Karp, Vernier 9 (16 stages, 9th order).
  - Symplectic: Gauss-Legendre (4th & 6th order), Forest-Ruth / Yoshida.
  - Predictor-corrector: Hermite 4th-order with Aarseth variable timestep control.

- **Post-Newtonian Celestial Dynamics:**
  - Conservative orders: 1PN, 2PN, 3PN.
  - Radiation reaction: 2.5PN and 3.5PN gravitational wave dissipation.
  - Spin couplings: Spin-Orbit (Lense-Thirring / geodetic) and Spin-Spin precession.
  - Spherical Harmonics: High-degree gravitational potential expansions ($J_2, J_3, J_4, C_{nm}, S_{nm}$).

- **Relativistic Hydrodynamics (GRHD / GRMHD):**
  - High-Resolution Shock Capturing: WENO5-Z, MP5 spatial reconstruction.
  - Riemann Solvers: HLL, HLLC, HLLD.
  - Equations of State: Ideal gas, Synge, Mathews, Relativistic Degenerate Fermi gas, Polytropic, and 3D Tabulated Nuclear tables (SFHo, Shen, LS220, SLy4, APR4).
  - Accretion Models: Novikov-Thorne thin disk, Fishbone-Moncrief thick torus.
  - Divergence-Free Magnetism: 2D/3D Constrained Transport ($\nabla \cdot \mathbf{B} = 0$).

- **Polarized Radiative Transfer & CIE Spectrum Pipeline:**
  - Raytracing: GPU Compute shader pipeline with native FP64 and Double-Single arithmetic.
  - Full Stokes transport: $\mathbf{S} = (I, Q, U, V)^T$ with Delano matrix exponential integration.
  - Radiative processes: Relativistic synchrotron, Bremsstrahlung, inverse Compton scattering with Maxwell-Jüttner velocity sampling.
  - Radiometry: Continuous spectral integration ($10^{-14}$ to $10^{3}$ m) convolved with CIE 1931 color matching functions and ACES HDR tonemapping.

- **Formal Uncertainty Quantification:**
  - Interval arithmetic (IEEE 1788) & Zonotopes with Girard order reduction.
  - Continuous Lyapunov phase covariance propagation along geodesics.
  - Generalized Polynomial Chaos Expansion (gPCE) on Gauss-Hermite / Gauss-Legendre quadratures.

- **Scientific I/O:**
  - NASA JPL SPICE SPK binary kernels and HORIZONS API ingestion.
  - FITS 2D images and 3D spectral cubes with WCS metadata.
  - HDF5 trajectory and metric tensor series serialization.
  - VTK PolyData XML format for ParaView/VisIt rendering.

---

## Building and Installation

### Prerequisites
- C++23 compliant compiler: GCC 13+, Clang 16+, or MSVC 2022 (v19.36+).
- CMake 3.25+.
- OpenGL and GLFW (automatically fetched via CMake FetchContent).

### Standard Build (Release with LTO)
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_LTO=ON
cmake --build build --config Release -j $(nproc)
```

### Profile-Guided Optimization (PGO) Build

#### 1. Generate Instrument Build:
```bash
cmake -B build-pgo -DCMAKE_BUILD_TYPE=Release -DENABLE_PGO_GENERATE=ON
cmake --build build-pgo --config Release -j $(nproc)
./build-pgo/headless_exporter --validate-benchmarks
```

#### 2. Build with Profile Data:
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

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.

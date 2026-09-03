# Technical Architecture & System Design

## 1. Architectural Principles & System Paradigms

The Relativistic Engine is an ISO C++23 simulation and visualization framework for relativistic mechanics and raytracing. The system is structured into four functional layers:
- Core Analytical & Numerical Physics Engine: Stateless, thread-parallel libraries executing tensor algebra, geodesic integration, gravitational multi-body dynamics, relativistic hydrodynamics, and uncertainty quantification.
- Simulation Runtime & State Orchestrator: A synchronous deterministic state machine driven by a fixed logical time-step scheduler, communicating with external control layers via lock-free queues.
- Render & Compute Pipeline: Hardware-accelerated GPU compute pipelines & multithreaded SIMD software compute engines performing backward null geodesic raytracing, polarized radiative transfer, & spectral reduction.
- Presentation, Instrumentation & Workspace Layer: An interactive multi-window graphical user interface with docking workspaces, real-time telemetry dashboards, spectrographs, & a non-blocking master command-line interpreter (REPL).

### Fundamental Design Paradigms

- Zero-Allocation Hot Path: Differential equations, tensor contractions, raytracing passes, and uncertainty loops execute without dynamic heap allocations. Core structures utilize pre-allocated memory arenas (`LinearMemoryArena`) aligned to 64-byte and 128-byte cache boundaries.
- Static Compile-Time Polymorphism: Elimination of virtual table dispatch across inner loops through C++23 concepts (`SpacetimeMetric`) and template specializations.
- Explicit SIMD Register Vectorization: Use of explicit vectorization abstractions (`SimdVec<T, Width>`, `SimdMask<T, Width>`, & `GeodesicBundle<T, Width>`) processing multiple geodesic rays & phase states concurrently across AVX2, AVX-512, & ARM Neon architectures.
- Strict Bit-Level Determinism: Guaranteed reproducibility across identical hardware targets via explicit-state pseudo-random number generators (`PCG64Engine`) & fixed-step temporal scheduling.

---

## 2. Core Subsystems & Computational Hierarchy

The system architecture is organized into modular subsystems operating across decoupled boundaries:

### 2.1. Tensor Algebra & Curvature Evaluation Layer

- Static Tensor Primitives: `Tensor<T, Rank, Dim>` defines cache-aligned multidimensional tensor storage with compile-time rank, dimension, & flat index resolution.
- Metric Inversion & Contraction: `inverse_metric_4x4` & `determinant_4x4` perform analytical cofactor inversions for 4D spacetime metrics; `contract` & `contract_tensors` execute general Einstein summation contractions.
- Christoffel Evaluation: `compute_christoffel` dynamically dispatches to exact analytical formulations when provided by the metric or evaluates numerical derivatives via 8th-order centered finite difference stencils (`compute_christoffel_numerical`).
- Curvature Invariants: `RiemannComputer` computes the Riemann tensor $R^\rho_{\phantom{\rho}\sigma\mu\nu}$, Ricci tensor $R_{\mu\nu}$, Ricci scalar $R$, & Kretschmann invariant $K_1 = R^{\alpha\beta\gamma\delta} R_{\alpha\beta\gamma\delta}$.

### 2.2. Spacetime Metric Modules

Every spacetime implementation satisfies the `SpacetimeMetric` concept, requiring `metric_tensor`, `inverse_metric`, `christoffel_symbols`, & `speed_of_light`:
- Analytical Vacuum Spacetimes: `FlatMinkowskiMetric`, `SchwarzschildMetric`, & `KerrMetric`.
- Regularized Coordinate Gauges: `SchwarzschildIsotropicMetric`, `PainleveGullstrandMetric`, `EddingtonFinkelsteinMetric`, & Cartesian `KerrSchildMetric`.
- Electrovacuum & Cosmological Spacetimes: `ReissnerNordstromMetric`, `KerrNewmanMetric`, `SchwarzschildDeSitterMetric`, & `FLRWMetric`.
- Exotic Spacetimes: `MorrisThorneWormholeMetric` & `AlcubierreWarpMetric`.
- Conformal 3+1 Numerical Relativity: `BssnGrid` manages 3D spatial field representations ($\phi, K, \tilde{\gamma}_{ij}, \tilde{A}_{ij}, \tilde{\Gamma}^i, \alpha, \beta^i$), `BssnEvolution` implements 4th-order spatial finite differencing with RK4 time stepping, `BssnConstraints` evaluates Hamiltonian constraint residuals, and `TricubicInterpolator` / `QuinticHermiteTimeInterpolator` provide spatial and temporal metric field evaluations.

### 2.3. Differential Solvers & Geodesic Integrators

- Embedded Adaptive Solvers: `RK45AdaptiveIntegrator` (Dormand-Prince 5(4)) & `CashKarpIntegrator` provide step-size regulation with constraint projection ensuring $u_\mu u^\mu = \text{const}$.
- High-Order Extended Solvers: `Vernier9Integrator` implements a 16-stage 9(8) embedded Runge-Kutta scheme for high-precision orbit tracking.
- Symplectic Solvers: `GaussLegendreIntegrator` provides implicit Runge-Kutta schemes (orders 4 & 6) guaranteeing preservation of phase-space symplectic 2-forms & Killing invariants.
- Predictor-Corrector Solvers: `Hermite4AarsethIntegrator` evaluates analytical jerk terms $\dot{\mathbf{a}}$ & higher derivatives for gravitational multi-body interactions.
- Horizon Boundary Handling: `HorizonDetector` monitors trajectory progression & executes absorption or interior continuation based on configured boundary modes.

### 2.4. Post-Newtonian Dynamics & Gravimetry

- Multi-Body Formulations: `PostNewtonianSolver` & `PostNewtonianSystem` evaluate multi-body equations of motion from Newtonian up to 3.5PN order, including 2.5PN radiation damping, spin-orbit, spin-spin, & self-spin interactions.
- Gravitational Radiation: `GravitationalWaveCalculator` extracts trace-free quadrupole moments & evaluates radiation reaction power $P_{\text{GW}}$ & waveform strain polarizations $(h_+, h_\times)$.
- Planetary Gravimetry: `SphericalHarmonicsGravityModel` & `AssociatedLegendreTable` execute fully normalized spherical harmonic potential & acceleration evaluations up to degree & order 32, coupled with `TidalPerturbationModel` Love number modifications.
- Analytical Precession: `OrbitalPrecessionAnalytic` computes secular nodal & apsidal drift rates ($J_2, J_4$).

### 2.5. Dark Matter & Modified Gravity

- Density Profiles: `NFWProfile`, `EinastoProfile`, `BurkertProfile`, & `HernquistProfile` compute enclosed mass, potential, & rotation curves.
- N-Body Collisionless Dynamics: `BarnesHutOctree` executes hierarchical octree spatial partitioning with quadrupole multipole expansions & symplectic leapfrog advancement.
- Modified Gravity Solvers: `MondFramework` implements non-linear MOND interpolation functions ($\mu, \nu$), `TeVeSSpacetimeMetric` solves the covariant Tensor-Vector-Scalar metric, & `FRChameleonModel` computes scalar-tensor field screening.

### 2.6. Relativistic Hydrodynamics (GRHD/GRMHD)

- State & Flux Containers: `PrimitiveVariables`, `ConservedVariables`, & `FluxVariables` encapsulate fluid states & magnetic vectors.
- Reconstruction & Riemann Solvers: `WENO5Reconstructor` (JS & Z variants), `MP5Reconstructor`, & `TVDReconstructor` compute cell interface states; `HLLRiemannSolver`, `HLLCRiemannSolver`, & `HLLDRiemannSolver` resolve interface fluxes.
- Inversion & Divergence Control: `Con2PrimSolver` executes 1D/2D root-finding inversions from conserved to primitive variables, & `ConstrainedTransport2D` enforces the solenoidal magnetic constraint $\nabla \cdot \mathbf{B} = 0$.
- Equations of State & Solvers: `IdealGasEOS`, `SyngeEOS`, `MathewsEOS`, `RelativisticFermiGasEOS`, `PolytropicEOS`, `PiecewisePolytropicEOS`, and `TabulatedNuclearEOS` model fluid thermodynamics; `RelativisticHydroSolver1D` integrates 1D relativistic fluids using SSP-RK3 time stepping; `TOVSolver` integrates the Tolman-Oppenheimer-Volkoff equations; `NovikovThorneDisk` and `FishboneMoncriefTorus` model thin and thick accretion systems.

### 2.7. Polarized Radiative Transfer & Optics

- Polarimetric Representation: `StokesVector`, `StokesEmissivity`, & `StokesTransferMatrix` represent full-Stokes polarized transport.
- Radiative Processes: `RadiativeProcessEngine` computes non-thermal synchrotron emission/absorption, thermal synchrotron with Faraday rotation ($\rho_V$) & conversion ($\rho_Q$), & relativistic Bremsstrahlung.
- Polarized Solver: `PolarizedRadiativeTransfer` executes exact analytical matrix exponential integration along ray segments via Delano's method.
- Kinetic Models: `MaxwellJuttnerDistribution` samples thermal relativistic electron distributions; `InverseComptonEngine` performs Monte Carlo photon packet scatterings via the Klein-Nishina cross section.
- Spectral Integration & Colorimetry: `ContinuousSpectrum` manages multi-wavelength discretized radiances; `CIE1931Observer` convolves radiances to XYZ & linear sRGB; `Tonemapper` applies ACES & logarithmic HDR tonemapping curves.

### 2.8. Uncertainty Quantification & Metrology

- Interval Arithmetic: `Interval<Scalar>` implements IEEE 1788 interval arithmetic operations, transcendental functions, & inclusion checks.
- Zonotopes: `Zonotope<Scalar, Dim>` manages multidimensional generator sets with Girard order reduction to eliminate wrapping effects.
- Continuous Covariance: `CovarianceMatrix<Scalar, Dim>` implements continuous Jacobi-Lyapunov differential propagation, eigensystem decompositions, & confidence hypervolume calculations.
- Variational Integration: `VariationalGeodesicIntegrator` propagates 8D phase states $(\mathbf{x}, \mathbf{p})$ along with variational transition matrices & covariance envelopes.
- Polynomial Chaos: `PolynomialChaosExpansion` executes spectral stochastic projections on orthogonal Hermite & Legendre bases via Gauss-Hermite & Gauss-Legendre quadratures; `PceGeodesicPropagator` integrates stochastic geodesic ensembles.
- Metrology: `MetrologyVisualizer` constructs 3D covariance ellipsoid meshes, generates 2D probability heatmaps, & extracts quantile confidence envelopes.

---

## 3. Concurrency, Execution Pipeline & Scheduling

The system employs a multi-threaded execution model designed to eliminate synchronization stalls & lock contention:

### 3.1. Deterministic Simulation Scheduler

The execution flow is governed by `Scheduler`, maintaining a fixed logical clock frequency (10 to 1000 Hz) decoupled from visual presentation refresh rates:
- Nanosecond Accumulator: Real-time increments are buffered into a high-precision accumulator. Logical ticks advance when the accumulator exceeds the fixed step interval ($\Delta t = 1 / f_{\text{tick}}$).
- Step & Warp Control: The scheduler supports pause states, single-tick stepping (`request_steps`), & continuous time dilation scaling (`warp_factor`).
- Sub-Tick Interpolation: For visual smoothing, the scheduler exposes the fractional alpha factor $\alpha = t_{\text{accum}} / \Delta t_{\text{tick}} \in [0, 1]$.

### 3.2. Asynchronous Lock-Free Command Queue

Communication between the master terminal/UI & the simulation core is mediated by `SpscQueue` (Single-Producer Single-Consumer lock-free ring buffer):
- Memory Ordering: Atomic operations utilize explicit acquire-release semantics (`std::memory_order_acquire`, `std::memory_order_release`) with cache-line-isolated pointers (64-byte padding) to eliminate false sharing.
- Command Dispatch: Commands are processed synchronously at the boundary of each logical simulation cycle. Command execution results are pushed back to a dedicated lock-free result queue.

### 3.3. Thread Pool & Raytracing Work Distribution

- Persistent Thread Pool: `ThreadPool` manages a static pool of `std::jthread` workers synchronized via condition variables, executing chunked parallel loops (`parallel_for`).
- Spatial Tiling & SIMD Bundling: Raytracing passes support both horizontal scanline slicing & 2D spatial block tiling ($32 \times 32$ pixels). Ray bundles are evaluated using 4-wide or 8-wide SIMD registers.

---

## 4. Precision Architecture & Numerical Stability

The engine provides multi-tiered precision configurations to balance numerical accuracy & throughput:

### 4.1. Hardware-Native 64-Bit Precision (FP64)

- Standard execution path using native IEEE 754 double precision (`double`) across all tensor algebra, Christoffel derivations, variational mechanics, & geodesic integration loops.
- Recommended for research trajectories, high-spin Kerr horizons, & multi-century post-Newtonian orbital baselines.

### 4.2. Compensated Double-Single Arithmetic (DS / fp32-fp32)

- `DoubleSingle` emulates quadrupled precision (approx. 48 bits of mantissa, matching $\approx 14$ decimal digits) using pairs of IEEE 754 single-precision floats (`hi`, `lo`).
- Exact error-free transformations via Knuth's `two_sum`, Dekker's `two_diff`, & Veltkamp-Dekker `two_prod` (using hardware `fma`).
- Deployed on GPU compute pipelines lacking native FP64 hardware execution units to prevent coordinate quantization artifacts near event horizons.

### 4.3. SIMD Register Vectorization

- `SimdVec<T, Width>` & `SimdMask<T, Width>` map to native SIMD registers across AVX2, AVX-512, & ARM Neon.
- `GeodesicBundle` formats ray coordinates & four-momenta in Structure-of-Arrays (SoA) layout, executing vectorized RK4 updates across concurrent ray lanes.

---

## 5. Control, Interface & Multi-Window Architecture

The platform provides decoupled interfaces for interactive exploration & batch execution:

### 5.1. Master Terminal Loop (REPL)

- `MasterTerminalRepl` provides an asynchronous, non-blocking command-line interface.
- Direct command parsing (`CommandParser`) translates text commands into strongly-typed `Command` structures dispatched to the simulation orchestrator without graphical dependencies.

### 5.2. Graphical Multi-Window Workspace (UI Architecture)

When executing in interactive mode, `UiManager` coordinates GLFW windowing, OpenGL 3.3+ rendering contexts, & ImGui/ImPlot multi-viewport docking:
- Primary Viewport (`ViewportPrimaryWindow`): High-resolution display of the raytraced image stream with interactive HUD telemetry overlays & camera transport toolbars.
- Control Panel (`ControlPanelWindow`): Tabbed configuration interfaces for spacetime parameters, camera optics, skybox environments, integrators, & 6-DOF relativistic rocket propulsion.
- Scenario Selector (`ScenarioSelectorWindow`): Scenario browser supporting preset loading & YAML file serialization.
- Curvature Diagnostics & Invariants (`TelemetryWindow`, `VisualDiagnosticsWindow`): Real-time numerical display & temporal history graphs of Ricci scalar curvature $R$, Kretschmann invariant $K_1$, metric tensor components $g_{\mu\nu}$, & horizon radii.
- Spectrograph Monitor (`SpectrographWindow`): Real-time plotting of spectral radiance curves $I(\lambda)$ with $1\sigma$ confidence bands & perceived CIE sRGB color swatches.
- Performance Settings (`PerformanceSettingsWindow`): Profiles, internal render scale adjustment, ray budget limits, & arithmetic precision toggling.
- Interactive Camera Controller (`InteractiveCameraController`): Manages navigation modes (Free-Fly 6-DOF, Orbit Center, Spherical Boyer-Lindquist, and Cockpit Flight) with mouse-look, hotkey shortcuts, and 8 projection modes (Pinhole, AutoZoom, FisheyeStereographic, Equirectangular360, FisheyeEquidistant, FisheyeOrthographic, PaniniCylindrical, HammerAitoff).

---

## 6. Scientific I/O & External Interoperability Pipeline

The engine interfaces with standard astronomical data formats & ephemeris services:

### 6.1. Ephemeris & Orbital Mechanics Ingestion

- `HorizonsInterface`: Formulates REST queries & parses NASA JPL Horizons CSV vector tables, converting barycentric state vectors to SI units.
- `SpkKernel` & `SpkChebyshevSegment`: Evaluates binary SPK ephemeris files (Type 2 position-only & Type 3 position/velocity) via Chebyshev polynomial recurrence relations.
- `FrameTransformer`: Converts state vectors & four-vectors between ICRF J2000, Heliocentric Ecliptic, Geocentric Equatorial, & comobile boosted reference frames.

### 6.2. Scientific Data Serialization

- `ScenarioSerializer`: Reads & writes declarative YAML scenario files (`.yaml`).
- `FitsExporter`: Generates 2D radiance images & 3D spectral data cubes $(X, Y, \lambda)$ adhering to the FITS Standard 4.0 with WCS astrometric headers & big-endian IEEE 754 formatting.
- `Hdf5Container`: Serializes hierarchical binary datasets including worldlines, metric series, covariance matrices, & tabulated nuclear equations of state.
- `VtkExporter`: Generates VTK XML PolyData files (`.vtp`) for polyline trajectories & event horizon surface meshes.

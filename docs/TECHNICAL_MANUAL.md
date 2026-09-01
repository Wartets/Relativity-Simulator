# Relativistic Engine - Technical Architecture Manual

## 1. Core Engineering & System Design

The Relativistic Engine is written in ISO C++23. It is structured around data-oriented memory layouts, static compile-time polymorphism, deterministic execution guarantees, and parallel computation.

### 1.1. Design Constraints

- Zero Dynamic Allocations in Execution Loops: All computational modules operate within pre-allocated linear memory arenas (`LinearMemoryArena`) aligned to 64-byte and 128-byte hardware cache boundaries. Heap allocations during differential integration and raytracing iterations are prohibited.
- Concept-Driven Polymorphism: Dynamic virtual method tables are eliminated using C++23 concepts (`SpacetimeMetric`). Metric evaluations, Christoffel symbol computations, and equations of state are inlined by the compiler.
- Strict Numerical Determinism: Trajectory calculations, N-body evolutions, and stochastic sampling use explicit-state PCG64 generators (`PCG64Engine`). Floating-point arithmetic rounding behavior is preserved to maintain identical results across runs.
- Continuous Multi-Paradigm Uncertainty: Variables support uncertainty tracking via interval bounds (IEEE 1788), order-reduced zonotopes, Jacobi-Lyapunov covariance matrices, and generalized polynomial chaos expansions (gPCE).

---

## 2. Core Subsystems & Components

### 2.1. Static Tensor & SIMD Algebra Layer

- `Tensor<T, Rank, Dim>`: Cache-aligned multidimensional tensor container supporting compile-time rank and dimension validation, outer products, Einstein summation contractions, metric inversions, and determinants.
- `SimdVec<T, Width>` and `SimdMask<T, Width>`: Register-width SIMD abstraction wrappers providing vector arithmetic, conditional blending (`select`), fused multiply-add (`fma`), and trigonometric evaluations.
- `GeodesicBundle<T, Width>`: Structure-of-Arrays (SoA) layout executing parallel Runge-Kutta integrations for bundles of rays (e.g. 4-wide or 8-wide double-precision channels).

### 2.2. Spacetime Metric Implementations

All metric types satisfy the `SpacetimeMetric` concept:
- `FlatMinkowskiMetric`: Pseudo-Euclidean baseline with signature $(-c^2, 1, 1, 1)$.
- `SchwarzschildMetric`: Static spherically symmetric vacuum metric with exact analytical Christoffel symbols.
- `SchwarzschildIsotropicMetric`, `PainleveGullstrandMetric`, `EddingtonFinkelsteinMetric`: Coordinate regularized forms of the Schwarzschild geometry.
- `KerrMetric`: Stationary axisymmetrical geometry in Boyer-Lindquist coordinates, providing conserved Killing quantities and Carter constant invariants.
- `KerrSchildMetric`: Horizon-regular Cartesian formulation $g_{\mu\nu} = \eta_{\mu\nu} + 2H k_\mu k_\nu$.
- `ReissnerNordstromMetric` and `KerrNewmanMetric`: Charged static and rotating electrovacuum spacetimes.
- `SchwarzschildDeSitterMetric`: Cosmological constant $\Lambda$ coupling.
- `FLRWMetric`: Expanding cosmological background with dynamic scale factor $a(t)$ and Hubble parameter $H(t)$.
- `MorrisThorneWormholeMetric`: Spherically symmetric traversable wormhole with throat radius $b_0$.
- `AlcubierreWarpMetric`: Spacetime bubble metric with hyperbolic tangent shaping functions.
- `BssnGrid` & `BssnEvolution`: Numerical relativity engine implementing 3+1 conformal BSSN evolution with 8th-order spatial finite differencing, iterative RK4 time advancement, tricubic spatial interpolation, and quintic Hermite temporal interpolation.

### 2.3. Differential Solvers & Integrators

- `RK45AdaptiveIntegrator`: Dormand-Prince 5(4) adaptive Runge-Kutta integrator with algebraic invariant constraint projection ($g_{\mu\nu}u^\mu u^\nu = \text{const}$).
- `CashKarpIntegrator`: Embedded 5(4) Runge-Kutta scheme for high-stability trajectory integration.
- `Vernier9Integrator`: 16-stage 9(8) high-order adaptive integrator for long orbital baselines.
- `GaussLegendreIntegrator`: Implicit symplectic Runge-Kutta integrator (orders 4 and 6) guaranteeing preservation of Hamiltonian phase-space invariants.
- `Hermite4AarsethIntegrator`: Predictor-corrector variable-step scheme evaluating jerk derivatives for gravitational multi-body interactions.

### 2.4. Post-Newtonian Multi-Body Subsystem

- `PostNewtonianBody`: State structure containing mass, radius, position, velocity, acceleration, spin vector, and multipole coefficients ($J_2, J_3, J_4$).
- `PostNewtonianSolver`: Evaluates gravitational accelerations from Newtonian up to 3.5PN order, including 2.5PN radiation damping, spin-orbit, spin-spin, and spherical harmonic potentials.
- `PostNewtonianSystem`: Container managing multi-body systems, energy conservation tracking, angular momentum bookkeeping, and quadrupole gravitational wave emission ($h_+, h_\times, P_{\text{GW}}$).
- `SphericalHarmonicsGravityModel`: Fully normalized associated Legendre polynomial potential solver up to degree and order 32 (EGM96, LP165, MRO110, Jupiter, Sun models).

### 2.5. Relativistic Hydrodynamics (GRHD/GRMHD) & Stellar Physics

- `PrimitiveVariables` & `ConservedVariables`: State representations with conversions (`Con2PrimSolver`) for hydrodynamical and magnetized plasma regimes.
- `WENO5Reconstructor` & `MP5Reconstructor`: High-order spatial polynomial reconstruction at cell interfaces.
- `HLLCRiemannSolver` & `HLLDRiemannSolver`: Approximate Riemann solvers with contact wave resolution and magnetic wave structures.
- `ConstrainedTransport2D`: Face-centered magnetic field update enforcing solenoidal constraint $\nabla \cdot \mathbf{B} = 0$.
- `IdealGasEOS`, `SyngeEOS`, `MathewsEOS`, `RelativisticFermiGasEOS`, `PiecewisePolytropicEOS`, `TabulatedNuclearEOS`: Equation of state models for relativistic fluids and degenerate nuclear matter.
- `TOVSolver`: Solves the Tolman-Oppenheimer-Volkoff equations to determine relativistic stellar mass-radius profiles and stability limits.
- `NovikovThorneDisk`: Stationary thin relativistic accretion disk model with Page-Thorne analytical flux evaluation and relativistic beaming.
- `FishboneMoncriefTorus`: Stationary magnetized thick accretion torus model in Kerr spacetime.

### 2.6. Polarized Radiative Transfer & Optics

- `StokesVector`: Four-component polarization representation $\mathbf{S} = (I, Q, U, V)^T$.
- `RadiativeProcessEngine`: Calculates thermal and non-thermal synchrotron emission/absorption, relativistic Bremsstrahlung with Gaunt factor corrections, and Faraday rotation/conversion.
- `PolarizedRadiativeTransfer`: Integrates polarized radiative transport along null geodesics via Delano's analytical matrix exponential method.
- `MaxwellJuttnerDistribution`: Relativistic thermal electron velocity distribution sampling and line-broadening kernels.
- `InverseComptonEngine`: Monte-Carlo photon packet scattering across relativistic electron distributions using the Klein-Nishina cross section.
- `CIE1931Observer` & `Tonemapper`: Continuous spectral radiance convolution ($X, Y, Z$) to linear sRGB, ACES filmic curve mapping, and extended logarithmic HDR tonemapping.

### 2.7. Uncertainty Quantification Subsystem

- `Interval<Scalar>`: IEEE 1788 compliant interval arithmetic arithmetic container.
- `Zonotope<Scalar, Dim>`: Affine vector generator sets with Girard order reduction for linear transformations and Minkowski sums.
- `CovarianceMatrix<Scalar, Dim>`: Covariance matrix operations, Jacobi-Lyapunov continuous ODE propagation ($\dot{\mathbf{\Sigma}} = \mathbf{J}\mathbf{\Sigma} + \mathbf{\Sigma}\mathbf{J}^T + \mathbf{Q}$), eigensystem decompositions, and multivariate Gaussian sampling.
- `PolynomialChaosExpansion<NumDims, MaxDegree>`: Spectral stochastic projections using orthogonal Hermite and Legendre polynomials with multi-dimensional Gauss quadratures.
- `VariationalGeodesicIntegrator`: Simultaneous integration of 8D phase states $(\mathbf{x}, \mathbf{p})$, variational transition matrices, and covariance bounds.

### 2.8. Orchestration, Scheduling & User Interface

- `Scheduler`: Deterministic fixed-step clock engine supporting dynamic time-warp factors, pause/step functionality, and real-time accumulator regulation.
- `SimulationOrchestrator`: Coordinates physics processing, parameter updates, camera state management, and thread communication over lock-free single-producer single-consumer queues (`SpscQueue`).
- `InteractiveCameraController`: Manages 6-DOF Free-Fly, Orbit Center, Spherical Boyer-Lindquist, and Cockpit Flight navigation modes.
- `GeodesicComputePipeline` & `SoftwareComputeEngine`: Multithreaded software compute engine executing native double-precision (FP64) and Double-Single (`fp32/fp32`) emulation for camera ray integration.
- `UiManager`: Coordinates GLFW windowing, ImGui docking workspaces, multi-viewport separation, and secondary camera views.
- Telemetry Windows:
  - `ViewportPrimaryWindow`: Displays the raytraced image buffer, interactive HUD overlay, and transport controls.
  - `ScenarioSelectorWindow`: Scenario catalog loader and YAML parser.
  - `ControlPanelWindow`: Parameters for metrics, camera properties, integrators, and rocket thrust.
  - `TelemetryWindow`: Curvature scalars (Ricci $R$, Kretschmann $K_1$), Hamiltonian residuals, and coordinate telemetry.
  - `SpectrographWindow`: Spectral radiance plots $I(\lambda)$ with 1-sigma uncertainty shading and CIE color swatches.
  - `PerformanceSettingsWindow`: Presets, render resolution scaling, and execution statistics.
  - `VisualDiagnosticsWindow`: Metric tensor components $g_{\mu\nu}$, horizon boundaries, and curvature history graphs.

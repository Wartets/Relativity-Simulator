# Relativistic Engine - Technical Architecture Manual

## 1. Core Engineering & System Design

The Relativistic Engine is written in ISO C++23. It is structured around data-oriented memory layouts, static compile-time polymorphism, deterministic execution guarantees, & parallel computation.

### 1.1. Design Constraints

- Zero Dynamic Allocations in Execution Loops: Computational loops operate within pre-allocated linear memory arenas (`LinearMemoryArena`) aligned to 64-byte and 128-byte cache boundaries. The per-ray integration loops of `SoftwareComputeEngine` do not allocate. Frame-level data of the render and capture pipelines (framebuffers, body lists, tile-culling tables, texture upload buffers) are `std::vector` objects resized on demand (see [ARCHITECTURE.md](ARCHITECTURE.md#29-render-pipeline)).
- Concept-Driven Polymorphism: Virtual method dispatch is replaced by C++23 concepts (`SpacetimeMetric`) and templates to enable compile-time inlining of metric and equation of state routines.
- Strict Numerical Determinism: Trajectory calculations, N-body evolutions, & stochastic sampling use explicit-state PCG64 generators (`PCG64Engine`). Floating-point arithmetic rounding behavior is preserved to maintain identical results across runs. Procedural sky and body surface patterns are integer hash functions of the ray direction or surface coordinates and a user-controlled seed. Randomized body creation in `BodyManagerWindow` uses `std::mt19937_64` seeded from `std::random_device` and is not reproducible.
- Continuous Multi-Paradigm Uncertainty: Variables support uncertainty tracking via interval bounds (IEEE 1788), order-reduced zonotopes, Jacobi-Lyapunov covariance matrices, & generalized polynomial chaos expansions (gPCE).

---

## 2. Core Subsystems & Components

### 2.1. Static Tensor & SIMD Algebra Layer

- `Tensor<T, Rank, Dim>`: Cache-aligned multidimensional tensor container supporting compile-time rank & dimension validation, outer products, Einstein summation contractions, metric inversions, & determinants.
- `SimdVec<T, Width>` & `SimdMask<T, Width>`: Register-width SIMD abstraction wrappers providing vector arithmetic, conditional blending (`select`), fused multiply-add (`fma`), & trigonometric evaluations.
- `GeodesicBundle<T, Width>`: Structure-of-Arrays (SoA) layout executing parallel Runge-Kutta integrations for bundles of rays (e.g. 4-wide or 8-wide double-precision channels).

### 2.2. Spacetime Metric Implementations

All metric types satisfy the `SpacetimeMetric` concept:
- `FlatMinkowskiMetric`: Pseudo-Euclidean baseline with signature $(-c^2, 1, 1, 1)$.
- `SchwarzschildMetric`: Static spherically symmetric vacuum metric with exact analytical Christoffel symbols.
- `SchwarzschildIsotropicMetric`, `PainleveGullstrandMetric`, `EddingtonFinkelsteinMetric`: Coordinate regularized forms of the Schwarzschild geometry.
- `KerrMetric`: Stationary axisymmetrical geometry in Boyer-Lindquist coordinates, providing conserved Killing quantities & Carter constant invariants.
- `KerrSchildMetric`: Horizon-regular Cartesian formulation $g_{\mu\nu} = \eta_{\mu\nu} + 2H k_\mu k_\nu$.
- `ReissnerNordstromMetric` & `KerrNewmanMetric`: Charged static & rotating electrovacuum spacetimes.
- `SchwarzschildDeSitterMetric`: Cosmological constant $\Lambda$ coupling.
- `FLRWMetric`: Expanding cosmological background with dynamic scale factor $a(t)$ & Hubble parameter $H(t)$.
- `MorrisThorneWormholeMetric`: Spherically symmetric traversable wormhole with throat radius $b_0$.
- `AlcubierreWarpMetric`: Spacetime bubble metric with hyperbolic tangent shaping functions.
- `BssnGrid` & `BssnEvolution`: Numerical relativity module implementing 3+1 conformal BSSN evolution with 4th-order spatial finite differencing, iterative RK4 time advancement, tricubic spatial interpolation, and quintic Hermite temporal interpolation.
- `include/relativistic/metrics/kerr_de_sitter.hpp`: Kerr-de Sitter geometry.
- `compute_kerr_invariants_bl` & `compute_zamo_angular_velocity` (`include/relativistic/metrics/kerr_invariants.hpp`): Conserved energy, axial angular momentum and Carter constant of a Boyer-Lindquist state, and the angular velocity of zero-angular-momentum observers.
- `BardeenKerrShadow` (`include/relativistic/metrics/bardeen_shadow.hpp`): Photon orbit radii and shadow boundary of the Kerr metric for a given observer polar angle.
- `SubsidiarySourceField` & `SubsidiarySourceParams` (`include/relativistic/metrics/subsidiary_source_field.hpp`): Weak-field deflection, horizon crossing, accretion-disk crossing and rim emission of N-body bodies flagged as spacetime sources, used by the scalar ray tracer.

#### Metric Usage in `SoftwareComputeEngine`

The metric identifiers are those of `Render::MetricId` (`include/relativistic/render/gpu_types.hpp`). The mass used by the tracers is `max(metric_mass, 1e-4)` and the spin is clamped to $\pm 0.999 M$. Thresholds of $10^{-9} M$ select the exact tracer.

| Id | `MetricId` | Tracer | Horizon test | Accretion disk |
| :--- | :--- | :--- | :--- | :--- |
| 0 | `FlatMinkowski` | Schwarzschild null tracer with $M = 0$ | No | No |
| 1 | `Schwarzschild` | Schwarzschild null tracer (`step_schwarzschild_null_rk4`) | Yes | Yes |
| 2 | `Kerr` | Schwarzschild null tracer when $\lvert a \rvert \le 10^{-9} M$, otherwise `trace_exact_photon` with `KerrMetric` | Yes | Yes |
| 3 | `KerrSchild` | Same selection as Kerr; the exact tracer instantiates `KerrMetric` | Yes | Yes |
| 4 | `ReissnerNordstrom` | Schwarzschild null tracer when $\lvert Q \rvert \le 10^{-9} M$, otherwise `trace_exact_photon` with `ReissnerNordstromMetric` | Yes | Yes |
| 5 | `KerrNewman` | Schwarzschild null tracer when both $a$ and $Q$ are below the threshold, otherwise `trace_exact_photon` with `KerrNewmanMetric` | Yes | Yes |
| 6 | `SchwarzschildDeSitter` | Schwarzschild null tracer | Yes | No |
| 7 | `FLRW` | Schwarzschild null tracer | Yes | No |
| 8 | `MorrisThorne` | Schwarzschild null tracer | No | No |
| 9 | `Alcubierre` | Schwarzschild null tracer | No | No |

`cosmological_lambda`, `wormhole_throat` and `warp_velocity` are copied into `GpuCameraPushConstants` but are not evaluated by `SoftwareComputeEngine`. `BssnGrid` is not referenced by the render path: the BSSN selection resolves to Schwarzschild (see [CLI_REFERENCE.md](CLI_REFERENCE.md#23-spacetime-metric--integrator-selection)). The Vulkan shader is not covered by this table. Ray-tracing equations are given in [MATHEMATICAL_FORMULATION.md](MATHEMATICAL_FORMULATION.md#8-image-formation-in-the-renderer).

### 2.3. Differential Solvers & Integrators

- `RK45AdaptiveIntegrator`: Dormand-Prince 5(4) adaptive Runge-Kutta integrator with algebraic invariant constraint projection ($g_{\mu\nu}u^\mu u^\nu = \text{const}$).
- `CashKarpIntegrator`: Embedded 5(4) Runge-Kutta scheme for high-stability trajectory integration.
- `Vernier9Integrator`: 16-stage 9(8) high-order adaptive integrator for long orbital baselines.
- `GaussLegendreIntegrator`: Implicit symplectic Runge-Kutta integrator (orders 4 & 6) guaranteeing preservation of Hamiltonian phase-space invariants.
- `Hermite4AarsethIntegrator`: Predictor-corrector variable-step scheme evaluating jerk derivatives for gravitational multi-body interactions.

#### Runtime Use of the Integrator Selection

- The integrator name stored by `SimulationOrchestrator` is consumed by `SimulationOrchestrator::step_nbody_dynamics`, which selects `SymplecticForestRuthPNIntegrator` when the name contains `Symplectic` or `Gauss` and `RungeKutta4PNIntegrator` otherwise (`include/relativistic/dynamics/pn_integrator.hpp`, [ARCHITECTURE.md](ARCHITECTURE.md#34-orchestrator-state-advance)).
- `SoftwareComputeEngine` does not call the integrators of this section. It uses `step_schwarzschild_null_rk4` (`include/relativistic/core/schwarzschild_null_integrator.hpp`) and, for the exact tracer, a fixed-step RK4 scheme on the Christoffel symbols of the selected metric (`Core::compute_christoffel` with 8th-order differences).
- The ray step is derived from `step_size_factor`, `min_step_size`, `max_step_size`, `far_field_step_scale` and `pole_guard_precision_scale`. `integration_rtol` and `integration_atol` are stored, saved in scenarios and benchmark runs, and displayed, but are not read by the renderer. The step formulas are given in [MATHEMATICAL_FORMULATION.md](MATHEMATICAL_FORMULATION.md#82-step-control-termination-and-space-skipping).
- The compatibility warnings of `include/relativistic/ui/compatibility_notes.hpp` for BSSN with symplectic or Hermite schemes are display-only.

### 2.4. Post-Newtonian Multi-Body Subsystem

- `PostNewtonianBody`: State structure containing mass, radius, position, velocity, acceleration, spin vector, & multipole coefficients ($J_2, J_3, J_4$).
- `PostNewtonianSolver`: Evaluates gravitational accelerations from Newtonian up to 3.5PN order, including 2.5PN radiation damping, spin-orbit, spin-spin, & spherical harmonic potentials.
- `PostNewtonianSystem`: Container managing multi-body systems, energy conservation tracking, angular momentum bookkeeping, & quadrupole gravitational wave emission ($h_+, h_\times, P_{\text{GW}}$).
- `SphericalHarmonicsGravityModel`: Fully normalized associated Legendre polynomial potential solver up to degree & order 32 (EGM96, LP165, MRO110, Jupiter, Sun models).
- `RungeKutta4PNIntegrator` & `SymplecticForestRuthPNIntegrator` (`include/relativistic/dynamics/pn_integrator.hpp`): Fixed-step integrators of `PostNewtonianSystem`. The orchestrator subdivides each tick into at most 500 sub-steps ([ARCHITECTURE.md](ARCHITECTURE.md#34-orchestrator-state-advance)).
- Spacetime source bodies: A `PostNewtonianBody` with `is_spacetime_source` set carries a Kerr outer horizon derived from its mass and spin (`kerr_outer_horizon_radius`, `kerr_spin_parameter`, `set_spin_state`, `enforce_spacetime_source_invariants`). Absorption and merging are handled by `SimulationOrchestrator::handle_horizon_absorption`; the primary source stays fixed at the origin.
- `InteractionConfig` (`include/relativistic/dynamics/interaction_config.hpp`) & the interaction solver (`interaction_solver.hpp`): Electromagnetic forces (Coulomb and dipole-dipole, with configurable permittivity and permeability), collisions (elastic or inelastic response, rotation, friction, restitution multiplier, contact stiffness scale, position correction), thermodynamics (radiative exchange with an ambient temperature and a coupling scale), fragmentation (integrity loss from collisions and tidal stress, minimum fragment mass, maximum fragments per event) and annihilation (contact scale, opposite-charge requirement). `interaction_compatibility.hpp` produces the warnings displayed in the Interactions tab.
- `BodySurfaceLayerRegistry` (`include/relativistic/dynamics/body_surface_layers.hpp`): Per-body procedural texture layers, at most `Render::kMaxSurfaceLayers` (4) per body. Each layer selects one of 10 patterns, 5 blend modes and 7 region masks (`SurfaceLayerPattern`, `SurfaceLayerBlend`, `SurfaceLayerMask`).
- `BulkBodyActions` (`include/relativistic/dynamics/bulk_body_actions.hpp`): System-wide operations on enabled bodies (velocity and spin assignment or inversion, grid snapping, scattering, mass equalization and averaging, generalized parameter assignment and equalization over 16 scalar parameters, culling by radius or camera frustum).
- Thread safety: `PostNewtonianSystem::bodies_mutex()` is a recursive mutex locked by the orchestrator, the UI windows, the path preview and the capture recorder when they read or modify the body list.

### 2.5. Relativistic Hydrodynamics (GRHD/GRMHD) & Stellar Physics

- `PrimitiveVariables` & `ConservedVariables`: State representations with conversions (`Con2PrimSolver`) for hydrodynamical & magnetized plasma regimes.
- `WENO5Reconstructor` & `MP5Reconstructor`: High-order spatial polynomial reconstruction at cell interfaces.
- `HLLCRiemannSolver` & `HLLDRiemannSolver`: Approximate Riemann solvers with contact wave resolution & magnetic wave structures.
- `ConstrainedTransport2D`: Face-centered magnetic field update enforcing solenoidal constraint $\nabla \cdot \mathbf{B} = 0$.
- `IdealGasEOS`, `SyngeEOS`, `MathewsEOS`, `RelativisticFermiGasEOS`, `PolytropicEOS`, `PiecewisePolytropicEOS`, `TabulatedNuclearEOS`: Equation of state models for relativistic ideal fluids, degenerate Fermi gases, polytropes, and tabulated nuclear matter.
- `TOVSolver`: Solves the Tolman-Oppenheimer-Volkoff equations to determine relativistic stellar mass-radius profiles & stability limits.
- `NovikovThorneDisk`: Stationary thin relativistic accretion disk model with Page-Thorne analytical flux evaluation & relativistic beaming.
- `FishboneMoncriefTorus`: Stationary magnetized thick accretion torus model in Kerr spacetime.

### 2.6. Polarized Radiative Transfer & Optics

- `StokesVector`: Four-component polarization representation $\mathbf{S} = (I, Q, U, V)^T$.
- `RadiativeProcessEngine`: Calculates thermal & non-thermal synchrotron emission/absorption, relativistic Bremsstrahlung with Gaunt factor corrections, & Faraday rotation/conversion.
- `PolarizedRadiativeTransfer`: Integrates polarized radiative transport along null geodesics via Delano's analytical matrix exponential method.
- `MaxwellJuttnerDistribution`: Relativistic thermal electron velocity distribution sampling & line-broadening kernels.
- `InverseComptonEngine`: Monte-Carlo photon packet scattering across relativistic electron distributions using the Klein-Nishina cross section.
- `CIE1931Observer` & `Tonemapper`: Continuous spectral radiance convolution ($X, Y, Z$) to linear sRGB, ACES filmic curve mapping, & extended logarithmic HDR tonemapping.

### 2.7. Uncertainty Quantification Subsystem

- `Interval<Scalar>`: IEEE 1788 compliant interval arithmetic arithmetic container.
- `Zonotope<Scalar, Dim>`: Affine vector generator sets with Girard order reduction for linear transformations & Minkowski sums.
- `CovarianceMatrix<Scalar, Dim>`: Covariance matrix operations, Jacobi-Lyapunov continuous ODE propagation ($\dot{\mathbf{\Sigma}} = \mathbf{J}\mathbf{\Sigma} + \mathbf{\Sigma}\mathbf{J}^T + \mathbf{Q}$), eigensystem decompositions, & multivariate Gaussian sampling.
- `PolynomialChaosExpansion<NumDims, MaxDegree>`: Spectral stochastic projections using orthogonal Hermite & Legendre polynomials with multi-dimensional Gauss quadratures.
- `VariationalGeodesicIntegrator`: Simultaneous integration of 8D phase states $(\mathbf{x}, \mathbf{p})$, variational transition matrices, & covariance bounds.

### 2.8. Orchestration, Scheduling & User Interface

- `Scheduler`: Deterministic fixed-step clock engine supporting dynamic time-warp factors, pause/step functionality, & real-time accumulator regulation. It starts paused at 60 Hz, accepts rates from 10 to 1000 Hz, and clamps the accumulator to 0.25 s ([ARCHITECTURE.md](ARCHITECTURE.md#31-deterministic-simulation-scheduler)).
- `SimulationOrchestrator`: Coordinates physics processing, parameter updates, camera state management, & thread communication over lock-free single-producer single-consumer queues (`SpscQueue`, capacity 1024 for commands and results). It owns `PhysicalParameters`, `CameraState` and a home camera, the N-body system, the surface layer registry, `PerformanceProfiler`, `ConstantsEngine`, `InteractionConfig`, `UnitDisplayPreferences`, the active metric, integrator and scenario, and 32 custom parameter slots (names up to 63 characters). It applies 25 `CommandType` values and 123 `ParameterType` identifiers. `state_version()` is incremented after every applied command and simulation advance, and the windows compare it to resynchronize their widgets. Command semantics are listed in [CLI_REFERENCE.md](CLI_REFERENCE.md#25-command-processing-and-performance-presets) and the state advance in [ARCHITECTURE.md](ARCHITECTURE.md#34-orchestrator-state-advance).
- `InteractiveCameraController`: Manages the Free-Fly 6-DOF, Orbit Center, Spherical Boyer-Lindquist and Rocket Thrust navigation modes ([CLI_REFERENCE.md](CLI_REFERENCE.md#32-navigation-modes)), the quick snap positions (equatorial front and side, poles, ISCO, photon sphere), look-at-target, and mouse look with a 4 pixel drag threshold. Speeds, sensitivities, inversions and zoom limits are stored in `CameraControlConfig` (`include/relativistic/ui/camera_control_config.hpp`). The projection is a render parameter selected among 8 models.
- `GeodesicComputePipeline`: Owns the framebuffers, the render worker thread, the `VulkanContext` and `VulkanComputeExecutor`, and the per-frame telemetry. It selects between the Vulkan path and `SoftwareComputeEngine` for every frame, and also serves capture rendering in bands ([ARCHITECTURE.md](ARCHITECTURE.md#29-render-pipeline)).
- `SoftwareComputeEngine`: Multithreaded CPU ray tracer with a 4-lane double-precision SIMD path, a 8-lane single-precision SIMD path, and scalar paths in both precisions. Details are given in [Section 2.9](#29-rendering-back-ends).
- `UiManager`: Coordinates GLFW windowing, ImGui docking workspaces, multi-viewport separation, & secondary camera views. It restores window visibility, units, constants, interaction settings, camera controls and secondary views from `UserSettings`, queues the startup scenario, processes the global hotkeys through `ActionEdgeTracker` (67 rebindable `InputAction` values in 8 categories, `include/relativistic/ui/input_actions.hpp`), and draws the main menu bar. The layout preset `DeepAnalysis` has no dedicated branch and is applied as the docked layout.
- Telemetry Windows:
  - `ViewportPrimaryWindow`: Owns the `GeodesicComputePipeline` and the `CaptureCoordinator`. It builds the camera constants and the culled body list for every frame, applies the CPU color grading, uploads the `RGBA32F` texture, draws the toolbar (24 optional controls selected in `ToolbarButtonVisibility`), the 25 HUD elements, the loading indicator, the schematic projection and overlay, and the capture progress bar, and records one `FrameSample` per frame in the profiler. The hold-to-zoom magnification acts on the displayed texture only.
  - `ScenarioSelectorWindow`: Two tabs. Browse Catalog scans the scenario directory, lists compatible and incompatible files with filter and sort modes, shows the metadata and parameters of the selected file, loads it, assigns the startup scenario (with the startup behavior options), and deletes files after confirmation; the built-in startup file cannot be deleted. Save & Export edits the scenario metadata (author, version tag with major, minor and patch increment, name, description, output block), shows the live content summary with duplicate and delete actions for bodies, saves a new preset without overwriting any file, or saves to a given path with an overwrite confirmation. File formats are in [FILE_FORMATS.md](FILE_FORMATS.md#1-declarative-scenario-definition-yaml).
  - `ControlPanelWindow` (Master Simulation Controls): 11 tabs. Spacetime & Metrics, Optics & Camera (projection, tonemapper, disk color model, color grading, manual camera placement), Camera Controls, Skybox & Environment (procedural or panorama sky), Solvers & Integrators, Relativistic Rocket (6-DOF), Time & Execution, HUD & Overlay, Schematic View, 3D Body Render and Units & Scales. In the Relativistic Rocket tab only the clock flow selection is sent to the orchestrator; the throttle and thrust sliders are local widgets, and rocket thrust is produced by the movement keys using `RocketControlProfile`.
  - `TelemetryWindow`: Observer position in user units, Ricci scalar $R$, Kretschmann invariant $K_1$, the residual $\lvert R \rvert$, static lapse, time dilation, ZAMO angular velocity, Keplerian period and Newtonian escape fraction at the observer radius, inside or outside status relative to the horizon, ISCO and outer photon orbit, and the scheduler state. Values are recomputed every 0.15 s or when mass, spin or the observer radius (by more than 0.1) change. $K_1$ is $48M^2/r^6$ for $\lvert a \rvert < 10^{-12}$ and is otherwise evaluated by `RiemannComputer` with fourth-order differences on the Kerr metric.
  - `SpectrographWindow`: Spectral radiance $I(\lambda)$ sampled at 400 wavelengths from 380 to 779 nm for a blackbody, a synchrotron power law (`make_synchrotron(-0.7, 1e-12)`) or a 550 nm line, shifted by a Doppler factor $g$ that can be linked to the observer radius, with the perceived CIE sRGB color. The temperature can be linked to the disk profile, which is also plotted (60 samples from the ISCO to the disk outer radius). The shaded band is a fixed 8 % of the radiance and is not derived from the uncertainty subsystem. For the synchrotron process the window lists the electron cyclotron frequency, the emissivity and absorptivity at 550 nm and an approximate cooling time. The displayed bolometric radiance is the sum of the sampled values.
  - `PerformanceSettingsWindow`: Preset selection (preset contents in [CLI_REFERENCE.md](CLI_REFERENCE.md#25-command-processing-and-performance-presets)), internal render scale, ray step budget, arithmetic precision, SIMD, thread pool and tiling options, render distance, level of detail, adaptive space-skipping, polar step damping, far-field step multiplier, texture reallocation, dynamic resolution, adaptive tile sky prepass, interlacing, GPU compute with device status and the active path of the last frame, body tile coverage, motion-adaptive quality, step controller mode, and orchestrator counters.
  - `VisualDiagnosticsWindow`: Local curvature invariants and metric tensor $g_{\mu\nu}$ at the observer, horizon structure (outer and inner horizon, outer ergosphere at the observer polar angle, extremality status), horizon thermodynamics (area, surface gravity, Hawking temperature, entropy, angular velocity, irreducible mass), the photon orbit radii range for $\lvert a_* \rvert < 0.999999$, and a radial profile of $\log_{10} K_1$ and $\log_{10} \lvert R \rvert$ on the equatorial plane. The profile uses 48 samples between $\max(1.05\,r_+, 0.05M)$ and $\max(2 r_{\min}, 60M)$ and is recomputed only when mass or spin change; it is a function of the parameters, not of simulation time. Formulas: [MATHEMATICAL_FORMULATION.md](MATHEMATICAL_FORMULATION.md#24-kerr-characteristic-radii-and-horizon-quantities).
  - `BodyManagerWindow`: Celestial body & N-body catalog browser, per-body creation wizard with archetype-based randomized defaults, per-body physical/material/thermal/electromagnetic property editing, system-wide dynamics summary (total mass, center of mass, linear & angular momentum, mechanical energy, gravitational-wave luminosity), & electromagnetic/collision/thermodynamics/fragmentation/annihilation interaction configuration.
  - `HudManagerWindow`: Master HUD switch and, for each of the 25 elements, enabled state, anchor corner, offsets, text scale, text color and background panel. The display format, decimal precision, label, horizontal layout, draw priority, refresh interval, warning and critical color rules, automatic gap-free stacking, toolbar button selection and keybind summary content are edited in the HUD & Overlay tab of `ControlPanelWindow`.
  - `ConstantsWindow`: Editable base physical constants ($c$, $G$, $h$, $k_B$, $N_A$, $K_e$, $K_{cd}$) under SI, Planck, or Custom presets, alongside derived constants & dimensional scaling factors between simulation & SI units.
  - `KeybindSettingsWindow`: Full keybinding rebinding interface with conflict detection, QWERTY/AZERTY layout presets, & hold/toggle activation modes for modifier-style actions.
  - `LogConsoleWindow`: Scrollback viewer for the engine log with severity filtering (info, warning, error).
  - `PerformanceAnalysisWindow`: Live frame-time & stage-breakdown monitoring, statistical summaries over configurable sample windows, bottleneck analysis with a weighted render-dispatch cost breakdown, & persisted benchmark run capture & comparison.
  - `SecondaryViewportManager` / `SecondaryViewWindow`: Independent auxiliary observer viewports rendered synchronously on the calling thread through `SoftwareComputeEngine::dispatch_fp64` (CPU, native precision, no thread pool). Each view has its own position (free or following the primary camera with an angular offset), optics, exposure, tonemapper, projection, step budget and resolution scale, up to 1920 x 1080 pixels. With throttling enabled, every open view receives the scale $\max(s^{n-1}, 0.15)$, where $n$ is the number of open views and $s$ the per-view falloff (0.4 to 1.0, default 0.85). A view without auto-refresh renders again only when its constants change or when requested. At most 8 views are persisted.

### 2.9. Rendering Back Ends

- Entry points: `SoftwareComputeEngine::dispatch_fp64`, `dispatch_fp32` and `dispatch_double_single`. The scalar tracer is used when the metric requires the exact path (`requires_exact_metric_path`) or when `USE_SCALAR_PIPELINE` is set. Otherwise the SIMD tracer is used; when bodies are present, `compute_body_screen_tiles` marks the 32 x 32 pixel tiles covered by body bounding volumes, the SIMD tracer skips the marked tiles and the scalar tracer renders them with a per-tile list of candidate bodies.
- SIMD tracers: `GeodesicBundle4d` (4 double-precision lanes) and `GeodesicBundle8f` (8 single-precision lanes). They trace the Schwarzschild null equations only, without 3D bodies, exact metrics or subsidiary sources. Only the double-precision SIMD tracer applies the Carter pre-classification and the far-field step factor.
- Precision: `dispatch_double_single` forwards to `dispatch_fp32`; the render mode named double-single therefore integrates rays in `float`, and exact-metric rays are still traced in `double`. `DoubleSingle` (`include/relativistic/render/double_single.hpp`) provides error-free sum and product transformations, arithmetic operators, `ds_sqrt`, `ds_sin`, `ds_cos` and `ds_atan2`, but is not referenced by `SoftwareComputeEngine`. The shader `shaders/geodesic_tracer_ds.comp` is part of the repository, whereas `VulkanComputeExecutor` loads only `geodesic_tracer_fp64.comp.spv`.
- Disk parameters: the scalar double-precision tracer and both single-precision tracers use the constants 18000 K and $g^4$ in the disk model, and the SIMD and exact double-precision tracers use `disk_temperature_scale_k`, `disk_temperature_floor_k` and `disk_doppler_beaming_exponent` (see [MATHEMATICAL_FORMULATION.md](MATHEMATICAL_FORMULATION.md#82-step-control-termination-and-space-skipping)).
- Spacetime source bodies (`preset_3d == 8`): `collect_subsidiary_sources` extracts them from the body list. The scalar tracer applies their deflection (`apply_subsidiary_deflection_step`), horizon absorption, disk crossing and rim emission, reduces the step budget by $\min(n+1, 8)$ (minimum 256 steps) and disables analytic space-skipping.
- Per-pixel output: `GpuPixelOutput` stores color, alpha, redshift, affine parameter, `PixelFlags` (horizon absorbed, celestial hit, accretion disk hit, photon sphere proximity, body surface hit) and the iteration count. `RenderFlags` assigns bits 0 to 3 to the skybox mode and bits 4 to 18 to the options (grid skybox, scalar pipeline, per-frame threads, tiled distribution, texture reallocation, level of detail, space-skipping, adaptive tile prepass, 3D bodies, body Doppler beaming, body gravitational redshift, atmosphere scattering, body shadows, bodies-only mode, body disk occlusion).
- Camera constants: `GpuCameraPushConstants` defines `operator==`; the viewport compares the new constants with the previous ones to decide whether a new dispatch is required. `GpuBodyData` is converted to the 704-byte `GpuBodyGpuLayout` for the Vulkan storage buffer.
- Sky: the procedural sky combines a starfield with a galactic band, an optional coordinate grid, deep-field galaxies, dust clouds and star clusters, followed by hue, saturation and background offset; `sky_background_source` selects an equirectangular panorama (`SkyPanoramaLoader`) instead. Panoramas are described by `sky_panorama_catalog.hpp`.
- Body rendering: `evaluate_3d_bodies` intersects rays with oblate, prolate or triaxial ellipsoids, shades the base texture mode, `BodySurfaceShading` layers, lighting from the central source, shadows, relativistic effects and atmospheric rim (formulas in [MATHEMATICAL_FORMULATION.md](MATHEMATICAL_FORMULATION.md#84-celestial-body-shading)).

### 2.10. Capture Modules

- `CaptureCoordinator` (`include/relativistic/capture/capture_coordinator.hpp`) and `CaptureProgress`: screenshot and sequence sessions, deterministic and real-time modes, banded rendering, supersampling and temporal averaging, event execution, ffmpeg assembly. Behavior is specified in [ARCHITECTURE.md](ARCHITECTURE.md#210-capture-subsystem) and the output files in [FILE_FORMATS.md](FILE_FORMATS.md#7-capture-session-directory).
- `MotionScript`, `ScriptSegment`, `ShapeSpec`, `ScalarChannel`, `DriverSpec`, `ShakeSpec`, `SegmentTransition`, `ScriptEvent` (`include/relativistic/capture/motion_script.hpp`) with `EasingSpec` (`easing.hpp`), `Expression` (`expression.hpp`), `ScriptEventDispatcher` (`script_events.hpp`) and the text serialization of `motion_script_file.hpp`. `CameraPath` (`camera_path.hpp`) is the legacy keyframe and parametric path format; `MotionScript::from_legacy_path` converts it.
- `build_path_preview` and `make_body_position_lookup` (`path_preview_builder.hpp`) with the data structures of `path_preview.hpp`: trajectory, reference trajectory without shake and transitions, markers and camera frustum drawn by `SchematicViewRenderer`.
- `PhysicsRecorder` (`physics_recorder.hpp`) with `TelemetryTable` and `RecordingSettings`: derived kinematic and relativistic channels ([FILE_FORMATS.md](FILE_FORMATS.md#8-telemetry-recording-files)).
- `CaptureStudioWindow` (`include/relativistic/ui/capture_studio_window.hpp`): tabs Screenshot, Sequence, Motion Script, Data Recording and Video Encoding, a summary bar, a status footer with a resizable splitter, and the viewport path preview controls. `MotionScriptEditor` (`motion_script_editor.hpp`) provides the timeline and the panes Path Editor, Events, Curves and Script; the Curves pane plots 513 samples of each evaluated quantity.

### 2.11. Constants, Units and Expressions

- `ConstantsEngine` (`include/relativistic/core/physical_constants_engine.hpp`): base constants, presets, derived constants and simulation-to-SI scale factors ([ARCHITECTURE.md](ARCHITECTURE.md#213-physical-constants-engine)). The post-Newtonian configuration follows the values of $c$ and $G$ (`sync_nbody_constants_with_engine`).
- `Units` (`include/relativistic/units/unit_system.hpp`): `UnitDisplayPreferences` holds one unit selection for each of 19 quantities (distance, mass, velocity, energy, angle, temperature, charge, current, frame rate, time, acceleration, angular velocity, density, pressure, power, frequency, force, magnetic field, voltage) and the `format_*` functions used by the HUD and windows.
- `Units::ExpressionEvaluator` (`include/relativistic/units/expression_engine.hpp`): evaluates unit expressions with a `DimensionVector` and rejects results whose dimension differs from the expected one (`evaluate_expect_dimension`).
- `unit_aware_slider_double`, `unit_aware_input_double3` and `unit_aware_input_float3` (`unit_aware_widgets.hpp`) and `smart_quantity_input` (`numeric_slider_utils.hpp`): widgets that display and edit values in the selected units.

### 2.12. Interface Support Modules

- `numeric_slider_utils.hpp`: `slider_float_with_input`, `slider_double_with_input` and `slider_int_with_input` combine a slider, a numeric field and an optional logarithmic mode with explicit bounds.
- `tooltip_utils.hpp`: delayed tooltips with an optional warning line, and wrapped colored text.
- `compatibility_notes.hpp`: five checks (BSSN with symplectic or Hermite schemes, GPU compute with double-single precision, near-extremal Kerr spin, wormhole with disk overlay, level of detail beyond the render distance). The first three are displayed by `ControlPanelWindow` and `PerformanceSettingsWindow`; the wormhole and level-of-detail checks are defined but not called by the windows reviewed.
- `hud_layout_config.hpp`: `HudLayoutConfig`, `HudElementStyle`, `HudColorRule`, `HudAutoArranger` and `hud_anchor_resolve` (anchor corners and clamping at zero).
- `schematic_view_config.hpp`: `SchematicViewConfig` with object display styles (point, fixed-radius sphere, parameter-driven sphere; opaque, translucent, wireframe, shaded and gradient styles; 10 color coding modes), per-body style overrides, vector styles for velocity, total force, spin and rotation axis, trails, orbit predictions and off-screen indicators.
- `capture_widgets.hpp`: form scope, flow layout, property grids, statistic tables, splitters and `edit_easing`.
- `BodyManagerWindow`: 9 body templates (`kBodyTemplateSpecs`), 6 creation archetypes (`kCreationArchetypes`), and the tabs Body Catalog, Create Body, Spacetime Sources, System Dynamics and Interactions. Body editor sections are Identity, Physical State, Multipoles, Material, and Surface (base surface, atmosphere and lighting, texture layers).

# Technical Architecture & System Design

## 1. Architectural Principles & System Paradigms

The Relativistic Engine is an ISO C++23 simulation and visualization framework for relativistic mechanics and raytracing. The system is structured into four functional layers:
- Core Analytical & Numerical Physics Engine: Stateless, thread-parallel libraries executing [tensor algebra](TECHNICAL_MANUAL.md#21-static-tensor--simd-algebra-layer), [geodesic integration](TECHNICAL_MANUAL.md#23-differential-solvers--integrators), [post-Newtonian multi-body dynamics](MATHEMATICAL_FORMULATION.md#3-post-newtonian-pn-n-body-dynamics), [relativistic hydrodynamics](MATHEMATICAL_FORMULATION.md#4-relativistic-hydrodynamics-grhdgrmhd), and [uncertainty quantification](MATHEMATICAL_FORMULATION.md#7-uncertainty-quantification-formulation).
- Simulation Runtime & State Orchestrator: A synchronous deterministic state machine driven by a [fixed logical time-step scheduler](#31-deterministic-simulation-scheduler), communicating with external control layers via [lock-free SPSC queues](#32-asynchronous-lock-free-command-queue).
- Render & Compute Pipeline: Hardware-accelerated GPU compute pipelines ([Vulkan compute](#29-render-pipeline)) and multithreaded SIMD software compute engines ([SoftwareComputeEngine](TECHNICAL_MANUAL.md#29-rendering-back-ends)) performing [backward null geodesic raytracing](MATHEMATICAL_FORMULATION.md#8-image-formation-in-the-renderer), [polarized radiative transfer](MATHEMATICAL_FORMULATION.md#6-polarized-radiative-transfer), and [spectral reduction](DESCRIPTION.md#93-continuous-spectral-pipeline--cie-1931-integration).
- Presentation, Instrumentation & Workspace Layer: An interactive multi-window graphical user interface with docking workspaces ([UI Architecture](#52-graphical-multi-window-workspace-ui-architecture)), real-time telemetry dashboards ([Telemetry & Diagnostics](TECHNICAL_MANUAL.md#28-orchestration-scheduling--user-interface)), spectrographs, and a non-blocking [master command-line interpreter (REPL)](#51-master-terminal-loop-repl).

### Fundamental Design Paradigms

- Zero-Allocation Hot Path: Differential equations, tensor contractions, raytracing passes, and uncertainty loops execute without dynamic heap allocations. Core structures utilize pre-allocated memory arenas (`LinearMemoryArena`, `include/relativistic/core/memory_arena.hpp`) aligned to 64-byte and 128-byte cache boundaries ([Technical Manual](TECHNICAL_MANUAL.md#11-design-constraints)).
- Static Compile-Time Polymorphism: Elimination of virtual table dispatch across inner loops through C++23 concepts (`SpacetimeMetric`, `include/relativistic/metrics/spacetime_concept.hpp`) and template specializations.
- Explicit SIMD Register Vectorization: Use of explicit vectorization abstractions (`SimdVec<T, Width>`, `SimdMask<T, Width>`, `include/relativistic/core/simd.hpp`, and `GeodesicBundle<T, Width>`, `include/relativistic/core/geodesic_bundle.hpp`) processing multiple geodesic rays and phase states concurrently across AVX2, AVX-512, and ARM Neon architectures ([Section 4.3](#43-simd-register-vectorization)).
- Strict Bit-Level Determinism: Guaranteed reproducibility across identical hardware targets via explicit-state pseudo-random number generators (`PCG64Engine`, `include/relativistic/core/pcg64.hpp`) and fixed-step temporal scheduling ([Section 3.1](#31-deterministic-simulation-scheduler)).

---

## 2. Core Subsystems & Computational Hierarchy

The system architecture is organized into modular subsystems operating across decoupled boundaries:

### 2.1. Tensor Algebra & Curvature Evaluation Layer

- Static Tensor Primitives: `Tensor<T, Rank, Dim>` (`include/relativistic/core/tensor.hpp`) defines cache-aligned multidimensional tensor storage with compile-time rank, dimension, and flat index resolution ([Technical Manual](TECHNICAL_MANUAL.md#21-static-tensor--simd-algebra-layer)).
- Metric Inversion & Contraction: `inverse_metric_4x4` and `determinant_4x4` perform analytical cofactor inversions for 4D spacetime metrics; `contract` and `contract_tensors` (`include/relativistic/core/tensor_ops.hpp`) execute general Einstein summation contractions.
- Christoffel Evaluation: `compute_christoffel` (`include/relativistic/core/christoffel.hpp`) dynamically dispatches to exact analytical formulations when provided by the metric or evaluates numerical derivatives via 8th-order centered finite difference stencils (`compute_christoffel_numerical`; see [Mathematical Formulation](MATHEMATICAL_FORMULATION.md#21-christoffel-symbols-of-the-second-kind)).
- Curvature Invariants: `RiemannComputer` (`include/relativistic/core/riemann.hpp`) computes the Riemann tensor $R^\rho_{\phantom{\rho}\sigma\mu\nu}$, Ricci tensor $R_{\mu\nu}$, Ricci scalar $R$, and Kretschmann invariant $K_1 = R^{\alpha\beta\gamma\delta} R_{\alpha\beta\gamma\delta}$ ([Mathematical Formulation](MATHEMATICAL_FORMULATION.md#23-riemann-tensor-ricci-tensor--kretschmann-scalar)).

### 2.2. Spacetime Metric Modules

Every spacetime implementation satisfies the `SpacetimeMetric` concept (`include/relativistic/metrics/spacetime_concept.hpp`), requiring `metric_tensor`, `inverse_metric`, `christoffel_symbols`, and `speed_of_light`:
- Analytical Vacuum Spacetimes: `FlatMinkowskiMetric` ([MATHEMATICAL_FORMULATION.md Section 1.1](MATHEMATICAL_FORMULATION.md#11-minkowski-metric-flat-spacetime)), `SchwarzschildMetric` ([Section 1.2](MATHEMATICAL_FORMULATION.md#12-schwarzschild-metric)), and `KerrMetric` ([Section 1.3](MATHEMATICAL_FORMULATION.md#13-kerr-metric-rotating-black-hole)).
- Regularized Coordinate Gauges: `SchwarzschildIsotropicMetric`, `PainleveGullstrandMetric`, `EddingtonFinkelsteinMetric`, and Cartesian `KerrSchildMetric` ([Section 1.3](MATHEMATICAL_FORMULATION.md#13-kerr-metric-rotating-black-hole)).
- Electrovacuum & Cosmological Spacetimes: `ReissnerNordstromMetric` ([Section 1.4](MATHEMATICAL_FORMULATION.md#14-reissner-nordström-metric-charged-black-hole)), `KerrNewmanMetric` ([Section 1.5](MATHEMATICAL_FORMULATION.md#15-kerr-newman-metric-charged-rotating-black-hole)), `SchwarzschildDeSitterMetric` ([Section 1.6](MATHEMATICAL_FORMULATION.md#16-schwarzschild-de-sitter--kottler-metric)), and `FLRWMetric` ([Section 1.7](MATHEMATICAL_FORMULATION.md#17-flrw-metric-cosmological-spacetime)).
- Exotic Spacetimes: `MorrisThorneWormholeMetric` ([Section 1.8](MATHEMATICAL_FORMULATION.md#18-morris-thorne-traversable-wormhole)) and `AlcubierreWarpMetric` ([Section 1.9](MATHEMATICAL_FORMULATION.md#19-alcubierre-warp-drive-metric)).
- Conformal 3+1 Numerical Relativity: `BssnGrid` (`include/relativistic/metrics/bssn_grid.hpp`) manages 3D spatial field representations ($\phi, K, \tilde{\gamma}_{ij}, \tilde{A}_{ij}, \tilde{\Gamma}^i, \alpha, \beta^i$), `BssnEvolution` (`include/relativistic/metrics/bssn_evolution.hpp`) implements 4th-order spatial finite differencing with RK4 time stepping, `BssnConstraints` (`include/relativistic/metrics/bssn_constraints.hpp`) evaluates Hamiltonian constraint residuals, and `TricubicInterpolator` / `QuinticHermiteTimeInterpolator` (`include/relativistic/metrics/bssn_interpolation.hpp`) provide spatial and temporal metric field evaluations ([Description](DESCRIPTION.md#44-31-numerical-relativity-grids-bssn)).
- Runtime tracer dispatch per metric is tabulated in [TECHNICAL_MANUAL.md](TECHNICAL_MANUAL.md#metric-usage-in-softwarecomputeengine).

### 2.3. Differential Solvers & Geodesic Integrators

- Embedded Adaptive Solvers: `RK45AdaptiveIntegrator` (Dormand-Prince 5(4), `include/relativistic/integrators/rk45_adaptive.hpp`) and `CashKarpIntegrator` (`include/relativistic/integrators/cash_karp.hpp`) provide step-size regulation with constraint projection ensuring $u_\mu u^\mu = \text{const}$.
- High-Order Extended Solvers: `Vernier9Integrator` (`include/relativistic/integrators/vernier9.hpp`) implements a 16-stage 9(8) embedded Runge-Kutta scheme for high-precision orbit tracking.
- Symplectic Solvers: `GaussLegendreIntegrator` (`include/relativistic/integrators/symplectic_gauss_legendre.hpp`) provides implicit Runge-Kutta schemes (orders 4 and 6) guaranteeing preservation of phase-space symplectic 2-forms and Killing invariants.
- Predictor-Corrector Solvers: `Hermite4AarsethIntegrator` (`include/relativistic/integrators/hermite4_aarseth.hpp`) evaluates analytical jerk terms $\dot{\mathbf{a}}$ and higher derivatives for gravitational multi-body interactions.
- Horizon Boundary Handling: `HorizonDetector` (`include/relativistic/integrators/horizon_manager.hpp`) monitors trajectory progression and executes absorption or interior continuation based on configured boundary modes ([Description](DESCRIPTION.md#32-strong-gravity--curved-spacetime-phenomena)).
- Runtime integration selection rules for N-body dynamics versus image ray tracing are detailed in [TECHNICAL_MANUAL.md](TECHNICAL_MANUAL.md#runtime-use-of-the-integrator-selection).

### 2.4. Post-Newtonian Dynamics & Gravimetry

- Multi-Body Formulations: `PostNewtonianSolver` (`include/relativistic/dynamics/pn_acceleration.hpp`) and `PostNewtonianSystem` (`include/relativistic/dynamics/pn_nbody_system.hpp`) evaluate multi-body equations of motion from Newtonian up to 3.5PN order, including 2.5PN radiation damping, spin-orbit, spin-spin, and self-spin interactions ([Mathematical Formulation](MATHEMATICAL_FORMULATION.md#3-post-newtonian-pn-n-body-dynamics)).
- Gravitational Radiation: `GravitationalWaveCalculator` (`include/relativistic/dynamics/pn_gravitational_waves.hpp`) extracts trace-free quadrupole moments and evaluates radiation reaction power $P_{\text{GW}}$ and waveform strain polarizations $(h_+, h_\times)$ ([MATHEMATICAL_FORMULATION.md Section 3.4](MATHEMATICAL_FORMULATION.md#34-gravitational-wave-quadrupole-emission)).
- Planetary Gravimetry: `SphericalHarmonicsGravityModel` (`include/relativistic/gravimetry/spherical_harmonics.hpp`) and `AssociatedLegendreTable` (`include/relativistic/gravimetry/legendre_table.hpp`) execute fully normalized spherical harmonic potential and acceleration evaluations up to degree and order 32, coupled with `TidalPerturbationModel` (`include/relativistic/gravimetry/tidal_perturbations.hpp`) Love number modifications ([Description](DESCRIPTION.md#52-spherical-harmonics--high-degree-geodesy)).
- Analytical Precession: `OrbitalPrecessionAnalytic` (`include/relativistic/gravimetry/orbital_precession.hpp`) computes secular nodal and apsidal drift rates ($J_2, J_4$).

### 2.5. Dark Matter & Modified Gravity

- Density Profiles: `NFWProfile`, `EinastoProfile`, `BurkertProfile`, and `HernquistProfile` (`include/relativistic/dark_matter/dark_matter_profiles.hpp`) compute enclosed mass, potential, and rotation curves ([Description](DESCRIPTION.md#53-dark-matter-halos--alternative-gravitational-theories)).
- N-Body Collisionless Dynamics: `BarnesHutOctree` (`include/relativistic/dark_matter/barnes_hut.hpp`) executes hierarchical octree spatial partitioning with quadrupole multipole expansions and symplectic leapfrog advancement.
- Modified Gravity Solvers: `MondFramework` (`include/relativistic/modified_gravity/mond.hpp`) implements non-linear MOND interpolation functions ($\mu, \nu$), `TeVeSSpacetimeMetric` (`include/relativistic/modified_gravity/teves.hpp`) solves the covariant Tensor-Vector-Scalar metric, and `FRChameleonModel` (`include/relativistic/modified_gravity/f_r_gravity.hpp`) computes scalar-tensor field screening.

### 2.6. Relativistic Hydrodynamics (GRHD/GRMHD)

- State & Flux Containers: `PrimitiveVariables`, `ConservedVariables`, and `FluxVariables` (`include/relativistic/hydro/hydro_types.hpp`) encapsulate fluid states and magnetic vectors ([Mathematical Formulation](MATHEMATICAL_FORMULATION.md#42-conservative-31-form)).
- Reconstruction & Riemann Solvers: `WENO5Reconstructor` (JS and Z variants), `MP5Reconstructor`, and `TVDReconstructor` (`include/relativistic/hydro/reconstruction.hpp`) compute cell interface states; `HLLRiemannSolver`, `HLLCRiemannSolver`, and `HLLDRiemannSolver` (`include/relativistic/hydro/riemann_solvers.hpp`) resolve interface fluxes ([Description](DESCRIPTION.md#61-curved-spacetime-hydrodynamics)).
- Inversion & Divergence Control: `Con2PrimSolver` (`include/relativistic/hydro/con2prim.hpp`) executes 1D/2D root-finding inversions from conserved to primitive variables, and `ConstrainedTransport2D` (`include/relativistic/hydro/constrained_transport.hpp`) enforces the solenoidal magnetic constraint $\nabla \cdot \mathbf{B} = 0$.
- Equations of State & Solvers: `IdealGasEOS`, `SyngeEOS`, `MathewsEOS`, `RelativisticFermiGasEOS`, `PolytropicEOS`, `PiecewisePolytropicEOS`, and `TabulatedNuclearEOS` (`include/relativistic/hydro/eos.hpp`) model fluid thermodynamics ([MATHEMATICAL_FORMULATION.md Section 4.3](MATHEMATICAL_FORMULATION.md#43-equations-of-state-eos)); `RelativisticHydroSolver1D` (`include/relativistic/hydro/grhd_solver.hpp`) integrates 1D relativistic fluids using SSP-RK3 time stepping; `TOVSolver` (`include/relativistic/hydro/tov_solver.hpp`) integrates the Tolman-Oppenheimer-Volkoff equations; `NovikovThorneDisk` (`include/relativistic/hydro/novikov_thorne.hpp`, [MATHEMATICAL_FORMULATION.md Section 5.1](MATHEMATICAL_FORMULATION.md#51-novikov-thorne-thin-disk-profile)) and `FishboneMoncriefTorus` (`include/relativistic/hydro/fishbone_moncrief.hpp`) model thin and thick accretion systems.

### 2.7. Polarized Radiative Transfer & Optics

- Polarimetric Representation: `StokesVector` (`include/relativistic/optics/stokes_vector.hpp`), `StokesEmissivity`, and `StokesTransferMatrix` represent full-Stokes polarized transport ([Mathematical Formulation](MATHEMATICAL_FORMULATION.md#62-full-stokes-polarized-transfer-equations)).
- Radiative Processes: `RadiativeProcessEngine` (`include/relativistic/optics/radiative_processes.hpp`) computes non-thermal synchrotron emission/absorption, thermal synchrotron with Faraday rotation ($\rho_V$) and conversion ($\rho_Q$), and relativistic Bremsstrahlung ([Description](DESCRIPTION.md#64-radiative-processes--local-emission)).
- Polarized Solver: `PolarizedRadiativeTransfer` (`include/relativistic/optics/polarized_radiative_transfer.hpp`) executes exact analytical matrix exponential integration along ray segments via Delano's method.
- Kinetic Models: `MaxwellJuttnerDistribution` (`include/relativistic/optics/maxwell_juttner.hpp`) samples thermal relativistic electron distributions; `InverseComptonEngine` (`include/relativistic/optics/inverse_compton.hpp`) performs Monte Carlo photon packet scatterings via the Klein-Nishina cross section.
- Spectral Integration & Colorimetry: `ContinuousSpectrum` (`include/relativistic/optics/spectrum.hpp`) manages multi-wavelength discretized radiances; `CIE1931Observer` (`include/relativistic/optics/cie_observer.hpp`) convolves radiances to XYZ and linear sRGB ([MATHEMATICAL_FORMULATION.md Section 8.2](MATHEMATICAL_FORMULATION.md#82-step-control-termination-and-space-skipping)); `Tonemapper` (`include/relativistic/optics/tonemapping.hpp`) applies ACES and logarithmic HDR tonemapping curves ([MATHEMATICAL_FORMULATION.md Section 8.3](MATHEMATICAL_FORMULATION.md#83-tone-mapping-and-color-grading)).

### 2.8. Uncertainty Quantification & Metrology

- Interval Arithmetic: `Interval<Scalar>` (`include/relativistic/uncertainty/interval.hpp`) implements IEEE 1788 interval arithmetic operations, transcendental functions, and inclusion checks ([MATHEMATICAL_FORMULATION.md Section 7.1](MATHEMATICAL_FORMULATION.md#71-interval-arithmetic-ieee-1788)).
- Zonotopes: `Zonotope<Scalar, Dim>` (`include/relativistic/uncertainty/zonotope.hpp`) manages multidimensional generator sets with Girard order reduction to eliminate wrapping effects ([Section 7.2](MATHEMATICAL_FORMULATION.md#72-zonotope-enclosure)).
- Continuous Covariance: `CovarianceMatrix<Scalar, Dim>` (`include/relativistic/uncertainty/covariance.hpp`) implements continuous Jacobi-Lyapunov differential propagation, eigensystem decompositions, and confidence hypervolume calculations ([Section 7.3](MATHEMATICAL_FORMULATION.md#73-lyapunov-covariance-matrix-propagation)).
- Variational Integration: `VariationalGeodesicIntegrator` (`include/relativistic/uncertainty/variational_geodesic.hpp`) propagates 8D phase states $(\mathbf{x}, \mathbf{p})$ along with variational transition matrices and covariance envelopes.
- Polynomial Chaos: `PolynomialChaosExpansion` (`include/relativistic/uncertainty/polynomial_chaos.hpp`) executes spectral stochastic projections on orthogonal Hermite and Legendre bases via Gauss-Hermite and Gauss-Legendre quadratures ([Section 7.4](MATHEMATICAL_FORMULATION.md#74-generalized-polynomial-chaos-expansion-gpce)); `PceGeodesicPropagator` (`include/relativistic/uncertainty/pce_geodesic.hpp`) integrates stochastic geodesic ensembles.
- Metrology: `MetrologyVisualizer` (`include/relativistic/uncertainty/metrology.hpp`) constructs 3D covariance ellipsoid meshes, generates 2D probability heatmaps, and extracts quantile confidence envelopes ([Description](DESCRIPTION.md#73-metrology--visual-representation)).

### 2.9. Render Pipeline

**`GeodesicComputePipeline`** (`include/relativistic/render/geodesic_compute_pipeline.hpp`) owns a front and a back framebuffer of `GpuPixelOutput`. In windowed mode a worker thread renders the most recent pending request (`GpuCameraPushConstants` and body list); a request arriving while a frame is rendering raises a cancellation flag, so superseded frames are abandoned. In headless mode `dispatch` renders synchronously. After each frame the pipeline classifies pixels (horizon absorbed, celestial escape, accretion disk hit, step saturated), computes iteration statistics and records tile statistics of the adaptive sky prepass.

**Path selection.** The Vulkan path is attempted when GPU compute is enabled and the executor is ready, and all of the following hold:

- The body count does not exceed 512.
- The precision mode is native FP64.
- The metric identifier is FlatMinkowski, Schwarzschild, Kerr, ReissnerNordstrom, KerrNewman or SchwarzschildDeSitter.
- Interlacing is disabled.
- No body requires the exact metric path (`SoftwareComputeEngine::requires_exact_metric_path`).

Otherwise `SoftwareComputeEngine::dispatch_fp64` or `dispatch_double_single` renders on the CPU thread pool, with the precision taken from the custom parameter `precision_mode`. Capture rendering uses the same selection through `render_capture` and `render_capture_band`.

**Vulkan execution** (`include/relativistic/render/vulkan_context.hpp`, `include/relativistic/render/vulkan_compute_executor.hpp`).

- Device selection requires a compute queue and `scalarBlockLayout`, and `shaderFloat64` when FP64 is requested. The score favors discrete GPUs, vendor identifier 0x10DE and FP64 support. When no device qualifies, the context stays initialized without a compute device and the CPU path is used.
- The shader `shaders/geodesic_tracer_fp64.comp.spv` is searched in `shaders`, `../shaders`, `./shaders` and `../../shaders`. CMake compiles it with `glslangValidator` when available.
- Descriptor bindings: 0 camera uniform buffer, 1 output storage buffer, 2 body storage buffer (`GpuBodyGpuLayout`), 3 panorama combined image sampler, 4 persistent counter storage buffer.
- Frames are dispatched in bands whose row count is a multiple of 16, limited to 64 MiB of output, and adapted toward 120 ms per band; cancellation is checked between bands.
- A fence timeout of 10 s (3 s for panorama uploads) marks the device as lost; GPU compute is then disabled and the CPU renderer is used for subsequent frames.
- When `sky_background_source` is not zero, the selected panorama (`SkyPanoramaLoader`) is uploaded as an `R8G8B8A8_SRGB` image.

**Viewport frame logic** (`ViewportPrimaryWindow`, `include/relativistic/ui/viewport_primary_window.hpp`).

- The internal resolution is the content size multiplied by the active scale, clamped to 64 to 3840 by 64 to 2160 pixels. The active scale is the resolution scale, replaced during navigation by the motion quality setting (disabled, automatic at 0.65 times the base scale, or fixed), and multiplied by the dynamic resolution factor (between 0.25 and 1.0, reduced by 6 % when the last frame exceeds 108 % of the target frame time and increased by 3 % when it is below 82 %).
- A new dispatch occurs only when the camera constants, the orchestrator state version, the precision selection or the advancing simulation time change.
- Bodies are culled by escape radius and, for perspective projections without lensing, by the view cone.
- Post-processing (contrast, saturation, lift, gamma, gain, highlights, shadows, vignette) is applied on the CPU before upload to an `RGBA32F` texture.
- In schematic mode the ray tracing dispatch is skipped ([Section 2.12](#212-schematic-view-and-hud)).

### 2.10. Capture Subsystem

**Coordinator.** `CaptureCoordinator` (`include/relativistic/capture/capture_coordinator.hpp`) manages one capture session at a time. `update(dt)` is called once per UI frame and prepares frames; a worker thread executes the queued tasks (frame rendering, file writing, ffmpeg assembly). The capture constants are derived from the last constants dispatched by the viewport, with the current camera, exposure and mass parameters substituted (`ViewportPrimaryWindow::build_capture_constants`).

**Frame rendering.**

- The output is rendered in horizontal bands of `clamp(2^21 / (width * k^2), 1, height)` rows, where `k` is the supersampling factor. Each band is traced at $k$ times the output resolution through `GeodesicComputePipeline::render_capture_band` and streamed to an `ImageStreamWriter`, so memory use does not depend on the output height.
- Supersampling and temporal samples are averaged in linear light using lookup tables for sRGB decoding and encoding. A fade factor scales the result.
- Capture constants disable interlacing, the level-of-detail flag and the adaptive tile prepass, optionally replace the step budget, and divide `step_size_factor`, `min_step_size` and `max_step_size` by the step refinement factor.
- Limits: 65535 pixels per output axis, 262144 per traced axis, supersampling at most 8.

**Deterministic mode.** For each frame the coordinator processes script events, then for each of the `temporal_samples` sub-frame times: advances the simulation (`ticks_per_frame` multiplied by the script simulation rate and divided by the sample count, accumulated as a fractional tick count), applies the script pose, and stores constants and bodies. The shutter offsets are centered on the frame time and span `shutter_fraction` of the frame interval. At most two frames are queued at once. World advance modes: frozen, fixed ticks per frame, or simulation seconds per video second (ticks per frame = `s / (fps * tick_dt)`). With the trigger `PathDuration`, the frame index is mapped linearly onto the script duration. Pause and resume events freeze and release the world. Manual stepping renders one frame per request (at most 64 pending). The live viewport render is suppressed unless `preview_in_viewport` is set.

**Real-time mode.** The front framebuffer is copied at the frame interval. Pacing either drops late frames or duplicates the last frame to fill gaps; frames waiting for disk writing are limited by the queue depth. The viewport resolution multiplier is locked, and frames whose size changes are resampled by nearest neighbor to the first frame size.

**Session end.** The recorder is finalized, the camera, field of view, exposure, camera mode, warp factor and pause state are restored (the camera only when `restore_camera_after_capture` is set or a script was used), `sequence_info.txt` is written, and ffmpeg is invoked through `std::system` when assembly is enabled and the session completed. Frames are deleted after a successful assembly when requested. Output files are described in [FILE_FORMATS.md](FILE_FORMATS.md#7-capture-session-directory).

**Motion script** (`include/relativistic/capture/motion_script.hpp`). A script is an ordered list of segments with a global easing and an end behavior (clamp, loop, ping-pong).

- Segment: duration, time easing, anchor (world, continue previous, offset from previous, track body), shape layers, orientation, scalar channels (field of view, exposure, roll, simulation warp), shake and a transition blending from the previous segment (orientation, position and lens durations).
- Shape layers: 21 shape kinds (hold, line, quadratic and cubic Bezier, Catmull-Rom spline, polyline, B-spline, arc, helix, spherical orbit, logarithmic and Archimedean spirals, Lissajous curve, torus knot, lemniscate, rose, epitrochoid, wave, and Cartesian, cylindrical and spherical expressions) with an affine transform, combined by weight with the blend modes replace, add and add relative inside a progress window.
- Orientation modes: free, fixed, interpolated, look at target, along travel, expression, target path.
- Scalar channels: start and end values with easing, optional expression, wave modulation (sine, triangle, square, sawtooth, noise) and a driver that maps a measured signal (distance to target or origin, position components, speed, segment or script progress or time, event time offset, expression) onto the value with replace, add or multiply blending.
- Easing: 46 kinds (linear, quadratic to quintic, sinusoidal, exponential, circular, back, elastic and bounce families in in, out and in-out variants, smoothstep variants, power, steps, delay window, cubic Bezier, damped spring, custom expression, custom curve, sigmoid, curvature, smooth steps, wobble) with blend, repeat, ping-pong, reversal, input and output windows, bias, gain and quantization.
- Events: four triggers (script time, segment start, segment end, segment fraction) and 14 actions (marker, capture still, set parameter, set warp, pause, resume, step ticks, set metric, set integrator, load scenario, set tick rate, apply performance preset, set overlay, set resolution scale). Parameter, warp, tick rate and resolution events can ramp over a duration with an easing. The selectable parameters are listed in `kEventParameters` (`include/relativistic/capture/script_events.hpp`).
- Presets: Orbit Reveal, Spiral Infall, Fly-By With Stop, Dolly Zoom, Figure Eight Survey, Helical Approach, Torus Knot Showcase, Multi-Stage Tour, Photon Sphere Skim, Handheld Drift.

**Expression language** (`include/relativistic/capture/expression.hpp`). Expressions compile to a stack program. Variables: `t` (segment time), `u` (progress), `d` (duration), `g` (script time), `a`, `b`, `c`, `k` (parameters), `r` (radius), `x`, `y`, `z` (position), `v` (speed), `m` (driven measurement), `p` (script progress), `s` (segment index). Constants: `pi`, `tau`, `e`. Operators: `+ - * / % ^`, `< > <= >= == !=`. Functions: `sin cos tan asin acos atan sinh cosh tanh exp log ln log2 log10 sqrt cbrt abs floor ceil round fract sign rad deg saturate smooth noise tri saw square atan2 min max hypot step pow mod clamp mix if smoothstep`. Division by zero evaluates to 0 and non-finite results are replaced by 0.

**Recording and preview.** `PhysicsRecorder` derives kinematic and relativistic channels from the live camera pose and the orchestrator state for each frame ([FILE_FORMATS.md](FILE_FORMATS.md#8-telemetry-recording-files)). `build_path_preview` (`include/relativistic/capture/path_preview_builder.hpp`) samples the script into a `PathPreview` (vertices, reference path without shake and transitions, markers, camera frustum) drawn by the schematic renderer.

### 2.11. Profiling, Logging and Persistence

**`PerformanceProfiler`** (`include/relativistic/orchestrator/performance_profiler.hpp`) keeps a ring of `FrameSample` records (default capacity 3600, minimum 16). Stage durations are accumulated through `ScopedStageTimer` and attached to the next recorded frame. The stages are FrameTotal, RenderDispatch, TextureUpload, HudOverlay, CameraUpdate, SchematicOverlay, PostProcessing, FramebufferReadback, GpuDispatchExecution, CpuDispatchExecution, AdaptiveTilePrepassSky, PixelClassification and CameraConstantsBuild.

- `StatisticalSummary` provides mean, median, minimum, maximum, population standard deviation and the 95th and 99th percentiles (linear interpolation).
- `analyze_bottleneck` selects the dominant stage among all stages except FrameTotal and the five render sub-stages, reports ray step saturation when the mean saturated ray fraction exceeds 0.35, and flags a window without any GPU frame.
- Benchmark runs are captured over a duration or a frame count, store the configuration of the last captured frame together with the statistics, and are persisted ([FILE_FORMATS.md](FILE_FORMATS.md#102-configperformance_profilercfg)).
- `EngineSignature::compute` (`include/relativistic/core/engine_signature.hpp`) is the first 8 bytes (16 hexadecimal characters) of the SHA-256 digest of the build tag `relativistic-engine-perf-abi`, the structural revision, and `sizeof(GpuCameraPushConstants)` and `sizeof(GpuPixelOutput)`. Runs with a different signature are marked as not directly comparable.

**Logging.** `EngineLog` (`include/relativistic/core/engine_log.hpp`) keeps the last 4096 entries with a timestamp and severity (info, warning, error) and mirrors them to standard output or standard error.

**Settings.** `UserSettings` and `CaptureStudioSettings` are loaded at startup and saved by `UiManager::export_runtime_settings`. The crash guard and load policy are described in [FILE_FORMATS.md](FILE_FORMATS.md#101-configuser_settingscfg).

### 2.12. Schematic View and HUD

`SchematicViewRenderer` (`include/relativistic/ui/schematic_view_renderer.hpp`) draws bodies, the central object, background grid, field lines, trails, orbit predictions, vectors and the capture path preview as a projected overlay, using the camera tetrad and the selected projection (pinhole when `human_perspective_mode` is set). Objects are depth-sorted and path segments are split at occlusion boundaries so that spheres cover the segments behind them. Orbit predictions integrate a two-body orbit around the central mass with a velocity Verlet scheme. Spacetime source bodies are drawn with a horizon disc, ISCO and disk outer rings and a spin axis. The overlay can be shown on top of the ray-traced image, optionally applying a first-order deflection $4M/b$ (limited to 0.35 rad) to body positions.

The HUD (`HudLayoutConfig`, `include/relativistic/ui/hud_layout_config.hpp`) consists of 25 elements with anchor, offset, scale, color, background, display mode (compact, standard, extended), horizontal layout, draw priority, decimal precision and warning and critical color rules. Automatic arrangement stacks elements per anchor without gaps. Frame time readouts use a rolling average over `rolling_average_frame_count` samples (2 to 120).

### 2.13. Physical Constants Engine

`ConstantsEngine` (`include/relativistic/core/physical_constants_engine.hpp`) stores the simulation values of $c$, $G$, $h$, $k_B$, $N_A$, $K_e$ and $K_{cd}$. It is constructed with the Planck preset ($c = G = k_B = K_e = K_{cd} = 1$, $h = 2\pi$, $N_A$ unchanged); `UiManager` then applies the preset stored in the user settings (SI by default). Setting any constant switches the preset to Custom. Each value is clamped to at least $10^{-300}$. The post-Newtonian solver uses the engine values of $c$ and $G$.

The scale factors between simulation units and SI units are:

$$T_0 = \sqrt{\frac{G_{SI}}{G}\frac{h_{SI}}{h}\left(\frac{c}{c_{SI}}\right)^5}, \quad L_0 = \frac{c_{SI} T_0}{c}, \quad M_0 = \frac{G}{G_{SI}}\frac{L_0^3}{T_0^2}$$

$$Q_0 = \sqrt{\frac{K_e}{K_{e,SI}}\frac{M_0 L_0^3}{T_0^2}}, \quad K_0 = \frac{k_B}{k_{B,SI}}\frac{M_0 L_0^2}{T_0^2}, \quad N_0 = \frac{N_A}{N_{A,SI}}, \quad I_0 = K_{cd}, \quad A_0 = \frac{Q_0}{T_0}$$

Derived simulation quantities include the reduced Planck constant $h / 2\pi$, the Stefan-Boltzmann constant $2\pi^5 k_B^4 / (15 h^3 c^2)$, the vacuum permittivity $1 / (4\pi K_e)$, the vacuum permeability $4\pi K_e / c^2$, the magnetic coupling $K_e / c^2$ and astronomical quantities (solar mass, astronomical unit, Earth and Jupiter masses, parsec, light year, particle masses, solar Schwarzschild radius, Thomson cross section, Wien constant) divided by the corresponding scale. The Constants window accepts unit-aware expressions checked against the expected dimension (`Units::ExpressionEvaluator::evaluate_expect_dimension`).

---

## 3. Concurrency, Execution Pipeline & Scheduling

The system employs a multi-threaded execution model designed to eliminate synchronization stalls & lock contention:

### 3.1. Deterministic Simulation Scheduler

The execution flow is governed by `Scheduler`, maintaining a fixed logical clock frequency (10 to 1000 Hz) decoupled from visual presentation refresh rates:
- Nanosecond Accumulator: Real-time increments are buffered into a high-precision accumulator. Logical ticks advance when the accumulator exceeds the fixed step interval ($\Delta t = 1 / f_{\text{tick}}$).
- Step & Warp Control: The scheduler supports pause states, single-tick stepping (`request_steps`), & continuous time dilation scaling (`warp_factor`).
- Sub-Tick Interpolation: For visual smoothing, the scheduler exposes the fractional alpha factor $\alpha = t_{\text{accum}} / \Delta t_{\text{tick}} \in [0, 1]$.
- Initial State & Clamping: The scheduler starts paused at 60 Hz with a warp factor of 1. The accumulator is clamped to 0.25 s so that a long frame does not produce a burst of ticks. A requested step executes regardless of the accumulator, and the scheduler pauses when the requested step count reaches zero. Each tick advances the logical time by $\Delta t_{\text{tick}} \cdot w$, where $w$ is the warp factor.

### 3.2. Asynchronous Lock-Free Command Queue

Communication between the master terminal/UI & the simulation core is mediated by `SpscQueue` (Single-Producer Single-Consumer lock-free ring buffer):
- Memory Ordering: Atomic operations utilize explicit acquire-release semantics (`std::memory_order_acquire`, `std::memory_order_release`) with cache-line-isolated pointers (64-byte padding) to eliminate false sharing.
- Command Dispatch: Commands are processed synchronously at the boundary of each logical simulation cycle. Command execution results are pushed back to a dedicated lock-free result queue.

### 3.3. Thread Pool & Raytracing Work Distribution

- Persistent Thread Pool: `ThreadPool` manages a static pool of `std::jthread` workers synchronized via condition variables, executing chunked parallel loops (`parallel_for`).
- Spatial Tiling & SIMD Bundling: Raytracing passes support both horizontal scanline slicing & 2D spatial block tiling ($32 \times 32$ pixels). Ray bundles are evaluated using 4-wide or 8-wide SIMD registers.

### 3.4. Orchestrator State Advance

`SimulationOrchestrator::advance_simulation(dt)` (`include/relativistic/orchestrator/simulation_orchestrator.hpp`) performs the following steps:

1. The central body is synchronized: a body named `Central Object` of mass $M$, radius $2M$ and spin $(0, 0, aM)$ is registered as the spacetime source, unless an enabled body of mass above $0.5M$ already sits at the origin.
2. The interaction configuration is passed to the N-body system.
3. The N-body system is advanced. The step is divided into $\min(500, \lceil \Delta t / \Delta t_s \rceil)$ sub-steps with $\Delta t_s = 0.05 / \omega_{\max}$ and $\omega_{\max} = \sqrt{\max(M, 10^{-4}) / \max(r_{\min}^3, 10^{-6})}$, where $r_{\min}$ is the smallest radius of an enabled body. Each sub-step applies the symplectic Forest-Ruth post-Newtonian integrator when the integrator name contains `Symplectic` or `Gauss`, and the fourth-order Runge-Kutta post-Newtonian integrator otherwise, followed by the interaction step and the horizon absorption step. Bodies disabled before the step keep their state.
4. In the Rocket Thrust camera mode, the camera position is integrated with the camera velocity while the scheduler is not paused.
5. The state version counter is incremented. The user interface compares this counter to detect parameter changes and to trigger a re-render.

Horizon absorption (`handle_horizon_absorption`) operates on the central mass with spin clamped to $\pm 0.999M$ and the outer horizon radius $r_h = M + \sqrt{M^2 - a^2}$:

- A regular body with $r \le 1.001 r_h$ is absorbed: its mass is added to the central mass and the body is disabled.
- A spacetime source body within $r_h + r_{h,b}$ of the origin is merged with the central object. The central spin is recomputed from the total angular momentum (central spin, body spin and orbital angular momentum), clamped to $\pm 0.999M$.
- A regular body within $1.001$ times the horizon radius of a source body is merged into it by momentum-weighted velocity, spin and charge accumulation.
- Two source bodies within $1.05$ times the larger horizon radius merge into the more massive one.
- When any merger occurs, the body list is rebuilt without the disabled bodies.

Command application, including the `Reset` semantics, is described in [CLI_REFERENCE.md](CLI_REFERENCE.md#25-command-processing-and-performance-presets). Changes to $c$ and $G$ in the constants engine are copied to the N-body configuration.

### 3.5. Scenario Resolution and Startup

`ScenarioLocator` (`include/relativistic/io/scenario_locator.hpp`) resolves files and the scenario directory relative to the working directory and up to four parent levels (`kMaxAncestorDepth`). A requested path is tried as given, then prefixed by up to four `..` components, then, when its parent directory is named `scenarios`, against the located scenario directory. `portable_path` stores paths inside the scenario directory in the form `scenarios/<file>`.

At startup `UiManager::queue_startup_scenario` loads the configured `default_scenario_path` when `load_scenario_on_startup` is set. If the file cannot be resolved, parsed or validated, the built-in `scenarios/schwarzschild_accretion.yaml` is loaded instead and becomes the stored startup path; when it is also unavailable, the simulation starts empty. The Scenario Manager scans the directory for `.yaml` and `.yml` files, marks files that fail parsing or validation as incompatible, protects the built-in startup file from deletion, and saves new presets under collision-free file and scenario names (`<stem>_<n>.yaml` and `<name> (<n>)`).

---

## 4. Precision Architecture & Numerical Stability

The engine provides multi-tiered precision configurations to balance numerical accuracy & throughput:

### 4.1. Hardware-Native 64-Bit Precision (FP64)

- Standard execution path using native IEEE 754 double precision (`double`) across all tensor algebra, Christoffel derivations, variational mechanics, & geodesic integration loops.
- Recommended for research trajectories, high-spin Kerr horizons, & multi-century post-Newtonian orbital baselines.

### 4.2. Compensated Double-Single Arithmetic (DS / fp32-fp32)

- `DoubleSingle` (`include/relativistic/render/double_single.hpp`) emulates extended precision (approx. 48 bits of mantissa, matching $\approx 14$ decimal digits) using pairs of IEEE 754 single-precision floats (`hi`, `lo`). The functions `ds_sin`, `ds_cos` and `ds_atan2` evaluate in `double` and split the result.
- Exact error-free transformations via Knuth's `two_sum`, Dekker's `two_diff`, and Veltkamp-Dekker `two_prod` (using hardware `fma`).
- Executed by `SoftwareComputeEngine::dispatch_double_single` on the CPU ([Technical Manual](TECHNICAL_MANUAL.md#29-rendering-back-ends)). The Vulkan compute path requires native FP64 and is skipped when double-single precision is selected (see [Section 2.9](#29-render-pipeline)).

### 4.3. SIMD Register Vectorization

- `SimdVec<T, Width>` and `SimdMask<T, Width>` (`include/relativistic/core/simd.hpp`) map to native SIMD registers across AVX2, AVX-512, and ARM Neon ([Technical Manual](TECHNICAL_MANUAL.md#21-static-tensor--simd-algebra-layer)).
- `GeodesicBundle` (`include/relativistic/core/geodesic_bundle.hpp`) formats ray coordinates and four-momenta in Structure-of-Arrays (SoA) layout, executing vectorized RK4 updates across concurrent ray lanes (`GeodesicBundle4d` and `GeodesicBundle8f`; see [TECHNICAL_MANUAL.md Section 2.9](TECHNICAL_MANUAL.md#29-rendering-back-ends)).

---

## 5. Control, Interface & Multi-Window Architecture

The platform provides decoupled interfaces for interactive exploration & batch execution:

### 5.1. Master Terminal Loop (REPL)

- `MasterTerminalRepl` provides an asynchronous, non-blocking command-line interface.
- Direct command parsing (`CommandParser`) translates text commands into strongly-typed `Command` structures dispatched to the simulation orchestrator without graphical dependencies. A line is rejected when the queue (capacity 1024) is full; applied commands report their outcome through the result queue. Command processing rules are listed in [CLI_REFERENCE.md](CLI_REFERENCE.md#25-command-processing-and-performance-presets).

### 5.2. Graphical Multi-Window Workspace (UI Architecture)

When executing in interactive mode, `UiManager` coordinates GLFW windowing, OpenGL 3.3+ rendering contexts, & ImGui/ImPlot multi-viewport docking:
- Primary Viewport (`ViewportPrimaryWindow`): High-resolution display of the raytraced image stream with interactive HUD telemetry overlays & camera transport toolbars.
- Control Panel (`ControlPanelWindow`): Tabbed configuration interfaces for spacetime parameters, camera optics, skybox environments, integrators, & 6-DOF relativistic rocket propulsion.
- Scenario Selector (`ScenarioSelectorWindow`): Scenario browser supporting preset loading & YAML file serialization.
- Curvature Diagnostics & Invariants (`TelemetryWindow`, `VisualDiagnosticsWindow`): Real-time numerical display & temporal history graphs of Ricci scalar curvature $R$, Kretschmann invariant $K_1$, metric tensor components $g_{\mu\nu}$, & horizon radii.
- Spectrograph Monitor (`SpectrographWindow`): Real-time plotting of spectral radiance curves $I(\lambda)$ with $1\sigma$ confidence bands & perceived CIE sRGB color swatches.
- Performance Settings (`PerformanceSettingsWindow`): Profiles, internal render scale adjustment, ray budget limits, & arithmetic precision toggling.
- Interactive Camera Controller (`InteractiveCameraController`): Manages navigation modes (Free-Fly 6-DOF, Orbit Center, Spherical Boyer-Lindquist, and Rocket Thrust) with mouse-look, hotkey shortcuts, and 8 projection modes (Pinhole, AutoZoom, FisheyeStereographic, Equirectangular360, FisheyeEquidistant, FisheyeOrthographic, PaniniCylindrical, HammerAitoff). Mode behaviors are listed in [CLI_REFERENCE.md](CLI_REFERENCE.md#32-navigation-modes).
- Auxiliary Interface Windows: `BodyManagerWindow` (N-body catalog & interaction configuration), `HudManagerWindow` (on-screen telemetry layout), `ConstantsWindow` (physical constants presets), `KeybindSettingsWindow` (input rebinding), `LogConsoleWindow` (engine log viewer with severity filters and, on Windows, a system console toggle), `PerformanceAnalysisWindow` (benchmark capture & bottleneck analysis), `SecondaryViewportManager` (auxiliary observer viewports), and `CaptureStudioWindow` (screenshots, sequences, motion scripts, data recording and video encoding settings, described in [Section 2.10](#210-capture-subsystem)).
- Layout Presets: `UiManager` provides the presets MultiWindowDetached, DockedWorkspace, ViewportFocused and DeepAnalysis, applied on the next frame; global hotkeys are processed only when no text widget captures the keyboard.

---

## 6. Scientific I/O & External Interoperability Pipeline

The engine interfaces with standard astronomical data formats & ephemeris services:

### 6.1. Ephemeris & Orbital Mechanics Ingestion

- `HorizonsInterface`: Formulates REST queries & parses NASA JPL Horizons CSV vector tables, converting barycentric state vectors to SI units.
- `SpkKernel` & `SpkChebyshevSegment`: Evaluates Type 2 (position-only) and Type 3 (position/velocity) Chebyshev segments via polynomial recurrence relations. `parse_daf_spk_bytes` validates the file header only; segment loading from DAF records is not implemented (see [FILE_FORMATS.md](FILE_FORMATS.md#52-spice-binary-spk-kernels-bsp)).
- `FrameTransformer`: Converts state vectors & four-vectors between ICRF J2000, Heliocentric Ecliptic, Geocentric Equatorial, & comobile boosted reference frames.

### 6.2. Scientific Data Serialization

- `ScenarioSerializer`: Reads & writes declarative YAML scenario files (`.yaml`).
- `FitsExporter`: Generates 2D radiance images & 3D spectral data cubes $(X, Y, \lambda)$ adhering to the FITS Standard 4.0 with WCS astrometric headers & big-endian IEEE 754 formatting.
- `Hdf5Container`: Serializes hierarchical binary datasets including worldlines, metric series, covariance matrices, & tabulated nuclear equations of state.
- `VtkExporter`: Generates VTK XML PolyData files (`.vtp`) for polyline trajectories & event horizon surface meshes.
- `ImageCodecs`, `ImageStreamWriter` and `ScreenshotExporter`: Encode and stream ten image formats ([FILE_FORMATS.md](FILE_FORMATS.md#6-image-output-formats)).
- `TelemetryTable` and `write_telemetry_table`: Store and serialize recorded channels in seven formats ([FILE_FORMATS.md](FILE_FORMATS.md#8-telemetry-recording-files)).
- `UserSettings`, `CaptureStudioSettings` and `PerformanceProfiler` persistence: Key-value configuration files under `config/` ([FILE_FORMATS.md](FILE_FORMATS.md#10-persistent-configuration-files)).

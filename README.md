# Relativity-Simulator

Deterministic simulation and ray-tracing engine for General Relativity, Post-Newtonian dynamics, relativistic hydrodynamics, and polarized radiative transfer in curved spacetimes.

---

## Documentation Map

Primary references:

- [System Description](docs/DESCRIPTION.md): scientific scope, runtime modes, validation targets.
- [Technical Architecture](docs/ARCHITECTURE.md): subsystem structure, scheduling, concurrency, precision model.
- [Technical Manual](docs/TECHNICAL_MANUAL.md): component-level implementation and engineering constraints.
- [Mathematical Formulation](docs/MATHEMATICAL_FORMULATION.md): equations, metrics, transport formalisms, PN terms.
- [File Formats & I/O](docs/FILE_FORMATS.md): YAML schema, FITS/HDF5/VTK formats, SPICE/HORIZONS ingestion, cluster configuration.
- [CLI & REPL Reference](docs/CLI_REFERENCE.md): interactive commands, headless exporter options, cluster worker daemon.
- [Testing Guide](docs/testing/README.md): test suite structure, writing rules, and validation targets.
- [Documentation Index](docs/INDEX.md): index of the documentation set.
- [Source Tree](docs/TREE.md): repository layout.

---

## Key Features

- **Spacetime Models**
  - Analytic metrics: [Minkowski](docs/MATHEMATICAL_FORMULATION.md#11-minkowski-metric-flat-spacetime), [Schwarzschild](docs/MATHEMATICAL_FORMULATION.md#12-schwarzschild-metric), [Kerr](docs/MATHEMATICAL_FORMULATION.md#13-kerr-metric-rotating-black-hole), [Reissner–Nordström](docs/MATHEMATICAL_FORMULATION.md#14-reissner-nordström-metric-charged-black-hole), [Kerr–Newman](docs/MATHEMATICAL_FORMULATION.md#15-kerr-newman-metric-charged-rotating-black-hole), [Schwarzschild–de Sitter (Kottler)](docs/MATHEMATICAL_FORMULATION.md#16-schwarzschild-de-sitter--kottler-metric) ([Catalog & Gauges](docs/DESCRIPTION.md#41-analytical-vacuum--electrovacuum-solutions)).
  - Coordinate forms: [Boyer–Lindquist](docs/MATHEMATICAL_FORMULATION.md#13-kerr-metric-rotating-black-hole), [Kerr–Schild Cartesian](docs/MATHEMATICAL_FORMULATION.md#13-kerr-metric-rotating-black-hole), [Eddington–Finkelstein](docs/MATHEMATICAL_FORMULATION.md#12-schwarzschild-metric), [Painlevé–Gullstrand](docs/MATHEMATICAL_FORMULATION.md#12-schwarzschild-metric), [isotropic](docs/MATHEMATICAL_FORMULATION.md#12-schwarzschild-metric) ([Coordinate Systems](docs/DESCRIPTION.md#42-coordinate-systems--gauges)).
  - Additional models: [FLRW](docs/MATHEMATICAL_FORMULATION.md#17-flrw-metric-cosmological-spacetime), [Morris–Thorne](docs/MATHEMATICAL_FORMULATION.md#18-morris-thorne-traversable-wormhole), [Alcubierre](docs/MATHEMATICAL_FORMULATION.md#19-alcubierre-warp-drive-metric), [Lemaitre–Tolman–Bondi](docs/MATHEMATICAL_FORMULATION.md#110-lemaitre-tolman-bondi-void-metric), [Quintessence](docs/MATHEMATICAL_FORMULATION.md#111-quintessence-dark-energy-cosmology), [Chaplygin](docs/MATHEMATICAL_FORMULATION.md#112-chaplygin-gas-cosmology), [Vaidya evaporating](docs/MATHEMATICAL_FORMULATION.md#113-evaporating-vaidya-black-hole-metric), and [TeVeS](docs/MATHEMATICAL_FORMULATION.md#114-teves-tensor-vector-scalar-metric) ([Cosmological & Exotic Spacetimes](docs/DESCRIPTION.md#43-cosmological--exotic-spacetimes)).
  - Specialized numerical solvers (`trace_extended_photon` and `trace_spherical_spacetime_dispatch`) directly integrate expanding cosmologies, wormhole throats, warp bubbles, scalar fields, and time-dependent evaporation rates ([Technical Manual](docs/TECHNICAL_MANUAL.md#metric-usage-in-softwarecomputeengine)).
  - Multi-source Kerr black hole superposition: simultaneous Hamiltonian integration through up to 16 Kerr black holes using Cartesian Kerr-Schild metrics with analytical horizon entry and disk crossing detection ([Technical Manual](docs/TECHNICAL_MANUAL.md#22-spacetime-metric-implementations), [Mathematical Formulation](docs/MATHEMATICAL_FORMULATION.md#13-kerr-metric-rotating-black-hole)).
  - Numerical spacetime support: [3+1 BSSN metric grids](docs/DESCRIPTION.md#44-31-numerical-relativity-grids-bssn) with spatial and temporal interpolation ([Metric Modules](docs/ARCHITECTURE.md#22-spacetime-metric-modules)).
  - Metric invariants and curvature tensors: [Riemann, Ricci, and Kretschmann invariants](docs/MATHEMATICAL_FORMULATION.md#23-riemann-tensor-ricci-tensor--kretschmann-scalar) ([Diagnostics Window](docs/TECHNICAL_MANUAL.md#28-orchestration-scheduling--user-interface)).

- **Geodesics and Integrators**
  - Adaptive explicit integrators: [RK45 Dormand–Prince with infalling radial step limiting and algebraic constraint projection](docs/ARCHITECTURE.md#23-differential-solvers--geodesic-integrators), [Cash–Karp 5(4) with wide stability intervals](docs/ARCHITECTURE.md#23-differential-solvers--geodesic-integrators), [Vernier 9(8) 16-stage high-order Runge-Kutta scheme](docs/ARCHITECTURE.md#23-differential-solvers--geodesic-integrators).
  - Symplectic integrators: [Gauss–Legendre orders 4 and 6](docs/ARCHITECTURE.md#23-differential-solvers--geodesic-integrators) satisfying $b_i A_{ij} + b_j A_{ji} - b_i b_j = 0$ with Picard stage iterations to tolerance $10^{-14}$, [Forest–Ruth post-Newtonian scheme](docs/ARCHITECTURE.md#34-orchestrator-state-advance).
  - Predictor-corrector: [Hermite 4th-order Aarseth scheme](docs/ARCHITECTURE.md#23-differential-solvers--geodesic-integrators) evaluating 4th-order finite difference jerk, snap, crackle, and Aarseth variable timestep regulation.
  - Digital filter step controllers: [Standard, PI30, and PID42 digital filter controllers](docs/MATHEMATICAL_FORMULATION.md#185-digital-filter-adaptive-step-size-controllers-standard-pi30-pid42) eliminating step chattering.
  - Horizon boundary state machine: [HorizonDetector](docs/MATHEMATICAL_FORMULATION.md#186-algebraic-invariant-constraint-projection--horizon-boundary-classification) tracking exterior, horizon surface, interior, and physical singularity boundaries with absorption and continuity modes.
  - Specialized null geodesic solvers: [Cartesian angular-momentum conserving Schwarzschild RK4, Kerr-Schild Hamiltonian RK4, planar equatorial RK4, and SIMD geodesic bundles](docs/MATHEMATICAL_FORMULATION.md#187-specialized-null-geodesic-differential-integrators).
  - Image rendering integrates null rays with specialized null RK4 schemes; the integrator selected at runtime determines the N-body integration only ([Technical Manual](docs/TECHNICAL_MANUAL.md#runtime-use-of-the-integrator-selection)).

- **Relativistic Dynamics**
  - Post-Newtonian N-body dynamics: conservative [1PN Einstein-Infeld-Hoffmann](docs/MATHEMATICAL_FORMULATION.md#31-conservative-1pn-acceleration-einstein-infeld-hoffmann), [2PN, and 3PN](docs/DESCRIPTION.md#51-post-newtonian-multi-body-dynamics) terms.
  - Dissipative terms: [2.5PN radiation reaction](docs/MATHEMATICAL_FORMULATION.md#32-25pn-radiation-reaction-acceleration) and [3.5PN](docs/DESCRIPTION.md#51-post-newtonian-multi-body-dynamics) radiation dissipation.
  - Spin dynamics: [spin-orbit and spin-spin couplings](docs/MATHEMATICAL_FORMULATION.md#33-spin-orbit--spin-spin-coupling).
  - Dedicated [Hulse–Taylor binary pulsar model](docs/DESCRIPTION.md#113-standard-simulation-scenarios) ([Dynamics Subsystem](docs/TECHNICAL_MANUAL.md#24-post-newtonian-multi-body-subsystem)).
  - Interaction layers: [Coulomb, magnetic dipole, collision, thermodynamic, fragmentation, and annihilation](docs/DESCRIPTION.md#54-body-interactions--spacetime-source-bodies) ([Dynamics Subsystem](docs/TECHNICAL_MANUAL.md#24-post-newtonian-multi-body-subsystem)).
  - [Spacetime-source bodies](docs/DESCRIPTION.md#54-body-interactions--spacetime-source-bodies) with Kerr horizons, horizon absorption, and body merging ([State Advance](docs/ARCHITECTURE.md#34-orchestrator-state-advance)).
  - [Body Manager architecture](docs/TECHNICAL_MANUAL.md#24-post-newtonian-multi-body-subsystem) partitioned into seven functional tabs (Object Catalog, Source Catalog, Create Object, Create Source, System Dynamics, Interactions, Randomizer Controls).
  - [Multi-Body Geometric Pattern Generator](docs/MATHEMATICAL_FORMULATION.md#115-multi-body-geometric-pattern-formations) synthesizing annular asteroid belts, coplanar debris swarms, and 3D spherical formations (Plummer clusters, uniform spheres, Fibonacci lattice shells, infalling blast waves) across 10 archetype presets with Kirkwood gap simulation and collision safeguards.
  - [Gravitational Hierarchy Engine](docs/MATHEMATICAL_FORMULATION.md#114-gravitational-hierarchy--hill-sphere-stability) with Hill sphere occupancy tracking and Kozai-Lidov resonance detection.
  - [Local Frenet-Serret RTN Maneuver Injector](docs/MATHEMATICAL_FORMULATION.md#116-orbital-maneuvers--rtn-frame-impulses) decomposing impulses along radial, tangential, and normal axes with pre-burn orbit prediction, Hohmann transfer departures, and isokinetic plane-change burns.
  - [Relative Orbital Plane Alignment Tool](docs/MATHEMATICAL_FORMULATION.md#112-relative-orbital-plane-alignment--rotations) supporting coplanar, polar, and retrograde snaps relative to any attractor.
  - [Bulk body actions](docs/TECHNICAL_MANUAL.md#24-post-newtonian-multi-body-subsystem) (velocity and spin assignment, grid snapping, scattering, mass equalization, parameter equalization, culling; [Keybinds](docs/CLI_REFERENCE.md#34-additional-rebindable-actions)).

- **Gravimetry and Large-Scale Gravity**
  - [Spherical harmonic gravity expansions](docs/DESCRIPTION.md#52-spherical-harmonics--high-degree-geodesy) up to degree and order 32 ($J_n$, $C_{nm}$, $S_{nm}$ [Architecture](docs/ARCHITECTURE.md#24-post-newtonian-dynamics--gravimetry)).
  - [Analytical orbital precession](docs/ARCHITECTURE.md#24-post-newtonian-dynamics--gravimetry) and [tidal Love number perturbations](docs/ARCHITECTURE.md#24-post-newtonian-dynamics--gravimetry).
  - Dark matter modules: [thirteen analytic density profiles](docs/MATHEMATICAL_FORMULATION.md#171-analytic-density-profiles--enclosed-mass) (NFW, Hernquist, Burkert, Einasto, Plummer, Cored Isothermal, Singular Isothermal, Dehnen, Point Mass, Jaffe, Moore, Soliton BEC core, King Beta model), [triaxial ellipsoidal geometry](docs/MATHEMATICAL_FORMULATION.md#172-triaxial-ellipsoidal-coordinates--frame-rotations) ($q_y, q_z$), Tait-Bryan Euler orientations, [geodesic lensing kicks](docs/MATHEMATICAL_FORMULATION.md#173-weak-field-geodesic-deflection-kicks--step-control), [three false-color visualization modes](docs/MATHEMATICAL_FORMULATION.md#174-line-of-sight-emissive-density-field--velocity-visualization), [Keplerian halo orbits](docs/MATHEMATICAL_FORMULATION.md#175-halo-keplerian-orbital-dynamics), [five geometric layout generators](docs/DESCRIPTION.md#53-dark-matter-halos--alternative-gravitational-theories), [subhalo population generator](docs/MATHEMATICAL_FORMULATION.md#176-subhalo-population-statistical-generation), [Barnes–Hut octree acceleration with quadrupole multipoles](docs/MATHEMATICAL_FORMULATION.md#177-collisionless-n-body-dynamics--barnes-hut-octree-multipoles), [composite galaxy synthesis](docs/MATHEMATICAL_FORMULATION.md#178-composite-multi-component-galaxy-model-generation), and [MOND, TeVeS, and \(f(R)\) Chameleon models](docs/ARCHITECTURE.md#25-dark-matter--modified-gravity).
  - Cosmological and semiclassical modules: [inhomogeneous LTB voids](docs/MATHEMATICAL_FORMULATION.md#110-lemaitre-tolman-bondi-void-metric) with exact cycloid shell evolution and bang times, [quintessence scalar fields with four potentials](docs/MATHEMATICAL_FORMULATION.md#111-quintessence-dark-energy-cosmology) (Exponential, Ratra-Peebles, PNGB, SUGRA), [generalized Chaplygin gas](docs/MATHEMATICAL_FORMULATION.md#112-chaplygin-gas-cosmology), [Kerr horizon surface gravity thermodynamics](docs/MATHEMATICAL_FORMULATION.md#113-semiclassical-hawking-evaporation--unruh-thermometry), [Polyakov 2D renormalized stress-energy tensors](docs/MATHEMATICAL_FORMULATION.md#113-semiclassical-hawking-evaporation--unruh-thermometry), [coupled Hawking mass loss and spin-down integration](docs/MATHEMATICAL_FORMULATION.md#113-semiclassical-hawking-evaporation--unruh-thermometry), and [Unruh-DeWitt detector excitation rates](docs/MATHEMATICAL_FORMULATION.md#113-semiclassical-hawking-evaporation--unruh-thermometry).

- **Application Lifecycle, Platform Bootstrap & Startup Window**
  - [Operating-system platform bootstrap](docs/ARCHITECTURE.md#215-application-lifecycle-platform-bootstrap--startup-window-subsystem): per-monitor v2 DPI scaling, explicit Application User Model ID registration, heap corruption termination, and asset path normalization across ancestor directories ([Technical Manual](docs/TECHNICAL_MANUAL.md#214-application-lifecycle-platform-bootstrap--startup-window-subsystem)).
  - [Single-instance mutual exclusion](docs/ARCHITECTURE.md#215-application-lifecycle-platform-bootstrap--startup-window-subsystem): Win32 named mutex and POSIX advisory lockfile with existing window detection, unminimizing, and foreground activation.
  - [Staged startup progress monitoring](docs/ARCHITECTURE.md#215-application-lifecycle-platform-bootstrap--startup-window-subsystem): 9 sequential range stages (`StartupStage`), asymptotic creep limit smoothing, cooperative cancellation, and live shader compilation tracking.
  - [Autonomous native Win32 splash window](docs/TECHNICAL_MANUAL.md#214-application-lifecycle-platform-bootstrap--startup-window-subsystem): background worker thread with `std::promise` synchronization, flicker-free GDI double-buffering, 8 dynamic typography fonts, 15 ms progress bar easing, 100 ms elapsed timer, interactive controls (Minimize, Cancel, Docs, License, GitHub), and Windows 11 DWM rounded corners.

- **Relativistic Hydrodynamics and Compact Objects**
  - [GRHD/GRMHD 3+1 conservative formulation](docs/MATHEMATICAL_FORMULATION.md#4-relativistic-hydrodynamics-grhdgrmhd) with [WENO5-JS, WENO5-Z, and MP5 reconstructions](docs/DESCRIPTION.md#61-curved-spacetime-hydrodynamics) and [TVD slope limiters](docs/MATHEMATICAL_FORMULATION.md#45-high-order-spatial-reconstruction-schemes).
  - Multi-wave approximate Riemann solvers: [HLL, HLLC with contact wave restoration, and HLLD](docs/MATHEMATICAL_FORMULATION.md#46-multi-wave-approximate-riemann-solvers) ([Architecture](docs/ARCHITECTURE.md#26-relativistic-hydrodynamics-grhdgrmhd)).
  - Exact divergence control: [constrained transport 2D](docs/MATHEMATICAL_FORMULATION.md#47-constrained-transport-ct--solenoidal-magnetic-constraint) enforcing \(\nabla \cdot \mathbf{B} = 0\) via vertex electric fields.
  - [Conservative-to-primitive variable inversion](docs/MATHEMATICAL_FORMULATION.md#44-conservative-to-primitive-variable-inversion-formulation): bracketed 1D Newton-Raphson solvers for hydro ($p$) and MHD ($z$) with bisection safeguards.
  - One-dimensional relativistic fluid solver: [RelativisticHydroSolver1D](docs/TECHNICAL_MANUAL.md#25-relativistic-hydrodynamics-grhdgrmhd--stellar-physics) with 3 ghost cells, Outflow/Periodic/Reflecting boundaries, acoustic CFL stepping, and SSP-RK3 temporal evolution.
  - EOS models: [ideal gas, Synge, Mathews, relativistic Fermi gas, polytropic, piecewise polytropic, and tabulated nuclear EOS](docs/MATHEMATICAL_FORMULATION.md#49-relativistic-equations-of-state-eos-formulations) with 3D trilinear interpolation (SFHo, Shen, LS220, SLy4, APR4) and HDF5 container serialization.
  - Compact-object modeling: [Tolman-Oppenheimer-Volkoff (TOV) solver](docs/MATHEMATICAL_FORMULATION.md#48-tolman-oppenheimer-volkoff-tov-stellar-structure--redshift-formulation) with adaptive scale-height RK4 stepping, surface/central redshifts, compactness, and mass-radius stability curve generation ($M_{\max}$, $R_{1.4 M_\odot}$).
  - Relativistic accretion and magnetosphere models: [Novikov–Thorne thin disk](docs/MATHEMATICAL_FORMULATION.md#51-novikov-thorne-thin-disk-profile) with Page-Thorne analytical flux integration, [Fishbone–Moncrief torus](docs/MATHEMATICAL_FORMULATION.md#52-fishbone-moncrief-magnetized-thick-torus) in barotropic equilibrium, and [Blandford-Znajek jet magnetosphere](docs/MATHEMATICAL_FORMULATION.md#53-blandford-znajek-jet-magnetosphere).

- **Radiative Transfer and Rendering**
  - [Backward null geodesic ray tracing](docs/DESCRIPTION.md#91-backward-null-geodesic-raytracing) in curved spacetimes ([Mathematical Formulation](docs/MATHEMATICAL_FORMULATION.md#8-image-formation-in-the-renderer)).
  - [Polarized transfer in full Stokes representation](docs/MATHEMATICAL_FORMULATION.md#6-polarized-radiative-transfer--relativistic-optics) \((I,Q,U,V)\) via Delano analytical matrix exponential scaling and squaring, linear/circular/total polarization degrees, EVPA, and Mueller reference frame rotations; image rendering uses [blackbody disk emission integrated with CIE 1931 matching functions](docs/MATHEMATICAL_FORMULATION.md#82-step-control-termination-and-space-skipping).
  - Radiative processes: [thermal and non-thermal synchrotron with Faraday rotation and conversion, thermal bremsstrahlung with relativistic Gaunt factor, and inverse Compton scattering with Klein-Nishina differential cross sections](docs/MATHEMATICAL_FORMULATION.md#64-radiative-processes--coefficients) ([Architecture](docs/ARCHITECTURE.md#27-polarized-radiative-transfer--optics)).
  - Kinetic electron distributions: [Maxwell-Jüttner 4-regime sampling](docs/MATHEMATICAL_FORMULATION.md#65-maxwell-jüttner-relativistic-thermal-distribution) with rational Bessel approximations and Doppler line broadening kernels.
  - [Continuous spectral pipeline with 4096-bin logarithmic grid, CIE 1931 observer convolution, and ACES/logarithmic HDR tonemapping](docs/MATHEMATICAL_FORMULATION.md#67-continuous-spectral-pipeline--cie-1931-integration) ([Mathematical Formulation](docs/MATHEMATICAL_FORMULATION.md#68-high-dynamic-range-tone-mapping-operators)).
  - [Analytical Carter constant null geodesic classification](docs/MATHEMATICAL_FORMULATION.md#69-carter-constant-geodesic-classification--analytical-turning-points) using Cardano and Ferrari depressed quartic root solving for absorption and escape turning points.
  - [Geodesic ray probe inspection](docs/MATHEMATICAL_FORMULATION.md#610-relativistic-ray-probe-diagnostics--image-orders) resolving constants of motion ($E, L_z, \xi, \mathcal{Q}, \eta, b$), sub-step disk crossings, observed temperatures, image orders, and 3D deflection angles.
  - [Relativistic Terrell-Penrose aberration](docs/MATHEMATICAL_FORMULATION.md#611-terrell-penrose-relativistic-aberration--moving-sphere-intersections) and moving sphere analytical spacetime ray intersections with retarded time solving.
  - [Vulkan compute pipeline in native FP64](docs/ARCHITECTURE.md#29-render-pipeline) with [CPU SIMD (AVX2/AVX-512/Neon) and scalar fallbacks](docs/TECHNICAL_MANUAL.md#29-rendering-back-ends) in double and single precision.
  - [Ray-traced ellipsoidal celestial bodies](docs/MATHEMATICAL_FORMULATION.md#84-celestial-body-shading) with 27 surface texture modes, including 14 photographic planetary maps (Earth Blue Marble with automatic twilight terminator blending, Sun, Mercury, Venus, Mars, Jupiter, Saturn, Uranus, Neptune, Ceres, Eris, Haumea, Makemake) at 1K/2K resolutions ([Technical Manual](docs/TECHNICAL_MANUAL.md#29-rendering-back-ends)).
  - [3D body illumination engine](docs/MATHEMATICAL_FORMULATION.md#84-celestial-body-shading) supporting 7 light source modes (CentralSource, CameraHeadlight, FixedDirection with azimuth/elevation, FixedPoint, NearestEmissiveBody, SpecificBody, Unlit), inverse-square distance attenuation with $64\times$ clamp, wrapped diffuse terminator softness, Blinn-Phong specular reflections, shadow occlusion rays tested against body ellipsoids and horizon spheres, and inter-body emission lighting with $16\times$ clamp across 3 detail presets (Minimal, Reduced, Full; [Architecture](docs/ARCHITECTURE.md#29-render-pipeline), [Technical Manual](docs/TECHNICAL_MANUAL.md#29-rendering-back-ends)).
  - [Procedural multi-layer skybox engine](docs/MATHEMATICAL_FORMULATION.md#12-procedural-skybox-mathematics) evaluating equal-area spherical voxel partitioning, 8 star point spread function models (Gaussian, Lorentzian, Moffat, Airy diffraction with Chebyshev Bessel approximation, HardDisc, DiffractionSpikes, PinpointDirac, CinematicBloom), 6 star placement laws, 4 parameter sampling distributions, multi-octave curl-noise nebulae, open and globular star clusters with Plummer density profiles, interstellar dust extinction lanes, deep-field Sersic galaxies across 6 morphologies, quasars with relativistic jets and Doppler beaming ($\delta^3$), pulsars with rotating magnetic sweep beams, supernova remnant shells, and 3D spherical coordinate grids supporting degree ticks and 5 cell fill modes over a 16-color palette ([Technical Manual](docs/TECHNICAL_MANUAL.md#29-rendering-back-ends)).
  - [Schematic projection overlay](docs/ARCHITECTURE.md#212-schematic-view-and-hud) with first-order light deflection ([Mathematical Formulation](docs/MATHEMATICAL_FORMULATION.md#85-schematic-overlay-deflection)) and [secondary observer viewports](docs/TECHNICAL_MANUAL.md#28-orchestration-scheduling--user-interface) executing comoving free-fall probe kinematics, asymptotic telephoto cameras, and 3D lensed beacon markers ([Mathematical Formulation](docs/MATHEMATICAL_FORMULATION.md#16-multi-observer-secondary-viewports--relativistic-probe-kinematics)).
  - [3D spatial reference system](docs/ARCHITECTURE.md#212-schematic-view-and-hud) providing Cartesian reference planes (XY, XZ, YZ), adaptive logarithmic spacing, graduation ticks, distance rings, horizon occlusion, and dynamic scale bar calibration ([Mathematical Formulation](docs/MATHEMATICAL_FORMULATION.md#15-3d-spatial-reference--coordinate-grid-mathematics)).
  - Still frame and sequence image streaming in [ten output formats](docs/FILE_FORMATS.md#6-image-output-formats) ([Capture Subsystem](docs/ARCHITECTURE.md#210-capture-subsystem)).
  - Synthetic VLBI interferometry pipeline: [Event Horizon Telescope array modeling](docs/DESCRIPTION.md#95-synthetic-vlbi-interferometry--event-horizon-telescope-modeling), Fourier $(u, v)$ coverage synthesis, [closure phase evaluation](docs/MATHEMATICAL_FORMULATION.md#135-closure-phase-invariance--thin-ring-diameter-estimation), thin-ring first null diameter fitting, and [OIFITS standard export](docs/FILE_FORMATS.md#23-opticalinfraredradio-interferometry-exchange-format-oifits).

- **Distributed Computing and Heterogeneous Cluster Engine**
  - [Cluster topology and operational roles](docs/ARCHITECTURE.md#214-distributed-compute-subsystem--heterogeneous-cluster-architecture): master orchestrator, autonomous worker daemon (`cluster_worker`), hybrid participatory nodes, and isolated offline standalone mode.
  - [Zero-overhead offline autonomy](docs/ARCHITECTURE.md#214-distributed-compute-subsystem--heterogeneous-cluster-architecture): complete decoupling ensuring zero network allocations, sockets, or thread overhead when `cluster.enabled = 0`.
  - [Dynamic latency-damped load balancing](docs/MATHEMATICAL_FORMULATION.md#101-latency-damped-capacity-share-formulation) allocating tile quotas proportionally to measured throughput and $(\text{RTT})^{-\alpha}$ latency penalties.
  - [Fault-tolerant dynamic work stealing](docs/MATHEMATICAL_FORMULATION.md#103-dynamic-work-stealing-deadline-threshold) reclaiming stalled remote tiles onto the local thread pool upon deadline expiration without frame drops.
  - [Cryptographic transport channel](docs/TECHNICAL_MANUAL.md#213-distributed-computing--cluster-engine): X25519 ECDH key agreement, HKDF-SHA256 derivation, and ChaCha20-Poly1305 AEAD authenticated encryption with a 64-packet anti-replay sliding window.
  - [Adaptive tile compression codecs](docs/TECHNICAL_MANUAL.md#213-distributed-computing--cluster-engine): DirectRaw for local 10GbE LAN, QOI HDR with FP16 radiance and RLE, and LZ4 block compression with SIMD channel-shuffling delivering $> 3.5\,\text{GB/s}$ decompression throughput.
  - [SIMD radiance data sanitization](docs/TECHNICAL_MANUAL.md#213-distributed-computing--cluster-engine) guarding against IEEE 754 NaNs, infinities, and unphysical negative fluxes.
  - [Distributed task routing](docs/TECHNICAL_MANUAL.md#213-distributed-computing--cluster-engine): five task classes encompassing $32 \times 32$ ray-traced viewport tiles with pre-pass culling, capture studio image bands, row-partitioned post-Newtonian N-body accelerations evaluated on GPU via `GpuNBodyExecutor` (`nbody_accel_fp64.comp`) or CPU, Monte Carlo geodesic bundle ensembles, and polynomial chaos stochastic quadratures.
  - [UDP multicast auto-discovery](docs/TECHNICAL_MANUAL.md#213-distributed-computing--cluster-engine) on `239.255.77.88:42424` via packed 72-byte `DiscoveryBeacon` datagrams with SHA-256 namespace tag derivation and [HPC Slurm cluster integration](docs/CLI_REFERENCE.md#5-cluster-compute-worker-cluster_worker) with automatic MPI rank and nodelist detection.
  - [Dedicated SPIR-V compute shaders](docs/TECHNICAL_MANUAL.md#213-distributed-computing--cluster-engine): `cluster_tile_tracer_fp64.comp` and `nbody_accel_fp64.comp`.
  - [Distributed verification test suites](docs/testing/README.md#test-catalog): cluster cryptography ([suites/cluster_crypto_protocol.md](docs/testing/suites/cluster_crypto_protocol.md)), scheduler work stealing ([suites/cluster_scheduler_stealing.md](docs/testing/suites/cluster_scheduler_stealing.md)), compression codecs ([suites/cluster_serialization_codecs.md](docs/testing/suites/cluster_serialization_codecs.md)), worker execution ([suites/cluster_worker_execution.md](docs/testing/suites/cluster_worker_execution.md)), distributed pipeline reassembly ([suites/distributed_geodesic_pipeline.md](docs/testing/suites/distributed_geodesic_pipeline.md)), and multi-worker throughput scaling ([suites/cluster_throughput.md](docs/testing/suites/cluster_throughput.md)).

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
  - [Scriptable multi-segment camera motion paths](docs/ARCHITECTURE.md#210-capture-subsystem) with 29 shape layers across 7 categories, 9 orientation modes (including chase-lead velocity tracking), 16 presets, 7 projection transition morphing modes, 46 easing curves, and expression evaluations ([Mathematical Formulation](docs/MATHEMATICAL_FORMULATION.md#9-motion-script-mathematics)).
  - [Motion script serialization and scripted timeline events](docs/FILE_FORMATS.md#9-motion-script-and-capture-settings-files) with 15 actions, continuous ramps, and 52 parameter bindings ([Capture Subsystem](docs/ARCHITECTURE.md#210-capture-subsystem)).
  - [Deterministic and real-time capture pipelines](docs/ARCHITECTURE.md#210-capture-subsystem) with banded rendering, sub-frame temporal averaging, camera collision avoidance, post-processing color grading, triangular noise dithering, and [automated ffmpeg video encoding](docs/FILE_FORMATS.md#7-capture-session-directory) with custom filter chains and rate control.
  - [Multi-channel physics telemetry recording across 54 kinematic and relativistic channels in seven file formats](docs/FILE_FORMATS.md#8-telemetry-recording-files) ([Capture Modules](docs/TECHNICAL_MANUAL.md#210-capture-modules)).
  - [Path preview generation](docs/ARCHITECTURE.md#210-capture-subsystem) for visual trajectory inspection in the schematic overlay ([Capture Modules](docs/TECHNICAL_MANUAL.md#210-capture-modules)).

- **Interactive Workspace**
  - [Primary viewport window](docs/ARCHITECTURE.md#29-render-pipeline) with camera transport toolbar, hold-to-zoom magnification, and a [configurable 25-element HUD](docs/ARCHITECTURE.md#212-schematic-view-and-hud) ([Technical Manual](docs/TECHNICAL_MANUAL.md#28-orchestration-scheduling--user-interface)).
  - [Multi-window docking workspace](docs/ARCHITECTURE.md#52-graphical-multi-window-workspace-ui-architecture) with dedicated interfaces for master simulation controls, scenario selector, celestial body manager, curvature diagnostics, spectrograph (including synthetic VLBI interferometry and polarization analysis), performance optimization, profiling, [Comparison Workbench](docs/TECHNICAL_MANUAL.md#28-orchestration-scheduling--user-interface) with Welch's $t$-test significance testing, physical constants, key bindings, log console, and Capture Studio ([Technical Manual](docs/TECHNICAL_MANUAL.md#28-orchestration-scheduling--user-interface)).
  - [Six navigation modes](docs/CLI_REFERENCE.md#32-navigation-modes) (Free-Fly 6-DOF, Orbit Center, Spherical Boyer-Lindquist, Rocket Thrust, Surface Walk on celestial bodies, and Planet Orbit).
  - [External gamepad flight controller integration](docs/CLI_REFERENCE.md#31-navigation--viewport-keybindings) supporting Xbox, Bluetooth, and USB dual-stick controllers with deadzones, trigger elevation, bumper roll, tap and double-tap chords, alongside configurable arrow key modes.
  - [Forensic crash reporting and hang monitoring](docs/ARCHITECTURE.md#211-profiling-logging-and-persistence): structured crash dumps under `logs/crashes/` (Vectored Exception Handlers, Unhandled Exception Filters, `sigaction` with 64 KiB `sigaltstack`, registers, RIP disassembly, RSP stack words, DbgHelp/backtrace symbol stacks, process minidumps `minidump.dmp`, preference attachments, retention pruning), background `HangWatchdog` thread tracking UI and simulation heartbeats with GUI modal loop exclusion, and `.session_active` crash guard file.
  - [Physical constants engine](docs/ARCHITECTURE.md#213-physical-constants-engine) supporting SI, Planck, and Custom presets, with [independent unit preferences across 19 physical quantities](docs/TECHNICAL_MANUAL.md#211-constants-units-and-expressions) and [dimensional expression parsing](docs/TECHNICAL_MANUAL.md#211-constants-units-and-expressions).
  - [Modular persistent preferences architecture](docs/FILE_FORMATS.md#101-modular-preferences-architecture-configpreferences) partitioned across nine domain configuration files in `config/preferences/`.

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
- `glslangValidator` (optional): used by CMake to compile the primary geodesic tracer, cluster tile tracer, and N-body acceleration compute shaders to SPIR-V.
- Network and compression libraries (via CMake FetchContent when `ENGINE_ENABLE_DISTRIBUTED=ON`, enabled by default): ENet and LZ4.
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

### Cluster Worker Daemon Execution
```bash
# Start an autonomous worker listening on LAN (auto-discovered by running masters)
./build/cluster_worker --auth-token "a8f5c2d3e9b14708215649fbcde71024385960abcedf1234567890abcdef1234" --backend auto

# Start an outbound worker targeting a remote master host (cloud instance / Colab)
./build/cluster_worker --master-address 198.51.100.24:42425 --auth-token "a8f5c2d3e9b14708215649fbcde71024385960abcedf1234567890abcdef1234" --no-listen --no-discovery
```
Worker options, Slurm HPC submission scripts, and configuration schemas are documented in [Cluster Compute Worker](docs/CLI_REFERENCE.md#5-cluster-compute-worker-cluster_worker) and [Cluster Configuration File](docs/FILE_FORMATS.md#103-configdistributed_clustercfg).

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

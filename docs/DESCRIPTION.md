# Specification & System Description

## 1. Global Vision, Philosophy & Core Objectives

The engine is a scientific computing and interactive simulation software for Special Relativity, General Relativity, and alternative gravitational models. The platform implements analytical metrics, post-Newtonian dynamics, relativistic hydrodynamics, and polarized radiative transfer derived from standard relativistic physics formulations.

The platform operates across four complementary runtime paradigms:
- Application Lifecycle & Platform Bootstrap Orchestration: Staged execution initialization, per-monitor DPI scaling, single-instance inter-process synchronization, asset path normalization, and asynchronous native splash progress tracking with shader compilation monitoring ([Architecture Section 2.15](ARCHITECTURE.md#215-application-lifecycle-platform-bootstrap--startup-window-subsystem)).
- Real-Time Interactive Exploration Mode: A low-latency, interactive 3D navigation environment with direct 6-DOF controls, enabling an observer to traverse complex spacetimes, modify physical parameters dynamically, & observe relativistic optical effects, kinematic transformations, & geodesic trajectories in real time.
- High-Fidelity Batch / Headless Generation Mode: An automated computation pipeline executing intensive simulation sweeps, high-density raytracing, multi-body orbital evolutions, gravitational wave strain extractions, & spectral cube generation without requiring an active graphical context.
- Distributed & Heterogeneous Cluster Compute Execution: A decoupled network computation extension distributing screen tile ray-tracing, GPU-accelerated post-Newtonian N-body row reductions (`GpuNBodyExecutor`), Monte Carlo geodesic ensembles, and polynomial chaos quadratures across clusters (LAN multicast discovery on `239.255.77.88:42424`, cloud workers, and Slurm HPC allocations) while maintaining zero-overhead offline autonomy.

The architectural foundation relies on modularity: spacetimes, numerical integrators, equations of state, radiative transfer models, observer kinematics, & uncertainty quantification engines are interchangeable components adhering to uniform concepts. All computational paths prioritize performance through shared-memory multithreading, explicit SIMD register vectorization, persistent memory arenas, & GPU compute pipelines.

Uncertainty quantification & formal error propagation are native primitives within the system, allowing the rigorous qualification of truncation errors, initial condition perturbations, & numerical discretization artifacts alongside physical signals.

---

## 2. System Modularity & Component Selection Matrix

The engine provides configuration flexibility across all layers of the physical simulation stack. Each simulation scenario explicitly defines the active theoretical framework, numerical approximation order, & observation pipeline.

### 2.1. Permutable Component Matrix

- Spacetime Representation: Selection between stationary analytical solutions ([MATHEMATICAL_FORMULATION.md Section 1](MATHEMATICAL_FORMULATION.md#1-spacetime-metrics--line-elements)), dynamical vacuum metrics, cosmological expanding backgrounds ([Section 1.7](MATHEMATICAL_FORMULATION.md#17-flrw-metric-cosmological-spacetime)), Lemaitre-Tolman-Bondi void geometries ([Section 4.3](#43-cosmological--exotic-spacetimes)), dark energy cosmologies (Quintessence, Chaplygin gas), semiclassical evaporating black holes (Vaidya), alternative scalar-tensor theories (TeVeS), and numerical 3+1 BSSN metric grids with spatial interpolation ([Section 4.4](#44-31-numerical-relativity-grids-bssn)).
- Coordinate Systems: Freedom to select coordinate gauges for a given metric to bypass coordinate singularities, optimize numerical stability across horizons, or maintain spatial conformality ([Section 4.2](#42-coordinate-systems--gauges)).
- Differential Integration Solvers: Selection of adaptive variable-step Runge-Kutta solvers (Dormand-Prince RK45, Cash-Karp 5(4) with wide stability intervals), high-order embedded formulas (Vernier 9(8) with 16 stages), implicit symplectic Gauss-Legendre integrators (orders 4 and 6) with Picard stage iterations conserving phase-space symplectic 2-forms and Killing invariants, or predictor-corrector Hermite schemes with 4th-order centered finite difference jerk evaluation and Aarseth step regulation, coupled with digital filter step controllers (Standard, PI30, PID42), infalling radial step limiters, and horizon state boundary tracking ([ARCHITECTURE.md Section 2.3](ARCHITECTURE.md#23-differential-solvers--geodesic-integrators) and [MATHEMATICAL_FORMULATION.md Section 18](MATHEMATICAL_FORMULATION.md#18-differential-solvers-symplectic-integrators--step-control-mathematics)). The integrator name selected at runtime determines the N-body integrator only (symplectic Forest-Ruth post-Newtonian scheme for names containing `Symplectic` or `Gauss`, fourth-order Runge-Kutta otherwise); the ray tracers use specialized null RK4 schemes (Schwarzschild Cartesian angular momentum conserving, Kerr-Schild Hamiltonian, planar equatorial, or exact tetrad projection; see [TECHNICAL_MANUAL.md Section 2.3](TECHNICAL_MANUAL.md#23-differential-solvers--integrators) and [Section 2.9](TECHNICAL_MANUAL.md#29-rendering-back-ends)).
- Matter & Hydrodynamic Models: Representation of matter via test particles, rigid multipolar bodies with spherical harmonic expansions ([Section 5.2](#52-spherical-harmonics--high-degree-geodesy)), 1D relativistic hydro/MHD grid evolutions with WENO5-JS, WENO5-Z, MP5, and TVD spatial reconstructions ([Section 6.1](#61-curved-spacetime-hydrodynamics)), HLL, HLLC, and HLLD approximate Riemann solvers ([MATHEMATICAL_FORMULATION.md Section 4.6](MATHEMATICAL_FORMULATION.md#46-multi-wave-approximate-riemann-solvers)), constrained transport divergence control enforcing $\nabla \cdot \mathbf{B} = 0$ ([Section 4.7](MATHEMATICAL_FORMULATION.md#47-constrained-transport-ct--solenoidal-magnetic-constraint)), relativistic ideal gases, Synge and Mathews kinetic gases, degenerate Fermi gases with low-$x$ expansions and Newton-Raphson density inversion, tabulated dense nuclear matter equations of state ([Section 6.2](#62-multi-regime-equations-of-state-eos) and [MATHEMATICAL_FORMULATION.md Section 4.9](MATHEMATICAL_FORMULATION.md#49-relativistic-equations-of-state-eos-formulations)), Tolman-Oppenheimer-Volkoff (TOV) relativistic stellar structure profiles, mass-radius curves, and stability boundaries ([Section 6.5](#65-tolman-oppenheimer-volkoff-tov-relativistic-stellar-structure--equilibrium)), Novikov-Thorne thin accretion disks, Fishbone-Moncrief relativistic thick tori, and Blandford-Znajek jet magnetospheres ([Section 6.3](#63-relativistic-accretion-disks-tori--jets)).
- Post-Newtonian Regimes: Configuration of multi-body gravitational interactions across orders (Newtonian, 1PN, 2PN, 2.5PN radiative reaction, 3PN, 3.5PN with spin-orbit and spin-spin couplings; see [Section 5.1](#51-post-newtonian-multi-body-dynamics) and [MATHEMATICAL_FORMULATION.md Section 3](MATHEMATICAL_FORMULATION.md#3-post-newtonian-pn-n-body-dynamics)).
- Radiative Transfer & Polarimetry: Execution of radiative transport ranging from direct kinematic Doppler shifting to full-Stokes polarimetric transport integrating synchrotron emission, Bremsstrahlung, Faraday rotation, and inverse Compton scattering ([MATHEMATICAL_FORMULATION.md Section 6](MATHEMATICAL_FORMULATION.md#6-polarized-radiative-transfer)).
- Colorimetry & Spectral Integration: Continuous integration over arbitrary electromagnetic spectra coupled with standard CIE 1931 observer matching functions and wide-gamut HDR tone mapping ([Section 9.3](#93-continuous-spectral-pipeline--cie-1931-integration) and [MATHEMATICAL_FORMULATION.md Section 8.3](MATHEMATICAL_FORMULATION.md#83-tone-mapping-and-color-grading)).
- Uncertainty Quantification: Real-time selection of bounded interval arithmetic (IEEE 1788 with directed division and trigonometric enclosure), Girard-reduced zonotope enclosures with $L_1 - L_\infty$ generator scoring, continuous Jacobi-Lyapunov covariance matrix propagation with cyclic Jacobi eigensystems, variational geodesic integration with 8th-order Christoffel Jacobians and symplectic determinant monitoring, generalized polynomial chaos expansions (gPCE) on Hermite and Legendre bases with Golub-Welsch quadratures, antithetic Monte Carlo covariance validation, orbital wrapping benchmarks, and metrology visualization meshes and heatmaps ([Section 7.2](#72-uncertainty-propagation-frameworks) and [MATHEMATICAL_FORMULATION.md Section 7](MATHEMATICAL_FORMULATION.md#7-uncertainty-quantification-formulation)).
- Gravitational Theories: General Relativity, Modified Newtonian Dynamics (MOND), Tensor-Vector-Scalar gravity (TeVeS), scalar-tensor $f(R)$ Chameleon models, and non-baryonic dark matter halo profiles ([Section 5.3](#53-dark-matter-halos--alternative-gravitational-theories)).
- Physical Constants & Units: SI, Planck or custom base constants ($c$, $G$, $h$, $k_B$, $N_A$, $K_e$, $K_{cd}$) with derived scale factors between simulation units and SI units ([ARCHITECTURE.md Section 2.13](ARCHITECTURE.md#213-physical-constants-engine)), display units selectable independently across 19 physical quantities ([TECHNICAL_MANUAL.md Section 2.11](TECHNICAL_MANUAL.md#211-constants-units-and-expressions)), and an expression engine evaluating arithmetic expressions with dimensional analysis over a 6D base dimension vector $[L, M, T, I, \Theta, A]$ across a 164-token unit registry ([MATHEMATICAL_FORMULATION.md Section 19](MATHEMATICAL_FORMULATION.md#19-dimensional-analysis-physical-expressions--base-vector-algebra)).
- Body Interactions: Electromagnetic, collision, thermodynamic, fragmentation and annihilation interactions between N-body bodies ([Section 5.4](#54-body-interactions--spacetime-source-bodies)).
- Computational Distribution Topology: Operation across standalone local hardware (Vulkan FP64 GPU or SIMD multi-threaded CPU), cluster master nodes with three-tier work stealing, dedicated headless worker nodes (`cluster_worker`), or hybrid participatory nodes ([ARCHITECTURE.md Section 2.14](ARCHITECTURE.md#214-distributed-compute-subsystem--heterogeneous-cluster-architecture)). Distributes five task categories across network workers: $32 \times 32$ ray-traced viewport tiles, capture studio image bands, row-partitioned post-Newtonian N-body accelerations evaluated via GPU compute (`GpuNBodyExecutor` running `nbody_accel_fp64.comp`) or CPU, Monte Carlo geodesic ensembles reduced via the Chan-Golub-LeVeque algorithm, and generalized polynomial chaos quadratures ([MATHEMATICAL_FORMULATION.md Section 10](MATHEMATICAL_FORMULATION.md#10-distributed-scheduling-load-balancing--reduction-mathematics)).

---

## 3. Relativistic Phenomenology & Physical Modeling

The simulation platform models the manifestations of Einstein's field equations, relativistic kinematics, & spacetime curvature:

### 3.1. Relativistic Kinematics & Optical Effects

- Relativistic Aberration: Anisotropic angular compression of the apparent celestial sphere towards the instantaneous direction of motion, parameterized by the observer's 3-velocity.
- Relativistic Doppler Effect: Generalized frequency shifts incorporating both longitudinal line-of-sight velocity & transverse kinematic time dilation.
- Relativistic Beaming (Doppler Boosting): Directional amplification & beaming of specific intensity proportional to $g^3$ & bolometric flux proportional to $g^4$, governing the apparent brightness distribution of high-speed matter & rotating accretion disks.
- Terrell-Penrose Rotation: Apparent visual rotation of three-dimensional extended objects traveling at ultra-relativistic velocities without visible rectilinear Lorentz contraction.
- Lorentz Contraction & Proper Time Dilation: Physical contraction of spatial intervals along the displacement vector & slowing of comobile clocks relative to asymptotic coordinate time.
- High-Lorentz Factor Regularization: Formulations preventing floating-point overflow & precision loss under extreme kinematic regimes ($\gamma \gg 10^3$).

### 3.2. Strong Gravity & Curved Spacetime Phenomena

- Gravitational Lensing: Geodesic deflection of null trajectories yielding multiple images, Einstein rings, & gravitational arcs.
- Gravitational Redshift: Energy loss experienced by photons climbing out of gravitational potential wells, evaluated via coordinate-independent contraction of four-momenta with four-velocities.
- Frame Dragging (Lense-Thirring Effect): Spacetime vorticity induced by rotating central masses, dragging inertial frames and deforming the ergosphere boundary ([MATHEMATICAL_FORMULATION.md Section 1.3](MATHEMATICAL_FORMULATION.md#13-kerr-metric-rotating-black-hole)).
- Photon Spheres & Black Hole Shadows: Determination of unstable circular photon orbits, critical impact parameters, and the central absorption shadow boundary ([MATHEMATICAL_FORMULATION.md Section 2.4](MATHEMATICAL_FORMULATION.md#24-kerr-characteristic-radii-and-horizon-quantities)).
- Event Horizon Boundary Behavior: Configurable handling when null geodesics encounter event horizon surfaces:
  - *Pure Absorption Mode*: Immediate geodesic termination with zero luminance assignment or horizon background injection ([MATHEMATICAL_FORMULATION.md Section 8.2](MATHEMATICAL_FORMULATION.md#82-step-control-termination-and-space-skipping)).
  - *Continuous Interior Propagation*: Uninterrupted integration across coordinate-regularized horizons into interior geometries towards physical singularities. The ray tracers of the renderer implement the absorption mode only: a ray is terminated with the status `HORIZON_ABSORBED` when $r \le 1.0001\,r_h$ or when the Kerr function $\Delta \le 0$ ([Technical Manual](TECHNICAL_MANUAL.md#metric-usage-in-softwarecomputeengine)).
- Multi-Source Black Hole Systems: Simultaneous gravitational deflection and event horizon absorption across binary and multiple black hole configurations superposed in Cartesian Kerr-Schild coordinates, evaluating independent spins, orientation triads, and individual primary/subsidiary accretion disks ([MATHEMATICAL_FORMULATION.md Section 1.3](MATHEMATICAL_FORMULATION.md#13-kerr-metric-rotating-black-hole)). Evaluated via GPU compute shaders using workgroup shared memory caching or CPU SIMD multi-threading.
- Relativistic Tidal Forces & Geodesic Deviation: Computation of the Riemann tidal tensor governing differential acceleration across extended bodies ([MATHEMATICAL_FORMULATION.md Section 2.3](MATHEMATICAL_FORMULATION.md#23-riemann-tensor-ricci-tensor--kretschmann-scalar)).
- Shapiro Gravitational Time Delay: Propagation delay of null signals traversing curved spacetime relative to flat Minkowski baselines ([Section 12](#12-verification-criteria--benchmark-protocols)).

---

## 4. Spacetime Catalog & Coordinate Representations

The engine includes exact analytical solutions, modified metrics, cosmological models, & numerical relativity grids:

### 4.1. Analytical Vacuum & Electrovacuum Solutions

- Minkowski Spacetime: Flat pseudo-Euclidean reference metric with signature $(-c^2, 1, 1, 1)$.
- Schwarzschild Metric: Static spherically symmetric geometry for non-rotating uncharged central masses.
- Kerr Metric: Stationary axisymmetric geometry for rotating uncharged black holes, parameterized by mass $M$ & spin $a \in [-M, M]$.
- Reissner-Nordström Metric: Static spherically symmetric geometry for charged non-rotating black holes with mass $M$ & electric charge $Q$.
- Kerr-Newman Metric: Electrovacuum solution combining mass $M$, spin parameter $a$, & net charge $Q$.
- Schwarzschild-de Sitter / Kottler Metric: Inclusion of the cosmological constant $\Lambda$, modeling background cosmological expansion in localized gravitational wells.

### 4.2. Coordinate Systems & Gauges

Metrics are formulated in multiple coordinate systems to manage gauge regularity & numerical convergence:
- Standard Schwarzschild / Boyer-Lindquist Coordinates: Asymptotically Cartesian representations highlighting global spacetime symmetries.
- Isotropic Coordinates: Spatially conformal coordinates suited for post-Newtonian multi-body coupling.
- Eddington-Finkelstein Coordinates (Ingoing/Outgoing): Coordinate-regularized representations removing metric determinant divergence at event horizons.
- Painlevé-Gullstrand Coordinates: Spatially flat slicing with a coordinate time corresponding to the proper time of observers in radial free-fall from infinity.
- Kerr-Schild Cartesian Coordinates: Horizon-regular formulation decomposing the metric into flat Minkowski spacetime & a null vector outer product ($g_{\mu\nu} = \eta_{\mu\nu} + 2H k_\mu k_\nu$), ensuring global regularity across the horizon.

### 4.3. Cosmological & Exotic Spacetimes

- FLRW Metric (Friedmann-Lemaître-Robertson-Walker): Homogeneous & isotropic expanding universe with dynamic scale factor $a(t)$, spatial curvature parameter $k \in \{-1, 0, 1\}$, & multi-component cosmological fluid equations of state.
- Morris-Thorne Traversable Wormhole: Non-singular geometry parameterized by shape function $b(r)$ & tidal potential $\Phi(r)$, supporting continuous bidirectional transit between distinct asymptotically flat universes without curvature singularities.
- Alcubierre Warp Drive Metric: Dynamic spacetime bubble generating localized contraction in the direction of motion & expansion in the rear, parameterized by bubble velocity $v_s(t)$ & hyperbolic tangent wall shaping functions.

- Lemaitre-Tolman-Bondi (LTB) Void Metric: Inhomogeneous spherically symmetric cosmological void model with dynamic density contrast, non-simultaneous big-bang times, and differential shell expansion rates ([MATHEMATICAL_FORMULATION.md Section 1.10](MATHEMATICAL_FORMULATION.md#110-lemaitre-tolman-bondi-void-metric)).
- Quintessence Dark Energy Cosmology: Dynamical scalar-field dark energy with four analytical potentials (`Exponential`, `InversePowerLaw`, `PseudoNambuGoldstone`, `Supergravity`), integrated via shooting bisection over e-fold time $N = \ln a$ ([Section 1.11](MATHEMATICAL_FORMULATION.md#111-quintessence-dark-energy-cosmology)).
- Chaplygin Gas Cosmology: Unified dark energy-matter fluid model parameterized by stiffness $A_s$ and exponent $\alpha$ with analytical density evolution $\rho_{\text{ch}}(a)$ ([Section 1.12](MATHEMATICAL_FORMULATION.md#112-chaplygin-gas-cosmology)).
- Evaporating Vaidya Black Hole: Semiclassical metric with time-dependent mass $M(v)$ driven by Hawking radiation, Kerr horizon surface gravity thermodynamics, Polyakov 2D renormalized stress tensors, and Unruh-DeWitt detector response ([Section 1.13](MATHEMATICAL_FORMULATION.md#113-evaporating-vaidya-black-hole-metric)).
- TeVeS Metric: Tensor-Vector-Scalar covariant relativistic modified gravity coupling physical metric $g_{\mu\nu}$, dynamic scalar field $\phi$, and timelike 4-vector $U^\mu$ with critical acceleration $a_0$ ([Section 1.14](MATHEMATICAL_FORMULATION.md#114-teves-tensor-vector-scalar-metric)).

The image renderer evaluates the Schwarzschild-de Sitter, FLRW, Morris-Thorne, Alcubierre, LTB, Quintessence, Chaplygin, Vaidya, and TeVeS selections through dedicated numerical solvers (`trace_extended_photon` and `trace_spherical_spacetime_dispatch`), solving their non-vanishing curvature potentials, cosmological expansion scales, and geometric throat or bubble parameters directly along the null paths ([TECHNICAL_MANUAL.md](TECHNICAL_MANUAL.md#metric-usage-in-softwarecomputeengine)).

### 4.4. 3+1 Numerical Relativity Grids (BSSN)

- Numerical 3+1 ADM/BSSN metric evolution on 3D spatial grids with conformal factor $\phi$, conformal 3-metric $\tilde{\gamma}_{ij}$, trace of extrinsic curvature $K$, trace-free extrinsic curvature $\tilde{A}_{ij}$, conformal connection functions $\tilde{\Gamma}^i$, lapse $\alpha$, and shift $\beta^i$, integrated using 4th-order spatial differencing and RK4 time stepping.
- Spatial interpolation using local tricubic B-splines and quintic Hermite temporal interpolation providing continuous metric evaluations along traversing geodesics.

### 4.5. Metric Invariants & Curvature Tensors

- Automated evaluation of Christoffel symbols of the second kind $\Gamma^\sigma_{\mu\nu}$ via closed-form analytical expressions or 8th-order centered finite difference stencils.
- Curvature tensor evaluation including the Riemann tensor $R^\rho_{\phantom{\rho}\sigma\mu\nu}$, Ricci tensor $R_{\mu\nu}$, Ricci scalar $R$, & Kretschmann invariant $K_1 = R^{\alpha\beta\gamma\delta} R_{\alpha\beta\gamma\delta}$ for physical singularity detection & Hamiltonian constraint residual validation.

---

## 5. Post-Newtonian Dynamics & Gravitational Theories

### 5.1. Post-Newtonian Multi-Body Dynamics

Relative gravitational accelerations among compact & extended bodies incorporate corrections up to order 3.5PN:
- 1PN Order: Relativistic orbital corrections, perihelion advance, & primary geodesic light deflection.
- 2PN Order: Non-linear multi-body cross-interactions & higher-order self-gravitating corrections.
- 2.5PN Order: Gravitational radiation reaction damping resulting in continuous secular loss of orbital energy & angular momentum.
- 3PN & 3.5PN Orders: Conservative 3PN terms, including the logarithmic dependence on the reference scale `r0_scale`, & 3.5PN radiation dissipation.
- Spin Couplings: Spin-orbit, spin-spin and self-spin accelerations, enabled independently of the PN order, & spin precession rates (`PostNewtonianSpinSolver`, `include/relativistic/dynamics/pn_spin_precession.hpp`).

The terms are selected by `PNOrderConfig` (`include/relativistic/dynamics/pn_orders.hpp`). `PostNewtonianSystem::update_accelerations` (`include/relativistic/dynamics/pn_nbody_system.hpp`) applies:
- the complete relative acceleration of `PostNewtonianSolver::compute_binary_relative_acceleration` to a system of exactly two bodies without central body;
- the Newtonian, 1PN (Einstein-Infeld-Hoffmann) and zonal harmonic terms only to the pairwise interactions of every other system;
- the complete relative acceleration between each body and the central body, added to the pairwise result, when a central body is present.

The quadrupole radiation quantities of [Section 5.1](#51-post-newtonian-multi-body-dynamics) are recomputed at every acceleration update.

### 5.2. Spherical Harmonics & High-Degree Geodesy

- Fully normalized associated Legendre polynomial gravitational potential expansion up to degree & order 32.
- High-precision modeling of zonal gravitational moments $J_2, J_3, J_4, \dots, J_{20}$, sectorial, & tesseral coefficients $C_{nm}, S_{nm}$ calibrated for the Solar System (EGM96, LP165, MRO110, Jupiter, & Sun models).
- Dynamic tidal Love number perturbations ($k_2, k_3$) & rotational flattening models.
- In the N-body solver, the field of each body is evaluated from its zonal coefficients $J_2$, $J_3$, $J_4$ and its reference radius. The rendered body shape uses $1 - J_2$ as axis ratio when $\lvert J_2 \rvert > 10^{-9}$.

### 5.3. Dark Matter Halos & Alternative Gravitational Theories

- Non-Baryonic Dark Matter Profiles & Halo Engine:
  - *Thirteen Analytic Density Profiles*: Navarro-Frenk-White (NFW), Hernquist, Burkert (cored), Einasto (power-law index $\alpha$), Plummer sphere, Cored Isothermal, Singular Isothermal Sphere, Dehnen ($\gamma$-cusp model), Point Mass perturber, Jaffe, Moore (steep cusp), Soliton core (Fuzzy Dark Matter / Bose-Einstein condensate), and King Beta model (cluster profile) ([MATHEMATICAL_FORMULATION.md Section 17.1](MATHEMATICAL_FORMULATION.md#171-analytic-density-profiles--enclosed-mass)).
  - *Triaxial Geometry & Orientation*: Generalized ellipsoidal radius $m^2 = x_{\text{loc}}^2 + (y_{\text{loc}}/q_y)^2 + (z_{\text{loc}}/q_z)^2$ with Tait-Bryan Euler orientation angles (yaw, pitch, roll), core softening $\epsilon$, and outer truncation radius $r_{\text{trunc}}$ ([MATHEMATICAL_FORMULATION.md Section 17.2](MATHEMATICAL_FORMULATION.md#172-triaxial-ellipsoidal-coordinates--frame-rotations)).
  - *Geodesic Lensing & False-Color Visualization*: Transverse acceleration kicks with step deflection limiters, paired with line-of-sight optical integration across three modes: column density, gravitational field strength, and circular speed squared ([MATHEMATICAL_FORMULATION.md Section 17.3](MATHEMATICAL_FORMULATION.md#173-weak-field-geodesic-deflection-kicks--step-control) and [Section 17.4](MATHEMATICAL_FORMULATION.md#174-line-of-sight-emissive-density-field--velocity-visualization)).
  - *Keplerian Orbital Motion*: Full 3D Keplerian orbits for halos around primary attractors with eccentric anomaly root-finding, periapsis, inclination, and line-of-nodes orientation ([MATHEMATICAL_FORMULATION.md Section 17.5](MATHEMATICAL_FORMULATION.md#175-halo-keplerian-orbital-dynamics)).
  - *Multi-Halo Layout & Population Generation*: Five layout patterns (Orbiting Ring, Binary Pair, Linear Chain, Cubic Lattice, Spherical Shell) and Monte Carlo subhalo population generator with differential mass and radial power-law sampling ([MATHEMATICAL_FORMULATION.md Section 17.6](MATHEMATICAL_FORMULATION.md#176-subhalo-population-statistical-generation)).
  - *Collisionless N-Body Dynamics*: Hierarchical Barnes-Hut octree spatial decomposition with quadrupole multipole expansions and symplectic leapfrog time integration ([MATHEMATICAL_FORMULATION.md Section 17.7](MATHEMATICAL_FORMULATION.md#177-collisionless-n-body-dynamics--barnes-hut-octree-multipoles)).
  - *Composite Galaxy Modeling*: Multi-component galaxy generator synthesizing exponential stellar disks, Hernquist stellar bulges, and NFW dark matter halos with dispersion kinematics and ICRF orientation transforms ([MATHEMATICAL_FORMULATION.md Section 17.8](MATHEMATICAL_FORMULATION.md#178-composite-multi-component-galaxy-model-generation)).
- Modified Newtonian Dynamics (MOND): Low-acceleration phenomenology ($a \ll a_0 \approx 1.2 \times 10^{-10} \, \text{m/s}^2$) with standard, simple, exponential, & Bekenstein interpolation functions.
- TeVeS (Tensor-Vector-Scalar Gravity): Relativistic covariant MOND formulation coupling physical metric $g_{\mu\nu}$, dynamic scalar field $\phi$, & unit timelike 4-vector field $U^\mu$.
- Scalar-Tensor $f(R)$ Gravity: Modified gravity under the Jordan & Einstein frames supporting Hu-Sawicki & Starobinsky models with thin-shell Chameleon screening mechanisms in high-density environments.

### 5.4. Body Interactions & Spacetime-Source Bodies

`InteractionConfig` (`include/relativistic/dynamics/interaction_config.hpp`) and `InteractionSolver` (`include/relativistic/dynamics/interaction_solver.hpp`) provide the following optional interactions, all disabled by default:
- Electricity: Coulomb force between charged bodies with a configurable permittivity.
- Magnetism: Dipole-dipole force with a configurable permeability, requiring a non-zero magnetic moment on both bodies.
- Collisions: Contact detection on the sum of radii, Hertzian repulsion from the cold or hot Young's modulus (selected by the critical temperature) scaled by the contact stiffness, Baumgarte-type position correction, impulse response with elastic (mean restitution multiplied by a global factor) or inelastic model, Coulomb-clamped friction, and optional spin-up of the rotation rate.
- Thermodynamics: Stefan-Boltzmann exchange between each body and an ambient temperature, scaled by the absorption factor and a coupling scale. A negative ambient temperature disables the exchange.
- Fragmentation: Integrity loss from collision energy and, optionally, tidal stress of the central source; a body whose integrity reaches zero is replaced by up to `max_fragments_per_event` fragments, unless the fragment mass is below `minimum_fragment_mass`.
- Annihilation: Removal of both bodies in contact, optionally restricted to opposite charges.

The electromagnetic accelerations are added to `PostNewtonianBody::acceleration` after each integration sub-step; `PostNewtonianSystem::update_accelerations` recomputes this field from the gravitational terms at the start of the next integrator stage. The warnings displayed for inconsistent settings are produced by `include/relativistic/dynamics/interaction_compatibility.hpp`.

A body with `is_spacetime_source` set carries a Kerr outer horizon derived from its mass and spin (clamped to $0.999$ in dimensionless units). Such bodies are integrated like other bodies, absorb ordinary bodies crossing their horizon, and merge with each other or with the central object; the merging rules are listed in [ARCHITECTURE.md](ARCHITECTURE.md#34-orchestrator-state-advance). The primary source stays fixed at the origin and alone defines the background metric. Secondary sources act on rays only in the scalar CPU tracer (deflection, horizon absorption, disk crossing, rim emission; [TECHNICAL_MANUAL.md](TECHNICAL_MANUAL.md#29-rendering-back-ends)).

Each body can carry up to four procedural surface layers (10 patterns, 5 blend modes, 7 region masks; `include/relativistic/dynamics/body_surface_layers.hpp`), or one of 14 photographic surface texture maps ([TECHNICAL_MANUAL.md Section 2.9](TECHNICAL_MANUAL.md#29-rendering-back-ends)).

The body management subsystem is structured into seven functional tabs:
- *Object Catalog*: Manages ordinary N-body celestial bodies in flat list or hierarchical tree views. Includes continuous physical attribute range filtering across five properties (mass, radius, temperature, eccentricity, distance from center) with population statistics and breakdown badges. Supports multi-selection batch parameter staging with absolute overrides and relative delta/scaling transformations (mass scaling, radius scaling, position shifts, velocity deltas, speed factors, temperature deltas, uniform inclination alignments).
- *Source Catalog*: Manages independent Kerr black holes (spacetime sources) with their own horizons, spin vectors, charges, and subsidiary accretion disks, alongside the primary central metric source. Evaluates safe separation boundaries, mutual horizon overlaps, and binary system metrics (mass ratio $q$, symmetric mass ratio $\eta$, separation in $r_g$, spin-orbit misalignment $\theta_{LS}$, mutual spin angle $\theta_{S1, S2}$, Peters 1964 gravitational wave coalescence timescale).
- *Create Object*: Single-body drafting with intelligent correlated randomizer defaults, starting templates (Sun, Earth, Moon, Jupiter, Mars, Neutron Star, Supermassive BH, Stellar BH, Test Particle), comparator against system scales, and the *Multi-Body Geometric Pattern Generator* synthesizing annular asteroid belts, coplanar debris swarms, and 3D spherical test formations (Plummer polytropic clusters, uniform spheres, Fibonacci lattice shells, infalling/expanding blast waves) across 10 archetype presets with Kirkwood gap simulation and collision-avoidance safeguards ([MATHEMATICAL_FORMULATION.md Section 11.5](MATHEMATICAL_FORMULATION.md#115-multi-body-geometric-pattern-formations)).
- *Create Source*: Spawns independent black holes based on observed astrophysical templates (Sagittarius A*, M87*, Cygnus X-1, GW150914 remnant, intermediate-mass black holes, companion Kerr sources) with automatic placement outside horizon overlap zones.
- *System Dynamics*: Telemetry for system mass, center-of-mass barycenter, total linear and angular momentum, mechanical energy, quadrupole gravitational wave emission ($P_{\text{GW}}$, dominant $f_{\text{gw}}$, strain $h$ at 10 kpc), center-of-mass velocity shifting, and scoped bulk operations across four target scopes (all bodies, ordinary objects only, independent black holes only, or specific object type categories).
- *Interactions*: Comprehensive configuration of electromagnetic forces, Hertzian contact mechanics with Baumgarte stabilization, radiative thermodynamics, fragmentation, and matter-antimatter annihilation, coupled with a physical coupling consistency dashboard.
- *Randomizer Controls*: Parameter intervals and distribution laws (Uniform, Log-Uniform, Gaussian, Power-Law) governing procedural generation.
- Pre-built System Presets: Real Solar System (Sun through Neptune, Pluto, Earth-Moon, Galilean moons: Io, Europa, Ganymede, Callisto, Titan), Inner Solar System, Jovian sub-system, and Earth-Moon high-precision frame with automatic physical unit scaling through the [ConstantsEngine](ARCHITECTURE.md#213-physical-constants-engine).
- Gravitational Hierarchy Tree: Automatic identification of dominant parent attractors based on Hill sphere and tidal dominance analysis ([MATHEMATICAL_FORMULATION.md Section 11.4](MATHEMATICAL_FORMULATION.md#114-gravitational-hierarchy--hill-sphere-stability)), displaying hierarchical parent-satellite groupings with Hill sphere occupancy percentages and Kozai-Lidov resonance detection ($i_{\text{rel}} \in [39.23^\circ, 140.77^\circ]$).
- Local Frenet-Serret RTN Maneuver Injector: Directional velocity vector impulses decomposed along radial ($\hat{\mathbf{R}}$), tangential prograde ($\hat{\mathbf{T}}$), and normal out-of-plane ($\hat{\mathbf{N}}$) axes relative to any designated reference body, with real-time pre-burn orbit prediction diagnostics (eccentricity, semi-major axis, periapsis, apoapsis, plunge warnings), Hohmann transfer departures, and isokinetic plane-change burns ([MATHEMATICAL_FORMULATION.md Section 11.1](MATHEMATICAL_FORMULATION.md#111-frenet-serret-local-rtn-frame) and [Section 11.6](MATHEMATICAL_FORMULATION.md#116-orbital-maneuvers--rtn-frame-impulses)).
- Relative Orbital Plane Alignment Tool: Real-time measurement and reorientation of mutual orbital plane inclination ($i_{\text{rel}}$), line of nodes ($\Omega_{\text{rel}}$), and argument of periapsis ($\omega_{\text{rel}}$) relative to any reference attractor, with quick coplanar (0 deg), polar (90 deg), or retrograde (180 deg) snaps ([MATHEMATICAL_FORMULATION.md Section 11.2](MATHEMATICAL_FORMULATION.md#112-relative-orbital-plane-alignment--rotations)).

---

## 6. Relativistic Hydrodynamics (GRHD/GRMHD) & Accretion Systems

### 6.1. Curved Spacetime Hydrodynamics

- Conservative 3+1 formulation of baryon mass conservation $\nabla_\mu (\rho u^\mu) = 0$ & energy-momentum conservation $\nabla_\mu T^{\mu\nu} = 0$.
- High-Resolution Shock-Capturing (HRSC): Approximate Riemann solvers (HLL, HLLC with contact wave restoration, HLLD for magnetohydrodynamics) coupled with 5th-order spatial reconstruction (WENO5-JS, WENO5-Z, MP5).
- Constrained Transport (CT): Staggered face-centered magnetic field integration enforcing the solenoidal constraint $\nabla \cdot \mathbf{B} = 0$ to machine precision.

### 6.2. Multi-Regime Equations of State (EOS)

- Relativistic Ideal Gas (Gamma-Law): $P = (\Gamma - 1)\rho\epsilon$ with adiabatic index $\Gamma \in (1, 5/3]$.
- Synge / Mathews Relativistic Gas: Exact kinetic models for relativistic monoatomic gases across arbitrary temperatures.
- Relativistic Degenerate Fermi Gas: Complete integration of Fermi-Dirac degeneracy pressure for relativistic electrons, neutrons, & protons.
- Polytropic & Piecewise Polytropic EOS: Polytropic ($P = K \rho^\Gamma$) and 4-piece continuous piecewise polytropic models calibrated for dense nuclear matter.
- Tabulated Nuclear Matter (Tabulated EOS): 3D interpolation over density $\log_{10}\rho$, temperature $\log_{10}T$, and electron fraction $Y_e$ supporting SFHo, Shen, LS220, SLy4, and APR4 models, serializable to the container format described in [FILE_FORMATS.md](FILE_FORMATS.md#3-hierarchical-data-format-h5).

### 6.3. Relativistic Accretion Disks, Tori & Jets

- Procedural Thin Disk: Geometrically thin, equatorial Keplerian accretion flow with procedural rings, logarithmic spiral density waves, turbulence, and differential rotation shearing ([MATHEMATICAL_FORMULATION.md Section 8.2](MATHEMATICAL_FORMULATION.md#82-step-control-termination-and-space-skipping)).
- Novikov-Thorne Thin Disk: Radiatively efficient, geometrically thin equatorial accretion disk around Kerr black holes with Page-Thorne analytical relativistic flux integration vanishing at the ISCO, radiative efficiency diagnostics, and accretion rate scaling ([MATHEMATICAL_FORMULATION.md Section 5.1](MATHEMATICAL_FORMULATION.md#51-novikov-thorne-thin-disk-profile)).
- Fishbone-Moncrief Magnetized Thick Torus: Relativistic stationary thick torus with constant specific angular momentum $l = -u_\phi / u_t$, polytropic equation of state, barotropic pressure equilibrium, and optical depth integration ([Section 5.2](MATHEMATICAL_FORMULATION.md#52-fishbone-moncrief-magnetized-thick-torus)).
- Blandford-Znajek Jet Magnetosphere: Force-free magnetosphere threading the Kerr event horizon, modeling poloidal magnetic flux, Znajek horizon boundary currents, Poynting flux power extraction, and synchrotron emission ([Section 5.3](MATHEMATICAL_FORMULATION.md#53-blandford-znajek-jet-magnetosphere)).

### 6.4. Radiative Processes & Local Emission

- Relativistic Synchrotron Radiation: Thermal & non-thermal power-law emission & self-absorption coefficients from relativistic electrons in magnetic fields, including analytical Gamma function integrals for electron distributions $N(\gamma) \propto \gamma^{-p}$, Petrosian-Mahadevan thermal emissivity profiles, and Faraday rotation ($\rho_V$) and conversion ($\rho_Q$) coefficients ([MATHEMATICAL_FORMULATION.md Section 6.4](MATHEMATICAL_FORMULATION.md#64-radiative-processes--coefficients)).
- Relativistic Thermal Bremsstrahlung: Electron-ion free-free radiation incorporating thermal Gaunt factor evaluations with modified Bessel function $K_0(u/2)$ and relativistic thermal corrections $(1 + 4.4 \times 10^{-10} T_e)$ ([MATHEMATICAL_FORMULATION.md Section 6.4](MATHEMATICAL_FORMULATION.md#64-radiative-processes--coefficients)).
- Inverse Compton Scattering: Monte Carlo photon packet scattering across relativistic thermal electron populations using the exact Klein-Nishina differential cross section with low-energy series expansions ($x < 10^{-4}$), evaluation of Compton $y$-parameters ($y = 4\Theta_e(1 + 4\Theta_e)\tau_{\text{eff}}$), and frame boost transformations ([MATHEMATICAL_FORMULATION.md Section 6.6](MATHEMATICAL_FORMULATION.md#66-inverse-compton-scattering--klein-nishina-cross-section)).
- Maxwell-Jüttner Electron Distribution: Relativistic thermal velocity distribution sampling using a 4-regime decomposition algorithm across temperature intervals with rational polynomial Bessel functions ($K_0, K_1, K_2, K_3$), mean Lorentz factors $\langle \gamma \rangle = K_1/K_2 + 3\Theta_e$, and Gaussian Doppler line-broadening kernels ([MATHEMATICAL_FORMULATION.md Section 6.5](MATHEMATICAL_FORMULATION.md#65-maxwell-jüttner-relativistic-thermal-distribution)):
  $$f(\gamma) = \frac{\gamma \sqrt{\gamma^2 - 1}}{\Theta_e K_2(1/\Theta_e)} \exp\left(-\frac{\gamma}{\Theta_e}\right), \quad \Theta_e = \frac{k_B T_e}{m_e c^2}$$

### 6.5. Tolman-Oppenheimer-Volkoff (TOV) Relativistic Stellar Structure & Equilibrium

- Coupled Differential System: Integrates stellar interior equations $(dm/dr, dm_b/dr, dp/dr, d\Phi/dr)$ using 4th-order Runge-Kutta stepping initialized via quadratic Taylor series expansions at $r_0 = 1.0\,\text{m}$, with adaptive pressure scale-height regulation $\Delta r_{\text{opt}} = \operatorname{clamp}(0.05 h_p, \Delta r_{\min}, \Delta r_{\max})$ where $h_p = p / |dp/dr|$ and bounds $[\Delta r_{\min}, \Delta r_{\max}] = [10^{-4}, 20000.0]\,\text{m}$ ([MATHEMATICAL_FORMULATION.md Section 4.8](MATHEMATICAL_FORMULATION.md#48-tolman-oppenheimer-volkoff-tov-stellar-structure--redshift-formulation)).
- Surface Root-Finding & Exterior Metric Potential Matching: Locates stellar surface radius $R$ via linear sub-step interpolation upon detecting $p_{n+1} \le 0$, evaluating total gravitational mass $M$, baryonic mass $M_b$, compactness $C = GM / (c^2 R)$, surface gravitational redshift $z_s = (1 - 2C)^{-1/2} - 1$, and central redshift $z_c = e^{-\Phi(0)} - 1$ after matching interior potential $\Phi(R)$ to the exterior Schwarzschild metric $\Phi_{\text{ext}}(R) = \frac{1}{2}\ln(1 - 2GM/(c^2 R))$.
- Mass-Radius Relation & Stability Boundary: Evaluates full equilibrium sequences $M(R)$ by sweeping central density $\log_{10}\rho_c$, identifying maximum stable gravitational mass $M_{\max}$, radius at maximum mass, and canonical $1.4 M_\odot$ neutron star radius while detecting radial instability boundaries ($\partial M / \partial \rho_c \le 0$).

### 6.6. Conservative-to-Primitive Inversions & Divergence-Free Magnetohydrodynamics

- High-Precision Numerical Inversions: `Con2PrimSolver` executes bracketed 1D Newton-Raphson iterations on pressure $p$ for relativistic fluids and scalar $z = \rho h W^2$ for magnetohydrodynamics, with bisection safeguards handling relativistic shocks ([Technical Manual Section 2.5](TECHNICAL_MANUAL.md#25-relativistic-hydrodynamics-grhdgrmhd--stellar-physics)).
- Staggered Induction Field Evolution: `ConstrainedTransport2D` evaluates corner electric field curls to preserve $\nabla \cdot \mathbf{B} = 0$ to machine precision across cell interfaces.

---

## 7. Uncertainty Quantification & Error Propagation

The engine provides a unified framework to quantify truncation errors, parametric sensitivity, & stochastic dispersion across all dynamical variables:

### 7.1. Target Quantities Subject to Uncertainty

- Initial phase-space coordinates & velocities of particles & extended bodies.
- Energy-momentum tensors $T^{\mu\nu}$ & fluid state primitives.
- Spacetime parameters (mass $M$, spin parameter $a$, net charge $Q$, cosmological constant $\Lambda$).
- Fundamental physical constants ($G$, $c$).
- Numerical integration residuals & spatial truncation errors.

### 7.2. Uncertainty Propagation Frameworks

- Bounded Interval Arithmetic: Strict lower & upper interval bounds $[\underline{x}, \bar{x}]$ adhering to the IEEE 1788 standard, supporting directed division with zero-inclusion branches, integer power parity handling, and transcendental functions (`sqrt`, `exp`, `log`, `sin`, `cos` with critical period detection; [MATHEMATICAL_FORMULATION.md Section 7.1](MATHEMATICAL_FORMULATION.md#71-interval-arithmetic-ieee-1788)).
- Girard-Reduced Zonotopes: Symmetric affine generator polytopes $\mathcal{Z} = \mathbf{c} \oplus \sum \alpha_i \mathbf{g}_i$ mitigating wrapping effects during extended orbital integrations via $L_1 - L_\infty$ generator sorting and axis-aligned box condensation ([Section 7.2](MATHEMATICAL_FORMULATION.md#72-zonotope-enclosure--girard-order-reduction)).
- Continuous Jacobi-Lyapunov Covariance Propagation: Simultaneous integration of the 8D phase state $(\mathbf{x}, \mathbf{p})$, variational Jacobian transition matrices, & the continuous Lyapunov covariance ODE:
  $$\frac{d\mathbf{\Sigma}}{d\lambda} = \mathbf{J}\mathbf{\Sigma} + \mathbf{\Sigma}\mathbf{J}^T + \mathbf{Q}$$
  using explicit Euler or Runge-Kutta 4th-order stages with intermediate symmetrization, cyclic Jacobi eigensystem decomposition, and confidence hypervolume calculation ([Section 7.3](MATHEMATICAL_FORMULATION.md#73-lyapunov-covariance-matrix-differential-propagation)).
- Variational Geodesic Integrator: Simultaneous RK4 integration of 8D phase states $(\mathbf{x}, \mathbf{p})$, state covariance matrix $\mathbf{\Sigma}$, and variational transition matrix $\mathbf{\Phi}$ driven by an exact $8 \times 8$ Jacobian evaluated via 8th-order centered finite difference derivatives of Christoffel symbols $\partial_\nu \Gamma^\mu_{\alpha\beta}$, with Gaussian elimination determinant checks for phase volume preservation ([Section 7.4](MATHEMATICAL_FORMULATION.md#74-variational-geodesic-equations--christoffel-derivatives)).
- Generalized Polynomial Chaos Expansion (gPCE): Orthogonal polynomial projections on probabilists' Hermite bases (Gaussian variables) and Legendre bases (uniform variables) with total degree truncation. Quadrature nodes and weights are generated via the Golub-Welsch algorithm using symmetric tridiagonal QL iterations. Statistical moments (mean, variance, skewness, kurtosis) and quantile confidence intervals are extracted via the Beasley-Springer-Moro inverse normal CDF approximation ([Section 7.5](MATHEMATICAL_FORMULATION.md#75-generalized-polynomial-chaos-expansion--numerical-quadratures)).
- Antithetic Monte Carlo Ensemble Validation: Thread-parallel generation of whitened antithetic sample pairs matching analytical initial covariance, RK45 adaptive propagation across worker threads, and semi-axes relative error validation ([Section 7.6](MATHEMATICAL_FORMULATION.md#76-antithetic-monte-carlo-covariance-validation)).
- Orbital Wrapping Benchmark: Quantitative evaluation of interval box wrapping versus zonotope Girard reduction under rotational orbital dynamics ([Section 7.7](MATHEMATICAL_FORMULATION.md#77-metrology--spatial-uncertainty-visualization)).
- Unified Uncertainty Algebra: Generic quantity abstraction `UncertainQuantity<T, Method>` providing uniform operator overloading across interval, zonotope, covariance, and polynomial chaos representations.

### 7.3. Metrology & Visual Representation

- 3D Covariance Ellipsoids: Real-time generation of $1\sigma, 2\sigma, 3\sigma$ iso-probability confidence meshes with analytical normals along particle trajectories ([MATHEMATICAL_FORMULATION.md Section 7.7](MATHEMATICAL_FORMULATION.md#77-metrology--spatial-uncertainty-visualization)).
- 2D/3D Probability Density Heatmaps: Spatial projection of positional probability distributions around photon rings, horizons, and trajectories evaluated via 2D bivariate Gaussian inverse covariance Mahalanobis metrics or empirical 2D histogram grids ([Section 7.7](MATHEMATICAL_FORMULATION.md#77-metrology--spatial-uncertainty-visualization)).
- Spectral & Temporal Quantile Bands: Real-time visualization of confidence intervals ($1\sigma, 2\sigma, 3\sigma$) on spectral radiance curves, bolometric light curves, and gravitational wave polarizations ($h_+, h_\times$) extracted from polynomial chaos expansions. The shaded band of `SpectrographWindow` is a fixed 8 % of the radiance and is not derived from the propagation frameworks of [Section 7.2](#72-uncertainty-propagation-frameworks).

---

## 8. Observer Kinematics & Coordinate Transport

### 8.1. Observer Definition & Comobile Orthonormal Tetrads

An observer is defined by a 4-position $x^\mu(\tau)$, a normalized timelike 4-velocity $u^\mu = dx^\mu / d\tau$ ($u_\mu u^\mu = -c^2$), & an orthonormal comobile tetrad $\{e^\mu_{(0)}, e^\mu_{(1)}, e^\mu_{(2)}, e^\mu_{(3)}\}$ where $e^\mu_{(0)} = u^\mu / c$ & $e^\mu_{(a)} e_{\mu (b)} = \eta_{ab}$.

The observer operates across six distinct navigation modes:
- Kinematic Decoupled Observer (Free-Fly 6-DOF, Mode 0): Massless point observer following coordinate directions, unaffected by inertial or tidal forces.
- Orbit Center Observer (Mode 1): Polar and azimuthal navigation on a sphere centered at the origin or a targeted object.
- Spherical Observer (Boyer-Lindquist, Mode 2): Radial, colatitudinal, and azimuthal coordinate traversal with gaze locked to the origin.
- Relativistic Rocket Observer (6-DOF Dynamic Vehicle, Mode 3): Massive test vehicle governed by relativistic propulsion equations:
  $$\frac{du^\mu}{d\tau} + \Gamma^\mu_{\alpha\beta} u^\alpha u^\beta = a^\mu_{\text{proper}}$$
  subject to proper thrust, fuel consumption, inertia, & local gravitational curvature gradients.

The interactive Rocket Thrust mode (`InteractiveCameraController::update_rocket_mode`, `include/relativistic/ui/interactive_camera_controller.hpp`) implements a reduced model: thrust accelerations along the camera axes with a magnitude cap of `max_proper_acceleration`, a Newtonian gravitational deceleration $M/r^2$ toward the origin, and explicit Euler integration of velocity and position while the simulation clock runs. Fuel consumption, inertia and curvature gradients are not modeled.
- Surface Walk Observer (Mode 4): Bound surface walker exploring celestial bodies with local surface adherence, gravity (automatic or manual), jumping, crouching, head bobbing, and surface rotation tracking (`Observer::SurfaceWalker`, `include/relativistic/observer/surface_walker.hpp`). The walker is dynamically constrained to the surface of the selected celestial body, automatically scaling walking speed, step length, and eye height with body dimensions.
- Planet Orbit Observer (Mode 5): Simplified orbital navigation on a sphere around a target celestial body with altitude zooming, mouse dragging, and surface collision avoidance without gravitational falling (`Observer::PlanetOrbit`, `include/relativistic/observer/planet_orbit.hpp`).
- Secondary Observer Viewports: Independent concurrent observer viewports (`SecondaryViewWindow`, `include/relativistic/ui/secondary_view_window.hpp`) executing native FP64 raytracing across four operational roles:
  - *Free Observer*: Unconstrained orbital camera around any chosen physical target.
  - *Comoving Free-Fall Probe*: Cockpit viewpoint of a test particle in radial free-fall from infinity ($v_{\text{ff}} = \sqrt{\min(2M/r, 0.999^2)}$) with inward/outward toggle and velocity boost 4-vector $\mathbf{\beta}$, inducing relativistic aberration and Doppler shift ([MATHEMATICAL_FORMULATION.md Section 16.2](MATHEMATICAL_FORMULATION.md#162-comoving-free-fall-probe-kinematics--velocity-boost-4-vector)).
  - *Asymptotic Global Camera*: Quasi-orthogonal observer positioned at $100M$ to $20000M$ with auto-calibrated telephoto field of view $\theta_{\text{FOV}} = 2\arctan(H / D)$ ([Section 16.3](MATHEMATICAL_FORMULATION.md#163-asymptotic-telephoto-projection--quasi-orthogonal-rays)).
  - *Equirectangular 360 Panorama*: Full-sky $4\pi$ steradian spherical projection.
- Gamepad Flight Controller: Full analog flight controls supporting Xbox, Bluetooth, and USB gamepads with quadratic deadzone filtering on left stick translation ($d_{\text{LS}} = 0.14$), right stick look inclination ($d_{\text{RS}} = 0.14$), and trigger elevation axes ($d_{\text{trig}} = 0.08$), bumper roll rotation, 10 directional virtual buttons (indices 15-24) triggered at $0.40$ axis threshold, and rebindable button chords supporting tap durations ($\le 0.25\,\text{s}$) and double-tap windows ($\le 0.30\,\text{s}$), alongside four configurable arrow key operational modes (`Movement`, `LookPitchYaw`, `LookPitchRoll`, `LookRollYaw`; [MATHEMATICAL_FORMULATION.md Section 8.6](MATHEMATICAL_FORMULATION.md#86-gamepad-analog-transfer-functions--observer-kinematic-integration)).

### 8.2. Tetrad Transport Formulations

- Parallel Transport: $\nabla_u e^\mu_{(i)} = 0$, preserving spatial axis orientation along geodesic free-fall lines.
- Fermi-Walker Transport: Applied to accelerating ($a^\mu = \nabla_u u^\mu \neq 0$) & rotating observers:
  $$\frac{D_{\text{FW}} e^\mu_{(i)}}{d\tau} = \nabla_u e^\mu_{(i)} + \frac{1}{c^2} \left( a^\mu u_\nu - u^\mu a_\nu \right) e^\nu_{(i)} = 0$$
  capturing Thomas precession, geodetic (de Sitter) precession, & Lense-Thirring frame-dragging precession.

The interactive camera does not transport a tetrad: its orientation is stored as pitch, yaw and roll angles, and the tetrad passed to the renderer is rebuilt from these angles at every frame (`SimulationOrchestrator::build_gpu_push_constants`). The exact tracer builds a ZAMO tetrad of the selected metric at the observer position.

### 8.3. Optical Projections & Field of View

To accommodate optical aberration and wide-angle observation, the engine implements eight projection geometries:
- Standard Perspective (Pinhole): Planar perspective projection with focal length scaling.
- Aberration-Compensated Auto-Zoom: Dynamic focal length scaling compensating for forward relativistic beaming compression.
- Stereographic Conformal Fisheye: Conformal azimuthal mapping preserving local angles.
- Equirectangular $360^\circ$ Panorama: Full $4\pi$ steradian spherical projection.
- Equidistant Fisheye: Azimuthal equidistant projection preserving radial angular distances.
- Orthographic Fisheye: Hemispherical orthographic projection.
- Panini Cylindrical: Cylindrical perspective projection maintaining vertical straight lines.
- Hammer-Aitoff: Equal-area all-sky projection mapping the entire celestial sphere.

---

## 9. Polarized Radiative Transfer & Spectral Pipeline

### 9.1. Backward Null Geodesic Raytracing

- For each pixel at coordinate time $t_{\text{obs}}$, a null 4-momentum $p^\mu$ is initialized via the observer's local tetrad:
  $$p^\mu = e^\mu_{(0)} + n^{(1)} e^\mu_{(1)} + n^{(2)} e^\mu_{(2)} + n^{(3)} e^\mu_{(3)}, \quad n^{(i)} n_{(i)} = 1, \quad p_\mu p^\mu = 0$$
- Backward temporal integration ($d\lambda < 0$) continues until intersecting an emitting volume, traversing an absorbing horizon, or escaping to asymptotic infinity.

### 9.2. Polarized Transport Integration (Delano Method)

Transport of the Stokes vector $\mathbf{S} = (I, Q, U, V)^T$ is integrated along ray segments via Delano's analytical matrix exponential method (`step_delano_analytical`), employing matrix 1-norm scaling and squaring ($m = \lceil \log_2(2\mu_K) \rceil$), 12th-order Taylor series expansions of $\Psi(A)$, and squaring recurrences to ensure numerical stability in the presence of extreme optical depths, Faraday rotation ($\rho_V$), and Faraday conversion ($\rho_Q$), with RK4 fallbacks and hybrid thermal/non-thermal segment integration ([MATHEMATICAL_FORMULATION.md Section 6.3](MATHEMATICAL_FORMULATION.md#63-polarized-transfer-system--delano-matrix-exponential-integration)).

### 9.3. Continuous Spectral Pipeline & CIE 1931 Integration

- Continuous Spectral Discretization: `ContinuousSpectrum` provides 4096 logarithmic wavelength bins spanning $10^{-14}\,\text{m}$ (gamma rays) to $10^3\,\text{m}$ (radio waves) with Planck blackbody, synchrotron power-law, and monochromatic line emission generators, frequency beaming transformations with $g^5 I_\lambda$ scaling, and linear/logarithmic trapezoidal band integrations ([MATHEMATICAL_FORMULATION.md Section 6.7](MATHEMATICAL_FORMULATION.md#67-continuous-spectral-pipeline--cie-1931-integration)).
- Frequency Shifting & Beaming: Local emission shifts via $\lambda_{\text{obs}} = \lambda_{\text{emit}} / g$ and specific intensity beams via $g^3$ (bolometric flux via $g^4$, wavelength intensity via $g^5$).
- Colorimetry & Tristimulus Integration: `CIE1931Observer` convolves continuous spectral radiances against analytical tri-Gaussian CIE 1931 color matching functions $\bar{x}(\lambda), \bar{y}(\lambda), \bar{z}(\lambda)$ over 380 to 780 nm to obtain tristimulus values $(X, Y, Z)$, mapped to linear sRGB and gamma-encoded sRGB ([MATHEMATICAL_FORMULATION.md Section 6.7](MATHEMATICAL_FORMULATION.md#67-continuous-spectral-pipeline--cie-1931-integration)).
- High Dynamic Range (HDR) Tone Mapping: `Tonemapper` provides ACES filmic curve mapping, logarithmic HDR mapping across $[10^{-12}, 10^{20}]\,\text{W/m}^2/\text{sr}$, extended Reinhard mapping with maximum white parameter $L_{\text{white}} = 10.0$, and EV exposure scaling ([MATHEMATICAL_FORMULATION.md Section 6.8](MATHEMATICAL_FORMULATION.md#68-high-dynamic-range-tone-mapping-operators)).
- Carter Constant Classification & Ray Probing: `CarterPhaseClassifier` pre-classifies Kerr null geodesics via depressed quartic root extraction into certain absorption, certain escape, and integration required; `GeodesicRayProbe` evaluates exact null geodesics under screen pixels resolving constants of motion ($E, L_z, \xi, \mathcal{Q}, \eta, b$), sub-step disk crossings, observed temperatures, image orders, and 3D deflection angles ([MATHEMATICAL_FORMULATION.md Section 6.9](MATHEMATICAL_FORMULATION.md#69-carter-constant-geodesic-classification--analytical-turning-points) and [Section 6.10](MATHEMATICAL_FORMULATION.md#610-relativistic-ray-probe-diagnostics--image-orders)).
- Relativistic Kinematic Optics: `TerrellPenroseOptics` evaluates forward/backward relativistic aberration for arbitrary 3-velocity $\boldsymbol{\beta}$, apparent visual Terrell rotation angle $\arcsin(\beta)$, and moving sphere analytical spacetime ray intersections with retarded time solving ([MATHEMATICAL_FORMULATION.md Section 6.11](MATHEMATICAL_FORMULATION.md#611-terrell-penrose-relativistic-aberration--moving-sphere-intersections)).

The viewport renderer implements a subset of this pipeline: the disk color is the blackbody spectrum at the Doppler-shifted temperature integrated against the CIE 1931 functions with 24 samples between 380 and 780 nm, Stokes transport is not evaluated, and four tone mapping operators (linear, ACES approximation, logarithmic, extended Reinhard) are followed by CPU color grading ([MATHEMATICAL_FORMULATION.md Section 8.3](MATHEMATICAL_FORMULATION.md#83-tone-mapping-and-color-grading)).

### 9.4. Celestial Bodies, Sky & Execution Paths

- Bodies: Ray-traced ellipsoids (oblate, rigid sphere, prolate, triaxial) with 27 surface texture modes (including 14 photographic textures for Earth, Sun, Mercury, Venus Atmosphere, Venus Radar, Mars, Jupiter, Saturn, Uranus, Neptune, Ceres, Eris, Haumea, and Makemake with 1K/2K resolution and Day/Night/Automatic twilight terminator blending), up to four procedural texture layers, atmospheric rim scattering, Doppler boosting ($g^3$), gravitational redshift attenuation, three levels of detail (full, simplified, flat dot), and an illumination engine featuring 7 light source modes (central source, camera headlight, fixed direction, fixed point, nearest emissive body, specific body, unlit), distance attenuation, terminator softness, specular highlights, inter-body shadow casting, and inter-body emission lighting ([MATHEMATICAL_FORMULATION.md Section 8.4](MATHEMATICAL_FORMULATION.md#84-celestial-body-shading)).
- Multiple Kerr Spacetime Sources: Independent black holes with individual Kerr spin parameters and accretion disks rendered via multi-source Kerr-Schild metric superposition, evaluating simultaneous horizon entries and disk crossings without coordinate singularities.
- Sky: Procedural multi-layer celestial skybox evaluated via equal-area spherical voxel partitioning ($N_{\text{grid}} \approx 14\sqrt{\rho_{\text{stars}} Q_{\text{eff}}}$), generating deterministic celestial environments along escaping null geodesics. Features 8 analytical star point spread function models (Gaussian, Lorentzian, Moffat, Airy diffraction with Chebyshev rational $J_1(x)$ polynomial approximation, HardDisc, DiffractionSpikes with 4/6/8 spikes, PinpointDirac with concentration control, and CinematicBloom with secondary wide-angle exponential halos), 6 star spatial placement density laws (UniformEqualArea, GalacticDisk with exponential vertical decay, FractalClustered via 3D FBM noise, DualPopulation bimodal disk-halo mixture, SpiralArmDensity with logarithmic arms, and ConstellationAssociations via 3D spatial hashing), 4 parameter sampling distributions (Uniform, PowerLaw/Pareto, Exponential, LogNormal), blackbody chromaticity lookup via Wien displacement temperature intervals, multi-layer FBM nebulae with divergence-free 3D curl-noise advection and ionization emission, open and globular star clusters governed by Plummer potential profiles with individual member star sampling via cumulative mass inversion, interstellar dust extinction lanes governed by Beer-Lambert exponential absorption, deep-field background galaxies across 6 morphologies (Grand Design Spiral, Flocculent Spiral, Barred Spiral, Giant Elliptical with de Vaucouleurs Sersic profile, Lenticular with resonant dust rings, Ring Galaxy) with inclined near-side dust extinction, quasars with collimated relativistic synchrotron jets and Doppler boosting ($\delta^3$), rotating magnetic dipole pulsars with lighthouse sweep beams and toroidal pulsar wind nebulae (PWN), supernova remnant shells with filamentary turbulence and reverse-shock interior glow, and an antialiased 3D spherical coordinate grid supporting major circles, minor circles, degree tick marks, polar convergence singularity tapering, and 5 cell fill modes (None, Checkerboard2Color, CyclicPalette, CheckerboardPattern, RandomHashFill) indexing an editable 16-color palette ([MATHEMATICAL_FORMULATION.md Section 12](MATHEMATICAL_FORMULATION.md#12-procedural-skybox-mathematics)), accompanied by equirectangular panorama image fallback (`include/relativistic/optics/textures/sky_panorama_catalog.hpp`).
- Lighting Engine: Comprehensive direct illumination and inter-body radiative transfer subsystem supporting 7 light source modes: CentralSource (0, fixed at origin), CameraHeadlight (1, co-located with observer), FixedDirection (2, distant celestial key light with spherical horizontal azimuth $\phi_L \in [-180^\circ, 180^\circ]$ and vertical elevation $\theta_L \in [-90^\circ, 90^\circ]$), FixedPoint (3, localized point light at arbitrary Cartesian coordinates), NearestEmissiveBody (4, dynamic emitter search across active catalog bodies with intrinsic emission $I_{\text{em}} > 10^{-3}$), SpecificBody (5, assigned emitting catalog body with primary color modulation), and Unlit (6, uniform shading without direct light or shadows). Evaluates distance attenuation under either None (constant irradiance) or InverseSquare ($\mathcal{E}(d) = I_{\text{light}} \min((d_{\text{ref}}/d)^2, 64.0)$), adjustable wrapped diffuse terminator softness $s \in [0.0, 1.0]$, Blinn-Phong specular reflections with microfacet roughness exponent $n_{\text{spec}} = 4 + 80(1 - \varrho)$, shadow occlusion rays tested against body bounding ellipsoids and central horizon spheres, and multi-source inter-body emission lighting accumulating radiant flux from luminous bodies with inverse-square falloff clamped to $16.0\times$ maximum gain ([MATHEMATICAL_FORMULATION.md Section 8.4](MATHEMATICAL_FORMULATION.md#84-celestial-body-shading)). Execution complexity is governed by 3 detail presets (`Minimal`, `Reduced`, `Full`), with configurable integer caps on tested shadow casters (`body_shadow_caster_limit`, 1 to 512) and active radiant emitters (`body_emission_source_limit`, 1 to 512). Six standardized scene lighting presets (`Stellar Default`, `Headlight`, `Studio Key`, `Unlit`, `Dramatic`, `Reset`) provide quick configuration. Evaluated identically in GLSL compute shaders (`shaders/geodesic_tracer_fp64.comp`) and C++ CPU software pipelines (`SoftwareComputeEngine::evaluate_3d_bodies`).
- Execution paths: Vulkan compute path in native FP64 for six metric identifiers, CPU SIMD and scalar paths in double and single precision, with the selection rules of [ARCHITECTURE.md](ARCHITECTURE.md#29-render-pipeline).
- Presentation: Schematic projection of bodies, trails, orbit predictions and vectors without ray tracing, an overlay of this projection on the ray-traced image, independent auxiliary observer viewports with 3D lensed beacon markers and comoving free-fall probes ([ARCHITECTURE.md Section 2.12](ARCHITECTURE.md#212-schematic-view-and-hud), [MATHEMATICAL_FORMULATION.md Section 16](MATHEMATICAL_FORMULATION.md#16-multi-observer-secondary-viewports--relativistic-probe-kinematics)), a 3D spatial reference overlay with adaptive coordinate planes, graduation ticks, distance rings, and horizon occlusion ([MATHEMATICAL_FORMULATION.md Section 15](MATHEMATICAL_FORMULATION.md#15-3d-spatial-reference--coordinate-grid-mathematics)), and a HUD of 34 configurable elements ([ARCHITECTURE.md Section 2.12](ARCHITECTURE.md#212-schematic-view-and-hud)).

### 9.5. Synthetic VLBI Interferometry & Event Horizon Telescope Modeling

- Discrete Fourier Transform (DFT) and 2D FFT visibility synthesis from the ray-traced image plane onto ground and space baselines ([MATHEMATICAL_FORMULATION.md Section 13.1](MATHEMATICAL_FORMULATION.md#131-van-cittert-zernike-theorem--visibility-transform)).
- Real-world source calibration presets (M87*, Sgr A*, and custom targets) converting geometrized lengths to sky angular scales via the gravitational angular radius $\theta_g = GM / (c^2 D)$ ([Section 13.2](MATHEMATICAL_FORMULATION.md#132-angular-scaling--gravitational-radius-angle)).
- Earth-aperture synthesis over configurable observation schedules with station elevation masks, digital correlation quantization losses, and Gaussian thermal receiver noise derived from station System Equivalent Flux Densities (SEFD) ([Section 13.4](MATHEMATICAL_FORMULATION.md#134-thermal-noise--quantization-efficiency)).
- Ground-based array models (Event Horizon Telescope, Next-Generation EHT, Global mm-VLBI Array) and space-VLBI orbiters with Keplerian orbital parameters ([Section 13.3](MATHEMATICAL_FORMULATION.md#133-baseline-track-geometry-in-the-uv-plane)).
- Observable diagnostics: visibility amplitude vs baseline length with radial FFT profile, first null detection estimating equivalent thin-ring diameter, closure phase evaluation over station triangles invariant to antenna-based atmospheric phase corruptions, and 2D Fourier plane coverage heatmaps ([Section 13.5](MATHEMATICAL_FORMULATION.md#135-closure-phase-invariance--thin-ring-diameter-estimation)).
- Standardized astronomical export in the Optical/Infrared/Radio Interferometry FITS (OIFITS) format ([FILE_FORMATS.md Section 2.3](FILE_FORMATS.md#23-opticalinfraredradio-interferometry-exchange-format-oifits)).

---

## 10. Data Interchange & Ephemeris Ingestion

### 10.1. Astronomical Ephemerides & Coordinate Frames

- NASA JPL Horizons: Construction of API request URLs (`HorizonsInterface::build_query_url`) and parsing of CSV vector tables into ICRF/J2000 state vectors in SI units (`HorizonsInterface::parse_horizons_response`, `include/relativistic/io/horizons_parser.hpp`). The engine performs no network access. Osculating Keplerian elements are converted to state vectors by `KeplerianElements::to_state_vector` (`include/relativistic/io/ephemeris_types.hpp`).
- SPICE SPK segments: Evaluation of Chebyshev Type 2 and Type 3 segments through recurrence relations (`SpkChebyshevSegment::evaluate`, `include/relativistic/io/spk_reader.hpp`). `SpkKernel::parse_daf_spk_bytes` validates the file header only, and segments are added programmatically ([FILE_FORMATS.md](FILE_FORMATS.md#5-planetary-ephemeris--spice-integration)).
- Coordinate frame transformations between ICRF J2000, Heliocentric Ecliptic, Geocentric Equatorial, Planetocentric body-fixed frames, & comobile boosted tetrads.

### 10.2. Scientific Data Export

- FITS (Flexible Image Transport System): 2D surface radiance maps & 3D spectral data cubes $(X, Y, \lambda)$ with standard astronomical WCS metadata headers, as well as synthetic interferometry datasets in OIFITS format ([FILE_FORMATS.md Section 2.3](FILE_FORMATS.md#23-opticalinfraredradio-interferometry-exchange-format-oifits)).
- HDF5-signature container (`Hdf5Container`): Custom binary container for worldlines, metric tensor series, covariance matrices, equation of state tables & telemetry channels. It reuses the HDF5 signature value and is not readable by the HDF5 library (dataset layout in [FILE_FORMATS.md](FILE_FORMATS.md#3-hierarchical-data-format-h5)).
- VTK / VTP (Visualization Toolkit PolyData): Polyline representation of geodesic rays, worldlines, & event horizon surface meshes.
- Images: Ten formats (PPM, BMP, PNG, TGA, HDR, PNG16, QOI, PFM, TIFF, PAM) written row by row by stream writers ([FILE_FORMATS.md](FILE_FORMATS.md#6-image-output-formats)).

### 10.3. Capture, Sequence Rendering & Telemetry Recording

The Capture Studio (`include/relativistic/ui/capture_studio_window.hpp`) and `CaptureCoordinator` (`include/relativistic/capture/capture_coordinator.hpp`) provide:
- Screenshots rendered offline at an output resolution independent of the viewport, with supersampling up to 8 x 8 rays per pixel and a separate step budget and step refinement factor.
- Image sequences in a deterministic mode (frame-by-frame rendering with controlled world advance, temporal sub-frame averaging for motion blur, fades, manual stepping) or a real-time mode (recording of the displayed frames with drop or duplicate pacing). Frames are rendered in horizontal bands and streamed to disk, so memory use does not depend on the output height.
- Motion scripts: Segments composed of shape layers (29 shape kinds across 7 categories including relativistic infall, conical jet helix, Keplerian orbit, and dynamic chase), 9 orientation modes (including chase-lead tracking), animated scalar channels (field of view, exposure, roll, simulation rate) with easing, drivers, expressions and modulation, camera shake, segment transitions with 7 projection morphing modes ([MATHEMATICAL_FORMULATION.md Section 9.4](MATHEMATICAL_FORMULATION.md#94-projection-mode-transitions--morphing-mathematics)), and timed events (15 actions across 52 mapped parameters). A 3D path preview is drawn in the viewport before rendering.
- Telemetry recording: 54 channels of camera kinematics, relativistic quantities at the camera, spacetime parameters and body aggregates, with optional per-body columns, written in seven formats.
- Video encoding and assembly: Automated assembly via ffmpeg supporting 9 video codecs, 6 containers, Constant Quality (CRF) and Target Bitrate rate controls, 9 encoder speed presets (`Ultrafast` to `Veryslow`), custom video filter chaining, custom output argument passing, and GIF palette generation with Bayer dithering ([FILE_FORMATS.md Section 7](FILE_FORMATS.md#7-capture-session-directory)).

The behavior of these components is specified in [ARCHITECTURE.md](ARCHITECTURE.md#210-capture-subsystem) and the files they write in [FILE_FORMATS.md](FILE_FORMATS.md#7-capture-session-directory).

---

## 11. Determinism, Replay Architecture & Scenarios

### 11.1. Bit-Level Determinism & Random Number Generation

- Explicit-state PCG64 pseudo-random engines, with per-stream sub-engines derived by `DeterministicRngRegistry` (`include/relativistic/core/pcg64.hpp`). Procedural sky and body surface patterns are integer hashes of the direction or surface coordinates and a user-controlled seed. Randomized body creation in the Body Manager uses `std::mt19937_64` seeded from `std::random_device` and is not reproducible.
- Fixed logical scheduler time step execution combined with strict floating-point rounding control.

### 11.2. Forensic Diagnostics & Hang Monitoring

- Structured Crash Dumps: Automatic interception of unhandled SEH exceptions, signals (`SIGSEGV`, `SIGBUS`, `SIGFPE`, `SIGILL`, `SIGABRT`), `std::terminate`, and CRT assertions writing complete diagnostics to `logs/crashes/` ([Technical Manual](TECHNICAL_MANUAL.md#211-constants-units-and-expressions)).
- Diagnostic Data Assets: Each crash folder preserves register state, disassembly at RIP, stack words at RSP, symbol-resolved call stack backtraces, minidumps (`minidump.dmp`), module memory maps, thread activity tables, active preference category files, and session log tails.
- Autonomous Hang Watchdog: Background monitoring thread tracking UI frame rates (threshold 20 s, excluding native modal drag/size loops) and simulation tick heartbeats (threshold 45 s), capturing non-terminating diagnostic dumps upon deadlock or livelock.

### 11.3. Synchronous Command Journal & Replay

- `EventJournal` (`include/relativistic/core/deterministic_replay.hpp`) stores `SimulationInputEvent` records (21 bytes packed: tick index, action identifier, value, flags) in a caller-provided buffer, with a write count and a replay cursor. The defined actions are radial impulse, angular impulse, mass modification, spin modification and time-step change, and the modes are `Record`, `Replay` and `Detached`.
- The journal is a standalone primitive: `SimulationOrchestrator` does not record its commands into it, and replay of orchestrator sessions is not implemented. Reruns rely on saved scenarios ([FILE_FORMATS.md](FILE_FORMATS.md#1-declarative-scenario-definition-yaml)) and on the deterministic capture mode ([ARCHITECTURE.md](ARCHITECTURE.md#210-capture-subsystem)).

### 11.4. Standard Simulation Scenarios

- *Solar System Post-Newtonian Validation*: Multi-body planetary integration initialized from JPL DE440 ephemerides, validating Mercury's secular perihelion advance under 1PN corrections & solar quadrupole moment $J_2$.
- *Kerr Black Hole & Novikov-Thorne Accretion Disk*: High-spin black hole ($a = 0.94M$) surrounded by a thin accretion disk, illustrating Doppler beaming, gravitational redshift, & black hole shadow geometry.
- *Hulse-Taylor Binary Pulsar (PSR B1913+16)*: Binary neutron star system executing 2.5PN radiation reaction orbital decay & generating gravitational wave strain waveforms.
- *Traversable Morris-Thorne Wormhole*: Observer transit across a wormhole throat connecting two distinct celestial environments without metric singularities.
- *Alcubierre Warp Bubble Exploration*: Superluminal metric bubble navigating a regular grid of celestial beacons, demonstrating spacetime contraction & horizon formation.
- *Relativistic Hydrodynamic Shock Tubes*: Sod shock tube & Balsara-1 magnetized relativistic shock tube validations.
- *Galactic Collision with Dark Matter*: Merging disk-bulge-halo galaxies modeled with collisionless N-body particles & NFW/Burkert dark matter halos under Barnes-Hut octree acceleration.

### 11.5. Benchmark Performance Workbench & Statistical Comparison Engine

- Multi-metric benchmark run comparison supporting up to 6 simultaneous Y-series from the profiler catalog ([ARCHITECTURE.md Section 2.11](ARCHITECTURE.md#211-profiling-logging-and-persistence)).
- Four chart visualization modes: Grouped Bars, Lines (ordered by X), Scatter, and Stacked Stage Breakdowns.
- Value normalization transformations: Raw values, Log10, Natural Log, Log2, Ratio to Baseline, Percent Change from Baseline, Min-Max normalization, and Z-score standardization ([MATHEMATICAL_FORMULATION.md Section 14.2](MATHEMATICAL_FORMULATION.md#142-data-normalization-transforms)).
- Two-sample Welch's $t$-test and Cohen's $d$ effect size evaluations determining statistical significance and confidence intervals between baseline and comparative benchmark runs ([Section 14.1](MATHEMATICAL_FORMULATION.md#141-two-sample-welch-t-test--cohens-d-effect-size)).
- Tab-separated value (TSV) clipboard serialization for external spreadsheet analysis and reporting.

---

## 12. Verification Criteria & Benchmark Protocols

The engine incorporates automated analytical test suites verifying numerical convergence & physical conservation laws:

| Benchmark Case | Physical Target | Theoretical Reference | Validation Criteria |
| :--- | :--- | :--- | :--- |
| Solar Light Deflection | Null geodesic solar limb grazing | $\Delta\theta = \frac{4GM_\odot}{c^2 R_\odot} \approx 1.7512''$ | Relative error $\epsilon < 10^{-3}$ |
| Mercury Perihelion Advance | 1PN secular orbital precession | $\Delta\phi = \frac{6\pi GM_\odot}{c^2 a(1-e^2)} \approx 42.98''/\text{century}$ | Precession error $\epsilon < 5\%$ |
| Shapiro Time Delay | Signal round-trip through solar field | $\Delta t = \frac{4GM}{c^3}\left[\ln\left(\frac{4r_1 r_2}{r_0^2}\right)+1\right]$ | Double-precision identity |
| Kerr Shadow Boundary | Critical null orbits & Bardeen shadow | Analytical Carter constant integrals | Boundary overlap $> 99.99\%$ |
| Hulse-Taylor Decay | 2.5PN radiation reaction energy loss | Peters-Mathews $\dot{P}_b \approx -2.423 \times 10^{-12} \, \text{s/s}$ | Discrepancy $< 1.0\%$ |
| Hydro Shock Tube | Relativistic Sod / Balsara-1 shocks | Rankine-Hugoniot jump conditions | Monotonic profile at $t = 0.35$ |
| TOV Stellar Maximum Mass | Relativistic hydrostatic equilibrium | Tabulated nuclear EoS maximum $M_{\text{max}}$ | Convergence to mass peak |
| Geodetic Precession | Gyroscope spin transport along orbit | Fokker-de Sitter precession rate | Agreement with $\Omega_{\text{geodetic}}$ |

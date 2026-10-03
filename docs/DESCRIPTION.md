# Specification & System Description

## 1. Global Vision, Philosophy & Core Objectives

The engine is a scientific computing and interactive simulation software for Special Relativity, General Relativity, and alternative gravitational models. The platform implements analytical metrics, post-Newtonian dynamics, relativistic hydrodynamics, and polarized radiative transfer derived from standard relativistic physics formulations.

The platform operates across two complementary runtime paradigms:
- Real-Time Interactive Exploration Mode: A low-latency, interactive 3D navigation environment with direct 6-DOF controls, enabling an observer to traverse complex spacetimes, modify physical parameters dynamically, & observe relativistic optical effects, kinematic transformations, & geodesic trajectories in real time.
- High-Fidelity Batch / Headless Generation Mode: An automated computation pipeline executing intensive simulation sweeps, high-density raytracing, multi-body orbital evolutions, gravitational wave strain extractions, & spectral cube generation without requiring an active graphical context.

The architectural foundation relies on modularity: spacetimes, numerical integrators, equations of state, radiative transfer models, observer kinematics, & uncertainty quantification engines are interchangeable components adhering to uniform concepts. All computational paths prioritize performance through shared-memory multithreading, explicit SIMD register vectorization, persistent memory arenas, & GPU compute pipelines.

Uncertainty quantification & formal error propagation are native primitives within the system, allowing the rigorous qualification of truncation errors, initial condition perturbations, & numerical discretization artifacts alongside physical signals.

---

## 2. System Modularity & Component Selection Matrix

The engine provides configuration flexibility across all layers of the physical simulation stack. Each simulation scenario explicitly defines the active theoretical framework, numerical approximation order, & observation pipeline.

### 2.1. Permutable Component Matrix

- Spacetime Representation: Selection between stationary analytical solutions ([MATHEMATICAL_FORMULATION.md Section 1](MATHEMATICAL_FORMULATION.md#1-spacetime-metrics--line-elements)), dynamical vacuum metrics, cosmological expanding backgrounds ([Section 1.7](MATHEMATICAL_FORMULATION.md#17-flrw-metric-cosmological-spacetime)), and numerical 3+1 BSSN metric grids with spatial interpolation ([Section 4.4](#44-31-numerical-relativity-grids-bssn)).
- Coordinate Systems: Freedom to select coordinate gauges for a given metric to bypass coordinate singularities, optimize numerical stability across horizons, or maintain spatial conformality ([Section 4.2](#42-coordinate-systems--gauges)).
- Differential Integration Solvers: Selection of adaptive variable-step Runge-Kutta solvers, high-order embedded formulas, implicit symplectic Gauss-Legendre integrators conserving phase-space Killing invariants, or predictor-corrector Hermite schemes with jerk evaluation ([ARCHITECTURE.md Section 2.3](ARCHITECTURE.md#23-differential-solvers--geodesic-integrators)). The integrator name selected at runtime determines the N-body integrator only (symplectic Forest-Ruth post-Newtonian scheme for names containing `Symplectic` or `Gauss`, fourth-order Runge-Kutta otherwise); the ray tracers use a fixed-step RK4 scheme (see [TECHNICAL_MANUAL.md](TECHNICAL_MANUAL.md#runtime-use-of-the-integrator-selection)).
- Matter & Hydrodynamic Models: Representation of matter via test particles, rigid multipolar bodies with spherical harmonic expansions ([Section 5.2](#52-spherical-harmonics--high-degree-geodesy)), relativistic ideal fluids, degenerate Fermi gases, or tabulated nuclear matter equations of state ([MATHEMATICAL_FORMULATION.md Section 4.3](MATHEMATICAL_FORMULATION.md#43-equations-of-state-eos)).
- Post-Newtonian Regimes: Configuration of multi-body gravitational interactions across orders (Newtonian, 1PN, 2PN, 2.5PN radiative reaction, 3PN, 3.5PN with spin-orbit and spin-spin couplings; see [Section 5.1](#51-post-newtonian-multi-body-dynamics) and [MATHEMATICAL_FORMULATION.md Section 3](MATHEMATICAL_FORMULATION.md#3-post-newtonian-pn-n-body-dynamics)).
- Radiative Transfer & Polarimetry: Execution of radiative transport ranging from direct kinematic Doppler shifting to full-Stokes polarimetric transport integrating synchrotron emission, Bremsstrahlung, Faraday rotation, and inverse Compton scattering ([MATHEMATICAL_FORMULATION.md Section 6](MATHEMATICAL_FORMULATION.md#6-polarized-radiative-transfer)).
- Colorimetry & Spectral Integration: Continuous integration over arbitrary electromagnetic spectra coupled with standard CIE 1931 observer matching functions and wide-gamut HDR tone mapping ([Section 9.3](#93-continuous-spectral-pipeline--cie-1931-integration) and [MATHEMATICAL_FORMULATION.md Section 8.3](MATHEMATICAL_FORMULATION.md#83-tone-mapping-and-color-grading)).
- Uncertainty Quantification: Real-time selection of bounded interval arithmetic (IEEE 1788), Girard-reduced zonotope enclosures, continuous Jacobi-Lyapunov covariance propagation, generalized polynomial chaos expansions (gPCE), or Monte Carlo ensemble sampling ([Section 7.2](#72-uncertainty-propagation-frameworks) and [MATHEMATICAL_FORMULATION.md Section 7](MATHEMATICAL_FORMULATION.md#7-uncertainty-quantification-formulation)).
- Gravitational Theories: General Relativity, Modified Newtonian Dynamics (MOND), Tensor-Vector-Scalar gravity (TeVeS), scalar-tensor $f(R)$ Chameleon models, and non-baryonic dark matter halo profiles ([Section 5.3](#53-dark-matter-halos--alternative-gravitational-theories)).
- Physical Constants & Units: SI, Planck or custom base constants ($c$, $G$, $h$, $k_B$, $N_A$, $K_e$, $K_{cd}$) with the derived scale factors between simulation units and SI units ([ARCHITECTURE.md Section 2.13](ARCHITECTURE.md#213-physical-constants-engine)), and display units selectable independently for 19 physical quantities ([TECHNICAL_MANUAL.md Section 2.11](TECHNICAL_MANUAL.md#211-constants-units-and-expressions)).
- Body Interactions: Electromagnetic, collision, thermodynamic, fragmentation and annihilation interactions between N-body bodies ([Section 5.4](#54-body-interactions--spacetime-source-bodies)).

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

The image renderer traces the Schwarzschild-de Sitter, FLRW, Morris-Thorne and Alcubierre selections with the Schwarzschild null tracer, and does not evaluate $\Lambda$, $b_0$ or $v_s$. The metric classes implement the corresponding geometries ([TECHNICAL_MANUAL.md](TECHNICAL_MANUAL.md#metric-usage-in-softwarecomputeengine)).

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

- Non-Baryonic Dark Matter Profiles:
  - *Navarro-Frenk-White (NFW)* profile with scale radius $r_s$ & characteristic density $\rho_0$.
  - *Einasto* profile with shape parameter $\alpha$ ensuring finite central density.
  - *Burkert* & *Hernquist* analytical profiles for cuspy & cored galactic cores.
  - *Collisionless N-Body Dynamics*: Hierarchical Barnes-Hut octree spatial decomposition with quadrupole moment corrections & symplectic leapfrog time integration for galactic collision simulations.
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

Each body can carry up to four procedural surface layers (10 patterns, 5 blend modes, 7 region masks; `include/relativistic/dynamics/body_surface_layers.hpp`).

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
- Tolman-Oppenheimer-Volkoff (TOV) Solver: Relativistic hydrostatic equilibrium solver computing stellar structure profiles, mass-radius curves, compactness, surface redshifts, and stability boundaries.

### 6.3. Relativistic Accretion Disks & Tori

- Novikov-Thorne Thin Disk: Radiatively efficient, geometrically thin, equatorial Keplerian accretion disk around Kerr black holes with exact Page-Thorne analytical boundary integration down to the innermost stable circular orbit ($r_{\text{ISCO}}$). The ray tracers do not evaluate this profile; they use the parametrized thin-disk model of [MATHEMATICAL_FORMULATION.md](MATHEMATICAL_FORMULATION.md#82-step-control-termination-and-space-skipping).
- Fishbone-Moncrief Magnetized Thick Torus: Relativistic stationary torus with constant specific angular momentum $l = -u_\phi / u_t$ & barotropic pressure equilibrium.

### 6.4. Radiative Processes & Local Emission

- Relativistic Synchrotron Radiation: Thermal & non-thermal power-law emission & self-absorption coefficients from relativistic electrons in magnetic fields.
- Relativistic Thermal Bremsstrahlung: Electron-ion free-free radiation incorporating relativistic Gaunt factor corrections.
- Inverse Compton Scattering: Monte Carlo photon packet scattering across relativistic thermal electron populations using the exact Klein-Nishina differential cross section.
- Maxwell-Jüttner Electron Distribution: Exact relativistic thermal velocity distribution sampling:
  $$f(\gamma) = \frac{\gamma \sqrt{\gamma^2 - 1}}{\theta_e K_2(1/\theta_e)} \exp\left(-\frac{\gamma}{\theta_e}\right), \quad \theta_e = \frac{k_B T_e}{m_e c^2}$$

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

- Bounded Interval Arithmetic: Strict lower & upper interval bounds $[\underline{x}, \bar{x}]$ adhering to the IEEE 1788 standard.
- Girard-Reduced Zonotopes: Symmetric affine generator polytopes $\mathcal{Z} = \mathbf{c} \oplus \sum \alpha_i \mathbf{g}_i$ mitigating wrapping effects during extended orbital integrations.
- Continuous Jacobi-Lyapunov Covariance Propagation: Simultaneous integration of the 8D phase state $(\mathbf{x}, \mathbf{p})$, variational Jacobian transition matrices, & the continuous Lyapunov covariance ODE:
  $$\frac{d\mathbf{\Sigma}}{d\lambda} = \mathbf{J}\mathbf{\Sigma} + \mathbf{\Sigma}\mathbf{J}^T + \mathbf{Q}$$
- Generalized Polynomial Chaos Expansion (gPCE): Orthogonal polynomial projections (Hermite for Gaussian, Legendre for uniform distributions) evaluated via multi-dimensional Gauss-Hermite & Gauss-Legendre quadratures.
- Monte Carlo Ensemble Sampling: Thread-parallel generation & integration of stochastically perturbed geodesic bundles.

### 7.3. Metrology & Visual Representation

- 3D Covariance Ellipsoids: Real-time generation of $1\sigma, 2\sigma, 3\sigma$ iso-probability confidence meshes along particle trajectories.
- 2D/3D Probability Density Heatmaps: Spatial projection of positional probability distributions around photon rings, horizons, & shock fronts.
- Spectral & Temporal Quantile Bands: Real-time visualization of confidence intervals ($1\sigma, 2\sigma, 3\sigma$) on spectral radiance curves, bolometric light curves, & gravitational wave polarizations ($h_+, h_\times$). The shaded band of `SpectrographWindow` is a fixed 8 % of the radiance and is not derived from the propagation frameworks of [Section 7.2](#72-uncertainty-propagation-frameworks).

---

## 8. Observer Kinematics & Coordinate Transport

### 8.1. Observer Definition & Comobile Orthonormal Tetrads

An observer is defined by a 4-position $x^\mu(\tau)$, a normalized timelike 4-velocity $u^\mu = dx^\mu / d\tau$ ($u_\mu u^\mu = -c^2$), & an orthonormal comobile tetrad $\{e^\mu_{(0)}, e^\mu_{(1)}, e^\mu_{(2)}, e^\mu_{(3)}\}$ where $e^\mu_{(0)} = u^\mu / c$ & $e^\mu_{(a)} e_{\mu (b)} = \eta_{ab}$.

The observer operates in two distinct dynamical modes:
- Kinematic Decoupled Observer (Free-Fly Camera): Massless point observer following user-defined coordinate paths, unaffected by inertial or tidal forces.
- Relativistic Rocket Observer (6-DOF Dynamic Vehicle): Massive test vehicle governed by relativistic propulsion equations:
  $$\frac{du^\mu}{d\tau} + \Gamma^\mu_{\alpha\beta} u^\alpha u^\beta = a^\mu_{\text{proper}}$$
  subject to proper thrust, fuel consumption, inertia, & local gravitational curvature gradients.

The interactive Rocket Thrust mode (`InteractiveCameraController::update_rocket_mode`, `include/relativistic/ui/interactive_camera_controller.hpp`) implements a reduced model: thrust accelerations along the camera axes with a magnitude cap of `max_proper_acceleration`, a Newtonian gravitational deceleration $M/r^2$ toward the origin, and explicit Euler integration of velocity and position while the simulation clock runs. Fuel consumption, inertia and curvature gradients are not modeled.

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

Transport of the Stokes vector $\mathbf{S} = (I, Q, U, V)^T$ is integrated along ray segments via Delano's analytical matrix exponential method, ensuring stable solutions in the presence of extreme optical depths, Faraday rotation ($\rho_V$), & Faraday conversion ($\rho_Q$).

### 9.3. Continuous Spectral Pipeline & CIE 1931 Integration

- Spectral discretization over 64 to 400 logarithmic wavelength bins from radio ($10^3 \, \text{m}$) to gamma rays ($10^{-14} \, \text{m}$).
- Frequency shifting of local emission: $\lambda_{\text{obs}} = \lambda_{\text{emit}} / g$.
- Convolution with standard CIE 1931 color matching functions $\bar{x}(\lambda), \bar{y}(\lambda), \bar{z}(\lambda)$ to obtain tristimulus values $(X, Y, Z)$ converted into linear sRGB.
- High dynamic range (HDR) tone mapping supporting ACES filmic curve mapping, extended logarithmic scaling ($10^{-12}$ to $10^{20} \, \text{W/m}^2/\text{sr}$), & Reinhard extensions.

The renderer implements a subset of this pipeline: the disk color is the blackbody spectrum at the Doppler-shifted temperature integrated against the CIE 1931 functions with 24 samples between 380 and 780 nm, Stokes transport is not evaluated, and four tone mapping operators (linear, ACES approximation, logarithmic, extended Reinhard) are followed by CPU color grading ([MATHEMATICAL_FORMULATION.md](MATHEMATICAL_FORMULATION.md#83-tone-mapping-and-color-grading)).

### 9.4. Celestial Bodies, Sky & Execution Paths

- Bodies: Ray-traced ellipsoids (oblate, rigid sphere, prolate, triaxial) with 13 base texture modes, up to four texture layers, atmospheric rim, lighting from the central source, shadows, Doppler beaming ($g^3$), gravitational redshift, and three levels of detail (full, simplified, flat dot) selected from the apparent pixel coverage ([MATHEMATICAL_FORMULATION.md](MATHEMATICAL_FORMULATION.md#84-celestial-body-shading)).
- Sky: Procedural sky from a seeded starfield, galactic band, optional coordinate grid, deep-field galaxies, dust clouds and star clusters, or an equirectangular panorama selected from the catalog in `include/relativistic/optics/sky_panorama_catalog.hpp`.
- Execution paths: Vulkan compute path in native FP64 for six metric identifiers, CPU SIMD and scalar paths in double and single precision, with the selection rules of [ARCHITECTURE.md](ARCHITECTURE.md#29-render-pipeline).
- Presentation: Schematic projection of bodies, trails, orbit predictions and vectors without ray tracing, an overlay of this projection on the ray-traced image, up to eight secondary observer viewports, and a HUD of 25 configurable elements ([ARCHITECTURE.md](ARCHITECTURE.md#212-schematic-view-and-hud)).

---

## 10. Data Interchange & Ephemeris Ingestion

### 10.1. Astronomical Ephemerides & Coordinate Frames

- NASA JPL Horizons: Construction of API request URLs (`HorizonsInterface::build_query_url`) and parsing of CSV vector tables into ICRF/J2000 state vectors in SI units (`HorizonsInterface::parse_horizons_response`, `include/relativistic/io/horizons_parser.hpp`). The engine performs no network access. Osculating Keplerian elements are converted to state vectors by `KeplerianElements::to_state_vector` (`include/relativistic/io/ephemeris_types.hpp`).
- SPICE SPK segments: Evaluation of Chebyshev Type 2 and Type 3 segments through recurrence relations (`SpkChebyshevSegment::evaluate`, `include/relativistic/io/spk_reader.hpp`). `SpkKernel::parse_daf_spk_bytes` validates the file header only, and segments are added programmatically ([FILE_FORMATS.md](FILE_FORMATS.md#5-planetary-ephemeris--spice-integration)).
- Coordinate frame transformations between ICRF J2000, Heliocentric Ecliptic, Geocentric Equatorial, Planetocentric body-fixed frames, & comobile boosted tetrads.

### 10.2. Scientific Data Export

- FITS (Flexible Image Transport System): 2D surface radiance maps & 3D spectral data cubes $(X, Y, \lambda)$ with standard astronomical WCS metadata headers.
- HDF5-signature container (`Hdf5Container`): Custom binary container for worldlines, metric tensor series, covariance matrices, equation of state tables & telemetry channels. It reuses the HDF5 signature value and is not readable by the HDF5 library (dataset layout in [FILE_FORMATS.md](FILE_FORMATS.md#3-hierarchical-data-format-h5)).
- VTK / VTP (Visualization Toolkit PolyData): Polyline representation of geodesic rays, worldlines, & event horizon surface meshes.
- Images: Ten formats (PPM, BMP, PNG, TGA, HDR, PNG16, QOI, PFM, TIFF, PAM) written row by row by stream writers ([FILE_FORMATS.md](FILE_FORMATS.md#6-image-output-formats)).

### 10.3. Capture, Sequence Rendering & Telemetry Recording

The Capture Studio (`include/relativistic/ui/capture_studio_window.hpp`) and `CaptureCoordinator` (`include/relativistic/capture/capture_coordinator.hpp`) provide:
- Screenshots rendered offline at an output resolution independent of the viewport, with supersampling up to 8 x 8 rays per pixel and a separate step budget and step refinement factor.
- Image sequences in a deterministic mode (frame-by-frame rendering with controlled world advance, temporal sub-frame averaging for motion blur, fades, manual stepping) or a real-time mode (recording of the displayed frames with drop or duplicate pacing). Frames are rendered in horizontal bands and streamed to disk, so memory use does not depend on the output height.
- Motion scripts: Segments composed of shape layers (21 shape kinds), orientation modes, animated scalar channels (field of view, exposure, roll, simulation rate) with easing, drivers, expressions and modulation, camera shake, transitions between segments, and timed events (14 actions). A path preview is drawn in the viewport before rendering.
- Telemetry recording: 54 channels of camera kinematics, relativistic quantities at the camera, spacetime parameters and body aggregates, with optional per-body columns, written in seven formats.
- Optional assembly of the frames into a video with ffmpeg.

The behavior of these components is specified in [ARCHITECTURE.md](ARCHITECTURE.md#210-capture-subsystem) and the files they write in [FILE_FORMATS.md](FILE_FORMATS.md#7-capture-session-directory).

---

## 11. Determinism, Replay Architecture & Scenarios

### 11.1. Bit-Level Determinism & Random Number Generation

- Explicit-state PCG64 pseudo-random engines, with per-stream sub-engines derived by `DeterministicRngRegistry` (`include/relativistic/core/pcg64.hpp`). Procedural sky and body surface patterns are integer hashes of the direction or surface coordinates and a user-controlled seed. Randomized body creation in the Body Manager uses `std::mt19937_64` seeded from `std::random_device` and is not reproducible.
- Fixed logical scheduler time step execution combined with strict floating-point rounding control.

### 11.2. Synchronous Command Journal & Replay

- `EventJournal` (`include/relativistic/core/deterministic_replay.hpp`) stores `SimulationInputEvent` records (21 bytes packed: tick index, action identifier, value, flags) in a caller-provided buffer, with a write count and a replay cursor. The defined actions are radial impulse, angular impulse, mass modification, spin modification and time-step change, and the modes are `Record`, `Replay` and `Detached`.
- The journal is a standalone primitive: `SimulationOrchestrator` does not record its commands into it, and replay of orchestrator sessions is not implemented. Reruns rely on saved scenarios ([FILE_FORMATS.md](FILE_FORMATS.md#1-declarative-scenario-definition-yaml)) and on the deterministic capture mode ([ARCHITECTURE.md](ARCHITECTURE.md#210-capture-subsystem)).

### 11.3. Standard Simulation Scenarios

- *Solar System Post-Newtonian Validation*: Multi-body planetary integration initialized from JPL DE440 ephemerides, validating Mercury's secular perihelion advance under 1PN corrections & solar quadrupole moment $J_2$.
- *Kerr Black Hole & Novikov-Thorne Accretion Disk*: High-spin black hole ($a = 0.94M$) surrounded by a thin accretion disk, illustrating Doppler beaming, gravitational redshift, & black hole shadow geometry.
- *Hulse-Taylor Binary Pulsar (PSR B1913+16)*: Binary neutron star system executing 2.5PN radiation reaction orbital decay & generating gravitational wave strain waveforms.
- *Traversable Morris-Thorne Wormhole*: Observer transit across a wormhole throat connecting two distinct celestial environments without metric singularities.
- *Alcubierre Warp Bubble Exploration*: Superluminal metric bubble navigating a regular grid of celestial beacons, demonstrating spacetime contraction & horizon formation.
- *Relativistic Hydrodynamic Shock Tubes*: Sod shock tube & Balsara-1 magnetized relativistic shock tube validations.
- *Galactic Collision with Dark Matter*: Merging disk-bulge-halo galaxies modeled with collisionless N-body particles & NFW/Burkert dark matter halos under Barnes-Hut octree acceleration.

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

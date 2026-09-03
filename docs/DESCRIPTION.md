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

- Spacetime Representation: Selection between stationary analytical solutions, dynamical vacuum metrics, cosmological expanding backgrounds, & numerical 3+1 BSSN metric grids with spatial interpolation.
- Coordinate Systems: Freedom to select coordinate gauges for a given metric to bypass coordinate singularities, optimize numerical stability across horizons, or maintain spatial conformality.
- Differential Integration Solvers: Selection of adaptive variable-step Runge-Kutta solvers, high-order embedded formulas, implicit symplectic Gauss-Legendre integrators conserving phase-space Killing invariants, or predictor-corrector Hermite schemes with jerk evaluation.
- Matter & Hydrodynamic Models: Representation of matter via test particles, rigid multipolar bodies with spherical harmonic expansions, relativistic ideal fluids, degenerate Fermi gases, or tabulated nuclear matter equations of state.
- Post-Newtonian Regimes: Configuration of multi-body gravitational interactions across orders (Newtonian, 1PN, 2PN, 2.5PN radiative reaction, 3PN, 3.5PN with spin-orbit & spin-spin couplings).
- Radiative Transfer & Polarimetry: Execution of radiative transport ranging from direct kinematic Doppler shifting to full-Stokes polarimetric transport integrating synchrotron emission, Bremsstrahlung, Faraday rotation, & inverse Compton scattering.
- Colorimetry & Spectral Integration: Continuous integration over arbitrary electromagnetic spectra coupled with standard CIE 1931 observer matching functions & wide-gamut HDR tone mapping.
- Uncertainty Quantification: Real-time selection of bounded interval arithmetic (IEEE 1788), Girard-reduced zonotope enclosures, continuous Jacobi-Lyapunov covariance propagation, generalized polynomial chaos expansions (gPCE), or Monte Carlo ensemble sampling.
- Gravitational Theories: General Relativity, Modified Newtonian Dynamics (MOND), Tensor-Vector-Scalar gravity (TeVeS), scalar-tensor $f(R)$ Chameleon models, & non-baryonic dark matter halo profiles.

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
- Frame Dragging (Lense-Thirring Effect): Spacetime vorticity induced by rotating central masses, dragging inertial frames & deforming the ergosphere boundary.
- Photon Spheres & Black Hole Shadows: Determination of unstable circular photon orbits, critical impact parameters, & the central absorption shadow boundary.
- Event Horizon Boundary Behavior: Configurable handling when null geodesics encounter event horizon surfaces:
  - *Pure Absorption Mode*: Immediate geodesic termination with zero luminance assignment or horizon background injection.
  - *Continuous Interior Propagation*: Uninterrupted integration across coordinate-regularized horizons into interior geometries towards physical singularities.
- Relativistic Tidal Forces & Geodesic Deviation: Computation of the Riemann tidal tensor governing differential acceleration across extended bodies.
- Shapiro Gravitational Time Delay: Propagation delay of null signals traversing curved spacetime relative to flat Minkowski baselines.

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
- 3PN & 3.5PN Orders: Lense-Thirring frame-dragging spin-orbit coupling, Barker-O'Connell spin-spin precession, self-spin interactions, & high-order radiation dissipation.

### 5.2. Spherical Harmonics & High-Degree Geodesy

- Fully normalized associated Legendre polynomial gravitational potential expansion up to degree & order 32.
- High-precision modeling of zonal gravitational moments $J_2, J_3, J_4, \dots, J_{20}$, sectorial, & tesseral coefficients $C_{nm}, S_{nm}$ calibrated for the Solar System (EGM96, LP165, MRO110, Jupiter, & Sun models).
- Dynamic tidal Love number perturbations ($k_2, k_3$) & rotational flattening models.

### 5.3. Dark Matter Halos & Alternative Gravitational Theories

- Non-Baryonic Dark Matter Profiles:
  - *Navarro-Frenk-White (NFW)* profile with scale radius $r_s$ & characteristic density $\rho_0$.
  - *Einasto* profile with shape parameter $\alpha$ ensuring finite central density.
  - *Burkert* & *Hernquist* analytical profiles for cuspy & cored galactic cores.
  - *Collisionless N-Body Dynamics*: Hierarchical Barnes-Hut octree spatial decomposition with quadrupole moment corrections & symplectic leapfrog time integration for galactic collision simulations.
- Modified Newtonian Dynamics (MOND): Low-acceleration phenomenology ($a \ll a_0 \approx 1.2 \times 10^{-10} \, \text{m/s}^2$) with standard, simple, exponential, & Bekenstein interpolation functions.
- TeVeS (Tensor-Vector-Scalar Gravity): Relativistic covariant MOND formulation coupling physical metric $g_{\mu\nu}$, dynamic scalar field $\phi$, & unit timelike 4-vector field $U^\mu$.
- Scalar-Tensor $f(R)$ Gravity: Modified gravity under the Jordan & Einstein frames supporting Hu-Sawicki & Starobinsky models with thin-shell Chameleon screening mechanisms in high-density environments.

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
- Polytropic & Piecewise Polytropic EOS: Multi-piece polytropic models calibrated for cold nuclear matter in neutron stars.
- Tabulated Nuclear Matter (Tabulated EOS): Multidimensional trilinear interpolation over density $\rho$, temperature $T$, & electron fraction $Y_e$ supporting SFHo, Shen, LS220, SLy4, & APR4 models.
- Polytropic & Piecewise Polytropic EOS: Polytropic ($P = K \rho^\Gamma$) and 4-piece continuous piecewise polytropic models calibrated for dense nuclear matter.
- Tabulated Nuclear Matter (Tabulated EOS): 3D interpolation over density $\log_{10}\rho$, temperature $\log_{10}T$, and electron fraction $Y_e$ supporting SFHo, Shen, LS220, SLy4, and APR4 models with HDF5 serialization.
- Tolman-Oppenheimer-Volkoff (TOV) Solver: Relativistic hydrostatic equilibrium solver computing stellar structure profiles, mass-radius curves, compactness, surface redshifts, and stability boundaries.

### 6.3. Relativistic Accretion Disks & Tori

- Novikov-Thorne Thin Disk: Radiatively efficient, geometrically thin, equatorial Keplerian accretion disk around Kerr black holes with exact Page-Thorne analytical boundary integration down to the innermost stable circular orbit ($r_{\text{ISCO}}$).
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
- Spectral & Temporal Quantile Bands: Real-time visualization of confidence intervals ($1\sigma, 2\sigma, 3\sigma$) on spectral radiance curves, bolometric light curves, & gravitational wave polarizations ($h_+, h_\times$).

---

## 8. Observer Kinematics & Coordinate Transport

### 8.1. Observer Definition & Comobile Orthonormal Tetrads

An observer is defined by a 4-position $x^\mu(\tau)$, a normalized timelike 4-velocity $u^\mu = dx^\mu / d\tau$ ($u_\mu u^\mu = -c^2$), & an orthonormal comobile tetrad $\{e^\mu_{(0)}, e^\mu_{(1)}, e^\mu_{(2)}, e^\mu_{(3)}\}$ where $e^\mu_{(0)} = u^\mu / c$ & $e^\mu_{(a)} e_{\mu (b)} = \eta_{ab}$.

The observer operates in two distinct dynamical modes:
- Kinematic Decoupled Observer (Free-Fly Camera): Massless point observer following user-defined coordinate paths, unaffected by inertial or tidal forces.
- Relativistic Rocket Observer (6-DOF Dynamic Vehicle): Massive test vehicle governed by relativistic propulsion equations:
  $$\frac{du^\mu}{d\tau} + \Gamma^\mu_{\alpha\beta} u^\alpha u^\beta = a^\mu_{\text{proper}}$$
  subject to proper thrust, fuel consumption, inertia, & local gravitational curvature gradients.

### 8.2. Tetrad Transport Formulations

- Parallel Transport: $\nabla_u e^\mu_{(i)} = 0$, preserving spatial axis orientation along geodesic free-fall lines.
- Fermi-Walker Transport: Applied to accelerating ($a^\mu = \nabla_u u^\mu \neq 0$) & rotating observers:
  $$\frac{D_{\text{FW}} e^\mu_{(i)}}{d\tau} = \nabla_u e^\mu_{(i)} + \frac{1}{c^2} \left( a^\mu u_\nu - u^\mu a_\nu \right) e^\nu_{(i)} = 0$$
  capturing Thomas precession, geodetic (de Sitter) precession, & Lense-Thirring frame-dragging precession.

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

---

## 10. Data Interchange & Ephemeris Ingestion

### 10.1. Astronomical Ephemerides & Coordinate Frames

- Direct ingestion of NASA JPL Horizons API state vectors & osculating Keplerian orbital elements expressed in the ICRF/J2000 barycentric reference frame.
- Evaluation of binary SPICE SPK kernels (DE440, DE441) using Chebyshev polynomial recurrence relations for planetary & spacecraft trajectories.
- Coordinate frame transformations between ICRF J2000, Heliocentric Ecliptic, Geocentric Equatorial, Planetocentric body-fixed frames, & comobile boosted tetrads.

### 10.2. Scientific Data Export

- FITS (Flexible Image Transport System): 2D surface radiance maps & 3D spectral data cubes $(X, Y, \lambda)$ with standard astronomical WCS metadata headers.
- HDF5: Binary storage for multi-dimensional worldlines, metric tensor time series, relativistic fluid grids, & nuclear equation of state tables.
- VTK / VTP (Visualization Toolkit PolyData): Polyline representation of geodesic rays, worldlines, & event horizon surface meshes.

---

## 11. Determinism, Replay Architecture & Scenarios

### 11.1. Bit-Level Determinism & Random Number Generation

- Complete simulation reproducibility via explicit-state PCG64 pseudo-random engines.
- Fixed logical scheduler time step execution combined with strict floating-point rounding control.

### 11.2. Synchronous Command Journal & Replay

- All operator actions, parameter adjustments, & camera manipulations are recorded into a compact, binary-packed event journal indexed by the logical tick identifier.
- Automated replay functionality reconstructs physical trajectories & rendering states with bit-level identity.

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

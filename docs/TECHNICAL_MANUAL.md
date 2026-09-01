# Relativistic Engine - Technical Architecture Manual

## 1. Executive Summary & Design Principles

The Relativistic Engine is a deterministic computing library and rendering framework implemented in ISO C++23. It provides numerical solutions for curved spacetime metrics, resolves timelike and null geodesics, calculates post-Newtonian celestial dynamics up to order 3.5PN, executes general relativistic hydrodynamics and magnetohydrodynamics (GRHD/GRMHD), and integrates full-Stokes polarized radiative transfer across arbitrary pseudo-Riemannian manifolds.

### 1.1. Core Architectural Constraints

- Zero-Allocation Hot Path: Critical computational pathways utilize pre-allocated, linear memory arenas aligned to 64-byte and 128-byte cache line boundaries. Dynamic heap allocations during state evolution and ray integration loops are prohibited.
- Static Polymorphism: Virtual dispatch tables are replaced with C++23 concepts and the Curiously Recurring Template Pattern (CRTP), eliminating indirect branching overhead during metric evaluation and differential integration.
- Determinism: State propagation is bit-reproducible across identical architectures through controlled IEEE 754 floating-point rounding modes, explicit scalar types, and deterministic pseudorandom number generators using the PCG64 algorithm.
- Integrated Uncertainty: Spacetime coordinates, metric parameters, and dynamical quantities support embedded uncertainty propagation via interval arithmetic (IEEE 1788), order-reduced zonotopes, continuous Lyapunov covariance matrices, and generalized polynomial chaos expansions (gPCE).

---

## 2. System Architecture & Module Separation

The engine is organized into subsystems coordinated through an orchestration layer.

### 2.1. Analytical and Numerical Spacetime Core

The core physics layer operates as a stateless mathematical evaluation engine. Spacetime geometries conform to the `SpacetimeMetric` concept, requiring implementations of the covariant metric tensor $g_{\mu\nu}$, the contravariant inverse metric $g^{\mu\nu}$, partial coordinate derivatives $\partial_\alpha g_{\mu\nu}$, and the Christoffel symbols of the second kind $\Gamma^\sigma_{\mu\nu}$.

Metrics support both exact closed-form Christoffel evaluations and high-order numerical finite difference stencils up to eighth order. The module includes stationary and dynamic electrovacuum solutions (Minkowski, Schwarzschild, Kerr, Kerr-Schild, Reissner-Nordström, Kerr-Newman, Schwarzschild-de Sitter), cosmological models (FLRW), exotic geometries (Morris-Thorne wormholes, Alcubierre warp drive), and numerical 3+1 BSSN spacetimes with tricubic spatial and quintic Hermite temporal interpolation.

### 2.2. Differential Geodesic Solvers

Geodesic trajectories for massive particles and photons follow the second-order differential equation:

$$\frac{d^2 x^\mu}{d\lambda^2} + \Gamma^\mu_{\alpha\beta} \frac{dx^\alpha}{d\lambda} \frac{dx^\beta}{d\lambda} = 0$$

The differential solver layer provides five distinct integration paradigms:
- Dormand-Prince (RK45): Adaptive fifth-order Runge-Kutta scheme with fourth-order embedded error estimation and algebraic projection for invariant constraint stabilization.
- Cash-Karp: Embedded Runge-Kutta method optimized for smooth potentials.
- Vernier 9: 16-stage, ninth-order adaptive integrator with eighth-order error estimation for extreme numerical precision over prolonged integration baselines.
- Gauss-Legendre (Orders 4 and 6): Fully implicit symplectic integrators that exactly preserve Hamiltonian invariants, Killing energy, angular momentum, and Carter constants.
- Hermite 4th-Order (Aarseth): Predictor-corrector variable timestep scheme incorporating jerk derivatives for dense gravitational encounters.

### 2.3. Post-Newtonian N-Body Dynamics

For multi-body celestial configurations and compact binary systems, equations of motion are resolved through post-Newtonian expansions up to order 3.5PN:
- 1PN Order: Primary relativistic corrections, Schwarzschild perihelion advance, and gravitational light bending.
- 2PN Order: Non-linear multi-body cross interactions and higher-order self-gravitation.
- 2.5PN Order: Dissipative gravitational radiation reaction causing orbital energy and angular momentum loss according to the Peters-Mathews quadrupole formalism.
- 3PN and 3.5PN Orders: High-order conservative corrections, spin-orbit couplings (Lense-Thirring frame dragging and geodetic precession), spin-spin interactions (Barker-O'Connell precession), and higher-order gravitational radiation losses.
- High-Degree Spherical Harmonics: Gravitational potential expansion utilizing fully normalized associated Legendre polynomials up to degree and order 32 (EGM96, LP165, MRO110 models) to account for planetary oblateness and tidal deformations.

### 2.4. Relativistic Hydrodynamics (GRHD/GRMHD)

The fluid dynamics module resolves the conservative conservation laws $\nabla_\mu (\rho u^\mu) = 0$ and $\nabla_\mu T^{\mu\nu} = 0$ over a 3+1 ADM background metric.
- Spatial Reconstruction: Fifth-order Weighted Essentially Non-Oscillatory (WENO5-Z) and Monotonicity-Preserving (MP5) schemes on primitive variables $(\rho, P, v^i, B^i)$.
- Approximate Riemann Solvers: HLL, HLLC with contact wave restoration, and HLLD for magnetohydrodynamics.
- Primitive Variable Recovery (`con2prim`): Robust Newton-Raphson solvers with bracketed fallback for both purely hydrodynamical and magnetized relativistic regimes.
- Equations of State: Modular interfaces supporting Ideal Gamma-Law, Synge relativistic monoatomic gas, Mathews approximation, Relativistic Degenerate Fermi gas, Piecewise Polytropic models, and multidimensional Tabulated Nuclear tables (SFHo, Shen, LS220, SLy4, APR4).
- Magnetic Solenoidal Constraint: 2D and 3D Constrained Transport (CT) algorithms preserving $\nabla \cdot \mathbf{B} = 0$ to machine precision.
- Accretion Models: Stationary thin accretion disks based on Novikov-Thorne and Page-Thorne relativistic profiles, alongside thick non-radiative tori based on the Fishbone-Moncrief equilibrium solution.

### 2.5. Polarized Radiative Transfer & CIE Optical Pipeline

Backward light tracing integrates null geodesics from the observer aperture into the scene:
- Relativistic Kinematics: Generalized frequency shift factor $g = (p_\mu u^\mu_{\text{obs}}) / (p_\nu u^\nu_{\text{emit}})$, relativistic beaming transforming specific intensity as $I_\nu = g^3 I_{0,\nu}$, and bolometric flux as $F = g^4 F_0$.
- Polarization Transport: Integration of the full Stokes vector $\mathbf{S} = (I, Q, U, V)^T$ along light rays via Delano's analytical matrix exponential method with scaling and squaring.
- Physical Processes: Relativistic thermal and non-thermal synchrotron emission and absorption, thermal relativistic Bremsstrahlung with Gaunt factor corrections, and Monte-Carlo inverse Compton scattering with Maxwell-Jüttner electron distributions.
- Radiometric Conversion: Spectral radiance integrated over continuous wavelengths ($10^{-14}\text{ m}$ to $10^3\text{ m}$) convolved with standard CIE 1931 color matching functions, converted to linear sRGB / Rec.709, and compressed through ACES filmic tonemapping curves.

### 2.6. Uncertainty Propagation Engine

- Interval Arithmetic: Rigorous bound tracking according to IEEE 1788 standards.
- Zonotopes: Affine transformation and Minkowski sum operations with Girard order reduction to eliminate wrapping growth during long-duration orbital integrations.
- Lyapunov Covariance Propagation: Continuous integration of Jacobi variational deviation equations along geodesics.
- Generalized Polynomial Chaos Expansion (gPCE): Orthogonal polynomial projections (Hermite and Legendre) on multi-dimensional Gauss quadrature grids for extraction of mean, variance, skewness, kurtosis, and exact probability quantile envelopes.

### 2.7. Orchestration & Presentation Layer

- Deterministic Scheduler: Fixed-timestep logic execution with variable real-time accumulation and controllable warp factors.
- SPSC Command Queues: Lock-free single-producer single-consumer ring buffers connecting the interactive REPL and user interface with the physics thread.
- Scientific I/O: Exporters for FITS 2D images and 3D spectral cubes with WCS metadata, HDF5 datasets for trajectories and spacetime tensors, VTK PolyData XML files for ParaView visualization, and ingestion interfaces for NASA JPL SPICE SPK kernels and HORIZONS API records.

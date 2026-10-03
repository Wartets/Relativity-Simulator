# Relativistic Engine - Mathematical & Theoretical Formulation

## 1. Spacetime Metrics & Line Elements

### 1.1. Minkowski Metric (Flat Spacetime)

$$ds^2 = -c^2 dt^2 + dx^2 + dy^2 + dz^2$$

Signature: $\eta_{\mu\nu} = \text{diag}(-c^2, 1, 1, 1)$.

### 1.2. Schwarzschild Metric

#### Standard Schwarzschild Coordinates $(t, r, \theta, \phi)$
$$ds^2 = -\left(1 - \frac{r_s}{r}\right) c^2 dt^2 + \left(1 - \frac{r_s}{r}\right)^{-1} dr^2 + r^2 \left(d\theta^2 + \sin^2\theta \, d\phi^2\right)$$
where $r_s = \frac{2GM}{c^2}$.

#### Isotropic Coordinates $(t, r, \theta, \phi)$
$$ds^2 = -\left(\frac{1 - \frac{r_s}{4r}}{1 + \frac{r_s}{4r}}\right)^2 c^2 dt^2 + \left(1 + \frac{r_s}{4r}\right)^4 \left(dr^2 + r^2 d\theta^2 + r^2 \sin^2\theta \, d\phi^2\right)$$

#### Painlevé-Gullstrand Coordinates $(t, r, \theta, \phi)$
$$ds^2 = -c^2 \left(1 - \frac{r_s}{r}\right) dt^2 + 2c \sqrt{\frac{r_s}{r}} \, dt dr + dr^2 + r^2 \left(d\theta^2 + \sin^2\theta \, d\phi^2\right)$$

#### Eddington-Finkelstein Ingoing Coordinates $(t, r, \theta, \phi)$
$$ds^2 = -c^2 \left(1 - \frac{r_s}{r}\right) dt^2 + 2c \, dt dr + r^2 \left(d\theta^2 + \sin^2\theta \, d\phi^2\right)$$

### 1.3. Kerr Metric (Rotating Black Hole)

#### Boyer-Lindquist Coordinates $(t, r, \theta, \phi)$
$$ds^2 = -\left(1 - \frac{2 r_g r}{\rho^2}\right) c^2 dt^2 - \frac{4 r_g r a \sin^2\theta}{\rho^2} c \, dt d\phi + \frac{\rho^2}{\Delta} dr^2 + \rho^2 d\theta^2 + \frac{\Sigma \sin^2\theta}{\rho^2} d\phi^2$$
where:
$$r_g = \frac{GM}{c^2}, \quad \rho^2 = r^2 + a^2 \cos^2\theta, \quad \Delta = r^2 - 2 r_g r + a^2, \quad \Sigma = (r^2 + a^2)^2 - a^2 \Delta \sin^2\theta$$

Outer & inner event horizon radii:
$$r_\pm = r_g \pm \sqrt{r_g^2 - a^2}$$

Outer & inner ergosphere boundary radii:
$$r_{\text{ergo},\pm}(\theta) = r_g \pm \sqrt{r_g^2 - a^2 \cos^2\theta}$$

#### Kerr-Schild Cartesian Coordinates $(t, x, y, z)$
$$g_{\mu\nu} = \eta_{\mu\nu} + 2 H k_\mu k_\nu, \quad g^{\mu\nu} = \eta^{\mu\nu} - 2 H k^\mu k^\nu$$
$$H = \frac{r_g r^3}{r^4 + a^2 z^2}$$
$$k_\mu = \left(c, \frac{r x + a y}{r^2 + a^2}, \frac{r y - a x}{r^2 + a^2}, \frac{z}{r}\right), \quad k^\mu = \left(-\frac{1}{c}, \frac{r x + a y}{r^2 + a^2}, \frac{r y - a x}{r^2 + a^2}, \frac{z}{r}\right)$$
The radial coordinate $r$ is determined from the positive root of:
$$r^4 - (x^2 + y^2 + z^2 - a^2) r^2 - a^2 z^2 = 0 \implies r^2 = \frac{1}{2}\left(R^2 - a^2 + \sqrt{(R^2 - a^2)^2 + 4 a^2 z^2}\right)$$
where $R^2 = x^2 + y^2 + z^2$.

### 1.4. Reissner-Nordström Metric (Charged Black Hole)

$$ds^2 = -f(r) c^2 dt^2 + f(r)^{-1} dr^2 + r^2 \left(d\theta^2 + \sin^2\theta \, d\phi^2\right)$$
$$f(r) = 1 - \frac{r_s}{r} + \frac{r_q^2}{r^2}, \quad r_q^2 = \frac{G k_e Q^2}{c^4}$$

### 1.5. Kerr-Newman Metric (Charged Rotating Black Hole)

$$ds^2 = -\left(1 - \frac{2 r_g r - r_q^2}{\rho^2}\right) c^2 dt^2 - \frac{2(2 r_g r - r_q^2) a \sin^2\theta}{\rho^2} c \, dt d\phi + \frac{\rho^2}{\Delta} dr^2 + \rho^2 d\theta^2 + \frac{\Sigma \sin^2\theta}{\rho^2} d\phi^2$$
$$\Delta = r^2 - 2 r_g r + a^2 + r_q^2, \quad \Sigma = (r^2 + a^2)^2 - a^2 \Delta \sin^2\theta$$

### 1.6. Schwarzschild-de Sitter / Kottler Metric

$$ds^2 = -f(r) c^2 dt^2 + f(r)^{-1} dr^2 + r^2 \left(d\theta^2 + \sin^2\theta \, d\phi^2\right)$$
$$f(r) = 1 - \frac{r_s}{r} - \frac{\Lambda r^2}{3}$$

### 1.7. FLRW Metric (Cosmological Spacetime)

$$ds^2 = -c^2 dt^2 + a^2(t) \left[ \frac{dr^2}{1 - k r^2} + r^2 \left(d\theta^2 + \sin^2\theta \, d\phi^2\right) \right]$$
Scale factor evolution:
$$\frac{H^2(t)}{H_0^2} = \Omega_{r} a^{-4}(t) + \Omega_{m} a^{-3}(t) + \Omega_{k} a^{-2}(t) + \Omega_{\Lambda}$$

### 1.8. Morris-Thorne Traversable Wormhole

$$ds^2 = -e^{2\Phi(l)} c^2 dt^2 + dl^2 + r^2(l) \left(d\theta^2 + \sin^2\theta \, d\phi^2\right)$$
$$r(l) = \sqrt{l^2 + b_0^2}, \quad \Phi(l) = -\frac{\Phi_0}{r(l)}$$

### 1.9. Alcubierre Warp Drive Metric

$$ds^2 = -\left(c^2 - v_s^2(t) f^2(r_s)\right) dt^2 - 2 v_s(t) f(r_s) dx dt + dx^2 + dy^2 + dz^2$$
$$f(r_s) = \frac{\tanh(\sigma (r_s + R)) - \tanh(\sigma (r_s - R))}{2 \tanh(\sigma R)}, \quad r_s = \sqrt{(x - x_s(t))^2 + y^2 + z^2}$$

---

## 2. Geodesic Equations & Curvature Invariants

### 2.1. Christoffel Symbols of the Second Kind

$$\Gamma^\sigma_{\mu\nu} = \frac{1}{2} g^{\sigma\lambda} \left( \partial_\mu g_{\nu\lambda} + \partial_\nu g_{\mu\lambda} - \partial_\lambda g_{\mu\nu} \right)$$

8th-order centered finite difference stencil for numerical evaluation:
$$\partial_\alpha g_{\mu\nu} = \frac{1}{840 h} \left[ 672 \delta_1 g_{\mu\nu} - 168 \delta_2 g_{\mu\nu} + 32 \delta_3 g_{\mu\nu} - 3 \delta_4 g_{\mu\nu} \right]$$
where $\delta_k g_{\mu\nu} = g_{\mu\nu}(x + k h \mathbf{e}_\alpha) - g_{\mu\nu}(x - k h \mathbf{e}_\alpha)$.

### 2.2. Second-Order Geodesic Differential Equations

$$\frac{d^2 x^\mu}{d\lambda^2} + \Gamma^\mu_{\alpha\beta} \frac{dx^\alpha}{d\lambda} \frac{dx^\beta}{d\lambda} = 0$$

Algebraic invariant constraints:
$$g_{\mu\nu} u^\mu u^\nu = \begin{cases} -c^2 & \text{for timelike trajectories} \\ 0 & \text{for null trajectories (light rays)} \end{cases}$$

### 2.3. Riemann Tensor, Ricci Tensor & Kretschmann Scalar

Riemann curvature tensor:
$$R^\rho_{\phantom{\rho}\sigma\mu\nu} = \partial_\mu \Gamma^\rho_{\nu\sigma} - \partial_\nu \Gamma^\rho_{\mu\sigma} + \Gamma^\rho_{\mu\lambda}\Gamma^\lambda_{\nu\sigma} - \Gamma^\rho_{\nu\lambda}\Gamma^\lambda_{\mu\sigma}$$

Ricci curvature tensor:
$$R_{\mu\nu} = R^\rho_{\phantom{\rho}\mu\rho\nu}$$

Ricci scalar curvature:
$$R = g^{\mu\nu} R_{\mu\nu}$$

Kretschmann curvature invariant:
$$K_1 = R^{\alpha\beta\gamma\delta} R_{\alpha\beta\gamma\delta}$$

For Schwarzschild spacetime:
$$K_1 = \frac{48 G^2 M^2}{c^4 r^6} = \frac{12 r_s^2}{r^6}$$

### 2.4. Kerr Characteristic Radii and Horizon Quantities

The expressions of this section are evaluated in geometrized units ($G = c = 1$) by `TelemetryWindow`, `VisualDiagnosticsWindow` and `PhysicsRecorder` (see [TECHNICAL_MANUAL.md](TECHNICAL_MANUAL.md#28-orchestration-scheduling--user-interface)). The spin is $a_* = a / M$, clamped to $\lvert a_* \rvert \le 0.9999999$ for the ISCO and to $\lvert a \rvert \le 0.999 M$ for the horizon and photon orbit of the recorder.

Innermost stable circular orbit (Bardeen, Press & Teukolsky):
$$Z_1 = 1 + (1 - a_*^2)^{1/3}\left[(1 + a_*)^{1/3} + (1 - a_*)^{1/3}\right], \quad Z_2 = \sqrt{3 a_*^2 + Z_1^2}$$
$$r_{\text{ISCO}} = M\left[3 + Z_2 \mp \sqrt{(3 - Z_1)(3 + Z_1 + 2 Z_2)}\right]$$
The upper sign applies to $a \ge 0$ (prograde orbit) and the lower sign to $a < 0$.

Prograde photon orbit radius:
$$r_{\text{ph}} = 2M\left[1 + \cos\left(\tfrac{2}{3}\arccos(-a_*)\right)\right]$$
which equals $3M$ for $\lvert a \rvert \le 10^{-6}$ in the recorder. `VisualDiagnosticsWindow` and `TelemetryWindow` obtain the retrograde and prograde radii from `BardeenKerrShadow`.

Horizon quantities for $\lvert a \rvert \le M$ (units $G = c = \hbar = k_B = 1$):
$$A = 4\pi (r_+^2 + a^2), \quad \kappa = \frac{r_+ - r_-}{2 (r_+^2 + a^2)}, \quad T_H = \frac{\kappa}{2\pi}, \quad S = \frac{A}{4}$$
$$\Omega_H = \frac{a}{r_+^2 + a^2}, \quad M_{\text{irr}} = \sqrt{\frac{A}{16\pi}}$$

Local quantities of a static observer at $(r, \theta)$, evaluated at $\max(r, 2.05 M)$ when a metric tensor is required:
$$\alpha = \sqrt{-g_{tt}}, \quad \frac{d\tau}{dt} = \alpha, \quad \text{time dilation} = \frac{1}{\alpha}, \quad \omega_{\text{ZAMO}} = -\frac{g_{t\phi}}{g_{\phi\phi}}$$
$$\Omega_K = \sqrt{\frac{M}{r^3}}, \quad P_K = \frac{2\pi}{\Omega_K}, \quad v_{\text{esc}} = \min\left(\sqrt{\frac{2M}{r}}, 1\right)$$
The recorder channel `grav_redshift` is $\sqrt{\max(1 - 2M/r, 0)}$ for every spin, and `static_proper_time` accumulates $\alpha \, \Delta t_{\text{logical}}$.

---

## 3. Post-Newtonian (PN) N-Body Dynamics

Equations of motion for $N$ gravitationally interacting bodies:
$$\mathbf{a}_i = \mathbf{a}_i^{\text{Newton}} + \frac{1}{c^2}\mathbf{a}_i^{\text{1PN}} + \frac{1}{c^4}\mathbf{a}_i^{\text{2PN}} + \frac{1}{c^5}\mathbf{a}_i^{\text{2.5PN}} + \frac{1}{c^6}\mathbf{a}_i^{\text{3PN}} + \frac{1}{c^7}\mathbf{a}_i^{\text{3.5PN}} + \mathbf{a}_i^{\text{SO}} + \mathbf{a}_i^{\text{SS}} + \mathbf{a}_i^{\text{Harmonics}}$$

### 3.1. Conservative 1PN Acceleration (Einstein-Infeld-Hoffmann)

$$\mathbf{a}_i^{\text{1PN}} = \sum_{j \neq i} \frac{G m_j \mathbf{n}_{ji}}{r_{ij}^2} \left[ v_i^2 + 2v_j^2 - 4\mathbf{v}_i \cdot \mathbf{v}_j - \frac{3}{2}(\mathbf{n}_{ij} \cdot \mathbf{v}_j)^2 - 4\sum_{k \neq i} \frac{G m_k}{r_{ik}} - \sum_{k \neq j} \frac{G m_k}{r_{jk}} \right] + \sum_{j \neq i} \frac{G m_j \mathbf{v}_{ij}}{r_{ij}^2} \left[ \mathbf{n}_{ji} \cdot (4\mathbf{v}_i - 3\mathbf{v}_j) \right] - \sum_{j \neq i} \sum_{k \neq j} \frac{7 G^2 m_j m_k \mathbf{n}_{kj}}{2 r_{ij} r_{jk}^2}$$

### 3.2. 2.5PN Radiation Reaction Acceleration

For binary separation $\mathbf{r} = \mathbf{r}_1 - \mathbf{r}_2$, $\eta = \frac{m_1 m_2}{(m_1+m_2)^2}$, $M = m_1 + m_2$:
$$\mathbf{a}_{\text{rel}}^{\text{2.5PN}} = \frac{8}{5} \frac{G^2 M^2 \eta}{c^5 r^3} \left[ \left(3v^2 + \frac{17}{3}\frac{GM}{r}\right) \dot{r} \mathbf{n} - \left(v^2 + 3\frac{GM}{r}\right) \mathbf{v} \right]$$
where $\mathbf{n} = \mathbf{r}/r$, $v = \|\mathbf{v}\|$, and $\dot{r} = \mathbf{n} \cdot \mathbf{v}$.

### 3.3. Spin-Orbit & Spin-Spin Coupling

Spin-orbit interaction (Lense-Thirring precession):
$$\mathbf{a}_{\text{rel}}^{\text{SO}} = \frac{G}{c^2 r^3} \left[ \frac{3}{2}(\mathbf{v} \cdot (\mathbf{n} \times \mathbf{S}_{\text{eff}})) \mathbf{n} + \mathbf{v} \times \mathbf{S}'_{\text{eff}} - \frac{3}{2}\dot{r} (\mathbf{n} \times \mathbf{S}_{\text{eff}}) \right]$$
where $\mathbf{S}_{\text{eff}} = 2\mathbf{S} + \boldsymbol{\sigma}$, $\boldsymbol{\sigma} = \frac{m_2}{m_1}\mathbf{S}_1 + \frac{m_1}{m_2}\mathbf{S}_2$.

Spin-spin interaction:
$$\mathbf{a}_{\text{rel}}^{\text{SS}} = -\frac{3G}{\mu c^2 r^4} \left[ \mathbf{n} (\mathbf{S}_1 \cdot \mathbf{S}_2 - 5(\mathbf{n} \cdot \mathbf{S}_1)(\mathbf{n} \cdot \mathbf{S}_2)) + \mathbf{S}_1(\mathbf{n} \cdot \mathbf{S}_2) + \mathbf{S}_2(\mathbf{n} \cdot \mathbf{S}_1) \right]$$

### 3.4. Gravitational Wave Quadrupole Emission

Trace-free quadrupole moment tensor:
$$I_{ij} = \sum_{a=1}^N m_a \left( x_a^i x_a^j - \frac{1}{3} \delta^{ij} r_a^2 \right)$$

Total radiated power (Peters-Mathews formula):
$$P_{\text{GW}} = \frac{G}{5 c^5} \dddot{I}_{ij} \dddot{I}_{ij}$$

Gravitational wave strain polarizations at distance $D$:
$$h_+ = \frac{G}{c^4 D} (\ddot{I}_{11} - \ddot{I}_{22}), \quad h_\times = \frac{2G}{c^4 D} \ddot{I}_{12}$$

---

## 4. Relativistic Hydrodynamics (GRHD/GRMHD)

### 4.1. Magnetohydrodynamic Energy-Momentum Tensor

$$T^{\mu\nu} = \left(\rho h + b^2\right) u^\mu u^\nu + \left(P + \frac{1}{2}b^2\right) g^{\mu\nu} - b^\mu b^\nu$$
where $\rho$ is rest-mass density, $h = 1 + \epsilon + \frac{P}{\rho}$ is specific enthalpy, $P$ is isotropic pressure, $u^\mu$ is fluid four-velocity ($u_\mu u^\mu = -1$), & $b^\mu$ is the comobile magnetic four-vector ($b^\mu u_\mu = 0$, $b^2 = b^\mu b_\mu$).

### 4.2. Conservative 3+1 Form

$$\partial_t \mathbf{U} + \partial_i \mathbf{F}^i = \mathbf{S}$$
Primitive variables: $\mathbf{P} = (\rho, P, v^x, v^y, v^z, B^x, B^y, B^z)^T$.
Conserved variables: $\mathbf{U} = (D, S_x, S_y, S_z, \tau, B^x, B^y, B^z)^T$ where:
$$D = \rho W, \quad \mathbf{S} = \left(\rho h W^2 + B^2\right)\mathbf{v} - (\mathbf{B} \cdot \mathbf{v})\mathbf{B}, \quad \tau = \rho h W^2 + B^2 - P - \frac{1}{2}b^2 - D$$
Lorentz factor: $W = (1 - v^2)^{-1/2}$.

### 4.3. Equations of State (EOS)

- Ideal Gas Law: $P = (\Gamma - 1)\rho\epsilon$.
- Synge / Mathews Relativistic Monoatomic Gas: $h = \frac{5}{2}\theta + \sqrt{\frac{9}{4}\theta^2 + 1}$ with $\theta = P/\rho$.
- Relativistic Degenerate Fermi Gas: $P(x) = p_0 \left[ x(2x^2 - 3)\sqrt{1+x^2} + 3\operatorname{asinh}(x) \right]$ with $x = ( \rho / \rho_0 )^{1/3}$.
- Tolman-Oppenheimer-Volkoff (TOV) Hydrostatic Balance:
  $$\frac{dP}{dr} = -\frac{G(\epsilon/c^2 + P/c^2)(m + 4\pi r^3 P / c^2)}{r^2 \left(1 - \frac{2Gm}{c^2 r}\right)}, \quad \frac{dm}{dr} = 4\pi r^2 \frac{\epsilon(r)}{c^2}$$

---

## 5. Accretion Disk Physics

### 5.1. Novikov-Thorne Thin Disk Profile

Radiative surface flux $F(r)$:
$$F(r) = \frac{3GM\dot{M}}{8\pi r^3} \frac{f(x)}{x(x^3 + a_*)}, \quad x = \sqrt{\frac{r}{r_g}}$$
Page-Thorne analytical integral $f(x)$:
$$f(x) = x - x_0 - \frac{3}{2}a_* \ln\left(\frac{x}{x_0}\right) - \sum_{i=1}^3 \frac{3(x_i - a_*)^2}{x_i(x_i - x_j)(x_i - x_k)} \ln\left(\frac{x - x_i}{x_0 - x_i}\right)$$
where $x_0 = \sqrt{r_{\text{ISCO}}/r_g}$ & $x_1, x_2, x_3$ are roots of $x^3 - 3x + 2a_* = 0$.

Effective blackbody emission temperature:
$$T_{\text{eff}}(r) = \left( \frac{F(r)}{\sigma_{\text{SB}}} \right)^{1/4}$$

`NovikovThorneDisk` implements this profile. The ray tracer does not evaluate it and uses the parametrized model of [Section 8.2](#82-step-control-termination-and-space-skipping).

---

## 6. Polarized Radiative Transfer

### 6.1. Relativistic Frequency Shift & Invariance

Generalized frequency shift ratio:
$$g = \frac{\nu_{\text{obs}}}{\nu_{\text{emit}}} = \frac{p_\mu u^\mu_{\text{obs}}}{p_\nu u^\nu_{\text{emit}}}$$
Intensity transformation:
$$I_{\text{obs}}(\nu_{\text{obs}}) = g^3 I_{\text{emit}}\left(\frac{\nu_{\text{obs}}}{g}\right), \quad F_{\text{bolometric, obs}} = g^4 F_{\text{bolometric, emit}}$$

### 6.2. Full-Stokes Polarized Transfer Equations

Transport of Stokes vector $\mathbf{S} = (I, Q, U, V)^T$ along affine parameter $\lambda$:
$$\frac{d}{d\lambda} \begin{pmatrix} I \\ Q \\ U \\ V \end{pmatrix} = \begin{pmatrix} j_I \\ j_Q \\ j_U \\ j_V \end{pmatrix} - \begin{pmatrix} \alpha_I & \alpha_Q & \alpha_U & \alpha_V \\ \alpha_Q & \alpha_I & \rho_V & -\rho_U \\ \alpha_U & -\rho_V & \alpha_I & \rho_Q \\ \alpha_V & \rho_U & -\rho_Q & \alpha_I \end{pmatrix} \begin{pmatrix} I \\ Q \\ U \\ V \end{pmatrix}$$

Formal integration step via Delano's analytical matrix exponential method:
$$\mathbf{S}(\lambda + \Delta\lambda) = e^{-\mathbf{K}\Delta\lambda} \mathbf{S}(\lambda) + \mathbf{K}^{-1} \left(\mathbf{I} - e^{-\mathbf{K}\Delta\lambda}\right) \mathbf{j}$$

---

## 7. Uncertainty Quantification Formulation

### 7.1. Interval Arithmetic (IEEE 1788)

$$\mathbf{x} = [\underline{x}, \bar{x}], \quad \mathbf{y} = [\underline{y}, \bar{y}]$$
$$\mathbf{x} + \mathbf{y} = [\underline{x} + \underline{y}, \bar{x} + \bar{y}]$$
$$\mathbf{x} \times \mathbf{y} = [\min(\underline{x}\underline{y}, \underline{x}\bar{y}, \bar{x}\underline{y}, \bar{x}\bar{y}), \max(\underline{x}\underline{y}, \underline{x}\bar{y}, \bar{x}\underline{y}, \bar{x}\bar{y})]$$

### 7.2. Zonotope Enclosure

$$\mathcal{Z} = \mathbf{c} \oplus \sum_{i=1}^p \alpha_i \mathbf{g}_i, \quad \alpha_i \in [-1, 1]$$
Linear transformation $\mathbf{A} \mathcal{Z} = (\mathbf{A}\mathbf{c}) \oplus \sum_{i=1}^p \alpha_i (\mathbf{A}\mathbf{g}_i)$.

### 7.3. Lyapunov Covariance Matrix Propagation

For phase state vector $\mathbf{Y} = (\mathbf{x}, \mathbf{p})^T \in \mathbb{R}^8$ & Jacobian $\mathbf{J} = \frac{\partial \mathbf{f}}{\partial \mathbf{Y}}$:
$$\frac{d\mathbf{\Sigma}}{d\lambda} = \mathbf{J}\mathbf{\Sigma} + \mathbf{\Sigma}\mathbf{J}^T + \mathbf{Q}$$

### 7.4. Generalized Polynomial Chaos Expansion (gPCE)

$$X(\boldsymbol{\xi}) = \sum_{k=0}^{P} X_k \Psi_k(\boldsymbol{\xi})$$
Orthogonality: $\mathbb{E}[\Psi_j(\boldsymbol{\xi})\Psi_k(\boldsymbol{\xi})] = \langle \Psi_j, \Psi_k \rangle \delta_{jk}$.
Statistical moments:
$$\mu = X_0, \quad \sigma^2 = \sum_{k=1}^P X_k^2 \langle \Psi_k, \Psi_k \rangle$$

---

## 8. Image Formation in the Renderer

The formulas of this section are those of `SoftwareComputeEngine` (`include/relativistic/render/software_compute_engine.hpp`) and `ViewportPrimaryWindow`. The metric dependent selection of tracers is listed in [TECHNICAL_MANUAL.md](TECHNICAL_MANUAL.md#metric-usage-in-softwarecomputeengine), and the render pipeline in [ARCHITECTURE.md](ARCHITECTURE.md#29-render-pipeline).

### 8.1. Camera Frame and Initial Conditions

With pitch $p$, yaw $y$ and roll $\rho$ in radians, the camera basis is
$$\mathbf{f} = (\cos p \cos y, \cos p \sin y, \sin p), \quad \mathbf{r}_0 = (\sin y, -\cos y, 0), \quad \mathbf{u}_0 = (-\sin p \cos y, -\sin p \sin y, \cos p)$$
$$\mathbf{r} = \cos\rho \, \mathbf{r}_0 - \sin\rho \, \mathbf{u}_0, \quad \mathbf{u} = \sin\rho \, \mathbf{r}_0 + \cos\rho \, \mathbf{u}_0$$

For pixel $(x, y)$ of a $W \times H$ image, $v = 1 - 2(y + 0.5)/H$ and $u = (2(x + 0.5)/W - 1) W/H$. The all-sky projections (Equirectangular360 and Hammer-Aitoff) use $u = 2(x + 0.5)/W - 1$. The projection returns a local direction $\mathbf{n}_{\text{loc}}$ whose components 0, 2 and 1 are the forward, right and up coefficients, and the world direction is $\mathbf{d} = n_0 \mathbf{f} + n_2 \mathbf{r} + n_1 \mathbf{u}$.

The components of $\mathbf{d}$ on the observer spherical basis $(\mathbf{e}_r, \mathbf{e}_\theta, \mathbf{e}_\phi)$ are $n_r$, $n_\theta$, $n_\phi$. With $f_{\text{obs}} = \max(1 - r_s / r, 10^{-6})$, the Schwarzschild null tracer starts from
$$p_r = -n_r, \quad p_\theta = -\frac{n_\theta}{r \sqrt{f_{\text{obs}}}}, \quad p_\phi = -\frac{n_\phi}{r \sin\theta \sqrt{f_{\text{obs}}}}, \quad E = 1, \quad L_z = p_\phi \, r^2 \sin^2\theta$$
The travel direction of an escaping ray is obtained from the local components $(p_r / \sqrt{f}, \; r p_\theta, \; r \sin\theta \, p_\phi)$ and is used to sample the sky and the bodies. The exact tracer builds a ZAMO tetrad of the selected metric at the observer and constructs the null four-vector with `construct_light_ray`.

### 8.2. Step Control, Termination and Space-Skipping

With $\kappa$ = `step_size_factor`, $s_{ff}$ = `far_field_step_scale`, $r_h$ the horizon radius and $\Delta_{\min}$, $\Delta_{\max}$ the step bounds, the affine step of the fast tracers is
$$F(r) = 1 + (s_{ff} - 1)\,\text{clamp}\left(\frac{r - 20 r_h}{80 r_h}, 0, 1\right), \quad \Delta\lambda = -\text{clamp}\left(\kappa \sqrt{r \max(r - r_h, 0.02 M)} \, F(r), \; \Delta_{\min}, \; \Delta_{\max} s_{ff}\right)$$
The single-precision tracers use $F = 1$ and the bounds $\Delta_{\min}$, $\Delta_{\max}$. The exact tracer multiplies the step by the polar guard $\text{clamp}(12 \lvert \sin\theta \rvert \, s_{\text{pole}}, 0.02, 1)$, and the scalar double-precision tracer multiplies it by $\text{clamp}(h / 8, 0.03, 1)$ when spacetime source bodies exist, where $h$ is the smallest distance to a source horizon expressed in horizon units.

Horizon radius and disk limits of the fast tracers: $r_h = M + \sqrt{M^2 - a^2}$ for $\lvert a \rvert > 10^{-12}$ and $2M$ otherwise, $r_{\text{ISCO}} = 6M$ for $\lvert a \rvert \le 10^{-12}$ and $\max(1.05\,r_h, 6M - 4a)$ otherwise, and an outer disk radius $24 M$. The exact tracer uses the Bardeen ISCO formula of [Section 2.4](#24-kerr-characteristic-radii-and-horizon-quantities) and requires $r_{\text{cross}} > 1.02\,r_h$ for a disk hit.

A ray is absorbed when $r \le 1.0001\,r_h$ or $\Delta_{\text{Kerr}} = r^2 - 2Mr + a^2 \le 0$ (when the metric has a horizon), escapes when $r \ge r_{\text{esc}}$, and stops when the throughput falls to 0.01. A ray that has neither been absorbed nor escaped and has $r < 3 r_s$ after the step budget is treated as absorbed. In the viewport, $r_{\text{esc}} = \max(d_{\text{render}} M, 2 r_{\text{obs}})$ with $d_{\text{render}}$ = `render_distance_scale`, or $10^7$ when the render distance is unbounded. The step budget is `max_integration_steps`, replaced by `lod_reduced_steps` when the level of detail is active and $r_{\text{obs}}$ exceeds `lod_distance_threshold`, halved (minimum 256) for the exact tracer, and divided by $\min(n_{\text{src}} + 1, 8)$ (minimum 256) when $n_{\text{src}}$ spacetime sources exist.

Analytic space-skipping applies to a ray at radius $r > r_{\text{skip}}$, with $r_{\text{skip}} = \max(s M, 1.05 \cdot 24 M)$ when the metric has a disk and $\max(s M, 2 r_h)$ otherwise ($s$ = `space_skip_radius_scale`). For the position $\mathbf{p}$ and unit travel direction $\mathbf{d}$, the impact parameter is $b = \lvert \mathbf{p} \times \mathbf{d} \rvert$. When $b > 0.05 M$, the direction is rotated about $\mathbf{n} = (\mathbf{p} \times \mathbf{d}) / \lvert \mathbf{p} \times \mathbf{d} \rvert$ by
$$\delta = \min\left(\frac{4M}{\max(b, 10^{-6} M)}, 0.6\right), \quad \mathbf{d}' = \mathbf{d} \cos\delta + (\mathbf{n} \times \mathbf{d}) \sin\delta$$
and the ray is moved along $\mathbf{d}'$ to the nearest intersection with the spheres of radius $r_{\text{skip}}$ and $r_{\text{esc}}$.

Disk emission. The disk plane is $\theta = \pi/2$. A crossing between two steps is located by $s = \lvert \theta_{\text{prev}} - \pi/2 \rvert / \lvert \theta - \theta_{\text{prev}} \rvert$, $r_c = r_{\text{prev}} + s (r - r_{\text{prev}})$, and is accepted for $r_{\text{ISCO}} \le r_c \le 24 M$. With $v = \sqrt{M / r_c}$, $\Omega = v / r_c$ and $\gamma = (\max(1 - 3M / r_c, 10^{-4}))^{-1/2}$:
$$g = \frac{\sqrt{\max(1 - r_s / r_c, 10^{-4})}}{\gamma \, (1 - \Omega L_z / E)}, \quad \tau_n = \left(\frac{r_{\text{ISCO}}}{r_c}\right)^{3/4}\left(1 - \sqrt{\frac{r_{\text{ISCO}}}{r_c}}\right)^{1/4}$$
$$T_{\text{obs}} = g \, (T_{\text{scale}} \, \tau_n + T_{\text{floor}}), \quad E_r = \text{clamp}\left(\frac{24M - r_c}{1.5M}, 0, 1\right) \text{clamp}\left(\frac{r_c - r_{\text{ISCO}}}{0.8M}, 0, 1\right)$$
$$I = 1.5 \, \max\left(g^{n} \, \tau_n \, E_r \, \mathcal{T}, 0\right), \quad \mathcal{T} = 1 - 0.12\,\epsilon + 0.12\,\epsilon \sin\left(8\phi_c - 4 \ln\frac{r_c}{r_{\text{ISCO}}}\right)$$
$$\alpha_{\text{disk}} = \text{clamp}(0.95 \, E_r, 0, 0.98), \quad C \leftarrow C + \beta \, I \, \mathbf{c}(T_{\text{obs}}), \quad \beta \leftarrow \beta (1 - \alpha_{\text{disk}})$$
where $\beta$ is the throughput, $n$ = `disk_doppler_beaming_exponent` (default 5.32), $T_{\text{scale}}$ = `disk_temperature_scale_k` (default 23796 K), $T_{\text{floor}}$ = `disk_temperature_floor_k` (default 1200 K), and $\epsilon = \text{clamp}\left(\frac{2 r_h / r_{\text{obs}}}{24 \, \text{FOV} / W}, 0, 1\right)$ attenuates the turbulence term when the black hole covers few pixels. The scalar double-precision and the single-precision tracers use $T_{\text{scale}} = 18000$ K, $T_{\text{floor}} = 1200$ K and $n = 4$.

The color $\mathbf{c}(T)$ is the blackbody spectrum at $T \in [800, 60000]$ K integrated against the CIE 1931 matching functions with 24 samples between 380 and 780 nm, converted to linear sRGB and divided by its largest component. The saturation $\varsigma$ (`disk_color_saturation`) is applied about the luminance $Y = 0.2126 R + 0.7152 G + 0.0722 B$ as $c \leftarrow \max(0, Y + \varsigma (c - Y))$.

### 8.3. Tone Mapping and Color Grading

With exposure $e$ in EV, linear values are scaled by $2^{e}$ and negative values are set to zero. The operators are:

- Mode 0: clamp to $[0, 1]$.
- Mode 1 (ACES approximation): $\text{clamp}\left(\dfrac{x(2.51x + 0.03)}{x(2.43x + 0.59) + 0.14}, 0, 1\right)$ per channel.
- Mode 2 (logarithmic): with luminance $L$, $L' = \dfrac{\log_{10}(1 + 100 L)}{\log_{10}(1 + 10^4)}$ and each channel multiplied by $\text{clamp}(L' / L, 0, 1)$, then clamped to $[0, 1]$.
- Mode 3 (extended Reinhard): $\dfrac{x(1 + x/25)}{1 + x}$ per channel, without clamping.

The result is encoded with the sRGB transfer function ($12.92 x$ for $x \le 0.0031308$, otherwise $1.055 x^{1/2.4} - 0.055$).

The viewport then applies, per channel $c$, when any grading parameter differs from its neutral value: lift $c \leftarrow \text{clamp}(c + \ell (1 - c), 0, 4)$; contrast $c \leftarrow \max((c - 0.5) k + 0.5, 0)$; gamma and gain $c \leftarrow c^{1/\gamma} G$; highlights $c \leftarrow c + h \, w_h (1 - c) / 2$ with $w_h = \text{clamp}((c - 0.6)/0.4, 0, 1)$; shadows $c \leftarrow c + s \, w_s \, c / 2$ with $w_s = \text{clamp}(1 - c/0.4, 0, 1)$; clamp to $[0, 1]$. Saturation is applied about the luminance, and the vignette multiplies the color by $1 - v \, \min(2 (x_n^2 + y_n^2), 1)$, with $x_n, y_n \in [-0.5, 0.5]$ the normalized pixel coordinates.

### 8.4. Celestial Body Shading

A body of radius $R$ and oblateness ratio $o \in [0.1, 5]$ is the ellipsoid $(x/a)^2 + (y/b)^2 + (z/c)^2 = 1$ with $a = b = c = R$ except $c = R\,o$ for the oblate model (0), $c = R / o$ for the prolate model (2), and $b = 0.85 R$, $c = R\,o$ for the triaxial model (3). The nearest positive root of the quadratic in the ray parameter gives the hit point $\mathbf{h}$ (local coordinates $x_l, y_l, z_l$), the normal $\hat{\mathbf{N}} \propto (x_l / a^2, y_l / b^2, z_l / c^2)$ and the surface angles $\theta = \arccos(\text{clamp}(z_l / c, -1, 1))$, $\phi = \operatorname{atan2}(y_l, x_l) + \omega_{\text{rot}} \, t$.

The light direction points from the hit point to the origin, $\hat{\mathbf{L}} = -\mathbf{h} / \lvert \mathbf{h} \rvert$. With roughness $\varrho$, the lit color is
$$C = C_0 \left(a_0 + D (1 - a_0)\right) + E_b + S, \quad D = \max(0, \hat{\mathbf{N}} \cdot \hat{\mathbf{L}}), \quad a_0 = 0.04$$
$$S = 0.6 (1 - \varrho) \max(0, \hat{\mathbf{N}} \cdot \hat{\mathbf{H}})^{4 + 80 (1 - \varrho)}, \quad \hat{\mathbf{H}} \propto \hat{\mathbf{L}} - \hat{\mathbf{d}}$$
where $E_b$ is the emission intensity. $D$ is set to zero when the segment to the origin crosses another body (sphere of radius $R \max(1, o)$) or the horizon sphere, if shadows are enabled. The pixel coverage ratio is $\arcsin(\text{clamp}(R_{\max} / d_{\text{cam}}, 0, 1)) / (\text{FOV} / W)$; below the point threshold the body is a flat dot with $a_0 = 1$, below the LOD threshold the procedural noise, layers, shadows and atmosphere are skipped.

Relativistic factors, with the body velocity components interpreted as fractions of $c$ and $\beta = \min(\lvert \mathbf{v} \rvert, 0.9999)$:
$$\mathcal{D} = \frac{1}{\gamma (1 - \boldsymbol{\beta} \cdot \hat{\mathbf{d}})}, \quad C \leftarrow C \cdot \text{clamp}(\mathcal{D}^3, 0.1, 10), \quad z_g = \sqrt{\max\left(1 - \frac{2 m}{R}, 0.05\right)}$$
with $z_g$ applied to the channels as $(z_g, 0.9 z_g, 0.7 z_g)$. The atmospheric rim with $q = (1 - \hat{\mathbf{N}} \cdot \hat{\mathbf{V}})^3$, thickness $\tau_a$ and color $(\mathbf{c}_a, \alpha_a)$ gives $C \leftarrow C (1 - q \alpha_a) + \mathbf{c}_a \, q \, \tau_a \, g_{\text{atm}} \cdot 2.5$, where $g_{\text{atm}}$ is `body_atmosphere_global_intensity`. Each channel is clamped to $[0, 10]$.

### 8.5. Schematic Overlay Deflection

When the lens approximation is enabled, `SchematicViewRenderer` shifts a projected point by the first-order deflection
$$\delta = \text{clamp}\left(\frac{4M}{b}, 0, 0.35\right), \quad b = \max\left(r_{\text{cam}} \sqrt{1 - \cos^2\chi}, 2.05 M\right)$$
where $\chi$ is the angle between the direction to the point and the direction to the origin. The point is displaced by $\rho \, \delta$ along the unit vector toward the origin, with $\rho$ the distance from the camera to the point.

---

## 9. Motion Script Mathematics

The structures are described in [ARCHITECTURE.md](ARCHITECTURE.md#210-capture-subsystem).

### 9.1. Easing Pipeline

For a progress $x \in [0, 1]$, `EasingSpec::evaluate` applies in order: the input window $x \leftarrow \text{clamp}((x - x_0)/(x_1 - x_0), 0, 1)$; reversal $x \leftarrow 1 - x$; repetition over $N \ge 1$ cycles with optional ping-pong; blending with the identity $y = x + (f(x) - x) \, \beta$; the bias and gain remapping; quantization to $q$ levels $y \leftarrow \text{round}(y q)/q$; the output remapping $y \leftarrow y_0 + (y_1 - y_0) y$; and an optional clamp to $[y_0, y_1]$.

Bias and gain use the Schlick function $S(v, b) = v / \left((1/b - 2)(1 - v) + 1\right)$ with $b$ clamped to $[10^{-4}, 1 - 10^{-4}]$: $y \leftarrow S(y, \text{bias})$, then for gain $g' = 1 - \text{gain}$, $y \leftarrow \tfrac{1}{2} S(2y, g')$ if $y < 0.5$ and $1 - \tfrac{1}{2} S(2 - 2y, g')$ otherwise. Bias and gain are applied only for $0 < y < 1$ and when either differs from 0.5.

Families (quadratic, cubic, quartic, quintic, sinusoidal, exponential, circular, back, elastic, bounce) are defined by an in-function $f_{\text{in}}$, with
$$f_{\text{out}}(x) = 1 - f_{\text{in}}(1 - x), \quad f_{\text{in-out}}(x) = \begin{cases} \tfrac{1}{2} f_{\text{in}}(2x) & x < 0.5 \\ 1 - \tfrac{1}{2} f_{\text{in}}(2 - 2x) & x \ge 0.5 \end{cases}$$
The in-functions are $x^2$, $x^3$, $x^4$, $x^5$, $1 - \cos(\pi x / 2)$, $2^{10x - 10}$, $1 - \sqrt{1 - x^2}$, $(c_1 + 1) x^3 - c_1 x^2$ with $c_1 = 1.70158$, and the standard elastic and bounce expressions.

Other curves: smoothstep $3x^2 - 2x^3$; smootherstep $6x^5 - 15x^4 + 10x^3$; the seventh-order form $x^4 (35 - 84 x + 70 x^2 - 20 x^3)$; power $x^p$; the logistic curve normalized between its values at 0 and 1; the exponential curvature $(e^{kx} - 1)/(e^{k} - 1)$; the damped spring $R(x) / R(1)$ with $R(t) = 1 - e^{-\zeta t} \cos(2\pi n t)$; the wobble $x + A \sin(2\pi n x) \sin(\pi x)$; the cubic Bezier timing curve solved for $x$ by Newton iterations followed by bisection; and a custom curve interpolated by cubic Hermite segments with finite-difference slopes (or linearly when smoothing is disabled).

### 9.2. Position Curves

Waypoint curves use $P(s) = h_{00} P_i + h_{10} m_i + h_{01} P_{i+1} + h_{11} m_{i+1}$ with the cubic Hermite basis $h_{00} = 2s^3 - 3s^2 + 1$, $h_{10} = s^3 - 2s^2 + s$, $h_{01} = -2s^3 + 3s^2$, $h_{11} = s^3 - s^2$, and the tangents $m_i = \tau (P_{i+1} - P_{i-1})$ with $\tau$ the tension (0.5 by default). Closed curves wrap the indices and open curves repeat the end points. With uniform speed, the parameter is converted to a segment index and local coordinate through the chord lengths. The B-spline mode uses the uniform cubic basis
$$b_0 = \tfrac{(1 - s)^3}{6}, \quad b_1 = \tfrac{3 s^3 - 6 s^2 + 4}{6}, \quad b_2 = \tfrac{-3 s^3 + 3 s^2 + 3 s + 1}{6}, \quad b_3 = \tfrac{s^3}{6}$$
Bezier shapes use the Bernstein form of degree 2 or 3.

Parametric shapes with $u \in [0, 1]$ and center $\mathbf{c}$:

| Shape | Position |
| :--- | :--- |
| Spherical orbit | $\mathbf{c} + R(u)\,(\cos\varepsilon \cos\varphi, \cos\varepsilon \sin\varphi, \sin\varepsilon)$ with $R$ and $\varepsilon$ linear in $u$ and $\varphi = \varphi_0 + 2\pi N u$ |
| Logarithmic spiral | $R(u) = R_0 \exp\left(\ln\frac{R_1}{R_0}\, u^{k}\right)$, angle $2\pi N u$, height linear in $u$ |
| Archimedean spiral | $R(u)$ linear in $u$, angle $\varphi_0 + 2\pi N u$, height linear in $u$ |
| Helix | Axis point linear between the axis end points, radius linear in $u$, angle $\varphi_0 + 2\pi N u$ in the plane orthogonal to the axis |
| Lissajous | $c_i + A_i \sin(2\pi f_i u + \varphi_i)$ per axis |
| Torus knot | $\theta = 2\pi T u$, $\varrho = R + r\cos(q\theta)$, position $(\varrho \cos(p\theta), \varrho \sin(p\theta), r \sin(q\theta))$ |
| Lemniscate | $\theta = 2\pi T u$, $\left(\frac{S \cos\theta}{1 + \sin^2\theta}, \frac{S \sin\theta \cos\theta}{1 + \sin^2\theta}, H \sin\theta\right)$ |
| Rose | $\varrho = R \cos(k\theta)$ with $\theta = 2\pi T u$, height $h u$ |
| Epitrochoid | $\left((R + r)\cos\theta - d \cos\frac{(R + r)\theta}{r}, (R + r)\sin\theta - d \sin\frac{(R + r)\theta}{r}\right)$, $\theta = 2\pi T u$ |
| Wave | Linear interpolation between two points plus two sinusoidal offsets with the envelope $e^{-\delta u}$ |

Layers combine as follows with weight $w(u)$ linear between the start and end weights: Replace sets $P = w\,p(u)$, Add sets $P \leftarrow P + w\,p(u)$, and Add Relative sets $P \leftarrow P + w\,(p(u) - p(0))$. Each shape then receives the affine transform $\mathbf{p} \mapsto \mathbf{c}_p + \mathbf{R}(\mathbf{S}(\mathbf{p} - \mathbf{c}_p)) + \mathbf{t}$ with the pivot $\mathbf{c}_p$, scale $\mathbf{S}$, Euler rotation $\mathbf{R}$ and translation $\mathbf{t}$.

A segment transition of duration $T_k$ blends the previous end pose $X_{\text{prev}}$ with the current pose $X$ by $X \leftarrow X_{\text{prev}} + (X - X_{\text{prev}}) \, w(\tau / T_k)$, where $w$ is the easing of the corresponding group (orientation, position or lens). Angular differences use the wrapped difference $\text{wrap}(\Delta) = ((\Delta + 180) \bmod 360) - 180$ in the shortest arc mode.

### 9.3. Sampling and Averaging

For $N$ temporal samples per frame, shutter fraction $\varphi$ and frame interval $\Delta t_f$, the sample times are $t_s = t_f + \left(\frac{s + 0.5}{N} - 0.5\right)\varphi \, \Delta t_f$. With supersampling factor $k$ and fade factor $\Phi$, each output channel is
$$c_{\text{out}} = \text{enc}\left(\frac{\Phi}{k^2 N} \sum_{s=1}^{N} \sum_{i=1}^{k^2} \text{dec}(c_{s,i})\right)$$
where $\text{dec}$ and $\text{enc}$ are the sRGB decoding and encoding functions, implemented with lookup tables of 4096 and 16384 entries. The fade factors are $\min\left(\frac{i + 1}{n_{\text{in}} + 1}, \frac{m}{n_{\text{out}} + 1}\right)$ for frame index $i$ and $m$ remaining frames.

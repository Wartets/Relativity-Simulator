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

Outer and inner event horizon radii:
$$r_\pm = r_g \pm \sqrt{r_g^2 - a^2}$$

Outer and inner ergosphere boundary radii:
$$r_{\text{ergo},\pm}(\theta) = r_g \pm \sqrt{r_g^2 - a^2 \cos^2\theta}$$

#### Kerr-Schild Cartesian Coordinates $(t, x, y, z)$
$$g_{\mu\nu} = \eta_{\mu\nu} + 2 H k_\mu k_\nu, \quad g^{\mu\nu} = \eta^{\mu\nu} - 2 H k^\mu k^\nu$$
$$H = \frac{r_g r^3}{r^4 + a^2 z^2}$$
$$k_\mu = \left(c, \frac{r x + a y}{r^2 + a^2}, \frac{r y - a x}{r^2 + a^2}, \frac{z}{r}\right), \quad k^\mu = \left(-\frac{1}{c}, \frac{r x + a y}{r^2 + a^2}, \frac{r y - a x}{r^2 + a^2}, \frac{z}{r}\right)$$
The radial coordinate $r$ is solved from:
$$r^4 - (x^2 + y^2 + z^2 - a^2) r^2 - a^2 z^2 = 0$$

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

---

## 3. Post-Newtonian (PN) N-Body Dynamics

Equations of motion for $N$ gravitationally interacting bodies:
$$\mathbf{a}_i = \mathbf{a}_i^{\text{Newton}} + \frac{1}{c^2}\mathbf{a}_i^{\text{1PN}} + \frac{1}{c^4}\mathbf{a}_i^{\text{2PN}} + \frac{1}{c^5}\mathbf{a}_i^{\text{2.5PN}} + \frac{1}{c^6}\mathbf{a}_i^{\text{3PN}} + \frac{1}{c^7}\mathbf{a}_i^{\text{3.5PN}} + \mathbf{a}_i^{\text{SO}} + \mathbf{a}_i^{\text{SS}} + \mathbf{a}_i^{\text{Harmonics}}$$

### 3.1. Conservative 1PN Acceleration (Einstein-Infeld-Hoffmann)

$$\mathbf{a}_i^{\text{1PN}} = \sum_{j \neq i} \frac{G m_j \mathbf{n}_{ji}}{r_{ij}^2} \left[ v_i^2 + 2v_j^2 - 4\mathbf{v}_i \cdot \mathbf{v}_j - \frac{3}{2}(\mathbf{n}_{ij} \cdot \mathbf{v}_j)^2 - 4\sum_{k \neq i} \frac{G m_k}{r_{ik}} - \sum_{k \neq j} \frac{G m_k}{r_{jk}} \right] + \sum_{j \neq i} \frac{G m_j \mathbf{v}_{ij}}{r_{ij}^2} \left[ \mathbf{n}_{ji} \cdot (4\mathbf{v}_i - 3\mathbf{v}_j) \right] - \sum_{j \neq i} \sum_{k \neq j} \frac{7 G^2 m_j m_k \mathbf{n}_{kj}}{2 r_{ij} r_{jk}^2}$$

### 3.2. 2.5PN Radiation Reaction Acceleration

For binary separation $\mathbf{r} = \mathbf{r}_1 - \mathbf{r}_2$, $\eta = \frac{m_1 m_2}{(m_1+m_2)^2}$, $M = m_1 + m_2$:
$$\mathbf{a}_{\text{rel}}^{\text{2.5PN}} = \frac{8}{5} \frac{G^2 M^2 \eta}{c^5 r^3} \left[ \left(3v^2 + \frac{17}{3}\frac{GM}{r}\right) \dot{r} \mathbf{n} - \left(v^2 + 3\frac{GM}{r}\right) \mathbf{v} \right]$$

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
where $\rho$ is rest-mass density, $h = 1 + \epsilon + \frac{P}{\rho}$ is specific enthalpy, $P$ is isotropic pressure, $u^\mu$ is fluid four-velocity ($u_\mu u^\mu = -1$), and $b^\mu$ is the comobile magnetic four-vector ($b^\mu u_\mu = 0$, $b^2 = b^\mu b_\mu$).

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
  $$\frac{dP}{dr} = -\frac{G(\epsilon + P)(m + 4\pi r^3 P / c^2)}{r^2 \left(1 - \frac{2Gm}{c^2 r}\right)}$$

---

## 5. Accretion Disk Physics

### 5.1. Novikov-Thorne Thin Disk Profile

Radiative surface flux $F(r)$:
$$F(r) = \frac{3GM\dot{M}}{8\pi r^3} \frac{f(x)}{x(x^3 + a_*)}, \quad x = \sqrt{\frac{r}{r_g}}$$
Page-Thorne analytical integral $f(x)$:
$$f(x) = x - x_0 - \frac{3}{2}a_* \ln\left(\frac{x}{x_0}\right) - \sum_{i=1}^3 \frac{3(x_i - a_*)^2}{x_i(x_i - x_j)(x_i - x_k)} \ln\left(\frac{x - x_i}{x_0 - x_i}\right)$$
where $x_0 = \sqrt{r_{\text{ISCO}}/r_g}$ and $x_1, x_2, x_3$ are roots of $x^3 - 3x + 2a_* = 0$.

Effective blackbody emission temperature:
$$T_{\text{eff}}(r) = \left( \frac{F(r)}{\sigma_{\text{SB}}} \right)^{1/4}$$

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

For phase state vector $\mathbf{Y} = (\mathbf{x}, \mathbf{p})^T \in \mathbb{R}^8$ and Jacobian $\mathbf{J} = \frac{\partial \mathbf{f}}{\partial \mathbf{Y}}$:
$$\frac{d\mathbf{\Sigma}}{d\lambda} = \mathbf{J}\mathbf{\Sigma} + \mathbf{\Sigma}\mathbf{J}^T + \mathbf{Q}$$

### 7.4. Generalized Polynomial Chaos Expansion (gPCE)

$$X(\boldsymbol{\xi}) = \sum_{k=0}^{P} X_k \Psi_k(\boldsymbol{\xi})$$
Orthogonality: $\mathbb{E}[\Psi_j(\boldsymbol{\xi})\Psi_k(\boldsymbol{\xi})] = \langle \Psi_j, \Psi_k \rangle \delta_{jk}$.
Statistical moments:
$$\mu = X_0, \quad \sigma^2 = \sum_{k=1}^P X_k^2 \langle \Psi_k, \Psi_k \rangle$$

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

The Kerr geometry is implemented in `include/relativistic/metrics/kerr.hpp` and `include/relativistic/metrics/kerr_schild.hpp` ([Technical Manual](TECHNICAL_MANUAL.md#22-spacetime-metric-implementations)).

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
where $R^2 = x^2 + y^2 + z^2$. Metric selection rules in the rendering pipeline are detailed in [TECHNICAL_MANUAL.md](TECHNICAL_MANUAL.md#metric-usage-in-softwarecomputeengine).

#### Multiple Kerr Black Holes in Kerr-Schild Coordinates
For a spacetime containing $N_{\text{src}} \le 16$ black holes located at positions $\mathbf{x}_i$, each with mass $M_i$ and spin parameter $a_i$ oriented along local unit axis $\hat{\mathbf{s}}_i$, the metric tensor is superposed in Cartesian coordinates:
$$g_{\mu\nu} = \eta_{\mu\nu} + 2 \sum_{i=1}^{N_{\text{src}}} H_i \, k^{(i)}_\mu k^{(i)}_\nu, \quad g^{\mu\nu} = \eta^{\mu\nu} - 2 \sum_{i=1}^{N_{\text{src}}} H_i \, k^{(i)\mu} k^{(i)\nu}$$
where the local null vector $k^{(i)}_\mu$ and scalar potential $H_i$ are evaluated from the relative coordinate vector $\mathbf{x}_l = \mathbf{R}_i (\mathbf{x} - \mathbf{x}_i)$ in the local orthonormal frame of source $i$:
$$r_i^2 = \frac{1}{2}\left(R_i^2 - a_i^2 + \sqrt{(R_i^2 - a_i^2)^2 + 4 a_i^2 z_{l, i}^2}\right), \quad R_i^2 = x_{l, i}^2 + y_{l, i}^2 + z_{l, i}^2$$
$$H_i = \frac{M_i r_i^3}{r_i^4 + a_i^2 z_{l, i}^2}, \quad \mathbf{k}^{(i)} = \left( \frac{r_i x_{l, i} + a_i y_{l, i}}{r_i^2 + a_i^2}, \frac{r_i y_{l, i} - a_i x_{l, i}}{r_i^2 + a_i^2}, \frac{z_{l, i}}{r_i} \right)$$
The Hamiltonian null geodesic equations of motion evaluate:
$$\frac{d\mathbf{x}}{d\lambda} = \mathbf{p} - 2 \sum_{i=1}^{N_{\text{src}}} H_i s_i \mathbf{R}_i^T \mathbf{k}^{(i)}$$
$$\frac{d\mathbf{p}}{d\lambda} = \sum_{i=1}^{N_{\text{src}}} \mathbf{R}_i^T \left[ s_i^2 \nabla H_i + 2 H_i s_i \nabla (\mathbf{k}^{(i)} \cdot \mathbf{p}_l) \right]$$
where $s_i = 1 + \mathbf{k}^{(i)} \cdot \mathbf{p}_l$, $\mathbf{p}_l = \mathbf{R}_i \mathbf{p}$, and the spatial gradients evaluate analytically:
$$\nabla H_i = \frac{M_i r_i^2 (3 a_i^2 z_{l, i}^2 - r_i^4)}{(r_i^4 + a_i^2 z_{l, i}^2)^2} \nabla r_i - \frac{2 M_i a_i^2 r_i^3 z_{l, i}}{(r_i^4 + a_i^2 z_{l, i}^2)^2} \hat{\mathbf{e}}_z$$
$$\nabla r_i = \frac{1}{r_i^4 + a_i^2 z_{l, i}^2} \begin{pmatrix} r_i^3 x_{l, i} \\ r_i^3 y_{l, i} \\ r_i (r_i^2 + a_i^2) z_{l, i} \end{pmatrix}$$
$$\nabla(\mathbf{k}^{(i)} \cdot \mathbf{p}_l) = \begin{pmatrix}
\frac{(\partial_x r_i x_{l, i} + r_i)(r_i^2 + a_i^2) - 2 r_i \partial_x r_i (r_i x_{l, i} + a_i y_{l, i})}{(r_i^2 + a_i^2)^2} p_{l, x} + \frac{(\partial_x r_i y_{l, i} - a_i)(r_i^2 + a_i^2) - 2 r_i \partial_x r_i (r_i y_{l, i} - a_i x_{l, i})}{(r_i^2 + a_i^2)^2} p_{l, y} - \frac{z_{l, i} \partial_x r_i}{r_i^2} p_{l, z} \\
\frac{(\partial_y r_i x_{l, i} + a_i)(r_i^2 + a_i^2) - 2 r_i \partial_y r_i (r_i x_{l, i} + a_i y_{l, i})}{(r_i^2 + a_i^2)^2} p_{l, x} + \frac{(\partial_y r_i y_{l, i} + r_i)(r_i^2 + a_i^2) - 2 r_i \partial_y r_i (r_i y_{l, i} - a_i x_{l, i})}{(r_i^2 + a_i^2)^2} p_{l, y} - \frac{z_{l, i} \partial_y r_i}{r_i^2} p_{l, z} \\
\frac{\partial_z r_i x_{l, i} (r_i^2 + a_i^2) - 2 r_i \partial_z r_i (r_i x_{l, i} + a_i y_{l, i})}{(r_i^2 + a_i^2)^2} p_{l, x} + \frac{\partial_z r_i y_{l, i} (r_i^2 + a_i^2) - 2 r_i \partial_z r_i (r_i y_{l, i} - a_i x_{l, i})}{(r_i^2 + a_i^2)^2} p_{l, y} + \frac{r_i - z_{l, i} \partial_z r_i}{r_i^2} p_{l, z}
\end{pmatrix}$$

Initial observer 4-momentum is lifted from the local camera orthonormal triad $\mathbf{e}_{(a)}$ into the superposed metric via Gram-Schmidt orthonormalization:
$$\mathbf{e}_{(0)} = \frac{1}{\sqrt{1 - 2 \sum_i H_i}} \begin{pmatrix} 1 \\ 2 \sum_i H_i \mathbf{k}^{(i)} \end{pmatrix}, \quad \mathbf{e}_{(a)} = \begin{pmatrix} 2 \sum_i H_i (\mathbf{k}^{(i)} \cdot \mathbf{v}_{(a)}) \\ \mathbf{v}_{(a)} \end{pmatrix}$$
followed by normalization $g_{\mu\nu} e^\mu_{(a)} e^\nu_{(b)} = \eta_{ab}$ and null ray construction $\mathbf{p} = \mathbf{e}_{(0)} - \sum_{a=1}^3 n^{(a)} \mathbf{e}_{(a)}$.

Along each numerical step from $\mathbf{x}_{\text{prev}}$ to $\mathbf{x}$, the local integration step size $\Delta\lambda$ is dynamically regulated across all $N_{\text{src}}$ sources:
$$\Delta\lambda = -\operatorname{clamp}\left( \kappa \cdot s_{\text{scale}} \cdot f_{\text{far}}, \; \Delta_{\min}, \; \Delta_{\max} f_{\text{far}} \right)$$
where the multi-center proximity scale $s_{\text{scale}}$ and far-field multiplier $f_{\text{far}}$ evaluate:
$$r_{\text{dist}, i} = \max\left( r_i - r_{h, i}, \; 0.005 M_i \right)$$
$$s_{\text{scale}} = \min_{1 \le i \le N_{\text{src}}} \left( \sqrt{r_i r_{\text{dist}, i}}, \; \max(0.35 r_{\text{dist}, i}, \; \Delta_{\min}) \right)$$
$$f_{\text{far}} = 1 + (s_{ff} - 1) \min_{1 \le i \le N_{\text{src}}} \operatorname{clamp}\left( \frac{r_i - 20 r_{h, i}}{80 r_{h, i}}, \; 0, \; 1 \right)$$

Along each ray segment from $\mathbf{x}_{\text{prev}}$ to $\mathbf{x}$, analytical horizon absorption and accretion disk crossings are resolved in the local orthonormal frame of each source $i$:
1. Local coordinate transformation: $\mathbf{x}_l^{(i)} = \mathbf{R}_i (\mathbf{x} - \mathbf{x}_i)$.
2. Event horizon entry detection: The outer event horizon is the oblate ellipsoid $(x_l / r_{h,\text{eq}})^2 + (y_l / r_{h,\text{eq}})^2 + (z_l / r_{h,\text{pol}})^2 \le 1$, where $r_{h,\text{pol}} = r_{h, i} = M_i + \sqrt{M_i^2 - a_i^2}$ and $r_{h,\text{eq}} = \sqrt{r_{h, i}^2 + a_i^2}$. Ray segment intersection evaluates the quadratic equation in parameter $t \in [0, 1]$:
   $$\|\mathbf{s} + t \mathbf{d}\|^2 \le 1, \quad \mathbf{s} = \mathbf{x}_{l, \text{prev}}^{(i)} \oslash \mathbf{r}_{h, i}, \quad \mathbf{d} = (\mathbf{x}_l^{(i)} - \mathbf{x}_{l, \text{prev}}^{(i)}) \oslash \mathbf{r}_{h, i}$$
   If discriminant $q_b^2 - 4 q_a q_c \ge 0$, the earliest entry parameter $t \in [0, 1]$ marks horizon absorption with priority over subsequent crossings.
3. Accretion disk plane crossing: Evaluated when $z_l^{(i)}(\mathbf{x}_{\text{prev}}) \cdot z_l^{(i)}(\mathbf{x}) \le 0$, yielding crossing parameter $s = |z_{l, \text{prev}}^{(i)}| / |z_l^{(i)} - z_{l, \text{prev}}^{(i)}|$, crossing radius $r_c = \sqrt{x_c^2 + y_c^2 - a_i^2}$, local azimuth $\phi_c = \operatorname{atan2}(y_c, x_c) - \operatorname{atan2}(a_i, r_c)$, and normalized Keplerian angular momentum $L_{z, i} = (\mathbf{x}_l \times \mathbf{p}_l)_z / \|\mathbf{p}_l\|$.
4. Multiple crossings along a single step are sorted by crossing parameter $t \in [0, 1]$ and accumulated in order until absorbed or escaped.

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

### 1.10. Lemaitre-Tolman-Bondi Void Metric

Spherically symmetric inhomogeneous dust solution in synchronous comoving coordinates $(t, r, \theta, \phi)$:
$$ds^2 = -c^2 dt^2 + \frac{(R'(r, t))^2}{1 + k(r)} dr^2 + R^2(r, t) \left(d\theta^2 + \sin^2\theta \, d\phi^2\right)$$
where $R(r, t)$ is the areal radius, $R'(r, t) = \partial_r R(r, t)$, and $k(r) = -2E(r)$ denotes local spatial curvature.

#### Spatial Density Profile & Curvature Function
The spatial profile models a spherical void parameterized by asymptotic Hubble constant $H_0$, void radius $r_0$, transition half-width $w$, inner density parameter $\Omega_{\text{in}}$, and outer density parameter $\Omega_{\text{out}}$:
$$\Omega(r) = \Omega_{\text{out}} + (\Omega_{\text{in}} - \Omega_{\text{out}}) \frac{1 - \tanh\left(\frac{r - r_0}{2w}\right)}{1 + \tanh\left(\frac{r_0}{2w}\right)}$$
$$\Omega'(r) = (\Omega_{\text{in}} - \Omega_{\text{out}}) \frac{-(1 - \tanh^2\left(\frac{r - r_0}{2w}\right)) / (2w)}{1 + \tanh\left(\frac{r_0}{2w}\right)}$$
Curvature function $k(r)$ and its radial gradient evaluate:
$$k(r) = H_0^2 \left(1 - \Omega(r)\right) r^2, \quad k'(r) = H_0^2 \left[-\Omega'(r) r^2 + 2(1 - \Omega(r)) r\right]$$

#### Shell Age & Bang-Time Function
For shell parameter $d = 1 - \Omega(r)$, the cosmological age $t_{\text{age}}(\Omega)$ satisfies:
$$t_{\text{age}}(\Omega) = \begin{cases} \frac{2}{3 H_0} & \text{if } |d| < 10^{-9} \\ \frac{\Omega}{2 H_0 d^{3/2}} (\sinh\eta - \eta), \quad \cosh\eta = \frac{2 - \Omega}{\Omega} & \text{if } d > 0 \\ \frac{\Omega}{2 H_0 (-d)^{3/2}} (\eta - \sin\eta), \quad \cos\eta = \frac{2 - \Omega}{\Omega} & \text{if } d < 0 \end{cases}$$
Present coordinate epoch evaluates $t_0 = \max(t_{\text{age}}(\Omega_{\text{in}}), t_{\text{age}}(\Omega_{\text{out}}))$, yielding non-simultaneous big-bang time:
$$t_B(r) = t_0 - t_{\text{age}}(\Omega(r))$$

#### Exact Shell Evolution & Expansion Velocity
For comoving shell $r$ at coordinate time $t$, elapsed shell time evaluates $\tau = t - t_B(r)$. For $\tau > 0$, areal radius $R(r, t)$ and expansion velocity $\dot{R}(r, t) = \partial_t R(r, t)$ evaluate:
$$R(r, t) = \begin{cases} r \left(\frac{9}{4} H_0^2 \tau^2\right)^{1/3} & \text{if } |d| < 10^{-9} \\ \frac{r \Omega}{2 d} (\cosh\eta - 1), \quad \sinh\eta - \eta = \frac{2 H_0 d^{3/2}}{\Omega} \tau & \text{if } d > 0 \\ \frac{r \Omega}{2 (-d)} (1 - \cos\eta), \quad \eta - \sin\eta = \frac{2 H_0 (-d)^{3/2}}{\Omega} \tau & \text{if } d < 0 \end{cases}$$
$$\dot{R}(r, t) = \sqrt{\max\left( \frac{H_0^2 \Omega(r) r^3}{R(r, t)} + k(r), \; 0 \right)}$$
Spatial derivatives $R'(r, t)$, $\dot{R}'(r, t)$, and $R''(r, t)$ are evaluated via 4th-order centered finite difference stencils on shell states.

---

### 1.11. Quintessence Dark Energy Cosmology

FLRW background governed by dynamic scalar field $\phi(t)$ with self-interaction potential $V(\phi)$:
$$ds^2 = -c^2 dt^2 + a^2(t) \left( dx^2 + dy^2 + dz^2 \right)$$
Equations of motion in e-fold time $N = \ln a$ spanning $N \in [\ln a_{\min}, \ln a_{\max}] = [-6.9078, 1.3863]$:
$$\frac{d\phi}{dN} = \phi', \quad \frac{d\phi'}{dN} = -\left(3 + \frac{1}{2} \frac{d\ln H^2}{dN}\right) \phi' - \frac{V'(\phi)}{H^2}$$
Friedmann constraint:
$$H^2(N) = \frac{\rho_m(N) + \rho_r(N) + V(\phi)}{3 - \frac{1}{2} (\phi')^2}$$
where $\rho_m(N) = 3 \Omega_m e^{-3N}$ and $\rho_r(N) = 3 \Omega_r e^{-4N}$. The logarithmic slope satisfies:
$$\frac{d\ln H^2}{dN} = -\frac{\rho_m + \frac{4}{3}\rho_r}{H^2} - (\phi')^2$$

#### Analytical Quintessence Potentials
1. `Exponential` (0):
   $$V(\phi) = V_0 \exp(-\lambda \phi), \quad V'(\phi) = -\lambda V_0 \exp(-\lambda \phi)$$
2. `InversePowerLaw` (1, Ratra-Peebles):
   $$V(\phi) = V_0 \phi^{-\alpha}, \quad V'(\phi) = -\alpha V_0 \phi^{-\alpha - 1}$$
3. `PseudoNambuGoldstone` (2, PNGB):
   $$V(\phi) = V_0 \left[ 1 + \cos\left(\frac{\phi}{f}\right) \right], \quad V'(\phi) = -\frac{V_0}{f} \sin\left(\frac{\phi}{f}\right)$$
4. `Supergravity` (3, SUGRA):
   $$V(\phi) = V_0 \phi^{-\alpha} \exp\left(\frac{1}{2}\phi^2\right), \quad V'(\phi) = V_0 \phi^{-\alpha} \exp\left(\frac{1}{2}\phi^2\right) \left(\phi - \frac{\alpha}{\phi}\right)$$

#### Shooting Boundary Method & Cosmological Diagnostics
Amplitude $V_0$ is solved via 90-iteration bisection over $\ln V_0 \in [\ln 10^{-8}, \ln 10^8]$ enforcing $H^2(N = 0) = 1.0 \pm 10^{-5}$ at $a = 1$. The background state yields:
$$w_{\phi}(N) = \frac{\frac{1}{2} H^2 (\phi')^2 - V(\phi)}{\frac{1}{2} H^2 (\phi')^2 + V(\phi)}$$
$$q_0 = -1 - \left. \frac{1}{2} \frac{d\ln H^2}{dN} \right|_{N=0}, \quad t_0 H_0 = \int_{\ln a_{\min}}^0 \frac{e^{-N}}{H(N)} dN$$

---

### 1.12. Chaplygin Gas Cosmology

Unified dark fluid model governed by equation of state $P_{\text{ch}} = -A / \rho_{\text{ch}}^\alpha$. In dimensionless variables parameterized by stiffness $A_s \in [0, 1]$ and exponent $\alpha \ge 0$:
$$\rho_{\text{ch}}(N) = \rho_{\text{ch}, 0} \left[ A_s + (1 - A_s) e^{-3(1+\alpha)N} \right]^{\frac{1}{1+\alpha}}$$
Hubble expansion rate:
$$H(N) = H_0 \sqrt{\Omega_m e^{-3N} + \Omega_r e^{-4N} + \Omega_{\text{fluid}} \left[ A_s + (1 - A_s) e^{-3(1+\alpha)N} \right]^{\frac{1}{1+\alpha}}}$$
where $\Omega_{\text{fluid}} = 1 - \Omega_m - \Omega_r$ and present equation of state evaluates $w_{\text{today}} = -A_s$.

---

### 1.13. Semiclassical Hawking Evaporation & Unruh Thermometry

#### Kerr Horizon Thermodynamics
For black hole of mass $M$ and dimensionless spin $a_* = a / r_g \in (-1, 1)$, horizon geometry evaluates:
$$r_g = \frac{GM}{c^2}, \quad r_\pm = r_g \pm \sqrt{r_g^2 - a^2}, \quad A_H = 4\pi (r_+^2 + a^2)$$
Horizon surface gravity $\kappa$ and Hawking temperature $T_H$:
$$\kappa = \frac{c^2 (r_+ - r_-)}{2 (r_+^2 + a^2)}, \quad T_H = \frac{\hbar \kappa}{2\pi c k_B}$$

#### Stress-Energy Models & Radiated Power
1. `BlackbodyGreybody` (0): Horizon emission across $g_{\text{eff}}$ degrees of freedom with greybody factor $\Gamma$:
   $$L_H = \frac{1}{2} g_{\text{eff}} \Gamma \sigma_{\text{SB}} A_H T_H^4$$
2. `PolyakovSWave` (1): Two-dimensional renormalized stress tensor in the Unruh state:
   $$L_H = g_{\text{eff}} \Gamma \frac{\hbar \kappa^2}{48\pi c^2}$$
Degrees of freedom $g_{\text{eff}}$ evaluate $2.0$ for `PhotonsOnly`, $9.25$ for `MasslessStandardModel` (photons: $2$, 3 neutrino flavours: $3 \times 2 \times \frac{7}{8} = 5.25$, gravitons: $2$), or user-configured values.

#### Coupled Mass and Spin Evolution
Mass loss rate and secular spin-down governed by spin-down ratio $k_{\text{spin}}$:
$$\frac{dM}{dt} = -\frac{L_H}{c^2}, \quad \frac{da_*}{dt} = (k_{\text{spin}} - 2) a_* \frac{1}{M} \frac{dM}{dt}$$
Integrated with adaptive RK4 until reaching remnant threshold $M \le M_{\text{min}}$. Black hole lifetime:
$$\tau_{\text{evap}} = \frac{M^3 - M_{\text{min}}^3}{3 C_H}, \quad C_H = -M^2 \frac{dM}{dt}$$

#### Unruh Acceleration & Detector Excitation Rate
For proper acceleration $a_{\text{proper}}$, Unruh temperature evaluates:
$$T_U = \frac{\hbar a_{\text{proper}}}{2\pi c k_B}$$
For a static observer at radius $r > 2GM/c^2$ in Schwarzschild spacetime:
$$a_{\text{proper}}(r) = \frac{GM}{r^2 \sqrt{1 - \frac{2GM}{c^2 r}}}, \quad T_{\text{local}}(r) = \frac{T_H}{\sqrt{1 - \frac{2GM}{c^2 r}}}$$
An Unruh-DeWitt monopole detector with energy gap $\Delta E = \hbar \omega$ and coupling $g_c$ evaluates excitation rate:
$$\dot{\mathcal{F}}(\Delta E) = \frac{g_c^2 \omega}{2\pi \left( \exp\left(\frac{2\pi c \omega}{a_{\text{proper}}}\right) - 1 \right)}$$

### 1.14. TeVeS (Tensor-Vector-Scalar) Metric

Physical metric $\tilde{g}_{\mu\nu}$ conformal-disformal relation to Einstein metric $g_{\mu\nu}$:
$$\tilde{g}_{\mu\nu} = e^{-2\phi} g_{\mu\nu} - 2\sinh(2\phi) U_\mu U_\nu$$
where $\phi$ is the dynamical scalar field and $U^\mu$ is the unit timelike 4-vector field.

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

### 2.5. Subsidiary Spacetime Source Deflections & Capture Radii

For celestial bodies designated as spacetime sources (`is_spacetime_source == true` or `preset_3d == 8`) operating within scalar ray tracing passes, weak-field ray deflections evaluate the total deflection angle $\delta$:
$$\delta = \min\left( \frac{4 M_i}{b} \left(1 + \frac{15\pi M_i}{16 b}\right) - \frac{2 a_i M_i (\hat{\mathbf{s}}_i \cdot \hat{\mathbf{n}})}{b^2}, \; 0.5 \right) \frac{\Delta\lambda}{r_i^2}$$
where $\mathbf{r}_i = \mathbf{x} - \mathbf{x}_i$, $b = \|\mathbf{r}_i - (\mathbf{r}_i \cdot \hat{\mathbf{d}})\hat{\mathbf{d}}\|$ is the impact parameter, and $\hat{\mathbf{n}} = (\mathbf{r}_i \times \hat{\mathbf{d}}) / \|\mathbf{r}_i \times \hat{\mathbf{d}}\|$.

The ray travel direction rotates about $\hat{\mathbf{n}}$ via Rodrigues' rotation formula:
$$\hat{\mathbf{d}}' = \hat{\mathbf{d}} \cos\delta + (\hat{\mathbf{n}} \times \hat{\mathbf{d}}) \sin\delta$$

When the ray enters strong field $r_i < \max(r_{\text{disk,out}, i}, 40 r_{h, i})$, the trajectory is integrated through local Kerr-Schild equations of motion. A ray segment intersecting the critical shadow capture sphere of radius
$$r_{\text{cap}, i} = \max\left( 0.9 M_i \left(5.1961524 - 0.55 a_{*, i}^2\right), \; 1.05 \sqrt{r_{h, i}^2 + a_i^2} \right)$$
is immediately absorbed (`PixelFlags::HORIZON_ABSORBED`).

---

## 3. Post-Newtonian (PN) N-Body Dynamics

Post-Newtonian accelerations are evaluated by `PostNewtonianSolver` (`include/relativistic/dynamics/pn_acceleration.hpp`) and integrated by `PostNewtonianSystem` (`include/relativistic/dynamics/pn_nbody_system.hpp`; see [Technical Manual](TECHNICAL_MANUAL.md#24-post-newtonian-multi-body-subsystem) and [State Advance](ARCHITECTURE.md#34-orchestrator-state-advance)).

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
Primitive variables:
$$\mathbf{P} = \left(\rho, \; p, \; v^x, \; v^y, \; v^z, \; B^x, \; B^y, \; B^z\right)^T$$
Conserved variables:
$$\mathbf{U} = \left(D, \; S_x, \; S_y, \; S_z, \; \tau, \; B^x, \; B^y, \; B^z\right)^T$$
where:
$$W = \frac{1}{\sqrt{1 - \|\mathbf{v}\|^2}}, \quad D = \rho W, \quad z = \rho h W^2$$
The comobile magnetic four-vector $b^\mu$ and scalar magnetic energy density $b^2 = b_\mu b^\mu$ evaluate:
$$b^0 = W (\mathbf{B} \cdot \mathbf{v}), \quad \mathbf{b} = \frac{\mathbf{B}}{W} + b^0 \mathbf{v}, \quad b^2 = \frac{\|\mathbf{B}\|^2}{W^2} + (\mathbf{B} \cdot \mathbf{v})^2$$
Total isotropic pressure:
$$P_{\text{tot}} = p + \frac{1}{2} b^2$$
Conserved momentum and energy densities:
$$\mathbf{S} = \left( z + \|\mathbf{B}\|^2 \right) \mathbf{v} - (\mathbf{B} \cdot \mathbf{v}) \mathbf{B}$$
$$\tau = z + \|\mathbf{B}\|^2 - P_{\text{tot}} - D$$

In 1D Cartesian geometry along the $x$-axis, the physical flux vector $\mathbf{F}^x$ evaluates:
$$\mathbf{F}^x = \begin{pmatrix}
D v_x \\
S_x v_x + P_{\text{tot}} - \frac{B_x^2}{W^2} - (\mathbf{B} \cdot \mathbf{v}) B_x v_x \\
S_y v_x - \frac{B_x B_y}{W^2} - (\mathbf{B} \cdot \mathbf{v}) B_y v_x \\
S_z v_x - \frac{B_x B_z}{W^2} - (\mathbf{B} \cdot \mathbf{v}) B_z v_x \\
S_x - D v_x \\
0 \\
B_y v_x - B_x v_y \\
B_z v_x - B_x v_z
\end{pmatrix}$$

---

### 4.3. Equations of State (EOS)

The equation of state relates rest-mass density $\rho$ and specific internal energy $\varepsilon$ to isotropic pressure $p$, enthalpy $h$, and sound speed $c_s$:
- Relativistic Ideal Gas (Gamma-Law):
  $$p = (\Gamma - 1)\rho \varepsilon, \quad h = 1 + \frac{\Gamma p}{(\Gamma - 1)\rho}, \quad c_s^2 = \frac{\Gamma p}{\rho h}$$
- Synge / Mathews Relativistic Gas:
  $$h = \frac{5}{2}\theta + \sqrt{\frac{9}{4}\theta^2 + 1}, \quad \theta = \frac{p}{\rho}, \quad p(h) = \rho \frac{h^2 - 1}{5h}$$
  Derivative and sound speed in Synge fluid:
  $$\frac{dh}{d\theta} = \frac{5}{2} + \frac{2.25\theta}{\sqrt{2.25\theta^2 + 1}}, \quad c_s^2 = \frac{\theta}{h \left( \frac{dh}{d\theta} - \frac{1}{h} \right)}$$
- Mathews Relativistic Kinetic Gas:
  $$h = \frac{5}{2}\theta + \sqrt{\frac{9}{4}\theta^2 + 1}, \quad \theta = \frac{p}{\rho}, \quad p(h) = \rho \frac{h^2 - 1}{5h}$$
  Sound speed in Mathews fluid:
  $$c_s^2 = \frac{5\theta \left( h - \frac{1}{h} \right)}{3h \sqrt{2.25\theta^2 + 1}}$$
- Polytropic Fluid:
  $$p = K \rho^\Gamma, \quad \varepsilon = \rho + \frac{p}{\Gamma - 1}, \quad h = 1 + \frac{\Gamma K}{\Gamma - 1}\rho^{\Gamma - 1}, \quad c_s^2 = \frac{\Gamma p}{\rho h}, \quad \rho(p) = \left( \frac{p}{K} \right)^{\frac{1}{\Gamma}}$$

---

### 4.4. Conservative-to-Primitive Variable Inversion Formulation

The inversion from $\mathbf{U}$ to $\mathbf{P}$ is executed by `Con2PrimSolver` (`include/relativistic/hydro/solvers/con2prim.hpp`).

#### Unmagnetized Relativistic Hydrodynamics (GRHD)
Given conserved variables $(D, \mathbf{S}, \tau)$, let $S^2 = \|\mathbf{S}\|^2$ and define the auxiliary scalar $z = \tau + p + D$. The 3-velocity squared is $v^2(p) = S^2 / z^2$, and the Lorentz factor satisfies $W = (1 - v^2)^{-1/2}$ subject to velocity clamping $v^2 \le 1 - 10^{-15}$. Specific enthalpy evaluates $h = z / (D W)$ and rest-mass density evaluates $\rho = D / W$.
The scalar nonlinear equation for pressure $p$ evaluates:
$$f(p) = \Gamma p - (\Gamma - 1) \left( \frac{z}{W^2} - \frac{D}{W} \right) = 0$$
with exact analytical derivative:
$$\frac{df}{dp} = \Gamma - (\Gamma - 1) v^2 \left( 1 - \frac{1}{\max(h, 10^{-15})} \right)$$
Starting from initial guess $p_0 = \max(p_{\text{floor}}, p_{\text{EOS}}(D, \tau / D))$ and bounds $p_{\min} = p_{\text{floor}} = 10^{-15}$, $p_{\max} = \max(\tau + D, 100 p_0)$, iterates update via:
$$p_{k+1} = p_k - \frac{f(p_k)}{df/dp}$$
If $|df/dp| < 10^{-8}$, the denominator is regularized to $1.0$. If $p_{k+1} < p_{\min}$, $p_{k+1} > p_{\max}$, or $p_{k+1}$ is non-finite, the step falls back to bisection:
$$p_{k+1} = \frac{1}{2}(p_{\min} + p_{\max})$$
The bracket updates according to $p_{\max} = p_k$ if $f(p_k) > 0$, else $p_{\min} = p_k$. Upon convergence ($|f| < \text{tol} \cdot \max(p, 1.0)$ with $\text{tol} = 10^{-12}$), velocities reconstruct as:
$$\mathbf{v} = \frac{\mathbf{S}}{z}, \quad z = \tau + p + D$$

#### Relativistic Magnetohydrodynamics (GRMHD)
When $B^2 = \|\mathbf{B}\|^2 > 10^{-15}$, the inversion solves a 1D scalar equation for $z = \rho h W^2$ on bracket:
$$[z_{\min}, z_{\max}] = \left[\max\left(\frac{1}{2}\sqrt{S^2}, 10^{-12}\right), \; 100(D + \tau + B^2)\right]$$
The squared velocity evaluates:
$$v^2(z) = \frac{S^2 z^2 + (\mathbf{S} \cdot \mathbf{B})^2 (2z + B^2)}{z^2 (z + B^2)^2}$$
subject to $v^2 \le 1 - 10^{-15}$, yielding Lorentz factor $W(z) = (1 - v^2)^{-1/2}$. Comobile magnetic field projections evaluate:
$$\mathbf{B} \cdot \mathbf{v} = \frac{\mathbf{S} \cdot \mathbf{B}}{z}, \quad b^2 = \frac{B^2}{W^2} + (\mathbf{B} \cdot \mathbf{v})^2$$
Trial gas pressure, rest-mass density, and specific enthalpy evaluate:
$$p(z) = z + B^2 - \frac{1}{2}b^2 - \tau - D, \quad \rho(z) = \max\left(\rho_{\text{floor}}, \frac{D}{W(z)}\right), \quad h(z) = \frac{z}{D W(z)}$$
The objective root equation evaluates:
$$f(z) = p(z) - p_{\text{EOS}}(\rho(z), h(z)) = 0$$
with analytical derivative approximation:
$$\frac{df}{dz} \approx 1 + \frac{1}{2}(\Gamma - 1)$$
Iterates update via $z_{k+1} = z_k - f(z_k)/(df/dz)$, with bisection fallback $z_{k+1} = \frac{1}{2}(z_{\min} + z_{\max})$ whenever $z_{k+1}$ escapes $[z_{\min}, z_{\max}]$ or evaluates to non-finite values. Upon convergence ($|f| < \text{tol} \cdot \max(z, 1.0)$), primitive velocities reconstruct via:
$$\mathbf{v} = \frac{\mathbf{S}}{z + B^2} + \frac{(\mathbf{S} \cdot \mathbf{B}) \mathbf{B}}{z(z + B^2)}$$

---

### 4.5. High-Order Spatial Reconstruction Schemes

Reconstruction routines in `include/relativistic/hydro/solvers/reconstruction.hpp` extrapolate cell-centered values $q_i$ to cell interface boundaries $q_{i+1/2}^L$ and $q_{i+1/2}^R$.

#### WENO5 Stencils & Smoothness Indicators
The 5th-order reconstruction evaluates three 3-point candidate stencils:
$$p_0 = \frac{1}{6} \left( 2 q_{i-2} - 7 q_{i-1} + 11 q_i \right)$$
$$p_1 = \frac{1}{6} \left( -q_{i-1} + 5 q_i + 2 q_{i+1} \right)$$
$$p_2 = \frac{1}{6} \left( 2 q_i + 5 q_{i+1} - q_{i+2} \right)$$
Linear optimal weights: $d_0 = 0.1, d_1 = 0.6, d_2 = 0.3$.
Smoothness indicators:
$$\beta_0 = \frac{13}{12}(q_{i-2} - 2q_{i-1} + q_i)^2 + \frac{1}{4}(q_{i-2} - 4q_{i-1} + 3q_i)^2$$
$$\beta_1 = \frac{13}{12}(q_{i-1} - 2q_i + q_{i+1})^2 + \frac{1}{4}(q_{i-1} - q_{i+1})^2$$
$$\beta_2 = \frac{13}{12}(q_i - 2q_{i+1} + q_{i+2})^2 + \frac{1}{4}(3q_i - 4q_{i+1} + q_{i+2})^2$$
In the WENO-Z formulation:
$$\tau_5 = |\beta_0 - \beta_2|, \quad \alpha_k = d_k \left( 1 + \left(\frac{\tau_5}{\beta_k + \epsilon}\right)^2 \right), \quad \omega_k = \frac{\alpha_k}{\sum_{j=0}^2 \alpha_j}$$
The left-face interface value evaluates:
$$q_{i+1/2}^L = \sum_{k=0}^2 \omega_k p_k$$

#### Monotonicity-Preserving 5th-Order (MP5)
The 5th-order polynomial value $q^{\text{orig}}$ evaluates:
$$q^{\text{orig}} = \frac{1}{60} \left( 2 q_{i-2} - 13 q_{i-1} + 47 q_i + 27 q_{i+1} - 3 q_{i+2} \right)$$
Let $q^{\text{MP}} = q_i + \operatorname{minmod}(q_{i+1} - q_i, \; \alpha(q_i - q_{i-1}))$ with $\alpha = 4.0$. If $(q^{\text{orig}} - q_i)(q^{\text{orig}} - q^{\text{MP}}) \le \epsilon$, $q^{\text{orig}}$ is preserved without modification. Otherwise, candidate curvature bounds evaluate:
$$d_k = q_{i+k+1} - 2 q_{i+k} + q_{i+k-1}$$
$$d^{\text{M4}} = \operatorname{minmod4}\left( \alpha d_0 - d_1, \; \alpha d_1 - d_0, \; d_0, \; d_1 \right)$$
$$q^{\text{MD}} = \frac{1}{2}(q_i + q_{i+1}) - \frac{1}{2} d^{\text{M4}}, \quad q^{\text{LC}} = q_i + \frac{1}{2}(q_i - q_{i-1}) + \frac{4}{3} d^{\text{M4}}_{-1}$$
The reconstructed interface value evaluates:
$$q_{i+1/2} = \operatorname{median}\left( q^{\text{orig}}, \; \max(\min(q_i, q_{i+1}, q^{\text{MD}}), \min(q_i, q^{\text{UL}}, q^{\text{LC}})), \; \min(\max(q_i, q_{i+1}, q^{\text{MD}}), \max(q_i, q^{\text{UL}}, q^{\text{LC}})) \right)$$

---

### 4.6. Multi-Wave Approximate Riemann Solvers

#### HLL Riemann Solver
Given left and right states $U_L, U_R$, sound speeds $c_{s,L}, c_{s,R}$ define acoustic signal speeds via relativistic velocity addition:
$$\lambda_{L,\pm} = \frac{v_{x,L} \pm c_{s,L}}{1 \pm v_{x,L} c_{s,L}}, \quad \lambda_{R,\pm} = \frac{v_{x,R} \pm c_{s,R}}{1 \pm v_{x,R} c_{s,R}}$$
$$\lambda_L = \min(\lambda_{L,-}, \lambda_{R,-}, 0), \quad \lambda_R = \max(\lambda_{L,+}, \lambda_{R,+}, 0)$$
The HLL interface flux evaluates:
$$\mathbf{F}^{\text{HLL}} = \frac{\lambda_R \mathbf{F}_L - \lambda_L \mathbf{F}_R + \lambda_L \lambda_R (\mathbf{U}_R - \mathbf{U}_L)}{\lambda_R - \lambda_L}$$

#### HLLC Riemann Solver (Contact Wave Restoration)
The HLL state yields intermediate energy and momentum densities:
$$U^{\text{HLL}}_{S_x} = \frac{\lambda_R U_{R, S_x} - \lambda_L U_{L, S_x} + F_{L, S_x} - F_{R, S_x}}{\lambda_R - \lambda_L}$$
$$E^{\text{HLL}} = \frac{\lambda_R (U_{R, \tau} + U_{R, D}) - \lambda_L (U_{L, \tau} + U_{L, D}) + F_{L, \tau} + F_{L, D} - F_{R, \tau} - F_{R, D}}{\lambda_R - \lambda_L}$$
The contact wave speed $\lambda_*$ is extracted from the quadratic relation:
$$\lambda_* = \frac{b_0 - \sqrt{b_0^2 - 4}}{2}, \quad b_0 = \frac{E^{\text{HLL}} + F^{\text{HLL}}_{S_x}}{U^{\text{HLL}}_{S_x}}$$
The star-region total pressure satisfies $p_* = -U^{\text{HLL}}_{S_x} \lambda_* + F^{\text{HLL}}_{S_x}$.
For $K \in \{L, R\}$, star states evaluate:
$$D^*_K = D_K \frac{\lambda_K - v_{x, K}}{\lambda_K - \lambda_*}$$
$$S_{x, K}^* = \frac{\lambda_K S_{x, K} - F_{K, S_x} + p_*}{\lambda_K - \lambda_*}$$
$$S_{y, K}^* = S_{y, K} \frac{\lambda_K - v_{x, K}}{\lambda_K - \lambda_*}, \quad S_{z, K}^* = S_{z, K} \frac{\lambda_K - v_{x, K}}{\lambda_K - \lambda_*}$$
$$\tau^*_K = \frac{\lambda_K (\tau_K + D_K) - S_{x, K} + p_* \lambda_*}{\lambda_K - \lambda_*} - D^*_K$$
The HLLC interface flux evaluates:
$$\mathbf{F}^{\text{HLLC}} = \begin{cases} \mathbf{F}_L & \text{if } \lambda_L \ge 0 \\ \mathbf{F}_L + \lambda_L (\mathbf{U}^*_L - \mathbf{U}_L) & \text{if } \lambda_L < 0 \le \lambda_* \\ \mathbf{F}_R + \lambda_R (\mathbf{U}^*_R - \mathbf{U}_R) & \text{if } \lambda_* < 0 \le \lambda_R \\ \mathbf{F}_R & \text{if } \lambda_R \le 0 \end{cases}$$

---

### 4.7. Constrained Transport (CT) & Solenoidal Magnetic Constraint

On a two-dimensional staggered mesh, face-centered magnetic fields $B_{x, i+1/2, j}$ and $B_{y, i, j+1/2}$ are defined at cell interfaces, while electric fields $E_{z, i+1/2, j+1/2}$ reside at cell vertices.
Vertex electric fields are computed by arithmetic averaging from the four adjacent cell centers:
$$E_{z, i+1/2, j+1/2} = -\frac{1}{4} \sum_{p \in \{0, 1\}} \sum_{q \in \{0, 1\}} \left( v_{x, i+p, j+q} B_{y, i+p, j+q} - v_{y, i+p, j+q} B_{x, i+p, j+q} \right)$$
Faraday's induction equation discrete updates evaluate:
$$B_{x, i+1/2, j}^{n+1} = B_{x, i+1/2, j}^n - \frac{\Delta t}{\Delta y} \left( E_{z, i+1/2, j+1/2} - E_{z, i+1/2, j-1/2} \right)$$
$$B_{y, i, j+1/2}^{n+1} = B_{y, i, j+1/2}^n + \frac{\Delta t}{\Delta x} \left( E_{z, i+1/2, j+1/2} - E_{z, i-1/2, j+1/2} \right)$$
Discrete divergence over cell $(i, j)$:
$$(\nabla \cdot \mathbf{B})_{i, j}^n = \frac{B_{x, i+1/2, j}^n - B_{x, i-1/2, j}^n}{\Delta x} + \frac{B_{y, i, j+1/2}^n - B_{y, i, j-1/2}^n}{\Delta y}$$
Substituting the face induction updates into the time-advanced divergence yields:
$$\begin{aligned}
(\nabla \cdot \mathbf{B})_{i, j}^{n+1} &= \frac{B_{x, i+1/2, j}^{n+1} - B_{x, i-1/2, j}^{n+1}}{\Delta x} + \frac{B_{y, i, j+1/2}^{n+1} - B_{y, i, j-1/2}^{n+1}}{\Delta y} \\
&= (\nabla \cdot \mathbf{B})_{i, j}^n - \frac{\Delta t}{\Delta x \Delta y} \Big[ (E_{z, i+1/2, j+1/2} - E_{z, i+1/2, j-1/2}) - (E_{z, i-1/2, j+1/2} - E_{z, i-1/2, j-1/2}) \Big] \\
&\quad + \frac{\Delta t}{\Delta x \Delta y} \Big[ (E_{z, i+1/2, j+1/2} - E_{z, i-1/2, j+1/2}) - (E_{z, i+1/2, j-1/2} - E_{z, i-1/2, j-1/2}) \Big] \\
&= (\nabla \cdot \mathbf{B})_{i, j}^n + 0 \equiv (\nabla \cdot \mathbf{B})_{i, j}^n
\end{aligned}$$
Thus, if the initial field satisfies $\nabla \cdot \mathbf{B} = 0$, the discrete solenoidal constraint is preserved to machine precision for all time steps. Cell-centered magnetic fields are obtained via arithmetic projection $B_{x,\text{cell}} = \frac{1}{2}(B_{x, i+1/2, j} + B_{x, i-1/2, j})$ and $B_{y,\text{cell}} = \frac{1}{2}(B_{y, i, j+1/2} + B_{y, i, j-1/2})$.

---

### 4.8. Tolman-Oppenheimer-Volkoff (TOV) Stellar Structure & Redshift Formulation

For a static spherically symmetric fluid star with metric $ds^2 = -e^{2\Phi(r)} c^2 dt^2 + (1 - 2Gm(r)/(c^2 r))^{-1} dr^2 + r^2 d\Omega^2$:
$$\frac{dm}{dr} = 4\pi r^2 \frac{\varepsilon(r)}{c^2}$$
$$\frac{dp}{dr} = -\frac{G \left( \frac{\varepsilon}{c^2} + \frac{p}{c^2} \right) \left( m + 4\pi r^3 \frac{p}{c^2} \right)}{r^2 \left( 1 - \frac{2Gm}{c^2 r} \right)}$$
$$\frac{d\Phi}{dr} = -\frac{1}{\varepsilon + p} \frac{dp}{dr} = \frac{G \left( m + 4\pi r^3 \frac{p}{c^2} \right)}{c^2 r^2 \left( 1 - \frac{2Gm}{c^2 r} \right)}$$
$$\frac{dm_b}{dr} = \frac{4\pi r^2 \rho(r)}{\sqrt{1 - \frac{2Gm(r)}{c^2 r}}}$$

#### Origin Taylor Series Initialization
At the stellar origin ($r \to 0$), the differential equations exhibit coordinate singularities. Initial conditions at offset radius $r_0 = \Delta r_{\text{init}} = 1.0\,\text{m}$ evaluate via quadratic Taylor series expansions:
$$m(r_0) = \frac{4\pi}{3} r_0^3 \frac{\varepsilon_c}{c^2}, \quad m_b(r_0) = \frac{4\pi}{3} r_0^3 \rho_c$$
$$p(r_0) = p_c - \frac{2\pi}{3} G \left(\frac{\varepsilon_c}{c^2} + \frac{p_c}{c^2}\right)\left(\frac{\varepsilon_c}{c^2} + \frac{3 p_c}{c^2}\right) r_0^2$$
$$\Phi(r_0) = \frac{2\pi}{3} \frac{G}{c^2} \left(\frac{\varepsilon_c}{c^2} + \frac{3 p_c}{c^2}\right) r_0^2$$
where $\rho_c$ is central rest-mass density, $p_c = p_{\text{EOS}}(\rho_c)$, and $\varepsilon_c = \varepsilon_{\text{EOS}}(\rho_c, p_c)$.

#### Adaptive Scale-Height Step-Size Regulation
Between radial steps, integration step size $\Delta r$ adapts to the local pressure scale height:
$$h_p = \frac{p}{|dp/dr|}$$
$$\Delta r_{\text{opt}} = \operatorname{clamp}\left( 0.05 h_p, \; \Delta r_{\min}, \; \Delta r_{\max} \right)$$
$$\Delta r_{n+1} = \operatorname{clamp}\left( \Delta r_{\text{opt}}, \; 0.5 \Delta r_n, \; 1.5 \Delta r_n \right)$$
with bounds $\Delta r_{\min} = 10^{-4}\,\text{m}$ and $\Delta r_{\max} = 20000.0\,\text{m}$.

#### Stellar Surface Resolution & Exterior Metric Potential Matching
Integration halts when $p \le p_{\text{cutoff}} = 10^{-14} p_c$. When a Runge-Kutta step crosses below zero ($p_{n+1} \le 0$), the precise stellar boundary radius $R$ and surface state are evaluated by linear sub-step interpolation:
$$\theta_{\text{sub}} = \frac{0 - p_n}{p_{n+1} - p_n} \in [0, 1)$$
$$R = r_n + \theta_{\text{sub}} \Delta r_n$$
$$M = m_n + \theta_{\text{sub}} (m_{n+1} - m_n), \quad M_b = m_{b, n} + \theta_{\text{sub}} (m_{b, n+1} - m_{b, n})$$
$$\Phi_{\text{int}}(R) = \Phi_n + \theta_{\text{sub}} (\Phi_{n+1} - \Phi_n)$$
At surface radius $R$, the interior metric potential matches the Schwarzschild vacuum metric:
$$\Phi_{\text{ext}}(R) = \frac{1}{2} \ln\left( 1 - \frac{2GM}{c^2 R} \right)$$
All interior potential values $\Phi(r)$ receive the global shift:
$$\Phi(r) \leftarrow \Phi(r) + \left[ \Phi_{\text{ext}}(R) - \Phi_{\text{int}}(R) \right]$$
Physical diagnostics evaluate:
$$C = \frac{GM}{c^2 R}, \quad z_s = \frac{1}{\sqrt{1 - 2C}} - 1, \quad z_c = \exp(-\Phi(0)) - 1$$
Dynamical stability boundaries along equilibrium sequences $M(\rho_c)$ satisfy $\partial M / \partial \rho_c = 0$, with branches $\partial M / \partial \rho_c < 0$ tagged as unstable against radial collapse.

---

### 4.9. Relativistic Equations of State (EOS) Formulations

#### Relativistic Degenerate Fermi Gas
For fermion species with rest mass $m$ and mean molecular weight $\mu_e$, scale constants evaluate:
$$p_0 = \frac{m^4 c^5}{24\pi^2 \hbar^3}, \quad \rho_0 = \mu_e \frac{m^4 c^3}{3\pi^2 \hbar^3}$$
Dimensionless Fermi momentum parameter evaluates $x = (\rho / \rho_0)^{1/3}$.
Isotropic degeneracy pressure:
$$p(x) = p_0 \left[ x (2x^2 - 3)\sqrt{1 + x^2} + 3\operatorname{asinh}(x) \right]$$
Kinetic energy density evaluates:
$$E_k(x) = \begin{cases} p_0 \left[ \frac{12}{5} x^5 \left( 1 - \frac{5}{28} x^2 + \frac{5}{72} x^4 \right) \right] & \text{if } x < 0.05 \\ p_0 \left[ 3x (2x^2 + 1)\sqrt{1 + x^2} - 3\operatorname{asinh}(x) - 8x^3 \right] & \text{if } x \ge 0.05 \end{cases}$$
$$\varepsilon(\rho) = \rho c^2 + E_k(x), \quad h(\rho) = \frac{\varepsilon(\rho) + p(\rho)}{\rho c^2}, \quad c_s^2(x) = \frac{c^2 x^2}{3(1 + x^2)}$$
Density inversion from pressure $p$ evaluates via Newton-Raphson iteration on $x$:
$$x_{k+1} = x_k - \frac{p(x_k) - p}{dp/dx}, \quad \frac{dp}{dx} = \frac{8 p_0 x^4}{\sqrt{1 + x^2}}$$
with initial guess $x_0 = (15 p / (8 p_0))^{0.2}$ for $x \le 1$ and $x_0 = (p / (2 p_0))^{0.25}$ for $x > 1$, converging to $|dx| < 10^{-12} \max(x, 1.0)$ and yielding $\rho = \rho_0 x^3$.

#### Piecewise Polytropic EOS
Across $N$ density intervals separated by boundaries $\rho_{\text{div}, 0} < \rho_{\text{div}, 1} < \dots < \rho_{\text{div}, N-2}$:
$$p(\rho) = K_i \rho^{\Gamma_i}, \quad \rho \in [\rho_{\text{div}, i-1}, \rho_{\text{div}, i})$$
Pressure continuity determines polytropic constants $K_i$:
$$K_i = \frac{p_{i-1}}{\rho_{\text{div}, i-1}^{\Gamma_i}}$$
Specific enthalpy continuity $h(\rho) = 1 + a_i + \frac{\Gamma_i}{\Gamma_i - 1}\frac{p}{\rho c^2}$ determines energy shift constants $a_i$ ($a_0 = 0$):
$$a_i = a_{i-1} + \frac{p_{i-1}}{\rho_{\text{div}, i-1} c^2} \left( \frac{1}{\Gamma_{i-1} - 1} - \frac{1}{\Gamma_i - 1} \right)$$
Total energy density evaluates:
$$\varepsilon(\rho) = (1 + a_i)\rho c^2 + \frac{p}{\Gamma_i - 1}$$
Sound speed squared evaluates:
$$c_s^2(\rho) = \frac{\Gamma_i p}{\rho h(\rho)}$$

#### Tabulated Nuclear Matter (3D Trilinear Interpolation)
Indexed over $(\log_{10}\rho_i, \log_{10}T_j, Y_{e, k})$, fractional coordinate weights evaluate:
$$f_\rho = \frac{\log_{10}\rho - \log_{10}\rho_0}{\Delta\log_{10}\rho}, \quad f_T = \frac{\log_{10}T - \log_{10}T_0}{\Delta\log_{10}T}, \quad f_Y = \frac{Y_e - Y_{e, 0}}{\Delta Y_e}$$
Interpolated thermodynamic quantities $\mathcal{Q} \in \{\log_{10}p, \log_{10}\varepsilon, h, c_s^2\}$ evaluate:
$$\mathcal{Q} = \sum_{a=0}^1 \sum_{b=0}^1 \sum_{c=0}^1 \left[(1 - f_\rho)^{1-a} f_\rho^a\right] \left[(1 - f_T)^{1-b} f_T^b\right] \left[(1 - f_Y)^{1-c} f_Y^c\right] \mathcal{Q}_{i+a, j+b, k+c}$$
Cold-slice density inversion evaluates by bracketed bisection on $\log_{10}p(\log_{10}\rho, T_{\min}, Y_{e, \min})$ over table grid points.

---

## 5. Accretion Disk Physics

### 5.1. Novikov-Thorne Thin Disk Profile

Radiative surface flux $F(r)$ around a Kerr black hole ([Section 1.3](#13-kerr-metric-rotating-black-hole)):
$$F(r) = \frac{3GM\dot{M}}{8\pi r^3} \frac{f(x)}{x(x^3 + a_*)}, \quad x = \sqrt{\frac{r}{r_g}}$$
The roots of the cubic algebraic equation $x^3 - 3x + 2a_* = 0$ evaluate analytically for $\psi = \frac{1}{3} \arccos(a_*)$:
$$x_1 = 2 \cos\left(\psi - \frac{\pi}{3}\right), \quad x_2 = 2 \cos\left(\psi + \frac{\pi}{3}\right), \quad x_3 = -2 \cos(\psi)$$
Page-Thorne analytical integral $f(x)$:
$$f(x) = x - x_0 - \frac{3}{2}a_* \ln\left(\frac{x}{x_0}\right) - \sum_{i=1}^3 \frac{3(x_i - a_*)^2}{x_i(x_i - x_j)(x_i - x_k)} \ln\left(\frac{x - x_i}{x_0 - x_i}\right)$$
where $x_0 = \sqrt{r_{\text{ISCO}}/r_g}$ with $r_{\text{ISCO}}$ from [Section 2.4](#24-kerr-characteristic-radii-and-horizon-quantities).
Effective blackbody emission temperature:
$$T_{\text{eff}}(r) = \left( \frac{F(r)}{\sigma_{\text{SB}}} \right)^{1/4}$$
Disk scale height $H(r)$ and surface density $\Sigma(r)$:
$$c_s(r) = \sqrt{\frac{k_B T_{\text{eff}}}{0.5 m_p}}, \quad H(r) = \frac{c_s(r)}{\Omega_K(r)}, \quad \Sigma(r) = \frac{\dot{M}}{3\pi \alpha_{\text{visc}} c_s(r) H(r)}$$
Net accretion radiative efficiency:
$$\eta = 1 - \tilde{E}(r_{\text{ISCO}}), \quad \tilde{E}(r) = \frac{x^3 - 2x + a_*}{\sqrt{x^3 (x^3 - 3x + 2a_*)}}$$

---

### 5.2. Fishbone-Moncrief Magnetized Thick Torus

Stationary barotropic thick torus around a Kerr black hole with constant specific angular momentum $l = -u_\phi / u_t$ anchored to the Keplerian specific angular momentum at pressure maximum $r_c$:
$$l = \frac{\sqrt{M}(r_c^2 - 2a\sqrt{M}\sqrt{r_c} + a^2)}{r_c^{3/2} - 2M\sqrt{r_c} + a\sqrt{M}}$$
Effective potential $W(r, \theta)$:
$$W(r, \theta) = \frac{1}{2} \ln \left[ \frac{\rho^2 \Delta \sin^2\theta}{\Sigma \sin^2\theta - 4M a r l \sin^2\theta - l^2 (\Delta - a^2 \sin^2\theta)} \right]$$
where $\rho^2 = r^2 + a^2 \cos^2\theta$, $\Delta = r^2 - 2Mr + a^2$, and $\Sigma = (r^2 + a^2)^2 - a^2 \Delta \sin^2\theta$.
The inner cusp $r_{\text{in}}$ defines potential boundary $W_{\text{in}} = W(r_{\text{in}}, \pi/2)$.
Specific enthalpy $h(r, \theta)$:
$$h(r, \theta) = \max\left( 1.0, \; \exp(W_{\text{in}} - W(r, \theta)) \right)$$
For polytropic equation of state $p = K \rho^\Gamma$, the rest-mass density satisfies:
$$\rho(r, \theta) = \begin{cases} \left[ \frac{\Gamma - 1}{\Gamma K} (h(r, \theta) - 1) \right]^{\frac{1}{\Gamma - 1}} & \text{if } W(r, \theta) < W_{\text{in}} \\ 0 & \text{otherwise} \end{cases}$$
Four-velocity components:
$$u_t = -\exp(W), \quad u_\phi = -l u_t$$
$$u^t = g^{tt} u_t + g^{t\phi} u_\phi, \quad u^\phi = g^{\phi t} u_t + g^{\phi\phi} u_\phi$$

### 5.3. Blandford-Znajek Jet Magnetosphere

Force-free magnetosphere stream function $A_\phi(r, \theta) = C r^\nu (\sin\theta)^{2\mu}$. The Znajek horizon boundary condition enforces regularity:
$$B_\phi = (\Omega_H - \Omega_F) \sin\theta \sqrt{\frac{g_{\theta\theta}}{g_{rr}}} B^r$$
Extracted Poynting flux power:
$$P_{\text{BZ}} = \frac{k}{4\pi c} \Omega_H^2 \Phi_{\text{BH}}^2 f\left(\frac{\Omega_F}{\Omega_H}\right)$$
where $\Phi_{\text{BH}} = \int |B^r| dS$ is the net horizon magnetic flux and $\Omega_F / \Omega_H \approx 0.5$ maximizes power extraction.

---

## 6. Polarized Radiative Transfer & Relativistic Optics

### 6.1. Relativistic Frequency Shift, Beaming & Frame Invariance

For a null photon with 4-momentum $p^\mu$ emitted by a source with 4-velocity $u^\mu_{\text{emit}}$ and observed by an observer with 4-velocity $u^\mu_{\text{obs}}$, the generalized spectral shift ratio $g$ evaluates:
$$g = \frac{\nu_{\text{obs}}}{\nu_{\text{emit}}} = \frac{g_{\mu\nu}(x_{\text{obs}}) p^\mu_{\text{obs}} u^\nu_{\text{obs}}}{g_{\alpha\beta}(x_{\text{emit}}) p^\alpha_{\text{emit}} u^\beta_{\text{emit}}}$$
In flat Minkowski spacetime with observer velocity $\boldsymbol{\beta} = \mathbf{v}/c$ and line-of-sight unit vector $\hat{\mathbf{n}}$, the kinematic Doppler factor reduces to:
$$g = \frac{\sqrt{1 - \|\boldsymbol{\beta}\|^2}}{1 - \boldsymbol{\beta} \cdot \hat{\mathbf{n}}}$$

Specific intensity $I_\nu$, bolometric flux $F_{\text{bol}}$, and wavelength-specific intensity $I_\lambda$ transform across reference frames according to:
$$I_{\text{obs}}(\nu_{\text{obs}}) = g^3 I_{\text{emit}}\left(\frac{\nu_{\text{obs}}}{g}\right), \quad F_{\text{bolometric, obs}} = g^4 F_{\text{bolometric, emit}}, \quad I_{\lambda, \text{obs}}(\lambda_{\text{obs}}) = g^5 I_{\lambda, \text{emit}}\left(g \lambda_{\text{obs}}\right)$$
Observed and emitted wavelengths satisfy $\lambda_{\text{obs}} = \lambda_{\text{emit}} / g$ and $\lambda_{\text{emit}} = g \lambda_{\text{obs}}$.

### 6.2. Full-Stokes Polarized Representation & Polarimetric Invariants

Polarized light is parameterized by the 4-component Stokes vector $\mathbf{S} = (I, Q, U, V)^T$ aligned to a 32-byte boundary:
- $I$: Total radiant intensity.
- $Q$: Linear polarization along coordinate parallel/perpendicular axes.
- $U$: Linear polarization along $\pm 45^\circ$ diagonal axes.
- $V$: Circular polarization (right-handed vs left-handed).

Polarimetric observables evaluate as follows:
- Linear polarization intensity:
  $$I_L = \sqrt{\max\left(Q^2 + U^2, \; 0\right)}$$
- Total polarization intensity:
  $$I_P = \sqrt{\max\left(Q^2 + U^2 + V^2, \; 0\right)}$$
- Degree of Linear Polarization (DoLP):
  $$\Pi_L = \operatorname{clamp}\left(\frac{I_L}{I}, \; 0.0, \; 1.0\right) \quad (\text{with } \Pi_L = 0 \text{ when } I \le 10^{-30})$$
- Degree of Circular Polarization (DoCP):
  $$\Pi_V = \operatorname{clamp}\left(\frac{V}{I}, \; -1.0, \; 1.0\right)$$
- Total Degree of Polarization (DoP):
  $$\Pi = \operatorname{clamp}\left(\frac{I_P}{I}, \; 0.0, \; 1.0\right)$$
- Electric Vector Position Angle (EVPA):
  $$\chi = \frac{1}{2} \operatorname{atan2}(U, Q) \in \left[-\frac{\pi}{2}, \; \frac{\pi}{2}\right]$$
- Polarization ellipse geometry with semi-major axis $a_{\text{ell}}$, semi-minor axis $b_{\text{ell}}$, and orientation angle $\psi_{\text{ell}} = \chi$:
  $$a_{\text{ell}} = \sqrt{\frac{I_P + I_L}{2 I}}, \quad b_{\text{ell}} = \sqrt{\frac{I_P - I_L}{2 I}}$$

#### Physical Realizability Invariant
Physical Stokes states satisfy the positivity and Stokes cone bound within numerical tolerance $\epsilon_{\text{tol}} = 10^{-9}$:
$$I \ge -\epsilon_{\text{tol}}, \quad Q^2 + U^2 + V^2 \le (I + \epsilon_{\text{tol}})^2$$

#### Reference Frame Rotation
Under a spatial frame rotation by angle $\psi$, the Stokes vector transforms via the Mueller rotation matrix:
$$\mathbf{S}' = \mathbf{M}_{\text{rot}}(\psi) \mathbf{S} = \begin{pmatrix}
1 & 0 & 0 & 0 \\
0 & \cos(2\psi) & \sin(2\psi) & 0 \\
0 & -\sin(2\psi) & \cos(2\psi) & 0 \\
0 & 0 & 0 & 1
\end{pmatrix} \begin{pmatrix} I \\ Q \\ U \\ V \end{pmatrix}$$

### 6.3. Polarized Transfer System & Delano Matrix Exponential Integration

Transport of Stokes vector $\mathbf{S}$ along affine parameter segment $\Delta\lambda$ obeys the coupled system:
$$\frac{d\mathbf{S}}{d\lambda} = \mathbf{j} - \mathbf{K} \mathbf{S}$$
where the emission vector $\mathbf{j} = (j_I, j_Q, j_U, j_V)^T$ and the transfer matrix $\mathbf{K}$ are given by:
$$\mathbf{K} = \begin{pmatrix}
\alpha_I & \alpha_Q & \alpha_U & \alpha_V \\
\alpha_Q & \alpha_I & \rho_V & -\rho_U \\
\alpha_U & -\rho_V & \alpha_I & \rho_Q \\
\alpha_V & \rho_U & -\rho_Q & \alpha_I
\end{pmatrix}$$
with absorptivities $(\alpha_I, \alpha_Q, \alpha_U, \alpha_V)$, Faraday rotation coefficients $(\rho_V, \rho_U)$, and Faraday conversion coefficient $\rho_Q$.

#### Delano Analytical Scaling and Squaring Integration Method
Direct numerical integration of stiff transfer equations is evaluated by Delano's analytical matrix exponential method (`step_delano_analytical`):
1. Compute the 1-norm of the transfer matrix:
   $$\|K\|_1 = \max_{0 \le r < 4} \sum_{c=0}^3 |K_{rc}|, \quad \mu_K = \|K\|_1 \Delta\lambda$$
2. If $\mu_K > 0.5$, calculate the number of matrix squarings $m = \lceil \log_2(2\mu_K) \rceil$ and the scaled segment length $h = \Delta\lambda / 2^m$; otherwise $m = 0$ and $h = \Delta\lambda$.
3. Form the scaled matrix $\mathbf{A} = -h \mathbf{K}$. Evaluate $\mathbf{E}_0 = e^{\mathbf{A}}$ and auxiliary matrix $\mathbf{\Psi}_0 = \int_0^h e^{t \mathbf{A}} dt$ via a 12th-order Taylor series:
   $$\mathbf{E}_0 = \mathbf{I} + \sum_{n=1}^{12} \frac{\mathbf{A}^n}{n!}, \quad \mathbf{\Psi}_0 = h \left[ \mathbf{I} + \sum_{n=1}^{12} \frac{\mathbf{A}^n}{(n+1)!} \right]$$
4. Execute $m$ successive squaring and doubling recursions:
   $$\mathbf{\Psi}_{k+1} = \mathbf{\Psi}_k (\mathbf{I} + \mathbf{E}_k), \quad \mathbf{E}_{k+1} = \mathbf{E}_k^2 \quad (k = 0, \dots, m - 1)$$
5. The advanced Stokes vector evaluates:
   $$\mathbf{S}(\lambda + \Delta\lambda) = \mathbf{E}_m \mathbf{S}(\lambda) + \mathbf{\Psi}_m \mathbf{j}, \quad I \leftarrow \max(0, S_0)$$

#### Relativistic Scaling of Radiative Coefficients
Across curved ray segments with Doppler factor $g = \nu_{\text{obs}} / \nu_{\text{emit}}$:
$$\mathbf{j}_{\text{obs}}(\nu_{\text{obs}}) = g^2 \mathbf{j}_{\text{emit}}\left(\frac{\nu_{\text{obs}}}{g}\right), \quad \mathbf{K}_{\text{obs}}(\nu_{\text{obs}}) = \frac{1}{g} \mathbf{K}_{\text{emit}}\left(\frac{\nu_{\text{obs}}}{g}\right)$$

#### Hybrid Thermal / Non-Thermal Plasma Transfer
For plasma with non-thermal electron fraction $f_{\text{nt}} \in [0, 1]$ and thermal electron fraction $1 - f_{\text{nt}}$:
$$\mathbf{j}_{\text{hybrid}} = \mathbf{j}_{\text{nt}}(n_e f_{\text{nt}}) + \mathbf{j}_{\text{th}}(n_e (1 - f_{\text{nt}})), \quad \mathbf{K}_{\text{hybrid}} = \mathbf{K}_{\text{nt}}(n_e f_{\text{nt}}) + \mathbf{K}_{\text{th}}(n_e (1 - f_{\text{nt}}))$$

### 6.4. Radiative Processes & Coefficients

#### Cyclotron Frequency & Plasma Frequency
$$\nu_B = \frac{e B}{2\pi m_e}, \quad \nu_p^2 = \frac{e^2 n_e}{m_e \epsilon_0}$$

#### Relativistic Thermal Bremsstrahlung
Free-free electron-ion bremsstrahlung emissivity in unmagnetized plasma:
$$j_\nu^{\text{ff}} = \frac{1}{4\pi} \frac{32\pi e^6}{3\sqrt{3} m_e^3 c^3} \sqrt{\frac{2\pi m_e}{3 k_B T_e}} Z^2 n_e n_i \exp\left(-\frac{h\nu}{k_B T_e}\right) \bar{g}_{\text{ff}}(\nu, T_e) \left(1 + 4.4 \times 10^{-10} T_e\right)$$
where the thermal Gaunt factor $\bar{g}_{\text{ff}}$ evaluates for $u = h\nu / (k_B T_e)$:
$$\bar{g}_{\text{ff}} = \begin{cases} \max\left(1.0, \; \frac{\sqrt{3}}{\pi} \ln\left(\frac{2.25}{\gamma_E u}\right)\right) & \text{if } u < 10^{-4} \quad (\gamma_E \approx 0.57721566) \\ \frac{\sqrt{3}}{\pi} \exp\left(\frac{u}{2}\right) K_0\left(\frac{u}{2}\right) & \text{if } u \ge 10^{-4} \end{cases}$$

#### Non-Thermal Power-Law Synchrotron
For relativistic electrons with energy distribution $N(\gamma) = C_e \gamma^{-p}$ spanning $\gamma \in [\gamma_{\min}, \gamma_{\max}]$, where $C_e = n_e f_{\text{nt}} (p - 1) \gamma_{\min}^{p-1}$:
- Linear polarization fraction:
  $$\Pi_L = \frac{p + 1}{p + 7/3}$$
- Total emissivity and polarized components:
  $$j_I = \frac{\sqrt{3} e^3 B \sin\theta}{4\pi m_e c^2 (p + 1)} C_e \Gamma\left(\frac{3p - 1}{12}\right) \Gamma\left(\frac{3p + 19}{12}\right) \left(\frac{2\pi m_e c \nu}{3 e B \sin\theta}\right)^{-\frac{p - 1}{2}} \frac{1}{4\pi}, \quad j_Q = \Pi_L j_I, \quad j_U = j_V = 0$$
- Absorptivity components:
  $$\alpha_I = \frac{\sqrt{3} e^3 B \sin\theta}{8\pi m_e^2 c^2 \nu} C_e \Gamma\left(\frac{3p + 2}{12}\right) \Gamma\left(\frac{3p + 22}{12}\right) \left(\frac{2\pi m_e c \nu}{3 e B \sin\theta}\right)^{-\frac{p + 2}{2}}, \quad \alpha_Q = \Pi_L \alpha_I, \quad \alpha_U = \alpha_V = 0$$
- Faraday rotation coefficient:
  $$\rho_V = \frac{\nu_p^2 \nu_B \cos\theta}{c \nu^2}, \quad \rho_Q = 0$$

#### Thermal Synchrotron Radiation
For thermal relativistic electrons with dimensionless temperature $\Theta_e = \frac{k_B T_e}{m_e c^2}$:
- Characteristic synchrotron frequency:
  $$\nu_s = \frac{2}{9} \nu_B \Theta_e^2 \sin\theta, \quad X = \frac{\nu}{\nu_s}$$
- Petrosian-Mahadevan emissivity profile function:
  $$I(X) = \frac{4.0505}{X^{1/3}} \left(1 + \frac{0.40}{\sqrt{X}} + \frac{0.5316}{X^{1/3}}\right) \exp\left(-X^{1/3}\right)$$
- Total emissivity:
  $$j_I = \frac{\sqrt{2}\pi e^2 n_e \nu_s}{3 c K_2(1/\Theta_e)} \frac{I(X)}{4\pi}, \quad \Pi_L = \operatorname{clamp}\left( \frac{1 + 0.5/\Theta_e}{1 + 1.5/\Theta_e}, \; 0.5, \; 0.85 \right), \quad j_Q = \Pi_L j_I$$
- Thermal absorptivities from Kirchhoff's law:
  $$\alpha_I = \frac{j_I}{B_\nu(T_e)}, \quad \alpha_Q = \frac{j_Q}{B_\nu(T_e)}, \quad B_\nu(T_e) = \frac{2 h \nu^3}{c^2 \left[\exp\left(\frac{h\nu}{k_B T_e}\right) - 1\right]}$$
- Faraday rotativity and convertivity:
  $$\rho_V = \frac{\nu_p^2 \nu_B \cos\theta}{c \nu^2} \frac{K_0(1/\Theta_e)}{K_2(1/\Theta_e)}, \quad \rho_Q = \frac{\nu_p^2 \nu_B^2 \sin^2\theta}{2 c \nu^3} \frac{K_1(1/\Theta_e)}{K_2(1/\Theta_e)}$$

### 6.5. Maxwell-Jüttner Relativistic Thermal Distribution

Relativistic thermal electron distribution with $\Theta_e = \frac{k_B T_e}{m_e c^2}$:
$$f(\gamma) = \frac{\gamma \sqrt{\gamma^2 - 1}}{\Theta_e K_2(1/\Theta_e)} \exp\left(-\frac{\gamma}{\Theta_e}\right), \quad \gamma \ge 1$$
The mean Lorentz factor satisfies:
$$\langle \gamma \rangle = \frac{K_1(1/\Theta_e)}{K_2(1/\Theta_e)} + 3 \Theta_e$$
The modified Bessel functions $K_0(z), K_1(z), K_2(z), K_3(z)$ are evaluated via rational polynomial approximations (`RelativisticBessel`).

#### 4-Regime Monte Carlo Sampling Algorithm
`MaxwellJuttnerDistribution::sample_lorentz_factor` implements the Pozdnyakov-Sobol decomposition algorithm:
1. Weights evaluate:
   $$w_1 = \sqrt{\frac{\pi}{2}}, \quad w_2 = \sqrt{\Theta_e}, \quad w_3 = \frac{3}{2}\sqrt{\frac{\pi}{2}}\Theta_e, \quad w_4 = 2\Theta_e^{3/2}, \quad w_{\text{tot}} = \sum_{i=1}^4 w_i$$
2. Select random interval $r = u \cdot w_{\text{tot}}$ ($u \sim \mathcal{U}(0, 1)$), drawing candidate $x \ge 0$ via Gaussian variates $z \sim \mathcal{N}(0, 1)$ and uniform variates $u_k \sim \mathcal{U}(0, 1)$:
   $$x = \begin{cases} -\ln(u_1) + \frac{1}{2}z^2 & \text{if } r < w_1 \\ -\ln(u_1 u_2) & \text{if } w_1 \le r < w_1 + w_2 \\ -\ln(u_1) + \frac{1}{2}z^2 & \text{if } w_1 + w_2 \le r < w_1 + w_2 + w_3 \\ -\ln(u_1 u_2 u_3) & \text{if } r \ge w_1 + w_2 + w_3 \end{cases}$$
3. The acceptance probability evaluates:
   $$P_{\text{accept}} = \frac{(1 + \Theta_e x)\sqrt{x(2 + \Theta_e x)}}{\sqrt{2x} + \sqrt{\Theta_e}x + \sqrt{2}\Theta_e x^{3/2} + \Theta_e^{3/2}x^2}$$
   Upon acceptance ($u_4 \le P_{\text{accept}}$), the Lorentz factor is $\gamma = 1 + \Theta_e x$. Velocity vectors sample isotropically with speed $\beta = \sqrt{1 - 1/\gamma^2}$.
4. Thermal Doppler broadening line profile:
   $$\phi(\nu, \nu_0) = \frac{1}{\sqrt{2\pi \Theta_e} \nu_0} \exp\left(-\frac{(\nu - \nu_0)^2}{2 \Theta_e \nu_0^2}\right)$$

### 6.6. Inverse Compton Scattering & Klein-Nishina Cross Section

#### Klein-Nishina Total Cross Section
For an electron with rest mass $m_e$ and photon energy $E' = h\nu'$ in the electron rest frame, let $x = E' / (m_e c^2)$:
$$\sigma_{\text{KN}}(x) = \begin{cases} \sigma_T \left(1 - 2x + \frac{26}{5}x^2\right) & \text{if } x < 10^{-4} \\ \frac{3}{4}\sigma_T \left[ \frac{1+x}{x^3}\left(\frac{2x(1+x)}{1+2x} - \ln(1+2x)\right) + \frac{\ln(1+2x)}{2x} - \frac{1+3x}{(1+2x)^2} \right] & \text{if } x \ge 10^{-4} \end{cases}$$
where $\sigma_T = \frac{8\pi}{3}\left(\frac{e^2}{4\pi\epsilon_0 m_e c^2}\right)^2 \approx 6.6524587 \times 10^{-29}\,\text{m}^2$.

#### Compton y-Parameter
$$y = 4\Theta_e (1 + 4\Theta_e) \tau_{\text{eff}}, \quad \tau_{\text{eff}} = \max\left(\tau, \; \tau^2\right)$$

#### Monte Carlo Photon Packet Scattering Kinematics
For packet with energy $E$ and direction $\hat{\mathbf{n}}$ colliding with an electron of velocity $\boldsymbol{\beta} = \mathbf{v}/c$ and Lorentz factor $\gamma = (1 - \beta^2)^{-1/2}$:
1. Energy in electron rest frame:
   $$E' = \gamma E (1 - \hat{\mathbf{n}} \cdot \boldsymbol{\beta}), \quad \epsilon = \frac{E'}{m_e c^2}$$
2. Rejection sample scattered scattering angle $\cos\theta'$ from the differential Klein-Nishina probability:
   $$P(\cos\theta') = \frac{1}{2} \xi^2 \left(\xi + \frac{1}{\xi} - \sin^2\theta'\right), \quad \xi = \frac{1}{1 + \epsilon(1 - \cos\theta')}$$
3. Scattered energy in the laboratory frame:
   $$E_{\text{scat}} = \gamma E' \xi (1 - \beta \cos\theta')$$

### 6.7. Continuous Spectral Pipeline & CIE 1931 Integration

#### Logarithmic Spectral Discretization
`ContinuousSpectrum` discretizes spectral radiance across $N = 4096$ logarithmic wavelength bins spanning $\lambda \in [10^{-14}, 10^3]\,\text{m}$:
$$\ln\lambda_k = \ln\lambda_{\min} + k \frac{\ln\lambda_{\max} - \ln\lambda_{\min}}{N - 1} \quad (k = 0, \dots, N - 1)$$
Interpolation of radiance $I(\lambda)$ uses piecewise linear interpolation over $\ln\lambda$.

#### Discretized Emission Models
- Planck Blackbody Radiance:
  $$B_\lambda(\lambda, T) = \frac{2 h c^2}{\lambda^5 \left[\exp\left(\frac{hc}{\lambda k_B T}\right) - 1\right]}$$
- Non-Thermal Synchrotron Power-Law:
  $$I_\lambda(\lambda) = I_0 \left(\frac{\lambda}{\lambda_0}\right)^{\alpha - 1}$$
- Monochromatic Line:
  $$I_\lambda(\lambda) = I_0 \delta(\lambda - \lambda_0)$$
- Doppler Transformation:
  $$I_{\lambda, \text{obs}}(\lambda) = g^5 I_{\lambda, \text{emit}}(g \lambda)$$
- Logarithmic Band Integration:
  $$F_{\text{band}} = \int_{\lambda_a}^{\lambda_b} I_\lambda(\lambda) d\lambda = \int_{\ln\lambda_a}^{\ln\lambda_b} I_\lambda(e^u) e^u du$$

#### CIE 1931 Tristimulus Color Matching Functions
Analytical multi-Gaussian approximations for $\bar{x}(\lambda), \bar{y}(\lambda), \bar{z}(\lambda)$ with $\lambda$ in nanometers:
$$\begin{aligned}
\bar{x}(\lambda) &= 1.056 \exp\left(-\frac{(\lambda - 599.8)^2}{2 \sigma_{x1}^2}\right) + 0.362 \exp\left(-\frac{(\lambda - 442.0)^2}{2 \sigma_{x2}^2}\right) - 0.065 \exp\left(-\frac{(\lambda - 501.1)^2}{2 \sigma_{x3}^2}\right) \\
\bar{y}(\lambda) &= 0.821 \exp\left(-\frac{(\lambda - 568.8)^2}{2 \sigma_{y1}^2}\right) + 0.286 \exp\left(-\frac{(\lambda - 530.9)^2}{2 \sigma_{y2}^2}\right) \\
\bar{z}(\lambda) &= 1.217 \exp\left(-\frac{(\lambda - 437.0)^2}{2 \sigma_{z1}^2}\right) + 0.681 \exp\left(-\frac{(\lambda - 459.0)^2}{2 \sigma_{z2}^2}\right)
\end{aligned}$$
where widths $\sigma$ switch between lower and upper standard deviations at each peak.

Tristimulus values $(X, Y, Z)$ evaluate across $\lambda \in [380, 780]\,\text{nm}$:
$$X = \int_{380}^{780} I_\lambda(\lambda) \bar{x}(\lambda) d\lambda, \quad Y = \int_{380}^{780} I_\lambda(\lambda) \bar{y}(\lambda) d\lambda, \quad Z = \int_{380}^{780} I_\lambda(\lambda) \bar{z}(\lambda) d\lambda$$
Linear sRGB conversion matrix:
$$\begin{pmatrix} R_{\text{linear}} \\ G_{\text{linear}} \\ B_{\text{linear}} \end{pmatrix} = \begin{pmatrix}
3.2404542 & -1.5371385 & -0.4985314 \\
-0.9692660 & 1.8760108 & 0.0415560 \\
0.0556434 & -0.2040259 & 1.0572252
\end{pmatrix} \begin{pmatrix} X \\ Y \\ Z \end{pmatrix}$$
sRGB non-linear electro-optical transfer function:
$$C_{\text{sRGB}} = \begin{cases} 12.92 \, C_{\text{linear}} & \text{if } C_{\text{linear}} \le 0.0031308 \\ 1.055 \, C_{\text{linear}}^{1/2.4} - 0.055 & \text{if } C_{\text{linear}} > 0.0031308 \end{cases}$$

### 6.8. High Dynamic Range Tone Mapping Operators

1. `ACES` Filmic Curve:
   $$f_{\text{ACES}}(x) = \operatorname{clamp}\left( \frac{x(2.51x + 0.03)}{x(2.43x + 0.59) + 0.14}, \; 0.0, \; 1.0 \right)$$
2. `Logarithmic` HDR Curve over dynamic range $[L_{\min}, L_{\max}] = [10^{-12}, 10^{20}]\,\text{W/m}^2/\text{sr}$:
   $$L = 0.2126 R + 0.7152 G + 0.0722 B, \quad L' = \frac{\log_{10}(1 + L / L_{\min})}{\log_{10}(1 + L_{\max} / L_{\min})}, \quad \mathbf{C}_{\text{mapped}} = \mathbf{C} \frac{\operatorname{clamp}(L', 0, 1)}{L}$$
3. `Extended Reinhard` with maximum white parameter $L_{\text{white}} = 10.0$:
   $$f_{\text{Reinhard}}(x) = \frac{x \left(1 + \frac{x}{L_{\text{white}}^2}\right)}{1 + x}$$
4. EV Exposure Compensation:
   $$\mathbf{C}_{\text{exposed}} = \mathbf{C} \cdot 2^{\text{EV}}$$

### 6.9. Carter Constant Classification & Analytical Turning Points

For null geodesics in Kerr spacetime parameterized by central mass $M$ and spin $a$, the constants of motion are:
$$E = -p_t, \quad L_z = p_\phi, \quad \xi = \frac{L_z}{E}, \quad \eta = \frac{\mathcal{Q}}{E^2}$$
where Carter's constant $\mathcal{Q} = p_\theta^2 + \cos^2\theta [a^2(E^2 - \mu^2) + L_z^2 / \sin^2\theta]$.
The radial geodesic motion is governed by $R(r) = (dr/d\lambda)^2$:
$$R(r) = r^4 + \left(a^2 - \xi^2 - \eta\right) r^2 + 2M\left[\eta + (\xi - a)^2\right] r - a^2 \eta = 0$$

#### Analytical Depressed Quartic Real Root Resolution
`CarterPhaseClassifier` extracts turning points by transforming $R(r) = 0$ into a depressed quartic $x^4 + p x^2 + q x + r_0 = 0$:
$$p = a^2 - \xi^2 - \eta, \quad q = 2M\left[\eta + (\xi - a)^2\right], \quad r_0 = -a^2 \eta$$
Ferrari's resolvent cubic evaluates:
$$m^3 + p m^2 + \left(\frac{p^2}{4} - r_0\right) m - \frac{q^2}{8} = 0$$
Roots are solved via Cardano's analytical formulas. The largest positive real root $m > 0$ yields quadratic factors:
$$x^2 \pm \sqrt{2m} x + \left(\frac{p}{2} + m \mp \frac{q}{2\sqrt{2m}}\right) = 0$$

#### Geodesic Phase Classification
Let $r_{\text{max\_ext}}$ denote the largest real root outside the outer event horizon $r_+ = M + \sqrt{M^2 - a^2}$:
1. If no real root satisfies $r > r_+$ and the ray is infalling ($dr/d\lambda < 0$), classify as `CertainAbsorption`: the geodesic plunges past the horizon.
2. If $r_{\text{max\_ext}} > 3 r_{\text{disk,out}}$, classify as `CertainEscape`: the geodesic scatters to asymptotic infinity without encountering the accretion disk.
3. Otherwise, classify as `RequiresIntegration`.

### 6.10. Geodesic Ray Probe Diagnostics & Image Orders

`GeodesicRayProbe` traces an exact null geodesic under any screen pixel $(x, y)$, extracting physical quantities at the observer:
1. Lift ray direction $\hat{\mathbf{d}}$ to null 4-momentum $p^\mu$ via ZAMO observer tetrad orthonormalization:
   $$E = -g_{\mu\nu} p^\mu u^\nu_{\text{obs}}, \quad L_z = g_{\mu\nu} p^\mu \phi^\nu, \quad \xi = \frac{L_z}{E}, \quad \eta = \frac{\mathcal{Q}}{E^2}$$
2. Static gravitational redshift factor:
   $$\kappa_{\text{static}} = \frac{1}{E} = \frac{1}{\sqrt{-g_{00}}}$$
3. Sub-step equatorial disk crossing detection ($\theta = \pi/2$):
   For steps crossing the equatorial plane $(\theta_n - \pi/2)(\theta_{n+1} - \pi/2) \le 0$, linear interpolation determines crossing radius $r_c$ and azimuth $\phi_c$:
   $$s = \frac{|\theta_n - \pi/2|}{|\theta_{n+1} - \theta_n|}, \quad r_c = r_n + s(r_{n+1} - r_n), \quad \phi_c = \phi_n + s(\phi_{n+1} - \phi_n)$$
4. Local Keplerian disk redshift:
   $$g_{\text{emit}} = \frac{\sqrt{1 - 2M/r_c + 2a\sqrt{M}r_c^{-3/2}}}{1 + \sqrt{M}r_c^{-3/2}\xi}, \quad g_{\text{total}} = g_{\text{emit}} \cdot \kappa_{\text{static}}$$
5. Image order $n$: counts the total number of equatorial plane crossings ($n = 1$ direct image, $n = 2$ secondary image, $n \ge 3$ higher-order photon rings).
6. Total geodesic deflection angle:
   $$\theta_{\text{defl}} = \arccos\left(\hat{\mathbf{d}}_{\text{init}} \cdot \hat{\mathbf{d}}_{\text{terminal}}\right)$$

### 6.11. Terrell-Penrose Relativistic Aberration & Moving Sphere Intersections

For an observer with 3-velocity $\boldsymbol{\beta} = \mathbf{v}/c$ and Lorentz factor $\gamma = (1 - \beta^2)^{-1/2}$:
- Forward Relativistic Aberration:
  $$\hat{\mathbf{n}}_{\text{obs}} = \frac{\frac{\hat{\mathbf{n}}_{\text{emit}}}{\gamma} + \left[1 + \frac{\gamma - 1}{\gamma \beta^2}(\boldsymbol{\beta} \cdot \hat{\mathbf{n}}_{\text{emit}})\right] \boldsymbol{\beta}}{1 + \boldsymbol{\beta} \cdot \hat{\mathbf{n}}_{\text{emit}}}$$
- Backward Relativistic Aberration:
  $$\hat{\mathbf{n}}_{\text{emit}} = \operatorname{aberrate\_forward}(\hat{\mathbf{n}}_{\text{obs}}, -\boldsymbol{\beta})$$
- Apparent Visual Terrell Rotation Angle:
  $$\theta_{\text{rot}} = \arcsin(\|\boldsymbol{\beta}\|)$$

#### Moving Sphere Spacetime Intersection
For a sphere of radius $R$ moving with constant velocity $\mathbf{v}$ with trajectory $\mathbf{x}_{\text{sphere}}(t) = \mathbf{x}_0 + \mathbf{v} t$, light emitted at retarded time $t_{\text{emit}}$ reaches observer $\mathbf{x}_{\text{obs}}$ at coordinate time $t_{\text{obs}}$ along ray direction $\hat{\mathbf{d}}$:
$$\mathbf{r}_0 = \mathbf{x}_{\text{obs}} + c t_{\text{obs}} \hat{\mathbf{d}} - \mathbf{x}_0, \quad \mathbf{w} = c \hat{\mathbf{d}} + \mathbf{v}$$
$$A t_{\text{emit}}^2 + B t_{\text{emit}} + C = 0$$
where $A = \|\mathbf{w}\|^2$, $B = -2 (\mathbf{r}_0 \cdot \mathbf{w})$, and $C = \|\mathbf{r}_0\|^2 - R^2$.
The latest physical root $t_{\text{emit}} \le t_{\text{obs}}$ yields hit position $\mathbf{h} = \mathbf{x}_{\text{obs}} + c (t_{\text{obs}} - t_{\text{emit}}) \hat{\mathbf{d}}$, surface normal $\hat{\mathbf{N}} = (\mathbf{h} - \mathbf{x}_{\text{sphere}}(t_{\text{emit}})) / R$, and kinematic Doppler factor $g = \frac{\sqrt{1 - \beta^2}}{1 - \boldsymbol{\beta} \cdot \hat{\mathbf{d}}}$.

---

## 7. Uncertainty Quantification Formulation

### 7.1. Interval Arithmetic (IEEE 1788)

An interval $\mathbf{x} = [\underline{x}, \bar{x}]$ represents a real set bounded by lower infimum $\underline{x}$ and upper supremum $\bar{x}$ where $\underline{x} \le \bar{x}$ (`Interval<Scalar>`, `include/relativistic/uncertainty/interval.hpp`).
- Metric properties:
  $$\operatorname{mid}(\mathbf{x}) = \frac{1}{2}(\underline{x} + \bar{x}), \quad \operatorname{wid}(\mathbf{x}) = \bar{x} - \underline{x}, \quad \operatorname{rad}(\mathbf{x}) = \frac{1}{2}(\bar{x} - \underline{x}), \quad |\mathbf{x}| = \max(|\underline{x}|, |\bar{x}|)$$
- Elementary arithmetic operations:
  $$\mathbf{x} + \mathbf{y} = [\underline{x} + \underline{y}, \; \bar{x} + \bar{y}]$$
  $$\mathbf{x} - \mathbf{y} = [\underline{x} - \bar{y}, \; \bar{x} - \underline{y}]$$
  $$\mathbf{x} \times \mathbf{y} = [\min(p_1, p_2, p_3, p_4), \; \max(p_1, p_2, p_3, p_4)], \quad p \in \{\underline{x}\underline{y}, \underline{x}\bar{y}, \bar{x}\underline{y}, \bar{x}\bar{y}\}$$
- Directed division:
  $$\mathbf{x} / \mathbf{y} = \begin{cases}
    \mathbf{x} \times [1 / \bar{y}, \; 1 / \underline{y}] & \text{if } 0 \notin \mathbf{y} \\
    \emptyset & \text{if } \underline{y} = \bar{y} = 0 \\
    \mathbf{x} \times [1 / \bar{y}, \; \infty) & \text{if } \underline{y} = 0 < \bar{y} \\
    \mathbf{x} \times (-\infty, \; 1 / \underline{y}] & \text{if } \underline{y} < \bar{y} = 0 \\
    (-\infty, \; \infty) & \text{if } \underline{y} < 0 < \bar{y}
  \end{cases}$$
- Nonlinear transcendental operations:
  $$\mathbf{x}^2 = \begin{cases} [0, \; \max(\underline{x}^2, \bar{x}^2)] & \text{if } 0 \in \mathbf{x} \\ [\min(\underline{x}^2, \bar{x}^2), \; \max(\underline{x}^2, \bar{x}^2)] & \text{otherwise} \end{cases}$$
  $$\sqrt{\mathbf{x}} = [\sqrt{\max(\underline{x}, 0)}, \; \sqrt{\max(\bar{x}, 0)}]$$
  $$\exp(\mathbf{x}) = [\exp(\underline{x}), \; \exp(\bar{x})], \quad \ln(\mathbf{x}) = [(\underline{x} > 0 ? \ln(\underline{x}) : -\infty), \; \ln(\bar{x})] \quad (\bar{x} > 0)$$
  $$\mathbf{x}^n = \begin{cases} [\underline{x}^n, \; \bar{x}^n] & \text{if } n \text{ is odd} \\ [0, \; \max(\underline{x}^n, \bar{x}^n)] & \text{if } n \text{ is even and } 0 \in \mathbf{x} \\ [\min(\underline{x}^n, \bar{x}^n), \; \max(\underline{x}^n, \bar{x}^n)] & \text{if } n \text{ is even and } 0 \notin \mathbf{x} \end{cases}$$
- Trigonometric functions:
  If $\operatorname{wid}(\mathbf{x}) \ge 2\pi$, $\sin(\mathbf{x}) = [-1, 1]$. Otherwise, let $\hat{y}_{\min} = \min(\sin\underline{x}, \sin\bar{x})$ and $\hat{y}_{\max} = \max(\sin\underline{x}, \sin\bar{x})$. For integer periods $k \in [\lfloor(\underline{x} - \frac{\pi}{2})/(2\pi)\rfloor, \lceil(\bar{x} - \frac{\pi}{2})/(2\pi)\rceil]$, critical points $\frac{\pi}{2} + 2k\pi \in \mathbf{x} \implies \hat{y}_{\max} = 1$ and $-\frac{\pi}{2} + 2k\pi \in \mathbf{x} \implies \hat{y}_{\min} = -1$. Cosine evaluates via identity $\cos(\mathbf{x}) = \sin(\mathbf{x} + \pi/2)$.

### 7.2. Zonotope Enclosure & Girard Order Reduction

A zonotope $\mathcal{Z} \subset \mathbb{R}^D$ is parameterized by centroid vector $\mathbf{c} \in \mathbb{R}^D$ and generator matrix $\mathbf{G} = [\mathbf{g}_1, \dots, \mathbf{g}_p] \in \mathbb{R}^{D \times p}$ (`Zonotope<Scalar, Dim>`, `include/relativistic/uncertainty/zonotope.hpp`):
$$\mathcal{Z} = \mathbf{c} \oplus \sum_{i=1}^p \alpha_i \mathbf{g}_i, \quad \alpha_i \in [-1, 1]$$
- Zonotope order: $\kappa = p / D$.
- Axis-aligned bounding box:
  $$\mathcal{B}(\mathcal{Z}) = \prod_{d=1}^D [c_d - r_d, \; c_d + r_d], \quad r_d = \sum_{i=1}^p |g_{i, d}|$$
  Bounding box hypervolume evaluates $\operatorname{Vol}_{\mathcal{B}} = \prod_{d=1}^D (2 r_d)$.
- Exact two-dimensional hypervolume:
  $$\operatorname{Vol}_{2\text{D}}(\mathcal{Z}) = 4 \sum_{i=1}^{p-1} \sum_{j=i+1}^p \left| g_{i, 1} g_{j, 2} - g_{i, 2} g_{j, 1} \right|$$
- Set operations:
  $$\mathcal{Z}_1 \oplus \mathcal{Z}_2 = (\mathbf{c}_1 + \mathbf{c}_2) \oplus [\mathbf{G}_1 \mid \mathbf{G}_2], \quad \mathbf{A} \mathcal{Z} = (\mathbf{A}\mathbf{c}) \oplus [\mathbf{A}\mathbf{g}_1 \mid \dots \mid \mathbf{A}\mathbf{g}_p], \quad \beta \mathcal{Z} = (\beta\mathbf{c}) \oplus (\beta\mathbf{G})$$
- Girard order reduction algorithm:
  When $p > p_{\max} = \kappa_{\max} D$, generators are sorted descending according to geometric heuristic score:
  $$s_i = \|\mathbf{g}_i\|_1 - \|\mathbf{g}_i\|_\infty = \sum_{d=1}^D |g_{i, d}| - \max_{1 \le d \le D} |g_{i, d}|$$
  The top $p_{\text{keep}} = p_{\max} - D$ generators are retained intact. The remaining $p - p_{\text{keep}}$ generators are condensed into $D$ orthogonal axis-aligned interval generators:
  $$\mathbf{g}_{\text{box}, d} = \left( \sum_{i=p_{\text{keep}}+1}^p |g_{i, d}| \right) \hat{\mathbf{e}}_d, \quad d \in \{1, \dots, D\}$$
  The reduced zonotope satisfies $\mathcal{Z}_{\text{reduced}} = \mathbf{c} \oplus [\mathbf{g}_1, \dots, \mathbf{g}_{p_{\text{keep}}}, \mathbf{g}_{\text{box}, 1}, \dots, \mathbf{g}_{\text{box}, D}]$ with generator count $p_{\text{keep}} + D = p_{\max}$.

### 7.3. Lyapunov Covariance Matrix Differential Propagation

For state covariance matrix $\mathbf{\Sigma} \in \mathbb{R}^{D \times D}$ and phase state Jacobian $\mathbf{J} = \partial \mathbf{f} / \partial \mathbf{Y}$ (`CovarianceMatrix<Scalar, Dim>`, `include/relativistic/uncertainty/covariance.hpp`):
$$\frac{d\mathbf{\Sigma}}{d\lambda} = \mathbf{J}\mathbf{\Sigma} + \mathbf{\Sigma}\mathbf{J}^T + \mathbf{Q}$$
where $\mathbf{Q} \ge 0$ is the continuous process noise covariance matrix.
- Sandwich transition transform under state transition matrix $\mathbf{\Phi}$:
  $$\mathbf{\Sigma}(\lambda) = \mathbf{\Phi} \, \mathbf{\Sigma}(0) \, \mathbf{\Phi}^T$$
- Temporal stage stepping:
  Evaluated via explicit Euler stepping $\mathbf{\Sigma}_{n+1} = \mathbf{\Sigma}_n + \Delta\lambda \, \dot{\mathbf{\Sigma}}(\mathbf{\Sigma}_n)$ or classical Runge-Kutta 4th-order (RK4) stepping with intermediate symmetrization $\mathbf{\Sigma} \leftarrow \frac{1}{2}(\mathbf{\Sigma} + \mathbf{\Sigma}^T)$.
- Cyclic Jacobi eigensystem decomposition:
  For symmetric matrix $\mathbf{A}^{(0)} = \mathbf{\Sigma}$, off-diagonal elements are eliminated via Givens plane rotations in coordinate planes $(p, q)$ ($p < q$):
  $$\theta = \frac{A_{qq} - A_{pp}}{2 A_{pq}}, \quad t = \frac{\operatorname{sgn}(\theta)}{|\theta| + \sqrt{1 + \theta^2}}, \quad c = \frac{1}{\sqrt{1 + t^2}}, \quad s = t c, \quad \tau = \frac{s}{1 + c}$$
  Updating diagonal and off-diagonal elements in place:
  $$A_{pp}' = A_{pp} - t A_{pq}, \quad A_{qq}' = A_{qq} + t A_{pq}, \quad A_{pq}' = 0$$
  $$A_{jp}' = A_{jp} - s (A_{jq} + \tau A_{jp}), \quad A_{jq}' = A_{jq} + s (A_{jp} - \tau A_{jq}) \quad (j \ne p, j \ne q)$$
  Eigenvector columns $\mathbf{V}$ accumulate Givens transformations $V_{jp}' = V_{jp} - s(V_{jq} + \tau V_{jp})$ and $V_{jq}' = V_{jq} + s(V_{jp} - \tau V_{jq})$ until off-diagonal residual norm $\max_{p < q} |A_{pq}| \le 10^{-15} \sum_i |A_{ii}|$.
- Confidence hypervolume:
  For eigenvalues $\lambda_1, \dots, \lambda_D \ge 0$ and confidence scale $n_\sigma$:
  $$V_n = \frac{\pi^{D/2}}{\Gamma(D/2 + 1)} \prod_{d=1}^D \left( n_\sigma \sqrt{\lambda_d} \right)$$
- Multivariate Gaussian sampling:
  $$\mathbf{x} = \boldsymbol{\mu} + \sum_{k=1}^D z_k \sqrt{\lambda_k} \mathbf{v}_k, \quad z_k \sim \mathcal{N}(0, 1)$$
  where independent standard normal variates $z_k$ are drawn via Box-Muller pairs from `PCG64Engine`.

### 7.4. Variational Geodesic Equations & Christoffel Derivatives

The joint variational state integrates 8D phase coordinates $\mathbf{Y} = (\mathbf{x}, \mathbf{p})^T \in \mathbb{R}^8$ along with variational transition matrix $\mathbf{\Phi}(\lambda) \in \mathbb{R}^{8 \times 8}$ and covariance matrix $\mathbf{\Sigma}(\lambda) \in \mathbb{R}^{8 \times 8}$ (`VariationalGeodesicIntegrator`, `include/relativistic/uncertainty/variational_geodesic.hpp`):
$$\frac{dx^\mu}{d\lambda} = p^\mu, \quad \frac{dp^\mu}{d\lambda} = -\Gamma^\mu_{\alpha\beta}(x) p^\alpha p^\beta$$
$$\frac{d\mathbf{\Phi}}{d\lambda} = \mathbf{J}(x, p) \, \mathbf{\Phi}, \quad \mathbf{\Phi}(0) = \mathbf{I}_{8 \times 8}$$
$$\frac{d\mathbf{\Sigma}}{d\lambda} = \mathbf{J} \mathbf{\Sigma} + \mathbf{\Sigma} \mathbf{J}^T + \mathbf{Q}, \quad \mathbf{\Sigma}(0) = \mathbf{\Sigma}_0$$

#### Analytical Phase-Space Jacobian Matrix $\mathbf{J}$
The $8 \times 8$ Jacobian partitions into four $4 \times 4$ blocks:
$$\mathbf{J}(x, p) = \begin{pmatrix}
\mathbf{0}_{4 \times 4} & \mathbf{I}_{4 \times 4} \\
\mathbf{A}(x, p) & \mathbf{B}(x, p)
\end{pmatrix}$$
where block elements evaluate:
$$A^\mu_{\phantom{\mu}\nu} = \frac{\partial \dot{p}^\mu}{\partial x^\nu} = -\partial_\nu \Gamma^\mu_{\alpha\beta}(x) p^\alpha p^\beta$$
$$B^\mu_{\phantom{\mu}\nu} = \frac{\partial \dot{p}^\mu}{\partial p^\nu} = -2 \Gamma^\mu_{\nu\beta}(x) p^\beta$$
Spatial derivatives of Christoffel symbols $\partial_\nu \Gamma^\mu_{\alpha\beta}$ are evaluated via 8th-order centered finite difference stencils (`GeodesicJacobianComputer`):
$$\partial_\nu \Gamma^\mu_{\alpha\beta}(x) = \frac{1}{840 h_\nu} \sum_{k=1}^4 w_k \left[ \Gamma^\mu_{\alpha\beta}(x + k h_\nu \hat{\mathbf{e}}_\nu) - \Gamma^\mu_{\alpha\beta}(x - k h_\nu \hat{\mathbf{e}}_\nu) \right]$$
with weights $w = (672, -168, 32, -3)$ and adaptive coordinate step $h_\nu = \max(|x^\nu| \cdot 10^{-4}, 10^{-6})$.

#### Symplectic Phase Determinant Monitoring
For Hamiltonian geodesic systems in conservative spacetimes, Liouville's theorem guarantees unit volume preservation in phase space:
$$\det \mathbf{\Phi}(\lambda) \equiv 1.0$$
The determinant is evaluated numerically via Gaussian elimination with partial pivoting to track numerical dissipation and integration drift.

### 7.5. Generalized Polynomial Chaos Expansion & Numerical Quadratures

A stochastic variable $X(\boldsymbol{\xi})$ dependent on independent standardized random variables $\boldsymbol{\xi} = (\xi_1, \dots, \xi_D)^T$ is expanded in an orthogonal polynomial basis (`PolynomialChaosExpansion`, `include/relativistic/uncertainty/polynomial_chaos.hpp`):
$$X(\boldsymbol{\xi}) = \sum_{k=0}^{P-1} \hat{x}_k \Psi_k(\boldsymbol{\xi}), \quad \Psi_k(\boldsymbol{\xi}) = \prod_{d=1}^D \psi_{\alpha_k^{(d)}}(\xi_d)$$
where $\boldsymbol{\alpha}_k = (\alpha_k^{(1)}, \dots, \alpha_k^{(D)}) \in \mathbb{N}_0^D$ satisfies total degree truncation $\sum_{d=1}^D \alpha_k^{(d)} \le M$. Basis dimension evaluates $P = \frac{(D + M)!}{D! M!}$.

#### Orthogonal Polynomial Families
1. Probabilists' Hermite polynomials $H_n(x)$ for Gaussian variables $\xi \sim \mathcal{N}(0, 1)$ with weight $w(x) = \frac{1}{\sqrt{2\pi}} e^{-x^2 / 2}$:
   $$H_0(x) = 1, \quad H_1(x) = x, \quad H_{n+1}(x) = x H_n(x) - n H_{n-1}(x)$$
   Inner product norm: $\|H_n\|^2 = \mathbb{E}[H_n^2(\xi)] = n!$.
2. Legendre polynomials $P_n(x)$ for uniform variables $\xi \sim \mathcal{U}(-1, 1)$ with weight $w(x) = \frac{1}{2}$:
   $$P_0(x) = 1, \quad P_1(x) = x, \quad (n+1) P_{n+1}(x) = (2n + 1) x P_n(x) - n P_{n-1}(x)$$
   Inner product norm: $\|P_n\|^2 = \int_{-1}^1 P_n^2(x) \frac{dx}{2} = \frac{1}{2n + 1}$.

#### Golub-Welsch Quadrature Node and Weight Generation
Gauss-Hermite and Gauss-Legendre quadrature rules (`GaussQuadrature`) construct symmetric tridiagonal Jacobi matrices $\mathbf{J}_Q$ with zero diagonal and sub-diagonal elements:
- Gauss-Hermite: $J_{i, i+1} = \sqrt{i+1}$.
- Gauss-Legendre: $J_{i, i+1} = \frac{i+1}{\sqrt{4(i+1)^2 - 1}}$.
Eigenvalues and eigenvectors of $\mathbf{J}_Q$ are resolved via the implicit QL algorithm (`tridiagonal_symmetric_ql`). Nodes $x_q$ correspond to eigenvalues, and weights evaluate $w_q = z_{0, q}^2$.
Expansion coefficients evaluate via multidimensional tensor-grid quadratures:
$$\hat{x}_k = \frac{1}{\|\Psi_k\|^2} \sum_{q_1=1}^{N_1} \dots \sum_{q_D=1}^{N_D} \left( \prod_{d=1}^D w_{q_d} \right) f(\mathbf{x}_q) \Psi_k(\mathbf{x}_q), \quad \|\Psi_k\|^2 = \prod_{d=1}^D \|\psi_{\alpha_k^{(d)}}\|^2$$

#### Statistical Moments and Quantile Estimation
- Mean: $\mu = \hat{x}_0$.
- Variance: $\sigma^2 = \sum_{k=1}^{P-1} \hat{x}_k^2 \|\Psi_k\|^2$.
- Higher moments:
  $$\mu_3 = \frac{1}{N_s} \sum_{i=1}^{N_s} (X(\boldsymbol{\xi}_i) - \mu)^3, \quad \text{Skewness} = \frac{\mu_3}{\sigma^3}$$
  $$\mu_4 = \frac{1}{N_s} \sum_{i=1}^{N_s} (X(\boldsymbol{\xi}_i) - \mu)^4, \quad \text{Kurtosis} = \frac{\mu_4}{\sigma^4}$$
- Quantile bands (`QuantileBands`):
  Evaluated across $N_s$ quasi-random samples using the Beasley-Springer-Moro rational approximation for the standard normal inverse cumulative distribution function $\Phi^{-1}(u)$:
  $$\text{median} = X_{0.50}, \quad \sigma_{1\pm} = X_{0.50 \pm 0.34134}, \quad \sigma_{2\pm} = X_{0.50 \pm 0.47725}, \quad \sigma_{3\pm} = X_{0.50 \pm 0.49865}$$

### 7.6. Antithetic Monte Carlo Covariance Validation

To validate analytical covariance propagation against empirical sampling without systematic bias, `MonteCarloCovarianceValidator` (`include/relativistic/uncertainty/variational_geodesic.hpp`) generates antithetic Gaussian ensembles.
1. Whitened sample generation:
   Given initial covariance $\mathbf{\Sigma}_0 = \mathbf{V} \mathbf{\Lambda} \mathbf{V}^T$, $N/2$ independent Gaussian pairs $\mathbf{z}_p, -\mathbf{z}_p$ are generated with sample covariance $\mathbf{S}_z$. The whitening matrix evaluates:
   $$\mathbf{W} = \mathbf{V}_z \mathbf{\Lambda}_z^{-1/2} \mathbf{V}_z^T, \quad \mathbf{z}' = \mathbf{W} \mathbf{z}$$
   Ensemble states evaluate $\mathbf{x}_p^\pm = \boldsymbol{\mu} \pm \mathbf{V} \mathbf{\Lambda}^{1/2} \mathbf{z}'$, guaranteeing exact initial covariance matching $\mathbf{S}_{\text{initial}} \equiv \mathbf{\Sigma}_0$.
2. Empirical covariance estimation:
   Following parallel multi-threaded geodesic integration of all $N$ trajectories to affine parameter $\lambda$:
   $$\bar{\mathbf{Y}} = \frac{1}{N} \sum_{i=1}^N \mathbf{Y}_i, \quad \mathbf{S}_{\text{empirical}} = \frac{1}{N - 1} \sum_{i=1}^N (\mathbf{Y}_i - \bar{\mathbf{Y}})(\mathbf{Y}_i - \bar{\mathbf{Y}})^T$$
3. Principal semi-axes comparison:
   Principal semi-axes $a_k = \sqrt{\lambda_k(\mathbf{\Sigma})}$ and $e_k = \sqrt{\lambda_k(\mathbf{S}_{\text{empirical}})}$ sorted descending evaluate relative discrepancy:
   $$\epsilon_k = \frac{|a_k - e_k|}{\max(a_k, 10^{-12})}, \quad \epsilon_{\max} = \max_k \epsilon_k, \quad \bar{\epsilon} = \frac{1}{8} \sum_{k=1}^8 \epsilon_k$$
   Validation passes when $\epsilon_{\max} \le \text{tol}$ (default $\text{tol} = 0.001$).

### 7.7. Metrology & Spatial Uncertainty Visualization

`MetrologyVisualizer` (`include/relativistic/uncertainty/metrology.hpp`) synthesizes visual representations of continuous probability distributions:
- 3D Covariance Ellipsoid Mesh:
  For 3D spatial covariance $\mathbf{\Sigma}_{3\text{D}}$ with eigensystem $\mathbf{\Sigma}_{3\text{D}} \mathbf{v}_k = \lambda_k \mathbf{v}_k$, semi-axis lengths evaluate $a = n_\sigma \sqrt{\lambda_0}$, $b = n_\sigma \sqrt{\lambda_1}$, $c = n_\sigma \sqrt{\lambda_2}$.
  Spherical parameter coordinates $(\theta, \phi) \in [0, \pi] \times [0, 2\pi]$ mapped to world coordinates:
  $$\mathbf{x}_{\text{local}} = (a \sin\theta \cos\phi, \; b \sin\theta \sin\phi, \; c \cos\theta)^T$$
  $$\mathbf{x}_{\text{world}} = \mathbf{x}_{\text{center}} + \sum_{k=0}^2 x_{\text{local}, k} \mathbf{v}_k$$
  Outward surface normals evaluate $\hat{\mathbf{n}}_{\text{local}} = (x_{\text{local}} / a^2, \; y_{\text{local}} / b^2, \; z_{\text{local}} / c^2)^T / \|\dots\|$ transformed to world coordinates.
- 2D Bivariate Gaussian Probability Heatmap:
  For 2D sub-covariance $\mathbf{\Sigma}_{2\text{D}}$ with determinant $\Delta = \Sigma_{00}\Sigma_{11} - \Sigma_{01}^2 > 0$:
  $$\mathbf{\Sigma}_{2\text{D}}^{-1} = \frac{1}{\Delta} \begin{pmatrix} \Sigma_{11} & -\Sigma_{01} \\ -\Sigma_{01} & \Sigma_{00} \end{pmatrix}$$
  Probability density at point $(x, y)$:
  $$p(x, y) = \frac{1}{2\pi \sqrt{\Delta}} \exp\left( -\frac{1}{2} \left[ \Sigma_{11}^{-1} (x - \mu_x)^2 + 2 \Sigma_{01}^{-1} (x - \mu_x)(y - \mu_y) + \Sigma_{22}^{-1} (y - \mu_y)^2 \right] \right)$$
- Orbital Wrapping Comparison Benchmark:
  `OrbitalWrappingBenchmark` (`include/relativistic/uncertainty/orbital_wrapping_benchmark.hpp`) compares interval wrapping against zonotope Girard reduction under 2D rotation $\mathbf{R}(\Delta\theta) = \begin{pmatrix} \cos\Delta\theta & -\sin\Delta\theta \\ \sin\Delta\theta & \cos\Delta\theta \end{pmatrix}$. Interval boxes suffer exponential expansion from coordinate wrapping:
  $$\mathbf{x}_{n+1} = [x_n] \cos\Delta\theta - [y_n] \sin\Delta\theta, \quad \mathbf{y}_{n+1} = [x_n] \sin\Delta\theta + [y_n] \cos\Delta\theta$$
  whereas zonotopes transform via linear generator rotation $\mathcal{Z}_{n+1} = \mathbf{R} \mathcal{Z}_n$ with order reduction $\kappa \le 4$, yielding bounding box hypervolume reduction ratios $V_{\text{interval}} / V_{\text{zonotope}} \gg 1$.

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

A celestial body of equatorial radius $R$ and oblateness ratio $o \in [0.1, 5.0]$ is represented by the quadric ellipsoid surface $(x_l/a)^2 + (y_l/b)^2 + (z_l/c)^2 = 1$ oriented by unit quaternion $\mathbf{q}$, with semi-axes defined according to the geometry model $M_{\text{geom}}$:
- Oblate Spheroid (0): $a = b = R$, $c = R \cdot o$
- Rigid Sphere (1): $a = b = c = R$
- Prolate Spheroid (2): $a = b = R$, $c = R / \max(o, 0.1)$
- Triaxial Ellipsoid (3): $a = R$, $b = 0.85 R$, $c = R \cdot o$

Given ray origin $\mathbf{x}_{\text{orig}}$ and unit travel direction $\hat{\mathbf{d}}$, local coordinates evaluate $\mathbf{x}' = \mathbf{q}^{-1} (\mathbf{x}_{\text{orig}} - \mathbf{x}_{\text{body}}) \mathbf{q}$ and $\mathbf{d}' = \mathbf{q}^{-1} \hat{\mathbf{d}} \mathbf{q}$. The quadratic intersection evaluates:
$$A = \frac{d_x'^2}{a^2} + \frac{d_y'^2}{b^2} + \frac{d_z'^2}{c^2}, \quad B = 2\left(\frac{x_0' d_x'}{a^2} + \frac{y_0' d_y'}{b^2} + \frac{z_0' d_z'}{c^2}\right), \quad C = \frac{x_0'^2}{a^2} + \frac{y_0'^2}{b^2} + \frac{z_0'^2}{c^2} - 1$$
The smallest positive root $t = (-B - \sqrt{B^2 - 4AC}) / (2A)$ defines hit position $\mathbf{h} = \mathbf{x}_{\text{orig}} + t \hat{\mathbf{d}}$, outward normal $\hat{\mathbf{N}} = \mathbf{q} \left(\frac{x_l}{a^2}, \frac{y_l}{b^2}, \frac{z_l}{c^2}\right) \mathbf{q}^{-1} / \left\|\dots\right\|$, and surface angles:
$$\theta = \arccos\left(\operatorname{clamp}\left(\frac{z_l}{c}, -1, 1\right)\right), \quad \phi = \operatorname{atan2}(y_l, x_l) + \omega_{\text{rot}} t_{\text{sim}}$$

#### Light Source Modes & Direction Resolution
The incident light vector $\hat{\mathbf{L}}$, distance $d$, and emission color $\mathbf{C}_{\text{light}}$ are resolved according to `LightSourceMode`:
1. `CentralSource` (0): $\mathbf{x}_{\text{light}} = (0, 0, 0)$, $d = \|\mathbf{h}\|$, $\hat{\mathbf{L}} = -\mathbf{h} / d$.
2. `CameraHeadlight` (1): $\mathbf{x}_{\text{light}} = \mathbf{x}_{\text{obs}}$, $d = \|\mathbf{x}_{\text{obs}} - \mathbf{h}\|$, $\hat{\mathbf{L}} = (\mathbf{x}_{\text{obs}} - \mathbf{h}) / d$.
3. `FixedDirection` (2): Distant celestial key light at azimuth $\phi_L$ and elevation $\theta_L$:
   $$\hat{\mathbf{L}} = (\cos\theta_L \cos\phi_L, \cos\theta_L \sin\phi_L, \sin\theta_L), \quad d \to \infty$$
4. `FixedPoint` (3): Localized emitter at world coordinates $\mathbf{x}_{\text{light}}$, $d = \|\mathbf{x}_{\text{light}} - \mathbf{h}\|$, $\hat{\mathbf{L}} = (\mathbf{x}_{\text{light}} - \mathbf{h}) / d$.
5. `NearestEmissiveBody` (4): Evaluated dynamically across all active catalog bodies $j \ne i$ with intrinsic emission $I_{\text{em}, j} > 10^{-3}$:
   $$j^* = \operatorname{argmin}_{j} \|\mathbf{x}_j - \mathbf{h}\|, \quad \mathbf{x}_{\text{light}} = \mathbf{x}_{j^*}, \quad \mathbf{C}_{\text{light}} = \mathbf{C}_{\text{primary}, j^*} \odot \mathbf{C}_{\text{param}}$$
   Falls back to `CentralSource` if no emissive bodies exist.
6. `SpecificBody` (5): Emitter bound to assigned catalog identifier $B_{\text{id}}$, with position $\mathbf{x}_{B_{\text{id}}}$ and color modulation $\mathbf{C}_{\text{primary}, B_{\text{id}}} \odot \mathbf{C}_{\text{param}}$. Falls back to `CentralSource` if absent or disabled.
7. `Unlit` (6): Bypasses directional shading; unit illumination with zero shadows ($D = 1, A_0 = 1, S = 0, \mathcal{E} = 1$).

#### Distance Attenuation Formulation
For localized light sources ($d < \infty$), incident irradiance $\mathcal{E}$ obeys the inverse-square law with reference distance $d_{\text{ref}}$ and maximum amplification clamp:
$$\mathcal{E}(d) = \begin{cases} I_{\text{light}} & \text{if mode is None (0) or } d \to \infty \\ I_{\text{light}} \cdot \min\left( \left(\frac{d_{\text{ref}}}{\max(d, 10^{-6})}\right)^2, \; 64.0 \right) & \text{if mode is InverseSquare (1)} \end{cases}$$
The $64.0\times$ clamp prevents floating-point overflow and excessive blooming when the observer or an illuminated surface approaches the source.

#### Wrapped Diffuse Lighting & Terminator Softness
To model diffuse atmospheric wrapping, the dot product $\mu = \hat{\mathbf{N}} \cdot \hat{\mathbf{L}}$ is filtered through terminator transition softness $s \in [0, 1]$:
$$D(s) = \operatorname{clamp}\left( \frac{\mu + s}{1 + s}, \; 0, \; 1 \right)$$
When $s = 0$, $D$ reproduces standard Lambertian cosine shading $\max(0, \mu)$. For photographic Earth textures with Day/Night/Automatic blending, the twilight transition width uses $s_{\text{eff}} = \max(s, s_{\text{earth}})$, evaluating day weight $w_{\text{day}} = t^3 (t(6t - 15) + 10)$ in the GPU shader ($w_{\text{day}} = t^2 (3 - 2t)$ on CPU) where $t = \operatorname{clamp}((\mu + 0.5 s_{\text{eff}}) / s_{\text{eff}}, 0, 1)$.

#### Blinn-Phong Specular Reflection
For lit surfaces ($D(s) > 0$), specular highlights evaluate via the half-vector $\hat{\mathbf{H}} = \frac{\hat{\mathbf{L}} - \hat{\mathbf{d}}}{\|\hat{\mathbf{L}} - \hat{\mathbf{d}}\|}$:
$$S = 0.6 \, (1 - \varrho) \, s_{\text{spec}} \left[ \max(0, \hat{\mathbf{N}} \cdot \hat{\mathbf{H}}) \right]^{4 + 80(1 - \varrho)} \cdot I_{\text{light}} \, \mathbf{C}_{\text{light}}$$
where $\varrho \in [0, 1]$ is specular microfacet roughness and $s_{\text{spec}}$ is the global specular scale.

#### Shadow Occlusion Rays
When body shadows are enabled (`ENABLE_BODY_SHADOWS`) and detail level is not Minimal, shadow rays $\mathbf{r}(t) = \mathbf{h} + t \hat{\mathbf{L}}$ are cast towards localized sources ($t \in [10^{-5}, d]$). The diffuse term $D(s)$ is zeroed if the ray intersects the bounding sphere of any occluding body $j$ up to cap $N_{\text{casters}} \le N_{\text{shadow\_limit}}$, or the central horizon sphere of radius $r_h$:
$$(\mathbf{h} + t \hat{\mathbf{L}} - \mathbf{x}_j)^2 < R_{j, \text{bound}}^2 \implies D(s) \leftarrow 0$$

#### Multi-Source Inter-Body Emission Lighting
When enabled (`body_emission_lighting_enabled = 1`), radiant energy emitted by luminous celestial bodies ($I_{\text{em}, j} > 10^{-3}$) illuminates facing surfaces:
$$\mathbf{E}_{\text{bodies}} = \sum_{j \ne i, j \le N_{\text{emit\_limit}}} G_{\text{em}} \cdot I_{\text{em}, j} \cdot \min\left( \left(\frac{d_{\text{ref}, e}}{\max(d_j, \max(R_j, 10^{-6}))}\right)^2, \; 16.0 \right) \cdot \max(0, \hat{\mathbf{N}} \cdot \hat{\mathbf{L}}_j) \cdot \mathbf{C}_{\text{primary}, j}$$
where $G_{\text{em}}$ is the emission lighting gain and $d_{\text{ref}, e}$ is the emission reference distance.

#### Total Surface Radiance
The combined surface radiance prior to relativistic transformations evaluates:
$$\mathbf{C} = \mathbf{C}_{\text{albedo}} \odot \left[ a_0 \mathbf{1} + D(s) (1 - a_0) \mathcal{E} \mathbf{C}_{\text{light}} + \mathbf{E}_{\text{bodies}} \right] + \mathbf{S} + \mathbf{E}_{\text{intrinsic}} + \mathbf{E}_{\text{city}}$$
where $a_0$ is the ambient floor, $\mathbf{E}_{\text{intrinsic}}$ is the self-emission radiance, and $\mathbf{E}_{\text{city}}$ represents nocturnal artificial city illumination on unlit hemispheres:
$$\mathbf{E}_{\text{city}} = \operatorname{clamp}(1 - 5 D(s), 0, 1) \cdot I_{\text{night}} \cdot \sigma_{\text{speckle}} \begin{pmatrix} 1.10 \\ 0.95 \\ 0.60 \end{pmatrix}$$

#### Lighting Quality Detail Presets
Execution complexity is governed by three detail presets (`body_lighting_detail`):
- `Minimal` (0): Diffuse and ambient terms only; shadow rays, specular highlights, and inter-body emission lighting are bypassed ($N_{\text{casters}} = 0, N_{\text{emitters}} = 0$).
- `Reduced` (1): Shadow occlusion and inter-body emission active with explicit clamps: `body_shadow_caster_limit` $\le 512$ and `body_emission_source_limit` $\le 512$.
- `Full` (2): Unrestricted shadow occlusion testing against all catalog occluders and accumulation across all active emitters.

Relativistic factors, with the body velocity components interpreted as fractions of $c$ and $\beta = \min(\lvert \mathbf{v} \rvert, 0.9999)$:
$$\mathcal{D} = \frac{1}{\gamma (1 - \boldsymbol{\beta} \cdot \hat{\mathbf{d}})}, \quad C \leftarrow C \cdot \text{clamp}(\mathcal{D}^3, 0.1, 10), \quad z_g = \sqrt{\max\left(1 - \frac{2 m}{R}, 0.05\right)}$$
with $z_g$ applied to the channels as $(z_g, 0.9 z_g, 0.7 z_g)$. The atmospheric rim with $q = (1 - \hat{\mathbf{N}} \cdot \hat{\mathbf{V}})^3$, thickness $\tau_a$ and color $(\mathbf{c}_a, \alpha_a)$ gives $C \leftarrow C (1 - q \alpha_a) + \mathbf{c}_a \, q \, \tau_a \, g_{\text{atm}} \cdot 2.5$, where $g_{\text{atm}}$ is `body_atmosphere_global_intensity`. Each channel is clamped to $[0, 10]$.

### 8.5. Schematic Overlay Deflection

When the lens approximation is enabled, `SchematicViewRenderer` shifts a projected point by the first-order deflection
$$\delta = \text{clamp}\left(\frac{4M}{b}, 0, 0.35\right), \quad b = \max\left(r_{\text{cam}} \sqrt{1 - \cos^2\chi}, 2.05 M\right)$$
where $\chi$ is the angle between the direction to the point and the direction to the origin. The point is displaced by $\rho \, \delta$ along the unit vector toward the origin, with $\rho$ the distance from the camera to the point.

---

### 8.6. Gamepad Analog Transfer Functions & Observer Kinematic Integration

#### Non-Linear Quadratic Deadzone Transfer Function
Raw analog stick and trigger axes $v_{\text{raw}} \in [-1, 1]$ undergo piecewise quadratic deadzone filtering parameterized by deadzone threshold $d \in [0, 1)$:
$$\mathcal{F}_{\text{deadzone}}(v, d) = \begin{cases} 0 & \text{if } |v| \le d \\ \operatorname{sgn}(v) \left( \frac{|v| - d}{1 - d} \right)^2 & \text{if } |v| > d \end{cases}$$
The quadratic curvature provides high sensitivity near the deadzone boundary for fine alignment adjustments while enabling rapid maximum traversal rates at full stick deflection.

For analog trigger axes $a_{\text{trig}} \in [-1, 1]$ (where $-1$ represents an unpressed trigger and $+1$ represents full depression), raw readings are normalized into $[0, 1]$ prior to deadzone filtering:
$$a_{\text{norm}} = \frac{a_{\text{trig}} + 1.0}{2.0}, \quad \tilde{a}_{\text{trig}} = \mathcal{F}_{\text{deadzone}}(a_{\text{norm}}, d_{\text{trig}})$$
with default trigger deadzone $d_{\text{trig}} = 0.08$. Stick deadzones default to $d_{\text{LS}} = d_{\text{RS}} = 0.14$.

#### Virtual Directional Stick and Trigger Button Thresholding
Continuous analog axes synthesize 10 discrete virtual button inputs (indices 15 to 24) when filtered values exceed the directional engagement threshold $\theta_{\text{stick}} = 0.40$:
$$B_{\text{LS,Up}} = (\tilde{y}_{\text{LS}} < -\theta_{\text{stick}}), \quad B_{\text{LS,Down}} = (\tilde{y}_{\text{LS}} > \theta_{\text{stick}}), \quad B_{\text{LS,Left}} = (\tilde{x}_{\text{LS}} < -\theta_{\text{stick}}), \quad B_{\text{LS,Right}} = (\tilde{x}_{\text{LS}} > \theta_{\text{stick}})$$
$$B_{\text{RS,Up}} = (\tilde{y}_{\text{RS}} < -\theta_{\text{stick}}), \quad B_{\text{RS,Down}} = (\tilde{y}_{\text{RS}} > \theta_{\text{stick}}), \quad B_{\text{RS,Left}} = (\tilde{x}_{\text{RS}} < -\theta_{\text{stick}}), \quad B_{\text{RS,Right}} = (\tilde{x}_{\text{RS}} > \theta_{\text{stick}})$$
$$B_{\text{LT}} = (\tilde{a}_{\text{LT}} > \theta_{\text{stick}}), \quad B_{\text{RT}} = (\tilde{a}_{\text{RT}} > \theta_{\text{stick}})$$

#### Observer Spatial Translation Update
Given camera comobile triad $(\mathbf{f}, \mathbf{r}, \mathbf{u})$ representing forward gaze, lateral right strafe, and vertical elevation vectors, translational displacement increments over timestep $\Delta t$ evaluate:
$$\Delta\mathbf{x} = \left( \tilde{x}_{\text{LS}} \cdot \mathbf{r} - \tilde{y}_{\text{LS}} \cdot \mathbf{f} + k_{\text{inv,vert}} (\tilde{a}_{\text{RT}} - \tilde{a}_{\text{LT}}) \cdot \mathbf{u} \right) \cdot \mu_{\text{move}} \cdot v_{\text{speed}} \cdot \kappa_{\text{speed}} \cdot \Delta t$$
where $k_{\text{inv,vert}} = -1$ if `invert_vertical` is set (otherwise $+1$), $\mu_{\text{move}}$ is the movement sensitivity multiplier, and $\kappa_{\text{speed}}$ evaluates modifier scaling:
$$\kappa_{\text{speed}} = \begin{cases} \kappa_{\text{sprint}} & \text{if LeftThumb click is active (default } 4.0\times\text{)} \\ \kappa_{\text{crawl}} & \text{if RightThumb click is active (default } 0.2\times\text{)} \\ 1.0 & \text{otherwise} \end{cases}$$

#### Angular Orientation Integration
Camera pitch ($\theta$), yaw ($\psi$), and roll ($\rho$) update according to:
$$\Delta\psi = -\tilde{x}_{\text{RS}} \cdot s_{\text{yaw}} \cdot k_{\text{inv,x}} \cdot \Delta t$$
$$\Delta\theta = -\tilde{y}_{\text{RS}} \cdot s_{\text{pitch}} \cdot k_{\text{inv,y}} \cdot \Delta t$$
$$\Delta\rho = v_{\text{roll}} \cdot s_{\text{roll}} \cdot k_{\text{inv,roll}} \cdot \Delta t$$
where $v_{\text{roll}} = \mathbf{1}_{\text{RB}} - \mathbf{1}_{\text{LB}}$, default angular sensitivities evaluate $s_{\text{yaw}} = 80^\circ/\text{s}$, $s_{\text{pitch}} = 65^\circ/\text{s}$, and $s_{\text{roll}} = 45^\circ/\text{s}$, with pitch clamped to $[-89^\circ, 89^\circ]$ to prevent gimbal singularity inversion.

#### Multi-Mode Chord & Temporal Window Evaluation
Actions bound to `GamepadBinding` evaluate according to execution mode $M \in \{0, 1, 2, 3\}$:
- *Mode 0 (`Normal`)*: True while the assigned button (and optional modifier button) remains actively depressed.
- *Mode 1 (`Toggle`)*: Flips logical state upon the rising edge $\text{raw} \land \neg\text{prev}$.
- *Mode 2 (`Tap`)*: Triggers on the falling edge $\neg\text{raw} \land \text{prev}$ if press duration satisfies $t_{\text{release}} - t_{\text{press}} \le \tau_{\text{tap}}$ (default $\tau_{\text{tap}} = 0.25\,\text{s}$).
- *Mode 3 (`DoubleTap`)*: Triggers if a secondary tap occurs within elapsed interval $t_{\text{press}, 2} - t_{\text{release}, 1} \le \tau_{\text{double}}$ (default $\tau_{\text{double}} = 0.30\,\text{s}$).

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
| Free-fall infall | Semiclassical geodesic infall plunge: $r(u) = r_{\text{start}} + (r_{\text{end}} - r_{\text{start}})\left(1 - (1 - u)^{p_{\text{acc}}}\right)$, $\theta(u) = \theta_{\text{start}} + (\theta_{\text{end}} - \theta_{\text{start}}) u$, $\phi(u) = 2\pi N_{\text{rev}} \left[(1 - u)u^{1.5} + u \frac{r^{-1} - r_{\text{start}}^{-1}}{r_{\text{end}}^{-1} - r_{\text{start}}^{-1}}\right] + \frac{2 M a}{r^3} t_{\text{local}}$ |
| Conical helix | Conical jet helix: $\mathbf{x}_{\text{center}}(u) = \mathbf{c}_0 + u^{p_{\text{ax}}} L_{\text{axial}} \hat{\mathbf{s}}_{\text{axis}}$, $r(u) = \max\left(\left[r_0 + (r_1 - r_0)u\right] \left[1 + A_{\text{pinch}} \sin(6\pi u)\right], 0\right)$, $\mathbf{x}(u) = \mathbf{x}_{\text{center}}(u) + r(u) (\hat{\mathbf{e}}_1 \cos\phi(u) + \hat{\mathbf{e}}_2 \sin\phi(u))$ with $\phi(u) = \phi_0 + 2\pi N_{\text{rev}} u$ |
| Keplerian orbit | 3D Keplerian elliptical orbit: solves $M(u) = \operatorname{fmod}(M_0 + 2\pi N_{\text{rev}} u, 2\pi) = E - e \sin E$ via Newton-Raphson, true anomaly $\nu = 2\arctan\left(\sqrt{\frac{1+e}{1-e}} \tan\frac{E}{2}\right)$, orbital radius $r = a(1 - e \cos E)$, rotated via Gaussian orbital vectors $\mathbf{x}_{\text{orb}} = \mathbf{c}_0 + r(\cos\nu \, \mathbf{P} + \sin\nu \, \mathbf{Q})$ |
| Dynamic chase | Second-order pursuit camera: target comoving frame $(\hat{\mathbf{v}}_{\text{tgt}}, \hat{\mathbf{r}}, \hat{\mathbf{u}})$, ideal standoff $\mathbf{x}_{\text{ideal}} = \mathbf{x}_{\text{tgt}} - d_{\text{standoff}} \hat{\mathbf{v}}_{\text{tgt}} + \Delta x_{\text{lat}} \hat{\mathbf{r}} + \Delta z_{\text{h}} \hat{\mathbf{u}}$, harmonic response filter $w(t) = 1 - e^{-\zeta \omega_n t}\left(\cos\omega_d t + \frac{\zeta}{\sqrt{1-\zeta^2}} \sin\omega_d t\right)$ ($\omega_d = \omega_n \sqrt{1 - \zeta^2}$), with velocity lead anticipation $\mathbf{x}(t) = \mathbf{x}_{\text{ideal}} + \mathbf{v}_{\text{tgt}} \tau_{\text{lead}} (1 - e^{-2t})$ |

Layers combine as follows with weight $w(u)$ linear between the start and end weights: Replace sets $P = w\,p(u)$, Add sets $P \leftarrow P + w\,p(u)$, and Add Relative sets $P \leftarrow P + w\,(p(u) - p(0))$. Each shape then receives the affine transform $\mathbf{p} \mapsto \mathbf{c}_p + \mathbf{R}(\mathbf{S}(\mathbf{p} - \mathbf{c}_p)) + \mathbf{t}$ with the pivot $\mathbf{c}_p$, scale $\mathbf{S}$, Euler rotation $\mathbf{R}$ and translation $\mathbf{t}$.

A segment transition of duration $T_k$ blends the previous end pose $X_{\text{prev}}$ with the current pose $X$ by $X \leftarrow X_{\text{prev}} + (X - X_{\text{prev}}) \, w(\tau / T_k)$, where $w$ is the easing of the corresponding group (orientation, position or lens). Angular differences use the wrapped difference $\text{wrap}(\Delta) = ((\Delta + 180) \bmod 360) - 180$ in the shortest arc mode.

### 9.3. Sampling and Averaging

For $N$ temporal samples per frame, shutter fraction $\varphi$ and frame interval $\Delta t_f$, the sample times are $t_s = t_f + \left(\frac{s + 0.5}{N} - 0.5\right)\varphi \, \Delta t_f$. With supersampling factor $k$ and fade factor $\Phi$, each output channel is
$$c_{\text{out}} = \text{enc}\left(\frac{\Phi}{k^2 N} \sum_{s=1}^{N} \sum_{i=1}^{k^2} \text{dec}(c_{s,i})\right)$$
where $\text{dec}$ and $\text{enc}$ are the sRGB decoding and encoding functions, implemented with lookup tables of 4096 and 16384 entries. The fade factors are $\min\left(\frac{i + 1}{n_{\text{in}} + 1}, \frac{m}{n_{\text{out}} + 1}\right)$ for frame index $i$ and $m$ remaining frames.

### 9.4. Projection Mode Transitions & Morphing Mathematics

When consecutive motion script segments specify differing optical projection modes $\mathcal{P}_1$ and $\mathcal{P}_2$ (`projection_mode`), `SegmentTransition` evaluates a continuous transition over interval $T_{\text{proj}}$ with transition progress $w = \mathcal{E}_{\text{proj}}(\operatorname{clamp}(t_{\text{local}} / T_{\text{proj}}, 0, 1))$.

The active and secondary projection modes along with the blend weight $w_{\text{blend}} = 1 - w$ are dispatched to the renderer according to `ProjectionTransitionMode`:
1. `StepAtMidpoint` (0): $\mathcal{P}_{\text{active}} = \mathcal{P}_1$ if $w < 0.5$ else $\mathcal{P}_2$.
2. `StepAtStart` (1): $\mathcal{P}_{\text{active}} = \mathcal{P}_1$ if $w = 0$ else $\mathcal{P}_2$.
3. `StepAtEnd` (2): $\mathcal{P}_{\text{active}} = \mathcal{P}_1$ if $w < 1.0$ else $\mathcal{P}_2$.
4. `SmoothMorph` (3): Renders ray directions linearly interpolated between projection mappings in the shader, with $\mathcal{P}_{\text{active}} = \mathcal{P}_2$ ($w \ge 0.5$) and blend factor $1 - w$.
5. `FovCompensatedMorph` (4): Dynamically compensates for focal length scaling discrepancies across projection geometries. With geometric focal scale $f(\mathcal{P})$ (1.0 for Pinhole/AutoZoom/Orthographic, 1.25 for Stereographic, 1.57 for Equirectangular/Hammer, 1.27 for Equidistant, 1.15 for Panini):
   $$\theta_{\text{FOV}}(w) = \operatorname{clamp}\left( \theta_{\text{base}} \cdot \left[ 1 + \left( \frac{f(\mathcal{P}_2)}{f(\mathcal{P}_1)} - 1 \right) (0.5 - |w - 0.5|) \cdot 2 \cdot C_{\text{comp}} \right], \; 5^\circ, \; 175^\circ \right)$$
6. `CrossZoom` (5): Generates a vertigo-style zoom flare pulse during projection morphing:
   $$\theta_{\text{FOV}}(w) = \operatorname{clamp}\left( \theta_{\text{base}} \cdot \left[ 1 + 0.35 C_{\text{comp}} \sin(\pi w) \right], \; 5^\circ, \; 175^\circ \right)$$
7. `WarpCurvature` (6): Evaluates a non-linear curvature progression $w' = 2w^2$ ($w < 0.5$) or $1 - 2(1-w)^2$ ($w \ge 0.5$), coupled with a curvature compression pulse:
   $$\theta_{\text{FOV}}(w) = \operatorname{clamp}\left( \theta_{\text{base}} \cdot \left[ 1 - 0.18 C_{\text{comp}} \sin(\pi w) \right], \; 5^\circ, \; 175^\circ \right)$$

### 9.5. Capture Frame Post-Processing & Triangular Noise Dithering

#### Color Grade Channel Transformation
When `apply_post_processing = true`, captured pixel radiances undergo per-channel color grading (`ColorGrade`):
$$c' = \operatorname{clamp}(c + \ell (1 - c), \; 0, \; 4)$$
$$c'' = \max((c' - 0.5) k_{\text{contrast}} + 0.5, \; 0)$$
$$c''' = (c'')^{1 / \gamma} \cdot G_{\text{gain}}$$
$$c'''' = c''' + \frac{1}{2} H_{\text{rec}} \operatorname{clamp}\left(\frac{c''' - 0.6}{0.4}, 0, 1\right) (1 - c''')$$
$$c''''' = \operatorname{clamp}\left( c'''' + \frac{1}{2} S_{\text{rec}} \operatorname{clamp}\left(1 - \frac{c'''}{0.4}, 0, 1\right) c''', \; 0, \; 1 \right)$$
Color saturation is applied about the luminance $Y = 0.2126 r + 0.7152 g + 0.0722 b$, followed by radial vignette falloff:
$$V(x, y) = 1.0 - v_{\text{vignette}} \min\left( 2 \left( \left(\frac{x + 0.5}{W} - 0.5\right)^2 + \left(\frac{y + 0.5}{H} - 0.5\right)^2 \right), \; 1.0 \right)$$

#### Triangular Noise Dither
To eliminate quantization contouring on 8-bit image formats, high-frequency triangular noise dither is added using 32-bit linear-congruential hashing:
$$c_{\text{final}} = \operatorname{clamp}\left( c + \frac{d_{\text{strength}}}{255} \cdot \Delta_{\text{tri}}(x, y, \text{channel}), \; 0.0, \; 1.0 \right)$$
where $\Delta_{\text{tri}}(x, y, c) = \mathcal{H}(x, y, 2c) + \mathcal{H}(x, y, 2c + 1) - 1.0$, with integer hash:
$$\mathcal{H}(x, y, s) = \left( \left[ \left((x \cdot 0x8DA6B343) \oplus (y \cdot 0xD8163841) \oplus (s \cdot 0xCB1AB31F)\right) \right] \star \right) \cdot \frac{1}{2^{24}}$$
yielding a symmetric triangular probability density function on $[-1.0, 1.0]$ with variance $\sigma^2 = 1/6$.

---

## 10. Distributed Scheduling, Load Balancing & Reduction Mathematics

The mathematical formulations of this section govern the cluster scheduler (`DistributionScheduler`), the telemetry estimators (`LatencyEstimator`, `ThroughputEstimator`), and the parallel reduction kernels (`compute_kernels.cpp`).

### 10.1. Latency-Damped Capacity Share Formulation

For a cluster of $M$ active nodes where each node $i \in \{1, \dots, M\}$ possesses a user-configured static weight $\omega_i > 0$, an exponential moving average ray execution rate $\overline{\Delta t_{\text{exec}, i}}$ (in seconds per $10^3$ rays), and a measured round-trip time $\text{RTT}_i$, the normalized capacity share $\Omega_i(t)$ is:

$$\Omega_i(t) = \frac{\omega_i \cdot \left(\overline{\Delta t_{\text{exec}, i}}\right)^{-1} \cdot \Pi(\text{RTT}_i, \alpha)}{\sum_{j=1}^M \omega_j \cdot \left(\overline{\Delta t_{\text{exec}, j}}\right)^{-1} \cdot \Pi(\text{RTT}_j, \alpha)}$$

where the latency damping function $\Pi(\text{RTT}, \alpha)$ is parameterized by the reference latency $\text{RTT}_0 = 1.0 \times 10^{-3}\,\text{s}$ and damping exponent $\alpha \ge 0$:

$$\Pi(\text{RTT}, \alpha) = \left( \max\left(\frac{\text{RTT}}{\text{RTT}_0}, 1\right) \right)^{-\alpha}$$

For the local host ($i = \text{local}$), $\text{RTT}_{\text{local}} = 0 \implies \Pi(0, \alpha) = 1.0$.

### 10.2. Tile Quota Allocation and Remainder Conservation

For $N_{\text{active}}$ active non-culled screen tiles, initial integer tile quotas $K_i$ are assigned via the floor operator:

$$K_i = \left\lfloor \Omega_i(t) \cdot N_{\text{active}} \right\rfloor$$

To preserve total work conservation ($\sum_{i=1}^M K_i = N_{\text{active}}$), the allocation deficit is accumulated onto the local node:

$$K_{\text{local}} \leftarrow K_{\text{local}} + \left( N_{\text{active}} - \sum_{i=1}^M K_i \right)$$

### 10.3. Dynamic Work-Stealing Deadline Threshold

The execution deadline $t_{\text{deadline}, i}$ for a tile assigned to node $i$ is evaluated from the dispatch epoch $t_{\text{dispatch}}$:

$$t_{\text{deadline}, i} = t_{\text{dispatch}} + \text{clamp}\left( \Delta t_{\text{margin}, i} + t_{\text{compute}, i}, \; t_{\min}, \; t_{\max} \right)$$

where the deadline latency margin $\Delta t_{\text{margin}, i}$ and estimated tile compute duration $t_{\text{compute}, i}$ evaluate:

$$\Delta t_{\text{margin}, i} = \overline{\text{RTT}}_i + 3.5 \, \sigma_{\text{RTT}, i}$$

$$t_{\text{compute}, i} = N_{\text{pixels}} \cdot 10^{-3} \cdot \overline{\Delta t_{\text{exec}, i}} \cdot \frac{\max(C_{\text{tile}}, 1)}{C_{\text{nominal}}}$$

with importance cost factor $C_{\text{tile}} \in [1, 255]$ ($C_{\text{nominal}} = 128$), default bounds $t_{\min} = 4.0\times 10^{-3}\,\text{s}$ and $t_{\max} = 1.2\times 10^{-1}\,\text{s}$ for interactive viewports ($t_{\min} = 0.25\,\text{s}$ and $t_{\max} = 30.0\,\text{s}$ in offline capture mode).

### 10.4. Network Statistical Estimators (RFC 6298 Filter)

The running round-trip time mean $\overline{\text{RTT}}_k$ and variance $\sigma^2_{\text{RTT}, k}$ update upon each ping-pong sample $\text{RTT}_k$ using gain coefficient $\beta = 0.125$:

$$\overline{\text{RTT}}_k = (1 - \beta) \overline{\text{RTT}}_{k-1} + \beta \text{RTT}_k$$

$$\sigma^2_{\text{RTT}, k} = (1 - \beta) \left( \sigma^2_{\text{RTT}, k-1} + \beta (\text{RTT}_k - \overline{\text{RTT}}_{k-1})^2 \right)$$

### 10.5. Parallel Moment Reduction for Stochastic Ensembles (Chan-Golub-LeVeque Algorithm)

When combining two partial Monte Carlo geodesic ensemble records $A$ and $B$ containing sample counts $n_A, n_B$, sample means $\mu_A, \mu_B$, and sum-of-squared differences $M_{2, A}, M_{2, B}$:

$$n_{A \cup B} = n_A + n_B$$

$$\delta = \mu_B - \mu_A$$

$$\mu_{A \cup B} = \mu_A + \delta \frac{n_B}{n_{A \cup B}}$$

$$M_{2, A \cup B} = M_{2, A} + M_{2, B} + \delta^2 \frac{n_A n_B}{n_{A \cup B}}$$

Population variance is extracted as $\sigma^2 = M_{2} / n$.

### 10.6. Post-Newtonian Parallel Row Decomposition

For an $N$-body system, the pairwise post-Newtonian acceleration matrix is partitioned into row blocks of size $B \le 4096$ (`rows_per_unit`), evaluated across two consecutive synchronization phases:

$$\text{Block}_k = \{ i \in \mathbb{N} \mid k \cdot B \le i < \min((k + 1)B, N) \}$$

#### Phase 0: Newtonian Gravitational Potential and Acceleration
For row $i \in \text{Block}_k$, worker threads or GPU compute shaders evaluate:
$$\Phi_i = \sum_{j \neq i}^N \frac{G m_j}{\|\mathbf{r}_{ij}\|}, \quad \mathbf{a}_{i, \text{Newton}} = -\sum_{j \neq i}^N \frac{G m_j \mathbf{r}_{ij}}{\|\mathbf{r}_{ij}\|^3}$$
where $\mathbf{r}_{ij} = \mathbf{x}_i - \mathbf{x}_j$. Evaluated potentials $\Phi_i$ and Newtonian accelerations are assembled by the orchestrator and broadcast to state arrays.

#### Phase 1: 1PN Einstein-Infeld-Hoffmann Relative Acceleration
Given updated potentials $\Phi_i, \Phi_j$ and accelerations $\mathbf{a}_j$, workers evaluate the full 1PN row accelerations:
$$\begin{aligned}
\mathbf{a}_i = -\sum_{j \neq i}^N \frac{G m_j \mathbf{r}_{ij}}{\|\mathbf{r}_{ij}\|^3} \left[ 1 + \frac{1}{c^2}\left( -4\Phi_i - \Phi_j + v_i^2 + 2v_j^2 - 4\mathbf{v}_i \cdot \mathbf{v}_j - \frac{3}{2}(\hat{\mathbf{r}}_{ij} \cdot \mathbf{v}_j)^2 - \frac{1}{2}(\mathbf{r}_{ij} \cdot \mathbf{a}_j) \right) \right] \\
\quad + \frac{1}{c^2} \sum_{j \neq i}^N \frac{G m_j}{\|\mathbf{r}_{ij}\|^3} \left[ \mathbf{r}_{ij} \cdot (4\mathbf{v}_i - 3\mathbf{v}_j) \right] (\mathbf{v}_i - \mathbf{v}_j) + \frac{7}{2 c^2} \sum_{j \neq i}^N \frac{G m_j}{\|\mathbf{r}_{ij}\|} \mathbf{a}_j
\end{aligned}$$

### 10.7. Stochastic Polynomial Chaos Expansion (gPCE) Quadrature Distribution

For a stochastic system with dimension $D$ and polynomial order $P$, multidimensional Gauss-Hermite or Gauss-Legendre quadrature evaluates $N_Q$ discrete nodes $\boldsymbol{\xi}_k$ with quadrature weights $w_k$. The evaluation set is partitioned into disjoint index subsets across $M$ nodes:

$$\text{Range}_m = \{ k \in \mathbb{N} \mid k_{\text{start}, m} \le k < k_{\text{start}, m} + N_m \}, \quad \sum_{m=1}^M N_m = N_Q$$

Each remote worker evaluates the dynamical response $Y(\boldsymbol{\xi}_k)$ for its assigned subset in parallel. The orchestrator reassembles the complete spectral projection:

$$\hat{y}_j = \sum_{m=1}^M \sum_{k \in \text{Range}_m} w_k Y(\boldsymbol{\xi}_k) \Psi_j(\boldsymbol{\xi}_k)$$

Statistical mean and variance are recovered directly from the expansion coefficients:

$$\mathbb{E}[Y] = \hat{y}_0, \quad \operatorname{Var}(Y) = \sum_{j=1}^P \hat{y}_j^2 \langle \Psi_j, \Psi_j \rangle$$

### 10.8. Screen Tile Grid Geometry & Pixel Pitch Decomposition

Viewports of raster dimensions $W \times H$ are discretized into uniform $32 \times 32$ pixel work units (`kTileDim = 32`). The column and row dimensions evaluate:

$$N_{\text{cols}} = \left\lceil \frac{W}{32} \right\rceil = \left\lfloor \frac{W + 31}{32} \right\rfloor, \quad N_{\text{rows}} = \left\lceil \frac{H}{32} \right\rceil = \left\lfloor \frac{H + 31}{32} \right\rfloor, \quad N_{\text{total}} = N_{\text{cols}} \times N_{\text{rows}}$$

For discrete tile index $k \in \{0, \dots, N_{\text{total}} - 1\}$, the spatial tile bounding box coordinates evaluate:

$$c_{\text{col}} = k \bmod N_{\text{cols}}, \quad c_{\text{row}} = \left\lfloor \frac{k}{N_{\text{cols}}} \right\rfloor$$

$$x_{\text{origin}} = c_{\text{col}} \times 32, \quad y_{\text{origin}} = c_{\text{row}} \times 32$$

$$w_k = \min(32, \; W - x_{\text{origin}}), \quad h_k = \min(32, \; H - y_{\text{origin}})$$

The pixel count of tile $k$ satisfies $P_k = w_k \times h_k$. A screen tile is internal when $w_k = h_k = 32$, and boundary-truncated otherwise.

### 10.9. IEEE 754 Half-Precision (FP16) Quantization Bounds

In `QoiHdrRle` compression mode, red, green, blue, and alpha radiances are quantized into IEEE 754 binary16 half-precision floats (`HalfFloat`) with 1 sign bit, 5 exponent bits (bias 15), and 10 explicit mantissa bits ($p = 11$ bits of precision). Values exceeding 65504 clamp to maximum half-float finite magnitude (`0x7BFF`), and subnormals down to $2^{-24} \approx 5.96 \times 10^{-8}$ evaluate round-to-nearest-even tie breaking:

$$\epsilon_{\text{rel}} = \frac{|r - \operatorname{fl}_{16}(r)|}{r} \le 2^{-p} = 2^{-11} \approx 4.8828 \times 10^{-4} < 10^{-3}$$

Quantization error remains strictly below $0.05\%$ across normalized dynamic ranges, preserving colorimetric accuracy while halving radiance payload bytes.

### 10.10. ChaCha20-Poly1305 AEAD Authentication & Anti-Replay Sliding Window

Authenticated encryption follows RFC 8439. Ephemeral X25519 scalar multiplications establish shared secret $K_{\text{dh}}$, from which transmission and reception keys are derived via HKDF-SHA256 with cluster token salt and info string `"RelativisticEngineV1"`.

The 96-bit nonce is formed as $\mathbf{N} = (\text{session\_epoch}, \text{seq} \mathbin{\&} \text{0xFFFFFFFF}, \text{seq} \gg 32)$. ChaCha20 block 0 generates the 256-bit one-time Poly1305 key $(r, s)$ where $r$ is clamped:
$$r \leftarrow r \mathbin{\&} \text{0x0ffffffc0ffffffc0ffffffc0fffffff}$$
ChaCha20 blocks $1, 2, \dots$ generate the keystream XOR-masking the payload plaintext.

The Poly1305 authenticator evaluates MAC tag $\mathbf{T}$ over prime $2^{130} - 5$:
$$\mathbf{T} = \left( \sum_{i=1}^L c_i r^{L - i + 1} \bmod (2^{130} - 5) + s \right) \bmod 2^{128}$$
over the message sequence concatenating the 28-byte authenticated header $H_{\text{AAD}}$ (the first 28 bytes of `PacketEnvelope`), zero-padding to 16 bytes, ciphertext payload $C$, zero-padding to 16 bytes, and 64-bit little-endian lengths $|H_{\text{AAD}}|$ and $|C|$. The resulting 16-byte tag is written into bytes 28 through 43 of `PacketEnvelope`.

Anti-replay protection maintains a 64-packet bitmask window $W$ tracking sequence number $S$ against highest received sequence $S_{\max}$:
- If $S > S_{\max}$: window shifts left by $\Delta = \min(S - S_{\max}, 64)$, bit 0 is set, and $S_{\max} \leftarrow S$.
- If $S \le S_{\max}$ and $S_{\max} - S < 64$: packet is accepted if bit $(S_{\max} - S)$ of $W$ is 0, setting the bit to 1; duplicates are rejected.
- If $S_{\max} - S \ge 64$: packet is discarded.

### 10.11. Interleaved Credit Distribution Algorithm

When partitioning $N_{\text{active}}$ active tiles across $M$ nodes with integer quotas $K_i$ under `TileInterleaveMode::Interleaved`, the scheduler evaluates a Bresenham credit balance accumulator to eliminate spatial tile clustering:
1. Initialize credit accumulators $C_i = 0$ for all nodes $i \in \{1, \dots, M\}$.
2. For each active tile index $k \in \{0, \dots, N_{\text{active}} - 1\}$:
   $$C_i \leftarrow C_i + K_i \quad \forall i \in \{1, \dots, M\}$$
   Select node $i^* = \operatorname{argmax}_i C_i$ among nodes where $K_i > 0$.
   $$C_{i^*} \leftarrow C_{i^*} - N_{\text{active}}$$
   Assign tile $k$ to node $i^*$.
This guarantees uniform spatial distribution of tile workloads across worker nodes regardless of quota disparity.

Anti-replay protection tracks monotonic sequence numbers $S$ using a 64-bit bitmask window $W_{\text{replay}}$ anchored at highest observed sequence number $S_{\max}$:

- If $S > S_{\max}$, the window shifts left by $\min(S - S_{\max}, 64)$ bits, bit 0 is set to 1, and $S_{\max} \leftarrow S$.
- If $S \le S_{\max}$ and $S_{\max} - S < 64$, the packet is accepted if bit $(S_{\max} - S)$ of $W_{\text{replay}}$ is 0, setting the bit to 1.
- If $S_{\max} - S \ge 64$ or the bit is already 1, the packet is rejected as a duplicate or stale transmission.

---

## 11. Orbital Mechanics, Local Frames & Relative Orbital Planes

The mathematical formulations of this section govern the orbital diagnostics and maneuver tools in `BodyManagerContext`, `source_property_editor.hpp`, and `body_catalog_list.hpp`.

### 11.1. Frenet-Serret Local RTN Frame

For a body with relative position $\mathbf{r}$ and velocity $\mathbf{v}$ with respect to a reference attractor, the orthonormal orbital triad $(\hat{\mathbf{R}}, \hat{\mathbf{T}}, \hat{\mathbf{N}})$ evaluates:
$$\hat{\mathbf{R}} = \frac{\mathbf{r}}{r}, \quad \hat{\mathbf{N}} = \frac{\mathbf{r} \times \mathbf{v}}{\|\mathbf{r} \times \mathbf{v}\|}, \quad \hat{\mathbf{T}} = \hat{\mathbf{N}} \times \hat{\mathbf{R}}$$
Instantaneous velocity vector decomposition:
$$\mathbf{v} = v_r \hat{\mathbf{R}} + v_t \hat{\mathbf{T}} + v_n \hat{\mathbf{N}}$$
where $v_r = \mathbf{v} \cdot \hat{\mathbf{R}}$ (radial speed), $v_t = \mathbf{v} \cdot \hat{\mathbf{T}}$ (in-plane tangential speed), and $v_n = \mathbf{v} \cdot \hat{\mathbf{N}}$ (out-of-plane normal drift). Flight-path angle $\gamma$:
$$\sin\gamma = \frac{v_r}{\|\mathbf{v}\|}$$

### 11.2. Relative Orbital Plane Alignment & Rotations

For a target inclination $i_{\text{rel}}$, longitude of ascending node $\Omega_{\text{rel}}$, and argument of periapsis $\omega_{\text{rel}}$ defined relative to the reference plane normal $\hat{\mathbf{n}}_{\text{ref}}$, the orbital plane normal transforms as:
$$\hat{\mathbf{n}}_{\text{orb}} = \sin i_{\text{rel}} (\cos\Omega_{\text{rel}} \hat{\mathbf{u}}_{\text{ref}} + \sin\Omega_{\text{rel}} \hat{\mathbf{v}}_{\text{ref}}) + \cos i_{\text{rel}} \hat{\mathbf{n}}_{\text{ref}}$$
where $\hat{\mathbf{u}}_{\text{ref}}$ and $\hat{\mathbf{v}}_{\text{ref}}$ form an orthonormal basis spanning the reference plane.

### 11.3. Kozai-Lidov Resonant Inclination Window

When a satellite orbits an attractor perturbed by a distant coplanar third body, the relative inclination $i_{\text{rel}}$ exhibits secular Kozai-Lidov resonance oscillations whenever:
$$\cos^2 i_{\text{rel}} \le \frac{3}{5} \iff i_{\text{rel}} \in [39.23^\circ, 140.77^\circ]$$
exchanging mutual inclination for high orbital eccentricity.

### 11.4. Gravitational Hierarchy & Hill Sphere Stability

For a secondary body of mass $m$ orbiting a primary of mass $M$ at separation $r$, the Hill sphere radius is:
$$r_{\text{Hill}} = r \left(\frac{m}{3(M + m)}\right)^{1/3}$$
The satellite orbit occupancy percentage is $\chi_{\text{Hill}} = (d_{\text{rel}} / r_{\text{Hill}}) \times 100\%$, where $\chi_{\text{Hill}} \le 50\%$ indicates long-term orbital stability.

### 11.5. Multi-Body Geometric Pattern Formations

#### Radial Distribution Laws
For mean semi-major axis $\bar{a}$ and radial thickness $\Delta r$, random radial coordinate $r$ is generated according to:
- *Uniform Distribution*: $r = \bar{a} + (u - 0.5) \Delta r$, where $u \sim \mathcal{U}(0, 1)$.
- *Gaussian Distribution*: $r = \bar{a} + \sigma_r \mathcal{N}(0, 1)$, with standard deviation $\sigma_r$.
- *Power-Law Distribution*: $r = \left[ r_{\min}^{1+\alpha} + u (r_{\max}^{1+\alpha} - r_{\min}^{1+\alpha}) \right]^{\frac{1}{1+\alpha}}$, with exponent $\alpha \neq -1$.
- *Kirkwood Resonance Depletion*: Gaps are carved at fractional resonant radii $r_k = \bar{a} + \delta_k \Delta r$ by rejecting candidate positions within a proximity window $\Delta r_{\text{gap}} = 0.035 \Delta r$ with depletion probability $P_{\text{deplete}} \in [0, 1]$.

#### Vertical Scale Height & Out-of-Plane Dispersion
Planar coordinates $(u, v)$ in the common orbital plane defined by inclination $i$, longitude of ascending node $\Omega$, and argument of periapsis $\omega$ are transformed to Cartesian coordinates with vertical offset $z_{\text{off}}$:
$$\mathbf{x} = r (\cos\theta \, \hat{\mathbf{u}} + \sin\theta \, \hat{\mathbf{v}}) + z_{\text{off}} \, \hat{\mathbf{n}}$$
where $\hat{\mathbf{n}} = (\sin i \sin\Omega, -\sin i \cos\Omega, \cos i)$ and $z_{\text{off}} \sim \mathcal{N}(0, h_z)$ for scale height $h_z$.

#### Spherical Formation Models
- *Plummer Sphere Profile*: $r = b \, q^{1/3} / \sqrt{1 - q^{2/3}}$, with Plummer scale radius $b$ and mass fraction $q \sim \mathcal{U}(0, 1)$.
- *Uniform Density Sphere*: $r = R_{\text{sphere}} \, q^{1/3}$, with uniform boundary $R_{\text{sphere}}$.
- *Fibonacci Geodesic Lattice*: For body index $k \in \{0, \dots, N-1\}$, coordinates on a sphere of radius $R$ evaluate:
  $$y_k = 1 - \frac{2k}{N - 1}, \quad r_k = \sqrt{1 - y_k^2}, \quad \phi_k = \pi (3 - \sqrt{5}) k$$
  $$\mathbf{x}_k = R \left( r_k \cos\phi_k, \, r_k \sin\phi_k, \, y_k \right)$$

#### Kinematics, Velocity Dispersion & Mass Functions
- *Keplerian Orbit*: Orbital speed evaluates $v_k = \sqrt{G(M + m) / r}$ with random fractional dispersion $\delta_v \sim \mathcal{N}(0, \sigma_v)$ and prograde probability fraction $\chi_{\text{prograde}} \in [0, 1]$.
- *Isotropic Virial Dispersion*: Velocity components follow three independent Gaussian distributions with dispersion $\sigma_{\text{virial}} = \sqrt{G M / (2r)}$:
  $$\mathbf{v} = \frac{\sigma_{\text{virial}}}{\sqrt{3}} \left( \mathcal{N}_x(0, 1), \, \mathcal{N}_y(0, 1), \, \mathcal{N}_z(0, 1) \right)$$
- *Solid-Body Rotation*: $\mathbf{v} = \boldsymbol{\omega} \times \mathbf{x}$, with angular velocity vector $\boldsymbol{\omega}$ tilted by polar angle $\theta_{\text{rot}}$ and azimuth $\phi_{\text{rot}}$.
- *Differential Mass Function*: Sampled via the power law $dN/dm \propto m^{-\alpha}$ (Dohnanyi cascade $\alpha = 1.833$; Salpeter IMF $\alpha = 2.35$):
  $$m = \left[ m_{\min}^{1-\alpha} + u (m_{\max}^{1-\alpha} - m_{\min}^{1-\alpha}) \right]^{\frac{1}{1-\alpha}}$$
- *Physical Radius from Volumetric Density*:
  $$R = \left( \frac{3 m}{4 \pi \bar{\rho}} \right)^{1/3}$$
- *Equilibrium Radiative Surface Temperature*:
  $$T(r) = T_{\text{ref}} \left( \frac{r_{\text{ref}}}{r} \right)^\beta$$

### 11.6. Orbital Maneuvers & RTN Frame Impulses

#### Instantaneous Delta-V Vector Decomposition
Velocity impulses injected in the local Frenet-Serret RTN frame transform to Cartesian velocity deltas:
$$\Delta\mathbf{v} = \Delta v_r \, \hat{\mathbf{R}} + \Delta v_t \, \hat{\mathbf{T}} + \Delta v_n \, \hat{\mathbf{N}}$$
where $\hat{\mathbf{R}} = \mathbf{r} / r$, $\hat{\mathbf{N}} = (\mathbf{r} \times \mathbf{v}) / \|\mathbf{r} \times \mathbf{v}\|$, and $\hat{\mathbf{T}} = \hat{\mathbf{N}} \times \hat{\mathbf{R}}$.

#### Hohmann Transfer Departure Burn
For departure orbit radius $r_1$ and target destination radius $r_2$ around attractor with gravitational parameter $\mu = G(M + m)$:
$$\Delta v_t = \sqrt{\mu \left( \frac{2}{r_1} - \frac{2}{r_1 + r_2} \right)} - \sqrt{\frac{\mu}{r_1}}$$

#### Isokinetic Plane-Change Burn
To rotate orbital plane inclination by angle $\Delta i$ while maintaining constant orbital speed $v_t$:
$$\Delta v_n = 2 v_t \sin\left(\frac{\Delta i}{2}\right), \quad v_t' = v_t \cos(\Delta i)$$

---

## 12. Procedural Skybox Mathematics

### 12.1. Starfield Generation, Spherical Partitioning & Point Spread Functions

The celestial sphere $\mathbb{S}^2$ is partitioned using an equal-area spherical voxel partition grid of resolution $N_{\text{grid}} = \operatorname{clamp}(14 \sqrt{\rho_{\text{stars}} \cdot Q_{\text{eff}}}, 6, 120)$. For ray travel direction $\mathbf{d} \in \mathbb{S}^2$, spatial grid cell indices evaluate $\mathbf{c} = \lfloor N_{\text{grid}} \mathbf{d} \rfloor$. Candidate star presence in adjacent 27 neighborhood cells $\mathbf{c} + \Delta\mathbf{c}$ ($\Delta c_i \in \{-1, 0, 1\}$) is queried via 64-bit integer hashing:
$$h = \operatorname{hash}_{3\text{D}}(c_x + \Delta c_x, c_y + \Delta c_y, c_z + \Delta c_z, S_{\text{seed}})$$
A star exists when normalized hash $u(h) > 1 - \rho_{\text{stars}} Q_{\text{eff}} \cdot 0.16$. Star direction evaluates from normalized cell offsets $\mathbf{d}_{\text{star}} = \operatorname{normalize}(\mathbf{c} + \Delta\mathbf{c} + \mathbf{u}(h))$. Angular separation satisfies $\theta^2 = 2(1 - \mathbf{d} \cdot \mathbf{d}_{\text{star}})$.

#### Star Point Spread Function (PSF) Models
Given effective optical dispersion $\sigma_{\text{eff}} = \sqrt{R_{\text{star}}^2 \cdot k_{\text{geom}} + \theta_{\text{pixel}}^2 \cdot k_{\text{sub}}}$, angular radiance profiles evaluate according to `StarProfileModel`:
1. `Gaussian` (0):
   $$\mathcal{P}(\theta) = \exp\left(-\frac{\theta^2}{2\sigma_{\text{eff}}^2}\right) + 0.06 \exp\left(-\frac{\theta}{4\sigma_{\text{eff}}}\right)$$
2. `Lorentzian` (1):
   $$\mathcal{P}(\theta) = \frac{\gamma^2}{\gamma^2 + \theta^2} + 0.25 \frac{\gamma^2}{\gamma^2 + 0.25 \theta^2}, \quad \gamma^2 = \sigma_{\text{eff}}^2 \cdot p_1$$
3. `Moffat` (2):
   $$\mathcal{P}(\theta) = \left( 1 + \frac{\theta^2}{p_2 \sigma_{\text{eff}}^2} \right)^{-\beta}, \quad \beta = \operatorname{clamp}(p_1, 1.05, 8.0)$$
4. `AiryDiffraction` (3):
   $$\mathcal{P}(\theta) = \min\left( p_2 \left[\frac{2 J_1(v)}{v}\right]^2, \; 2.0 \right), \quad v = \frac{\pi \theta}{\sigma_{\text{eff}}}$$
   where the first-order Bessel function $J_1(v)$ is evaluated via Chebyshev polynomial approximation.
5. `HardDisc` (4):
   $$\mathcal{P}(\theta) = 1 - \operatorname{clamp}\left( \frac{\theta - (R_{\text{disc}} - w)}{2w}, 0, 1 \right), \quad w = \max(0.4 \theta_{\text{pixel}}, R_{\text{disc}} s)$$
6. `DiffractionSpikes` (5):
   $$\mathcal{P}(\theta) = \exp\left(-\frac{\theta^2}{2\sigma_{\text{eff}}^2}\right) + I_{\text{spikes}} \cdot 2.5 \left| \cos\left(n (\phi_{\text{local}} - \theta_{\text{spike}})\right) \right|^{32} \exp\left(-\frac{\theta}{L_{\text{spike}}}\right)$$
   where $n \in \{2, 3, 4\}$ denotes spike symmetry (4, 6, or 8 spikes) and $L_{\text{spike}} = \sigma_{\text{eff}} \cdot k_{\text{length}}$.
7. `PinpointDirac` (6):
   $$\mathcal{P}(\theta) = \exp\left(-\frac{\theta^2}{2 w_{\text{core}}^2}\right), \quad w_{\text{core}} = \theta_{\text{pixel}} (1 - \eta_{\text{dirac}}) \cdot 0.4$$
8. `CinematicBloom` (7):
   $$\mathcal{P}(\theta) = \exp\left(-\frac{\theta^2}{2\sigma_{\text{eff}}^2}\right) + 1.8 I_{\text{bloom}} \exp\left(-\frac{\theta}{R_{\text{bloom}}}\right)$$

#### Star Placement Probability Densities
Star candidate acceptance in cell hashing evaluates according to `StarPlacementModel`:
1. `UniformEqualArea` (0): $P(\mathbf{d}) = 1.0$.
2. `GalacticDisk` (1): Exponential scale-height falloff from galactic plane $\hat{\mathbf{n}}_{\text{gal}}$:
   $$P(\mathbf{d}) = 0.05 + 0.95 \exp\left(-|\mathbf{d} \cdot \hat{\mathbf{n}}_{\text{gal}}| \cdot k_{\text{conc}}\right)$$
3. `FractalClustered` (2): 3D Perlin/FBM field clustering $P(\mathbf{d}) = \operatorname{clamp}((\mathcal{N}(3.8\mathbf{d}) + 0.6) \cdot 0.85, 0, 1)^3$.
4. `DualPopulation` (3): Bimodal mixture: 70% thin-disk population with $P(\mathbf{d})$ from (1) and 30% spherical isotropic halo.
5. `SpiralArmDensity` (4): Two-arm logarithmic spiral density modulation:
   $$P(\mathbf{d}) = \operatorname{clamp}\left( \exp(-3 |\mathbf{d} \cdot \hat{\mathbf{n}}|) \left[0.15 + 0.85 \left(0.5 + 0.5 \sin(2\phi_{\text{plane}} - 4.5 \ln(|\mathbf{d} \cdot \hat{\mathbf{n}}| + 0.2))\right)\right], 0.02, 1.0 \right)$$
6. `ConstellationAssociations` (5): Spatial clustering via hash noise $P(\mathbf{d}) = 0.1 + 0.9 |\operatorname{hash}_{3\text{D}}(\lfloor 5 \mathbf{d} \rfloor)|$.

#### Parameter Sampling Distributions
Radius, luminosity, and galaxy sizes are sampled via four distribution functions parameterized by uniform hash $u \sim \mathcal{U}(0, 1)$:
- `Uniform` (0): $x = x_{\min} + u (x_{\max} - x_{\min})$.
- `PowerLaw` (1): $x = \left[ (1 - u) x_{\min}^{1-\alpha} + u x_{\max}^{1-\alpha} \right]^{\frac{1}{1-\alpha}}$ (for $\alpha = 1$, $x = x_{\min} (x_{\max}/x_{\min})^u$).
- `Exponential` (2): $x = x_{\min} - \frac{1}{\lambda} \ln\left(1 - u (1 - e^{-\lambda(x_{\max}-x_{\min})})\right)$.
- `LogNormal` (3): $x = \operatorname{clamp}\left(\exp(\mu + \sigma \sqrt{-2\ln u_1} \cos(2\pi u_2)), x_{\min}, x_{\max}\right)$.

#### Blackbody Spectral Chromaticity
Spectral radiance correlates with normalized surface temperature $T^* = \operatorname{clamp}(u_{\text{temp}} + 0.4 \tau_{\text{bias}}, 0, 1)$, mapped to RGB tint factors $(c_r, c_g, c_b)$ via Wien chromatic intervals ($T^* < 0.20$ red M-dwarf, $0.20 \le T^* < 0.45$ orange K-dwarf, $0.45 \le T^* < 0.70$ yellow G-type, $0.70 \le T^* < 0.88$ white A-type, $T^* \ge 0.88$ blue O/B supergiant) scaled by color variation parameter $k_{\text{var}}$.

### 12.2. Volumetric Galactic Nebulae & Fractal Dust Extinction

Galactic diffuse nebulae evaluate along direction $\mathbf{d}$ rotated by Tait-Bryan orientation angles $(\psi_{\text{yaw}}, \theta_{\text{pitch}}, \phi_{\text{roll}})$. The galactic disc envelope models an exponential profile with plane normal $\hat{\mathbf{n}}_{\text{gal}}$:
$$E_{\text{disc}} = \exp\left( - \left|\frac{\mathbf{d} \cdot \hat{\mathbf{n}}_{\text{gal}}}{h_{\text{plane}}}\right|^{k_{\text{falloff}}} \right)$$
Asymmetric galactic core emission towards direction $\hat{\mathbf{a}}_{\text{core}}$ satisfies:
$$E_{\text{asym}} = \left[ \frac{1 + \mathbf{d} \cdot \hat{\mathbf{a}}_{\text{core}}}{2} \right]^{2.5 \alpha_{\text{asym}} + 0.1} + I_{\text{core\_glow}} \exp\left(-2.5 (1 - \mathbf{d} \cdot \hat{\mathbf{a}}_{\text{core}})\right)$$
Divergence-free curl noise perturbing sampling coordinates models turbulent advection:
$$\mathbf{d}' = \operatorname{normalize}\left( \mathbf{d} + \eta_{\text{curl}} \left( \nabla \times \mathbf{\Psi}(\mathbf{d}) \right) \right), \quad \nabla \times \mathbf{\Psi} = \begin{pmatrix} \partial_y \mathcal{N}_z - \partial_z \mathcal{N}_y \\ \partial_z \mathcal{N}_x - \partial_x \mathcal{N}_z \\ \partial_x \mathcal{N}_y - \partial_y \mathcal{N}_x \end{pmatrix}$$

Radiance accumulates across $L$ layers ($L \in [1, 5]$) evaluating three FBM noise fields:
$$I_{\text{nebula}} = E_{\text{disc}} \cdot E_{\text{asym}} \cdot \exp(-\tau_{\text{dust}}) \sum_{l=0}^{L-1} \frac{1}{1 + 0.45 l} \left[ e_{1, l} \mathbf{C}_{\text{emission}} + e_{2, l} \mathbf{C}_{\text{secondary}} \cdot 0.35 + e_{3, l} \mathbf{C}_{\text{tertiary}} \right]$$
where $e_{1, l} = \max(0, \mathcal{N}_1 + 0.18)^{1.8 \gamma_{\text{gas}}}$, $e_{2, l} = \max(0, \mathcal{N}_2 + 0.12)^{2.2 \gamma_{\text{gas}}} \cdot I_{\text{ion}}$, and $e_{3, l} = \max(0, \mathcal{N}_3 + 0.05)^{2.4 \gamma_{\text{gas}}}$.

Interstellar dust extinction follows the Beer-Lambert attenuation law:
$$\tau_{\text{dust}}(\mathbf{d}) = 2.6 \rho_{\text{dust}} \max\left(0, \; \mathcal{N}_{\text{ridged}}(2.6 f_{\text{neb}} \mathbf{d}') - 0.26\right)$$

### 12.3. Deep-Field Background Galaxies & Morphological Profiles

Background galaxies are distributed across spherical rings $N_{\text{rings}} = \operatorname{clamp}(14 \sqrt{\rho_{\text{gal}}}, 4, 36)$. Given projected angular coordinates $(x_{\text{disk}}, y_{\text{disk}} = y_{\text{obs}} / \cos i)$ tilted by inclination $i$, radial distance evaluates $r = \sqrt{x_{\text{disk}}^2 + y_{\text{disk}}^2}$ and azimuth $\phi = \operatorname{atan2}(y_{\text{disk}}, x_{\text{disk}})$. Emission decomposes into central bulge $I_{\text{core}}$ and disk component $I_{\text{disk}}$ governed by `GalaxyMorphologyModel`:
1. `GrandDesignSpiral` (0):
   $$I_{\text{core}} = \exp(-4 r^2), \quad I_{\text{disk}} = \exp(-2.2 r) \left[ 0.5 + 0.5 \cos(N_{\text{arms}} \phi - 4.8 r \tan\psi) \right]$$
2. `FlocculentSpiral` (1):
   $$I_{\text{core}} = \exp(-4 r), \quad I_{\text{disk}} = \exp(-2.5 r) \left[ 0.5 + 0.5 \cos(6\phi - 7 r \tan\psi + 2.5 \mathcal{N}_{\text{clump}}) \right] (1 + k_{\text{clump}} \mathcal{N}_{\text{clump}})$$
3. `BarredSpiral` (2):
   $$I_{\text{bar}} = \exp\left(-\frac{x_{\text{disk}}^2}{2 L_{\text{bar}}^2} - \frac{y_{\text{disk}}^2}{2 w_{\text{bar}}^2}\right), \quad I_{\text{disk}} = 0.7 I_{\text{bar}} + 0.5 \exp(-2.2 r) \left[0.5 + 0.5 \cos(2(\phi - 3.5 r \tan\psi))\right]$$
4. `GiantElliptical` (3): Generalized de Vaucouleurs Sersic profile ($n = k_{\text{sersic}}$):
   $$I_{\text{core}} = \exp\left( - b_n r^{1/n} \right), \quad b_n = 2n - \frac{1}{3} + \frac{4}{405 n}, \quad I_{\text{disk}} = 0$$
5. `Lenticular` (4): Bulge-disk composite with inner resonant dust lane:
   $$I_{\text{core}} = \exp(-3.5 r), \quad I_{\text{disk}} = \exp(-1.8 r) \left[ 1 - \tau_{\text{dust}} \exp\left(-\frac{(r - 0.65)^2}{0.008}\right) \right]$$
6. `RingGalaxy` (5): Resonant collisional ring:
   $$I_{\text{core}} = 1.2 \exp(-8 r^2), \quad I_{\text{disk}} = 1.4 \exp\left(-\frac{(r - 0.70)^2}{2(0.12)^2}\right)$$
Near-side dust lane extinction on inclined systems ($y_{\text{obs}} \sin i > 0$) attenuates radiance by $\exp(-9 |y_{\text{obs}} \sin i - 0.18|) \tau_{\text{dust}}$.

### 12.4. Open & Globular Star Clusters

Clusters are distributed across halo and plane structures governed by `ClusterPlacementModel`:
- `GlobularHalo` (0): Concentrated towards galactic center $\hat{\mathbf{a}}_{\text{core}}$:
  $$P(\mathbf{d}) \propto \left[ 1 + \frac{1 - \mathbf{d} \cdot \hat{\mathbf{a}}_{\text{core}}}{R_{\text{halo}}^2} \right]^{-1}$$
- `OpenClusterPlane` (1): Confined to galactic plane: $P(\mathbf{d}) \propto \exp\left(-(\mathbf{d} \cdot \hat{\mathbf{n}}_{\text{gal}})^2 / h_{\text{plane}}^2\right)$.

Radial surface brightness evaluates via a Plummer potential profile:
$$I_{\text{halo}}(\theta) = I_{\text{cluster}} \cdot \kappa_{\text{halo}} \left[ 1 + \frac{\theta^2}{0.40 R_{\text{cluster}}^2 \cdot s_{\text{spread}}} \right]^{-c_{\text{conc}}}$$
Individual member stars ($M \in [4, 256]$) are positioned by inverting the Plummer mass cumulative distribution:
$$r_m = R_{\text{core}} \sqrt{\frac{u^{2/3}}{1 - u^{2/3}}}, \quad \phi_m = 2\pi u_\phi, \quad u \sim \mathcal{U}(0, 1)$$
Each member evaluates the PSF profile $\mathcal{P}(\theta_m)$ with blackbody spectral temperature tinting.

### 12.5. Quasars & Relativistic Beaming

Quasars combine a compact unresolved nucleus and a collimated relativistic synchrotron jet:
$$I_{\text{quasar}} = I_{\text{core}} \exp\left(-\frac{\theta^2}{2 R_{\text{core}}^2}\right) + I_{\text{jet}} \cdot \mathcal{P}_{\text{jet}}(\mathbf{d}) \cdot \delta^3$$
The jet profile with opening angle $\theta_{\text{open}}$ and shock knots along axis $\hat{\mathbf{s}}_{\text{jet}}$ satisfies:
$$\mathcal{P}_{\text{jet}}(\mathbf{d}) = \exp\left(-\frac{\theta_\perp^2}{2 (\theta_{\text{core}} + \theta_\parallel \tan\theta_{\text{open}})^2}\right) \exp\left(-\frac{\theta_\parallel}{L_{\text{jet}}}\right) \left[ 0.8 + 0.2 \cos(160 \theta_\parallel) \right]$$
where $\theta_\parallel = |(\mathbf{d} - \mathbf{d}_q) \cdot \hat{\mathbf{s}}_{\text{jet}}|$ and $\theta_\perp^2 = \theta^2 - \theta_\parallel^2$.

Relativistic Doppler boosting amplification evaluates with bulk Lorentz factor $\Gamma$ and apparent speed $\beta = \sqrt{1 - 1/\Gamma^2}$:
$$\delta = \frac{1}{\Gamma (1 - \beta \cos\theta_{\text{view}})}, \quad \cos\theta_{\text{view}} = |\mathbf{d} \cdot \hat{\mathbf{s}}_{\text{jet}}|$$

### 12.6. Pulsars & Supernova Remnants

#### Rotating Dipole Pulsar Beam
Pulsar emission incorporates a rotating magnetic axis $\hat{\mathbf{m}}(t)$ inclined by magnetic tilt $\alpha_{\text{tilt}}$ relative to spin axis $\hat{\mathbf{s}}_{\text{pulsar}}$:
$$\hat{\mathbf{m}}(t) = \cos\alpha_{\text{tilt}} \hat{\mathbf{s}}_{\text{pulsar}} + \sin\alpha_{\text{tilt}} \left[ \cos(\omega t + \phi_0) \hat{\mathbf{u}}_1 + \sin(\omega t + \phi_0) \hat{\mathbf{u}}_2 \right]$$
The lighthouse beam emission observed along line-of-sight $\mathbf{d}$ satisfies:
$$I_{\text{pulse}}(t) = \exp\left(-\frac{\theta^2}{2 R_{\text{core}}^2}\right) \exp\left( - \frac{2(1 - |\mathbf{d} \cdot \hat{\mathbf{m}}(t)|)}{2 \theta_{\text{beam}}^2} \right) + I_{\text{PWN}}(\mathbf{d})$$
where the surrounding Pulsar Wind Nebula (PWN) toroidal glow evaluates $I_{\text{PWN}} = \exp\left(-\frac{\theta_\perp^2}{2 R_{\text{pwn}}^2} - \frac{|\theta_\parallel|}{0.4 R_{\text{pwn}}}\right)$.

#### Supernova Remnant (SNR) Shells
The remnant shell is modeled as an expanding triaxial ellipsoid perturbed by fractal filaments:
$$R_{\text{perturbed}}(\mathbf{d}) = R_{\text{base}} \left[ 1 + 0.35 \alpha_{\text{asym}} \mathcal{N}_{\text{FBM}}(30 f_{\text{fil}} \mathbf{d}) \right]$$
Radiance integrates the forward shock shell and reverse shock interior glow:
$$I_{\text{SNR}} = I_{\text{shell}} \exp\left( - \frac{(d - R_{\text{perturbed}})^2}{2 (\Delta R_{\text{shell}})^2} \right) + I_{\text{interior}} \cdot \mathbf{1}_{d < R_{\text{perturbed}}} \exp\left(-\frac{d}{R_{\text{perturbed}}}\right)$$

### 12.7. Celestial Coordinate Grid & Multi-Mode Cell Shading

For unit direction $\mathbf{d}$ rotated by grid orientation angles, spherical celestial coordinates evaluate:
$$\delta = \arcsin(\operatorname{clamp}(d_z, -1, 1)), \quad \alpha = \operatorname{atan2}(d_y, d_x)$$
Grid line coverage evaluates with angular spacing $\Delta\theta_{\text{grid}}$, pixel footprint $\epsilon_{\text{px}} = \max(\theta_{\text{FOV}} / 1080, 10^{-7})$, and line half-width $w$:
$$\operatorname{cov}(\theta, w) = 1 - \operatorname{clamp}\left( \frac{|\operatorname{remainder}(\theta, \Delta\theta_{\text{grid}})| - (w - \epsilon_{\text{px}})}{2 \epsilon_{\text{px}}}, \; 0, \; 1 \right)$$
Major circles evaluate with polar tapering $\kappa_{\text{pole}} = \operatorname{clamp}\left(\frac{\Delta\theta \cos\delta - 0.5 w}{2.5 w}, 0, 1\right)$ eliminating polar line convergence singularities:
- Equator ($\delta = 0$): $\operatorname{cov}_{\text{eq}} = \operatorname{cov}(\delta, w_{\text{eq}}) \cdot \alpha_{\text{eq}}$
- Meridian ($\alpha = 0, \pi$): $\operatorname{cov}_{\text{mer}} = \operatorname{cov}(\alpha, w_{\text{mer}}) \cdot \kappa_{\text{pole}} \cdot \alpha_{\text{mer}}$
- Polar Caps ($|\delta| = \frac{\pi}{2} - \Delta\theta$): $\operatorname{cov}_{\text{polar}} = \operatorname{cov}(|\delta| - (\frac{\pi}{2} - \Delta\theta), w_{\text{polar}}) \cdot \alpha_{\text{polar}}$
- Degree Ticks: evaluated within angular window $\theta_{\text{tick}}$ along equator and central meridian.

#### Cell Fill Modes
Spherical sectors delimited by grid lines ($c_\delta = \lfloor (\delta + \pi/2)/\Delta\theta \rfloor$, $c_\alpha = \lfloor (\alpha + \pi)/\Delta\theta \rfloor$) support 5 fill modes (`GridCellFillMode`):
1. `None` (0): Zero cell background opacity ($\alpha_{\text{cell}} = 0$).
2. `Checkerboard2Color` (1): Binary parity index $k = ((c_\delta + c_\alpha) \bmod 2)$.
3. `CyclicPalette` (2): Cyclic index $k = ((c_\delta + 2 c_\alpha) \bmod N_{\text{colors}})$.
4. `CheckerboardPattern` (3): 2D spatial block index $k = (c_\alpha \bmod 2 + 2(c_\delta \bmod 2)) \bmod N_{\text{colors}})$.
5. `RandomHashFill` (4): Deterministic pseudo-random index $k = \lfloor u(\operatorname{hash}(c_\delta, c_\alpha, S)) \cdot N_{\text{colors}} \rfloor \bmod N_{\text{colors}}$.
Selected palette colors $\mathbf{C}_{\text{cell}}[k]$ blend into the background with opacity $\alpha_{\text{cell}}$.

---

## 13. Synthetic VLBI Interferometry Mathematics

The mathematical formulations of this section govern the synthetic VLBI observation and visibility synthesis pipeline implemented in `VisibilitySynthesizer` (`include/relativistic/interferometry/visibility_synthesis.hpp`) and `InterferometryPanel` (`include/relativistic/ui/interferometry_panel.hpp`).

### 13.1. Van Cittert-Zernike Theorem & Visibility Transform

For an incoherent celestial source with specific intensity distribution $I(\boldsymbol{\theta})$ where $\boldsymbol{\theta} = (\alpha, \beta)$ denotes angular coordinates on the sky in radians, the spatial coherence function (complex visibility) $V(\mathbf{u})$ measured across spatial baseline $\mathbf{u} = (u, v)$ in units of the observing wavelength $\lambda$ satisfies the Fourier transform relation:
$$V(u, v) = \iint_{\mathbb{R}^2} I(\alpha, \beta) \exp\left( -2\pi i (u \alpha + v \beta) \right) d\alpha \, d\beta$$
where $u = B_u / \lambda$ and $v = B_v / \lambda$.

Due to the reality of the physical sky brightness $I(\alpha, \beta) \in \mathbb{R}$, the visibility exhibits Hermitian symmetry:
$$V(-u, -v) = V^*(u, v)$$

Discrete Fourier evaluation over an $N \times N$ discretized raster with angular pixel pitch $\Delta\theta$:
$$V(u, v) = \sum_{j=0}^{N-1} \sum_{k=0}^{N-1} I_{jk} \exp\left( -2\pi i \left[ u \left(k - \frac{N}{2}\right) \Delta\theta + v \left(\frac{N}{2} - j\right) \Delta\theta \right] \right) (\Delta\theta)^2$$
Normalized to match the total source flux density $S_{\text{tot}}$ in Janskys (Jy):
$$V_{\text{norm}}(u, v) = S_{\text{tot}} \frac{V(u, v)}{V(0, 0)}$$

### 13.2. Angular Scaling & Gravitational Radius Angle

The physical length scale of one gravitational radius $r_g = GM/c^2$ at source distance $D$ subtends the angular scale $\theta_g$:
$$\theta_g = \frac{GM}{c^2 D} \quad [\text{rad}]$$
In microarcseconds ($\mu\text{as}$):
$$\theta_g \, [\mu\text{as}] = \left( \frac{GM}{c^2 D} \right) \times \left( \frac{180 \times 3600 \times 10^6}{\pi} \right) \approx 206264.806 \times 10^6 \left( \frac{GM}{c^2 D} \right)$$
For M87* ($M \approx 6.5 \times 10^9 \, M_\odot, D \approx 16.8 \, \text{Mpc}$): $\theta_g \approx 3.78 \, \mu\text{as}$.
For Sgr A* ($M \approx 4.15 \times 10^6 \, M_\odot, D \approx 8178 \, \text{pc}$): $\theta_g \approx 5.08 \, \mu\text{as}$.

When auto-calibration from the simulation viewport is enabled, the pixel scale in radians evaluates:
$$\Delta\theta_{\text{px}} = \frac{L_{\text{screen}} \cdot \theta_g}{M \cdot N}$$
where $L_{\text{screen}} = 2 r_{\text{obs}} \tan(\theta_{\text{FOV}} / 2)$ is the physical horizontal screen width in gravitational units.

### 13.3. Baseline Track Geometry in the $(u, v)$ Plane

Given stations $i$ and $j$ on the rotating Earth with geocentric coordinates $\mathbf{X}_i = (X_i, Y_i, Z_i)^T$ and $\mathbf{X}_j = (X_j, Y_j, Z_j)^T$ in meters, the instantaneous coordinate baseline is $\Delta\mathbf{X} = \mathbf{X}_i - \mathbf{X}_j$.
For source right ascension $\alpha_0$, declination $\delta_0$, and Greenwich Mean Sidereal Time $\text{GMST}(t)$, the local hour angle is $H(t) = \text{GMST}(t) - \alpha_0$.

The spatial baseline components $(u, v, w)$ in units of observing wavelength $\lambda = c / \nu$ evaluate:
$$\begin{pmatrix} u \\ v \\ w \end{pmatrix} = \frac{1}{\lambda} \begin{pmatrix} \sin H & \cos H & 0 \\ -\sin\delta_0 \cos H & \sin\delta_0 \sin H & \cos\delta_0 \\ \cos\delta_0 \cos H & -\cos\delta_0 \sin H & \sin\delta_0 \end{pmatrix} \begin{pmatrix} \Delta X \\ \Delta Y \\ \Delta Z \end{pmatrix}$$
For an orbiting space antenna on a circular Keplerian orbit at altitude $h$, inclination $i_{\text{orb}}$, right ascension of ascending node $\Omega_{\text{orb}}$, and orbital phase $\omega t + \phi_0$, coordinates $\mathbf{X}(t)$ are projected into the geocentric frame prior to baseline evaluation.

Given sky position angle $\psi$ of the image orientation measured East of North:
$$\begin{pmatrix} u_{\text{img}} \\ v_{\text{img}} \end{pmatrix} = \begin{pmatrix} \cos\psi & -\sin\psi \\ \sin\psi & \cos\psi \end{pmatrix} \begin{pmatrix} u \\ v \end{pmatrix}$$

### 13.4. Thermal Noise & Quantization Efficiency

Each baseline visibility measurement $V_{ij}$ is corrupted by zero-mean additive complex Gaussian thermal noise:
$$\widetilde{V}_{ij} = V_{ij} + \epsilon_{\text{re}} + i \, \epsilon_{\text{im}}$$
where $\epsilon_{\text{re}}, \epsilon_{\text{im}} \sim \mathcal{N}(0, \sigma_{ij}^2)$ and the 1-sigma baseline thermal noise standard deviation evaluates:
$$\sigma_{ij} = \frac{1}{\eta_{\text{quant}}} \sqrt{\frac{\text{SEFD}_i \cdot \text{SEFD}_j}{2 \cdot \Delta\nu \cdot \tau_{\text{int}}}}$$
where $\text{SEFD}_i, \text{SEFD}_j$ are station System Equivalent Flux Densities in Janskys (Jy), $\Delta\nu$ is observing bandwidth in Hertz, $\tau_{\text{int}}$ is coherent accumulation interval in seconds, and $\eta_{\text{quant}} \approx 0.88$ is the 2-bit correlation quantization efficiency factor.

The signal-to-noise ratio (SNR) on baseline $(i, j)$ evaluates:
$$\text{SNR}_{ij} = \frac{|V_{ij}|}{\sigma_{ij}}$$

### 13.5. Closure Phase Invariance & Thin-Ring Diameter Estimation

#### Closure Phase Invariance
For any closed triangle of three stations $(i, j, k)$, measured visibility phases $\widetilde{\phi}_{ij} = \phi_{ij} + (\theta_i - \theta_j)$ are corrupted by station-dependent atmospheric and instrumental phase delays $\theta_i, \theta_j$.
The bispectrum (triple product) evaluates:
$$B_{ijk} = V_{ij} V_{jk} V_{ki}$$
The closure phase $\Phi_{ijk} = \arg(B_{ijk})$ satisfies:
$$\Phi_{ijk} = \widetilde{\phi}_{ij} + \widetilde{\phi}_{jk} + \widetilde{\phi}_{ki} = (\phi_{ij} + \theta_i - \theta_j) + (\phi_{jk} + \theta_j - \theta_k) + (\phi_{ki} + \theta_k - \theta_i) = \phi_{ij} + \phi_{jk} + \phi_{ki} \pmod{2\pi}$$
Station phase errors cancel identically, providing robust observables of source structural asymmetry.
The 1-sigma closure phase uncertainty in radians evaluates:
$$\sigma_{\Phi, ijk} = \sqrt{\frac{1}{\text{SNR}_{ij}^2} + \frac{1}{\text{SNR}_{jk}^2} + \frac{1}{\text{SNR}_{ki}^2}}$$

#### Thin-Ring Diameter Estimation
For an infinitesimally thin circular ring of angular diameter $d_{\text{ring}}$ and total flux $S_0$, the visibility amplitude satisfies the zero-order Bessel function:
$$|V(q)| = S_0 \left| J_0(\pi d_{\text{ring}} q) \right|$$
where $q = \sqrt{u^2 + v^2}$.
The first zero of $J_0(x)$ occurs at $x_0 \approx 2.40483$. Therefore, the first visibility amplitude null baseline $q_{\text{null}}$ directly determines the ring angular diameter:
$$\pi d_{\text{ring}} q_{\text{null}} = 2.40483 \implies d_{\text{ring}} = \frac{2.40483}{\pi q_{\text{null}}} \approx \frac{0.7655}{q_{\text{null}}} \quad [\text{rad}]$$
In microarcseconds ($\mu\text{as}$):
$$d_{\text{ring}} \, [\mu\text{as}] \approx \frac{0.7655}{q_{\text{null}} \, [\lambda]} \times 206264.806 \times 10^6$$

---

## 14. Benchmark Comparative Statistics & Normalization Mathematics

The mathematical formulations of this section govern the comparative statistical engine and normalization transforms in `BenchmarkWorkbench` (`include/relativistic/ui/benchmark_workbench.hpp`) and `PerformanceAnalysisWindow` (`include/relativistic/ui/performance_analysis_window.hpp`).

### 14.1. Two-Sample Welch's t-Test & Cohen's d Effect Size

When comparing two independent benchmark runs $A$ (baseline) and $B$ (candidate) with sample counts $n_A, n_B$, sample means $\bar{X}_A, \bar{X}_B$, and sample variances $s_A^2, s_B^2$:

#### Welch's t-Statistic & Effective Degrees of Freedom
$$t = \frac{\bar{X}_B - \bar{X}_A}{\sqrt{\frac{s_A^2}{n_A} + \frac{s_B^2}{n_B}}}$$
The Welch-Satterthwaite approximation evaluates the effective degrees of freedom $\nu$:
$$\nu = \frac{\left( \frac{s_A^2}{n_A} + \frac{s_B^2}{n_B} \right)^2}{\frac{(s_A^2 / n_A)^2}{n_A - 1} + \frac{(s_B^2 / n_B)^2}{n_B - 1}}$$
For large samples ($n_A, n_B \ge 30$), the two-tailed $p$-value evaluates via the standard normal complementary error function:
$$p = 2 \left( 1 - \Phi(|t|) \right) = \operatorname{erfc}\left( \frac{|t|}{\sqrt{2}} \right)$$
Differences are classified as statistically significant when $p < 0.05$.

#### 95% Confidence Interval
For standard normal critical value $z_{0.975} \approx 1.95996$:
$$CI_{95\%} = \left[ (\bar{X}_B - \bar{X}_A) - 1.96 \, \sigma_{\Delta}, \; (\bar{X}_B - \bar{X}_A) + 1.96 \, \sigma_{\Delta} \right], \quad \sigma_{\Delta} = \sqrt{\frac{s_A^2}{n_A} + \frac{s_B^2}{n_B}}$$

#### Cohen's d Standardized Effect Size
Pooled standard deviation $s_{\text{pooled}}$:
$$s_{\text{pooled}} = \sqrt{\frac{(n_A - 1)s_A^2 + (n_B - 1)s_B^2}{n_A + n_B - 2}}$$
Cohen's $d$ statistic:
$$d = \frac{\bar{X}_B - \bar{X}_A}{s_{\text{pooled}}}$$
Categorical magnitude classification:
$$|d| < 0.2 \implies \text{Negligible}, \quad 0.2 \le |d| < 0.5 \implies \text{Small}, \quad 0.5 \le |d| < 0.8 \implies \text{Medium}, \quad |d| \ge 0.8 \implies \text{Large}$$

### 14.2. Data Normalization Transforms

Given raw metric series values $y_i$ ($i \in \{1, \dots, N\}$):
1. *Raw*: $\mathcal{T}(y_i) = y_i$.
2. *Logarithmic Base 10*: $\mathcal{T}(y_i) = \log_{10}(\max(y_i, \epsilon))$.
3. *Natural Logarithm*: $\mathcal{T}(y_i) = \ln(\max(y_i, \epsilon))$.
4. *Logarithmic Base 2*: $\mathcal{T}(y_i) = \log_2(\max(y_i, \epsilon))$.
5. *Ratio to Baseline*: For designated baseline run value $y_{\text{base}} \neq 0$:
   $$\mathcal{T}(y_i) = \frac{y_i}{y_{\text{base}}}$$
6. *Percent Change from Baseline*:
   $$\mathcal{T}(y_i) = \left( \frac{y_i}{y_{\text{base}}} - 1 \right) \times 100\%$$
7. *Min-Max Normalization*:
   $$\mathcal{T}(y_i) = \frac{y_i - y_{\min}}{\max(y_{\max} - y_{\min}, 10^{-12})}$$
8. *Z-Score Standardization*:
   $$\mathcal{T}(y_i) = \frac{y_i - \bar{y}}{\max(s_y, 10^{-12})}$$

---

## 15. 3D Spatial Reference & Coordinate Grid Mathematics

The mathematical formulations of this section govern the 3D coordinate reference overlay implemented in `SpatialReferenceRenderer` (`include/relativistic/ui/spatial_reference/spatial_reference_renderer.hpp`) and configured through `SpatialReferenceConfig` (`include/relativistic/ui/spatial_reference/spatial_reference_config.hpp`).

### 15.1. Reference Origin Anchor & Dynamic Positioning

The coordinate frame anchor point $\mathbf{x}_{\text{ref}}$ is resolved according to `SpatialCenterMode`:
- *World Origin* (0): $\mathbf{x}_{\text{ref}} = (0, 0, 0)^T$.
- *Custom Point* (1): $\mathbf{x}_{\text{ref}} = \mathbf{x}_{\text{custom}}$.
- *Body Center* (2): $\mathbf{x}_{\text{ref}} = \mathbf{x}_{\text{body}}(t)$ tracked dynamically.
- *Camera Snapped Local Frame* (3): To maintain reference lines within the observer's field of view across large spatial translations without origin drift, coordinates snap to integer multiples of the adaptive grid spacing $s_{\text{grid}}$:
  $$\mathbf{x}_{\text{ref}, i} = \operatorname{round}\left( \frac{\mathbf{x}_{\text{cam}, i}}{s_{\text{grid}}} \right) \cdot s_{\text{grid}}, \quad i \in \{x, y, z\}$$

### 15.2. Adaptive Logarithmic "Nice Number" Spacing Formulation

When spacing is set to `SpatialSizeMode::Automatic`, tick intervals, grid line separations, and ring radii adapt dynamically to the observer's distance to the reference center $d_{\text{focus}} = \|\mathbf{x}_{\text{ref}} - \mathbf{x}_{\text{cam}}\|$:
$$s_{\text{raw}} = \frac{d_{\text{focus}}}{k_{\text{div}}}, \quad k_{\text{div}} \in \{6.0, 8.0, 14.0\}$$
Let $L_{\text{unit}} = \ell_{\text{unit}} / \ell_0$ denote the conversion ratio from the active display distance unit (e.g., meters, astronomical units, light years) to simulation units. The raw value in display units evaluates $v = s_{\text{raw}} / L_{\text{unit}}$.
The decade scale $b = 10^{\lfloor \log_{10} v \rfloor}$ and normalized mantissa $m = v / b \in [1, 10)$ yield the rounded "nice" step:
$$\operatorname{nice}(v) = \begin{cases} 1.0 \cdot b & \text{if } m < 1.5 \\ 2.0 \cdot b & \text{if } 1.5 \le m < 3.5 \\ 5.0 \cdot b & \text{if } 3.5 \le m < 7.5 \\ 10.0 \cdot b & \text{if } m \ge 7.5 \end{cases}$$
The final spacing in simulation units evaluates:
$$s = \operatorname{nice}\left( \frac{s_{\text{raw}}}{L_{\text{unit}}} \right) \cdot L_{\text{unit}}$$

### 15.3. Projection, Weak-Field Deflection & Line Clipping

Given camera spatial position $\mathbf{x}_{\text{cam}}$, orthonormal orientation triad $(\hat{\mathbf{f}}, \hat{\mathbf{r}}, \hat{\mathbf{u}})$, and world point $\mathbf{x}$, the coordinate vector evaluates:
$$\Delta\mathbf{x} = \mathbf{x} - \mathbf{x}_{\text{cam}}, \quad f = \Delta\mathbf{x} \cdot \hat{\mathbf{f}}, \quad r = \Delta\mathbf{x} \cdot \hat{\mathbf{r}}, \quad u = \Delta\mathbf{x} \cdot \hat{\mathbf{u}}$$
When the weak-field lens approximation is enabled (`lens_approximation = true`) and central mass $M > 0$:
$$r_{\text{cam}} = \|\mathbf{x}_{\text{cam}}\|, \quad r_{\text{ray}} = \|\Delta\mathbf{x}\|$$
$$\hat{\mathbf{l}} = -\frac{\mathbf{x}_{\text{cam}}}{r_{\text{cam}}}, \quad l_f = \hat{\mathbf{l}} \cdot \hat{\mathbf{f}}, \quad l_r = \hat{\mathbf{l}} \cdot \hat{\mathbf{r}}, \quad l_u = \hat{\mathbf{l}} \cdot \hat{\mathbf{u}}$$
$$\cos\psi = \operatorname{clamp}\left( \frac{f \cdot l_f + r \cdot l_r + u \cdot l_u}{r_{\text{ray}}}, -1, 1 \right)$$
$$b_{\text{impact}} = \max\left( r_{\text{cam}} \sqrt{\max(1 - \cos^2\psi, 0)}, \; 2.05 M \right)$$
$$\delta = \operatorname{clamp}\left( \frac{4M}{b_{\text{impact}}}, 0, 0.35 \right)$$
The deflected forward, right, and up coordinates evaluate:
$$f' = f + r_{\text{ray}} \cdot \delta \cdot l_f, \quad r' = r + r_{\text{ray}} \cdot \delta \cdot l_r, \quad u' = u + r_{\text{ray}} \cdot \delta \cdot l_u$$
Direction coordinates $(f', r', u')$ project onto normalized screen coordinates $(u_{\text{screen}}, v_{\text{screen}})$ via the active projection geometry $\mathcal{P}$.

For straight segments connecting $\mathbf{x}_A$ and $\mathbf{x}_B$, front clipping enforces $f \ge d_{\text{near}} = 10^{-3}$:
$$\mathbf{x}(t) = \mathbf{x}_A + t (\mathbf{x}_B - \mathbf{x}_A), \quad t_{\text{clip}} = \frac{d_{\text{near}} - f_A}{f_B - f_A}$$
Subdivided segments are tested against the screen rectangle $[x_{\min} - \Delta, x_{\max} + \Delta] \times [y_{\min} - \Delta, y_{\max} + \Delta]$ ($\Delta = 64\,\text{px}$) via the Liang-Barsky parametric clipping algorithm.

### 15.4. Continuous Distance Fading, Radial Attenuation & Horizon Occlusion

Line alpha $\alpha$ evaluates the product of user opacity and spatial attenuation factors:
$$\alpha = \alpha_0 \cdot \alpha_{\text{global}} \cdot w_{\text{depth}} \cdot w_{\text{radial}} \cdot w_{\text{horizon}}$$

#### Continuous Depth Fading
$$w_{\text{depth}} = \begin{cases} \operatorname{clamp}\left( \frac{d_{\text{fade\_ref}}}{\max(\|\mathbf{x} - \mathbf{x}_{\text{cam}}\|, 10^{-9})}, \; w_{\text{min}}, \; 1.0 \right) & \text{if enabled} \\ 1.0 & \text{otherwise} \end{cases}$$
where $d_{\text{fade\_ref}}$ defaults to $2.5 \cdot d_{\text{focus}}$ in automatic mode.

#### Radial Edge Grid Fading
For grid lines extending to distance $R_{\text{extent}}$, a quadratic decay prevents hard geometric cuts at the boundary:
$$w_{\text{radial}} = \operatorname{clamp}\left( 1.0 - \left(\frac{\|\mathbf{x} - \mathbf{x}_{\text{grid\_center}}\|}{R_{\text{extent}}}\right)^2, \; 0.0, \; 1.0 \right)$$

#### Event Horizon Sphere Occlusion
When `hide_behind_horizon = true` and horizon radius $r_h > 0$, the line of sight $\mathbf{r}(t) = \mathbf{x}_{\text{cam}} + t(\mathbf{x} - \mathbf{x}_{\text{cam}})$ ($t \in [0, 1]$) is tested against the central horizon sphere:
$$\|\mathbf{x}_{\text{cam}} + t \mathbf{d}\|^2 \le r_h^2 \iff a t^2 + b t + c \le 0$$
where $a = \|\mathbf{d}\|^2$, $b = 2(\mathbf{x}_{\text{cam}} \cdot \mathbf{d})$, and $c = \|\mathbf{x}_{\text{cam}}\|^2 - r_h^2$. If the discriminant $b^2 - 4ac \ge 0$ and the entry parameter $t_{\text{entry}} = (-b - \sqrt{b^2 - 4ac})/(2a) \in (0, 1)$, then $w_{\text{horizon}} = 0.0$ (occluded), otherwise $1.0$.

### 15.5. Dynamic Orthogonal Scale Bar Calibration

For screen-space scale bar display, a 3D focus probe evaluates at displacement $\mathbf{x}_{\text{focus}} = \mathbf{x}_{\text{cam}} + \hat{\mathbf{f}} \cdot d_{\text{focus}}$ with orthogonal test offset $\mathbf{x}_{\text{probe}} = \mathbf{x}_{\text{focus}} + \hat{\mathbf{r}} \cdot (0.05 d_{\text{focus}})$.
Screen-space projections $\mathbf{p}_{\text{focus}}$ and $\mathbf{p}_{\text{probe}}$ yield the local pixel density:
$$\rho_{\text{px}} = \frac{\|\mathbf{p}_{\text{probe}} - \mathbf{p}_{\text{focus}}\|}{0.05 d_{\text{focus}}} \quad [\text{pixels / simulation unit}]$$
Given target screen width $W_{\text{target}}$ (default $160\,\text{px}$), the physical length in display units evaluates:
$$L_{\text{bar}} = \operatorname{nice}\left( \frac{W_{\text{target}}}{\rho_{\text{px}} \cdot L_{\text{unit}}} \right) \cdot L_{\text{unit}}$$
The rendered scale bar width on screen evaluates $W_{\text{bar}} = L_{\text{bar}} \cdot \rho_{\text{px}}$.

---

## 16. Multi-Observer Secondary Viewports & Relativistic Probe Kinematics

The mathematical formulations of this section govern the independent auxiliary observer viewports implemented in `SecondaryViewWindow` (`include/relativistic/ui/secondary_view_window.hpp`) and managed through `SecondaryViewportManager` (`include/relativistic/ui/secondary_viewport_manager.hpp`).

### 16.1. Observer Kinematic Roles & Coordinate Frames

Each secondary viewport $k$ evaluates an independent observer state $(\mathbf{x}_k, \hat{\mathbf{f}}_k, \hat{\mathbf{r}}_k, \hat{\mathbf{u}}_k, \mathbf{\beta}_k, \theta_{\text{FOV}, k}, \mathcal{P}_k)$ anchored relative to a target $\mathbf{x}_{\text{tgt}}$:
- *Target Tracking*: $\mathbf{x}_{\text{tgt}}$ is resolved from `SecondaryTargetMode`: origin $(0, 0, 0)$, primary camera position $\mathbf{x}_{\text{cam}}$, primary camera focus point $\mathbf{x}_{\text{cam}} + \hat{\mathbf{f}} d_{\text{orbit}}$, tracked body $\mathbf{x}_{\text{body}}(t)$, or custom coordinate $\mathbf{x}_{\text{custom}}$.
- *Role 0: Free Observer*: Spherical orbital coordinates $(r_k, \theta_k, \phi_k)$ centered on $\mathbf{x}_{\text{tgt}}$:
  $$\mathbf{x}_k = \mathbf{x}_{\text{tgt}} + r_k \begin{pmatrix} \sin\theta_k \cos\phi_k \\ \sin\theta_k \sin\phi_k \\ \cos\theta_k \end{pmatrix}$$
  The gaze vector $\hat{\mathbf{f}}_k$ directs toward $\mathbf{x}_{\text{tgt}}$ with optional yaw ($\psi_k$) and pitch ($\vartheta_k$) offsets.
- *Role 3: Equirectangular 360 Panorama*: Spherical panorama mapped over $4\pi$ steradians with $\mathcal{P}_k = \text{Equirectangular360}$ and $\theta_{\text{FOV}} = 60^\circ$.

### 16.2. Comoving Free-Fall Probe Kinematics & Velocity Boost 4-Vector

Under *Role 1 (`ComovingProbe`)*, the secondary viewport models the comoving reference frame of a massive test vehicle in radial free-fall from infinity in Schwarzschild or Kerr spacetime.

#### Radial Free-Fall Speed
At radial distance $r = \|\mathbf{x}_k\|$, the coordinate-independent physical speed relative to a local static (ZAMO) observer evaluates:
$$v_{\text{ff}}(r) = \sqrt{\min\left( \frac{2GM}{c^2 r}, \; 0.999^2 \right)} \quad [c]$$
Regulated by user speed fraction $\kappa_v \in [0, 1]$ and direction sign $\eta_{\text{dir}} = -1$ (inbound) or $+1$ (outbound):
$$v_{\text{probe}} = \kappa_v \cdot v_{\text{ff}}(r)$$
$$\hat{\mathbf{v}}_{\text{probe}} = \eta_{\text{dir}} \frac{\mathbf{x}_k}{\|\mathbf{x}_k\|}$$

#### Observer Tetrad Boost Vector
The velocity vector $\mathbf{v}_{\text{probe}}$ is projected onto the observer's local orthonormal tetrad $(\hat{\mathbf{f}}_k, \hat{\mathbf{u}}_k, \hat{\mathbf{r}}_k)$ to formulate the boost 4-vector transferred to the null geodesic integrator:
$$\mathbf{\beta} = v_{\text{probe}} \begin{pmatrix} \hat{\mathbf{v}}_{\text{probe}} \cdot \hat{\mathbf{f}}_k \\ \hat{\mathbf{v}}_{\text{probe}} \cdot \hat{\mathbf{u}}_k \\ \hat{\mathbf{v}}_{\text{probe}} \cdot \hat{\mathbf{r}}_k \\ 0 \end{pmatrix}$$
The corresponding bulk Lorentz factor satisfies:
$$\gamma = \frac{1}{\sqrt{1 - \|\mathbf{\beta}\|^2}}$$
In the null tracer, initial light ray 4-momenta $p^\mu$ undergo Lorentz transformation under $\mathbf{\beta}$, yielding kinematic relativistic aberration and line-of-sight Doppler frequency shifting:
$$\cos\alpha_{\text{obs}} = \frac{\cos\alpha_{\text{rest}} - \beta}{1 - \beta \cos\alpha_{\text{rest}}}, \quad g = \frac{1}{\gamma (1 - \mathbf{\beta} \cdot \hat{\mathbf{d}})}$$

### 16.3. Asymptotic Global Telephoto Projection

Under *Role 2 (`AsymptoticGlobal`)*, the secondary viewport simulates an observer at spatial infinity observing the central spacetime through a quasi-orthogonal telephoto beam.
- The observer distance $d_{\text{asymp}}$ is positioned at $d_{\text{asymp}} = D \cdot M$ with $D \in [100, 20000]$ (default $1500M$).
- Given user-configured half-extent $H \cdot M$ ($H \in [1, 2000]$, default $40M$), the field of view aperture is derived analytically:
  $$\theta_{\text{FOV}} = 2 \arctan\left( \frac{H \cdot M}{d_{\text{asymp}}} \right) = 2 \arctan\left( \frac{H}{D} \right)$$
- The ray escape radius is expanded to $r_{\text{esc}} = \max(1.5 d_{\text{asymp}}, 50.0)$ to prevent premature ray termination before traversing the background field.

### 16.4. Golden-Ratio Chromatic Distribution for 3D Observer Beacons

When `beacon_enabled = true`, secondary observers inject 3D geometric beacon bodies into the primary raytraced scene, participating in gravitational lensing.
For viewport index $k \in \mathbb{N}$, the primary spherical beacon color is synthesized using the golden-ratio fractional sequence to guarantee maximum perceptual chromatic divergence across arbitrary observer counts:
$$\phi = \frac{\sqrt{5} - 1}{2} \approx 0.61803398875$$
$$\text{Hue}_k = \operatorname{fract}\left( k \cdot \phi \right) \in [0, 1)$$
With fixed saturation $S = 0.80$ and value baseline $V = 1.0$, RGB components evaluate via piecewise chromatic rotation:
$$\text{RGB}_k = \operatorname{HSV\_to\_RGB}(\text{Hue}_k, 0.80, 1.0)$$
The physical beacon radius scales with distance to avoid sub-pixel disappearance or visual clamping:
$$R_{\text{beacon}} = \max\left( \|\mathbf{x}_k - \mathbf{x}_{\text{cam}}\| \times 0.007, \; 0.02 \right)$$
A white heading indicator dot ($R_{\text{dot}} = 0.4 R_{\text{beacon}}$) is positioned at displacement $\mathbf{x}_{\text{dot}} = \mathbf{x}_k + \hat{\mathbf{f}}_k \cdot (2.4 R_{\text{beacon}})$, indicating the observer's instantaneous line-of-sight gaze vector.

### 16.5. Multi-View Dynamic Performance Throttling Power Law

When multiple secondary viewports execute concurrently, cumulative CPU/GPU computational cost scales linearly with active viewport count $N_{\text{active}}$.
When `auto_throttle_enabled = true`, each secondary view's internal render scale factor $\sigma_{\text{eff}}$ is throttled via an exponential power law governed by falloff factor $\lambda_{\text{falloff}} \in [0.4, 1.0]$ (default $0.85$):
$$\sigma_{\text{budget}} = \begin{cases} 1.0 & \text{if } N_{\text{active}} \le 1 \\ \max\left( \lambda_{\text{falloff}}^{N_{\text{active}} - 1}, \; 0.15 \right) & \text{if } N_{\text{active}} > 1 \end{cases}$$
The effective render scale for view $k$ evaluates:
$$\sigma_{\text{eff}, k} = \operatorname{clamp}\left( \sigma_{\text{user}, k} \cdot \sigma_{\text{budget}}, \; 0.05, \; 2.0 \right)$$
bounding total frame time independently of the number of instantiated observers.

---

## 17. Dark Matter Profiles, Triaxial Fields & Collisionless N-Body Dynamics

The mathematical formulations of this section govern the dark matter halo catalog, geodesic lensing deflections, visualization integration, Keplerian orbits, and collisionless N-body dynamics implemented in `include/relativistic/dark_matter/`.

### 17.1. Analytic Density Profiles & Enclosed Mass

For dimensionless radial variable $x = r / a$ with scale radius $a$ and profile shape parameter $\alpha$, the enclosed mass within sphere $r$ evaluates:
$$M(r) = M_{\text{norm}} \cdot f_{\text{enc}}(x, \alpha), \quad M_{\text{norm}} = \frac{M_{\text{halo}}}{f_{\text{enc}}(r_{\text{mass}} / a, \alpha)}$$
where $r_{\text{mass}} = \min(c \cdot a, r_{\text{trunc}})$ denotes boundary radius at concentration $c$. Volumetric density satisfies:
$$\rho(r) = \frac{M_{\text{norm}}}{4\pi a^3} \frac{f'_{\text{enc}}(x, \alpha)}{x^2}$$

#### Profile Enclosed Mass Fractions $f_{\text{enc}}(x)$ & Derivatives $f'_{\text{enc}}(x)$
1. `NavarroFrenkWhite` (0, NFW):
   $$f_{\text{enc}}(x) = \ln(1 + x) - \frac{x}{1 + x}, \quad f'_{\text{enc}}(x) = \frac{x}{(1 + x)^2}$$
2. `Hernquist` (1):
   $$f_{\text{enc}}(x) = \left(\frac{x}{1 + x}\right)^2, \quad f'_{\text{enc}}(x) = \frac{2x}{(1 + x)^3}$$
3. `Burkert` (2, Cored):
   $$f_{\text{enc}}(x) = \ln(1 + x) + \frac{1}{2}\ln(1 + x^2) - \arctan(x), \quad f'_{\text{enc}}(x) = \frac{2x^2}{(1 + x)(1 + x^2)}$$
4. `Einasto` (3):
   $$f_{\text{enc}}(x) = \frac{\gamma\left(\frac{3}{\alpha}, \frac{2}{\alpha} x^\alpha\right)}{\Gamma\left(\frac{3}{\alpha}\right)}, \quad f'_{\text{enc}}(x) = \frac{4\pi x^2}{\Gamma\left(\frac{3}{\alpha}\right)} \left(\frac{2}{\alpha}\right)^{\frac{3}{\alpha}} \frac{\alpha}{4\pi} \exp\left(-\frac{2}{\alpha} x^\alpha\right)$$
5. `Plummer` (4):
   $$f_{\text{enc}}(x) = \frac{x^3}{(1 + x^2)^{3/2}}, \quad f'_{\text{enc}}(x) = \frac{3x^2}{(1 + x^2)^{5/2}}$$
6. `CoredIsothermal` (5):
   $$f_{\text{enc}}(x) = x - \arctan(x), \quad f'_{\text{enc}}(x) = \frac{x^2}{1 + x^2}$$
7. `SingularIsothermal` (6):
   $$f_{\text{enc}}(x) = x, \quad f'_{\text{enc}}(x) = 1$$
8. `Dehnen` (7, $\gamma$-Model):
   $$f_{\text{enc}}(x) = \left(\frac{x}{1 + x}\right)^{3 - \gamma}, \quad f'_{\text{enc}}(x) = \frac{(3 - \gamma) x^{2 - \gamma}}{(1 + x)^{4 - \gamma}}$$
9. `PointMass` (8):
   $$f_{\text{enc}}(x) = 1, \quad f'_{\text{enc}}(x) = 0$$
10. `Jaffe` (9):
    $$f_{\text{enc}}(x) = \frac{x}{1 + x}, \quad f'_{\text{enc}}(x) = \frac{1}{(1 + x)^2}$$
11. `Moore` (10, Steep Cusp):
    $$f_{\text{enc}}(x) = \ln(1 + x^{3/2}), \quad f'_{\text{enc}}(x) = \frac{1.5 \sqrt{x}}{1 + x^{3/2}}$$
12. `Soliton` (11, Fuzzy Dark Matter BEC Core):
    $$\rho(x) = \rho_c \left(1 + 0.091 x^2\right)^{-8}, \quad f_{\text{enc}}(x) = \int_0^x 4\pi u^2 (1 + 0.091 u^2)^{-8} du$$
13. `BetaModel` (12, King Cluster Profile, $\beta = 2/3$):
    $$f_{\text{enc}}(x) = \operatorname{asinh}(x) - \frac{x}{\sqrt{1 + x^2}}, \quad f'_{\text{enc}}(x) = \frac{x^2}{(1 + x^2)^{3/2}}$$

### 17.2. Triaxial Ellipsoidal Coordinates & Frame Rotations

Given centroid $\mathbf{x}_{\text{halo}}(t)$ and Tait-Bryan orientation angles $(\psi, \theta, \phi)$ defining orthonormal basis matrix $\mathbf{B} = \mathbf{R}_z(\psi) \mathbf{R}_y(\theta) \mathbf{R}_x(\phi)$:
$$\mathbf{x}_{\text{loc}} = \mathbf{B} \left(\mathbf{x} - \mathbf{x}_{\text{halo}}(t)\right)$$
Triaxial ellipsoidal radius $m$ parameterized by axis ratios $q_y, q_z \in [0.05, 10.0]$:
$$m = \sqrt{x_{\text{loc}}^2 + \left(\frac{y_{\text{loc}}}{q_y}\right)^2 + \left(\frac{z_{\text{loc}}}{q_z}\right)^2}$$
Gravitational acceleration in local coordinates incorporates softening $\epsilon$:
$$\mathbf{a}_{\text{loc}} = -\frac{G M_{\text{enc}}(\min(m, r_{\text{trunc}})) \, m}{(m^2 + \epsilon^2)^{3/2}} \begin{pmatrix} x_{\text{loc}} / m \\ y_{\text{loc}} / (q_y^2 m) \\ z_{\text{loc}} / (q_z^2 m) \end{pmatrix}$$
Transformed to world coordinates:
$$\mathbf{a}_{\text{world}} = s_{\text{lens}} \mathbf{B}^T \mathbf{a}_{\text{loc}}$$

### 17.3. Weak-Field Geodesic Deflection Kicks & Step Control

For null rays with direction $\hat{\mathbf{v}} = \mathbf{v} / \|\mathbf{v}\|$, cumulative lensing acceleration evaluates $\mathbf{g} = s_{\text{field}} \sum_{i=1}^{N_{\text{halos}}} \mathbf{a}_{\text{world}, i}$. The transverse acceleration component evaluates:
$$\mathbf{g}_\perp = \mathbf{g} - (\mathbf{g} \cdot \hat{\mathbf{v}}) \hat{\mathbf{v}}$$
Deflection angle $\Delta\theta = 2 \|\mathbf{g}_\perp\| |\Delta t| \|\mathbf{v}\|$ is clamped via limiter $\kappa_{\text{lim}} = \min(1.0, 0.2 / \Delta\theta)$. The updated ray velocity evaluates:
$$\mathbf{v}' = \|\mathbf{v}\| \cdot \operatorname{normalize}\left(\mathbf{v} + 2 \|\mathbf{v}\|^2 \Delta t \, \kappa_{\text{lim}} \mathbf{g}_\perp\right)$$
Adaptive geodesic integration step bound:
$$\Delta\lambda_{\text{bound}} = \min_{1 \le i \le N_{\text{halos}}} \left[ \max\left( f_{\text{step}} \max(m_i, 0.5 a_i), \; 10^{-4} \right) \right]$$

### 17.4. Line-of-Sight Emissive Density, Field & Velocity Visualization

Along ray segment $\Delta s$, radiant intensity accumulates across three visualization modes:
1. `Density` (0): $\tau = \int \rho(m) ds = \frac{f'_{\text{enc}}(m/a)}{(m/a)^2} \frac{\Delta s}{a}$.
2. `FieldStrength` (1): $\tau = \frac{\|\mathbf{a}_{\text{loc}}\|}{G M_{\text{norm}} / a^2} \frac{\Delta s}{a}$.
3. `CircularSpeed` (2): $\tau = \frac{v_c^2(m)}{G M_{\text{norm}} / a} \frac{\Delta s}{a} = \frac{M_{\text{enc}}(m)}{m} \frac{a}{M_{\text{norm}}} \frac{\Delta s}{a}$.
Screen emission evaluates via transmittance saturation:
$$\mathbf{C}_{\text{emit}} = I_{\text{vis}} \sum_{i=1}^{N_{\text{halos}}} \frac{\tau_i}{1 + \tau_i} G_{\text{vis}, i} \mathbf{C}_{\text{tint}, i}$$

### 17.5. Halo Keplerian Orbital Dynamics

For halos anchored to an orbital trajectory around attractor mass $M_{\text{att}}$ with semi-major axis $a_{\text{orb}}$, eccentricity $e$, inclination $i_{\text{orb}}$, line of nodes $\Omega_{\text{orb}}$, and periapsis $\omega_{\text{orb}}$:
$$n = \sqrt{\frac{G M_{\text{att}}}{a_{\text{orb}}^3}}, \quad M_{\text{anom}}(t) = n t + \phi_0$$
Eccentric anomaly $E$ is resolved via Newton-Raphson iteration: $E_{k+1} = E_k - \frac{E_k - e\sin E_k - M_{\text{anom}}}{1 - e\cos E_k}$. In-plane coordinates evaluate:
$$x_p = a_{\text{orb}} (\cos E - e), \quad y_p = a_{\text{orb}} \sqrt{1 - e^2} \sin E$$
Rotated to world coordinates:
$$\mathbf{x}_{\text{orbit}}(t) = \begin{pmatrix} x_p (\cos\omega_{\text{orb}}\cos\Omega_{\text{orb}} - \sin\omega_{\text{orb}}\cos i_{\text{orb}}\sin\Omega_{\text{orb}}) - y_p (\sin\omega_{\text{orb}}\cos\Omega_{\text{orb}} + \cos\omega_{\text{orb}}\cos i_{\text{orb}}\sin\Omega_{\text{orb}}) \\ x_p (\cos\omega_{\text{orb}}\sin\Omega_{\text{orb}} + \sin\omega_{\text{orb}}\cos i_{\text{orb}}\cos\Omega_{\text{orb}}) - y_p (\sin\omega_{\text{orb}}\sin\Omega_{\text{orb}} - \cos\omega_{\text{orb}}\cos i_{\text{orb}}\cos\Omega_{\text{orb}}) \\ x_p \sin\omega_{\text{orb}}\sin i_{\text{orb}} + y_p \cos\omega_{\text{orb}}\sin i_{\text{orb}} \end{pmatrix}$$

### 17.6. Subhalo Population Statistical Generation

Subhalos are generated via Monte Carlo sampling from differential mass and spatial distributions:
$$m = \begin{cases} \left[ m_{\min}^{1 - \alpha_m} + u (m_{\max}^{1 - \alpha_m} - m_{\min}^{1 - \alpha_m}) \right]^{\frac{1}{1 - \alpha_m}} & \text{if } \alpha_m \neq 1 \\ m_{\min} \left(\frac{m_{\max}}{m_{\min}}\right)^u & \text{if } \alpha_m = 1 \end{cases}$$
$$r = \left[ r_{\min}^{3 - \alpha_r} + u (r_{\max}^{3 - \alpha_r} - r_{\min}^{3 - \alpha_r}) \right]^{\frac{1}{3 - \alpha_r}}$$
Scale radii scale with subhalo mass: $a = a_{\text{ref}} (m / m_{\text{ref}})^{1/3}$. Coordinates are generated over spherical coordinates with colatitudinal flattening $z \leftarrow z \cdot q_z$.

### 17.7. Collisionless N-Body Dynamics & Barnes-Hut Octree Multipoles

For collisionless systems of $N$ particles, the Barnes-Hut octree groups distant particles into cluster nodes. For node bounding box of size $s$ and separation $d$ to target, the multipole approximation is accepted when:
$$\frac{s^2}{d^2} < \theta^2, \quad \theta \approx 0.6$$
The gravitational acceleration including quadrupole tensor $Q_{ij} = \sum_k [Q_{ij}^{(k)} + m_k (3 d_i d_j - \delta_{ij} d^2)]$ evaluates:
$$\mathbf{a} = \frac{G M_{\text{node}} \mathbf{d}}{(d^2 + \epsilon^2)^{3/2}} + \frac{G \mathbf{Q} \mathbf{d}}{(d^2 + \epsilon^2)^{5/2}} - \frac{5 G (\mathbf{d} \cdot \mathbf{Q} \mathbf{d}) \mathbf{d}}{2 (d^2 + \epsilon^2)^{7/2}}$$
Trajectories are integrated via the symplectic leapfrog scheme:
$$\mathbf{v}^{n+1/2} = \mathbf{v}^n + \frac{\Delta t}{2} \mathbf{a}^n, \quad \mathbf{x}^{n+1} = \mathbf{x}^n + \Delta t \, \mathbf{v}^{n+1/2}, \quad \mathbf{v}^{n+1} = \mathbf{v}^{n+1/2} + \frac{\Delta t}{2} \mathbf{a}^{n+1}$$

### 17.8. Composite Multi-Component Galaxy Model Generation

Galaxies are initialized with exponential stellar disk, Hernquist stellar bulge, and NFW dark matter halo:
1. *Exponential Disk*: Surface density $\Sigma(R) = \Sigma_0 \exp(-R / R_d)$ with vertical profile $\rho_z(z) = \frac{1}{2 z_d} \operatorname{sech}^2(z / z_d)$. Circular speed $v_c^2(R) = R \frac{\partial \Phi_{\text{total}}}{\partial R}$. Radial and azimuthal velocity dispersion: $\sigma_R = \sigma_\phi = 0.1 v_c$, vertical dispersion $\sigma_z = 0.05 v_c$.
2. *Hernquist Bulge*: Density $\rho_b(r) = \frac{M_b a_b}{2\pi r (r + a_b)^3}$, sampled via cumulative mass inversion $r = a_b \frac{\sqrt{u}}{1 - \sqrt{u}}$, with isotropic velocity dispersion $\sigma_b^2 = \frac{G M_b}{6 a_b}$.
3. *NFW Halo*: Particles sampled up to truncation $10 r_s$ via rejection sampling, with isotropic velocity dispersion $\sigma_h = v_c(r) / \sqrt{2}$.
Coordinate orientation transforms via inclination $i$ and node $\Omega$:
$$\mathbf{x}' = \mathbf{R}_z(\Omega) \mathbf{R}_x(i) \mathbf{x}, \quad \mathbf{v}' = \mathbf{R}_z(\Omega) \mathbf{R}_x(i) \mathbf{v}$$

---

## 18. Differential Solvers, Symplectic Integrators & Step Control Mathematics

The mathematical formulations of this section govern the numerical integration schemes implemented in `include/relativistic/integrators/`.

### 18.1. Embedded Adaptive Runge-Kutta Tableaus

Geodesic trajectories are integrated as first-order systems for 8D state vector $\mathbf{y} = (\mathbf{x}, \mathbf{u})^T$:
$$\frac{d\mathbf{x}}{d\lambda} = \mathbf{u}, \quad \frac{du^\mu}{d\lambda} = -\Gamma^\mu_{\alpha\beta} u^\alpha u^\beta$$

#### Infall Radial Step-Size Limiter
For infalling trajectories ($u^r < 0$) in non-Cartesian coordinate gauges, the step size is bounded to prevent radial overshoot:
$$\Delta t_{\text{rad}} = 0.5 \frac{r}{|u^r|}, \quad |\Delta t| \leftarrow \min(|\Delta t|, \max(\Delta t_{\text{rad}}, \Delta t_{\min}))$$

#### Dormand-Prince 5(4) Scheme (RK45)
Seven-stage embedded Runge-Kutta formula with FSAL (First Same As Last) property:
$$c = \left(0, \; \frac{1}{5}, \; \frac{3}{10}, \; \frac{4}{5}, \; \frac{8}{9}, \; 1, \; 1\right)^T$$
$$\mathbf{A} = \begin{pmatrix}
0 & 0 & 0 & 0 & 0 & 0 & 0 \\
1/5 & 0 & 0 & 0 & 0 & 0 & 0 \\
3/40 & 9/40 & 0 & 0 & 0 & 0 & 0 \\
44/45 & -56/15 & 32/9 & 0 & 0 & 0 & 0 \\
19372/6561 & -25360/2187 & 64448/6561 & -212/729 & 0 & 0 & 0 \\
9017/3168 & -355/33 & 46732/5247 & 49/176 & -5103/18656 & 0 & 0 \\
35/384 & 0 & 500/1113 & 125/192 & -2187/6784 & 11/84 & 0
\end{pmatrix}$$
5th-order solution:
$$\mathbf{b}^{(5)} = \left(\frac{35}{384}, \; 0, \; \frac{500}{1113}, \; \frac{125}{192}, \; -\frac{2187}{6784}, \; \frac{11}{84}, \; 0\right)^T$$
Error estimator vector $\mathbf{e} = \mathbf{b}^{(5)} - \mathbf{b}^{(4)}$:
$$\mathbf{e} = \left(\frac{71}{57600}, \; 0, \; -\frac{71}{16695}, \; \frac{71}{1920}, \; -\frac{17253}{339200}, \; \frac{22}{525}, \; -\frac{1}{40}\right)^T$$

#### Cash-Karp 5(4) Scheme
Six-stage embedded Runge-Kutta formula optimized for wide stability intervals:
$$c = \left(0, \; \frac{1}{5}, \; \frac{3}{10}, \; \frac{3}{5}, \; 1, \; \frac{7}{8}\right)^T$$
$$\mathbf{A} = \begin{pmatrix}
0 & 0 & 0 & 0 & 0 & 0 \\
1/5 & 0 & 0 & 0 & 0 & 0 \\
3/40 & 9/40 & 0 & 0 & 0 & 0 \\
3/10 & -9/10 & 6/5 & 0 & 0 & 0 \\
-11/54 & 5/2 & -70/27 & 35/27 & 0 & 0 \\
1631/55296 & 175/512 & 575/13824 & 44275/110592 & 253/4096 & 0
\end{pmatrix}$$
5th-order weights:
$$\mathbf{b}^{(5)} = \left(\frac{37}{378}, \; 0, \; \frac{250}{621}, \; \frac{125}{594}, \; 0, \; \frac{512}{1771}\right)^T$$
4th-order weights:
$$\mathbf{b}^{(4)} = \left(\frac{2825}{27648}, \; 0, \; \frac{18575}{48384}, \; \frac{13525}{55296}, \; \frac{277}{14336}, \; \frac{1}{4}\right)^T$$
Error vector $\mathbf{e} = \mathbf{b}^{(5)} - \mathbf{b}^{(4)}$:
$$\mathbf{e} = \left( \frac{37}{378} - \frac{2825}{27648}, \; 0, \; \frac{250}{621} - \frac{18575}{48384}, \; \frac{125}{594} - \frac{13525}{55296}, \; -\frac{277}{14336}, \; \frac{512}{1771} - \frac{1}{4} \right)^T$$

#### Error Metric Formulation
Component error bounds evaluate:
$$\text{scale}_{x, i} = \text{rtol} \cdot \max(|x_i|, |x_{i, \text{next}}|) + \text{atol}$$
$$\text{scale}_{u, i} = \text{rtol} \cdot \max(|u_i|, |u_{i, \text{next}}|) + \text{atol}$$
Normalized scalar error norm:
$$\text{err} = \max_{0 \le i < 4} \left( \frac{|\Delta x_i|}{\text{scale}_{x, i}}, \; \frac{|\Delta u_i|}{\text{scale}_{u, i}} \right)$$

### 18.2. Vernier 9(8) High-Order 16-Stage Runge-Kutta Scheme

For high-precision long orbital integrations, `Vernier9Integrator` evaluates a 16-stage embedded 9(8) pair.
Stage nodes:
$$c = \left(0, \; \frac{1}{18}, \; \frac{1}{12}, \; \frac{1}{8}, \; \frac{5}{16}, \; \frac{3}{8}, \; \frac{59}{128}, \; \frac{7}{16}, \; \frac{17}{32}, \; \frac{25}{32}, \; \frac{23}{32}, \; \frac{13}{16}, \; \frac{7}{8}, \; 1, \; 1, \; 1\right)^T$$
Ninth-order weights:
$$\mathbf{b}^{(9)} = \left(\frac{2379}{58752}, \; 0, \; 0, \; 0, \; 0, \; 0, \; 0, \; \frac{279841}{3110400}, \; \frac{20873}{182784}, \; \frac{1552}{4995}, \; \frac{13364}{72225}, \; \frac{67456}{465525}, \; \frac{8192}{92025}, \; 0, \; \frac{11}{320}, \; \frac{11}{320}\right)^T$$
Eighth-order embedded weights:
$$\mathbf{b}^{(8)} = \left(\frac{2379}{58752}, \; 0, \; 0, \; 0, \; 0, \; 0, \; 0, \; \frac{279841}{3110400}, \; \frac{20873}{182784}, \; \frac{1552}{4995}, \; \frac{13364}{72225}, \; \frac{67456}{465525}, \; \frac{8192}{92025}, \; \frac{11}{160}, \; 0, \; 0\right)^T$$
Step adaptation uses order $p = 9$ for accepted and $p = 8$ for rejected steps.

### 18.3. Implicit Symplectic Gauss-Legendre Integrators (Orders 4 and 6)

Symplectic implicit Runge-Kutta methods preserve phase space symplectic 2-forms $d\mathbf{p} \wedge d\mathbf{x}$ and conserved Killing invariants identically by satisfying the algebraic symmetry condition:
$$b_i A_{ij} + b_j A_{ji} - b_i b_j = 0 \quad \forall i, j \in \{1, \dots, S\}$$

#### Fourth-Order Gauss-Legendre (2 Stages)
$$c = \left(\frac{1}{2} - \frac{\sqrt{3}}{6}, \; \frac{1}{2} + \frac{\sqrt{3}}{6}\right)^T, \quad b = \left(\frac{1}{2}, \; \frac{1}{2}\right)^T$$
$$\mathbf{A} = \begin{pmatrix} 1/4 & 1/4 - \sqrt{3}/6 \\ 1/4 + \sqrt{3}/6 & 1/4 \end{pmatrix}$$

#### Sixth-Order Gauss-Legendre (3 Stages)
$$c = \left(\frac{1}{2} - \frac{\sqrt{15}}{10}, \; \frac{1}{2}, \; \frac{1}{2} + \frac{\sqrt{15}}{10}\right)^T, \quad b = \left(\frac{5}{18}, \; \frac{4}{9}, \; \frac{5}{18}\right)^T$$
$$\mathbf{A} = \begin{pmatrix}
5/36 & 2/9 - \sqrt{15}/15 & 5/36 - \sqrt{15}/30 \\
5/36 + \sqrt{15}/24 & 2/9 & 5/36 - \sqrt{15}/24 \\
5/36 + \sqrt{15}/30 & 2/9 + \sqrt{15}/15 & 5/36
\end{pmatrix}$$

#### Fixed-Point Picard Stage Iteration
Stage derivative vectors $\mathbf{K}_{x, i}$ and $\mathbf{K}_{u, i}$ satisfy the implicit nonlinear algebraic system:
$$\mathbf{X}_i = \mathbf{x} + \Delta\lambda \sum_{j=1}^S A_{ij} \mathbf{K}_{x, j}, \quad \mathbf{U}_i = \mathbf{u} + \Delta\lambda \sum_{j=1}^S A_{ij} \mathbf{K}_{u, j}$$
$$\mathbf{K}_{x, i} = \mathbf{U}_i, \quad \mathbf{K}_{u, i} = \mathbf{a}(\mathbf{X}_i, \mathbf{U}_i)$$
Iterated from initial estimates $\mathbf{K}_{x, i}^{(0)} = \mathbf{u}$ and $\mathbf{K}_{u, i}^{(0)} = \mathbf{a}(\mathbf{x}, \mathbf{u})$ until residual convergence:
$$\max_{0 \le i < S, \; 0 \le c < 4} \max\left( \left|K_{x, i}^{(m+1)}(c) - K_{x, i}^{(m)}(c)\right|, \; \left|K_{u, i}^{(m+1)}(c) - K_{u, i}^{(m)}(c)\right| \right) < \text{tol}_{\text{implicit}}$$
with tolerance $\text{tol}_{\text{implicit}} = 10^{-14}$ and iteration ceiling $N_{\max} = 100$.
State updates evaluate:
$$\mathbf{x}^{n+1} = \mathbf{x}^n + \Delta\lambda \sum_{i=1}^S b_i \mathbf{K}_{x, i}, \quad \mathbf{u}^{n+1} = \mathbf{u}^n + \Delta\lambda \sum_{i=1}^S b_i \mathbf{K}_{u, i}$$

### 18.4. Hermite 4th-Order Predictor-Corrector Scheme & Aarseth Adaptive Timestep

Evaluates acceleration $\mathbf{a}$ and jerk $\mathbf{j} = \dot{\mathbf{a}}$ via 4th-order centered finite differencing along trajectory tangents:
$$\mathbf{j}(\mathbf{x}, \mathbf{u}, \mathbf{a}) = \frac{-\mathbf{a}(\mathbf{x} + 2\epsilon \mathbf{u}, \mathbf{u} + 2\epsilon \mathbf{a}) + 8\mathbf{a}(\mathbf{x} + \epsilon \mathbf{u}, \mathbf{u} + \epsilon \mathbf{a}) - 8\mathbf{a}(\mathbf{x} - \epsilon \mathbf{u}, \mathbf{u} - \epsilon \mathbf{a}) + \mathbf{a}(\mathbf{x} - 2\epsilon \mathbf{u}, \mathbf{u} - 2\epsilon \mathbf{a})}{12\epsilon}$$
with parameter $\epsilon = 10^{-6}$.

#### Predictor Step
$$\mathbf{x}_p = \mathbf{x}_0 + \mathbf{u}_0 \Delta t + \frac{1}{2} \mathbf{a}_0 \Delta t^2 + \frac{1}{6} \mathbf{j}_0 \Delta t^3$$
$$\mathbf{u}_p = \mathbf{u}_0 + \mathbf{a}_0 \Delta t + \frac{1}{2} \mathbf{j}_0 \Delta t^2$$
Evaluate predicted acceleration $\mathbf{a}_1 = \mathbf{a}(\mathbf{x}_p, \mathbf{u}_p)$ and jerk $\mathbf{j}_1 = \mathbf{j}(\mathbf{x}_p, \mathbf{u}_p, \mathbf{a}_1)$.

#### Hermite Interpolation of Higher Derivatives
Snap $\mathbf{s}_0 = \mathbf{a}^{(2)}_0$ and crackle $\mathbf{c}_0 = \mathbf{a}^{(3)}_0$ evaluate:
$$\mathbf{s}_0 = \frac{-6(\mathbf{a}_0 - \mathbf{a}_1) - \Delta t (4\mathbf{j}_0 + 2\mathbf{j}_1)}{\Delta t^2}$$
$$\mathbf{c}_0 = \frac{12(\mathbf{a}_0 - \mathbf{a}_1) + 6\Delta t (\mathbf{j}_0 + \mathbf{j}_1)}{\Delta t^3}$$

#### Corrector Step
$$\mathbf{x}_c = \mathbf{x}_p + \frac{1}{24} \mathbf{s}_0 \Delta t^4 + \frac{1}{120} \mathbf{c}_0 \Delta t^5$$
$$\mathbf{u}_c = \mathbf{u}_p + \frac{1}{6} \mathbf{s}_0 \Delta t^3 + \frac{1}{24} \mathbf{c}_0 \Delta t^4$$

#### Aarseth Step-Size Criterion
$$\Delta t_{\text{next}} = \eta \sqrt{ \frac{\|\mathbf{a}_1\| \|\mathbf{s}_1\| + \|\mathbf{j}_1\|^2}{\|\mathbf{j}_1\| \|\mathbf{c}_0\| + \|\mathbf{s}_1\|^2} }$$
where $\mathbf{s}_1 = \mathbf{s}_0 + \mathbf{c}_0 \Delta t$, clamped to $[\Delta t_{\min}, \Delta t_{\max}]$.

### 18.5. Digital Filter Adaptive Step-Size Controllers

Given normalized error ratio $\text{err}_n = \max(\text{err}, 10^{-12})$, step factor updates evaluate according to `StepControllerMode`:
- *Standard (Integral controller, order $k = p + 1$)*:
  $$\text{factor} = \left(\frac{1}{\text{err}_n}\right)^{\frac{1}{k}}$$
- *PI30 (Proportional-Integral controller, $\beta_I = 0.7/k, \beta_P = 0.3/k$)*:
  $$\text{factor} = \left(\frac{1}{\text{err}_n}\right)^{\frac{0.7}{k}} \left(\frac{\text{err}_{n-1}}{\text{err}_n}\right)^{\frac{0.3}{k}}$$
- *PID42 (Proportional-Integral-Derivative controller, $\beta_I = 0.6/k, \beta_P = 0.4/k, \beta_D = 0.2/k$)*:
  $$\text{factor} = \left(\frac{1}{\text{err}_n}\right)^{\frac{0.6}{k}} \left(\frac{\text{err}_{n-1}}{\text{err}_n}\right)^{\frac{0.4}{k}} \left(\frac{\text{err}_{n-1}}{\text{err}_{n-2}}\right)^{\frac{0.2}{k}}$$

Step size scaling applies safety factor $S = 0.9$:
$$\Delta\lambda_{n+1} = \Delta\lambda_n \cdot \operatorname{clamp}(S \cdot \text{factor}, \; 0.2, \; 5.0) \quad \text{for accepted steps}$$
$$\Delta\lambda_{\text{retry}} = \Delta\lambda_n \cdot \operatorname{clamp}(S \cdot \text{factor}, \; 0.1, \; 0.9) \quad \text{for rejected steps}$$

### 18.6. Algebraic Invariant Constraint Projection & Horizon Boundary Classification

#### Four-Velocity Norm Constraint Projection
The algebraic invariant evaluates:
$$g_{\mu\nu} u^\mu u^\nu = \begin{cases} -c^2 & \text{for timelike geodesics} \\ 0 & \text{for null geodesics} \end{cases}$$
When residual $|g_{\mu\nu} u^\mu u^\nu - \text{target}| \ge \text{tol}_{\text{inv}} = 10^{-12}$, the time component $u^0$ is projected onto the constraint hypersurface by solving the quadratic equation:
$$A (u^0)^2 + B u^0 + C = 0$$
where:
$$A = g_{00}, \quad B = 2 \sum_{i=1}^3 g_{0i} u^i, \quad C = \sum_{i=1}^3 g_{ii} (u^i)^2 + 2 \sum_{i=1}^3 \sum_{j=i+1}^3 g_{ij} u^i u^j - \text{target}$$
Selecting the root $u^0$ closest to the pre-projection state:
$$r_1 = \frac{-B + \sqrt{B^2 - 4AC}}{2A}, \quad r_2 = \frac{-B - \sqrt{B^2 - 4AC}}{2A}$$
When $|A| < 10^{-14}$ and $|B| > 10^{-14}$, $u^0$ evaluates via linear solution $u^0 = -C / B$.

#### Horizon Event Boundary State Machine
`HorizonDetector` tracks coordinate radius $r$ relative to outer horizon $r_s$:
- *Physical Singularity*: $r \le \delta_{\text{sing}} r_s$ ($\delta_{\text{sing}} = 10^{-4}$). Regional state transitions to `TrajectoryRegion::PhysicalSingularity`, setting `reached_singularity = true` and terminating trajectory progression.
- *Horizon Surface*: $|r - r_s| \le \epsilon_h r_s$ ($\epsilon_h = 10^{-5}$). Regional state transitions to `TrajectoryRegion::HorizonSurface`.
- *Absorption Mode*: When `crossing_mode == HorizonCrossingMode::Absorption`, crossing $r \le r_s$ sets `absorbed_at_horizon = true` and terminates trajectory progression immediately.
- *Continuity Mode*: When `crossing_mode == HorizonCrossingMode::Continuity`, regional state transitions to `TrajectoryRegion::Interior`, continuing numerical integration into the black hole interior until reaching the singularity threshold.

### 18.7. Specialized Null Geodesic Differential Integrators

#### Spherical Planar Null Geodesic RK4 Integrator
For static spherical spacetimes with equatorial planar symmetry ($\theta = \pi/2, u^\theta = 0$), the 5D planar null state $\mathbf{y} = (r, \psi, u^t, u^r, u^\psi)^T$ evaluates via `step_planar_null_rk4`:
$$\frac{dr}{d\lambda} = u^r, \quad \frac{d\psi}{d\lambda} = u^\psi$$
$$\frac{du^t}{d\lambda} = -2 \Gamma^t_{tr} u^t u^r$$
$$\frac{du^r}{d\lambda} = -\left[ \Gamma^r_{tt} (u^t)^2 + \Gamma^r_{rr} (u^r)^2 + \Gamma^r_{\psi\psi} (u^\psi)^2 \right]$$
$$\frac{du^\psi}{d\lambda} = -2 \Gamma^\psi_{r\psi} u^r u^\psi$$
where Christoffel symbols are evaluated dynamically via 8th-order centered finite difference stencils.

#### Schwarzschild Cartesian Null Geodesic RK4 Integrator
To eliminate coordinate singularities at the poles, `step_schwarzschild_null_rk4` converts spherical ray coordinates $(r, \theta, \phi, u^r, u^\theta, u^\phi)$ to Cartesian position and velocity $(\mathbf{r}, \mathbf{v})$:
$$\mathbf{h} = \mathbf{r} \times \mathbf{v}, \quad h^2 = \|\mathbf{h}\|^2$$
The exact second-order null geodesic equation of motion evaluates:
$$\frac{d^2\mathbf{r}}{d\lambda^2} = -\frac{3}{2} \frac{r_s h^2}{r^5} \mathbf{r}$$
Integrated via standard 4th-order Runge-Kutta stepping. Trajectory state converts back to spherical coordinates $(r, \theta, \phi)$ with continuous angle unwrapping relative to previous azimuth:
$$\phi = \phi_{\text{prev}} + \left[ (\operatorname{atan2}(y, x) - \phi_{\text{prev}}) - 2\pi \operatorname{round}\left(\frac{\operatorname{atan2}(y, x) - \phi_{\text{prev}}}{2\pi}\right) \right]$$
When near the polar axis ($\rho = \sqrt{x^2 + y^2} \le 8\epsilon r$), lateral speed $v_\perp = \sqrt{v_x^2 + v_y^2}$ sets $u^\theta = \operatorname{sgn}(z) v_\perp / r$ and $u^\phi = 0$.

#### Kerr-Schild Cartesian Hamiltonian Null Integrator
`step_kerr_schild_null_rk4` integrates Boyer-Lindquist Kerr null geodesics in Cartesian coordinates:
$$R^2 = x^2 + y^2 + z^2, \quad r^2 = \frac{1}{2}\left(R^2 - a^2 + \sqrt{(R^2 - a^2)^2 + 4 a^2 z^2}\right)$$
$$H = \frac{M r^3}{r^4 + a^2 z^2}, \quad \mathbf{k} = \left(\frac{r x + a y}{r^2 + a^2}, \; \frac{r y - a x}{r^2 + a^2}, \; \frac{z}{r}\right)^T$$
Hamiltonian equations of motion:
$$\frac{d\mathbf{x}}{d\lambda} = \mathbf{p} - 2 H s \mathbf{k}, \quad \frac{d\mathbf{p}}{d\lambda} = s^2 \nabla H + 2 H s \nabla(\mathbf{k} \cdot \mathbf{p})$$
where $s = 1 + \mathbf{k} \cdot \mathbf{p}$.
Initial observer 4-momentum $\mathbf{p}$ is lifted via Gram-Schmidt orthonormalization in the local Kerr-Schild metric:
$$e_{(0)}^\mu = \frac{1}{\sqrt{1 - 2H}} \begin{pmatrix} 1 \\ 2H\mathbf{k} \end{pmatrix}, \quad e_{(i)}^\mu = \begin{pmatrix} 2H(\mathbf{k} \cdot \mathbf{v}_{(i)}) \\ \mathbf{v}_{(i)} \end{pmatrix}$$
with normalization $g_{\mu\nu} e^\mu_{(a)} e^\nu_{(b)} = \eta_{ab}$ and null ray construction $\mathbf{p} = \mathbf{e}_{(0)} - \sum_{i=1}^3 n^{(i)} \mathbf{e}_{(i)}$.

---

## 19. Dimensional Analysis, Physical Expressions & Base Vector Algebra

The physical constants, units, and expression evaluation subsystem (`include/relativistic/units/`) implements compile-time and runtime dimensional analysis governing unit conversions and mathematical expressions.

### 19.1. Base Dimension Vector Representation

Physical dimensions are represented by a 6-dimensional integer vector $\mathbf{D} \in \mathbb{Z}^6$ (`DimensionVector`, `include/relativistic/units/expression_engine.hpp`):
$$\mathbf{D} = \begin{pmatrix} d_L \\ d_M \\ d_T \\ d_I \\ d_\Theta \\ d_A \end{pmatrix} \equiv [\text{Length}]^{d_L} \, [\text{Mass}]^{d_M} \, [\text{Time}]^{d_T} \, [\text{Current}]^{d_I} \, [\text{Temperature}]^{d_\Theta} \, [\text{Angle}]^{d_A}$$
- Dimensionless quantity: $\mathbf{D} = \mathbf{0} \iff \sum_{i=1}^6 |d_i| = 0$.
- Algebraic composition:
  $$\mathbf{D}(A \cdot B) = \mathbf{D}(A) + \mathbf{D}(B), \quad \mathbf{D}(A / B) = \mathbf{D}(A) - \mathbf{D}(B), \quad \mathbf{D}(A^k) = k \cdot \mathbf{D}(A) \quad (k \in \mathbb{Z})$$
  $$\mathbf{D}(\sqrt{A}) = \frac{1}{2} \mathbf{D}(A) \quad (\text{valid only if } d_i \equiv 0 \pmod 2 \quad \forall i)$$
- Additive dimensional homogeneity:
  Addition $A + B$ and subtraction $A - B$ require strict dimensional identity $\mathbf{D}(A) == \mathbf{D}(B)$. Violations throw `ExpressionError`.

### 19.2. Physical Quantity Representation & Offset Conversion

A physical quantity $Q$ encapsulates a scalar floating-point value and dimension vector:
$$Q = \langle v, \; \mathbf{D} \rangle, \quad v \in \mathbb{R}, \; \mathbf{D} \in \mathbb{Z}^6$$
Unit definitions in `UnitRegistry` provide scale factor $s_u$, affine offset $o_u$, and dimension $\mathbf{D}_u$.
- Multiplicative unit scaling:
  $$Q' = \langle v \cdot s_u^k, \; \mathbf{D} + k \mathbf{D}_u \rangle$$
- Affine temperature offset conversion (e.g. Celsius, Fahrenheit):
  $$v_{\text{SI}} = (v_{\text{displayed}} + o_u) \cdot s_u, \quad v_{\text{displayed}} = \frac{v_{\text{SI}}}{s_u} - o_u$$
  Units with non-zero offsets ($o_u \ne 0$) cannot be combined in composite products or exponentiated.

### 19.3. Recursive Descent Expression Evaluation Grammar

`ExpressionEvaluator` parses arithmetic text expressions into dimensioned physical quantities using recursive descent:
$$\begin{aligned}
E &\to T \ \{ (+ \mid -) \ T \}^* \\
T &\to U \ \{ (* \mid /) \ U \}^* \\
U &\to (+ \mid -) \ U \mid P \\
P &\to B \ \{ \wedge \ U \}^? \\
B &\to \text{number} \ S^* \mid ( E ) \ S^* \mid \operatorname{sqrt}( E ) \ S^*
\end{aligned}$$
where $S$ denotes a sequence of unit tokens with optional integer exponents (`symbol^k` or `/ symbol^k`).
- Numbers support standard decimal and scientific notation (e.g. `1.98847e30`).
- Exponentiation $B^U$ requires dimensionless exponent $\mathbf{D}(U) == \mathbf{0}$. If $B$ has non-zero dimension, $U$ must evaluate to an integer within numerical tolerance $|u - \operatorname{round}(u)| \le 10^{-9}$.
- Function $\operatorname{sqrt}(E)$ enforces $v(E) \ge 0$ and $d_i(E) \equiv 0 \pmod 2$ for all 6 base dimension components.

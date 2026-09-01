# Relativistic Engine - Mathematical & Theoretical Formulation

## 1. Spacetime Geometries & Line Elements

### 1.1. Schwarzschild Metric (Standard Schwarzschild Coordinates)

$$ds^2 = -\left(1 - \frac{2GM}{c^2 r}\right) c^2 dt^2 + \left(1 - \frac{2GM}{c^2 r}\right)^{-1} dr^2 + r^2 \left(d\theta^2 + \sin^2\theta \, d\phi^2\right)$$

### 1.2. Kerr Metric (Boyer-Lindquist Coordinates)

$$ds^2 = -\left(1 - \frac{2Mr}{\rho^2}\right) c^2 dt^2 - \frac{4Mar\sin^2\theta}{\rho^2} c \, dt d\phi + \frac{\rho^2}{\Delta} dr^2 + \rho^2 d\theta^2 + \frac{\Sigma \sin^2\theta}{\rho^2} d\phi^2$$

where:
$$\rho^2 = r^2 + a^2 \cos^2\theta, \quad \Delta = r^2 - 2Mr + a^2, \quad \Sigma = (r^2 + a^2)^2 - a^2 \Delta \sin^2\theta$$

### 1.3. Kerr-Schild Metric (Regular Cartesian Coordinates)

$$g_{\mu\nu} = \eta_{\mu\nu} + 2 H k_\mu k_\nu$$

where $k_\mu$ is a null, geodesic vector with $k^\mu k_\mu = 0$, and:
$$H = \frac{M r^3}{r^4 + a^2 z^2}, \quad k_\mu = \left(c, \frac{rx + ay}{r^2 + a^2}, \frac{ry - ax}{r^2 + a^2}, \frac{z}{r}\right)$$
$$r^4 - (x^2 + y^2 + z^2 - a^2)r^2 - a^2 z^2 = 0$$

### 1.4. Reissner-Nordström Metric (Charged Black Hole)

$$ds^2 = -f(r) c^2 dt^2 + f(r)^{-1} dr^2 + r^2 (d\theta^2 + \sin^2\theta \, d\phi^2)$$
$$f(r) = 1 - \frac{2GM}{c^2 r} + \frac{G k_e Q^2}{c^4 r^2}$$

### 1.5. Kerr-Newman Metric (Rotating Charged Black Hole)

$$ds^2 = -\frac{\Delta - a^2\sin^2\theta}{\rho^2} c^2 dt^2 - \frac{2a\sin^2\theta (r^2 + a^2 - \Delta)}{\rho^2} c \, dt d\phi + \frac{\rho^2}{\Delta} dr^2 + \rho^2 d\theta^2 + \frac{\Sigma \sin^2\theta}{\rho^2} d\phi^2$$
$$\Delta = r^2 - \frac{2GMr}{c^2} + a^2 + \frac{G k_e Q^2}{c^4}$$

### 1.6. Schwarzschild-de Sitter / Kottler Metric

$$f(r) = 1 - \frac{2GM}{c^2 r} - \frac{\Lambda r^2}{3}$$

### 1.7. FLRW Cosmological Metric

$$ds^2 = -c^2 dt^2 + a^2(t) \left[ \frac{dr^2}{1 - k r^2} + r^2 (d\theta^2 + \sin^2\theta \, d\phi^2) \right]$$
$$H(t) = \frac{\dot{a}}{a} = H_0 \sqrt{\Omega_r a^{-4} + \Omega_m a^{-3} + \Omega_k a^{-2} + \Omega_\Lambda}$$

### 1.8. Morris-Thorne Traversable Wormhole

$$ds^2 = -e^{2\Phi(l)} c^2 dt^2 + dl^2 + r^2(l) (d\theta^2 + \sin^2\theta \, d\phi^2), \quad r(l) = \sqrt{l^2 + b_0^2}$$

### 1.9. Alcubierre Warp Drive Metric

$$ds^2 = -\left(c^2 - v_s^2(t) f^2(r_s)\right) dt^2 - 2 v_s(t) f(r_s) dx dt + dx^2 + dy^2 + dz^2$$
$$f(r_s) = \frac{\tanh(\sigma(r_s + R)) - \tanh(\sigma(r_s - R))}{2\tanh(\sigma R)}$$

---

## 2. Geodesics & Kinematics

### 2.1. Geodesic Differential Equation

$$\frac{d^2 x^\mu}{d\lambda^2} + \Gamma^\mu_{\alpha\beta} \frac{dx^\alpha}{d\lambda} \frac{dx^\beta}{d\lambda} = 0$$

Invariant preservation:

$$g_{\mu\nu} \frac{dx^\mu}{d\lambda} \frac{dx^\nu}{d\lambda} = \begin{cases} 0 & \text{null geodesics (photons)} \\ -c^2 & \text{timelike geodesics (massive observers)} \end{cases}$$

### 2.2. Generalized Doppler Shift Factor

$$g = \frac{\nu_{\text{obs}}}{\nu_{\text{emit}}} = \frac{(p_\mu u^\mu)_{\text{obs}}}{(p_\nu u^\nu)_{\text{emit}}}$$

Specific intensity transformation:

$$I_{\text{obs}}(\nu_{\text{obs}}) = g^3 I_{\text{emit}}(\nu_{\text{emit}}) = g^3 I_{\text{emit}}\left(\frac{\nu_{\text{obs}}}{g}\right), \quad F_{\text{bolometric, obs}} = g^4 F_{\text{bolometric, emit}}$$

### 2.3. Fermi-Walker Tetrad Transport

$$\frac{D_{\text{FW}} e^\mu_{(i)}}{d\tau} = \nabla_u e^\mu_{(i)} + \frac{1}{c^2}\left(a^\mu u_\nu - u^\mu a_\nu\right) e^\nu_{(i)} = 0, \quad a^\mu = \nabla_u u^\mu$$

---

## 3. Accretion Disk Physics (Novikov-Thorne & Page-Thorne)

Equatorial circular orbital angular velocity on Kerr metric:

$$\Omega = \frac{\sqrt{GM}}{r^{3/2} + a \sqrt{GM/c^2}}$$

Radiative surface flux:

$$F(r) = \frac{3GM\dot{M}}{8\pi r^3} \frac{f(x)}{x(x^3 + a_*)}, \quad x = \sqrt{\frac{r}{r_g}}$$

Page-Thorne analytical function $f(x)$:

$$f(x) = x - x_0 - \frac{3}{2}a_* \ln\left(\frac{x}{x_0}\right) - \sum_{i=1}^3 \frac{3(x_i - a_*)^2}{x_i(x_i - x_j)(x_i - x_k)} \ln\left(\frac{x - x_i}{x_0 - x_i}\right)$$

where $x_1, x_2, x_3$ are roots of $x^3 - 3x + 2a_* = 0$.

---

## 4. Post-Newtonian Multi-Body Dynamics

Conservative 1PN Einstein-Infeld-Hoffmann acceleration:

$$\mathbf{a}_i^{\text{1PN}} = \sum_{j \neq i} \frac{G m_j \mathbf{n}_{ji}}{c^2 r_{ij}^2} \left[ v_i^2 + 2v_j^2 - 4\mathbf{v}_i \cdot \mathbf{v}_j - \frac{3}{2}(\mathbf{n}_{ij} \cdot \mathbf{v}_j)^2 - 4\sum_{k \neq i} \frac{G m_k}{r_{ik}} - \sum_{k \neq j} \frac{G m_k}{r_{jk}} \right] + \sum_{j \neq i} \frac{G m_j \mathbf{v}_{ij}}{c^2 r_{ij}^2} \left[ \mathbf{n}_{ji} \cdot (4\mathbf{v}_i - 3\mathbf{v}_j) \right]$$

2.5PN Radiation reaction acceleration:

$$\mathbf{a}_i^{\text{2.5PN}} = \frac{8}{5}\frac{G^2 M^2 \eta}{c^5 r^3} \left[ \left(3v^2 + \frac{17}{3}\frac{GM}{r}\right) \dot{r} \mathbf{n} - \left(v^2 + 3\frac{GM}{r}\right) \mathbf{v} \right]$$

Gravitational wave quadrupole radiation power (Peters & Mathews):

$$P_{\text{GW}} = \frac{G}{5 c^5} \dddot{I}_{ij} \dddot{I}_{ij}$$

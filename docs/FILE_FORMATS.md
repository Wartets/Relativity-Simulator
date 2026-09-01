# Relativistic Engine - Data File Specifications & I/O Protocol

## 1. Scenario Definition Format (`.yaml`)

Scenario definitions are declarative, YAML documents describing initial spacetime conditions, physical properties of celestial bodies, observer kinematics, and output configuration.

### 1.1. Specification Schema

```yaml
scenario_name: "scenario_identifier"
description: "Detailed description of physical setup"
spacetime:
  metric_type: "Schwarzschild | Kerr | Minkowski | ReissnerNordstrom | KerrNewman | SchwarzschildDeSitter | FLRW | MorrisThorne | Alcubierre"
  central_mass: 1.0                  # In geometrized or SI units
  central_spin: 0.0                  # Dimensionless spin a/M in [-1.0, 1.0]
  central_charge: 0.0                # Normalized electric charge
  cosmological_lambda: 0.0           # Cosmological constant Lambda
  wormhole_throat: 1.0               # Throat radius b_0 for Morris-Thorne
  warp_velocity: 0.0                 # Velocity v_s for Alcubierre bubble
  speed_of_light: 1.0                # c (1.0 or 299792458.0)
  gravitational_constant: 1.0        # G (1.0 or 6.67430e-11)
integrator:
  scheme: "RK45 | Vernier9 | CashKarp | GaussLegendre4 | GaussLegendre6 | Hermite4"
  initial_step: 0.01
  min_step: 1.0e-8
  max_step: 10.0
  relative_tolerance: 1.0e-10
  absolute_tolerance: 1.0e-14
output:
  fits_enabled: true
  hdf5_enabled: true
  vtk_enabled: true
  export_directory: "./output"
bodies:
  - name: "BodyName"
    body_id: 1
    mass: 1.0
    radius: 1.0
    spin: 0.0
    charge: 0.0
    position: [t, r, theta, phi]     # Four-position in spacetime coordinates
    velocity: [dt, dr, dtheta, dphi] # Four-velocity components
observers:
  - name: "CameraPrimary"
    fov_deg: 60.0
    resolution: [1920, 1080]
    position: [t, r, theta, phi]
    four_velocity: [u0, u1, u2, u3]
```

---

## 2. Flexible Image Transport System (`.fits`)

Exported FITS files conform to the NASA/IAU FITS Standard 4.0 in 64-bit IEEE 754 floating-point format (`BITPIX = -64`), with big-endian byte order and 2880-byte block padding.

### 2.1. Primary Header Keywords

- `SIMPLE  = T`: Standard FITS conforming file.
- `BITPIX  = -64`: Double-precision floating-point pixel values.
- `NAXIS   = 2` (2D intensity image) or `NAXIS = 3` (3D spectral data cube).
- `NAXIS1`, `NAXIS2`, `NAXIS3`: Spatial and spectral dimensions.
- `BUNIT   = 'W/m2/sr'`: Physical surface radiance units.
- `CRPIXn`, `CRVALn`, `CDELTn`, `CTYPEn`: WCS coordinates (RA---TAN, DEC--TAN, WAVE).

---

## 3. Hierarchical Data Format 5 (`.h5`)

The engine serializes dense trajectories, spacetime metric series, and uncertainty tensors in HDF5 binary containers.

### 3.1. Standard Group Hierarchy

- `/bodies/<name>/position`: $[N \times 4]$ array of four-positions $(t, x^1, x^2, x^3)$ in `Float64`.
- `/bodies/<name>/velocity`: $[N \times 4]$ array of four-velocities $(u^0, u^1, u^2, u^3)$ in `Float64`.
- `/spacetime/metric_series`: $[N \times 4 \times 4]$ array of metric tensors $g_{\mu\nu}$ along the worldline.
- `/uncertainty/covariance`: $[N \times 8 \times 8]$ array of Lyapunov phase covariance matrices.
- `/eos/<model_name>/...`: 3D/4D nuclear equation of state tables (log $\rho$, log $T$, $Y_e$, log $P$, log $\epsilon$, $c_s^2$).

---

## 4. Visualization Toolkit XML PolyData (`.vtp`)

Meshes of black hole event horizons, photon spheres, and bundles of null/timelike geodesics are exported in `.vtp` format for visualization in ParaView or VisIt.

### 4.1. Point Data Attributes

- `Points`: Coordinates in 3-space $(x, y, z)$.
- `AffineParameter`: Affine length $\lambda$ or proper time $\tau$.
- `Redshift`: Generalized Doppler/gravitational frequency ratio $g$.
- `FourVelocity`: Comobile 4-velocity vector $(u^0, u^1, u^2, u^3)$.

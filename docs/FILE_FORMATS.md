# Relativistic Engine - Data File Specifications & I/O Protocol

## 1. Declarative Scenario Definition (`.yaml`)

Scenarios are defined in YAML format & specify initial spacetime properties, numerical integrator settings, observer definitions, & celestial bodies.

### 1.1. Schema Specification

```yaml
format_version: 1                            # Scenario format version; values other than 1 are rejected
scenario_name: "ScenarioName"
description: "Physical scenario description"
author: "Unknown"                            # Attribution stored inside the file
created_at: ""                               # ISO 8601 creation timestamp, optional
version_tag: "1.0.0"                         # Semantic version tag (major.minor.patch)

spacetime:
  metric_type: "Schwarzschild | Kerr | FlatMinkowski | Minkowski | KerrSchild | ReissnerNordstrom | KerrNewman | SchwarzschildDeSitter | FLRW | MorrisThorne | Alcubierre | BSSN"
  central_mass: 1.0                          # Central mass M in geometrized or SI units
  central_spin: 0.0                          # Spin parameter a in [-M, M]
  central_charge: 0.0                        # Electric charge Q
  cosmological_lambda: 0.0                   # Cosmological constant Lambda
  wormhole_throat: 1.0                       # Throat radius b_0 for Morris-Thorne metric
  warp_velocity: 0.0                         # Warp bubble velocity v_s for Alcubierre metric
  speed_of_light: 1.0                        # Speed of light c; validated as strictly positive, not applied to the constants engine on load
  gravitational_constant: 1.0                # Gravitational constant G; validated as strictly positive, not applied to the constants engine on load

integrator:
  scheme: "RK45 | CashKarp | Vernier9 | GaussLegendre4 | GaussLegendre6 | Hermite4" # Free-form name; names containing Symplectic or Gauss select the symplectic N-body integrator
  initial_step: 0.01                         # Initial step size
  min_step: 1.0e-8                           # Minimum step size bound
  max_step: 10.0                             # Maximum step size bound
  relative_tolerance: 1.0e-10                # Relative error tolerance (rtol)
  absolute_tolerance: 1.0e-14                # Absolute error tolerance (atol)

output:
  fits_enabled: true                         # Export FITS images & spectral cubes
  hdf5_enabled: true                         # Export HDF5 state datasets
  vtk_enabled: true                          # Export VTK PolyData geometry
  export_directory: "./output"               # Target destination directory

interactions:
  electricity_enabled: false                 # Coulomb interaction between charged bodies
  magnetism_enabled: false                   # Magnetic dipole interaction
  vacuum_permittivity: 8.8541878128e-12      # Vacuum permittivity used by the electromagnetic solver
  vacuum_permeability: 1.25663706212e-6      # Vacuum permeability used by the electromagnetic solver
  collisions_enabled: false                  # Collision detection and response
  collision_response_model: 0                # 0: Elastic, 1: Inelastic
  collision_consider_rotation: true          # Include rotation in collision resolution
  collision_consider_friction: true          # Include surface friction in collision resolution
  collision_restitution_multiplier: 1.0      # Scale applied to the body restitution coefficients
  collision_stiffness_scale: 1.0e-9          # Contact stiffness scale
  collision_position_correction_factor: 0.2  # Fraction of penetration corrected per step
  thermodynamics_enabled: false              # Radiative heat exchange between bodies and environment
  ambient_temperature_kelvin: -1.0           # Ambient temperature in Kelvin
  radiative_coupling_scale: 1.0              # Scale of the radiative coupling
  fragmentation_enabled: false               # Fragmentation of bodies whose integrity is exhausted
  fragmentation_tidal_stress_enabled: false  # Integrity loss from tidal stress
  minimum_fragment_mass: 1.0e-6              # Smallest mass of a produced fragment
  max_fragments_per_event: 2                 # Maximum number of fragments per fragmentation event
  collision_energy_to_integrity_loss: 1.0e-6 # Integrity loss per unit of collision energy
  tidal_stress_to_integrity_loss: 1.0e-6     # Integrity loss per unit of tidal stress
  annihilation_enabled: false                # Matter-antimatter annihilation
  annihilation_contact_scale: 1.0            # Contact distance scale for annihilation
  annihilation_require_opposite_charge: true # Restrict annihilation to opposite charges

bodies:
  - name: "BodyIdentifier"
    body_id: 1                               # Unique integer identifier
    mass: 1.0                                # Mass in kg or geometrized units
    radius: 1.0                              # Mean physical radius
    spin: 0.0                                # Spin angular momentum magnitude; loaded as the z component of the spin vector and saved as the vector magnitude
    charge: 0.0                              # Electric charge
    enabled: true                            # Whether the body participates in rendering, gravity, and integration
    color: [0.62, 0.75, 1.0, 1.0]            # Primary RGBA display color
    color_secondary: [0.18, 0.30, 0.75, 1.0] # Secondary RGBA display color
    magnetic_moment: 0.0                     # Magnetic dipole moment, used by the electromagnetic interaction solver
    rotation_speed: 0.0                      # Body rotation rate about its spin axis
    friction_coefficient: 0.0                # Surface friction coefficient used in collision resolution
    restitution: 0.5                         # Coefficient of restitution used in elastic collision resolution
    integrity: 1.0                           # Structural integrity consumed by collision and tidal-stress damage
    lifetime: 0.0                            # Optional lifetime bookkeeping value
    temperature: 0.0                         # Body temperature in Kelvin, used by the thermodynamics interaction solver
    heat_capacity: 0.0                       # Heat capacity used by the thermodynamics interaction solver
    absorption_factor: 1.0                   # Radiative absorption factor used by the thermodynamics interaction solver
    composition: ""                          # Free-form material composition label
    position: [0.0, 10.0, 0.0, 0.0]          # Four-position [t, x, y, z]; components 1 to 3 are Cartesian coordinates, component 0 is ignored
    velocity: [1.0, 0.0, 0.0, 0.1]           # [component 0 (ignored), vx, vy, vz]; components 1 to 3 are the Cartesian velocity
    quadrupole: 0.0                          # Reserved quadrupole deformation parameter
    j2: 0.0                                  # Zonal J2 gravitational harmonic coefficient
    j3: 0.0                                  # Zonal J3 gravitational harmonic coefficient
    j4: 0.0                                  # Zonal J4 gravitational harmonic coefficient
    reference_radius: 0.0                    # Reference radius at which the zonal harmonics above are defined

observers:
  - name: "PrimaryObserver"
    fov_deg: 60.0                            # Field of view in degrees
    resolution: [1920, 1080]                 # Raster dimensions [width, height]
    position: [0.0, 50.0, 0.0, 0.0]          # Observer position [t, x, y, z]; components 1 to 3 are Cartesian coordinates, component 0 is ignored
    orientation: [0.0, 180.0, 0.0]           # Camera orientation [pitch, yaw, roll] in degrees
    orientation_convention: 1                # 0 or missing orientation: look at the origin; 1 or more: use orientation
    four_velocity: [1.0, 0.0, 0.0, 0.0]      # Comobile four-velocity [u0, u1, u2, u3]; stored, not applied by the simulation
```

### 1.2. Parsing, Validation and Application Rules

**Parsing.** `ScenarioSerializer::from_yaml` (`include/relativistic/io/scenario_serializer.hpp`) is a line-oriented reader, not a complete YAML parser:

- Section keys (`spacetime`, `integrator`, `output`, `interactions`, `bodies`, `observers`) switch the active section; keys outside a section apply to the root.
- Entries of `bodies` and `observers` start with `-`; only a `name` key is accepted on the same line as the dash.
- Vectors use the inline form `[a, b, c, d]`.
- Unknown keys are ignored.
- Values must not carry trailing comments: a quoted string followed by a comment is not unquoted. The comments in the schema above are descriptive only.

**Validation.** `ScenarioSerializer::validate` (`include/relativistic/io/scenario_serializer.hpp`) rejects a scenario when:

- `format_version` is 0 or greater than 1.
- `scenario_name` or `metric_type` is empty.
- `metric_type` is none of the names listed in the schema and contains none of the substrings `Schwarzschild`, `Kerr`, `Minkowski`, `Wormhole`, `Warp` ([CLI Reference](CLI_REFERENCE.md#23-spacetime-metric--integrator-selection)).
- A Schwarzschild or Kerr family metric (excluding wormhole and warp names) has `central_mass <= 0` ([MATHEMATICAL_FORMULATION.md Section 1.2](MATHEMATICAL_FORMULATION.md#12-schwarzschild-metric)).
- A Kerr metric without `Newman` in its name has `|central_spin| > 1.0001 * central_mass` ([Section 1.3](MATHEMATICAL_FORMULATION.md#13-kerr-metric-rotating-black-hole)).
- A Morris-Thorne metric has `wormhole_throat <= 0` ([Section 1.8](MATHEMATICAL_FORMULATION.md#18-morris-thorne-traversable-wormhole)).
- An Alcubierre metric has `warp_velocity < 0` ([Section 1.9](MATHEMATICAL_FORMULATION.md#19-alcubierre-warp-drive-metric)).
- `speed_of_light` or `gravitational_constant` is not strictly positive.
- An observer field of view is outside the open interval (0, 180) degrees.
- A body has a negative mass or radius.

**Application on load** (`SimulationOrchestrator::load_scenario_file` in `include/relativistic/orchestrator/simulation_orchestrator.hpp`). The loader applies the scenario name and path, the metric name, `central_mass`, `central_spin`, `central_charge`, `cosmological_lambda`, `wormhole_throat`, `warp_velocity`, the integrator `scheme`, the complete `interactions` block ([DESCRIPTION.md Section 5.4](DESCRIPTION.md#54-body-interactions--spacetime-source-bodies)), the first observer (position, field of view, orientation) and all bodies, which replace the existing body list. Surface layers are cleared. The fields `speed_of_light`, `gravitational_constant`, the numeric integrator fields, additional observers and the `output` block are parsed but not applied to the simulation state.

**Content of saved files.** `SimulationOrchestrator::save_scenario_file` writes the live central parameters, the integrator name and the current `integration_rtol` and `integration_atol`, the interaction configuration, one observer built from the live camera, and all bodies. Rendering and performance settings are never written. Paths are resolved as described in [ARCHITECTURE.md](ARCHITECTURE.md#35-scenario-resolution-and-startup).

---

## 2. Flexible Image Transport System (`.fits`)

Images & spectral cubes generated by the raytracing engine comply with the NASA/IAU FITS Standard 4.0. Pixel arrays use 64-bit IEEE 754 floating-point values (`BITPIX = -64`) stored in big-endian byte order, formatted in 2880-byte logical records.

### 2.1. 2D Surface Radiance Image Keywords

| Keyword | Value Type | Description |
| :--- | :--- | :--- |
| `SIMPLE` | Logical (`T`) | Standard FITS conforming header. |
| `BITPIX` | Integer (`-64`) | IEEE 754 double-precision floating-point format. |
| `NAXIS` | Integer (`2`) | Two-dimensional image matrix. |
| `NAXIS1` | Integer | Image raster width in pixels. |
| `NAXIS2` | Integer | Image raster height in pixels. |
| `BSCALE` | Float (`1.0`) | Linear scale factor. |
| `BZERO` | Float (`0.0`) | Zero offset. |
| `BUNIT` | String (`'W/m2/sr'`) | Physical specific intensity units. |
| `CRPIX1`, `CRPIX2` | Float | Reference pixel coordinates. |
| `CRVAL1`, `CRVAL2` | Float | Astrometric coordinates at reference pixel. |
| `CDELT1`, `CDELT2` | Float | Coordinate increment per pixel. |
| `CTYPE1`, `CTYPE2` | String | Astrometric projection types (`'RA---TAN'`, `'DEC--TAN'`). |
| `CUNIT1`, `CUNIT2` | String | Coordinate units (`'deg'`). |
| `OBJECT` | String | Simulated astronomical source or target identifier. |
| `TELESCOP` | String | Synthetic instrument identifier (`'RelativisticEngine'`). |
| `DATE-OBS` | String | Observation epoch in ISO 8601 UTC. |
| `OBSERVER` | String | Observer identifier. |
| `EXTEND` | Logical (`T`) | Extensions permitted. |
| `END` | None | End of header; the header is padded with spaces and the data with null bytes to a multiple of 2880 bytes. |

### 2.2. 3D Spectral Radiance Cube Format

For spectral raytracing across multiple wavelength channels, `NAXIS = 3`:
- `NAXIS1`: Horizontal spatial dimension $X$.
- `NAXIS2`: Vertical spatial dimension $Y$.
- `NAXIS3`: Number of discrete wavelength channels $\lambda_k$.
- `CRPIX3`, `CRVAL3`, `CDELT3`: Reference wavelength channel, central wavelength, & spectral channel width in meters (`CUNIT3 = 'm'`, `CTYPE3 = 'WAVE'`).

---

## 3. Hierarchical Data Format (`.h5`)

Dense trajectories, metric tensor series, & uncertainty quantities are stored in containerized HDF5 binary files.

`Hdf5Container` (`include/relativistic/io/hdf5_serializer.hpp`) is a custom binary serialization that reuses the HDF5 signature value. Files are not readable by the HDF5 library. Telemetry recordings stored in this container use the extension `.rcap` (see [Section 8](#8-telemetry-recording-files)). Multi-byte values are written in the byte order of the host.

### 3.1. Binary Header Structure

The container header format:
- `Signature` (8 bytes): Magic identifier `0x894844460D0A1A0A`.
- `Version` (4 bytes): Container format version (1).
- `DatasetCount` (4 bytes): Number of stored dataset blocks.

Each dataset record contains:
- `PathLength` (4 bytes) & `PathString` (ASCII path string).
- `DataType` (4 bytes): `Float64` (1), `Float32` (2), `Int64` (3), `Int32` (4), `Byte` (5).
- `Rank` (4 bytes): Number of array dimensions.
- `Dimensions` ($Rank \times 8$ bytes): Array dimension extents.
- `DataSize` (8 bytes) followed by the contiguous raw payload bytes.

### 3.2. Standard Group Hierarchy

| Dataset Path | DataType | Dimensions | Description |
| :--- | :--- | :--- | :--- |
| `/bodies/<name>/position` | `Float64` | $[N \times 4]$ | Spacetime coordinates $[c t, x, y, z]$ along worldline. |
| `/bodies/<name>/velocity` | `Float64` | $[N \times 4]$ | Four-velocity components $[c, v_x, v_y, v_z]$. |
| `/spacetime/metric_series` | `Float64` | $[N \times 4 \times 4]$ | Covariant metric tensors $g_{\mu\nu}(\lambda)$. |
| `/uncertainty/covariance` | `Float64` | $[N \times 8 \times 8]$ | Phase covariance matrices $\mathbf{\Sigma}_{ij}(\lambda)$. |
| `/eos/log_rho` | `Float64` | $[N_\rho]$ | Base-10 logarithm of density grid in $\text{kg/m}^3$. |
| `/eos/log_temp` | `Float64` | $[N_T]$ | Base-10 logarithm of temperature grid in MeV. |
| `/eos/ye` | `Float64` | $[N_{Y_e}]$ | Electron fraction grid values. |
| `/eos/log_press` | `Float64` | $[N_\rho \times N_T \times N_{Y_e}]$ | Base-10 logarithm of pressure in $\text{Pa}$. |
| `/eos/log_eps` | `Float64` | $[N_\rho \times N_T \times N_{Y_e}]$ | Base-10 logarithm of energy density in $\text{J/m}^3$. |
| `/eos/enthalpy` | `Float64` | $[N_\rho \times N_T \times N_{Y_e}]$ | Dimensionless specific enthalpy $h$. |
| `/eos/cs2` | `Float64` | $[N_\rho \times N_T \times N_{Y_e}]$ | Dimensionless sound speed squared $(c_s / c)^2$. |
| `channels/<channel_name>` | `Float64` | $[N]$ | One recorded telemetry channel (see [Section 8](#8-telemetry-recording-files)). |
| `meta/info` | `Byte` | $[L]$ | Telemetry schema document as UTF-8 JSON. |

---

## 4. Visualization Toolkit PolyData (`.vtp`)

Geometry exports of horizons & geodesic trajectories use VTK XML PolyData format in little-endian byte ordering.

### 4.1. Geodesic Polyline Specifications

```xml
<?xml version="1.0"?>
<VTKFile type="PolyData" version="1.0" byte_order="LittleEndian">
  <PolyData>
    <Piece NumberOfPoints="N" NumberOfLines="M">
      <Points>
        <DataArray type="Float64" Name="Points" NumberOfComponents="3" format="ascii">
          x0 y0 z0 x1 y1 z1 ...
        </DataArray>
      </Points>
      <PointData Scalars="Redshift" Vectors="FourVelocity">
        <DataArray type="Float64" Name="AffineParameter" format="ascii">
          lambda0 lambda1 ...
        </DataArray>
        <DataArray type="Float64" Name="Redshift" format="ascii">
          g0 g1 ...
        </DataArray>
        <DataArray type="Float64" Name="FourVelocity" NumberOfComponents="4" format="ascii">
          u0_0 u1_0 u2_0 u3_0 ...
        </DataArray>
      </PointData>
      <Lines>
        <DataArray type="Int64" Name="connectivity" format="ascii">
          0 1 2 3 ...
        </DataArray>
        <DataArray type="Int64" Name="offsets" format="ascii">
          N0 N1 ...
        </DataArray>
      </Lines>
    </Piece>
  </PolyData>
</VTKFile>
```

### 4.2. Event Horizon Surface Meshes

`VtkExporter::export_horizon_sphere_vtp(radius, u_res, v_res)` writes a sphere of the given radius with `<Polys>` connectivity. The surface is parameterized by $\theta \in [0, \pi]$ with `v_res` intervals and $\phi \in [0, 2\pi]$ with `u_res` intervals (defaults 16 and 32), giving $(u_{res}+1)(v_{res}+1)$ points and $u_{res} \cdot v_{res}$ quadrilaterals. The file contains no point data. Polyline exports of recorded camera trajectories are described in [Section 8](#8-telemetry-recording-files).

---

## 5. Planetary Ephemeris & SPICE Integration

The engine ingests planetary state vectors & orbital elements from NASA/JPL data formats.

### 5.1. NASA JPL HORIZONS API Responses

`HorizonsInterface::build_query_url` (`include/relativistic/io/horizons_parser.hpp`) builds a request with `COMMAND` set to the target body identifier, `CENTER='@<id>'`, `EPHEM_TYPE='VECTORS'` (or `'ELEMENTS'`), `REF_SYSTEM='ICRF'`, `VEC_TABLE='3'`, `CSV_FORMAT='YES'`, Julian date start and stop times and a step size in days. Body identifiers follow the NAIF convention (`NaifBodyId` in `include/relativistic/io/ephemeris_types.hpp`). Parsed states are stored as `EphemerisStateVector` with the four-position $(c \cdot t_{J2000}, x, y, z)$ and four-velocity $(c, v_x, v_y, v_z)$ in SI units. CSV vector tables are parsed between the `$$SOE` & `$$EOE` markers; a line is accepted when it has at least 8 comma-separated fields and all numeric fields parse:
- Field 1: Julian Date epoch (`JD`).
- Fields 3, 4, 5: Cartesian positions $(X, Y, Z)$ in kilometers, converted to SI meters.
- Fields 6, 7, 8: Cartesian velocities $(V_X, V_Y, V_Z)$ in $\text{km/s}$, converted to $\text{m/s}$.

### 5.2. SPICE Binary SPK Kernels (`.bsp`)

`SpkKernel::parse_daf_spk_bytes` (`include/relativistic/io/spk_reader.hpp`) validates the file size (at least 1024 bytes) and the `DAF/SPK` identification word and returns an empty kernel; DAF record parsing is not implemented. Segments (`SpkChebyshevSegment`) are added programmatically and evaluated as follows: coefficients are stored per record as 3 (Type 2) or 6 (Type 3) components of `coefficients_per_component` values (at most 64), expressed in kilometers and kilometers per second and converted to SI units; epochs are seconds past J2000; the record is selected from `initial_epoch_record` and `interval_length_sec`; when several segments contain an epoch, the most recently added one is used. `SpkKernel::evaluate_relative` subtracts the state of the observer center from the state of the target. Chebyshev polynomial position & velocity segments are evaluated through recurrence relations:
- Type 2: Chebyshev polynomials for position coordinates only; velocity is obtained via analytical derivative recurrence:
  $$\dot{T}_n(s) = n U_{n-1}(s) \frac{ds}{dt}$$
- Type 3: Independent Chebyshev polynomial sets for position & velocity coordinates.

---

## 6. Image Output Formats

Screenshots and sequence frames are written by the stream writers in `include/relativistic/io/image_stream_writers.hpp` (descriptors in `include/relativistic/io/image_format.hpp`). Pixel channels are clamped to $[0, 1]$ and quantized as $\lfloor 255 v + 0.5 \rfloor$ (or $65535$ for 16-bit samples). Rows are written incrementally, and an interrupted write removes the partial file.

| Format | Extension | Samples | Alpha | Comment | Maximum side (pixels) | Additional limit |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| PPM | `ppm` | 8-bit RGB | No | No | 4294967295 | None |
| BMP | `bmp` | 8-bit BGR, rows top-down (negative height) | No | No | 2147483647 | Estimated size at most 4290000000 bytes |
| PNG | `png` | 8-bit RGBA | Yes | `tEXt` chunk, keyword `Comment` | 2147483647 | None |
| TGA | `tga` | 8-bit BGR, top-left origin | No | No | 65535 | None |
| HDR | `hdr` | Radiance RGBE, linear floating point | No | Header line `# <text>` | 4294967295 | None |
| PNG16 | `png` | 16-bit RGBA | Yes | `tEXt` chunk, keyword `Comment` | 2147483647 | None |
| QOI | `qoi` | 8-bit RGBA | Yes | No | 4294967295 | At most 400000000 pixels |
| PFM | `pfm` | 32-bit floating point RGB, little-endian (`-1.0` scale), rows bottom-to-top | No | No | 4294967295 | None |
| TIFF | `tif` | 8-bit RGB, little-endian, uncompressed strips of about 1 MiB, 72 dpi | No | `ImageDescription` tag | 4294967295 | Estimated size at most 4290000000 bytes |
| PAM | `pam` | 8-bit RGBA, `TUPLTYPE RGB_ALPHA` | Yes | No | 4294967295 | None |

PNG data uses stored (uncompressed) deflate blocks. Capture requests are additionally limited to 65535 pixels per output axis, 262144 pixels per traced axis and a supersampling factor of 8 (see [ARCHITECTURE.md](ARCHITECTURE.md#210-capture-subsystem)).

**Screenshot file names.** The pattern is expanded by `ScreenshotFilenameBuilder` (`include/relativistic/io/screenshot_capture_settings.hpp`) and then by `strftime`. The default is `relativistic_%metric%_%Y%m%d_%H%M%S`.

| Token | Replacement |
| :--- | :--- |
| `%metric%` | Active metric name in lower case with every non-alphanumeric run replaced by `_` |
| `%mass%`, `%spin%` | Central mass and spin with two decimals |
| `%width%`, `%height%` | Output size in pixels |
| `%tick%` | Scheduler tick index |
| `strftime` tokens | Local time fields, for example `%Y %m %d %H %M %S` |

Overwrite policies: `AutoIncrement` appends `_1`, `_2`, ... until the name is free, `Overwrite` replaces the file, `SkipIfExists` cancels the capture.

---

## 7. Capture Session Directory

A sequence capture writes into `<sequence_directory>/<session_name>/`. An empty session name is replaced by `sequence_%Y%m%d_%H%M%S`.

| File | Content |
| :--- | :--- |
| `frame_<index>.<ext>` | Frame images. The index starts at `start_frame_index` and is zero-padded to `frame_name_padding` digits (1 to 12, default 6). |
| `event_<n>_<label>.<ext>` | Stills produced by `Capture Still` script events; the label is lower-cased with non-alphanumeric characters replaced by `_`. |
| `sequence_info.txt` | `key=value` lines: `mode` (`deterministic` or `real-time`), `frames_submitted`, `frames_written`, `frames_dropped`, `frames_duplicated`, `frames_per_second`, `frame_format`, `start_frame_index`, `recording` (path of the telemetry file when recorded), and for deterministic sessions `resolution`, `supersampling`, `temporal_samples`, followed by `ffmpeg` (assembly command) when applicable. |
| `motion_script.cfg` | Copy of the motion script used by the session (see [Section 9](#9-motion-script-and-capture-settings-files)). |
| `video.<ext>` | Output of the optional ffmpeg assembly. |
| Telemetry files | Described in [Section 8](#8-telemetry-recording-files). |

`VideoSequenceSettings::build_ffmpeg_command` (`include/relativistic/io/video_capture_settings.hpp`) generates the assembly command from `-framerate`, `-start_number`, the frame pattern `frame_%0<padding>d.<ext>`, a padding filter `pad=ceil(iw/2)*2:ceil(ih/2)*2` (palette generation and use for GIF), `-c:v <encoder>` and codec-specific rate control options. The image sequence codec generates no command.

| Codec | Encoder | Notes |
| :--- | :--- | :--- |
| H264 | `libx264` | `-preset`, `-crf` or `-b:v`, pixel format `yuv420p` by default |
| H265 | `libx265` | `-preset`, `-crf` or `-b:v` |
| VP9 | `libvpx-vp9` | `-crf` with `-b:v 0` or `-b:v`, `-deadline` derived from the encoder speed |
| ProRes | `prores_ks` | `-profile:v 3`, pixel format `yuv422p10le`, container forced to MOV |
| PngSequence | `png` | No command |
| AV1 | `libsvtav1` | `-crf` or `-b:v`, `-preset` 0 to 12 |
| FFV1 | `ffv1` | `-level 3`; MP4 and MOV containers are replaced by MKV |
| MJPEG | `mjpeg` | `-q:v` derived from the CRF, pixel format `yuvj420p` |
| GIF | `gif` | Container forced to GIF, optional `-loop 0` |

Container resolution: a GIF container with a non-GIF codec becomes MKV, and WebM with a codec other than VP9 or AV1 becomes MKV.

---

## 8. Telemetry Recording Files

`PhysicsRecorder` (`include/relativistic/capture/physics_recorder.hpp`) samples the selected channels once per captured frame; one row is written every `decimation` frames. The file is `<file_stem>.<extension>` in the session directory, accompanied by `<file_stem>.schema.json` and, when events were fired and `write_events` is set, `<file_stem>_events.csv`.

| Format | Extension | Layout |
| :--- | :--- | :--- |
| CSV | `csv` | Header of channel names, one row per sample, comma-separated, `%.*g` with the configured precision (3 to 17 digits), non-finite values written as `nan` |
| TSV | `tsv` | Same as CSV with tab separators |
| JSON | `json` | One document with the metadata fields of the schema and a `data` object mapping each channel name to an array; non-finite values written as `null` |
| JSON Lines | `jsonl` | One JSON object per row |
| Binary columns | `bin` | Magic `RCAP`, `uint32` version (1), `uint32` column count, `uint64` row count, then each column as little-endian `float64` values |
| Container | `rcap` | `Hdf5Container` with datasets `channels/<name>` and `meta/info` (see [Section 3](#3-hierarchical-data-format-h5)) |
| VTK polyline | `vtp` | One polyline from `camera_x`, `camera_y`, `camera_z`, with point data `AffineParameter` (`session_time`), `Redshift` (`grav_redshift`) and `FourVelocity` $(1, v_x, v_y, v_z)$ from `camera_vx`, `camera_vy`, `camera_vz` when recorded, otherwise 0 |

The VTK format forces the channels `session_time`, `camera_x`, `camera_y`, `camera_z` and `grav_redshift`.

**Schema file.** JSON object with the keys `format` (`relativistic-telemetry`), `version` (1), `storage` (file extension), `session`, `script`, `unit_system` (`geometric` or `si`), `frames_per_second`, `decimation`, `rows`, `endianness` (`little`) and `channels` (array of objects with `name` and `unit`).

**Event file.** CSV with the header `frame,time,event_index,action,label,value`.

**Units.** In geometric mode lengths, times, masses, energies, angular momenta and frequencies are in simulation units (symbols `L`, `T`, `M`, `E`, `J`, `1/T` below). With `si_units` set, these channels are multiplied by the scales of the constants engine (see [ARCHITECTURE.md](ARCHITECTURE.md#213-physical-constants-engine)): length by `L0`, time by `T0`, mass by `M0`, energy by `M0 L0^2 / T0^2`, angular momentum by `M0 L0^2 / T0` and frequency by `1/T0`. Angles are in degrees unless stated.

| Channel | Unit | SI unit | Group |
| :--- | :--- | :--- | :--- |
| `frame_index` | | | Timing |
| `session_time` | s | s | Timing |
| `script_time` | s | s | Timing |
| `script_progress` | | | Timing |
| `segment_index` | | | Timing |
| `segment_progress` | | | Timing |
| `logical_time` | T | s | Timing |
| `tick_index` | | | Timing |
| `simulation_rate` | | | Timing |
| `time_warp` | | | Timing |
| `camera_x`, `camera_y`, `camera_z` | L | m | Camera Kinematics |
| `camera_r` | L | m | Camera Kinematics |
| `camera_theta`, `camera_phi` | rad | rad | Camera Kinematics |
| `camera_pitch`, `camera_yaw`, `camera_roll` | deg | deg | Camera Kinematics |
| `camera_fov` | deg | deg | Camera Kinematics |
| `camera_exposure` | EV | EV | Camera Kinematics |
| `camera_vx`, `camera_vy`, `camera_vz` | L/s | m/s | Camera Kinematics |
| `camera_speed` | L/s | m/s | Camera Kinematics |
| `camera_ax`, `camera_ay`, `camera_az` | L/s^2 | m/s^2 | Camera Kinematics |
| `camera_acceleration` | L/s^2 | m/s^2 | Camera Kinematics |
| `camera_angular_rate` | deg/s | deg/s | Camera Kinematics |
| `camera_path_length` | L | m | Camera Kinematics |
| `distance_to_center` | L | m | Relativity At Camera |
| `distance_to_horizon` | L | m | Relativity At Camera |
| `static_lapse` | | | Relativity At Camera |
| `time_dilation` | | | Relativity At Camera |
| `zamo_omega` | 1/T | rad/s | Relativity At Camera |
| `static_proper_time` | T | s | Relativity At Camera |
| `grav_redshift` | | | Relativity At Camera |
| `escape_velocity_fraction` | c | c | Relativity At Camera |
| `disk_doppler_factor` | | | Relativity At Camera |
| `on_disk_band` | | | Relativity At Camera |
| `horizon_radius` | L | m | Relativity At Camera |
| `photon_sphere_radius` | L | m | Relativity At Camera |
| `isco_radius` | L | m | Relativity At Camera |
| `central_mass` | M | kg | Spacetime Parameters |
| `central_spin` | L | m | Spacetime Parameters |
| `central_charge` | Q | Q | Spacetime Parameters |
| `cosmological_lambda` | 1/L^2 | 1/L^2 | Spacetime Parameters |
| `body_count` | | | Bodies Aggregate |
| `body_total_kinetic_energy` | E | J | Bodies Aggregate |
| `body_total_angular_momentum` | J | kg m^2/s | Bodies Aggregate |
| `body_nearest_distance` | L | m | Bodies Aggregate |
| `body_nearest_id` | | | Bodies Aggregate |
| `body_total_mass` | M | kg | Bodies Aggregate |

**Definitions.**

- Velocity and acceleration are finite differences of the camera position between consecutive captured frames, evaluated for every frame before decimation.
- `camera_angular_rate` is the root of the squared differences of pitch, yaw and roll divided by the elapsed time.
- `static_lapse` is $\sqrt{-g_{tt}}$ of the Kerr metric at $\max(r, 2.05M)$ ([MATHEMATICAL_FORMULATION.md Section 2.4](MATHEMATICAL_FORMULATION.md#24-kerr-characteristic-radii-and-horizon-quantities)).
- `grav_redshift` is $\sqrt{\max(1 - 2M/r, 0)}$ for every spin.
- `static_proper_time` accumulates the lapse multiplied by the elapsed logical time.
- `disk_doppler_factor` is 1 outside the disk band ([MATHEMATICAL_FORMULATION.md Section 8.2](MATHEMATICAL_FORMULATION.md#82-step-control-termination-and-space-skipping)).
- `on_disk_band` is 1 when the camera radius lies between the ISCO and the disk outer radius.
- The horizon, ISCO and photon orbit radii are evaluated with the spin clamped to $\pm 0.999 M$ ([MATHEMATICAL_FORMULATION.md Section 2.4](MATHEMATICAL_FORMULATION.md#24-kerr-characteristic-radii-and-horizon-quantities)).
- All metric-dependent channels are 0 or 1 when the central mass is not larger than $10^{-9}$.

**Per-body columns.** When any per-body option is enabled, the recorder selects up to `max_bodies` (at most 256) enabled bodies present at the start of the session and appends, per body, the columns `body<id>_x`, `body<id>_y`, `body<id>_z` (positions), `body<id>_vx`, `body<id>_vy`, `body<id>_vz` (velocities), `body<id>_camera_distance` and `body<id>_speed`, each only when the corresponding option is enabled. Values are NaN when the body is absent or disabled. Presets: 0 none, 1 trajectory, 2 camera kinematics, 3 full kinematics, 4 relativistic physics, 5 all channels and all per-body options.

---

## 9. Motion Script and Capture Settings Files

All files in this section use `key=value` lines written with 17 significant digits. Booleans are `0` or `1`, enumerations are stored as their numeric value, and unknown keys are ignored. Vectors are stored as three keys with the suffixes `.x`, `.y` and `.z`.

**`config/capture_script.cfg`** contains one motion script (`MotionScript::write` in `include/relativistic/capture/motion_script.hpp`) and is also copied into capture sessions as `motion_script.cfg`. A file is accepted when the key `script.segments` exists. The keys are organized by prefix:

| Prefix | Content |
| :--- | :--- |
| `script.name`, `script.end` | Script name and end behavior (0 clamp, 1 loop, 2 ping-pong) |
| `script.global_ease.` | Easing applied to the global script progress |
| `script.segments` | Number of segments |
| `script.seg<i>.` | Segment: name, enabled, duration, `time.` easing, anchor mode (0 world, 1 continue previous, 2 offset from previous, 3 track body), `anchor_body`, `anchor_offset`, `layers` count |
| `script.seg<i>.layer<j>.` | Shape layer: name, enabled, blend (0 replace, 1 add, 2 add relative), weights `ws` and `we`, window `win0` and `win1`, `ease.` easing, `shape.` shape (kind, controls `c0` to `c3`, values `v0` to `v7`, expressions `e0` to `e2`, closed, uniform, waypoints `wp`, transform `scale`, `rot`, `pivot`, `trans`) |
| `script.seg<i>.orient.`, `fov.`, `exposure.`, `roll.`, `warp.`, `shake.`, `transition.` | Orientation, scalar channels, camera shake and transition to the previous segment |
| `script.events`, `script.ev<i>.` | Number of events and event fields (trigger, time, segment, fraction, offset, action, parameter, value, value_end, duration, easing, text) |

Easing blocks (`EasingSpec::write` in `include/relativistic/capture/easing.hpp`) use the suffixes `kind`, `p0` to `p3`, `blend`, `repeat`, `ping_pong`, `reverse`, `in0`, `in1`, `out0`, `out1`, `bias`, `gain`, `quant`, `clamp_out`, `smooth`, `expr`, `points`, `pt<i>.u` and `pt<i>.v`. The script structure and the available shapes, easings, orientation modes, events and expression language are described in [ARCHITECTURE.md](ARCHITECTURE.md#210-capture-subsystem).

**`config/capture_path.cfg`** is the legacy camera path format (lines such as `kind=`, `duration=`, `orbit=`, `key=`, see `CameraPath::to_text` in `include/relativistic/capture/camera_path.hpp`). It is read only when `config/capture_script.cfg` is absent or invalid, and is converted by `MotionScript::from_legacy_path`.

**`config/capture_studio.cfg`** is written by `CaptureStudioSettings::save` (`include/relativistic/io/capture_studio_settings.hpp`) and clamped on load by `CaptureStudioSettings::sanitize`.

| Keys | Content |
| :--- | :--- |
| `screenshot_<k>`, `sequence_<k>` with `<k>` in `use_explicit_resolution`, `resolution_multiplier`, `explicit_width`, `explicit_height`, `supersampling`, `max_ray_steps`, `step_refinement` | Capture target of screenshots and sequences |
| `frames_per_second`, `duration_seconds`, `fixed_frame_count`, `resolution_scale`, `frame_format`, `mode`, `trigger`, `advance_mode`, `pause_simulation_during_capture`, `ticks_per_frame`, `simulation_seconds_per_video_second`, `temporal_samples`, `shutter_fraction`, `fade_in_frames`, `fade_out_frames`, `start_frame_index`, `frame_name_padding`, `preview_in_viewport`, `restore_camera_after_capture`, `pacing`, `realtime_queue_depth`, `codec`, `container`, `rate_control`, `encoder_speed`, `crf`, `target_bitrate_kbps`, `pixel_format`, `encode_frames_per_second`, `loop_output`, `assemble_video_after_capture`, `delete_frames_after_assembly`, `ffmpeg_executable` | Sequence timing and video encoding (`VideoSequenceSettings`) |
| `sequence_directory`, `session_name`, `use_script` (legacy alias `use_camera_path`), `manual_stepping` | Session destination and options |
| `recording_<k>` with `<k>` in `enabled`, `format`, `channel_mask`, `body_positions`, `body_velocities`, `body_camera_distance`, `body_speeds`, `max_bodies`, `decimation`, `precision`, `si_units`, `write_events`, `file_stem` | Telemetry recording (`RecordingSettings`); `channel_mask` is a bit mask over the 54 channels in the order of the table in [Section 8](#8-telemetry-recording-files) |

---

## 10. Persistent Configuration Files

### 10.1. `config/user_settings.cfg`

Written by `UserSettings::save` (`include/relativistic/io/user_settings.hpp`), format version 3. A file with version 0 or a version above 3 is ignored. Files older than version 3 have the camera inversion flags reset.

| Key prefix | Content |
| :--- | :--- |
| `format_version`, `load_policy` | Format version and load policy (0: always reset to defaults, 1: restore previous session) |
| `default_*`, `load_scenario_on_startup` | Default camera mode, performance preset, GPU compute and startup scenario path |
| `screenshot_*` | Output directory, filename pattern, format, watermark and overwrite policy |
| `show_system_console`, `window_*_open`, `last_window_layout`, `multi_window_mode` | Window visibility and layout |
| `constants_*` | Constants preset (0: SI, 1: Planck, 2: Custom) and the seven base constants |
| `interaction_*` | Interaction configuration (same fields as the scenario `interactions` block) |
| `body_manager_*` | Body manager sorting, filter, pane width and logarithmic slider modes |
| `unit_*` | Display units: distance, velocity, mass, energy, temperature, angle, charge, current, time, acceleration, angular velocity, density, pressure, power, frequency, force, magnetic field, voltage |
| `cam_ff_*`, `cam_orbit_*`, `cam_rocket_*`, `cam_keyboard_layout` | Camera control parameters |
| `kb_<i>_primary`, `kb_<i>_secondary`, `kb_<i>_mode` | Key bindings of action `i` |
| `hud_master_enabled`, `hud_el_<i>_<field>` | HUD master switch and per-element `enabled`, `anchor`, `offset_x`, `offset_y`, `scale`, `color_r`, `color_g`, `color_b`, `color_a`, `show_background`, `background_opacity` |
| `secview_<i>_<field>` | Secondary viewport state (active, open, name, radius, theta, phi, fov_deg, exposure, tonemapping_mode, projection_mode, max_steps, resolution_scale, follow_primary, follow_offset_theta, follow_offset_phi); up to 8 views |

The following state is not persisted: HUD display mode, label, layout, nudge, priority, precision, refresh interval and color rules, HUD auto-arrangement and toolbar button visibility, the schematic view configuration, the camera free-fly yaw, pitch and roll speeds, the zoom configuration and the free-fly pitch/roll movement option.

**Crash guard.** `config/.session_active` is created at the start of a session and removed on a clean exit. When it exists at the next start, or when `load_policy` is 0, all settings are reset to defaults except the screenshot output directory.

### 10.2. `config/performance_profiler.cfg`

Written by `PerformanceProfiler::save_to_disk` (`include/relativistic/orchestrator/performance_profiler.hpp`) when persistence is enabled. The header contains `format_version=1` and `history_capacity`. Each benchmark run is enclosed by the lines `[run]` and `[/run]` and contains:

| Keys | Content |
| :--- | :--- |
| `label`, `timestamp`, `engine_signature` | Run identification; the signature is described in [ARCHITECTURE.md](ARCHITECTURE.md#211-profiling-logging-and-persistence) |
| `cfg_metric`, `cfg_integrator`, `cfg_res_scale`, `cfg_ray_steps`, `cfg_precision`, `cfg_preset`, `cfg_gpu`, `cfg_tiled`, `cfg_simd`, `cfg_width`, `cfg_height`, `cfg_step_controller`, `cfg_motion_quality_mode`, `cfg_motion_quality_scale`, `cfg_space_skip_enabled`, `cfg_space_skip_radius`, `cfg_pole_guard_precision`, `cfg_far_field_step_scale`, `cfg_lod_enabled`, `cfg_lod_distance_scale`, `cfg_lod_reduced_steps`, `cfg_render_distance_scale`, `cfg_interlace_enabled`, `cfg_dynamic_res_enabled`, `cfg_dynamic_res_target_fps`, `cfg_adaptive_tile_prepass`, `cfg_rolling_average_count`, `cfg_integration_rtol`, `cfg_integration_atol` | Configuration at the end of the capture |
| `ft_mean`, `ft_median`, `ft_min`, `ft_max`, `ft_std`, `ft_p95`, `ft_p99` | Frame time statistics in milliseconds |
| `fps_mean`, `fps_min`, `fps_max` | Frame rate statistics |
| `avg_iterations`, `gpu_ratio`, `horizon_ratio`, `celestial_ratio`, `disk_ratio`, `sample_count`, `duration` | Mean ray iterations, GPU path frame ratio, ray classification ratios, sample count and capture duration in seconds |

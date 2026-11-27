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

units:
  distance: "m"                              # Display units for distance, mass, velocity, time, energy, temperature, charge, angle
  mass: "kg"
  velocity: "m/s"
  time: "s"
  energy: "J"
  temperature: "K"
  charge: "C"
  angle: "rad"

spacetime:
  metric_type: "Schwarzschild | Kerr | FlatMinkowski | Minkowski | KerrSchild | ReissnerNordstrom | KerrNewman | SchwarzschildDeSitter | FLRW | MorrisThorne | Alcubierre | BSSN"
  central_mass: 1.0                          # Central mass M in geometrized or SI units
  central_spin: 0.0                          # Spin parameter a in [-M, M]
  central_charge: 0.0                        # Electric charge Q
  cosmological_lambda: 0.0                   # Cosmological constant Lambda
  wormhole_throat: 1.0                       # Throat radius b_0 for Morris-Thorne metric
  warp_velocity: 0.0                         # Warp bubble velocity v_s for Alcubierre metric
  warp_bubble_radius: 15.0                   # Alcubierre bubble radius
  warp_wall_sharpness: 0.4                   # Alcubierre bubble wall sharpness parameter
  wormhole_tidal_potential: 0.0              # Morris-Thorne wormhole tidal redshift parameter
  teves_critical_acceleration: 1.0e-3        # TeVeS critical acceleration scale a0
  pole_guard_precision_scale: 5.5            # Polar damping precision scale for rotating metrics
  far_field_step_scale: 7.8                  # Step multiplier in asymptotic weak field
  speed_of_light: 1.0                        # Speed of light c
  gravitational_constant: 1.0                # Gravitational constant G

time:
  warp_factor: 1.0                           # Time warp acceleration factor
  tick_rate: 60.0                            # Logical scheduler clock rate in Hz
  is_paused: true                            # Initial pause state

skybox:
  enabled: true                              # Celestial sky rendering master toggle
  seed: 56                                   # Procedural generation master seed
  master_intensity: 1.0                      # Master skybox radiance gain
  stars_enabled: true                        # Procedural starfield toggle
  star_density: 91.064                       # Star spatial density parameter
  star_brightness: 1.81                      # Star point radiance multiplier
  star_size_scale: 1.0                       # Star angular diameter multiplier
  star_profile_model: 0                      # 0: Gaussian, 1: Lorentzian, 2: Moffat, 3: Airy, 4: HardDisc, 5: Spikes, 6: Dirac, 7: Bloom
  star_placement_model: 3                    # 0: Uniform, 1: GalacticDisk, 2: Clustered, 3: DualPopulation, 4: SpiralArm, 5: Associations
  star_size_distribution: 1                  # 0: Uniform, 1: PowerLaw, 2: Exponential, 3: LogNormal
  star_luminosity_distribution: 1            # 0: Uniform, 1: PowerLaw, 2: Exponential, 3: LogNormal
  star_spike_count: 4                        # Diffraction spike count (4, 6, 8)
  star_spike_length: 4.0                     # Diffraction spike radial length
  star_spike_intensity: 0.6                  # Diffraction spike radiance contrast
  star_spike_angle_deg: 45.0                 # Diffraction spike rotation angle in degrees
  star_color_variation: 1.0                  # Chromatic blackbody temperature dispersion
  star_temperature_bias: 0.32                # Overall spectral temperature bias [-1.0, 1.0]
  star_brightness_variation: 0.5             # Intra-stellar magnitude variance
  nebula_enabled: true                       # Procedural galactic nebulae toggle
  nebula_intensity: 0.23                     # Nebular volumetric radiance multiplier
  nebula_octaves: 5                          # Fractal noise octave layers (1 to 8)
  nebula_frequency: 2.82                     # Spatial turbulence frequency
  nebula_plane_thickness: 0.174              # Galactic disc scale height
  nebula_plane_falloff: 4.09                 # Disc profile exponential steepness
  nebula_asymmetry: 0.43                     # Core asymmetry concentration factor
  nebula_curl_strength: 0.46                 # Divergence-free curl-noise advection strength
  nebula_ionization_glow: 0.85               # Secondary ionization emission contrast
  nebula_gas_contrast: 1.94                  # Midtone gas density power-law exponent
  galaxies_enabled: true                     # Deep-field background galaxies toggle
  galaxy_density: 0.5                        # Background galaxy spatial density
  galaxy_brightness: 0.07                    # Background galaxy radiance multiplier
  galaxy_size_scale: 1.0                     # Background galaxy angular diameter scale
  galaxy_morphology_model: 0                 # 0: Mixed, 1: GrandDesign, 2: Flocculent, 3: Barred, 4: Elliptical, 5: Lenticular, 6: Ring
  clusters_enabled: true                     # Open and globular star clusters toggle
  cluster_density: 0.184                     # Cluster spatial density
  cluster_brightness: 0.5                    # Cluster halo radiance multiplier
  cluster_size_scale: 1.61                   # Cluster angular diameter scale
  cluster_placement_model: 0                 # 0: GlobularHalo, 1: OpenClusterPlane, 2: UniformIsotropic
  cluster_stars_enabled: true                # Explicit member star rendering toggle
  cluster_member_count: 48                   # Member star count per cluster
  dust_enabled: true                         # Interstellar dust extinction lanes toggle
  dust_density: 3.85                         # Dust extinction spatial density
  dust_intensity: 1.3                        # Interstellar dust optical depth contrast
  dust_scale: 3.82                           # Spatial scale for fractal dust lanes
  quasar_enabled: true                       # Quasars with relativistic jets toggle
  quasar_density: 1.136                      # Quasar spatial density
  quasar_brightness: 1.2                     # Quasar nucleus radiance multiplier
  pulsar_enabled: true                       # Pulsars with rotating sweep beams toggle
  pulsar_density: 0.01                       # Pulsar spatial density
  pulsar_brightness: 1.0                     # Pulsar beam radiance multiplier
  snr_enabled: true                          # Supernova remnant shells toggle
  snr_density: 0.06                          # Supernova remnant spatial density
  snr_brightness: 1.2                        # Supernova remnant shell radiance
  grid_enabled: false                        # Celestial coordinate grid toggle
  grid_opacity: 0.5                          # Coordinate grid line opacity
  grid_spacing_deg: 30.0                     # Coordinate grid angular spacing in degrees
  grid_line_width: 0.0001                    # Coordinate grid line angular width
  grid_cell_fill_mode: 0                     # 0: None, 1: Checkerboard2Color, 2: CyclicPalette, 3: CheckerboardPattern, 4: RandomHashFill
  grid_cell_opacity: 0.1                     # Grid cell background fill opacity
  grid_cell_color_count: 2                   # Number of active cell palette colors (1 to 16)
  background_source: 0                       # 0: Procedural, 1: Panorama image
  panorama_id: 0                             # Panorama asset identifier (0: AmbientCG, 1: ESO Milky Way)
  panorama_quality: 1                        # Panorama resolution tier (0: 2K, 1: 4K, 2: 8K)
  orientation_yaw_deg: 0.0                   # Skybox rotation around world Z axis in degrees
  orientation_pitch_deg: 0.0                 # Skybox tilt angle in degrees
  orientation_roll_deg: 0.0                  # Skybox roll angle in degrees

lighting:
  source_mode: 0                             # 0: CentralSource, 1: CameraHeadlight, 2: FixedDirection, 3: FixedPoint, 4: NearestEmissive, 5: SpecificBody, 6: Unlit
  attenuation_mode: 0                        # 0: None (Constant), 1: InverseSquare (E(d) = I * min((d_ref/d)^2, 64.0))
  intensity: 1.0                             # Direct light source radiance multiplier [0.0, 50.0]
  ambient: 0.04                              # Ambient illumination floor on unlit hemispheres [0.0, 1.0]
  terminator_softness: 0.12                  # Terminator transition softness width parameter s in [0.0, 1.0]
  specular_scale: 1.0                        # Blinn-Phong specular highlight scale multiplier [0.0, 8.0]
  color: [1.0, 1.0, 1.0]                     # Direct light RGB color components [0.0, 4.0]
  direction_azimuth_deg: 45.0                # Directional light horizontal azimuth in degrees [-180, 180]
  direction_elevation_deg: 35.0              # Directional light vertical elevation in degrees [-90, 90]
  point_position: [100.0, 0.0, 0.0]          # Fixed point light Cartesian coordinates [x, y, z] (mode 3)
  reference_distance: 50.0                   # Inverse-square light reference distance d_ref [1e-3, 1e7]
  source_body_id: 0                          # Catalog body ID acting as light emitter (mode 5)
  detail_level: 2                            # 0: Minimal (diffuse/ambient only), 1: Reduced (capped), 2: Full (uncapped)
  shadow_caster_limit: 32                    # Maximum tested shadow occluders per point (1 to 512) in reduced mode
  emission_source_limit: 8                   # Maximum accumulated radiant emitters per point (1 to 512) in reduced mode
  emission_lighting_enabled: false           # Inter-body radiant transfer toggle (bodies with I_em > 1e-3 illuminate others)
  emission_lighting_gain: 1.0                # Radiance multiplier for body-to-body emission lighting [0.0, 1e5]
  emission_lighting_reference: 30.0          # Reference distance for body-to-body emission lighting [1e-3, 1e7]

primary_disk:
  enabled: false                             # Primary accretion disk master toggle
  inner_radius_scale: 1.0                    # Inner edge multiple of ISCO
  outer_radius_mass_units: 25.0              # Outer radius in mass units M
  peak_temperature_k: 12546.0                # Peak blackbody temperature in Kelvin
  floor_temperature_k: 1200.0                # Temperature floor in Kelvin
  brightness: 1.0                            # Absolute radiance gain multiplier
  opacity_scale: 1.0                         # Optical depth scale factor
  doppler_beaming_exponent: 0.0              # Additional Doppler beaming exponent
  color_saturation: 1.0                      # Perceptual color saturation multiplier
  edge_softness: 0.3                         # Radial edge fade softness width
  temperature_exponent: 0.75                 # Radial temperature decay power-law exponent
  zero_torque_strength: 1.0                  # Novikov-Thorne zero-torque inner condition
  tint_strength: 0.0                         # Post-shading color tint blend factor
  tint: [1.0, 1.0, 1.0]                      # RGB tint color vector
  rotation_speed_scale: 1.0                  # Keplerian angular velocity multiplier
  ring_amplitude: 0.0                        # Radial density ring modulation depth
  ring_frequency: 8.0                        # Radial ring count across disk extent
  ring_sharpness: 3.0                        # Contrast sharpness of concentric rings
  spiral_arm_count: 0                        # Number of logarithmic spiral density arms
  spiral_amplitude: 0.0                      # Contrast amplitude of spiral arms
  spiral_pitch: 1.0                          # Pitch winding angle factor
  turbulence_amplitude: 0.0                  # Fractional turbulence fluctuation depth
  turbulence_scale: 5.0                      # Spatial scale of convective turbulence
  turbulence_octaves: 3                      # Fractal noise octave layers
  radial_stretch: 2.0                        # Azimuthal elongation shear factor
  grain_amplitude: 0.0                       # High-frequency grain noise amplitude
  grain_scale: 50.0                          # Granular noise frequency scale
  noise_seed: 42                             # Deterministic procedural structure seed
  hydro_model: 0                             # 0: ThinProcedural, 1: NovikovThorne, 2: FishboneMoncrief
  accretion_rate_scale: 1.0                  # Accretion mass rate scale (Novikov-Thorne)
  torus_inner_radius: 6.0                    # Inner cusp radius in mass units M (Fishbone-Moncrief)
  torus_center_radius: 12.0                  # Pressure maximum radius in mass units M (Fishbone-Moncrief)
  adiabatic_index: 1.333333                  # Polytropic fluid index Gamma
  optical_depth_scale: 1.0                   # Volumetric optical depth scale
  hydro_sampling_density: 2.0                # Ray integration samples per gravitational radius

primary_jet:
  enabled: false                             # Blandford-Znajek magnetosphere jet toggle
  geometry: 0                                # 0: Cylindrical, 1: Paraboloidal, 2: SplitMonopole
  field_line_index: 1.0                      # Stream function radial power-law exponent nu
  angular_velocity_law: 0                    # 0: HorizonFraction, 1: Fixed
  angular_velocity_fraction: 0.5             # Ratio Omega_F / Omega_H
  angular_velocity_variation: 0.0            # Footpoint colatitude angular reduction factor
  fixed_angular_velocity: 0.0                # Fixed field angular velocity in 1/M
  toroidal_field_scale: 1.0                  # Polar toroidal field scaling multiplier
  rotation_coupling: 0.5                     # Fraction of field rotation transferred to plasma
  magnetization: 10.0                        # Horizon magnetization parameter sigma
  magnetization_index: 0.0                   # Radial magnetization power-law exponent
  inner_radius_scale: 1.05                   # Inner radial cutoff in horizon units r_h
  footpoint_edge_deg: 15.0                   # Collimation opening angle from polar axis
  footpoint_softness_deg: 2.0                # Collimation boundary edge softness width
  outer_radius_mass_units: 200.0             # Outer longitudinal jet extent in units M
  lorentz_factor_inner: 1.5                  # Plasma Lorentz factor at footpoint
  lorentz_factor_max: 10.0                   # Terminal plasma Lorentz factor Gamma_max
  acceleration_radius_mass_units: 50.0       # Radial distance of terminal acceleration
  acceleration_index: 1.0                    # Longitudinal acceleration gradient exponent
  doppler_beaming_enabled: true              # Kinematic searchlight beaming toggle
  gravitational_redshift_enabled: true       # Metric gravitational redshift attenuation toggle
  emission_model: 0                          # 0: NonThermalPowerLaw, 1: ThermalSynchrotron, 2: Hybrid
  power_law_index: 2.5                       # Non-thermal electron energy index p
  minimum_lorentz_factor: 10.0               # Minimum electron Lorentz factor gamma_min
  non_thermal_fraction: 0.1                  # Fractional non-thermal population share
  electron_temperature_k: 1.0e9              # Thermal electron temperature in Kelvin
  temperature_index: 0.0                     # Radial temperature power-law gradient
  observing_frequency_hz: 2.3e11             # Observing frequency in Hertz
  absorption_scale: 1.0                      # Synchrotron self-absorption multiplier
  faraday_scale: 1.0                         # Faraday rotation and conversion multiplier
  display_mode: 0                            # 0: RadianceRGB, 1: LinearPolarization, 2: CircularPolarization
  auto_normalization: true                   # Automatic display brightness normalization
  reference_brightness_temperature_k: 1.0e10 # Absolute reference brightness temperature in Kelvin
  horizon_field_tesla: 1.0e4                 # Poloidal magnetic field magnitude at horizon
  core_density_scale: 1.0                    # Central plasma rest-mass density factor
  brightness: 1.0                            # Absolute visual display gain multiplier
  tint: [1.0, 1.0, 1.0]                      # Display RGB color tint vector
  polarization_coherence: 0.8                # Survived linear polarization fraction
  sampling_density: 1.0                      # Integration step sampling density
  max_samples: 32                            # Maximum integration samples per ray segment
  use_engine_length_scale: true              # Scale conversion toggle with constants engine
  meters_per_length_unit: 1477.0             # Custom metric scale conversion factor in meters

integrator:
  scheme: "RK45 | CashKarp | Vernier9 | GaussLegendre4 | GaussLegendre6 | Hermite4" # Numerical ODE solver name
  initial_step: 0.01                         # Initial step size
  min_step: 1.0e-8                           # Minimum step size bound
  max_step: 10.0                             # Maximum step size bound
  relative_tolerance: 1.0e-10                # Relative error tolerance (rtol)
  absolute_tolerance: 1.0e-14                # Absolute error tolerance (atol)

cosmology:
  omega_matter: 0.3                          # Dimensionless matter density parameter
  omega_radiation: 9.0e-5                    # Dimensionless radiation density parameter
  quintessence_potential: 1                  # 0: Exponential, 1: InversePowerLaw, 2: PseudoNambuGoldstone, 3: Supergravity
  potential_slope: 2.0                       # Potential slope or exponent parameter (lambda, alpha)
  potential_scale: 1.0                       # Characteristic potential scale parameter f
  initial_field: 0.1                         # Initial scalar field value phi_0
  chaplygin_stiffness: 0.8                   # Chaplygin gas stiffness parameter A_s in [0, 1]
  chaplygin_exponent: 1.0                    # Chaplygin gas exponent parameter alpha
  ltb_hubble0: 0.005                         # LTB asymptotic Hubble expansion parameter H_0
  ltb_void_radius: 60.0                      # LTB void transition radius r_0
  ltb_void_width: 25.0                       # LTB void wall transition thickness w
  ltb_omega_inner: 0.2                       # LTB central underdensity fraction Omega_in
  ltb_omega_outer: 1.0                       # LTB asymptotic boundary density fraction Omega_out
  evaporation_enabled: false                 # Semiclassical Hawking evaporation master toggle
  evaporation_spectrum: 0                    # 0: PhotonsOnly, 1: MasslessStandardModel, 2: Custom
  evaporation_model: 0                       # 0: BlackbodyGreybody, 1: PolyakovSWave
  custom_degrees_of_freedom: 2.0             # Custom effective radiation degrees of freedom
  greybody_factor: 1.0                       # Semiclassical greybody transmission factor
  evaporation_time_acceleration: 1.0         # Evaporation mass loss time acceleration factor
  spin_down_ratio: 3.0                       # Differential angular momentum loss ratio
  minimum_mass_kg: 2.176434e-8               # Semiclassical remnant mass threshold (Planck mass)
  detector_gap_ev: 1.0e-6                    # Unruh-DeWitt detector energy level gap in eV
  detector_coupling: 1.0                     # Unruh-DeWitt scalar field coupling constant

dark_matter:
  enabled: false                             # Dark matter halo field master toggle
  lensing_enabled: true                      # Geodesic gravitational lensing through halos toggle
  visualization_enabled: false               # Emissive field visualization toggle
  affects_bodies: true                       # N-body gravitational coupling to celestial bodies toggle
  lensing_strength: 1.0                      # Global geodesic deflection multiplier
  visual_intensity: 0.25                     # Emissive false-color visualization gain
  step_fraction: 0.35                        # Ray-tracing adaptive step fraction through halos
  visualization_mode: 0                      # 0: ColumnDensity, 1: FieldStrength, 2: CircularSpeedSquared
  halos:
    - name: "Primary Galactic Halo"
      enabled: true
      profile: 0                             # 0: NFW, 1: Hernquist, 2: Burkert, 3: Einasto, 4: Plummer, 5: CoredIsothermal, 6: SIS, 7: Dehnen, 8: PointMass, 9: Jaffe, 10: Moore, 11: Soliton, 12: BetaModel
      mass: 8.0                              # Halo mass in simulation mass units
      scale_radius: 40.0                     # Characteristic scale radius a
      concentration: 10.0                    # Virial concentration parameter c = r_vir / a
      shape: 0.17                            # Profile shape index (Einasto alpha, Dehnen gamma)
      softening: 0.0                         # Core Plummer softening length epsilon
      truncation_radius: 0.0                 # Sharp outer boundary radius (0 for infinite)
      position: [0.0, 0.0, 0.0]              # Centroid coordinates [x, y, z]
      velocity: [0.0, 0.0, 0.0]              # Drift velocity vector [vx, vy, vz]
      anchor_body_id: 0                      # Tracking catalog body identifier (0 for none)
      axis_ratio_y: 1.0                      # Triaxial minor axis ratio q_y = b / a
      axis_ratio_z: 1.0                      # Triaxial minor axis ratio q_z = c / a
      yaw_deg: 0.0                           # Orientation yaw rotation angle around Z in degrees
      pitch_deg: 0.0                         # Orientation pitch tilt angle around Y in degrees
      roll_deg: 0.0                          # Orientation roll angle around X in degrees
      lens_scale: 1.0                        # Per-halo gravitational lensing multiplier
      lensing_enabled: true                  # Per-halo geodesic lensing toggle
      affects_bodies: true                   # Per-halo gravitational coupling toggle
      visualize: true                        # Per-halo visualization emission toggle
      visual_gain: 1.0                       # Per-halo radiance gain multiplier
      tint: [0.55, 0.40, 1.0]                # RGB false-color visualization tint vector
      orbit_enabled: false                   # Keplerian orbit toggle around reference center
      orbit_radius: 0.0                      # Orbital semi-major axis
      orbit_eccentricity: 0.0                # Orbital eccentricity in [0, 0.95]
      orbit_angular_speed: 0.0               # Mean orbital angular rate in rad/s
      orbit_inclination_deg: 0.0             # Orbital plane inclination in degrees
      orbit_node_deg: 0.0                    # Line of nodes angle in degrees
      orbit_periapsis_deg: 0.0               # Argument of periapsis in degrees
      orbit_phase_deg: 0.0                   # Initial orbital phase angle in degrees

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
    color_tertiary: [0.95, 0.95, 0.98, 1.0]  # Tertiary accent RGBA display color
    is_spacetime_source: false               # Independent Kerr black hole toggle (enables Kerr horizon, spin triad, and multi-source ray tracing)
    preset_3d: 1                             # 0: Star, 1: Terrestrial, 2: GasGiant, 3: IceGiant, 4: Metallic, 5: Asteroid, 6: NeutronStar, 7: Pulsar, 8: BlackHole, 9: Custom
    surface_texture_mode: 0                  # 0: ProceduralNoise, 1: Solid, 2: Palette, 3: BandedGas, 4: Cratered, 5: Stellar, 6: Accretion, 7: Marbled, 8: RingedGas, 9: Ice, 10: Magma, 11: CityLights, 12: Nebula, 13: EarthBlueMarble, 14: SunPhotosphere, 15: MercuryCratered, 16: VenusAtmosphere, 17: VenusRadarAltimetry, 18: MarsDesertBasalt, 19: JupiterStormBands, 20: SaturnCloudBelts, 21: UranusMethaneHaze, 22: NeptuneVortexBands, 23: CeresDwarfPlanet, 24: ErisTransNeptunian, 25: HaumeaCrystallineIce, 26: MakemakeTholinFrost
    geometry_model: 0                        # 0: OblateSpheroid, 1: RigidSphere, 2: ProlateSpheroid, 3: TriaxialEllipsoid
    surface_noise_scale: 10.0                # Base procedural noise frequency scale
    surface_roughness: 0.5                   # Diffuse surface microfacet roughness
    texture_detail_scale: 1.0                # Multiplier for secondary pattern octaves
    polar_cap_strength: 0.0                  # Tertiary color polar ice cap blending
    night_side_light_intensity: 0.0          # Unlit hemisphere city-lights luminance
    ring_system_enabled: false               # Equatorial ring shadow occlusion toggle
    atmosphere_mode: 2                       # 0: RayleighLimb, 1: Volumetric, 2: Off, 3: ThickHaze, 4: VolumetricMie, 5: Corona
    atmosphere_thickness: 0.0                # Atmosphere limb shell radial thickness
    atmosphere_color: [0.3, 0.6, 1.0, 1.0]   # Atmosphere scattering RGBA color
    emission_intensity: 0.0                  # Intrinsic self-illumination radiance
    specular_roughness: 0.5                  # Specular reflection highlight roughness
    rotation_speed_3d: 0.0                   # Visual 3D rotation rate around spin axis
    rotation_axis_3d: [0.0, 0.0, 1.0]        # Local 3D rotation axis unit vector
    orientation_euler: [0.0, 0.0, 0.0]       # Euler angles [roll, pitch, yaw] in degrees
    spin_axis: [0.0, 0.0, 1.0]               # Intrinsic spin angular momentum axis
    earth_map_variant: 0                     # 0: Day, 1: Night, 2: Automatic
    earth_map_quality: 0                     # 0: 1K (1024x512), 1: 2K (2048x1024)
    earth_terminator_softness: 0.15          # Twilight day/night transition blend width
    layer_template_1: -1                     # First procedural layer template index (-1 for none)
    layer_template_2: -1                     # Second procedural layer template index (-1 for none)
    has_orbit: false                         # Keplerian orbital parameter toggle
    parent_name: ""                          # Name of gravitational parent attractor
    parent_id: -2                            # Numeric ID of parent (-2 for central mass)
    semi_major_axis: 0.0                     # Semi-major axis in simulation length units
    eccentricity: 0.0                        # Orbital eccentricity in [0, 1)
    inclination_deg: 0.0                     # Orbital inclination in degrees
    longitude_node_deg: 0.0                  # Longitude of ascending node in degrees
    argument_periapsis_deg: 0.0              # Argument of periapsis in degrees
    true_anomaly_deg: 0.0                    # Initial true anomaly in degrees
    count: 1                                 # Batch expansion instance count
    disk_enabled: false                      # Subsidiary black hole accretion disk toggle
    disk_inner_scale: 1.0                    # Subsidiary disk inner edge multiple of ISCO
    disk_outer_rg: 25.0                      # Subsidiary disk outer radius in units M
    disk_peak_temp: 15000.0                  # Subsidiary disk peak blackbody temperature in Kelvin
    disk_floor_temp: 1200.0                  # Subsidiary disk floor temperature in Kelvin
    disk_brightness: 1.0                     # Subsidiary disk radiance multiplier
    disk_opacity: 1.0                        # Subsidiary disk optical depth scale
    disk_beaming: 0.0                        # Subsidiary disk extra Doppler beaming exponent
    disk_saturation: 1.0                     # Subsidiary disk color saturation multiplier
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

patterns:
  - pattern_type: "asteroid_belt"            # "asteroid_belt", "debris_swarm", "spherical"
    preset: "main_asteroid_belt"             # Preconfigured archetype template name
    object_count: 150                        # Number of bodies to instantiate
    parent_name: "Central Object"            # Gravitational parent attractor name
    parent_id: -2                            # Attractor identifier (-2 for central origin)
    center_position: [0.0, 0.0, 0.0]         # Centroid coordinate offset [x, y, z]
    center_velocity: [0.0, 0.0, 0.0]         # Centroid velocity offset [vx, vy, vz]
    plane_inclination_deg: 0.0               # Common orbital plane inclination in degrees
    plane_longitude_node_deg: 0.0            # Common line of nodes angle in degrees
    plane_argument_periapsis_deg: 0.0        # Common argument of periapsis in degrees
    mean_semi_major_axis: 32.0               # Central radial distance or semi-major axis
    radial_thickness: 12.0                   # Radial width of formation band
    radial_distribution: "uniform"           # "uniform", "gaussian", "power_law", "kirkwood"
    radial_gaussian_sigma: 2.5               # Standard deviation for Gaussian radial profile
    radial_power_law_index: -1.5             # Radial density power-law exponent
    kirkwood_gaps_enabled: false             # Depleted resonance gaps toggle
    kirkwood_gap_count: 3                    # Number of simulated resonance gaps
    kirkwood_resonance_depth: 0.85           # Resonant gap depletion depth fraction
    vertical_scale_height: 1.0               # Vertical half-thickness scale height
    vertical_distribution: "gaussian"        # "gaussian", "uniform"
    mean_eccentricity: 0.06                  # Mean orbital eccentricity
    eccentricity_dispersion: 0.035           # Random eccentricity dispersion width
    eccentricity_distribution: "rayleigh"    # "rayleigh", "gaussian", "uniform"
    mean_inclination_dispersion_deg: 3.5     # Out-of-plane inclination dispersion width
    inclination_distribution: "rayleigh"     # "rayleigh", "gaussian", "uniform"
    angular_distribution: "uniform"          # "uniform", "lagrangian", "spiral"
    angular_opening_deg: 360.0               # Azimuthal sector opening width
    angular_center_deg: 0.0                  # Central azimuth of debris wedge
    lagrangian_clustering_strength: 0.8      # L4/L5 Trojan concentration factor
    velocity_mode: "keplerian_circular"      # "keplerian_circular", "keplerian_eccentric", "radial_expansion", "virial", "solid_body"
    mean_velocity_multiplier: 1.0            # Velocity scale multiplier
    velocity_dispersion_fraction: 0.03       # Fractional random kinematic velocity dispersion
    velocity_distribution: "maxwell_boltzmann" # "maxwell_boltzmann", "gaussian", "uniform"
    prograde_fraction: 1.0                   # Fraction of prograde orbit trajectories
    fixed_velocity_vector: [0.0, 0.0, 0.0]   # Directional velocity vector
    expansion_speed: 0.05                    # Outward velocity magnitude for expansion
    spherical_type: "plummer_sphere"         # "thin_shell", "thick_shell", "plummer_sphere", "uniform_sphere", "fibonacci_shell"
    thin_shell_radius: 28.0                  # Radius of thin spherical shell
    plummer_scale_radius: 16.0               # Scale radius b of Plummer cluster profile
    uniform_sphere_radius: 25.0              # Outer radius of uniform density sphere
    shell_inner_radius: 14.0                 # Inner hollow cavity boundary
    shell_outer_radius: 42.0                 # Outer shell boundary
    spherical_power_law_gamma: -2.0          # Radial density falloff exponent
    solid_body_rotation_speed: 0.04          # Solid-body angular rotation velocity
    solid_body_axis_tilt_deg: 0.0            # Solid-body rotation axis polar tilt
    solid_body_axis_azimuth_deg: 0.0         # Solid-body rotation axis azimuth
    archetype: "asteroid"                    # Physical material and visual template
    mass_distribution: "power_law"           # "fixed", "uniform", "log_uniform", "power_law"
    fixed_mass: 1.0e-7                       # Body mass for fixed distribution
    min_mass: 1.0e-9                         # Lower bound of mass interval
    max_mass: 2.0e-6                         # Upper bound of mass interval
    mass_power_law_alpha: 1.833              # Differential mass function exponent
    radius_from_density: true                # Derive radius from mass and density toggle
    mean_density: 2700.0                     # Volumetric mass density in kg/m^3
    density_dispersion_pct: 15.0             # Density variance percentage
    fixed_radius: 0.05                       # Radius for explicit radius mode
    min_radius: 0.005                        # Minimum explicit radius
    max_radius: 0.4                          # Maximum explicit radius
    temperature_equilibrium: true            # Central radiative equilibrium temperature toggle
    reference_temperature_k: 250.0           # Blackbody temperature at reference radius
    temperature_falloff_exponent: 0.5        # Radial temperature decay power-law exponent
    color_jitter: 0.2                        # Random surface color chromatic variation
    name_prefix: "Asteroid"                  # Name prefix for enumerated bodies
    avoid_horizon_overlap: true              # Event horizon proximity rejection toggle
    avoid_mutual_collisions: true            # Initial body overlap rejection toggle
    seed: 1337                               # Deterministic PRNG seed

observers:
  - name: "PrimaryObserver"
    fov_deg: 60.0                            # Field of view in degrees
    resolution: [1920, 1080]                 # Raster dimensions [width, height]
    position: [0.0, 50.0, 0.0, 0.0]          # Observer position [t, x, y, z]
    orientation: [0.0, 180.0, 0.0]           # Camera orientation [pitch, yaw, roll] in degrees
    orientation_convention: 1                # 0: target origin; 1: explicit orientation
    target_name: ""                          # Designated tracking target label
    target_body_id: -1                       # Target body ID (-1 for none)
    target_mode: "center_of_mass"            # "center_of_mass", "geometric"
    look_at_offset: [0.0, 0.0, 0.0]          # Gaze target position offset [dx, dy, dz]
    position_unit: "m"                       # Distance unit for coordinate export
    fov_unit: "deg"                          # Angular unit for FOV
    four_velocity: [1.0, 0.0, 0.0, 0.0]      # Comobile four-velocity [u0, u1, u2, u3]
```

### 1.2. Parsing, Validation and Application Rules

**Parsing.** `ScenarioSerializer::from_yaml` (`include/relativistic/io/scenario_serializer.hpp`) is a line-oriented reader, not a complete YAML parser:

- Section keys (`spacetime`, `integrator`, `output`, `interactions`, `bodies`, `patterns`, `observers`) switch the active section; keys outside a section apply to the root.
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

**Application on load** (`SimulationOrchestrator::load_scenario_file` in `include/relativistic/orchestrator/simulation_orchestrator.hpp`). The loader applies the scenario name and path, the metric name, `central_mass`, `central_spin`, `central_charge`, `cosmological_lambda`, `wormhole_throat`, `warp_velocity`, `warp_bubble_radius`, `warp_wall_sharpness`, `wormhole_tidal_potential`, `teves_critical_acceleration`, `pole_guard_precision_scale`, `far_field_step_scale`, the integrator `scheme`, the `time` configuration (warp factor, tick rate, pause state), the `skybox` configuration (master intensity, star density, brightness, size, profile, placement, nebulae, galaxies, clusters, dust, quasars, pulsars, supernova remnants, coordinate grid, and panoramas), the `primary_disk` parameters (temperatures, beaming, rings, spirals, turbulence, and equation of state), the `primary_jet` parameters (geometry, field index, angular velocity, magnetization, opening angle, Lorentz factor, emission model), the complete `interactions` block ([DESCRIPTION.md Section 5.4](DESCRIPTION.md#54-body-interactions--spacetime-source-bodies)), the physical display units, the first observer (position, field of view, orientation, target tracking), and all bodies and geometric patterns, replacing the active catalog. Surface layers are populated from template indices. The fields `speed_of_light`, `gravitational_constant`, numeric integrator tolerance fields, additional observers, and the `output` block are parsed but not applied to the runtime simulation state.

**Content of saved files.** `SimulationOrchestrator::save_scenario_file` writes the live central parameters, the integrator name and current `integration_rtol` and `integration_atol`, the `time` scheduler settings, the active `skybox` parameters (procedural generator seed, intensities, stars, nebulae, galaxies, clusters, dust, coordinate grid lines and cell colors), the `primary_disk` parameters, the `primary_jet` parameters, the interaction configuration, physical display units, one observer built from the live camera, and all bodies with their geometric models, surface textures, photographic variants, procedural layers, and subsidiary disks. Rendering and performance settings are never written. Paths are resolved as described in [ARCHITECTURE.md](ARCHITECTURE.md#35-scenario-resolution-and-startup).

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

### 2.3. Optical/Infrared/Radio Interferometry Exchange Format (`.oifits`)

Synthetic interferometric datasets exported by `OifitsWriter` (`include/relativistic/interferometry/oifits_writer.hpp`) adhere to the IAU OIFITS Standard format (FITS binary table extensions). The primary HDU contains observation metadata keywords followed by standard binary extension tables:

#### Primary HDU Keywords
| Keyword | Value Type | Description |
| :--- | :--- | :--- |
| `SIMPLE` | Logical (`T`) | Standard FITS conforming header. |
| `BITPIX` | Integer (`8`) | Header-only primary HDU. |
| `NAXIS` | Integer (`0`) | Zero-dimensional array; data stored in extension tables. |
| `EXTEND` | Logical (`T`) | Binary table extensions follow. |
| `TELESCOP` | String | Synthetic instrument identifier (`'EHT_VLBI_SYNTH'`). |
| `ORIGIN` | String | Generating software (`'RelativisticEngine'`). |
| `OBJECT` | String | Target astronomical source designation. |
| `RA`, `DEC` | Float | J2000 target celestial coordinates in degrees. |

#### Extension Tables

1. `OI_TARGET`: Defines astronomical source targets.
   - Columns: `TARGET_ID` (Int16), `TARGET` (Char16), `RAEP0` (Float64, deg), `DECEP0` (Float64, deg), `EQUINOX` (Float32, 2000.0), `RA_ERR` (Float64), `DEC_ERR` (Float64), `SYSVEL` (Float64), `VELTYP` (Char8), `VELDEF` (Char8), `PMRA` (Float64), `PMDEC` (Float64), `PMRA_ERR` (Float64), `PMDEC_ERR` (Float64), `PARALLAX` (Float32), `PARA_ERR` (Float32), `SPECTYP` (Char16).

2. `OI_ARRAY`: Defines station coordinates and telescope geometries.
   - Keywords: `ARRNAME` (Array designation), `FRAME` (`'GEOCENTRIC'`), `ARRAYX`, `ARRAYY`, `ARRAYZ` (Float64, meters).
   - Columns: `TEL_NAME` (Char16), `STA_NAME` (Char16), `STA_INDEX` (Int32), `DIAMETER` (Float32, meters), `STAXYZ` (Float64[3], Cartesian coordinates relative to array center).

3. `OI_WAVELENGTH`: Defines effective spectral channels.
   - Keywords: `INSNAME` (Instrument name).
   - Columns: `EFF_WAVE` (Float32, meters), `EFF_BAND` (Float32, bandwidth in meters).

4. `OI_VIS`: Complex visibilities and correlated flux densities.
   - Keywords: `DATE-OBS` (Observation epoch).
   - Columns: `TARGET_ID` (Int16), `TIME` (Float64, seconds past reference), `MJD` (Float64, modified Julian date), `INT_TIME` (Float64, integration seconds), `VISAMP` (Float64, correlated flux density in Jy), `VISAMPERR` (Float64, 1-sigma uncertainty), `VISPHI` (Float64, visibility phase in degrees), `VISPHIERR` (Float64, phase uncertainty in degrees), `UCOORD` (Float64, spatial frequency $u$ in meters), `VCOORD` (Float64, spatial frequency $v$ in meters), `STA_INDEX` (Int32[2], station pair indices), `FLAG` (Logical, invalid/shadowed flag).

5. `OI_VIS2`: Squared visibility amplitudes.
   - Columns: `TARGET_ID` (Int16), `TIME` (Float64), `MJD` (Float64), `INT_TIME` (Float64), `VIS2DATA` (Float64, $|V|^2$ in $\text{Jy}^2$), `VIS2ERR` (Float64), `UCOORD` (Float64, meters), `VCOORD` (Float64, meters), `STA_INDEX` (Int32[2]), `FLAG` (Logical).

6. `OI_T3`: Triple products and closure phases.
   - Columns: `TARGET_ID` (Int16), `TIME` (Float64), `MJD` (Float64), `INT_TIME` (Float64), `T3AMP` (Float64, triple product amplitude), `T3AMPERR` (Float64), `T3PHI` (Float64, closure phase in degrees $[-\pi, \pi]$), `T3PHIERR` (Float64), `U1COORD`, `V1COORD` (Float64, baseline 1 in meters), `U2COORD`, `V2COORD` (Float64, baseline 2 in meters), `STA_INDEX` (Int32[3], station triplet indices), `FLAG` (Logical).

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
| `/eos/log_press` | `Float64` | $[N_\rho \times N_T \times N_{Y_e}]$ | Base-10 logarithm of pressure in $\text{Pa}$ across $(\log_{10}\rho, \log_{10}T, Y_e)$ ([MATHEMATICAL_FORMULATION.md Section 4.9](MATHEMATICAL_FORMULATION.md#49-relativistic-equations-of-state-eos-formulations)). |
| `/eos/log_eps` | `Float64` | $[N_\rho \times N_T \times N_{Y_e}]$ | Base-10 logarithm of energy density in $\text{J/m}^3$ across $(\log_{10}\rho, \log_{10}T, Y_e)$. |
| `/eos/enthalpy` | `Float64` | $[N_\rho \times N_T \times N_{Y_e}]$ | Dimensionless specific enthalpy $h$ across $(\log_{10}\rho, \log_{10}T, Y_e)$. |
| `/eos/cs2` | `Float64` | $[N_\rho \times N_T \times N_{Y_e}]$ | Dimensionless sound speed squared $(c_s / c)^2$ across $(\log_{10}\rho, \log_{10}T, Y_e)$. |
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
| `sequence_info.txt` | `key=value` lines: `mode` (`deterministic` or `real-time`), `frames_submitted`, `frames_written`, `frames_dropped`, `frames_duplicated`, `frames_per_second`, `frame_format`, `start_frame_index`, `camera_collision` (obstacle avoidance state 0 or 1), `recording` (path of the telemetry file when recorded), and for deterministic sessions `resolution`, `supersampling`, `temporal_samples`, followed by `ffmpeg` (assembly command) when applicable. |
| `motion_script.cfg` | Copy of the motion script used by the session (see [Section 9](#9-motion-script-and-capture-settings-files)). |
| `video.<ext>` | Output of the optional ffmpeg assembly. |
| Telemetry files | Described in [Section 8](#8-telemetry-recording-files). |

`VideoSequenceSettings::build_ffmpeg_command` (`include/relativistic/io/video_capture_settings.hpp`) generates the assembly command from `-framerate`, `-start_number`, the frame pattern `frame_%0<padding>d.<ext>`, custom video filter chains (`custom_video_filters`) prefixed to `pad=ceil(iw/2)*2:ceil(ih/2)*2` (or two-pass Bayer palette generation for GIF: `split[s0][s1];[s0]palettegen=stats_mode=diff[p];[s1][p]paletteuse=dither=bayer:bayer_scale=3`), `-c:v <encoder>`, rate control options (`ConstantQuality` via `-crf` or `TargetBitrate` via `-b:v <kbps>k`), encoder speed presets (`Ultrafast` through `Veryslow`), output framerate override (`-r <fps>`), and custom output arguments (`custom_output_arguments`). The image sequence codec generates no command.

| Codec | Encoder | Rate Control & Speed Presets | Container Defaults |
| :--- | :--- | :--- | :--- |
| H264 | `libx264` | `-preset` (`ultrafast` to `veryslow`), `-crf` or `-b:v <k>k`, `-pix_fmt yuv420p` | MP4, MKV |
| H265 | `libx265` | `-preset` (`ultrafast` to `veryslow`), `-crf` or `-b:v <k>k` | MP4, MKV |
| VP9 | `libvpx-vp9` | `-crf <n> -b:v 0` or `-b:v <k>k`, `-deadline` (`realtime` for speed $\le 2$, `good` for $3..5$, `best` for $\ge 6$), `-pix_fmt yuv420p` | WebM, MKV |
| ProRes | `prores_ks` | `-profile:v 3`, `-pix_fmt yuv422p10le` | Forced to MOV |
| PngSequence | `png` | Direct individual image frame dump without assembly invocation | Folder of PNGs |
| AV1 | `libsvtav1` | `-preset` 0 to 12, `-crf` or `-b:v <k>k`, `-pix_fmt yuv420p` | WebM, MKV, MP4 |
| FFV1 | `ffv1` | `-level 3`, `-pix_fmt yuv420p` (lossless archive) | Forced to MKV |
| MJPEG | `mjpeg` | `-q:v` scaled from CRF ($1$ to $31$), `-pix_fmt yuvj420p` | AVI, MKV |
| GIF | `gif` | Two-pass palette generation with differential stats and Bayer dither, optional `-loop 0` | Forced to GIF |

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
| `script.seg<i>.layer<j>.` | Shape layer: name, enabled, blend (0 replace, 1 add, 2 add relative), weights `ws` and `we`, window `win0` and `win1`, `ease.` easing, `shape.` shape (kind [0 to 28], controls `c0` to `c3`, values `v0` to `v7`, expressions `e0` to `e2`, closed, uniform, waypoints `wp`, transform `scale`, `rot`, `pivot`, `trans`). Supports all 29 shape kinds including surface walk, surface loop, surface route, planet orbit, free-fall infall, conical helix, Keplerian orbit, and dynamic chase. |
| `script.seg<i>.orient.` | View orientation: mode (0 free, 1 fixed, 2 interpolated, 3 look at target, 4 along travel, 5 expressions, 6 target path, 7 surface walker, 8 chase lead), start/end pitch and yaw, easing, target body ID, target offset vector, look-ahead progress factor, pitch/yaw offsets, expressions, target shape stack, and `vel_lead` (velocity lead time in seconds for mode 8). |
| `script.seg<i>.<channel>.` with `<channel>` in `fov`, `exposure`, `roll`, `warp` | Animated scalar channels: enabled, start, end, easing, expression override, periodic modulation (`mod.enabled`, `mod.wave`, `mod.amplitude`, `mod.amplitude_end`, `mod.frequency`, `mod.phase`, `mod.fade`), and reactive driver (`driver.enabled`, `driver.source` [0 to 11], `driver.body`, `driver.point`, `driver.event`, `driver.expression`, `driver.input_min/max`, `driver.output_min/max`, `driver.clamp_input`, `driver.blend`, `driver.response.` easing). |
| `script.seg<i>.shake.` | Camera shake: enabled, position amplitude vector, rotation amplitude vector in degrees, frequency in Hz, integer seed, edge fade fraction. |
| `script.seg<i>.transition.` | Segment transition blend from previous pose: enabled, orientation blend duration, orientation blend mode (0 shortest arc, 1 direct), orientation easing, position blend duration, position easing, lens/rate blend duration, lens easing, blend simulation rate toggle, projection transition duration (`proj_s`), projection transition easing (`proj_ease.`), projection transition mode (`proj_mode`: 0 StepAtMidpoint, 1 StepAtStart, 2 StepAtEnd, 3 SmoothMorph, 4 FovCompensatedMorph, 5 CrossZoom, 6 WarpCurvature), and projection FOV compensation scale (`proj_comp`). |
| `script.seg<i>.proj_mode` | Explicit projection mode assignment for the segment (-1 to 7: -1 Live Default, 0 Pinhole, 1 AutoZoom, 2 FisheyeStereographic, 3 Equirectangular360, 4 FisheyeEquidistant, 5 FisheyeOrthographic, 6 PaniniCylindrical, 7 HammerAitoff). |
| `script.events`, `script.ev<i>.` | Scheduled events count and event entries: name, enabled, trigger (0 script time, 1 segment start, 2 segment end, 3 segment fraction), time in seconds, segment index, fraction, time offset in seconds, action (0 to 14: 0 Marker, 1 CaptureStill, 2 SetParameter, 3 SetWarp, 4 Pause, 5 Resume, 6 StepTicks, 7 SetMetric, 8 SetIntegrator, 9 LoadScenario, 10 SetTickRate, 11 SetPerformancePreset, 12 SetOverlay, 13 SetResolutionScale, 14 SetProjectionMode), parameter type (52 mapped parameter types), start value, end value, transition ramp duration, ramp easing curve, and text argument. |

Easing blocks (`EasingSpec::write` in `include/relativistic/capture/easing.hpp`) use the suffixes `kind`, `p0` to `p3`, `blend`, `repeat`, `ping_pong`, `reverse`, `in0`, `in1`, `out0`, `out1`, `bias`, `gain`, `quant`, `clamp_out`, `smooth`, `expr`, `points`, `pt<i>.u` and `pt<i>.v`. The script structure and the available shapes, easings, orientation modes, events and expression language are described in [ARCHITECTURE.md](ARCHITECTURE.md#210-capture-subsystem).

**`config/capture_path.cfg`** is the legacy camera path format (lines such as `kind=`, `duration=`, `orbit=`, `key=`, see `CameraPath::to_text` in `include/relativistic/capture/camera_path.hpp`). It is read only when `config/capture_script.cfg` is absent or invalid, and is converted by `MotionScript::from_legacy_path`.

**`config/capture_studio.cfg`** is written by `CaptureStudioSettings::save` (`include/relativistic/io/capture/capture_studio_settings.hpp`) and clamped on load by `CaptureStudioSettings::sanitize`.

| Keys | Content |
| :--- | :--- |
| `screenshot_<k>`, `sequence_<k>` with `<k>` in `use_explicit_resolution`, `resolution_multiplier`, `explicit_width`, `explicit_height`, `lock_aspect_ratio`, `locked_aspect_ratio`, `supersampling`, `max_ray_steps`, `step_refinement`, `layers`, `body_quality`, `hide_accretion_disk`, `projection_override`, `tonemapping_override`, `fov_override`, `exposure_offset`, `apply_post_processing`, `dither_strength` | Capture target configuration, resolution constraints, and render quality overrides for screenshots and sequences |
| `frames_per_second`, `duration_seconds`, `fixed_frame_count`, `resolution_scale`, `frame_format`, `mode`, `trigger`, `advance_mode`, `pause_simulation_during_capture`, `ticks_per_frame`, `simulation_seconds_per_video_second`, `temporal_samples`, `shutter_fraction`, `shutter_phase`, `fade_in_frames`, `fade_out_frames`, `start_frame_index`, `frame_name_padding`, `preview_in_viewport`, `restore_camera_after_capture`, `script_time_offset_seconds`, `script_speed`, `pacing`, `realtime_queue_depth`, `codec`, `container`, `rate_control`, `encoder_speed`, `crf`, `target_bitrate_kbps`, `pixel_format`, `encode_frames_per_second`, `loop_output`, `assemble_video_after_capture`, `delete_frames_after_assembly`, `ffmpeg_executable`, `custom_video_filters`, `custom_output_arguments` | Sequence timing, motion blur shutter phase, and ffmpeg encoding parameters (`VideoSequenceSettings`) |
| `sequence_directory`, `session_name`, `use_script` (legacy alias `use_camera_path`), `manual_stepping` | Session destination folder, naming template, and execution options |
| `recording_<k>` with `<k>` in `enabled`, `format`, `channel_mask`, `body_positions`, `body_velocities`, `body_camera_distance`, `body_speeds`, `max_bodies`, `decimation`, `precision`, `si_units`, `write_events`, `file_stem` | Telemetry recording (`RecordingSettings`); `channel_mask` is a bit mask over the 54 physical channels in the order of the table in [Section 8](#8-telemetry-recording-files) |

---

## 10. Persistent Configuration Files

### 10.1. Modular Preferences Architecture (`config/preferences/`)

Written by `UserSettings::save` (`include/relativistic/io/user_settings.hpp`), format version 7. Runtime preferences are partitioned into nine modular domain files stored inside the `config/preferences/` directory. Legacy unified `config/user_settings.cfg` files up to version 6 are automatically migrated to `config/preferences/` on startup and purged.

| Configuration File | Domain Keys | Contents |
| :--- | :--- | :--- |
| `general.cfg` | `format_version`, `load_policy`, `default_performance_preset`, `default_use_gpu_compute`, `load_scenario_on_startup`, `default_scenario_path`, `show_system_console`, `last_window_layout`, `multi_window_mode` | Core engine lifecycle, startup scenario resolution, and workspace dock layout. |
| `camera.cfg` | `default_camera_mode`, `cam_ff_*`, `cam_zoom_*`, `cam_orbit_*`, `cam_rocket_*`, `cam_walk_*`, `cam_planet_*`, `cam_keyboard_layout`, `cam_arrow_keys_mode`, `arrow_look_sensitivity`, `arrow_look_invert_pitch`, `arrow_look_invert_yaw`, `arrow_look_invert_roll`, `gp_enabled`, `gp_selected_joystick`, `gp_deadzone_ls`, `gp_deadzone_rs`, `gp_deadzone_trigger`, `gp_look_sens_x`, `gp_look_sens_y`, `gp_move_sens`, `gp_roll_sens`, `gp_invert_look_y`, `gp_invert_look_x`, `gp_invert_roll`, `gp_invert_vertical`, `gp_bind_*`, `kb_*` | 6-DOF Free-Fly, Orbit, Rocket, Surface Walker, Planet Orbit, Gamepad analog axes, virtual buttons, chords, and rebindable keyboard chords. |
| `render_sky.cfg` | `sky_rendering_enabled`, `sky_seed_locked`, `sky_seed`, `sky_master_intensity`, `sky_quality_factor`, `sky_star_quality_factor`, `sky_nebula_quality_factor`, `sky_cluster_quality_factor`, `sky_dust_quality_factor`, `sky_quasar_quality_factor`, `sky_pulsar_quality_factor`, `sky_snr_quality_factor`, `sky_stars_enabled`, `sky_star_density`, `sky_star_brightness`, `sky_star_size_scale`, `sky_star_size_min`, `sky_star_size_max`, `sky_star_size_param`, `sky_star_size_distribution`, `sky_star_luminosity_param`, `sky_star_luminosity_distribution`, `sky_star_profile_model`, `sky_star_profile_param`, `sky_star_profile_param2`, `sky_star_profile_softness`, `sky_star_placement_model`, `sky_star_galactic_concentration`, `sky_star_spike_count`, `sky_star_spike_length`, `sky_star_spike_intensity`, `sky_star_spike_angle_deg`, `sky_star_dirac_concentration`, `sky_star_bloom_radius`, `sky_star_bloom_intensity`, `sky_star_color_variation`, `sky_star_temperature_bias`, `sky_star_brightness_variation`, `sky_star_scintillation`, `sky_star_magnitude_limit`, `sky_nebula_enabled`, `sky_nebula_intensity`, `sky_nebula_octaves`, `sky_nebula_roughness`, `sky_nebula_frequency`, `sky_nebula_plane_thickness`, `sky_nebula_plane_falloff`, `sky_nebula_asymmetry`, `sky_nebula_absorption_density`, `sky_nebula_layer_count`, `sky_nebula_layer_spread`, `sky_nebula_core_glow`, `sky_nebula_curl_strength`, `sky_nebula_color_separation`, `sky_nebula_ionization_glow`, `sky_nebula_gas_contrast`, `sky_nebula_plane_normal_*`, `sky_nebula_asymmetry_dir_*`, `sky_nebula_emission_color_*`, `sky_nebula_secondary_color_*`, `sky_nebula_tertiary_color_*`, `sky_galaxies_enabled`, `sky_galaxy_density`, `sky_galaxy_brightness`, `sky_galaxy_size_scale`, `sky_galaxy_size_min`, `sky_galaxy_size_max`, `sky_galaxy_size_param`, `sky_galaxy_size_distribution`, `sky_galaxy_morphology_model`, `sky_galaxy_spiral_arms`, `sky_galaxy_pitch_angle`, `sky_galaxy_core_ratio`, `sky_galaxy_ellipticity_variance`, `sky_galaxy_bar_strength`, `sky_galaxy_dust_lane_opacity`, `sky_galaxy_clumpiness`, `sky_galaxy_disk_thickness`, `sky_galaxy_inclination_variance`, `sky_galaxy_bulge_sersic_index`, `sky_galaxy_weight_*`, `sky_clusters_enabled`, `sky_cluster_density`, `sky_cluster_brightness`, `sky_cluster_size_scale`, `sky_cluster_size_min`, `sky_cluster_size_max`, `sky_cluster_size_distribution`, `sky_cluster_placement_model`, `sky_cluster_concentration`, `sky_cluster_star_density_boost`, `sky_cluster_halo_radius`, `sky_cluster_plane_scale_height`, `sky_cluster_halo_intensity`, `sky_cluster_spread_factor`, `sky_cluster_core_tint_*`, `sky_cluster_stars_enabled`, `sky_cluster_member_count`, `sky_cluster_member_size`, `sky_cluster_member_brightness`, `sky_cluster_star_profile_model`, `sky_cluster_inherit_star_profile`, `sky_dust_enabled`, `sky_dust_density`, `sky_dust_intensity`, `sky_dust_scale`, `sky_dust_absorption_ratio`, `sky_dust_contrast`, `sky_dust_fractal_octaves`, `sky_dust_tint_*`, `sky_quasar_enabled`, `sky_quasar_density`, `sky_quasar_brightness`, `sky_quasar_core_size`, `sky_quasar_jet_length`, `sky_quasar_jet_intensity`, `sky_quasar_jet_opening_angle_deg`, `sky_quasar_jet_lorentz_factor`, `sky_quasar_accretion_disk_glow`, `sky_quasar_precession_angle_deg`, `sky_pulsar_enabled`, `sky_pulsar_density`, `sky_pulsar_brightness`, `sky_pulsar_pulse_rate`, `sky_pulsar_pulse_duty`, `sky_pulsar_beam_width_deg`, `sky_pulsar_magnetic_tilt_deg`, `sky_pulsar_wind_nebula_scale`, `sky_pulsar_wind_nebula_intensity`, `sky_pulsar_frequency_spread`, `sky_snr_enabled`, `sky_snr_density`, `sky_snr_brightness`, `sky_snr_size`, `sky_snr_shell_thickness`, `sky_snr_expansion_age`, `sky_snr_asymmetry`, `sky_snr_filament_scale`, `sky_snr_interior_glow`, `sky_snr_color_variation`, `sky_snr_morphology_variety`, `sky_snr_shell_color_*`, `sky_snr_interior_color_*`, `sky_grid_enabled`, `sky_grid_opacity`, `sky_grid_line_width`, `sky_grid_spacing_deg`, `sky_grid_subdivisions`, `sky_grid_glow`, `sky_grid_tick_interval_deg`, `sky_grid_tick_length`, `sky_grid_tick_width`, `sky_grid_cell_fill_mode`, `sky_grid_cell_opacity`, `sky_grid_cell_color_count`, `sky_grid_cell_color_*`, `sky_grid_equator_width`, `sky_grid_equator_opacity`, `sky_grid_polar_width`, `sky_grid_polar_opacity`, `sky_grid_meridian_width`, `sky_grid_meridian_opacity`, `sky_grid_major_width`, `sky_grid_minor_width`, `sky_grid_major_color_*`, `sky_grid_minor_color_*`, `sky_grid_equator_color_*`, `sky_grid_polar_color_*`, `sky_grid_meridian_color_*`, `sky_grid_tick_color_*`, `sky_grid_show_polar_caps`, `sky_grid_show_equator`, `sky_grid_show_meridians`, `sky_grid_show_ticks`, `sky_dynamic_motion`, `sky_dynamic_rotation`, `sky_motion_speed`, `sky_rotation_rate_deg_s`, `sky_adaptive_fov_lod`, `sky_adaptive_fov_quality`, `sky_adaptive_fov_reference`, `sky_adaptive_fov_sensitivity`, `sky_max_star_octaves`, `sky_background_source`, `sky_panorama_id`, `sky_panorama_quality`, `sky_background_color_*`, `sky_hue_shift_deg`, `sky_saturation`, `sky_orientation_yaw_deg`, `sky_orientation_pitch_deg`, `sky_orientation_roll_deg`, `sky_grid_orientation_yaw_deg`, `sky_grid_orientation_pitch_deg`, `sky_grid_orientation_roll_deg` | Procedural multi-layer celestial skybox (stars, nebulae, clusters, dust, quasars, pulsars, SNR, grids) and spherical panoramas. |
| `interface.cfg` | `last_active_window`, `window_*_open`, `hud_master_enabled`, `hud_ext_toolbar_padding_scale`, `hud_ext_auto_arrange`, `hud_ext_auto_arrange_spacing`, `hud_coord_primary`, `hud_coord_secondary`, `hud_coord_secondary_enabled`, `hud_coord_axis_labels`, `hud_tb_*` (26 toolbar button toggles), `hud_ex_<i>_*` (34 HUD element styles: anchor, offset_x/y, nudge_x/y, scale, color, show_background, background_opacity, display_mode, decimal_precision, show_label, horizontal_layout, draw_priority, refresh_interval, warn_*, crit_*), `hud_show_gamepad_in_summary`, `hud_kbs_*`, `hud_gp_vis_*`, `schem_*`, `spatial_ref_*` (`enabled`, `show_raytraced`, `show_schematic`, `opacity`, `center_mode`, `custom_center.*`, `center_body`, `hide_horizon`, `lens`, `depth_fade`, `depth_fade_mode`, `depth_fade_reference`, `depth_fade_minimum`, `subdivisions`, `max_lines`, `max_ticks`, `axis<i>.*`, `axis_negative`, `axis_length_mode`, `axis_length`, `axis_arrows`, `axis_arrow_size`, `axis_labels`, `ticks`, `tick_spacing_mode`, `tick_spacing`, `tick_length`, `tick_thickness`, `tick_labels`, `tick_label_interval`, `grid_plane<i>`, `grid_minor.*`, `grid_major.*`, `grid_extent_mode`, `grid_extent`, `grid_spacing_mode`, `grid_spacing`, `grid_major_interval`, `grid_offsets.*`, `grid_radial_fade`, `rings`, `ring_plane`, `ring_spacing_mode`, `ring_spacing`, `ring_count`, `ring_style.*`, `ring_labels`, `origin_marker`, `origin_shape`, `origin_size`, `origin_color.*`, `scale_bar`, `scale_bar_corner`, `scale_bar_margin`, `scale_bar_target`, `scale_bar_thickness`, `scale_bar_color.*`, `scale_bar_label`, `label_scale`, `label_decimals`, `label_unit`, `label_background`, `label_background_opacity`, `label_color.*`), `wchrome_<i>_custom`, `wchrome_<i>_r/g/b`, `wgeo_count`, `wgeo_<i>_*`, `secondary_manager_sync`, `secondary_manager_beacons`, `secondary_manager_target_mode`, `secondary_manager_target_body`, `secondary_manager_target_x/y/z`, `secondary_view_<i>_role`, `secondary_view_<i>_target_mode`, `secondary_view_<i>_target_body`, `secondary_view_<i>_target_x/y/z`, `secondary_view_<i>_use_shared`, `secondary_view_<i>_beacon`, `secondary_view_<i>_probe_inbound`, `secondary_view_<i>_probe_speed`, `secondary_view_<i>_look_yaw`, `secondary_view_<i>_look_pitch`, `secondary_view_<i>_asymptotic_distance`, `secondary_view_<i>_asymptotic_extent` | Window visibility, complete HUD layout (34 elements, anchors, offsets, nudges, formats, priorities, warning/critical rules, auto-stacking), 26 toolbar buttons, schematic styles, complete 3D spatial reference system parameters, Window Chrome theming (14 window accent colors and window geometries fitted to monitors), and secondary observer viewport managers, synchronized targets, and per-view kinematic states. |
| `units.cfg` | `unit_distance`, `unit_velocity`, `unit_mass`, `unit_energy`, `unit_temperature`, `unit_angle`, `unit_charge`, `unit_current`, `unit_time`, `unit_acceleration`, `unit_angular_velocity`, `unit_density`, `unit_pressure`, `unit_power`, `unit_frequency`, `unit_force`, `unit_magnetic_field`, `unit_voltage`, `unit_frame_rate`, `coord_widget_default` | Display unit selections across 19 physical quantities and global default coordinate system. |
| `physics.cfg` | `constants_preset`, `constants_c`, `constants_g`, `constants_h`, `constants_kb`, `constants_na`, `constants_ke`, `constants_kcd`, `interaction_*`, `cosmo_om`, `cosmo_or`, `cosmo_pot`, `cosmo_slope`, `cosmo_scale`, `cosmo_phi0`, `cosmo_chap_a`, `cosmo_chap_alpha`, `cosmo_ltb_h0`, `cosmo_ltb_r0`, `cosmo_ltb_w`, `cosmo_ltb_in`, `cosmo_ltb_out`, `cosmo_ev_enabled`, `cosmo_ev_spectrum`, `cosmo_ev_model`, `cosmo_ev_dof`, `cosmo_ev_grey`, `cosmo_ev_accel`, `cosmo_ev_spin`, `cosmo_ev_min`, `cosmo_ev_gap`, `cosmo_ev_coupling` | Physical constants engine base values, N-body interaction couplings (Coulomb, dipole, Hertzian, thermo, frag, annihilation), cosmological dark energy and LTB parameters, and semiclassical Hawking evaporation settings. |
| `celestial_bodies.cfg` | `system_preset_selected`, `body_manager_sort_mode`, `body_manager_sort_descending`, `body_manager_search_filter`, `body_manager_list_pane_width`, `body_manager_source_sort_mode`, `body_manager_source_sort_descending`, `body_manager_source_search_filter`, `body_manager_source_list_pane_width`, `body_manager_bulk_parameter_index`, `body_manager_new_body_mass_log_mode`, `body_manager_new_body_radius_log_mode`, `body_manager_new_source_mass_log_mode`, `body_manager_reference_body_id`, `body_manager_relative_coords_mode`, `light_source_mode`, `light_attenuation_mode`, `light_source_body_id`, `light_intensity`, `light_ambient`, `light_terminator_softness`, `light_specular_scale`, `light_color_r`, `light_color_g`, `light_color_b`, `light_direction_azimuth_deg`, `light_direction_elevation_deg`, `light_position_x`, `light_position_y`, `light_position_z`, `light_reference_distance`, `body_lighting_detail`, `body_shadow_caster_limit`, `body_emission_source_limit`, `body_emission_lighting_enabled`, `body_emission_lighting_gain`, `body_emission_lighting_reference_distance`, `body_sec_*`, `body_creation_sec_*`, `source_creation_sec_*`, `sys_dyn_sec_*`, `interactions_sec_*`, `catalog_filter_ranges_open`, `rnd_*`, `dm_enabled`, `dm_lensing`, `dm_visualization`, `dm_affects_bodies`, `dm_lensing_strength`, `dm_visual_intensity`, `dm_step_fraction`, `dm_count`, `dm_vis_mode`, `dm_pref_*`, `dm_h<i>_*` | Celestial body catalog view options, search filters, hierarchy tree state, 3D body lighting parameters (7 source modes, attenuation, softness, specular), inter-body emission lighting, shadow caster limits, CollapsingHeader fold states, procedural randomizer bounds, and multi-halo dark matter field settings (32 halo slots, profiles, orientations, orbits, and UI preferences). |
| `capture.cfg` | `screenshot_output_directory`, `screenshot_filename_pattern`, `screenshot_format`, `screenshot_watermark_enabled`, `screenshot_watermark_text`, `screenshot_overwrite_policy` | Offline screenshot output path, naming template, image codec, watermark, and collision overwrite policy. |
| `cluster.cfg` | `cluster_enabled`, `cluster_node_role`, `cluster_listen_port`, `cluster_discovery_multicast_enabled`, `cluster_dynamic_load_balancing`, `cluster_latency_penalty_alpha`, `cluster_work_stealing_enabled`, `cluster_tile_timeout_ms`, `cluster_compression_mode`, `cluster_require_encryption`, `cluster_shared_token_hex`, `cluster_route_interactive_viewport`, `cluster_route_capture_studio`, `cluster_route_monte_carlo_ensembles`, `cluster_route_nbody_interactions`, `cluster_route_exact_spacetime_metrics` | Distributed cluster network transport, multicast discovery, load balancing parameters, AEAD encryption credentials, and workload task routing matrix. |

**Crash guard & Diagnostics.** `config/.session_active` is created when the session starts and removed upon a clean shutdown. If this marker file exists at startup (indicating an unexpected crash or abort during the preceding run), or if `load_policy` is set to 0 (`AlwaysResetToDefaults`), all settings are restored to clean defaults except the configured screenshot output directory.

When an unhandled exception, fatal signal, CRT parameter failure, or watchdog hang occurs, a forensic report directory is generated under `<root>/crashes/`:
`crash_<YYYY-MM-DD_HH-MM-SS>_<code|signal>_pid<pid>_n<index>/`
Live snapshots triggered via the console `snapshot` command generate directories named `snapshot_<timestamp>_pid<pid>_n<index>/`, and watchdog stalls generate `hang_<timestamp>_pid<pid>_n<index>/`.

| File | Content |
| :--- | :--- |
| `report.txt` | Complete diagnostic text report: exception code/signal, fault address, faulting module and offset, CPU registers dump (RAX-R15, RIP, RSP, RBP, EFLAGS, MXCSR on x86-64), instruction bytes at RIP, stack words at RSP, symbol-resolved backtraces with file names and line numbers, target memory region descriptors (`VirtualQuery`), heuristic module classification, automatic diagnosis hints, thread activity table with per-thread durations and names, milestones, command journal, warnings/errors problem ring, double-buffered state snapshots (scene, render, startup, Vulkan, OpenGL), CPUID hardware features, memory statistics, and loaded module base address maps. |
| `minidump.dmp` | Full-fidelity process minidump (`MiniDumpWithDataSegs \| MiniDumpWithThreadInfo \| MiniDumpWithUnloadedModules \| MiniDumpWithIndirectlyReferencedMemory \| MiniDumpWithHandleData`), readable with WinDbg or Visual Studio. |
| `session.log` | Tail of the session log up to 16 MiB written before the crash. |
| `preferences/` | Complete copy of all active modular configuration files from `config/preferences/`. |
| `scenario.yaml` | Active YAML scenario file attached to the report (up to 4 MiB). |

The directory path of the crash folder is stored in `<root>/last_crash.txt`. On subsequent launch, `DiagnosticsSession` detects this file, logs a warning, sets environment variable `previous.crash.folder`, and renames the file to `last_crash_acknowledged.txt`. `DiagnosticsLocator` automatically prunes logs and crash reports upon session start, retaining at most 40 session logs and 25 crash report folders based on modification timestamps.

### 10.2. `config/performance_profiler.cfg`

Written by `PerformanceProfiler::save_to_disk` (`include/relativistic/orchestrator/performance_profiler.hpp`) when persistence is enabled. The header contains `format_version=1` and `history_capacity`. Each benchmark run is enclosed by the lines `[run]` and `[/run]` and contains:

| Keys | Content |
| :--- | :--- |
| `label`, `timestamp`, `engine_signature` | Run identification; the signature is described in [ARCHITECTURE.md](ARCHITECTURE.md#211-profiling-logging-and-persistence) |
| `cfg_metric`, `cfg_integrator`, `cfg_res_scale`, `cfg_ray_steps`, `cfg_precision`, `cfg_preset`, `cfg_gpu`, `cfg_tiled`, `cfg_simd`, `cfg_width`, `cfg_height`, `cfg_step_controller`, `cfg_motion_quality_mode`, `cfg_motion_quality_scale`, `cfg_space_skip_enabled`, `cfg_space_skip_radius`, `cfg_pole_guard_precision`, `cfg_far_field_step_scale`, `cfg_lod_enabled`, `cfg_lod_distance_scale`, `cfg_lod_reduced_steps`, `cfg_render_distance_scale`, `cfg_interlace_enabled`, `cfg_dynamic_res_enabled`, `cfg_dynamic_res_target_fps`, `cfg_adaptive_tile_prepass`, `cfg_rolling_average_count`, `cfg_integration_rtol`, `cfg_integration_atol` | Configuration at the end of the capture |
| `ft_mean`, `ft_median`, `ft_min`, `ft_max`, `ft_std`, `ft_p95`, `ft_p99` | Frame time statistics in milliseconds |
| `fps_mean`, `fps_min`, `fps_max`, `throughput_mean`, `throughput_min`, `throughput_max` | Frame rate (FPS) and throughput (megapixels/s) statistics |
| `stage_mean_<idx>` | Mean execution time in milliseconds for each task stage across the benchmark capture window |
| `avg_iterations`, `gpu_ratio`, `horizon_ratio`, `celestial_ratio`, `disk_ratio`, `sample_count`, `duration` | Mean ray iterations, GPU path frame ratio, ray classification ratios, sample count and capture duration in seconds |

### 10.3. `config/distributed_cluster.cfg`

Written and read by `ClusterConfiguration` (`include/relativistic/distributed/cluster_config.hpp`), format version 1. Configuration file governing network clustering, node topologies, cryptographic authentication, and load balancing parameters.

```ini
format_version=1

[cluster]
enabled=1
node_role=1
cluster_name=Relativistic_Alpha_Mesh
listen_interface=0.0.0.0
listen_port=42425
discovery_multicast_enabled=1
discovery_multicast_group=239.255.77.88
discovery_multicast_port=42424
heartbeat_interval_ms=500
node_timeout_ms=2500

[performance]
dynamic_load_balancing=1
latency_penalty_alpha=1.0000000000000000
work_stealing_enabled=1
tile_timeout_ms=120
max_inflight_frames=2
compression_mode=2
lz4_acceleration_level=1
auto_benchmark_nodes=1

[security]
require_encryption=1
cipher_suite=1
tls_certificate_path=config/certs/node.crt
tls_private_key_path=config/certs/node.key
tls_ca_chain_path=config/certs/ca_authority.crt
cluster_shared_token=a8f5c2d3e9b14708215649fbcde71024385960abcedf1234567890abcdef1234
allow_unauthenticated_lan=0
sanitize_remote_data=1

[task_routing]
route_interactive_viewport=1
route_capture_studio=1
route_monte_carlo_ensembles=1
route_nbody_interactions=0
route_exact_spacetime_metrics=1

[node_1]
enabled=1
name=Local_Workstation_GPU
address=127.0.0.1
port=42425
static_weight=2.0000000000000000
preferred_device=0
prefer_fp64=1
```

#### Detailed Configuration Parameters

| Section | Key | Type | Description |
| :--- | :--- | :--- | :--- |
| `cluster` | `enabled` | Boolean (`0`/`1`) | Master switch for the distributed compute subsystem. When set to `0`, no sockets, threads, or memory allocations occur. |
| `cluster` | `node_role` | Integer (`0`-`3`) | Operational role: `0`: Standalone, `1`: Master (orchestrator), `2`: Worker (compute node), `3`: Hybrid (participating master). |
| `cluster` | `cluster_name` | String | Cluster namespace tag. Discovery beacons from mismatched namespaces are ignored. |
| `cluster` | `listen_interface` | String | Local IPv4 address to bind the listening UDP socket (`0.0.0.0` binds all interfaces). |
| `cluster` | `listen_port` | Integer (`1024`-`65535`) | Listening port for inbound peer control and task datagrams (default `42425`). |
| `cluster` | `discovery_multicast_enabled` | Boolean (`0`/`1`) | Enables periodic UDP multicast presence beacons. |
| `cluster` | `discovery_multicast_group` | String | IPv4 Class D multicast group address (default `239.255.77.88`). |
| `cluster` | `discovery_multicast_port` | Integer (`1024`-`65535`) | Multicast destination port (default `42424`). |
| `cluster` | `heartbeat_interval_ms` | Integer (`50`-`5000`) | Period between heartbeat ping datagrams in milliseconds. |
| `cluster` | `node_timeout_ms` | Integer (`500`-`60000`) | Inactivity threshold before a silent node transitions to `Disconnected`. |
| `performance` | `dynamic_load_balancing` | Boolean (`0`/`1`) | Automatically scales tile quotas proportionally to measured throughput and latency. |
| `performance` | `latency_penalty_alpha` | Float (`0.0`-`4.0`) | Damping exponent $\alpha$ penalizing high-latency nodes via $(\text{RTT})^{-\alpha}$. |
| `performance` | `work_stealing_enabled` | Boolean (`0`/`1`) | Speculatively reclaims and executes expired remote tiles locally without stalling the frame. |
| `performance` | `tile_timeout_ms` | Integer (`4`-`5000`) | Upper bound on tile return deadlines before work-stealing triggers. |
| `performance` | `max_inflight_frames` | Integer (`1`-`4`) | Maximum number of concurrent pipelined frames dispatched to saturate network bandwidth. |
| `performance` | `compression_mode` | Integer (`0`-`2`) | Tile compression codec: `0`: DirectRaw, `1`: QoiHdrRle, `2`: Lz4Block. |
| `performance` | `lz4_acceleration_level`| Integer (`1`-`16`) | Compression speed factor for the LZ4 block codec. |
| `performance` | `auto_benchmark_nodes` | Boolean (`0`/`1`) | Evaluates node throughput on standard ray bundles during initial handshake. |
| `security` | `require_encryption` | Boolean (`0`/`1`) | Enforces authenticated ChaCha20-Poly1305 encryption on all packet envelopes. |
| `security` | `cipher_suite` | Integer (`0`-`1`) | `0`: Plaintext (closed LAN), `1`: ChaCha20-Poly1305 AEAD. |
| `security` | `cluster_shared_token` | Hex String (64 chars) | Pre-shared 256-bit symmetric cluster secret for forward-secret key exchange. |
| `security` | `allow_unauthenticated_lan` | Boolean (`0`/`1`) | Allows unauthenticated peers on local subnets when encryption is disabled. |
| `security` | `sanitize_remote_data` | Boolean (`0`/`1`) | SIMD vector validation checking IEEE 754 finiteness and clamping radiance values. |
| `task_routing` | `route_interactive_viewport` | Boolean (`0`/`1`) | Routes live viewport screen tiles to connected cluster workers. |
| `task_routing` | `route_capture_studio` | Boolean (`0`/`1`) | Routes capture studio sequence bands and super-sampled frames. |
| `task_routing` | `route_monte_carlo_ensembles`| Boolean (`0`/`1`) | Routes stochastic geodesic bundle sample batches. |
| `task_routing` | `route_nbody_interactions` | Boolean (`0`/`1`) | Routes post-Newtonian pairwise acceleration matrix row blocks. |
| `task_routing` | `route_exact_spacetime_metrics` | Boolean (`0`/`1`) | Routes compute-intensive rotating or charged spacetime geodesics. |
| `node_N` | `enabled` | Boolean (`0`/`1`) | Activates connection management for this static node entry. |
| `node_N` | `name` | String | Identifier tag for the node. |
| `node_N` | `address` | String | IPv4 address or hostname of the remote worker node. |
| `node_N` | `port` | Integer (`1024`-`65535`) | Port accepting connections on the remote worker node. |
| `node_N` | `static_weight` | Float ($> 0.0$) | Capacity multiplier weighting the node's quota in the load-balancer formulation. |
| `node_N` | `prefer_fp64` | Boolean (`0`/`1`) | Requests native FP64 precision from the worker GPU. |

---

## 11. Distributed Cluster Binary Network Protocol (Wire Format)

The distributed computing subsystem utilizes an explicit, architecture-independent binary wire protocol to coordinate work between cluster coordinators and worker daemons. The protocol enforces deterministic 16-byte structure alignment, host-independent little-endian byte ordering, and zero-copy deserialization across network packet buffers.

### 11.1. Network Transport Framing and Packet Envelope Structure

All control and data datagrams transmitted across the cluster network begin with a fixed 48-byte header termed the `PacketEnvelope`. The envelope layout is designed to eliminate compiler-dependent structure padding, ensuring identical in-memory representations across differing 64-bit operating systems and CPU architectures.

The first 4 bytes of the envelope contain the synchronization constant `0x52454C44`, corresponding to the ASCII characters `RELD` (Relativistic Engine Link Datagram). Sockets reject any received datagram whose initial four bytes do not match this identifier. The subsequent 2 bytes store the protocol version as an unsigned 16-bit integer, initialized to `1` (`kProtocolVersion`). Packets with mismatching version numbers are dropped during header parsing. Following the version, 2 bytes encode the functional packet opcode as an unsigned 16-bit enumeration (`PacketType`). The session identifier field occupies the next 4 bytes (`uint32_t`), assigned during the cryptographic handshake to isolate concurrent cluster sessions. The packet sequence number occupies 4 bytes (`uint32_t`), incrementing monotonically per session to drive anti-replay verification windows and duplicate detection. The frame index occupies 4 bytes (`uint32_t`), binding the datagram to a specific physical simulation or rendering frame. The payload length occupies 4 bytes (`uint32_t`), specifying the byte count of the contiguous variable payload immediately following the envelope. The payload CRC32 checksum occupies 4 bytes (`uint32_t`), computed via standard IEEE 802.3 polynomial `0xEDB88320` over the unencrypted payload bytes (`0` when the payload is empty). These 28 bytes constitute the authenticated header (`kHeaderSize = 28`) passed as Authenticated Associated Data (AAD) to the AEAD cipher. The subsequent 16 bytes contain the Poly1305 message authentication code tag (`authentication_tag`). Because the structure declares 16-byte alignment (`alignas(16)`), 4 bytes of compiler padding follow the authentication tag, establishing a total structure size of exactly 48 bytes (`kEnvelopeSize = 48`).

| Offset (Bytes) | Field Name | Data Type | Description |
| :--- | :--- | :--- | :--- |
| 0–3 | `magic_identifier` | `uint32_t` | Magic constant `0x52454C44` (`RELD`). |
| 4–5 | `protocol_version` | `uint16_t` | Wire protocol version number (currently 1). |
| 6–7 | `type` | `PacketType` (`uint16_t`) | Functional message opcode enumeration. |
| 8–11 | `session_identifier` | `uint32_t` | Session identification token generated during pairing. |
| 12–15 | `packet_sequence` | `uint32_t` | Monotonically increasing transmission sequence number. |
| 16–19 | `frame_index` | `uint32_t` | Target logical frame sequence index. |
| 20–23 | `payload_bytes` | `uint32_t` | Length of appended variable payload in bytes. |
| 24–27 | `payload_crc32` | `uint32_t` | IEEE 802.3 CRC32 checksum over the payload. |
| 28–43 | `authentication_tag` | `uint8_t[16]` | Poly1305 message authentication code tag over AAD and ciphertext. |
| 44–47 | `padding` | `uint8_t[4]` | Alignment padding enforcing 16-byte boundary (`alignas(16)`). |

### 11.2. Packet Types and Functional Channel Multiplexing

The network transport layer uses ENet to multiplex datagrams over UDP using two dedicated channels, separating latency-sensitive control traffic from high-bandwidth computation payloads.

Channel 0 operates as the sequenced reliable control channel. This channel guarantees in-order delivery and automatic packet retransmission for state-critical coordination messages. Handshake initiation (`HandshakeSyn`), handshake confirmation (`HandshakeAck`), heartbeat pings (`HeartbeatPing`), heartbeat replies (`HeartbeatPong`), task cancellations (`TaskCancellation`), and graceful disconnection notices (`NodeDisconnect`) are restricted to Channel 0.

Channel 1 operates as the unsequenced, unreliable-fragmented data channel. Workload assignments (`TaskAssignment`) and computational return results (`TileResultData`, `NBodyResultData`, `EnsembleResultData`, `PceResultData`) are routed through Channel 1. Because image tiles and row reductions execute independently, Channel 1 allows datagram fragmentation up to 256 KiB while eliminating head-of-line blocking stalls that would occur if dropped packets held back independent tiles.

| Type Identifier | Enumeration Name | Logical Channel | Delivery Policy | Functional Scope |
| :--- | :--- | :--- | :--- | :--- |
| `0x0001` | `HeartbeatPing` | Channel 0 | Reliable | Keep-alive probe timestamped in coordinator seconds. |
| `0x0002` | `HeartbeatPong` | Channel 0 | Reliable | Echoed keep-alive probe response measuring network RTT. |
| `0x0010` | `NodeAuthentication` | Channel 0 | Reliable | Plaintext transmission of client's 32-byte X25519 public key. |
| `0x0011` | `NodeAuthChallenge` | Channel 0 | Reliable | Plaintext transmission of server's 32-byte X25519 public key. |
| `0x0012` | `NodeAuthResponse` | Channel 0 | Reliable | Encrypted proof payload (`AuthenticationProofPayload`, 16 bytes). |
| `0x0020` | `CapabilitiesAdvertisement` | Channel 0 | Reliable | Hardware feature flags, CPU core count, and GPU capabilities. |
| `0x0030` | `SynchronizeClock` | Channel 0 | Reliable | Four-timestamp NTP-style clock offset estimation. |
| `0x0040` | `UpdateConstants` | Channel 0 | Reliable | Physical constants engine synchronization across nodes. |
| `0x0050` | `BodyCatalogChunk` | Channel 0 | Reliable | Chunked celestial body states for scene reassembly. |
| `0x0100` | `TaskAssignment` | Channel 1 | Unsequenced | Screen tile work unit parameters and camera push constants. |
| `0x0101` | `TaskCancel` | Channel 0 | Reliable | Cooperative abort signal terminating superseded frames or tiles. |
| `0x0200` | `TileResultData` | Channel 1 | Unsequenced | Completed ray-traced screen tile pixel buffer and timing. |
| `0x0300` | `SimulationSubStepData` | Channel 0/1 | Mixed | Chunked N-body initial state vectors, block tasks, and acceleration row reductions. |
| `0x0400` | `MonteCarloBatchData` | Channel 0/1 | Mixed | Geodesic ensemble packet tasks/partials and gPCE stochastic quadrature evaluations. |
| `0x0FFF` | `NodeDisconnect` | Channel 0 | Reliable | Clean session termination and peer deregistration notice. |

### 11.3. Cryptographic Encapsulation and Anti-Replay Processing

Network security is established via Authenticated Encryption with Associated Data (AEAD) combining the ChaCha20 stream cipher with the Poly1305 authenticator in compliance with RFC 8439.

During initial node discovery and pairing, peers exchange 32-byte X25519 public keys derived from local random seeds. Each peer computes the 32-byte shared Diffie-Hellman secret. This shared secret is passed through HKDF-SHA256 along with the cluster pre-shared authentication secret and session salt to derive symmetric encryption keys.

When establishing a peer connection, nodes generate ephemeral X25519 key pairs using PCG64 random state clamped to standard scalar requirements ($k_0 \mathbin{\&}= 248$, $k_{31} \mathbin{\&}= 127$, $k_{31} \mathbin{|}= 64$). A 32-byte shared Diffie-Hellman secret is evaluated via `x25519_scalarmult` against the peer's public key. The shared secret is passed into HKDF-SHA256 with the pre-shared cluster token as salt and domain info `"RelativisticEngineV1"`, deriving 64 bytes of key material:
- Server: bytes 0–31 form `transmission_key`, bytes 32–63 form `reception_key`.
- Client: bytes 0–31 form `reception_key`, bytes 32–63 form `transmission_key`.

When sealing a packet for transmission, the first 28 bytes of `PacketEnvelope`—encompassing `magic_identifier`, `protocol_version`, `type`, `session_identifier`, `packet_sequence`, `frame_index`, `payload_bytes`, and `payload_crc32`—serve as the Authenticated Associated Data (AAD). A 96-bit nonce is constructed from three 32-bit words:
$$\text{nonce}_0 = \text{session\_epoch}, \quad \text{nonce}_1 = \text{packet\_sequence} \mathbin{\&} \text{0xFFFFFFFF}, \quad \text{nonce}_2 = \text{packet\_sequence} \gg 32$$

ChaCha20 block 0 is generated with the transmission key and nonce; the first 32 bytes of this block provide the one-time key for Poly1305. Successive ChaCha20 blocks ($1, 2, \dots$) encrypt the plaintext payload. Poly1305 computes the MAC tag over the 28-byte AAD header, the ciphertext buffer, and a 16-byte length block storing the 64-bit little-endian lengths of AAD and ciphertext in compliance with RFC 8439. The resulting 16-byte authentication tag is written to bytes 28 through 43 of `PacketEnvelope`.

Upon packet reception, the receiver verifies the Poly1305 tag in constant time before attempting decryption. A failure increments `integrity_failures`; exceeding 32 consecutive failures triggers session termination (`DisconnectReason::AuthenticationFailed`).

Anti-replay protection maintains a 64-packet bitmask window tracking the highest sequence number observed (`receive_sequence`). If an incoming sequence exceeds the window boundary, the bitmask shifts left by the difference and sets bit 0. Packets within the 64-packet window are validated against the bitmask; duplicates are rejected. Packets falling behind the window are discarded. Re-keying is enforced when transmission sequence counts reach $2^{24}$ packets or 3600 seconds elapse (`DisconnectReason::RekeyRequired`).

### 11.4. Task and Result Payload Binary Layouts

#### Screen Tile Render Task Payload (`RenderTileTaskPayload`)

Screen tile tasks instruct worker nodes to integrate a designated spatial block of null geodesics. The payload has a fixed size of 1832 bytes (`kRenderTileTaskPayloadBytes`):
- `header` (`RenderTileTaskHeader`, 16 bytes packed): `tile_index` (uint32_t), `tile_x` (uint16_t, grid column origin), `tile_y` (uint16_t, grid row origin), `tile_width` (uint16_t, up to 32), `tile_height` (uint16_t, up to 32), `lod_reduced_steps` (uint16_t), `flags` (uint8_t bitmask: `kTileTaskFlagForceCpu = 1`, `kTileTaskFlagNoBodies = 2`, `kTileTaskFlagReuseLatestBodies = 4`), and `compression_type` (uint8_t: `0` DirectRaw, `1` QoiHdrRle, `2` Lz4Block).
- `dispatch_timestamp` (`double`, 8 bytes): coordinator monotonic timestamp in seconds.
- `camera_constants` (`Render::GpuCameraPushConstants`, 1808 bytes): observer four-position, comobile tetrad vectors, metric parameters, horizon radii, step bounds, accretion disk settings, and procedural skybox parameters.

#### Celestial Body Catalog Chunk Payload (`BodyCatalogChunkHeader`)

Celestial body states are transmitted in advance over Channel 0 via `BodyCatalogChunk` (0x0050) packets to eliminate redundant transmission inside tile tasks:
- `header` (`BodyCatalogChunkHeader`, 24 bytes packed): `frame_index` (uint64_t), `total_bodies` (uint32_t), `first_index` (uint32_t), `chunk_bodies` (uint32_t), `reserved` (uint32_t).
- Appended array: `chunk_bodies` contiguous instances of `Render::GpuBodyData` (704 bytes each), up to `kBodiesPerCatalogChunk` bodies per packet. Assembled by `BodyCatalogStore` across frames and retained up to `kCatalogRetainedFrames = 4`.

#### Screen Tile Render Result Payload (`RenderTileResultPayload`)

The render result payload returns computed pixel radiances to the coordinator:
- `header` (`RenderTileResultHeader`, 24 bytes packed): `tile_index` (uint32_t), `frame_index` (uint64_t), `pixel_count` (uint16_t, up to 1024), `compression_type` (uint8_t), `reserved` (uint8_t), `uncompressed_bytes` (uint32_t, equal to `pixel_count * 32`), and `execution_time_us` (uint32_t, microsecond kernel duration).
- `compressed_bytes` (`uint32_t`, 4 bytes): size of the trailing compressed or raw pixel byte buffer. Total header prefix is 28 bytes.

#### Post-Newtonian N-Body Compute Payloads

Distributed N-body dynamics utilize three payload structures:
1. `NBodyStateChunkHeader` (32 bytes packed): `base` (`ComputeMessageHeader`, 16 bytes: `kind = 1` StateChunk, `job_id`, `unit_index = 0`), `total_bodies` (uint32_t), `first_index` (uint32_t), `chunk_bodies` (uint32_t, up to 512), `reserved` (uint32_t). Followed by `chunk_bodies` instances of `NBodyBodyState` (88 bytes each: position[3], velocity[3], mass, acceleration[3], potential).
2. `NBodyBlockTask` (48 bytes packed): `base` (`kind = 2` BlockTask), `first_row` (uint32_t), `row_count` (uint32_t, up to 4096), `total_bodies` (uint32_t), `phase` (uint32_t: `0` Newtonian, `1` PostNewtonian), `gravitational_constant` (double), `speed_of_light` (double).
3. `BlockResult` Payload: `ComputeMessageHeader` (16 bytes, `kind = 3` BlockResult) followed by `row_count` instances of `NBodyRowResult` (32 bytes each: acceleration[3], potential). Evaluated on GPU via `GpuNBodyExecutor` running `nbody_accel_fp64.comp` or multi-threaded CPU SIMD.

#### Monte Carlo Geodesic Uncertainty Ensemble Payloads

Stochastic photon packet sampling distributes trajectory bunches across workers:
1. `GeodesicEnsembleTask` (120 bytes packed): `base` (16 bytes, `kind = 2` BlockTask), `spec` (`GeodesicEnsembleSpec`, 88 bytes: mass, observer_radius, escape_radius, launch_angle_rad, angular_sigma_rad, step_size_factor, min_step_size, max_step_size, seed, total_samples, max_steps), `first_sample` (uint64_t), `sample_count` (uint32_t, up to $2^{20}$), `reserved` (uint32_t).
2. `BlockResult` Payload: `ComputeMessageHeader` (16 bytes) followed by `GeodesicEnsemblePartial` (120 bytes packed: `samples`, `absorbed`, `escaped`, `saturated`, `mean_minimum_radius`, `m2_minimum_radius`, `mean_steps`, `m2_steps`, `bounds_min[3]`, `bounds_max[3]`, `has_bounds`). Online moment reduction uses the Chan-Golub-LeVeque algorithm.

#### Generalized Polynomial Chaos Expansion (gPCE) Quadrature Payloads

Stochastic spectral quadrature evaluations partition multi-dimensional tensor-grid nodes:
1. `StochasticQuadratureTask` (128 bytes packed): `base` (16 bytes, `kind = 2`), `spec` (`StochasticEvaluationSpec`, 96 bytes: dimension, polynomial_order, total_quadrature_points, evaluation_mode, parameter_means[4], parameter_sigmas[4], integration_step, max_steps), `first_point` (uint32_t), `point_count` (uint32_t), `reserved` (uint32_t).
2. `BlockResult` Payload: `ComputeMessageHeader` (16 bytes) followed by `point_count` instances of `StochasticQuadratureResult` (24 bytes each: `point_index`, `status`, `response_value`, `secondary_value`).

### 11.5. Compressed Tile Wire Encapsulation

Pixel output arrays returned by worker nodes are compressed to minimize network transmission latency and conserve network switch bandwidth. The payload envelope supports three selectable compression modes governed by `TileCompressionMode`.

#### DirectRaw Mode (`TileCompressionMode::DirectRaw`)

In DirectRaw mode (codec identifier 0), pixel data undergoes zero transformation or compression. Contiguous arrays of 32-byte `GpuPixelOutput` structures are written directly into the wire payload buffer.

Each pixel is represented by eight 32-bit words: IEEE 754 single-precision floating-point values for red, green, blue, alpha, gravitational redshift, and affine parameter, followed by an unsigned 32-bit status bitmask (`PixelFlags`), and an unsigned 32-bit iteration counter.

A standard $32 \times 32$ tile yields exactly 32768 bytes of uncompressed payload. DirectRaw mode incurs zero CPU encoding or decoding overhead, making it the preferred mode in low-latency 10-Gigabit Ethernet local area networks.

#### QOI-HDR RLE Mode (`TileCompressionMode::QoiHdrRle`)

QOI-HDR RLE mode (codec identifier 1) provides a specialized spectral compression format for floating-point colorimetry:
- Pixel Quantization: 32-bit floats for red, green, blue, and alpha are converted to IEEE 754 binary16 half-precision values via `HalfFloat::to_bits` (1 sign bit, 5 exponent bits, 10 mantissa bits with round-to-nearest-even tie breaking). Gravitational redshift and affine parameter preserve full 32-bit IEEE float representation via `std::bit_cast<uint32_t>`. Status flags and iteration counters are retained as 32-bit unsigned integers.
- State Matching & Hash Table: Encoder and decoder maintain a 64-entry reference table `table` indexed by hash:
  $$\text{hash}(P) = \left( 3 r_{16} + 5 g_{16} + 7 b_{16} + 11 a_{16} + 13 z_{32} + 17 \lambda_{32} + 19 s_{32} + 23 i_{32} \right) \bmod 64$$
- Run-Length Tokens: Consecutive identical quantized pixels emit RLE run tokens:
  - Short runs ($1 \le L \le 64$): tagged with `0x40 | (L - 1)`.
  - Long runs ($65 \le L \le 16448$): 2-byte token tagged with `0xC0 | ((L - 65) >> 8)` followed by `(L - 65) & 0xFF`.
- Hash Table Reference: When the current pixel matches the hash table entry, emits `0x00 | slot`.
- Partial Delta Updates: When differing from the previous pixel, emits a variable-length token tagged with `0x80 | mask`, where `mask` is a 6-bit field difference bitmask:
  - Bit 0 (`kGroupRgb`): emits 6 bytes (three `uint16_t` half-floats for R, G, B).
  - Bit 1 (`kGroupAlpha`): emits 2 bytes (`uint16_t` half-float for A).
  - Bit 2 (`kGroupRedshift`): emits 4 bytes (`uint32_t` redshift float bits).
  - Bit 3 (`kGroupAffine`): emits 4 bytes (`uint32_t` affine float bits).
  - Bit 4 (`kGroupStatus`): emits 4 bytes (`uint32_t` status flags).
  - Bit 5 (`kGroupIterations`): emits 4 bytes (`uint32_t` iterations used).
- Initial state is initialized to black unshifted pixel with unit alpha (`initial_pixel()`).

#### LZ4Block Mode (`TileCompressionMode::Lz4Block`)

LZ4Block mode (codec identifier 2) provides lossless block compression using `LZ4_compress_fast`:
- Direct Memory Compression: The contiguous array of 32-byte `GpuPixelOutput` structures is compressed directly with configurable acceleration level (`lz4_acceleration_level`, default 1).
- DirectRaw Fallback: When the compressed byte length equals or exceeds raw uncompressed bytes ($P \times 32$), or when `lz4_acceleration >= 8` with insufficient output space, the encoder falls back to uncompressed `DirectRaw` mode, setting `mode_used = DirectRaw`.
- High-Throughput Decompression: Decompression invokes `LZ4_decompress_safe` into a thread-local 32 KiB scratch buffer (`TileCodecScratch`), verifying that inflated bytes match `pixel_count * sizeof(GpuPixelOutput)`.

#### SIMD Vector Data Sanitization Protocol

Prior to assembling remote tile results into the master display framebuffer, all uncompressed pixel streams pass through the `TileSanitizer`.

The sanitizer executes an AVX2 or ARM Neon vectorized sweep inspecting every pixel payload. Any pixel containing IEEE 754 NaN (Not a Number), positive infinity, negative infinity, or unphysical negative radiances is flagged as corrupted.

Corrupted pixels have their color components neutralized to safe zero radiance and their status flags tagged with an error bit. Alpha values are clamped to 1.0, and non-finite redshifts are reset to neutral zero shift.

Valid radiances are clamped to an upper physical ceiling of $1.0 \times 10^8\,\text{W/m}^2/\text{sr}$ to prevent numerical overflow in subsequent bloom and HDR tonemapping passes.

The sanitizer generates a `SanitizationReport` tracking the count of neutralized pixels, clamped radiances, and repaired metadata values. If anomalous values are detected, a diagnostic problem entry is recorded in the coordinator registry without stalling the interactive frame.

### 11.6. Multicast Peer Discovery Beacon Wire Protocol

Autonomous cluster discovery utilizes periodic UDP multicast datagrams sent to destination `239.255.77.88:42424` (or configured group/port) using the packed 72-byte `DiscoveryBeacon` structure:

| Offset (Bytes) | Field Name | Data Type | Description |
| :--- | :--- | :--- | :--- |
| 0–3 | `magic_identifier` | `uint32_t` | Constant synchronization identifier `0x52444953` (`RDIS`). |
| 4–5 | `protocol_version` | `uint16_t` | Discovery wire protocol version (`1`). |
| 6 | `kind` | `uint8_t` | Message type: `1` Announce, `2` Reply, `3` Goodbye. |
| 7 | `role` | `uint8_t` | Sender operational role: `1` Master, `2` Worker, `3` Hybrid. |
| 8–15 | `instance_identifier` | `uint64_t` | Monotonic random node instance identifier. |
| 16–23 | `cluster_tag` | `uint8_t[8]` | 8-byte domain-separated SHA-256 namespace authentication tag. |
| 24–27 | `abi_signature` | `uint32_t` | Local engine ABI signature derived from `EngineSignature::compute`. |
| 28–29 | `service_port` | `uint16_t` | ENet UDP service port accepting cluster connections. |
| 30–31 | `reserved` | `uint16_t` | Explicit zero alignment padding. |
| 32–35 | `performance_weight` | `float` | Advertised relative performance capacity multiplier. |
| 36–39 | `payload_crc32` | `uint32_t` | IEEE 802.3 CRC32 checksum over the beacon with this field set to 0. |
| 40–71 | `node_name` | `char[32]` | Null-terminated UTF-8 host or worker display name. |

Cluster Tag Derivation: The 8-byte `cluster_tag` verifies namespace matching without disclosing the cluster secret over multicast:
$$\text{tag} = \text{SHA256}\Big( \text{"RELATIVISTIC-DISCOVERY-V1"} \mathbin{\Vert} \text{0x00} \mathbin{\Vert} \text{cluster\_name} \mathbin{\Vert} \text{0x00} \mathbin{\Vert} \text{cluster\_token} \Big)_{0..7}$$

Receivers ignore beacons from identical `instance_identifier` (self-reflection) or mismatching `cluster_tag`. When an `Announce` beacon is accepted from a complementary peer (`Master` $\leftrightarrow$ `Worker`), the receiver replies with a direct unicast `Reply` beacon. Clean shutdown transmits a `Goodbye` beacon, pruning the peer from discovery registries.

# Relativistic Engine - Command Line Interface & REPL Reference

## 1. Executable Binaries

The engine provides three standalone executables:
1. `engine_cli`: Master simulation host containing the non-blocking command interpreter & the graphical multi-window instrumentation workspace.
2. `headless_exporter`: Non-interactive batch processor for cluster execution, parameter sweeps, & validation benchmarks.
3. `cluster_worker`: Lightweight headless compute node daemon for network cluster rendering and parallel physical evaluations.

---

## 2. Interactive Terminal Commands (REPL)

In interactive mode, the engine provides the non-blocking master prompt:
`relativistic> `

Commands can be supplied directly via standard input or queued asynchronously through the inter-thread command bus.

### 2.1. Simulation Flow & State Commands

| Command | Arguments | Description | Example |
| :--- | :--- | :--- | :--- |
| `pause` | None | Halts the progression of logical simulation time. | `pause` |
| `resume` | None | Resumes continuous simulation progression. | `resume` |
| `step` | `[N]` | Advances the simulation by exactly `N` ticks (default: 1) & pauses. | `step 10` |
| `warp` | `<factor>` | Sets the logical time dilation factor ($> 0.0$). | `warp 5.0` |
| `tickrate` | `<Hz>` | Adjusts the scheduler logical frequency (10.0 to 1000.0 Hz). | `tickrate 120.0` |
| `reset` | None | Resets simulation time to zero & restores initial conditions. | `reset` |
| `status` | None | Prints the current state of the scheduler & active parameters. | `status` |
| `quit`, `exit`, `shutdown` | None | Terminates all worker threads & exits the application. | `quit` |

### 2.2. Camera & Observer Navigation Commands

| Command | Arguments | Description | Example |
| :--- | :--- | :--- | :--- |
| `camera reset` | None | Restores the default observer position and orientation. | `camera reset` |
| `camera fov` | `<degrees>` | Sets the camera field of view in degrees (5.0 to 175.0). | `camera fov 65.0` |
| `camera speed` | `<value>` | Sets the translation speed of the observer ($> 0.0$). | `camera speed 25.0` |
| `camera move` | `<dx> <dy> <dz>` | Applies an instantaneous Cartesian displacement. | `camera move 0.0 5.0 0.0` |
| `camera rotate` | `<pitch> <yaw> [roll]` | Applies angular increments in degrees. | `camera rotate -15.0 45.0 0.0` |
| `camera teleport` | `<x> <y> <z>` | Displaces camera to Cartesian coordinates (aliases: `pos`, `position`). | `camera teleport 0.0 50.0 0.0` |
| `camera lookat` | `<x> <y> <z>` | Reorients camera gaze toward coordinates (aliases: `look_at`, `aim`). | `camera lookat 0.0 0.0 0.0` |
| `camera snap` | `<isco \| [distance]>` | Snaps camera to circular ISCO orbit or equatorial front view. | `camera snap isco` |
| `camera mode` | `<0..5 \| name>` | Switches navigation mode (`free`, `orbit`, `spherical`, `rocket`, `walk`, `planet`). | `camera mode walk` |
| `camera collision` | `<on \| off>` | Toggles obstacle avoidance preventing penetration into bodies and horizons. | `camera collision on` |
| `camera clearance` | `<value>` | Sets clearance distance kept between camera and obstacle surfaces. | `camera clearance 0.1` |
| `camera motion_refresh` | `<on \| off>` | Toggles real-time rendering refresh during camera motion in walk mode. | `camera motion_refresh on` |
| `camera walk` | `<start \| stop \| target <id>>` | Controls surface walker mode and assigns target celestial body ID. | `camera walk target 2` |
| `camera planet` | None | Switches navigation mode to simplified planet-orbiting spherical mode. | `camera planet` |

### 2.3. Spacetime Metric & Integrator Selection

| Command | Arguments | Description | Example |
| :--- | :--- | :--- | :--- |
| `metric` | `<name>` | Changes the active metric geometry. | `metric Kerr` |
| `integrator` | `<name>` | Selects the numerical differential equation solver. | `integrator Vernier9` |
| `load` | `<path>` | Ingests a declarative YAML scenario configuration. | `load scenarios/kerr_accretion_disk.yaml` |
| `save` | `<path>` | Serializes the active simulation state to a YAML file. | `save scenarios/snapshot.yaml` |
| `export` | `[format]` | Triggers file generation (`fits`, `hdf5`, `vtk`, `all`). | `export fits` |

Supported metric names: `FlatMinkowski` (alias `Minkowski`), `Schwarzschild`, `Kerr`, `KerrSchild`, `ReissnerNordstrom`, `KerrNewman`, `SchwarzschildDeSitter`, `FLRW`, `MorrisThorne`, `Alcubierre`, `BSSN`, `LemaitreTolmanBondi` (alias `Lemaitre`), `QuintessenceCosmology` (alias `Quintessence`), `ChaplyginCosmology` (alias `Chaplygin`), `EvaporatingBlackHole` (alias `Vaidya`, `Evaporating`), `EddingtonFinkelstein` (alias `Eddington`), `PainleveGullstrand` (alias `Painleve`), `TeVeS` (alias `Tensor-Vector`).

The `metric` command stores the name without validating it. Two resolvers consume it:

- `SimulationOrchestrator::metric_name_to_id` (`include/relativistic/orchestrator/simulation_orchestrator.hpp`) matches the exact names above and their aliases; any other string resolves to Schwarzschild ([Architecture](ARCHITECTURE.md#22-spacetime-metric-modules)).
- The viewport (`ViewportPrimaryWindow::get_metric_id_from_name` in `include/relativistic/ui/viewport_primary_window.hpp`) delegates directly to `SimulationOrchestrator::metric_name_to_id`. The display names used by the metric cycle action resolve correctly. Names without a match, including `BSSN`, are traced as Schwarzschild ([Technical Manual](TECHNICAL_MANUAL.md#metric-usage-in-softwarecomputeengine)).

Metric mathematical definitions are provided in [MATHEMATICAL_FORMULATION.md Section 1](MATHEMATICAL_FORMULATION.md#1-spacetime-metrics--line-elements). Scenario files accept both forms; see the validation rules in [FILE_FORMATS.md](FILE_FORMATS.md#12-parsing-validation-and-application-rules). Execution path selection by metric is described in [ARCHITECTURE.md](ARCHITECTURE.md#29-render-pipeline).

Supported integrator names: `RK45`, `CashKarp`, `Vernier9`, `GaussLegendre4`, `GaussLegendre6`, `Hermite4`.

The `integrator` command stores the name without validating it:
- `RK45`: Dormand-Prince 5(4) embedded adaptive Runge-Kutta scheme with FSAL evaluation, automatic radial time-step limits $0.5 r / |u^r|$ in spherical coordinates, and algebraic constraint projection $g_{\mu\nu} u^\mu u^\nu = \text{const}$ solving the quadratic polynomial for $u^0$ to machine precision ([MATHEMATICAL_FORMULATION.md Section 18.1](MATHEMATICAL_FORMULATION.md#181-embedded-adaptive-runge-kutta-tableaus-dormand-prince--cash-karp)).
- `CashKarp`: 6-stage embedded 5(4) Runge-Kutta scheme providing wide stability intervals for stiff curved geometries ([MATHEMATICAL_FORMULATION.md Section 18.1](MATHEMATICAL_FORMULATION.md#181-embedded-adaptive-runge-kutta-tableaus-dormand-prince--cash-karp)).
- `Vernier9`: High-order 16-stage 9(8) embedded Runge-Kutta scheme optimizing truncation error bounds for long orbital baselines ([MATHEMATICAL_FORMULATION.md Section 18.2](MATHEMATICAL_FORMULATION.md#182-vernier-98-high-order-16-stage-embedded-runge-kutta-scheme)).
- `GaussLegendre4`: 2-stage 4th-order implicit symplectic Runge-Kutta scheme with Picard fixed-point stage iterations to tolerance $10^{-14}$, preserving phase-space symplectic 2-forms and Killing invariants ([MATHEMATICAL_FORMULATION.md Section 18.3](MATHEMATICAL_FORMULATION.md#183-implicit-symplectic-gauss-legendre-integrators-orders-4-and-6)).
- `GaussLegendre6`: 3-stage 6th-order implicit symplectic Runge-Kutta scheme with Picard stage iterations to tolerance $10^{-14}$ ([MATHEMATICAL_FORMULATION.md Section 18.3](MATHEMATICAL_FORMULATION.md#183-implicit-symplectic-gauss-legendre-integrators-orders-4-and-6)).
- `Hermite4`: 4th-order predictor-corrector scheme evaluating acceleration $\mathbf{a}$, 4th-order centered finite difference jerk $\mathbf{j}$, snap $\mathbf{s}$, and crackle $\mathbf{c}$ with Aarseth variable timestep regulation $\Delta t \propto \sqrt{(\|\mathbf{a}\| \|\mathbf{s}\| + \|\mathbf{j}\|^2) / (\|\mathbf{j}\| \|\mathbf{c}\| + \|\mathbf{s}\|^2)}$ for close gravitational encounters ([MATHEMATICAL_FORMULATION.md Section 18.4](MATHEMATICAL_FORMULATION.md#184-hermite-4th-order-predictor-corrector-scheme--aarseth-adaptive-timestep)).

The N-body integration uses the symplectic Forest-Ruth post-Newtonian integrator when the name contains `Symplectic` or `Gauss`, and the fourth-order Runge-Kutta post-Newtonian integrator otherwise (see [ARCHITECTURE.md](ARCHITECTURE.md#34-orchestrator-state-advance) and [TECHNICAL_MANUAL.md](TECHNICAL_MANUAL.md#runtime-use-of-the-integrator-selection)). The integrator cycle action uses the display names `Dormand-Prince RK45 (Adaptive)`, `Cash-Karp 5(4) (Adaptive)`, `Vernier 9(8) High-Order`, `Symplectic Gauss-Legendre 4th`, `Symplectic Gauss-Legendre 6th`, and `Hermite 4th-Order (Aarseth)`.

### 2.4. Parameter Modification Commands (`set`)

The `set` command updates physical properties, optical settings, & solver tolerances:
`set <parameter> <value>`

| Parameter Key | Value Type | Physical / Numerical Meaning | Example |
| :--- | :--- | :--- | :--- |
| `mass` | Float | Central body mass $M$. | `set mass 1.0` |
| `spin` | Float | Central body spin parameter $a$. | `set spin 0.94` |
| `charge` | Float | Central body net electric charge $Q$. | `set charge 0.5` |
| `lambda` | Float | Cosmological constant $\Lambda$. | `set lambda 1.1e-52` |
| `throat` | Float | Morris-Thorne wormhole throat radius $b_0$. | `set throat 5.0` |
| `warp_velocity`, `warp_vel` | Float | Alcubierre metric apparent velocity $v_s$. | `set warp_velocity 2.0` |
| `projection`, `proj` | Integer | Projection (0: Pinhole, 1: AutoZoom, 2: FisheyeStereographic, 3: Equirectangular360, 4: FisheyeEquidistant, 5: FisheyeOrthographic, 6: PaniniCylindrical, 7: HammerAitoff; see [DESCRIPTION.md Section 8.3](DESCRIPTION.md#83-optical-projections--field-of-view)). The default is 3. | `set projection 1` |
| `timeflow`, `time_flow` | Integer | Time frame (0: Proper time $\tau$, 1: Coordinate time $t$). | `set timeflow 0` |
| `speed`, `cameran_speed` | Float | Navigation movement rate. | `set speed 15.0` |
| `fov` | Float | Field of view in degrees. | `set fov 75.0` |
| `exposure` | Float | Exposure compensation in EV units ([MATHEMATICAL_FORMULATION.md Section 8.3](MATHEMATICAL_FORMULATION.md#83-tone-mapping-and-color-grading)). | `set exposure 1.5` |
| `tonemapper`, `tonemap` | Integer | Operator (0: Linear, 1: ACES, 2: Logarithmic, 3: Reinhard; see [MATHEMATICAL_FORMULATION.md Section 8.3](MATHEMATICAL_FORMULATION.md#83-tone-mapping-and-color-grading)). | `set tonemapper 1` |
| `rtol` | Float | Relative integration tolerance. | `set rtol 1e-12` |
| `atol` | Float | Absolute integration tolerance. | `set atol 1e-15` |
| `min_step` | Float | Minimum integration step size bound ([MATHEMATICAL_FORMULATION.md Section 8.2](MATHEMATICAL_FORMULATION.md#82-step-control-termination-and-space-skipping)). | `set min_step 1e-10` |
| `max_step` | Float | Maximum integration step size bound (clamped to 0.01 to 50). | `set max_step 1.0` |
| `step_factor`, `step_scale` | Float | Step size scaling factor of the adaptive controller (clamped to 0.002 to 0.5; see [MATHEMATICAL_FORMULATION.md Section 8.2](MATHEMATICAL_FORMULATION.md#82-step-control-termination-and-space-skipping)). | `set step_factor 0.45` |
| `render_scale`, `scale` | Float | Internal raster resolution scaling factor (0.1 to 2.0; see [ARCHITECTURE.md Section 2.9](ARCHITECTURE.md#29-render-pipeline)). | `set scale 1.0` |
| `ray_steps`, `steps_limit` | Integer | Maximum integration steps per ray (64 to 65536). | `set ray_steps 2048` |
| `performance`, `perf` | Integer | Performance profile preset (0 to 5; 6 denotes Custom and leaves the current values unchanged). Preset contents are listed in [Section 2.5](#25-command-processing-and-performance-presets). | `set performance 2` |
| `camera_mode`, `cam_mode` | Integer | Mode (0: Free Fly, 1: Orbit Center, 2: Spherical, 3: Rocket Thrust). Behaviors are described in [Section 3.2](#32-navigation-modes). | `set camera_mode 0` |
| `sky_star_density` | Float | Celestial star density scaling factor. | `set sky_star_density 1.0` |
| `sky_star_brightness` | Float | Star luminance multiplier. | `set sky_star_brightness 1.0` |
| `sky_nebula` | Float | Galactic band and nebula intensity. | `set sky_nebula 1.0` |
| `sky_grid_opacity` | Float | Coordinate grid opacity. | `set sky_grid_opacity 1.0` |
| `sky_rotation` | Float | Sky sphere rotation angle in degrees. | `set sky_rotation 45.0` |
| `sky_hue` | Float | Sky hue shift in degrees. | `set sky_hue 0.0` |
| `sky_saturation` | Float | Sky color saturation factor. | `set sky_saturation 1.0` |
| `sky_bg_r`, `sky_bg_g`, `sky_bg_b` | Float | Background color components [0.0, 1.0]. | `set sky_bg_r 0.0` |
| `work_distribution`, `tiling` | Integer | Work layout (0: Scanlines, 1: 32x32 Tiles). | `set tiling 1` |
| `force_realloc`, `realloc_texture` | Boolean (0/1) | Forces full GPU texture storage reallocation each frame instead of in-place sub-image updates. | `set force_realloc 1` |
| `space_skip`, `space_skip_enabled` | Boolean (0/1) | Enables analytic ray leaping across the weak-field region beyond the space-skip radius. | `set space_skip 1` |
| `space_skip_radius` | Float | Radius, in units of central mass M, beyond which space-skipping may activate. | `set space_skip_radius 60.0` |
| `pole_precision` | Float | Strength of the automatic step-size damping applied near the coordinate poles. | `set pole_precision 2.5` |
| `tickrate` | Float | Scheduler frequency in Hertz (10.0 to 1000.0). | `set tickrate 60.0` |
| `precision_mode` | Integer | Arithmetic precision of the software renderer (0: native FP64, 1: double-single emulation). Stored as a custom parameter and read by the viewport. | `set precision_mode 1` |
| `warp_radius`, `bubble_radius` | Float | Alcubierre bubble radius in simulation length units. | `set warp_radius 15.0` |
| `warp_sharpness`, `wall_sharpness` | Float | Alcubierre bubble wall sharpness parameter $\sigma$. | `set warp_sharpness 0.4` |
| `tidal_potential`, `wormhole_tidal` | Float | Morris-Thorne wormhole redshift potential parameter $\Phi_0$. | `set tidal_potential 0.0` |
| `teves_a0`, `critical_acceleration` | Float | TeVeS critical acceleration scale $a_0$. | `set teves_a0 1.0e-3` |
| `light_mode` | Integer | Light source mode (0: Central, 1: Headlight, 2: Direction, 3: Point, 4: Nearest, 5: Body, 6: Unlit). | `set light_mode 0` |
| `light_intensity` | Float | Direct light source radiance multiplier. | `set light_intensity 1.5` |
| `light_ambient` | Float | Ambient illumination floor on unlit hemispheres [0.0, 1.0]. | `set light_ambient 0.04` |
| `light_softness` | Float | Terminator transition softness width [0.0, 1.0]. | `set light_softness 0.12` |
| `light_specular` | Float | Specular reflection highlight multiplier. | `set light_specular 1.0` |
| `light_attenuation` | Integer | Light distance attenuation mode (0: None, 1: Inverse-Square). | `set light_attenuation 1` |
| `light_reference` | Float | Inverse-square light reference distance. | `set light_reference 50.0` |
| `light_azimuth` | Float | Directional light horizontal azimuth in degrees [-180, 180]. | `set light_azimuth 45.0` |
| `light_elevation` | Float | Directional light vertical elevation in degrees [-90, 90]. | `set light_elevation 35.0` |
| `light_body` | Integer | Specific celestial body ID acting as light emitter. | `set light_body 10` |
| `light_color_r`, `light_color_g`, `light_color_b` | Float | RGB color components of the direct light source [0.0, 4.0]. | `set light_color_r 1.0` |
| `light_pos_x`, `light_pos_y`, `light_pos_z` | Float | Coordinates of the point light source in simulation length units. | `set light_pos_x 100.0` |
| `body_lighting_detail` | Integer | Lighting quality preset (0: Minimal, 1: Reduced, 2: Full). | `set body_lighting_detail 2` |
| `body_shadow_caster_limit` | Integer | Maximum shadow occluders tested per surface point (1 to 512). | `set body_shadow_caster_limit 32` |
| `body_emission_source_limit` | Integer | Maximum radiant emitters accumulated per point (1 to 512). | `set body_emission_source_limit 8` |
| `cam_arrow_keys_mode`, `arrow_keys_mode` | Integer | Arrow keys operational mode (0: Movement, 1: LookPitchYaw, 2: LookPitchRoll, 3: LookRollYaw). | `set cam_arrow_keys_mode 1` |
| `arrow_look_sensitivity` | Float | Rotation rate multiplier applied when arrow keys operate in look mode (0.1 to 5.0). | `set arrow_look_sensitivity 1.5` |
| `contrast`, `post_contrast` | Float | Post-tonemap contrast scaling factor (0.1 to 3.0). | `set contrast 1.15` |
| `saturation`, `post_saturation` | Float | Post-tonemap color saturation multiplier (0.0 to 3.0). | `set saturation 1.1` |
| `lift`, `post_lift` | Float | Shadow pedestal offset [-0.5, 0.5]. | `set lift 0.02` |
| `gamma`, `post_gamma` | Float | Midtone power-law gamma correction (0.2 to 3.0). | `set gamma 1.05` |
| `gain`, `post_gain` | Float | Highlight gain multiplier (0.1 to 3.0). | `set gain 1.0` |
| `vignette`, `post_vignette` | Float | Radial corner darkening intensity [0.0, 1.0]. | `set vignette 0.25` |
| `highlights`, `post_highlights` | Float | Highlight recovery factor [-1.0, 1.0]. | `set highlights -0.2` |
| `shadows`, `post_shadows` | Float | Shadow detail lift factor [-1.0, 1.0]. | `set shadows 0.1` |
| `emission_lighting` | Boolean (0/1) | Enables light emission from bodies onto other bodies. | `set emission_lighting 1` |
| `emission_gain` | Float | Radiance multiplier for body-to-body emission lighting. | `set emission_gain 1.0` |
| `emission_reference` | Float | Reference distance for body-to-body emission lighting. | `set emission_reference 30.0` |
| `camera_collision` | Boolean (0/1) | Prevents camera from penetrating bodies or event horizons. | `set camera_collision 1` |
| `camera_collision_clearance` | Float | Minimum clearance distance kept between camera and surfaces. | `set camera_collision_clearance 0.05` |
| `camera_motion_refresh` | Boolean (0/1) | Enables live rendering refresh during surface navigation motion. | `set camera_motion_refresh 1` |
| `jet`, `jet_enabled` | Boolean (0/1) | Enables Blandford-Znajek jet magnetosphere rendering. | `set jet 1` |
| `jet_length` | Float | Outer radial extent of the jet in mass units $M$. | `set jet_length 200.0` |
| `jet_angle` | Float | Collimation opening angle of the jet footpoints in degrees. | `set jet_angle 15.0` |
| `jet_lorentz` | Float | Maximum terminal Lorentz factor $\Gamma_{\text{max}}$ of jet plasma. | `set jet_lorentz 10.0` |
| `jet_bfield` | Float | Horizon magnetic field strength in Teslas. | `set jet_bfield 1.0e4` |
| `disk_inner` | Float | Equatorial accretion disk inner radius in multiples of $r_{\text{ISCO}}$. | `set disk_inner 1.0` |
| `disk_outer` | Float | Equatorial accretion disk outer radius in mass units $M$. | `set disk_outer 25.0` |
| `disk_temp_scale` | Float | Accretion disk peak blackbody temperature in Kelvin. | `set disk_temp_scale 18000.0` |
| `disk_temp_floor` | Float | Accretion disk floor blackbody temperature in Kelvin. | `set disk_temp_floor 1200.0` |
| `disk_beaming` | Float | Extra Doppler beaming power-law exponent. | `set disk_beaming 0.0` |
| `disk_saturation` | Float | Accretion disk color saturation multiplier. | `set disk_saturation 1.0` |
| `dark_matter`, `dm_enabled` | Boolean (0/1) | Master toggle for dark matter halo field interactions. | `set dark_matter 1` |
| `dm_lensing` | Boolean (0/1) | Enables geodesic gravitational lensing through dark matter halos. | `set dm_lensing 1` |
| `dm_viz`, `dm_visualization` | Boolean (0/1) | Enables false-color dark matter field visualization. | `set dm_viz 1` |
| `dm_viz_mode` | Integer | Visualization mode (0: ColumnDensity, 1: FieldStrength, 2: CircularSpeedSquared). | `set dm_viz_mode 1` |
| `dm_affects_bodies` | Boolean (0/1) | Enables dark matter gravitational acceleration on celestial bodies. | `set dm_affects_bodies 1` |
| `dm_lensing_strength` | Float | Global geodesic gravitational lensing deflection multiplier (0.0 to 8.0). | `set dm_lensing_strength 1.5` |
| `dm_visual_intensity` | Float | False-color dark matter visualization radiance gain (0.0 to 20.0). | `set dm_visual_intensity 0.5` |
| `dm_step_fraction` | Float | Ray-tracing adaptive step fraction within halo boundaries (0.02 to 1.0). | `set dm_step_fraction 0.25` |
| `cosmo_om` | Float | Cosmological dimensionless matter density parameter $\Omega_m$ in [0.01, 0.99]. | `set cosmo_om 0.315` |
| `cosmo_or` | Float | Cosmological radiation density parameter $\Omega_r$ in [0.0, 0.05]. | `set cosmo_or 9.0e-5` |
| `cosmo_pot` | Integer | Quintessence potential (0: Exponential, 1: InversePowerLaw, 2: PNGB, 3: SUGRA). | `set cosmo_pot 1` |
| `cosmo_slope` | Float | Quintessence potential slope or power-law exponent (0.01 to 20.0). | `set cosmo_slope 2.0` |
| `cosmo_scale` | Float | Characteristic field scale parameter $f$ for PNGB potential (0.05 to 50.0). | `set cosmo_scale 1.0` |
| `cosmo_phi0` | Float | Initial scalar field value $\phi_0$ at $\ln a = -6.9078$ (0.01 to 20.0). | `set cosmo_phi0 0.1` |
| `cosmo_chap_a` | Float | Chaplygin gas stiffness parameter $A_s$ in [0.0, 1.0]. | `set cosmo_chap_a 0.75` |
| `cosmo_chap_alpha` | Float | Generalized Chaplygin gas exponent $\alpha$ in [0.0, 10.0]. | `set cosmo_chap_alpha 1.0` |
| `cosmo_void_radius`, `cosmo_ltb_r0` | Float | Lemaitre-Tolman-Bondi void transition radius $r_0$ in simulation length units. | `set cosmo_ltb_r0 60.0` |
| `cosmo_void_width`, `cosmo_ltb_w` | Float | Lemaitre-Tolman-Bondi void transition thickness $w$ in simulation units. | `set cosmo_ltb_w 25.0` |
| `cosmo_ltb_h0` | Float | Lemaitre-Tolman-Bondi asymptotic Hubble expansion parameter $H_0$. | `set cosmo_ltb_h0 0.005` |
| `cosmo_ltb_in` | Float | Lemaitre-Tolman-Bondi central matter fraction $\Omega_{\text{in}}$ in [0.01, 3.0]. | `set cosmo_ltb_in 0.2` |
| `cosmo_ltb_out` | Float | Lemaitre-Tolman-Bondi boundary matter fraction $\Omega_{\text{out}}$ in [0.01, 3.0]. | `set cosmo_ltb_out 1.0` |
| `hubble` | Float | Present-day Hubble constant $H_0$ in simulation frequency units. | `set hubble 0.05` |
| `evaporation`, `cosmo_ev_enabled` | Boolean (0/1) | Enables semiclassical black hole Hawking evaporation. | `set evaporation 1` |
| `cosmo_ev_spectrum` | Integer | Evaporation spectrum (0: PhotonsOnly, 1: MasslessStandardModel, 2: Custom). | `set cosmo_ev_spectrum 1` |
| `cosmo_ev_model` | Integer | Stress-energy tensor model (0: BlackbodyGreybody, 1: PolyakovSWave). | `set cosmo_ev_model 0` |
| `cosmo_ev_dof` | Float | Custom radiation degrees of freedom for evaporation (0.1 to 1000.0). | `set cosmo_ev_dof 9.25` |
| `cosmo_ev_grey` | Float | Semiclassical greybody factor multiplier (0.01 to 100.0). | `set cosmo_ev_grey 1.0` |
| `evap_accel`, `cosmo_ev_accel` | Float | Time acceleration factor scaling evaporation mass loss rate (1.0 to $10^{60}$). | `set evap_accel 1.0e6` |
| `cosmo_ev_spin` | Float | Differential spin-down ratio $k_{\text{spin}}$ (0.0 to 10.0). | `set cosmo_ev_spin 3.0` |
| `cosmo_ev_min` | Float | Semiclassical remnant mass floor $M_{\text{min}}$ in kg (default: Planck mass). | `set cosmo_ev_min 2.176434e-8` |
| `cosmo_ev_gap` | Float | Unruh-DeWitt detector energy level gap in eV ($10^{-15}$ to $10^{12}$). | `set cosmo_ev_gap 1.0e-6` |
| `cosmo_ev_coupling` | Float | Unruh-DeWitt scalar field coupling constant ($10^{-6}$ to $10^3$). | `set cosmo_ev_coupling 1.0` |
| `constants_preset` | Integer | Physical constants preset (0: SI, 1: Geometrized, 2: Planck, 3: Custom). | `set constants_preset 0` |
| `constant_c` | Float | Speed of light $c$ in simulation units. | `set constant_c 299792458.0` |
| `constant_g` | Float | Gravitational constant $G$ in simulation units. | `set constant_g 6.6743e-11` |
| `interaction_electricity` | Boolean (0/1) | Enables Coulomb electrostatic forces between charged bodies. | `set interaction_electricity 1` |
| `interaction_magnetism` | Boolean (0/1) | Enables magnetic dipole forces between bodies. | `set interaction_magnetism 1` |
| `interaction_collisions` | Boolean (0/1) | Enables Hertzian mechanical contact collision dynamics. | `set interaction_collisions 1` |
| `interaction_thermo` | Boolean (0/1) | Enables Stefan-Boltzmann radiative thermodynamic exchange. | `set interaction_thermo 1` |
| `interaction_frag` | Boolean (0/1) | Enables body fragmentation from impact and tidal stress. | `set interaction_frag 1` |
| `interaction_annihilation`| Boolean (0/1) | Enables matter-antimatter annihilation on contact. | `set interaction_annihilation 1` |
| `sky_seed` | Integer | Seed integer driving procedural star, nebula, and galaxy generators. | `set sky_seed 42` |
| `sky_galaxy_density` | Float | Density multiplier for background deep-field galaxies. | `set sky_galaxy_density 0.8` |
| `sky_galaxy_brightness` | Float | Radiance multiplier for background deep-field galaxies. | `set sky_galaxy_brightness 0.1` |
| `sky_galaxy_scale` | Float | Angular size scale factor for background galaxies. | `set sky_galaxy_scale 1.5` |
| `sky_dust_density` | Float | Interstellar dust extinction lane spatial density. | `set sky_dust_density 3.5` |
| `sky_dust_intensity` | Float | Interstellar dust lane visual extinction contrast. | `set sky_dust_intensity 1.2` |
| `sky_dust_scale` | Float | Spatial scale for fractal dust extinction structures. | `set sky_dust_scale 4.0` |
| `sky_cluster_density` | Float | Open and globular star cluster spatial density. | `set sky_cluster_density 0.2` |
| `sky_cluster_brightness` | Float | Radiance multiplier for star clusters. | `set sky_cluster_brightness 0.5` |
| `sky_cluster_scale` | Float | Angular diameter scale for star clusters. | `set sky_cluster_scale 1.8` |
| `sky_star_brightness_variation` | Float | Variance of procedural star brightness [0.0, 1.0]. | `set sky_star_brightness_variation 0.5` |
| `sky_star_size_variation` | Float | Variance of procedural star angular diameter [0.0, 1.0]. | `set sky_star_size_variation 0.54` |
| `sky_star_color_variation` | Float | Chromatic temperature variance of stars [0.0, 2.0]. | `set sky_star_color_variation 1.0` |
| `sky_star_temperature_bias` | Float | Overall star spectral temperature bias [-1.0, 1.0]. | `set sky_star_temperature_bias 0.32` |
| `sky_panorama_source` | Integer | Sky background source (0: Procedural, 1: Panorama image). | `set sky_panorama_source 0` |
| `sky_panorama_id` | Integer | Panorama asset identifier (0: AmbientCG, 1: ESO Milky Way). | `set sky_panorama_id 0` |
| `sky_panorama_quality` | Integer | Panorama resolution tier (0: 2K, 1: 4K, 2: 8K). | `set sky_panorama_quality 1` |
| `sky_stars` | Boolean (0/1) | Master toggle for starfield rendering. | `set sky_stars 1` |
| `sky_galaxies` | Boolean (0/1) | Master toggle for background deep-field galaxy rendering. | `set sky_galaxies 1` |
| `sky_dust` | Boolean (0/1) | Master toggle for interstellar dust extinction lanes. | `set sky_dust 1` |
| `sky_clusters` | Boolean (0/1) | Master toggle for open and globular star cluster rendering. | `set sky_clusters 1` |
| `<custom_name>` | Float | User-defined custom scalar quantity. Names are truncated to 63 characters and at most 32 distinct names are stored; further names are ignored. | `set dyn_res 1.0` |

### 2.5. Command Processing and Result Reporting

Command words are case-insensitive and arguments are separated by spaces or tabs. `CommandParser::parse` (`include/relativistic/orchestrator/command.hpp`) produces a `Command`, and `MasterTerminalRepl::execute_line` (`include/relativistic/orchestrator/repl.hpp`) pushes it onto the command queue (capacity 1024). When the queue is full the line is rejected with the message `Command queue full`. Commands are applied by `SimulationOrchestrator::process_incoming_commands` and each application pushes a `CommandResult` (message of at most 127 characters) onto the result queue. Text arguments (`metric`, `integrator`, `load`, `save`) are truncated to 255 characters.

### 2.6. Body Catalog Batch Operations (`body`)

| Command | Arguments | Description | Example |
| :--- | :--- | :--- | :--- |
| `body invert` | None | Inverts velocity vectors ($\mathbf{v} \leftarrow -\mathbf{v}$) of all catalog bodies. | `body invert` |
| `body scatter` | None | Adds random spatial perturbations to all body positions scaled to each body's radius. | `body scatter` |
| `body snap` | `[cell_size]` | Snaps all body positions to a spatial Cartesian grid of specified cell size (default: 1.0). | `body snap 2.5` |
| `body cull` | `[radius]` | Removes all catalog bodies located beyond the specified radius from the origin (default: 1000.0). | `body cull 500.0` |
| `body equalize` | None | Sets all body masses equal to the catalog arithmetic mean. | `body equalize` |
| `body average` | None | Replaces all body masses with their arithmetic mean (alias of `equalize`). | `body average` |
| `body zero_spin` | None | Sets intrinsic spin angular momentum vectors of all bodies to zero. | `body zero_spin` |
| `body clear` | None | Removes every body from the N-body catalog and clears surface texture layer registries. | `body clear` |
| `body remove`, `body del` | `<id>` | Permanently removes the body with the specified numeric identifier. | `body remove 4` |
| `body add` | `<m> <r> <x> <y> <z> [name]` | Instantiates a new body with mass $m$, physical radius $r$, initial Cartesian position $(x, y, z)$, and optional label. | `body add 1.0 0.5 25.0 0.0 0.0 Mars` |

Bodies instantiated with `preset_3d == 8` (`BlackHole`) or marked `is_spacetime_source == true` operate as Kerr spacetime sources. They possess an outer horizon radius $r_h = M + \sqrt{M^2 - a^2}$, evaluate local spin triads, host subsidiary accretion disks, and participate in superposed Kerr-Schild ray tracing when two or more independent black holes exist in the catalog ([TECHNICAL_MANUAL.md Section 2.2](TECHNICAL_MANUAL.md#22-spacetime-metric-implementations)).

### 2.7. Subsystem Direct & Quick Commands

| Command | Arguments | Subsystem / Operation | Example |
| :--- | :--- | :--- | :--- |
| `light mode` | `<0..6 \| name>` | Selects the active illumination source mode: `0`/`central` (origin), `1`/`headlight` (camera co-located), `2`/`direction` (distant key light), `3`/`point` (fixed Cartesian position), `4`/`nearest` (nearest emissive body $I_{\text{em}} > 10^{-3}$), `5`/`body` (specific catalog body ID), `6`/`unlit` (uniform unit shading without direct lighting or shadows). | `light mode headlight` |
| `light preset` | `<0..5 \| name>` | Applies an illumination scene preset: `0`/`stellar` (CentralSource, softness 0.12, ambient 0.04), `1`/`headlight` (CameraHeadlight, ambient 0.02, softness 0.0), `2`/`studio` (FixedDirection at az 40 deg, el 35 deg, warm color, ambient 0.12, softness 0.25), `3`/`unlit` (Unlit mode), `4`/`dramatic` (CentralSource, intensity 1.5, ambient 0.005, softness 0.02, specular 1.5, inverse-square attenuation), `5`/`reset` (restores all parameters including inter-body emission to default). | `light preset dramatic` |
| `light intensity` | `<value>` | Direct light irradiance multiplier ($0.0$ to $50.0$). Values $> 1.0$ brighten lit surfaces beyond unit gain. | `light intensity 1.5` |
| `light ambient` | `<value>` | Ambient illumination floor on unlit hemispheres ($0.0$ to $1.0$). Zero yields unlit nocturnal sides. | `light ambient 0.04` |
| `light softness` | `<value>` | Terminator transition softness width parameter $s \in [0.0, 1.0]$ governing wrapped diffuse shading. | `light softness 0.15` |
| `light specular` | `<value>` | Blinn-Phong specular highlight scale multiplier ($0.0$ to $8.0$). | `light specular 1.2` |
| `light attenuation` | `<0..1 \| name>` | Attenuation model for localized emitters: `0`/`none` (constant irradiance), `1`/`inverse_square` ($\mathcal{E}(d) = I \min((d_{\text{ref}}/d)^2, 64.0)$). Distant directional key light ignores attenuation. | `light attenuation inverse_square` |
| `light reference` | `<distance>` | Inverse-square distance attenuation reference baseline ($10^{-3}$ to $10^7$) in simulation length units. | `light reference 50.0` |
| `light azimuth` | `<degrees>` | Directional light horizontal azimuth in degrees ($-180.0$ to $180.0$), measured counterclockwise from $+X$. | `light azimuth 45.0` |
| `light elevation` | `<degrees>` | Directional light vertical elevation in degrees ($-90.0$ to $90.0$), where $+90^\circ$ points straight down $+Z$. | `light elevation 35.0` |
| `light body` | `<id>` | Specific catalog body ID acting as primary light emitter (mode 5). Modulated by body primary color. | `light body 10` |
| `light color` | `<r> <g> <b>` | Direct light source RGB color components ($0.0$ to $4.0$). | `light color 1.0 0.95 0.85` |
| `light position` | `<x> <y> <z>` | Cartesian coordinates of fixed point light source (mode 3) in simulation length units. | `light position 100.0 0.0 50.0` |
| `light detail` | `<0..2 \| name>` | Lighting execution complexity: `0`/`minimal` (ambient and diffuse only; shadows and emission bypassed), `1`/`reduced` (shadows and emission active subject to limits), `2`/`full` (uncapped testing across all bodies). | `light detail full` |
| `light shadow_limit` | `<count>` | Maximum shadow occluders tested per surface point in reduced mode ($1$ to $512$). | `light shadow_limit 32` |
| `light emission_limit` | `<count>` | Maximum radiant emitters accumulated per point in reduced mode ($1$ to $512$). | `light emission_limit 8` |
| `light emission` | `<on \| off>` | Master toggle for inter-body radiant transfer from emissive bodies ($I_{\text{em}} > 10^{-3}$). | `light emission on` |
| `light emission_gain` | `<value>` | Global radiance gain multiplier for body-to-body emission lighting ($0.0$ to $10^5$). | `light emission_gain 1.5` |
| `light emission_ref` | `<distance>` | Reference distance baseline for body-to-body emission lighting ($10^{-3}$ to $10^7$). | `light emission_ref 30.0` |
| `arrow mode` | `<0..3 \| name>` | Selects arrow keys behavior: `0`/`movement`, `1`/`look_pitch_yaw`, `2`/`look_pitch_roll`, `3`/`look_roll_yaw`. | `arrow mode look_pitch_yaw` |
| `post` | `<parameter> <value>` | Color grading (contrast, saturation, lift, gamma, gain, vignette, highlights, shadows). | `post contrast 1.2` |
| `post reset` | None | Resets all post-tonemapping color grading parameters to neutral defaults. | `post reset` |
| `disk` | `<parameter> <value>` | Accretion disk (temp_scale, temp_floor, beaming, saturation, inner, outer). | `disk inner 1.2` |
| `jet` | `<parameter> <value>` | Blandford-Znajek jet (on, off, length, angle, lorentz, bfield, precession). | `jet on` |
| `darkmatter` | `<parameter> <value>` | Dark matter field commands: `on`/`off`, `lensing <on/off>`, `viz <on/off>`, `viz_mode <0..2|density|field|speed>`, `bodies <on/off>`, `strength <val>`, `intensity <val>`, `step <val>`, `clear`, `sort <asc|desc>`, `rescale <mass>`. | `darkmatter viz_mode field` |
| `cosmology` | `<parameter> <value>` | Cosmology and semiclassical commands: `evaporation <on/off>`, `void_radius <val>`, `void_width <val>`, `ltb_in <val>`, `ltb_out <val>`, `hubble <val>`, `pot <0..3>`, `slope <val>`, `scale <val>`, `phi0 <val>`, `chap_a <val>`, `chap_alpha <val>`, `evap_accel <val>`, `evap_spectrum <0..2>`, `evap_model <0..1>`, `evap_dof <val>`, `evap_min <val>`. | `cosmology evap_accel 1e8` |
| `unit` | `<quantity> <unit>` | Display unit selection across 19 physical quantities. | `unit distance km` |
| `unit reset` | None | Resets all display units to SI defaults. | `unit reset` |
| `coord default` | `<system>` | Selects global coordinate system (cartesian, spherical, cylindrical, boyer_lindquist, tortoise, isotropic, kruskal, painleve, eddington). | `coord default spherical` |
| `sky` | `<parameter> <value>` | Skybox rendering parameters: `on`, `off`, `density <val>`, `brightness <val>`, `nebula <val>`, `grid <val>`, `rotation <deg>`, `seed <int>`, `hue <deg>`, `saturation <val>`, `bg <r> <g> <b>`, `stars` (`on`/`off`, `density`, `brightness`, `size_var`, `bright_var`, `color_var`, `temp_bias`), `galaxies` (`on`/`off`, `density`, `brightness`, `scale`), `dust` (`on`/`off`, `density`, `intensity`, `scale`), `clusters` (`on`/`off`, `density`, `brightness`, `scale`), `quasars` (`on`/`off`), `pulsars` (`on`/`off`), `snr` (`on`/`off`), `panorama` (`source <0/1>`, `id <0..2>`, `quality <0..2>`). | `sky nebula 0.5` |
| `snapshot` | `[reason]` | Captures live forensic diagnostic report and minidump to `logs/crashes/snapshot_<stamp>_pid<pid>_n<n>/`. | `snapshot milestone_reached` |
| `journal` | None | Outputs the last 64 entries of the command execution journal ring and the last 32 entries of the problem error/warning ring directly to standard output. | `journal` |
| `quicksave` | None | Serializes active simulation state to `scenarios/quicksave.yaml`. | `quicksave` |
| `quickload` | None | Ingests simulation state from `scenarios/quicksave.yaml`. | `quickload` |
| `help` | `[topic \| page]` | Displays command reference, topic manual, or specific help page. | `help camera` |

Direct parameter commands can also be issued without the `set` prefix: `mass <val>`, `spin <val>`, `charge <val>`, `lambda <val>`, `fov <val>`, `speed <val>`, `exposure <val>`, `preset <0..5|name>`, `scale <val>`, `steps <val>`, `gpu <on|off>`, `lod <on|off>`, `skip <on|off>`, `shadows <on|off>`, `atmosphere <off|on|val>`, `bodies_only <on|off>`, `disk_occlusion <on|off>`, `schematic <on|off>`.

### 2.8. Command Processing & Performance Presets

Performance presets applied by `SimulationOrchestrator::apply_performance_preset`:

| Index | Name | Resolution scale | Max ray steps | rtol | atol | Body LOD pixel threshold | Body point pixel threshold | Body noise octaves | Body shadows | Body disk occlusion | Atmosphere intensity | Body low power mode |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| 0 | Potato | 0.25 | 512 | 1e-6 | 1e-10 | 24 | 6 | 1 | Off | Off | 0.0 | On |
| 1 | Performance | 0.5 | 2048 | 1e-8 | 1e-12 | 16 | 4 | 2 | Off | Off | 0.5 | Off |
| 2 | Balanced | 1.0 | 4096 | 1e-9 | 1e-13 | 10 | 2 | 4 | Off | Off | 1.0 | Off |
| 3 | High | 1.25 | 16384 | 1e-10 | 1e-14 | 6 | 1 | 5 | On | On | 1.0 | Off |
| 4 | Ultra | 1.5 | 8192 | 1e-12 | 1e-15 | 4 | 1 | 6 | On | On | 1.25 | Off |
| 5 | Extreme | 2.0 | 16384 | 1e-14 | 1e-17 | 4 | 1 | 6 | On | On | 1.5 | Off |

The captions shown for some presets in the Performance & Engine Optimization window differ from these values; the values above are those applied to the simulation parameters.

---

## 3. Graphical Interface & Interactive Controls

When executed without the `--headless` flag, `engine_cli` initializes a graphical OpenGL context managed through ImGui & ImPlot.

### 3.1. Navigation & Viewport Keybindings

| Key / Input | Action | Mode |
| :--- | :--- | :--- |
| `W` / `Z` | Translate forward | Free Fly / Rocket Thrust / Orbit distance / Spherical radius |
| `S` | Translate backward | Free Fly / Rocket Thrust / Orbit distance / Spherical radius |
| `A` / `Q` | Translate / Orbit left | Free Fly / Orbit / Spherical |
| `D` | Translate / Orbit right | Free Fly / Orbit / Spherical |
| `Space` / `E` | Translate upward | Free Fly |
| `C` / `Ctrl` | Translate downward | Free Fly |
| `J` / `Page Up` | Roll counter-clockwise | Free Fly |
| `K` / `Page Down` | Roll clockwise | Free Fly |
| `Shift` (Hold) | High-speed boost modifier ($4\times$) | All Modes |
| `Ctrl` / `Alt` (Hold) | Precision crawl modifier ($0.2\times$) | All Modes |
| `Right Click` + Drag | Angular look (Pitch / Yaw) | All Modes |
| `Left Click` + Drag | Angular look; both buttons are handled identically and a drag starts after a 4 pixel displacement | All Modes |
| `Mouse Scroll` | Adjust camera field of view in steps of 2.5 degrees (5 to 170 degrees) | All Modes |
| Zoom modifier (Hold) + `Mouse Scroll` | Magnify the displayed image between `min_zoom` and `max_zoom` (1.0 to 8.0), centered on the cursor when `zoom_center_on_cursor` is set; the field of view is unchanged | All Modes |
| Gamepad Left Stick (LX, LY) | Fore/Aft translation and horizontal strafe evaluated via quadratic deadzone filtering ($d_{\text{LS}} = 0.14$) | Free Fly / Rocket Thrust |
| Gamepad Right Stick (RX, RY) | Camera look pitch and yaw rotation evaluated via quadratic deadzone filtering ($d_{\text{RS}} = 0.14$, sensitivity $80^\circ/\text{s}$ X, $65^\circ/\text{s}$ Y) | All Modes |
| Gamepad Left / Right Triggers (LT, RT) | Vertical descent (LT) and ascent (RT) elevation translation normalized to $[0, 1]$ with deadzone $d_{\text{trig}} = 0.08$ | Free Fly / Rocket Thrust |
| Gamepad Left / Right Bumpers (LB, RB) | Roll rotation around line of sight (LB: CCW, RB: CW at $45^\circ/\text{s}$) | Free Fly / Rocket Thrust |
| Gamepad Left Stick Click (LSB / L3) | Sprint translation speed multiplier boost ($4\times$) | Free Fly / Rocket Thrust / Orbit |
| Gamepad Right Stick Click (RSB / R3) | Crawl translation speed precision divisor ($0.2\times$) | Free Fly / Rocket Thrust / Orbit |
| Gamepad Button A | Single-step simulation tick advance (evaluates in Tap mode, window $\le 0.25\,\text{s}$) | All Modes |
| Gamepad Button X | Cycle optical projection geometry (evaluates in Tap mode) or hold-to-zoom magnification | All Modes |
| Gamepad Button Start | Pause / Resume simulation clock execution (evaluates in Tap mode) | All Modes |
| Gamepad D-Pad Up / Down | Increase / Decrease camera field of view aperture | All Modes |
| Gamepad Chord `RB + LB` | Dual-bumper simultaneous chord resetting camera roll angle to zero (evaluates in Tap mode) | Free Fly / Rocket Thrust |
| Gamepad Virtual Buttons (15-24) | Directional virtual buttons triggered when filtered stick or trigger deflections exceed $0.40$ (LSB Up: 15, Down: 16, Left: 17, Right: 18; RSB Up: 19, Down: 20, Left: 21, Right: 22; LT: 23, RT: 24) | Rebindable Actions |
| Arrow Keys (Mode 0: `Movement`) | Forward, backward, left strafe, right strafe translational motion | Free Fly / Rocket Thrust |
| Arrow Keys (Mode 1: `LookPitchYaw`) | Up/Down adjusts pitch angle; Left/Right adjusts yaw angle with sensitivity multiplier | All Modes |
| Arrow Keys (Mode 2: `LookPitchRoll`) | Up/Down adjusts pitch angle; Left/Right adjusts roll angle with sensitivity multiplier | All Modes |
| Arrow Keys (Mode 3: `LookRollYaw`) | Up/Down adjusts roll angle; Left/Right adjusts yaw angle with sensitivity multiplier | All Modes |

### 3.2. Navigation Modes

Speeds and sensitivities are stored in `CameraControlConfig` (`include/relativistic/ui/camera_control_config.hpp`) and persisted as described in [FILE_FORMATS.md](FILE_FORMATS.md#10-persistent-configuration-files). The camera pose is stored as Cartesian position and pitch, yaw and roll angles in degrees; the spherical coordinates `radius`, `theta` and `phi` are recomputed after every position change.

| Mode | Behavior |
| :--- | :--- |
| Free Fly (0) | Translation along the forward, right and up axes at 10 units per second by default. Pitch is ignored for movement when `ignore_pitch_roll_for_movement` is set (default). The sprint modifier multiplies the speed by 4 and the crawl modifier by 0.2. Roll rate is 45 degrees per second. |
| Orbit Center (1) | Forward and backward change the orbit distance (2 to 5000), left and right change the yaw, up and down change the pitch (limited to plus or minus 89 degrees). The camera is placed at `target - distance * view_direction`. |
| Spherical (2) | Forward and backward change `radius` (2.5 to 5000), left and right change `phi`, up and down change `theta` (0.01 to pi - 0.01) with angular speed `speed / radius`. The camera always looks at the origin. |
| Rocket Thrust (3) | Keys produce thrust accelerations (main 5, lateral 2, vertical 2) whose magnitude is capped at a proper acceleration of 20. Thrust is applied only while the simulation clock runs (`requires_time_running`). While the clock runs, the camera velocity receives a gravitational deceleration `M / r^2` directed toward the origin and the position is integrated. Roll rate is 15 degrees per second. |
| Surface Walk (4) | First-person surface traversal on the selected celestial body (`surface_walk_body_id`). Movement keys translate across local terrain, Space/Up jumps, C/Down crouches, sprint/crawl modifiers apply. Gravity holds the walker to the surface; maximum jump airtime, ground response time, head bob, and surface texture rotation tracking are configurable. |
| Planet Orbit (5) | Simplified spherical orbit around the selected celestial body. Movement or arrow keys orbit east/west and north/south, mouse drag orbits, wheel or Page Up/Down changes altitude, sprint and crawl modifiers scale speeds. Collision detection keeps camera outside body without gravity or falling physics. |

### 3.3. Global Function Shortcuts

| Shortcut | Description |
| :--- | :--- |
| `F1` | Toggle Master Simulation Controls window. |
| `F2` | Apply Multi-Window Detached workspace layout. |
| `F3` | Apply Docked Workspace layout container. |
| `F4` | Apply Viewport Fullscreen Focus mode. |
| `F5` / `P` | Toggle simulation pause / resume state. |
| `F6` | Advance simulation by a single logical tick. |
| `F7` | Reset simulation time and orbital clocks. |
| `F8` | Toggle Celestial Body & N-Body Manager window. |
| `F9` | Cycle through camera navigation modes. |
| `F10` | Toggle Telemetry & Invariants window. |
| `F11` | Toggle the Performance & Engine Optimization window and the Curvature Diagnostics & Tensor Inspector window together. |
| `F12` | Open the Capture Studio window. |
| `F` | Orient camera toward coordinate origin $(0, 0, 0)$. |
| `Home` | Reset camera roll angle to zero. |
| `[` / `]` | Decrease / Increase navigation movement speed by 5%. |
| `Numpad 1` | Snap viewpoint to equatorial front ($r = 50M$). |
| `Numpad 3` | Snap viewpoint to equatorial side ($r = 50M$). |
| `Numpad 7` | Snap viewpoint to north pole ($z = 50M$). |
| `Numpad 9` | Snap viewpoint to south pole ($z = -50M$). |
| `Numpad 5` | Snap viewpoint to ISCO orbital radius. |
| `H` | Toggle HUD master visibility. |
| `B` | Toggle Keybind Settings window. |
| `M` | Cycle the active spacetime metric. |
| `I` | Cycle the active ODE integrator. |
| `V` | Cycle the camera projection mode. |
| `T` | Cycle the HDR tonemapping operator. |
| `G` | Cycle the skybox style. |
| `U` | Toggle GPU compute offload. |
| `N` | Toggle adaptive space-skipping. |
| `L` | Toggle distance-based level of detail. |
| `=` | Increase exposure compensation (EV). |
| `-` | Decrease exposure compensation (EV). |
| `.` | Increase the time warp factor. |
| `,` | Decrease the time warp factor. |
| `Insert` | Quick-save the current scenario. |
| `Delete` | Quick-load the last quick-saved scenario. |
| `` ` `` | Toggle fullscreen viewport mode. |
| `R` | Toggle tiled work distribution. |
| `0` | Cycle the adaptive step-size controller. |
| `9` | Reset performance settings to the balanced preset. |
| `O` | Toggle Scenario Manager & Presets window. |
| `Y` | Toggle Curvature Diagnostics & Tensor Inspector window. |
| `X` | Toggle Radiative Transfer & Spectrograph Monitor window. |
| `;` | Toggle Physical Constants Engine window. |

### 3.4. Additional Rebindable Actions

The following actions are available in the Keybind Settings window. Each action has a primary and a secondary key, conflicts are highlighted, and the actions `ZoomModifier`, `Sprint` and `Crawl` support a Hold or Toggle activation mode. The keyboard layout preset (QWERTY or AZERTY) replaces all bindings with the defaults of that layout.

| Action | Effect |
| :--- | :--- |
| Toggle Capture Studio | Shows or hides the Capture Studio window. |
| Toggle Performance Analysis Window | Shows or hides the Performance Analysis & Profiling window; no default key. |
| Start/Stop Benchmark Capture | Starts a 10 second benchmark capture labeled `Quick Capture`, or cancels the running capture; no default key. |
| Quick Save Benchmark Run | Starts a 60 frame benchmark capture labeled `Quick Save`; no default key. |
| Bulk Invert All Velocities | Negates the velocity of every body. |
| Bulk Scatter Body Positions | Randomizes body positions (`BulkBodyActions::scatter_positions`). |
| Bulk Snap Bodies To Grid | Snaps body positions to a grid of spacing 1.0. |
| Bulk Cull Bodies Outside View | Removes bodies beyond `render_distance_scale * mass` (1e7 when the render distance is unbounded). |
| Bulk Equalize Body Masses | Sets all body masses to the same value. |
| Bulk Average Body Masses | Replaces body masses by their mean. |
| Bulk Zero All Spins | Sets all body spins to zero. |

### 3.5. Main Menu Bar

| Menu | Content |
| :--- | :--- |
| File | Capture Studio, quick screenshot with the current Capture Studio settings, snapshot scenario save (`scenarios/snapshot.yaml`), export entries (`fits`, `hdf5`, `vtk`), reload cluster configuration, exit. |
| Simulation | Pause/resume, single step, reset, camera mode cycle, snap to equatorial position (`r = 50`), snap to ISCO. |
| Capture | Capture Studio tab shortcuts (Screenshot, Sequence, Motion Script, Data Recording, Video Encoding), quick screenshot, start sequence, finish sequence, cancel capture. |
| Window Layouts | Multi-Window Detached, Docked Workspace, Viewport Fullscreen Focus, multi-window viewport separation toggle. |
| View Windows | Visibility toggles of every window, Show All Panels, Focus Viewport Only, Force Viewport Refresh, secondary viewport management. |

The Capture Studio, its sequence modes and its output files are described in [ARCHITECTURE.md](ARCHITECTURE.md#210-capture-subsystem) and [FILE_FORMATS.md](FILE_FORMATS.md#6-image-output-formats).

---

## 4. Headless Batch Exporter (`headless_exporter`)

The `headless_exporter` binary (`apps/headless_exporter/main.cpp`) processes declarative simulation scenarios without graphical dependencies.

### 4.1. Command Line Syntax

```bash
./headless_exporter [OPTIONS]
```

### 4.2. Command Line Arguments

| Option | Argument | Description | Default |
| :--- | :--- | :--- | :--- |
| `--scenario` | `<path>` | Path to declarative YAML scenario file ([FILE_FORMATS.md Section 1](FILE_FORMATS.md#1-declarative-scenario-definition-yaml)). | None |
| `--output-dir` | `<path>` | Output directory for scientific exports ([FILE_FORMATS.md Section 1.1](FILE_FORMATS.md#11-schema-specification)). | `./output` |
| `--format` | `<fmt>` | Export format filter: `fits` ([Section 2](FILE_FORMATS.md#2-flexible-image-transport-system-fits)), `hdf5` ([Section 3](FILE_FORMATS.md#3-hierarchical-data-format-h5)), `vtk` ([Section 4](FILE_FORMATS.md#4-visualization-toolkit-polydata-vtp)), `all`. | `all` |
| `--steps` | `<N>` | Number of discrete integration cycles. | `1000` |
| `--dt` | `<value>` | Discrete physical time step per cycle. | `0.01` |
| `--width` | `<pixels>` | Horizontal raytracing raster resolution. | `1920` |
| `--height` | `<pixels>` | Vertical raytracing raster resolution. | `1080` |
| `--validate-benchmarks` | None | Executes the formal analytical test suite ([DESCRIPTION.md Section 12](DESCRIPTION.md#12-verification-criteria--benchmark-protocols)). | Disabled |
| `--verbose` | None | Enables per-step constraint telemetry logging. | Disabled |
| `--help`, `-h` | None | Prints CLI argument syntax and exits. | None |

### 4.3. Execution Examples

#### Running Analytical Validation Suite

```bash
./headless_exporter --validate-benchmarks
```

#### Executing 4K Raytraced Kerr Black Hole Disk Render

```bash
./headless_exporter --scenario scenarios/kerr_accretion_disk.yaml --output-dir ./exports --width 3840 --height 2160 --format fits
```

#### High-Step N-Body Trajectory Generation in HDF5

```bash
./headless_exporter --scenario scenarios/solar_system_mercury.yaml --output-dir ./data --steps 100000 --dt 50.0 --format hdf5
```

---

## 5. Cluster Compute Worker (`cluster_worker`)

The `cluster_worker` binary (`apps/cluster_worker/main.cpp`) is an autonomous, lightweight compute daemon designed for distributed cluster rendering, HPC batch nodes, and cloud GPU instances without GLFW, OpenGL, or graphical window dependencies.

### 5.1. Command Line Syntax

```bash
./cluster_worker [OPTIONS]
```

### 5.2. Command Line Arguments

| Option | Argument | Description | Default |
| :--- | :--- | :--- | :--- |
| `--master-address` | `<host:port>` | Target cluster master endpoint to connect to (retried automatically). | None (listen-only) |
| `--auth-token` | `<hex64>` | 64-character hexadecimal pre-shared symmetric cluster secret token. | None |
| `--auth-token-file`| `<path>` | Path to a file containing the 64-character token on its first line. | None |
| `--listen-port` | `<port>` | Inbound UDP port accepting master connection requests. | `42425` |
| `--listen-interface`| `<addr>` | IPv4 address to bind the inbound transport socket. | `0.0.0.0` |
| `--no-listen` | None | Disables the inbound listening socket (outbound master connection only). | Listening enabled |
| `--threads` | `<N>` | Worker threads allocated to thread pool (0 uses logical processors). | Hardware concurrency |
| `--vulkan-device` | `<idx|auto>` | Vulkan physical GPU device index or `auto` for default. | `auto` |
| `--backend` | `<auto|gpu|cpu>` | Compute backend: `gpu` requires Vulkan FP64; `cpu` forces SIMD/scalar. | `auto` |
| `--weight` | `<float>` | Static performance weight advertised to master load balancer ($> 0.0$). | Derived from hardware |
| `--name` | `<text>` | Human-readable node identifier tag sent during handshake. | Hostname |
| `--cluster-name` | `<text>` | Cluster namespace identifier matching discovery beacons. | `Relativistic_Alpha_Mesh` |
| `--no-discovery` | None | Disables UDP multicast presence announcements and replies. | Discovery enabled |
| `--discovery-group`| `<addr>` | Multicast group IPv4 address for discovery beacons. | `239.255.77.88` |
| `--discovery-port` | `<port>` | Multicast destination port for discovery beacons. | `42424` |
| `--discovery-interface` | `<addr>` | Local interface address used for multicast beacon traffic. | Auto-assigned |
| `--reconnect-interval-ms` | `<ms>` | Delay between master reconnection attempts (100 to 600000 ms). | `2000` |
| `--cluster-config` | `<path>` | Path to `distributed_cluster.cfg` configuration file to load settings. | `config/distributed_cluster.cfg` |
| `--slurm-mode` | None | Enables automatic detection of Slurm MPI ranks, hosts, and task ports. | Auto-detected |
| `--headless`, `--verbose` | None | Accepted for batch script compatibility (always operates headlessly). | None |
| `--version` | None | Prints engine version string and exits. | None |
| `--help`, `-h`, `-?` | None | Prints CLI argument syntax and exits. | None |

### 5.3. Environment Variables

- `RELATIVISTIC_CLUSTER_TOKEN`: Specifies the 64-character hexadecimal pre-shared cluster secret if `--auth-token` is omitted.
- `SLURM_JOB_ID`, `SLURM_PROCID`, `SLURM_NTASKS`, `SLURM_STEP_NODELIST`: Detected automatically when running under the Slurm workload manager or with `--slurm-mode`.

### 5.4. Execution Examples

#### 1. Standalone Worker on Local Subnet (Discovered Automatically)

```bash
./cluster_worker --auth-token "a8f5c2d3e9b14708215649fbcde71024385960abcedf1234567890abcdef1234" --backend auto
```

#### 2. Cloud GPU Instance (Colab / Dedicated VPS) Connecting Directly to Master

```bash
./cluster_worker \
  --master-address 198.51.100.24:42425 \
  --auth-token "a8f5c2d3e9b14708215649fbcde71024385960abcedf1234567890abcdef1234" \
  --no-listen \
  --no-discovery \
  --vulkan-device auto \
  --weight 3.0
```

#### 3. Slurm HPC Cluster Multi-Node Batch Submission

When launched under the Slurm workload manager, `cluster_worker` inspects `SLURM_PROCID`, `SLURM_NTASKS`, and `SLURM_STEP_NODELIST` via [SlurmInterface](ARCHITECTURE.md#214-distributed-compute-subsystem--heterogeneous-cluster-architecture). Rank 0 starts as the cluster master orchestrator while higher ranks execute autonomous worker instances connected back to rank 0:

```bash
#!/bin/bash
#SBATCH --job-name=relativistic_cluster
#SBATCH --nodes=4
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=32
#SBATCH --time=04:00:00

export RELATIVISTIC_CLUSTER_TOKEN="a8f5c2d3e9b14708215649fbcde71024385960abcedf1234567890abcdef1234"

srun ./cluster_worker \
  --slurm-mode \
  --threads 32 \
  --backend auto
```

#### 4. Containerized Docker Deployment

Workers can be containerized and scaled horizontally across container clusters:

```bash
docker run -d --rm \
  --net=host \
  --gpus all \
  -e RELATIVISTIC_CLUSTER_TOKEN="a8f5c2d3e9b14708215649fbcde71024385960abcedf1234567890abcdef1234" \
  relativistic-worker:latest \
  --backend gpu \
  --threads 16
```

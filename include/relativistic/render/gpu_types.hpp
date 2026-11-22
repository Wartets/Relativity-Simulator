#pragma once

#include "relativistic/render/skybox_renderer.hpp"
#include <cstdint>
#include <cstddef>
#include <array>
#include <algorithm>

namespace Relativistic::Render {

enum class MetricId : uint32_t {
	FlatMinkowski = 0,
	Schwarzschild = 1,
	Kerr = 2,
	KerrSchild = 3,
	ReissnerNordstrom = 4,
	KerrNewman = 5,
	SchwarzschildDeSitter = 6,
	FLRW = 7,
	MorrisThorne = 8,
	Alcubierre = 9,
	LemaitreTolmanBondi = 10,
	QuintessenceCosmology = 11,
	ChaplyginCosmology = 12,
	EvaporatingBlackHole = 13,
	EddingtonFinkelstein = 14,
	PainleveGullstrand = 15,
	TeVeS = 16
};

enum class PrecisionMode : uint32_t {
	NativeFloat64 = 0,
	DoubleSingleEmulation = 1
};

namespace SkyBackgroundLimits {
	static constexpr double MIN_VALUE = 0.0;
	static constexpr double MAX_VALUE = 1.0;
}

struct GpuDiskProfile {
	float enabled{1.0f};
	float inner_radius_scale{1.0f};
	float outer_radius_mass_units{24.0f};
	float peak_temperature_k{9500.0f};
	float floor_temperature_k{1200.0f};
	float temperature_exponent{0.75f};
	float zero_torque_strength{1.0f};
	float extra_beaming_exponent{2.0f};
	float brightness{0.45f};
	float color_saturation{1.0f};
	float tint_strength{0.0f};
	float opacity_scale{1.0f};
	float tint_r{1.0f};
	float tint_g{1.0f};
	float tint_b{1.0f};
	float rotation_speed_scale{4.0f};
	float ring_amplitude{0.35f};
	float ring_frequency{9.0f};
	float ring_sharpness{3.0f};
	float spiral_arm_count{2.0f};
	float spiral_pitch{1.2f};
	float spiral_amplitude{0.25f};
	float turbulence_amplitude{0.45f};
	float turbulence_scale{5.0f};
	float turbulence_octaves{4.0f};
	float radial_stretch{3.0f};
	float grain_amplitude{0.2f};
	float grain_scale{40.0f};
	float reference_luminance{1.0f};
	float noise_seed{1.0f};
	float temperature_normalization{1.0f};
	float edge_softness{1.0f};

	[[nodiscard]] bool operator==(const GpuDiskProfile&) const noexcept = default;
};

static_assert(sizeof(GpuDiskProfile) == 128);

enum class HydroDiskModel : uint32_t {
	ThinProcedural = 0,
	NovikovThorne = 1,
	FishboneMoncrief = 2
};

namespace HydroDiskFlags {
	static constexpr uint32_t TORUS_VALID = 1U << 0;
}

struct GpuHydroDiskProfile {
	uint32_t model{0};
	float accretion_rate_scale{1.0f};
	float torus_inner_radius{6.0f};
	float torus_center_radius{12.0f};
	float adiabatic_index{1.3333334f};
	float optical_depth_scale{0.35f};
	float sampling_density{2.0f};
	uint32_t flags{0};
	float specific_angular_momentum{0.0f};
	float potential_inner{0.0f};
	float max_enthalpy{1.0f};
	float inner_bound{0.0f};
	float outer_bound{0.0f};
	float flux_peak{1.0f};
	float rate_factor{1.0f};
	float reserved{0.0f};

	[[nodiscard]] bool operator==(const GpuHydroDiskProfile&) const noexcept = default;
};

static_assert(sizeof(GpuHydroDiskProfile) == 64);

enum class JetFieldGeometry : uint32_t {
	Monopole = 0,
	SplitMonopole = 1,
	Paraboloidal = 2,
	Vertical = 3
};

enum class JetEmissionModel : uint32_t {
	NonThermal = 0,
	Thermal = 1,
	Hybrid = 2
};

enum class JetDisplayMode : uint32_t {
	TotalIntensity = 0,
	PolarizedIntensity = 1,
	PolarizationFraction = 2
};

enum class JetAngularVelocityLaw : uint32_t {
	HorizonFraction = 0,
	Fixed = 1
};

struct GpuJetProfile {
	float enabled{0.0f};
	float geometry{2.0f};
	float field_line_index{1.5f};
	float horizon_field_tesla{1.0e4f};
	float magnetization{50.0f};
	float magnetization_index{0.5f};
	float angular_velocity_law{0.0f};
	float angular_velocity_fraction{0.5f};
	float fixed_angular_velocity{0.1f};
	float angular_velocity_variation{0.0f};
	float toroidal_field_scale{1.0f};
	float rotation_coupling{1.0f};
	float mass{1.0f};
	float spin{0.0f};
	float horizon_radius{2.0f};
	float horizon_angular_velocity{0.0f};
	float inner_radius{2.2f};
	float outer_radius{150.0f};
	float footpoint_edge{1.3089969f};
	float footpoint_softness{0.2617994f};
	float lorentz_inner{1.3f};
	float lorentz_max{6.0f};
	float acceleration_radius{40.0f};
	float acceleration_index{1.0f};
	float doppler_enabled{1.0f};
	float redshift_enabled{1.0f};
	float emission_model{0.0f};
	float power_law_index{2.5f};
	float gamma_min{10.0f};
	float non_thermal_fraction{0.02f};
	float electron_temperature_k{3.0e10f};
	float temperature_index{0.5f};
	float observing_frequency_hz{2.3e11f};
	float meters_per_unit{1.477e4f};
	float log_emissivity_constant{0.0f};
	float log_absorptivity_constant{0.0f};
	float faraday_constant{0.0f};
	float polarization_fraction{0.5f};
	float polarization_coherence{0.6f};
	float absorption_scale{1.0f};
	float faraday_scale{1.0f};
	float log_reference_intensity{0.0f};
	float brightness{1.0f};
	float tint_r{0.35f};
	float tint_g{0.6f};
	float tint_b{1.0f};
	float display_mode{0.0f};
	float sampling_density{1.0f};
	float max_samples{24.0f};
	float reserved{0.0f};

	[[nodiscard]] bool operator==(const GpuJetProfile&) const noexcept = default;
};

static_assert(sizeof(GpuJetProfile) == 200);

inline constexpr size_t kMaxDarkMatterHalos = 64;

namespace DarkMatterHaloFlags {
	static constexpr uint32_t ENABLED = 1U << 0;
	static constexpr uint32_t LENSING = 1U << 1;
	static constexpr uint32_t VISUAL = 1U << 2;
}

namespace DarkMatterFieldFlags {
	static constexpr uint32_t LENSING = 1U << 0;
	static constexpr uint32_t VISUALIZATION = 1U << 1;
}

struct GpuDarkMatterHalo {
	uint32_t profile{0};
	uint32_t flags{0};
	float position_x{0.0f};
	float position_y{0.0f};
	float position_z{0.0f};
	float mass_norm{0.0f};
	float scale_radius{1.0f};
	float shape{0.0f};
	float softening{0.0f};
	float truncation_radius{0.0f};
	float axis_ratio_y{1.0f};
	float axis_ratio_z{1.0f};
	std::array<float, 9> basis{1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};
	float lens_scale{1.0f};
	float visual_gain{1.0f};
	float tint_r{0.55f};
	float tint_g{0.4f};
	float tint_b{1.0f};
	float shape_aux{0.0f};
	float reserved0{0.0f};

	[[nodiscard]] bool operator==(const GpuDarkMatterHalo&) const noexcept = default;
};

static_assert(sizeof(GpuDarkMatterHalo) == 112);

struct GpuDarkMatterField {
	uint32_t halo_count{0};
	uint32_t flags{0};
	float lensing_strength{1.0f};
	float visual_intensity{0.25f};
	float step_fraction{0.35f};
	float visual_mode{0.0f};
	float reserved1{0.0f};
	float reserved2{0.0f};
	std::array<GpuDarkMatterHalo, kMaxDarkMatterHalos> halos{};

	[[nodiscard]] bool operator==(const GpuDarkMatterField&) const noexcept = default;
};

static_assert(sizeof(GpuDarkMatterField) == 32 + kMaxDarkMatterHalos * 112);

inline constexpr size_t kCosmologyTableSize = 192;

struct GpuCosmologyProfile {
	uint32_t table_size{0};
	float table_log_a_min{-6.9077553f};
	float table_log_a_max{1.3862944f};
	float hubble0{0.0f};
	float ltb_hubble0{0.005f};
	float ltb_void_radius{60.0f};
	float ltb_void_width{25.0f};
	float ltb_omega_inner{0.2f};
	float ltb_omega_outer{1.0f};
	float ltb_present_time{0.0f};
	float evaporation_rate{0.0f};
	float reserved{0.0f};
	std::array<float, kCosmologyTableSize> log_hubble{};

	[[nodiscard]] bool operator==(const GpuCosmologyProfile&) const noexcept = default;
};

static_assert(sizeof(GpuCosmologyProfile) == 48 + kCosmologyTableSize * 4);

struct GpuSkyProfile {
	uint32_t enabled{1};
	uint32_t seed{56U};
	uint32_t stars_enabled{1};
	uint32_t star_profile_model{0};
	uint32_t star_placement_model{3};
	uint32_t star_size_distribution{1};
	uint32_t star_luminosity_distribution{1};
	uint32_t star_spike_count{4};
	uint32_t nebula_enabled{1};
	uint32_t nebula_octaves{5};
	uint32_t nebula_layer_count{3};
	uint32_t clusters_enabled{0};
	uint32_t cluster_placement_model{0};
	uint32_t cluster_size_distribution{2};
	uint32_t cluster_stars_enabled{1};
	uint32_t cluster_member_count{48};
	uint32_t cluster_star_profile_model{0};
	uint32_t cluster_inherit_star_profile{1};
	uint32_t dust_enabled{1};
	uint32_t dust_fractal_octaves{5};
	uint32_t galaxies_enabled{1};
	uint32_t galaxy_size_distribution{1};
	uint32_t galaxy_morphology_model{0};
	uint32_t exotic_objects_enabled{0};
	uint32_t quasar_enabled{1};
	uint32_t pulsar_enabled{1};
	uint32_t snr_enabled{1};
	uint32_t grid_enabled{0};
	uint32_t grid_cell_fill_mode{0};
	uint32_t grid_subdivisions{1};
	uint32_t grid_show_polar_caps{0};
	uint32_t grid_show_equator{1};
	uint32_t grid_show_meridians{0};
	uint32_t grid_show_ticks{0};
	uint32_t dynamic_motion{0};
	uint32_t dynamic_rotation{0};
	uint32_t adaptive_fov_quality{1};
	uint32_t max_star_octaves{8};

	double master_intensity{1.0};
	double quality_factor{1.0};
	double star_quality_factor{1.0};
	double nebula_quality_factor{1.0};
	double cluster_quality_factor{1.0};
	double dust_quality_factor{1.0};
	double exotic_quality_factor{1.0};

	double adaptive_fov_reference{90.0};
	double adaptive_fov_sensitivity{1.0};
	double time_seconds{0.0};
	double motion_speed{0.05};
	double rotation_rate_deg_s{0.01};

	double star_density{91.064};
	double star_brightness{1.81};
	double star_size_scale{1.0};
	double star_size_min{5e-05};
	double star_size_max{0.00263027};
	double star_size_param{3.96};
	double star_luminosity_param{1.84};
	double star_color_variation{1.0};
	double star_temperature_bias{0.32};
	double star_brightness_variation{0.5};
	double star_scintillation{0.0};
	double star_magnitude_limit{21.0};

	double star_profile_param{2.5};
	double star_profile_param2{1.0};
	double star_spike_length{4.0};
	double star_spike_intensity{0.6};
	double star_spike_angle_deg{45.0};
	double star_profile_softness{0.15};
	double star_dirac_concentration{0.95};
	double star_bloom_radius{3.0};
	double star_bloom_intensity{0.5};
	double star_galactic_concentration{2.5};

	double nebula_intensity{0.23};
	double nebula_roughness{0.76};
	double nebula_frequency{2.82};
	double nebula_plane_thickness{0.174};
	double nebula_plane_falloff{4.09};
	double nebula_asymmetry{0.43};
	double nebula_absorption_density{1.35};
	double nebula_layer_spread{0.69};
	double nebula_core_glow{0.78};
	double nebula_curl_strength{0.46};
	double nebula_color_separation{1.6};
	double nebula_ionization_glow{0.85};
	double nebula_gas_contrast{1.94};

	double cluster_density{0.8};
	double cluster_brightness{1.0};
	double cluster_size_scale{0.6};
	double cluster_size_min{0.002};
	double cluster_size_max{0.03};
	double cluster_concentration{1.6};
	double cluster_star_density_boost{2.0};
	double cluster_halo_radius{0.8};
	double cluster_plane_scale_height{0.15};
	double cluster_member_size{0.0006};
	double cluster_member_brightness{1.2};
	double cluster_halo_intensity{0.25};
	double cluster_spread_factor{1.0};

	double dust_density{3.85};
	double dust_intensity{1.3};
	double dust_scale{3.82};
	double dust_absorption_ratio{0.8};
	double dust_contrast{3.54};

	double galaxy_density{0.12};
	double galaxy_brightness{0.5};
	double galaxy_size_scale{0.32};
	double galaxy_size_min{0.001};
	double galaxy_size_max{0.02};
	double galaxy_size_param{1.5};
	double galaxy_spiral_arms{2.0};
	double galaxy_pitch_angle{1.2};
	double galaxy_core_ratio{0.4};
	double galaxy_ellipticity_variance{0.5};
	double galaxy_bar_strength{0.5};
	double galaxy_dust_lane_opacity{0.6};
	double galaxy_clumpiness{0.5};
	double galaxy_disk_thickness{0.2};
	double galaxy_inclination_variance{0.7};
	double galaxy_bulge_sersic_index{4.0};

	double quasar_density{0.01};
	double quasar_brightness{1.2};
	double quasar_core_size{0.0003};
	double quasar_jet_length{0.08};
	double quasar_jet_intensity{1.5};
	double quasar_jet_opening_angle_deg{5.0};
	double quasar_jet_lorentz_factor{4.0};
	double quasar_accretion_disk_glow{1.0};
	double quasar_precession_angle_deg{10.0};

	double pulsar_density{0.01};
	double pulsar_brightness{1.0};
	double pulsar_pulse_rate{2.5};
	double pulsar_pulse_duty{0.15};
	double pulsar_beam_width_deg{8.0};
	double pulsar_magnetic_tilt_deg{30.0};
	double pulsar_wind_nebula_scale{0.003};
	double pulsar_wind_nebula_intensity{0.4};

	double snr_density{0.06};
	double snr_brightness{1.2};
	double snr_shell_thickness{0.08};
	double snr_expansion_age{0.5};
	double snr_size{0.015};
	double snr_asymmetry{0.4};
	double snr_filament_scale{3.0};
	double snr_interior_glow{0.3};

	double grid_opacity{0.5};
	double grid_line_width{0.0001};
	double grid_spacing_deg{30.0};
	double grid_glow{0.0};
	double grid_tick_interval_deg{5.0};
	double grid_tick_length{0.003};
	double grid_tick_width{0.0001};
	double grid_cell_opacity{0.0};
	double grid_equator_width{0.0003};
	double grid_equator_opacity{0.8};
	double grid_polar_width{0.0002};
	double grid_polar_opacity{0.7};
	double grid_meridian_width{0.0003};
	double grid_meridian_opacity{0.8};
	double grid_major_width{0.00015};
	double grid_minor_width{0.00008};

	double hue_shift_deg{0.0};
	double saturation{0.86};
	double orientation_yaw_deg{0.0};
	double orientation_pitch_deg{0.0};
	double orientation_roll_deg{0.0};

	float nebula_plane_normal[3]{0.0f, 0.4472136f, 0.8944272f};
	float pad1{0.0f};
	float nebula_asymmetry_dir[3]{0.7071068f, 0.0f, 0.7071068f};
	float pad2{0.0f};
	float nebula_emission_color[3]{0.92f, 0.22f, 0.38f};
	float pad3{0.0f};
	float nebula_secondary_color[3]{0.15f, 0.72f, 0.90f};
	float pad4{0.0f};
	float nebula_tertiary_color[3]{0.95f, 0.65f, 0.15f};
	float pad5{0.0f};
	float cluster_core_tint[3]{1.1f, 1.0f, 0.8f};
	float pad6{0.0f};
	float dust_tint[3]{0.4f, 0.25f, 0.15f};
	float pad7{0.0f};
	float grid_major_color[3]{0.40f, 0.75f, 1.0f};
	float pad8{0.0f};
	float grid_minor_color[3]{0.20f, 0.45f, 0.80f};
	float pad9{0.0f};
	float background_color[3]{0.0f, 0.0f, 0.0f};
	float pad10{0.0f};

	float grid_equator_color[3]{0.95f, 0.85f, 0.30f};
	float pad11{0.0f};
	float grid_polar_color[3]{0.35f, 0.85f, 0.95f};
	float pad12{0.0f};
	float grid_meridian_color[3]{0.95f, 0.45f, 0.35f};
	float pad13{0.0f};
	float grid_tick_color[3]{1.0f, 1.0f, 1.0f};
	float pad14{0.0f};
	float grid_cell_color1[3]{0.10f, 0.12f, 0.18f};
	float pad15{0.0f};
	float grid_cell_color2[3]{0.05f, 0.07f, 0.12f};
	float pad16{0.0f};
	float grid_cell_color3[3]{0.08f, 0.15f, 0.15f};
	float pad17{0.0f};
	float grid_cell_color4[3]{0.15f, 0.10f, 0.12f};
	float pad18{0.0f};

	[[nodiscard]] static GpuSkyProfile from(const ProceduralSkySettings& s) noexcept {
		GpuSkyProfile p{};
		p.enabled = s.enabled ? 1U : 0U;
		p.seed = s.seed;
		p.stars_enabled = s.stars_enabled ? 1U : 0U;
		p.star_profile_model = s.star_profile_model;
		p.star_placement_model = s.star_placement_model;
		p.star_size_distribution = s.star_size_distribution;
		p.star_luminosity_distribution = s.star_luminosity_distribution;
		p.star_spike_count = s.star_spike_count;
		p.nebula_enabled = s.nebula_enabled ? 1U : 0U;
		p.nebula_octaves = s.nebula_octaves;
		p.nebula_layer_count = s.nebula_layer_count;
		p.clusters_enabled = s.clusters_enabled ? 1U : 0U;
		p.cluster_placement_model = s.cluster_placement_model;
		p.cluster_size_distribution = s.cluster_size_distribution;
		p.cluster_stars_enabled = s.cluster_stars_enabled ? 1U : 0U;
		p.cluster_member_count = s.cluster_member_count;
		p.cluster_star_profile_model = s.cluster_star_profile_model;
		p.cluster_inherit_star_profile = s.cluster_inherit_star_profile ? 1U : 0U;
		p.dust_enabled = s.dust_enabled ? 1U : 0U;
		p.dust_fractal_octaves = s.dust_fractal_octaves;
		p.galaxies_enabled = s.galaxies_enabled ? 1U : 0U;
		p.galaxy_size_distribution = s.galaxy_size_distribution;
		p.galaxy_morphology_model = s.galaxy_morphology_model;
		p.exotic_objects_enabled = s.exotic_objects_enabled ? 1U : 0U;
		p.quasar_enabled = s.quasar_enabled ? 1U : 0U;
		p.pulsar_enabled = s.pulsar_enabled ? 1U : 0U;
		p.snr_enabled = s.snr_enabled ? 1U : 0U;
		p.grid_enabled = s.grid_enabled ? 1U : 0U;
		p.grid_cell_fill_mode = s.grid_cell_fill_mode;
		p.grid_subdivisions = s.grid_subdivisions;
		p.grid_show_polar_caps = s.grid_show_polar_caps ? 1U : 0U;
		p.grid_show_equator = s.grid_show_equator ? 1U : 0U;
		p.grid_show_meridians = s.grid_show_meridians ? 1U : 0U;
		p.grid_show_ticks = s.grid_show_ticks ? 1U : 0U;
		p.dynamic_motion = s.dynamic_motion ? 1U : 0U;
		p.dynamic_rotation = s.dynamic_rotation ? 1U : 0U;
		p.adaptive_fov_quality = s.adaptive_fov_quality ? 1U : 0U;
		p.max_star_octaves = s.max_star_octaves;

		p.master_intensity = s.master_intensity;
		p.quality_factor = s.quality_factor;
		p.star_quality_factor = s.star_quality_factor;
		p.nebula_quality_factor = s.nebula_quality_factor;
		p.cluster_quality_factor = s.cluster_quality_factor;
		p.dust_quality_factor = s.dust_quality_factor;
		p.exotic_quality_factor = s.exotic_quality_factor;

		p.adaptive_fov_reference = s.adaptive_fov_reference;
		p.adaptive_fov_sensitivity = s.adaptive_fov_sensitivity;
		p.time_seconds = s.time_seconds;
		p.motion_speed = s.motion_speed;
		p.rotation_rate_deg_s = s.rotation_rate_deg_s;

		p.star_density = s.star_density;
		p.star_brightness = s.star_brightness;
		p.star_size_scale = s.star_size_scale;
		p.star_size_min = s.star_size_min;
		p.star_size_max = s.star_size_max;
		p.star_size_param = s.star_size_param;
		p.star_luminosity_param = s.star_luminosity_param;
		p.star_color_variation = s.star_color_variation;
		p.star_temperature_bias = s.star_temperature_bias;
		p.star_brightness_variation = s.star_brightness_variation;
		p.star_scintillation = s.star_scintillation;
		p.star_magnitude_limit = s.star_magnitude_limit;

		p.star_profile_param = s.star_profile_param;
		p.star_profile_param2 = s.star_profile_param2;
		p.star_spike_length = s.star_spike_length;
		p.star_spike_intensity = s.star_spike_intensity;
		p.star_spike_angle_deg = s.star_spike_angle_deg;
		p.star_profile_softness = s.star_profile_softness;
		p.star_dirac_concentration = s.star_dirac_concentration;
		p.star_bloom_radius = s.star_bloom_radius;
		p.star_bloom_intensity = s.star_bloom_intensity;
		p.star_galactic_concentration = s.star_galactic_concentration;

		p.nebula_intensity = s.nebula_intensity;
		p.nebula_roughness = s.nebula_roughness;
		p.nebula_frequency = s.nebula_frequency;
		p.nebula_plane_thickness = s.nebula_plane_thickness;
		p.nebula_plane_falloff = s.nebula_plane_falloff;
		p.nebula_asymmetry = s.nebula_asymmetry;
		p.nebula_absorption_density = s.nebula_absorption_density;
		p.nebula_layer_spread = s.nebula_layer_spread;
		p.nebula_core_glow = s.nebula_core_glow;
		p.nebula_curl_strength = s.nebula_curl_strength;
		p.nebula_color_separation = s.nebula_color_separation;
		p.nebula_ionization_glow = s.nebula_ionization_glow;
		p.nebula_gas_contrast = s.nebula_gas_contrast;

		p.cluster_density = s.cluster_density;
		p.cluster_brightness = s.cluster_brightness;
		p.cluster_size_scale = s.cluster_size_scale;
		p.cluster_size_min = s.cluster_size_min;
		p.cluster_size_max = s.cluster_size_max;
		p.cluster_concentration = s.cluster_concentration;
		p.cluster_star_density_boost = s.cluster_star_density_boost;
		p.cluster_halo_radius = s.cluster_halo_radius;
		p.cluster_plane_scale_height = s.cluster_plane_scale_height;
		p.cluster_member_size = s.cluster_member_size;
		p.cluster_member_brightness = s.cluster_member_brightness;
		p.cluster_halo_intensity = s.cluster_halo_intensity;
		p.cluster_spread_factor = s.cluster_spread_factor;

		p.dust_density = s.dust_density;
		p.dust_intensity = s.dust_intensity;
		p.dust_scale = s.dust_scale;
		p.dust_absorption_ratio = s.dust_absorption_ratio;
		p.dust_contrast = s.dust_contrast;

		p.galaxy_density = s.galaxy_density;
		p.galaxy_brightness = s.galaxy_brightness;
		p.galaxy_size_scale = s.galaxy_size_scale;
		p.galaxy_size_min = s.galaxy_size_min;
		p.galaxy_size_max = s.galaxy_size_max;
		p.galaxy_size_param = s.galaxy_size_param;
		p.galaxy_spiral_arms = s.galaxy_spiral_arms;
		p.galaxy_pitch_angle = s.galaxy_pitch_angle;
		p.galaxy_core_ratio = s.galaxy_core_ratio;
		p.galaxy_ellipticity_variance = s.galaxy_ellipticity_variance;
		p.galaxy_bar_strength = s.galaxy_bar_strength;
		p.galaxy_dust_lane_opacity = s.galaxy_dust_lane_opacity;
		p.galaxy_clumpiness = s.galaxy_clumpiness;
		p.galaxy_disk_thickness = s.galaxy_disk_thickness;
		p.galaxy_inclination_variance = s.galaxy_inclination_variance;
		p.galaxy_bulge_sersic_index = s.galaxy_bulge_sersic_index;

		p.quasar_density = s.quasar_density;
		p.quasar_brightness = s.quasar_brightness;
		p.quasar_core_size = s.quasar_core_size;
		p.quasar_jet_length = s.quasar_jet_length;
		p.quasar_jet_intensity = s.quasar_jet_intensity;
		p.quasar_jet_opening_angle_deg = s.quasar_jet_opening_angle_deg;
		p.quasar_jet_lorentz_factor = s.quasar_jet_lorentz_factor;
		p.quasar_accretion_disk_glow = s.quasar_accretion_disk_glow;
		p.quasar_precession_angle_deg = s.quasar_precession_angle_deg;

		p.pulsar_density = s.pulsar_density;
		p.pulsar_brightness = s.pulsar_brightness;
		p.pulsar_pulse_rate = s.pulsar_pulse_rate;
		p.pulsar_pulse_duty = s.pulsar_pulse_duty;
		p.pulsar_beam_width_deg = s.pulsar_beam_width_deg;
		p.pulsar_magnetic_tilt_deg = s.pulsar_magnetic_tilt_deg;
		p.pulsar_wind_nebula_scale = s.pulsar_wind_nebula_scale;
		p.pulsar_wind_nebula_intensity = s.pulsar_wind_nebula_intensity;

		p.snr_density = s.snr_density;
		p.snr_brightness = s.snr_brightness;
		p.snr_shell_thickness = s.snr_shell_thickness;
		p.snr_expansion_age = s.snr_expansion_age;
		p.snr_size = s.snr_size;
		p.snr_asymmetry = s.snr_asymmetry;
		p.snr_filament_scale = s.snr_filament_scale;
		p.snr_interior_glow = s.snr_interior_glow;

		p.grid_opacity = s.grid_opacity;
		p.grid_line_width = s.grid_line_width;
		p.grid_spacing_deg = s.grid_spacing_deg;
		p.grid_glow = s.grid_glow;
		p.grid_tick_interval_deg = s.grid_tick_interval_deg;
		p.grid_tick_length = s.grid_tick_length;
		p.grid_tick_width = s.grid_tick_width;
		p.grid_cell_opacity = s.grid_cell_opacity;
		p.grid_equator_width = s.grid_equator_width;
		p.grid_equator_opacity = s.grid_equator_opacity;
		p.grid_polar_width = s.grid_polar_width;
		p.grid_polar_opacity = s.grid_polar_opacity;
		p.grid_meridian_width = s.grid_meridian_width;
		p.grid_meridian_opacity = s.grid_meridian_opacity;
		p.grid_major_width = s.grid_major_width;
		p.grid_minor_width = s.grid_minor_width;

		p.hue_shift_deg = s.hue_shift_deg;
		p.saturation = s.saturation;
		p.orientation_yaw_deg = s.orientation_yaw_deg;
		p.orientation_pitch_deg = s.orientation_pitch_deg;
		p.orientation_roll_deg = s.orientation_roll_deg;

		for (int i = 0; i < 3; ++i) {
			p.nebula_plane_normal[i] = static_cast<float>(s.nebula_plane_normal[i]);
			p.nebula_asymmetry_dir[i] = static_cast<float>(s.nebula_asymmetry_dir[i]);
			p.nebula_emission_color[i] = s.nebula_emission_color[i];
			p.nebula_secondary_color[i] = s.nebula_secondary_color[i];
			p.nebula_tertiary_color[i] = s.nebula_tertiary_color[i];
			p.cluster_core_tint[i] = s.cluster_core_tint[i];
			p.dust_tint[i] = s.dust_tint[i];
			p.grid_major_color[i] = s.grid_major_color[i];
			p.grid_minor_color[i] = s.grid_minor_color[i];
			p.background_color[i] = s.background_color[i];
			p.grid_equator_color[i] = s.grid_equator_color[i];
			p.grid_polar_color[i] = s.grid_polar_color[i];
			p.grid_meridian_color[i] = s.grid_meridian_color[i];
			p.grid_tick_color[i] = s.grid_tick_color[i];
			p.grid_cell_color1[i] = s.grid_cell_color1[i];
			p.grid_cell_color2[i] = s.grid_cell_color2[i];
			p.grid_cell_color3[i] = s.grid_cell_color3[i];
			p.grid_cell_color4[i] = s.grid_cell_color4[i];
		}
		return p;
	}

	[[nodiscard]] ProceduralSkySettings to_settings() const noexcept {
		ProceduralSkySettings s{};
		s.enabled = (enabled != 0U);
		s.seed = seed;
		s.stars_enabled = (stars_enabled != 0U);
		s.star_profile_model = star_profile_model;
		s.star_placement_model = star_placement_model;
		s.star_size_distribution = star_size_distribution;
		s.star_luminosity_distribution = star_luminosity_distribution;
		s.star_spike_count = star_spike_count;
		s.nebula_enabled = (nebula_enabled != 0U);
		s.nebula_octaves = nebula_octaves;
		s.nebula_layer_count = nebula_layer_count;
		s.clusters_enabled = (clusters_enabled != 0U);
		s.cluster_placement_model = cluster_placement_model;
		s.cluster_size_distribution = cluster_size_distribution;
		s.cluster_stars_enabled = (cluster_stars_enabled != 0U);
		s.cluster_member_count = cluster_member_count;
		s.cluster_star_profile_model = cluster_star_profile_model;
		s.cluster_inherit_star_profile = (cluster_inherit_star_profile != 0U);
		s.dust_enabled = (dust_enabled != 0U);
		s.dust_fractal_octaves = dust_fractal_octaves;
		s.galaxies_enabled = (galaxies_enabled != 0U);
		s.galaxy_size_distribution = galaxy_size_distribution;
		s.galaxy_morphology_model = galaxy_morphology_model;
		s.exotic_objects_enabled = (exotic_objects_enabled != 0U);
		s.quasar_enabled = (quasar_enabled != 0U);
		s.pulsar_enabled = (pulsar_enabled != 0U);
		s.snr_enabled = (snr_enabled != 0U);
		s.grid_enabled = (grid_enabled != 0U);
		s.grid_cell_fill_mode = grid_cell_fill_mode;
		s.grid_subdivisions = grid_subdivisions;
		s.grid_show_polar_caps = (grid_show_polar_caps != 0U);
		s.grid_show_equator = (grid_show_equator != 0U);
		s.grid_show_meridians = (grid_show_meridians != 0U);
		s.grid_show_ticks = (grid_show_ticks != 0U);
		s.dynamic_motion = (dynamic_motion != 0U);
		s.dynamic_rotation = (dynamic_rotation != 0U);
		s.adaptive_fov_quality = (adaptive_fov_quality != 0U);
		s.max_star_octaves = max_star_octaves;

		s.master_intensity = master_intensity;
		s.quality_factor = quality_factor;
		s.star_quality_factor = star_quality_factor;
		s.nebula_quality_factor = nebula_quality_factor;
		s.cluster_quality_factor = cluster_quality_factor;
		s.dust_quality_factor = dust_quality_factor;
		s.exotic_quality_factor = exotic_quality_factor;

		s.adaptive_fov_reference = adaptive_fov_reference;
		s.adaptive_fov_sensitivity = adaptive_fov_sensitivity;
		s.time_seconds = time_seconds;
		s.motion_speed = motion_speed;
		s.rotation_rate_deg_s = rotation_rate_deg_s;

		s.star_density = star_density;
		s.star_brightness = star_brightness;
		s.star_size_scale = star_size_scale;
		s.star_size_min = star_size_min;
		s.star_size_max = star_size_max;
		s.star_size_param = star_size_param;
		s.star_luminosity_param = star_luminosity_param;
		s.star_color_variation = star_color_variation;
		s.star_temperature_bias = star_temperature_bias;
		s.star_brightness_variation = star_brightness_variation;
		s.star_scintillation = star_scintillation;
		s.star_magnitude_limit = star_magnitude_limit;

		s.star_profile_param = star_profile_param;
		s.star_profile_param2 = star_profile_param2;
		s.star_spike_length = star_spike_length;
		s.star_spike_intensity = star_spike_intensity;
		s.star_spike_angle_deg = star_spike_angle_deg;
		s.star_profile_softness = star_profile_softness;
		s.star_dirac_concentration = star_dirac_concentration;
		s.star_bloom_radius = star_bloom_radius;
		s.star_bloom_intensity = star_bloom_intensity;
		s.star_galactic_concentration = star_galactic_concentration;

		s.nebula_intensity = nebula_intensity;
		s.nebula_roughness = nebula_roughness;
		s.nebula_frequency = nebula_frequency;
		s.nebula_plane_thickness = nebula_plane_thickness;
		s.nebula_plane_falloff = nebula_plane_falloff;
		s.nebula_asymmetry = nebula_asymmetry;
		s.nebula_absorption_density = nebula_absorption_density;
		s.nebula_layer_spread = nebula_layer_spread;
		s.nebula_core_glow = nebula_core_glow;
		s.nebula_curl_strength = nebula_curl_strength;
		s.nebula_color_separation = nebula_color_separation;
		s.nebula_ionization_glow = nebula_ionization_glow;
		s.nebula_gas_contrast = nebula_gas_contrast;

		s.cluster_density = cluster_density;
		s.cluster_brightness = cluster_brightness;
		s.cluster_size_scale = cluster_size_scale;
		s.cluster_size_min = cluster_size_min;
		s.cluster_size_max = cluster_size_max;
		s.cluster_concentration = cluster_concentration;
		s.cluster_star_density_boost = cluster_star_density_boost;
		s.cluster_halo_radius = cluster_halo_radius;
		s.cluster_plane_scale_height = cluster_plane_scale_height;
		s.cluster_member_size = cluster_member_size;
		s.cluster_member_brightness = cluster_member_brightness;
		s.cluster_halo_intensity = cluster_halo_intensity;
		s.cluster_spread_factor = cluster_spread_factor;

		s.dust_density = dust_density;
		s.dust_intensity = dust_intensity;
		s.dust_scale = dust_scale;
		s.dust_absorption_ratio = dust_absorption_ratio;
		s.dust_contrast = dust_contrast;

		s.galaxy_density = galaxy_density;
		s.galaxy_brightness = galaxy_brightness;
		s.galaxy_size_scale = galaxy_size_scale;
		s.galaxy_size_min = galaxy_size_min;
		s.galaxy_size_max = galaxy_size_max;
		s.galaxy_size_param = galaxy_size_param;
		s.galaxy_spiral_arms = galaxy_spiral_arms;
		s.galaxy_pitch_angle = galaxy_pitch_angle;
		s.galaxy_core_ratio = galaxy_core_ratio;
		s.galaxy_ellipticity_variance = galaxy_ellipticity_variance;
		s.galaxy_bar_strength = galaxy_bar_strength;
		s.galaxy_dust_lane_opacity = galaxy_dust_lane_opacity;
		s.galaxy_clumpiness = galaxy_clumpiness;
		s.galaxy_disk_thickness = galaxy_disk_thickness;
		s.galaxy_inclination_variance = galaxy_inclination_variance;
		s.galaxy_bulge_sersic_index = galaxy_bulge_sersic_index;

		s.quasar_density = quasar_density;
		s.quasar_brightness = quasar_brightness;
		s.quasar_core_size = quasar_core_size;
		s.quasar_jet_length = quasar_jet_length;
		s.quasar_jet_intensity = quasar_jet_intensity;
		s.quasar_jet_opening_angle_deg = quasar_jet_opening_angle_deg;
		s.quasar_jet_lorentz_factor = quasar_jet_lorentz_factor;
		s.quasar_accretion_disk_glow = quasar_accretion_disk_glow;
		s.quasar_precession_angle_deg = quasar_precession_angle_deg;

		s.pulsar_density = pulsar_density;
		s.pulsar_brightness = pulsar_brightness;
		s.pulsar_pulse_rate = pulsar_pulse_rate;
		s.pulsar_pulse_duty = pulsar_pulse_duty;
		s.pulsar_beam_width_deg = pulsar_beam_width_deg;
		s.pulsar_magnetic_tilt_deg = pulsar_magnetic_tilt_deg;
		s.pulsar_wind_nebula_scale = pulsar_wind_nebula_scale;
		s.pulsar_wind_nebula_intensity = pulsar_wind_nebula_intensity;

		s.snr_density = snr_density;
		s.snr_brightness = snr_brightness;
		s.snr_shell_thickness = snr_shell_thickness;
		s.snr_expansion_age = snr_expansion_age;
		s.snr_size = snr_size;
		s.snr_asymmetry = snr_asymmetry;
		s.snr_filament_scale = snr_filament_scale;
		s.snr_interior_glow = snr_interior_glow;

		s.grid_opacity = grid_opacity;
		s.grid_line_width = grid_line_width;
		s.grid_spacing_deg = grid_spacing_deg;
		s.grid_glow = grid_glow;
		s.grid_tick_interval_deg = grid_tick_interval_deg;
		s.grid_tick_length = grid_tick_length;
		s.grid_tick_width = grid_tick_width;
		s.grid_cell_opacity = grid_cell_opacity;
		s.grid_equator_width = grid_equator_width;
		s.grid_equator_opacity = grid_equator_opacity;
		s.grid_polar_width = grid_polar_width;
		s.grid_polar_opacity = grid_polar_opacity;
		s.grid_meridian_width = grid_meridian_width;
		s.grid_meridian_opacity = grid_meridian_opacity;
		s.grid_major_width = grid_major_width;
		s.grid_minor_width = grid_minor_width;

		s.hue_shift_deg = hue_shift_deg;
		s.saturation = saturation;
		s.orientation_yaw_deg = orientation_yaw_deg;
		s.orientation_pitch_deg = orientation_pitch_deg;
		s.orientation_roll_deg = orientation_roll_deg;

		for (int i = 0; i < 3; ++i) {
			s.nebula_plane_normal[i] = static_cast<double>(nebula_plane_normal[i]);
			s.nebula_asymmetry_dir[i] = static_cast<double>(nebula_asymmetry_dir[i]);
			s.nebula_emission_color[i] = nebula_emission_color[i];
			s.nebula_secondary_color[i] = nebula_secondary_color[i];
			s.nebula_tertiary_color[i] = nebula_tertiary_color[i];
			s.cluster_core_tint[i] = cluster_core_tint[i];
			s.dust_tint[i] = dust_tint[i];
			s.grid_major_color[i] = grid_major_color[i];
			s.grid_minor_color[i] = grid_minor_color[i];
			s.background_color[i] = background_color[i];
			s.grid_equator_color[i] = grid_equator_color[i];
			s.grid_polar_color[i] = grid_polar_color[i];
			s.grid_meridian_color[i] = grid_meridian_color[i];
			s.grid_tick_color[i] = grid_tick_color[i];
			s.grid_cell_color1[i] = grid_cell_color1[i];
			s.grid_cell_color2[i] = grid_cell_color2[i];
			s.grid_cell_color3[i] = grid_cell_color3[i];
			s.grid_cell_color4[i] = grid_cell_color4[i];
		}
		return s;
	}

	operator ProceduralSkySettings() const noexcept {
		return to_settings();
	}

	[[nodiscard]] bool operator==(const GpuSkyProfile&) const noexcept = default;
};

static_assert(sizeof(GpuSkyProfile) == 1456);

struct alignas(16) GpuCameraPushConstants {
	std::array<double, 4> observer_position{};
	std::array<double, 4> tetrad_e0{};
	std::array<double, 4> tetrad_e1{};
	std::array<double, 4> tetrad_e2{};
	std::array<double, 4> tetrad_e3{};

	double field_of_view_rad{1.0471975511965976};
	double metric_mass{1.0};
	double metric_spin{0.0};
	double metric_charge{0.0};

	double speed_of_light{1.0};
	double gravitational_constant{1.0};
	double initial_step_size{-0.05};
	double min_step_size{0.004};

	double max_step_size{3.5};
	double horizon_radius{2.0};
	double escape_radius{100.0};
	double cosmological_lambda{0.0};

	double wormhole_throat{1.0};
	double warp_velocity{1.0};
	double camera_exposure{0.0};
	double warp_bubble_radius{15.0};
	double warp_wall_sharpness{0.4};
	double wormhole_tidal_potential{0.0};
	double teves_critical_acceleration{1.0e-3};

	double lod_distance_threshold{0.0};

	double sky_rotation_rad{56.0};
	double sky_hue_shift_rad{-15.7};
	double sky_saturation{0.84};
	double sky_star_density{0.78};
	double sky_star_brightness{1.0};
	double sky_nebula_intensity{0.87};
	double sky_grid_opacity{1.0};
	double sky_background_r{0.0};
	double sky_background_g{0.0};
	double sky_background_b{0.0};
	double sky_star_brightness_variation{0.67};
	double sky_star_size_variation{0.54};
	double sky_star_color_variation{1.11};
	double sky_star_temperature_bias{0.39};

	double sky_galaxy_density{0.09};
	double sky_galaxy_brightness{0.42};
	double sky_galaxy_size_scale{0.2};
	double sky_dust_density{3.05};
	double sky_dust_intensity{2.13};
	double sky_dust_scale{1.55};
	double sky_cluster_density{1.06};
	double sky_cluster_brightness{1.0};
	double sky_cluster_size_scale{0.74};

	double space_skip_radius_scale{140.0};
	double pole_guard_precision_scale{4.0};
	double far_field_step_scale{7.8};
	double time{0.0};
	double body_atmosphere_global_intensity{1.0};

	double disk_temperature_scale_k{23796.0};
	double disk_temperature_floor_k{1200.0};
	double disk_doppler_beaming_exponent{5.32};
	double disk_color_saturation{1.0};

	double step_size_factor{0.5};

	uint32_t screen_width{3840};
	uint32_t screen_height{2160};
	uint32_t metric_type{1};
	uint32_t precision_mode{0};

	uint32_t tonemapping_mode{0};

	uint32_t max_integration_steps{2048};
	uint32_t render_flags{0};
	uint32_t projection_mode{3};
	uint32_t interlace_mode{0};

	uint32_t lod_reduced_steps{256};
	uint32_t interlace_phase{0};

	uint32_t sky_procedural_seed{12345};
	uint32_t sky_background_source{0};
	uint32_t sky_panorama_id{0};
	uint32_t sky_panorama_quality{1};
	uint32_t sky_panorama_width{0};
	uint32_t sky_panorama_height{0};

	uint32_t body_render_lod_pixel_threshold{10};
	uint32_t body_render_low_power_mode{0};
	uint32_t body_count{0};
	uint32_t body_noise_octaves{4};
	uint32_t body_render_point_pixel_threshold{2};

	uint32_t dispatch_row_offset{0};
	uint32_t dispatch_row_count{0};

	uint32_t earth_day_width{0};
	uint32_t earth_night_width{0};

	uint32_t light_source_mode{0};
	uint32_t light_attenuation_mode{0};
	uint32_t light_source_body_id{0};
	uint32_t body_emission_lighting_enabled{0};
	uint32_t body_lighting_detail{2};
	uint32_t body_shadow_caster_limit{32};
	uint32_t body_emission_source_limit{8};

	float light_intensity{1.0f};
	float light_color_r{1.0f};
	float light_color_g{1.0f};
	float light_color_b{1.0f};
	float light_ambient{0.04f};
	float light_terminator_softness{0.12f};
	float light_specular_scale{1.0f};
	float light_direction_x{0.5f};
	float light_direction_y{0.5f};
	float light_direction_z{0.7f};
	float light_position_x{100.0f};
	float light_position_y{0.0f};
	float light_position_z{0.0f};
	float light_reference_distance{50.0f};
	float body_emission_lighting_gain{1.0f};
	float body_emission_lighting_reference_distance{30.0f};

	GpuDiskProfile primary_disk{};
	GpuHydroDiskProfile hydro_disk{};
	GpuJetProfile jet{};
	GpuDarkMatterField dark_matter{};
	GpuCosmologyProfile cosmology{};
	std::array<double, 4> observer_boost{0.0, 0.0, 0.0, 0.0};

	uint32_t tile_origin_x{0};
	uint32_t tile_origin_y{0};
	uint32_t tile_extent_x{0};
	uint32_t tile_extent_y{0};
	GpuSkyProfile sky{};

	[[nodiscard]] bool operator==(const GpuCameraPushConstants&) const noexcept = default;
};

struct alignas(16) GpuPixelOutput {
	float r{0.0f};
	float g{0.0f};
	float b{0.0f};
	float a{1.0f};

	float redshift{1.0f};
	float affine_parameter{0.0f};
	uint32_t status_flags{0};
	uint32_t iterations_used{0};
};

namespace PixelFlags {
	static constexpr uint32_t HORIZON_ABSORBED = 1U << 0;
	static constexpr uint32_t CELESTIAL_HIT = 1U << 1;
	static constexpr uint32_t ACCRETION_DISK_HIT = 1U << 2;
	static constexpr uint32_t PHOTON_SPHERE_PROXIMITY = 1U << 3;
	static constexpr uint32_t BODY_SURFACE_HIT = 1U << 4;
	static constexpr uint32_t JET_EMISSION_HIT = 1U << 5;
}

namespace RenderFlags {
	static constexpr uint32_t SKYBOX_STARS = 0U;
	static constexpr uint32_t SKYBOX_GRID = 1U;
	static constexpr uint32_t SKYBOX_COMPOSITE = 2U;
	static constexpr uint32_t SKYBOX_VOID = 3U;
	static constexpr uint32_t SKYBOX_STARS_NO_NEBULA = 4U;
	static constexpr uint32_t SKYBOX_GRID_STARS = 5U;
	static constexpr uint32_t SKYBOX_MODE_MASK = 0x0FU;
	static constexpr uint32_t USE_GRID_SKYBOX = 1U << 4;
	static constexpr uint32_t USE_SCALAR_PIPELINE = 1U << 5;
	static constexpr uint32_t USE_PER_FRAME_THREADS = 1U << 6;
	static constexpr uint32_t USE_TILED_DISTRIBUTION = 1U << 7;
	static constexpr uint32_t FORCE_TEXTURE_REALLOCATION = 1U << 8;
	static constexpr uint32_t USE_LOD_SYSTEM = 1U << 9;
	static constexpr uint32_t SPACE_SKIP_ENABLED = 1U << 10;
	static constexpr uint32_t ADAPTIVE_TILE_PREPASS = 1U << 11;
	static constexpr uint32_t ENABLE_3D_BODY_RAYTRACING = 1U << 12;
	static constexpr uint32_t ENABLE_BODY_DOPPLER_BEAMING = 1U << 13;
	static constexpr uint32_t ENABLE_BODY_GRAV_REDSHIFT = 1U << 14;
	static constexpr uint32_t ENABLE_ATMOSPHERE_SCATTERING = 1U << 15;
	static constexpr uint32_t ENABLE_BODY_SHADOWS = 1U << 16;
	static constexpr uint32_t BODIES_ONLY_MODE = 1U << 17;
	static constexpr uint32_t ENABLE_BODY_DISK_OCCLUSION = 1U << 18;
}

inline constexpr size_t kMaxSurfaceLayers = 4;
inline constexpr uint32_t kBeaconBodyIdBase = 0xFFFF0000U;

enum class SurfaceLayerPattern : uint32_t {
	Noise = 0,
	Ridged = 1,
	Billow = 2,
	Bands = 3,
	Meridians = 4,
	Speckle = 5,
	Craters = 6,
	Cracks = 7,
	Swirl = 8,
	Clouds = 9
};

enum class SurfaceLayerBlend : uint32_t {
	Mix = 0,
	Add = 1,
	Multiply = 2,
	Screen = 3,
	Overlay = 4
};

enum class SurfaceLayerMask : uint32_t {
	Global = 0,
	PolarCaps = 1,
	EquatorialBand = 2,
	NorthernHemisphere = 3,
	SouthernHemisphere = 4,
	DaySide = 5,
	NightSide = 6
};

struct alignas(16) GpuSurfaceLayer {
	std::array<float, 4> color{1.0f, 1.0f, 1.0f, 1.0f};
	float opacity{0.0f};
	float scale{6.0f};
	float contrast{1.0f};
	float threshold{0.35f};
	float softness{0.3f};
	float mask_width{0.35f};
	float rotation_factor{0.0f};
	float emission{0.0f};
	uint32_t pattern{0};
	uint32_t blend{0};
	uint32_t mask{0};
	uint32_t seed{1};
	uint32_t enabled{0};
	uint32_t octaves{3};
	uint32_t reserved0{0};
	uint32_t reserved1{0};
};

static_assert(sizeof(GpuSurfaceLayer) == 80);

struct alignas(16) GpuBodyData {
	std::array<double, 4> position{0.0, 0.0, 0.0, 0.0};
	std::array<double, 4> velocity{0.0, 0.0, 0.0, 0.0};
	std::array<double, 4> color_primary{0.62, 0.75, 1.0, 1.0};
	std::array<double, 4> color_secondary{0.18, 0.30, 0.75, 1.0};
	std::array<double, 4> atmosphere_color{0.3, 0.6, 1.0, 0.4};

	double radius{1.0};
	double mass{1.0};
	double charge{0.0};
	double temperature{5778.0};

	double noise_scale{4.0};
	double noise_roughness{0.5};
	double atmosphere_thickness{0.15};
	double specular_roughness{0.3};

	double emission_intensity{0.0};
	double rotation_speed{0.1};
	double oblateness_ratio{1.0};
	double spin_parameter{0.0};
	uint32_t body_id{0};

	uint32_t geometry_model{0};
	uint32_t surface_texture_mode{0};
	uint32_t atmosphere_mode{0};
	uint32_t preset_3d{0};

	std::array<double, 4> color_tertiary{0.9, 0.85, 0.6, 1.0};
	double texture_detail_scale{1.0};
	double polar_cap_strength{0.0};
	double ring_system_enabled{0.0};
	double night_side_light_intensity{0.0};
	uint32_t earth_map_variant{0};
	uint32_t earth_map_quality{0};
	float earth_terminator_softness{0.25f};
	std::array<double, 4> spin_axis{0.0, 0.0, 1.0, 0.0};
	std::array<double, 4> orientation{1.0, 0.0, 0.0, 0.0};

	uint32_t surface_layer_count{0};
	std::array<GpuSurfaceLayer, kMaxSurfaceLayers> surface_layers{};
	GpuDiskProfile disk{};
};

struct alignas(16) GpuBodyGpuLayout {
	double position_xy[2]{0.0, 0.0};
	double position_zw[2]{0.0, 0.0};
	double velocity_xy[2]{0.0, 0.0};
	double velocity_zw[2]{0.0, 0.0};
	double color_primary_xy[2]{0.0, 0.0};
	double color_primary_zw[2]{0.0, 0.0};
	double color_secondary_xy[2]{0.0, 0.0};
	double color_secondary_zw[2]{0.0, 0.0};
	double atmosphere_color_xy[2]{0.0, 0.0};
	double atmosphere_color_zw[2]{0.0, 0.0};
	double radius{1.0};
	double mass{1.0};
	double charge{0.0};
	double temperature{5778.0};
	double noise_scale{4.0};
	double noise_roughness{0.5};
	double atmosphere_thickness{0.15};
	double specular_roughness{0.3};
	double emission_intensity{0.0};
	double rotation_speed{0.1};
	double oblateness_ratio{1.0};
	double spin_parameter{0.0};
	uint32_t body_id{0};
	uint32_t geometry_model{0};
	uint32_t surface_texture_mode{0};
	uint32_t atmosphere_mode{0};
	uint32_t preset_3d{0};
	uint32_t earth_map_variant{0};
	double color_tertiary_r{0.9};
	double color_tertiary_g{0.85};
	double color_tertiary_b{0.6};
	double texture_detail_scale{1.0};
	double polar_cap_strength{0.0};
	double ring_system_enabled{0.0};
	double night_side_light_intensity{0.0};
	double spin_axis_x{0.0};
	double spin_axis_y{0.0};
	double spin_axis_z{1.0};
	double spin_axis_reserved{0.0};
	double orientation_w{1.0};
	double orientation_x{0.0};
	double orientation_y{0.0};
	double orientation_z{0.0};
	uint32_t surface_layer_count{0};
	uint32_t earth_map_quality{0};
	float earth_terminator_softness{0.25f};
	uint32_t layer_pad2{0};
	GpuSurfaceLayer surface_layers[kMaxSurfaceLayers]{};
	GpuDiskProfile disk{};

	[[nodiscard]] static GpuBodyGpuLayout from(const GpuBodyData& b) noexcept {
		GpuBodyGpuLayout g{};
		g.position_xy[0] = b.position[0]; g.position_xy[1] = b.position[1];
		g.position_zw[0] = b.position[2]; g.position_zw[1] = b.position[3];
		g.velocity_xy[0] = b.velocity[0]; g.velocity_xy[1] = b.velocity[1];
		g.velocity_zw[0] = b.velocity[2]; g.velocity_zw[1] = b.velocity[3];
		g.color_primary_xy[0] = b.color_primary[0]; g.color_primary_xy[1] = b.color_primary[1];
		g.color_primary_zw[0] = b.color_primary[2]; g.color_primary_zw[1] = b.color_primary[3];
		g.color_secondary_xy[0] = b.color_secondary[0]; g.color_secondary_xy[1] = b.color_secondary[1];
		g.color_secondary_zw[0] = b.color_secondary[2]; g.color_secondary_zw[1] = b.color_secondary[3];
		g.atmosphere_color_xy[0] = b.atmosphere_color[0]; g.atmosphere_color_xy[1] = b.atmosphere_color[1];
		g.atmosphere_color_zw[0] = b.atmosphere_color[2]; g.atmosphere_color_zw[1] = b.atmosphere_color[3];
		g.radius = b.radius;
		g.mass = b.mass;
		g.charge = b.charge;
		g.temperature = b.temperature;
		g.noise_scale = b.noise_scale;
		g.noise_roughness = b.noise_roughness;
		g.atmosphere_thickness = b.atmosphere_thickness;
		g.specular_roughness = b.specular_roughness;
		g.emission_intensity = b.emission_intensity;
		g.rotation_speed = b.rotation_speed;
		g.oblateness_ratio = b.oblateness_ratio;
		g.spin_parameter = b.spin_parameter;
		g.body_id = b.body_id;
		g.geometry_model = b.geometry_model;
		g.surface_texture_mode = b.surface_texture_mode;
		g.atmosphere_mode = b.atmosphere_mode;
		g.preset_3d = b.preset_3d;
		g.earth_map_variant = b.earth_map_variant;
		g.color_tertiary_r = b.color_tertiary[0];
		g.color_tertiary_g = b.color_tertiary[1];
		g.color_tertiary_b = b.color_tertiary[2];
		g.texture_detail_scale = b.texture_detail_scale;
		g.polar_cap_strength = b.polar_cap_strength;
		g.ring_system_enabled = b.ring_system_enabled;
		g.night_side_light_intensity = b.night_side_light_intensity;
		g.spin_axis_x = b.spin_axis[0];
		g.spin_axis_y = b.spin_axis[1];
		g.spin_axis_z = b.spin_axis[2];
		g.orientation_w = b.orientation[0];
		g.orientation_x = b.orientation[1];
		g.orientation_y = b.orientation[2];
		g.orientation_z = b.orientation[3];
		g.surface_layer_count = std::min<uint32_t>(b.surface_layer_count, static_cast<uint32_t>(kMaxSurfaceLayers));
		g.earth_map_quality = b.earth_map_quality;
		g.earth_terminator_softness = b.earth_terminator_softness;
		for (size_t i = 0; i < kMaxSurfaceLayers; ++i) {
			g.surface_layers[i] = b.surface_layers[i];
		}
		g.disk = b.disk;
		return g;
	}
};

static_assert(sizeof(GpuBodyGpuLayout) == 864);

}

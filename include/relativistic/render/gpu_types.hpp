#pragma once

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
	Alcubierre = 9
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

static_assert(sizeof(GpuBodyGpuLayout) == 832);

}

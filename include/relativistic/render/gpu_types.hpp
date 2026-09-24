#pragma once

#include <cstdint>
#include <cstddef>
#include <array>

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
	double min_step_size{1e-8};

	double max_step_size{1.0};
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

	double space_skip_radius_scale{40.0};
	double pole_guard_precision_scale{0.15};
	double far_field_step_scale{2.0};
	double time{0.0};
	double body_atmosphere_global_intensity{1.0};

	double disk_temperature_scale_k{23796.0};
	double disk_temperature_floor_k{1200.0};
	double disk_doppler_beaming_exponent{5.32};
	double disk_color_saturation{1.0};

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

	uint32_t body_render_lod_pixel_threshold{10};
	uint32_t body_render_low_power_mode{0};
	uint32_t body_count{0};
	uint32_t body_noise_octaves{4};
	uint32_t body_render_point_pixel_threshold{2};

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
	uint32_t body_id{0};
	uint32_t geometry_model{0};
	uint32_t surface_texture_mode{0};
	uint32_t atmosphere_mode{0};
	uint32_t preset_3d{0};
	uint32_t pad0{0};

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
		g.body_id = b.body_id;
		g.geometry_model = b.geometry_model;
		g.surface_texture_mode = b.surface_texture_mode;
		g.atmosphere_mode = b.atmosphere_mode;
		g.preset_3d = b.preset_3d;
		g.pad0 = 0U;
		return g;
	}
};

static_assert(sizeof(GpuBodyGpuLayout) == 272);

}

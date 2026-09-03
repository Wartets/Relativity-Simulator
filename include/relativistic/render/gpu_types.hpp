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

	uint32_t screen_width{3840};
	uint32_t screen_height{2160};
	uint32_t metric_type{1};
	uint32_t precision_mode{0};

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
	double warp_velocity{0.0};
	double camera_exposure{0.0};
	uint32_t tonemapping_mode{1};

	uint32_t max_integration_steps{2048};
	uint32_t render_flags{0};
	uint32_t projection_mode{0};
	uint32_t padding1{0};

	double lod_distance_threshold{0.0};
	uint32_t lod_reduced_steps{256};
	uint32_t padding2{0};

	double sky_rotation_rad{0.0};
	double sky_hue_shift_rad{0.0};
	double sky_saturation{1.0};
	double sky_star_density{1.0};
	double sky_star_brightness{1.0};
	double sky_nebula_intensity{1.0};
	double sky_grid_opacity{1.0};
	double sky_background_r{0.0};
	double sky_background_g{0.0};
	double sky_background_b{0.0};

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
}

}

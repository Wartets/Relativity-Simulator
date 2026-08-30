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

	uint32_t max_integration_steps{2048};
	uint32_t render_flags{0};
	uint32_t projection_mode{0};
	uint32_t padding1{0};
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

}

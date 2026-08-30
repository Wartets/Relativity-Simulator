#include "relativistic/render/geodesic_compute_pipeline.hpp"
#include "relativistic/observer/observer_tetrad.hpp"
#include "relativistic/metrics/schwarzschild.hpp"
#include <cassert>
#include <iostream>
#include <cmath>
#include <numbers>

int main() {
	using namespace Relativistic::Render;
	using namespace Relativistic::Observer;
	using namespace Relativistic::Metrics;
	using namespace Relativistic::Core;

	const double mass = 1.0;
	const double r_obs = 50.0;
	const SchwarzschildMetric<double> metric(mass);
	const FourVector<double> obs_pos(0.0, r_obs, std::numbers::pi_v<double> / 2.0, 0.0);
	const auto tetrad = ObserverTetrad<double>::make_stationary(metric, obs_pos);

	const double b_crit = 3.0 * std::sqrt(3.0) * mass;
	const double sin_theta_crit = (b_crit * std::sqrt(1.0 - 2.0 * mass / r_obs)) / r_obs;
	const double theta_crit_analytical = std::asin(sin_theta_crit);

	const uint32_t width = 128;
	const uint32_t height = 128;
	const double fov_rad = 0.4;

	GeodesicPipelineConfig config{
		.width = width,
		.height = height,
		.precision = PrecisionMode::NativeFloat64,
		.metric = MetricId::Schwarzschild,
		.field_of_view_deg = fov_rad * 180.0 / std::numbers::pi_v<double>,
		.max_steps = 512,
		.initial_step = -0.05,
		.headless = true
	};

	GeodesicComputePipeline pipeline(config);

	GpuCameraPushConstants constants{};
	constants.observer_position = {obs_pos(0), obs_pos(1), obs_pos(2), obs_pos(3)};
	for (size_t i = 0; i < 4; ++i) {
		constants.tetrad_e0[i] = tetrad.e(0)(i);
		constants.tetrad_e1[i] = tetrad.e(1)(i);
		constants.tetrad_e2[i] = tetrad.e(2)(i);
		constants.tetrad_e3[i] = tetrad.e(3)(i);
	}
	constants.screen_width = width;
	constants.screen_height = height;
	constants.field_of_view_rad = fov_rad;
	constants.metric_mass = mass;
	constants.horizon_radius = 2.0 * mass;
	constants.escape_radius = 100.0;
	constants.initial_step_size = -0.05;
	constants.max_integration_steps = 512;

	pipeline.dispatch(constants);

	const auto fb = pipeline.framebuffer();
	const size_t center_y = height / 2;

	size_t shadow_boundary_x = 0;
	for (size_t x = width / 2; x < width; ++x) {
		const size_t idx = center_y * width + x;
		if (fb[idx].status_flags == PixelFlags::CELESTIAL_HIT) {
			shadow_boundary_x = x;
			break;
		}
	}

	assert(shadow_boundary_x > width / 2);

	const double u_norm = ((static_cast<double>(shadow_boundary_x) + 0.5) / static_cast<double>(width) * 2.0 - 1.0);
	const double theta_num = std::atan(u_norm * std::tan(fov_rad * 0.5));
	const double rel_err = std::abs((theta_num - theta_crit_analytical) / theta_crit_analytical);

	assert(rel_err < 0.05);

	std::cout << "test_schwarzschild_shadow_render passed successfully. Relative error: " << rel_err << "\n";
	return 0;
}

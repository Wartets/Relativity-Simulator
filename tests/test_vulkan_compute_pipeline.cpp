#include "relativistic/render/geodesic_compute_pipeline.hpp"
#include "relativistic/observer/observer_tetrad.hpp"
#include "relativistic/metrics/schwarzschild.hpp"
#include <cassert>
#include <iostream>
#include <cmath>

int main() {
	using namespace Relativistic::Render;
	using namespace Relativistic::Observer;
	using namespace Relativistic::Metrics;
	using namespace Relativistic::Core;

	GeodesicPipelineConfig config{
		.width = 64,
		.height = 64,
		.precision = PrecisionMode::NativeFloat64,
		.metric = MetricId::Schwarzschild,
		.field_of_view_deg = 60.0,
		.max_steps = 1024,
		.initial_step = -0.05,
		.headless = true
	};

	GeodesicComputePipeline pipeline(config);
	assert(pipeline.context().is_initialized());
	assert(pipeline.context().supports_fp64());

	const SchwarzschildMetric<double> metric(1.0);
	const FourVector<double> obs_pos(0.0, 15.0, std::numbers::pi_v<double> / 2.0, 0.0);
	const auto tetrad = ObserverTetrad<double>::make_stationary(metric, obs_pos);

	GpuCameraPushConstants constants{};
	constants.observer_position = {obs_pos(0), obs_pos(1), obs_pos(2), obs_pos(3)};
	for (size_t i = 0; i < 4; ++i) {
		constants.tetrad_e0[i] = tetrad.e(0)(i);
		constants.tetrad_e1[i] = tetrad.e(1)(i);
		constants.tetrad_e2[i] = tetrad.e(2)(i);
		constants.tetrad_e3[i] = tetrad.e(3)(i);
	}
	constants.screen_width = config.width;
	constants.screen_height = config.height;
	constants.field_of_view_rad = config.field_of_view_deg * std::numbers::pi_v<double> / 180.0;
	constants.metric_mass = 1.0;
	constants.horizon_radius = 2.0;
	constants.escape_radius = 50.0;
	constants.initial_step_size = -0.05;
	constants.max_integration_steps = 1024;

	pipeline.dispatch(constants);
	const auto fb_fp64 = pipeline.framebuffer();
	assert(pipeline.telemetry().total_pixels_processed == 64 * 64);
	assert(pipeline.telemetry().horizon_pixels_absorbed > 0);
	assert(pipeline.telemetry().celestial_pixels_hit > 0);

	std::vector<GpuPixelOutput> fb_ds;
	fb_ds.resize(64 * 64);
	SoftwareComputeEngine::dispatch_double_single(constants, fb_ds);

	size_t matching_status = 0;
	for (size_t i = 0; i < fb_fp64.size(); ++i) {
		if (fb_fp64[i].status_flags == fb_ds[i].status_flags) {
			++matching_status;
		}
	}

	const double status_match_ratio = static_cast<double>(matching_status) / static_cast<double>(fb_fp64.size());
	assert(status_match_ratio > 0.98);

	FourVector<double> test_ray_x(0.0, 15.0, std::numbers::pi_v<double> / 2.0, 0.0);
	FourVector<double> test_ray_u = tetrad.construct_light_ray(1.0, 0.0, 0.4);

	std::array<DoubleSingle, 4> ds_ray_x{DoubleSingle(0.0), DoubleSingle(15.0), DoubleSingle(std::numbers::pi_v<double> / 2.0), DoubleSingle(0.0)};
	std::array<DoubleSingle, 4> ds_ray_u{DoubleSingle(test_ray_u(0)), DoubleSingle(test_ray_u(1)), DoubleSingle(test_ray_u(2)), DoubleSingle(test_ray_u(3))};

	const double rs_val = 2.0;
	for (int step = 0; step < 50; ++step) {
		const double dt_val = -0.02 * test_ray_x(1);
		const DoubleSingle ds_dt(dt_val);

		const double r_val = test_ray_x(1);
		const double r_diff = r_val - rs_val;
		const double g001 = rs_val / (2.0 * r_val * r_diff);
		const double g100 = (rs_val * r_diff) / (2.0 * r_val * r_val * r_val);
		const double g111 = -g001;
		const double g133 = -r_diff;
		const double g313 = 1.0 / r_val;

		const double acc_r = -(g100 * test_ray_u(0) * test_ray_u(0) + g111 * test_ray_u(1) * test_ray_u(1) + g133 * test_ray_u(3) * test_ray_u(3));
		const double acc_phi = -2.0 * g313 * test_ray_u(1) * test_ray_u(3);

		test_ray_x(1) += dt_val * test_ray_u(1);
		test_ray_x(3) += dt_val * test_ray_u(3);
		test_ray_u(1) += dt_val * acc_r;
		test_ray_u(3) += dt_val * acc_phi;

		const DoubleSingle ds_r = ds_ray_x[1];
		const DoubleSingle ds_r_diff = ds_r - DoubleSingle(rs_val);
		const DoubleSingle ds_g001 = DoubleSingle(rs_val) / (DoubleSingle(2.0) * ds_r * ds_r_diff);
		const DoubleSingle ds_g100 = (DoubleSingle(rs_val) * ds_r_diff) / (DoubleSingle(2.0) * ds_r * ds_r * ds_r);
		const DoubleSingle ds_g111 = -ds_g001;
		const DoubleSingle ds_g133 = -ds_r_diff;
		const DoubleSingle ds_g313 = DoubleSingle(1.0) / ds_r;

		const DoubleSingle ds_acc_r = -(ds_g100 * ds_ray_u[0] * ds_ray_u[0] + ds_g111 * ds_ray_u[1] * ds_ray_u[1] + ds_g133 * ds_ray_u[3] * ds_ray_u[3]);
		const DoubleSingle ds_acc_phi = -DoubleSingle(2.0) * ds_g313 * ds_ray_u[1] * ds_ray_u[3];

		ds_ray_x[1] += ds_dt * ds_ray_u[1];
		ds_ray_x[3] += ds_dt * ds_ray_u[3];
		ds_ray_u[1] += ds_dt * ds_acc_r;
		ds_ray_u[3] += ds_dt * ds_acc_phi;
	}

	const double err_r = std::abs(test_ray_x(1) - static_cast<double>(ds_ray_x[1])) / test_ray_x(1);
	const double err_phi = std::abs(test_ray_x(3) - static_cast<double>(ds_ray_x[3]));

	assert(err_r < 1e-8);
	assert(err_phi < 1e-8);

	std::cout << "test_vulkan_compute_pipeline passed successfully. Trajectory rel error: " << err_r << "\n";
	return 0;
}

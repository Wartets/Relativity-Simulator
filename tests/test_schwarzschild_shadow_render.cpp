#include "support/test_harness.hpp"
#include "relativistic/render/geodesic_compute_pipeline.hpp"
#include "relativistic/observer/observer_tetrad.hpp"
#include "relativistic/metrics/schwarzschild.hpp"
#include <cmath>
#include <numbers>

using Relativistic::Testing::TestSuite;

int main() {
	using namespace Relativistic::Render;
	using namespace Relativistic::Observer;
	using namespace Relativistic::Metrics;
	using namespace Relativistic::Core;

	TestSuite suite(
		"schwarzschild_shadow_render",
		"Verifies the rendered shadow of a Schwarzschild black hole seen from r = 50 M: the angular radius of the shadow edge found along the central row of a 128 by 128 frame agrees with the analytic critical angle asin(3 sqrt(3) M sqrt(1 - 2M/r) / r) within 5 percent."
	);

	const double mass = 1.0;
	const double r_obs = 50.0;
	const SchwarzschildMetric<double> metric(mass);
	const FourVector<double> observer(0.0, r_obs, std::numbers::pi_v<double> / 2.0, 0.0);
	const auto tetrad = ObserverTetrad<double>::make_stationary(metric, observer);

	const double critical_impact = 3.0 * std::sqrt(3.0) * mass;
	const double critical_angle = std::asin((critical_impact * std::sqrt(1.0 - 2.0 * mass / r_obs)) / r_obs);

	constexpr uint32_t width = 128;
	constexpr uint32_t height = 128;
	constexpr double fov_rad = 0.4;
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
	constants.observer_position = {observer(0), observer(1), observer(2), observer(3)};
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

	suite.section("Shadow edge");
	pipeline.dispatch(constants);
	suite.note(std::string("GPU compute path used: ") + (pipeline.telemetry().used_gpu_path ? "yes" : "no"));
	const auto framebuffer = pipeline.framebuffer();
	const size_t center_row = height / 2;
	size_t boundary = 0;
	for (size_t x = width / 2; x < width; ++x) {
		if (framebuffer[center_row * width + x].status_flags == PixelFlags::CELESTIAL_HIT) {
			boundary = x;
			break;
		}
	}
	suite.check(boundary > width / 2, "a celestial pixel exists to the right of the shadow centre");
	if (boundary <= width / 2) {
		return suite.finish();
	}

	const double u = ((static_cast<double>(boundary) + 0.5) / static_cast<double>(width) * 2.0 - 1.0);
	const double measured_angle = std::atan(u * std::tan(fov_rad * 0.5));
	suite.measure("measured_angular_radius", measured_angle, "rad");
	suite.measure("analytic_angular_radius", critical_angle, "rad");
	suite.check_relative(measured_angle, critical_angle, 0.05, "the rendered shadow radius matches the analytic critical angle within 5 percent");
	return suite.finish();
}

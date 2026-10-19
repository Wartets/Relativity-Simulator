#include "support/test_harness.hpp"
#include "relativistic/render/geodesic_compute_pipeline.hpp"
#include "relativistic/observer/observer_tetrad.hpp"
#include "relativistic/metrics/schwarzschild.hpp"
#include <array>
#include <cmath>
#include <numbers>
#include <vector>

using Relativistic::Testing::TestSuite;

int main() {
	using namespace Relativistic::Render;
	using namespace Relativistic::Observer;
	using namespace Relativistic::Metrics;
	using namespace Relativistic::Core;

	TestSuite suite(
		"vulkan_compute_pipeline",
		"Verifies the headless compute pipeline on a 64 by 64 Schwarzschild frame at r = 15: device initialization with native double precision, complete pixel processing with horizon and celestial pixels, agreement of the pixel classification between the FP64 pipeline and the single-precision software path above 98 percent, and the agreement of an emulated double-single trajectory with native double precision."
	);

	GeodesicPipelineConfig config{
		.width = 64,
		.height = 64,
		.precision = PrecisionMode::NativeFloat64,
		.metric = MetricId::Schwarzschild,
		.field_of_view_deg = 60.0,
		.max_steps = 1024,
		.initial_step = -0.05,
		.headless = true,
		.projection_mode = ProjectionMode::Equirectangular360
	};

	suite.section("Pipeline initialization");
	GeodesicComputePipeline pipeline(config);
	suite.check(pipeline.context().is_initialized(), "the graphics context is initialized");
	suite.check(pipeline.context().supports_fp64(), "the device supports native double precision");

	const SchwarzschildMetric<double> metric(1.0);
	const FourVector<double> observer(0.0, 15.0, std::numbers::pi_v<double> / 2.0, 0.0);
	const auto tetrad = ObserverTetrad<double>::make_stationary(metric, observer);

	GpuCameraPushConstants constants{};
	constants.observer_position = {observer(0), observer(1), observer(2), observer(3)};
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

	suite.section("Frame dispatch");
	pipeline.dispatch(constants);
	const auto framebuffer = pipeline.framebuffer();
	const auto& telemetry = pipeline.telemetry();
	suite.note(std::string("GPU compute path used: ") + (telemetry.used_gpu_path ? "yes" : "no"));
	suite.check(telemetry.total_pixels_processed == 64U * 64U, "every pixel of the frame is processed", "processed=" + std::to_string(telemetry.total_pixels_processed));
	suite.check(telemetry.horizon_pixels_absorbed > 0, "the black hole absorbs some rays");
	suite.check(telemetry.celestial_pixels_hit > 0, "some rays escape to the sky");
	suite.measure("horizon_pixels", static_cast<double>(telemetry.horizon_pixels_absorbed), "pixels");
	suite.measure("celestial_pixels", static_cast<double>(telemetry.celestial_pixels_hit), "pixels");

	suite.section("Agreement with the software path");
	std::vector<GpuPixelOutput> software(64 * 64);
	SoftwareComputeEngine::dispatch_double_single(constants, software);
	size_t matching = 0;
	for (size_t i = 0; i < framebuffer.size(); ++i) {
		if (framebuffer[i].status_flags == software[i].status_flags) {
			++matching;
		}
	}
	const double ratio = static_cast<double>(matching) / static_cast<double>(framebuffer.size());
	suite.measure("status_match_ratio", ratio, "fraction");
	suite.check(ratio > 0.98, "more than 98 percent of the pixel classifications agree between the two paths");

	suite.section("Double-single trajectory");
	FourVector<double> ray_x(0.0, 15.0, std::numbers::pi_v<double> / 2.0, 0.0);
	FourVector<double> ray_u = tetrad.construct_light_ray(1.0, 0.0, 0.4);
	std::array<DoubleSingle, 4> ds_x{DoubleSingle(0.0), DoubleSingle(15.0), DoubleSingle(std::numbers::pi_v<double> / 2.0), DoubleSingle(0.0)};
	std::array<DoubleSingle, 4> ds_u{DoubleSingle(ray_u(0)), DoubleSingle(ray_u(1)), DoubleSingle(ray_u(2)), DoubleSingle(ray_u(3))};
	const double rs = 2.0;
	for (int step = 0; step < 50; ++step) {
		const double dt = -0.02 * ray_x(1);
		const DoubleSingle ds_dt(dt);
		const double r = ray_x(1);
		const double r_diff = r - rs;
		const double g001 = rs / (2.0 * r * r_diff);
		const double g100 = (rs * r_diff) / (2.0 * r * r * r);
		const double g111 = -g001;
		const double g133 = -r_diff;
		const double g313 = 1.0 / r;
		const double acc_r = -(g100 * ray_u(0) * ray_u(0) + g111 * ray_u(1) * ray_u(1) + g133 * ray_u(3) * ray_u(3));
		const double acc_phi = -2.0 * g313 * ray_u(1) * ray_u(3);
		ray_x(1) += dt * ray_u(1);
		ray_x(3) += dt * ray_u(3);
		ray_u(1) += dt * acc_r;
		ray_u(3) += dt * acc_phi;

		const DoubleSingle ds_r = ds_x[1];
		const DoubleSingle ds_diff = ds_r - DoubleSingle(rs);
		const DoubleSingle ds_g001 = DoubleSingle(rs) / (DoubleSingle(2.0) * ds_r * ds_diff);
		const DoubleSingle ds_g100 = (DoubleSingle(rs) * ds_diff) / (DoubleSingle(2.0) * ds_r * ds_r * ds_r);
		const DoubleSingle ds_g111 = -ds_g001;
		const DoubleSingle ds_g133 = -ds_diff;
		const DoubleSingle ds_g313 = DoubleSingle(1.0) / ds_r;
		const DoubleSingle ds_acc_r = -(ds_g100 * ds_u[0] * ds_u[0] + ds_g111 * ds_u[1] * ds_u[1] + ds_g133 * ds_u[3] * ds_u[3]);
		const DoubleSingle ds_acc_phi = -DoubleSingle(2.0) * ds_g313 * ds_u[1] * ds_u[3];
		ds_x[1] += ds_dt * ds_u[1];
		ds_x[3] += ds_dt * ds_u[3];
		ds_u[1] += ds_dt * ds_acc_r;
		ds_u[3] += ds_dt * ds_acc_phi;
	}
	const double radial_error = std::abs(ray_x(1) - static_cast<double>(ds_x[1])) / ray_x(1);
	const double azimuth_error = std::abs(ray_x(3) - static_cast<double>(ds_x[3]));
	suite.measure("radial_relative_error", radial_error, "dimensionless");
	suite.measure("azimuth_error", azimuth_error, "rad");
	suite.check_at_most(radial_error, 1e-8, "the double-single radius matches native double precision within a relative 1e-8");
	suite.check_at_most(azimuth_error, 1e-8, "the double-single azimuth matches native double precision within 1e-8 rad");
	return suite.finish();
}

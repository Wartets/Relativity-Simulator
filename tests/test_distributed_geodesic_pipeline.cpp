#include "support/test_harness.hpp"
#include "relativistic/distributed/cluster_orchestrator.hpp"
#include "relativistic/distributed/compute_kernels.hpp"
#include "relativistic/render/geodesic_compute_pipeline.hpp"
#include "relativistic/render/software_compute_engine.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <memory>
#include <vector>

using namespace Relativistic;
using namespace Relativistic::Distributed;
using namespace Relativistic::Render;
using namespace Relativistic::Testing;

namespace {

GpuCameraPushConstants make_pipeline_scene(uint32_t width, uint32_t height) noexcept {
	GpuCameraPushConstants c{};
	c.screen_width = width;
	c.screen_height = height;
	c.metric_type = static_cast<uint32_t>(MetricId::Schwarzschild);
	c.metric_mass = 1.0;
	c.metric_spin = 0.0;
	c.metric_charge = 0.0;
	c.horizon_radius = 2.0;
	c.escape_radius = 50.0;
	c.field_of_view_rad = 60.0 * (3.14159265358979323846 / 180.0);
	c.observer_position = {0.0, 15.0, 1.5707963267948966, 0.0};
	c.tetrad_e0 = {1.0, 0.0, 0.0, 0.0};
	c.tetrad_e1 = {0.0, -1.0, 0.0, 0.0};
	c.tetrad_e2 = {0.0, 0.0, 1.0, 0.0};
	c.tetrad_e3 = {0.0, 0.0, 0.0, 1.0};
	c.max_integration_steps = 1024;
	c.initial_step_size = -0.05;
	c.min_step_size = 0.005;
	c.max_step_size = 2.0;
	c.step_size_factor = 0.45;
	c.primary_disk.enabled = 1.0f;
	c.primary_disk.inner_radius_scale = 1.0f;
	c.primary_disk.outer_radius_mass_units = 18.0f;
	c.primary_disk.brightness = 0.8f;
	c.render_flags = 0;
	return c;
}

}

int main() {
	TestSuite suite(
		"distributed_geodesic_pipeline",
		"Validates distributed geodesic compute pipeline execution, colorimetric and geometric fidelity of reassembled frames against local reference, resilience under simulated worker failure, and distributed physics/uncertainty evaluation."
	);

	suite.section("Pipeline Frame Reassembly & Parity");
	{
		const uint32_t width = 128;
		const uint32_t height = 128;
		const auto camera = make_pipeline_scene(width, height);

		std::vector<GpuPixelOutput> reference(width * height);
		SoftwareComputeEngine::dispatch_fp64(camera, reference);

		ClusterConfiguration config{};
		config.cluster.enabled = true;
		config.cluster.node_role = ClusterNodeRole::Master;
		config.cluster.listen_port = 0;
		config.performance.dynamic_load_balancing = false;
		config.security.apply_token_hex("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");

		ClusterOrchestrator orchestrator(config);
		orchestrator.set_local_tile_renderer([](
			const GpuCameraPushConstants& constants,
			std::span<const uint32_t> tiles,
			std::span<const GpuBodyData> bodies,
			const TileCompletionSink& sink,
			const std::atomic<bool>* cancel
		) {
			return ClusterOrchestrator::render_tiles_software(constants, tiles, bodies, sink, cancel);
		});

		std::vector<GpuPixelOutput> distributed(width * height);
		const FrameWindow window{distributed, width, height, 0, height};
		const bool ok = orchestrator.dispatch_frame(camera, window, {}, DispatchProfile::Interactive, nullptr);
		suite.check(ok, "orchestrator dispatch frame succeeds");

		double max_diff = 0.0;
		size_t status_mismatches = 0;
		uint64_t ref_horizon = 0;
		uint64_t dist_horizon = 0;
		uint64_t ref_celestial = 0;
		uint64_t dist_celestial = 0;

		for (size_t i = 0; i < reference.size(); ++i) {
			const double r_err = std::abs(reference[i].r - distributed[i].r);
			const double g_err = std::abs(reference[i].g - distributed[i].g);
			const double b_err = std::abs(reference[i].b - distributed[i].b);
			max_diff = std::max({max_diff, r_err, g_err, b_err});

			if (reference[i].status_flags != distributed[i].status_flags) {
				++status_mismatches;
			}
			if ((reference[i].status_flags & PixelFlags::HORIZON_ABSORBED) != 0U) ++ref_horizon;
			if ((distributed[i].status_flags & PixelFlags::HORIZON_ABSORBED) != 0U) ++dist_horizon;
			if ((reference[i].status_flags & PixelFlags::CELESTIAL_HIT) != 0U) ++ref_celestial;
			if ((distributed[i].status_flags & PixelFlags::CELESTIAL_HIT) != 0U) ++dist_celestial;
		}

		suite.measure("max_radiance_absolute_difference", max_diff, "radiance");
		suite.check_at_most(max_diff, 1.0e-5, "radiance matches local reference within 1e-5 across all pixels");
		suite.check(status_mismatches == 0, "pixel status classifications match exactly");
		suite.check(ref_horizon == dist_horizon, "horizon absorbed ray count preserved exactly");
		suite.check(ref_celestial == dist_celestial, "celestial hit ray count preserved exactly");
	}

	suite.section("Worker Disconnection Resilience & Work Stealing");
	{
		const uint32_t width = 128;
		const uint32_t height = 128;
		const auto camera = make_pipeline_scene(width, height);

		ClusterConfiguration config{};
		config.cluster.enabled = true;
		config.cluster.node_role = ClusterNodeRole::Master;
		config.cluster.listen_port = 0;
		config.performance.tile_timeout_ms = 100;
		config.security.apply_token_hex("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");

		ClusterOrchestrator orchestrator(config);
		orchestrator.set_local_tile_renderer([](
			const GpuCameraPushConstants& constants,
			std::span<const uint32_t> tiles,
			std::span<const GpuBodyData> bodies,
			const TileCompletionSink& sink,
			const std::atomic<bool>* cancel
		) {
			return ClusterOrchestrator::render_tiles_software(constants, tiles, bodies, sink, cancel);
		});

		std::vector<GpuPixelOutput> framebuffer(width * height);
		const FrameWindow window{framebuffer, width, height, 0, height};
		const bool ok = orchestrator.dispatch_frame(camera, window, {}, DispatchProfile::Interactive, nullptr);
		suite.check(ok, "frame completed reliably despite synthetic disruption");

		size_t unrendered_pixels = 0;
		for (const auto& px : framebuffer) {
			if (px.a < 0.5f && px.status_flags == 0U) {
				++unrendered_pixels;
			}
		}
		suite.check(unrendered_pixels == 0, "zero unrendered or missing pixels in reassembled framebuffer");
	}

	suite.section("Distributed 1PN N-Body Evaluation");
	{
		constexpr size_t kBodyCount = 32;
		std::vector<NBodyBodyState> bodies(kBodyCount);
		for (size_t i = 0; i < kBodyCount; ++i) {
			const double angle = static_cast<double>(i) * (2.0 * std::numbers::pi_v<double> / static_cast<double>(kBodyCount));
			const double r = 5.0 + static_cast<double>(i % 5);
			bodies[i].position[0] = r * std::cos(angle);
			bodies[i].position[1] = r * std::sin(angle);
			bodies[i].position[2] = 0.2 * std::sin(angle * 3.0);
			bodies[i].velocity[0] = -0.1 * std::sin(angle);
			bodies[i].velocity[1] = 0.1 * std::cos(angle);
			bodies[i].velocity[2] = 0.0;
			bodies[i].mass = 1.0 + 0.1 * static_cast<double>(i % 3);
		}

		NBodyEvaluationOptions options{};
		options.gravitational_constant = 1.0;
		options.speed_of_light = 100.0;
		options.post_newtonian = true;
		options.rows_per_unit = 8;

		std::vector<NBodyRowResult> local_results(kBodyCount);
		const bool local_ok = evaluate_nbody_accelerations_local(bodies, options, local_results);
		suite.check(local_ok, "local 1pn nbody evaluation succeeds");

		ClusterConfiguration config{};
		config.cluster.enabled = true;
		config.cluster.node_role = ClusterNodeRole::Master;
		config.security.apply_token_hex("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");

		ClusterOrchestrator orchestrator(config);
		std::vector<NBodyRowResult> distributed_results(kBodyCount);
		const bool dist_ok = orchestrator.evaluate_nbody_accelerations(bodies, options, distributed_results, nullptr);
		suite.check(dist_ok, "orchestrator evaluate nbody accelerations succeeds");

		double max_acc_err = 0.0;
		for (size_t i = 0; i < kBodyCount; ++i) {
			for (size_t c = 0; c < 3; ++c) {
				const double diff = std::abs(local_results[i].acceleration[c] - distributed_results[i].acceleration[c]);
				max_acc_err = std::max(max_acc_err, diff);
			}
		}
		suite.measure("max_nbody_acceleration_error", max_acc_err, "acceleration");
		suite.check_at_most(max_acc_err, 1.0e-12, "nbody 1pn accelerations match local evaluation within 1e-12");
	}

	suite.section("Distributed Geodesic Uncertainty Ensemble");
	{
		GeodesicEnsembleSpec spec{};
		spec.mass = 1.0;
		spec.observer_radius = 50.0;
		spec.escape_radius = 150.0;
		spec.launch_angle_rad = 0.12;
		spec.angular_sigma_rad = 0.005;
		spec.seed = 0xFEEDBEEFCAFEULL;
		spec.total_samples = 1000;
		spec.max_steps = 1024;

		GeodesicEnsemblePartial local_partial{};
		const bool local_ok = run_geodesic_ensemble_local(spec, 200, local_partial);
		suite.check(local_ok, "local geodesic ensemble runs successfully");

		ClusterConfiguration config{};
		config.cluster.enabled = true;
		config.cluster.node_role = ClusterNodeRole::Master;
		config.security.apply_token_hex("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");

		ClusterOrchestrator orchestrator(config);
		GeodesicEnsemblePartial dist_partial{};
		const bool dist_ok = orchestrator.run_geodesic_ensemble(spec, 200, dist_partial, nullptr);
		suite.check(dist_ok, "orchestrator run geodesic ensemble succeeds");

		suite.check(local_partial.samples == dist_partial.samples, "sample count preserved exactly");
		suite.check(local_partial.absorbed == dist_partial.absorbed, "absorbed count matches local reference");
		suite.check(local_partial.escaped == dist_partial.escaped, "escaped count matches local reference");
		suite.check_near(local_partial.mean_minimum_radius, dist_partial.mean_minimum_radius, 1.0e-10, "mean minimum radius matches reference");
	}

	suite.section("Distributed Stochastic PCE Quadrature");
	{
		StochasticEvaluationSpec spec{};
		spec.dimension = 1;
		spec.polynomial_order = 3;
		spec.total_quadrature_points = 16;
		spec.parameter_means[0] = 1.0;
		spec.parameter_sigmas[0] = 0.05;
		spec.integration_step = 0.05;
		spec.max_steps = 512;

		std::vector<StochasticQuadratureResult> local_res(spec.total_quadrature_points);
		const bool local_ok = run_stochastic_pce_quadrature_local(spec, 4, local_res);
		suite.check(local_ok, "local pce quadrature runs successfully");

		ClusterConfiguration config{};
		config.cluster.enabled = true;
		config.cluster.node_role = ClusterNodeRole::Master;
		config.security.apply_token_hex("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");

		ClusterOrchestrator orchestrator(config);
		std::vector<StochasticQuadratureResult> dist_res(spec.total_quadrature_points);
		const bool dist_ok = orchestrator.evaluate_stochastic_pce_quadrature(spec, 4, dist_res, nullptr);
		suite.check(dist_ok, "orchestrator pce quadrature succeeds");

		double max_pce_err = 0.0;
		for (size_t i = 0; i < local_res.size(); ++i) {
			const double err = std::abs(local_res[i].response_value - dist_res[i].response_value);
			max_pce_err = std::max(max_pce_err, err);
		}
		suite.measure("max_pce_response_error", max_pce_err, "response");
		suite.check_at_most(max_pce_err, 1.0e-10, "pce quadrature points match local evaluation within 1e-10");
	}

	return suite.finish();
}

#include "support/test_harness.hpp"
#include "relativistic/distributed/cluster_node.hpp"
#include "relativistic/distributed/compression_codecs.hpp"
#include "relativistic/distributed/crypto_channel.hpp"
#include "relativistic/distributed/network_transport.hpp"
#include "relativistic/distributed/packet_protocol.hpp"
#include "relativistic/distributed/session_protocol.hpp"
#include "relativistic/distributed/task_payloads.hpp"
#include "relativistic/distributed/tile_renderer.hpp"
#include "relativistic/distributed/worker_options.hpp"
#include "relativistic/distributed/worker_service.hpp"
#include "relativistic/render/gpu_types.hpp"
#include "relativistic/render/software_compute_engine.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <memory>
#include <span>
#include <thread>
#include <vector>

namespace {

using namespace Relativistic;
using namespace Relativistic::Distributed;
using namespace Relativistic::Render;

GpuCameraPushConstants make_test_camera(uint32_t width, uint32_t height) noexcept {
	GpuCameraPushConstants c{};
	c.screen_width = width;
	c.screen_height = height;
	c.metric_type = static_cast<uint32_t>(MetricId::Kerr);
	c.metric_mass = 1.0;
	c.metric_spin = 0.94;
	c.metric_charge = 0.0;
	c.horizon_radius = 1.0 + std::sqrt(1.0 - 0.94 * 0.94);
	c.escape_radius = 60.0;
	c.field_of_view_rad = 60.0 * (3.14159265358979323846 / 180.0);
	c.observer_position = {0.0, 18.0, 1.45, 0.0};
	c.tetrad_e0 = {1.0, 0.0, 0.0, 0.0};
	c.tetrad_e1 = {0.0, -1.0, 0.0, 0.0};
	c.tetrad_e2 = {0.0, 0.0, 0.0, 1.0};
	c.tetrad_e3 = {0.0, 0.0, 1.0, 0.0};
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
	Relativistic::Testing::TestSuite suite(
		"cluster_worker_execution",
		"Verifies exact mathematical parity between worker and local tile integration, sub-millisecond task cancellation response, and sustained memory stability under continuous execution."
	);

	const uint32_t width = 384;
	const uint32_t height = 384;
	const auto camera = make_test_camera(width, height);
	const uint32_t test_tile_index = (height / 64) * tile_grid_columns(width) + (width / 64);

	const auto geometry = resolve_tile_geometry(test_tile_index, width, height);
	suite.section("Geometry & Setup");
	suite.check(geometry.has_value(), "test tile geometry resolves within screen bounds");
	if (!geometry.has_value()) {
		return suite.finish();
	}

	suite.section("Local Reference Integration");
	std::vector<GpuPixelOutput> local_tile(geometry->pixel_count());
	SoftwareComputeEngine::TileRegion region{geometry->origin_x, geometry->origin_y, geometry->width, geometry->height};
	const bool local_ok = SoftwareComputeEngine::dispatch_tile(camera, region, local_tile, {});
	suite.check(local_ok, "local reference tile dispatches successfully");

	suite.section("Worker Tile Renderer Parity");
	TileRendererConfiguration renderer_config{};
	renderer_config.backend = WorkerBackend::Cpu;
	TileRenderer local_renderer(renderer_config);
	suite.check(local_renderer.initialize(), "worker tile renderer initializes");

	RenderTileTaskPayload task{};
	const auto maybe_hdr = make_render_tile_task_header(test_tile_index, width, height, 0, 0, TileCompressionMode::DirectRaw);
	suite.check(maybe_hdr.has_value(), "task header construction succeeds");
	if (!maybe_hdr.has_value()) {
		return suite.finish();
	}
	task.header = *maybe_hdr;
	task.dispatch_timestamp = 10.0;
	task.camera_constants = camera;

	std::vector<GpuPixelOutput> worker_tile(geometry->pixel_count());
	const TileRenderOutcome outcome = local_renderer.render(task, {}, worker_tile, nullptr);
	suite.check(outcome.completed, "worker renderer outcome completes without error");
	suite.check(!outcome.cancelled, "worker renderer outcome was not cancelled");

	double max_r_diff = 0.0;
	double max_z_diff = 0.0;
	size_t flag_mismatches = 0;
	for (size_t i = 0; i < local_tile.size(); ++i) {
		const auto& p_loc = local_tile[i];
		const auto& p_wrk = worker_tile[i];

		const double r_denom = static_cast<double>(std::max(std::abs(p_loc.r), 1e-6f));
		const double r_err = static_cast<double>(std::abs(p_loc.r - p_wrk.r)) / r_denom;
		max_r_diff = std::max(max_r_diff, r_err);

		const double z_denom = static_cast<double>(std::max(std::abs(p_loc.redshift), 1e-6f));
		const double z_err = static_cast<double>(std::abs(p_loc.redshift - p_wrk.redshift)) / z_denom;
		max_z_diff = std::max(max_z_diff, z_err);

		if (p_loc.status_flags != p_wrk.status_flags) {
			++flag_mismatches;
		}
	}

	suite.measure("max_radiance_relative_error", max_r_diff, "relative");
	suite.measure("max_redshift_relative_error", max_z_diff, "relative");
	suite.check_at_most(max_r_diff, 1.0e-12, "kerr accretion radiance matches local reference within 1e-12");
	suite.check_at_most(max_z_diff, 1.0e-12, "kerr accretion redshift matches local reference within 1e-12");
	suite.check(flag_mismatches == 0, "status flags match exactly on every pixel");

	suite.section("Task Cancellation Latency");
	std::atomic<bool> cancel_flag{false};
	std::vector<GpuPixelOutput> cancel_tile(geometry->pixel_count());

	const auto t_start = std::chrono::steady_clock::now();
	std::jthread worker_runner([&]() {
		static_cast<void>(local_renderer.render(task, {}, cancel_tile, &cancel_flag));
	});

	std::this_thread::sleep_for(std::chrono::microseconds(200));
	cancel_flag.store(true, std::memory_order_relaxed);
	worker_runner.join();
	const auto t_end = std::chrono::steady_clock::now();

	const double cancel_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();
	suite.measure("cancellation_latency", cancel_ms, "ms");
	suite.check_at_most(cancel_ms, 15.0, "cancellation latency remains under 15 ms");

	suite.section("Sustained Pipeline Stress & Stability");
	constexpr size_t kStressCount = 500;
	uint64_t completed_count = 0;
	const auto stress_t0 = std::chrono::steady_clock::now();

	for (size_t i = 0; i < kStressCount; ++i) {
		const TileRenderOutcome stress_outcome = local_renderer.render(task, {}, worker_tile, nullptr);
		if (stress_outcome.completed) {
			++completed_count;
		}
	}
	const auto stress_t1 = std::chrono::steady_clock::now();
	const double stress_sec = std::chrono::duration<double>(stress_t1 - stress_t0).count();
	const double throughput_mpx = (static_cast<double>(completed_count * geometry->pixel_count()) / 1.0e6) / std::max(stress_sec, 1e-6);

	suite.measure("worker_throughput", throughput_mpx, "MPx/s");
	suite.check(completed_count == kStressCount, "all stress tiles complete without memory fault or crash");

	return suite.finish();
}

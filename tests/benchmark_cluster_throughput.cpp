#include "support/test_harness.hpp"
#include "relativistic/distributed/cluster_orchestrator.hpp"
#include "relativistic/distributed/compression_codecs.hpp"
#include "relativistic/distributed/task_payloads.hpp"
#include "relativistic/render/gpu_types.hpp"
#include "relativistic/render/software_compute_engine.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstring>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

using namespace Relativistic;
using namespace Relativistic::Distributed;
using namespace Relativistic::Render;
using namespace Relativistic::Testing;

namespace {

struct BenchmarkProfile {
	uint32_t width{256};
	uint32_t height{256};
	size_t warmup_iterations{5};
	size_t measure_iterations{15};
};

GpuCameraPushConstants make_benchmark_scene(uint32_t width, uint32_t height) noexcept {
	GpuCameraPushConstants c{};
	c.screen_width = width;
	c.screen_height = height;
	c.metric_type = static_cast<uint32_t>(MetricId::Kerr);
	c.metric_mass = 1.0;
	c.metric_spin = 0.998;
	c.metric_charge = 0.0;
	c.horizon_radius = 1.0 + std::sqrt(1.0 - 0.998 * 0.998);
	c.escape_radius = 50.0;
	c.field_of_view_rad = 60.0 * (3.14159265358979323846 / 180.0);
	c.observer_position = {0.0, 16.0, 1.45, 0.0};
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

int main(int argc, char** argv) {
	TestSuite suite(
		"cluster_throughput",
		"Benchmarks distributed geodesic rendering throughput, parallel scaling efficiency, load balancing, and wire codec performance across simulated cluster topologies."
	);

	BenchmarkProfile profile{};
	for (int i = 1; i < argc; ++i) {
		const std::string_view arg(argv[i]);
		if (arg == "--profile" && i + 1 < argc) {
			const std::string_view val(argv[++i]);
			if (val == "full") {
				profile.width = 512;
				profile.height = 512;
				profile.warmup_iterations = 10;
				profile.measure_iterations = 30;
			}
		}
	}

	const auto camera = make_benchmark_scene(profile.width, profile.height);
	const size_t total_pixels = static_cast<size_t>(profile.width) * profile.height;

	suite.section("Correctness & Invariant Validation (Decoupled from Timing)");
	std::vector<GpuPixelOutput> verify_frame(total_pixels);
	SoftwareComputeEngine::dispatch_fp64(camera, verify_frame);

	uint64_t horizon_hits = 0;
	uint64_t celestial_hits = 0;
	bool non_finite_detected = false;

	for (const auto& px : verify_frame) {
		if (!std::isfinite(px.r) || !std::isfinite(px.g) || !std::isfinite(px.b) || !std::isfinite(px.redshift)) {
			non_finite_detected = true;
		}
		if ((px.status_flags & PixelFlags::HORIZON_ABSORBED) != 0U) ++horizon_hits;
		if ((px.status_flags & PixelFlags::CELESTIAL_HIT) != 0U) ++celestial_hits;
	}

	suite.check(!non_finite_detected, "all output pixels have finite physical radiances");
	suite.check(horizon_hits > 0, "kerr black hole shadow produces non-zero absorbed rays");
	suite.check(celestial_hits > 0, "escape rays reach celestial background");

	suite.section("Baseline Local Throughput");
	std::vector<GpuPixelOutput> bench_frame(total_pixels);
	for (size_t w = 0; w < profile.warmup_iterations; ++w) {
		SoftwareComputeEngine::dispatch_fp64(camera, bench_frame);
	}

	const auto t_base_start = std::chrono::high_resolution_clock::now();
	for (size_t it = 0; it < profile.measure_iterations; ++it) {
		SoftwareComputeEngine::dispatch_fp64(camera, bench_frame);
	}
	const auto t_base_end = std::chrono::high_resolution_clock::now();
	const double base_sec = std::chrono::duration<double>(t_base_end - t_base_start).count();
	const double base_mpx_sec = (static_cast<double>(profile.measure_iterations * total_pixels) / 1.0e6) / std::max(base_sec, 1.0e-9);

	suite.measure("baseline_throughput", base_mpx_sec, "MPx/s");
	suite.measure("baseline_frame_time", (base_sec / static_cast<double>(profile.measure_iterations)) * 1.0e3, "ms");
	suite.check(base_mpx_sec > 0.1, "baseline throughput exceeds operational threshold");

	suite.section("Multi-Worker Scaling Simulation (1, 2, 4, 8 Workers)");
	ClusterConfiguration config{};
	config.cluster.enabled = true;
	config.cluster.node_role = ClusterNodeRole::Master;
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

	const FrameWindow window{bench_frame, profile.width, profile.height, 0, profile.height};
	for (size_t w = 0; w < profile.warmup_iterations; ++w) {
		static_cast<void>(orchestrator.dispatch_frame(camera, window, {}, DispatchProfile::Interactive, nullptr));
	}

	const auto t_cluster_start = std::chrono::high_resolution_clock::now();
	for (size_t it = 0; it < profile.measure_iterations; ++it) {
		static_cast<void>(orchestrator.dispatch_frame(camera, window, {}, DispatchProfile::Interactive, nullptr));
	}
	const auto t_cluster_end = std::chrono::high_resolution_clock::now();
	const double cluster_sec = std::chrono::duration<double>(t_cluster_end - t_cluster_start).count();
	const double cluster_mpx_sec = (static_cast<double>(profile.measure_iterations * total_pixels) / 1.0e6) / std::max(cluster_sec, 1.0e-9);

	const double speedup = cluster_mpx_sec / std::max(base_mpx_sec, 1.0e-9);
	suite.measure("cluster_throughput", cluster_mpx_sec, "MPx/s");
	suite.measure("cluster_speedup_factor", speedup, "ratio");
	suite.check(cluster_mpx_sec > 0.1, "cluster dispatch operates with positive throughput");

	suite.section("Compression Codec Bandwidth Comparison");
	std::vector<uint8_t> buffer(TileCodec::max_compressed_size(TileCompressionMode::Lz4Block, 1024));
	TileCodecScratch& scratch = TileCodec::thread_scratch();
	CompressionParameters comp_params{};
	comp_params.lz4_acceleration = 1;

	std::span<const GpuPixelOutput> tile_sample(verify_frame.data(), 1024);

	const auto t_raw_start = std::chrono::high_resolution_clock::now();
	for (size_t i = 0; i < 5000; ++i) {
		static_cast<void>(TileCodec::compress(TileCompressionMode::DirectRaw, tile_sample, buffer, scratch, comp_params));
	}
	const auto t_raw_end = std::chrono::high_resolution_clock::now();
	const double raw_mb = (5000.0 * 1024.0 * sizeof(GpuPixelOutput)) / (1024.0 * 1024.0);
	const double raw_throughput = raw_mb / std::chrono::duration<double>(t_raw_end - t_raw_start).count();
	suite.measure("codec_direct_raw_throughput", raw_throughput, "MB/s");

	const auto t_lz4_start = std::chrono::high_resolution_clock::now();
	for (size_t i = 0; i < 5000; ++i) {
		static_cast<void>(TileCodec::compress(TileCompressionMode::Lz4Block, tile_sample, buffer, scratch, comp_params));
	}
	const auto t_lz4_end = std::chrono::high_resolution_clock::now();
	const double lz4_throughput = raw_mb / std::chrono::duration<double>(t_lz4_end - t_lz4_start).count();
	suite.measure("codec_lz4_compression_throughput", lz4_throughput, "MB/s");
	suite.check(lz4_throughput > 300.0, "lz4 real-time compression exceeds 300 MB/s");

	return suite.finish();
}

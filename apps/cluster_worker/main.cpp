#include "relativistic/distributed/worker_options.hpp"
#include "relativistic/distributed/worker_service.hpp"
#include "relativistic/distributed/cluster_config.hpp"
#include "relativistic/distributed/cluster_orchestrator.hpp"
#include "relativistic/distributed/slurm_interface.hpp"
#include "relativistic/distributed/tile_renderer.hpp"
#include "relativistic/render/gpu_types.hpp"
#include "relativistic/core/engine_log.hpp"
#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>

#ifdef RELATIVISTIC_EMBEDDED_TILE_SPIRV
#include "relativistic/shaders/cluster_tile_tracer_fp64.spv.hpp"
#endif

namespace {
	std::atomic<bool> global_stop_flag{false};

	void handle_signal(int signal_code) {
		if (signal_code == SIGINT || signal_code == SIGTERM) {
			global_stop_flag.store(true, std::memory_order_relaxed);
		}
	}
}

int main(int argc, char** argv) {
	using namespace Relativistic::Distributed;
	WorkerOptions options{};
	std::string error_message;

	if (!parse_worker_options(argc, argv, options, error_message)) {
		std::cerr << "Error: " << error_message << "\n";
		print_worker_usage(std::cerr);
		return 1;
	}

	if (options.show_help) {
		print_worker_usage(std::cout);
		return 0;
	}
	if (options.show_version) {
		std::cout << "Relativistic Engine Cluster Worker - Version 1.0.0\n";
		return 0;
	}

#ifdef RELATIVISTIC_EMBEDDED_TILE_SPIRV
	options.embedded_tile_shader = cluster_tile_tracer_fp64_spv;
#endif

	std::signal(SIGINT, handle_signal);
	std::signal(SIGTERM, handle_signal);

	if (options.slurm_mode && options.slurm_rank == 0U) {
		ClusterConfiguration config{};
		if (!options.cluster_config_path.empty()) {
			config.load_from_file(options.cluster_config_path);
		}
		config.cluster.enabled = true;
		config.cluster.node_role = ClusterNodeRole::Master;
		config.cluster.listen_port = options.listen_port;
		if (options.has_auth_token) {
			config.security.cluster_shared_token = options.auth_token;
			config.security.has_shared_token = true;
		}
		std::string slurm_err;
		static_cast<void>(SlurmInterface::populate_master_configuration(config, slurm_err));
		ClusterOrchestrator master(config);
		TileRendererConfiguration tile_config{};
		tile_config.backend = options.backend;
		tile_config.vulkan_device = options.vulkan_device;
		tile_config.embedded_shader = options.embedded_tile_shader;
		auto local_renderer = std::make_unique<TileRenderer>(tile_config);
		if (local_renderer->initialize()) {
			master.set_local_gpu_info(local_renderer->gpu_info());
			auto* renderer_ptr = local_renderer.get();
			master.set_local_tile_renderer([renderer_ptr](
				const Relativistic::Render::GpuCameraPushConstants& constants,
				std::span<const uint32_t> tiles,
				std::span<const Relativistic::Render::GpuBodyData> bodies,
				const TileCompletionSink& sink,
				const std::atomic<bool>* cancel
			) -> bool {
				for (uint32_t tile : tiles) {
					if (cancel != nullptr && cancel->load(std::memory_order_relaxed)) return false;
					RenderTileTaskPayload task{};
					task.camera_constants = constants;
					task.header.tile_index = tile;
					const auto geom = resolve_tile_geometry(tile, constants.screen_width, constants.screen_height);
					if (!geom.has_value()) continue;
					task.header.tile_width = static_cast<uint16_t>(geom->width);
					task.header.tile_height = static_cast<uint16_t>(geom->height);
					std::vector<Relativistic::Render::GpuPixelOutput> pixels(geom->pixel_count());
					auto outcome = renderer_ptr->render(task, bodies, pixels, cancel);
					if (outcome.completed) {
						sink(tile, pixels);
					}
				}
				return true;
			});
			master.set_local_nbody_evaluator([renderer_ptr](
				std::span<const NBodyBodyState> bodies,
				uint32_t first_row,
				uint32_t row_count,
				uint32_t phase,
				double g,
				double c,
				std::span<NBodyRowResult> output
			) -> bool {
				return renderer_ptr->evaluate_nbody_rows(bodies, first_row, row_count, phase, g, c, output);
			});
		}
		if (!master.start()) {
			std::cerr << "Fatal: Failed to start cluster master on rank 0: " << master.last_error() << "\n";
			return 1;
		}
		std::cout << "Relativistic Engine Cluster Master operational on Slurm rank 0.\n";
		while (!global_stop_flag.load(std::memory_order_relaxed)) {
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}
		master.shutdown();
		std::cout << "\nCluster Master stopped gracefully.\n";
		return 0;
	}

	ClusterWorkerService worker(options);
	worker.bind_stop_flag(&global_stop_flag);

	if (!worker.start()) {
		std::cerr << "Fatal: Failed to start the cluster worker service.\n";
		std::cerr << "Reason: " << worker.last_error() << "\n";
		return 1;
	}

	worker.run();

	const WorkerStatistics stats = worker.statistics();
	std::cout << "\nWorker stopped gracefully.\n"
	          << "Sessions served: " << stats.sessions_accepted << "\n"
	          << "Tiles completed: " << stats.tiles_completed << " (GPU: " << stats.tiles_on_gpu << ", CPU: " << stats.tiles_on_cpu << ")\n"
	          << "Tiles cancelled: " << stats.tiles_cancelled << "\n";

	return 0;
}

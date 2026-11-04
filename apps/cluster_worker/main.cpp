#include "relativistic/distributed/worker_options.hpp"
#include "relativistic/distributed/worker_service.hpp"
#include "relativistic/core/engine_log.hpp"
#include <atomic>
#include <csignal>
#include <iostream>

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

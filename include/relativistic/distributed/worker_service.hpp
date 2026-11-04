#pragma once

#include "relativistic/distributed/worker_options.hpp"
#include <atomic>
#include <cstdint>
#include <memory>
#include <string>

namespace Relativistic::Distributed {

struct WorkerStatistics {
	uint64_t sessions_accepted{0};
	uint64_t tiles_received{0};
	uint64_t tiles_completed{0};
	uint64_t tiles_cancelled{0};
	uint64_t tiles_rejected{0};
	uint64_t tiles_expired{0};
	uint64_t tiles_on_gpu{0};
	uint64_t tiles_on_cpu{0};
	uint64_t packets_rejected{0};
	uint64_t catalogs_received{0};
};

class ClusterWorkerService {
private:
	struct Impl;
	std::unique_ptr<Impl> impl_;

public:
	explicit ClusterWorkerService(const WorkerOptions& options);
	~ClusterWorkerService();

	ClusterWorkerService(const ClusterWorkerService&) = delete;
	ClusterWorkerService& operator=(const ClusterWorkerService&) = delete;
	ClusterWorkerService(ClusterWorkerService&&) = delete;
	ClusterWorkerService& operator=(ClusterWorkerService&&) = delete;

	void bind_stop_flag(const std::atomic<bool>* flag) noexcept;

	[[nodiscard]] bool start();

	void run();

	void request_stop() noexcept;

	[[nodiscard]] WorkerStatistics statistics() const noexcept;

	[[nodiscard]] std::string last_error() const;
};

}

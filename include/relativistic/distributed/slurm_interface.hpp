#pragma once

#include "relativistic/distributed/worker_options.hpp"
#include <string>

namespace Relativistic::Distributed {

class SlurmInterface {
public:
	[[nodiscard]] static bool is_running_under_slurm() noexcept;
	
	[[nodiscard]] static bool initialize_from_environment(WorkerOptions& options, std::string& error) noexcept;
	
	[[nodiscard]] static uint32_t get_slurm_rank() noexcept;
	[[nodiscard]] static uint32_t get_slurm_world_size() noexcept;
	[[nodiscard]] static std::string get_master_node_address() noexcept;
};

}

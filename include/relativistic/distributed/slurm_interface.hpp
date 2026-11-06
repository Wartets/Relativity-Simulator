#pragma once

#include "relativistic/distributed/cluster_config.hpp"
#include "relativistic/distributed/worker_options.hpp"
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>
#include <string>

namespace Relativistic::Distributed {

struct SlurmAllocation {
	uint32_t rank{0};
	uint32_t task_count{1};
	std::string job_id{};
	std::vector<std::string> hosts{};
	std::vector<uint32_t> tasks_per_host{};
};

class SlurmInterface {
public:
	[[nodiscard]] static bool is_running_under_slurm() noexcept;
	
	[[nodiscard]] static bool initialize_from_environment(WorkerOptions& options, std::string& error) noexcept;
	
	[[nodiscard]] static uint32_t get_slurm_rank() noexcept;
	[[nodiscard]] static uint32_t get_slurm_world_size() noexcept;
	[[nodiscard]] static std::string get_master_node_address() noexcept;
	[[nodiscard]] static std::optional<std::array<uint8_t, 32>> get_job_token() noexcept;

	[[nodiscard]] static std::vector<std::string> expand_node_list(std::string_view node_list);
	[[nodiscard]] static std::vector<uint32_t> parse_tasks_per_node(std::string_view text, size_t host_count);
	[[nodiscard]] static std::optional<SlurmAllocation> read_allocation();
	[[nodiscard]] static bool populate_master_configuration(ClusterConfiguration& configuration, std::string& error);
};

}

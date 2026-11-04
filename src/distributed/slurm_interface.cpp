#include "relativistic/distributed/slurm_interface.hpp"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <string_view>

namespace Relativistic::Distributed {

namespace {

[[nodiscard]] std::string clean_trim(std::string_view text) {
	size_t start = 0;
	while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start])) != 0) {
		++start;
	}
	size_t end = text.size();
	while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1U])) != 0) {
		--end;
	}
	return std::string(text.substr(start, end - start));
}

[[nodiscard]] const char* check_environment(std::initializer_list<const char*> names) noexcept {
	for (const char* name : names) {
		const char* value = std::getenv(name);
		if (value != nullptr && value[0] != '\0') {
			return value;
		}
	}
	return nullptr;
}

[[nodiscard]] std::string parse_nodelist_head(std::string_view nodelist) {
	const std::string text = clean_trim(nodelist);
	if (text.empty()) {
		return {};
	}
	const size_t bracket = text.find('[');
	const size_t comma = text.find(',');
	if (bracket == std::string::npos || (comma != std::string::npos && comma < bracket)) {
		return text.substr(0, comma);
	}
	const std::string prefix = text.substr(0, bracket);
	const size_t close = text.find(']', bracket);
	const size_t inside_len = ((close == std::string::npos) ? text.size() : close) - bracket - 1U;
	const std::string inside = text.substr(bracket + 1U, inside_len);
	const std::string first_item = inside.substr(0, inside.find(','));
	return prefix + first_item.substr(0, first_item.find('-'));
}

}

bool SlurmInterface::is_running_under_slurm() noexcept {
	return check_environment({"SLURM_JOB_ID", "SLURM_PROCID"}) != nullptr;
}

uint32_t SlurmInterface::get_slurm_rank() noexcept {
	const char* rank_str = check_environment({"SLURM_PROCID"});
	if (rank_str == nullptr) {
		return 0;
	}
	char* end = nullptr;
	const unsigned long val = std::strtoul(rank_str, &end, 10);
	return (end != rank_str) ? static_cast<uint32_t>(val) : 0;
}

uint32_t SlurmInterface::get_slurm_world_size() noexcept {
	const char* tasks_str = check_environment({"SLURM_NTASKS"});
	if (tasks_str == nullptr) {
		return 1;
	}
	char* end = nullptr;
	const unsigned long val = std::strtoul(tasks_str, &end, 10);
	return (end != tasks_str && val > 0) ? static_cast<uint32_t>(val) : 1;
}

std::string SlurmInterface::get_master_node_address() noexcept {
	const char* nodes = check_environment({"SLURM_STEP_NODELIST", "SLURM_JOB_NODELIST", "SLURM_NODELIST"});
	if (nodes == nullptr) {
		return {};
	}
	return parse_nodelist_head(nodes);
}

bool SlurmInterface::initialize_from_environment(WorkerOptions& options, std::string& error) noexcept {
	if (!is_running_under_slurm()) {
		error = "Slurm environment variables are not present";
		return false;
	}
	const uint32_t rank = get_slurm_rank();
	const uint32_t world_size = get_slurm_world_size();
	options.slurm_mode = true;
	options.slurm_rank = rank;
	options.slurm_task_count = world_size;

	if (options.node_name.empty()) {
		options.node_name = "slurm-rank-" + std::to_string(rank);
	}

	const uint16_t base_port = options.listen_port;
	if (rank == 0U) {
		options.master_host.clear();
		return true;
	}

	const char* local_id_str = check_environment({"SLURM_LOCALID"});
	uint32_t local_id = 0;
	if (local_id_str != nullptr) {
		char* end = nullptr;
		local_id = static_cast<uint32_t>(std::strtoul(local_id_str, &end, 10));
	}

	const uint64_t candidate_port = static_cast<uint64_t>(base_port) + 1U + local_id;
	if (candidate_port > 65535U) {
		error = "Derived Slurm worker port exceeds 65535";
		return false;
	}
	options.listen_port = static_cast<uint16_t>(candidate_port);

	if (!options.has_master()) {
		options.master_host = get_master_node_address();
		options.master_port = base_port;
		if (options.master_host.empty()) {
			error = "Could not resolve Slurm master hostname from nodelist";
			return false;
		}
	}
	return true;
}

}

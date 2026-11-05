#include "relativistic/distributed/slurm_interface.hpp"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstdio>
#include <string_view>
#include <vector>

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

namespace {

[[nodiscard]] std::vector<std::string> split_top_level(std::string_view text) {
	std::vector<std::string> tokens;
	std::string current;
	int depth = 0;
	for (const char c : text) {
		if (c == '[') {
			++depth;
		} else if (c == ']' && depth > 0) {
			--depth;
		}
		if (c == ',' && depth == 0) {
			if (!current.empty()) {
				tokens.push_back(current);
			}
			current.clear();
			continue;
		}
		current.push_back(c);
	}
	if (!current.empty()) {
		tokens.push_back(current);
	}
	return tokens;
}

void expand_bracket_token(std::string_view token, std::vector<std::string>& output) {
	const size_t open = token.find('[');
	if (open == std::string_view::npos) {
		output.emplace_back(token);
		return;
	}
	const size_t close = token.find(']', open);
	if (close == std::string_view::npos) {
		return;
	}
	const std::string prefix(token.substr(0, open));
	const std::string suffix(token.substr(close + 1U));
	const std::string_view inside = token.substr(open + 1U, close - open - 1U);
	size_t begin = 0;
	while (begin <= inside.size()) {
		size_t end = inside.find(',', begin);
		if (end == std::string_view::npos) {
			end = inside.size();
		}
		const std::string_view item = inside.substr(begin, end - begin);
		begin = end + 1U;
		if (item.empty()) {
			continue;
		}
		const size_t dash = item.find('-');
		if (dash == std::string_view::npos) {
			output.push_back(prefix + std::string(item) + suffix);
			continue;
		}
		const std::string low(item.substr(0, dash));
		const std::string high(item.substr(dash + 1U));
		const unsigned long long first = std::strtoull(low.c_str(), nullptr, 10);
		const unsigned long long last = std::strtoull(high.c_str(), nullptr, 10);
		if (last < first || last - first > 65535ULL) {
			continue;
		}
		for (unsigned long long value = first; value <= last; ++value) {
			std::string digits = std::to_string(value);
			if (digits.size() < low.size()) {
				digits.insert(0, low.size() - digits.size(), '0');
			}
			output.push_back(prefix + digits + suffix);
		}
	}
}

[[nodiscard]] std::vector<std::string> query_scontrol_hostnames(const std::string& list) {
	std::vector<std::string> hosts;
#if !defined(_WIN32)
	if (list.empty() || list.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789[],-_.") != std::string::npos) {
		return hosts;
	}
	const std::string command = "scontrol show hostnames '" + list + "' 2>/dev/null";
	FILE* pipe = popen(command.c_str(), "r");
	if (pipe == nullptr) {
		return hosts;
	}
	char line[256];
	while (std::fgets(line, sizeof(line), pipe) != nullptr) {
		std::string host = clean_trim(line);
		if (!host.empty()) {
			hosts.push_back(std::move(host));
		}
	}
	pclose(pipe);
#else
	static_cast<void>(list);
#endif
	return hosts;
}

}

std::vector<std::string> SlurmInterface::expand_node_list(std::string_view node_list) {
	const std::string text = clean_trim(node_list);
	std::vector<std::string> hosts = query_scontrol_hostnames(text);
	if (!hosts.empty()) {
		return hosts;
	}
	for (const std::string& token : split_top_level(text)) {
		expand_bracket_token(token, hosts);
	}
	return hosts;
}

std::vector<uint32_t> SlurmInterface::parse_tasks_per_node(std::string_view text, size_t host_count) {
	std::vector<uint32_t> counts;
	for (const std::string& token : split_top_level(text)) {
		const unsigned long value = std::max(std::strtoul(token.c_str(), nullptr, 10), 1UL);
		unsigned long repeat = 1;
		const size_t paren = token.find('(');
		if (paren != std::string::npos) {
			const size_t marker = token.find('x', paren);
			if (marker != std::string::npos) {
				repeat = std::clamp(std::strtoul(token.c_str() + marker + 1U, nullptr, 10), 1UL, 65536UL);
			}
		}
		for (unsigned long r = 0; r < repeat; ++r) {
			counts.push_back(static_cast<uint32_t>(value));
		}
	}
	if (counts.size() != host_count) {
		counts.assign(host_count, counts.empty() ? 1U : counts.front());
	}
	return counts;
}

std::optional<SlurmAllocation> SlurmInterface::read_allocation() {
	if (!is_running_under_slurm()) {
		return std::nullopt;
	}
	SlurmAllocation allocation;
	allocation.rank = get_slurm_rank();
	allocation.task_count = get_slurm_world_size();
	if (const char* job = check_environment({"SLURM_JOB_ID"})) {
		allocation.job_id = job;
	}
	const char* nodes = check_environment({"SLURM_STEP_NODELIST", "SLURM_JOB_NODELIST", "SLURM_NODELIST"});
	if (nodes == nullptr) {
		return allocation;
	}
	allocation.hosts = expand_node_list(nodes);
	const char* layout = check_environment({"SLURM_STEP_TASKS_PER_NODE", "SLURM_TASKS_PER_NODE"});
	allocation.tasks_per_host = parse_tasks_per_node((layout != nullptr) ? std::string_view(layout) : std::string_view{}, allocation.hosts.size());
	return allocation;
}

bool SlurmInterface::populate_master_configuration(ClusterConfiguration& configuration, std::string& error) {
	const std::optional<SlurmAllocation> allocation = read_allocation();
	if (!allocation.has_value() || allocation->rank != 0U) {
		error = "The master configuration can only be derived on rank 0 of a Slurm allocation";
		return false;
	}
	if (allocation->hosts.empty()) {
		error = "The Slurm node list could not be expanded";
		return false;
	}
	const uint32_t base_port = configuration.cluster.listen_port;
	size_t added = 0;
	for (size_t host_index = 0; host_index < allocation->hosts.size(); ++host_index) {
		const uint32_t tasks = allocation->tasks_per_host[host_index];
		for (uint32_t local_id = 0; local_id < tasks; ++local_id) {
			if (host_index == 0U && local_id == 0U) {
				continue;
			}
			const uint32_t port = base_port + 1U + local_id;
			if (port > 65535U) {
				error = "A derived Slurm worker port exceeds the valid port range";
				return added > 0U;
			}
			const std::string& host = allocation->hosts[host_index];
			const bool known = std::any_of(configuration.static_nodes.begin(), configuration.static_nodes.end(), [&host, port](const ClusterNodeDescriptor& node) {
				return node.endpoint.host == host && node.endpoint.port == port;
			});
			if (known) {
				continue;
			}
			ClusterNodeDescriptor descriptor{};
			descriptor.name = "slurm-" + host + "-" + std::to_string(local_id);
			descriptor.endpoint = TransportEndpoint{host, static_cast<uint16_t>(port)};
			descriptor.static_weight = 1.0;
			descriptor.enabled = true;
			configuration.static_nodes.push_back(std::move(descriptor));
			++added;
		}
	}
	if (added > 0U) {
		configuration.cluster.enabled = true;
	}
	return added > 0U;
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

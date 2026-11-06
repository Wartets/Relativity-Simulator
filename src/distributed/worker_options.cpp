#include "relativistic/distributed/worker_options.hpp"
#include "relativistic/distributed/slurm_interface.hpp"
#include <algorithm>
#include <filesystem>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <optional>
#include <ostream>
#include <string_view>
#include <vector>

namespace Relativistic::Distributed {

namespace {

constexpr const char* kTokenEnvironmentVariable = "RELATIVISTIC_CLUSTER_TOKEN";

[[nodiscard]] std::string trimmed(std::string_view text) {
	size_t first = 0;
	while (first < text.size() && std::isspace(static_cast<unsigned char>(text[first])) != 0) {
		++first;
	}
	size_t last = text.size();
	while (last > first && std::isspace(static_cast<unsigned char>(text[last - 1U])) != 0) {
		--last;
	}
	return std::string(text.substr(first, last - first));
}

[[nodiscard]] std::string lowered(std::string_view text) {
	std::string result(text);
	std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	return result;
}

[[nodiscard]] bool parse_unsigned(std::string_view text, uint64_t maximum, uint64_t& value) noexcept {
	if (text.empty()) {
		return false;
	}
	uint64_t parsed = 0;
	for (const char c : text) {
		if (c < '0' || c > '9') {
			return false;
		}
		const auto digit = static_cast<uint64_t>(c - '0');
		if (maximum < digit || parsed > (maximum - digit) / 10U) {
			return false;
		}
		parsed = parsed * 10U + digit;
	}
	value = parsed;
	return true;
}

[[nodiscard]] bool parse_real(std::string_view text, double& value) {
	const std::string owned(text);
	if (owned.empty()) {
		return false;
	}
	char* end = nullptr;
	const double parsed = std::strtod(owned.c_str(), &end);
	if (end != owned.c_str() + owned.size() || !std::isfinite(parsed)) {
		return false;
	}
	value = parsed;
	return true;
}

[[nodiscard]] bool parse_boolean(std::string_view text, bool& value) {
	const std::string normalized = lowered(trimmed(text));
	if (normalized == "1" || normalized == "true" || normalized == "yes" || normalized == "on") {
		value = true;
		return true;
	}
	if (normalized == "0" || normalized == "false" || normalized == "no" || normalized == "off") {
		value = false;
		return true;
	}
	return false;
}

[[nodiscard]] int hex_nibble(char c) noexcept {
	if (c >= '0' && c <= '9') {
		return c - '0';
	}
	if (c >= 'a' && c <= 'f') {
		return c - 'a' + 10;
	}
	if (c >= 'A' && c <= 'F') {
		return c - 'A' + 10;
	}
	return -1;
}

[[nodiscard]] bool decode_token(std::string_view text, std::array<uint8_t, 32>& token) noexcept {
	if (text.size() != 64U) {
		return false;
	}
	std::array<uint8_t, 32> decoded{};
	for (size_t i = 0; i < decoded.size(); ++i) {
		const int high = hex_nibble(text[2U * i]);
		const int low = hex_nibble(text[2U * i + 1U]);
		if (high < 0 || low < 0) {
			return false;
		}
		decoded[i] = static_cast<uint8_t>((high << 4) | low);
	}
	token = decoded;
	return true;
}

[[nodiscard]] bool parse_port(std::string_view text, uint16_t& port) noexcept {
	uint64_t value = 0;
	if (!parse_unsigned(text, 65535U, value) || value == 0U) {
		return false;
	}
	port = static_cast<uint16_t>(value);
	return true;
}

[[nodiscard]] bool parse_endpoint(std::string_view text, std::string& host, uint16_t& port) {
	const size_t colon = text.rfind(':');
	if (colon == std::string_view::npos) {
		host = trimmed(text);
		port = kDefaultClusterPort;
		return !host.empty();
	}
	host = trimmed(text.substr(0, colon));
	return !host.empty() && parse_port(text.substr(colon + 1U), port);
}

[[nodiscard]] bool apply_token_text(std::string_view text, WorkerOptions& options, std::string& error) {
	if (!decode_token(trimmed(text), options.auth_token)) {
		error = "The cluster token must be exactly 64 hexadecimal characters";
		return false;
	}
	options.has_auth_token = true;
	return true;
}

[[nodiscard]] bool apply_configuration_entry(const std::string& section, const std::string& key, const std::string& value, WorkerOptions& options, std::string& error) {
	if (section == "cluster") {
		if (key == "listen_port") {
			if (!parse_port(value, options.listen_port)) {
				error = "Invalid listen_port in the cluster configuration";
				return false;
			}
		} else if (key == "listen_interface") {
			options.listen_interface = value;
		} else if (key == "cluster_name") {
			options.cluster_name = value;
		} else if (key == "discovery_multicast_enabled") {
			if (!parse_boolean(value, options.discovery_enabled)) {
				error = "Invalid discovery_multicast_enabled in the cluster configuration";
				return false;
			}
		} else if (key == "discovery_multicast_group") {
			options.discovery_group = value;
		} else if (key == "discovery_multicast_port") {
			if (!parse_port(value, options.discovery_port)) {
				error = "Invalid discovery_multicast_port in the cluster configuration";
				return false;
			}
		}
	} else if (section == "security") {
		if (key == "cluster_shared_token") {
			return apply_token_text(value, options, error);
		}
	}
	return true;
}

[[nodiscard]] bool load_configuration_file(const std::string& path, WorkerOptions& options, std::string& error) {
	std::ifstream file(path);
	if (!file.is_open()) {
		error = "Cannot open the cluster configuration file '" + path + "'";
		return false;
	}
	std::string section;
	std::string line;
	while (std::getline(file, line)) {
		const std::string text = trimmed(line);
		if (text.empty() || text.front() == ';' || text.front() == '#') {
			continue;
		}
		if (text.front() == '[') {
			const size_t close = text.find(']');
			section = (close == std::string::npos) ? std::string{} : lowered(trimmed(text.substr(1, close - 1U)));
			continue;
		}
		const size_t equals = text.find('=');
		if (equals == std::string::npos) {
			continue;
		}
		const std::string key = lowered(trimmed(text.substr(0, equals)));
		const std::string value = trimmed(text.substr(equals + 1U));
		if (!apply_configuration_entry(section, key, value, options, error)) {
			return false;
		}
	}
	return true;
}
}

bool parse_worker_options(int argument_count, const char* const* arguments, WorkerOptions& options, std::string& error) {
	std::vector<std::string> tokens;
	tokens.reserve(static_cast<size_t>(std::max(argument_count - 1, 0)));
	for (int i = 1; i < argument_count; ++i) {
		tokens.emplace_back(arguments[i]);
	}

	for (size_t i = 0; i < tokens.size(); ++i) {
		std::string_view name(tokens[i]);
		std::optional<std::string> inline_value;
		const size_t equals = name.find('=');
		if (name.rfind("--", 0) == 0 && equals != std::string_view::npos) {
			inline_value = std::string(name.substr(equals + 1U));
			name = name.substr(0, equals);
		}
		if (name == "--cluster-config") {
			if (inline_value.has_value()) {
				options.cluster_config_path = *inline_value;
			} else if (i + 1U < tokens.size()) {
				options.cluster_config_path = tokens[i + 1U];
			}
		}
	}

	if (options.cluster_config_path.empty()) {
		std::error_code ec;
		if (std::filesystem::exists("config/distributed_cluster.cfg", ec)) {
			options.cluster_config_path = "config/distributed_cluster.cfg";
		}
	}

	if (!options.cluster_config_path.empty() && !load_configuration_file(options.cluster_config_path, options, error)) {
		return false;
	}
	if (const char* token_text = std::getenv(kTokenEnvironmentVariable)) {
		if (!apply_token_text(token_text, options, error)) {
			return false;
		}
	}

	for (size_t index = 0; index < tokens.size(); ++index) {
		std::string name = tokens[index];
		std::optional<std::string> inline_value;
		const size_t equals = name.find('=');
		if (name.rfind("--", 0) == 0 && equals != std::string::npos) {
			inline_value = name.substr(equals + 1U);
			name.resize(equals);
		}
		const auto take_value = [&](std::string& output) -> bool {
			if (inline_value.has_value()) {
				output = *inline_value;
				return true;
			}
			if (index + 1U >= tokens.size()) {
				error = "The option " + name + " requires a value";
				return false;
			}
			output = tokens[++index];
			return true;
		};

		std::string value;
		if (name == "--help" || name == "-h" || name == "-?") {
			options.show_help = true;
		} else if (name == "--version") {
			options.show_version = true;
		} else if (name == "--headless" || name == "--verbose") {
			continue;
		} else if (name == "--slurm-mode") {
			options.slurm_mode = true;
		} else if (name == "--no-listen") {
			options.listen_enabled = false;
		} else if (name == "--no-discovery") {
			options.discovery_enabled = false;
		} else if (name == "--master-address") {
			if (!take_value(value)) {
				return false;
			}
			if (!parse_endpoint(value, options.master_host, options.master_port)) {
				error = "Invalid master address '" + value + "', expected host:port";
				return false;
			}
		} else if (name == "--auth-token") {
			if (!take_value(value) || !apply_token_text(value, options, error)) {
				return false;
			}
		} else if (name == "--auth-token-file") {
			if (!take_value(value)) {
				return false;
			}
			std::ifstream file(value);
			std::string content;
			if (!file.is_open() || !std::getline(file, content)) {
				error = "Cannot read the token file '" + value + "'";
				return false;
			}
			if (!apply_token_text(content, options, error)) {
				return false;
			}
		} else if (name == "--listen-port") {
			if (!take_value(value) || !parse_port(value, options.listen_port)) {
				if (error.empty()) {
					error = "Invalid listen port '" + value + "'";
				}
				return false;
			}
		} else if (name == "--listen-interface") {
			if (!take_value(value)) {
				return false;
			}
			options.listen_interface = value;
		} else if (name == "--threads") {
			uint64_t parsed = 0;
			if (!take_value(value) || !parse_unsigned(value, 1024U, parsed)) {
				if (error.empty()) {
					error = "Invalid thread count '" + value + "'";
				}
				return false;
			}
			options.thread_count = static_cast<size_t>(parsed);
		} else if (name == "--vulkan-device") {
			if (!take_value(value)) {
				return false;
			}
			if (lowered(value) == "auto") {
				options.vulkan_device = -1;
			} else {
				uint64_t parsed = 0;
				if (!parse_unsigned(value, 1023U, parsed)) {
					error = "Invalid Vulkan device '" + value + "', expected an index or 'auto'";
					return false;
				}
				options.vulkan_device = static_cast<int>(parsed);
			}
		} else if (name == "--backend") {
			if (!take_value(value)) {
				return false;
			}
			const std::string backend = lowered(value);
			if (backend == "auto") {
				options.backend = WorkerBackend::Auto;
			} else if (backend == "gpu") {
				options.backend = WorkerBackend::Gpu;
			} else if (backend == "cpu") {
				options.backend = WorkerBackend::Cpu;
			} else {
				error = "Invalid backend '" + value + "', expected auto, gpu or cpu";
				return false;
			}
		} else if (name == "--weight") {
			double parsed = 0.0;
			if (!take_value(value) || !parse_real(value, parsed) || !(parsed > 0.0)) {
				if (error.empty()) {
					error = "Invalid weight '" + value + "', expected a positive number";
				}
				return false;
			}
			options.performance_weight = parsed;
		} else if (name == "--name") {
			if (!take_value(value)) {
				return false;
			}
			options.node_name = value;
		} else if (name == "--cluster-name") {
			if (!take_value(value)) {
				return false;
			}
			options.cluster_name = value;
		} else if (name == "--discovery-interface") {
			if (!take_value(value)) {
				return false;
			}
			options.discovery_interface = value;
		} else if (name == "--discovery-group") {
			if (!take_value(value)) {
				return false;
			}
			options.discovery_group = value;
		} else if (name == "--discovery-port") {
			if (!take_value(value) || !parse_port(value, options.discovery_port)) {
				if (error.empty()) {
					error = "Invalid discovery port '" + value + "'";
				}
				return false;
			}
		} else if (name == "--reconnect-interval-ms") {
			uint64_t parsed = 0;
			if (!take_value(value) || !parse_unsigned(value, 600000U, parsed) || parsed < 100U) {
				if (error.empty()) {
					error = "Invalid reconnect interval '" + value + "'";
				}
				return false;
			}
			options.reconnect_interval_ms = static_cast<uint32_t>(parsed);
		} else if (name == "--cluster-config") {
			if (!take_value(value)) {
				return false;
			}
		} else {
			error = "Unrecognized option '" + tokens[index] + "'";
			return false;
		}
	}

	if (options.show_help || options.show_version) {
		return true;
	}
	if (options.slurm_mode || SlurmInterface::is_running_under_slurm()) {
		if (!SlurmInterface::initialize_from_environment(options, error)) {
			return false;
		}
	}
	if (!options.has_auth_token) {
		error = "A cluster token is required (--auth-token, --auth-token-file, the configuration file or " + std::string(kTokenEnvironmentVariable) + ")";
		return false;
	}
	if (!options.listen_enabled && !options.has_master()) {
		error = "The worker needs either a listening port or a master address";
		return false;
	}
	return true;
}

void print_worker_usage(std::ostream& stream) {
	stream << "Usage: cluster_worker [options]\n"
	       << "  --master-address <host:port>   Connect to a master (retried automatically)\n"
	       << "  --auth-token <hex64>           Pre-shared cluster token (64 hexadecimal characters)\n"
	       << "  --auth-token-file <path>       Read the cluster token from the first line of a file\n"
	       << "  --listen-port <port>           Port accepting master connections (default " << kDefaultClusterPort << ")\n"
	       << "  --listen-interface <address>   Interface address to bind (default 0.0.0.0)\n"
	       << "  --no-listen                    Do not accept inbound connections\n"
	       << "  --threads <n>                  Render worker threads (default: all logical processors)\n"
	       << "  --vulkan-device <index|auto>   Vulkan physical device selection\n"
	       << "  --backend <auto|gpu|cpu>       Tile execution policy (default auto)\n"
	       << "  --weight <value>               Static performance weight override\n"
	       << "  --name <text>                  Node name announced to masters\n"
	       << "  --cluster-name <text>          Cluster name used for discovery matching\n"
	       << "  --no-discovery                 Disable multicast discovery announcements\n"
	       << "  --discovery-interface <addr>   Interface used for multicast discovery\n"
	       << "  --discovery-group <addr>       Multicast group (default " << kDefaultMulticastGroup << ")\n"
	       << "  --discovery-port <port>        Multicast port (default " << kDefaultDiscoveryPort << ")\n"
	       << "  --reconnect-interval-ms <ms>   Delay between master connection attempts\n"
	       << "  --cluster-config <path>        Read listen, discovery and token entries from a cluster configuration file\n"
	       << "  --slurm-mode                   Derive rank, ports and master host from the Slurm environment\n"
	       << "  --headless                     Accepted for compatibility, the worker never opens a window\n"
	       << "  --version                      Print the version\n"
	       << "  --help                         Print this message\n";
}

}

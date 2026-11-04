#pragma once

#include "relativistic/distributed/packet_protocol.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <span>
#include <string>

namespace Relativistic::Distributed {

enum class WorkerBackend : uint8_t {
	Auto = 0,
	Gpu = 1,
	Cpu = 2
};

struct WorkerOptions {
	std::string master_host{};
	uint16_t master_port{kDefaultClusterPort};
	std::string listen_interface{"0.0.0.0"};
	uint16_t listen_port{kDefaultClusterPort};
	bool listen_enabled{true};
	std::array<uint8_t, 32> auth_token{};
	bool has_auth_token{false};
	size_t thread_count{0};
	int vulkan_device{-1};
	WorkerBackend backend{WorkerBackend::Auto};
	double performance_weight{0.0};
	std::string node_name{};
	std::string cluster_name{"Relativistic_Alpha_Mesh"};
	bool discovery_enabled{true};
	std::string discovery_group{std::string(kDefaultMulticastGroup)};
	uint16_t discovery_port{kDefaultDiscoveryPort};
	std::string discovery_interface{};
	bool slurm_mode{false};
	uint32_t slurm_rank{0};
	uint32_t slurm_task_count{1};
	std::string cluster_config_path{};
	uint32_t reconnect_interval_ms{2000};
	bool show_help{false};
	bool show_version{false};
	std::span<const uint32_t> embedded_tile_shader{};

	[[nodiscard]] bool has_master() const noexcept {
		return !master_host.empty();
	}
};

[[nodiscard]] bool parse_worker_options(int argument_count, const char* const* arguments, WorkerOptions& options, std::string& error);

void print_worker_usage(std::ostream& stream);

}

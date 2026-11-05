#pragma once

#include "relativistic/distributed/cluster_node.hpp"
#include "relativistic/distributed/distribution_scheduler.hpp"
#include "relativistic/distributed/compression_codecs.hpp"
#include "relativistic/distributed/discovery_service.hpp"
#include "relativistic/distributed/network_transport.hpp"
#include "relativistic/distributed/packet_protocol.hpp"
#include <string>
#include <vector>
#include <array>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cmath>

namespace Relativistic::Distributed {

struct ClusterNetworkConfig {
	bool enabled{false};
	ClusterNodeRole node_role{ClusterNodeRole::Standalone};
	std::string cluster_name{"Relativistic_Alpha_Mesh"};
	std::string listen_interface{"0.0.0.0"};
	uint16_t listen_port{kDefaultClusterPort};
	bool discovery_multicast_enabled{true};
	std::string discovery_multicast_group{std::string(kDefaultMulticastGroup)};
	uint16_t discovery_multicast_port{kDefaultDiscoveryPort};
	uint32_t heartbeat_interval_ms{kDefaultHeartbeatIntervalMs};
	uint32_t node_timeout_ms{kDefaultNodeTimeoutMs};
};

struct ClusterPerformanceConfig {
	bool dynamic_load_balancing{true};
	double latency_penalty_alpha{1.0};
	bool work_stealing_enabled{true};
	uint32_t tile_timeout_ms{120};
	uint32_t max_inflight_frames{2};
	uint32_t compression_mode{2};
	int lz4_acceleration_level{1};
	bool auto_benchmark_nodes{true};
};

struct ClusterSecurityConfig {
	bool require_encryption{true};
	uint32_t cipher_suite{1};
	std::string tls_certificate_path{};
	std::string tls_private_key_path{};
	std::string tls_ca_chain_path{};
	std::string cluster_shared_token_hex{};
	std::array<uint8_t, 32> cluster_shared_token{};
	bool has_shared_token{false};
	bool allow_unauthenticated_lan{false};
	bool sanitize_remote_data{true};

	bool apply_token_hex(std::string_view hex) noexcept {
		if (hex.size() != 64U) {
			return false;
		}
		auto parse_nibble = [](char c) noexcept -> int {
			if (c >= '0' && c <= '9') return c - '0';
			if (c >= 'a' && c <= 'f') return c - 'a' + 10;
			if (c >= 'A' && c <= 'F') return c - 'A' + 10;
			return -1;
		};
		std::array<uint8_t, 32> parsed{};
		for (size_t i = 0; i < 32; ++i) {
			const int hi = parse_nibble(hex[2 * i]);
			const int lo = parse_nibble(hex[2 * i + 1]);
			if (hi < 0 || lo < 0) {
				return false;
			}
			parsed[i] = static_cast<uint8_t>((hi << 4) | lo);
		}
		cluster_shared_token = parsed;
		cluster_shared_token_hex = std::string(hex);
		has_shared_token = true;
		return true;
	}
};

struct ClusterTaskRoutingConfig {
	bool route_interactive_viewport{true};
	bool route_capture_studio{true};
	bool route_monte_carlo_ensembles{true};
	bool route_nbody_interactions{false};
	bool route_exact_spacetime_metrics{true};
};

struct ClusterConfiguration {
	uint32_t format_version{1};
	ClusterNetworkConfig cluster{};
	ClusterPerformanceConfig performance{};
	ClusterSecurityConfig security{};
	ClusterTaskRoutingConfig task_routing{};
	std::vector<ClusterNodeDescriptor> static_nodes{};

	[[nodiscard]] LoadBalancerParameters make_load_balancer_parameters() const noexcept {
		LoadBalancerParameters params{};
		params.dynamic_balancing = performance.dynamic_load_balancing;
		params.work_stealing = performance.work_stealing_enabled;
		params.latency_penalty_alpha = performance.latency_penalty_alpha;
		params.maximum_timeout_seconds = static_cast<double>(performance.tile_timeout_ms) * 1.0e-3;
		params.minimum_timeout_seconds = std::min(0.004, params.maximum_timeout_seconds * 0.1);
		return params;
	}

	[[nodiscard]] TransportConfiguration make_transport_configuration() const noexcept {
		TransportConfiguration config{};
		config.maximum_peers = 64;
		config.ping_interval_ms = cluster.heartbeat_interval_ms;
		config.timeout_minimum_ms = cluster.node_timeout_ms;
		config.timeout_maximum_ms = cluster.node_timeout_ms + 500;
		return config;
	}

	[[nodiscard]] DiscoveryConfiguration make_discovery_configuration(
		const std::string& node_name,
		uint32_t abi_sig,
		float weight
	) const noexcept {
		DiscoveryConfiguration config{};
		config.multicast_group = cluster.discovery_multicast_group;
		config.port = cluster.discovery_multicast_port;
		config.interface_address = cluster.listen_interface;
		config.role = cluster.node_role;
		config.service_port = cluster.listen_port;
		config.node_name = node_name;
		config.cluster_name = cluster.cluster_name;
		config.cluster_token = security.cluster_shared_token;
		config.abi_signature = abi_sig;
		config.performance_weight = weight;
		return config;
	}

	bool apply_token_hex(std::string_view hex) noexcept {
		return security.apply_token_hex(hex);
	}

	bool load_from_file(const std::string& path) {
		std::ifstream file(path);
		if (!file.is_open()) {
			return false;
		}
		auto trim = [](std::string_view s) noexcept -> std::string {
			size_t p0 = 0;
			while (p0 < s.size() && std::isspace(static_cast<unsigned char>(s[p0]))) ++p0;
			size_t p1 = s.size();
			while (p1 > p0 && std::isspace(static_cast<unsigned char>(s[p1 - 1]))) --p1;
			return std::string(s.substr(p0, p1 - p0));
		};
		auto lower = [](std::string_view s) -> std::string {
			std::string r(s);
			std::transform(r.begin(), r.end(), r.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
			return r;
		};

		std::string section;
		std::string line;
		ClusterNodeDescriptor current_node{};
		bool in_node_section = false;

		auto commit_current_node = [&]() {
			if (in_node_section && !current_node.endpoint.host.empty()) {
				static_nodes.push_back(current_node);
			}
			current_node = ClusterNodeDescriptor{};
			in_node_section = false;
		};

		while (std::getline(file, line)) {
			std::string trimmed = trim(line);
			if (trimmed.empty() || trimmed.front() == '#' || trimmed.front() == ';') {
				continue;
			}
			if (trimmed.front() == '[') {
				commit_current_node();
				size_t close = trimmed.find(']');
				if (close != std::string::npos) {
					section = lower(trim(trimmed.substr(1, close - 1)));
					if (section.rfind("node_", 0) == 0) {
						in_node_section = true;
					}
				}
				continue;
			}
			size_t eq = trimmed.find('=');
			if (eq == std::string::npos) {
				continue;
			}
			std::string key = lower(trim(trimmed.substr(0, eq)));
			std::string val = trim(trimmed.substr(eq + 1));

			if (section == "cluster") {
				if (key == "enabled") cluster.enabled = (val == "1" || val == "true");
				else if (key == "node_role") cluster.node_role = static_cast<ClusterNodeRole>(std::strtoul(val.c_str(), nullptr, 10));
				else if (key == "cluster_name") cluster.cluster_name = val;
				else if (key == "listen_interface") cluster.listen_interface = val;
				else if (key == "listen_port") cluster.listen_port = static_cast<uint16_t>(std::strtoul(val.c_str(), nullptr, 10));
				else if (key == "discovery_multicast_enabled") cluster.discovery_multicast_enabled = (val == "1" || val == "true");
				else if (key == "discovery_multicast_group") cluster.discovery_multicast_group = val;
				else if (key == "discovery_multicast_port") cluster.discovery_multicast_port = static_cast<uint16_t>(std::strtoul(val.c_str(), nullptr, 10));
				else if (key == "heartbeat_interval_ms") cluster.heartbeat_interval_ms = static_cast<uint32_t>(std::strtoul(val.c_str(), nullptr, 10));
				else if (key == "node_timeout_ms") cluster.node_timeout_ms = static_cast<uint32_t>(std::strtoul(val.c_str(), nullptr, 10));
			} else if (section == "performance") {
				if (key == "dynamic_load_balancing") performance.dynamic_load_balancing = (val == "1" || val == "true");
				else if (key == "latency_penalty_alpha") performance.latency_penalty_alpha = std::strtod(val.c_str(), nullptr);
				else if (key == "work_stealing_enabled") performance.work_stealing_enabled = (val == "1" || val == "true");
				else if (key == "tile_timeout_ms") performance.tile_timeout_ms = static_cast<uint32_t>(std::strtoul(val.c_str(), nullptr, 10));
				else if (key == "max_inflight_frames") performance.max_inflight_frames = static_cast<uint32_t>(std::strtoul(val.c_str(), nullptr, 10));
				else if (key == "compression_mode") performance.compression_mode = static_cast<uint32_t>(std::strtoul(val.c_str(), nullptr, 10));
				else if (key == "lz4_acceleration_level") performance.lz4_acceleration_level = static_cast<int>(std::strtol(val.c_str(), nullptr, 10));
				else if (key == "auto_benchmark_nodes") performance.auto_benchmark_nodes = (val == "1" || val == "true");
			} else if (section == "security") {
				if (key == "require_encryption") security.require_encryption = (val == "1" || val == "true");
				else if (key == "cipher_suite") security.cipher_suite = static_cast<uint32_t>(std::strtoul(val.c_str(), nullptr, 10));
				else if (key == "tls_certificate_path") security.tls_certificate_path = val;
				else if (key == "tls_private_key_path") security.tls_private_key_path = val;
				else if (key == "tls_ca_chain_path") security.tls_ca_chain_path = val;
				else if (key == "cluster_shared_token") apply_token_hex(val);
				else if (key == "allow_unauthenticated_lan") security.allow_unauthenticated_lan = (val == "1" || val == "true");
				else if (key == "sanitize_remote_data") security.sanitize_remote_data = (val == "1" || val == "true");
			} else if (section == "task_routing") {
				if (key == "route_interactive_viewport") task_routing.route_interactive_viewport = (val == "1" || val == "true");
				else if (key == "route_capture_studio") task_routing.route_capture_studio = (val == "1" || val == "true");
				else if (key == "route_monte_carlo_ensembles") task_routing.route_monte_carlo_ensembles = (val == "1" || val == "true");
				else if (key == "route_nbody_interactions") task_routing.route_nbody_interactions = (val == "1" || val == "true");
				else if (key == "route_exact_spacetime_metrics") task_routing.route_exact_spacetime_metrics = (val == "1" || val == "true");
			} else if (in_node_section) {
				if (key == "enabled") current_node.enabled = (val == "1" || val == "true");
				else if (key == "name") current_node.name = val;
				else if (key == "address") current_node.endpoint.host = val;
				else if (key == "port") current_node.endpoint.port = static_cast<uint16_t>(std::strtoul(val.c_str(), nullptr, 10));
				else if (key == "static_weight") current_node.static_weight = std::strtod(val.c_str(), nullptr);
				else if (key == "preferred_device") current_node.preferred_device = static_cast<uint32_t>(std::strtoul(val.c_str(), nullptr, 10));
				else if (key == "prefer_fp64") current_node.prefer_fp64 = (val == "1" || val == "true");
			}
		}
		commit_current_node();
		return true;
	}

	bool save_to_file(const std::string& path) const {
		std::ofstream out(path);
		if (!out.is_open()) {
			return false;
		}
		out << "format_version=" << format_version << "\n\n"
		    << "[cluster]\n"
		    << "enabled=" << (cluster.enabled ? 1 : 0) << "\n"
		    << "node_role=" << static_cast<uint32_t>(cluster.node_role) << "\n"
		    << "cluster_name=" << cluster.cluster_name << "\n"
		    << "listen_interface=" << cluster.listen_interface << "\n"
		    << "listen_port=" << cluster.listen_port << "\n"
		    << "discovery_multicast_enabled=" << (cluster.discovery_multicast_enabled ? 1 : 0) << "\n"
		    << "discovery_multicast_group=" << cluster.discovery_multicast_group << "\n"
		    << "discovery_multicast_port=" << cluster.discovery_multicast_port << "\n"
		    << "heartbeat_interval_ms=" << cluster.heartbeat_interval_ms << "\n"
		    << "node_timeout_ms=" << cluster.node_timeout_ms << "\n\n"
		    << "[performance]\n"
		    << "dynamic_load_balancing=" << (performance.dynamic_load_balancing ? 1 : 0) << "\n"
		    << "latency_penalty_alpha=" << performance.latency_penalty_alpha << "\n"
		    << "work_stealing_enabled=" << (performance.work_stealing_enabled ? 1 : 0) << "\n"
		    << "tile_timeout_ms=" << performance.tile_timeout_ms << "\n"
		    << "max_inflight_frames=" << performance.max_inflight_frames << "\n"
		    << "compression_mode=" << performance.compression_mode << "\n"
		    << "lz4_acceleration_level=" << performance.lz4_acceleration_level << "\n"
		    << "auto_benchmark_nodes=" << (performance.auto_benchmark_nodes ? 1 : 0) << "\n\n"
		    << "[security]\n"
		    << "require_encryption=" << (security.require_encryption ? 1 : 0) << "\n"
		    << "cipher_suite=" << security.cipher_suite << "\n"
		    << "tls_certificate_path=" << security.tls_certificate_path << "\n"
		    << "tls_private_key_path=" << security.tls_private_key_path << "\n"
		    << "tls_ca_chain_path=" << security.tls_ca_chain_path << "\n"
		    << "cluster_shared_token=" << security.cluster_shared_token_hex << "\n"
		    << "allow_unauthenticated_lan=" << (security.allow_unauthenticated_lan ? 1 : 0) << "\n"
		    << "sanitize_remote_data=" << (security.sanitize_remote_data ? 1 : 0) << "\n\n"
		    << "[task_routing]\n"
		    << "route_interactive_viewport=" << (task_routing.route_interactive_viewport ? 1 : 0) << "\n"
		    << "route_capture_studio=" << (task_routing.route_capture_studio ? 1 : 0) << "\n"
		    << "route_monte_carlo_ensembles=" << (task_routing.route_monte_carlo_ensembles ? 1 : 0) << "\n"
		    << "route_nbody_interactions=" << (task_routing.route_nbody_interactions ? 1 : 0) << "\n"
		    << "route_exact_spacetime_metrics=" << (task_routing.route_exact_spacetime_metrics ? 1 : 0) << "\n\n";

		for (size_t i = 0; i < static_nodes.size(); ++i) {
			const auto& n = static_nodes[i];
			out << "[node_" << (i + 1) << "]\n"
			    << "enabled=" << (n.enabled ? 1 : 0) << "\n"
			    << "name=" << n.name << "\n"
			    << "address=" << n.endpoint.host << "\n"
			    << "port=" << n.endpoint.port << "\n"
			    << "static_weight=" << n.static_weight << "\n"
			    << "preferred_device=" << n.preferred_device << "\n"
			    << "prefer_fp64=" << (n.prefer_fp64 ? 1 : 0) << "\n\n";
		}
		return true;
	}
};

}

#pragma once

#include <imgui.h>
#include "relativistic/ui/tooltip_utils.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <functional>
#include <random>
#include <string>
#include <utility>

#if defined(RELATIVISTIC_DISTRIBUTED_COMPUTE) && RELATIVISTIC_DISTRIBUTED_COMPUTE
#include "relativistic/distributed/cluster_master_service.hpp"
#endif

namespace Relativistic::UI {

#if defined(RELATIVISTIC_DISTRIBUTED_COMPUTE) && RELATIVISTIC_DISTRIBUTED_COMPUTE

class ClusterPanel {
public:
	using ReloadHandler = std::function<void()>;
	using StatusProvider = std::function<Distributed::ClusterStatus()>;

private:
	Distributed::ClusterConfiguration config_{};
	ReloadHandler reload_{};
	StatusProvider status_{};
	bool loaded_{false};
	bool dirty_{false};
	std::string feedback_{};
	std::string token_text_{};
	std::string new_name_{};
	std::string new_host_{};
	uint16_t new_port_{Distributed::kDefaultClusterPort};
	double new_weight_{1.0};

	static bool edit_text(const char* label, std::string& value) {
		char buffer[256];
		std::snprintf(buffer, sizeof(buffer), "%s", value.c_str());
		if (ImGui::InputText(label, buffer, sizeof(buffer))) {
			value = buffer;
			return true;
		}
		return false;
	}

	static bool edit_toggle(const char* label, bool& value) {
		return ImGui::Checkbox(label, &value);
	}

	static bool edit_combo(const char* label, uint32_t& value, const char* const* names, int count) {
		int index = std::min(static_cast<int>(value), count - 1);
		if (ImGui::Combo(label, &index, names, count)) {
			value = static_cast<uint32_t>(index);
			return true;
		}
		return false;
	}

	void load_from_disk() {
		config_ = Distributed::ClusterConfiguration{};
		const std::filesystem::path path = Distributed::ClusterMasterService::configuration_path();
		std::error_code ec;
		if (std::filesystem::exists(path, ec)) {
			feedback_ = config_.load_from_file(path.string()) ? "Configuration loaded from " + path.string() : "The configuration file could not be read";
		} else {
			feedback_ = "No configuration file exists yet; defaults are shown";
		}
		token_text_ = config_.security.cluster_shared_token_hex;
		loaded_ = true;
		dirty_ = false;
	}

	bool save_to_disk() {
		const std::filesystem::path path = Distributed::ClusterMasterService::configuration_path();
		std::error_code ec;
		std::filesystem::create_directories(path.parent_path(), ec);
		if (!config_.save_to_file(path.string())) {
			feedback_ = "The configuration could not be written to " + path.string();
			return false;
		}
		feedback_ = "Configuration saved to " + path.string();
		dirty_ = false;
		return true;
	}

	[[nodiscard]] static std::string generate_token() {
		std::random_device device;
		static constexpr char digits[] = "0123456789abcdef";
		std::string token;
		token.reserve(64);
		for (size_t i = 0; i < 32; ++i) {
			const auto value = static_cast<uint8_t>(device() & 0xFFU);
			token.push_back(digits[value >> 4U]);
			token.push_back(digits[value & 0x0FU]);
		}
		return token;
	}

	void render_cluster_section() {
		static constexpr const char* roles[] = {"Standalone", "Master", "Worker", "Hybrid"};
		auto& cluster = config_.cluster;
		dirty_ |= edit_toggle("Enable Distributed Rendering", cluster.enabled);
		render_setting_tooltip("Master switch of the subsystem. When disabled no socket is opened and rendering stays entirely local.");
		uint32_t role = static_cast<uint32_t>(cluster.node_role);
		if (edit_combo("Node Role", role, roles, 4)) {
			cluster.node_role = static_cast<Distributed::ClusterNodeRole>(role);
			dirty_ = true;
		}
		render_setting_tooltip("Master drives the interactive viewport and the capture studio. Hybrid also accepts tiles from other masters.");
		dirty_ |= edit_text("Cluster Name", cluster.cluster_name);
		render_setting_tooltip("Name used to match discovery announcements; it is hashed together with the shared token.");
		dirty_ |= edit_text("Listen Interface", cluster.listen_interface);
		int port = static_cast<int>(cluster.listen_port);
		if (ImGui::InputInt("Listen Port", &port)) {
			cluster.listen_port = static_cast<uint16_t>(std::clamp(port, 1, 65535));
			dirty_ = true;
		}
		dirty_ |= edit_toggle("Multicast Discovery", cluster.discovery_multicast_enabled);
		render_setting_tooltip("Finds workers on the local network automatically through IPv4 multicast.");
		if (cluster.discovery_multicast_enabled) {
			dirty_ |= edit_text("Discovery Group", cluster.discovery_multicast_group);
			int discovery_port = static_cast<int>(cluster.discovery_multicast_port);
			if (ImGui::InputInt("Discovery Port", &discovery_port)) {
				cluster.discovery_multicast_port = static_cast<uint16_t>(std::clamp(discovery_port, 1, 65535));
				dirty_ = true;
			}
		}
		int heartbeat = static_cast<int>(cluster.heartbeat_interval_ms);
		if (ImGui::SliderInt("Heartbeat Interval (ms)", &heartbeat, 50, 5000)) {
			cluster.heartbeat_interval_ms = static_cast<uint32_t>(heartbeat);
			dirty_ = true;
		}
		int timeout = static_cast<int>(cluster.node_timeout_ms);
		if (ImGui::SliderInt("Node Timeout (ms)", &timeout, 500, 60000)) {
			cluster.node_timeout_ms = static_cast<uint32_t>(timeout);
			dirty_ = true;
		}
		render_setting_tooltip("A node without heartbeat for this long is declared disconnected and its tiles are reclaimed locally.");
	}

	void render_performance_section() {
		static constexpr const char* compression[] = {"Raw (No Compression)", "QOI HDR Run-Length", "LZ4 Block"};
		auto& performance = config_.performance;
		dirty_ |= edit_toggle("Dynamic Load Balancing", performance.dynamic_load_balancing);
		render_setting_tooltip("Weights every node by its measured throughput and round-trip latency instead of its static weight only.");
		float alpha = static_cast<float>(performance.latency_penalty_alpha);
		if (ImGui::SliderFloat("Latency Penalty Exponent", &alpha, 0.0f, 4.0f, "%.2f")) {
			performance.latency_penalty_alpha = static_cast<double>(alpha);
			dirty_ = true;
		}
		render_setting_tooltip("Around 0.5 suits a local network, around 1.5 suits links across the Internet.");
		dirty_ |= edit_toggle("Work Stealing", performance.work_stealing_enabled);
		render_setting_tooltip("Computes late remote tiles locally without waiting for the worker.");
		int tile_timeout = static_cast<int>(performance.tile_timeout_ms);
		if (ImGui::SliderInt("Tile Timeout Ceiling (ms)", &tile_timeout, 4, 5000)) {
			performance.tile_timeout_ms = static_cast<uint32_t>(tile_timeout);
			dirty_ = true;
		}
		int frames = static_cast<int>(performance.max_inflight_frames);
		if (ImGui::SliderInt("Maximum In-Flight Frames", &frames, 1, 4)) {
			performance.max_inflight_frames = static_cast<uint32_t>(frames);
			dirty_ = true;
		}
		dirty_ |= edit_combo("Tile Compression", performance.compression_mode, compression, 3);
		render_setting_tooltip("Raw is best on gigabit links, QOI HDR on fast links, LZ4 on constrained links.");
		int acceleration = performance.lz4_acceleration_level;
		if (ImGui::SliderInt("LZ4 Acceleration", &acceleration, 1, 16)) {
			performance.lz4_acceleration_level = acceleration;
			dirty_ = true;
		}
		dirty_ |= edit_toggle("Benchmark Nodes On Connect", performance.auto_benchmark_nodes);
	}

	void render_security_section() {
		static constexpr const char* ciphers[] = {"Disabled (Closed Network Only)", "ChaCha20-Poly1305"};
		auto& security = config_.security;
		uint32_t cipher = std::min<uint32_t>(security.cipher_suite, 1U);
		if (edit_combo("Cipher Suite", cipher, ciphers, 2)) {
			security.cipher_suite = cipher;
			dirty_ = true;
		}
		dirty_ |= edit_toggle("Require Encryption", security.require_encryption);
		dirty_ |= edit_toggle("Sanitize Remote Pixel Data", security.sanitize_remote_data);
		render_setting_tooltip("Neutralizes non-finite and negative radiances in tiles received from workers.");
		dirty_ |= edit_toggle("Allow Unauthenticated LAN", security.allow_unauthenticated_lan);
		if (edit_text("Shared Token (64 Hex Characters)", token_text_)) {
			if (security.apply_token_hex(token_text_)) {
				dirty_ = true;
			}
		}
		if (!token_text_.empty() && token_text_.size() != 64U) {
			ImGui::TextColored(ImVec4(1.0f, 0.55f, 0.35f, 1.0f), "The token must contain exactly 64 hexadecimal characters.");
		}
		if (ImGui::SmallButton("Generate Token")) {
			token_text_ = generate_token();
			static_cast<void>(security.apply_token_hex(token_text_));
			dirty_ = true;
		}
		render_setting_tooltip("Creates a random 256-bit token. Give the same token to every worker.");
		ImGui::SameLine();
		if (ImGui::SmallButton("Copy Token")) {
			ImGui::SetClipboardText(token_text_.c_str());
		}
		render_setting_tooltip("Copies the authentication token string to the system clipboard.");
		dirty_ |= edit_text("Certificate Path", security.tls_certificate_path);
		dirty_ |= edit_text("Private Key Path", security.tls_private_key_path);
		dirty_ |= edit_text("CA Chain Path", security.tls_ca_chain_path);
	}

	void render_routing_section() {
		auto& routing = config_.task_routing;
		dirty_ |= edit_toggle("Interactive Viewport", routing.route_interactive_viewport);
		dirty_ |= edit_toggle("Capture Studio", routing.route_capture_studio);
		dirty_ |= edit_toggle("Monte Carlo Ensembles", routing.route_monte_carlo_ensembles);
		dirty_ |= edit_toggle("N-Body Interactions", routing.route_nbody_interactions);
		dirty_ |= edit_toggle("Exact Spacetime Metrics", routing.route_exact_spacetime_metrics);
		render_setting_tooltip("Chooses which workloads may be sent to remote workers.");
	}

	void render_nodes_section() {
		auto& nodes = config_.static_nodes;
		int remove_index = -1;
		for (size_t i = 0; i < nodes.size(); ++i) {
			auto& node = nodes[i];
			ImGui::PushID(static_cast<int>(i));
			ImGui::Separator();
			dirty_ |= edit_toggle("Enabled", node.enabled);
			dirty_ |= edit_text("Name", node.name);
			dirty_ |= edit_text("Address", node.endpoint.host);
			int port = static_cast<int>(node.endpoint.port);
			if (ImGui::InputInt("Port", &port)) {
				node.endpoint.port = static_cast<uint16_t>(std::clamp(port, 1, 65535));
				dirty_ = true;
			}
			dirty_ |= ImGui::InputDouble("Static Weight", &node.static_weight, 0.1, 1.0, "%.3f");
			node.static_weight = std::max(node.static_weight, 1.0e-3);
			int device = static_cast<int>(node.preferred_device);
			if (ImGui::InputInt("Preferred Device", &device)) {
				node.preferred_device = static_cast<uint32_t>(std::max(device, 0));
				dirty_ = true;
			}
			dirty_ |= edit_toggle("Prefer FP64 GPU", node.prefer_fp64);
			if (ImGui::SmallButton("Remove Node")) {
				remove_index = static_cast<int>(i);
			}
			ImGui::PopID();
		}
		if (remove_index >= 0) {
			nodes.erase(nodes.begin() + remove_index);
			dirty_ = true;
		}
		ImGui::Separator();
		ImGui::TextDisabled("Add Static Node");
		edit_text("New Name", new_name_);
		edit_text("New Address", new_host_);
		int port = static_cast<int>(new_port_);
		if (ImGui::InputInt("New Port", &port)) {
			new_port_ = static_cast<uint16_t>(std::clamp(port, 1, 65535));
		}
		ImGui::InputDouble("New Weight", &new_weight_, 0.1, 1.0, "%.3f");
		new_weight_ = std::max(new_weight_, 1.0e-3);
		ImGui::BeginDisabled(new_host_.empty());
		if (ImGui::Button("Add Node")) {
			Distributed::ClusterNodeDescriptor descriptor{};
			descriptor.name = new_name_.empty() ? ("node-" + std::to_string(nodes.size() + 1U)) : new_name_;
			descriptor.endpoint = Distributed::TransportEndpoint{new_host_, new_port_};
			descriptor.static_weight = new_weight_;
			nodes.push_back(std::move(descriptor));
			new_name_.clear();
			new_host_.clear();
			dirty_ = true;
		}
		ImGui::EndDisabled();
	}

	void render_status_section() {
		if (!status_) {
			ImGui::TextDisabled("The cluster status is not available.");
			return;
		}
		const Distributed::ClusterStatus status = status_();
		ImGui::Text("Configured: %s | Operational: %s", status.configured ? "Yes" : "No", status.operational ? "Yes" : "No");
		if (!status.error.empty()) {
			ImGui::TextColored(ImVec4(1.0f, 0.55f, 0.35f, 1.0f), "%s", status.error.c_str());
		}
		const auto& frame = status.last_frame;
		ImGui::Text("Last frame %llu: %u remote / %u local tiles | %u reclaimed | %.2f ms | %.2f MiB received | RTT %.2f ms", static_cast<unsigned long long>(frame.frame_index), frame.tiles_remote, frame.tiles_local, frame.tiles_reclaimed, frame.frame_ms, static_cast<double>(frame.bytes_received) / (1024.0 * 1024.0), frame.round_trip_mean_ms);
		const auto& scheduler = status.scheduler;
		ImGui::Text("Scheduler: %llu frames | %llu dispatched | %llu timeouts | %llu speculative | %llu node losses | %llu late discarded", static_cast<unsigned long long>(scheduler.frames_planned), static_cast<unsigned long long>(scheduler.tiles_dispatched), static_cast<unsigned long long>(scheduler.tiles_reclaimed_timeout), static_cast<unsigned long long>(scheduler.tiles_reclaimed_speculative), static_cast<unsigned long long>(scheduler.tiles_reclaimed_node_lost), static_cast<unsigned long long>(scheduler.late_results_discarded));
		if (ImGui::BeginTable("##ClusterNodes", 9, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
			ImGui::TableSetupColumn("Name");
			ImGui::TableSetupColumn("Endpoint");
			ImGui::TableSetupColumn("State");
			ImGui::TableSetupColumn("RTT (ms)");
			ImGui::TableSetupColumn("s / kRay");
			ImGui::TableSetupColumn("Tiles");
			ImGui::TableSetupColumn("In Flight");
			ImGui::TableSetupColumn("Misses");
			ImGui::TableSetupColumn("Clock Offset (ms)");
			ImGui::TableHeadersRow();
			for (const Distributed::NodeSnapshot& node : status.nodes) {
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(node.name.c_str());
				ImGui::TableNextColumn();
				ImGui::Text("%s:%u", node.endpoint.host.c_str(), static_cast<unsigned>(node.endpoint.port));
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(Distributed::node_state_name(node.state));
				ImGui::TableNextColumn();
				ImGui::Text("%.2f +/- %.2f", node.round_trip_mean_seconds * 1.0e3, node.round_trip_sigma_seconds * 1.0e3);
				ImGui::TableNextColumn();
				ImGui::Text("%.3e", node.seconds_per_kilo_ray);
				ImGui::TableNextColumn();
				ImGui::Text("%llu", static_cast<unsigned long long>(node.tiles_completed));
				ImGui::TableNextColumn();
				ImGui::Text("%u", node.in_flight_tasks);
				ImGui::TableNextColumn();
				ImGui::Text("%llu", static_cast<unsigned long long>(node.deadline_misses));
				ImGui::TableNextColumn();
				ImGui::Text("%.3f", node.clock_offset_seconds * 1.0e3);
			}
			ImGui::EndTable();
		}
	}

public:
	void attach(ReloadHandler reload, StatusProvider status) {
		reload_ = std::move(reload);
		status_ = std::move(status);
	}

	void render() {
		if (!loaded_) {
			load_from_disk();
		}
		if (ImGui::Button("Reload From Disk")) {
			load_from_disk();
		}
		render_setting_tooltip("Discards unsaved edits and reads config/distributed_cluster.cfg again.");
		ImGui::SameLine();
		if (ImGui::Button("Save")) {
			static_cast<void>(save_to_disk());
		}
		ImGui::SameLine();
		if (ImGui::Button("Save And Restart Cluster")) {
			if (save_to_disk() && reload_) {
				reload_();
			}
		}
		render_setting_tooltip("Writes the configuration and restarts the master service so that the new settings take effect immediately.");
		if (dirty_) {
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "Unsaved changes");
		}
		if (!feedback_.empty()) {
			ImGui::TextDisabled("%s", feedback_.c_str());
		}
		ImGui::Separator();
		if (ImGui::CollapsingHeader("Live Status", ImGuiTreeNodeFlags_DefaultOpen)) {
			render_status_section();
		}
		if (ImGui::CollapsingHeader("Network")) {
			render_cluster_section();
		}
		if (ImGui::CollapsingHeader("Load Balancing And Compression")) {
			render_performance_section();
		}
		if (ImGui::CollapsingHeader("Security")) {
			render_security_section();
		}
		if (ImGui::CollapsingHeader("Task Routing")) {
			render_routing_section();
		}
		if (ImGui::CollapsingHeader("Static Nodes")) {
			render_nodes_section();
		}
	}
};

#else

class ClusterPanel {
public:
	void render() {
		ImGui::TextDisabled("Distributed compute support is disabled in this build.");
	}
};

#endif

}

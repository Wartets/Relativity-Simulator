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
	using PingHandler = std::function<bool(uint32_t)>;

private:
	Distributed::ClusterConfiguration config_{};
	ReloadHandler reload_{};
	StatusProvider status_{};
	PingHandler ping_{};
	bool loaded_{false};
	bool dirty_{false};
	std::string feedback_{};
	std::string token_text_{};
	std::string new_name_{};
	std::string new_host_{};
	uint16_t new_port_{Distributed::kDefaultClusterPort};
	double new_weight_{1.0};
	uint32_t last_pinged_node_{0};
	double last_ping_timestamp_{0.0};

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

	[[nodiscard]] static ImVec4 node_state_color(Distributed::NodeState state) noexcept {
		switch (state) {
			case Distributed::NodeState::Ready:
				return ImVec4(0.25f, 0.90f, 0.40f, 1.0f);
			case Distributed::NodeState::Computing:
				return ImVec4(0.35f, 0.75f, 1.00f, 1.0f);
			case Distributed::NodeState::Synchronized:
				return ImVec4(0.85f, 0.85f, 0.35f, 1.0f);
			case Distributed::NodeState::Handshaking:
			case Distributed::NodeState::Unauthenticated:
				return ImVec4(1.00f, 0.65f, 0.20f, 1.0f);
			case Distributed::NodeState::Stalled:
				return ImVec4(0.95f, 0.35f, 0.25f, 1.0f);
			case Distributed::NodeState::Disconnected:
			default:
				return ImVec4(0.60f, 0.60f, 0.60f, 1.0f);
		}
	}

	void load_from_disk() {
		config_ = Distributed::ClusterConfiguration{};
		const std::filesystem::path path = Distributed::ClusterMasterService::configuration_path();
		std::error_code ec;
		if (std::filesystem::exists(path, ec)) {
			feedback_ = config_.load_from_file(path.string()) ? "Configuration loaded from " + path.string() : "The configuration file could not be read";
		} else {
			feedback_ = "No configuration file exists yet; default configuration populated";
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
		static constexpr const char* roles[] = {"Standalone (Local Only)", "Master (Orchestrator)", "Worker (Compute Node)", "Hybrid (Worker + Master)"};
		auto& cluster = config_.cluster;
		dirty_ |= edit_toggle("Enable Distributed Subsystem", cluster.enabled);
		render_setting_tooltip("Activates the distributed network subsystem. When set to 0, no sockets, threads, or memory allocations are created.");

		uint32_t role = static_cast<uint32_t>(cluster.node_role);
		if (edit_combo("Operational Node Role", role, roles, 4)) {
			cluster.node_role = static_cast<Distributed::ClusterNodeRole>(role);
			dirty_ = true;
		}
		render_setting_tooltip("Operational node role: Master orchestrates and composites; Worker evaluates assigned compute tasks; Hybrid does both.");

		dirty_ |= edit_text("Cluster Mesh Name", cluster.cluster_name);
		render_setting_tooltip("Logical cluster namespace. Discovery beacons with mismatched cluster names are ignored.");

		dirty_ |= edit_text("Local Listen Interface", cluster.listen_interface);
		render_setting_tooltip("IPv4 address for binding the inbound control and task sockets (0.0.0.0 binds all interfaces).");

		int port = static_cast<int>(cluster.listen_port);
		if (ImGui::InputInt("Listen Port", &port)) {
			cluster.listen_port = static_cast<uint16_t>(std::clamp(port, 1024, 65535));
			dirty_ = true;
		}
		render_setting_tooltip("UDP port accepting incoming worker connections and task traffic.");

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Heartbeat And Watchdog Timers:");
		int heartbeat = static_cast<int>(cluster.heartbeat_interval_ms);
		if (ImGui::SliderInt("Heartbeat Interval (ms)", &heartbeat, 50, 5000)) {
			cluster.heartbeat_interval_ms = static_cast<uint32_t>(heartbeat);
			dirty_ = true;
		}
		render_setting_tooltip("Frequency at which the master pings active workers to calculate moving-average RTT and clock skew.");

		int timeout = static_cast<int>(cluster.node_timeout_ms);
		if (ImGui::SliderInt("Node Timeout Threshold (ms)", &timeout, 500, 60000)) {
			cluster.node_timeout_ms = static_cast<uint32_t>(timeout);
			dirty_ = true;
		}
		render_setting_tooltip("Workers silent for longer than this duration are transitioned to Disconnected and their in-flight tasks are reclaimed.");
	}

	void render_discovery_section() {
		auto& cluster = config_.cluster;
		dirty_ |= edit_toggle("Enable UDP Multicast Discovery", cluster.discovery_multicast_enabled);
		render_setting_tooltip("Broadcasts UDP presence beacons to automatically discover worker instances on the local network without manual IP entry.");

		ImGui::BeginDisabled(!cluster.discovery_multicast_enabled);
		dirty_ |= edit_text("Multicast Group IP", cluster.discovery_multicast_group);
		render_setting_tooltip("Class D multicast group address (default 239.255.77.88).");

		int discovery_port = static_cast<int>(cluster.discovery_multicast_port);
		if (ImGui::InputInt("Discovery Port", &discovery_port)) {
			cluster.discovery_multicast_port = static_cast<uint16_t>(std::clamp(discovery_port, 1024, 65535));
			dirty_ = true;
		}
		render_setting_tooltip("UDP destination port for discovery announcements and responses.");
		ImGui::EndDisabled();

		if (status_) {
			const Distributed::ClusterStatus current_status = status_();
			ImGui::Separator();
			ImGui::TextColored(ImVec4(0.5f, 0.9f, 0.6f, 1.0f), "Discovered Network Workers (%zu Detected):", current_status.discovered.size());
			if (current_status.discovered.empty()) {
				ImGui::TextDisabled("No remote workers currently responding to discovery beacons on group %s.", cluster.discovery_multicast_group.c_str());
			} else {
				if (ImGui::BeginTable("##DiscoveredPeers", 6, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
					ImGui::TableSetupColumn("Name");
					ImGui::TableSetupColumn("Host Address");
					ImGui::TableSetupColumn("Port");
					ImGui::TableSetupColumn("Role");
					ImGui::TableSetupColumn("Weight");
					ImGui::TableSetupColumn("Action");
					ImGui::TableHeadersRow();
					for (const auto& peer : current_status.discovered) {
						ImGui::TableNextRow();
						ImGui::TableNextColumn();
						ImGui::TextUnformatted(peer.name.c_str());
						ImGui::TableNextColumn();
						ImGui::TextUnformatted(peer.address.c_str());
						ImGui::TableNextColumn();
						ImGui::Text("%u", static_cast<unsigned>(peer.service_port));
						ImGui::TableNextColumn();
						ImGui::TextUnformatted(Distributed::cluster_node_role_name(peer.role));
						ImGui::TableNextColumn();
						ImGui::Text("%.2f", static_cast<double>(peer.performance_weight));
						ImGui::TableNextColumn();
						const bool already_static = std::any_of(config_.static_nodes.begin(), config_.static_nodes.end(), [&peer](const Distributed::ClusterNodeDescriptor& n) {
							return n.endpoint.host == peer.address && n.endpoint.port == peer.service_port;
						});
						if (already_static) {
							ImGui::TextDisabled("Registered");
						} else {
							std::string button_label = "Add##" + std::to_string(peer.instance_identifier);
							if (ImGui::SmallButton(button_label.c_str())) {
								Distributed::ClusterNodeDescriptor descriptor{};
								descriptor.name = peer.name.empty() ? ("node-" + peer.address) : peer.name;
								descriptor.endpoint = Distributed::TransportEndpoint{peer.address, peer.service_port};
								descriptor.static_weight = std::max(static_cast<double>(peer.performance_weight), 0.1);
								descriptor.enabled = true;
								config_.static_nodes.push_back(std::move(descriptor));
								dirty_ = true;
							}
						}
					}
					ImGui::EndTable();
				}
			}
		}
	}

	void render_performance_section() {
		static constexpr const char* compression[] = {"Direct Raw (0 CPU, 10G LAN)", "QOI HDR RLE (Fast Spectral RLE)", "LZ4 Block (High Compression, WAN)"};
		auto& performance = config_.performance;

		dirty_ |= edit_toggle("Dynamic Adaptive Load Balancing", performance.dynamic_load_balancing);
		render_setting_tooltip("Dynamically adjusts tile packet quotas proportionally to measured execution throughput and network latency.");

		float alpha = static_cast<float>(performance.latency_penalty_alpha);
		if (ImGui::SliderFloat("Latency Penalty Exponent (Alpha)", &alpha, 0.0f, 4.0f, "%.2f")) {
			performance.latency_penalty_alpha = static_cast<double>(alpha);
			dirty_ = true;
		}
		render_setting_tooltip("Penalizes high-latency nodes according to (RTT)^(-alpha). Recommended: 0.5 for LAN, 1.5 for Internet links.");

		dirty_ |= edit_toggle("Dynamic Work Stealing", performance.work_stealing_enabled);
		render_setting_tooltip("Reclaims unreturned remote tiles and computes them locally if the timeout threshold is exceeded without blocking the frame.");

		int tile_timeout = static_cast<int>(performance.tile_timeout_ms);
		if (ImGui::SliderInt("Tile Timeout Ceiling (ms)", &tile_timeout, 4, 5000)) {
			performance.tile_timeout_ms = static_cast<uint32_t>(tile_timeout);
			dirty_ = true;
		}
		render_setting_tooltip("Upper limit on individual tile response deadlines before triggering speculative local re-dispatch.");

		int frames = static_cast<int>(performance.max_inflight_frames);
		if (ImGui::SliderInt("Max In-Flight Render Frames", &frames, 1, 4)) {
			performance.max_inflight_frames = static_cast<uint32_t>(frames);
			dirty_ = true;
		}
		render_setting_tooltip("Pipelines task dispatch to saturate network bandwidth by overlapping multiple frames.");

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Payload Compression Engine:");
		dirty_ |= edit_combo("Compression Codec", performance.compression_mode, compression, 3);
		render_setting_tooltip("Compression strategy for tile payloads: Direct Raw eliminates CPU overhead; QOI HDR quantizes radiance; LZ4 minimizes WAN bandwidth.");

		int acceleration = performance.lz4_acceleration_level;
		if (ImGui::SliderInt("LZ4 Acceleration Factor", &acceleration, 1, 16)) {
			performance.lz4_acceleration_level = acceleration;
			dirty_ = true;
		}
		render_setting_tooltip("LZ4 speed tuning. Higher values yield faster compression with marginal loss of compression ratio.");

		dirty_ |= edit_toggle("Auto-Benchmark Worker Nodes On Join", performance.auto_benchmark_nodes);
		render_setting_tooltip("Profiles worker execution speed on standard ray bundles during initial handshake.");
	}

	void render_security_section() {
		static constexpr const char* ciphers[] = {"Unencrypted (Closed Laboratory Only)", "ChaCha20-Poly1305 (RFC 8439 AEAD)"};
		auto& security = config_.security;
		uint32_t cipher = std::min<uint32_t>(security.cipher_suite, 1U);
		if (edit_combo("Cryptographic Cipher Suite", cipher, ciphers, 2)) {
			security.cipher_suite = cipher;
			dirty_ = true;
		}
		render_setting_tooltip("Selects the authenticated symmetric encryption cipher for network datagrams and state synchronization.");

		dirty_ |= edit_toggle("Enforce Packet Authentication", security.require_encryption);
		render_setting_tooltip("Rejects unauthenticated or tampered datagrams whose Poly1305 tag fails verification.");

		dirty_ |= edit_toggle("SIMD Pixel Data Sanitization", security.sanitize_remote_data);
		render_setting_tooltip("Runs SIMD validation on remote pixels to clamp out-of-range energies and neutralize NaNs/infinities.");

		dirty_ |= edit_toggle("Permit Unauthenticated LAN Peers", security.allow_unauthenticated_lan);
		render_setting_tooltip("Allows nodes on isolated private subnets to join without token validation.");

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Pre-Shared Cluster Authentication Token:");
		if (edit_text("Token (64 Hex Characters)", token_text_)) {
			if (security.apply_token_hex(token_text_)) {
				dirty_ = true;
			}
		}
		render_setting_tooltip("256-bit pre-shared symmetric key used to derive ephemeral forward-secret session keys.");

		if (!token_text_.empty() && token_text_.size() != 64U) {
			ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.35f, 1.0f), "Token must contain exactly 64 hexadecimal characters (%zu entered).", token_text_.size());
		}
		if (ImGui::Button("Generate Random 256-bit Token", ImVec2(220.0f, 24.0f))) {
			token_text_ = generate_token();
			static_cast<void>(security.apply_token_hex(token_text_));
			dirty_ = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Copy Token", ImVec2(100.0f, 24.0f))) {
			ImGui::SetClipboardText(token_text_.c_str());
		}
		render_setting_tooltip("Copies the 64-character token to the clipboard.");

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Mutual TLS Certificates (Optional WAN mTLS):");
		dirty_ |= edit_text("Node Certificate Path", security.tls_certificate_path);
		dirty_ |= edit_text("Private Key Path", security.tls_private_key_path);
		dirty_ |= edit_text("Root CA Chain Path", security.tls_ca_chain_path);
	}

	void render_routing_section() {
		auto& routing = config_.task_routing;
		ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Task Delegation Routing Matrix:");
		ImGui::TextDisabled("Select which simulation stages are distributed to the cluster rather than evaluated locally.");

		dirty_ |= edit_toggle("Interactive Viewport (32x32 Tiles)", routing.route_interactive_viewport);
		render_setting_tooltip("Distributes screen tiles of the live viewport among cluster workers.");

		dirty_ |= edit_toggle("Capture Studio (Bands & High-Res Frames)", routing.route_capture_studio);
		render_setting_tooltip("Delegates offline capture sequence frame bands, 4K/8K captures, and motion-blur sub-samples.");

		dirty_ |= edit_toggle("Monte Carlo Geodesic Ensembles", routing.route_monte_carlo_ensembles);
		render_setting_tooltip("Dispatches batches of stochastic null trajectories to workers for variance reductions.");

		dirty_ |= edit_toggle("N-Body Post-Newtonian Acceleration Reduction", routing.route_nbody_interactions);
		render_setting_tooltip("Evaluates row blocks of the O(N^2) pairwise interaction matrix on GPU/CPU workers.");

		dirty_ |= edit_toggle("Exact Metric Geodesics (Kerr, Kerr-Newman, TeVeS)", routing.route_exact_spacetime_metrics);
		render_setting_tooltip("Delegates numerically heavy spacetimes with frame dragging or scalar fields.");
	}

	void render_nodes_section() {
		auto& nodes = config_.static_nodes;
		int remove_index = -1;
		ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Static Node Directory (%zu Configured):", nodes.size());
		ImGui::TextDisabled("Permanent worker endpoints explicitly targeted for connections.");

		for (size_t i = 0; i < nodes.size(); ++i) {
			auto& node = nodes[i];
			ImGui::PushID(static_cast<int>(i));
			ImGui::Separator();
			ImGui::TextColored(ImVec4(0.9f, 0.85f, 0.4f, 1.0f), "Node #%zu: %s", i + 1, node.name.c_str());

			dirty_ |= edit_toggle("Enabled", node.enabled);
			ImGui::SameLine();
			dirty_ |= edit_toggle("Prefer FP64 GPU", node.prefer_fp64);

			dirty_ |= edit_text("Node Name", node.name);
			dirty_ |= edit_text("Host Address", node.endpoint.host);

			int port = static_cast<int>(node.endpoint.port);
			if (ImGui::InputInt("Port", &port)) {
				node.endpoint.port = static_cast<uint16_t>(std::clamp(port, 1024, 65535));
				dirty_ = true;
			}

			dirty_ |= ImGui::InputDouble("Static Capacity Weight", &node.static_weight, 0.1, 1.0, "%.3f");
			node.static_weight = std::max(node.static_weight, 1.0e-3);
			render_setting_tooltip("Weight multiplier applied in load balancing. 2.0 assigns roughly twice the work of 1.0.");

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
		ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.6f, 1.0f), "Add New Static Node:");
		edit_text("New Node Name", new_name_);
		edit_text("New Node IP / Hostname", new_host_);

		int port = static_cast<int>(new_port_);
		if (ImGui::InputInt("New Node Port", &port)) {
			new_port_ = static_cast<uint16_t>(std::clamp(port, 1024, 65535));
		}
		ImGui::InputDouble("New Node Weight", &new_weight_, 0.1, 1.0, "%.3f");
		new_weight_ = std::max(new_weight_, 1.0e-3);

		ImGui::BeginDisabled(new_host_.empty());
		if (ImGui::Button("Register Node In Directory", ImVec2(220.0f, 26.0f))) {
			Distributed::ClusterNodeDescriptor descriptor{};
			descriptor.name = new_name_.empty() ? ("node-" + new_host_) : new_name_;
			descriptor.endpoint = Distributed::TransportEndpoint{new_host_, new_port_};
			descriptor.static_weight = new_weight_;
			descriptor.enabled = true;
			nodes.push_back(std::move(descriptor));
			new_name_.clear();
			new_host_.clear();
			dirty_ = true;
		}
		ImGui::EndDisabled();
	}

	void render_status_section() {
		if (!status_) {
			ImGui::TextDisabled("Cluster master service status is unavailable.");
			return;
		}
		const Distributed::ClusterStatus status = status_();

		const auto& frame = status.last_frame;
		const uint32_t total_tiles = std::max(frame.tiles_total, 1U);
		const double total_tiles_d = static_cast<double>(total_tiles);
		const double remote_percent = (static_cast<double>(frame.tiles_remote) / total_tiles_d) * 100.0;
		const double local_percent = (static_cast<double>(frame.tiles_local) / total_tiles_d) * 100.0;
		const double stolen_percent = (static_cast<double>(frame.tiles_reclaimed) / total_tiles_d) * 100.0;
		const float remote_frac = static_cast<float>(static_cast<double>(frame.tiles_remote) / total_tiles_d);

		ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Real-Time Frame Workload Partition (Frame #%llu):", static_cast<unsigned long long>(frame.frame_index));
		char overlay_buf[128];
		if (frame.tiles_reclaimed > 0U) {
			std::snprintf(overlay_buf, sizeof(overlay_buf), "%u Remote (%.0f%%) | %u Local (%.0f%%) | %u Stolen (%.0f%%)", frame.tiles_remote, remote_percent, frame.tiles_local, local_percent, frame.tiles_reclaimed, stolen_percent);
		} else {
			std::snprintf(overlay_buf, sizeof(overlay_buf), "%u Remote (%.0f%%) | %u Local (%.0f%%)", frame.tiles_remote, remote_percent, frame.tiles_local, local_percent);
		}
		ImGui::ProgressBar(remote_frac, ImVec2(-1.0f, 20.0f), overlay_buf);

		ImGui::Text("Render: %.2f ms | Dispatch: %.2f ms | Decode: %.2f ms | Local: %.2f ms", frame.frame_ms, frame.dispatch_ms, frame.decode_ms, frame.local_render_ms);
		ImGui::Text("Network: %.2f MiB Received | Throughput: %.1f Mbps | Compression: %u%% of raw | Mean RTT: %.2f ms", static_cast<double>(frame.bytes_received) / (1024.0 * 1024.0), frame.effective_bandwidth_mbps, frame.compression_ratio_percent, frame.round_trip_mean_ms);

		const auto& sched = status.scheduler;
		ImGui::TextDisabled("Scheduler Totals: %llu planned | %llu dispatched | %llu stolen timeouts | %llu speculative steals | %llu node losses | %llu late discarded", static_cast<unsigned long long>(sched.frames_planned), static_cast<unsigned long long>(sched.tiles_dispatched), static_cast<unsigned long long>(sched.tiles_reclaimed_timeout), static_cast<unsigned long long>(sched.tiles_reclaimed_speculative), static_cast<unsigned long long>(sched.tiles_reclaimed_node_lost), static_cast<unsigned long long>(sched.late_results_discarded));

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Connected Cluster Nodes (%zu Active):", status.nodes.size());
		if (last_pinged_node_ != 0U && (ImGui::GetTime() - last_ping_timestamp_) < 3.0) {
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.4f, 1.0f), "| Probing node #%u...", last_pinged_node_);
		}

		if (status.nodes.empty()) {
			ImGui::TextDisabled("No cluster worker nodes currently connected.");
			return;
		}

		if (ImGui::BeginTable("##ClusterNodeMatrix", 10, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
			ImGui::TableSetupColumn("Name");
			ImGui::TableSetupColumn("Endpoint");
			ImGui::TableSetupColumn("State");
			ImGui::TableSetupColumn("Hardware / GPU");
			ImGui::TableSetupColumn("RTT +/- Sigma");
			ImGui::TableSetupColumn("Loss %");
			ImGui::TableSetupColumn("Throughput");
			ImGui::TableSetupColumn("Tasks In-Flight");
			ImGui::TableSetupColumn("Completed");
			ImGui::TableSetupColumn("Action");
			ImGui::TableHeadersRow();

			for (const Distributed::NodeSnapshot& node : status.nodes) {
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(node.name.c_str());

				ImGui::TableNextColumn();
				ImGui::Text("%s:%u", node.endpoint.host.c_str(), static_cast<unsigned>(node.endpoint.port));

				ImGui::TableNextColumn();
				ImGui::TextColored(node_state_color(node.state), "%s", Distributed::node_state_name(node.state));

				ImGui::TableNextColumn();
				if (node.hardware.has_vulkan_compute) {
					ImGui::Text("%s%u threads", node.hardware.has_fp64_support ? "FP64 GPU, " : "GPU, ", node.hardware.logical_cores);
				} else {
					ImGui::Text("CPU (%u threads)", node.hardware.logical_cores);
				}

				ImGui::TableNextColumn();
				ImGui::Text("%.1f +/- %.1f ms", node.round_trip_mean_seconds * 1.0e3, node.round_trip_sigma_seconds * 1.0e3);

				ImGui::TableNextColumn();
				if (node.transport_stats.valid) {
					ImGui::Text("%.1f%%", node.transport_stats.packet_loss_fraction * 100.0);
				} else {
					ImGui::TextDisabled("N/A");
				}

				ImGui::TableNextColumn();
				const double mrps = (node.seconds_per_kilo_ray > 0.0) ? (1.0e-3 / node.seconds_per_kilo_ray) : 0.0;
				ImGui::Text("%.2f MRay/s", mrps);

				ImGui::TableNextColumn();
				ImGui::Text("%u", node.in_flight_tasks);

				ImGui::TableNextColumn();
				ImGui::Text("%llu", static_cast<unsigned long long>(node.tiles_completed));

				ImGui::TableNextColumn();
				std::string ping_btn = "Ping##" + std::to_string(node.node_id);
				if (ImGui::SmallButton(ping_btn.c_str()) && ping_) {
					static_cast<void>(ping_(node.node_id));
					last_pinged_node_ = node.node_id;
					last_ping_timestamp_ = ImGui::GetTime();
				}
			}
			ImGui::EndTable();
		}
	}

public:
	void attach(ReloadHandler reload, StatusProvider status, PingHandler ping = nullptr) {
		reload_ = std::move(reload);
		status_ = std::move(status);
		ping_ = std::move(ping);
	}

	void render() {
		if (!loaded_) {
			load_from_disk();
		}

		Distributed::ClusterStatus current_status{};
		if (status_) {
			current_status = status_();
		}

		const bool is_op = current_status.operational;
		const bool has_workers = !current_status.nodes.empty();
		ImVec4 badge_color = ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
		const char* badge_text = "DISABLED";
		if (config_.cluster.enabled) {
			if (is_op && has_workers) {
				badge_color = ImVec4(0.2f, 0.85f, 0.35f, 1.0f);
				badge_text = "OPERATIONAL";
			} else if (is_op) {
				badge_color = ImVec4(0.95f, 0.75f, 0.2f, 1.0f);
				badge_text = "SEARCHING FOR PEERS";
			} else {
				badge_color = ImVec4(0.9f, 0.3f, 0.25f, 1.0f);
				badge_text = "FAILED / OFFLINE";
			}
		}

		ImGui::TextColored(badge_color, "[%s]", badge_text);
		ImGui::SameLine();
		ImGui::Text("Role: %s | Active Workers: %zu", Distributed::cluster_node_role_name(config_.cluster.node_role), current_status.nodes.size());

		if (ImGui::Button("Reload Config", ImVec2(105.0f, 24.0f))) {
			load_from_disk();
		}
		render_setting_tooltip("Discards unsaved edits and reloads config/distributed_cluster.cfg.");

		ImGui::SameLine();
		if (ImGui::Button("Save Config", ImVec2(95.0f, 24.0f))) {
			static_cast<void>(save_to_disk());
		}
		render_setting_tooltip("Persists all changes to config/distributed_cluster.cfg.");

		ImGui::SameLine();
		if (ImGui::Button("Save & Restart Subsystem", ImVec2(185.0f, 24.0f))) {
			if (save_to_disk() && reload_) {
				reload_();
			}
		}
		render_setting_tooltip("Saves configuration and restarts the master network transport so changes take immediate effect.");

		if (dirty_) {
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "Unsaved changes");
		}
		if (!feedback_.empty()) {
			ImGui::TextDisabled("%s", feedback_.c_str());
		}
		if (!current_status.error.empty()) {
			ImGui::TextColored(ImVec4(0.95f, 0.35f, 0.25f, 1.0f), "%s", current_status.error.c_str());
		}

		ImGui::Separator();
		if (ImGui::BeginTabBar("##ClusterSettingsTabs")) {
			if (ImGui::BeginTabItem("Live Status & Topology")) {
				render_status_section();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Node Directory & Discovery")) {
				render_discovery_section();
				ImGui::Separator();
				render_nodes_section();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Load Balancing & Codecs")) {
				render_performance_section();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Task Routing")) {
				render_routing_section();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Security & Encryption")) {
				render_security_section();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Network Interface")) {
				render_cluster_section();
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
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

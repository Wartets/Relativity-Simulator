#pragma once

#include <imgui.h>
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/dynamics/pn_body.hpp"
#include "relativistic/dynamics/pn_nbody_system.hpp"
#include "relativistic/core/constants.hpp"
#include "relativistic/ui/numeric_slider_utils.hpp"
#include "relativistic/ui/tooltip_utils.hpp"
#include "relativistic/dynamics/bulk_body_actions.hpp"
#include "relativistic/dynamics/interaction_compatibility.hpp"
#include <vector>
#include <string>
#include <string_view>
#include <array>
#include <cmath>
#include <numbers>
#include <algorithm>
#include <cstring>
#include <cfloat>
#include <cstdint>
#include <random>

namespace Relativistic::UI {

enum class BodyPresetTemplate : uint32_t {
	Custom = 0,
	Sun = 1,
	Earth = 2,
	Moon = 3,
	Jupiter = 4,
	Mars = 5,
	NeutronStar = 6,
	SupermassiveBlackHole = 7,
	StellarBlackHole = 8,
	TestParticle = 9
};

enum class BodyCatalogSortMode : uint32_t {
	CreationOrder = 0,
	Name = 1,
	Mass = 2,
	Distance = 3,
	Speed = 4
};

class BodyManagerWindow {
private:
	static constexpr int kCentralObjectIndex = -2;

	bool is_open_{false};
	Orchestrator::SimulationOrchestrator<1024>& orchestrator_;

	int selected_body_index_{-1};
	int creation_preset_{0};
	bool request_focus_creation_tab_{false};

	char new_body_name_[32]{"New Body"};
	float new_body_mass_{1.0f};
	float new_body_radius_{1.0f};
	float new_body_pos_[3]{10.0f, 0.0f, 0.0f};
	float new_body_vel_[3]{0.0f, 0.3f, 0.0f};
	float new_body_spin_[3]{0.0f, 0.0f, 0.0f};
	float new_body_j2_{0.0f};
	float new_body_j3_{0.0f};
	float new_body_j4_{0.0f};
	float new_body_r_ref_{1.0f};
	float new_body_quadrupole_{0.0f};
	bool new_body_mass_log_mode_{true};
	bool new_body_radius_log_mode_{true};
	bool new_body_r_ref_log_mode_{true};
	bool central_mass_log_mode_{true};
	bool selected_mass_log_mode_{true};
	bool selected_radius_log_mode_{true};
	bool selected_r_ref_log_mode_{true};
	std::mt19937_64 creation_rng_{std::random_device{}()};

	char search_filter_[64]{};
	int sort_mode_{static_cast<int>(BodyCatalogSortMode::CreationOrder)};
	bool sort_descending_{false};
	float list_pane_width_{230.0f};
	char rename_buffer_[32]{};
	int rename_target_id_{-1};
	int tracked_body_id_{-1};
	bool tracking_enabled_{false};
	float global_velocity_[3]{0.0f, 0.0f, 0.0f};
	float global_spin_[3]{0.0f, 0.0f, 0.0f};
	float grid_spacing_{1.0f};

public:
	explicit BodyManagerWindow(Orchestrator::SimulationOrchestrator<1024>& orchestrator)
		: orchestrator_(orchestrator) {
		randomize_creation_defaults();
	}

	[[nodiscard]] bool& open_state() noexcept {
		return is_open_;
	}

	void render() {
		if (!is_open_) return;

		ImGui::SetNextWindowPos(ImVec2(15.0f, 400.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(600.0f, 680.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSizeConstraints(ImVec2(440.0f, 380.0f), ImVec2(FLT_MAX, FLT_MAX));

		if (!ImGui::Begin("Celestial Body & N-Body Manager", &is_open_)) {
			ImGui::End();
			return;
		}

		auto& sys = orchestrator_.nbody_system();
		update_tracking(sys);
		const size_t body_count = sys.body_count();

		ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "Active Bodies in System: %zu", body_count);
		render_setting_tooltip("Total number of orbiting bodies currently tracked by the post-Newtonian N-body integrator, excluding the central spacetime source itself.");
		ImGui::SameLine();
		if (ImGui::Button("Clear All Bodies")) {
			sys.clear_bodies();
			selected_body_index_ = -1;
		}
		render_setting_tooltip("Removes every orbiting body from the system. The central spacetime source and its mass/spin/charge are unaffected.");

		ImGui::Separator();

		if (ImGui::BeginTabBar("BodyManagerTabs")) {
			ImGuiTabItemFlags catalog_flags = ImGuiTabItemFlags_None;
			ImGuiTabItemFlags creation_flags = ImGuiTabItemFlags_None;
			if (request_focus_creation_tab_) {
				creation_flags |= ImGuiTabItemFlags_SetSelected;
				request_focus_creation_tab_ = false;
			}
			if (ImGui::BeginTabItem("Body Catalog", nullptr, catalog_flags)) {
				render_body_list_tab();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Create Body", nullptr, creation_flags)) {
				render_creation_tab();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("System Dynamics")) {
				render_system_dynamics_tab();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Interactions")) {
				render_interactions_tab();
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}

		ImGui::End();
	}

private:
	void look_at(const std::array<double, 3>& target) noexcept {
		auto& camera = orchestrator_.camera();
		const double dx = target[0] - camera.position[0];
		const double dy = target[1] - camera.position[1];
		const double dz = target[2] - camera.position[2];
		const double distance = std::sqrt(dx * dx + dy * dy + dz * dz);
		if (distance <= 1e-9) return;
		camera.target = target;
		camera.yaw = std::atan2(dy, dx) * (180.0 / std::numbers::pi);
		camera.pitch = std::asin(std::clamp(dz / distance, -1.0, 1.0)) * (180.0 / std::numbers::pi);
		camera.roll = 0.0;
	}

	void update_tracking(Dynamics::PostNewtonianSystem& sys) noexcept {
		if (!tracking_enabled_) return;
		if (tracked_body_id_ == kCentralObjectIndex) {
			look_at({0.0, 0.0, 0.0});
			return;
		}
		for (const auto& body : sys.bodies()) {
			if (body.id == static_cast<uint32_t>(tracked_body_id_) && body.enabled) {
				look_at(body.position);
				return;
			}
		}
		tracking_enabled_ = false;
		tracked_body_id_ = -1;
	}

	[[nodiscard]] std::string unique_name(std::string_view base) const {
		std::string original = base.empty() ? "Body" : std::string(base);
		const auto& bodies = orchestrator_.nbody_system().bodies();
		auto exists = [&](std::string_view name) {
			return std::any_of(bodies.begin(), bodies.end(), [&](const auto& body) { return body.name_view() == name; });
		};

		std::string stem = original;
		uint32_t start_suffix = 2;
		const size_t space_pos = original.find_last_of(' ');
		if (space_pos != std::string::npos && space_pos + 1 < original.size()) {
			const std::string_view tail = std::string_view(original).substr(space_pos + 1);
			if (!tail.empty() && std::all_of(tail.begin(), tail.end(), [](unsigned char c) { return std::isdigit(c) != 0; })) {
				const uint32_t parsed = static_cast<uint32_t>(std::strtoul(std::string(tail).c_str(), nullptr, 10));
				if (parsed > 0) {
					stem = original.substr(0, space_pos);
					start_suffix = parsed + 1;
				}
			}
		}

		if (!exists(original)) return original;
		for (uint32_t suffix = start_suffix; suffix < 1000000; ++suffix) {
			const std::string numbered = stem + " " + std::to_string(suffix);
			if (!exists(numbered)) return numbered;
		}
		return original;
	}
	[[nodiscard]] static std::string display_name(const Dynamics::PostNewtonianBody& body) {
		if (body.has_name()) {
			return std::string(body.name_view());
		}
		return "Body #" + std::to_string(body.id);
	}

	[[nodiscard]] static double distance_from_center(const Dynamics::PostNewtonianBody& body) noexcept {
		return std::sqrt(body.position[0] * body.position[0] + body.position[1] * body.position[1] + body.position[2] * body.position[2]);
	}

	[[nodiscard]] std::array<double, 3> compute_circular_orbit_velocity(const std::array<double, 3>& pos, double central_mass) const noexcept {
		const double r2 = pos[0] * pos[0] + pos[1] * pos[1] + pos[2] * pos[2];
		const double r = std::sqrt(std::max(r2, 1e-12));
		const double speed = std::sqrt(std::max(orchestrator_.physical_gravitational_constant() * central_mass, 0.0) / r);
		const std::array<double, 3> up{0.0, 0.0, 1.0};
		std::array<double, 3> tangent{
			up[1] * pos[2] - up[2] * pos[1],
			up[2] * pos[0] - up[0] * pos[2],
			up[0] * pos[1] - up[1] * pos[0]
		};
		const double t_len = std::sqrt(tangent[0] * tangent[0] + tangent[1] * tangent[1] + tangent[2] * tangent[2]);
		if (t_len < 1e-9) {
			return {0.0, speed, 0.0};
		}
		return {tangent[0] / t_len * speed, tangent[1] / t_len * speed, tangent[2] / t_len * speed};
	}

	[[nodiscard]] double random_real(double min_val, double max_val) noexcept {
		std::uniform_real_distribution<double> dist(min_val, max_val);
		return dist(creation_rng_);
	}

	[[nodiscard]] int random_int(int min_val, int max_val) noexcept {
		std::uniform_int_distribution<int> dist(min_val, max_val);
		return dist(creation_rng_);
	}

	[[nodiscard]] double sample_log_uniform(double min_val, double max_val) noexcept {
		return std::pow(10.0, random_real(std::log10(min_val), std::log10(max_val)));
	}

	[[nodiscard]] double sample_signed_uniform(double min_abs, double max_abs) noexcept {
		return (random_int(0, 1) == 0 ? -1.0 : 1.0) * random_real(min_abs, max_abs);
	}

	[[nodiscard]] static std::string_view archetype_label(uint32_t archetype) noexcept {
		switch (archetype) {
			case 0: return "Probe";
			case 1: return "Shard";
			case 2: return "World";
			case 3: return "Giant";
			case 4: return "Compact";
			default: return "Astral";
		}
	}

	[[nodiscard]] std::string synthesize_creation_name(uint32_t archetype, double mass, double radius, double orbit_radius) {
		static constexpr std::array<std::string_view, 28> prefixes{
			"Astra", "Boreal", "Cinder", "Drift", "Echo", "Eon", "Flux", "Halo",
			"Ion", "Kestrel", "Lumen", "Nova", "Nyx", "Orbit", "Quasar", "Rift",
			"Solace", "Spectrum", "Titan", "Umbra", "Vanta", "Velvet", "Vesper",
			"Warden", "Zenith", "Auric", "Kepler", "Morrow"
		};
		static constexpr std::array<std::string_view, 8> accents{
			"Prime", "I", "II", "III", "Arc", "Node", "Field", "Halo"
		};

		const std::string_view prefix = prefixes[static_cast<size_t>(random_int(0, static_cast<int>(prefixes.size() - 1)))];
		const std::string_view accent = accents[static_cast<size_t>(random_int(0, static_cast<int>(accents.size() - 1)))];
		const uint32_t suffix = static_cast<uint32_t>(random_int(10, 999));

		std::string name = std::string(prefix) + " " + std::string(archetype_label(archetype));
		if (mass > 0.0 && radius > 0.0) {
			if (orbit_radius > radius * 100.0) {
				name += " Deep";
			} else if (orbit_radius < radius * 20.0) {
				name += " Inner";
			}
		}
		name += " ";
		name += std::string(accent);
		name += " ";
		name += std::to_string(suffix);
		return name;
	}

	void randomize_creation_defaults() noexcept {
		static constexpr std::array<uint32_t, 6> archetypes{0, 1, 2, 3, 4, 5};
		const uint32_t archetype = archetypes[static_cast<size_t>(random_int(0, static_cast<int>(archetypes.size() - 1)))];
		const double central_mass = std::max(orchestrator_.parameters().mass, 1e-12);
		double orbit_radius = 10.0;
		double spin_scale = 1e-3;

		switch (archetype) {
			case 0: // Probe
				new_body_mass_ = static_cast<float>(sample_log_uniform(1e-8, 1e-3));
				new_body_radius_ = static_cast<float>(sample_log_uniform(1e-3, 5e-2));
				orbit_radius = sample_log_uniform(12.0, 120.0);
				spin_scale = 1e-4;
				new_body_quadrupole_ = static_cast<float>(sample_signed_uniform(1e-12, 1e-8));
				new_body_j2_ = static_cast<float>(sample_signed_uniform(1e-10, 1e-6));
				new_body_j3_ = static_cast<float>(sample_signed_uniform(1e-12, 1e-7));
				new_body_j4_ = static_cast<float>(sample_signed_uniform(1e-12, 1e-7));
				break;
			case 1: // Shard
				new_body_mass_ = static_cast<float>(sample_log_uniform(1e-4, 1.0));
				new_body_radius_ = static_cast<float>(sample_log_uniform(5e-2, 1.5));
				orbit_radius = sample_log_uniform(16.0, 220.0);
				spin_scale = 1e-3;
				new_body_quadrupole_ = static_cast<float>(sample_signed_uniform(1e-10, 1e-6));
				new_body_j2_ = static_cast<float>(sample_signed_uniform(1e-8, 1e-4));
				new_body_j3_ = static_cast<float>(sample_signed_uniform(1e-10, 1e-5));
				new_body_j4_ = static_cast<float>(sample_signed_uniform(1e-10, 1e-5));
				break;
			case 2: // World
				new_body_mass_ = static_cast<float>(sample_log_uniform(1.0, 50.0));
				new_body_radius_ = static_cast<float>(sample_log_uniform(0.8, 6.0));
				orbit_radius = sample_log_uniform(30.0, 350.0);
				spin_scale = 5e-3;
				new_body_quadrupole_ = static_cast<float>(sample_signed_uniform(1e-9, 1e-5));
				new_body_j2_ = static_cast<float>(sample_signed_uniform(1e-6, 1e-3));
				new_body_j3_ = static_cast<float>(sample_signed_uniform(1e-8, 1e-5));
				new_body_j4_ = static_cast<float>(sample_signed_uniform(1e-9, 1e-5));
				break;
			case 3: // Giant
				new_body_mass_ = static_cast<float>(sample_log_uniform(10.0, 1e4));
				new_body_radius_ = static_cast<float>(sample_log_uniform(4.0, 20.0));
				orbit_radius = sample_log_uniform(60.0, 900.0);
				spin_scale = 8e-3;
				new_body_quadrupole_ = static_cast<float>(sample_signed_uniform(1e-8, 1e-4));
				new_body_j2_ = static_cast<float>(sample_signed_uniform(1e-4, 2e-2));
				new_body_j3_ = static_cast<float>(sample_signed_uniform(1e-7, 1e-4));
				new_body_j4_ = static_cast<float>(sample_signed_uniform(1e-7, 1e-4));
				break;
			case 4: // Compact
				new_body_mass_ = static_cast<float>(sample_log_uniform(1.0, 1e6));
				new_body_radius_ = static_cast<float>(sample_log_uniform(1e-4, 0.5));
				orbit_radius = sample_log_uniform(40.0, 400.0);
				spin_scale = 2e-2;
				new_body_quadrupole_ = static_cast<float>(sample_signed_uniform(1e-12, 1e-7));
				new_body_j2_ = static_cast<float>(sample_signed_uniform(1e-10, 1e-6));
				new_body_j3_ = static_cast<float>(sample_signed_uniform(1e-12, 1e-7));
				new_body_j4_ = static_cast<float>(sample_signed_uniform(1e-12, 1e-7));
				break;
			case 5:
			default: // Astral
				new_body_mass_ = static_cast<float>(sample_log_uniform(1e2, 1e8));
				new_body_radius_ = static_cast<float>(sample_log_uniform(5.0, 100.0));
				orbit_radius = sample_log_uniform(80.0, 1000.0);
				spin_scale = 1e-2;
				new_body_quadrupole_ = static_cast<float>(sample_signed_uniform(1e-8, 1e-3));
				new_body_j2_ = static_cast<float>(sample_signed_uniform(1e-5, 5e-2));
				new_body_j3_ = static_cast<float>(sample_signed_uniform(1e-7, 1e-4));
				new_body_j4_ = static_cast<float>(sample_signed_uniform(1e-7, 1e-4));
				break;
		}

		const double theta = std::acos(std::clamp(random_real(-1.0, 1.0), -1.0, 1.0));
		const double phi = random_real(0.0, 2.0 * std::numbers::pi);
		const double sin_theta = std::sin(theta);
		new_body_pos_[0] = static_cast<float>(orbit_radius * sin_theta * std::cos(phi));
		new_body_pos_[1] = static_cast<float>(orbit_radius * sin_theta * std::sin(phi));
		new_body_pos_[2] = static_cast<float>(orbit_radius * std::cos(theta));

		auto velocity = compute_circular_orbit_velocity({static_cast<double>(new_body_pos_[0]), static_cast<double>(new_body_pos_[1]), static_cast<double>(new_body_pos_[2])}, central_mass);
		const double speed_scale = random_real(0.82, 1.18);
		if (std::abs(velocity[0]) < 1e-12 && std::abs(velocity[1]) < 1e-12 && std::abs(velocity[2]) < 1e-12) {
			velocity = {0.0, std::sqrt(central_mass / std::max(orbit_radius, 1e-9)), 0.0};
		}
		new_body_vel_[0] = static_cast<float>(velocity[0] * speed_scale);
		new_body_vel_[1] = static_cast<float>(velocity[1] * speed_scale);
		new_body_vel_[2] = static_cast<float>(velocity[2] * speed_scale);

		new_body_spin_[0] = static_cast<float>(sample_signed_uniform(spin_scale * 0.2, spin_scale));
		new_body_spin_[1] = static_cast<float>(sample_signed_uniform(spin_scale * 0.2, spin_scale));
		new_body_spin_[2] = static_cast<float>(sample_signed_uniform(spin_scale * 0.2, spin_scale));
		new_body_r_ref_ = static_cast<float>(std::max(static_cast<double>(new_body_radius_) * random_real(0.85, 1.25), 1e-6));

		std::string generated = synthesize_creation_name(archetype, new_body_mass_, new_body_radius_, orbit_radius);
		const std::string unique = unique_name(generated);
		std::strncpy(new_body_name_, unique.c_str(), sizeof(new_body_name_) - 1);
		new_body_name_[sizeof(new_body_name_) - 1] = '\0';

		creation_preset_ = static_cast<int>(BodyPresetTemplate::Custom);
		new_body_mass_log_mode_ = true;
		new_body_radius_log_mode_ = true;
		new_body_r_ref_log_mode_ = true;
	}

	void render_body_list_tab() noexcept {
		auto& sys = orchestrator_.nbody_system();
		auto bodies = sys.bodies();
		const size_t n = bodies.size();

		const float total_width = ImGui::GetContentRegionAvail().x;
		list_pane_width_ = std::clamp(list_pane_width_, 190.0f, std::max(210.0f, total_width - 220.0f));

		ImGui::BeginChild("BodyCatalogListPane", ImVec2(list_pane_width_, ImGui::GetContentRegionAvail().y), true);

		ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "Spacetime Source");
		{
			const bool is_selected_central = (selected_body_index_ == kCentralObjectIndex);
			const auto& params = orchestrator_.parameters();
			const std::string central_label = "Central Object (M=" + std::to_string(params.mass).substr(0, 5) + ", a=" + std::to_string(params.spin).substr(0, 5) + ")";
			if (ImGui::Selectable(central_label.c_str(), is_selected_central)) {
				selected_body_index_ = kCentralObjectIndex;
			}
			render_setting_tooltip("The metric-generating central mass. Select this to edit mass, spin, and charge parameters that shape the background spacetime.");
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "N-Body Catalog");

		ImGui::SetNextItemWidth(-1.0f);
		ImGui::InputTextWithHint("##BodySearch", "Filter by name or ID...", search_filter_, sizeof(search_filter_));
		render_setting_tooltip("Narrows the catalog below to bodies whose name or numeric identifier contains this text.");

		const char* sort_options[] = {"Creation Order", "Name", "Mass", "Distance From Center", "Speed"};
		ImGui::SetNextItemWidth(-30.0f);
		ImGui::Combo("##BodySortMode", &sort_mode_, sort_options, IM_ARRAYSIZE(sort_options));
		ImGui::SameLine();
		if (ImGui::ArrowButton("##BodySortDirection", sort_descending_ ? ImGuiDir_Down : ImGuiDir_Up)) {
			sort_descending_ = !sort_descending_;
		}
		render_setting_tooltip("Chooses how the catalog list below is ordered, and toggles between ascending and descending order.");

		std::vector<size_t> visible_indices;
		visible_indices.reserve(n);
		const std::string_view filter_view(search_filter_);
		for (size_t i = 0; i < n; ++i) {
			if (!filter_view.empty()) {
				const std::string name = display_name(bodies[i]);
				const std::string id_str = std::to_string(bodies[i].id);
				if (name.find(filter_view) == std::string::npos && id_str.find(filter_view) == std::string::npos) {
					continue;
				}
			}
			visible_indices.push_back(i);
		}

		std::sort(visible_indices.begin(), visible_indices.end(), [&](size_t a, size_t b) noexcept {
			bool less;
			switch (static_cast<BodyCatalogSortMode>(sort_mode_)) {
				case BodyCatalogSortMode::Name:
					less = display_name(bodies[a]) < display_name(bodies[b]);
					break;
				case BodyCatalogSortMode::Mass:
					less = bodies[a].mass < bodies[b].mass;
					break;
				case BodyCatalogSortMode::Distance:
					less = distance_from_center(bodies[a]) < distance_from_center(bodies[b]);
					break;
				case BodyCatalogSortMode::Speed:
					less = bodies[a].speed() < bodies[b].speed();
					break;
				case BodyCatalogSortMode::CreationOrder:
				default:
					less = a < b;
					break;
			}
			return sort_descending_ ? !less : less;
		});

		ImGui::Spacing();

		if (n == 0) {
			ImGui::TextDisabled("No orbiting bodies populated.");
			ImGui::Spacing();
			if (ImGui::Button("Add Body", ImVec2(-1.0f, 28.0f))) {
				request_focus_creation_tab_ = true;
			}
			render_setting_tooltip("Switches to the Create Body tab so you can configure and spawn a new orbiting body.");
			ImGui::Spacing();
			if (ImGui::Button("Spawn Solar System Archetype", ImVec2(-1.0f, 26.0f))) {
				populate_solar_system_archetype();
			}
			render_setting_tooltip("Populates the system with a simplified Sun-and-two-planets configuration to quickly exercise N-body dynamics.");
		} else if (visible_indices.empty()) {
			ImGui::TextDisabled("No bodies match the current filter.");
		} else {
			for (const size_t i : visible_indices) {
				const bool is_selected = (selected_body_index_ == static_cast<int>(i));
				const double dist = distance_from_center(bodies[i]);
				const std::string label = display_name(bodies[i]) + "  (M=" + std::to_string(bodies[i].mass).substr(0, 4) + ", r=" + std::to_string(dist).substr(0, 5) + ")";
				if (ImGui::Selectable(label.c_str(), is_selected)) {
					selected_body_index_ = static_cast<int>(i);
					rename_target_id_ = -1;
				}
				if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
					ImGui::BeginTooltip();
					ImGui::Text("Identifier: #%u", bodies[i].id);
					ImGui::Text("Mass: %.6e", bodies[i].mass);
					ImGui::Text("Distance from center: %.6f", dist);
					ImGui::Text("Speed: %.6e", bodies[i].speed());
					ImGui::EndTooltip();
				}
			}
		}
		ImGui::EndChild();

		ImGui::SameLine();
		ImGui::InvisibleButton("BodyCatalogSplitter", ImVec2(6.0f, ImGui::GetContentRegionAvail().y));
		if (ImGui::IsItemActive()) {
			list_pane_width_ += ImGui::GetIO().MouseDelta.x;
		}
		if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
			ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
		}
		ImGui::SameLine();

		ImGui::BeginChild("BodyCatalogDetailPane", ImVec2(0.0f, ImGui::GetContentRegionAvail().y), true);
		if (selected_body_index_ == kCentralObjectIndex) {
			render_central_object_panel();
		} else if (selected_body_index_ >= 0 && selected_body_index_ < static_cast<int>(n)) {
			render_selected_nbody_panel(sys, bodies, n);
		} else {
			ImGui::TextDisabled("Select a body from the catalog to inspect or edit its parameters.");
		}
		ImGui::EndChild();
	}

	void render_central_object_panel() noexcept {
		auto& params = orchestrator_.parameters();

		ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Central Spacetime Source");
		ImGui::TextDisabled("Metric: %s", orchestrator_.active_metric_name().c_str());
		ImGui::Separator();

		float mass = static_cast<float>(params.mass);
		if (slider_float_with_input("Central Mass (M)", &mass, 0.001f, 1.0e6f, "%.4f", &central_mass_log_mode_, 1e-12f, 1e36f)) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::Mass, std::max(0.01, static_cast<double>(mass)))));
		}
		render_setting_tooltip("Central gravitating mass in geometrized units. Governs the Schwarzschild radius rs = 2M and the overall curvature strength.");

		float spin = static_cast<float>(params.spin);
		const float spin_bound = static_cast<float>(0.999 * params.mass);
		if (slider_float_with_input("Spin Parameter (a)", &spin, -spin_bound, spin_bound, "%.4f")) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::Spin, std::clamp(static_cast<double>(spin), -0.999 * params.mass, 0.999 * params.mass))));
		}
		render_setting_tooltip("Specific angular momentum a = J / M, clamped to the subextremal range. Only meaningful for Kerr-family metrics.");

		float charge = static_cast<float>(params.charge);
		if (slider_float_with_input("Electric Charge (Q)", &charge, -10.0f, 10.0f, "%.4f")) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::Charge, static_cast<double>(charge))));
		}
		render_setting_tooltip("Net electrostatic charge. Only meaningful for Reissner-Nordstrom and Kerr-Newman metrics.");

		ImGui::Spacing();
		if (ImGui::Button("Look At Central Object", ImVec2(-1.0f, 26.0f))) {
			look_at({0.0, 0.0, 0.0});
		}
		render_setting_tooltip("Orients the camera to face the central compact object without changing its position.");
		bool tracking_central = tracking_enabled_ && tracked_body_id_ == kCentralObjectIndex;
		if (ImGui::Checkbox("Track Central Object", &tracking_central)) {
			tracking_enabled_ = tracking_central;
			tracked_body_id_ = tracking_central ? kCentralObjectIndex : -1;
		}
	}

	void render_selected_nbody_panel(Dynamics::PostNewtonianSystem& sys, auto bodies, size_t n) noexcept {
		auto& b = bodies[static_cast<size_t>(selected_body_index_)];
		bool changed = false;

		ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%s", display_name(b).c_str());
		ImGui::TextDisabled("Identifier: #%u", b.id);
		if (ImGui::Checkbox("Enabled", &b.enabled)) changed = true;
		render_setting_tooltip("Disabled bodies remain in the catalog and scenario, but are excluded from rendering, prediction, gravity, integration, and horizon absorption.");
		ImGui::SameLine();
		bool tracking_this = tracking_enabled_ && tracked_body_id_ == static_cast<int>(b.id);
		if (ImGui::Checkbox("Track", &tracking_this)) {
			tracking_enabled_ = tracking_this;
			tracked_body_id_ = tracking_this ? static_cast<int>(b.id) : -1;
		}
		ImGui::Separator();

		if (rename_target_id_ != static_cast<int>(b.id)) {
			rename_target_id_ = static_cast<int>(b.id);
			std::memset(rename_buffer_, 0, sizeof(rename_buffer_));
			if (b.has_name()) {
				const auto view = b.name_view();
				std::memcpy(rename_buffer_, view.data(), std::min(view.size(), sizeof(rename_buffer_) - 1));
			}
		}
		ImGui::SetNextItemWidth(-90.0f);
		ImGui::InputTextWithHint("##RenameField", "Unnamed", rename_buffer_, sizeof(rename_buffer_));
		ImGui::SameLine();
		if (ImGui::Button("Rename", ImVec2(80.0f, 0.0f))) {
			b.set_name(std::string_view(rename_buffer_));
		}
		render_setting_tooltip("Assigns a human-readable label to this body, shown throughout the catalog, tags, and saved scenarios instead of its numeric identifier.");

		float m = static_cast<float>(b.mass);
		if (slider_float_with_input("Mass", &m, 0.001f, 1.0e6f, "%.4f", &selected_mass_log_mode_, 1e-12f, 1e36f)) {
			b.mass = std::max(0.0, static_cast<double>(m));
			changed = true;
		}
		render_setting_tooltip("Gravitating mass of this body in the same geometrized unit system as the central mass.");

		float r = static_cast<float>(b.radius);
		if (slider_float_with_input("Physical Radius", &r, 0.001f, 1.0e5f, "%.4f", &selected_radius_log_mode_, 1e-6f, 1e12f)) {
			b.radius = std::max(1e-6, static_cast<double>(r));
			changed = true;
		}
		render_setting_tooltip("Visual and collision radius used for rendering and default multipole reference radius.");

		float pos[3] = {static_cast<float>(b.position[0]), static_cast<float>(b.position[1]), static_cast<float>(b.position[2])};
		if (ImGui::InputFloat3("Position (x, y, z)", pos)) {
			b.position = {static_cast<double>(pos[0]), static_cast<double>(pos[1]), static_cast<double>(pos[2])};
			changed = true;
		}
		render_setting_tooltip("Cartesian position relative to the central object, in the same coordinate units used by the simulation.");

		float vel[3] = {static_cast<float>(b.velocity[0]), static_cast<float>(b.velocity[1]), static_cast<float>(b.velocity[2])};
		if (ImGui::InputFloat3("Velocity (vx, vy, vz)", vel)) {
			b.velocity = {static_cast<double>(vel[0]), static_cast<double>(vel[1]), static_cast<double>(vel[2])};
			changed = true;
		}
		render_setting_tooltip("Instantaneous coordinate velocity of this body.");

		if (ImGui::Button("Set Circular Orbit Velocity", ImVec2(-1.0f, 24.0f))) {
			b.velocity = compute_circular_orbit_velocity(b.position, orchestrator_.parameters().mass);
			changed = true;
		}
		render_setting_tooltip("Overwrites the velocity above with the Keplerian circular-orbit velocity for this body's current distance from the central mass.");

		float spin[3] = {static_cast<float>(b.spin[0]), static_cast<float>(b.spin[1]), static_cast<float>(b.spin[2])};
		if (ImGui::InputFloat3("Spin Vector", spin)) {
			b.spin = {static_cast<double>(spin[0]), static_cast<double>(spin[1]), static_cast<double>(spin[2])};
			changed = true;
		}
		render_setting_tooltip("Intrinsic angular momentum vector, feeding spin-orbit and spin-spin post-Newtonian coupling terms.");

		float quad = static_cast<float>(b.quadrupole_moment);
		if (slider_float_with_input("Quadrupole Moment (Q)", &quad, -1e-2f, 1e-2f, "%.6e")) {
			b.quadrupole_moment = static_cast<double>(quad);
			changed = true;
		}
		render_setting_tooltip("Reserved quadrupole deformation parameter for future tidal and multipolar force models.");

		float j2 = static_cast<float>(b.j2);
		if (slider_float_with_input("Zonal J2 Moment", &j2, -1e-2f, 1e-2f, "%.6e")) {
			b.j2 = static_cast<double>(j2);
			changed = true;
		}
		render_setting_tooltip("Dominant oblateness harmonic coefficient, producing nodal precession on other bodies passing nearby.");

		float j3 = static_cast<float>(b.j3);
		if (slider_float_with_input("Zonal J3 Moment", &j3, -1e-3f, 1e-3f, "%.6e")) {
			b.j3 = static_cast<double>(j3);
			changed = true;
		}
		render_setting_tooltip("Third-degree zonal harmonic coefficient, primarily contributing a north-south asymmetric perturbation.");

		float j4 = static_cast<float>(b.j4);
		if (slider_float_with_input("Zonal J4 Moment", &j4, -1e-3f, 1e-3f, "%.6e")) {
			b.j4 = static_cast<double>(j4);
			changed = true;
		}
		render_setting_tooltip("Fourth-degree zonal harmonic coefficient, a smaller correction to the oblateness perturbation.");

		float r_ref = static_cast<float>(b.reference_radius);
		if (slider_float_with_input("Multipole Reference Radius", &r_ref, 0.001f, 1.0e5f, "%.4f", &selected_r_ref_log_mode_, 1e-6f, 1e12f)) {
			b.reference_radius = std::max(1e-6, static_cast<double>(r_ref));
			changed = true;
		}
		render_setting_tooltip("Reference radius at which the zonal harmonic coefficients above are defined, typically the body's equatorial radius.");

		if (ImGui::CollapsingHeader("Material, Thermal & Electromagnetic Properties")) {
			float charge = static_cast<float>(b.charge);
			if (ImGui::InputFloat("Charge", &charge, 0.01f, 1.0f, "%.4e")) { b.charge = charge; changed = true; }
			float magnetic = static_cast<float>(b.magnetic_moment);
			if (ImGui::InputFloat("Magnetic Moment", &magnetic, 0.01f, 1.0f, "%.4e")) { b.magnetic_moment = magnetic; changed = true; }
			float rotation = static_cast<float>(b.rotation_speed);
			if (ImGui::InputFloat("Rotation Speed", &rotation, 0.01f, 1.0f, "%.4e")) { b.rotation_speed = rotation; changed = true; }
			float friction = static_cast<float>(b.friction_coefficient);
			if (ImGui::SliderFloat("Friction Coefficient", &friction, 0.0f, 1.0f)) { b.friction_coefficient = friction; changed = true; }
			float restitution = static_cast<float>(b.restitution);
			if (ImGui::SliderFloat("Restitution", &restitution, 0.0f, 1.0f)) { b.restitution = restitution; changed = true; }
			float integrity = static_cast<float>(b.integrity);
			if (ImGui::InputFloat("Integrity", &integrity, 0.01f, 1.0f, "%.4e")) { b.integrity = std::max(0.0, static_cast<double>(integrity)); changed = true; }
			float lifetime = static_cast<float>(b.lifetime);
			if (ImGui::InputFloat("Lifetime", &lifetime, 1.0f, 10.0f, "%.4e")) { b.lifetime = std::max(0.0, static_cast<double>(lifetime)); changed = true; }
			float temperature = static_cast<float>(b.temperature);
			if (ImGui::InputFloat("Temperature", &temperature, 1.0f, 100.0f, "%.4e")) { b.temperature = temperature; changed = true; }
			float heat_capacity = static_cast<float>(b.heat_capacity);
			if (ImGui::InputFloat("Heat Capacity", &heat_capacity, 0.01f, 1.0f, "%.4e")) { b.heat_capacity = std::max(0.0, static_cast<double>(heat_capacity)); changed = true; }
			ImGui::ColorEdit4("Primary Color", b.color.data());
			ImGui::ColorEdit4("Secondary Color", b.color_secondary.data());
			char composition[32]{};
			std::memcpy(composition, b.composition.data(), b.composition.size() - 1);
			if (ImGui::InputText("Composition", composition, sizeof(composition))) { b.set_composition(composition); changed = true; }
		}

		ImGui::Spacing();
		ImGui::Text("Speed: %.6e | Kinetic Energy: %.6e", b.speed(), b.kinetic_energy());

		ImGui::Spacing();
		if (ImGui::Button("Look At This Body", ImVec2(150.0f, 24.0f))) {
			look_at(b.position);
		}
		render_setting_tooltip("Rotates the camera to face this body without moving the camera position.");

		ImGui::SameLine();
		if (ImGui::Button("Duplicate Body")) {
			Dynamics::PostNewtonianBody clone = b;
			clone.id = 0;
			clone.set_name(unique_name(display_name(b)));
			clone.position[0] += clone.radius * 4.0;
			sys.add_body(clone);
			changed = true;
		}
		render_setting_tooltip("Creates a copy of this body offset along X, keeping all physical parameters and the name.");

		ImGui::SameLine();
		if (ImGui::Button("Delete Body")) {
			std::vector<Dynamics::PostNewtonianBody> updated;
			for (size_t k = 0; k < n; ++k) {
				if (k != static_cast<size_t>(selected_body_index_)) {
					updated.push_back(bodies[k]);
				}
			}
			sys.clear_bodies();
			for (auto& ub : updated) sys.add_body(ub);
			selected_body_index_ = -1;
			changed = true;
		}
		render_setting_tooltip("Permanently removes this body from the N-body system.");

		if (changed) {
			sys.update_accelerations();
		}
	}

	void render_creation_tab() noexcept {
		const char* preset_names[] = {
			"Custom Body",
			"Sun (Solar Mass & Radius)",
			"Earth (Terrestrial Planet)",
			"Moon (Natural Satellite)",
			"Jupiter (Gas Giant)",
			"Mars (Telluric Planet)",
			"Neutron Star (Compact)",
			"Supermassive Black Hole",
			"Stellar Mass Black Hole",
			"Test Particle (Zero Mass)"
		};

		if (ImGui::Combo("Template Preset", &creation_preset_, preset_names, IM_ARRAYSIZE(preset_names))) {
			apply_template_preset(static_cast<BodyPresetTemplate>(creation_preset_));
		}
		render_setting_tooltip("Fills the fields below with realistic physical parameters for a known body type. Position, velocity, and name are left for you to set.");

		ImGui::Separator();
		ImGui::InputText("Body Name", new_body_name_, sizeof(new_body_name_));
		render_setting_tooltip("Human-readable label shown in the catalog and in saved scenarios instead of a numeric identifier.");
		if (slider_float_with_input("Mass (kg / Geometrized)", &new_body_mass_, 0.001f, 1.0e6f, "%.4f", &new_body_mass_log_mode_, 1e-12f, 1e36f)) {
			if (new_body_name_[0] == '\0' || std::strcmp(new_body_name_, "New Body") == 0) {
				const std::string proposed_name = unique_name(synthesize_creation_name(5, new_body_mass_, new_body_radius_, std::max(10.0f, static_cast<float>(new_body_radius_) * 10.0f)));
				std::strncpy(new_body_name_, proposed_name.c_str(), sizeof(new_body_name_) - 1);
				new_body_name_[sizeof(new_body_name_) - 1] = '\0';
			}
		}
		if (slider_float_with_input("Physical Radius", &new_body_radius_, 0.001f, 1.0e5f, "%.4f", &new_body_radius_log_mode_, 1e-6f, 1e12f)) {
			if (new_body_name_[0] == '\0' || std::strcmp(new_body_name_, "New Body") == 0) {
				const std::string proposed_name = unique_name(synthesize_creation_name(2, new_body_mass_, new_body_radius_, std::max(10.0f, static_cast<float>(new_body_radius_) * 10.0f)));
				std::strncpy(new_body_name_, proposed_name.c_str(), sizeof(new_body_name_) - 1);
				new_body_name_[sizeof(new_body_name_) - 1] = '\0';
			}
		}
		ImGui::InputFloat3("Initial Position (x, y, z)", new_body_pos_);
		ImGui::InputFloat3("Initial Velocity (vx, vy, vz)", new_body_vel_);
		if (ImGui::Button("Randomize Intelligent Defaults", ImVec2(-1.0f, 24.0f))) {
			randomize_creation_defaults();
		}
		render_setting_tooltip("Generates a more varied body profile by sampling correlated mass, radius, orbit, spin, and multipole defaults. The name is refreshed with a more descriptive catalog-style label.");
		if (ImGui::Button("Auto-Fill Circular Orbit Velocity", ImVec2(-1.0f, 24.0f))) {
			const std::array<double, 3> pos{static_cast<double>(new_body_pos_[0]), static_cast<double>(new_body_pos_[1]), static_cast<double>(new_body_pos_[2])};
			const auto v = compute_circular_orbit_velocity(pos, orchestrator_.parameters().mass);
			new_body_vel_[0] = static_cast<float>(v[0]);
			new_body_vel_[1] = static_cast<float>(v[1]);
			new_body_vel_[2] = static_cast<float>(v[2]);
		}
		render_setting_tooltip("Computes the Keplerian circular-orbit velocity for the position entered above and writes it into the velocity fields.");
		ImGui::InputFloat3("Initial Spin Vector", new_body_spin_);

		ImGui::Separator();
		ImGui::TextDisabled("Gravitational Multipolar Moments:");
		slider_float_with_input("Quadrupole Moment (Q)", &new_body_quadrupole_, -1e-2f, 1e-2f, "%.6e");
		slider_float_with_input("Zonal J2", &new_body_j2_, -1e-2f, 1e-2f, "%.6e");
		slider_float_with_input("Zonal J3", &new_body_j3_, -1e-3f, 1e-3f, "%.6e");
		slider_float_with_input("Zonal J4", &new_body_j4_, -1e-3f, 1e-3f, "%.6e");
		slider_float_with_input("Reference Radius", &new_body_r_ref_, 0.001f, 1.0e5f, "%.4f", &new_body_r_ref_log_mode_, 1e-6f, 1e12f);
		render_setting_tooltip("Zonal harmonic coefficients used only when this body exerts oblateness perturbations on other bodies.");

		ImGui::Spacing();
		if (ImGui::Button("Spawn and Inject into System", ImVec2(-1.0f, 32.0f))) {
			auto& sys = orchestrator_.nbody_system();
			Dynamics::PostNewtonianBody body(
				0,
				static_cast<double>(new_body_mass_),
				static_cast<double>(new_body_radius_),
				{static_cast<double>(new_body_pos_[0]), static_cast<double>(new_body_pos_[1]), static_cast<double>(new_body_pos_[2])},
				{static_cast<double>(new_body_vel_[0]), static_cast<double>(new_body_vel_[1]), static_cast<double>(new_body_vel_[2])},
				{static_cast<double>(new_body_spin_[0]), static_cast<double>(new_body_spin_[1]), static_cast<double>(new_body_spin_[2])},
				static_cast<double>(new_body_quadrupole_),
				static_cast<double>(new_body_j2_),
				static_cast<double>(new_body_j3_),
				static_cast<double>(new_body_j4_),
				static_cast<double>(new_body_r_ref_)
			);
			body.set_name(unique_name(std::string_view(new_body_name_)));

			sys.add_body(body);
			sys.update_accelerations();
			selected_body_index_ = static_cast<int>(sys.body_count() - 1);
		}
		render_setting_tooltip("Adds the configured body to the running N-body system and selects it in the catalog.");
	}

	void render_system_dynamics_tab() noexcept {
		auto& sys = orchestrator_.nbody_system();
		const size_t n = sys.body_count();

		if (n == 0) {
			ImGui::TextDisabled("System is empty.");
			return;
		}

		if (ImGui::Button("Recompute System State", ImVec2(-1.0f, 26.0f))) {
			sys.update_accelerations();
		}
		render_setting_tooltip("Forces an immediate recomputation of accelerations and gravitational-wave emission from the current body states.");
		if (ImGui::CollapsingHeader("Global Body Actions", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::InputFloat3("Velocity For All", global_velocity_);
			if (ImGui::Button("Set Velocity For All")) {
				for (auto& body : sys.bodies()) if (body.enabled) body.velocity = {global_velocity_[0], global_velocity_[1], global_velocity_[2]};
				sys.update_accelerations();
			}
			ImGui::SameLine();
			if (ImGui::Button("Invert All Velocities")) {
				for (auto& body : sys.bodies()) if (body.enabled) for (double& component : body.velocity) component = -component;
				sys.update_accelerations();
			}
			ImGui::InputFloat3("Spin For All", global_spin_);
			if (ImGui::Button("Set Spin For All")) {
				for (auto& body : sys.bodies()) if (body.enabled) body.spin = {global_spin_[0], global_spin_[1], global_spin_[2]};
				sys.update_accelerations();
			}
			ImGui::InputFloat("Grid Precision", &grid_spacing_, 0.1f, 1.0f, "%.4g");
			if (ImGui::Button("Snap Positions To Grid")) {
				Dynamics::BulkBodyActions::snap_to_grid(sys, static_cast<double>(grid_spacing_));
			}
			ImGui::SameLine();
			if (ImGui::Button("Scatter Positions")) {
				Dynamics::BulkBodyActions::scatter_positions(sys);
			}
			if (ImGui::Button("Equalize Mass For All")) {
				Dynamics::BulkBodyActions::equalize_masses(sys);
			}
			ImGui::SameLine();
			if (ImGui::Button("Zero All Spins")) {
				Dynamics::BulkBodyActions::zero_all_spins(sys);
			}
			if (ImGui::Button("Cull Bodies Outside Render Distance")) {
				const auto& p = orchestrator_.parameters();
				const double limit = (p.render_distance_scale > 0.0) ? (p.render_distance_scale * std::max(p.mass, 1e-6)) : 1.0e7;
				Dynamics::BulkBodyActions::cull_outside_radius(sys, limit);
			}
			render_setting_tooltip("Removes every body whose distance from the origin exceeds the current render distance, approximating a view-frustum cull.");
		}
		ImGui::Separator();

		ImGui::Text("Total System Mass:     %.6e", sys.total_mass());
		const auto cm = sys.center_of_mass();
		ImGui::Text("Center of Mass (x,y,z): (%.2e, %.2e, %.2e)", cm[0], cm[1], cm[2]);
		render_setting_tooltip("Mass-weighted average position of all orbiting bodies.");

		const auto p_tot = sys.total_linear_momentum();
		ImGui::Text("Total Linear Momentum: (%.2e, %.2e, %.2e)", p_tot[0], p_tot[1], p_tot[2]);

		const auto l_tot = sys.total_angular_momentum();
		ImGui::Text("Total Angular Momentum:(%.2e, %.2e, %.2e)", l_tot[0], l_tot[1], l_tot[2]);
		render_setting_tooltip("Includes the 1PN spin-orbit correction when enabled and available for a two-body configuration.");

		ImGui::Text("Total Mechanical Energy: %.6e", sys.compute_total_energy());

		const auto& gw = sys.latest_gw_emission();
		ImGui::Text("GW Radiated Power:       %.6e W", gw.radiated_power);
		render_setting_tooltip("Quadrupole-formula gravitational-wave luminosity computed from the current body configuration.");
	}

	void render_interactions_tab() noexcept {
		auto& cfg = orchestrator_.interaction_config();
		const auto bodies_span = orchestrator_.nbody_system().bodies();

		ImGui::TextColored(ImVec4(0.3f, 0.9f, 1.0f, 1.0f), "Electromagnetic Interactions");
		bool electricity = cfg.electromagnetic.electricity_enabled;
		if (ImGui::Checkbox("Enable Electricity (Coulomb Force)", &electricity)) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionElectricityEnabled, electricity ? 1.0 : 0.0)));
		}
		render_setting_tooltip("Enables attraction and repulsion between charged bodies following Coulomb's law, scaled by the medium permittivity below. Charge is set per body in the Body Catalog tab.");
		bool magnetism = cfg.electromagnetic.magnetism_enabled;
		if (ImGui::Checkbox("Enable Magnetism (Dipole Force)", &magnetism)) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionMagnetismEnabled, magnetism ? 1.0 : 0.0)));
		}
		render_setting_tooltip("Enables dipole-dipole magnetic forces derived from each body's magnetic moment value. Both interacting bodies must have a non-zero magnetic moment for a force to appear.");
		{
			const auto warn = Dynamics::magnetism_without_moments_warning(bodies_span, cfg.electromagnetic);
			if (!warn.empty()) render_wrapped_colored_text(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), std::string(warn).c_str());
		}
		float permittivity = static_cast<float>(cfg.electromagnetic.vacuum_permittivity);
		if (slider_float_with_input("Medium Permittivity (epsilon)", &permittivity, 1e-14f, 1.0f, "%.4e")) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionVacuumPermittivity, static_cast<double>(permittivity))));
		}
		render_setting_tooltip("Electrical permittivity of the medium the bodies interact through. Reduces the Coulomb force below its vacuum strength as this value grows.");
		float permeability = static_cast<float>(cfg.electromagnetic.vacuum_permeability);
		if (slider_float_with_input("Medium Permeability (mu)", &permeability, 1e-10f, 10.0f, "%.4e")) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionVacuumPermeability, static_cast<double>(permeability))));
		}
		render_setting_tooltip("Magnetic permeability of the medium the bodies interact through, scaling the dipole-dipole magnetic force.");
		ImGui::Spacing();
		if (ImGui::Button("Sync To Vacuum Values From Constants Engine", ImVec2(-1.0f, 26.0f))) {
			const auto& engine = orchestrator_.constants_engine();
			cfg.electromagnetic.vacuum_permittivity = engine.sim_vacuum_permittivity();
			cfg.electromagnetic.vacuum_permeability = engine.sim_vacuum_permeability();
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionVacuumPermittivity, cfg.electromagnetic.vacuum_permittivity)));
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionVacuumPermeability, cfg.electromagnetic.vacuum_permeability)));
		}
		render_setting_tooltip("Overwrites the two fields above with the true vacuum permittivity and permeability derived from the fundamental constants c, G, h, kB currently active in the Physical Constants Engine window, keeping both subsystems consistent.");

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.2f, 1.0f), "Collisions");
		bool collisions = cfg.collisions.enabled;
		if (ImGui::Checkbox("Enable Collisions", &collisions)) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionCollisionsEnabled, collisions ? 1.0 : 0.0)));
		}
		render_setting_tooltip("Detects physical contact between overlapping bodies and resolves it with a Hertzian material-stiffness repulsion plus an impulse-based restitution and friction response. See docs/other/COLLISION_MODEL.md for the underlying derivation.");
		{
			const auto warn = Dynamics::collisions_without_radius_warning(bodies_span, cfg.collisions);
			if (!warn.empty()) render_wrapped_colored_text(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), std::string(warn).c_str());
		}
		if (collisions) {
			int response_idx = static_cast<int>(cfg.collisions.response_model);
			const char* response_names[] = {"Elastic (Restitution-Based)", "Inelastic (Perfectly Damped)"};
			if (ImGui::Combo("Response Model", &response_idx, response_names, IM_ARRAYSIZE(response_names))) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionCollisionResponseModel, static_cast<double>(response_idx))));
			}
			render_setting_tooltip("Elastic uses each body's Restitution property to bounce apart; Inelastic removes all normal-direction relative velocity on contact.");
			bool consider_rotation = cfg.collisions.consider_rotation;
			if (ImGui::Checkbox("Consider Rotation", &consider_rotation)) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionCollisionConsiderRotation, consider_rotation ? 1.0 : 0.0)));
			}
			render_setting_tooltip("Lets tangential friction impulses spin bodies up around their rotation axis, approximated as solid spheres (I = 0.4 * m * r^2).");
			ImGui::SameLine();
			bool consider_friction = cfg.collisions.consider_friction;
			if (ImGui::Checkbox("Consider Friction", &consider_friction)) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionCollisionConsiderFriction, consider_friction ? 1.0 : 0.0)));
			}
			render_setting_tooltip("Applies a Coulomb-clamped tangential impulse opposing contact-point sliding velocity, using the average of both bodies' Friction Coefficient property.");
			float restitution_mult = static_cast<float>(cfg.collisions.restitution_multiplier);
			if (slider_float_with_input("Restitution Multiplier", &restitution_mult, 0.0f, 4.0f, "%.2fx")) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionCollisionRestitutionMultiplier, static_cast<double>(restitution_mult))));
			}
			render_setting_tooltip("Global multiplier applied on top of each body's own Restitution property before it is clamped back into the physical [0, 1] range.");
			float stiffness = static_cast<float>(cfg.collisions.contact_stiffness_scale);
			if (slider_float_with_input("Contact Stiffness Scale", &stiffness, 0.0f, 1.0f, "%.6e")) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionCollisionStiffnessScale, static_cast<double>(stiffness))));
			}
			render_setting_tooltip("Scales the Hertzian penetration repulsion force computed from each body's Young's Modulus property and the current overlap depth. Zero disables material-stiffness pushback and relies only on the instantaneous impulse response below.");
			float position_correction = static_cast<float>(cfg.collisions.position_correction_factor);
			if (slider_float_with_input("Position Correction Factor", &position_correction, 0.0f, 1.0f, "%.2f")) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionCollisionPositionCorrectionFactor, static_cast<double>(position_correction))));
			}
			render_setting_tooltip("Baumgarte-style geometric correction that nudges deeply overlapping bodies apart each step, mass-weighted, preventing residual sinking when the material stiffness above is too soft to fully separate stiff bodies in a single step.");
		}

		ImGui::Separator();
		ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.3f, 1.0f), "Thermodynamics");
		bool thermo = cfg.thermodynamics.enabled;
		if (ImGui::Checkbox("Enable Thermodynamics", &thermo)) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionThermodynamicsEnabled, thermo ? 1.0 : 0.0)));
		}
		render_setting_tooltip("Tracks per-body Temperature and Heat Capacity properties and radiates energy toward the ambient temperature below via the Stefan-Boltzmann law, scaled by each body's Absorption Factor.");
		{
			const auto warn = Dynamics::thermodynamics_disabled_ambient_note(cfg.thermodynamics);
			if (!warn.empty()) render_wrapped_colored_text(ImVec4(0.6f, 0.75f, 1.0f, 1.0f), std::string(warn).c_str());
		}
		if (thermo) {
			float ambient = static_cast<float>(cfg.thermodynamics.ambient_temperature_kelvin);
			if (slider_float_with_input("Ambient Temperature (K, -1 = none)", &ambient, -1.0f, 6000.0f, "%.1f")) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionAmbientTemperature, static_cast<double>(ambient))));
			}
			render_setting_tooltip("Background radiative temperature bodies cool toward or heat toward via Stefan-Boltzmann emission. Set to -1 to disable ambient radiative coupling entirely while keeping per-body heat capacity bookkeeping active.");
			float coupling = static_cast<float>(cfg.thermodynamics.radiative_coupling_scale);
			if (slider_float_with_input("Radiative Coupling Scale", &coupling, 0.0f, 10.0f, "%.2fx")) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionRadiativeCouplingScale, static_cast<double>(coupling))));
			}
			render_setting_tooltip("Multiplies the Stefan-Boltzmann radiative exchange rate, letting the ambient coupling be sped up or slowed down without touching the physical Stefan-Boltzmann constant itself.");
		}

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.9f, 0.4f, 0.4f, 1.0f), "Fragmentation");
		bool fragmentation = cfg.fragmentation.enabled;
		if (ImGui::Checkbox("Enable Fragmentation", &fragmentation)) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionFragmentationEnabled, fragmentation ? 1.0 : 0.0)));
		}
		render_setting_tooltip("Allows bodies to shatter into smaller fragments once their Integrity property is depleted by high-energy collisions and, optionally, sustained tidal stress from the central spacetime source.");
		{
			const auto warn = Dynamics::fragmentation_requires_source_warning(cfg.fragmentation, cfg.collisions);
			if (!warn.empty()) render_wrapped_colored_text(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), std::string(warn).c_str());
		}
		if (fragmentation) {
			float min_fragment_mass = static_cast<float>(cfg.fragmentation.minimum_fragment_mass);
			if (slider_float_with_input("Minimum Fragment Mass", &min_fragment_mass, 1e-9f, 1e3f, "%.4e")) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionMinimumFragmentMass, static_cast<double>(min_fragment_mass))));
			}
			render_setting_tooltip("Fragments whose computed mass would fall below this floor are dispersed entirely rather than spawned as new bodies, preventing runaway fragment counts.");
			int max_fragments = static_cast<int>(cfg.fragmentation.max_fragments_per_event);
			if (slider_int_with_input("Max Fragments Per Event", &max_fragments, 1, 8)) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionFragmentationMaxFragments, static_cast<double>(max_fragments))));
			}
			float energy_to_integrity = static_cast<float>(cfg.fragmentation.collision_energy_to_integrity_loss);
			if (slider_float_with_input("Collision Energy To Integrity Loss", &energy_to_integrity, 0.0f, 1.0f, "%.4e")) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionCollisionEnergyToIntegrityLoss, static_cast<double>(energy_to_integrity))));
			}
			render_setting_tooltip("Conversion factor from an impact's kinetic energy along the collision normal into lost Integrity for both colliding bodies.");
			bool tidal_stress = cfg.fragmentation.enable_tidal_stress;
			if (ImGui::Checkbox("Enable Tidal Disruption From Central Source", &tidal_stress)) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionFragmentationTidalStressEnabled, tidal_stress ? 1.0 : 0.0)));
			}
			render_setting_tooltip("Continuously erodes Integrity for bodies close to the central mass based on the differential gravitational acceleration across their physical radius, approximating tidal stretching near the Roche limit.");
			if (tidal_stress) {
				float tidal_to_integrity = static_cast<float>(cfg.fragmentation.tidal_stress_to_integrity_loss);
				if (slider_float_with_input("Tidal Stress To Integrity Loss", &tidal_to_integrity, 0.0f, 1.0f, "%.4e")) {
					static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionFragmentationTidalStressToIntegrityLoss, static_cast<double>(tidal_to_integrity))));
				}
				render_setting_tooltip("Conversion factor from tidal stress energy density into lost Integrity per second.");
			}
		}

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.7f, 0.5f, 1.0f, 1.0f), "Annihilation");
		bool annihilation = cfg.annihilation.enabled;
		if (ImGui::Checkbox("Enable Annihilation", &annihilation)) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionAnnihilationEnabled, annihilation ? 1.0 : 0.0)));
		}
		render_setting_tooltip("Removes both colliding bodies entirely once they satisfy the contact and charge conditions below, modeling a complete matter-antimatter style annihilation event.");
		{
			const auto warn = Dynamics::annihilation_requires_charge_warning(cfg.annihilation, cfg.electromagnetic);
			if (!warn.empty()) render_wrapped_colored_text(ImVec4(0.6f, 0.75f, 1.0f, 1.0f), std::string(warn).c_str());
		}
		if (annihilation) {
			bool opposite_charge = cfg.annihilation.require_opposite_charge;
			if (ImGui::Checkbox("Require Opposite Charge", &opposite_charge)) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionAnnihilationRequireOppositeCharge, opposite_charge ? 1.0 : 0.0)));
			}
			float contact_scale = static_cast<float>(cfg.annihilation.contact_distance_scale);
			if (slider_float_with_input("Contact Distance Scale", &contact_scale, 0.01f, 4.0f, "%.2fx")) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionAnnihilationContactScale, static_cast<double>(contact_scale))));
			}
			render_setting_tooltip("Fraction of the combined radii within which annihilation triggers; values below 1 require deeper overlap than plain collision contact.");
		}
	}

	void apply_template_preset(BodyPresetTemplate preset) noexcept {
		switch (preset) {
			case BodyPresetTemplate::Sun:
				std::strncpy(new_body_name_, "Sun", sizeof(new_body_name_) - 1);
				new_body_mass_ = 1.98847e30f;
				new_body_radius_ = 6.9634e8f;
				new_body_j2_ = 2.2e-7f;
				new_body_r_ref_ = 6.9634e8f;
				break;
			case BodyPresetTemplate::Earth:
				std::strncpy(new_body_name_, "Earth", sizeof(new_body_name_) - 1);
				new_body_mass_ = 5.9722e24f;
				new_body_radius_ = 6.378137e6f;
				new_body_j2_ = 1.08263e-3f;
				new_body_r_ref_ = 6.378137e6f;
				break;
			case BodyPresetTemplate::Moon:
				std::strncpy(new_body_name_, "Moon", sizeof(new_body_name_) - 1);
				new_body_mass_ = 7.342e22f;
				new_body_radius_ = 1.7374e6f;
				new_body_j2_ = 2.0335e-4f;
				new_body_r_ref_ = 1.7374e6f;
				break;
			case BodyPresetTemplate::Jupiter:
				std::strncpy(new_body_name_, "Jupiter", sizeof(new_body_name_) - 1);
				new_body_mass_ = 1.89813e27f;
				new_body_radius_ = 7.1492e7f;
				new_body_j2_ = 1.469657e-2f;
				new_body_j4_ = -5.86609e-4f;
				new_body_r_ref_ = 7.1492e7f;
				break;
			case BodyPresetTemplate::Mars:
				std::strncpy(new_body_name_, "Mars", sizeof(new_body_name_) - 1);
				new_body_mass_ = 6.4171e23f;
				new_body_radius_ = 3.3895e6f;
				new_body_j2_ = 1.96045e-3f;
				new_body_r_ref_ = 3.3895e6f;
				break;
			case BodyPresetTemplate::NeutronStar:
				std::strncpy(new_body_name_, "Neutron Star", sizeof(new_body_name_) - 1);
				new_body_mass_ = 2.8e30f;
				new_body_radius_ = 12000.0f;
				new_body_j2_ = 0.0f;
				new_body_r_ref_ = 12000.0f;
				break;
			case BodyPresetTemplate::SupermassiveBlackHole:
				std::strncpy(new_body_name_, "Supermassive BH", sizeof(new_body_name_) - 1);
				new_body_mass_ = 8.0e36f;
				new_body_radius_ = 1.2e10f;
				new_body_j2_ = 0.0f;
				new_body_r_ref_ = 1.2e10f;
				break;
			case BodyPresetTemplate::StellarBlackHole:
				std::strncpy(new_body_name_, "Stellar Black Hole", sizeof(new_body_name_) - 1);
				new_body_mass_ = 2.0e31f;
				new_body_radius_ = 30000.0f;
				new_body_j2_ = 0.0f;
				new_body_r_ref_ = 30000.0f;
				break;
			case BodyPresetTemplate::TestParticle:
				std::strncpy(new_body_name_, "Test Particle", sizeof(new_body_name_) - 1);
				new_body_mass_ = 0.0f;
				new_body_radius_ = 1.0f;
				new_body_j2_ = 0.0f;
				new_body_r_ref_ = 1.0f;
				break;
			case BodyPresetTemplate::Custom:
			default:
				randomize_creation_defaults();
				break;
		}
		new_body_mass_log_mode_ = true;
		new_body_radius_log_mode_ = true;
		new_body_r_ref_log_mode_ = true;
		new_body_name_[sizeof(new_body_name_) - 1] = '\0';
	}

	void populate_solar_system_archetype() noexcept {
		auto& sys = orchestrator_.nbody_system();
		sys.clear_bodies();

		const double central_mass = orchestrator_.parameters().mass;

		Dynamics::PostNewtonianBody planet1(
			0, 1e-4, 0.05,
			{10.0, 0.0, 0.0},
			compute_circular_orbit_velocity({10.0, 0.0, 0.0}, central_mass),
			{0.0, 0.0, 0.0}
		);
		planet1.set_name(unique_name("Inner Planet"));

		Dynamics::PostNewtonianBody planet2(
			0, 3e-4, 0.08,
			{25.0, 0.0, 0.0},
			compute_circular_orbit_velocity({25.0, 0.0, 0.0}, central_mass),
			{0.0, 0.0, 0.0}
		);
		planet2.set_name(unique_name("Outer Planet"));

		sys.add_body(planet1);
		sys.add_body(planet2);
		sys.update_accelerations();
		selected_body_index_ = 0;
	}
};

}

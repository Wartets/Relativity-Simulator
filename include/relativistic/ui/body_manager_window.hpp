#pragma once

#include <imgui.h>
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/dynamics/pn_body.hpp"
#include "relativistic/dynamics/pn_nbody_system.hpp"
#include "relativistic/core/constants.hpp"
#include "relativistic/ui/tooltip_utils.hpp"
#include <vector>
#include <string>
#include <string_view>
#include <array>
#include <cmath>
#include <numbers>
#include <algorithm>
#include <cstring>
#include <cfloat>

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

	char search_filter_[64]{};
	int sort_mode_{static_cast<int>(BodyCatalogSortMode::CreationOrder)};
	bool sort_descending_{false};
	float list_pane_width_{230.0f};
	char rename_buffer_[32]{};
	int rename_target_id_{-1};

public:
	explicit BodyManagerWindow(Orchestrator::SimulationOrchestrator<1024>& orchestrator)
		: orchestrator_(orchestrator) {}

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
			if (ImGui::BeginTabItem("Body Catalog")) {
				render_body_list_tab();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Create Body")) {
				render_creation_tab();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("System Dynamics")) {
				render_system_dynamics_tab();
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}

		ImGui::End();
	}

private:
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
		const double speed = std::sqrt(std::max(central_mass, 0.0) / r);
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
		if (ImGui::InputFloat("Central Mass (M)", &mass, 0.1f, 10.0f, "%.4e")) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::Mass, std::max(0.01, static_cast<double>(mass)))));
		}
		render_setting_tooltip("Central gravitating mass in geometrized units. Governs the Schwarzschild radius rs = 2M and the overall curvature strength.");

		float spin = static_cast<float>(params.spin);
		if (ImGui::InputFloat("Spin Parameter (a)", &spin, 0.01f, 0.1f, "%.4f")) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::Spin, std::clamp(static_cast<double>(spin), -0.999 * params.mass, 0.999 * params.mass))));
		}
		render_setting_tooltip("Specific angular momentum a = J / M, clamped to the subextremal range. Only meaningful for Kerr-family metrics.");

		float charge = static_cast<float>(params.charge);
		if (ImGui::InputFloat("Electric Charge (Q)", &charge, 0.01f, 0.1f, "%.4f")) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::Charge, static_cast<double>(charge))));
		}
		render_setting_tooltip("Net electrostatic charge. Only meaningful for Reissner-Nordstrom and Kerr-Newman metrics.");

		ImGui::Spacing();
		if (ImGui::Button("Look At Central Object", ImVec2(-1.0f, 26.0f))) {
			orchestrator_.camera().target = {0.0, 0.0, 0.0};
		}
		render_setting_tooltip("Orients the camera to face the central object without changing its position.");
	}

	void render_selected_nbody_panel(Dynamics::PostNewtonianSystem& sys, auto bodies, size_t n) noexcept {
		auto& b = bodies[static_cast<size_t>(selected_body_index_)];
		bool changed = false;

		ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%s", display_name(b).c_str());
		ImGui::TextDisabled("Identifier: #%u", b.id);
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
		if (ImGui::InputFloat("Mass", &m, 0.01f, 1.0f, "%.4e")) {
			b.mass = std::max(0.0, static_cast<double>(m));
			changed = true;
		}
		render_setting_tooltip("Gravitating mass of this body in the same geometrized unit system as the central mass.");

		float r = static_cast<float>(b.radius);
		if (ImGui::InputFloat("Physical Radius", &r, 0.01f, 1.0f, "%.4e")) {
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
		if (ImGui::InputFloat("Quadrupole Moment (Q)", &quad, 1e-4f, 1e-2f, "%.6e")) {
			b.quadrupole_moment = static_cast<double>(quad);
			changed = true;
		}
		render_setting_tooltip("Reserved quadrupole deformation parameter for future tidal and multipolar force models.");

		float j2 = static_cast<float>(b.j2);
		if (ImGui::InputFloat("Zonal J2 Moment", &j2, 1e-5f, 1e-3f, "%.6e")) {
			b.j2 = static_cast<double>(j2);
			changed = true;
		}
		render_setting_tooltip("Dominant oblateness harmonic coefficient, producing nodal precession on other bodies passing nearby.");

		float j3 = static_cast<float>(b.j3);
		if (ImGui::InputFloat("Zonal J3 Moment", &j3, 1e-6f, 1e-4f, "%.6e")) {
			b.j3 = static_cast<double>(j3);
			changed = true;
		}
		render_setting_tooltip("Third-degree zonal harmonic coefficient, primarily contributing a north-south asymmetric perturbation.");

		float j4 = static_cast<float>(b.j4);
		if (ImGui::InputFloat("Zonal J4 Moment", &j4, 1e-6f, 1e-4f, "%.6e")) {
			b.j4 = static_cast<double>(j4);
			changed = true;
		}
		render_setting_tooltip("Fourth-degree zonal harmonic coefficient, a smaller correction to the oblateness perturbation.");

		float r_ref = static_cast<float>(b.reference_radius);
		if (ImGui::InputFloat("Multipole Reference Radius", &r_ref, 0.01f, 1.0f, "%.4e")) {
			b.reference_radius = std::max(1e-6, static_cast<double>(r_ref));
			changed = true;
		}
		render_setting_tooltip("Reference radius at which the zonal harmonic coefficients above are defined, typically the body's equatorial radius.");

		ImGui::Spacing();
		ImGui::Text("Speed: %.6e | Kinetic Energy: %.6e", b.speed(), b.kinetic_energy());

		ImGui::Spacing();
		if (ImGui::Button("Look At This Body", ImVec2(150.0f, 24.0f))) {
			orchestrator_.camera().target = b.position;
			const double dx = b.position[0] - orchestrator_.camera().position[0];
			const double dy = b.position[1] - orchestrator_.camera().position[1];
			const double dz = b.position[2] - orchestrator_.camera().position[2];
			const double d_tot = std::sqrt(dx * dx + dy * dy + dz * dz);
			if (d_tot > 1e-6) {
				orchestrator_.camera().yaw = std::atan2(dx, -dy) * (180.0 / std::numbers::pi);
				orchestrator_.camera().pitch = std::asin(std::clamp(dz / d_tot, -0.9999, 0.9999)) * (180.0 / std::numbers::pi);
			}
		}
		render_setting_tooltip("Rotates the camera to face this body without moving the camera position.");

		ImGui::SameLine();
		if (ImGui::Button("Duplicate Body")) {
			Dynamics::PostNewtonianBody clone = b;
			clone.id = static_cast<uint32_t>(sys.body_count() + 1);
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
		ImGui::InputFloat("Mass (kg / Geometrized)", &new_body_mass_, 0.1f, 10.0f, "%.4e");
		ImGui::InputFloat("Physical Radius", &new_body_radius_, 0.1f, 10.0f, "%.4e");
		ImGui::InputFloat3("Initial Position (x, y, z)", new_body_pos_);
		ImGui::InputFloat3("Initial Velocity (vx, vy, vz)", new_body_vel_);
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
		ImGui::InputFloat("Quadrupole Moment (Q)", &new_body_quadrupole_, 1e-4f, 1e-2f, "%.6e");
		ImGui::InputFloat("Zonal J2", &new_body_j2_, 1e-5f, 1e-3f, "%.6e");
		ImGui::InputFloat("Zonal J3", &new_body_j3_, 1e-6f, 1e-4f, "%.6e");
		ImGui::InputFloat("Zonal J4", &new_body_j4_, 1e-6f, 1e-4f, "%.6e");
		ImGui::InputFloat("Reference Radius", &new_body_r_ref_, 0.1f, 1.0f, "%.4e");
		render_setting_tooltip("Zonal harmonic coefficients used only when this body exerts oblateness perturbations on other bodies.");

		ImGui::Spacing();
		if (ImGui::Button("Spawn and Inject into System", ImVec2(-1.0f, 32.0f))) {
			auto& sys = orchestrator_.nbody_system();
			const uint32_t next_id = static_cast<uint32_t>(sys.body_count() + 1);

			Dynamics::PostNewtonianBody body(
				next_id,
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
			body.set_name(std::string_view(new_body_name_));

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
				std::strncpy(new_body_name_, "New Body", sizeof(new_body_name_) - 1);
				new_body_mass_ = 1.0f;
				new_body_radius_ = 1.0f;
				new_body_j2_ = 0.0f;
				new_body_r_ref_ = 1.0f;
				break;
		}
		new_body_name_[sizeof(new_body_name_) - 1] = '\0';
	}

	void populate_solar_system_archetype() noexcept {
		auto& sys = orchestrator_.nbody_system();
		sys.clear_bodies();

		Dynamics::PostNewtonianBody sun(
			1, 1.0, 0.5,
			{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
			0.0, 2.2e-7, 0.0, 0.0, 0.5
		);
		sun.set_name("Sun");

		Dynamics::PostNewtonianBody planet1(
			2, 1e-4, 0.05,
			{10.0, 0.0, 0.0}, {0.0, 0.3162, 0.0}, {0.0, 0.0, 0.0}
		);
		planet1.set_name("Inner Planet");

		Dynamics::PostNewtonianBody planet2(
			3, 3e-4, 0.08,
			{25.0, 0.0, 0.0}, {0.0, 0.2, 0.0}, {0.0, 0.0, 0.0}
		);
		planet2.set_name("Outer Planet");

		sys.add_body(sun);
		sys.add_body(planet1);
		sys.add_body(planet2);
		sys.update_accelerations();
		selected_body_index_ = 0;
	}
};

}

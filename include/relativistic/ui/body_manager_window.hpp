#pragma once

#include <imgui.h>
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/dynamics/pn_body.hpp"
#include "relativistic/dynamics/pn_nbody_system.hpp"
#include "relativistic/core/constants.hpp"
#include <vector>
#include <string>
#include <array>
#include <cmath>
#include <numbers>
#include <algorithm>

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

class BodyManagerWindow {
private:
	bool is_open_{false};
	Orchestrator::SimulationOrchestrator<1024>& orchestrator_;

	int selected_body_index_{-1};
	int creation_preset_{0};

	char new_body_name_[64]{"New Body"};
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

public:
	explicit BodyManagerWindow(Orchestrator::SimulationOrchestrator<1024>& orchestrator)
		: orchestrator_(orchestrator) {}

	[[nodiscard]] bool& open_state() noexcept {
		return is_open_;
	}

	void render() {
		if (!is_open_) return;

		ImGui::SetNextWindowPos(ImVec2(15.0f, 400.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(460.0f, 580.0f), ImGuiCond_FirstUseEver);

		if (!ImGui::Begin("Celestial Body & N-Body Manager", &is_open_)) {
			ImGui::End();
			return;
		}

		auto& sys = orchestrator_.nbody_system();
		const size_t body_count = sys.body_count();

		ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "Active Bodies in System: %zu", body_count);
		ImGui::SameLine();
		if (ImGui::Button("Clear All Bodies")) {
			sys.clear_bodies();
			selected_body_index_ = -1;
		}

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
	void render_body_list_tab() noexcept {
		auto& sys = orchestrator_.nbody_system();
		auto bodies = sys.bodies();
		const size_t n = bodies.size();

		if (n == 0) {
			ImGui::TextDisabled("No celestial bodies currently populated.");
			ImGui::Spacing();
			if (ImGui::Button("Spawn Solar System Archetype", ImVec2(240.0f, 28.0f))) {
				populate_solar_system_archetype();
			}
			return;
		}

		ImGui::Columns(2, "BodyColumns", true);
		ImGui::SetColumnWidth(0, 160.0f);

		for (size_t i = 0; i < n; ++i) {
			const bool is_selected = (selected_body_index_ == static_cast<int>(i));
			std::string label = "Body #" + std::to_string(bodies[i].id) + " (M=" + std::to_string(bodies[i].mass).substr(0, 4) + ")";
			if (ImGui::Selectable(label.c_str(), is_selected)) {
				selected_body_index_ = static_cast<int>(i);
			}
		}

		ImGui::NextColumn();

		if (selected_body_index_ >= 0 && selected_body_index_ < static_cast<int>(n)) {
			auto& b = bodies[selected_body_index_];

			ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Body ID: %u", b.id);
			ImGui::Separator();

			float m = static_cast<float>(b.mass);
			if (ImGui::InputFloat("Mass", &m, 0.01f, 1.0f, "%.4e")) {
				b.mass = std::max(0.0, static_cast<double>(m));
			}

			float r = static_cast<float>(b.radius);
			if (ImGui::InputFloat("Physical Radius", &r, 0.01f, 1.0f, "%.4e")) {
				b.radius = std::max(1e-6, static_cast<double>(r));
			}

			float pos[3] = {static_cast<float>(b.position[0]), static_cast<float>(b.position[1]), static_cast<float>(b.position[2])};
			if (ImGui::InputFloat3("Position (x, y, z)", pos)) {
				b.position = {static_cast<double>(pos[0]), static_cast<double>(pos[1]), static_cast<double>(pos[2])};
			}

			float vel[3] = {static_cast<float>(b.velocity[0]), static_cast<float>(b.velocity[1]), static_cast<float>(b.velocity[2])};
			if (ImGui::InputFloat3("Velocity (vx, vy, vz)", vel)) {
				b.velocity = {static_cast<double>(vel[0]), static_cast<double>(vel[1]), static_cast<double>(vel[2])};
			}

			float spin[3] = {static_cast<float>(b.spin[0]), static_cast<float>(b.spin[1]), static_cast<float>(b.spin[2])};
			if (ImGui::InputFloat3("Spin Vector", spin)) {
				b.spin = {static_cast<double>(spin[0]), static_cast<double>(spin[1]), static_cast<double>(spin[2])};
			}

			float j2 = static_cast<float>(b.j2);
			if (ImGui::InputFloat("Zonal J2 Moment", &j2, 1e-5f, 1e-3f, "%.6e")) {
				b.j2 = static_cast<double>(j2);
			}

			float j3 = static_cast<float>(b.j3);
			if (ImGui::InputFloat("Zonal J3 Moment", &j3, 1e-6f, 1e-4f, "%.6e")) {
				b.j3 = static_cast<double>(j3);
			}

			float j4 = static_cast<float>(b.j4);
			if (ImGui::InputFloat("Zonal J4 Moment", &j4, 1e-6f, 1e-4f, "%.6e")) {
				b.j4 = static_cast<double>(j4);
			}

			ImGui::Spacing();
			if (ImGui::Button("Look At This Body", ImVec2(160.0f, 24.0f))) {
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
			}
		}

		ImGui::Columns(1);
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

		ImGui::Separator();
		ImGui::InputText("Identifier Tag", new_body_name_, sizeof(new_body_name_));
		ImGui::InputFloat("Mass (kg / Geometrized)", &new_body_mass_, 0.1f, 10.0f, "%.4e");
		ImGui::InputFloat("Physical Radius", &new_body_radius_, 0.1f, 10.0f, "%.4e");
		ImGui::InputFloat3("Initial Position (x, y, z)", new_body_pos_);
		ImGui::InputFloat3("Initial Velocity (vx, vy, vz)", new_body_vel_);
		ImGui::InputFloat3("Initial Spin Vector", new_body_spin_);

		ImGui::Separator();
		ImGui::TextDisabled("Gravitational Multipolar Moments:");
		ImGui::InputFloat("Zonal J2", &new_body_j2_, 1e-5f, 1e-3f, "%.6e");
		ImGui::InputFloat("Zonal J3", &new_body_j3_, 1e-6f, 1e-4f, "%.6e");
		ImGui::InputFloat("Zonal J4", &new_body_j4_, 1e-6f, 1e-4f, "%.6e");
		ImGui::InputFloat("Reference Radius", &new_body_r_ref_, 0.1f, 1.0f, "%.4e");

		ImGui::Spacing();
		if (ImGui::Button("Spawn and Inject into System", ImVec2(260.0f, 32.0f))) {
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

			sys.add_body(body);
			sys.update_accelerations();
			selected_body_index_ = static_cast<int>(sys.body_count() - 1);
		}
	}

	void render_system_dynamics_tab() noexcept {
		auto& sys = orchestrator_.nbody_system();
		const size_t n = sys.body_count();

		if (n == 0) {
			ImGui::TextDisabled("System is empty.");
			return;
		}

		ImGui::Text("Total System Mass:     %.6e", sys.total_mass());
		const auto cm = sys.center_of_mass();
		ImGui::Text("Center of Mass (x,y,z): (%.2e, %.2e, %.2e)", cm[0], cm[1], cm[2]);

		const auto p_tot = sys.total_linear_momentum();
		ImGui::Text("Total Linear Momentum: (%.2e, %.2e, %.2e)", p_tot[0], p_tot[1], p_tot[2]);

		const auto l_tot = sys.total_angular_momentum();
		ImGui::Text("Total Angular Momentum:(%.2e, %.2e, %.2e)", l_tot[0], l_tot[1], l_tot[2]);

		ImGui::Text("Total Mechanical Energy: %.6e", sys.compute_total_energy());

		const auto& gw = sys.latest_gw_emission();
		ImGui::Text("GW Radiated Power:       %.6e W", gw.radiated_power);
	}

	void apply_template_preset(BodyPresetTemplate preset) noexcept {
		switch (preset) {
			case BodyPresetTemplate::Sun:
				new_body_mass_ = 1.98847e30f;
				new_body_radius_ = 6.9634e8f;
				new_body_j2_ = 2.2e-7f;
				new_body_r_ref_ = 6.9634e8f;
				break;
			case BodyPresetTemplate::Earth:
				new_body_mass_ = 5.9722e24f;
				new_body_radius_ = 6.378137e6f;
				new_body_j2_ = 1.08263e-3f;
				new_body_r_ref_ = 6.378137e6f;
				break;
			case BodyPresetTemplate::Moon:
				new_body_mass_ = 7.342e22f;
				new_body_radius_ = 1.7374e6f;
				new_body_j2_ = 2.0335e-4f;
				new_body_r_ref_ = 1.7374e6f;
				break;
			case BodyPresetTemplate::Jupiter:
				new_body_mass_ = 1.89813e27f;
				new_body_radius_ = 7.1492e7f;
				new_body_j2_ = 1.469657e-2f;
				new_body_j4_ = -5.86609e-4f;
				new_body_r_ref_ = 7.1492e7f;
				break;
			case BodyPresetTemplate::Mars:
				new_body_mass_ = 6.4171e23f;
				new_body_radius_ = 3.3895e6f;
				new_body_j2_ = 1.96045e-3f;
				new_body_r_ref_ = 3.3895e6f;
				break;
			case BodyPresetTemplate::NeutronStar:
				new_body_mass_ = 2.8e30f;
				new_body_radius_ = 12000.0f;
				new_body_j2_ = 0.0f;
				new_body_r_ref_ = 12000.0f;
				break;
			case BodyPresetTemplate::SupermassiveBlackHole:
				new_body_mass_ = 8.0e36f;
				new_body_radius_ = 1.2e10f;
				new_body_j2_ = 0.0f;
				new_body_r_ref_ = 1.2e10f;
				break;
			case BodyPresetTemplate::StellarBlackHole:
				new_body_mass_ = 2.0e31f;
				new_body_radius_ = 30000.0f;
				new_body_j2_ = 0.0f;
				new_body_r_ref_ = 30000.0f;
				break;
			case BodyPresetTemplate::TestParticle:
				new_body_mass_ = 0.0f;
				new_body_radius_ = 1.0f;
				new_body_j2_ = 0.0f;
				new_body_r_ref_ = 1.0f;
				break;
			case BodyPresetTemplate::Custom:
			default:
				new_body_mass_ = 1.0f;
				new_body_radius_ = 1.0f;
				new_body_j2_ = 0.0f;
				new_body_r_ref_ = 1.0f;
				break;
		}
	}

	void populate_solar_system_archetype() noexcept {
		auto& sys = orchestrator_.nbody_system();
		sys.clear_bodies();

		Dynamics::PostNewtonianBody sun(
			1, 1.0, 0.5,
			{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0},
			0.0, 2.2e-7, 0.0, 0.0, 0.5
		);

		Dynamics::PostNewtonianBody planet1(
			2, 1e-4, 0.05,
			{10.0, 0.0, 0.0}, {0.0, 0.3162, 0.0}, {0.0, 0.0, 0.0}
		);

		Dynamics::PostNewtonianBody planet2(
			3, 3e-4, 0.08,
			{25.0, 0.0, 0.0}, {0.0, 0.2, 0.0}, {0.0, 0.0, 0.0}
		);

		sys.add_body(sun);
		sys.add_body(planet1);
		sys.add_body(planet2);
		sys.update_accelerations();
		selected_body_index_ = 0;
	}
};

}

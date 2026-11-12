#pragma once

#include <imgui.h>
#include "relativistic/ui/body_manager/body_manager_context.hpp"
#include "relativistic/ui/numeric_slider_utils.hpp"
#include "relativistic/ui/tooltip_utils.hpp"
#include "relativistic/ui/panel_text_helpers.hpp"
#include "relativistic/ui/coordinate_aware_widgets.hpp"
#include "relativistic/units/unit_system.hpp"
#include "relativistic/units/unit_aware_widgets.hpp"
#include "relativistic/dynamics/bulk_body_actions.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <mutex>
#include <numbers>

namespace Relativistic::UI {

class SystemDynamicsTab {
private:
	float global_velocity_[3]{0.0f, 0.0f, 0.0f};
	float global_spin_[3]{0.0f, 0.0f, 0.0f};
	float grid_spacing_{1.0f};
	int bulk_parameter_index_{0};
	float bulk_parameter_value_{1.0f};
	bool bulk_parameter_value_log_mode_{false};

public:
	[[nodiscard]] int bulk_parameter_index() const noexcept {
		return bulk_parameter_index_;
	}

	void set_bulk_parameter_index(int index) noexcept {
		bulk_parameter_index_ = index;
	}

	void render(BodyManagerContext& ctx) noexcept {
		auto& orchestrator = ctx.orchestrator();
		auto& sys = orchestrator.nbody_system();
		std::lock_guard<std::recursive_mutex> system_dynamics_lock(sys.bodies_mutex());
		const size_t n = sys.body_count();

		if (n == 0) {
			ImGui::TextDisabled("System is empty.");
			return;
		}

		if (ImGui::Button("Recompute System State", ImVec2(-1.0f, 26.0f))) {
			sys.update_accelerations();
			orchestrator.notify_state_changed();
		}
		render_setting_tooltip("Forces an immediate recomputation of accelerations and gravitational-wave emission from the current body states.");
		if (ImGui::CollapsingHeader("Global Body Actions", ImGuiTreeNodeFlags_DefaultOpen)) {
			static_cast<void>(unit_aware_input_float3("Velocity For All", global_velocity_, UnitCategory::Velocity, orchestrator.unit_preferences()));
			if (ImGui::Button("Set Velocity For All")) {
				for (auto& body : sys.bodies()) if (body.enabled) body.velocity = std::array<double, 3>{global_velocity_[0], global_velocity_[1], global_velocity_[2]};
				sys.update_accelerations();
			}
			ImGui::SameLine();
			if (ImGui::Button("Invert All Velocities")) {
				for (auto& body : sys.bodies()) if (body.enabled) for (double& component : body.velocity) component = -component;
				sys.update_accelerations();
			}
			ImGui::InputFloat3("Spin For All", global_spin_);
			if (ImGui::Button("Set Spin For All")) {
				for (auto& body : sys.bodies()) if (body.enabled) body.spin = std::array<double, 3>{global_spin_[0], global_spin_[1], global_spin_[2]};
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
				const auto& p = orchestrator.parameters();
				const double limit = (p.render_distance_scale > 0.0) ? (p.render_distance_scale * std::max(p.mass, 1e-6)) : 1.0e7;
				Dynamics::BulkBodyActions::cull_outside_radius(sys, limit);
			}
			render_setting_tooltip("Removes every body whose distance from the origin exceeds the current render distance, approximating a view-frustum cull.");
			ImGui::SameLine();
			if (ImGui::Button("Cull Bodies Outside Current View")) {
				const auto& cam = orchestrator.camera();
				const double pitch_rad = cam.pitch * (std::numbers::pi / 180.0);
				const double yaw_rad = cam.yaw * (std::numbers::pi / 180.0);
				const std::array<double, 3> forward{std::cos(pitch_rad) * std::cos(yaw_rad), std::cos(pitch_rad) * std::sin(yaw_rad), std::sin(pitch_rad)};
				const double half_fov_rad = std::clamp(cam.fov_deg, 5.0, 175.0) * (std::numbers::pi / 360.0);
				const auto& p = orchestrator.parameters();
				const double max_distance = (p.render_distance_scale > 0.0) ? (p.render_distance_scale * std::max(p.mass, 1e-6)) : 1.0e7;
				Dynamics::BulkBodyActions::cull_outside_camera_frustum(sys, cam.position, forward, half_fov_rad, max_distance);
			}
			render_setting_tooltip("Removes every body that falls outside the primary camera's current field-of-view cone, using its live position, orientation, and FOV.");

			ImGui::Spacing();
			ImGui::TextColored(ImVec4(0.6f, 0.85f, 1.0f, 1.0f), "Generalized Parameter Actions");
			const char* bulk_param_names[] = {
				"Mass", "Radius", "Charge", "Magnetic Moment", "Rotation Speed", "Friction Coefficient",
				"Restitution", "Integrity", "Lifetime", "Temperature", "Heat Capacity",
				"Quadrupole Moment", "Zonal J2", "Zonal J3", "Zonal J4", "Multipole Reference Radius"
			};
			ImGui::SetNextItemWidth(240.0f);
			ImGui::Combo("Target Parameter", &bulk_parameter_index_, bulk_param_names, IM_ARRAYSIZE(bulk_param_names));
			const auto bulk_param = static_cast<Dynamics::BulkScalarParameter>(bulk_parameter_index_);
			slider_float_with_input("Value", &bulk_parameter_value_, -1.0e6f, 1.0e6f, "%.4e", &bulk_parameter_value_log_mode_, 1e-9f, 1e12f);
			if (ImGui::Button("Set For All Bodies")) {
				Dynamics::BulkBodyActions::set_parameter_for_all(sys, bulk_param, static_cast<double>(bulk_parameter_value_));
			}
			ImGui::SameLine();
			if (ImGui::Button("Equalize (Set To Average)")) {
				Dynamics::BulkBodyActions::equalize_parameter(sys, bulk_param);
			}
			ImGui::SameLine();
			if (ImGui::Button("Average (Same As Equalize)")) {
				Dynamics::BulkBodyActions::average_parameter(sys, bulk_param);
			}
			render_setting_tooltip("Applies to the selected physical parameter across every enabled body: sets an explicit value, or replaces every value with the current mean.");
		}

		ImGui::Separator();
		if (ImGui::CollapsingHeader("Spacetime Sources (Black Holes)", ImGuiTreeNodeFlags_DefaultOpen)) {
			size_t objects = 0;
			size_t enabled_objects = 0;
			size_t sources = 0;
			size_t enabled_sources = 0;
			ctx.count_bodies(objects, enabled_objects, sources, enabled_sources);
			ImGui::TextColored(ImVec4(0.95f, 0.5f, 0.5f, 1.0f), "Primary Metric Source");
			ImGui::Text("Mass=%.6e  Spin a=%.4f  Position=(0,0,0)", orchestrator.parameters().mass, orchestrator.parameters().spin);
			render_setting_tooltip("The single spacetime source that curves the rendered background metric and lensing. It is always fixed at the coordinate origin; use the Source Catalog or Spacetime & Metrics to change its mass and spin.");
			ImGui::Spacing();
			ImGui::TextColored(ImVec4(0.95f, 0.5f, 0.5f, 1.0f), "Independent N-Body Spacetime Sources: %zu (%zu enabled)", sources, enabled_sources);
			bool any_source = false;
			for (auto& body : sys.bodies()) {
				if (!body.is_spacetime_source) continue;
				any_source = true;
				ImGui::PushID(static_cast<int>(body.id));
				const double dist = BodyManagerContext::distance_from_center(body);
				const std::string label = BodyManagerContext::display_name(body) + " (M=" + std::to_string(body.mass).substr(0, 5) + ", a/M=" + std::to_string(body.kerr_spin_parameter()).substr(0, 5) + ", r_h=" + std::to_string(body.kerr_outer_horizon_radius()).substr(0, 5) + ", dist=" + std::to_string(dist).substr(0, 6) + ")";
				if (ImGui::Selectable(label.c_str(), false)) {
					ctx.select_and_open_source(static_cast<int>(body.id));
				}
				ImGui::SameLine();
				if (ImGui::SmallButton("Look At")) {
					ctx.look_at(body.position);
				}
				ImGui::PopID();
			}
			if (!any_source) {
				ImGui::TextDisabled("No independent spacetime sources yet. Create one from the Create Source tab or enable the Spacetime Source option on any body.");
			}
			render_setting_tooltip("Bodies flagged as spacetime sources are fully integrated N-body gravitating objects with their own Kerr event horizon. They attract and are attracted by every other body and each other, merge with each other on contact, and absorb ordinary bodies crossing their horizon.");
			ImGui::Spacing();
			if (ImGui::Button("Open Source Catalog", ImVec2(-1.0f, 26.0f))) {
				ctx.request_tab(BodyManagerTab::SourceCatalog);
			}
			render_setting_tooltip("Switches to the Source Catalog tab where every black hole is listed and edited.");
			if (ImGui::Button("Spawn A Second Black Hole In Orbit", ImVec2(-1.0f, 28.0f))) {
				ctx.spawn_companion_source();
			}
			render_setting_tooltip("Creates a new independent black hole roughly half the mass of the primary central object, placed on a wide circular orbit with a small random inclination, ready for binary or multi-black-hole dynamics.");
		}
		ImGui::Separator();

		ImGui::Text("Total System Mass:     %.6e", sys.total_mass());
		const auto cm = sys.center_of_mass();
		ImGui::TextUnformatted(("Center of Mass: " + describe_position(orchestrator.coordinate_preferences().widget_default, Observer::CoordinateVector{cm[0], cm[1], cm[2]}, make_coordinate_frame(orchestrator), ctx.length_scale(), orchestrator.unit_preferences(), 4)).c_str());
		render_setting_tooltip("Mass-weighted average position of all orbiting bodies.");

		const auto p_tot = sys.total_linear_momentum();
		ImGui::Text("Total Linear Momentum: (%.2e, %.2e, %.2e)", p_tot[0], p_tot[1], p_tot[2]);

		const auto l_tot = sys.total_angular_momentum();
		ImGui::Text("Total Angular Momentum:(%.2e, %.2e, %.2e)", l_tot[0], l_tot[1], l_tot[2]);
		render_setting_tooltip("Includes the 1PN spin-orbit correction when enabled and available for a two-body configuration.");

		ImGui::Text("Total Mechanical Energy: %.6e", sys.compute_total_energy());
		ImGui::TextDisabled("%s", Units::format_energy(sys.compute_total_energy(), orchestrator.unit_preferences().energy).c_str());

		const auto& gw = sys.latest_gw_emission();
		ImGui::Text("GW Radiated Power:       %.6e W", gw.radiated_power);
		render_setting_tooltip("Quadrupole-formula gravitational-wave luminosity computed from the current body configuration.");
	}
};

}

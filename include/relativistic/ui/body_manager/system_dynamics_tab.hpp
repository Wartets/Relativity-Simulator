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
	int bulk_scope_{0};
	int bulk_category_{0};

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
			const auto current_scope = static_cast<BulkActionScope>(bulk_scope_);
			const auto current_cat = static_cast<CatalogTypeCategory>(bulk_category_);

			size_t scoped_count = 0;
			for (const auto& b : sys.bodies()) {
				if (b.enabled && matches_bulk_scope(b, current_scope, current_cat)) ++scoped_count;
			}

			ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Target Action Scope:");
			ImGui::SetNextItemWidth(240.0f);
			ImGui::Combo("##BulkScope", &bulk_scope_, kBulkActionScopeNames.data(), static_cast<int>(kBulkActionScopeNames.size()));
			if (current_scope == BulkActionScope::ByTypeCategory) {
				ImGui::SameLine();
				ImGui::SetNextItemWidth(200.0f);
				ImGui::Combo("##BulkCat", &bulk_category_, kCatalogTypeCategoryNames.data(), static_cast<int>(kCatalogTypeCategoryNames.size()));
			}
			render_setting_tooltip("Restricts all global modifications below (velocities, spins, grid snapping, parameter overrides) to the chosen subset of bodies.");
			ImGui::TextDisabled("Scope matched: %zu active body(ies)", scoped_count);
			ImGui::Separator();

			static_cast<void>(unit_aware_input_float3("Velocity For Scope", global_velocity_, UnitCategory::Velocity, orchestrator.unit_preferences()));
			if (ImGui::Button("Set Velocity For Scoped Bodies")) {
				for (auto& body : sys.bodies()) {
					if (body.enabled && matches_bulk_scope(body, current_scope, current_cat)) {
						body.velocity = std::array<double, 3>{global_velocity_[0], global_velocity_[1], global_velocity_[2]};
					}
				}
				sys.update_accelerations();
			}
			ImGui::SameLine();
			if (ImGui::Button("Invert Scoped Velocities")) {
				for (auto& body : sys.bodies()) {
					if (body.enabled && matches_bulk_scope(body, current_scope, current_cat)) {
						for (double& component : body.velocity) component = -component;
					}
				}
				sys.update_accelerations();
			}
			ImGui::InputFloat3("Spin For Scope", global_spin_);
			if (ImGui::Button("Set Spin For Scoped Bodies")) {
				for (auto& body : sys.bodies()) {
					if (body.enabled && matches_bulk_scope(body, current_scope, current_cat)) {
						body.spin = std::array<double, 3>{global_spin_[0], global_spin_[1], global_spin_[2]};
					}
				}
				sys.update_accelerations();
			}
			ImGui::InputFloat("Grid Precision", &grid_spacing_, 0.1f, 1.0f, "%.4g");
			if (ImGui::Button("Snap Scoped Positions To Grid")) {
				const double spacing = static_cast<double>(grid_spacing_);
				if (spacing > 1e-9) {
					for (auto& body : sys.bodies()) {
						if (body.enabled && matches_bulk_scope(body, current_scope, current_cat)) {
							body.position[0] = std::round(body.position[0] / spacing) * spacing;
							body.position[1] = std::round(body.position[1] / spacing) * spacing;
							body.position[2] = std::round(body.position[2] / spacing) * spacing;
						}
					}
					sys.update_accelerations();
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Scatter Scoped Positions")) {
				for (auto& body : sys.bodies()) {
					if (body.enabled && matches_bulk_scope(body, current_scope, current_cat)) {
						const double r = std::max(body.radius, 1.0) * 0.25;
						body.position[0] += ctx.sample_signed_uniform(r * 0.1, r);
						body.position[1] += ctx.sample_signed_uniform(r * 0.1, r);
						body.position[2] += ctx.sample_signed_uniform(r * 0.1, r);
					}
				}
				sys.update_accelerations();
			}
			if (ImGui::Button("Equalize Scoped Masses")) {
				double sum_m = 0.0;
				size_t count_m = 0;
				for (const auto& body : sys.bodies()) {
					if (body.enabled && matches_bulk_scope(body, current_scope, current_cat)) {
						sum_m += body.mass;
						++count_m;
					}
				}
				if (count_m > 0) {
					const double avg_m = sum_m / static_cast<double>(count_m);
					for (auto& body : sys.bodies()) {
						if (body.enabled && matches_bulk_scope(body, current_scope, current_cat)) {
							body.mass = avg_m;
						}
					}
					sys.update_accelerations();
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Zero Scoped Spins")) {
				for (auto& body : sys.bodies()) {
					if (body.enabled && matches_bulk_scope(body, current_scope, current_cat)) {
						body.spin = {0.0, 0.0, 0.0};
					}
				}
				sys.update_accelerations();
			}
			if (ImGui::Button("Cull Scoped Bodies Outside Render Distance")) {
				const auto& p = orchestrator.parameters();
				const double limit = (p.render_distance_scale > 0.0) ? (p.render_distance_scale * std::max(p.mass, 1e-6)) : 1.0e7;
				std::vector<uint32_t> to_remove;
				for (const auto& body : sys.bodies()) {
					if (body.enabled && matches_bulk_scope(body, current_scope, current_cat)) {
						if (BodyManagerContext::distance_from_center(body) > limit) to_remove.push_back(body.id);
					}
				}
				for (uint32_t id : to_remove) ctx.remove_body_by_id(id);
			}
			render_setting_tooltip("Removes scoped bodies whose distance from the origin exceeds the current render distance.");
			ImGui::SameLine();
			if (ImGui::Button("Cull Scoped Bodies Outside Current View")) {
				const auto& cam = orchestrator.camera();
				const double pitch_rad = cam.pitch * (std::numbers::pi / 180.0);
				const double yaw_rad = cam.yaw * (std::numbers::pi / 180.0);
				const std::array<double, 3> forward{std::cos(pitch_rad) * std::cos(yaw_rad), std::cos(pitch_rad) * std::sin(yaw_rad), std::sin(pitch_rad)};
				const double half_fov_rad = std::clamp(cam.fov_deg, 5.0, 175.0) * (std::numbers::pi / 360.0);
				const auto& p = orchestrator.parameters();
				const double max_distance = (p.render_distance_scale > 0.0) ? (p.render_distance_scale * std::max(p.mass, 1e-6)) : 1.0e7;
				std::vector<uint32_t> to_remove;
				for (const auto& body : sys.bodies()) {
					if (!body.enabled || !matches_bulk_scope(body, current_scope, current_cat)) continue;
					const double dx = body.position[0] - cam.position[0];
					const double dy = body.position[1] - cam.position[1];
					const double dz = body.position[2] - cam.position[2];
					const double dist = std::sqrt(dx * dx + dy * dy + dz * dz);
					if (dist > max_distance) {
						to_remove.push_back(body.id);
						continue;
					}
					if (dist > 1e-9) {
						const double cos_angle = (dx * forward[0] + dy * forward[1] + dz * forward[2]) / dist;
						if (std::acos(std::clamp(cos_angle, -1.0, 1.0)) > half_fov_rad) to_remove.push_back(body.id);
					}
				}
				for (uint32_t id : to_remove) ctx.remove_body_by_id(id);
			}
			render_setting_tooltip("Removes scoped bodies that fall outside the primary camera's current field-of-view cone.");

			ImGui::Spacing();
			ImGui::TextColored(ImVec4(0.6f, 0.85f, 1.0f, 1.0f), "Generalized Parameter Actions (Scoped)");
			const char* bulk_param_names[] = {
				"Mass", "Radius", "Charge", "Magnetic Moment", "Rotation Speed", "Friction Coefficient",
				"Restitution", "Integrity", "Lifetime", "Temperature", "Heat Capacity",
				"Quadrupole Moment", "Zonal J2", "Zonal J3", "Zonal J4", "Multipole Reference Radius"
			};
			ImGui::SetNextItemWidth(240.0f);
			ImGui::Combo("Target Parameter", &bulk_parameter_index_, bulk_param_names, IM_ARRAYSIZE(bulk_param_names));
			const int param_index = bulk_parameter_index_;
			slider_float_with_input("Value", &bulk_parameter_value_, -1.0e6f, 1.0e6f, "%.4e", &bulk_parameter_value_log_mode_, 1e-9f, 1e12f);

			auto get_param = [](const Dynamics::PostNewtonianBody& b, int idx) noexcept -> double {
				switch (idx) {
					case 0: return b.mass;
					case 1: return b.radius;
					case 2: return b.charge;
					case 3: return b.magnetic_moment;
					case 4: return b.rotation_speed;
					case 5: return b.friction_coefficient;
					case 6: return b.restitution;
					case 7: return b.integrity;
					case 8: return b.lifetime;
					case 9: return b.temperature;
					case 10: return b.heat_capacity;
					case 11: return b.quadrupole_moment;
					case 12: return b.j2;
					case 13: return b.j3;
					case 14: return b.j4;
					case 15: return b.reference_radius;
					default: return 0.0;
				}
			};
			auto set_param = [](Dynamics::PostNewtonianBody& b, int idx, double val) noexcept {
				switch (idx) {
					case 0: b.mass = std::max(val, 0.0); break;
					case 1: b.radius = std::max(val, 1e-6); break;
					case 2: b.charge = val; break;
					case 3: b.magnetic_moment = std::max(val, 0.0); break;
					case 4: b.rotation_speed = val; break;
					case 5: b.friction_coefficient = std::clamp(val, 0.0, 1.0); break;
					case 6: b.restitution = std::clamp(val, 0.0, 1.0); break;
					case 7: b.integrity = std::max(val, 0.0); break;
					case 8: b.lifetime = std::max(val, 0.0); break;
					case 9: b.temperature = std::max(val, 0.0); break;
					case 10: b.heat_capacity = std::max(val, 1e-6); break;
					case 11: b.quadrupole_moment = val; break;
					case 12: b.j2 = val; break;
					case 13: b.j3 = val; break;
					case 14: b.j4 = val; break;
					case 15: b.reference_radius = std::max(val, 1e-6); break;
					default: break;
				}
			};

			if (ImGui::Button("Set For Scoped Bodies")) {
				for (auto& body : sys.bodies()) {
					if (body.enabled && matches_bulk_scope(body, current_scope, current_cat)) {
						set_param(body, param_index, static_cast<double>(bulk_parameter_value_));
					}
				}
				sys.update_accelerations();
			}
			ImGui::SameLine();
			if (ImGui::Button("Equalize Scoped (Set To Average)")) {
				double sum_p = 0.0;
				size_t count_p = 0;
				for (const auto& body : sys.bodies()) {
					if (body.enabled && matches_bulk_scope(body, current_scope, current_cat)) {
						sum_p += get_param(body, param_index);
						++count_p;
					}
				}
				if (count_p > 0) {
					const double avg_p = sum_p / static_cast<double>(count_p);
					for (auto& body : sys.bodies()) {
						if (body.enabled && matches_bulk_scope(body, current_scope, current_cat)) {
							set_param(body, param_index, avg_p);
						}
					}
					sys.update_accelerations();
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Average (Same As Equalize)")) {
				double sum_p = 0.0;
				size_t count_p = 0;
				for (const auto& body : sys.bodies()) {
					if (body.enabled && matches_bulk_scope(body, current_scope, current_cat)) {
						sum_p += get_param(body, param_index);
						++count_p;
					}
				}
				if (count_p > 0) {
					const double avg_p = sum_p / static_cast<double>(count_p);
					for (auto& body : sys.bodies()) {
						if (body.enabled && matches_bulk_scope(body, current_scope, current_cat)) {
							set_param(body, param_index, avg_p);
						}
					}
					sys.update_accelerations();
				}
			}
			render_setting_tooltip("Applies to the selected physical parameter across every scoped enabled body.");
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
		const double p_norm = std::sqrt(p_tot[0] * p_tot[0] + p_tot[1] * p_tot[1] + p_tot[2] * p_tot[2]);
		ImGui::Text("Total Linear Momentum: (%.2e, %.2e, %.2e) | Norm: %.4e", p_tot[0], p_tot[1], p_tot[2], p_norm);
		if (p_norm > 1e-6) {
			ImGui::SameLine();
			if (ImGui::Button("Shift to Center-of-Mass Frame")) {
				ctx.shift_to_center_of_mass_frame(static_cast<BulkActionScope>(bulk_scope_), static_cast<CatalogTypeCategory>(bulk_category_));
			}
			render_setting_tooltip("Subtracts the barycentric drift velocity v_cm = P_tot / M_tot from all scoped bodies so the system's center of mass stays stationary in space.");
		}

		const auto l_tot = sys.total_angular_momentum();
		ImGui::Text("Total Angular Momentum:(%.2e, %.2e, %.2e)", l_tot[0], l_tot[1], l_tot[2]);
		render_setting_tooltip("Includes the 1PN spin-orbit correction when enabled and available for a two-body configuration.");

		ImGui::Text("Total Mechanical Energy: %.6e", sys.compute_total_energy());
		ImGui::TextDisabled("%s", Units::format_energy(sys.compute_total_energy(), orchestrator.unit_preferences().energy).c_str());

		const auto& gw = sys.latest_gw_emission();
		ImGui::Text("GW Radiated Power:       %.6e W", gw.radiated_power);
		render_setting_tooltip("Quadrupole-formula gravitational-wave luminosity computed from the current body configuration.");

		double dominant_f_orb = 0.0;
		const double central_m = std::max(orchestrator.parameters().mass, 1e-12);
		for (const auto& b : sys.bodies()) {
			if (!b.enabled) continue;
			const auto el = ctx.compute_keplerian_elements(b.position, b.velocity, central_m + (b.is_spacetime_source ? b.mass : 0.0));
			if (el.valid && el.is_bound && el.period_seconds > 1e-6) {
				const double f = 1.0 / el.period_seconds;
				if (f > dominant_f_orb) dominant_f_orb = f;
			}
		}

		const double f_gw = (dominant_f_orb > 1e-12) ? (2.0 * dominant_f_orb) : 0.0;
		if (f_gw > 0.0) {
			ImGui::Text("GW Dominant Frequency:   %.4e Hz (f_gw = 2 * f_orb)", f_gw);
			render_setting_tooltip("Dominant quadrupole gravitational radiation frequency, equal to twice the highest orbital frequency of the bound system.");

			const double j_dot_gw = gw.radiated_power / (std::numbers::pi * f_gw);
			ImGui::Text("GW Angular Momentum Loss:%.4e N*m (dJ/dt)", j_dot_gw);
			render_setting_tooltip("Rate of orbital angular momentum loss carried away by gravitational waves in the quadrupole approximation.");

			constexpr double dist_10kpc_meters = 3.08567758e20;
			constexpr double g_const_si = 6.6743e-11;
			constexpr double c_const_si = 299792458.0;
			const double h_strain = (1.0 / (std::numbers::pi * f_gw * dist_10kpc_meters)) * std::sqrt(std::max(g_const_si * gw.radiated_power / (c_const_si * c_const_si * c_const_si), 0.0));
			ImGui::Text("GW Estimated Strain h:   %.4e (at 10 kpc)", h_strain);
			render_setting_tooltip("Estimated dimensionless gravitational wave strain amplitude h at a standard astronomical distance of 10 kpc.");
		} else {
			ImGui::TextDisabled("GW Frequency & Strain:   No bound periodic orbit detected in system");
		}
	}
};

}

#pragma once

#include <imgui.h>
#include "relativistic/ui/body_manager/body_manager_context.hpp"
#include "relativistic/ui/spacetime_source_panel.hpp"
#include "relativistic/ui/panel_text_helpers.hpp"
#include "relativistic/ui/tooltip_utils.hpp"
#include "relativistic/ui/coordinate_aware_widgets.hpp"
#include "relativistic/ui/accretion_disk_editor.hpp"
#include "relativistic/units/unit_system.hpp"
#include "relativistic/units/unit_aware_widgets.hpp"
#include "relativistic/optics/disk_thermal_profile.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <string>

namespace Relativistic::UI {

[[nodiscard]] inline bool render_source_parameters_section(BodyManagerContext& ctx, Dynamics::PostNewtonianBody& b, BodyEditorViewState& view) noexcept {
	bool changed = false;
	auto& orchestrator = ctx.orchestrator();
	const auto& prefs = orchestrator.unit_preferences();
	const double length_scale_m = ctx.length_scale();
	const double speed_scale_mps = ctx.speed_scale();
	const double mass_scale_kg = ctx.mass_scale();
	static const Metrics::HorizonConstraintSet kerr_constraints = Metrics::HorizonConstraintSet::from_metric_name("Kerr");

	render_hint_text("Mass, spin and charge define the Kerr horizon of this black hole. The dimensionless spin a/M is preserved when the mass changes, and the spin axis below orients the horizon and its accretion disk.");

	const double previous_ratio = b.kerr_spin_parameter();
	SpacetimeSourceValues values{b.mass, previous_ratio * b.mass, b.charge, 0.0};
	SpacetimeSourcePanelOptions options;
	options.constraints = &kerr_constraints;
	options.guarantee_horizon = &view.guarantee_horizon;
	options.render_lock_controls = true;
	options.constrain_mass = false;
	options.spin_non_negative = true;
	options.mass_label = "Mass (M)";
	options.mass_slider_minimum = 0.001;
	options.mass_slider_maximum = 1.0e6;
	options.mass_log_minimum = 1e-12;
	options.mass_log_maximum = 1e60;
	options.charge_limit = std::max(b.mass, 1e-6);
	const SpacetimeSourcePanelResult panel = render_spacetime_source_panel(orchestrator, view.source_parameters, values, options);
	if (panel.mass_changed || panel.spin_changed || panel.charge_changed) {
		const std::array<double, 3> preserved_axis = b.spin_axis_unit();
		b.mass = std::max(1e-9, values.mass);
		const double ratio = panel.spin_changed ? (values.spin / b.mass) : previous_ratio;
		b.set_spin_state(std::clamp(ratio, 0.0, 0.999), preserved_axis);
		b.charge = std::clamp(values.charge, -b.mass, b.mass);
		changed = true;
	}

	float spin_ratio = static_cast<float>(b.kerr_spin_parameter());
	const float max_ratio = static_cast<float>(std::sqrt(std::max(1.0 - (b.charge * b.charge) / (b.mass * b.mass), 0.0)));
	const float allowed_max_ratio = view.guarantee_horizon ? std::min(max_ratio, 0.999f) : 0.999f;
	if (slider_float_with_input("Dimensionless Spin (a/M)", &spin_ratio, 0.0f, allowed_max_ratio, "%.4f")) {
		b.set_spin_state(std::clamp(static_cast<double>(spin_ratio), 0.0, static_cast<double>(allowed_max_ratio)), b.spin_axis_unit());
		changed = true;
	}
	render_setting_tooltip("Kerr spin parameter a/M in [0, 0.999]. Directly adjusts the rotation rate relative to the mass limit. It flattens the horizon, shrinks the ISCO and drives frame dragging and spin-orbit coupling.");

	render_group_title("Spin Axis", ImVec4(0.8f, 0.6f, 1.0f, 1.0f));
	const std::array<double, 3> axis = b.spin_axis_unit();
	constexpr double radians_to_degrees = 180.0 / std::numbers::pi;
	float polar_degrees = static_cast<float>(std::acos(std::clamp(axis[2], -1.0, 1.0)) * radians_to_degrees);
	float azimuth_degrees = static_cast<float>(std::atan2(axis[1], axis[0]) * radians_to_degrees);
	bool orientation_changed = ImGui::SliderFloat("Spin Axis Polar Angle", &polar_degrees, 0.0f, 180.0f, "%.1f deg");
	render_setting_tooltip("Angle between the black hole rotation axis and the world +Z axis. It orients the flattened horizon, the accretion disk plane and the frame dragging direction.");
	orientation_changed = ImGui::SliderFloat("Spin Axis Azimuth", &azimuth_degrees, -180.0f, 180.0f, "%.1f deg") || orientation_changed;
	render_setting_tooltip("Azimuth of the rotation axis around the world Z axis.");
	if (orientation_changed) {
		const double polar = static_cast<double>(polar_degrees) / radians_to_degrees;
		const double azimuth = static_cast<double>(azimuth_degrees) / radians_to_degrees;
		b.set_spin_state(b.kerr_spin_parameter(), {std::sin(polar) * std::cos(azimuth), std::sin(polar) * std::sin(azimuth), std::cos(polar)});
		changed = true;
	}
	if (ImGui::SmallButton("Align With Orbital Angular Momentum")) {
		const std::array<double, 3> angular_momentum{
			b.position[1] * b.velocity[2] - b.position[2] * b.velocity[1],
			b.position[2] * b.velocity[0] - b.position[0] * b.velocity[2],
			b.position[0] * b.velocity[1] - b.position[1] * b.velocity[0]
		};
		if (std::sqrt(angular_momentum[0] * angular_momentum[0] + angular_momentum[1] * angular_momentum[1] + angular_momentum[2] * angular_momentum[2]) > 1e-12) {
			b.set_spin_state(b.kerr_spin_parameter(), angular_momentum);
			changed = true;
		}
	}
	render_setting_tooltip("Points the spin axis along the orbital angular momentum of this black hole around the origin, producing an aligned (prograde) configuration.");
	ImGui::SameLine();
	if (ImGui::SmallButton("Align With Primary Spin")) {
		b.set_spin_state(b.kerr_spin_parameter(), {0.0, 0.0, (orchestrator.parameters().spin >= 0.0) ? 1.0 : -1.0});
		changed = true;
	}
	render_setting_tooltip("Points the spin axis along the rotation axis of the primary central source.");
	ImGui::SameLine();
	if (ImGui::SmallButton("Reset Axis To +Z")) {
		b.set_spin_state(b.kerr_spin_parameter(), {0.0, 0.0, 1.0});
		changed = true;
	}

	render_group_title("Orbital State", ImVec4(0.5f, 0.9f, 0.85f, 1.0f));
	if (coordinate_aware_input_position("Position", b.position, orchestrator)) {
		changed = true;
	}
	render_setting_tooltip("Position of the black hole relative to the primary source in the selected coordinate system. The system button switches between Cartesian, spherical, cylindrical and metric-adapted coordinates.");

	double velocity_mps[3] = {b.velocity[0] * speed_scale_mps, b.velocity[1] * speed_scale_mps, b.velocity[2] * speed_scale_mps};
	if (unit_aware_input_double3("Velocity (vx, vy, vz)", velocity_mps, UnitCategory::Velocity, prefs)) {
		b.velocity = {velocity_mps[0] / speed_scale_mps, velocity_mps[1] / speed_scale_mps, velocity_mps[2] / speed_scale_mps};
		changed = true;
	}
	render_setting_tooltip(("Coordinate velocity of the black hole, displayed in " + std::string(Units::velocity_unit_suffix(prefs.velocity)) + ".").c_str());

	if (ImGui::Button("Set Circular Orbit Velocity", ImVec2(-1.0f, 24.0f))) {
		b.velocity = ctx.compute_circular_orbit_velocity(b.position, orchestrator.parameters().mass + b.mass);
		changed = true;
	}
	render_setting_tooltip("Overwrites the velocity with the Keplerian circular-orbit velocity around the primary source, including the mass of this black hole.");

	if (ImGui::Button("Move To Safe Separation", ImVec2(-1.0f, 24.0f))) {
		ctx.resolve_source_placement(b);
		changed = true;
	}
	render_setting_tooltip("Pushes the black hole outward until its horizon no longer overlaps the primary source or any other independent black hole, keeping a 25% margin.");

	const std::string overlap_warning = ctx.source_overlap_warning(b);
	if (!overlap_warning.empty()) {
		render_caution_text(overlap_warning.c_str());
	}

	render_group_title("Derived Properties", ImVec4(1.0f, 0.75f, 0.35f, 1.0f));
	const double horizon = b.kerr_outer_horizon_radius();
	const double spin_length = b.kerr_spin_parameter() * b.mass;
	const double equatorial_extent = std::sqrt(horizon * horizon + spin_length * spin_length);
	const double isco = Optics::DiskThermalProfile::kerr_isco_radius(b.mass, spin_length);
	const double kerr_disc = b.mass * b.mass - spin_length * spin_length - b.charge * b.charge;
	const double cauchy_horizon = (kerr_disc >= 0.0) ? (b.mass - std::sqrt(kerr_disc)) : 0.0;
	const double a_over_m = std::clamp(b.kerr_spin_parameter(), 0.0, 0.999);
	const double photon_sphere = 2.0 * b.mass * (1.0 + std::cos(2.0 / 3.0 * std::acos(-a_over_m)));
	ImGui::TextDisabled("Outer Event Horizon (r+): %s  (%.4f M)", Units::format_distance(horizon * length_scale_m, prefs.distance).c_str(), horizon / b.mass);
	if (cauchy_horizon > 0.0) {
		ImGui::TextDisabled("Inner Cauchy Horizon (r-): %s  (%.4f M)", Units::format_distance(cauchy_horizon * length_scale_m, prefs.distance).c_str(), cauchy_horizon / b.mass);
	}
	ImGui::TextDisabled("Equatorial Horizon Extent: %s  (%.4f M)", Units::format_distance(equatorial_extent * length_scale_m, prefs.distance).c_str(), equatorial_extent / b.mass);
	ImGui::TextDisabled("Equatorial Ergosphere Boundary: %s  (%.4f M)", Units::format_distance(2.0 * b.mass * length_scale_m, prefs.distance).c_str(), 2.0);
	ImGui::TextDisabled("Photon Sphere Orbit (r_ph): %s  (%.4f M)", Units::format_distance(photon_sphere * length_scale_m, prefs.distance).c_str(), photon_sphere / b.mass);
	ImGui::TextDisabled("Innermost Stable Orbit (ISCO): %s  (%.4f M)", Units::format_distance(isco * length_scale_m, prefs.distance).c_str(), isco / b.mass);
	const double kinetic_energy_joules = b.kinetic_energy() * mass_scale_kg * speed_scale_mps * speed_scale_mps;
	ImGui::TextDisabled("Speed: %s", Units::format_velocity(b.speed() * speed_scale_mps, prefs.velocity).c_str());
	ImGui::TextDisabled("Kinetic Energy: %s", Units::format_energy(kinetic_energy_joules, prefs.energy).c_str());

	if (changed) {
		b.enforce_spacetime_source_invariants();
	}
	return changed;
}

[[nodiscard]] inline bool render_source_disk_section(Dynamics::PostNewtonianBody& b) noexcept {
	render_hint_text("Thermal profile, geometry and structure of the accretion disk orbiting this black hole. The editor is the same one used by the primary source in the Spacetime & Metrics tab.");
	const double horizon = b.kerr_outer_horizon_radius();
	const double spin_length = b.kerr_spin_parameter() * b.mass;
	const double isco = Optics::DiskThermalProfile::kerr_isco_radius(b.mass, spin_length);
	ImGui::TextDisabled("Outer Horizon: %.4f M | ISCO: %.4f M", horizon / std::max(b.mass, 1e-9), isco / std::max(b.mass, 1e-9));
	return render_accretion_disk_editor(b.accretion_disk, true);
}

}

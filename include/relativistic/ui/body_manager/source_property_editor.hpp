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
		if (view.lock_disk_inner_to_isco) {
			b.accretion_disk.inner_radius_scale = 1.0f;
		}
		changed = true;
	}

	float spin_ratio = static_cast<float>(b.kerr_spin_parameter());
	const float max_ratio = static_cast<float>(std::sqrt(std::max(1.0 - (b.charge * b.charge) / (b.mass * b.mass), 0.0)));
	const float allowed_max_ratio = view.guarantee_horizon ? std::min(max_ratio, 0.999f) : 0.999f;
	if (slider_float_with_input("Dimensionless Spin (a/M)", &spin_ratio, 0.0f, allowed_max_ratio, "%.4f")) {
		b.set_spin_state(std::clamp(static_cast<double>(spin_ratio), 0.0, static_cast<double>(allowed_max_ratio)), b.spin_axis_unit());
		if (view.lock_disk_inner_to_isco) {
			b.accretion_disk.inner_radius_scale = 1.0f;
		}
		changed = true;
	}
	render_setting_tooltip("Kerr spin parameter a/M in [0, 0.999]. Directly adjusts the rotation rate relative to the mass limit. It flattens the horizon, shrinks the ISCO and drives frame dragging and spin-orbit coupling.");

	const double a_ratio = b.kerr_spin_parameter();
	const double q_ratio = std::abs(b.charge) / std::max(b.mass, 1e-9);
	const double chi_extremal = std::sqrt(a_ratio * a_ratio + q_ratio * q_ratio);
	const ImVec4 chi_color = (chi_extremal > 1.0) ? ImVec4(1.0f, 0.35f, 0.35f, 1.0f)
		: ((chi_extremal >= 0.95) ? ImVec4(1.0f, 0.8f, 0.25f, 1.0f) : ImVec4(0.35f, 0.9f, 0.45f, 1.0f));
	ImGui::TextColored(chi_color, "Extremal Parameter (a^2+Q^2)^1/2 / M = %.4f (%s)", chi_extremal, (chi_extremal > 1.0) ? "Superextremal / Naked Singularity" : ((chi_extremal >= 0.95) ? "Near-Extremal Kerr-Newman" : "Standard Subextremal Horizon"));

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

	const double effective_central_mass = orchestrator.parameters().mass + b.mass;
	const auto hazard = ctx.evaluate_trajectory_hazard(b.position, b.velocity, b.mass, b.radius, effective_central_mass);
	if (hazard.plunge_hazard) {
		render_caution_text("Plunge Trajectory: Periapsis is inside the central event horizon. This black hole will plunge and coalesce into the primary source.");
	} else if (hazard.roche_hazard) {
		render_caution_text("Tidal Disruption Hazard: Periapsis is inside the mutual Roche limit of the system.");
	}

	const auto orbit = ctx.compute_keplerian_elements(b.position, b.velocity, effective_central_mass);
	if (orbit.valid) {
		ImGui::Spacing();
		ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Osculating Orbital Elements:");
		ImGui::SameLine();
		const ImVec4 badge_col = orbit.is_bound ? ImVec4(0.2f, 0.85f, 0.4f, 1.0f) : ImVec4(1.0f, 0.45f, 0.35f, 1.0f);
		ImGui::TextColored(badge_col, "[%s | %s]", orbit.type_name, orbit.is_prograde ? "Prograde" : "Retrograde");

		if (orbit.is_bound) {
			ImGui::TextDisabled("Semi-Major Axis (a): %s  (%.4f M)", Units::format_distance(orbit.semi_major_axis * length_scale_m, prefs.distance).c_str(), orbit.semi_major_axis / b.mass);
			ImGui::TextDisabled("Eccentricity (e): %.5f | Inclination (i): %.2f deg", orbit.eccentricity, orbit.inclination_deg);
			ImGui::TextDisabled("Periapsis (r_p): %s | Apoapsis (r_a): %s", Units::format_distance(orbit.periapsis * length_scale_m, prefs.distance).c_str(), Units::format_distance(orbit.apoapsis * length_scale_m, prefs.distance).c_str());
			ImGui::TextDisabled("Orbital Period (T): %s", Units::format_time(orbit.period_seconds, prefs.time).c_str());
		} else {
			ImGui::TextDisabled("Eccentricity (e): %.5f (Unbound) | Inclination (i): %.2f deg", orbit.eccentricity, orbit.inclination_deg);
			ImGui::TextDisabled("Periapsis (r_p): %s", Units::format_distance(orbit.periapsis * length_scale_m, prefs.distance).c_str());
		}
	}

	ImGui::Spacing();
	if (ImGui::Button("Circular Prograde Orbit", ImVec2(175.0f, 24.0f))) {
		b.velocity = ctx.compute_circular_orbit_velocity(b.position, effective_central_mass);
		changed = true;
	}
	render_setting_tooltip("Overwrites coordinate velocity with the circular prograde Keplerian orbital velocity.");
	ImGui::SameLine();
	if (ImGui::Button("Circular Retrograde Orbit", ImVec2(185.0f, 24.0f))) {
		b.velocity = ctx.compute_retrograde_circular_orbit_velocity(b.position, effective_central_mass);
		changed = true;
	}
	render_setting_tooltip("Overwrites coordinate velocity with the circular retrograde Keplerian orbital velocity.");
	ImGui::SameLine();
	if (ImGui::Button("Escape Velocity", ImVec2(130.0f, 24.0f))) {
		b.velocity = ctx.compute_escape_velocity(b.position, b.velocity, effective_central_mass);
		changed = true;
	}
	render_setting_tooltip("Scales orbital speed to exact local parabolic escape velocity.");

	float target_ecc = view.target_eccentricity;
	if (ImGui::SliderFloat("Target Eccentricity", &target_ecc, 0.0f, 2.0f, "%.3f")) {
		view.target_eccentricity = target_ecc;
	}
	render_setting_tooltip("Select a target eccentricity to re-scale the tangential velocity while keeping the current distance as periapsis.");
	ImGui::SameLine();
	if (ImGui::Button("Apply Eccentricity", ImVec2(140.0f, 24.0f))) {
		b.velocity = ctx.compute_target_eccentricity_velocity(b.position, b.velocity, effective_central_mass, static_cast<double>(view.target_eccentricity));
		changed = true;
	}

	if (ImGui::Button("Move To Safe Separation", ImVec2(-1.0f, 24.0f))) {
		ctx.resolve_source_placement(b);
		changed = true;
	}
	render_setting_tooltip("Pushes the black hole outward until its horizon no longer overlaps the primary source or any other independent black hole, keeping a 25% margin.");

	const std::string overlap_warning = ctx.source_overlap_warning(b);
	if (!overlap_warning.empty()) {
		render_caution_text(overlap_warning.c_str());
	}

	render_group_title("Binary Configuration (Primary <-> This Source)", ImVec4(1.0f, 0.6f, 0.2f, 1.0f));
	const auto binary = ctx.compute_binary_configuration(b);
	if (binary.valid) {
		ImGui::TextDisabled("Mass Ratio q = M2/M1: %.5f | Symmetric Ratio eta: %.5f", binary.mass_ratio, binary.symmetric_mass_ratio);
		ImGui::TextDisabled("Orbital Separation: %s  (%.2f r_g,1)", Units::format_distance(binary.separation_distance * length_scale_m, prefs.distance).c_str(), binary.separation_rg);
		ImGui::TextDisabled("Spin-Orbit Misalignment (theta_LS): %.2f deg", binary.spin_orbit_misalignment_deg);
		ImGui::TextDisabled("Mutual Spin Tilt (theta_S1,S2): %.2f deg", binary.mutual_spin_angle_deg);
		ImGui::TextDisabled("GW Coalescence Time (Peters 1964): %s", Units::format_time(binary.peters_coalescence_time_seconds, prefs.time).c_str());
	} else {
		ImGui::TextDisabled("Binary configuration metrics unavailable.");
	}

	render_group_title("Derived Properties", ImVec4(1.0f, 0.75f, 0.35f, 1.0f));
	const double horizon = b.kerr_outer_horizon_radius();
	const double spin_length = b.kerr_spin_parameter() * b.mass;
	const double equatorial_extent = std::sqrt(horizon * horizon + spin_length * spin_length);
	const double isco_pro = Optics::DiskThermalProfile::kerr_isco_radius(b.mass, spin_length);
	const double isco_retro = Optics::DiskThermalProfile::kerr_isco_radius(b.mass, -spin_length);
	const double kerr_disc = b.mass * b.mass - spin_length * spin_length - b.charge * b.charge;
	const double cauchy_horizon = (kerr_disc >= 0.0) ? (b.mass - std::sqrt(kerr_disc)) : 0.0;
	const double a_over_m = std::clamp(b.kerr_spin_parameter(), 0.0, 0.999);
	const double photon_sphere_pro = 2.0 * b.mass * (1.0 + std::cos(2.0 / 3.0 * std::acos(-a_over_m)));
	const double photon_sphere_retro = 2.0 * b.mass * (1.0 + std::cos(2.0 / 3.0 * std::acos(a_over_m)));
	ImGui::TextDisabled("Outer Event Horizon (r+): %s  (%.4f M)", Units::format_distance(horizon * length_scale_m, prefs.distance).c_str(), horizon / b.mass);
	if (cauchy_horizon > 0.0) {
		ImGui::TextDisabled("Inner Cauchy Horizon (r-): %s  (%.4f M)", Units::format_distance(cauchy_horizon * length_scale_m, prefs.distance).c_str(), cauchy_horizon / b.mass);
	}
	ImGui::TextDisabled("Equatorial Horizon Extent: %s  (%.4f M)", Units::format_distance(equatorial_extent * length_scale_m, prefs.distance).c_str(), equatorial_extent / b.mass);
	ImGui::TextDisabled("Equatorial Ergosphere Boundary: %s  (%.4f M)", Units::format_distance(2.0 * b.mass * length_scale_m, prefs.distance).c_str(), 2.0);
	ImGui::TextDisabled("Photon Sphere (Prograde): %s  (%.4f M)", Units::format_distance(photon_sphere_pro * length_scale_m, prefs.distance).c_str(), photon_sphere_pro / b.mass);
	ImGui::TextDisabled("Photon Sphere (Retrograde): %s  (%.4f M)", Units::format_distance(photon_sphere_retro * length_scale_m, prefs.distance).c_str(), photon_sphere_retro / b.mass);
	ImGui::TextDisabled("Innermost Stable Orbit (ISCO, Prograde): %s  (%.4f M)", Units::format_distance(isco_pro * length_scale_m, prefs.distance).c_str(), isco_pro / b.mass);
	ImGui::TextDisabled("Innermost Stable Orbit (ISCO, Retrograde): %s  (%.4f M)", Units::format_distance(isco_retro * length_scale_m, prefs.distance).c_str(), isco_retro / b.mass);
	const double kinetic_energy_joules = b.kinetic_energy() * mass_scale_kg * speed_scale_mps * speed_scale_mps;
	ImGui::TextDisabled("Speed: %s", Units::format_velocity(b.speed() * speed_scale_mps, prefs.velocity).c_str());
	ImGui::TextDisabled("Kinetic Energy: %s", Units::format_energy(kinetic_energy_joules, prefs.energy).c_str());

	if (changed) {
		b.enforce_spacetime_source_invariants();
	}
	return changed;
}

[[nodiscard]] inline bool render_source_disk_section(Dynamics::PostNewtonianBody& b, BodyEditorViewState& view) noexcept {
	bool changed = false;
	ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Disk Model: Relativistic Thin Accretion Flow (g^4 Doppler Beaming)");
	render_hint_text("Independent black holes evaluate a multi-pass relativistic thin disk with local Keplerian orbital frames and exact g^4 Doppler beaming. Volumetric hydrodynamic tori (Fishbone-Moncrief) and Blandford-Znajek magnetosphere jets require global coordinate grids that are currently tied to the background manifold of the central source.");

	const double horizon = b.kerr_outer_horizon_radius();
	const double spin_length = b.kerr_spin_parameter() * b.mass;
	const double isco_pro = Optics::DiskThermalProfile::kerr_isco_radius(b.mass, spin_length);
	const double isco_retro = Optics::DiskThermalProfile::kerr_isco_radius(b.mass, -spin_length);
	ImGui::TextDisabled("Outer Horizon: %.4f M | Prograde ISCO: %.4f M | Retrograde ISCO: %.4f M", horizon / std::max(b.mass, 1e-9), isco_pro / std::max(b.mass, 1e-9), isco_retro / std::max(b.mass, 1e-9));

	if (ImGui::Checkbox("Lock Inner Edge To ISCO (r_in = 1.0 x ISCO)", &view.lock_disk_inner_to_isco)) {
		if (view.lock_disk_inner_to_isco) {
			b.accretion_disk.inner_radius_scale = 1.0f;
		}
		changed = true;
	}
	render_setting_tooltip("Automatically locks the emitting inner edge of the accretion disk to the innermost stable circular orbit of this black hole. When the mass or spin parameter changes, the inner edge adjusts dynamically.");

	if (view.lock_disk_inner_to_isco) {
		b.accretion_disk.inner_radius_scale = 1.0f;
	}

	changed = render_accretion_disk_editor(b.accretion_disk, true) || changed;

	if (view.lock_disk_inner_to_isco) {
		b.accretion_disk.inner_radius_scale = 1.0f;
	}
	return changed;
}

}

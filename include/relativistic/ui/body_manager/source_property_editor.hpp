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

[[nodiscard]] inline bool render_relative_reference_and_orbital_plane_tool(
	BodyManagerContext& ctx,
	Dynamics::PostNewtonianBody& b,
	BodyEditorViewState& view
) noexcept {
	bool changed = false;
	auto& orchestrator = ctx.orchestrator();
	auto& sys = orchestrator.nbody_system();

	render_group_title("Orbital Plane & Relative Reference Alignment Tool", ImVec4(0.4f, 0.85f, 1.0f, 1.0f));
	render_hint_text("Designate a reference body (central origin or any orbiting body) to express positions, velocities, accelerations and orbital elements relative to it. Tilt and orient the orbital plane with exact critical angle diagnostics.");

	std::vector<int> ref_ids{BodyManagerContext::kCentralSourceId};
	std::vector<std::string> ref_labels{"Central Spacetime Attractor (Origin #0)"};

	{
		for (const auto& other : sys.bodies()) {
			if (!other.enabled || other.id == b.id) continue;
			ref_ids.push_back(static_cast<int>(other.id));
			ref_labels.push_back(BodyManagerContext::display_name(other) + " (#" + std::to_string(other.id) + ")");
		}
	}

	int current_ref_idx = 0;
	for (size_t i = 0; i < ref_ids.size(); ++i) {
		if (ref_ids[i] == view.reference_frame_body_id) {
			current_ref_idx = static_cast<int>(i);
			break;
		}
	}

	std::vector<const char*> ref_ptrs;
	ref_ptrs.reserve(ref_labels.size());
	for (const auto& s : ref_labels) ref_ptrs.push_back(s.c_str());

	if (ImGui::Combo("Reference Body / Origin", &current_ref_idx, ref_ptrs.data(), static_cast<int>(ref_ptrs.size()))) {
		view.reference_frame_body_id = ref_ids[static_cast<size_t>(current_ref_idx)];
		view.plane_angles_initialized = false;
	}
	render_setting_tooltip("Designate which celestial body serves as the relative origin and provides the reference equatorial/orbital plane.");

	if (ImGui::SmallButton("Auto-Select Dominant Parent")) {
		view.reference_frame_body_id = ctx.find_dominant_parent_id(b);
		view.plane_angles_initialized = false;
	}
	render_setting_tooltip("Detects and selects the dominant gravitational parent attractor from Hill sphere and tidal dominance analysis.");
	ImGui::SameLine();
	if (ImGui::SmallButton("Reset to Central Attractor")) {
		view.reference_frame_body_id = BodyManagerContext::kCentralSourceId;
		view.plane_angles_initialized = false;
	}
	render_setting_tooltip("Resets reference origin back to the central spacetime attractor at coordinate (0, 0, 0).");

	const auto rel_state = ctx.compute_relative_orbital_plane_state(b, view.reference_frame_body_id);
	if (!rel_state.valid) return false;

	const double signed_offset = rel_state.signed_inclination_offset_deg;
	const ImVec4 badge_color = (signed_offset >= 0.0)
		? ImVec4(0.35f, 0.95f, 0.45f, 1.0f)
		: ImVec4(1.0f, 0.35f, 0.35f, 1.0f);

	const char* ref_short_name = (rel_state.reference_id == BodyManagerContext::kCentralSourceId)
		? "Equatorial Plane of Central Source"
		: rel_state.reference_name.c_str();

	char offset_buf[128];
	std::snprintf(offset_buf, sizeof(offset_buf), "%+.*f deg", 2, signed_offset);
	ImGui::Text("Relative Tilt Offset: ");
	ImGui::SameLine();
	ImGui::TextColored(badge_color, "%s relative to %s", offset_buf, ref_short_name);
	render_setting_tooltip("Signed angle difference between this body's orbital plane and the reference body's orbital/equatorial plane. Positive indicates a northward tilt; negative indicates a southward tilt.");

	ImGui::TextDisabled("Mutual Plane Angle: %.2f deg | Alignment: %s", rel_state.relative_inclination_deg, rel_state.alignment_category);

	if (rel_state.is_in_kozai_zone) {
		render_caution_text(rel_state.kozai_warning);
	}

	if (view.plane_initialized_for_body != b.id || !view.plane_angles_initialized) {
		view.plane_target_inclination_deg = static_cast<float>(rel_state.relative_inclination_deg);
		view.plane_target_node_deg = static_cast<float>(rel_state.relative_node_deg);
		view.plane_target_periapsis_deg = static_cast<float>(rel_state.relative_periapsis_deg);
		view.plane_angles_initialized = true;
		view.plane_initialized_for_body = b.id;
	}

	bool plane_modified = false;
	float target_inc = view.plane_target_inclination_deg;
	if (slider_float_with_input("Relative Inclination (i_rel)", &target_inc, 0.0f, 180.0f, "%.2f deg")) {
		view.plane_target_inclination_deg = target_inc;
		plane_modified = true;
	}
	render_setting_tooltip("Tilts the orbital plane around the line of nodes without altering the current distance from the reference body, orbital speed, or energy.");

	float target_node = view.plane_target_node_deg;
	if (slider_float_with_input("Longitude of Ascending Node (Omega_rel)", &target_node, 0.0f, 360.0f, "%.2f deg")) {
		view.plane_target_node_deg = target_node;
		plane_modified = true;
	}
	render_setting_tooltip("Pivots the line of nodes around the reference plane normal vector, rotating the entire orbit in azimuth.");

	float target_peri = view.plane_target_periapsis_deg;
	if (slider_float_with_input("Argument of Periapsis (omega_rel)", &target_peri, 0.0f, 360.0f, "%.2f deg")) {
		view.plane_target_periapsis_deg = target_peri;
		plane_modified = true;
	}
	render_setting_tooltip("Rotates the ellipse inside its orbital plane, moving the periapsis position around the orbit normal vector.");

	if (plane_modified) {
		ctx.apply_relative_orbital_plane_rotation(
			b, view.reference_frame_body_id,
			static_cast<double>(view.plane_target_inclination_deg),
			static_cast<double>(view.plane_target_node_deg),
			static_cast<double>(view.plane_target_periapsis_deg)
		);
		changed = true;
	}

	if (ImGui::Button("Coplanar (0 deg)", ImVec2(105.0f, 24.0f))) {
		view.plane_target_inclination_deg = 0.0f;
		ctx.apply_relative_orbital_plane_rotation(b, view.reference_frame_body_id, 0.0, static_cast<double>(view.plane_target_node_deg), static_cast<double>(view.plane_target_periapsis_deg));
		changed = true;
	}
	render_setting_tooltip("Snaps the orbital plane to be perfectly coplanar with the reference plane (0 deg relative inclination).");
	ImGui::SameLine();
	if (ImGui::Button("Polar (90 deg)", ImVec2(95.0f, 24.0f))) {
		view.plane_target_inclination_deg = 90.0f;
		ctx.apply_relative_orbital_plane_rotation(b, view.reference_frame_body_id, 90.0, static_cast<double>(view.plane_target_node_deg), static_cast<double>(view.plane_target_periapsis_deg));
		changed = true;
	}
	render_setting_tooltip("Snaps the orbital plane orthogonal (polar 90 deg) to the reference plane.");
	ImGui::SameLine();
	if (ImGui::Button("Retrograde (180 deg)", ImVec2(130.0f, 24.0f))) {
		view.plane_target_inclination_deg = 180.0f;
		ctx.apply_relative_orbital_plane_rotation(b, view.reference_frame_body_id, 180.0, static_cast<double>(view.plane_target_node_deg), static_cast<double>(view.plane_target_periapsis_deg));
		changed = true;
	}
	render_setting_tooltip("Inverts the orbital plane to counter-rotating coplanar retrograde (180 deg relative inclination).");

	if (ImGui::Button("Circularize Around Reference", ImVec2(190.0f, 24.0f))) {
		ctx.circularize_around_reference(b, view.reference_frame_body_id);
		view.plane_angles_initialized = false;
		changed = true;
	}
	render_setting_tooltip("Calculates circular orbital speed and redirects relative velocity tangentially in the current orbital plane.");
	ImGui::SameLine();
	if (ImGui::Button("Zero Drift (Match Velocity)", ImVec2(185.0f, 24.0f))) {
		ctx.match_reference_velocity(b, view.reference_frame_body_id);
		view.plane_angles_initialized = false;
		changed = true;
	}
	render_setting_tooltip("Sets relative velocity to zero, locking the body's motion to match the reference body's velocity vector.");
	ImGui::SameLine();
	if (ImGui::Button("Align Spin With Normal", ImVec2(160.0f, 24.0f))) {
		b.set_spin_state(b.kerr_spin_parameter(), rel_state.orbit_normal);
		changed = true;
	}
	render_setting_tooltip("Orients this body's intrinsic spin vector to point along the relative orbital plane normal.");

	ImGui::Checkbox("Express Kinematics Relative to Reference Body", &view.show_relative_coordinates);
	render_setting_tooltip("Toggles direct display and input of position, velocity, and acceleration vectors measured relative to the designated reference body.");

	const auto& prefs = ctx.orchestrator().unit_preferences();
	const double length_scale_m = ctx.length_scale();
	const double speed_scale_mps = ctx.speed_scale();

	if (view.show_relative_coordinates) {
		double rel_pos_m[3] = {
			rel_state.relative_position[0] * length_scale_m,
			rel_state.relative_position[1] * length_scale_m,
			rel_state.relative_position[2] * length_scale_m
		};
		if (unit_aware_input_double3("Relative Position (dx, dy, dz)", rel_pos_m, UnitCategory::Distance, prefs)) {
			b.position = {
				rel_state.reference_position[0] + rel_pos_m[0] / length_scale_m,
				rel_state.reference_position[1] + rel_pos_m[1] / length_scale_m,
				rel_state.reference_position[2] + rel_pos_m[2] / length_scale_m
			};
			view.plane_angles_initialized = false;
			changed = true;
		}

		double rel_vel_mps[3] = {
			rel_state.relative_velocity[0] * speed_scale_mps,
			rel_state.relative_velocity[1] * speed_scale_mps,
			rel_state.relative_velocity[2] * speed_scale_mps
		};
		if (unit_aware_input_double3("Relative Velocity (dvx, dvy, dvz)", rel_vel_mps, UnitCategory::Velocity, prefs)) {
			b.velocity = {
				rel_state.reference_velocity[0] + rel_vel_mps[0] / speed_scale_mps,
				rel_state.reference_velocity[1] + rel_vel_mps[1] / speed_scale_mps,
				rel_state.reference_velocity[2] + rel_vel_mps[2] / speed_scale_mps
			};
			view.plane_angles_initialized = false;
			changed = true;
		}

		ImGui::TextDisabled("Relative Acceleration: (%.2e, %.2e, %.2e) | Mag: %s",
			rel_state.relative_acceleration[0], rel_state.relative_acceleration[1], rel_state.relative_acceleration[2],
			Units::format_acceleration(rel_state.relative_acceleration_mag, prefs.acceleration).c_str()
		);
	}

	ImGui::TextDisabled("Distance to Reference: %s | Relative Speed: %s (v_r = %s, v_t = %s)",
		Units::format_distance(rel_state.relative_distance * length_scale_m, prefs.distance).c_str(),
		Units::format_velocity(rel_state.relative_speed * speed_scale_mps, prefs.velocity).c_str(),
		Units::format_velocity(rel_state.relative_radial_speed * speed_scale_mps, prefs.velocity).c_str(),
		Units::format_velocity(rel_state.relative_tangential_speed * speed_scale_mps, prefs.velocity).c_str()
	);

	if (rel_state.is_bound) {
		ImGui::TextDisabled("Relative Orbit: a = %s | e = %.5f | T = %s | Peri = %s",
			Units::format_distance(rel_state.semi_major_axis * length_scale_m, prefs.distance).c_str(),
			rel_state.eccentricity,
			Units::format_time(rel_state.orbital_period_seconds, prefs.time).c_str(),
			Units::format_distance(rel_state.periapsis_distance * length_scale_m, prefs.distance).c_str()
		);
	} else {
		ImGui::TextDisabled("Relative Orbit: Hyperbolic/Escape (e = %.5f) | Periapsis = %s",
			rel_state.eccentricity,
			Units::format_distance(rel_state.periapsis_distance * length_scale_m, prefs.distance).c_str()
		);
	}

	if (rel_state.reference_id != BodyManagerContext::kCentralSourceId && rel_state.hill_radius > 0.0) {
		ImGui::TextDisabled("Reference Hill Sphere: %s | Occupancy: %.1f%% (%s) | Dominance: %.1fx tide",
			Units::format_distance(rel_state.hill_radius * length_scale_m, prefs.distance).c_str(),
			rel_state.hill_occupancy_pct,
			(rel_state.hill_occupancy_pct <= 50.0) ? "Deeply Stable Satellite" : ((rel_state.hill_occupancy_pct <= 100.0) ? "Stable Satellite Orbit" : "Near/Beyond Hill Limit"),
			rel_state.tidal_dominance_ratio
		);
	}

	return changed;
}

[[nodiscard]] inline bool render_frenet_serret_velocity_injector(
	BodyManagerContext& ctx,
	Dynamics::PostNewtonianBody& b,
	BodyEditorViewState& view
) noexcept {
	bool changed = false;
	const auto& prefs = ctx.orchestrator().unit_preferences();
	const double speed_scale_mps = ctx.speed_scale();
	const double length_scale_m = ctx.length_scale();
	const auto fr = ctx.compute_local_frenet_frame(b, view.reference_frame_body_id);
	if (!fr.valid) return false;

	render_group_title("Frenet-Serret Local Velocity Vector Injector (RTN Frame)", ImVec4(0.35f, 0.95f, 0.75f, 1.0f));
	render_hint_text("Manipulate instantaneous velocity directly in the local orbital Frenet-Serret RTN frame (Radial, Tangential Prograde, Normal Out-of-plane) relative to the designated reference attractor. Preview predicted orbital alterations in real time before committing burns.");

	const std::string vr_str = Units::format_velocity(fr.v_radial * speed_scale_mps, prefs.velocity);
	const std::string vt_str = Units::format_velocity(fr.v_tangential * speed_scale_mps, prefs.velocity);
	const std::string vn_str = Units::format_velocity(fr.v_normal * speed_scale_mps, prefs.velocity);
	const std::string v_tot_str = Units::format_velocity(fr.speed * speed_scale_mps, prefs.velocity);
	const std::string v_circ_str = Units::format_velocity(fr.circular_speed * speed_scale_mps, prefs.velocity);
	const std::string v_esc_str = Units::format_velocity(fr.escape_speed * speed_scale_mps, prefs.velocity);

	ImGui::Text("Current RTN Frame Components (Relative to %s):", fr.reference_name.c_str());
	ImGui::TextDisabled("  • Radial v_r: %s (%s, gamma = %+.2f deg)", vr_str.c_str(), (fr.v_radial >= 0.0 ? "Outward Expansion" : "Inward Descent"), fr.flight_path_angle_deg);
	ImGui::TextDisabled("  • Tangential v_t: %s (%s, v_circ = %s)", vt_str.c_str(), (fr.v_tangential >= 0.0 ? "Prograde Motion" : "Retrograde Motion"), v_circ_str.c_str());
	ImGui::TextDisabled("  • Normal v_n: %s (Cross-Track Drift)", vn_str.c_str());
	ImGui::TextDisabled("  • Net Speed: %s | Parabolic Escape Threshold: %s", v_tot_str.c_str(), v_esc_str.c_str());

	ImGui::Spacing();
	ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.35f, 1.0f), "Staged Delta-V Maneuver Inputs:");

	double d_vr = Units::convert_velocity_from_mps(static_cast<double>(view.frenet_delta_radial_mps), prefs.velocity);
	if (unit_aware_slider_double("Delta v_r (Radial +/-)", &d_vr, -50000.0, 50000.0, UnitCategory::Velocity, prefs, "%.3f")) {
		view.frenet_delta_radial_mps = static_cast<float>(Units::convert_velocity_to_mps(d_vr, prefs.velocity));
	}
	render_setting_tooltip("Velocity increment directed along the local radial unit vector. Positive points away from the central attractor (radial boost), negative points inward.");

	double d_vt = Units::convert_velocity_from_mps(static_cast<double>(view.frenet_delta_tangential_mps), prefs.velocity);
	if (unit_aware_slider_double("Delta v_t (Tangential Prograde +/-)", &d_vt, -50000.0, 50000.0, UnitCategory::Velocity, prefs, "%.3f")) {
		view.frenet_delta_tangential_mps = static_cast<float>(Units::convert_velocity_to_mps(d_vt, prefs.velocity));
	}
	render_setting_tooltip("Velocity increment directed along the in-plane tangential vector. Positive accelerates prograde (raises opposite apoapsis), negative decelerates retrograde (lowers opposite periapsis).");

	double d_vn = Units::convert_velocity_from_mps(static_cast<double>(view.frenet_delta_normal_mps), prefs.velocity);
	if (unit_aware_slider_double("Delta v_n (Normal Plane Tilt +/-)", &d_vn, -50000.0, 50000.0, UnitCategory::Velocity, prefs, "%.3f")) {
		view.frenet_delta_normal_mps = static_cast<float>(Units::convert_velocity_to_mps(d_vn, prefs.velocity));
	}
	render_setting_tooltip("Velocity increment directed along the cross-track normal vector (angular momentum axis). Rotates the inclination and tilts the orbital plane.");

	const double dv_r_sim = static_cast<double>(view.frenet_delta_radial_mps) / speed_scale_mps;
	const double dv_t_sim = static_cast<double>(view.frenet_delta_tangential_mps) / speed_scale_mps;
	const double dv_n_sim = static_cast<double>(view.frenet_delta_normal_mps) / speed_scale_mps;

	const auto preview = ctx.preview_frenet_maneuver(b, view.reference_frame_body_id, dv_r_sim, dv_t_sim, dv_n_sim);

	if (preview.valid) {
		ImGui::Spacing();
		ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Pre-Burn Orbit Prediction Diagnostics:");
		const double dv_mag_mps = preview.delta_v_magnitude * speed_scale_mps;
		ImGui::TextDisabled("  • Staged Total Delta-V: %s", Units::format_velocity(dv_mag_mps, prefs.velocity).c_str());

		const std::string curr_spd_str = Units::format_velocity(preview.current_speed * speed_scale_mps, prefs.velocity);
		const std::string pred_spd_str = Units::format_velocity(preview.predicted_speed * speed_scale_mps, prefs.velocity);
		ImGui::TextDisabled("  • Speed: %s -> %s", curr_spd_str.c_str(), pred_spd_str.c_str());

		ImGui::TextDisabled("  • Eccentricity: %.5f -> %.5f (%s)", preview.current_eccentricity, preview.predicted_eccentricity, preview.predicted_orbit_type);

		if (preview.predicted_is_bound) {
			const std::string curr_a = Units::format_distance(preview.current_semi_major_axis * length_scale_m, prefs.distance, 2);
			const std::string pred_a = Units::format_distance(preview.predicted_semi_major_axis * length_scale_m, prefs.distance, 2);
			const std::string curr_p = Units::format_distance(preview.current_periapsis * length_scale_m, prefs.distance, 2);
			const std::string pred_p = Units::format_distance(preview.predicted_periapsis * length_scale_m, prefs.distance, 2);
			const std::string curr_ap = Units::format_distance(preview.current_apoapsis * length_scale_m, prefs.distance, 2);
			const std::string pred_ap = Units::format_distance(preview.predicted_apoapsis * length_scale_m, prefs.distance, 2);
			ImGui::TextDisabled("  • Semi-Major Axis: %s -> %s", curr_a.c_str(), pred_a.c_str());
			ImGui::TextDisabled("  • Periapsis: %s -> %s | Apoapsis: %s -> %s", curr_p.c_str(), pred_p.c_str(), curr_ap.c_str(), pred_ap.c_str());
			ImGui::TextDisabled("  • Period: %s -> %s", Units::format_time(preview.current_period_seconds, prefs.time).c_str(), Units::format_time(preview.predicted_period_seconds, prefs.time).c_str());
		} else {
			const std::string pred_p = Units::format_distance(preview.predicted_periapsis * length_scale_m, prefs.distance, 2);
			ImGui::TextDisabled("  • Unbound Orbit: Predicted Periapsis = %s", pred_p.c_str());
		}
		ImGui::TextDisabled("  • Inclination: %.2f deg -> %.2f deg", preview.current_inclination_deg, preview.predicted_inclination_deg);

		if (preview.causes_plunge) {
			render_caution_text("Warning: Burn will drive periapsis inside the event horizon (Plunge Hazard)!");
		}
		if (preview.causes_escape) {
			render_caution_text("Notice: Burn will unbind the orbit onto an escape trajectory (Hyperbolic Drift)!");
		}
	}

	ImGui::Spacing();
	const bool has_staged_dv = (std::abs(view.frenet_delta_radial_mps) > 1e-4f ||
		std::abs(view.frenet_delta_tangential_mps) > 1e-4f ||
		std::abs(view.frenet_delta_normal_mps) > 1e-4f);

	ImGui::BeginDisabled(!has_staged_dv);
	if (ImGui::Button("Execute Burn / Apply Delta-V", ImVec2(220.0f, 26.0f))) {
		ctx.apply_frenet_delta_v(b, view.reference_frame_body_id, dv_r_sim, dv_t_sim, dv_n_sim);
		view.frenet_delta_radial_mps = 0.0f;
		view.frenet_delta_tangential_mps = 0.0f;
		view.frenet_delta_normal_mps = 0.0f;
		changed = true;
	}
	render_setting_tooltip("Applies the staged RTN delta-V vector to the body velocity and resets staged values to zero.");
	ImGui::SameLine();
	if (ImGui::Button("Zero Staged Delta-V", ImVec2(150.0f, 26.0f))) {
		view.frenet_delta_radial_mps = 0.0f;
		view.frenet_delta_tangential_mps = 0.0f;
		view.frenet_delta_normal_mps = 0.0f;
	}
	render_setting_tooltip("Discards currently staged delta-V values without altering body velocity.");
	ImGui::EndDisabled();

	render_group_title("Canonical Orbital Maneuver Shortcuts", ImVec4(0.5f, 0.85f, 1.0f, 1.0f));
	double step_disp = Units::convert_velocity_from_si(static_cast<double>(view.frenet_step_magnitude_mps), prefs.velocity);
	if (unit_aware_slider_double("Impulse Step Delta-V", &step_disp, 0.1, 10000.0, UnitCategory::Velocity, prefs, "%.2f")) {
		view.frenet_step_magnitude_mps = static_cast<float>(Units::convert_velocity_to_si(step_disp, prefs.velocity));
	}
	render_setting_tooltip("Magnitude of velocity increment applied by the quick impulse buttons below.");

	const double step_sim = static_cast<double>(view.frenet_step_magnitude_mps) / speed_scale_mps;

	if (ImGui::Button("+Prograde", ImVec2(100.0f, 24.0f))) {
		ctx.apply_frenet_delta_v(b, view.reference_frame_body_id, 0.0, step_sim, 0.0);
		changed = true;
	}
	render_setting_tooltip("Applies instantaneous positive tangential acceleration (+v_t), raising orbital energy and apoapsis.");
	ImGui::SameLine();
	if (ImGui::Button("-Retrograde", ImVec2(105.0f, 24.0f))) {
		ctx.apply_frenet_delta_v(b, view.reference_frame_body_id, 0.0, -step_sim, 0.0);
		changed = true;
	}
	render_setting_tooltip("Applies instantaneous negative tangential acceleration (-v_t), braking the orbit and lowering periapsis.");
	ImGui::SameLine();
	if (ImGui::Button("+Radial Out", ImVec2(105.0f, 24.0f))) {
		ctx.apply_frenet_delta_v(b, view.reference_frame_body_id, step_sim, 0.0, 0.0);
		changed = true;
	}
	render_setting_tooltip("Applies radial impulse pointing outward away from the central attractor.");
	ImGui::SameLine();
	if (ImGui::Button("-Radial In", ImVec2(100.0f, 24.0f))) {
		ctx.apply_frenet_delta_v(b, view.reference_frame_body_id, -step_sim, 0.0, 0.0);
		changed = true;
	}
	render_setting_tooltip("Applies radial impulse pointing inward toward the central attractor.");

	if (ImGui::Button("+Normal North", ImVec2(110.0f, 24.0f))) {
		ctx.apply_frenet_delta_v(b, view.reference_frame_body_id, 0.0, 0.0, step_sim);
		changed = true;
	}
	render_setting_tooltip("Applies normal impulse along angular momentum vector (+v_n), tilting the orbital plane northward.");
	ImGui::SameLine();
	if (ImGui::Button("-Anti-Normal South", ImVec2(145.0f, 24.0f))) {
		ctx.apply_frenet_delta_v(b, view.reference_frame_body_id, 0.0, 0.0, -step_sim);
		changed = true;
	}
	render_setting_tooltip("Applies anti-normal impulse opposing angular momentum vector (-v_n), tilting the orbital plane southward.");
	ImGui::SameLine();
	if (ImGui::Button("Circularize (Zero v_r, v_n)", ImVec2(175.0f, 24.0f))) {
		const double dir_sign = (fr.v_tangential >= 0.0) ? 1.0 : -1.0;
		ctx.set_frenet_velocity_components(b, view.reference_frame_body_id, 0.0, dir_sign * fr.circular_speed, 0.0);
		changed = true;
	}
	render_setting_tooltip("Sets radial and normal velocity to zero and sets tangential speed to the circular orbit velocity at the current radius.");

	if (ImGui::Button("Zero Radial Drift (Cancel v_r)", ImVec2(200.0f, 24.0f))) {
		ctx.set_frenet_velocity_components(b, view.reference_frame_body_id, 0.0, fr.v_tangential, fr.v_normal);
		changed = true;
	}
	render_setting_tooltip("Cancels only radial motion, locking current tangential and normal velocities.");
	ImGui::SameLine();
	if (ImGui::Button("Zero Normal Drift (Cancel v_n)", ImVec2(200.0f, 24.0f))) {
		ctx.set_frenet_velocity_components(b, view.reference_frame_body_id, fr.v_radial, fr.v_tangential, 0.0);
		changed = true;
	}
	render_setting_tooltip("Cancels cross-track out-of-plane velocity, confining motion strictly to the in-plane tangential-radial disk.");

	if (ImGui::Button("Parabolic Escape (v_t = v_esc)", ImVec2(205.0f, 24.0f))) {
		const double dir_sign = (fr.v_tangential >= 0.0) ? 1.0 : -1.0;
		ctx.set_frenet_velocity_components(b, view.reference_frame_body_id, 0.0, dir_sign * fr.escape_speed, 0.0);
		changed = true;
	}
	render_setting_tooltip("Sets tangential velocity to exact local parabolic escape velocity, placing the body onto an escape parabola.");

	double r_hohmann_disp = static_cast<double>(view.frenet_target_hohmann_radius) * length_scale_m;
	if (unit_aware_slider_double("Hohmann Target Radius", &r_hohmann_disp, 0.1 * length_scale_m, 2000.0 * length_scale_m, UnitCategory::Distance, prefs, "%.2f", &view.frenet_hohmann_radius_log_mode)) {
		view.frenet_target_hohmann_radius = static_cast<float>(r_hohmann_disp / length_scale_m);
	}
	render_setting_tooltip("Destination radius for the transfer orbit departure burn.");
	ImGui::SameLine();
	if (ImGui::Button("Inject Hohmann Burn", ImVec2(150.0f, 24.0f))) {
		ctx.apply_frenet_hohmann_transfer(b, view.reference_frame_body_id, static_cast<double>(view.frenet_target_hohmann_radius));
		changed = true;
	}
	render_setting_tooltip("Computes and executes the tangential departure impulse for a two-impulse transfer orbit to the target radius.");

	float p_change = view.frenet_plane_change_deg;
	if (ImGui::SliderFloat("Plane Change Angle", &p_change, -90.0f, 90.0f, "%+.2f deg")) {
		view.frenet_plane_change_deg = p_change;
	}
	render_setting_tooltip("Target inclination change angle for a pure isokinetic out-of-plane burn.");
	ImGui::SameLine();
	if (ImGui::Button("Execute Plane Change", ImVec2(150.0f, 24.0f))) {
		ctx.apply_frenet_plane_change_burn(b, view.reference_frame_body_id, static_cast<double>(view.frenet_plane_change_deg));
		changed = true;
	}
	render_setting_tooltip("Executes an isokinetic burn rotating the orbital plane without modifying total speed or mechanical energy.");

	return changed;
}

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

	changed = render_relative_reference_and_orbital_plane_tool(ctx, b, view) || changed;

	changed = render_frenet_serret_velocity_injector(ctx, b, view) || changed;

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

inline bool render_batch_source_editor(
	BodyManagerContext& ctx,
	Dynamics::PostNewtonianSystem& sys,
	std::vector<Dynamics::PostNewtonianBody*>& bodies,
	BodyEditorViewState& view
) noexcept {
	if (bodies.empty()) return false;
	bool changed = false;
	const auto& prefs = ctx.orchestrator().unit_preferences();
	const double mass_scale_kg = ctx.mass_scale();
	const double speed_scale_mps = ctx.speed_scale();

	size_t enabled_count = 0;
	double total_mass = 0.0;
	double total_spin_ratio = 0.0;
	for (const auto* b : bodies) {
		if (b->enabled) ++enabled_count;
		total_mass += b->mass;
		total_spin_ratio += b->kerr_spin_parameter();
	}
	const double mean_spin_ratio = bodies.empty() ? 0.0 : (total_spin_ratio / static_cast<double>(bodies.size()));

	ImGui::TextColored(ImVec4(1.0f, 0.65f, 0.4f, 1.0f), "Batch Kerr Inspector: %zu Black Holes Selected (%zu enabled)", bodies.size(), enabled_count);
	ImGui::TextDisabled("Combined Mass: %s | Mean Kerr Spin a/M: %.4f", Units::format_mass(total_mass * mass_scale_kg, prefs.mass, 3).c_str(), mean_spin_ratio);

	if (ImGui::Button("Look At Centroid", ImVec2(120.0f, 24.0f))) {
		ctx.look_at_selection(bodies);
	}
	ImGui::SameLine();
	if (ImGui::Button("Move All to Safe Separation", ImVec2(190.0f, 24.0f))) {
		for (auto* b : bodies) ctx.resolve_source_placement(*b);
		changed = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("Circularize All", ImVec2(110.0f, 24.0f))) {
		const double central_mass = ctx.orchestrator().parameters().mass;
		for (auto* b : bodies) {
			b->velocity = ctx.compute_circular_orbit_velocity(b->position, central_mass + b->mass);
		}
		changed = true;
	}

	if (ImGui::Button("Demote All Selected to Ordinary Celestial Bodies", ImVec2(-1.0f, 26.0f))) {
		for (auto* b : bodies) {
			b->is_spacetime_source = false;
			b->radius = std::max(b->radius, 4.0 * b->mass);
			b->reference_radius = b->radius;
			apply_body_appearance_preset(*b, Dynamics::Body3DPreset::Metallic);
		}
		std::vector<int> obj_ids;
		obj_ids.reserve(bodies.size());
		for (const auto* b : bodies) obj_ids.push_back(static_cast<int>(b->id));
		ctx.set_selected_objects(obj_ids);
		ctx.request_tab(BodyManagerTab::ObjectCatalog);
		changed = true;
	}

	if (ImGui::CollapsingHeader("Spacetime & Metric Parameters (Batch)", ImGuiTreeNodeFlags_DefaultOpen)) {
		changed = render_batch_scalar_slider(
			"Mass (M)", bodies,
			[](const auto& b) { return b.mass; },
			[](auto& b, double v) { b.mass = std::max(v, 1e-9); b.radius = b.kerr_outer_horizon_radius(); b.reference_radius = b.radius; },
			[&](double v) { return Units::format_mass(v * mass_scale_kg, prefs.mass, 3); },
			0.001f, 1.0e6f, "%.4f", &view.source_parameters.mass_log_mode, 1e-12f, 1e30f
		) || changed;

		changed = render_batch_scalar_slider(
			"Dimensionless Spin (a/M)", bodies,
			[](const auto& b) { return b.kerr_spin_parameter(); },
			[](auto& b, double v) { b.set_spin_state(std::clamp(v, 0.0, 0.999), b.spin_axis_unit()); b.radius = b.kerr_outer_horizon_radius(); b.reference_radius = b.radius; },
			[](double v) { char buf[32]; std::snprintf(buf, sizeof(buf), "%.4f", v); return std::string(buf); },
			0.0f, 0.999f, "%.4f"
		) || changed;

		changed = render_batch_scalar_slider(
			"Electric Charge (Q)", bodies,
			[](const auto& b) { return b.charge; },
			[](auto& b, double v) { b.charge = std::clamp(v, -b.mass, b.mass); b.radius = b.kerr_outer_horizon_radius(); b.reference_radius = b.radius; },
			[&](double v) { return Units::format_charge(v, prefs.charge, 3); },
			-10.0f, 10.0f, "%.4e"
		) || changed;

		const bool x_eq = batch_are_equal(bodies, [](const auto& b) { return b.position[0]; });
		const bool y_eq = batch_are_equal(bodies, [](const auto& b) { return b.position[1]; });
		const bool z_eq = batch_are_equal(bodies, [](const auto& b) { return b.position[2]; });
		if (x_eq && y_eq && z_eq) {
			if (coordinate_aware_input_position("Position", bodies.front()->position, ctx.orchestrator())) {
				for (size_t i = 1; i < bodies.size(); ++i) bodies[i]->position = bodies.front()->position;
				changed = true;
			}
		} else {
			ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "Position: * (Mixed Coordinates)");
			batch_hover_tooltip(bodies, [](const auto& b) {
				return "(" + std::to_string(b.position[0]).substr(0, 7) + ", " + std::to_string(b.position[1]).substr(0, 7) + ", " + std::to_string(b.position[2]).substr(0, 7) + ")";
			}, [](const std::string& s) { return s; }, "Position");
		}

		const bool vx_eq = batch_are_equal(bodies, [](const auto& b) { return b.velocity[0]; });
		const bool vy_eq = batch_are_equal(bodies, [](const auto& b) { return b.velocity[1]; });
		const bool vz_eq = batch_are_equal(bodies, [](const auto& b) { return b.velocity[2]; });
		if (vx_eq && vy_eq && vz_eq) {
			double vel[3] = {bodies.front()->velocity[0] * speed_scale_mps, bodies.front()->velocity[1] * speed_scale_mps, bodies.front()->velocity[2] * speed_scale_mps};
			if (unit_aware_input_double3("Velocity", vel, UnitCategory::Velocity, prefs)) {
				for (auto* b : bodies) {
					b->velocity = {vel[0] / speed_scale_mps, vel[1] / speed_scale_mps, vel[2] / speed_scale_mps};
				}
				changed = true;
			}
		} else {
			ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "Velocity: * (Mixed Velocities)");
			batch_hover_tooltip(bodies, [](const auto& b) {
				return "(" + std::to_string(b.velocity[0]).substr(0, 6) + ", " + std::to_string(b.velocity[1]).substr(0, 6) + ", " + std::to_string(b.velocity[2]).substr(0, 6) + ")";
			}, [](const std::string& s) { return s; }, "Velocity");
		}
	}

	if (ImGui::CollapsingHeader("Batch Orbital Plane & Reference Alignment", ImGuiTreeNodeFlags_DefaultOpen)) {
		render_group_title("Reference Attractor Selection", ImVec4(0.4f, 0.85f, 1.0f, 1.0f));
		render_hint_text("Select a common reference origin to evaluate or reorient the orbital planes of all selected black holes simultaneously.");

		std::vector<int> src_b_ref_ids{BodyManagerContext::kCentralSourceId};
		std::vector<std::string> src_b_ref_labels{"Central Spacetime Attractor (Origin #0)"};
		for (const auto& other : sys.bodies()) {
			if (!other.enabled) continue;
			bool in_selection = false;
			for (const auto* sel : bodies) {
				if (sel->id == other.id) { in_selection = true; break; }
			}
			if (in_selection) continue;
			src_b_ref_ids.push_back(static_cast<int>(other.id));
			src_b_ref_labels.push_back(BodyManagerContext::display_name(other) + " (#" + std::to_string(other.id) + ")");
		}

		int src_b_ref_combo = 0;
		for (size_t i = 0; i < src_b_ref_ids.size(); ++i) {
			if (src_b_ref_ids[i] == view.reference_frame_body_id) {
				src_b_ref_combo = static_cast<int>(i);
				break;
			}
		}

		std::vector<const char*> src_b_ref_ptrs;
		src_b_ref_ptrs.reserve(src_b_ref_labels.size());
		for (const auto& s : src_b_ref_labels) src_b_ref_ptrs.push_back(s.c_str());

		if (ImGui::Combo("Common Reference Body##SourceBatch", &src_b_ref_combo, src_b_ref_ptrs.data(), static_cast<int>(src_b_ref_ptrs.size()))) {
			view.reference_frame_body_id = src_b_ref_ids[static_cast<size_t>(src_b_ref_combo)];
		}

		double min_tilt = 1e30, max_tilt = -1e30, sum_tilt = 0.0;
		for (const auto* b_item : bodies) {
			const auto st = ctx.compute_relative_orbital_plane_state(*b_item, view.reference_frame_body_id);
			if (st.valid) {
				min_tilt = std::min(min_tilt, st.signed_inclination_offset_deg);
				max_tilt = std::max(max_tilt, st.signed_inclination_offset_deg);
				sum_tilt += st.signed_inclination_offset_deg;
			}
		}
		const double avg_tilt = bodies.empty() ? 0.0 : (sum_tilt / static_cast<double>(bodies.size()));
		const bool is_uniform_tilt = (max_tilt - min_tilt < 0.01);
		const ImVec4 tilt_badge_col = (avg_tilt >= 0.0) ? ImVec4(0.35f, 0.95f, 0.45f, 1.0f) : ImVec4(1.0f, 0.35f, 0.35f, 1.0f);

		if (is_uniform_tilt) {
			ImGui::Text("Batch Tilt Offset: ");
			ImGui::SameLine();
			ImGui::TextColored(tilt_badge_col, "%+.*f deg", 2, avg_tilt);
		} else {
			ImGui::Text("Batch Tilt Span: ");
			ImGui::SameLine();
			ImGui::TextColored(tilt_badge_col, "[%+.*f deg .. %+.*f deg] (Mean: %+.*f deg)", 2, min_tilt, 2, max_tilt, 2, avg_tilt);
		}

		static float src_batch_uniform_inc = 0.0f;
		if (slider_float_with_input("Set Uniform Inclination##SourceBatch", &src_batch_uniform_inc, 0.0f, 180.0f, "%.2f deg")) {
			for (auto* b_item : bodies) {
				const auto st = ctx.compute_relative_orbital_plane_state(*b_item, view.reference_frame_body_id);
				ctx.apply_relative_orbital_plane_rotation(*b_item, view.reference_frame_body_id, static_cast<double>(src_batch_uniform_inc), st.relative_node_deg, st.relative_periapsis_deg);
			}
			changed = true;
		}

		if (ImGui::Button("Align All Coplanar (0 deg)##SourceBatch", ImVec2(180.0f, 24.0f))) {
			for (auto* b_item : bodies) {
				const auto st = ctx.compute_relative_orbital_plane_state(*b_item, view.reference_frame_body_id);
				ctx.apply_relative_orbital_plane_rotation(*b_item, view.reference_frame_body_id, 0.0, st.relative_node_deg, st.relative_periapsis_deg);
			}
			changed = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Set All Polar (90 deg)##SourceBatch", ImVec2(180.0f, 24.0f))) {
			for (auto* b_item : bodies) {
				const auto st = ctx.compute_relative_orbital_plane_state(*b_item, view.reference_frame_body_id);
				ctx.apply_relative_orbital_plane_rotation(*b_item, view.reference_frame_body_id, 90.0, st.relative_node_deg, st.relative_periapsis_deg);
			}
			changed = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Invert All Retrograde (180 deg)##SourceBatch", ImVec2(220.0f, 24.0f))) {
			for (auto* b_item : bodies) {
				const auto st = ctx.compute_relative_orbital_plane_state(*b_item, view.reference_frame_body_id);
				ctx.apply_relative_orbital_plane_rotation(*b_item, view.reference_frame_body_id, 180.0, st.relative_node_deg, st.relative_periapsis_deg);
			}
			changed = true;
		}

		if (ImGui::Button("Circularize All Around Reference##SourceBatch", ImVec2(240.0f, 24.0f))) {
			for (auto* b_item : bodies) {
				ctx.circularize_around_reference(*b_item, view.reference_frame_body_id);
			}
			changed = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Zero Relative Drift (Match Velocity)##SourceBatch", ImVec2(260.0f, 24.0f))) {
			for (auto* b_item : bodies) {
				ctx.match_reference_velocity(*b_item, view.reference_frame_body_id);
			}
			changed = true;
		}
	}

	if (ImGui::CollapsingHeader("Accretion Disk System (Batch)", ImGuiTreeNodeFlags_DefaultOpen)) {
		const bool disk_on_eq = batch_are_equal_exact(bodies, [](const auto& b) { return b.accretion_disk.enabled; });
		if (disk_on_eq) {
			bool disk_on = bodies.front()->accretion_disk.enabled;
			if (ImGui::Checkbox("Render Accretion Disk (All Selected)", &disk_on)) {
				for (auto* b : bodies) b->accretion_disk.enabled = disk_on;
				changed = true;
			}
		} else {
			ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "Accretion Disk: * (Mixed Active States)");
			ImGui::SameLine();
			if (ImGui::SmallButton("Enable All Disks")) {
				for (auto* b : bodies) b->accretion_disk.enabled = true;
				changed = true;
			}
			ImGui::SameLine();
			if (ImGui::SmallButton("Disable All Disks")) {
				for (auto* b : bodies) b->accretion_disk.enabled = false;
				changed = true;
			}
		}

		if (ImGui::BeginCombo("Apply Preset To All Disks", "Select a preset...")) {
			for (size_t i = 0; i < Render::kAccretionDiskPresetNames.size(); ++i) {
				if (ImGui::Selectable(Render::kAccretionDiskPresetNames[i])) {
					for (auto* b : bodies) b->accretion_disk.apply_preset(static_cast<Render::AccretionDiskPreset>(i), false);
					changed = true;
				}
			}
			ImGui::EndCombo();
		}

		changed = render_batch_scalar_slider(
			"Inner Radius (x ISCO)", bodies,
			[](const auto& b) { return b.accretion_disk.inner_radius_scale; },
			[](auto& b, double v) { b.accretion_disk.inner_radius_scale = static_cast<float>(v); },
			[](double v) { char buf[32]; std::snprintf(buf, sizeof(buf), "%.2f", v); return std::string(buf); },
			1.0f, 4.0f, "%.2f"
		) || changed;

		changed = render_batch_scalar_slider(
			"Outer Radius (M)", bodies,
			[](const auto& b) { return b.accretion_disk.outer_radius_mass_units; },
			[](auto& b, double v) { b.accretion_disk.outer_radius_mass_units = static_cast<float>(v); },
			[](double v) { char buf[32]; std::snprintf(buf, sizeof(buf), "%.1f", v); return std::string(buf); },
			6.0f, 120.0f, "%.1f"
		) || changed;

		changed = render_batch_scalar_slider(
			"Peak Temperature (K)", bodies,
			[](const auto& b) { return b.accretion_disk.peak_temperature_k; },
			[](auto& b, double v) { b.accretion_disk.peak_temperature_k = static_cast<float>(v); },
			[](double v) { char buf[32]; std::snprintf(buf, sizeof(buf), "%.0f K", v); return std::string(buf); },
			1000.0f, 200000.0f, "%.0f"
		) || changed;

		changed = render_batch_scalar_slider(
			"Brightness", bodies,
			[](const auto& b) { return b.accretion_disk.brightness; },
			[](auto& b, double v) { b.accretion_disk.brightness = static_cast<float>(v); },
			[](double v) { char buf[32]; std::snprintf(buf, sizeof(buf), "%.3f", v); return std::string(buf); },
			0.0f, 20.0f, "%.3f"
		) || changed;

		changed = render_batch_scalar_slider(
			"Opacity", bodies,
			[](const auto& b) { return b.accretion_disk.opacity_scale; },
			[](auto& b, double v) { b.accretion_disk.opacity_scale = static_cast<float>(v); },
			[](double v) { char buf[32]; std::snprintf(buf, sizeof(buf), "%.2f", v); return std::string(buf); },
			0.0f, 2.0f, "%.2f"
		) || changed;
	}

	if (ImGui::CollapsingHeader("Batch Frenet-Serret Local Velocity Vector Injection (RTN)", ImGuiTreeNodeFlags_DefaultOpen)) {
		render_group_title("Batch Local RTN Maneuver Injection", ImVec4(0.35f, 0.95f, 0.75f, 1.0f));
		render_hint_text("Apply velocity impulses decomposed in each individual black hole's own local Frenet-Serret RTN frame (Radial, Tangential Prograde, Normal Out-of-plane) relative to the common reference body.");

		static float batch_src_d_vr_mps = 0.0f;
		static float batch_src_d_vt_mps = 0.0f;
		static float batch_src_d_vn_mps = 0.0f;
		static float batch_src_step_mps = 50.0f;

		double b_dvr = Units::convert_velocity_from_mps(static_cast<double>(batch_src_d_vr_mps), prefs.velocity);
		if (unit_aware_slider_double("Batch Delta v_r (Radial +/-)##SourceBatch", &b_dvr, -50000.0, 50000.0, UnitCategory::Velocity, prefs, "%.3f")) {
			batch_src_d_vr_mps = static_cast<float>(Units::convert_velocity_to_mps(b_dvr, prefs.velocity));
		}
		double b_dvt = Units::convert_velocity_from_mps(static_cast<double>(batch_src_d_vt_mps), prefs.velocity);
		if (unit_aware_slider_double("Batch Delta v_t (Tangential Prograde +/-)##SourceBatch", &b_dvt, -50000.0, 50000.0, UnitCategory::Velocity, prefs, "%.3f")) {
			batch_src_d_vt_mps = static_cast<float>(Units::convert_velocity_to_mps(b_dvt, prefs.velocity));
		}
		double b_dvn = Units::convert_velocity_from_mps(static_cast<double>(batch_src_d_vn_mps), prefs.velocity);
		if (unit_aware_slider_double("Batch Delta v_n (Normal Plane Tilt +/-)##SourceBatch", &b_dvn, -50000.0, 50000.0, UnitCategory::Velocity, prefs, "%.3f")) {
			batch_src_d_vn_mps = static_cast<float>(Units::convert_velocity_to_mps(b_dvn, prefs.velocity));
		}

		const double b_dvr_sim = static_cast<double>(batch_src_d_vr_mps) / speed_scale_mps;
		const double b_dvt_sim = static_cast<double>(batch_src_d_vt_mps) / speed_scale_mps;
		const double b_dvn_sim = static_cast<double>(batch_src_d_vn_mps) / speed_scale_mps;

		if (ImGui::Button("Execute Batch RTN Burn on All Selected Sources", ImVec2(330.0f, 26.0f))) {
			for (auto* b_item : bodies) {
				ctx.apply_frenet_delta_v(*b_item, view.reference_frame_body_id, b_dvr_sim, b_dvt_sim, b_dvn_sim);
			}
			batch_src_d_vr_mps = 0.0f;
			batch_src_d_vt_mps = 0.0f;
			batch_src_d_vn_mps = 0.0f;
			changed = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Zero Staged Batch Delta-V##SourceBatch", ImVec2(190.0f, 26.0f))) {
			batch_src_d_vr_mps = 0.0f;
			batch_src_d_vt_mps = 0.0f;
			batch_src_d_vn_mps = 0.0f;
		}

		double b_step_disp = Units::convert_velocity_from_mps(static_cast<double>(batch_src_step_mps), prefs.velocity);
		if (unit_aware_slider_double("Batch Impulse Step##SourceBatch", &b_step_disp, 0.1, 10000.0, UnitCategory::Velocity, prefs, "%.2f")) {
			batch_src_step_mps = static_cast<float>(Units::convert_velocity_to_mps(b_step_disp, prefs.velocity));
		}

		const double b_step_sim = static_cast<double>(batch_src_step_mps) / speed_scale_mps;
		if (ImGui::Button("+Prograde All##SourceBatchRTN", ImVec2(120.0f, 24.0f))) {
			for (auto* b_item : bodies) {
				ctx.apply_frenet_delta_v(*b_item, view.reference_frame_body_id, 0.0, b_step_sim, 0.0);
			}
			changed = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("-Retrograde All##SourceBatchRTN", ImVec2(125.0f, 24.0f))) {
			for (auto* b_item : bodies) {
				ctx.apply_frenet_delta_v(*b_item, view.reference_frame_body_id, 0.0, -b_step_sim, 0.0);
			}
			changed = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("+Radial Out All##SourceBatchRTN", ImVec2(125.0f, 24.0f))) {
			for (auto* b_item : bodies) {
				ctx.apply_frenet_delta_v(*b_item, view.reference_frame_body_id, b_step_sim, 0.0, 0.0);
			}
			changed = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("-Radial In All##SourceBatchRTN", ImVec2(120.0f, 24.0f))) {
			for (auto* b_item : bodies) {
				ctx.apply_frenet_delta_v(*b_item, view.reference_frame_body_id, -b_step_sim, 0.0, 0.0);
			}
			changed = true;
		}

		if (ImGui::Button("+Normal North All##SourceBatchRTN", ImVec2(150.0f, 24.0f))) {
			for (auto* b_item : bodies) {
				ctx.apply_frenet_delta_v(*b_item, view.reference_frame_body_id, 0.0, 0.0, b_step_sim);
			}
			changed = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("-Anti-Normal South All##SourceBatchRTN", ImVec2(175.0f, 24.0f))) {
			for (auto* b_item : bodies) {
				ctx.apply_frenet_delta_v(*b_item, view.reference_frame_body_id, 0.0, 0.0, -b_step_sim);
			}
			changed = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Circularize All in Respective Local Frames##SourceBatchRTN", ImVec2(290.0f, 24.0f))) {
			for (auto* b_item : bodies) {
				const auto fr_item = ctx.compute_local_frenet_frame(*b_item, view.reference_frame_body_id);
				if (fr_item.valid) {
					const double dir_sign = (fr_item.v_tangential >= 0.0) ? 1.0 : -1.0;
					ctx.set_frenet_velocity_components(*b_item, view.reference_frame_body_id, 0.0, dir_sign * fr_item.circular_speed, 0.0);
				}
			}
			changed = true;
		}
	}

	if (changed) {
		for (auto* b : bodies) b->enforce_spacetime_source_invariants();
		sys.update_accelerations();
		ctx.orchestrator().notify_state_changed();
	}
	return changed;
}

}

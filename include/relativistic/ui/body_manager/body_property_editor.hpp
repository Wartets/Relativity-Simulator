#pragma once

#include <imgui.h>
#include "relativistic/ui/body_manager/body_manager_context.hpp"
#include "relativistic/ui/body_manager/source_property_editor.hpp"
#include "relativistic/ui/numeric_slider_utils.hpp"
#include "relativistic/ui/tooltip_utils.hpp"
#include "relativistic/ui/panel_text_helpers.hpp"
#include "relativistic/ui/coordinate_aware_widgets.hpp"
#include "relativistic/units/unit_system.hpp"
#include "relativistic/units/unit_aware_widgets.hpp"
#include "relativistic/dynamics/body_surface_layers.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <numbers>
#include <string>
#include <string_view>

namespace Relativistic::UI {

[[nodiscard]] inline bool body_editor_slider(const char* label, double& value, float min_value, float max_value, const char* format, bool* log_mode = nullptr, float log_min = 0.0f, float log_max = 0.0f) noexcept {
	float scratch = static_cast<float>(value);
	const float effective_log_min = (log_min > 0.0f) ? log_min : min_value;
	const float effective_log_max = (log_max > 0.0f) ? log_max : max_value;
	if (slider_float_with_input(label, &scratch, min_value, max_value, format, log_mode, effective_log_min, effective_log_max)) {
		value = static_cast<double>(scratch);
		return true;
	}
	return false;
}

[[nodiscard]] inline const char* body_texture_mode_description(uint32_t mode) noexcept {
	switch (mode) {
		case 0U: return "Procedural noise: oceans below the noise threshold, land blended from primary to secondary color. Uses Noise Scale, Roughness and Texture Detail Scale.";
		case 1U: return "Solid color: flat primary color. Layers, polar caps, city lights and ring band still apply on top.";
		case 2U: return "Palette blend: smooth gradient primary, midpoint, secondary driven by noise. Uses Noise Scale and Texture Detail Scale.";
		case 3U: return "Banded gas giant: latitude bands warped by noise between primary and secondary colors.";
		case 4U: return "Cratered terrain: crater darkening and rims with secondary color speckle.";
		case 5U: return "Stellar granulation: limb-darkened granular surface, strongly driven by Emission Intensity.";
		case 6U: return "Accretion flow: hot streaks along longitude, driven by Emission Intensity and Noise Scale.";
		case 7U: return "Marbled stone: primary/secondary base with tertiary color veins scaled by Texture Detail Scale.";
		case 8U: return "Ringed gas giant: bands, tertiary polar caps and equatorial ring shadow driven by Polar Cap Strength and Ring System.";
		case 9U: return "Icy cracked surface: secondary color cracks with tertiary color shimmer.";
		case 10U: return "Volcanic magma: tertiary color glowing cracks over primary crust with secondary patches.";
		case 11U: return "City lights: primary/secondary land with a speckled night side driven by Night Side City Lights.";
		case 12U: return "Nebulous cloud: three-color wisps mixing primary, secondary and tertiary colors.";
		case 13U: return "Earth photographic map: real day and night imagery with an automatic lit-side mask. Only the texture layers stay editable on top, and the images are loaded only while a body uses this mode.";
		default: return "Unknown texture mode.";
	}
}

[[nodiscard]] inline bool render_body_identity_section(BodyManagerContext& ctx, Dynamics::PostNewtonianBody& b, bool allow_source_toggle) noexcept {
	bool changed = false;
	char name_buffer[32]{};
	const std::string_view current_name = b.name_view();
	std::memcpy(name_buffer, current_name.data(), std::min(current_name.size(), sizeof(name_buffer) - 1));
	if (ImGui::InputTextWithHint("Name", "Unnamed", name_buffer, sizeof(name_buffer))) {
		b.set_name(std::string_view(name_buffer));
		changed = true;
	}
	render_setting_tooltip("Human-readable label shown in the catalog, tags and saved scenarios instead of the numeric identifier.");
	if (ImGui::Checkbox("Enabled", &b.enabled)) changed = true;
	render_setting_tooltip("Disabled bodies remain in the catalog and scenario, but are excluded from rendering, prediction, gravity, integration, and horizon absorption.");

	if (allow_source_toggle) {
		ImGui::Spacing();
		if (!b.is_spacetime_source) {
			ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 0.65f, 0.3f, 0.7f));
			const float card_h = ImGui::GetTextLineHeightWithSpacing() * 2.2f + 32.0f;
			ImGui::BeginChild("##PromoteCard", ImVec2(0.0f, card_h), true);
			ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.35f, 1.0f), "Classification: Ordinary N-Body Celestial Body");
			render_hint_text("Promote this body into an independent Kerr black hole with its own event horizon, capable of horizon absorption and mergers.");
			if (ImGui::Button("Promote to Independent Black Hole (Spacetime Source)", ImVec2(-1.0f, 26.0f))) {
				b.is_spacetime_source = true;
				b.radius = b.kerr_outer_horizon_radius();
				b.reference_radius = b.radius;
				apply_body_appearance_preset(b, Dynamics::Body3DPreset::BlackHole);
				b.enforce_spacetime_source_invariants();
				ctx.resolve_source_placement(b);
				ctx.select_and_open_source(static_cast<int>(b.id));
				changed = true;
			}
			render_setting_tooltip("Converts this body into a fully gravitating Kerr black hole. Its radius becomes the event horizon, its appearance switches to a black hole with optional accretion disk, and the inspector navigates to the Source Catalog.");
			ImGui::EndChild();
			ImGui::PopStyleColor();
		} else {
			ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.4f, 0.8f, 1.0f, 0.7f));
			const float card_h = ImGui::GetTextLineHeightWithSpacing() * 2.8f + 32.0f;
			ImGui::BeginChild("##DemoteCard", ImVec2(0.0f, card_h), true);
			ImGui::TextColored(ImVec4(1.0f, 0.65f, 0.4f, 1.0f), "Classification: Independent Kerr Spacetime Source");
			ImGui::TextDisabled("Outer Horizon: %.4f M | Kerr spin a/M: %.4f", b.kerr_outer_horizon_radius() / std::max(b.mass, 1e-9), b.kerr_spin_parameter());
			render_hint_text("Demote this black hole into an ordinary celestial body. Disables horizon absorption and transfers the body to the Object Catalog.");
			if (ImGui::Button("Demote to Ordinary Celestial Body", ImVec2(-1.0f, 26.0f))) {
				b.is_spacetime_source = false;
				b.radius = std::max(b.radius, 4.0 * b.mass);
				b.reference_radius = b.radius;
				apply_body_appearance_preset(b, Dynamics::Body3DPreset::Metallic);
				apply_body_material_preset(b, Dynamics::Body3DPreset::Metallic);
				ctx.select_and_open_object(static_cast<int>(b.id));
				changed = true;
			}
			render_setting_tooltip("Converts this black hole back into a solid body with physical surface collision parameters, and navigates to the Object Catalog.");
			ImGui::EndChild();
			ImGui::PopStyleColor();
		}
	}
	return changed;
}

[[nodiscard]] inline bool render_body_orientation_controls(Dynamics::PostNewtonianBody& b) noexcept {
	bool changed = false;
	constexpr double radians_to_degrees = 180.0 / std::numbers::pi;
	render_group_title("Orientation & Rotation", ImVec4(0.8f, 0.6f, 1.0f, 1.0f));

	const auto euler = b.orientation_euler_degrees();
	float roll = static_cast<float>(euler[0]);
	float pitch = static_cast<float>(euler[1]);
	float yaw = static_cast<float>(euler[2]);
	bool orientation_edited = ImGui::SliderFloat("Orientation Roll (X)", &roll, -180.0f, 180.0f, "%.1f deg");
	render_setting_tooltip("Rotation of the body around its X axis. Together with pitch and yaw it sets the attitude of the body in the world; the value updates live while the body spins.");
	orientation_edited = ImGui::SliderFloat("Orientation Pitch (Y)", &pitch, -90.0f, 90.0f, "%.1f deg") || orientation_edited;
	render_setting_tooltip("Rotation of the body around its Y axis, applied after roll. Tilting the polar axis of a spheroid reproduces an axial tilt.");
	orientation_edited = ImGui::SliderFloat("Orientation Yaw (Z)", &yaw, -180.0f, 180.0f, "%.1f deg") || orientation_edited;
	render_setting_tooltip("Rotation of the body around the world Z axis, applied last.");
	if (orientation_edited) {
		b.set_orientation_euler_degrees(static_cast<double>(roll), static_cast<double>(pitch), static_cast<double>(yaw));
		changed = true;
	}
	if (ImGui::SmallButton("Reset Orientation")) {
		b.orientation = {1.0, 0.0, 0.0, 0.0};
		changed = true;
	}
	render_setting_tooltip("Aligns the body axes with the world axes.");

	const auto axis = b.body_spin_axis_unit();
	float axis_polar = static_cast<float>(std::acos(std::clamp(axis[2], -1.0, 1.0)) * radians_to_degrees);
	float axis_azimuth = static_cast<float>(std::atan2(axis[1], axis[0]) * radians_to_degrees);
	bool axis_edited = ImGui::SliderFloat("Rotation Axis Polar Angle", &axis_polar, 0.0f, 180.0f, "%.1f deg");
	render_setting_tooltip("Angle between the rotation axis and the body Z axis. The axis is expressed in the body frame and turns with the body.");
	axis_edited = ImGui::SliderFloat("Rotation Axis Azimuth", &axis_azimuth, -180.0f, 180.0f, "%.1f deg") || axis_edited;
	render_setting_tooltip("Azimuth of the rotation axis around the body Z axis.");
	if (axis_edited) {
		const double polar = static_cast<double>(axis_polar) / radians_to_degrees;
		const double azimuth = static_cast<double>(axis_azimuth) / radians_to_degrees;
		b.rotation_axis_3d = {std::sin(polar) * std::cos(azimuth), std::sin(polar) * std::sin(azimuth), std::cos(polar)};
		changed = true;
	}

	auto omega = b.angular_velocity();
	if (ImGui::InputScalarN("Angular Velocity (rad/s)", ImGuiDataType_Double, omega.data(), 3, nullptr, nullptr, "%.6g")) {
		b.set_angular_velocity(omega);
		changed = true;
	}
	render_setting_tooltip("Angular velocity vector in the body frame. Its direction sets the rotation axis, its magnitude the spin rate, and the orientation is integrated from it at every simulation step.");
	return changed;
}

[[nodiscard]] inline bool render_body_physical_section(BodyManagerContext& ctx, Dynamics::PostNewtonianBody& b, BodyEditorViewState& view) noexcept {
	bool changed = false;
	auto& orchestrator = ctx.orchestrator();
	const auto& prefs = orchestrator.unit_preferences();
	const double mass_scale_kg = ctx.mass_scale();
	const double length_scale_m = ctx.length_scale();
	const double speed_scale_mps = ctx.speed_scale();

	render_hint_text("Mass and radius drive gravity, collisions and rendering. Position and velocity are expressed relative to the primary source at the origin.");

	render_group_title("Mass, Size & Charge", ImVec4(1.0f, 0.75f, 0.35f, 1.0f));
	double mass_kg = b.mass * mass_scale_kg;
	if (unit_aware_slider_double("Mass", &mass_kg, 0.001 * mass_scale_kg, 1.0e6 * mass_scale_kg, UnitCategory::Mass, prefs, "%.4f", &view.mass_log_mode, 1e-12 * mass_scale_kg, 1e60 * mass_scale_kg)) {
		b.mass = std::max(0.0, mass_kg / mass_scale_kg);
		changed = true;
	}
	render_setting_tooltip(("Gravitating mass of this body, displayed in " + std::string(Units::mass_unit_suffix(prefs.mass)) + ".").c_str());

	double radius_m = b.radius * length_scale_m;
	if (unit_aware_slider_double("Physical Radius", &radius_m, 0.001 * length_scale_m, 1.0e5 * length_scale_m, UnitCategory::Distance, prefs, "%.4f", &view.radius_log_mode, 1e-6 * length_scale_m, 1e50 * length_scale_m)) {
		b.radius = std::max(1e-6, radius_m / length_scale_m);
		changed = true;
	}
	render_setting_tooltip(("Visual and collision radius, displayed in " + std::string(Units::distance_unit_suffix(prefs.distance)) + ".").c_str());

	double charge = b.charge;
	if (unit_aware_slider_double("Charge", &charge, -10.0, 10.0, UnitCategory::Charge, prefs, "%.4e")) {
		b.charge = charge;
		changed = true;
	}
	render_setting_tooltip(("Net electric charge, displayed in " + std::string(Units::charge_unit_suffix(prefs.charge)) + ".").c_str());

	render_group_title("Motion", ImVec4(0.5f, 0.9f, 0.85f, 1.0f));
	if (coordinate_aware_input_position("Position", b.position, orchestrator)) {
		changed = true;
	}
	render_setting_tooltip("Position relative to the central object in the selected coordinate system. The system button switches between Cartesian, spherical, cylindrical and metric-adapted coordinates.");

	double velocity_mps[3] = {b.velocity[0] * speed_scale_mps, b.velocity[1] * speed_scale_mps, b.velocity[2] * speed_scale_mps};
	if (unit_aware_input_double3("Velocity (vx, vy, vz)", velocity_mps, UnitCategory::Velocity, prefs)) {
		b.velocity = {velocity_mps[0] / speed_scale_mps, velocity_mps[1] / speed_scale_mps, velocity_mps[2] / speed_scale_mps};
		changed = true;
	}
	render_setting_tooltip(("Instantaneous coordinate velocity of this body, displayed in " + std::string(Units::velocity_unit_suffix(prefs.velocity)) + ".").c_str());

	const double central_mass = orchestrator.parameters().mass;
	const auto hazard = ctx.evaluate_trajectory_hazard(b.position, b.velocity, b.mass, b.radius, central_mass);
	if (hazard.plunge_hazard) {
		render_caution_text("Plunge Trajectory: Periapsis is inside the central event horizon. This body will cross the horizon and be absorbed.");
	} else if (hazard.roche_hazard) {
		render_caution_text("Tidal Disruption Hazard: Periapsis is within the central Roche limit. Tidal forces will induce severe stress and fragmentation.");
	}

	const auto orbit = ctx.compute_keplerian_elements(b.position, b.velocity, central_mass);
	if (orbit.valid) {
		ImGui::Spacing();
		ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Osculating Orbital Elements:");
		ImGui::SameLine();
		const ImVec4 badge_col = orbit.is_bound ? ImVec4(0.2f, 0.85f, 0.4f, 1.0f) : ImVec4(1.0f, 0.45f, 0.35f, 1.0f);
		ImGui::TextColored(badge_col, "[%s | %s]", orbit.type_name, orbit.is_prograde ? "Prograde" : "Retrograde");

		if (orbit.is_bound) {
			ImGui::TextDisabled("Semi-Major Axis (a): %s", Units::format_distance(orbit.semi_major_axis * length_scale_m, prefs.distance).c_str());
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
		b.velocity = ctx.compute_circular_orbit_velocity(b.position, central_mass);
		changed = true;
	}
	render_setting_tooltip("Overwrites the velocity with the circular prograde Keplerian orbit velocity.");
	ImGui::SameLine();
	if (ImGui::Button("Circular Retrograde Orbit", ImVec2(185.0f, 24.0f))) {
		b.velocity = ctx.compute_retrograde_circular_orbit_velocity(b.position, central_mass);
		changed = true;
	}
	render_setting_tooltip("Overwrites the velocity with the circular retrograde Keplerian orbit velocity.");
	ImGui::SameLine();
	if (ImGui::Button("Escape Velocity", ImVec2(130.0f, 24.0f))) {
		b.velocity = ctx.compute_escape_velocity(b.position, b.velocity, central_mass);
		changed = true;
	}
	render_setting_tooltip("Scales speed to the exact local parabolic escape velocity.");

	float target_ecc = view.target_eccentricity;
	if (ImGui::SliderFloat("Target Eccentricity", &target_ecc, 0.0f, 2.0f, "%.3f")) {
		view.target_eccentricity = target_ecc;
	}
	render_setting_tooltip("Select a target eccentricity to re-scale the tangential velocity while keeping the current distance as periapsis.");
	ImGui::SameLine();
	if (ImGui::Button("Apply Eccentricity", ImVec2(140.0f, 24.0f))) {
		b.velocity = ctx.compute_target_eccentricity_velocity(b.position, b.velocity, central_mass, static_cast<double>(view.target_eccentricity));
		changed = true;
	}

	if (ImGui::InputScalarN("Spin Vector", ImGuiDataType_Double, b.spin.data(), 3, nullptr, nullptr, "%.6g")) {
		changed = true;
	}
	render_setting_tooltip("Intrinsic angular momentum vector, feeding spin-orbit and spin-spin post-Newtonian coupling terms.");

	changed = render_relative_reference_and_orbital_plane_tool(ctx, b, view) || changed;

	changed = render_frenet_serret_velocity_injector(ctx, b, view) || changed;

	if (render_body_orientation_controls(b)) {
		changed = true;
	}

	const double kinetic_energy_joules = b.kinetic_energy() * mass_scale_kg * speed_scale_mps * speed_scale_mps;
	ImGui::Spacing();
	ImGui::TextDisabled("Speed: %s", Units::format_velocity(b.speed() * speed_scale_mps, prefs.velocity).c_str());
	ImGui::TextDisabled("Kinetic Energy: %s", Units::format_energy(kinetic_energy_joules, prefs.energy).c_str());
	return changed;
}

[[nodiscard]] inline bool render_body_multipole_section(BodyManagerContext& ctx, Dynamics::PostNewtonianBody& b, BodyEditorViewState& view) noexcept {
	bool changed = false;
	render_hint_text("Zonal harmonics describe the deviation of the gravitational field from a perfect sphere and flatten the rendered spheroid.");
	changed = body_editor_slider("Quadrupole Moment (Q)", b.quadrupole_moment, -1e-2f, 1e-2f, "%.6e") || changed;
	render_setting_tooltip("Quadrupole deformation parameter for tidal and multipolar force models.");
	changed = body_editor_slider("Zonal J2 Moment", b.j2, -1e-2f, 1e-2f, "%.6e") || changed;
	render_setting_tooltip("Dominant oblateness harmonic coefficient, also flattening the rendered spheroid and producing nodal precession on other bodies passing nearby.");
	changed = body_editor_slider("Zonal J3 Moment", b.j3, -1e-3f, 1e-3f, "%.6e") || changed;
	render_setting_tooltip("Third-degree zonal harmonic coefficient, primarily contributing a north-south asymmetric perturbation.");
	changed = body_editor_slider("Zonal J4 Moment", b.j4, -1e-3f, 1e-3f, "%.6e") || changed;
	render_setting_tooltip("Fourth-degree zonal harmonic coefficient, a smaller correction to the oblateness perturbation.");

	const auto& prefs = ctx.orchestrator().unit_preferences();
	const double length_scale_m = ctx.length_scale();
	double reference_radius_m = b.reference_radius * length_scale_m;
	if (unit_aware_slider_double("Multipole Reference Radius", &reference_radius_m, 0.001 * length_scale_m, 1.0e5 * length_scale_m, UnitCategory::Distance, prefs, "%.4f", &view.reference_radius_log_mode, 1e-6 * length_scale_m, 1e50 * length_scale_m)) {
		b.reference_radius = std::max(1e-6, reference_radius_m / length_scale_m);
		changed = true;
	}
	render_setting_tooltip(("Reference radius, displayed in " + std::string(Units::distance_unit_suffix(prefs.distance)) + ", at which the zonal harmonic coefficients are defined, typically the body's equatorial radius.").c_str());
	return changed;
}

[[nodiscard]] inline bool render_body_material_section(BodyManagerContext& ctx, Dynamics::PostNewtonianBody& b, BodyEditorViewState& view) noexcept {
	bool changed = false;
	const auto& prefs = ctx.orchestrator().unit_preferences();

	render_hint_text("Material properties feed the electromagnetic, collision, thermodynamic and fragmentation models enabled in the Interactions tab.");

	render_group_title("Electromagnetic & Rotation", ImVec4(0.3f, 0.9f, 1.0f, 1.0f));
	changed = body_editor_slider("Magnetic Moment", b.magnetic_moment, 1e-6f, 1.0e6f, "%.4e", &view.magnetic_moment_log_mode, 1e-9f, 1e9f) || changed;
	render_setting_tooltip("Magnetic dipole moment used by the dipole-dipole force when magnetism is enabled.");

	if (unit_aware_slider_double("Rotation Speed", &b.rotation_speed, 1e-6, 1.0e6, UnitCategory::AngularVelocity, prefs, "%.4e", &view.rotation_speed_log_mode, 1e-9, 1e9)) {
		changed = true;
	}
	render_setting_tooltip("Physical spin rate used by collision friction torque, independent from the visual 3D rotation speed.");

	render_group_title("Collision & Integrity", ImVec4(0.9f, 0.8f, 0.2f, 1.0f));
	changed = body_editor_slider("Friction Coefficient", b.friction_coefficient, 0.0f, 1.0f, "%.3f") || changed;
	changed = body_editor_slider("Restitution", b.restitution, 0.0f, 1.0f, "%.3f") || changed;
	changed = body_editor_slider("Elasticity", b.elasticity, 0.0f, 1.0f, "%.3f") || changed;
	changed = body_editor_slider("Integrity", b.integrity, 0.0f, 1.0f, "%.3f") || changed;
	b.integrity = std::max(0.0, b.integrity);
	render_setting_tooltip("Structural integrity reserve. Collision impacts and tidal stress erode it, and the body fragments once it reaches zero.");

	if (unit_aware_slider_double("Lifetime", &b.lifetime, 1e-6, 1.0e9, UnitCategory::Time, prefs, "%.4e", &view.lifetime_log_mode, 1e-6, 1e12)) {
		b.lifetime = std::max(0.0, b.lifetime);
		changed = true;
	}

	render_group_title("Thermal Properties", ImVec4(1.0f, 0.6f, 0.3f, 1.0f));
	if (unit_aware_slider_double("Temperature", &b.temperature, 0.0, 50000.0, UnitCategory::Temperature, prefs, "%.2f")) {
		b.temperature = std::max(0.0, b.temperature);
		changed = true;
	}
	changed = body_editor_slider("Heat Capacity", b.heat_capacity, 1e-6f, 1.0e9f, "%.4e", &view.heat_capacity_log_mode, 1e-6f, 1e12f) || changed;
	b.heat_capacity = std::max(0.0, b.heat_capacity);
	changed = body_editor_slider("Absorption Factor", b.absorption_factor, 0.0f, 1.0f, "%.3f") || changed;
	render_setting_tooltip("Fraction of incident radiative energy absorbed by the body, scaling the Stefan-Boltzmann exchange with the ambient temperature.");
	changed = body_editor_slider("Transmission Factor", b.transmission_factor, 0.0f, 1.0f, "%.3f") || changed;
	if (unit_aware_slider_double("Critical Temperature", &b.critical_temperature, 0.0, 50000.0, UnitCategory::Temperature, prefs, "%.2f")) {
		b.critical_temperature = std::max(0.0, b.critical_temperature);
		changed = true;
	}
	render_setting_tooltip("Temperature above which the hot Young's modulus applies instead of the cold one. Zero disables the transition.");
	changed = body_editor_slider("Cold Resistance", b.cold_resistance, 0.0f, 1.0e6f, "%.4e", &view.resistance_log_mode, 1e-6f, 1e9f) || changed;
	changed = body_editor_slider("Hot Resistance", b.hot_resistance, 0.0f, 1.0e6f, "%.4e", &view.resistance_log_mode, 1e-6f, 1e9f) || changed;

	render_group_title("Mechanical Stiffness", ImVec4(0.6f, 0.9f, 0.6f, 1.0f));
	changed = body_editor_slider("Young's Modulus (Cold)", b.youngs_modulus_cold, 0.0f, 1.0e12f, "%.4e", &view.youngs_modulus_log_mode, 1e-3f, 1e15f) || changed;
	changed = body_editor_slider("Young's Modulus (Hot)", b.youngs_modulus_hot, 0.0f, 1.0e12f, "%.4e", &view.youngs_modulus_log_mode, 1e-3f, 1e15f) || changed;
	render_setting_tooltip("Material stiffness driving the Hertzian contact repulsion when collisions are enabled. A value of zero disables stiffness pushback for the body.");

	char composition[32]{};
	std::memcpy(composition, b.composition.data(), b.composition.size() - 1);
	if (ImGui::InputText("Composition", composition, sizeof(composition))) {
		b.set_composition(composition);
		changed = true;
	}
	render_setting_tooltip("Free-form material tag used by the physical intelligence color coding.");
	return changed;
}

[[nodiscard]] inline bool render_earth_texture_controls(Dynamics::PostNewtonianBody& b) noexcept {
	bool changed = false;

	int variant_idx = std::min(static_cast<int>(b.earth_map_variant), static_cast<int>(kEarthMapVariantNames.size()) - 1);
	if (ImGui::Combo("Earth Map Version", &variant_idx, kEarthMapVariantNames.data(), static_cast<int>(kEarthMapVariantNames.size()))) {
		b.earth_map_variant = Optics::earth_map_variant_from_index(static_cast<uint32_t>(variant_idx));
		changed = true;
	}
	render_setting_tooltip("Day shows the sunlit photographic map, Night shows the city-lights map as self-illumination, and Automatic masks both by the lit side of the body for a continuous blue-marble transition.");

	int quality_idx = std::min(static_cast<int>(b.earth_map_quality), static_cast<int>(kEarthMapQualityNames.size()) - 1);
	if (ImGui::Combo("Earth Map Quality", &quality_idx, kEarthMapQualityNames.data(), static_cast<int>(kEarthMapQualityNames.size()))) {
		b.earth_map_quality = Optics::earth_map_quality_from_index(static_cast<uint32_t>(quality_idx));
		changed = true;
	}
	render_setting_tooltip("Resolution of the Earth images decoded and uploaded to the GPU. When several Earth bodies request different qualities, the highest one is used on the GPU.");

	if (b.earth_map_variant == Optics::EarthMapVariant::Automatic) {
		if (ImGui::SliderFloat("Terminator Blend Softness", &b.earth_terminator_softness, 0.0f, 1.0f, "%.2f")) {
			changed = true;
		}
		render_setting_tooltip("Width of the transition between the day and night images around the terminator. Low values give a sharp line, high values hide the mask with a wide twilight blend.");
	}

	render_hint_text("While the Earth texture is active, colors, noise, roughness, detail, polar caps, city lights and ring shadow are locked. Surface texture layers remain fully editable so clouds and other details can still be added.");
	return changed;
}

[[nodiscard]] inline bool render_base_surface_tab(Dynamics::PostNewtonianBody& b) noexcept {
	bool changed = false;

	int geometry_idx = std::min(static_cast<int>(b.geometry_model), static_cast<int>(kBodyGeometryNames.size()) - 1);
	if (ImGui::Combo("3D Geometry Model", &geometry_idx, kBodyGeometryNames.data(), static_cast<int>(kBodyGeometryNames.size()))) {
		b.geometry_model = static_cast<Dynamics::Body3DGeometryModel>(geometry_idx);
		changed = true;
	}
	render_setting_tooltip("Defines the ray-traced 3D shape. Oblate and prolate spheroids deform from the J2 moment, and the triaxial ellipsoid adds a flattened second axis.");

	const bool earth_texture_active = (b.surface_texture_mode == Dynamics::Body3DSurfaceTextureMode::EarthBlueMarble);
	ImGui::BeginDisabled(earth_texture_active);
	int preset_idx = std::min(static_cast<int>(b.preset_3d), static_cast<int>(kBodySurfacePresetNames.size()) - 1);
	if (ImGui::Combo("Surface Preset", &preset_idx, kBodySurfacePresetNames.data(), static_cast<int>(kBodySurfacePresetNames.size()))) {
		apply_body_appearance_preset(b, static_cast<Dynamics::Body3DPreset>(preset_idx));
		changed = true;
	}
	render_setting_tooltip("Applies a matched base texture, palette, roughness, emission and atmosphere profile for the chosen body family. Surface texture layers are preserved.");
	ImGui::SameLine();
	if (ImGui::SmallButton("Reapply")) {
		apply_body_appearance_preset(b, b.preset_3d);
		changed = true;
	}
	render_setting_tooltip("Restores the base appearance parameters of the current preset without touching texture layers.");
	ImGui::EndDisabled();
	if (earth_texture_active) {
		render_hint_text("Surface presets are locked while the Earth texture is active; change the Base Texture Mode to leave it.");
	}

	int texture_idx = std::min(static_cast<int>(b.surface_texture_mode), static_cast<int>(kBodyTextureModeNames.size()) - 1);
	if (ImGui::Combo("Base Texture Mode", &texture_idx, kBodyTextureModeNames.data(), static_cast<int>(kBodyTextureModeNames.size()))) {
		b.surface_texture_mode = static_cast<Dynamics::Body3DSurfaceTextureMode>(texture_idx);
		changed = true;
	}
	render_hint_text(body_texture_mode_description(static_cast<uint32_t>(texture_idx)));

	if (earth_texture_active) {
		return render_earth_texture_controls(b) || changed;
	}

	if (ImGui::ColorEdit4("Primary Color", b.color.data())) changed = true;
	if (ImGui::ColorEdit4("Secondary Color", b.color_secondary.data())) changed = true;
	if (ImGui::ColorEdit4("Tertiary Color", b.color_tertiary.data())) changed = true;
	render_setting_tooltip("Third accent color used by Marbled, Ringed, Icy, Volcanic and Nebulous modes and by polar caps.");
	if (ImGui::SliderFloat("Surface Noise Scale", &b.surface_noise_scale, 0.5f, 20.0f, "%.2f")) changed = true;
	if (ImGui::SliderFloat("Surface Roughness", &b.surface_roughness, 0.05f, 1.0f, "%.2f")) changed = true;
	if (ImGui::SliderFloat("Texture Detail Scale", &b.texture_detail_scale, 0.1f, 5.0f, "%.2fx")) changed = true;
	render_setting_tooltip("Multiplies the frequency of secondary surface patterns such as marble veining or gas giant banding.");
	if (ImGui::SliderFloat("Polar Cap Strength", &b.polar_cap_strength, 0.0f, 1.0f, "%.2f")) changed = true;
	render_setting_tooltip("Blends the tertiary color over the poles to depict ice caps or polar storm bands.");
	if (ImGui::SliderFloat("Night Side City Lights", &b.night_side_light_intensity, 0.0f, 2.0f, "%.2f")) changed = true;
	render_setting_tooltip("Adds a speckled glow on the unlit hemisphere.");
	if (ImGui::Checkbox("Ring System (Equatorial Shadow Band)", &b.ring_system_enabled)) changed = true;
	render_setting_tooltip("Darkens a thin equatorial band to suggest a shadow cast by an orbiting ring plane.");
	return changed;
}

[[nodiscard]] inline bool render_atmosphere_lighting_tab(Dynamics::PostNewtonianBody& b) noexcept {
	bool changed = false;
	int atmosphere_idx = std::min(static_cast<int>(b.atmosphere_mode), static_cast<int>(kBodyAtmosphereModeNames.size()) - 1);
	if (ImGui::Combo("Atmosphere Mode", &atmosphere_idx, kBodyAtmosphereModeNames.data(), static_cast<int>(kBodyAtmosphereModeNames.size()))) {
		b.atmosphere_mode = static_cast<Dynamics::Body3DAtmosphereMode>(atmosphere_idx);
		changed = true;
	}
	render_setting_tooltip("Selects the rim-lit atmospheric glow drawn around the silhouette. Off disables the effect entirely.");
	if (ImGui::SliderFloat("Atmosphere Thickness", &b.atmosphere_thickness, 0.0f, 0.5f, "%.3f")) changed = true;
	if (ImGui::ColorEdit4("Atmosphere Color", b.atmosphere_color.data())) changed = true;
	if (ImGui::SliderFloat("Emission Intensity", &b.emission_intensity, 0.0f, 5.0f, "%.2f")) changed = true;
	render_setting_tooltip("Self-illumination added on top of the lit surface, required for stars and compact remnants.");
	if (ImGui::SliderFloat("Specular Roughness", &b.specular_roughness, 0.05f, 1.0f, "%.2f")) changed = true;
	if (ImGui::SliderFloat("3D Rotation Speed", &b.rotation_speed_3d, -10.0f, 10.0f, "%.3f rad/s")) changed = true;
	render_setting_tooltip("Angular rate around the rotation axis. It advances the body orientation every simulation step; negative values reverse the spin.");
	return changed;
}

[[nodiscard]] inline bool render_surface_layer_editor(Dynamics::BodySurfaceLayerSet& set, BodyEditorViewState& view) noexcept {
	bool edited = false;
	const bool full = set.count >= Render::kMaxSurfaceLayers;
	ImGui::TextDisabled("Layers: %u / %zu (drawn first to last, on top of the base texture)", set.count, Render::kMaxSurfaceLayers);
	ImGui::SetNextItemWidth(220.0f);
	ImGui::Combo("Template", &view.layer_template_choice, Dynamics::kSurfaceLayerTemplateNames.data(), static_cast<int>(Dynamics::kSurfaceLayerTemplateNames.size()));
	if (full) ImGui::BeginDisabled(true);
	if (ImGui::Button("Add Template Layer", ImVec2(160.0f, 24.0f))) {
		edited = set.add(Dynamics::SurfaceLayerDefinition::from_template(static_cast<Dynamics::SurfaceLayerTemplate>(view.layer_template_choice))) || edited;
	}
	ImGui::SameLine();
	if (ImGui::Button("Add Blank Layer", ImVec2(140.0f, 24.0f))) {
		edited = set.add(Dynamics::SurfaceLayerDefinition{}) || edited;
	}
	if (full) ImGui::EndDisabled();
	ImGui::SameLine();
	if (ImGui::Button("Clear All Layers", ImVec2(130.0f, 24.0f)) && set.count > 0U) {
		set = Dynamics::BodySurfaceLayerSet{};
		edited = true;
	}
	render_setting_tooltip("Adds a procedural texture layer blended over the base surface. Each layer has its own pattern, blend mode, region mask, color, scale, drift and emission.");

	int remove_index = -1;
	int move_index = -1;
	int move_delta = 0;
	for (uint32_t i = 0; i < set.count; ++i) {
		auto& layer = set.layers[i];
		ImGui::PushID(static_cast<int>(i));
		const std::string header = std::to_string(i + 1U) + ". " + Dynamics::kSurfaceLayerPatternNames[static_cast<size_t>(layer.pattern)] + " - " + Dynamics::kSurfaceLayerMaskNames[static_cast<size_t>(layer.mask)] + "###surface_layer_header";
		if (ImGui::CollapsingHeader(header.c_str())) {
			if (ImGui::Checkbox("Enabled", &layer.enabled)) edited = true;
			ImGui::SameLine();
			if (ImGui::SmallButton("Move Up")) { move_index = static_cast<int>(i); move_delta = -1; }
			ImGui::SameLine();
			if (ImGui::SmallButton("Move Down")) { move_index = static_cast<int>(i); move_delta = 1; }
			ImGui::SameLine();
			if (ImGui::SmallButton("Remove")) { remove_index = static_cast<int>(i); }

			int pattern_idx = static_cast<int>(layer.pattern);
			if (ImGui::Combo("Pattern", &pattern_idx, Dynamics::kSurfaceLayerPatternNames.data(), static_cast<int>(Dynamics::kSurfaceLayerPatternNames.size()))) {
				layer.pattern = static_cast<Render::SurfaceLayerPattern>(pattern_idx);
				edited = true;
			}
			int blend_idx = static_cast<int>(layer.blend);
			if (ImGui::Combo("Blend Mode", &blend_idx, Dynamics::kSurfaceLayerBlendNames.data(), static_cast<int>(Dynamics::kSurfaceLayerBlendNames.size()))) {
				layer.blend = static_cast<Render::SurfaceLayerBlend>(blend_idx);
				edited = true;
			}
			int mask_idx = static_cast<int>(layer.mask);
			if (ImGui::Combo("Region Mask", &mask_idx, Dynamics::kSurfaceLayerMaskNames.data(), static_cast<int>(Dynamics::kSurfaceLayerMaskNames.size()))) {
				layer.mask = static_cast<Render::SurfaceLayerMask>(mask_idx);
				edited = true;
			}
			if (ImGui::ColorEdit3("Layer Color", layer.color.data())) edited = true;
			if (ImGui::SliderFloat("Opacity", &layer.opacity, 0.0f, 1.0f, "%.2f")) edited = true;
			if (ImGui::SliderFloat("Pattern Scale", &layer.scale, 0.5f, 30.0f, "%.2f")) edited = true;
			if (ImGui::SliderFloat("Contrast", &layer.contrast, 0.2f, 4.0f, "%.2f")) edited = true;
			if (ImGui::SliderFloat("Threshold", &layer.threshold, 0.0f, 0.95f, "%.2f")) edited = true;
			if (ImGui::SliderFloat("Edge Softness", &layer.softness, 0.01f, 1.0f, "%.2f")) edited = true;
			if (layer.mask != Render::SurfaceLayerMask::Global) {
				if (ImGui::SliderFloat("Mask Width", &layer.mask_width, 0.05f, 1.0f, "%.2f")) edited = true;
			}
			if (ImGui::SliderFloat("Drift Speed", &layer.rotation_factor, -2.0f, 2.0f, "%.2f")) edited = true;
			if (ImGui::SliderFloat("Emission", &layer.emission, 0.0f, 4.0f, "%.2f")) edited = true;
			int octave_value = static_cast<int>(layer.octaves);
			if (ImGui::SliderInt("Detail Octaves", &octave_value, 1, 6)) {
				layer.octaves = static_cast<uint32_t>(octave_value);
				edited = true;
			}
			int seed_value = static_cast<int>(layer.seed);
			if (ImGui::InputInt("Seed", &seed_value)) {
				layer.seed = static_cast<uint32_t>(std::max(seed_value, 0));
				edited = true;
			}
		}
		ImGui::PopID();
	}

	if (move_index >= 0) {
		edited = set.move(static_cast<uint32_t>(move_index), move_delta) || edited;
	}
	if (remove_index >= 0) {
		edited = set.remove(static_cast<uint32_t>(remove_index)) || edited;
	}
	return edited;
}

inline void render_body_surface_section(Dynamics::PostNewtonianBody& b, Dynamics::BodySurfaceLayerSet& layers, BodyEditorViewState& view, BodyEditResult& result) noexcept {
	const size_t texture_idx = std::min(static_cast<size_t>(b.surface_texture_mode), kBodyTextureModeNames.size() - 1);
	ImGui::TextDisabled("Composition: %s, then %u layer(s), atmosphere and relativistic effects", kBodyTextureModeNames[texture_idx], layers.count);
	if (ImGui::BeginTabBar("SurfaceEditorTabs")) {
		if (ImGui::BeginTabItem("Base Surface")) {
			result.body_changed = render_base_surface_tab(b) || result.body_changed;
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Atmosphere & Lighting")) {
			result.body_changed = render_atmosphere_lighting_tab(b) || result.body_changed;
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Texture Layers")) {
			result.layers_changed = render_surface_layer_editor(layers, view) || result.layers_changed;
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
}

[[nodiscard]] inline BodyEditResult render_body_editor(BodyManagerContext& ctx, Dynamics::PostNewtonianBody& b, Dynamics::BodySurfaceLayerSet* layers, BodyEditorViewState& view, uint32_t sections) noexcept {
	BodyEditResult result;
	if (b.is_spacetime_source) {
		sections &= ~(BodyEditorSection::Multipoles | BodyEditorSection::Material | BodyEditorSection::Surface);
	} else {
		sections &= ~BodyEditorSection::AccretionDisk;
	}
	if ((sections & BodyEditorSection::Identity) != 0U && ImGui::CollapsingHeader("Identity & State", ImGuiTreeNodeFlags_DefaultOpen)) {
		result.body_changed = render_body_identity_section(ctx, b, (sections & BodyEditorSection::SourceToggle) != 0U) || result.body_changed;
	}
	if ((sections & BodyEditorSection::Physical) != 0U && ImGui::CollapsingHeader(b.is_spacetime_source ? "Spacetime & Metric Parameters" : "Physical State & Motion", ImGuiTreeNodeFlags_DefaultOpen)) {
		result.body_changed = (b.is_spacetime_source ? render_source_parameters_section(ctx, b, view) : render_body_physical_section(ctx, b, view)) || result.body_changed;
	}
	if ((sections & BodyEditorSection::AccretionDisk) != 0U && ImGui::CollapsingHeader("Accretion Disk", ImGuiTreeNodeFlags_DefaultOpen)) {
		result.body_changed = render_source_disk_section(b, view) || result.body_changed;
	}
	if ((sections & BodyEditorSection::Multipoles) != 0U && ImGui::CollapsingHeader("Gravitational Multipoles")) {
		result.body_changed = render_body_multipole_section(ctx, b, view) || result.body_changed;
	}
	if ((sections & BodyEditorSection::Material) != 0U && ImGui::CollapsingHeader("Material, Thermal & Electromagnetic")) {
		result.body_changed = render_body_material_section(ctx, b, view) || result.body_changed;
	}
	if ((sections & BodyEditorSection::Surface) != 0U && layers != nullptr && ImGui::CollapsingHeader("Surface Appearance & Layers")) {
		render_body_surface_section(b, *layers, view, result);
	}
	return result;
}

inline bool render_batch_object_editor(
	BodyManagerContext& ctx,
	Dynamics::PostNewtonianSystem& sys,
	std::vector<Dynamics::PostNewtonianBody*>& bodies,
	BodyEditorViewState& view
) noexcept {
	if (bodies.empty()) return false;
	bool changed = false;
	const auto& prefs = ctx.orchestrator().unit_preferences();
	const double mass_scale_kg = ctx.mass_scale();
	const double length_scale_m = ctx.length_scale();
	const double speed_scale_mps = ctx.speed_scale();

	size_t enabled_count = 0;
	double total_mass = 0.0;
	std::array<double, 3> centroid{0.0, 0.0, 0.0};
	double total_weight = 0.0;
	for (const auto* b : bodies) {
		if (b->enabled) ++enabled_count;
		total_mass += b->mass;
		const double w = std::max(b->mass, 1e-6);
		centroid[0] += b->position[0] * w;
		centroid[1] += b->position[1] * w;
		centroid[2] += b->position[2] * w;
		total_weight += w;
	}
	if (total_weight > 0.0) {
		centroid[0] /= total_weight;
		centroid[1] /= total_weight;
		centroid[2] /= total_weight;
	}

	ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Batch Inspector: %zu Objects Selected (%zu enabled)", bodies.size(), enabled_count);
	ImGui::TextDisabled("Total Mass: %s | Centroid Position: (%.2e, %.2e, %.2e)", Units::format_mass(total_mass * mass_scale_kg, prefs.mass, 3).c_str(), centroid[0], centroid[1], centroid[2]);

	if (ImGui::Button("Look At Centroid", ImVec2(120.0f, 24.0f))) {
		ctx.look_at_selection(bodies);
	}
	render_setting_tooltip("Orients the camera gaze toward the center of mass of the current selection.");
	ImGui::SameLine();
	if (ImGui::Button("Enable All", ImVec2(85.0f, 24.0f))) {
		for (auto* b : bodies) b->enabled = true;
		changed = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("Disable All", ImVec2(90.0f, 24.0f))) {
		for (auto* b : bodies) b->enabled = false;
		changed = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("Circularize All", ImVec2(110.0f, 24.0f))) {
		const double central_mass = ctx.orchestrator().parameters().mass;
		for (auto* b : bodies) {
			b->velocity = ctx.compute_circular_orbit_velocity(b->position, central_mass + (b->is_spacetime_source ? b->mass : 0.0));
		}
		changed = true;
	}

	static int batch_staging_mode = 0;
	ImGui::RadioButton("Absolute Value Staging", &batch_staging_mode, 0);
	ImGui::SameLine();
	ImGui::RadioButton("Relative Delta & Scaling", &batch_staging_mode, 1);
	render_setting_tooltip("Choose whether controls set uniform absolute values across the selection or apply relative offsets/deltas to each body's current value.");

	if (batch_staging_mode == 1) {
		render_group_title("Relative Transformations & Deltas", ImVec4(1.0f, 0.7f, 0.35f, 1.0f));

		static float delta_mass_scale = 1.0f;
		static float delta_mass_add_kg = 0.0f;
		ImGui::InputFloat("Mass Multiplier (*)", &delta_mass_scale, 0.0f, 0.0f, "%.4gx");
		ImGui::SameLine();
		if (ImGui::Button("Scale Masses##Batch")) {
			for (auto* b : bodies) b->mass = std::max(b->mass * static_cast<double>(delta_mass_scale), 0.0);
			changed = true;
		}
		ImGui::InputFloat("Mass Additive Delta (kg)", &delta_mass_add_kg, 0.0f, 0.0f, "%.4e");
		ImGui::SameLine();
		if (ImGui::Button("Add Mass Delta##Batch")) {
			for (auto* b : bodies) b->mass = std::max(b->mass + static_cast<double>(delta_mass_add_kg) / mass_scale_kg, 0.0);
			changed = true;
		}

		static float delta_radius_scale = 1.0f;
		ImGui::InputFloat("Radius Multiplier (*)", &delta_radius_scale, 0.0f, 0.0f, "%.4gx");
		ImGui::SameLine();
		if (ImGui::Button("Scale Radii##Batch")) {
			for (auto* b : bodies) b->radius = std::max(b->radius * static_cast<double>(delta_radius_scale), 1e-6);
			changed = true;
		}

		static float delta_pos[3]{0.0f, 0.0f, 0.0f};
		ImGui::InputFloat3("Position Shift (dx, dy, dz)", delta_pos);
		ImGui::SameLine();
		if (ImGui::Button("Shift Positions##Batch")) {
			for (auto* b : bodies) {
				b->position[0] += static_cast<double>(delta_pos[0]);
				b->position[1] += static_cast<double>(delta_pos[1]);
				b->position[2] += static_cast<double>(delta_pos[2]);
			}
			changed = true;
		}

		static float delta_vel[3]{0.0f, 0.0f, 0.0f};
		ImGui::InputFloat3("Velocity Delta (dvx, dvy, dvz)", delta_vel);
		ImGui::SameLine();
		if (ImGui::Button("Apply Velocity Delta##Batch")) {
			for (auto* b : bodies) {
				b->velocity[0] += static_cast<double>(delta_vel[0]);
				b->velocity[1] += static_cast<double>(delta_vel[1]);
				b->velocity[2] += static_cast<double>(delta_vel[2]);
			}
			changed = true;
		}

		static float speed_factor = 1.0f;
		ImGui::InputFloat("Speed Multiplier (*)", &speed_factor, 0.0f, 0.0f, "%.4gx");
		ImGui::SameLine();
		if (ImGui::Button("Scale Speeds##Batch")) {
			for (auto* b : bodies) {
				b->velocity[0] *= static_cast<double>(speed_factor);
				b->velocity[1] *= static_cast<double>(speed_factor);
				b->velocity[2] *= static_cast<double>(speed_factor);
			}
			changed = true;
		}

		static float delta_temp_k = 0.0f;
		ImGui::InputFloat("Temperature Delta (K)", &delta_temp_k, 0.0f, 0.0f, "%.2f");
		ImGui::SameLine();
		if (ImGui::Button("Apply Temperature Delta##Batch")) {
			for (auto* b : bodies) b->temperature = std::max(b->temperature + static_cast<double>(delta_temp_k), 0.0);
			changed = true;
		}
	}

	if (ImGui::CollapsingHeader("Identity & Batch Renaming", ImGuiTreeNodeFlags_DefaultOpen)) {
		const bool all_enabled = batch_are_equal_exact(bodies, [](const auto& b) { return b.enabled; });
		if (all_enabled) {
			bool en = bodies.front()->enabled;
			if (ImGui::Checkbox("Enabled (All)", &en)) {
				for (auto* b : bodies) b->enabled = en;
				changed = true;
			}
		} else {
			ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "State: * (Mixed: %zu enabled, %zu disabled)", enabled_count, bodies.size() - enabled_count);
			ImGui::SameLine();
			if (ImGui::SmallButton("Set All Enabled")) {
				for (auto* b : bodies) b->enabled = true;
				changed = true;
			}
			ImGui::SameLine();
			if (ImGui::SmallButton("Set All Disabled")) {
				for (auto* b : bodies) b->enabled = false;
				changed = true;
			}
		}

		static char batch_prefix[32] = "";
		static char batch_suffix[32] = "";
		static char batch_base_name[32] = "Entity";
		ImGui::InputTextWithHint("##BatchPrefix", "Prefix...", batch_prefix, sizeof(batch_prefix));
		ImGui::SameLine();
		if (ImGui::Button("Add Prefix", ImVec2(90.0f, 24.0f)) && batch_prefix[0] != '\0') {
			for (auto* b : bodies) {
				b->set_name(std::string(batch_prefix) + " " + BodyManagerContext::display_name(*b));
			}
			changed = true;
		}
		ImGui::InputTextWithHint("##BatchSuffix", "Suffix...", batch_suffix, sizeof(batch_suffix));
		ImGui::SameLine();
		if (ImGui::Button("Add Suffix", ImVec2(90.0f, 24.0f)) && batch_suffix[0] != '\0') {
			for (auto* b : bodies) {
				b->set_name(BodyManagerContext::display_name(*b) + " " + std::string(batch_suffix));
			}
			changed = true;
		}
		ImGui::InputTextWithHint("##BatchBase", "Base name...", batch_base_name, sizeof(batch_base_name));
		ImGui::SameLine();
		if (ImGui::Button("Numbered Rename", ImVec2(130.0f, 24.0f)) && batch_base_name[0] != '\0') {
			for (size_t i = 0; i < bodies.size(); ++i) {
				bodies[i]->set_name(std::string(batch_base_name) + " " + std::to_string(i + 1));
			}
			changed = true;
		}

		if (ImGui::Button("Promote All Selected to Independent Black Holes", ImVec2(-1.0f, 26.0f))) {
			for (auto* b : bodies) {
				b->is_spacetime_source = true;
				b->radius = b->kerr_outer_horizon_radius();
				b->reference_radius = b->radius;
				apply_body_appearance_preset(*b, Dynamics::Body3DPreset::BlackHole);
				b->enforce_spacetime_source_invariants();
				ctx.resolve_source_placement(*b);
			}
			std::vector<int> src_ids;
			src_ids.reserve(bodies.size());
			for (const auto* b : bodies) src_ids.push_back(static_cast<int>(b->id));
			ctx.set_selected_sources(src_ids);
			ctx.request_tab(BodyManagerTab::SourceCatalog);
			changed = true;
		}
	}

	if (ImGui::CollapsingHeader("Batch Orbital Plane & Reference Alignment", ImGuiTreeNodeFlags_DefaultOpen)) {
		render_group_title("Reference Attractor Selection", ImVec4(0.4f, 0.85f, 1.0f, 1.0f));
		render_hint_text("Select a common reference origin to evaluate or reorient the orbital planes of all selected bodies simultaneously.");

		std::vector<int> b_ref_ids{BodyManagerContext::kCentralSourceId};
		std::vector<std::string> b_ref_labels{"Central Spacetime Attractor (Origin #0)"};
		for (const auto& other : sys.bodies()) {
			if (!other.enabled) continue;
			bool in_selection = false;
			for (const auto* sel : bodies) {
				if (sel->id == other.id) { in_selection = true; break; }
			}
			if (in_selection) continue;
			b_ref_ids.push_back(static_cast<int>(other.id));
			b_ref_labels.push_back(BodyManagerContext::display_name(other) + " (#" + std::to_string(other.id) + ")");
		}

		int b_ref_combo = 0;
		for (size_t i = 0; i < b_ref_ids.size(); ++i) {
			if (b_ref_ids[i] == view.reference_frame_body_id) {
				b_ref_combo = static_cast<int>(i);
				break;
			}
		}

		std::vector<const char*> b_ref_ptrs;
		b_ref_ptrs.reserve(b_ref_labels.size());
		for (const auto& s : b_ref_labels) b_ref_ptrs.push_back(s.c_str());

		if (ImGui::Combo("Common Reference Body##Batch", &b_ref_combo, b_ref_ptrs.data(), static_cast<int>(b_ref_ptrs.size()))) {
			view.reference_frame_body_id = b_ref_ids[static_cast<size_t>(b_ref_combo)];
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

		static float batch_uniform_inc = 0.0f;
		if (slider_float_with_input("Set Uniform Inclination (All Selected)", &batch_uniform_inc, 0.0f, 180.0f, "%.2f deg")) {
			for (auto* b_item : bodies) {
				const auto st = ctx.compute_relative_orbital_plane_state(*b_item, view.reference_frame_body_id);
				ctx.apply_relative_orbital_plane_rotation(*b_item, view.reference_frame_body_id, static_cast<double>(batch_uniform_inc), st.relative_node_deg, st.relative_periapsis_deg);
			}
			changed = true;
		}

		static float batch_delta_inc = 0.0f;
		ImGui::InputFloat("Inclination Delta (+/- deg)", &batch_delta_inc, 0.0f, 0.0f, "%.2f deg");
		ImGui::SameLine();
		if (ImGui::Button("Apply Tilt Offset##Batch")) {
			for (auto* b_item : bodies) {
				const auto st = ctx.compute_relative_orbital_plane_state(*b_item, view.reference_frame_body_id);
				const double new_inc = std::clamp(st.relative_inclination_deg + static_cast<double>(batch_delta_inc), 0.0, 180.0);
				ctx.apply_relative_orbital_plane_rotation(*b_item, view.reference_frame_body_id, new_inc, st.relative_node_deg, st.relative_periapsis_deg);
			}
			changed = true;
		}

		if (ImGui::Button("Align All Coplanar (0 deg)##Batch", ImVec2(180.0f, 24.0f))) {
			for (auto* b_item : bodies) {
				const auto st = ctx.compute_relative_orbital_plane_state(*b_item, view.reference_frame_body_id);
				ctx.apply_relative_orbital_plane_rotation(*b_item, view.reference_frame_body_id, 0.0, st.relative_node_deg, st.relative_periapsis_deg);
			}
			changed = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Set All Polar (90 deg)##Batch", ImVec2(180.0f, 24.0f))) {
			for (auto* b_item : bodies) {
				const auto st = ctx.compute_relative_orbital_plane_state(*b_item, view.reference_frame_body_id);
				ctx.apply_relative_orbital_plane_rotation(*b_item, view.reference_frame_body_id, 90.0, st.relative_node_deg, st.relative_periapsis_deg);
			}
			changed = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Invert All Retrograde (180 deg)##Batch", ImVec2(220.0f, 24.0f))) {
			for (auto* b_item : bodies) {
				const auto st = ctx.compute_relative_orbital_plane_state(*b_item, view.reference_frame_body_id);
				ctx.apply_relative_orbital_plane_rotation(*b_item, view.reference_frame_body_id, 180.0, st.relative_node_deg, st.relative_periapsis_deg);
			}
			changed = true;
		}

		if (ImGui::Button("Circularize All Around Reference##Batch", ImVec2(240.0f, 24.0f))) {
			for (auto* b_item : bodies) {
				ctx.circularize_around_reference(*b_item, view.reference_frame_body_id);
			}
			changed = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Zero Relative Drift (Match Velocity)##Batch", ImVec2(260.0f, 24.0f))) {
			for (auto* b_item : bodies) {
				ctx.match_reference_velocity(*b_item, view.reference_frame_body_id);
			}
			changed = true;
		}
	}

	if (ImGui::CollapsingHeader("Physical State & Motion (Batch)", ImGuiTreeNodeFlags_DefaultOpen)) {
		changed = render_batch_scalar_slider(
			"Mass", bodies,
			[](const auto& b) { return b.mass; },
			[](auto& b, double v) { b.mass = std::max(v, 0.0); },
			[&](double v) { return Units::format_mass(v * mass_scale_kg, prefs.mass, 3); },
			0.001f, 1.0e6f, "%.4f", &view.mass_log_mode, 1e-12f, 1e30f
		) || changed;

		changed = render_batch_scalar_slider(
			"Physical Radius", bodies,
			[](const auto& b) { return b.radius; },
			[](auto& b, double v) { b.radius = std::max(v, 1e-6); },
			[&](double v) { return Units::format_distance(v * length_scale_m, prefs.distance, 3); },
			0.001f, 1.0e5f, "%.4f", &view.radius_log_mode, 1e-6f, 1e30f
		) || changed;

		changed = render_batch_scalar_slider(
			"Charge", bodies,
			[](const auto& b) { return b.charge; },
			[](auto& b, double v) { b.charge = v; },
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

	if (ImGui::CollapsingHeader("Gravitational Multipoles (Batch)")) {
		changed = render_batch_scalar_slider(
			"Quadrupole Moment", bodies,
			[](const auto& b) { return b.quadrupole_moment; },
			[](auto& b, double v) { b.quadrupole_moment = v; },
			[](double v) { char buf[32]; std::snprintf(buf, sizeof(buf), "%.4e", v); return std::string(buf); },
			-1e-2f, 1e-2f, "%.6e"
		) || changed;

		changed = render_batch_scalar_slider(
			"Zonal J2 Moment", bodies,
			[](const auto& b) { return b.j2; },
			[](auto& b, double v) { b.j2 = v; },
			[](double v) { char buf[32]; std::snprintf(buf, sizeof(buf), "%.4e", v); return std::string(buf); },
			-1e-2f, 1e-2f, "%.6e"
		) || changed;

		changed = render_batch_scalar_slider(
			"Zonal J3 Moment", bodies,
			[](const auto& b) { return b.j3; },
			[](auto& b, double v) { b.j3 = v; },
			[](double v) { char buf[32]; std::snprintf(buf, sizeof(buf), "%.4e", v); return std::string(buf); },
			-1e-3f, 1e-3f, "%.6e"
		) || changed;

		changed = render_batch_scalar_slider(
			"Zonal J4 Moment", bodies,
			[](const auto& b) { return b.j4; },
			[](auto& b, double v) { b.j4 = v; },
			[](double v) { char buf[32]; std::snprintf(buf, sizeof(buf), "%.4e", v); return std::string(buf); },
			-1e-3f, 1e-3f, "%.6e"
		) || changed;
	}

	if (ImGui::CollapsingHeader("Material, Thermal & Mechanical (Batch)")) {
		changed = render_batch_scalar_slider(
			"Friction Coefficient", bodies,
			[](const auto& b) { return b.friction_coefficient; },
			[](auto& b, double v) { b.friction_coefficient = std::clamp(v, 0.0, 1.0); },
			[](double v) { char buf[32]; std::snprintf(buf, sizeof(buf), "%.3f", v); return std::string(buf); },
			0.0f, 1.0f, "%.3f"
		) || changed;

		changed = render_batch_scalar_slider(
			"Restitution", bodies,
			[](const auto& b) { return b.restitution; },
			[](auto& b, double v) { b.restitution = std::clamp(v, 0.0, 1.0); },
			[](double v) { char buf[32]; std::snprintf(buf, sizeof(buf), "%.3f", v); return std::string(buf); },
			0.0f, 1.0f, "%.3f"
		) || changed;

		changed = render_batch_scalar_slider(
			"Integrity", bodies,
			[](const auto& b) { return b.integrity; },
			[](auto& b, double v) { b.integrity = std::max(v, 0.0); },
			[](double v) { char buf[32]; std::snprintf(buf, sizeof(buf), "%.3f", v); return std::string(buf); },
			0.0f, 1.0f, "%.3f"
		) || changed;

		changed = render_batch_scalar_slider(
			"Temperature", bodies,
			[](const auto& b) { return b.temperature; },
			[](auto& b, double v) { b.temperature = std::max(v, 0.0); },
			[&](double v) { return Units::format_temperature(v, prefs.temperature); },
			0.0f, 50000.0f, "%.2f"
		) || changed;

		changed = render_batch_scalar_slider(
			"Heat Capacity", bodies,
			[](const auto& b) { return b.heat_capacity; },
			[](auto& b, double v) { b.heat_capacity = std::max(v, 0.0); },
			[](double v) { char buf[32]; std::snprintf(buf, sizeof(buf), "%.4e", v); return std::string(buf); },
			1e-6f, 1.0e9f, "%.4e", &view.heat_capacity_log_mode, 1e-6f, 1e12f
		) || changed;

		changed = render_batch_scalar_slider(
			"Young's Modulus", bodies,
			[](const auto& b) { return b.youngs_modulus_cold; },
			[](auto& b, double v) { b.youngs_modulus_cold = std::max(v, 0.0); b.youngs_modulus_hot = std::max(v, 0.0); },
			[](double v) { char buf[32]; std::snprintf(buf, sizeof(buf), "%.4e", v); return std::string(buf); },
			0.0f, 1.0e12f, "%.4e", &view.youngs_modulus_log_mode, 1e-3f, 1e15f
		) || changed;
	}

	if (ImGui::CollapsingHeader("Surface Appearance & Presets (Batch)")) {
		const bool geom_eq = batch_are_equal_exact(bodies, [](const auto& b) { return b.geometry_model; });
		int geom_idx = geom_eq ? static_cast<int>(bodies.front()->geometry_model) : -1;
		if (ImGui::Combo("3D Geometry Model", &geom_idx, kBodyGeometryNames.data(), static_cast<int>(kBodyGeometryNames.size()))) {
			for (auto* b : bodies) b->geometry_model = static_cast<Dynamics::Body3DGeometryModel>(geom_idx);
			changed = true;
		}
		if (!geom_eq) batch_hover_tooltip(bodies, [](const auto& b) { return std::string(kBodyGeometryNames[static_cast<size_t>(b.geometry_model)]); }, [](const std::string& s) { return s; }, "Geometry Model");

		const bool preset_eq = batch_are_equal_exact(bodies, [](const auto& b) { return b.preset_3d; });
		int preset_idx = preset_eq ? static_cast<int>(bodies.front()->preset_3d) : -1;
		if (ImGui::Combo("Surface Preset", &preset_idx, kBodySurfacePresetNames.data(), static_cast<int>(kBodySurfacePresetNames.size()))) {
			for (auto* b : bodies) apply_body_appearance_preset(*b, static_cast<Dynamics::Body3DPreset>(preset_idx));
			changed = true;
		}
		if (!preset_eq) batch_hover_tooltip(bodies, [](const auto& b) { return std::string(kBodySurfacePresetNames[static_cast<size_t>(b.preset_3d)]); }, [](const std::string& s) { return s; }, "Surface Preset");

		const bool texture_eq = batch_are_equal_exact(bodies, [](const auto& b) { return b.surface_texture_mode; });
		int texture_idx = texture_eq ? static_cast<int>(bodies.front()->surface_texture_mode) : -1;
		if (ImGui::Combo("Base Texture Mode", &texture_idx, kBodyTextureModeNames.data(), static_cast<int>(kBodyTextureModeNames.size()))) {
			for (auto* b : bodies) b->surface_texture_mode = static_cast<Dynamics::Body3DSurfaceTextureMode>(texture_idx);
			changed = true;
		}
		if (!texture_eq) batch_hover_tooltip(bodies, [](const auto& b) { return std::string(kBodyTextureModeNames[static_cast<size_t>(b.surface_texture_mode)]); }, [](const std::string& s) { return s; }, "Base Texture Mode");

		changed = render_batch_scalar_slider(
			"Surface Noise Scale", bodies,
			[](const auto& b) { return b.surface_noise_scale; },
			[](auto& b, double v) { b.surface_noise_scale = static_cast<float>(v); },
			[](double v) { char buf[32]; std::snprintf(buf, sizeof(buf), "%.2f", v); return std::string(buf); },
			0.5f, 20.0f, "%.2f"
		) || changed;

		changed = render_batch_scalar_slider(
			"Surface Roughness", bodies,
			[](const auto& b) { return b.surface_roughness; },
			[](auto& b, double v) { b.surface_roughness = static_cast<float>(v); },
			[](double v) { char buf[32]; std::snprintf(buf, sizeof(buf), "%.2f", v); return std::string(buf); },
			0.05f, 1.0f, "%.2f"
		) || changed;

		changed = render_batch_scalar_slider(
			"Emission Intensity", bodies,
			[](const auto& b) { return b.emission_intensity; },
			[](auto& b, double v) { b.emission_intensity = static_cast<float>(v); },
			[](double v) { char buf[32]; std::snprintf(buf, sizeof(buf), "%.2f", v); return std::string(buf); },
			0.0f, 5.0f, "%.2f"
		) || changed;

		changed = render_batch_scalar_slider(
			"Atmosphere Thickness", bodies,
			[](const auto& b) { return b.atmosphere_thickness; },
			[](auto& b, double v) { b.atmosphere_thickness = static_cast<float>(v); },
			[](double v) { char buf[32]; std::snprintf(buf, sizeof(buf), "%.3f", v); return std::string(buf); },
			0.0f, 0.5f, "%.3f"
		) || changed;
	}

	if (ImGui::CollapsingHeader("Batch Frenet-Serret Local Velocity Vector Injection (RTN)", ImGuiTreeNodeFlags_DefaultOpen)) {
		render_group_title("Batch Local RTN Maneuver Injection", ImVec4(0.35f, 0.95f, 0.75f, 1.0f));
		render_hint_text("Apply velocity impulses decomposed in each individual body's own local Frenet-Serret RTN frame (Radial, Tangential Prograde, Normal Out-of-plane) relative to the common reference body.");

		static float batch_d_vr_mps = 0.0f;
		static float batch_d_vt_mps = 0.0f;
		static float batch_d_vn_mps = 0.0f;
		static float batch_step_mps = 50.0f;

		double b_dvr = Units::convert_velocity_from_mps(static_cast<double>(batch_d_vr_mps), prefs.velocity);
		if (unit_aware_slider_double("Batch Delta v_r (Radial +/-)", &b_dvr, -50000.0, 50000.0, UnitCategory::Velocity, prefs, "%.3f")) {
			batch_d_vr_mps = static_cast<float>(Units::convert_velocity_to_mps(b_dvr, prefs.velocity));
		}
		double b_dvt = Units::convert_velocity_from_mps(static_cast<double>(batch_d_vt_mps), prefs.velocity);
		if (unit_aware_slider_double("Batch Delta v_t (Tangential Prograde +/-)", &b_dvt, -50000.0, 50000.0, UnitCategory::Velocity, prefs, "%.3f")) {
			batch_d_vt_mps = static_cast<float>(Units::convert_velocity_to_mps(b_dvt, prefs.velocity));
		}
		double b_dvn = Units::convert_velocity_from_mps(static_cast<double>(batch_d_vn_mps), prefs.velocity);
		if (unit_aware_slider_double("Batch Delta v_n (Normal Plane Tilt +/-)", &b_dvn, -50000.0, 50000.0, UnitCategory::Velocity, prefs, "%.3f")) {
			batch_d_vn_mps = static_cast<float>(Units::convert_velocity_to_mps(b_dvn, prefs.velocity));
		}

		const double b_dvr_sim = static_cast<double>(batch_d_vr_mps) / speed_scale_mps;
		const double b_dvt_sim = static_cast<double>(batch_d_vt_mps) / speed_scale_mps;
		const double b_dvn_sim = static_cast<double>(batch_d_vn_mps) / speed_scale_mps;

		if (ImGui::Button("Execute Batch RTN Burn on All Selected Bodies", ImVec2(320.0f, 26.0f))) {
			for (auto* b_item : bodies) {
				ctx.apply_frenet_delta_v(*b_item, view.reference_frame_body_id, b_dvr_sim, b_dvt_sim, b_dvn_sim);
			}
			batch_d_vr_mps = 0.0f;
			batch_d_vt_mps = 0.0f;
			batch_d_vn_mps = 0.0f;
			changed = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Zero Staged Batch Delta-V", ImVec2(190.0f, 26.0f))) {
			batch_d_vr_mps = 0.0f;
			batch_d_vt_mps = 0.0f;
			batch_d_vn_mps = 0.0f;
		}

		double b_step_disp = Units::convert_velocity_from_mps(static_cast<double>(batch_step_mps), prefs.velocity);
		if (unit_aware_slider_double("Batch Impulse Step", &b_step_disp, 0.1, 10000.0, UnitCategory::Velocity, prefs, "%.2f")) {
			batch_step_mps = static_cast<float>(Units::convert_velocity_to_mps(b_step_disp, prefs.velocity));
		}

		const double b_step_sim = static_cast<double>(batch_step_mps) / speed_scale_mps;
		if (ImGui::Button("+Prograde All##BatchRTN", ImVec2(120.0f, 24.0f))) {
			for (auto* b_item : bodies) {
				ctx.apply_frenet_delta_v(*b_item, view.reference_frame_body_id, 0.0, b_step_sim, 0.0);
			}
			changed = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("-Retrograde All##BatchRTN", ImVec2(125.0f, 24.0f))) {
			for (auto* b_item : bodies) {
				ctx.apply_frenet_delta_v(*b_item, view.reference_frame_body_id, 0.0, -b_step_sim, 0.0);
			}
			changed = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("+Radial Out All##BatchRTN", ImVec2(125.0f, 24.0f))) {
			for (auto* b_item : bodies) {
				ctx.apply_frenet_delta_v(*b_item, view.reference_frame_body_id, b_step_sim, 0.0, 0.0);
			}
			changed = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("-Radial In All##BatchRTN", ImVec2(120.0f, 24.0f))) {
			for (auto* b_item : bodies) {
				ctx.apply_frenet_delta_v(*b_item, view.reference_frame_body_id, -b_step_sim, 0.0, 0.0);
			}
			changed = true;
		}

		if (ImGui::Button("+Normal North All##BatchRTN", ImVec2(150.0f, 24.0f))) {
			for (auto* b_item : bodies) {
				ctx.apply_frenet_delta_v(*b_item, view.reference_frame_body_id, 0.0, 0.0, b_step_sim);
			}
			changed = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("-Anti-Normal South All##BatchRTN", ImVec2(175.0f, 24.0f))) {
			for (auto* b_item : bodies) {
				ctx.apply_frenet_delta_v(*b_item, view.reference_frame_body_id, 0.0, 0.0, -b_step_sim);
			}
			changed = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Circularize All in Respective Local Frames##BatchRTN", ImVec2(290.0f, 24.0f))) {
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
		sys.update_accelerations();
		ctx.orchestrator().notify_state_changed();
	}
	return changed;
}

}

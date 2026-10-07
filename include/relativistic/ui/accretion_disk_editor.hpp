#pragma once

#include <imgui.h>
#include "relativistic/render/accretion_disk/accretion_disk_settings.hpp"
#include "relativistic/render/accretion_disk/hydro_disk_settings.hpp"
#include "relativistic/render/accretion_disk/hydro_disk_shading.hpp"
#include <cmath>
#include <string>
#include "relativistic/ui/numeric_slider_utils.hpp"
#include "relativistic/ui/tooltip_utils.hpp"
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/orchestrator/command.hpp"
#include <algorithm>
#include <cstdint>
#include <string_view>

namespace Relativistic::UI {

[[nodiscard]] inline bool render_accretion_disk_editor(Render::AccretionDiskSettings& settings, bool include_thermal_basics) noexcept {
	bool changed = false;
	ImGui::PushID("AccretionDiskEditor");

	changed = ImGui::Checkbox("Render Accretion Disk", &settings.enabled) || changed;
	render_setting_tooltip("Enables or disables the emitting disk of this black hole. When disabled, only the lensed background and the event horizon remain.");

	if (ImGui::BeginCombo("Apply Disk Preset", "Select a preset...")) {
		for (size_t i = 0; i < Render::kAccretionDiskPresetNames.size(); ++i) {
			if (ImGui::Selectable(Render::kAccretionDiskPresetNames[i])) {
				settings.apply_preset(static_cast<Render::AccretionDiskPreset>(i), !include_thermal_basics);
				changed = true;
			}
		}
		ImGui::EndCombo();
	}
	render_setting_tooltip("Replaces the structural parameters below with a coherent configuration. The thermal basics of the primary black hole are kept because they are edited in the spectral color model section.");

	ImGui::Spacing();
	ImGui::TextColored(ImVec4(0.5f, 0.85f, 1.0f, 1.0f), "Geometry");
	changed = slider_float_with_input("Inner Edge (x ISCO)", &settings.inner_radius_scale, 1.0f, 4.0f, "%.2f") || changed;
	render_setting_tooltip("Inner edge of the emitting disk as a multiple of the innermost stable circular orbit. Values above 1 truncate the disk and open a dark cavity around the horizon.");
	changed = slider_float_with_input("Outer Radius (M)", &settings.outer_radius_mass_units, 6.0f, 120.0f, "%.1f") || changed;
	render_setting_tooltip("Outer edge of the disk in units of the black hole mass.");
	changed = slider_float_with_input("Edge Softness", &settings.edge_softness, 0.05f, 6.0f, "%.2f") || changed;
	render_setting_tooltip("Width of the smooth fade at the inner and outer edges. Small values give razor-sharp rims.");

	ImGui::Spacing();
	ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.35f, 1.0f), "Thermal Profile & Radiometry");
	if (include_thermal_basics) {
		changed = slider_float_with_input("Peak Temperature (K)", &settings.peak_temperature_k, 1000.0f, 200000.0f, "%.0f") || changed;
		render_setting_tooltip("Maximum local blackbody temperature reached in the disk before Doppler and gravitational shifting. The color is obtained by integrating the shifted Planck spectrum over the CIE 1931 color matching functions.");
		changed = slider_float_with_input("Floor Temperature (K)", &settings.floor_temperature_k, 0.0f, 20000.0f, "%.0f") || changed;
		render_setting_tooltip("Temperature retained at the disk edges and in the cool gaps between structures.");
		changed = slider_float_with_input("Additional Doppler Boost", &settings.extra_beaming_exponent, -2.0f, 8.0f, "%.2f") || changed;
		render_setting_tooltip("Extra power of the redshift factor applied on top of the physical spectral shift, which already scales the bolometric flux as g^4. Zero is purely physical.");
		changed = slider_float_with_input("Color Saturation", &settings.color_saturation, 0.0f, 3.0f, "%.2fx") || changed;
		render_setting_tooltip("Chroma multiplier around the computed luminance. One keeps the CIE derived color.");
	}
	changed = slider_float_with_input("Temperature Falloff Exponent", &settings.temperature_exponent, 0.3f, 2.0f, "%.3f") || changed;
	render_setting_tooltip("Radial power law of the effective temperature. The thin-disk value is 0.75.");
	changed = slider_float_with_input("Zero-Torque Inner Boundary", &settings.zero_torque_strength, 0.0f, 1.0f, "%.2f") || changed;
	render_setting_tooltip("Strength of the Novikov-Thorne inner boundary condition. At one the temperature vanishes at the inner edge and peaks further out; at zero the disk is hottest at its inner edge.");
	changed = slider_float_with_input("Brightness", &settings.brightness, 0.0f, 20.0f, "%.3f") || changed;
	render_setting_tooltip("Absolute radiance gain, normalized so that a value of one shows the hottest unshifted region at unit luminance.");
	changed = slider_float_with_input("Opacity", &settings.opacity_scale, 0.0f, 2.0f, "%.2f") || changed;
	render_setting_tooltip("Optical thickness scale. Lower values let the background and the far side of the disk show through.");
	changed = slider_float_with_input("Tint Strength", &settings.tint_strength, 0.0f, 1.0f, "%.2f") || changed;
	changed = ImGui::ColorEdit3("Tint Color", settings.tint.data()) || changed;
	render_setting_tooltip("Multiplicative artistic color grade applied after the physical spectral color, blended by the tint strength.");

	ImGui::Spacing();
	ImGui::TextColored(ImVec4(0.6f, 0.9f, 0.6f, 1.0f), "Rotation");
	changed = slider_float_with_input("Pattern Rotation Speed", &settings.rotation_speed_scale, 0.0f, 200.0f, "%.2fx") || changed;
	render_setting_tooltip("Multiplier on the Keplerian angular velocity that drives the motion of rings, spirals, turbulence and grain. Turbulence and grain shear differentially with radius, spiral arms rotate rigidly.");

	ImGui::Spacing();
	ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.4f, 1.0f), "Radial Rings");
	changed = slider_float_with_input("Ring Contrast", &settings.ring_amplitude, 0.0f, 1.0f, "%.2f") || changed;
	changed = slider_float_with_input("Ring Count", &settings.ring_frequency, 0.0f, 64.0f, "%.1f") || changed;
	changed = slider_float_with_input("Ring Sharpness", &settings.ring_sharpness, 1.0f, 12.0f, "%.1f") || changed;
	render_setting_tooltip("Concentric brightness and temperature bands distributed logarithmically across the disk radius.");

	ImGui::Spacing();
	ImGui::TextColored(ImVec4(0.8f, 0.6f, 1.0f, 1.0f), "Spiral Density Waves");
	changed = ImGui::SliderInt("Spiral Arms", &settings.spiral_arm_count, 0, 8) || changed;
	changed = slider_float_with_input("Spiral Contrast", &settings.spiral_amplitude, 0.0f, 1.0f, "%.2f") || changed;
	changed = slider_float_with_input("Spiral Pitch", &settings.spiral_pitch, -6.0f, 6.0f, "%.2f") || changed;
	render_setting_tooltip("Logarithmic spiral arms rotating rigidly at the pattern speed of the mid-disk. Negative pitch winds the arms the other way.");

	ImGui::Spacing();
	ImGui::TextColored(ImVec4(0.5f, 0.9f, 0.85f, 1.0f), "Turbulence & Grain");
	changed = slider_float_with_input("Turbulence Strength", &settings.turbulence_amplitude, 0.0f, 2.0f, "%.2f") || changed;
	changed = slider_float_with_input("Turbulence Scale", &settings.turbulence_scale, 0.5f, 40.0f, "%.1f") || changed;
	changed = ImGui::SliderInt("Turbulence Octaves", &settings.turbulence_octaves, 1, 6) || changed;
	changed = slider_float_with_input("Azimuthal Elongation", &settings.radial_stretch, 0.25f, 16.0f, "%.2f") || changed;
	render_setting_tooltip("Higher values stretch structures along the orbital direction into thin streaks and ring-like filaments.");
	changed = slider_float_with_input("Grain Strength", &settings.grain_amplitude, 0.0f, 1.5f, "%.2f") || changed;
	changed = slider_float_with_input("Grain Scale", &settings.grain_scale, 2.0f, 400.0f, "%.0f") || changed;
	render_setting_tooltip("Fine-scale granular emission that is carried along the differential rotation of the disk.");
	int seed = static_cast<int>(settings.noise_seed);
	if (ImGui::InputInt("Structure Seed", &seed)) {
		settings.noise_seed = static_cast<uint32_t>(std::max(seed, 0));
		changed = true;
	}
	render_setting_tooltip("Deterministic seed of the turbulence and grain patterns.");

	ImGui::Spacing();
	if (ImGui::Button("Reset Disk To Defaults", ImVec2(-1.0f, 24.0f))) {
		settings.apply_preset(Render::AccretionDiskPreset::BalancedThinDisk, !include_thermal_basics);
		changed = true;
	}

	if (changed) {
		settings.sanitize();
	}
	ImGui::PopID();
	return changed;
}

[[nodiscard]] inline bool metric_supports_accretion_disk(std::string_view metric_name) noexcept {
	if (metric_name.find("de Sitter") != std::string_view::npos || metric_name.find("DeSitter") != std::string_view::npos) {
		return false;
	}
	return metric_name.find("Schwarzschild") != std::string_view::npos
		|| metric_name.find("Kerr") != std::string_view::npos
		|| metric_name.find("Reissner") != std::string_view::npos;
}

[[nodiscard]] inline bool render_hydro_disk_editor(Orchestrator::SimulationOrchestrator<1024>& orchestrator) noexcept {
	auto& params = orchestrator.parameters();
	auto& settings = params.hydro_disk;
	const bool spin_active = orchestrator.active_metric_name().find("Kerr") != std::string::npos;
	const double mass = std::max(params.mass, 1e-4);
	const double spin = spin_active ? std::clamp(params.spin, -0.999 * mass, 0.999 * mass) : 0.0;
	bool changed = false;
	ImGui::PushID("HydroDiskEditor");

	ImGui::TextColored(ImVec4(0.45f, 0.85f, 1.0f, 1.0f), "Accretion Disk Model");
	int model_index = static_cast<int>(settings.model);
	if (ImGui::Combo("Disk Model", &model_index, Render::kHydroDiskModelNames.data(), static_cast<int>(Render::kHydroDiskModelNames.size()))) {
		settings.model = static_cast<Render::HydroDiskModel>(model_index);
		changed = true;
	}
	render_setting_tooltip("Selects the stationary procedural thin disk, the relativistic Novikov-Thorne thin disk flux profile, or the volumetric Fishbone-Moncrief hydrostatic torus. All three are traced on the Vulkan compute path.");

	if (settings.model == Render::HydroDiskModel::ThinProcedural) {
		ImGui::TextDisabled("Geometrically thin stationary disk with a procedural temperature profile.");
		ImGui::PopID();
		if (changed) {
			settings.sanitize();
		}
		return changed;
	}

	if (settings.model == Render::HydroDiskModel::NovikovThorne) {
		static bool rate_log_mode = true;
		changed = slider_float_with_input("Accretion Rate Scale", &settings.accretion_rate_scale, 0.01f, 100.0f, "%.3f", &rate_log_mode, 0.01f, 100.0f) || changed;
		render_setting_tooltip("Scales the effective temperature of the Novikov-Thorne disk as the fourth root of the accretion rate. The radial profile follows the Page-Thorne relativistic flux vanishing at the ISCO.");
	} else {
		if (ImGui::BeginCombo("Torus Preset", "Select a preset...")) {
			for (size_t i = 0; i < Render::kHydroDiskPresetNames.size(); ++i) {
				if (ImGui::Selectable(Render::kHydroDiskPresetNames[i])) {
					settings.apply_preset(static_cast<Render::HydroDiskPreset>(i));
					changed = true;
				}
			}
			ImGui::EndCombo();
		}
		render_setting_tooltip("Replaces the torus geometry and optical depth with a coherent configuration.");
		changed = slider_float_with_input("Torus Inner Edge (M)", &settings.torus_inner_radius, 1.2f, 80.0f, "%.2f") || changed;
		render_setting_tooltip("Equatorial radius of the inner edge of the torus where the equipotential surface closes.");
		changed = slider_float_with_input("Pressure Maximum Radius (M)", &settings.torus_center_radius, 1.26f, 160.0f, "%.2f") || changed;
		render_setting_tooltip("Equatorial radius of the pressure maximum, which sets the constant specific angular momentum of the torus.");
		changed = slider_float_with_input("Adiabatic Index", &settings.adiabatic_index, 1.2f, 2.0f, "%.3f") || changed;
		render_setting_tooltip("Polytropic index of the torus gas. It controls how sharply the density falls towards the surface.");
		static bool depth_log_mode = true;
		changed = slider_float_with_input("Optical Depth Scale", &settings.optical_depth_scale, 1e-3f, 50.0f, "%.3f", &depth_log_mode, 1e-3f, 50.0f) || changed;
		render_setting_tooltip("Optical depth per unit of mass length through the densest part of the torus.");
		changed = slider_float_with_input("Samples Per M", &settings.sampling_density, 0.5f, 8.0f, "%.2f") || changed;
		render_setting_tooltip("Number of radiative transfer samples per gravitational radius along each integration step.");
		if (ImGui::Button("Fit Torus To ISCO", ImVec2(-1.0f, 24.0f))) {
			const auto radii = Render::HydroDiskShader::equilibrium_torus_radii(mass, spin);
			settings.torus_inner_radius = static_cast<float>(radii.first);
			settings.torus_center_radius = static_cast<float>(radii.second);
			changed = true;
		}
		render_setting_tooltip("Places the inner edge at the ISCO and the pressure maximum at twice that radius for the current mass and spin.");
	}

	if (changed) {
		settings.sanitize();
	}

	const auto bounds = Render::HydroDiskShader::primary_disk_bounds(mass, spin, params.primary_disk.inner_radius_scale, params.primary_disk.outer_radius_mass_units);
	const Render::GpuHydroDiskProfile profile = settings.to_gpu_profile();
	const Render::HydroDiskContext& context = Render::HydroDiskShader::context(profile, mass, spin, bounds.first, bounds.second);
	if (settings.model == Render::HydroDiskModel::NovikovThorne) {
		ImGui::TextDisabled("ISCO: %.4f M | Marginally Bound Orbit: %.4f M | Radiative Efficiency: %.2f %%", context.isco_radius() / mass, context.marginally_bound_radius() / mass, 100.0 * context.radiative_efficiency());
	} else if (context.torus_valid()) {
		ImGui::TextDisabled("Peak Enthalpy: %.4f | Equatorial Outer Edge: %.2f M", context.max_enthalpy(), context.torus_outer_radius() / mass);
		if (!context.torus_closed()) {
			ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "The torus does not close at a finite radius for these parameters; it is truncated by the integration bound.");
		}
	} else {
		ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.4f, 1.0f), "No equilibrium torus exists for the current mass, spin and radii. Move the inner edge outside the horizon and below the pressure maximum.");
	}
	ImGui::TextWrapped("Brightness, opacity, temperatures, saturation and tint below apply to every model. Ring, spiral, turbulence and grain structure modulate the thin disk models only.");

	ImGui::PopID();
	return changed;
}

inline void render_primary_accretion_disk_editor(Orchestrator::SimulationOrchestrator<1024>& orchestrator) noexcept {
	if (!metric_supports_accretion_disk(orchestrator.active_metric_name())) {
		ImGui::TextDisabled("The active spacetime metric does not render an accretion disk.");
		return;
	}
	auto& params = orchestrator.parameters();
	ImGui::PushID("PrimaryAccretionDisk");

	ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.35f, 1.0f), "Spectral Color Model");
	float peak_temperature = static_cast<float>(params.disk_temperature_scale_k);
	if (slider_float_with_input("Disk Peak Temperature Scale (K)", &peak_temperature, 1000.0f, 60000.0f, "%.0f")) {
		static_cast<void>(orchestrator.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::DiskTemperatureScale, static_cast<double>(peak_temperature))));
	}
	render_setting_tooltip("Blackbody temperature at the hottest radius of the primary disk before Doppler and gravitational shifting. The color is obtained by integrating the shifted Planck spectrum through the CIE 1931 color matching functions.");
	float floor_temperature = static_cast<float>(params.disk_temperature_floor_k);
	if (slider_float_with_input("Disk Temperature Floor (K)", &floor_temperature, 0.0f, 20000.0f, "%.0f")) {
		static_cast<void>(orchestrator.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::DiskTemperatureFloor, static_cast<double>(floor_temperature))));
	}
	render_setting_tooltip("Minimum blackbody temperature retained in the cool gaps and at the outer edge of the primary disk.");
	float beaming_boost = static_cast<float>(params.disk_doppler_beaming_exponent);
	if (slider_float_with_input("Additional Doppler Boost", &beaming_boost, -2.0f, 8.0f, "%.2f")) {
		static_cast<void>(orchestrator.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::DiskDopplerBeamingExponent, static_cast<double>(beaming_boost))));
	}
	render_setting_tooltip("Extra power of the redshift factor applied on top of the physical spectral shift, which already reproduces the exact relativistic beaming of the specific intensity. Zero is purely physical.");
	float color_saturation = static_cast<float>(params.disk_color_saturation);
	if (slider_float_with_input("Disk Color Saturation", &color_saturation, 0.0f, 3.0f, "%.2fx")) {
		static_cast<void>(orchestrator.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::DiskColorSaturation, static_cast<double>(color_saturation))));
	}
	render_setting_tooltip("Chroma multiplier around the computed luminance. One keeps the CIE derived color.");

	ImGui::Spacing();
	const bool hydro_changed = render_hydro_disk_editor(orchestrator);
	ImGui::Separator();
	const bool structure_changed = render_accretion_disk_editor(params.primary_disk, false);
	if (hydro_changed || structure_changed) {
		orchestrator.notify_state_changed();
	}
	ImGui::PopID();
}

}

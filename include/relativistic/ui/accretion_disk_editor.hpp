#pragma once

#include <imgui.h>
#include "relativistic/render/accretion_disk_settings.hpp"
#include "relativistic/ui/numeric_slider_utils.hpp"
#include "relativistic/ui/tooltip_utils.hpp"
#include <algorithm>
#include <cstdint>

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

}

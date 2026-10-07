#pragma once

#include <imgui.h>
#include "relativistic/magnetosphere/blandford_znajek.hpp"
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/render/jet/jet_settings.hpp"
#include "relativistic/ui/accretion_disk_editor.hpp"
#include "relativistic/ui/numeric_slider_utils.hpp"
#include "relativistic/ui/tooltip_utils.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>

namespace Relativistic::UI {

inline void render_jet_magnetosphere_editor(Orchestrator::SimulationOrchestrator<1024>& orchestrator) noexcept {
	auto& params = orchestrator.parameters();
	auto& settings = params.jet;
	bool changed = false;
	ImGui::PushID("JetMagnetosphereEditor");

	changed = ImGui::Checkbox("Enable Blandford-Znajek Jet Emission", &settings.enabled) || changed;
	render_setting_tooltip("Renders the synchrotron emission of the force-free magnetosphere threading the horizon. Disabled by default. The CPU SIMD pipeline is bypassed while enabled; thermal and hybrid models run on the CPU renderer.");

	if (!metric_supports_accretion_disk(orchestrator.active_metric_name())) {
		ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "The active spacetime metric does not trace the jet emission.");
	}
	if (std::abs(params.spin) < 1e-9) {
		ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "The central spin is zero: the horizon angular velocity vanishes, so no toroidal field or Poynting flux is generated.");
	}

	if (ImGui::BeginCombo("Apply Jet Preset", "Select a preset...")) {
		for (size_t i = 0; i < Render::kJetPresetNames.size(); ++i) {
			if (ImGui::Selectable(Render::kJetPresetNames[i])) {
				settings.apply_preset(static_cast<Render::JetPreset>(i));
				changed = true;
			}
		}
		ImGui::EndCombo();
	}
	render_setting_tooltip("Replaces every jet parameter with a coherent configuration while keeping the enable state.");

	ImGui::Spacing();
	ImGui::TextColored(ImVec4(0.5f, 0.85f, 1.0f, 1.0f), "Magnetic Field Geometry");
	int geometry_index = static_cast<int>(settings.geometry);
	if (ImGui::Combo("Field Geometry", &geometry_index, Render::kJetGeometryNames.data(), static_cast<int>(Render::kJetGeometryNames.size()))) {
		settings.geometry = static_cast<Render::JetFieldGeometry>(geometry_index);
		changed = true;
	}
	render_setting_tooltip("Analytic stream function A_phi(r, theta) = C r^nu g(theta). The toroidal field follows from the Znajek horizon condition applied at the footpoint of each field line.");
	if (settings.geometry == Render::JetFieldGeometry::Paraboloidal) {
		changed = slider_float_with_input("Field Line Index (nu)", &settings.field_line_index, 0.25f, 4.0f, "%.2f") || changed;
		render_setting_tooltip("Radial exponent of the stream function. One recovers the monopole, larger values collimate the field lines.");
	}
	static bool field_log_mode = true;
	changed = slider_float_with_input("Horizon Field Strength (T)", &settings.horizon_field_tesla, 1.0f, 1.0e9f, "%.3e", &field_log_mode, 1.0f, 1.0e9f) || changed;
	render_setting_tooltip("Poloidal field magnitude at the horizon. It sets the magnetic flux, the plasma density through the magnetization and the extracted power.");

	ImGui::Spacing();
	ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.35f, 1.0f), "Field Rotation And Current");
	int law_index = static_cast<int>(settings.angular_velocity_law);
	if (ImGui::Combo("Angular Velocity Law", &law_index, Render::kJetAngularVelocityLawNames.data(), static_cast<int>(Render::kJetAngularVelocityLawNames.size()))) {
		settings.angular_velocity_law = static_cast<Render::JetAngularVelocityLaw>(law_index);
		changed = true;
	}
	if (settings.angular_velocity_law == Render::JetAngularVelocityLaw::HorizonFraction) {
		changed = slider_float_with_input("Field Angular Velocity / Horizon", &settings.angular_velocity_fraction, 0.0f, 2.0f, "%.3f") || changed;
		render_setting_tooltip("Ratio Omega_F / Omega_H. The Blandford-Znajek monopole value is one half, which maximizes the extracted power.");
		changed = slider_float_with_input("Footpoint Angular Variation", &settings.angular_velocity_variation, -1.0f, 1.0f, "%.3f") || changed;
		render_setting_tooltip("Reduces Omega_F by this fraction times sin^2 of the footpoint angle, mimicking the decrease of Omega_F away from the axis.");
	} else {
		changed = slider_float_with_input("Fixed Angular Velocity (1/M)", &settings.fixed_angular_velocity, -1.0f, 1.0f, "%.4f") || changed;
		render_setting_tooltip("Constant field line angular velocity expressed in inverse central masses.");
	}
	changed = slider_float_with_input("Toroidal Field Scale", &settings.toroidal_field_scale, 0.0f, 4.0f, "%.2f") || changed;
	render_setting_tooltip("Multiplier on the toroidal field fixed by the polar current I(A_phi). Zero keeps a purely poloidal field.");
	changed = slider_float_with_input("Rotation Coupling", &settings.rotation_coupling, 0.0f, 1.0f, "%.2f") || changed;
	render_setting_tooltip("Fraction of the field line corotation velocity transferred to the plasma azimuthal motion.");

	ImGui::Spacing();
	ImGui::TextColored(ImVec4(0.6f, 0.9f, 0.6f, 1.0f), "Plasma Flow And Extent");
	static bool sigma_log_mode = true;
	changed = slider_float_with_input("Magnetization (sigma)", &settings.magnetization, 0.01f, 1.0e5f, "%.3f", &sigma_log_mode, 0.01f, 1.0e5f) || changed;
	render_setting_tooltip("Ratio of magnetic to rest-mass energy density at the horizon. The electron density follows n = eps0 B^2 / (sigma m_p).");
	changed = slider_float_with_input("Magnetization Radial Index", &settings.magnetization_index, -2.0f, 3.0f, "%.2f") || changed;
	changed = slider_float_with_input("Inner Radius (x Horizon)", &settings.inner_radius_scale, 1.001f, 10.0f, "%.3f") || changed;
	changed = slider_float_with_input("Outer Radius (M)", &settings.outer_radius_mass_units, 5.0f, 2000.0f, "%.1f") || changed;
	changed = slider_float_with_input("Footpoint Edge Angle (deg)", &settings.footpoint_edge_deg, 1.0f, 90.0f, "%.1f") || changed;
	render_setting_tooltip("Only field lines anchored closer to the rotation axis than this horizon angle are filled with emitting plasma.");
	changed = slider_float_with_input("Footpoint Edge Softness (deg)", &settings.footpoint_softness_deg, 0.1f, 45.0f, "%.1f") || changed;
	changed = slider_float_with_input("Inner Lorentz Factor", &settings.lorentz_factor_inner, 1.0f, 50.0f, "%.2f") || changed;
	changed = slider_float_with_input("Terminal Lorentz Factor", &settings.lorentz_factor_max, 1.0f, 100.0f, "%.2f") || changed;
	changed = slider_float_with_input("Acceleration Radius (M)", &settings.acceleration_radius_mass_units, 0.1f, 2000.0f, "%.1f") || changed;
	changed = slider_float_with_input("Acceleration Index", &settings.acceleration_index, 0.1f, 4.0f, "%.2f") || changed;
	changed = ImGui::Checkbox("Doppler Beaming And Aberration", &settings.doppler_beaming_enabled) || changed;
	ImGui::SameLine();
	changed = ImGui::Checkbox("Gravitational Redshift", &settings.gravitational_redshift_enabled) || changed;

	ImGui::Spacing();
	ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.4f, 1.0f), "Radiating Electrons");
	int model_index = static_cast<int>(settings.emission_model);
	if (ImGui::Combo("Emission Model", &model_index, Render::kJetEmissionModelNames.data(), static_cast<int>(Render::kJetEmissionModelNames.size()))) {
		settings.emission_model = static_cast<Render::JetEmissionModel>(model_index);
		changed = true;
	}
	render_setting_tooltip("Non-thermal emission is evaluated on the Vulkan compute shader. Thermal and hybrid models fall back to the CPU renderer.");
	changed = slider_float_with_input("Power Law Index (p)", &settings.power_law_index, 1.5f, 5.0f, "%.2f") || changed;
	static bool gamma_log_mode = true;
	changed = slider_float_with_input("Minimum Electron Lorentz Factor", &settings.minimum_lorentz_factor, 1.0f, 1.0e4f, "%.1f", &gamma_log_mode, 1.0f, 1.0e4f) || changed;
	changed = slider_float_with_input("Non-Thermal Fraction", &settings.non_thermal_fraction, 0.0f, 1.0f, "%.3f") || changed;
	static bool temperature_log_mode = true;
	changed = slider_float_with_input("Electron Temperature (K)", &settings.electron_temperature_k, 1.0e5f, 1.0e13f, "%.3e", &temperature_log_mode, 1.0e5f, 1.0e13f) || changed;
	changed = slider_float_with_input("Temperature Radial Index", &settings.temperature_index, -1.0f, 3.0f, "%.2f") || changed;
	static bool frequency_log_mode = true;
	changed = slider_float_with_input("Observing Frequency (Hz)", &settings.observing_frequency_hz, 1.0e6f, 1.0e20f, "%.3e", &frequency_log_mode, 1.0e6f, 1.0e20f) || changed;
	changed = slider_float_with_input("Absorption Scale", &settings.absorption_scale, 0.0f, 10.0f, "%.2f") || changed;
	changed = slider_float_with_input("Faraday Scale", &settings.faraday_scale, 0.0f, 10.0f, "%.2f") || changed;

	ImGui::Spacing();
	ImGui::TextColored(ImVec4(0.8f, 0.6f, 1.0f, 1.0f), "Display");
	int display_index = static_cast<int>(settings.display_mode);
	if (ImGui::Combo("Display Mode", &display_index, Render::kJetDisplayModeNames.data(), static_cast<int>(Render::kJetDisplayModeNames.size()))) {
		settings.display_mode = static_cast<Render::JetDisplayMode>(display_index);
		changed = true;
	}
	changed = ImGui::Checkbox("Automatic Intensity Normalization", &settings.auto_normalization) || changed;
	render_setting_tooltip("Normalizes the display to the emission of a canonical sample so the jet is visible for any field strength. Disable to use the Rayleigh-Jeans brightness temperature below as an absolute reference.");
	if (!settings.auto_normalization) {
		static bool reference_log_mode = true;
		changed = slider_float_with_input("Reference Brightness Temperature (K)", &settings.reference_brightness_temperature_k, 1.0e3f, 1.0e14f, "%.3e", &reference_log_mode, 1.0e3f, 1.0e14f) || changed;
	}
	changed = slider_float_with_input("Brightness", &settings.brightness, 0.0f, 1000.0f, "%.3f") || changed;
	changed = ImGui::ColorEdit3("Tint Color", settings.tint.data()) || changed;
	changed = slider_float_with_input("Polarization Coherence", &settings.polarization_coherence, 0.0f, 1.0f, "%.2f") || changed;
	render_setting_tooltip("Fraction of the locally generated linear polarization that survives the sum of field orientations along the ray.");

	ImGui::Spacing();
	ImGui::TextColored(ImVec4(0.5f, 0.9f, 0.85f, 1.0f), "Sampling And Units");
	changed = slider_float_with_input("Samples Per M", &settings.sampling_density, 0.1f, 8.0f, "%.2f") || changed;
	changed = ImGui::SliderInt("Maximum Samples Per Segment", &settings.max_samples, 1, 128) || changed;
	changed = ImGui::Checkbox("Use Engine Length Scale", &settings.use_engine_length_scale) || changed;
	if (!settings.use_engine_length_scale) {
		static bool meters_log_mode = true;
		changed = slider_float_with_input("Meters Per Length Unit", &settings.meters_per_length_unit, 1.0f, 1.0e15f, "%.4e", &meters_log_mode, 1.0f, 1.0e15f) || changed;
	}

	if (changed) {
		settings.sanitize();
		orchestrator.notify_state_changed();
	}

	if (settings.enabled) {
		const Render::GpuJetProfile profile = settings.to_gpu_profile(params.mass, params.spin, orchestrator.constants_engine().length_scale());
		const Magnetosphere::BlandfordZnajekModel model(profile);
		ImGui::Separator();
		ImGui::TextDisabled("Horizon Radius: %.4f | Horizon Angular Velocity: %.5f / M", model.horizon_radius(), model.horizon_angular_velocity() * static_cast<double>(profile.mass));
		ImGui::TextDisabled("Horizon Magnetic Flux: %.4e Wb | Extracted Power: %.4e W", model.magnetic_flux_weber(), model.blandford_znajek_power_watts());
		ImGui::TextDisabled("Normalization Reference: %.4e W m^-2 Hz^-1 sr^-1", std::exp(static_cast<double>(profile.log_reference_intensity)));
	}
	ImGui::PopID();
}

}

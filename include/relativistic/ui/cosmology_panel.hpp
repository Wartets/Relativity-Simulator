#pragma once

#include <imgui.h>
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/semiclassical/hawking_unruh.hpp"
#include "relativistic/cosmology/cosmology_settings.hpp"
#include "relativistic/ui/tooltip_utils.hpp"
#include "relativistic/ui/compatibility_notes.hpp"
#include <algorithm>
#include <cmath>
#include <string>

namespace Relativistic::UI {

class CosmologyPanel {
private:
	using OrchestratorType = Orchestrator::SimulationOrchestrator<1024>;

	static bool real_slider(const char* label, double* value, double minimum, double maximum, const char* format, bool logarithmic = false) noexcept {
		const ImGuiSliderFlags flags = logarithmic ? ImGuiSliderFlags_Logarithmic : ImGuiSliderFlags_None;
		return ImGui::SliderScalar(label, ImGuiDataType_Double, value, &minimum, &maximum, format, flags);
	}

	static uint32_t cosmology_metric_id(const std::string& metric_name) noexcept {
		if (metric_name.find("Chaplygin") != std::string::npos) return 12U;
		if (metric_name.find("Quintessence") != std::string::npos) return 11U;
		return 6U;
	}

	static bool render_lemaitre_tolman_bondi(Cosmology::CosmologySettings& settings) noexcept {
		bool changed = false;
		auto& ltb = settings.ltb;
		changed |= real_slider("Hubble Rate H0 (void exterior)", &ltb.hubble0, 1.0e-4, 1.0, "%.5f", true);
		render_setting_tooltip("Present-day expansion rate of the asymptotic region, in inverse simulation time units.");
		changed |= real_slider("Void Radius", &ltb.void_radius, 1.0, 5000.0, "%.2f", true);
		render_setting_tooltip("Comoving radius at which the density profile crosses halfway between the interior and exterior values.");
		changed |= real_slider("Void Wall Width", &ltb.void_width, 0.5, 2000.0, "%.2f", true);
		render_setting_tooltip("Thickness of the tanh transition between the underdense interior and the exterior.");
		changed |= real_slider("Interior Omega_m", &ltb.omega_inner, 0.01, 3.0, "%.3f");
		render_setting_tooltip("Local matter density parameter at the void center. Values below 1 give an open interior.");
		changed |= real_slider("Exterior Omega_m", &ltb.omega_outer, 0.01, 3.0, "%.3f");
		render_setting_tooltip("Matter density parameter far from the void. A value of 1 recovers the Einstein-de Sitter background.");
		if (changed) {
			ltb.sanitize();
		}
		const Cosmology::LtbProfile profile(ltb);
		ImGui::TextDisabled("Present Epoch: %.4f | Central Omega_m: %.4f | Central Age: %.4f | Exterior Age: %.4f", profile.present_time(), profile.omega(0.0), profile.age(profile.omega(0.0)), profile.age(ltb.omega_outer));
		if (ImGui::Button("Reset Void Parameters", ImVec2(220.0f, 24.0f))) {
			ltb = Cosmology::LtbParameters{};
			changed = true;
		}
		return changed;
	}

	static bool render_dark_energy(OrchestratorType& orchestrator, Cosmology::CosmologySettings& settings) noexcept {
		bool changed = false;
		auto& energy = settings.dark_energy;
		changed |= real_slider("Omega Matter", &energy.omega_matter, 0.01, 0.99, "%.3f");
		changed |= real_slider("Omega Radiation", &energy.omega_radiation, 0.0, 0.05, "%.2e");
		render_setting_tooltip("Matter and radiation fractions today. The dark component closes the flat budget.");

		int potential = static_cast<int>(energy.potential);
		if (ImGui::Combo("Quintessence Potential", &potential, Cosmology::kQuintessencePotentialNames.data(), static_cast<int>(Cosmology::kQuintessencePotentialCount))) {
			energy.potential = static_cast<Cosmology::QuintessencePotential>(potential);
			changed = true;
		}
		render_setting_tooltip("Scalar field potential. The amplitude is solved by a shooting method so that H = H0 today.");
		changed |= real_slider("Potential Slope (lambda / alpha)", &energy.potential_slope, 0.01, 20.0, "%.3f", true);
		changed |= real_slider("Potential Scale (f)", &energy.potential_scale, 0.05, 50.0, "%.3f", true);
		changed |= real_slider("Initial Field Value", &energy.initial_field, 0.01, 20.0, "%.3f", true);
		render_setting_tooltip("Field value at the start of the integration, a = exp(-6.9).");
		changed |= real_slider("Chaplygin Stiffness A_s", &energy.chaplygin_stiffness, 0.0, 1.0, "%.3f");
		changed |= real_slider("Chaplygin Exponent alpha", &energy.chaplygin_exponent, 0.0, 10.0, "%.3f");
		render_setting_tooltip("Generalized Chaplygin gas with p = -A / rho^alpha, interpolating between dust and a cosmological constant.");
		if (changed) {
			energy.sanitize();
		}

		const uint32_t metric_id = cosmology_metric_id(orchestrator.active_metric_name());
		const auto expansion = orchestrator.cosmology_expansion(metric_id);
		const ImVec4 status_color = expansion.diagnostics.converged ? ImVec4(0.3f, 1.0f, 0.4f, 1.0f) : ImVec4(1.0f, 0.6f, 0.3f, 1.0f);
		ImGui::TextColored(status_color, "Background Solution: %s", expansion.diagnostics.converged ? "Converged" : "Fallback to LambdaCDM");
		ImGui::TextDisabled("w0: %.4f | Omega_DE: %.4f | q0: %.4f | Age: %.4f / H0", expansion.diagnostics.equation_of_state_today, expansion.diagnostics.dark_energy_fraction_today, expansion.diagnostics.deceleration_today, expansion.diagnostics.age_in_hubble_times);
		if (ImGui::Button("Reset Dark Energy Parameters", ImVec2(250.0f, 24.0f))) {
			energy = Cosmology::DarkEnergyParameters{};
			changed = true;
		}
		return changed;
	}

	static bool render_evaporation(Semiclassical::EvaporationSettings& evaporation) noexcept {
		bool changed = false;
		changed |= ImGui::Checkbox("Enable Black Hole Evaporation", &evaporation.enabled);
		render_setting_tooltip("Integrates the central mass and spin under the renormalized stress tensor flux every simulation step. The Evaporating Vaidya metric additionally resolves the retarded mass loss along each ray.");

		int spectrum = static_cast<int>(evaporation.spectrum);
		if (ImGui::Combo("Emission Spectrum", &spectrum, Semiclassical::kEmissionSpectrumNames.data(), static_cast<int>(Semiclassical::kEmissionSpectrumNames.size()))) {
			evaporation.spectrum = static_cast<Semiclassical::EmissionSpectrum>(spectrum);
			changed = true;
		}
		if (evaporation.spectrum == Semiclassical::EmissionSpectrum::Custom) {
			changed |= real_slider("Effective Degrees Of Freedom", &evaporation.custom_degrees_of_freedom, 0.1, 1000.0, "%.2f", true);
		}
		int model = static_cast<int>(evaporation.model);
		if (ImGui::Combo("Stress Tensor Model", &model, Semiclassical::kStressTensorModelNames.data(), static_cast<int>(Semiclassical::kStressTensorModelNames.size()))) {
			evaporation.model = static_cast<Semiclassical::StressTensorModel>(model);
			changed = true;
		}
		render_setting_tooltip("Greybody blackbody emission over the horizon area, or the Polyakov two-dimensional renormalized stress tensor in the Unruh state.");
		changed |= real_slider("Greybody Factor", &evaporation.greybody_factor, 0.01, 100.0, "%.3f", true);
		changed |= real_slider("Time Acceleration", &evaporation.time_acceleration, 1.0, 1.0e60, "%.3e", true);
		render_setting_tooltip("Multiplier applied to the physical evaporation clock so that stellar-mass black holes evaporate on visible timescales.");
		changed |= real_slider("Spin-Down Ratio", &evaporation.spin_down_ratio, 0.0, 10.0, "%.3f");
		render_setting_tooltip("Ratio between the angular momentum and mass loss rates, in units of the normalized spin.");
		changed |= real_slider("Remnant Mass (kg)", &evaporation.minimum_mass_kg, 1.0e-12, 1.0e40, "%.3e", true);
		changed |= real_slider("Detector Gap (eV)", &evaporation.detector_gap_ev, 1.0e-15, 1.0e12, "%.3e", true);
		changed |= real_slider("Detector Coupling", &evaporation.detector_coupling, 1.0e-6, 1.0e3, "%.3e", true);
		render_setting_tooltip("Unruh-DeWitt detector parameters used for the excitation rate readout below.");
		if (changed) {
			evaporation.sanitize();
		}
		if (ImGui::Button("Reset Evaporation Settings", ImVec2(240.0f, 24.0f))) {
			evaporation = Semiclassical::EvaporationSettings{};
			changed = true;
		}
		return changed;
	}

	static void render_live_thermodynamics(OrchestratorType& orchestrator) noexcept {
		const auto& params = orchestrator.parameters();
		const auto& evaporation = params.cosmology.evaporation;
		const auto& constants = orchestrator.constants_engine();
		const double mass_kg = params.mass * constants.mass_scale();
		const double spin_star = (params.mass > 0.0) ? std::clamp(params.spin / params.mass, -0.999, 0.999) : 0.0;
		if (!(mass_kg > 0.0)) {
			ImGui::TextDisabled("No central mass: no horizon thermodynamics.");
			return;
		}
		const auto horizon = Semiclassical::KerrHorizon::evaluate(mass_kg, spin_star);
		ImGui::TextDisabled("Mass: %.4e kg | Spin a*: %.4f | Outer Horizon: %.4e m", mass_kg, spin_star, horizon.outer_radius_m);
		ImGui::TextDisabled("Surface Gravity: %.4e m/s^2 | Hawking Temperature: %.4e K | Area: %.4e m^2", horizon.surface_gravity, horizon.temperature_k, horizon.area_m2);
		const double luminosity = Semiclassical::HawkingRadiation::luminosity_watts(mass_kg, spin_star, evaporation);
		const double loss = Semiclassical::HawkingRadiation::mass_loss_rate(mass_kg, spin_star, evaporation);
		ImGui::TextDisabled("Luminosity: %.4e W | Mass Loss: %.4e kg/s | Lifetime: %.4e s", luminosity, loss, Semiclassical::HawkingRadiation::lifetime_seconds(mass_kg, evaporation));
		const auto fluxes = Semiclassical::RenormalizedStressTensor::unruh_state_fluxes(horizon.surface_gravity);
		ImGui::TextDisabled("Unruh-State Flux: outgoing %.4e | ingoing %.4e W", fluxes.outgoing_at_infinity, fluxes.ingoing_at_horizon);
		if (mass_kg <= evaporation.minimum_mass_kg * (1.0 + 1.0e-9)) {
			ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "Remnant mass reached: evaporation has stopped.");
		}

		const double radius_m = orchestrator.camera().radius * constants.length_scale();
		const double acceleration = Semiclassical::UnruhEffect::static_observer_acceleration(mass_kg, radius_m);
		if (acceleration < 0.0) {
			ImGui::TextDisabled("Camera lies inside the horizon: no static observer exists.");
			return;
		}
		const double unruh_temperature = Semiclassical::UnruhEffect::temperature_k(acceleration);
		const double local_hawking = Semiclassical::UnruhEffect::local_hawking_temperature_k(horizon.temperature_k, mass_kg, radius_m);
		const double gap_joule = evaporation.detector_gap_ev * Core::PhysicalConstants<double>::ELEMENTARY_CHARGE;
		ImGui::TextDisabled("Static Camera: proper acceleration %.4e m/s^2 | Unruh T %.4e K | Local Hawking T %.4e K", acceleration, unruh_temperature, local_hawking);
		ImGui::TextDisabled("Detector: occupation %.4e | excitation rate %.4e Hz", Semiclassical::UnruhEffect::thermal_occupation(gap_joule, unruh_temperature), Semiclassical::UnruhEffect::detector_excitation_rate(gap_joule, acceleration, evaporation.detector_coupling));
	}

public:
	void render(OrchestratorType& orchestrator) noexcept {
		auto& settings = orchestrator.parameters().cosmology;
		const std::string metric_name = orchestrator.active_metric_name();
		bool changed = false;

		ImGui::TextColored(ImVec4(0.3f, 0.9f, 1.0f, 1.0f), "Cosmology & Semiclassical Gravity");
		ImGui::TextDisabled("Active spacetime: %s", metric_name.c_str());
		const auto cosmology_note = cosmology_metric_note(metric_name);
		if (!cosmology_note.empty()) {
			ImGui::TextWrapped("%s", std::string(cosmology_note).c_str());
		}
		ImGui::Separator();

		if (ImGui::CollapsingHeader("Lemaitre-Tolman-Bondi Void", ImGuiTreeNodeFlags_DefaultOpen)) {
			changed |= render_lemaitre_tolman_bondi(settings);
		}
		if (ImGui::CollapsingHeader("Dark Energy: Quintessence & Chaplygin Gas", ImGuiTreeNodeFlags_DefaultOpen)) {
			changed |= render_dark_energy(orchestrator, settings);
		}
		if (ImGui::CollapsingHeader("Hawking Evaporation & Unruh Effect", ImGuiTreeNodeFlags_DefaultOpen)) {
			changed |= render_evaporation(settings.evaporation);
			const auto note = evaporation_metric_incompatibility(metric_name, settings.evaporation.enabled);
			if (!note.empty()) {
				ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "%s", std::string(note).c_str());
			}
			render_live_thermodynamics(orchestrator);
		}

		if (changed) {
			settings.sanitize();
			orchestrator.notify_state_changed();
		}
	}
};

}

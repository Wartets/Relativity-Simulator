#pragma once

#include "relativistic/core/physical_constants_engine.hpp"
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/orchestrator/command.hpp"
#include "relativistic/ui/tooltip_utils.hpp"
#include "relativistic/units/unit_system.hpp"
#include "relativistic/ui/numeric_slider_utils.hpp"
#include <imgui.h>
#include <cmath>
#include <algorithm>
#include <string>

namespace Relativistic::UI {

class ConstantsWindow {
private:
	bool is_open_{false};
	Orchestrator::SimulationOrchestrator<1024>& orchestrator_;
	double c_quantity_{0.0};
	double g_quantity_{0.0};
	double h_quantity_{0.0};
	double kb_quantity_{0.0};
	double ke_quantity_{0.0};
	double na_quantity_{0.0};
	double kcd_quantity_{0.0};
	std::string c_expr_error_{};
	std::string g_expr_error_{};
	std::string h_expr_error_{};
	std::string kb_expr_error_{};
	std::string ke_expr_error_{};
	std::string na_expr_error_{};
	std::string kcd_expr_error_{};

	[[nodiscard]] static float value_to_slider(double value, double min_val, double max_val) noexcept {
		if (value <= 0.0) return 0.0f;
		const double clamped = std::clamp(value, min_val, max_val);
		return static_cast<float>((std::log10(clamped) - std::log10(min_val)) / (std::log10(max_val) - std::log10(min_val)));
	}

	[[nodiscard]] static double slider_to_value(float slider, double min_val, double max_val) noexcept {
		return std::pow(10.0, std::log10(min_val) + static_cast<double>(slider) * (std::log10(max_val) - std::log10(min_val)));
	}

	void render_log_constant_slider(const char* label, double current_value, double min_val, double max_val, Orchestrator::ParameterType param_type) noexcept {
		float slider_pos = value_to_slider(current_value, min_val, max_val);

		ImGui::PushID(label);
		ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - 130.0f);
		if (ImGui::SliderFloat("##slider", &slider_pos, 0.0f, 1.0f, "%.3f")) {
			const double new_value = slider_to_value(slider_pos, min_val, max_val);
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(param_type, new_value)));
		}
		ImGui::SameLine();
		double input_value = current_value;
		ImGui::SetNextItemWidth(120.0f);
		if (ImGui::InputDouble("##input", &input_value, 0.0, 0.0, "%.6e")) {
			const double clamped = std::clamp(input_value, min_val, max_val);
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(param_type, clamped)));
		}
		ImGui::SameLine();
		ImGui::TextUnformatted(label);
		ImGui::SameLine();
		ImGui::TextDisabled("(?)");
		render_setting_tooltip("Adjust this simulation constant using either the logarithmic slider or the numeric input. The value is constrained to the supported range of this control. Unit-aware editing is available in the quantity field directly below when the constant has a dimensional representation.");
		ImGui::PopID();
	}

	static void draw_derived_row(const char* label, double value) noexcept {
		ImGui::Text("%s:", label);
		ImGui::SameLine(320.0f);
		ImGui::Text("%.8e", value);
	}

public:
	explicit ConstantsWindow(Orchestrator::SimulationOrchestrator<1024>& orchestrator) noexcept
		: orchestrator_(orchestrator) {}

	[[nodiscard]] bool& open_state() noexcept { return is_open_; }

	void render() {
		if (!is_open_) return;

		ImGui::SetNextWindowPos(ImVec2(200.0f, 200.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(640.0f, 720.0f), ImGuiCond_FirstUseEver);

		if (!ImGui::Begin("Physical Constants Engine", &is_open_)) {
			ImGui::End();
			return;
		}

		auto& engine = orchestrator_.constants_engine();

		ImGui::TextColored(ImVec4(0.3f, 0.9f, 1.0f, 1.0f), "Constants Preset");
		const char* presets[] = {"SI (International System)", "Planck (Natural Units)", "Custom"};
		int preset_idx = static_cast<int>(engine.active_preset());
		if (ImGui::Combo("Base Preset", &preset_idx, presets, IM_ARRAYSIZE(presets)) && preset_idx != static_cast<int>(Core::ConstantsPreset::Custom)) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::ConstantsPresetSelect, static_cast<double>(preset_idx))));
		}
		render_setting_tooltip("Selects the fundamental system of units the simulation's base constants are expressed in. SI restores standard real-world values (scaling factors of 1). Planck sets c=G=hbar=kB=Ke=1, the natural unit system used in theoretical relativity. Editing any constant below automatically switches this to Custom.");

		if (engine.active_preset() == Core::ConstantsPreset::Custom) {
			ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.3f, 1.0f), "Custom base constants active: every geometrized length, mass, time, charge, and temperature scale below is now rescaled consistently relative to SI, and the post-Newtonian N-body integrator follows these same values.");
		}

		ImGui::Separator();
		ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Editable Base Constants");
		ImGui::TextDisabled("Set each constant numerically or use the unit-aware field below it. Derived values and scale factors update automatically.");
		ImGui::Spacing();

		render_log_constant_slider("Speed of Light (c)", engine.sim_speed_of_light(), 1e-6, 1e12, Orchestrator::ParameterType::ConstantSimC);
		render_setting_tooltip("Fundamental speed limit of the simulated spacetime. Changing this rescales the time and length unit conversion factors (T0, L0) relative to real-world SI values.");
		c_quantity_ = engine.sim_speed_of_light();
		if (smart_quantity_input("c  [dim: L T^-1]", &c_quantity_, Units::Dimensions::Velocity, c_expr_error_)) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::ConstantSimC, c_quantity_)));
		}
		render_setting_tooltip("Enter the speed of light as a plain value or as a unit-aware expression, for example \"299792458 m/s\". The value must have dimensions of velocity. Changing c updates the simulation time and length scaling factors.");

		render_log_constant_slider("Gravitational Constant (G)", engine.sim_gravitational_constant(), 1e-20, 1e20, Orchestrator::ParameterType::ConstantSimG);
		render_setting_tooltip("Strength of gravitational coupling. Directly scales the mass unit conversion factor (M0) and every Schwarzschild radius computed by the engine.");
		g_quantity_ = engine.sim_gravitational_constant();
		if (smart_quantity_input("G  [dim: L^3 M^-1 T^-2]", &g_quantity_, Units::Dimensions::GravitationalConstant, g_expr_error_)) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::ConstantSimG, g_quantity_)));
		}
		render_setting_tooltip("Enter the gravitational constant as a plain value or as a unit-aware expression, for example \"6.674e-11 m^3/(kg*s^2)\". The value must have dimensions of gravitational constant.");

		render_log_constant_slider("Planck Constant (h)", engine.sim_planck_constant(), 1e-40, 1e10, Orchestrator::ParameterType::ConstantSimH);
		render_setting_tooltip("Quantum of action. Governs the reduced Planck constant (hbar), the time unit scale (T0), and the Stefan-Boltzmann constant derived below.");
		h_quantity_ = engine.sim_planck_constant();
		if (smart_quantity_input("h  [dim: L^2 M T^-1]", &h_quantity_, Units::Dimensions::PlanckAction, h_expr_error_)) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::ConstantSimH, h_quantity_)));
		}
		render_setting_tooltip("Enter Planck's constant as a plain value or as a unit-aware expression, for example \"6.62607015e-34 J*s\". The value must have dimensions of action.");

		render_log_constant_slider("Boltzmann Constant (kB)", engine.sim_boltzmann_constant(), 1e-30, 1e10, Orchestrator::ParameterType::ConstantSimKB);
		render_setting_tooltip("Relates thermal energy to temperature. Scales the temperature unit conversion factor (K0) and the Stefan-Boltzmann radiation constant.");
		kb_quantity_ = engine.sim_boltzmann_constant();
		if (smart_quantity_input("kB  [dim: L^2 M T^-2 Theta^-1]", &kb_quantity_, Units::Dimensions::BoltzmannConstant, kb_expr_error_)) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::ConstantSimKB, kb_quantity_)));
		}
		render_setting_tooltip("Enter the Boltzmann constant as a plain value or as a unit-aware expression, for example \"1.380649e-23 J/K\". The value must have dimensions of energy per temperature.");

		render_log_constant_slider("Avogadro Constant (NA)", engine.sim_avogadro_constant(), 1e10, 1e30, Orchestrator::ParameterType::ConstantSimNA);
		render_setting_tooltip("Number of elementary entities per mole. Sets the amount-of-substance unit conversion factor (N0). Rarely needs adjustment for relativistic simulations.");
		na_quantity_ = engine.sim_avogadro_constant();
		if (smart_quantity_input("NA  [entities per mole, dimensionless in this engine]", &na_quantity_, Units::Dimensions::Dimensionless, na_expr_error_)) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::ConstantSimNA, na_quantity_)));
		}
		render_setting_tooltip("Enter the Avogadro constant as a plain value or as a scientific-notation expression, for example \"6.02214076e23\". Amount-of-substance is not tracked as a physical dimension by this engine, so only dimensionless expressions are accepted here.");

		ImGui::Spacing();

		render_log_constant_slider("Coulomb Constant (Ke)", engine.sim_coulomb_constant(), 1e-6, 1e20, Orchestrator::ParameterType::ConstantSimKe);
		render_setting_tooltip("Electrostatic coupling strength. Governs the charge unit conversion factor (Q0), vacuum permittivity, vacuum permeability, and the magnetic coupling constant.");
		ke_quantity_ = engine.sim_coulomb_constant();
		if (smart_quantity_input("Ke  [dim: L^3 M T^-4 I^-2]", &ke_quantity_, Units::Dimensions::CoulombConstant, ke_expr_error_)) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::ConstantSimKe, ke_quantity_)));
		}
		render_setting_tooltip("Enter the Coulomb constant as a plain value or as a unit-aware expression, for example \"8.9875517923e9 N*m^2/C^2\". The value must have the dimensions of the Coulomb constant.");

		render_log_constant_slider("Luminous Efficacy (Kcd)", engine.sim_luminous_efficacy(), 1e-6, 1e6, Orchestrator::ParameterType::ConstantSimKcd);
		render_setting_tooltip("Luminous intensity scale. Sets the I0 dimensional coefficient used for photometric quantities.");
		kcd_quantity_ = engine.sim_luminous_efficacy();
		if (smart_quantity_input("Kcd  [lm/W, dimensionless in this engine]", &kcd_quantity_, Units::Dimensions::Dimensionless, kcd_expr_error_)) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::ConstantSimKcd, kcd_quantity_)));
		}
		render_setting_tooltip("Enter the luminous efficacy as a plain value or as a scientific-notation expression, for example \"683\". Luminous intensity is not tracked as a physical dimension by this engine, so only dimensionless expressions are accepted here.");

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.6f, 1.0f), "Derived Fundamental Constants");
		draw_derived_row("Reduced Planck Constant (hbar)", engine.sim_reduced_planck());
		draw_derived_row("Elementary Charge (e)", engine.sim_elementary_charge());
		draw_derived_row("Vacuum Permittivity (epsilon0)", engine.sim_vacuum_permittivity());
		draw_derived_row("Vacuum Permeability (mu0)", engine.sim_vacuum_permeability());
		draw_derived_row("Magnetic Coupling (Km = Ke/c^2)", engine.sim_magnetic_coupling());
		draw_derived_row("Stefan-Boltzmann Constant (sigma)", engine.sim_stefan_boltzmann());
		draw_derived_row("Fine-Structure Constant (alpha)", Core::ConstantsEngine::FINE_STRUCTURE_CONSTANT);

		ImGui::Separator();
		ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.4f, 1.0f), "Astrophysical Reference Quantities (Simulation Units)");
		draw_derived_row("Solar Mass", engine.sim_solar_mass());
		draw_derived_row("Astronomical Unit", engine.sim_astronomical_unit());
		draw_derived_row("Earth Mass", engine.sim_earth_mass());
		render_setting_tooltip("Mass of Earth converted into current simulation mass units via the Mass Scale factor below. Useful for sizing small orbiting bodies realistically in the Body Manager.");
		draw_derived_row("Jupiter Mass", engine.sim_jupiter_mass());
		render_setting_tooltip("Mass of Jupiter converted into current simulation mass units. A convenient reference point for gas-giant-scale N-body bodies.");
		draw_derived_row("Parsec", engine.sim_parsec());
		render_setting_tooltip("One parsec (3.2616 light years) converted into current simulation length units via the Length Scale factor below. Useful for placing bodies at galactic distances.");
		draw_derived_row("Light Year", engine.sim_light_year());
		render_setting_tooltip("Distance light travels in one Julian year, converted into current simulation length units.");
		draw_derived_row("Electron Mass", engine.sim_electron_mass());
		draw_derived_row("Proton Mass", engine.sim_proton_mass());
		draw_derived_row("Neutron Mass", engine.sim_neutron_mass());
		draw_derived_row("Solar Schwarzschild Radius", engine.sim_solar_schwarzschild_radius());
		draw_derived_row("Thomson Cross Section", engine.sim_thomson_cross_section());
		render_setting_tooltip("Electron scattering cross section, converted into simulation area units (Length Scale squared). Governs opacity in inverse-Compton and radiative transfer calculations throughout the optics module.");
		draw_derived_row("Wien Displacement Constant", engine.sim_wien_displacement_constant());
		render_setting_tooltip("Relates blackbody peak emission wavelength to temperature (lambda_max times T = constant), converted into simulation length-times-temperature units.");
		draw_derived_row("Stefan-Boltzmann (CODATA Reference)", Core::ConstantsEngine::reference_stefan_boltzmann_constant());
		render_setting_tooltip("Fixed real-world CODATA value shown for comparison against the computed Stefan-Boltzmann Constant above, which drifts under Custom or Planck presets since it is derived from h, kB, and c rather than fixed independently.");

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.6f, 0.85f, 1.0f, 1.0f), "Dimensional Scaling Factors (SI = Simulation x Factor)");
		const auto& unit_prefs = orchestrator_.unit_preferences();
		draw_derived_row("Time Scale (T0)", engine.time_scale());
		render_setting_tooltip("Number of SI seconds represented by one simulation time unit.");
		ImGui::TextDisabled("Displayed As: %s", Units::format_time(engine.time_scale(), unit_prefs.time).c_str());

		draw_derived_row("Length Scale (L0)", engine.length_scale());
		render_setting_tooltip("Number of SI meters represented by one simulation length unit.");
		ImGui::TextDisabled("Displayed As: %s", Units::format_distance(engine.length_scale(), unit_prefs.distance).c_str());

		draw_derived_row("Mass Scale (M0)", engine.mass_scale());
		render_setting_tooltip("Number of SI kilograms represented by one simulation mass unit.");
		ImGui::TextDisabled("Displayed As: %s", Units::format_mass(engine.mass_scale(), unit_prefs.mass).c_str());

		draw_derived_row("Charge Scale (Q0)", engine.charge_scale());
		render_setting_tooltip("Number of SI Coulombs represented by one simulation charge unit.");
		ImGui::TextDisabled("Displayed As: %s", Units::format_charge(engine.charge_scale(), unit_prefs.charge).c_str());

		draw_derived_row("Temperature Scale (K0)", engine.temperature_scale());
		render_setting_tooltip("Number of SI Kelvin represented by one simulation temperature unit.");
		ImGui::TextDisabled("Displayed As: %s", Units::format_temperature(engine.temperature_scale(), unit_prefs.temperature).c_str());

		draw_derived_row("Amount Scale (N0)", engine.amount_scale());
		render_setting_tooltip("Number of SI moles represented by one simulation amount-of-substance unit.");

		draw_derived_row("Luminous Intensity Scale (I0)", engine.luminous_intensity_scale());
		render_setting_tooltip("Number of SI candela represented by one simulation luminous intensity unit.");

		draw_derived_row("Current Scale (A0 = Q0/T0)", engine.current_scale());
		render_setting_tooltip("Number of SI Amperes represented by one simulation electric current unit.");
		ImGui::TextDisabled("Displayed As: %s", Units::format_current(engine.current_scale(), unit_prefs.current).c_str());

		ImGui::Separator();
		if (ImGui::Button("Reset To SI Defaults", ImVec2(180.0f, 26.0f))) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::ConstantsPresetSelect, static_cast<double>(static_cast<uint32_t>(Core::ConstantsPreset::SI)))));
		}
		ImGui::SameLine();
		if (ImGui::Button("Apply Planck Units", ImVec2(180.0f, 26.0f))) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::ConstantsPresetSelect, static_cast<double>(static_cast<uint32_t>(Core::ConstantsPreset::Planck)))));
		}

		ImGui::End();
	}
};

}

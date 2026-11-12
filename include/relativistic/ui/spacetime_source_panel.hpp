#pragma once

#include <imgui.h>
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/metrics/horizon_regime.hpp"
#include "relativistic/ui/horizon_zone_strip.hpp"
#include "relativistic/ui/compatibility_notes.hpp"
#include "relativistic/ui/numeric_slider_utils.hpp"
#include "relativistic/ui/tooltip_utils.hpp"
#include "relativistic/ui/spacetime_source_state.hpp"
#include "relativistic/units/unit_system.hpp"
#include "relativistic/units/unit_aware_widgets.hpp"
#include <algorithm>
#include <cmath>
#include <string>

namespace Relativistic::UI {

struct SpacetimeSourcePanelOptions {
	const Metrics::HorizonConstraintSet* constraints{nullptr};
	bool* guarantee_horizon{nullptr};
	const std::string* metric_name{nullptr};
	const char* mass_label{"Mass (M)"};
	bool show_mass{true};
	bool show_spin{true};
	bool show_charge{true};
	bool show_lambda{false};
	bool render_lock_controls{false};
	bool constrain_mass{true};
	bool spin_non_negative{false};
	double mass_slider_minimum{0.01};
	double mass_slider_maximum{100.0};
	double mass_log_minimum{0.01};
	double mass_log_maximum{100.0};
	double charge_limit{0.0};
};

struct SpacetimeSourcePanelResult {
	bool mass_changed{false};
	bool spin_changed{false};
	bool charge_changed{false};
	bool cosmological_constant_changed{false};

	[[nodiscard]] bool any() const noexcept {
		return mass_changed || spin_changed || charge_changed || cosmological_constant_changed;
	}
};

[[nodiscard]] inline SpacetimeSourcePanelResult render_spacetime_source_panel(
	Orchestrator::SimulationOrchestrator<1024>& orchestrator,
	SpacetimeSourceEditState& edit,
	SpacetimeSourceValues& values,
	const SpacetimeSourcePanelOptions& options
) noexcept {
	SpacetimeSourcePanelResult result;
	const Metrics::HorizonConstraintSet* constraints = options.constraints;
	const bool has_constraints = (constraints != nullptr) && constraints->any();
	const auto& prefs = orchestrator.unit_preferences();

	const auto current_state = [&values]() noexcept {
		Metrics::HorizonState state;
		state.mass = values.mass;
		state.spin = values.spin;
		state.charge = values.charge;
		state.cosmological_constant = values.cosmological_constant;
		return state;
	};
	const auto locked = [&]() noexcept -> bool {
		return has_constraints && options.guarantee_horizon != nullptr && *options.guarantee_horizon;
	};
	const auto constrain = [&](Metrics::HorizonParameter parameter, double value) noexcept -> double {
		if (!locked()) {
			return value;
		}
		return Metrics::HorizonRegimeAnalyzer::constrain(parameter, value, current_state(), *constraints);
	};
	const auto slider_extent = [&](Metrics::HorizonParameter parameter) noexcept -> double {
		const double unlocked_extent = std::max(1.25 * values.mass, 1.0e-3);
		if (!locked()) {
			return unlocked_extent;
		}
		const double limit = Metrics::HorizonRegimeAnalyzer::guaranteed_maximum(parameter, current_state(), *constraints);
		return std::clamp(limit, 1.0e-6, unlocked_extent);
	};
	const auto render_strip = [&](const char* id, Metrics::HorizonParameter parameter, double range_min, double range_max, bool logarithmic) noexcept {
		if (constraints == nullptr) {
			return;
		}
		const Metrics::HorizonState state = current_state();
		const Metrics::HorizonZoneLayout layout = Metrics::HorizonRegimeAnalyzer::zone_layout(parameter, state, *constraints);
		if (!layout.valid) {
			return;
		}
		render_horizon_zone_strip(
			id, layout, range_min, range_max,
			Metrics::HorizonRegimeAnalyzer::value_of(parameter, state), logarithmic,
			Metrics::HorizonRegimeAnalyzer::classify(state, *constraints),
			Metrics::HorizonRegimeAnalyzer::describe(state, *constraints)
		);
	};

	if (options.render_lock_controls && has_constraints && options.guarantee_horizon != nullptr) {
		const Metrics::HorizonRegime regime = Metrics::HorizonRegimeAnalyzer::classify(current_state(), *constraints);
		const ImVec4 locked_color(0.12f, 0.45f, 0.22f, 1.0f);
		const ImVec4 unlocked_color(0.45f, 0.30f, 0.12f, 1.0f);
		const ImVec4 base_color = *options.guarantee_horizon ? locked_color : unlocked_color;
		ImGui::PushStyleColor(ImGuiCol_Button, base_color);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(base_color.x + 0.1f, base_color.y + 0.1f, base_color.z + 0.1f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(base_color.x + 0.18f, base_color.y + 0.18f, base_color.z + 0.18f, 1.0f));
		const bool toggled = ImGui::Button(*options.guarantee_horizon ? "Horizon Guaranteed (Locked)" : "Guarantee Horizon (Unlocked)", ImVec2(250.0f, 24.0f));
		ImGui::PopStyleColor(3);
		if (toggled) {
			*options.guarantee_horizon = !*options.guarantee_horizon;
			if (*options.guarantee_horizon) {
				Metrics::HorizonState state = current_state();
				if (Metrics::HorizonRegimeAnalyzer::project(state, *constraints)) {
					result.mass_changed = (state.mass != values.mass);
					result.spin_changed = (state.spin != values.spin);
					result.charge_changed = (state.charge != values.charge);
					result.cosmological_constant_changed = (state.cosmological_constant != values.cosmological_constant);
					values.mass = state.mass;
					values.spin = state.spin;
					values.charge = state.charge;
					values.cosmological_constant = state.cosmological_constant;
				}
			}
		}
		render_setting_tooltip("When locked, mass, spin, charge and cosmological constant can never be edited beyond the extremal limit a^2 + Q^2 = M^2 (or 9 Lambda M^2 = 1), so an event horizon always exists. Locking also pulls an already super-extremal configuration back inside the physical domain.");
		ImGui::SameLine();
		ImGui::TextColored(horizon_regime_color(regime), "%s", Metrics::HorizonRegimeAnalyzer::regime_name(regime));
		render_horizon_zone_legend();
	}

	if (options.show_mass) {
		const double mass_scale = orchestrator.constants_engine().mass_scale();
		double mass_kg = values.mass * mass_scale;
		if (unit_aware_slider_double(options.mass_label, &mass_kg, options.mass_slider_minimum * mass_scale, options.mass_slider_maximum * mass_scale, UnitCategory::Mass, prefs, "%.3f", &edit.mass_log_mode, options.mass_log_minimum * mass_scale, options.mass_log_maximum * mass_scale)) {
			const double requested = mass_kg / mass_scale;
			values.mass = options.constrain_mass ? constrain(Metrics::HorizonParameter::Mass, requested) : requested;
			result.mass_changed = true;
		}
		const std::string mass_tooltip = "Gravitating mass in " + std::string(Units::mass_unit_suffix(prefs.mass)) +
			". Non-rotating horizon rs = " + std::to_string(2.0 * values.mass).substr(0, 5) +
			" M and photon orbit r_ph = " + std::to_string(3.0 * values.mass).substr(0, 5) + " M.";
		render_setting_tooltip(mass_tooltip.c_str());
		render_strip("##MassHorizonZones", Metrics::HorizonParameter::Mass, options.mass_slider_minimum, options.mass_slider_maximum, edit.mass_log_mode);

		edit.mass_quantity_kg = values.mass * mass_scale;
		if (smart_quantity_input("M  [dim: Mass]", &edit.mass_quantity_kg, Units::Dimensions::Mass, edit.mass_expression_error)) {
			if (mass_scale > 0.0) {
				const double requested = edit.mass_quantity_kg / mass_scale;
				values.mass = options.constrain_mass ? constrain(Metrics::HorizonParameter::Mass, requested) : requested;
				result.mass_changed = true;
			}
		}
		render_setting_tooltip("Enter the mass with an arbitrary unit expression (e.g. '1.989e30 kg', '4.3e6 msun'). Automatically scaled via the active constants.");
	}

	if (options.show_spin) {
		const float spin_extent = static_cast<float>(slider_extent(Metrics::HorizonParameter::Spin));
		const float spin_minimum = options.spin_non_negative ? 0.0f : -spin_extent;
		float spin = static_cast<float>(values.spin);
		if (slider_float_with_input("Spin Parameter (a)", &spin, spin_minimum, spin_extent, "%.4f")) {
			values.spin = constrain(Metrics::HorizonParameter::Spin, static_cast<double>(spin));
			result.spin_changed = true;
		}
		const double spin_ratio = std::abs(values.spin) / std::max(values.mass, 1e-6);
		const std::string spin_info = "Dimensionless spin a/M = " + std::to_string(spin_ratio).substr(0, 5) +
			(spin_ratio > 0.95 ? " (near-extremal ergosphere deformation)" : " (standard frame dragging)");
		std::string spin_warning_text;
		if (options.metric_name != nullptr) {
			const auto spin_warning = metric_spin_incompatibility(*options.metric_name, values.spin, values.mass);
			spin_warning_text = std::string(spin_warning);
		}
		if (!spin_warning_text.empty()) {
			render_setting_tooltip_warning(spin_info.c_str(), spin_warning_text.c_str());
		} else {
			render_setting_tooltip(spin_info.c_str());
		}
		render_strip("##SpinHorizonZones", Metrics::HorizonParameter::Spin, static_cast<double>(spin_minimum), static_cast<double>(spin_extent), false);
	}

	if (options.show_charge) {
		double charge_extent = slider_extent(Metrics::HorizonParameter::Charge);
		if (options.charge_limit > 0.0) {
			charge_extent = std::min(charge_extent, options.charge_limit);
		}
		charge_extent = std::max(charge_extent, 1.0e-6);
		double charge = values.charge;
		if (unit_aware_slider_double("Electric Charge (Q)", &charge, -charge_extent, charge_extent, UnitCategory::Charge, prefs, "%.3f")) {
			values.charge = constrain(Metrics::HorizonParameter::Charge, charge);
			result.charge_changed = true;
		}
		const double charge_ratio = std::abs(values.charge) / std::max(values.mass, 1e-6);
		const std::string charge_info = "Electrostatic charge in " + std::string(Units::charge_unit_suffix(prefs.charge)) +
			" (|Q|/M = " + std::to_string(charge_ratio).substr(0, 5) + "). Repulsive geometry shifts the outer horizon inwards.";
		render_setting_tooltip(charge_info.c_str());
		render_strip("##ChargeHorizonZones", Metrics::HorizonParameter::Charge, -charge_extent, charge_extent, false);
	}

	if (options.show_lambda) {
		const float lambda_extent = static_cast<float>(slider_extent(Metrics::HorizonParameter::CosmologicalConstant));
		const float lambda_max = std::clamp(lambda_extent, 1e-4f, 1e-2f);
		float lambda_val = static_cast<float>(values.cosmological_constant);
		if (slider_float_with_input("Cosmological Lambda", &lambda_val, 1e-8f, lambda_max, "%.2e", &edit.lambda_log_mode)) {
			values.cosmological_constant = constrain(Metrics::HorizonParameter::CosmologicalConstant, static_cast<double>(lambda_val));
			result.cosmological_constant_changed = true;
		}
		render_setting_tooltip("Cosmological constant. Establishes asymptotic de Sitter cosmological horizon r_c ~ sqrt(3/Lambda).");
		render_strip("##LambdaHorizonZones", Metrics::HorizonParameter::CosmologicalConstant, 1e-8, static_cast<double>(lambda_max), edit.lambda_log_mode);
	}

	return result;
}

}

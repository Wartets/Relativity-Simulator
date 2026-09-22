#pragma once

#include "relativistic/units/unit_system.hpp"
#include "relativistic/ui/numeric_slider_utils.hpp"
#include <imgui.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <limits>

namespace Relativistic::UI {

enum class UnitCategory : uint32_t {
	Distance = 0,
	Mass,
	Velocity,
	Energy,
	Angle,
	Temperature,
	Charge,
	Current
};

[[nodiscard]] inline const char* unit_category_suffix(UnitCategory category, const Units::UnitDisplayPreferences& prefs) noexcept {
	switch (category) {
		case UnitCategory::Distance: return Units::distance_unit_suffix(prefs.distance);
		case UnitCategory::Mass: return Units::mass_unit_suffix(prefs.mass);
		case UnitCategory::Velocity: return Units::velocity_unit_suffix(prefs.velocity);
		case UnitCategory::Energy: return Units::energy_unit_suffix(prefs.energy);
		case UnitCategory::Angle: return Units::angle_unit_suffix(prefs.angle);
		case UnitCategory::Temperature: return Units::temperature_unit_suffix(prefs.temperature);
		case UnitCategory::Charge: return Units::charge_unit_suffix(prefs.charge);
		case UnitCategory::Current: return Units::current_unit_suffix(prefs.current);
		default: return "";
	}
}

[[nodiscard]] inline double unit_category_from_canonical(UnitCategory category, double canonical, const Units::UnitDisplayPreferences& prefs) noexcept {
	switch (category) {
		case UnitCategory::Distance: return Units::convert_distance_from_meters(canonical, prefs.distance);
		case UnitCategory::Mass: return Units::convert_mass_from_kg(canonical, prefs.mass);
		case UnitCategory::Velocity: return Units::convert_velocity_from_mps(canonical, prefs.velocity);
		case UnitCategory::Energy: return Units::convert_energy_from_joules(canonical, prefs.energy);
		case UnitCategory::Angle: return Units::convert_angle_from_radians(canonical, prefs.angle);
		case UnitCategory::Temperature: return Units::convert_temperature_from_kelvin(canonical, prefs.temperature);
		case UnitCategory::Charge: return Units::convert_charge_from_coulombs(canonical, prefs.charge);
		case UnitCategory::Current: return Units::convert_current_from_amperes(canonical, prefs.current);
		default: return canonical;
	}
}

[[nodiscard]] inline double unit_category_to_canonical(UnitCategory category, double displayed, const Units::UnitDisplayPreferences& prefs) noexcept {
	switch (category) {
		case UnitCategory::Distance: return Units::convert_distance_to_meters(displayed, prefs.distance);
		case UnitCategory::Mass: return Units::convert_mass_to_kg(displayed, prefs.mass);
		case UnitCategory::Velocity: return Units::convert_velocity_to_mps(displayed, prefs.velocity);
		case UnitCategory::Energy: return Units::convert_energy_to_joules(displayed, prefs.energy);
		case UnitCategory::Angle: return Units::convert_angle_to_radians(displayed, prefs.angle);
		case UnitCategory::Temperature: return Units::convert_temperature_to_kelvin(displayed, prefs.temperature);
		case UnitCategory::Charge: return Units::convert_charge_to_coulombs(displayed, prefs.charge);
		case UnitCategory::Current: return Units::convert_current_to_amperes(displayed, prefs.current);
		default: return displayed;
	}
}

[[nodiscard]] inline bool unit_aware_slider_double(
	const char* base_label,
	double* canonical_value,
	double canonical_min,
	double canonical_max,
	UnitCategory category,
	const Units::UnitDisplayPreferences& prefs,
	const char* format = "%.4f",
	bool* log_mode = nullptr,
	double canonical_log_min_override = std::numeric_limits<double>::quiet_NaN(),
	double canonical_log_max_override = std::numeric_limits<double>::quiet_NaN()
) noexcept {
	const char* suffix = unit_category_suffix(category, prefs);

	double displayed_min = unit_category_from_canonical(category, canonical_min, prefs);
	double displayed_max = unit_category_from_canonical(category, canonical_max, prefs);
	if (displayed_min > displayed_max) {
		std::swap(displayed_min, displayed_max);
	}

	float log_min_f = std::numeric_limits<float>::quiet_NaN();
	float log_max_f = std::numeric_limits<float>::quiet_NaN();
	if (std::isfinite(canonical_log_min_override) && std::isfinite(canonical_log_max_override)) {
		double dl_min = unit_category_from_canonical(category, canonical_log_min_override, prefs);
		double dl_max = unit_category_from_canonical(category, canonical_log_max_override, prefs);
		if (dl_min > dl_max) {
			std::swap(dl_min, dl_max);
		}
		log_min_f = static_cast<float>(dl_min);
		log_max_f = static_cast<float>(dl_max);
	}

	char labeled[192];
	if (suffix != nullptr && suffix[0] != '\0') {
		std::snprintf(labeled, sizeof(labeled), "%s (%s)", base_label, suffix);
	} else {
		std::snprintf(labeled, sizeof(labeled), "%s", base_label);
	}

	float displayed_value = static_cast<float>(unit_category_from_canonical(category, *canonical_value, prefs));
	const bool changed = slider_float_with_input(
		labeled, &displayed_value,
		static_cast<float>(displayed_min), static_cast<float>(displayed_max),
		format, log_mode, log_min_f, log_max_f
	);

	if (changed) {
		*canonical_value = unit_category_to_canonical(category, static_cast<double>(displayed_value), prefs);
	}
	return changed;
}

}

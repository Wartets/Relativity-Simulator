#pragma once

#include <cstdint>
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <limits>
#include "relativistic/units/unit_system.hpp"
#include "relativistic/ui/numeric_slider_utils.hpp"
#include <imgui.h>

namespace Relativistic::UI {

enum class UnitCategory : uint32_t {
	Distance = 0,
	Mass,
	Velocity,
	Energy,
	Angle,
	Temperature,
	Charge,
	Current,
	Time,
	Acceleration,
	AngularVelocity,
	Density,
	Pressure,
	Power,
	Frequency,
	Force,
	MagneticField,
	Voltage
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
		case UnitCategory::Time: return Units::time_unit_suffix(prefs.time);
		case UnitCategory::Acceleration: return Units::acceleration_unit_suffix(prefs.acceleration);
		case UnitCategory::AngularVelocity: return Units::angular_velocity_unit_suffix(prefs.angular_velocity);
		case UnitCategory::Density: return Units::density_unit_suffix(prefs.density);
		case UnitCategory::Pressure: return Units::pressure_unit_suffix(prefs.pressure);
		case UnitCategory::Power: return Units::power_unit_suffix(prefs.power);
		case UnitCategory::Frequency: return Units::frequency_unit_suffix(prefs.frequency);
		case UnitCategory::Force: return Units::force_unit_suffix(prefs.force);
		case UnitCategory::MagneticField: return Units::magnetic_field_unit_suffix(prefs.magnetic_field);
		case UnitCategory::Voltage: return Units::voltage_unit_suffix(prefs.voltage);
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
		case UnitCategory::Time: return Units::convert_time_from_seconds(canonical, prefs.time);
		case UnitCategory::Acceleration: return Units::convert_acceleration_from_mps2(canonical, prefs.acceleration);
		case UnitCategory::AngularVelocity: return Units::convert_angular_velocity_from_radps(canonical, prefs.angular_velocity);
		case UnitCategory::Density: return Units::convert_density_from_kg_m3(canonical, prefs.density);
		case UnitCategory::Pressure: return Units::convert_pressure_from_pascals(canonical, prefs.pressure);
		case UnitCategory::Power: return Units::convert_power_from_watts(canonical, prefs.power);
		case UnitCategory::Frequency: return Units::convert_frequency_from_hertz(canonical, prefs.frequency);
		case UnitCategory::Force: return Units::convert_force_from_newtons(canonical, prefs.force);
		case UnitCategory::MagneticField: return Units::convert_magnetic_field_from_teslas(canonical, prefs.magnetic_field);
		case UnitCategory::Voltage: return Units::convert_voltage_from_volts(canonical, prefs.voltage);
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
		case UnitCategory::Time: return Units::convert_time_to_seconds(displayed, prefs.time);
		case UnitCategory::Acceleration: return Units::convert_acceleration_to_mps2(displayed, prefs.acceleration);
		case UnitCategory::AngularVelocity: return Units::convert_angular_velocity_to_radps(displayed, prefs.angular_velocity);
		case UnitCategory::Density: return Units::convert_density_to_kg_m3(displayed, prefs.density);
		case UnitCategory::Pressure: return Units::convert_pressure_to_pascals(displayed, prefs.pressure);
		case UnitCategory::Power: return Units::convert_power_to_watts(displayed, prefs.power);
		case UnitCategory::Frequency: return Units::convert_frequency_to_hertz(displayed, prefs.frequency);
		case UnitCategory::Force: return Units::convert_force_to_newtons(displayed, prefs.force);
		case UnitCategory::MagneticField: return Units::convert_magnetic_field_to_teslas(displayed, prefs.magnetic_field);
		case UnitCategory::Voltage: return Units::convert_voltage_to_volts(displayed, prefs.voltage);
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

[[nodiscard]] inline bool unit_aware_input_double3(
	const char* base_label,
	double pos_canonical[3],
	UnitCategory category,
	const Units::UnitDisplayPreferences& prefs,
	const char* format = "%.4f"
) noexcept {
	const char* suffix = unit_category_suffix(category, prefs);
	char labeled[192];
	if (suffix != nullptr && suffix[0] != '\0') {
		std::snprintf(labeled, sizeof(labeled), "%s (%s)", base_label, suffix);
	} else {
		std::snprintf(labeled, sizeof(labeled), "%s", base_label);
	}

	float displayed[3] = {
		static_cast<float>(unit_category_from_canonical(category, pos_canonical[0], prefs)),
		static_cast<float>(unit_category_from_canonical(category, pos_canonical[1], prefs)),
		static_cast<float>(unit_category_from_canonical(category, pos_canonical[2], prefs))
	};

	if (ImGui::InputFloat3(labeled, displayed, format)) {
		pos_canonical[0] = unit_category_to_canonical(category, static_cast<double>(displayed[0]), prefs);
		pos_canonical[1] = unit_category_to_canonical(category, static_cast<double>(displayed[1]), prefs);
		pos_canonical[2] = unit_category_to_canonical(category, static_cast<double>(displayed[2]), prefs);
		return true;
	}
	return false;
}

[[nodiscard]] inline bool unit_aware_input_float3(
	const char* base_label,
	float pos_canonical[3],
	UnitCategory category,
	const Units::UnitDisplayPreferences& prefs,
	const char* format = "%.4f"
) noexcept {
	double double_vec[3] = { static_cast<double>(pos_canonical[0]), static_cast<double>(pos_canonical[1]), static_cast<double>(pos_canonical[2]) };
	if (unit_aware_input_double3(base_label, double_vec, category, prefs, format)) {
		pos_canonical[0] = static_cast<float>(double_vec[0]);
		pos_canonical[1] = static_cast<float>(double_vec[1]);
		pos_canonical[2] = static_cast<float>(double_vec[2]);
		return true;
	}
	return false;
}

[[nodiscard]] inline bool unit_aware_input_double(
	const char* base_label,
	double* canonical_val,
	UnitCategory category,
	const Units::UnitDisplayPreferences& prefs,
	const char* format = "%.4f"
) noexcept {
	const char* suffix = unit_category_suffix(category, prefs);
	char labeled[192];
	if (suffix != nullptr && suffix[0] != '\0') {
		std::snprintf(labeled, sizeof(labeled), "%s (%s)", base_label, suffix);
	} else {
		std::snprintf(labeled, sizeof(labeled), "%s", base_label);
	}

	float displayed = static_cast<float>(unit_category_from_canonical(category, *canonical_val, prefs));
	if (ImGui::InputFloat(labeled, &displayed, 0.0f, 0.0f, format)) {
		*canonical_val = unit_category_to_canonical(category, static_cast<double>(displayed), prefs);
		return true;
	}
	return false;
}

[[nodiscard]] inline bool unit_aware_input_float(
	const char* base_label,
	float* canonical_val,
	UnitCategory category,
	const Units::UnitDisplayPreferences& prefs,
	const char* format = "%.4f"
) noexcept {
	double double_val = static_cast<double>(*canonical_val);
	if (unit_aware_input_double(base_label, &double_val, category, prefs, format)) {
		*canonical_val = static_cast<float>(double_val);
		return true;
	}
	return false;
}

}

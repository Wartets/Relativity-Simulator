#pragma once

#include "relativistic/observer/camera_collision.hpp"
#include "relativistic/observer/coordinate_systems.hpp"
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/units/unit_system.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace Relativistic::UI {

inline constexpr std::array<double, 10> kDistanceUnitMeters{
	1.0, 1.0e3, 0.3048, 1609.344, 1852.0, 1.495978707e11, 9.4607304725808e15, 3.0856775814913673e16, 3.0856775814913673e19, 6.957e8
};

inline constexpr std::array<double, 7> kAngleUnitRadians{
	1.0, 0.017453292519943295, 2.908882086657216e-4, 4.84813681109536e-6, 0.015707963267948967, 6.283185307179586, 1.0e-3
};

[[nodiscard]] inline Observer::CoordinateFrame make_coordinate_frame(const Orchestrator::SimulationOrchestrator<1024>& orchestrator) noexcept {
	const auto& parameters = orchestrator.parameters();
	const std::string& metric = orchestrator.active_metric_name();
	Observer::CoordinateFrame frame;
	frame.mass = std::max(parameters.mass, 0.0);
	frame.outer_horizon = Observer::CameraCollisionField::primary_horizon_radius(parameters, metric);
	frame.inner_horizon = (frame.outer_horizon > 0.0) ? std::max(2.0 * frame.mass - frame.outer_horizon, 0.0) : 0.0;
	frame.spin = (metric.find("Kerr") != std::string::npos) ? std::clamp(parameters.spin, -0.999 * frame.mass, 0.999 * frame.mass) : 0.0;
	return frame;
}

[[nodiscard]] inline double distance_unit_in_meters(Units::DistanceUnit unit) noexcept {
	return kDistanceUnitMeters[std::min(static_cast<size_t>(unit), kDistanceUnitMeters.size() - 1)];
}

[[nodiscard]] inline double angle_unit_in_radians(Units::AngleUnit unit) noexcept {
	return kAngleUnitRadians[std::min(static_cast<size_t>(unit), kAngleUnitRadians.size() - 1)];
}

[[nodiscard]] inline double coordinate_axis_to_display(Observer::CoordinateSystem system, size_t axis, double value, double length_scale_meters, const Units::UnitDisplayPreferences& preferences) noexcept {
	if (Observer::coordinate_axis_kind(system, axis) == Observer::CoordinateAxisKind::Length) {
		return value * length_scale_meters / distance_unit_in_meters(preferences.distance);
	}
	return value / angle_unit_in_radians(preferences.angle);
}

[[nodiscard]] inline double coordinate_axis_from_display(Observer::CoordinateSystem system, size_t axis, double value, double length_scale_meters, const Units::UnitDisplayPreferences& preferences) noexcept {
	if (Observer::coordinate_axis_kind(system, axis) == Observer::CoordinateAxisKind::Length) {
		return value * distance_unit_in_meters(preferences.distance) / length_scale_meters;
	}
	return value * angle_unit_in_radians(preferences.angle);
}

[[nodiscard]] inline std::string coordinate_axis_suffix(Observer::CoordinateSystem system, size_t axis, const Units::UnitDisplayPreferences& preferences) {
	if (Observer::coordinate_axis_kind(system, axis) == Observer::CoordinateAxisKind::Length) {
		return std::string(Units::distance_unit_suffix(preferences.distance));
	}
	return std::string(Units::angle_unit_suffix(preferences.angle));
}

[[nodiscard]] inline std::string describe_position(
	Observer::CoordinateSystem system,
	const Observer::CoordinateVector& cartesian,
	const Observer::CoordinateFrame& frame,
	double length_scale_meters,
	const Units::UnitDisplayPreferences& preferences,
	int precision,
	bool labels = true
) {
	const Observer::CoordinateVector values = Observer::CoordinateConverter::to_system(system, cartesian, frame);
	std::string text;
	for (size_t axis = 0; axis < 3; ++axis) {
		if (axis > 0) {
			text += ", ";
		}
		if (labels) {
			text += Observer::coordinate_axis_label(system, axis);
			text += "=";
		}
		if (Observer::coordinate_axis_kind(system, axis) == Observer::CoordinateAxisKind::Length) {
			text += Units::format_distance(values[axis] * length_scale_meters, preferences.distance, precision);
		} else {
			char buffer[64];
			std::snprintf(buffer, sizeof(buffer), "%.*f %s", precision, Units::convert_angle_from_radians(values[axis], preferences.angle), Units::angle_unit_suffix(preferences.angle));
			text += buffer;
		}
	}
	if (system == Observer::CoordinateSystem::MetricAdapted && frame.has_horizon()) {
		const double radius = std::sqrt(cartesian[0] * cartesian[0] + cartesian[1] * cartesian[1] + cartesian[2] * cartesian[2]);
		text += labels ? ", r*=" : ", ";
		text += Units::format_distance(Observer::CoordinateConverter::tortoise_radius(frame, radius) * length_scale_meters, preferences.distance, precision);
	}
	return text;
}

[[nodiscard]] inline std::vector<std::string> describe_position_lines(
	const std::string& prefix,
	const Observer::CoordinateVector& cartesian,
	const Observer::CoordinateFrame& frame,
	double length_scale_meters,
	const Units::UnitDisplayPreferences& preferences,
	Observer::CoordinateSystem primary,
	Observer::CoordinateSystem secondary,
	bool secondary_enabled,
	int precision,
	bool compact,
	bool labels
) {
	std::vector<std::string> lines;
	const auto append = [&](Observer::CoordinateSystem system) {
		const size_t index = Observer::coordinate_system_index(system);
		std::string line;
		if (compact) {
			line = std::string(Observer::kCoordinateSystemShortNames[index]) + " ";
		} else {
			line = prefix + " [" + Observer::kCoordinateSystemTitles[index] + "]: ";
		}
		line += describe_position(system, cartesian, frame, length_scale_meters, preferences, precision, labels);
		lines.push_back(std::move(line));
	};
	append(primary);
	if (secondary_enabled && secondary != primary) {
		append(secondary);
	}
	return lines;
}

}

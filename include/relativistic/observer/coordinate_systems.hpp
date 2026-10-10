#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace Relativistic::Observer {

enum class CoordinateSystem : uint32_t {
	Cartesian = 0,
	Spherical = 1,
	Cylindrical = 2,
	MetricAdapted = 3
};

inline constexpr size_t kCoordinateSystemCount = 4;

enum class CoordinateAxisKind : uint32_t {
	Length = 0,
	Angle = 1
};

using CoordinateVector = std::array<double, 3>;

inline constexpr std::array<const char*, kCoordinateSystemCount> kCoordinateSystemNames{
	"Cartesian (x, y, z)",
	"Spherical (r, theta, phi)",
	"Cylindrical (rho, phi, z)",
	"Metric-Adapted (l, theta, phi)"
};

inline constexpr std::array<const char*, kCoordinateSystemCount> kCoordinateSystemTitles{
	"Cartesian",
	"Spherical",
	"Cylindrical",
	"Metric-Adapted"
};

inline constexpr std::array<const char*, kCoordinateSystemCount> kCoordinateSystemShortNames{
	"XYZ",
	"SPH",
	"CYL",
	"MET"
};

inline constexpr std::array<std::array<const char*, 3>, kCoordinateSystemCount> kCoordinateAxisLabels{{
	{"x", "y", "z"},
	{"r", "theta", "phi"},
	{"rho", "phi", "z"},
	{"l", "theta", "phi"}
}};

[[nodiscard]] constexpr CoordinateSystem coordinate_system_from_index(uint32_t index) noexcept {
	return static_cast<CoordinateSystem>(std::min<uint32_t>(index, static_cast<uint32_t>(kCoordinateSystemCount - 1)));
}

[[nodiscard]] constexpr size_t coordinate_system_index(CoordinateSystem system) noexcept {
	return static_cast<size_t>(system);
}

[[nodiscard]] constexpr const char* coordinate_axis_label(CoordinateSystem system, size_t axis) noexcept {
	return kCoordinateAxisLabels[coordinate_system_index(system)][std::min<size_t>(axis, 2)];
}

[[nodiscard]] constexpr CoordinateAxisKind coordinate_axis_kind(CoordinateSystem system, size_t axis) noexcept {
	switch (system) {
		case CoordinateSystem::Spherical:
		case CoordinateSystem::MetricAdapted:
			return (axis == 0) ? CoordinateAxisKind::Length : CoordinateAxisKind::Angle;
		case CoordinateSystem::Cylindrical:
			return (axis == 1) ? CoordinateAxisKind::Angle : CoordinateAxisKind::Length;
		case CoordinateSystem::Cartesian:
		default:
			return CoordinateAxisKind::Length;
	}
}

struct CoordinateFrame {
	double mass{0.0};
	double spin{0.0};
	double outer_horizon{0.0};
	double inner_horizon{0.0};

	[[nodiscard]] constexpr bool has_horizon() const noexcept {
		return outer_horizon > 0.0 && mass > 0.0;
	}
};

struct CoordinatePreferences {
	CoordinateSystem widget_default{CoordinateSystem::Cartesian};
};

class CoordinateConverter {
public:
	[[nodiscard]] static double polar_angle_of(double z, double radius) noexcept {
		return (radius > 1e-300) ? std::acos(std::clamp(z / radius, -1.0, 1.0)) : 0.0;
	}

	[[nodiscard]] static double proper_radial_distance(const CoordinateFrame& frame, double radius, double theta) noexcept {
		if (!frame.has_horizon()) {
			return radius;
		}
		const double outer = frame.outer_horizon;
		if (radius < outer) {
			return -proper_radial_distance(frame, std::max(2.0 * outer - radius, outer), theta);
		}
		const double cos_theta = std::cos(theta);
		const double polar_term = frame.spin * frame.spin * cos_theta * cos_theta;
		const double upper = std::sqrt(radius - outer);
		constexpr int intervals = 256;
		const double step = upper / static_cast<double>(intervals);
		double sum = 0.0;
		for (int i = 0; i <= intervals; ++i) {
			const double u = step * static_cast<double>(i);
			const double r = outer + u * u;
			const double rho = std::sqrt(r * r + polar_term);
			const double gap = std::max(r - frame.inner_horizon, 1e-6 * outer);
			const double value = 2.0 * rho / std::sqrt(gap);
			const double weight = (i == 0 || i == intervals) ? 1.0 : ((i & 1) != 0 ? 4.0 : 2.0);
			sum += weight * value;
		}
		return sum * step / 3.0;
	}

	[[nodiscard]] static double radius_from_proper_distance(const CoordinateFrame& frame, double distance, double theta) noexcept {
		if (!frame.has_horizon()) {
			return std::max(distance, 0.0);
		}
		const double outer = frame.outer_horizon;
		if (distance < 0.0) {
			return std::max(2.0 * outer - radius_from_proper_distance(frame, -distance, theta), 0.0);
		}
		double low = outer;
		double high = outer + distance + 1e-9 * outer;
		for (int iteration = 0; iteration < 64; ++iteration) {
			const double middle = 0.5 * (low + high);
			if (proper_radial_distance(frame, middle, theta) < distance) {
				low = middle;
			} else {
				high = middle;
			}
		}
		return 0.5 * (low + high);
	}

	[[nodiscard]] static double tortoise_radius(const CoordinateFrame& frame, double radius) noexcept {
		if (!frame.has_horizon()) {
			return radius;
		}
		constexpr double tiny = 1e-12;
		const double outer = frame.outer_horizon;
		const double inner = frame.inner_horizon;
		const double spin_squared = frame.spin * frame.spin;
		const double scale = std::max(2.0 * frame.mass, tiny);
		const double gap = outer - inner;
		if (gap > 1e-9 * outer) {
			const double outer_weight = (outer * outer + spin_squared) / gap;
			const double inner_weight = (inner * inner + spin_squared) / gap;
			return radius
				+ outer_weight * std::log(std::max(std::abs(radius - outer), tiny) / scale)
				- inner_weight * std::log(std::max(std::abs(radius - inner), tiny) / scale);
		}
		double offset = radius - outer;
		if (std::abs(offset) < tiny) {
			offset = tiny;
		}
		return radius + 2.0 * frame.mass * std::log(std::abs(offset) / scale) - (frame.mass * frame.mass + spin_squared) / offset;
	}

	[[nodiscard]] static CoordinateVector to_system(CoordinateSystem system, const CoordinateVector& cartesian, const CoordinateFrame& frame) noexcept {
		const double x = cartesian[0];
		const double y = cartesian[1];
		const double z = cartesian[2];
		switch (system) {
			case CoordinateSystem::Spherical: {
				const double radius = std::sqrt(x * x + y * y + z * z);
				return {radius, polar_angle_of(z, radius), std::atan2(y, x)};
			}
			case CoordinateSystem::Cylindrical:
				return {std::hypot(x, y), std::atan2(y, x), z};
			case CoordinateSystem::MetricAdapted: {
				const double radius = std::sqrt(x * x + y * y + z * z);
				const double theta = polar_angle_of(z, radius);
				return {proper_radial_distance(frame, radius, theta), theta, std::atan2(y, x)};
			}
			case CoordinateSystem::Cartesian:
			default:
				return cartesian;
		}
	}

	[[nodiscard]] static CoordinateVector from_system(CoordinateSystem system, const CoordinateVector& values, const CoordinateFrame& frame) noexcept {
		switch (system) {
			case CoordinateSystem::Spherical: {
				const double sin_theta = std::sin(values[1]);
				return {values[0] * sin_theta * std::cos(values[2]), values[0] * sin_theta * std::sin(values[2]), values[0] * std::cos(values[1])};
			}
			case CoordinateSystem::Cylindrical:
				return {values[0] * std::cos(values[1]), values[0] * std::sin(values[1]), values[2]};
			case CoordinateSystem::MetricAdapted: {
				const double radius = radius_from_proper_distance(frame, values[0], values[1]);
				const double sin_theta = std::sin(values[1]);
				return {radius * sin_theta * std::cos(values[2]), radius * sin_theta * std::sin(values[2]), radius * std::cos(values[1])};
			}
			case CoordinateSystem::Cartesian:
			default:
				return values;
		}
	}
};

}

#pragma once

#include "relativistic/dynamics/pn_body.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace Relativistic::Observer::SurfaceGeometry {

using Vector3 = std::array<double, 3>;

inline constexpr double kDegreesToRadians = std::numbers::pi_v<double> / 180.0;
inline constexpr double kRadiansToDegrees = 180.0 / std::numbers::pi_v<double>;

struct FrameAngles {
	double pitch_deg{0.0};
	double yaw_deg{0.0};
	double roll_deg{0.0};
};

[[nodiscard]] inline Vector3 add(const Vector3& a, const Vector3& b) noexcept {
	return {a[0] + b[0], a[1] + b[1], a[2] + b[2]};
}

[[nodiscard]] inline Vector3 subtract(const Vector3& a, const Vector3& b) noexcept {
	return {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
}

[[nodiscard]] inline Vector3 scale(const Vector3& a, double factor) noexcept {
	return {a[0] * factor, a[1] * factor, a[2] * factor};
}

[[nodiscard]] inline double dot(const Vector3& a, const Vector3& b) noexcept {
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

[[nodiscard]] inline Vector3 cross(const Vector3& a, const Vector3& b) noexcept {
	return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
}

[[nodiscard]] inline double length(const Vector3& a) noexcept {
	return std::sqrt(dot(a, a));
}

[[nodiscard]] inline Vector3 normalized(const Vector3& a, const Vector3& fallback) noexcept {
	const double magnitude = length(a);
	return (magnitude > 1e-12) ? scale(a, 1.0 / magnitude) : fallback;
}

[[nodiscard]] inline Vector3 rotate_about_axis(const Vector3& value, const Vector3& axis, double angle) noexcept {
	const double cosine = std::cos(angle);
	const double sine = std::sin(angle);
	return add(add(scale(value, cosine), scale(cross(axis, value), sine)), scale(axis, dot(axis, value) * (1.0 - cosine)));
}

[[nodiscard]] inline Vector3 rotate_z(const Vector3& value, double angle) noexcept {
	const double cosine = std::cos(angle);
	const double sine = std::sin(angle);
	return {value[0] * cosine - value[1] * sine, value[0] * sine + value[1] * cosine, value[2]};
}

[[nodiscard]] inline Vector3 tangent_fallback(const Vector3& normal) noexcept {
	const Vector3 secondary = normalized(subtract({1.0, 0.0, 0.0}, scale(normal, normal[0])), {0.0, 1.0, 0.0});
	return normalized(subtract({0.0, 0.0, 1.0}, scale(normal, normal[2])), secondary);
}

[[nodiscard]] inline Vector3 body_semi_axes(const Dynamics::PostNewtonianBody& body) noexcept {
	if (body.is_spacetime_source) {
		const double horizon = body.kerr_outer_horizon_radius();
		const double spin_length = body.kerr_spin_parameter() * body.mass;
		const double extent = std::max(std::sqrt(horizon * horizon + spin_length * spin_length), 1e-6);
		return {extent, extent, extent};
	}
	const double radius = std::max(body.radius, 1e-6);
	const double oblateness = std::clamp((std::abs(body.j2) > 1e-9) ? (1.0 - body.j2) : 1.0, 0.1, 5.0);
	switch (body.geometry_model) {
		case Dynamics::Body3DGeometryModel::OblateSpheroid:
			return {radius, radius, radius * oblateness};
		case Dynamics::Body3DGeometryModel::ProlateSpheroid:
			return {radius, radius, radius / std::max(oblateness, 0.1)};
		case Dynamics::Body3DGeometryModel::TriaxialEllipsoid:
			return {radius, radius * 0.85, radius * oblateness};
		default:
			return {radius, radius, radius};
	}
}

[[nodiscard]] inline double ellipsoid_radius(const Vector3& axes, const Vector3& direction) noexcept {
	const double ratio_x = direction[0] / axes[0];
	const double ratio_y = direction[1] / axes[1];
	const double ratio_z = direction[2] / axes[2];
	return 1.0 / std::sqrt(std::max(ratio_x * ratio_x + ratio_y * ratio_y + ratio_z * ratio_z, 1e-30));
}

[[nodiscard]] inline Vector3 ellipsoid_normal(const Vector3& axes, const Vector3& direction) noexcept {
	const double radius = ellipsoid_radius(axes, direction);
	const Vector3 point = scale(direction, radius);
	return normalized({point[0] / (axes[0] * axes[0]), point[1] / (axes[1] * axes[1]), point[2] / (axes[2] * axes[2])}, direction);
}

[[nodiscard]] inline FrameAngles euler_from_frame(const Vector3& forward, const Vector3& up, double fallback_yaw_deg) noexcept {
	const Vector3 axis = normalized(forward, {1.0, 0.0, 0.0});
	const double horizontal = std::hypot(axis[0], axis[1]);
	const double yaw = (horizontal > 1e-9) ? std::atan2(axis[1], axis[0]) : (fallback_yaw_deg * kDegreesToRadians);
	const double pitch = std::asin(std::clamp(axis[2], -1.0, 1.0));
	const double cos_pitch = std::cos(pitch);
	const double sin_pitch = std::sin(pitch);
	const double cos_yaw = std::cos(yaw);
	const double sin_yaw = std::sin(yaw);
	const Vector3 level_right{sin_yaw, -cos_yaw, 0.0};
	const Vector3 level_up{-sin_pitch * cos_yaw, -sin_pitch * sin_yaw, cos_pitch};
	const double roll = std::atan2(dot(up, level_right), dot(up, level_up));
	return FrameAngles{pitch * kRadiansToDegrees, yaw * kRadiansToDegrees, roll * kRadiansToDegrees};
}

}

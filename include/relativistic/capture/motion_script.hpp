#pragma once

#include "relativistic/capture/camera_path.hpp"
#include "relativistic/capture/easing.hpp"
#include "relativistic/capture/expression.hpp"
#include "relativistic/observer/surface_geometry.hpp"
#include "relativistic/io/capture/capture_settings_io.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <functional>
#include <numbers>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <type_traits>
#include <utility>
#include <vector>

namespace Relativistic::Capture {

using Vec3 = std::array<double, 3>;
class BodyPositionLookup {
public:
	using PositionFunction = std::function<std::optional<Vec3>(int32_t)>;
	using NearestFunction = std::function<std::optional<std::pair<int32_t, Vec3>>(const Vec3&)>;
	using AxesFunction = std::function<std::optional<Vec3>(int32_t)>;

	BodyPositionLookup() = default;

	template <typename Callable>
		requires (!std::is_same_v<std::remove_cvref_t<Callable>, BodyPositionLookup> && std::is_invocable_r_v<std::optional<Vec3>, Callable&, int32_t>)
	BodyPositionLookup(Callable&& position, NearestFunction nearest = {}, AxesFunction axes = {})
		: position_(std::forward<Callable>(position)), nearest_(std::move(nearest)), axes_(std::move(axes)) {}

	[[nodiscard]] explicit operator bool() const noexcept {
		return static_cast<bool>(position_);
	}

	[[nodiscard]] std::optional<Vec3> operator()(int32_t id) const {
		return position_ ? position_(id) : std::nullopt;
	}

	[[nodiscard]] bool has_nearest() const noexcept {
		return static_cast<bool>(nearest_);
	}

	[[nodiscard]] std::optional<std::pair<int32_t, Vec3>> nearest(const Vec3& from) const {
		return nearest_ ? nearest_(from) : std::nullopt;
	}

	[[nodiscard]] std::optional<Vec3> axes(int32_t id) const {
		return axes_ ? axes_(id) : std::nullopt;
	}

private:
	PositionFunction position_{};
	NearestFunction nearest_{};
	AxesFunction axes_{};
};

inline constexpr int32_t kFixedPointReference = -2;
inline constexpr int32_t kOriginReference = -1;
inline constexpr int32_t kNearestBodyReference = -3;

namespace ScriptMath {

[[nodiscard]] inline Vec3 add(const Vec3& a, const Vec3& b) noexcept { return {a[0] + b[0], a[1] + b[1], a[2] + b[2]}; }
[[nodiscard]] inline Vec3 sub(const Vec3& a, const Vec3& b) noexcept { return {a[0] - b[0], a[1] - b[1], a[2] - b[2]}; }
[[nodiscard]] inline Vec3 scaled(const Vec3& a, double s) noexcept { return {a[0] * s, a[1] * s, a[2] * s}; }
[[nodiscard]] inline Vec3 lerp(const Vec3& a, const Vec3& b, double t) noexcept { return {a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t, a[2] + (b[2] - a[2]) * t}; }
[[nodiscard]] inline double dot(const Vec3& a, const Vec3& b) noexcept { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }
[[nodiscard]] inline double length(const Vec3& a) noexcept { return std::sqrt(dot(a, a)); }
[[nodiscard]] inline Vec3 cross(const Vec3& a, const Vec3& b) noexcept { return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]}; }

[[nodiscard]] inline Vec3 normalized(const Vec3& a, const Vec3& fallback) noexcept {
	const double len = length(a);
	return (len > 1e-12) ? scaled(a, 1.0 / len) : fallback;
}

[[nodiscard]] inline Vec3 rotate_euler(const Vec3& v, const Vec3& degrees) noexcept {
	const double roll = degrees[0] * PathDetail::kDegToRad;
	const double pitch = degrees[1] * PathDetail::kDegToRad;
	const double yaw = degrees[2] * PathDetail::kDegToRad;
	const double cr = std::cos(roll), sr = std::sin(roll);
	const double cp = std::cos(pitch), sp = std::sin(pitch);
	const double cy = std::cos(yaw), sy = std::sin(yaw);
	Vec3 r{v[0], v[1] * cr - v[2] * sr, v[1] * sr + v[2] * cr};
	r = {r[0] * cp + r[2] * sp, r[1], -r[0] * sp + r[2] * cp};
	return {r[0] * cy - r[1] * sy, r[0] * sy + r[1] * cy, r[2]};
}

[[nodiscard]] inline bool angles_from_direction(const Vec3& direction, double& pitch_deg, double& yaw_deg) noexcept {
	const double len = length(direction);
	if (len < 1e-9) return false;
	yaw_deg = std::atan2(direction[1], direction[0]) * PathDetail::kRadToDeg;
	pitch_deg = std::asin(std::clamp(direction[2] / len, -1.0, 1.0)) * PathDetail::kRadToDeg;
	return true;
}

inline void write_vec(IO::SettingsWriter& writer, const std::string& key, const Vec3& v) {
	writer.real(key + ".x", v[0]);
	writer.real(key + ".y", v[1]);
	writer.real(key + ".z", v[2]);
}

[[nodiscard]] inline Vec3 read_vec(const IO::SettingsReader& reader, const std::string& key, const Vec3& fallback) {
	return {reader.real(key + ".x", fallback[0]), reader.real(key + ".y", fallback[1]), reader.real(key + ".z", fallback[2])};
}

[[nodiscard]] inline std::optional<Vec3> resolve_body(const BodyPositionLookup& lookup, int32_t id, const Vec3& from) {
	if (!lookup) return std::nullopt;
	if (id == kNearestBodyReference) {
		if (const auto nearest = lookup.nearest(from)) return nearest->second;
		return std::nullopt;
	}
	if (id >= 0) return lookup(id);
	return std::nullopt;
}

[[nodiscard]] inline std::optional<Vec3> resolve_axes(const BodyPositionLookup& lookup, int32_t id) {
	if (!lookup || id < 0) return std::nullopt;
	return lookup.axes(id);
}

[[nodiscard]] inline Vec3 evaluate_point_curve(const std::vector<Vec3>& points, double u, bool closed, bool uniform_speed, double tension, int mode) noexcept {
	const size_t n = points.size();
	if (n == 0) return {0.0, 0.0, 0.0};
	if (n == 1) return points[0];
	const double x = std::clamp(u, 0.0, 1.0);
	const auto wrap = [n](int64_t k) { const int64_t m = static_cast<int64_t>(n); return static_cast<size_t>(((k % m) + m) % m); };
	if (mode == 2) {
		const size_t segments = closed ? n : n + 1;
		const double scaled_x = x * static_cast<double>(segments);
		const size_t i = std::min(static_cast<size_t>(std::floor(scaled_x)), segments - 1);
		const double s = scaled_x - static_cast<double>(i);
		const auto ctrl = [&](size_t k) -> const Vec3& {
			return closed ? points[wrap(static_cast<int64_t>(k) - 1)] : points[static_cast<size_t>(std::clamp<int64_t>(static_cast<int64_t>(k) - 2, 0, static_cast<int64_t>(n) - 1))];
		};
		const double s2 = s * s;
		const double s3 = s2 * s;
		const double b0 = (1.0 - s) * (1.0 - s) * (1.0 - s) / 6.0;
		const double b1 = (3.0 * s3 - 6.0 * s2 + 4.0) / 6.0;
		const double b2 = (-3.0 * s3 + 3.0 * s2 + 3.0 * s + 1.0) / 6.0;
		const double b3 = s3 / 6.0;
		Vec3 r{};
		for (size_t c = 0; c < 3; ++c) r[c] = b0 * ctrl(i)[c] + b1 * ctrl(i + 1)[c] + b2 * ctrl(i + 2)[c] + b3 * ctrl(i + 3)[c];
		return r;
	}
	const auto point = [&](int64_t k) -> const Vec3& {
		return closed ? points[wrap(k)] : points[static_cast<size_t>(std::clamp<int64_t>(k, 0, static_cast<int64_t>(n) - 1))];
	};
	const size_t segments = closed ? n : n - 1;
	size_t i = 0;
	double s = 0.0;
	if (uniform_speed) {
		std::vector<double> lengths(segments);
		double total = 0.0;
		for (size_t k = 0; k < segments; ++k) {
			lengths[k] = length(sub(point(static_cast<int64_t>(k) + 1), point(static_cast<int64_t>(k))));
			total += lengths[k];
		}
		if (total < 1e-12) return points[0];
		double target = x * total;
		while (i + 1 < segments && target > lengths[i]) {
			target -= lengths[i];
			++i;
		}
		s = (lengths[i] > 1e-12) ? std::clamp(target / lengths[i], 0.0, 1.0) : 0.0;
	} else {
		const double scaled_x = x * static_cast<double>(segments);
		i = std::min(static_cast<size_t>(std::floor(scaled_x)), segments - 1);
		s = scaled_x - static_cast<double>(i);
	}
	const int64_t ki = static_cast<int64_t>(i);
	if (mode == 0) return lerp(point(ki), point(ki + 1), s);
	const double s2 = s * s;
	const double s3 = s2 * s;
	const double h00 = 2.0 * s3 - 3.0 * s2 + 1.0;
	const double h10 = s3 - 2.0 * s2 + s;
	const double h01 = -2.0 * s3 + 3.0 * s2;
	const double h11 = s3 - s2;
	Vec3 r{};
	for (size_t c = 0; c < 3; ++c) {
		const double m0 = tension * (point(ki + 1)[c] - point(ki - 1)[c]);
		const double m1 = tension * (point(ki + 2)[c] - point(ki)[c]);
		r[c] = h00 * point(ki)[c] + h10 * m0 + h01 * point(ki + 1)[c] + h11 * m1;
	}
	return r;
}

}

enum class ShapeKind : uint32_t {
	Hold = 0, Linear, QuadraticBezier, CubicBezier, Spline, Polyline, BSpline, Arc, Helix, Orbit, LogarithmicSpiral, ArchimedeanSpiral,
	Lissajous, TorusKnot, Lemniscate, Rose, Epitrochoid, Wave, ExpressionCartesian, ExpressionCylindrical, ExpressionSpherical, SurfaceWalk, SurfaceLoop, SurfaceRoute, PlanetOrbit
};

inline constexpr size_t kShapeKindCount = static_cast<size_t>(ShapeKind::PlanetOrbit) + 1;

[[nodiscard]] constexpr bool is_surface_shape(ShapeKind kind) noexcept {
	return kind == ShapeKind::SurfaceWalk || kind == ShapeKind::SurfaceLoop || kind == ShapeKind::SurfaceRoute;
}

[[nodiscard]] constexpr bool is_planet_orbit_shape(ShapeKind kind) noexcept {
	return kind == ShapeKind::PlanetOrbit;
}

struct ShapeDescriptor {
	const char* name;
	std::array<const char*, 4> control_labels;
	std::array<const char*, 8> value_labels;
	bool waypoints;
	bool expressions;
};

inline constexpr std::array<ShapeDescriptor, kShapeKindCount> kShapeDescriptors{{
	{"Hold Position", {"Position"}, {}, false, false},
	{"Straight Line", {"Start", "End"}, {}, false, false},
	{"Quadratic Bezier", {"Start", "Control", "End"}, {}, false, false},
	{"Cubic Bezier", {"Start", "Control 1", "Control 2", "End"}, {}, false, false},
	{"Catmull-Rom Spline", {}, {"Tension"}, true, false},
	{"Waypoint Polyline", {}, {}, true, false},
	{"B-Spline", {}, {}, true, false},
	{"Arc / Rotation Around Axis", {"Center", "Start Offset"}, {"Sweep (deg)", "End Radius Scale", "Axial Rise"}, false, false},
	{"Helix Between Points", {"Axis Start", "Axis End"}, {"Radius", "Revolutions", "Phase (deg)", "End Radius"}, false, false},
	{"Spherical Orbit", {"Center"}, {"Radius Start", "Radius End", "Elevation Start (deg)", "Elevation End (deg)", "Azimuth Start (deg)", "Revolutions"}, false, false},
	{"Logarithmic Spiral", {"Center"}, {"Radius Start", "Radius End", "Height Start", "Height End", "Revolutions", "Expansion Rate"}, false, false},
	{"Archimedean Spiral", {"Center"}, {"Radius Start", "Radius End", "Height Start", "Height End", "Revolutions", "Phase (deg)"}, false, false},
	{"Lissajous Curve", {"Center", "Amplitude", "Frequency (cycles)", "Phase (deg)"}, {}, false, false},
	{"Torus Knot", {"Center"}, {"Major Radius", "Minor Radius", "Winding P", "Winding Q", "Turns"}, false, false},
	{"Lemniscate Figure Eight", {"Center"}, {"Size", "Height", "Turns"}, false, false},
	{"Rose Curve", {"Center"}, {"Radius", "Petal Factor", "Turns", "Height Rise"}, false, false},
	{"Epitrochoid / Hypotrochoid", {"Center"}, {"Fixed Radius", "Rolling Radius (negative = hypo)", "Pen Offset", "Turns", "Height Rise"}, false, false},
	{"Wave Along Line", {"Start", "End", "Amplitude A", "Amplitude B"}, {"Frequency A", "Phase A (deg)", "Damping", "Frequency B", "Phase B (deg)"}, false, false},
	{"Equation (Cartesian)", {"Center Offset"}, {"Parameter a", "Parameter b", "Parameter c", "Parameter k"}, false, true},
	{"Equation (Cylindrical)", {"Center Offset"}, {"Parameter a", "Parameter b", "Parameter c", "Parameter k"}, false, true},
	{"Equation (Spherical)", {"Center Offset"}, {"Parameter a", "Parameter b", "Parameter c", "Parameter k"}, false, true},
	{"Surface Walk", {"Body Center Offset", "Hop (Amplitude, Cycles, Y Axis Ratio)"}, {"Start Latitude (deg)", "Start Longitude (deg)", "Heading At Start (deg)", "Heading At End (deg)", "Distance Walked", "Body Radius", "Eye Height", "Polar Axis Ratio"}, false, false},
	{"Surface Loop (Circle Around A Point)", {"Body Center Offset", "Hop (Amplitude, Cycles, Y Axis Ratio)"}, {"Center Latitude (deg)", "Center Longitude (deg)", "Angular Radius (deg)", "Revolutions", "Start Phase (deg)", "Body Radius", "Eye Height", "Polar Axis Ratio"}, false, false},
	{"Surface Route (Coordinate To Coordinate)", {"Body Center Offset", "Hop (Amplitude, Cycles, Y Axis Ratio)"}, {"Start Latitude (deg)", "Start Longitude (deg)", "End Latitude (deg)", "End Longitude (deg)", "Route Mode (0 Short Arc, 1 Long Arc)", "Body Radius", "Eye Height", "Polar Axis Ratio"}, false, false},
	{"Planet Orbit (Altitude Over Surface)", {"Body Center Offset", "Axis Shape (Unused, Unused, Y Axis Ratio)"}, {"Start Latitude (deg)", "Start Longitude (deg)", "End Latitude (deg)", "Revolutions", "Start Altitude (Mean Radii)", "Body Radius", "End Altitude (Mean Radii)", "Polar Axis Ratio"}, false, false}
}};

[[nodiscard]] inline const ShapeDescriptor& shape_descriptor(ShapeKind kind) noexcept {
	return kShapeDescriptors[std::min(static_cast<size_t>(kind), kShapeKindCount - 1)];
}

[[nodiscard]] inline const std::vector<const char*>& shape_kind_names() {
	static const std::vector<const char*> names = [] {
		std::vector<const char*> result;
		for (const ShapeDescriptor& descriptor : kShapeDescriptors) result.push_back(descriptor.name);
		return result;
	}();
	return names;
}

[[nodiscard]] inline std::array<const char*, 3> shape_expression_labels(ShapeKind kind) noexcept {
	switch (kind) {
		case ShapeKind::ExpressionCylindrical: return {"rho(u)", "phi(u) [rad]", "z(u)"};
		case ShapeKind::ExpressionSpherical: return {"r(u)", "theta(u) [rad]", "phi(u) [rad]"};
		default: return {"x(u)", "y(u)", "z(u)"};
	}
}

struct ShapeEvaluationContext {
	double progress{0.0};
	double local_seconds{0.0};
	double duration{1.0};
	double global_seconds{0.0};
};

struct ShapeSpec {
	ShapeKind kind{ShapeKind::Linear};
	std::array<Vec3, 4> controls{};
	std::vector<Vec3> waypoints{};
	std::array<double, 8> values{};
	std::array<Expression, 3> expressions{Expression{"0"}, Expression{"0"}, Expression{"0"}};
	bool closed{false};
	bool uniform_speed{true};
	Vec3 scale{1.0, 1.0, 1.0};
	Vec3 rotation_deg{0.0, 0.0, 0.0};
	Vec3 pivot{0.0, 0.0, 0.0};
	Vec3 translation{0.0, 0.0, 0.0};

	[[nodiscard]] static ShapeSpec make(ShapeKind kind) {
		ShapeSpec spec;
		spec.reset(kind);
		return spec;
	}

	void reset(ShapeKind new_kind) {
		const Vec3 old_translation = translation;
		*this = ShapeSpec{};
		kind = new_kind;
		translation = old_translation;
		const std::vector<Vec3> sample_points{{-60.0, 40.0, 0.0}, {-20.0, 60.0, 25.0}, {20.0, 30.0, -15.0}, {60.0, 50.0, 10.0}, {80.0, 20.0, 0.0}};
		switch (kind) {
			case ShapeKind::Hold: controls[0] = {0.0, 60.0, 10.0}; break;
			case ShapeKind::Linear: controls[0] = {-60.0, 40.0, 0.0}; controls[1] = {60.0, 40.0, 0.0}; break;
			case ShapeKind::QuadraticBezier: controls[0] = {-60.0, 40.0, 0.0}; controls[1] = {0.0, -40.0, 40.0}; controls[2] = {60.0, 40.0, 0.0}; break;
			case ShapeKind::CubicBezier: controls[0] = {-60.0, 40.0, 0.0}; controls[1] = {-20.0, 100.0, 30.0}; controls[2] = {20.0, -20.0, 30.0}; controls[3] = {60.0, 40.0, 0.0}; break;
			case ShapeKind::Spline: waypoints = sample_points; values[0] = 0.5; break;
			case ShapeKind::Polyline:
			case ShapeKind::BSpline: waypoints = sample_points; break;
			case ShapeKind::Arc: controls[1] = {40.0, 0.0, 0.0}; values[0] = 180.0; values[1] = 1.0; break;
			case ShapeKind::Helix: controls[0] = {0.0, 0.0, -40.0}; controls[1] = {0.0, 0.0, 40.0}; values[0] = 25.0; values[1] = 4.0; values[3] = 25.0; break;
			case ShapeKind::Orbit: values = {40.0, 40.0, 10.0, 10.0, 0.0, 1.0, 0.0, 0.0}; break;
			case ShapeKind::LogarithmicSpiral: values = {80.0, 15.0, 20.0, 2.0, 3.0, 1.0, 0.0, 0.0}; break;
			case ShapeKind::ArchimedeanSpiral: values = {15.0, 70.0, 10.0, 10.0, 3.0, 0.0, 0.0, 0.0}; break;
			case ShapeKind::Lissajous: controls[1] = {40.0, 40.0, 15.0}; controls[2] = {3.0, 2.0, 1.0}; controls[3] = {0.0, 90.0, 0.0}; break;
			case ShapeKind::TorusKnot: values = {40.0, 15.0, 2.0, 3.0, 1.0, 0.0, 0.0, 0.0}; break;
			case ShapeKind::Lemniscate: values = {50.0, 12.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0}; break;
			case ShapeKind::Rose: values = {45.0, 3.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0}; break;
			case ShapeKind::Epitrochoid: values = {30.0, 10.0, 15.0, 1.0, 0.0, 0.0, 0.0, 0.0}; break;
			case ShapeKind::Wave: controls[0] = {-60.0, 40.0, 0.0}; controls[1] = {60.0, 40.0, 0.0}; controls[2] = {0.0, 0.0, 10.0}; controls[3] = {0.0, 6.0, 0.0}; values = {3.0, 0.0, 0.0, 7.0, 0.0, 0.0, 0.0, 0.0}; break;
			case ShapeKind::ExpressionCartesian: expressions = {Expression{"40*cos(2*pi*u)"}, Expression{"40*sin(2*pi*u)"}, Expression{"10*sin(4*pi*u)"}}; break;
			case ShapeKind::ExpressionCylindrical: expressions = {Expression{"40-25*u"}, Expression{"4*pi*u"}, Expression{"10+10*u"}}; break;
			case ShapeKind::ExpressionSpherical: expressions = {Expression{"50-30*u"}, Expression{"pi/2+0.4*sin(2*pi*u)"}, Expression{"6*pi*u"}}; break;
			case ShapeKind::SurfaceWalk: controls[1] = {0.0, 2.0, 1.0}; values = {0.0, 0.0, 90.0, 90.0, 20.0, 10.0, 0.2, 1.0}; break;
			case ShapeKind::SurfaceLoop: controls[1] = {0.0, 0.0, 1.0}; values = {0.0, 0.0, 30.0, 1.0, 0.0, 10.0, 0.2, 1.0}; break;
			case ShapeKind::SurfaceRoute: controls[1] = {0.0, 0.0, 1.0}; values = {0.0, 0.0, 30.0, 90.0, 0.0, 10.0, 0.2, 1.0}; break;
			case ShapeKind::PlanetOrbit: controls[1] = {0.0, 0.0, 1.0}; values = {15.0, 0.0, 15.0, 1.0, 1.0, 10.0, 1.0, 1.0}; break;
		}
	}

	struct SurfaceFrame {
		Vec3 center{0.0, 0.0, 0.0};
		Vec3 axes{1.0, 1.0, 1.0};
		double mean_radius{1.0};
		double eye_height{0.0};
	};

	[[nodiscard]] SurfaceFrame surface_frame() const noexcept {
		const auto ratio = [](double value) noexcept { return (value > 1e-3) ? std::clamp(value, 0.05, 20.0) : 1.0; };
		const double radius = std::max(values[5], 1e-9);
		SurfaceFrame frame;
		frame.center = controls[0];
		frame.axes = {radius, radius * ratio(controls[1][2]), radius * ratio(values[7])};
		frame.mean_radius = std::cbrt(frame.axes[0] * frame.axes[1] * frame.axes[2]);
		frame.eye_height = std::max(values[6], 0.0);
		return frame;
	}

	void fit_surface_axes(const Vec3& axes, double eye_height) noexcept {
		const double equatorial = std::max(axes[0], 1e-9);
		values[5] = equatorial;
		values[6] = std::max(eye_height, 0.0);
		values[7] = axes[2] / equatorial;
		controls[1][2] = axes[1] / equatorial;
	}

	void fit_orbit_axes(const Vec3& axes) noexcept {
		const double equatorial = std::max(axes[0], 1e-9);
		values[5] = equatorial;
		values[7] = axes[2] / equatorial;
		controls[1][2] = axes[1] / equatorial;
	}

	[[nodiscard]] static Vec3 surface_direction(double latitude_deg, double longitude_deg) noexcept {
		const double latitude = latitude_deg * PathDetail::kDegToRad;
		const double longitude = longitude_deg * PathDetail::kDegToRad;
		return {std::cos(latitude) * std::cos(longitude), std::cos(latitude) * std::sin(longitude), std::sin(latitude)};
	}

	[[nodiscard]] static Vec3 surface_point_above(const SurfaceFrame& frame, const Vec3& direction, double clearance) noexcept {
		const Vec3 unit = ScriptMath::normalized(direction, {0.0, 0.0, 1.0});
		const double radius = Observer::SurfaceGeometry::ellipsoid_radius(frame.axes, unit);
		const Vec3 normal = Observer::SurfaceGeometry::ellipsoid_normal(frame.axes, unit);
		return ScriptMath::add(frame.center, ScriptMath::add(ScriptMath::scaled(unit, radius), ScriptMath::scaled(normal, clearance)));
	}

	[[nodiscard]] bool has_transform() const noexcept {
		return scale != Vec3{1.0, 1.0, 1.0} || rotation_deg != Vec3{0.0, 0.0, 0.0} || translation != Vec3{0.0, 0.0, 0.0};
	}

	[[nodiscard]] Vec3 evaluate(const ShapeEvaluationContext& ctx) const noexcept {
		constexpr double tau = 2.0 * std::numbers::pi_v<double>;
		constexpr double deg = PathDetail::kDegToRad;
		const double u = std::clamp(ctx.progress, 0.0, 1.0);
		const Vec3& c0 = controls[0];
		const Vec3& c1 = controls[1];
		const Vec3& c2 = controls[2];
		const Vec3& c3 = controls[3];
		const auto& v = values;
		Vec3 raw{};
		switch (kind) {
			case ShapeKind::Hold: raw = c0; break;
			case ShapeKind::Linear: raw = ScriptMath::lerp(c0, c1, u); break;
			case ShapeKind::QuadraticBezier: {
				const double m = 1.0 - u;
				for (size_t i = 0; i < 3; ++i) raw[i] = m * m * c0[i] + 2.0 * m * u * c1[i] + u * u * c2[i];
				break;
			}
			case ShapeKind::CubicBezier: {
				const double m = 1.0 - u;
				for (size_t i = 0; i < 3; ++i) raw[i] = m * m * m * c0[i] + 3.0 * m * m * u * c1[i] + 3.0 * m * u * u * c2[i] + u * u * u * c3[i];
				break;
			}
			case ShapeKind::Spline: raw = ScriptMath::evaluate_point_curve(waypoints, u, closed, uniform_speed, v[0], 1); break;
			case ShapeKind::Polyline: raw = ScriptMath::evaluate_point_curve(waypoints, u, closed, uniform_speed, 0.5, 0); break;
			case ShapeKind::BSpline: raw = ScriptMath::evaluate_point_curve(waypoints, u, closed, false, 0.5, 2); break;
			case ShapeKind::Arc: {
				const double angle = v[0] * deg * u;
				const double radius_scale = v[1] + (1.0 - v[1]) * (1.0 - u);
				const double ca = std::cos(angle);
				const double sa = std::sin(angle);
				raw = {c0[0] + (c1[0] * ca - c1[1] * sa) * radius_scale, c0[1] + (c1[0] * sa + c1[1] * ca) * radius_scale, c0[2] + c1[2] * radius_scale + v[2] * u};
				break;
			}
			case ShapeKind::Helix: {
				const Vec3 axis = ScriptMath::normalized(ScriptMath::sub(c1, c0), {0.0, 0.0, 1.0});
				const Vec3 helper = (std::abs(axis[2]) < 0.9) ? Vec3{0.0, 0.0, 1.0} : Vec3{1.0, 0.0, 0.0};
				const Vec3 e1 = ScriptMath::normalized(ScriptMath::cross(helper, axis), {1.0, 0.0, 0.0});
				const Vec3 e2 = ScriptMath::cross(axis, e1);
				const double radius = v[0] + (v[3] - v[0]) * u;
				const double angle = v[2] * deg + tau * v[1] * u;
				const Vec3 center = ScriptMath::lerp(c0, c1, u);
				for (size_t i = 0; i < 3; ++i) raw[i] = center[i] + radius * (e1[i] * std::cos(angle) + e2[i] * std::sin(angle));
				break;
			}
			case ShapeKind::Orbit: {
				const double radius = v[0] + (v[1] - v[0]) * u;
				const double elevation = (v[2] + (v[3] - v[2]) * u) * deg;
				const double azimuth = (v[4] + 360.0 * v[5] * u) * deg;
				raw = {c0[0] + radius * std::cos(elevation) * std::cos(azimuth), c0[1] + radius * std::cos(elevation) * std::sin(azimuth), c0[2] + radius * std::sin(elevation)};
				break;
			}
			case ShapeKind::LogarithmicSpiral: {
				const double log_ratio = std::log(std::max(v[1] / std::max(v[0], 1e-6), 1e-6));
				const double radius = v[0] * std::exp(log_ratio * std::pow(u, std::max(v[5], 0.01)));
				const double angle = tau * v[4] * u;
				raw = {c0[0] + radius * std::cos(angle), c0[1] + radius * std::sin(angle), c0[2] + v[2] + (v[3] - v[2]) * u};
				break;
			}
			case ShapeKind::ArchimedeanSpiral: {
				const double radius = v[0] + (v[1] - v[0]) * u;
				const double angle = v[5] * deg + tau * v[4] * u;
				raw = {c0[0] + radius * std::cos(angle), c0[1] + radius * std::sin(angle), c0[2] + v[2] + (v[3] - v[2]) * u};
				break;
			}
			case ShapeKind::Lissajous:
				for (size_t i = 0; i < 3; ++i) raw[i] = c0[i] + c1[i] * std::sin(tau * c2[i] * u + c3[i] * deg);
				break;
			case ShapeKind::TorusKnot: {
				const double theta = tau * v[4] * u;
				const double ring = v[0] + v[1] * std::cos(v[3] * theta);
				raw = {c0[0] + ring * std::cos(v[2] * theta), c0[1] + ring * std::sin(v[2] * theta), c0[2] + v[1] * std::sin(v[3] * theta)};
				break;
			}
			case ShapeKind::Lemniscate: {
				const double theta = tau * v[2] * u;
				const double denominator = 1.0 + std::sin(theta) * std::sin(theta);
				raw = {c0[0] + v[0] * std::cos(theta) / denominator, c0[1] + v[0] * std::sin(theta) * std::cos(theta) / denominator, c0[2] + v[1] * std::sin(theta)};
				break;
			}
			case ShapeKind::Rose: {
				const double theta = tau * v[2] * u;
				const double radius = v[0] * std::cos(v[1] * theta);
				raw = {c0[0] + radius * std::cos(theta), c0[1] + radius * std::sin(theta), c0[2] + v[3] * u};
				break;
			}
			case ShapeKind::Epitrochoid: {
				const double rolling = (std::abs(v[1]) < 1e-9) ? 1e-9 : v[1];
				const double theta = tau * v[3] * u;
				const double ratio = (v[0] + rolling) / rolling;
				raw = {c0[0] + (v[0] + rolling) * std::cos(theta) - v[2] * std::cos(ratio * theta), c0[1] + (v[0] + rolling) * std::sin(theta) - v[2] * std::sin(ratio * theta), c0[2] + v[4] * u};
				break;
			}
			case ShapeKind::Wave: {
				const Vec3 base = ScriptMath::lerp(c0, c1, u);
				const double envelope = std::exp(-v[2] * u);
				const double wave_a = std::sin(tau * v[0] * u + v[1] * deg) * envelope;
				const double wave_b = std::sin(tau * v[3] * u + v[4] * deg) * envelope;
				for (size_t i = 0; i < 3; ++i) raw[i] = base[i] + c2[i] * wave_a + c3[i] * wave_b;
				break;
			}
			case ShapeKind::SurfaceWalk: {
				const SurfaceFrame frame = surface_frame();
				const double total_angle = v[4] / frame.mean_radius;
				const double latitude = v[0] * deg;
				const double longitude = v[1] * deg;
				const double start_heading = v[2] * deg;
				const double heading_change = (v[3] - v[2]) * deg * u;
				Vec3 direction{std::cos(latitude) * std::cos(longitude), std::cos(latitude) * std::sin(longitude), std::sin(latitude)};
				const Vec3 north{-std::sin(latitude) * std::cos(longitude), -std::sin(latitude) * std::sin(longitude), std::cos(latitude)};
				const Vec3 east{-std::sin(longitude), std::cos(longitude), 0.0};
				Vec3 tangent = ScriptMath::add(ScriptMath::scaled(north, std::cos(start_heading)), ScriptMath::scaled(east, std::sin(start_heading)));
				const int steps = std::clamp(static_cast<int>(std::ceil(u * 96.0)), 1, 96);
				const double step_angle = total_angle * u / static_cast<double>(steps);
				const double step_turn = heading_change / static_cast<double>(steps);
				for (int i = 0; i < steps; ++i) {
					const double cosine = std::cos(step_angle);
					const double sine = std::sin(step_angle);
					const Vec3 next_direction = ScriptMath::add(ScriptMath::scaled(direction, cosine), ScriptMath::scaled(tangent, sine));
					const Vec3 next_tangent = ScriptMath::sub(ScriptMath::scaled(tangent, cosine), ScriptMath::scaled(direction, sine));
					direction = ScriptMath::normalized(next_direction, direction);
					tangent = Observer::SurfaceGeometry::rotate_about_axis(next_tangent, direction, -step_turn);
				}
				const double hop = c1[0] * std::abs(std::sin(std::numbers::pi_v<double> * c1[1] * u));
				raw = surface_point_above(frame, direction, frame.eye_height + hop);
				break;
			}
			case ShapeKind::SurfaceLoop: {
				const SurfaceFrame frame = surface_frame();
				const double latitude = v[0] * deg;
				const double longitude = v[1] * deg;
				const Vec3 center_direction = surface_direction(v[0], v[1]);
				const Vec3 north{-std::sin(latitude) * std::cos(longitude), -std::sin(latitude) * std::sin(longitude), std::cos(latitude)};
				const Vec3 east{-std::sin(longitude), std::cos(longitude), 0.0};
				const double angular_radius = std::clamp(v[2], 0.0, 180.0) * deg;
				const double phase = v[4] * deg + tau * v[3] * u;
				const Vec3 lateral = ScriptMath::add(ScriptMath::scaled(north, std::cos(phase)), ScriptMath::scaled(east, std::sin(phase)));
				const Vec3 direction = ScriptMath::add(ScriptMath::scaled(center_direction, std::cos(angular_radius)), ScriptMath::scaled(lateral, std::sin(angular_radius)));
				const double hop = c1[0] * std::abs(std::sin(std::numbers::pi_v<double> * c1[1] * u));
				raw = surface_point_above(frame, direction, frame.eye_height + hop);
				break;
			}
			case ShapeKind::SurfaceRoute: {
				const SurfaceFrame frame = surface_frame();
				const Vec3 start = surface_direction(v[0], v[1]);
				const Vec3 finish = surface_direction(v[2], v[3]);
				const double separation = std::acos(std::clamp(ScriptMath::dot(start, finish), -1.0, 1.0));
				Vec3 axis = ScriptMath::cross(start, finish);
				axis = (ScriptMath::length(axis) < 1e-9) ? Observer::SurfaceGeometry::tangent_fallback(start) : ScriptMath::normalized(axis, {0.0, 0.0, 1.0});
				const double sweep = (v[4] > 0.5) ? -(tau - separation) : separation;
				const Vec3 direction = Observer::SurfaceGeometry::rotate_about_axis(start, axis, sweep * u);
				const double hop = c1[0] * std::abs(std::sin(std::numbers::pi_v<double> * c1[1] * u));
				raw = surface_point_above(frame, direction, frame.eye_height + hop);
				break;
			}
			case ShapeKind::PlanetOrbit: {
				const SurfaceFrame frame = surface_frame();
				const double latitude = std::clamp(v[0] + (v[2] - v[0]) * u, -89.0, 89.0);
				const double longitude = v[1] + 360.0 * v[3] * u;
				const double altitude = std::max(v[4] + (v[6] - v[4]) * u, 0.0) * frame.mean_radius;
				const Vec3 direction = ScriptMath::normalized(surface_direction(latitude, longitude), {0.0, 0.0, 1.0});
				const double surface_radius = Observer::SurfaceGeometry::ellipsoid_radius(frame.axes, direction);
				raw = ScriptMath::add(frame.center, ScriptMath::scaled(direction, surface_radius + altitude));
				break;
			}
			case ShapeKind::ExpressionCartesian:
			case ShapeKind::ExpressionCylindrical:
			case ShapeKind::ExpressionSpherical: {
				const ExpressionVariables variables{ctx.local_seconds, u, ctx.duration, ctx.global_seconds, v[0], v[1], v[2], v[3]};
				const double e0 = expressions[0].evaluate(variables);
				const double e1 = expressions[1].evaluate(variables);
				const double e2 = expressions[2].evaluate(variables);
				if (kind == ShapeKind::ExpressionCartesian) {
					raw = {c0[0] + e0, c0[1] + e1, c0[2] + e2};
				} else if (kind == ShapeKind::ExpressionCylindrical) {
					raw = {c0[0] + e0 * std::cos(e1), c0[1] + e0 * std::sin(e1), c0[2] + e2};
				} else {
					raw = {c0[0] + e0 * std::sin(e1) * std::cos(e2), c0[1] + e0 * std::sin(e1) * std::sin(e2), c0[2] + e0 * std::cos(e1)};
				}
				break;
			}
		}
		if (!has_transform()) return raw;
		Vec3 local = ScriptMath::sub(raw, pivot);
		for (size_t i = 0; i < 3; ++i) local[i] *= scale[i];
		local = ScriptMath::rotate_euler(local, rotation_deg);
		return ScriptMath::add(ScriptMath::add(pivot, local), translation);
	}

	void write(IO::SettingsWriter& writer, const std::string& prefix) const {
		writer.enumeration(prefix + "kind", kind);
		for (size_t i = 0; i < controls.size(); ++i) ScriptMath::write_vec(writer, prefix + "c" + std::to_string(i), controls[i]);
		for (size_t i = 0; i < values.size(); ++i) writer.real(prefix + "v" + std::to_string(i), values[i]);
		for (size_t i = 0; i < expressions.size(); ++i) writer.text(prefix + "e" + std::to_string(i), expressions[i].source());
		writer.flag(prefix + "closed", closed);
		writer.flag(prefix + "uniform", uniform_speed);
		writer.unsigned_value(prefix + "wp", waypoints.size());
		for (size_t i = 0; i < waypoints.size(); ++i) ScriptMath::write_vec(writer, prefix + "wp" + std::to_string(i), waypoints[i]);
		ScriptMath::write_vec(writer, prefix + "scale", scale);
		ScriptMath::write_vec(writer, prefix + "rot", rotation_deg);
		ScriptMath::write_vec(writer, prefix + "pivot", pivot);
		ScriptMath::write_vec(writer, prefix + "trans", translation);
	}

	void read(const IO::SettingsReader& reader, const std::string& prefix) {
		kind = reader.enumeration(prefix + "kind", kind, ShapeKind::PlanetOrbit);
		for (size_t i = 0; i < controls.size(); ++i) controls[i] = ScriptMath::read_vec(reader, prefix + "c" + std::to_string(i), controls[i]);
		for (size_t i = 0; i < values.size(); ++i) values[i] = reader.real(prefix + "v" + std::to_string(i), values[i]);
		for (size_t i = 0; i < expressions.size(); ++i) expressions[i].assign(reader.text(prefix + "e" + std::to_string(i), expressions[i].source()));
		closed = reader.flag(prefix + "closed", closed);
		uniform_speed = reader.flag(prefix + "uniform", uniform_speed);
		const size_t count = std::min<size_t>(reader.wide_value(prefix + "wp", 0), 4096);
		waypoints.clear();
		for (size_t i = 0; i < count; ++i) waypoints.push_back(ScriptMath::read_vec(reader, prefix + "wp" + std::to_string(i), {0.0, 0.0, 0.0}));
		scale = ScriptMath::read_vec(reader, prefix + "scale", scale);
		rotation_deg = ScriptMath::read_vec(reader, prefix + "rot", rotation_deg);
		pivot = ScriptMath::read_vec(reader, prefix + "pivot", pivot);
		translation = ScriptMath::read_vec(reader, prefix + "trans", translation);
	}
};

enum class LayerBlend : uint32_t { Replace = 0, Add, AddRelative };

struct ShapeLayer {
	std::string name{"Layer"};
	bool enabled{true};
	LayerBlend blend{LayerBlend::Replace};
	double weight_start{1.0};
	double weight_end{1.0};
	double window_start{0.0};
	double window_end{1.0};
	EasingSpec easing{};
	ShapeSpec shape{};

	void write(IO::SettingsWriter& writer, const std::string& prefix) const {
		writer.text(prefix + "name", name);
		writer.flag(prefix + "enabled", enabled);
		writer.enumeration(prefix + "blend", blend);
		writer.real(prefix + "ws", weight_start);
		writer.real(prefix + "we", weight_end);
		writer.real(prefix + "win0", window_start);
		writer.real(prefix + "win1", window_end);
		easing.write(writer, prefix + "ease.");
		shape.write(writer, prefix + "shape.");
	}

	void read(const IO::SettingsReader& reader, const std::string& prefix) {
		name = reader.text(prefix + "name", name);
		enabled = reader.flag(prefix + "enabled", enabled);
		blend = reader.enumeration(prefix + "blend", blend, LayerBlend::AddRelative);
		weight_start = reader.real(prefix + "ws", weight_start);
		weight_end = reader.real(prefix + "we", weight_end);
		window_start = reader.real(prefix + "win0", window_start);
		window_end = reader.real(prefix + "win1", window_end);
		easing.read(reader, prefix + "ease.");
		shape.read(reader, prefix + "shape.");
	}
};

enum class WaveKind : uint32_t { Sine = 0, Triangle, Square, Sawtooth, Noise };

struct ModulationSpec {
	bool enabled{false};
	WaveKind wave{WaveKind::Sine};
	double amplitude{1.0};
	double amplitude_end{1.0};
	double frequency{0.5};
	double phase_deg{0.0};
	double fade{0.0};

	[[nodiscard]] double evaluate(double progress, double seconds) const noexcept {
		if (!enabled) return 0.0;
		const double phase = frequency * seconds + phase_deg / 360.0;
		double w = 0.0;
		switch (wave) {
			case WaveKind::Sine: w = std::sin(2.0 * std::numbers::pi_v<double> * phase); break;
			case WaveKind::Triangle: w = evaluate_unary_op(ExpressionOp::Triangle, phase); break;
			case WaveKind::Square: w = evaluate_unary_op(ExpressionOp::Square, phase); break;
			case WaveKind::Sawtooth: w = evaluate_unary_op(ExpressionOp::Sawtooth, phase); break;
			case WaveKind::Noise: w = smooth_value_noise(phase * 4.0); break;
		}
		double envelope = 1.0;
		if (fade > 1e-9) {
			const double edge = std::clamp(std::min(progress, 1.0 - progress) / fade, 0.0, 1.0);
			envelope = edge * edge * (3.0 - 2.0 * edge);
		}
		return w * (amplitude + (amplitude_end - amplitude) * progress) * envelope;
	}

	void write(IO::SettingsWriter& writer, const std::string& prefix) const {
		writer.flag(prefix + "enabled", enabled);
		writer.enumeration(prefix + "wave", wave);
		writer.real(prefix + "amp", amplitude);
		writer.real(prefix + "amp_end", amplitude_end);
		writer.real(prefix + "freq", frequency);
		writer.real(prefix + "phase", phase_deg);
		writer.real(prefix + "fade", fade);
	}

	void read(const IO::SettingsReader& reader, const std::string& prefix) {
		enabled = reader.flag(prefix + "enabled", enabled);
		wave = reader.enumeration(prefix + "wave", wave, WaveKind::Noise);
		amplitude = reader.real(prefix + "amp", amplitude);
		amplitude_end = reader.real(prefix + "amp_end", amplitude_end);
		frequency = reader.real(prefix + "freq", frequency);
		phase_deg = reader.real(prefix + "phase", phase_deg);
		fade = reader.real(prefix + "fade", fade);
	}
};

struct SignalContext {
	Vec3 position{0.0, 0.0, 0.0};
	double speed{0.0};
	double segment_progress{0.0};
	double segment_time{0.0};
	double segment_duration{1.0};
	double segment_index{0.0};
	double script_progress{0.0};
	double script_time{0.0};
	const BodyPositionLookup* bodies{nullptr};
	std::function<double(uint32_t)> event_time{};

	[[nodiscard]] ExpressionVariables variables(double measurement) const noexcept {
		ExpressionVariables v{};
		v[ExpressionSlot::LocalTime] = segment_time;
		v[ExpressionSlot::Progress] = segment_progress;
		v[ExpressionSlot::Duration] = segment_duration;
		v[ExpressionSlot::GlobalTime] = script_time;
		v[ExpressionSlot::Radius] = ScriptMath::length(position);
		v[ExpressionSlot::PositionX] = position[0];
		v[ExpressionSlot::PositionY] = position[1];
		v[ExpressionSlot::PositionZ] = position[2];
		v[ExpressionSlot::Speed] = speed;
		v[ExpressionSlot::Measure] = measurement;
		v[ExpressionSlot::ScriptProgress] = script_progress;
		v[ExpressionSlot::SegmentIndex] = segment_index;
		return v;
	}
};

enum class DriverSource : uint32_t {
	DistanceToTarget = 0, DistanceToOrigin, PositionX, PositionY, PositionZ, Speed, SegmentProgress, ScriptProgress, SegmentTime, ScriptTime, EventTimeOffset, Expression
};

enum class DriverBlend : uint32_t { Replace = 0, Add, Multiply };

struct DriverSpec {
	bool enabled{false};
	DriverSource source{DriverSource::DistanceToTarget};
	int32_t body{kOriginReference};
	Vec3 point{0.0, 0.0, 0.0};
	uint32_t event{0};
	Expression expression{"r"};
	double input_min{5.0};
	double input_max{80.0};
	double output_min{0.0};
	double output_max{1.0};
	bool clamp_input{true};
	DriverBlend blend{DriverBlend::Replace};
	EasingSpec response{};

	[[nodiscard]] double measure(const SignalContext& ctx) const noexcept {
		switch (source) {
			case DriverSource::DistanceToTarget: {
				Vec3 target = point;
				if (ctx.bodies != nullptr) {
					if (const auto position = ScriptMath::resolve_body(*ctx.bodies, body, ctx.position)) {
						target = ScriptMath::add(*position, point);
					}
				}
				return ScriptMath::length(ScriptMath::sub(ctx.position, target));
			}
			case DriverSource::DistanceToOrigin: return ScriptMath::length(ctx.position);
			case DriverSource::PositionX: return ctx.position[0];
			case DriverSource::PositionY: return ctx.position[1];
			case DriverSource::PositionZ: return ctx.position[2];
			case DriverSource::Speed: return ctx.speed;
			case DriverSource::SegmentProgress: return ctx.segment_progress;
			case DriverSource::ScriptProgress: return ctx.script_progress;
			case DriverSource::SegmentTime: return ctx.segment_time;
			case DriverSource::ScriptTime: return ctx.script_time;
			case DriverSource::EventTimeOffset: return static_cast<bool>(ctx.event_time) ? (ctx.script_time - ctx.event_time(event)) : 0.0;
			case DriverSource::Expression: return expression.evaluate(ctx.variables(0.0), 0.0);
		}
		return 0.0;
	}

	[[nodiscard]] double normalize(double measured) const noexcept {
		const double span = input_max - input_min;
		const double t = (std::abs(span) > 1e-12) ? (measured - input_min) / span : ((measured >= input_min) ? 1.0 : 0.0);
		return clamp_input ? std::clamp(t, 0.0, 1.0) : t;
	}

	[[nodiscard]] double apply(double base, const SignalContext& ctx) const noexcept {
		if (!enabled) return base;
		const double t = normalize(measure(ctx));
		const double shaped = (t < 0.0 || t > 1.0) ? t : response.evaluate(t);
		const double driven = output_min + (output_max - output_min) * shaped;
		switch (blend) {
			case DriverBlend::Add: return base + driven;
			case DriverBlend::Multiply: return base * driven;
			case DriverBlend::Replace:
			default: return driven;
		}
	}

	void write(IO::SettingsWriter& writer, const std::string& prefix) const {
		writer.flag(prefix + "enabled", enabled);
		writer.enumeration(prefix + "source", source);
		writer.signed_value(prefix + "body", body);
		ScriptMath::write_vec(writer, prefix + "point", point);
		writer.unsigned_value(prefix + "event", event);
		writer.text(prefix + "expr", expression.source());
		writer.real(prefix + "in0", input_min);
		writer.real(prefix + "in1", input_max);
		writer.real(prefix + "out0", output_min);
		writer.real(prefix + "out1", output_max);
		writer.flag(prefix + "clamp", clamp_input);
		writer.enumeration(prefix + "blend", blend);
		response.write(writer, prefix + "resp.");
	}

	void read(const IO::SettingsReader& reader, const std::string& prefix) {
		enabled = reader.flag(prefix + "enabled", enabled);
		source = reader.enumeration(prefix + "source", source, DriverSource::Expression);
		body = reader.signed_value(prefix + "body", body);
		point = ScriptMath::read_vec(reader, prefix + "point", point);
		event = reader.unsigned_value(prefix + "event", event);
		expression.assign(reader.text(prefix + "expr", expression.source()));
		input_min = reader.real(prefix + "in0", input_min);
		input_max = reader.real(prefix + "in1", input_max);
		output_min = reader.real(prefix + "out0", output_min);
		output_max = reader.real(prefix + "out1", output_max);
		clamp_input = reader.flag(prefix + "clamp", clamp_input);
		blend = reader.enumeration(prefix + "blend", blend, DriverBlend::Multiply);
		response.read(reader, prefix + "resp.");
	}
};

struct ChannelContext {
	double progress{0.0};
	double local_seconds{0.0};
	double duration{1.0};
	double global_seconds{0.0};
	const SignalContext* signals{nullptr};
};

struct ScalarChannel {
	bool enabled{false};
	double start{0.0};
	double end{0.0};
	EasingSpec easing{};
	bool use_expression{false};
	Expression expression{"a+(b-a)*u"};
	ModulationSpec modulation{};
	DriverSpec driver{};

	[[nodiscard]] static ScalarChannel make(double start_value, double end_value, bool enabled_state) {
		ScalarChannel channel;
		channel.start = start_value;
		channel.end = end_value;
		channel.enabled = enabled_state;
		return channel;
	}

	[[nodiscard]] double evaluate(const ChannelContext& ctx, double fallback) const noexcept {
		if (!enabled) return fallback;
		double value = start + (end - start) * easing.evaluate(ctx.progress);
		if (use_expression && expression.valid()) {
			ExpressionVariables variables = (ctx.signals != nullptr) ? ctx.signals->variables(driver.enabled ? driver.measure(*ctx.signals) : 0.0) : ExpressionVariables{};
			variables[ExpressionSlot::LocalTime] = ctx.local_seconds;
			variables[ExpressionSlot::Progress] = ctx.progress;
			variables[ExpressionSlot::Duration] = ctx.duration;
			variables[ExpressionSlot::GlobalTime] = ctx.global_seconds;
			variables[ExpressionSlot::ParameterA] = start;
			variables[ExpressionSlot::ParameterB] = end;
			value = expression.evaluate(variables, value);
		}
		if (ctx.signals != nullptr) {
			value = driver.apply(value, *ctx.signals);
		}
		return value + modulation.evaluate(ctx.progress, ctx.local_seconds);
	}

	void write(IO::SettingsWriter& writer, const std::string& prefix) const {
		writer.flag(prefix + "enabled", enabled);
		writer.real(prefix + "start", start);
		writer.real(prefix + "end", end);
		easing.write(writer, prefix + "ease.");
		writer.flag(prefix + "use_expr", use_expression);
		writer.text(prefix + "expr", expression.source());
		modulation.write(writer, prefix + "mod.");
		driver.write(writer, prefix + "drv.");
	}

	void read(const IO::SettingsReader& reader, const std::string& prefix) {
		enabled = reader.flag(prefix + "enabled", enabled);
		start = reader.real(prefix + "start", start);
		end = reader.real(prefix + "end", end);
		easing.read(reader, prefix + "ease.");
		use_expression = reader.flag(prefix + "use_expr", use_expression);
		expression.assign(reader.text(prefix + "expr", expression.source()));
		modulation.read(reader, prefix + "mod.");
		driver.read(reader, prefix + "drv.");
	}
};

struct ShakeSpec {
	bool enabled{false};
	Vec3 position_amplitude{0.15, 0.15, 0.15};
	Vec3 rotation_amplitude{0.3, 0.3, 0.1};
	double frequency{1.5};
	uint32_t seed{1};
	double fade{0.1};

	void evaluate(double progress, double seconds, Vec3& position, Vec3& rotation) const noexcept {
		double envelope = 1.0;
		if (fade > 1e-9) {
			const double edge = std::clamp(std::min(progress, 1.0 - progress) / fade, 0.0, 1.0);
			envelope = edge * edge * (3.0 - 2.0 * edge);
		}
		const double base = seconds * frequency + static_cast<double>(seed) * 37.17;
		const auto channel = [&](int index) {
			const double x = base + static_cast<double>(index) * 101.31;
			return (smooth_value_noise(x) + 0.5 * smooth_value_noise(2.07 * x + 13.7)) / 1.5;
		};
		for (int i = 0; i < 3; ++i) {
			position[static_cast<size_t>(i)] = position_amplitude[static_cast<size_t>(i)] * channel(i) * envelope;
			rotation[static_cast<size_t>(i)] = rotation_amplitude[static_cast<size_t>(i)] * channel(i + 3) * envelope;
		}
	}

	void write(IO::SettingsWriter& writer, const std::string& prefix) const {
		writer.flag(prefix + "enabled", enabled);
		ScriptMath::write_vec(writer, prefix + "pos", position_amplitude);
		ScriptMath::write_vec(writer, prefix + "rot", rotation_amplitude);
		writer.real(prefix + "freq", frequency);
		writer.unsigned_value(prefix + "seed", seed);
		writer.real(prefix + "fade", fade);
	}

	void read(const IO::SettingsReader& reader, const std::string& prefix) {
		enabled = reader.flag(prefix + "enabled", enabled);
		position_amplitude = ScriptMath::read_vec(reader, prefix + "pos", position_amplitude);
		rotation_amplitude = ScriptMath::read_vec(reader, prefix + "rot", rotation_amplitude);
		frequency = reader.real(prefix + "freq", frequency);
		seed = reader.unsigned_value(prefix + "seed", seed);
		fade = reader.real(prefix + "fade", fade);
	}
};

enum class OrientationMode : uint32_t { Free = 0, Fixed, Interpolated, LookAtTarget, AlongTravel, Expression, TargetPath, SurfaceWalker };

struct OrientationSpec {
	OrientationMode mode{OrientationMode::LookAtTarget};
	std::array<double, 2> start{0.0, 0.0};
	std::array<double, 2> end{0.0, 0.0};
	EasingSpec easing{};
	int32_t target_body{kOriginReference};
	Vec3 target_offset{0.0, 0.0, 0.0};
	double look_ahead{0.02};
	double pitch_offset{0.0};
	double yaw_offset{0.0};
	std::array<Expression, 2> expressions{Expression{"0"}, Expression{"0"}};
	ShapeSpec target_path{};
	ScalarChannel pitch_modifier{};
	ScalarChannel yaw_modifier{};

	void write(IO::SettingsWriter& writer, const std::string& prefix) const {
		writer.enumeration(prefix + "mode", mode);
		writer.real(prefix + "s0", start[0]);
		writer.real(prefix + "s1", start[1]);
		writer.real(prefix + "e0", end[0]);
		writer.real(prefix + "e1", end[1]);
		easing.write(writer, prefix + "ease.");
		writer.signed_value(prefix + "body", target_body);
		ScriptMath::write_vec(writer, prefix + "offset", target_offset);
		writer.real(prefix + "ahead", look_ahead);
		writer.real(prefix + "pitch_offset", pitch_offset);
		writer.real(prefix + "yaw_offset", yaw_offset);
		writer.text(prefix + "expr0", expressions[0].source());
		writer.text(prefix + "expr1", expressions[1].source());
		target_path.write(writer, prefix + "path.");
		pitch_modifier.write(writer, prefix + "pitch_mod.");
		yaw_modifier.write(writer, prefix + "yaw_mod.");
	}

	void read(const IO::SettingsReader& reader, const std::string& prefix) {
		mode = reader.enumeration(prefix + "mode", mode, OrientationMode::SurfaceWalker);
		start = {reader.real(prefix + "s0", start[0]), reader.real(prefix + "s1", start[1])};
		end = {reader.real(prefix + "e0", end[0]), reader.real(prefix + "e1", end[1])};
		easing.read(reader, prefix + "ease.");
		target_body = reader.signed_value(prefix + "body", target_body);
		target_offset = ScriptMath::read_vec(reader, prefix + "offset", target_offset);
		look_ahead = reader.real(prefix + "ahead", look_ahead);
		pitch_offset = reader.real(prefix + "pitch_offset", pitch_offset);
		yaw_offset = reader.real(prefix + "yaw_offset", yaw_offset);
		expressions[0].assign(reader.text(prefix + "expr0", expressions[0].source()));
		expressions[1].assign(reader.text(prefix + "expr1", expressions[1].source()));
		target_path.read(reader, prefix + "path.");
		pitch_modifier.read(reader, prefix + "pitch_mod.");
		yaw_modifier.read(reader, prefix + "yaw_mod.");
	}
};

enum class TransitionBlendMode : uint32_t { ShortestArc = 0, Direct };

struct SegmentTransition {
	bool enabled{false};
	double orientation_seconds{0.0};
	double position_seconds{0.0};
	double lens_seconds{0.0};
	EasingSpec orientation_easing{EasingSpec::make(EasingKind::Smoothstep)};
	EasingSpec position_easing{EasingSpec::make(EasingKind::Smoothstep)};
	EasingSpec lens_easing{EasingSpec::make(EasingKind::Smoothstep)};
	TransitionBlendMode orientation_mode{TransitionBlendMode::ShortestArc};
	bool blend_simulation_rate{true};

	[[nodiscard]] double reach_seconds() const noexcept {
		return enabled ? std::max({orientation_seconds, position_seconds, lens_seconds}) : 0.0;
	}

	void write(IO::SettingsWriter& writer, const std::string& prefix) const {
		writer.flag(prefix + "enabled", enabled);
		writer.real(prefix + "orient_s", orientation_seconds);
		writer.real(prefix + "pos_s", position_seconds);
		writer.real(prefix + "lens_s", lens_seconds);
		orientation_easing.write(writer, prefix + "orient_ease.");
		position_easing.write(writer, prefix + "pos_ease.");
		lens_easing.write(writer, prefix + "lens_ease.");
		writer.enumeration(prefix + "mode", orientation_mode);
		writer.flag(prefix + "rate", blend_simulation_rate);
	}

	void read(const IO::SettingsReader& reader, const std::string& prefix) {
		enabled = reader.flag(prefix + "enabled", enabled);
		orientation_seconds = reader.real(prefix + "orient_s", orientation_seconds);
		position_seconds = reader.real(prefix + "pos_s", position_seconds);
		lens_seconds = reader.real(prefix + "lens_s", lens_seconds);
		orientation_easing.read(reader, prefix + "orient_ease.");
		position_easing.read(reader, prefix + "pos_ease.");
		lens_easing.read(reader, prefix + "lens_ease.");
		orientation_mode = reader.enumeration(prefix + "mode", orientation_mode, TransitionBlendMode::Direct);
		blend_simulation_rate = reader.flag(prefix + "rate", blend_simulation_rate);
	}
};

enum class AnchorMode : uint32_t { World = 0, ContinuePrevious, OffsetFromPrevious, TrackBody };

struct ScriptSegment {
	std::string name{"Segment"};
	bool enabled{true};
	double duration{5.0};
	EasingSpec time_easing{EasingSpec::make(EasingKind::Smoothstep)};
	AnchorMode anchor{AnchorMode::World};
	int32_t anchor_body{kOriginReference};
	Vec3 anchor_offset{0.0, 0.0, 0.0};
	std::vector<ShapeLayer> layers{};
	OrientationSpec orientation{};
	ScalarChannel fov{ScalarChannel::make(60.0, 60.0, false)};
	ScalarChannel exposure{ScalarChannel::make(0.0, 0.0, false)};
	ScalarChannel roll{ScalarChannel::make(0.0, 0.0, false)};
	ScalarChannel warp{ScalarChannel::make(1.0, 1.0, false)};
	ShakeSpec shake{};
	SegmentTransition transition{};

	[[nodiscard]] bool follows_surface() const noexcept {
		for (const ShapeLayer& layer : layers) {
			if (layer.enabled && (is_surface_shape(layer.shape.kind) || is_planet_orbit_shape(layer.shape.kind))) {
				return true;
			}
		}
		return false;
	}

	[[nodiscard]] bool uses_signals() const noexcept {
		const auto reactive = [](const ScalarChannel& channel) noexcept { return channel.enabled && (channel.driver.enabled || channel.use_expression); };
		return reactive(fov) || reactive(exposure) || reactive(roll) || reactive(warp)
			|| reactive(orientation.pitch_modifier) || reactive(orientation.yaw_modifier)
			|| orientation.mode == OrientationMode::Expression;
	}

	void write(IO::SettingsWriter& writer, const std::string& prefix) const {
		writer.text(prefix + "name", name);
		writer.flag(prefix + "enabled", enabled);
		writer.real(prefix + "duration", duration);
		time_easing.write(writer, prefix + "time.");
		writer.enumeration(prefix + "anchor", anchor);
		writer.signed_value(prefix + "anchor_body", anchor_body);
		ScriptMath::write_vec(writer, prefix + "anchor_offset", anchor_offset);
		writer.unsigned_value(prefix + "layers", layers.size());
		for (size_t i = 0; i < layers.size(); ++i) layers[i].write(writer, prefix + "layer" + std::to_string(i) + ".");
		orientation.write(writer, prefix + "orient.");
		fov.write(writer, prefix + "fov.");
		exposure.write(writer, prefix + "exposure.");
		roll.write(writer, prefix + "roll.");
		warp.write(writer, prefix + "warp.");
		shake.write(writer, prefix + "shake.");
		transition.write(writer, prefix + "transition.");
	}

	void read(const IO::SettingsReader& reader, const std::string& prefix) {
		name = reader.text(prefix + "name", name);
		enabled = reader.flag(prefix + "enabled", enabled);
		duration = reader.real(prefix + "duration", duration);
		time_easing.read(reader, prefix + "time.");
		anchor = reader.enumeration(prefix + "anchor", anchor, AnchorMode::TrackBody);
		anchor_body = reader.signed_value(prefix + "anchor_body", anchor_body);
		anchor_offset = ScriptMath::read_vec(reader, prefix + "anchor_offset", anchor_offset);
		const size_t count = std::min<size_t>(reader.wide_value(prefix + "layers", 0), 64);
		layers.assign(count, ShapeLayer{});
		for (size_t i = 0; i < count; ++i) layers[i].read(reader, prefix + "layer" + std::to_string(i) + ".");
		orientation.read(reader, prefix + "orient.");
		fov.read(reader, prefix + "fov.");
		exposure.read(reader, prefix + "exposure.");
		roll.read(reader, prefix + "roll.");
		warp.read(reader, prefix + "warp.");
		shake.read(reader, prefix + "shake.");
		transition.read(reader, prefix + "transition.");
	}
};

[[nodiscard]] inline ScriptSegment make_script_segment(ShapeKind kind, double duration) {
	ScriptSegment segment;
	segment.name = shape_descriptor(kind).name;
	segment.duration = duration;
	ShapeLayer layer;
	layer.name = "Base Path";
	layer.shape = ShapeSpec::make(kind);
	segment.layers.push_back(std::move(layer));
	if (is_surface_shape(kind)) {
		segment.time_easing = EasingSpec::make(EasingKind::Linear);
		segment.orientation.mode = OrientationMode::SurfaceWalker;
		segment.orientation.target_body = kOriginReference;
		segment.orientation.look_ahead = 0.03;
	} else if (is_planet_orbit_shape(kind)) {
		segment.time_easing = EasingSpec::make(EasingKind::Linear);
		segment.orientation.mode = OrientationMode::LookAtTarget;
		segment.orientation.target_body = kOriginReference;
	}
	return segment;
}

enum class EventTrigger : uint32_t { ScriptTime = 0, SegmentStart, SegmentEnd, SegmentFraction };

enum class EventAction : uint32_t { Marker = 0, CaptureStill, SetParameter, SetWarp, Pause, Resume, StepTicks, SetMetric, SetIntegrator, LoadScenario, SetTickRate, SetPerformancePreset, SetOverlay, SetResolutionScale };

[[nodiscard]] inline const char* event_action_name(EventAction action) noexcept {
	switch (action) {
		case EventAction::Marker: return "Marker";
		case EventAction::CaptureStill: return "Capture Still";
		case EventAction::SetParameter: return "Set Parameter";
		case EventAction::SetWarp: return "Set Time Warp";
		case EventAction::Pause: return "Pause Simulation";
		case EventAction::Resume: return "Resume Simulation";
		case EventAction::StepTicks: return "Step Ticks";
		case EventAction::SetMetric: return "Set Metric";
		case EventAction::SetIntegrator: return "Set Integrator";
		case EventAction::LoadScenario: return "Load Scenario";
		case EventAction::SetTickRate: return "Set Tick Rate";
		case EventAction::SetPerformancePreset: return "Apply Performance Preset";
		case EventAction::SetOverlay: return "Set Rendering Overlay";
		case EventAction::SetResolutionScale: return "Set Resolution Scale";
		default: return "Event";
	}
}

struct ScriptEvent {
	std::string name{};
	bool enabled{true};
	EventTrigger trigger{EventTrigger::ScriptTime};
	double time_seconds{0.0};
	uint32_t segment{0};
	double fraction{0.5};
	double offset_seconds{0.0};
	EventAction action{EventAction::Marker};
	uint32_t parameter{0};
	double value{1.0};
	double value_end{1.0};
	double duration{0.0};
	EasingSpec easing{};
	std::string text{};

	[[nodiscard]] std::string display_label() const {
		return name.empty() ? std::string(event_action_name(action)) : name;
	}

	void write(IO::SettingsWriter& writer, const std::string& prefix) const {
		writer.text(prefix + "name", name);
		writer.flag(prefix + "enabled", enabled);
		writer.enumeration(prefix + "trigger", trigger);
		writer.real(prefix + "time", time_seconds);
		writer.unsigned_value(prefix + "segment", segment);
		writer.real(prefix + "fraction", fraction);
		writer.real(prefix + "offset", offset_seconds);
		writer.enumeration(prefix + "action", action);
		writer.unsigned_value(prefix + "parameter", parameter);
		writer.real(prefix + "value", value);
		writer.real(prefix + "value_end", value_end);
		writer.real(prefix + "duration", duration);
		easing.write(writer, prefix + "ease.");
		writer.text(prefix + "text", text);
	}

	void read(const IO::SettingsReader& reader, const std::string& prefix) {
		name = reader.text(prefix + "name", name);
		enabled = reader.flag(prefix + "enabled", enabled);
		trigger = reader.enumeration(prefix + "trigger", trigger, EventTrigger::SegmentFraction);
		time_seconds = reader.real(prefix + "time", time_seconds);
		segment = reader.unsigned_value(prefix + "segment", segment);
		fraction = reader.real(prefix + "fraction", fraction);
		offset_seconds = reader.real(prefix + "offset", offset_seconds);
		action = reader.enumeration(prefix + "action", action, EventAction::SetResolutionScale);
		parameter = reader.unsigned_value(prefix + "parameter", parameter);
		value = reader.real(prefix + "value", value);
		value_end = reader.real(prefix + "value_end", value_end);
		duration = reader.real(prefix + "duration", duration);
		easing.read(reader, prefix + "ease.");
		text = reader.text(prefix + "text", text);
	}
};

struct ScriptSample {
	CameraPose pose{};
	double simulation_rate{1.0};
	size_t segment_index{0};
	double segment_progress{0.0};
	double script_time{0.0};
	double script_progress{0.0};
	bool valid{false};
	Vec3 shake_position{0.0, 0.0, 0.0};
	Vec3 shake_rotation{0.0, 0.0, 0.0};
	double transition_weight{0.0};
	double nearest_distance{-1.0};
};

enum class ScriptPreset : uint32_t {
	OrbitReveal = 0, SpiralInfall, FlyByWithStop, DollyZoom, FigureEightSurvey, HelicalApproach, TorusKnotShowcase, MultiStageTour, PhotonSphereSkim, HandheldDrift, SurfaceStroll, PlanetOrbitSurvey
};

inline constexpr size_t kScriptPresetCount = static_cast<size_t>(ScriptPreset::PlanetOrbitSurvey) + 1;

inline constexpr std::array<const char*, kScriptPresetCount> kScriptPresetNames{
	"Orbit Reveal", "Spiral Infall", "Fly-By With Stop", "Dolly Zoom", "Figure Eight Survey", "Helical Approach", "Torus Knot Showcase", "Multi-Stage Tour", "Photon Sphere Skim", "Handheld Drift", "Planetary Stroll", "Planet Orbit Survey"
};

struct MotionScript {
	std::string name{"Motion Script"};
	std::vector<ScriptSegment> segments{};
	std::vector<ScriptEvent> events{};
	PathEnd end_behavior{PathEnd::Clamp};
	EasingSpec global_easing{};
	double default_fov_deg{60.0};
	double default_exposure_ev{0.0};

	struct SegmentLocation {
		size_t active_slot{0};
		size_t segment_index{0};
		double local_seconds{0.0};
		double progress{0.0};
		double global_seconds{0.0};
		double script_progress{0.0};
	};

	[[nodiscard]] std::vector<size_t> active_segments() const {
		std::vector<size_t> active;
		for (size_t i = 0; i < segments.size(); ++i) {
			if (segments[i].enabled && segments[i].duration > 1e-9) active.push_back(i);
		}
		return active;
	}

	[[nodiscard]] bool is_usable() const {
		return !active_segments().empty();
	}

	[[nodiscard]] double total_duration() const noexcept {
		double total = 0.0;
		for (const ScriptSegment& segment : segments) {
			if (segment.enabled) total += std::max(segment.duration, 0.0);
		}
		return std::max(total, 1.0e-6);
	}

	[[nodiscard]] double segment_start_time(size_t segment_index) const noexcept {
		double start = 0.0;
		for (size_t i = 0; i < segments.size() && i < segment_index; ++i) {
			if (segments[i].enabled) start += std::max(segments[i].duration, 0.0);
		}
		return start;
	}

	void scale_time(double factor) {
		if (!(factor > 0.0) || !std::isfinite(factor)) return;
		for (ScriptSegment& segment : segments) {
			segment.duration *= factor;
			segment.transition.orientation_seconds *= factor;
			segment.transition.position_seconds *= factor;
			segment.transition.lens_seconds *= factor;
		}
		for (ScriptEvent& event : events) {
			if (event.trigger == EventTrigger::ScriptTime) event.time_seconds *= factor;
			event.duration *= factor;
			event.offset_seconds *= factor;
		}
	}

	void sanitize() {
		if (segments.size() > 512) segments.resize(512);
		if (events.size() > 1024) events.resize(1024);
		for (ScriptSegment& segment : segments) {
			segment.duration = std::clamp(segment.duration, 0.01, 86400.0);
			segment.transition.orientation_seconds = std::clamp(segment.transition.orientation_seconds, 0.0, segment.duration);
			segment.transition.position_seconds = std::clamp(segment.transition.position_seconds, 0.0, segment.duration);
			segment.transition.lens_seconds = std::clamp(segment.transition.lens_seconds, 0.0, segment.duration);
			for (ShapeLayer& layer : segment.layers) {
				layer.window_start = std::clamp(layer.window_start, 0.0, 1.0);
				layer.window_end = std::clamp(layer.window_end, 0.0, 1.0);
			}
		}
		for (ScriptEvent& event : events) {
			event.duration = std::max(event.duration, 0.0);
			event.fraction = std::clamp(event.fraction, 0.0, 1.0);
		}
	}

	[[nodiscard]] double resolved_event_time(const ScriptEvent& event) const noexcept {
		if (segments.empty() || event.trigger == EventTrigger::ScriptTime) return std::max(event.time_seconds + event.offset_seconds, 0.0);
		const size_t index = std::min<size_t>(event.segment, segments.size() - 1);
		const double start = segment_start_time(index);
		const double duration = segments[index].enabled ? segments[index].duration : 0.0;
		switch (event.trigger) {
			case EventTrigger::SegmentStart: return std::max(start + event.offset_seconds, 0.0);
			case EventTrigger::SegmentEnd: return std::max(start + duration + event.offset_seconds, 0.0);
			case EventTrigger::SegmentFraction: return std::max(start + duration * std::clamp(event.fraction, 0.0, 1.0) + event.offset_seconds, 0.0);
			case EventTrigger::ScriptTime:
			default: return std::max(event.time_seconds + event.offset_seconds, 0.0);
		}
	}

	[[nodiscard]] static std::optional<Vec3> surface_axes_of(const ScriptSegment& segment) noexcept {
		for (const ShapeLayer& layer : segment.layers) {
			if (layer.enabled && is_surface_shape(layer.shape.kind)) {
				return layer.shape.surface_frame().axes;
			}
		}
		return std::nullopt;
	}

	[[nodiscard]] std::optional<SegmentLocation> locate(double seconds) const noexcept {
		const auto active = active_segments();
		if (active.empty()) return std::nullopt;

		const double total = total_duration();
		double t = seconds;
		if (end_behavior == PathEnd::Loop) {
			t = std::fmod(t, total);
			if (t < 0.0) t += total;
		} else if (end_behavior == PathEnd::PingPong) {
			const double period = 2.0 * total;
			t = std::fmod(t, period);
			if (t < 0.0) t += period;
			if (t > total) t = period - t;
		} else {
			t = std::clamp(t, 0.0, total);
		}

		const double global_progress = (total > 1e-9) ? std::clamp(t / total, 0.0, 1.0) : 0.0;
		const double eased_global_progress = global_easing.evaluate(global_progress);
		const double effective_t = eased_global_progress * total;

		double accumulated = 0.0;
		for (size_t slot = 0; slot < active.size(); ++slot) {
			const size_t seg_idx = active[slot];
			const double seg_dur = segments[seg_idx].duration;
			const bool is_last = (slot + 1 == active.size());

			if (effective_t <= accumulated + seg_dur || is_last) {
				const double local_t = std::clamp(effective_t - accumulated, 0.0, seg_dur);
				const double linear_u = (seg_dur > 1e-9) ? std::clamp(local_t / seg_dur, 0.0, 1.0) : 0.0;
				const double eased_u = segments[seg_idx].time_easing.evaluate(linear_u);
				return SegmentLocation{slot, seg_idx, local_t, eased_u, effective_t, global_progress};
			}
			accumulated += seg_dur;
		}

		const size_t last_idx = active.back();
		return SegmentLocation{active.size() - 1, last_idx, segments[last_idx].duration, 1.0, total, 1.0};
	}

	[[nodiscard]] Vec3 evaluate_segment_raw_position(size_t segment_index, double progress, double local_sec, double global_sec) const noexcept {
		if (segment_index >= segments.size()) return {0.0, 0.0, 0.0};
		const ScriptSegment& segment = segments[segment_index];
		if (segment.layers.empty()) return {0.0, 0.0, 0.0};

		Vec3 composite{0.0, 0.0, 0.0};
		bool base_set = false;

		for (const ShapeLayer& layer : segment.layers) {
			if (!layer.enabled) continue;

			double layer_u = progress;
			const double win_span = layer.window_end - layer.window_start;
			if (std::abs(win_span) > 1e-9) {
				layer_u = std::clamp((progress - layer.window_start) / win_span, 0.0, 1.0);
			}
			const double eased_layer_u = layer.easing.evaluate(layer_u);
			const ShapeEvaluationContext ctx{eased_layer_u, local_sec, segment.duration, global_sec};
			const Vec3 layer_pos = layer.shape.evaluate(ctx);
			const double weight = layer.weight_start + (layer.weight_end - layer.weight_start) * progress;

			if (!base_set || layer.blend == LayerBlend::Replace) {
				composite = ScriptMath::scaled(layer_pos, weight);
				base_set = true;
			} else if (layer.blend == LayerBlend::Add) {
				composite = ScriptMath::add(composite, ScriptMath::scaled(layer_pos, weight));
			} else if (layer.blend == LayerBlend::AddRelative) {
				const ShapeEvaluationContext origin_ctx{0.0, 0.0, segment.duration, global_sec};
				const Vec3 origin_pos = layer.shape.evaluate(origin_ctx);
				const Vec3 delta = ScriptMath::sub(layer_pos, origin_pos);
				composite = ScriptMath::add(composite, ScriptMath::scaled(delta, weight));
			}
		}
		return composite;
	}

	[[nodiscard]] Vec3 evaluate_segment_anchor(size_t segment_index, const BodyPositionLookup& body_lookup) const noexcept {
		if (segment_index >= segments.size()) return {0.0, 0.0, 0.0};
		const ScriptSegment& segment = segments[segment_index];

		switch (segment.anchor) {
			case AnchorMode::World:
				return segment.anchor_offset;
			case AnchorMode::TrackBody: {
				if (body_lookup && segment.anchor_body >= 0) {
					if (auto pos = body_lookup(segment.anchor_body)) {
						return ScriptMath::add(*pos, segment.anchor_offset);
					}
				}
				return segment.anchor_offset;
			}
			case AnchorMode::ContinuePrevious:
			case AnchorMode::OffsetFromPrevious: {
				if (segment_index == 0) return segment.anchor_offset;
				size_t prev_idx = segment_index - 1;
				while (prev_idx > 0 && !segments[prev_idx].enabled) --prev_idx;
				if (!segments[prev_idx].enabled) return segment.anchor_offset;

				const double prev_dur = segments[prev_idx].duration;
				const Vec3 prev_anchor = evaluate_segment_anchor(prev_idx, body_lookup);
				const Vec3 prev_pos = evaluate_segment_raw_position(prev_idx, 1.0, prev_dur, 0.0);
				const Vec3 prev_world = ScriptMath::add(prev_anchor, prev_pos);

				if (segment.anchor == AnchorMode::ContinuePrevious) {
					return ScriptMath::sub(prev_world, evaluate_segment_raw_position(segment_index, 0.0, 0.0, 0.0));
				}
				return ScriptMath::add(prev_world, segment.anchor_offset);
			}
		}
		return segment.anchor_offset;
	}

	[[nodiscard]] Vec3 position_at(double seconds, const BodyPositionLookup& body_lookup) const noexcept {
		const auto loc = locate(seconds);
		if (!loc) return {0.0, 0.0, 0.0};
		return ScriptMath::add(
			evaluate_segment_anchor(loc->segment_index, body_lookup),
			evaluate_segment_raw_position(loc->segment_index, loc->progress, loc->local_seconds, loc->global_seconds)
		);
	}

	[[nodiscard]] double estimate_speed(double seconds, const BodyPositionLookup& body_lookup) const noexcept {
		constexpr double step = 2.0e-3;
		const double lower = std::max(seconds - step, 0.0);
		const double upper = seconds + step;
		const Vec3 before = position_at(lower, body_lookup);
		const Vec3 after = position_at(upper, body_lookup);
		return ScriptMath::length(ScriptMath::sub(after, before)) / std::max(upper - lower, 1e-9);
	}

	[[nodiscard]] ScriptSample sample(double seconds, const BodyPositionLookup& body_lookup = {}) const noexcept {
		ScriptSample result{};
		const auto loc = locate(seconds);
		if (!loc) return result;
		result = evaluate_segment_pose(loc->segment_index, loc->progress, loc->local_seconds, loc->global_seconds, loc->script_progress, seconds, body_lookup);
		apply_segment_transition(result, loc->segment_index, loc->local_seconds, loc->global_seconds, body_lookup);
		return result;
	}

	[[nodiscard]] ScriptSample sample_segment(size_t segment_index, double linear, const BodyPositionLookup& body_lookup = {}) const noexcept {
		if (segment_index >= segments.size()) return ScriptSample{};
		const ScriptSegment& target = segments[segment_index];
		const double clamped = std::clamp(linear, 0.0, 1.0);
		const double start = segment_start_time(segment_index);
		const double local = clamped * target.duration;
		const double global = start + local;
		ScriptSample result = evaluate_segment_pose(segment_index, target.time_easing.evaluate(clamped), local, global, global / total_duration(), global, body_lookup);
		apply_segment_transition(result, segment_index, local, global, body_lookup);
		return result;
	}

	void apply_segment_transition(ScriptSample& current, size_t segment_index, double local_seconds, double global_seconds, const BodyPositionLookup& body_lookup) const noexcept {
		if (!current.valid || segment_index >= segments.size()) return;
		const SegmentTransition& transition = segments[segment_index].transition;
		if (!transition.enabled || local_seconds >= transition.reach_seconds()) return;
		const std::vector<size_t> active = active_segments();
		const auto found = std::find(active.begin(), active.end(), segment_index);
		if (found == active.end() || found == active.begin()) return;
		const size_t previous_index = *(found - 1);
		const double previous_end = std::max(global_seconds - local_seconds, 0.0);
		const ScriptSample previous = evaluate_segment_pose(previous_index, 1.0, segments[previous_index].duration, previous_end, current.script_progress, previous_end, body_lookup);
		if (!previous.valid) return;

		const auto weight = [local_seconds](double span, const EasingSpec& easing) noexcept {
			return (span > 1e-9) ? easing.evaluate(std::clamp(local_seconds / span, 0.0, 1.0)) : 1.0;
		};
		const double view_weight = weight(transition.orientation_seconds, transition.orientation_easing);
		const double position_weight = weight(transition.position_seconds, transition.position_easing);
		const double lens_weight = weight(transition.lens_seconds, transition.lens_easing);
		const bool shortest = transition.orientation_mode == TransitionBlendMode::ShortestArc;
		const auto blend_angle = [shortest](double from, double to, double w) noexcept {
			return from + (shortest ? PathDetail::wrap_degrees(to - from) : (to - from)) * w;
		};

		current.pose.position = ScriptMath::lerp(previous.pose.position, current.pose.position, position_weight);
		current.pose.pitch_deg = PathDetail::lerp(previous.pose.pitch_deg, current.pose.pitch_deg, view_weight);
		current.pose.yaw_deg = blend_angle(previous.pose.yaw_deg, current.pose.yaw_deg, view_weight);
		current.pose.roll_deg = blend_angle(previous.pose.roll_deg, current.pose.roll_deg, view_weight);
		current.pose.fov_deg = PathDetail::lerp(previous.pose.fov_deg, current.pose.fov_deg, lens_weight);
		current.pose.exposure_ev = PathDetail::lerp(previous.pose.exposure_ev, current.pose.exposure_ev, lens_weight);
		if (transition.blend_simulation_rate) {
			current.simulation_rate = PathDetail::lerp(previous.simulation_rate, current.simulation_rate, lens_weight);
		}
		current.transition_weight = 1.0 - std::min({view_weight, position_weight, lens_weight});
	}

	[[nodiscard]] ScriptSample evaluate_segment_pose(size_t seg_idx, double u, double local_t, double global_t, double script_progress, double speed_time, const BodyPositionLookup& body_lookup) const noexcept {
		ScriptSample result{};
		if (seg_idx >= segments.size()) return result;

		const ScriptSegment& segment = segments[seg_idx];

		const Vec3 anchor = evaluate_segment_anchor(seg_idx, body_lookup);
		const Vec3 raw_pos = evaluate_segment_raw_position(seg_idx, u, local_t, global_t);
		Vec3 pos = ScriptMath::add(anchor, raw_pos);

		Vec3 shake_pos{0.0, 0.0, 0.0};
		Vec3 shake_rot{0.0, 0.0, 0.0};
		if (segment.shake.enabled) {
			segment.shake.evaluate(u, local_t, shake_pos, shake_rot);
			pos = ScriptMath::add(pos, shake_pos);
		}

		double pitch = 0.0;
		double yaw = 0.0;
		const OrientationSpec& orient = segment.orientation;
		double surface_roll = 0.0;

		SignalContext signals;
		signals.position = pos;
		signals.segment_progress = u;
		signals.segment_time = local_t;
		signals.segment_duration = segment.duration;
		signals.segment_index = static_cast<double>(seg_idx);
		signals.script_progress = script_progress;
		signals.script_time = global_t;
		signals.bodies = &body_lookup;
		signals.event_time = [this](uint32_t index) -> double {
			return index < events.size() ? resolved_event_time(events[index]) : 0.0;
		};
		if (segment.uses_signals()) {
			signals.speed = estimate_speed(speed_time, body_lookup);
		}

		switch (orient.mode) {
			case OrientationMode::Fixed:
				pitch = orient.start[0];
				yaw = orient.start[1];
				break;

			case OrientationMode::Interpolated: {
				const double orient_u = orient.easing.evaluate(u);
				pitch = orient.start[0] + (orient.end[0] - orient.start[0]) * orient_u;
				yaw = orient.start[1] + (orient.end[1] - orient.start[1]) * orient_u;
				break;
			}

			case OrientationMode::LookAtTarget: {
				Vec3 target{0.0, 0.0, 0.0};
				if (const auto bpos = ScriptMath::resolve_body(body_lookup, orient.target_body, pos)) {
					target = *bpos;
				}
				target = ScriptMath::add(target, orient.target_offset);
				const Vec3 dir = ScriptMath::sub(target, pos);
				static_cast<void>(ScriptMath::angles_from_direction(dir, pitch, yaw));
				break;
			}

			case OrientationMode::AlongTravel: {
				const double dt = std::clamp(orient.look_ahead, 0.001, 0.5);
				const double next_u = std::clamp(u + dt, 0.0, 1.0);
				const Vec3 next_raw = evaluate_segment_raw_position(seg_idx, next_u, local_t + dt * segment.duration, global_t);
				const Vec3 next_pos = ScriptMath::add(anchor, next_raw);
				const Vec3 dir = ScriptMath::sub(next_pos, pos);
				static_cast<void>(ScriptMath::angles_from_direction(dir, pitch, yaw));
				break;
			}

			case OrientationMode::Expression: {
				const ExpressionVariables variables = signals.variables(0.0);
				pitch = orient.expressions[0].evaluate(variables, 0.0);
				yaw = orient.expressions[1].evaluate(variables, 0.0);
				break;
			}

			case OrientationMode::TargetPath: {
				Vec3 anchor_point{0.0, 0.0, 0.0};
				if (const auto bpos = ScriptMath::resolve_body(body_lookup, orient.target_body, pos)) {
					anchor_point = *bpos;
				}
				const ShapeEvaluationContext target_ctx{orient.easing.evaluate(u), local_t, segment.duration, global_t};
				const Vec3 target = ScriptMath::add(ScriptMath::add(anchor_point, orient.target_path.evaluate(target_ctx)), orient.target_offset);
				static_cast<void>(ScriptMath::angles_from_direction(ScriptMath::sub(target, pos), pitch, yaw));
				break;
			}

			case OrientationMode::SurfaceWalker: {
				Vec3 center{0.0, 0.0, 0.0};
				if (const auto bpos = ScriptMath::resolve_body(body_lookup, orient.target_body, pos)) {
					center = *bpos;
				}
				center = ScriptMath::add(center, orient.target_offset);
				Vec3 surface_axes{1.0, 1.0, 1.0};
				if (const auto body_axes = ScriptMath::resolve_axes(body_lookup, orient.target_body)) {
					surface_axes = *body_axes;
				} else if (const auto shape_axes = surface_axes_of(segment)) {
					surface_axes = *shape_axes;
				}
				const Vec3 normal = Observer::SurfaceGeometry::ellipsoid_normal(surface_axes, ScriptMath::normalized(ScriptMath::sub(pos, center), {0.0, 0.0, 1.0}));
				const double lead = std::clamp(orient.look_ahead, 0.001, 0.5);
				const bool forward_sample = (u + lead) <= 1.0;
				const double sample_u = forward_sample ? (u + lead) : std::max(u - lead, 0.0);
				const Vec3 sample_pos = ScriptMath::add(anchor, evaluate_segment_raw_position(seg_idx, sample_u, local_t + (sample_u - u) * segment.duration, global_t));
				Vec3 travel = forward_sample ? ScriptMath::sub(sample_pos, pos) : ScriptMath::sub(pos, sample_pos);
				travel = ScriptMath::sub(travel, ScriptMath::scaled(normal, ScriptMath::dot(travel, normal)));
				const Vec3 north_tangent = ScriptMath::normalized(ScriptMath::sub({0.0, 0.0, 1.0}, ScriptMath::scaled(normal, normal[2])), {1.0, 0.0, 0.0});
				const auto angles = Observer::SurfaceGeometry::euler_from_frame(ScriptMath::normalized(travel, north_tangent), normal, 0.0);
				pitch = angles.pitch_deg;
				yaw = angles.yaw_deg;
				surface_roll = angles.roll_deg;
				break;
			}

			case OrientationMode::Free:
			default:
				break;
		}

		pitch += orient.pitch_offset + shake_rot[0];
		yaw += orient.yaw_offset + shake_rot[1];
		double roll = shake_rot[2] + surface_roll;

		ChannelContext ch_ctx{u, local_t, segment.duration, global_t};
		ch_ctx.signals = &signals;
		pitch += orient.pitch_modifier.evaluate(ch_ctx, 0.0);
		yaw += orient.yaw_modifier.evaluate(ch_ctx, 0.0);
		double fov = segment.fov.evaluate(ch_ctx, default_fov_deg);
		double exposure = segment.exposure.evaluate(ch_ctx, default_exposure_ev);
		roll += segment.roll.evaluate(ch_ctx, 0.0);
		double sim_warp = segment.warp.evaluate(ch_ctx, 1.0);

		result.pose.position = pos;
		result.pose.pitch_deg = pitch;
		result.pose.yaw_deg = yaw;
		result.pose.roll_deg = roll;
		result.pose.fov_deg = fov;
		result.pose.exposure_ev = exposure;
		result.simulation_rate = sim_warp;
		result.segment_index = seg_idx;
		result.segment_progress = u;
		result.script_time = global_t;
		result.script_progress = script_progress;
		result.valid = true;
		result.shake_position = shake_pos;
		result.shake_rotation = shake_rot;
		if (body_lookup.has_nearest()) {
			if (const auto nearest = body_lookup.nearest(pos)) {
				result.nearest_distance = ScriptMath::length(ScriptMath::sub(nearest->second, pos));
			}
		}

		return result;
	}

	static MotionScript make_preset(ScriptPreset preset) {
		MotionScript script;
		switch (preset) {
			case ScriptPreset::OrbitReveal: {
				script.name = "Orbit Reveal";
				ScriptSegment seg = make_script_segment(ShapeKind::Orbit, 10.0);
				seg.layers[0].shape.values = {80.0, 40.0, 45.0, 5.0, 0.0, 1.0, 0.0, 0.0};
				seg.orientation.mode = OrientationMode::LookAtTarget;
				seg.orientation.target_body = kOriginReference;
				script.segments.push_back(std::move(seg));
				break;
			}
			case ScriptPreset::SpiralInfall: {
				script.name = "Spiral Infall";
				ScriptSegment seg = make_script_segment(ShapeKind::LogarithmicSpiral, 15.0);
				seg.layers[0].shape.values = {120.0, 12.0, 30.0, 0.0, 4.0, 1.2, 0.0, 0.0};
				seg.orientation.mode = OrientationMode::LookAtTarget;
				seg.orientation.target_body = kOriginReference;
				seg.fov = ScalarChannel::make(60.0, 75.0, true);
				script.segments.push_back(std::move(seg));
				break;
			}
			case ScriptPreset::FlyByWithStop: {
				script.name = "Fly-By With Stop";
				ScriptSegment seg1 = make_script_segment(ShapeKind::Linear, 6.0);
				seg1.layers[0].shape.controls[0] = {-100.0, 30.0, 10.0};
				seg1.layers[0].shape.controls[1] = {-20.0, 20.0, 5.0};
				seg1.time_easing = EasingSpec::make(EasingKind::CubicOut);
				seg1.orientation.mode = OrientationMode::LookAtTarget;

				ScriptSegment seg2 = make_script_segment(ShapeKind::Hold, 3.0);
				seg2.anchor = AnchorMode::ContinuePrevious;
				seg2.orientation.mode = OrientationMode::LookAtTarget;

				ScriptSegment seg3 = make_script_segment(ShapeKind::Linear, 6.0);
				seg3.anchor = AnchorMode::ContinuePrevious;
				seg3.layers[0].shape.controls[0] = {0.0, 0.0, 0.0};
				seg3.layers[0].shape.controls[1] = {80.0, 40.0, 15.0};
				seg3.time_easing = EasingSpec::make(EasingKind::CubicIn);
				seg3.orientation.mode = OrientationMode::LookAtTarget;

				script.segments.push_back(std::move(seg1));
				script.segments.push_back(std::move(seg2));
				script.segments.push_back(std::move(seg3));
				break;
			}
			case ScriptPreset::DollyZoom: {
				script.name = "Dolly Zoom (Vertigo Effect)";
				ScriptSegment seg = make_script_segment(ShapeKind::Linear, 8.0);
				seg.layers[0].shape.controls[0] = {0.0, 120.0, 0.0};
				seg.layers[0].shape.controls[1] = {0.0, 25.0, 0.0};
				seg.orientation.mode = OrientationMode::LookAtTarget;
				seg.fov = ScalarChannel::make(30.0, 95.0, true);
				script.segments.push_back(std::move(seg));
				break;
			}
			case ScriptPreset::FigureEightSurvey: {
				script.name = "Figure Eight Survey";
				ScriptSegment seg = make_script_segment(ShapeKind::Lemniscate, 12.0);
				seg.layers[0].shape.values = {60.0, 15.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0};
				seg.orientation.mode = OrientationMode::LookAtTarget;
				script.segments.push_back(std::move(seg));
				break;
			}
			case ScriptPreset::HelicalApproach: {
				script.name = "Helical Approach";
				ScriptSegment seg = make_script_segment(ShapeKind::Helix, 14.0);
				seg.layers[0].shape.controls[0] = {0.0, 0.0, 60.0};
				seg.layers[0].shape.controls[1] = {0.0, 0.0, 5.0};
				seg.layers[0].shape.values = {50.0, 3.0, 0.0, 15.0, 0.0, 0.0, 0.0, 0.0};
				seg.orientation.mode = OrientationMode::LookAtTarget;
				script.segments.push_back(std::move(seg));
				break;
			}
			case ScriptPreset::TorusKnotShowcase: {
				script.name = "Torus Knot Showcase";
				ScriptSegment seg = make_script_segment(ShapeKind::TorusKnot, 16.0);
				seg.layers[0].shape.values = {50.0, 18.0, 2.0, 3.0, 1.0, 0.0, 0.0, 0.0};
				seg.orientation.mode = OrientationMode::LookAtTarget;
				script.segments.push_back(std::move(seg));
				break;
			}
			case ScriptPreset::MultiStageTour: {
				script.name = "Multi-Stage Tour";
				ScriptSegment seg1 = make_script_segment(ShapeKind::Arc, 6.0);
				seg1.layers[0].shape.controls[1] = {60.0, 0.0, 10.0};
				seg1.layers[0].shape.values[0] = 120.0;
				seg1.orientation.mode = OrientationMode::LookAtTarget;

				ScriptSegment seg2 = make_script_segment(ShapeKind::CubicBezier, 8.0);
				seg2.anchor = AnchorMode::ContinuePrevious;
				seg2.layers[0].shape.controls[0] = {0.0, 0.0, 0.0};
				seg2.layers[0].shape.controls[1] = {-20.0, 40.0, 20.0};
				seg2.layers[0].shape.controls[2] = {40.0, 20.0, -10.0};
				seg2.layers[0].shape.controls[3] = {60.0, -40.0, 5.0};
				seg2.orientation.mode = OrientationMode::AlongTravel;

				script.segments.push_back(std::move(seg1));
				script.segments.push_back(std::move(seg2));
				break;
			}
			case ScriptPreset::PhotonSphereSkim: {
				script.name = "Photon Sphere Skim";
				ScriptSegment seg = make_script_segment(ShapeKind::Orbit, 10.0);
				seg.layers[0].shape.values = {8.0, 3.5, 5.0, 2.0, 0.0, 2.0, 0.0, 0.0};
				seg.orientation.mode = OrientationMode::AlongTravel;
				seg.orientation.look_ahead = 0.01;
				seg.shake.enabled = true;
				seg.shake.position_amplitude = {0.05, 0.05, 0.05};
				seg.shake.frequency = 4.0;
				script.segments.push_back(std::move(seg));
				break;
			}
			case ScriptPreset::SurfaceStroll: {
				script.name = "Planetary Stroll";
				ScriptSegment seg = make_script_segment(ShapeKind::SurfaceWalk, 24.0);
				seg.time_easing = EasingSpec::make(EasingKind::Linear);
				seg.layers[0].shape.values = {10.0, 0.0, 90.0, 60.0, 22.0, 10.0, 0.25, 0.0};
				seg.layers[0].shape.controls[1] = {0.012, 24.0, 0.0};
				seg.orientation.mode = OrientationMode::SurfaceWalker;
				seg.orientation.target_body = kOriginReference;
				seg.orientation.look_ahead = 0.03;
				seg.orientation.pitch_offset = -4.0;
				script.segments.push_back(std::move(seg));
				break;
			}
			case ScriptPreset::PlanetOrbitSurvey: {
				script.name = "Planet Orbit Survey";
				ScriptSegment seg = make_script_segment(ShapeKind::PlanetOrbit, 24.0);
				seg.layers[0].shape.values = {20.0, 0.0, 55.0, 2.0, 2.5, 10.0, 0.6, 1.0};
				seg.orientation.mode = OrientationMode::LookAtTarget;
				seg.orientation.target_body = kOriginReference;
				script.segments.push_back(std::move(seg));
				break;
			}
			case ScriptPreset::HandheldDrift: {
				script.name = "Handheld Drift";
				ScriptSegment seg = make_script_segment(ShapeKind::Linear, 10.0);
				seg.layers[0].shape.controls[0] = {0.0, 50.0, 5.0};
				seg.layers[0].shape.controls[1] = {10.0, 45.0, 6.0};
				seg.orientation.mode = OrientationMode::LookAtTarget;
				seg.shake.enabled = true;
				seg.shake.position_amplitude = {0.2, 0.2, 0.15};
				seg.shake.rotation_amplitude = {0.5, 0.5, 0.3};
				seg.shake.frequency = 0.8;
				script.segments.push_back(std::move(seg));
				break;
			}
		}
		return script;
	}

	static MotionScript from_parametric_legacy_path(const CameraPath& legacy) {
		MotionScript script;
		script.name = "Converted Path";
		script.end_behavior = legacy.end_behavior;

		ShapeKind kind = ShapeKind::Linear;
		switch (legacy.kind) {
			case PathKind::Orbit:
			case PathKind::TargetTrackingOrbit: kind = ShapeKind::Orbit; break;
			case PathKind::LogarithmicSpiral: kind = ShapeKind::LogarithmicSpiral; break;
			case PathKind::Helical: kind = ShapeKind::Helix; break;
			case PathKind::DollyZoom:
			case PathKind::LinearFlyBy:
			case PathKind::Keyframes:
			default: kind = ShapeKind::Linear; break;
		}

		ScriptSegment segment = make_script_segment(kind, legacy.duration_seconds);
		switch (legacy.easing) {
			case PathEasing::EaseIn: segment.time_easing = EasingSpec::make(EasingKind::QuadIn); break;
			case PathEasing::EaseOut: segment.time_easing = EasingSpec::make(EasingKind::QuadOut); break;
			case PathEasing::EaseInOut: segment.time_easing = EasingSpec::make(EasingKind::Smoothstep); break;
			case PathEasing::Smootherstep: segment.time_easing = EasingSpec::make(EasingKind::Smootherstep); break;
			case PathEasing::Linear:
			default: segment.time_easing = EasingSpec::make(EasingKind::Linear); break;
		}

		ShapeSpec& shape = segment.layers.front().shape;
		switch (legacy.kind) {
			case PathKind::Orbit:
				shape.controls[0] = legacy.orbit.center;
				shape.values = {legacy.orbit.radius_start, legacy.orbit.radius_end, legacy.orbit.elevation_start_deg, legacy.orbit.elevation_end_deg, legacy.orbit.azimuth_start_deg, legacy.orbit.revolutions, 0.0, 0.0};
				break;
			case PathKind::TargetTrackingOrbit:
				shape.controls[0] = legacy.tracked_body_id >= 0 ? Vec3{0.0, 0.0, 0.0} : legacy.orbit.center;
				shape.values = {legacy.orbit.radius_start, legacy.orbit.radius_end, legacy.orbit.elevation_start_deg, legacy.orbit.elevation_end_deg, legacy.orbit.azimuth_start_deg, legacy.orbit.revolutions, 0.0, 0.0};
				if (legacy.tracked_body_id >= 0) {
					segment.anchor = AnchorMode::TrackBody;
					segment.anchor_body = legacy.tracked_body_id;
				}
				break;
			case PathKind::LogarithmicSpiral:
				shape.controls[0] = legacy.spiral.center;
				shape.values = {legacy.spiral.radius_start, legacy.spiral.radius_end, legacy.spiral.height_start, legacy.spiral.height_end, legacy.spiral.revolutions, legacy.spiral.expansion_rate, 0.0, 0.0};
				break;
			case PathKind::Helical:
				shape.controls[0] = legacy.helical.start;
				shape.controls[1] = legacy.helical.end;
				shape.values = {legacy.helical.radius, legacy.helical.revolutions, legacy.helical.phase_deg, legacy.helical.radius, 0.0, 0.0, 0.0, 0.0};
				break;
			case PathKind::DollyZoom:
				shape.controls[0] = legacy.dolly_zoom.start_position;
				shape.controls[1] = legacy.dolly_zoom.end_position;
				break;
			case PathKind::LinearFlyBy:
				shape.controls[0] = legacy.fly_by.start;
				shape.controls[1] = legacy.fly_by.end;
				break;
			case PathKind::Keyframes:
			default: break;
		}

		if (legacy.kind == PathKind::DollyZoom) {
			segment.orientation.mode = OrientationMode::LookAtTarget;
			segment.orientation.target_offset = legacy.dolly_zoom.target;
			segment.fov = ScalarChannel::make(legacy.dolly_zoom.fov_start_deg, legacy.dolly_zoom.fov_end_deg, true);
		} else {
			switch (legacy.orientation) {
				case PathOrientation::LookAtTarget:
					segment.orientation.mode = OrientationMode::LookAtTarget;
					segment.orientation.target_offset = legacy.look_target;
					if (legacy.kind == PathKind::TargetTrackingOrbit && legacy.tracked_body_id >= 0) {
						segment.orientation.target_body = legacy.tracked_body_id;
						segment.orientation.target_offset = {0.0, 0.0, 0.0};
					}
					break;
				case PathOrientation::AlongTravel:
					segment.orientation.mode = OrientationMode::AlongTravel;
					break;
				case PathOrientation::Keyframed:
				default:
					segment.orientation.mode = OrientationMode::Fixed;
					segment.orientation.start = legacy.fixed_pitch_yaw;
					break;
			}
			segment.fov = ScalarChannel::make(legacy.fov_start_deg, legacy.fov_end_deg, std::abs(legacy.fov_start_deg - legacy.fov_end_deg) > 1e-9);
		}
		segment.exposure = ScalarChannel::make(legacy.exposure_start_ev, legacy.exposure_end_ev, std::abs(legacy.exposure_start_ev - legacy.exposure_end_ev) > 1e-9);
		segment.roll = ScalarChannel::make(legacy.roll_deg, legacy.roll_deg, std::abs(legacy.roll_deg) > 1e-9);

		script.segments.push_back(std::move(segment));
		return script;
	}

	static MotionScript from_legacy_path(const CameraPath& legacy) {
		if (legacy.kind != PathKind::Keyframes) return from_parametric_legacy_path(legacy);

		MotionScript script;
		script.name = "Converted Path";
		script.end_behavior = legacy.end_behavior;

		if (legacy.keyframes.empty()) return script;
		if (legacy.keyframes.size() == 1) {
			ScriptSegment seg;
			seg.name = "Hold Keyframe";
			seg.duration = 5.0;
			ShapeLayer layer;
			layer.name = "Hold";
			layer.shape = ShapeSpec::make(ShapeKind::Hold);
			layer.shape.controls[0] = legacy.keyframes[0].pose.position;
			seg.layers.push_back(std::move(layer));
			seg.orientation.mode = OrientationMode::Fixed;
			seg.orientation.start = {legacy.keyframes[0].pose.pitch_deg, legacy.keyframes[0].pose.yaw_deg};
			seg.fov = ScalarChannel::make(legacy.keyframes[0].pose.fov_deg, legacy.keyframes[0].pose.fov_deg, true);
			script.segments.push_back(std::move(seg));
			return script;
		}

		for (size_t i = 0; i + 1 < legacy.keyframes.size(); ++i) {
			const auto& k0 = legacy.keyframes[i];
			const auto& k1 = legacy.keyframes[i + 1];
			const double dur = std::max(k1.time_seconds - k0.time_seconds, 0.1);

			ScriptSegment seg;
			seg.name = "Leg " + std::to_string(i + 1);
			seg.duration = dur;

			ShapeLayer layer;
			layer.name = "Line";
			layer.shape = ShapeSpec::make(ShapeKind::Linear);
			layer.shape.controls[0] = k0.pose.position;
			layer.shape.controls[1] = k1.pose.position;
			seg.layers.push_back(std::move(layer));

			seg.orientation.mode = OrientationMode::Interpolated;
			seg.orientation.start = {k0.pose.pitch_deg, k0.pose.yaw_deg};
			seg.orientation.end = {k1.pose.pitch_deg, k1.pose.yaw_deg};

			seg.fov = ScalarChannel::make(k0.pose.fov_deg, k1.pose.fov_deg, true);
			seg.exposure = ScalarChannel::make(k0.pose.exposure_ev, k1.pose.exposure_ev, true);
			seg.roll = ScalarChannel::make(k0.pose.roll_deg, k1.pose.roll_deg, true);

			script.segments.push_back(std::move(seg));
		}
		return script;
	}

	void write(IO::SettingsWriter& writer, const std::string& prefix = "script.") const {
		writer.text(prefix + "name", name);
		writer.enumeration(prefix + "end", end_behavior);
		global_easing.write(writer, prefix + "global_ease.");
		writer.unsigned_value(prefix + "segments", segments.size());
		for (size_t i = 0; i < segments.size(); ++i) {
			segments[i].write(writer, prefix + "seg" + std::to_string(i) + ".");
		}
		writer.unsigned_value(prefix + "events", events.size());
		for (size_t i = 0; i < events.size(); ++i) {
			events[i].write(writer, prefix + "ev" + std::to_string(i) + ".");
		}
	}

	void read(const IO::SettingsReader& reader, const std::string& prefix = "script.") {
		name = reader.text(prefix + "name", name);
		end_behavior = reader.enumeration(prefix + "end", end_behavior, PathEnd::PingPong);
		global_easing.read(reader, prefix + "global_ease.");

		const size_t seg_count = std::min<size_t>(reader.wide_value(prefix + "segments", 0), 512);
		segments.clear();
		segments.reserve(seg_count);
		for (size_t i = 0; i < seg_count; ++i) {
			ScriptSegment seg;
			seg.read(reader, prefix + "seg" + std::to_string(i) + ".");
			segments.push_back(std::move(seg));
		}

		const size_t ev_count = std::min<size_t>(reader.wide_value(prefix + "events", 0), 1024);
		events.clear();
		events.reserve(ev_count);
		for (size_t i = 0; i < ev_count; ++i) {
			ScriptEvent ev;
			ev.read(reader, prefix + "ev" + std::to_string(i) + ".");
			events.push_back(std::move(ev));
		}
	}
};

} // namespace Relativistic::Capture

#pragma once

#include "relativistic/observer/surface_geometry.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numbers>

namespace Relativistic::Observer {

using SurfaceGeometry::Vector3;

inline constexpr uint32_t kCameraNavigationModeCount = 6;
inline constexpr uint32_t kPlanetOrbitNavigationMode = 5;

struct PlanetOrbitParameters {
	double angular_speed_deg_s{40.0};
	double drag_sensitivity_deg_per_pixel{0.25};
	double zoom_rate{1.1};
	double wheel_zoom_factor{0.12};
	double minimum_altitude_ratio{0.02};
	double maximum_altitude_ratio{40.0};
	double smoothing_seconds{0.12};
	double maximum_pitch_deg{85.0};
	double sprint_multiplier{3.0};
	double crawl_multiplier{0.25};
	bool invert_drag{false};

	void sanitize() noexcept {
		const auto repair = [](double& value, double fallback) noexcept {
			if (!std::isfinite(value)) {
				value = fallback;
			}
		};
		repair(angular_speed_deg_s, 40.0);
		repair(drag_sensitivity_deg_per_pixel, 0.25);
		repair(zoom_rate, 1.1);
		repair(wheel_zoom_factor, 0.12);
		repair(minimum_altitude_ratio, 0.02);
		repair(maximum_altitude_ratio, 40.0);
		repair(smoothing_seconds, 0.12);
		repair(maximum_pitch_deg, 85.0);
		repair(sprint_multiplier, 3.0);
		repair(crawl_multiplier, 0.25);
		angular_speed_deg_s = std::clamp(angular_speed_deg_s, 1.0, 360.0);
		drag_sensitivity_deg_per_pixel = std::clamp(drag_sensitivity_deg_per_pixel, 0.01, 2.0);
		zoom_rate = std::clamp(zoom_rate, 0.05, 8.0);
		wheel_zoom_factor = std::clamp(wheel_zoom_factor, 0.01, 0.6);
		minimum_altitude_ratio = std::clamp(minimum_altitude_ratio, 1e-4, 5.0);
		maximum_altitude_ratio = std::clamp(maximum_altitude_ratio, minimum_altitude_ratio * 2.0, 1.0e5);
		smoothing_seconds = std::clamp(smoothing_seconds, 0.0, 1.0);
		maximum_pitch_deg = std::clamp(maximum_pitch_deg, 10.0, 89.0);
		sprint_multiplier = std::clamp(sprint_multiplier, 1.0, 20.0);
		crawl_multiplier = std::clamp(crawl_multiplier, 0.01, 1.0);
	}
};

struct PlanetOrbitEnvironment {
	Vector3 center{0.0, 0.0, 0.0};
	Vector3 axes{1.0, 1.0, 1.0};
};

struct PlanetOrbitInput {
	double orbit_east{0.0};
	double orbit_north{0.0};
	double zoom{0.0};
	double zoom_steps{0.0};
	double drag_east_pixels{0.0};
	double drag_north_pixels{0.0};
	bool sprint{false};
	bool crawl{false};
};

struct PlanetOrbitState {
	bool active{false};
	uint32_t body_id{0};
	double azimuth{0.0};
	double elevation{0.0};
	double altitude_ratio{1.0};
	double velocity_azimuth{0.0};
	double velocity_elevation{0.0};
	double velocity_zoom{0.0};
};

struct PlanetOrbitPose {
	Vector3 position{0.0, 0.0, 0.0};
	SurfaceGeometry::FrameAngles angles{};
	double altitude{0.0};
	double surface_radius{1.0};
};

class PlanetOrbit {
public:
	static constexpr double kMaximumFrameTime = 0.25;
	static constexpr double kMaximumDragRadians = 1.0;
	static constexpr double kMinimumPoleCosine = 0.15;
	static constexpr double kDefaultAltitudeRatio = 1.0;

	[[nodiscard]] static double mean_radius(const Vector3& axes) noexcept {
		return std::max(std::cbrt(std::max(axes[0], 1e-12) * std::max(axes[1], 1e-12) * std::max(axes[2], 1e-12)), 1e-12);
	}

	[[nodiscard]] static Vector3 direction(const PlanetOrbitState& state) noexcept {
		const double cosine = std::cos(state.elevation);
		return {cosine * std::cos(state.azimuth), cosine * std::sin(state.azimuth), std::sin(state.elevation)};
	}

	static void initialize(
		PlanetOrbitState& state,
		const PlanetOrbitEnvironment& environment,
		const PlanetOrbitParameters& parameters,
		uint32_t body_id,
		const Vector3& camera_position
	) noexcept {
		const Vector3 offset = SurfaceGeometry::subtract(camera_position, environment.center);
		const Vector3 unit = SurfaceGeometry::normalized(offset, {1.0, 0.0, 0.0});
		const double distance = SurfaceGeometry::length(offset);
		const double pitch_limit = parameters.maximum_pitch_deg * SurfaceGeometry::kDegreesToRadians;
		const double surface = SurfaceGeometry::ellipsoid_radius(environment.axes, unit);
		const double ratio = (distance - surface) / mean_radius(environment.axes);

		state = PlanetOrbitState{};
		state.active = true;
		state.body_id = body_id;
		state.azimuth = std::atan2(unit[1], unit[0]);
		state.elevation = std::clamp(std::asin(std::clamp(unit[2], -1.0, 1.0)), -pitch_limit, pitch_limit);
		const double resolved = (std::isfinite(ratio) && distance > surface) ? ratio : kDefaultAltitudeRatio;
		state.altitude_ratio = std::clamp(resolved, parameters.minimum_altitude_ratio, parameters.maximum_altitude_ratio);
		if (!std::isfinite(state.azimuth)) {
			state.azimuth = 0.0;
		}
	}

	static void advance(
		PlanetOrbitState& state,
		const PlanetOrbitParameters& parameters,
		const PlanetOrbitInput& input,
		double delta_time
	) noexcept {
		if (!state.active || !(delta_time > 0.0) || !std::isfinite(delta_time)) {
			return;
		}
		const auto finite_or = [](double& value, double fallback) noexcept {
			if (!std::isfinite(value)) {
				value = fallback;
			}
		};
		finite_or(state.azimuth, 0.0);
		finite_or(state.elevation, 0.0);
		finite_or(state.altitude_ratio, kDefaultAltitudeRatio);
		finite_or(state.velocity_azimuth, 0.0);
		finite_or(state.velocity_elevation, 0.0);
		finite_or(state.velocity_zoom, 0.0);

		const double step = std::min(delta_time, kMaximumFrameTime);
		const double speed_multiplier = input.sprint ? parameters.sprint_multiplier : (input.crawl ? parameters.crawl_multiplier : 1.0);
		const double altitude_scale = std::clamp(std::sqrt(state.altitude_ratio), 0.2, 1.5);
		const double rate = parameters.angular_speed_deg_s * SurfaceGeometry::kDegreesToRadians * altitude_scale * speed_multiplier;
		const double blend = (parameters.smoothing_seconds > 1e-4) ? (1.0 - std::exp(-step / parameters.smoothing_seconds)) : 1.0;

		const double target_azimuth = std::clamp(input.orbit_east, -1.0, 1.0) * rate;
		const double target_elevation = std::clamp(input.orbit_north, -1.0, 1.0) * rate;
		const double target_zoom = std::clamp(input.zoom, -1.0, 1.0) * parameters.zoom_rate * speed_multiplier;
		state.velocity_azimuth += (target_azimuth - state.velocity_azimuth) * blend;
		state.velocity_elevation += (target_elevation - state.velocity_elevation) * blend;
		state.velocity_zoom += (target_zoom - state.velocity_zoom) * blend;

		const double drag_scale = parameters.drag_sensitivity_deg_per_pixel * SurfaceGeometry::kDegreesToRadians * altitude_scale;
		const double drag_east = std::clamp(std::isfinite(input.drag_east_pixels) ? input.drag_east_pixels * drag_scale : 0.0, -kMaximumDragRadians, kMaximumDragRadians);
		const double drag_north = std::clamp(std::isfinite(input.drag_north_pixels) ? input.drag_north_pixels * drag_scale : 0.0, -kMaximumDragRadians, kMaximumDragRadians);

		const double pole_cosine = std::max(std::cos(state.elevation), kMinimumPoleCosine);
		state.azimuth += (state.velocity_azimuth * step + drag_east) / pole_cosine;
		const double pitch_limit = parameters.maximum_pitch_deg * SurfaceGeometry::kDegreesToRadians;
		state.elevation = std::clamp(state.elevation + state.velocity_elevation * step + drag_north, -pitch_limit, pitch_limit);
		state.azimuth = std::remainder(state.azimuth, 2.0 * std::numbers::pi_v<double>);

		const double wheel = std::isfinite(input.zoom_steps) ? std::clamp(input.zoom_steps, -20.0, 20.0) : 0.0;
		state.altitude_ratio *= std::exp(state.velocity_zoom * step - wheel * parameters.wheel_zoom_factor);
		state.altitude_ratio = std::clamp(state.altitude_ratio, parameters.minimum_altitude_ratio, parameters.maximum_altitude_ratio);
	}

	[[nodiscard]] static PlanetOrbitPose evaluate(
		const PlanetOrbitState& state,
		const PlanetOrbitEnvironment& environment,
		const PlanetOrbitParameters& parameters
	) noexcept {
		const Vector3 unit = direction(state);
		const double surface = SurfaceGeometry::ellipsoid_radius(environment.axes, unit);
		const double ratio = std::clamp(state.altitude_ratio, parameters.minimum_altitude_ratio, parameters.maximum_altitude_ratio);
		const double altitude = ratio * mean_radius(environment.axes);

		PlanetOrbitPose pose;
		pose.surface_radius = surface;
		pose.altitude = altitude;
		pose.position = SurfaceGeometry::add(environment.center, SurfaceGeometry::scale(unit, surface + altitude));

		const Vector3 forward = SurfaceGeometry::scale(unit, -1.0);
		const Vector3 world_up{0.0, 0.0, 1.0};
		const Vector3 up = SurfaceGeometry::normalized(
			SurfaceGeometry::subtract(world_up, SurfaceGeometry::scale(forward, SurfaceGeometry::dot(world_up, forward))),
			SurfaceGeometry::tangent_fallback(forward)
		);
		pose.angles = SurfaceGeometry::euler_from_frame(forward, up, state.azimuth * SurfaceGeometry::kRadiansToDegrees + 180.0);
		return pose;
	}
};

}

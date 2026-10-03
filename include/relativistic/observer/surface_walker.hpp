#pragma once

#include "relativistic/observer/surface_geometry.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace Relativistic::Observer {

using SurfaceGeometry::Vector3;

struct SurfaceWalkerDimensions {
	double height{1.0};
	double eye_height{0.93};
	double crouch_eye_height{0.5};
	double walk_speed{1.0};
	double step_length{0.5};
	double jump_height{0.5};
};

struct SurfaceWalkerParameters {
	bool scale_with_body{true};
	double walker_height{0.02};
	double eye_height_fraction{0.93};
	double crouch_fraction{0.55};
	double crouch_speed_multiplier{0.5};
	double walk_speed{0.06};
	double sprint_multiplier{2.5};
	double crawl_multiplier{0.3};
	double step_length{0.012};
	double jump_height{0.01};
	bool automatic_gravity{true};
	double gravity_scale{1.0};
	double manual_gravity{9.80665};
	double maximum_airtime_seconds{20.0};
	double ground_response_seconds{0.08};
	double air_control{0.25};
	bool head_bob_enabled{true};
	double head_bob_amplitude{0.03};
	bool follow_surface_rotation{true};

	void sanitize() noexcept {
		walker_height = std::clamp(walker_height, 1e-12, 1e12);
		eye_height_fraction = std::clamp(eye_height_fraction, 0.5, 1.0);
		crouch_fraction = std::clamp(crouch_fraction, 0.2, 1.0);
		crouch_speed_multiplier = std::clamp(crouch_speed_multiplier, 0.05, 1.0);
		walk_speed = std::clamp(walk_speed, 1e-12, 1e12);
		sprint_multiplier = std::clamp(sprint_multiplier, 1.0, 20.0);
		crawl_multiplier = std::clamp(crawl_multiplier, 0.01, 1.0);
		step_length = std::clamp(step_length, 1e-12, 1e12);
		jump_height = std::clamp(jump_height, 0.0, 1e12);
		gravity_scale = std::clamp(gravity_scale, 0.0, 1e6);
		manual_gravity = std::clamp(manual_gravity, 0.0, 1e12);
		maximum_airtime_seconds = std::clamp(maximum_airtime_seconds, 0.5, 600.0);
		ground_response_seconds = std::clamp(ground_response_seconds, 0.0, 2.0);
		air_control = std::clamp(air_control, 0.0, 1.0);
		head_bob_amplitude = std::clamp(head_bob_amplitude, 0.0, 0.2);
	}

	void convert_scale_mode(bool relative, double reference_length) noexcept {
		if (relative == scale_with_body) {
			return;
		}
		const double reference = std::max(reference_length, 1e-12);
		const double factor = relative ? (1.0 / reference) : reference;
		walker_height *= factor;
		walk_speed *= factor;
		step_length *= factor;
		jump_height *= factor;
		scale_with_body = relative;
	}

	[[nodiscard]] SurfaceWalkerDimensions resolve(double reference_length) const noexcept {
		const double unit = scale_with_body ? std::max(reference_length, 1e-12) : 1.0;
		SurfaceWalkerDimensions dimensions;
		dimensions.height = walker_height * unit;
		dimensions.eye_height = dimensions.height * eye_height_fraction;
		dimensions.crouch_eye_height = dimensions.eye_height * crouch_fraction;
		dimensions.walk_speed = walk_speed * unit;
		dimensions.step_length = step_length * unit;
		dimensions.jump_height = jump_height * unit;
		return dimensions;
	}
};

struct SurfaceWalkerEnvironment {
	Vector3 center{0.0, 0.0, 0.0};
	Vector3 axes{1.0, 1.0, 1.0};
	double mass{0.0};
	double gravitational_constant{1.0};
	double surface_rotation_rate{0.0};
	double logical_time{0.0};
};

struct SurfaceWalkerInput {
	double move_forward{0.0};
	double move_right{0.0};
	double look_yaw_delta_deg{0.0};
	double look_pitch_delta_deg{0.0};
	bool jump{false};
	bool crouch{false};
	bool sprint{false};
	bool crawl{false};
};

struct SurfaceWalkerState {
	bool active{false};
	uint32_t body_id{0};
	Vector3 direction{0.0, 0.0, 1.0};
	Vector3 heading{1.0, 0.0, 0.0};
	double look_pitch_deg{0.0};
	double lift{0.0};
	double vertical_velocity{0.0};
	double velocity_forward{0.0};
	double velocity_right{0.0};
	double stride_phase{0.0};
	double bob_weight{0.0};
	double crouch_blend{0.0};
	double last_yaw_deg{0.0};
	bool jump_was_pressed{false};
};

struct SurfaceWalkerPose {
	Vector3 position{0.0, 0.0, 0.0};
	Vector3 ground_normal{0.0, 0.0, 1.0};
	SurfaceGeometry::FrameAngles angles{};
	double surface_radius{1.0};
	double gravity{0.0};
	double altitude{0.0};
	double eye_height{0.0};
	bool grounded{true};
};

class SurfaceWalker {
public:
	static constexpr double kMaximumSubstep = 0.02;
	static constexpr double kMaximumFrameTime = 0.25;
	static constexpr double kMaximumLookPitchDeg = 89.0;

	[[nodiscard]] static double mean_radius(const Vector3& axes) noexcept {
		return std::cbrt(axes[0] * axes[1] * axes[2]);
	}

	[[nodiscard]] static double rotation_angle(const SurfaceWalkerParameters& parameters, const SurfaceWalkerEnvironment& environment) noexcept {
		return parameters.follow_surface_rotation ? (-environment.surface_rotation_rate * environment.logical_time) : 0.0;
	}

	[[nodiscard]] static double gravity(
		const SurfaceWalkerParameters& parameters,
		const SurfaceWalkerEnvironment& environment,
		const SurfaceWalkerDimensions& dimensions,
		double surface_radius
	) noexcept {
		const double radius = std::max(surface_radius, 1e-12);
		const double base = parameters.automatic_gravity
			? (parameters.gravity_scale * environment.gravitational_constant * std::max(environment.mass, 0.0) / (radius * radius))
			: parameters.manual_gravity;
		const double airtime = std::max(parameters.maximum_airtime_seconds, 0.5);
		const double floor = 8.0 * dimensions.jump_height / (airtime * airtime);
		return std::max({base, floor, 1e-15});
	}

	static void initialize(
		SurfaceWalkerState& state,
		const SurfaceWalkerEnvironment& environment,
		const SurfaceWalkerParameters& parameters,
		uint32_t body_id,
		const Vector3& camera_position,
		const Vector3& camera_forward
	) noexcept {
		const double angle = rotation_angle(parameters, environment);
		const Vector3 world_direction = SurfaceGeometry::normalized(SurfaceGeometry::subtract(camera_position, environment.center), {0.0, 0.0, 1.0});
		const double forward_vertical = SurfaceGeometry::dot(camera_forward, world_direction);
		const Vector3 world_heading = SurfaceGeometry::normalized(
			SurfaceGeometry::subtract(camera_forward, SurfaceGeometry::scale(world_direction, forward_vertical)),
			SurfaceGeometry::tangent_fallback(world_direction)
		);
		state = SurfaceWalkerState{};
		state.active = true;
		state.body_id = body_id;
		state.direction = SurfaceGeometry::rotate_z(world_direction, -angle);
		state.heading = SurfaceGeometry::rotate_z(world_heading, -angle);
		state.look_pitch_deg = std::clamp(std::asin(std::clamp(forward_vertical, -1.0, 1.0)) * SurfaceGeometry::kRadiansToDegrees, -kMaximumLookPitchDeg, kMaximumLookPitchDeg);
		state.last_yaw_deg = std::atan2(camera_forward[1], camera_forward[0]) * SurfaceGeometry::kRadiansToDegrees;
	}

	static void advance(
		SurfaceWalkerState& state,
		const SurfaceWalkerEnvironment& environment,
		const SurfaceWalkerParameters& parameters,
		const SurfaceWalkerDimensions& dimensions,
		const SurfaceWalkerInput& input,
		double delta_time
	) noexcept {
		if (!state.active || !(delta_time > 0.0)) {
			return;
		}
		apply_look(state, input);
		bool jump_request = input.jump && !state.jump_was_pressed;
		state.jump_was_pressed = input.jump;
		const double clamped_time = std::min(delta_time, kMaximumFrameTime);
		const int substeps = std::clamp(static_cast<int>(std::ceil(clamped_time / kMaximumSubstep)), 1, 64);
		const double step = clamped_time / static_cast<double>(substeps);
		for (int i = 0; i < substeps; ++i) {
			integrate(state, environment, parameters, dimensions, input, step, jump_request);
		}
	}

	[[nodiscard]] static SurfaceWalkerPose evaluate(
		const SurfaceWalkerState& state,
		const SurfaceWalkerEnvironment& environment,
		const SurfaceWalkerParameters& parameters,
		const SurfaceWalkerDimensions& dimensions
	) noexcept {
		const double angle = rotation_angle(parameters, environment);
		const Vector3 world_direction = SurfaceGeometry::normalized(SurfaceGeometry::rotate_z(state.direction, angle), {0.0, 0.0, 1.0});
		const Vector3 world_heading = SurfaceGeometry::rotate_z(state.heading, angle);
		const double surface_radius = SurfaceGeometry::ellipsoid_radius(environment.axes, world_direction);
		const Vector3 normal = SurfaceGeometry::ellipsoid_normal(environment.axes, world_direction);
		const Vector3 surface_point = SurfaceGeometry::add(environment.center, SurfaceGeometry::scale(world_direction, surface_radius));

		const double blend = std::clamp(state.crouch_blend, 0.0, 1.0);
		double eye_height = dimensions.eye_height + (dimensions.crouch_eye_height - dimensions.eye_height) * blend;
		if (parameters.head_bob_enabled) {
			eye_height -= state.bob_weight * parameters.head_bob_amplitude * dimensions.eye_height * std::abs(std::sin(state.stride_phase));
		}

		const Vector3 forward_tangent = SurfaceGeometry::normalized(
			SurfaceGeometry::subtract(world_heading, SurfaceGeometry::scale(normal, SurfaceGeometry::dot(world_heading, normal))),
			SurfaceGeometry::tangent_fallback(normal)
		);
		const double pitch = state.look_pitch_deg * SurfaceGeometry::kDegreesToRadians;
		const Vector3 forward = SurfaceGeometry::add(SurfaceGeometry::scale(forward_tangent, std::cos(pitch)), SurfaceGeometry::scale(normal, std::sin(pitch)));
		const Vector3 up = SurfaceGeometry::subtract(SurfaceGeometry::scale(normal, std::cos(pitch)), SurfaceGeometry::scale(forward_tangent, std::sin(pitch)));

		SurfaceWalkerPose pose;
		pose.position = SurfaceGeometry::add(surface_point, SurfaceGeometry::scale(normal, eye_height + state.lift));
		pose.ground_normal = normal;
		pose.angles = SurfaceGeometry::euler_from_frame(forward, up, state.last_yaw_deg);
		pose.surface_radius = surface_radius;
		pose.gravity = gravity(parameters, environment, dimensions, surface_radius);
		pose.altitude = state.lift;
		pose.eye_height = eye_height;
		pose.grounded = state.lift <= 0.0 && state.vertical_velocity <= 0.0;
		return pose;
	}

private:
	static void apply_look(SurfaceWalkerState& state, const SurfaceWalkerInput& input) noexcept {
		if (input.look_yaw_delta_deg != 0.0) {
			state.heading = SurfaceGeometry::rotate_about_axis(state.heading, state.direction, input.look_yaw_delta_deg * SurfaceGeometry::kDegreesToRadians);
		}
		state.look_pitch_deg = std::clamp(state.look_pitch_deg + input.look_pitch_delta_deg, -kMaximumLookPitchDeg, kMaximumLookPitchDeg);
	}

	static void integrate(
		SurfaceWalkerState& state,
		const SurfaceWalkerEnvironment& environment,
		const SurfaceWalkerParameters& parameters,
		const SurfaceWalkerDimensions& dimensions,
		const SurfaceWalkerInput& input,
		double step,
		bool& jump_request
	) noexcept {
		state.direction = SurfaceGeometry::normalized(state.direction, {0.0, 0.0, 1.0});
		state.heading = SurfaceGeometry::normalized(
			SurfaceGeometry::subtract(state.heading, SurfaceGeometry::scale(state.direction, SurfaceGeometry::dot(state.heading, state.direction))),
			SurfaceGeometry::tangent_fallback(state.direction)
		);
		const Vector3 right = SurfaceGeometry::cross(state.heading, state.direction);
		const Vector3 world_direction = SurfaceGeometry::rotate_z(state.direction, rotation_angle(parameters, environment));
		const double surface_radius = SurfaceGeometry::ellipsoid_radius(environment.axes, world_direction);
		const bool grounded = state.lift <= 0.0 && state.vertical_velocity <= 0.0;

		const double input_length = std::hypot(input.move_forward, input.move_right);
		const double input_scale = (input_length > 1.0) ? (1.0 / input_length) : 1.0;
		const bool has_input = input_length > 1e-6;
		double speed = dimensions.walk_speed;
		if (input.sprint) {
			speed *= parameters.sprint_multiplier;
		} else if (input.crawl) {
			speed *= parameters.crawl_multiplier;
		}
		if (state.crouch_blend > 0.5) {
			speed *= parameters.crouch_speed_multiplier;
		}
		const double target_forward = input.move_forward * input_scale * speed;
		const double target_right = input.move_right * input_scale * speed;
		const double ground_blend = (parameters.ground_response_seconds > 1e-6) ? (1.0 - std::exp(-step / parameters.ground_response_seconds)) : 1.0;
		if (grounded || has_input) {
			const double blend = grounded ? ground_blend : (ground_blend * parameters.air_control);
			state.velocity_forward += (target_forward - state.velocity_forward) * blend;
			state.velocity_right += (target_right - state.velocity_right) * blend;
		}

		const Vector3 displacement = SurfaceGeometry::add(
			SurfaceGeometry::scale(state.heading, state.velocity_forward * step),
			SurfaceGeometry::scale(right, state.velocity_right * step)
		);
		const double distance = SurfaceGeometry::length(displacement);
		if (distance > 1e-15) {
			const Vector3 axis = SurfaceGeometry::scale(displacement, 1.0 / distance);
			const double angle = distance / std::max(surface_radius, 1e-12);
			const double cosine = std::cos(angle);
			const double sine = std::sin(angle);
			const double along = SurfaceGeometry::dot(state.heading, axis);
			const Vector3 transported_axis = SurfaceGeometry::subtract(SurfaceGeometry::scale(axis, cosine), SurfaceGeometry::scale(state.direction, sine));
			const Vector3 next_direction = SurfaceGeometry::add(SurfaceGeometry::scale(state.direction, cosine), SurfaceGeometry::scale(axis, sine));
			const Vector3 next_heading = SurfaceGeometry::add(
				SurfaceGeometry::subtract(state.heading, SurfaceGeometry::scale(axis, along)),
				SurfaceGeometry::scale(transported_axis, along)
			);
			state.direction = SurfaceGeometry::normalized(next_direction, state.direction);
			state.heading = next_heading;
		}

		if (grounded && distance > 1e-15) {
			state.stride_phase = std::fmod(state.stride_phase + std::numbers::pi_v<double> * distance / std::max(dimensions.step_length, 1e-12), 2.0 * std::numbers::pi_v<double>);
		}
		const double bob_target = (grounded && distance > 1e-15) ? 1.0 : 0.0;
		state.bob_weight += (bob_target - state.bob_weight) * (1.0 - std::exp(-step * 8.0));

		const double crouch_target = input.crouch ? 1.0 : 0.0;
		state.crouch_blend += (crouch_target - state.crouch_blend) * (1.0 - std::exp(-step * 12.0));

		const double gravity_value = gravity(parameters, environment, dimensions, surface_radius);
		if (jump_request) {
			if (grounded) {
				state.vertical_velocity = std::sqrt(2.0 * gravity_value * dimensions.jump_height);
			}
			jump_request = false;
		}
		if (state.lift > 0.0 || state.vertical_velocity > 0.0) {
			state.lift += state.vertical_velocity * step - 0.5 * gravity_value * step * step;
			state.vertical_velocity -= gravity_value * step;
			if (state.lift <= 0.0) {
				state.lift = 0.0;
				state.vertical_velocity = 0.0;
			}
		}
	}
};

}

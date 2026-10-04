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
	bool head_bob_enabled{false};
	double head_bob_amplitude{0.03};
	bool follow_surface_rotation{true};
	double maximum_look_pitch_deg{85.0};
	double maximum_drop_heights{40.0};

	static constexpr double kMinimumRelativeHeight = 1e-5;
	static constexpr double kMaximumRelativeHeight = 1.0;
	static constexpr double kMinimumRelativeSpeed = 1e-5;
	static constexpr double kMaximumRelativeSpeed = 50.0;
	static constexpr double kMinimumRelativeStep = 1e-6;
	static constexpr double kMaximumRelativeJump = 2.0;

	void sanitize() noexcept {
		const auto repair = [](double& value, double fallback) noexcept {
			if (!std::isfinite(value)) {
				value = fallback;
			}
		};
		repair(walker_height, 0.02);
		repair(eye_height_fraction, 0.93);
		repair(crouch_fraction, 0.55);
		repair(crouch_speed_multiplier, 0.5);
		repair(walk_speed, 0.06);
		repair(sprint_multiplier, 2.5);
		repair(crawl_multiplier, 0.3);
		repair(step_length, 0.012);
		repair(jump_height, 0.01);
		repair(gravity_scale, 1.0);
		repair(manual_gravity, 9.80665);
		repair(maximum_airtime_seconds, 20.0);
		repair(ground_response_seconds, 0.08);
		repair(air_control, 0.25);
		repair(head_bob_amplitude, 0.03);
		repair(maximum_look_pitch_deg, 85.0);
		repair(maximum_drop_heights, 40.0);
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
		maximum_look_pitch_deg = std::clamp(maximum_look_pitch_deg, 10.0, 89.0);
		maximum_drop_heights = std::clamp(maximum_drop_heights, 0.0, 1e6);
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
		const double reference = std::max(reference_length, 1e-12);
		const double unit = scale_with_body ? reference : 1.0;
		SurfaceWalkerDimensions dimensions;
		dimensions.height = std::clamp(walker_height * unit, reference * kMinimumRelativeHeight, reference * kMaximumRelativeHeight);
		dimensions.eye_height = dimensions.height * eye_height_fraction;
		dimensions.crouch_eye_height = dimensions.eye_height * crouch_fraction;
		dimensions.walk_speed = std::clamp(walk_speed * unit, reference * kMinimumRelativeSpeed, reference * kMaximumRelativeSpeed);
		dimensions.step_length = std::clamp(step_length * unit, reference * kMinimumRelativeStep, reference * kMaximumRelativeHeight);
		dimensions.jump_height = std::clamp(jump_height * unit, 0.0, reference * kMaximumRelativeJump);
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
	double airborne_gravity{0.0};
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
	static constexpr double kMaximumLookDeltaDeg = 120.0;
	static constexpr double kMaximumArcPerSubstep = 0.1;
	static constexpr double kMaximumGravity = 1.0e9;
	static constexpr double kMinimumEyeClearanceFraction = 0.05;
	static constexpr double kMinimumAbsoluteEyeClearance = 2.0e-5;

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
		const double raw_base = parameters.automatic_gravity
			? (parameters.gravity_scale * environment.gravitational_constant * std::max(environment.mass, 0.0) / (radius * radius))
			: parameters.manual_gravity;
		const double base = std::isfinite(raw_base) ? std::clamp(raw_base, 0.0, kMaximumGravity) : 0.0;
		const double airtime = std::max(parameters.maximum_airtime_seconds, 0.5);
		const double floor = 8.0 * dimensions.jump_height / (airtime * airtime);
		return std::max({base, floor, 1e-15});
	}

	[[nodiscard]] static double fall_gravity(
		double base_gravity,
		double drop_height,
		const SurfaceWalkerParameters& parameters
	) noexcept {
		const double fall_time = std::max(parameters.maximum_airtime_seconds * 0.5, 0.25);
		return std::min(std::max(base_gravity, 2.0 * std::max(drop_height, 0.0) / (fall_time * fall_time)), kMaximumGravity);
	}

	static void initialize(
		SurfaceWalkerState& state,
		const SurfaceWalkerEnvironment& environment,
		const SurfaceWalkerParameters& parameters,
		const SurfaceWalkerDimensions& dimensions,
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
		const double pitch_limit = std::clamp(parameters.maximum_look_pitch_deg, 10.0, kMaximumLookPitchDeg);
		state.look_pitch_deg = std::clamp(std::asin(std::clamp(forward_vertical, -1.0, 1.0)) * SurfaceGeometry::kRadiansToDegrees, -pitch_limit, pitch_limit);
		state.last_yaw_deg = std::atan2(camera_forward[1], camera_forward[0]) * SurfaceGeometry::kRadiansToDegrees;
		const double surface_radius = SurfaceGeometry::ellipsoid_radius(environment.axes, world_direction);
		const double altitude = SurfaceGeometry::length(SurfaceGeometry::subtract(camera_position, environment.center)) - surface_radius - dimensions.eye_height;
		const double maximum_drop = std::max(parameters.maximum_drop_heights, 0.0) * dimensions.height;
		state.lift = std::isfinite(altitude) ? std::clamp(altitude, 0.0, maximum_drop) : 0.0;
		if (state.lift > 0.0) {
			state.airborne_gravity = fall_gravity(gravity(parameters, environment, dimensions, surface_radius), state.lift, parameters);
		}
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
		repair(state);
		apply_look(state, parameters, input);
		bool jump_request = input.jump && !state.jump_was_pressed;
		state.jump_was_pressed = input.jump;
		const double clamped_time = std::min(delta_time, kMaximumFrameTime);
		const double reference = std::max(mean_radius(environment.axes), 1e-12);
		const double fastest = dimensions.walk_speed * std::max(parameters.sprint_multiplier, 1.0);
		const double arc_substeps = std::ceil(fastest * clamped_time / (reference * kMaximumArcPerSubstep));
		const int substeps = std::clamp(static_cast<int>(std::max(std::ceil(clamped_time / kMaximumSubstep), std::isfinite(arc_substeps) ? arc_substeps : 1.0)), 1, 128);
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
		eye_height = std::max({eye_height, dimensions.height * kMinimumEyeClearanceFraction, kMinimumAbsoluteEyeClearance});

		const Vector3 forward_tangent = SurfaceGeometry::normalized(
			SurfaceGeometry::subtract(world_heading, SurfaceGeometry::scale(normal, SurfaceGeometry::dot(world_heading, normal))),
			SurfaceGeometry::tangent_fallback(normal)
		);
		const double pitch_limit = std::clamp(parameters.maximum_look_pitch_deg, 10.0, kMaximumLookPitchDeg);
		const double pitch = std::clamp(state.look_pitch_deg, -pitch_limit, pitch_limit) * SurfaceGeometry::kDegreesToRadians;
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
	static void apply_look(SurfaceWalkerState& state, const SurfaceWalkerParameters& parameters, const SurfaceWalkerInput& input) noexcept {
		const double pitch_limit = std::clamp(parameters.maximum_look_pitch_deg, 10.0, kMaximumLookPitchDeg);
		const double yaw_delta = std::isfinite(input.look_yaw_delta_deg) ? std::clamp(input.look_yaw_delta_deg, -kMaximumLookDeltaDeg, kMaximumLookDeltaDeg) : 0.0;
		const double pitch_delta = std::isfinite(input.look_pitch_delta_deg) ? std::clamp(input.look_pitch_delta_deg, -kMaximumLookDeltaDeg, kMaximumLookDeltaDeg) : 0.0;
		if (yaw_delta != 0.0) {
			state.heading = SurfaceGeometry::rotate_about_axis(state.heading, state.direction, yaw_delta * SurfaceGeometry::kDegreesToRadians);
		}
		state.look_pitch_deg = std::clamp(state.look_pitch_deg + pitch_delta, -pitch_limit, pitch_limit);
	}

	static void repair(SurfaceWalkerState& state) noexcept {
		const auto finite_vector = [](const Vector3& value) noexcept {
			return std::isfinite(value[0]) && std::isfinite(value[1]) && std::isfinite(value[2]);
		};
		const auto finite_scalar = [](double& value, double fallback) noexcept {
			if (!std::isfinite(value)) {
				value = fallback;
			}
		};
		if (!finite_vector(state.direction) || SurfaceGeometry::length(state.direction) < 1e-9) {
			state.direction = {0.0, 0.0, 1.0};
			state.heading = {1.0, 0.0, 0.0};
		}
		if (!finite_vector(state.heading) || SurfaceGeometry::length(state.heading) < 1e-9) {
			state.heading = SurfaceGeometry::tangent_fallback(SurfaceGeometry::normalized(state.direction, {0.0, 0.0, 1.0}));
		}
		finite_scalar(state.lift, 0.0);
		finite_scalar(state.vertical_velocity, 0.0);
		finite_scalar(state.velocity_forward, 0.0);
		finite_scalar(state.velocity_right, 0.0);
		finite_scalar(state.stride_phase, 0.0);
		finite_scalar(state.bob_weight, 0.0);
		finite_scalar(state.crouch_blend, 0.0);
		finite_scalar(state.look_pitch_deg, 0.0);
		finite_scalar(state.airborne_gravity, 0.0);
		state.lift = std::max(state.lift, 0.0);
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
		const double velocity_limit = dimensions.walk_speed * std::max(parameters.sprint_multiplier, 1.0) * 1.5;
		state.velocity_forward = std::clamp(state.velocity_forward, -velocity_limit, velocity_limit);
		state.velocity_right = std::clamp(state.velocity_right, -velocity_limit, velocity_limit);

		const Vector3 displacement = SurfaceGeometry::add(
			SurfaceGeometry::scale(state.heading, state.velocity_forward * step),
			SurfaceGeometry::scale(right, state.velocity_right * step)
		);
		const double distance = SurfaceGeometry::length(displacement);
		const double movement_threshold = std::max(surface_radius, 1e-12) * 1e-12;
		if (distance > movement_threshold) {
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

		if (grounded && distance > movement_threshold) {
			state.stride_phase = std::fmod(state.stride_phase + std::numbers::pi_v<double> * distance / std::max(dimensions.step_length, 1e-12), 2.0 * std::numbers::pi_v<double>);
		}
		const double bob_target = (grounded && distance > movement_threshold) ? 1.0 : 0.0;
		state.bob_weight += (bob_target - state.bob_weight) * (1.0 - std::exp(-step * 8.0));

		const double crouch_target = input.crouch ? 1.0 : 0.0;
		state.crouch_blend += (crouch_target - state.crouch_blend) * (1.0 - std::exp(-step * 12.0));

		const double base_gravity = gravity(parameters, environment, dimensions, surface_radius);
		if (jump_request) {
			if (grounded && dimensions.jump_height > 0.0) {
				state.airborne_gravity = base_gravity;
				state.vertical_velocity = std::sqrt(2.0 * base_gravity * dimensions.jump_height);
			}
			jump_request = false;
		}
		if (state.lift > 0.0 || state.vertical_velocity > 0.0) {
			const double gravity_value = (state.airborne_gravity > 0.0) ? state.airborne_gravity : fall_gravity(base_gravity, state.lift, parameters);
			state.lift += state.vertical_velocity * step - 0.5 * gravity_value * step * step;
			state.vertical_velocity -= gravity_value * step;
			if (!std::isfinite(state.lift) || !std::isfinite(state.vertical_velocity) || state.lift <= 0.0) {
				state.lift = 0.0;
				state.vertical_velocity = 0.0;
				state.airborne_gravity = 0.0;
			}
		}
	}
};

}

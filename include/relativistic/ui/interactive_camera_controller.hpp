#pragma once

#include "relativistic/core/engine_log.hpp"
#include "relativistic/observer/camera_collision.hpp"
#include "relativistic/observer/planet_orbit.hpp"
#include "relativistic/observer/surface_walker.hpp"
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/ui/camera_control_config.hpp"
#include <GLFW/glfw3.h>
#include <cmath>
#include <numbers>
#include <array>
#include <algorithm>
#include <limits>
#include <mutex>
#include <optional>

namespace Relativistic::UI {

enum class CameraNavigationMode : uint32_t {
	FreeFly6DOF = 0,
	OrbitCenter = 1,
	SphericalBoyerLindquist = 2,
	RocketThrust = 3,
	SurfaceWalk = 4,
	PlanetOrbit = 5
};

struct SurfaceWalkTelemetry {
	bool active{false};
	bool grounded{true};
	uint32_t body_id{0};
	double gravity{0.0};
	double altitude{0.0};
	double speed{0.0};
	double surface_radius{0.0};
	double reference_length{0.0};
};

class InteractiveCameraController {
private:
	Orchestrator::SimulationOrchestrator<1024>& orchestrator_;
	CameraNavigationMode navigation_mode_{CameraNavigationMode::FreeFly6DOF};
	CameraControlConfig config_{};
	bool is_dragging_{false};
	double last_mouse_x_{0.0};
	double last_mouse_y_{0.0};
	double press_mouse_x_{0.0};
	double press_mouse_y_{0.0};
	bool drag_threshold_exceeded_{false};
	static constexpr double kClickDragThresholdPixels = 4.0;
	CameraNavigationMode previous_mode_{CameraNavigationMode::FreeFly6DOF};
	Observer::SurfaceWalkerState walker_state_{};
	Observer::PlanetOrbitState planet_orbit_state_{};
	Observer::CameraCollisionField collision_field_{};
	SurfaceWalkTelemetry walk_telemetry_{};
	double look_delta_yaw_deg_{0.0};
	double look_delta_pitch_deg_{0.0};
	double mouse_delta_x_{0.0};
	double mouse_delta_y_{0.0};
	double pending_scroll_steps_{0.0};

public:
	explicit InteractiveCameraController(Orchestrator::SimulationOrchestrator<1024>& orchestrator) noexcept
		: orchestrator_(orchestrator) {}

	[[nodiscard]] CameraControlConfig& config() noexcept { return config_; }
	[[nodiscard]] const CameraControlConfig& config() const noexcept { return config_; }

	void set_navigation_mode(CameraNavigationMode mode) noexcept {
		navigation_mode_ = mode;
		orchestrator_.parameters().camera_mode = static_cast<uint32_t>(mode);
	}

	[[nodiscard]] CameraNavigationMode navigation_mode() const noexcept {
		return navigation_mode_;
	}

	void set_uniform_speed(double speed) noexcept {
		const double safe_speed = std::max(speed, 0.01);
		config_.free_fly.forward_speed = safe_speed;
		config_.free_fly.lateral_speed = safe_speed;
		config_.free_fly.vertical_speed = safe_speed;
		orchestrator_.camera().speed = safe_speed;
	}

	void set_move_speed(double speed) noexcept { set_uniform_speed(speed); }

	[[nodiscard]] double move_speed() const noexcept { return config_.free_fly.forward_speed; }

	void set_mouse_sensitivity(double sens) noexcept {
		config_.free_fly.mouse_sensitivity = std::clamp(sens, 0.01, 2.0);
	}

	[[nodiscard]] double mouse_sensitivity() const noexcept {
		return config_.free_fly.mouse_sensitivity;
	}

	[[nodiscard]] bool is_actively_navigating() const noexcept {
		return is_dragging_ && drag_threshold_exceeded_;
	}

	[[nodiscard]] bool is_in_motion() const noexcept {
		if (is_actively_navigating()) {
			return true;
		}
		switch (navigation_mode_) {
			case CameraNavigationMode::SurfaceWalk:
				return walk_telemetry_.active && (!walk_telemetry_.grounded || walk_telemetry_.speed > std::max(walk_telemetry_.reference_length, 1e-12) * kMotionSpeedThreshold);
			case CameraNavigationMode::PlanetOrbit:
				return planet_orbit_state_.active && (std::abs(planet_orbit_state_.velocity_azimuth) + std::abs(planet_orbit_state_.velocity_elevation) + std::abs(planet_orbit_state_.velocity_zoom)) > kMotionVelocityThreshold;
			default:
				return false;
		}
	}

	void follow_active_target(double dt) noexcept {
		if (!(dt > 0.0)) return;
		synchronize_navigation_mode();
		if (navigation_mode_ == CameraNavigationMode::SurfaceWalk) {
			update_surface_walk_mode(nullptr, dt, false);
		} else if (navigation_mode_ == CameraNavigationMode::PlanetOrbit) {
			update_planet_orbit_mode(nullptr, dt, false);
		}
	}

	void reset_follow_state() noexcept {
		walker_state_.active = false;
		planet_orbit_state_.active = false;
	}

	void snap_to_equatorial_front(double distance = 50.0) noexcept {
		auto& cam = orchestrator_.camera();
		cam.position = {0.0, distance, 0.0};
		cam.velocity = {0.0, 0.0, 0.0};
		cam.pitch = 0.0;
		cam.yaw = -90.0;
		cam.roll = 0.0;
		cam.orbit_distance = distance;
		sync_spherical_from_cartesian();
	}

	void snap_to_equatorial_side(double distance = 50.0) noexcept {
		auto& cam = orchestrator_.camera();
		cam.position = {distance, 0.0, 0.0};
		cam.velocity = {0.0, 0.0, 0.0};
		cam.pitch = 0.0;
		cam.yaw = 180.0;
		cam.roll = 0.0;
		cam.orbit_distance = distance;
		sync_spherical_from_cartesian();
	}

	void snap_to_north_pole(double distance = 50.0) noexcept {
		auto& cam = orchestrator_.camera();
		cam.position = {0.0, 0.001, distance};
		cam.velocity = {0.0, 0.0, 0.0};
		cam.pitch = -89.0;
		cam.yaw = 0.0;
		cam.roll = 0.0;
		cam.orbit_distance = distance;
		sync_spherical_from_cartesian();
	}

	void snap_to_south_pole(double distance = 50.0) noexcept {
		auto& cam = orchestrator_.camera();
		cam.position = {0.0, 0.001, -distance};
		cam.velocity = {0.0, 0.0, 0.0};
		cam.pitch = 89.0;
		cam.yaw = 0.0;
		cam.roll = 0.0;
		cam.orbit_distance = distance;
		sync_spherical_from_cartesian();
	}

	void snap_to_isco(double margin_factor = 1.2) noexcept {
		const double mass = orchestrator_.parameters().mass;
		snap_to_equatorial_front(6.0 * mass * margin_factor);
	}

	void snap_to_photon_sphere(double margin_factor = 1.1) noexcept {
		const double mass = orchestrator_.parameters().mass;
		snap_to_equatorial_front(3.0 * mass * margin_factor);
	}

	void look_at_target(const std::array<double, 3>& target_pos) noexcept {
		auto& cam = orchestrator_.camera();
		const double dx = target_pos[0] - cam.position[0];
		const double dy = target_pos[1] - cam.position[1];
		const double dz = target_pos[2] - cam.position[2];
		const double d_xy = std::sqrt(dx * dx + dy * dy);
		const double d_tot = std::sqrt(d_xy * d_xy + dz * dz);
		if (d_tot > 1e-6) {
			cam.yaw = std::atan2(dy, dx) * (180.0 / std::numbers::pi);
			cam.pitch = std::asin(std::clamp(dz / d_tot, -0.9999, 0.9999)) * (180.0 / std::numbers::pi);
			cam.roll = 0.0;
			cam.target = target_pos;
		}
	}

	void look_at_origin() noexcept {
		look_at_target({0.0, 0.0, 0.0});
	}

	void reset_roll() noexcept {
		orchestrator_.camera().roll = 0.0;
	}

	void enter_surface_walk(uint32_t body_id) noexcept {
		orchestrator_.parameters().surface_walk_body_id = body_id;
		walker_state_.active = false;
		set_navigation_mode(CameraNavigationMode::SurfaceWalk);
	}

	void leave_surface_walk() noexcept {
		set_navigation_mode(CameraNavigationMode::FreeFly6DOF);
	}

	void enter_planet_orbit(uint32_t body_id) noexcept {
		orchestrator_.parameters().surface_walk_body_id = body_id;
		planet_orbit_state_.active = false;
		set_navigation_mode(CameraNavigationMode::PlanetOrbit);
	}

	void leave_planet_orbit() noexcept {
		set_navigation_mode(CameraNavigationMode::FreeFly6DOF);
	}

	[[nodiscard]] const Observer::PlanetOrbitState& planet_orbit_state() const noexcept {
		return planet_orbit_state_;
	}

	[[nodiscard]] const SurfaceWalkTelemetry& surface_walk_telemetry() const noexcept {
		return walk_telemetry_;
	}

	void update(GLFWwindow* window, double dt, bool is_hovered) noexcept {
		if (window == nullptr || dt <= 0.0) return;

		handle_global_shortcuts(window);

		synchronize_navigation_mode();
		const std::array<double, 3> previous_position = orchestrator_.camera().position;

		double boost_multiplier = 1.0;
		if (config_.keybinds.is_active(InputAction::Sprint, window)) {
			boost_multiplier = config_.free_fly.sprint_multiplier;
		} else if (config_.keybinds.is_active(InputAction::Crawl, window)) {
			boost_multiplier = config_.free_fly.crawl_multiplier;
		}

		switch (navigation_mode_) {
			case CameraNavigationMode::OrbitCenter:
				update_orbit_mode(window, dt, is_hovered);
				break;
			case CameraNavigationMode::SphericalBoyerLindquist:
				update_spherical_mode(window, dt, boost_multiplier, is_hovered);
				break;
			case CameraNavigationMode::RocketThrust:
				update_rocket_mode(window, dt, is_hovered);
				break;
			case CameraNavigationMode::SurfaceWalk:
				update_surface_walk_mode(window, dt, is_hovered);
				break;
			case CameraNavigationMode::PlanetOrbit:
				update_planet_orbit_mode(window, dt, is_hovered);
				break;
			case CameraNavigationMode::FreeFly6DOF:
			default:
				update_free_fly_mode(window, dt, boost_multiplier, is_hovered);
				break;
		}

		if (orchestrator_.parameters().camera_collision_enabled && navigation_mode_ != CameraNavigationMode::SurfaceWalk && navigation_mode_ != CameraNavigationMode::PlanetOrbit) {
			apply_view_collision(previous_position);
		}
	}

	void handle_scroll(double yoffset) noexcept {
		if (navigation_mode_ == CameraNavigationMode::PlanetOrbit) {
			pending_scroll_steps_ += yoffset;
			return;
		}
		auto& cam = orchestrator_.camera();
		if (yoffset > 0.0) {
			cam.fov_deg = std::max(5.0, cam.fov_deg - 2.5);
		} else if (yoffset < 0.0) {
			cam.fov_deg = std::min(170.0, cam.fov_deg + 2.5);
		}
		orchestrator_.parameters().camera_fov_deg = cam.fov_deg;
		static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_camera_set_fov(cam.fov_deg)));
	}

private:
	static constexpr double kFollowPositionTolerance = 1.0e-10;
	static constexpr double kFollowAngleToleranceDeg = 1.0e-7;
	static constexpr double kMotionSpeedThreshold = 1.0e-6;
	static constexpr double kMotionVelocityThreshold = 1.0e-4;

	void synchronize_navigation_mode() noexcept {
		uint32_t raw_mode = orchestrator_.parameters().camera_mode;
		if (raw_mode >= Observer::kCameraNavigationModeCount) {
			raw_mode = 0U;
			orchestrator_.parameters().camera_mode = raw_mode;
		}
		const auto requested_mode = static_cast<CameraNavigationMode>(raw_mode);
		if (requested_mode != previous_mode_) {
			if (previous_mode_ == CameraNavigationMode::SurfaceWalk) {
				release_surface_walk_state();
			} else if (previous_mode_ == CameraNavigationMode::PlanetOrbit) {
				release_planet_orbit_state();
			}
			walker_state_.active = false;
			planet_orbit_state_.active = false;
			previous_mode_ = requested_mode;
		}
		navigation_mode_ = requested_mode;
	}

	void apply_follow_pose(const std::array<double, 3>& position, const Observer::SurfaceGeometry::FrameAngles& angles, const std::array<double, 3>& target, double reference_length) noexcept {
		auto& cam = orchestrator_.camera();
		cam.velocity = {0.0, 0.0, 0.0};
		cam.target = target;
		const double position_tolerance = std::max(reference_length, 1e-12) * kFollowPositionTolerance;
		const bool moved = Observer::SurfaceGeometry::length(Observer::SurfaceGeometry::subtract(position, cam.position)) > position_tolerance;
		const bool rotated = std::abs(angles.pitch_deg - cam.pitch) > kFollowAngleToleranceDeg
			|| std::abs(std::remainder(angles.yaw_deg - cam.yaw, 360.0)) > kFollowAngleToleranceDeg
			|| std::abs(std::remainder(angles.roll_deg - cam.roll, 360.0)) > kFollowAngleToleranceDeg;
		if (!moved && !rotated) {
			return;
		}
		cam.position = position;
		cam.pitch = angles.pitch_deg;
		cam.yaw = angles.yaw_deg;
		cam.roll = angles.roll_deg;
		sync_spherical_from_cartesian();
		cam.orbit_distance = cam.radius;
	}

	void handle_global_shortcuts(GLFWwindow* window) noexcept {
		if (config_.keybinds.is_pressed(InputAction::ResetRoll, window)) {
			reset_roll();
		}
		if (config_.keybinds.is_pressed(InputAction::LookAtOrigin, window)) {
			look_at_origin();
		}
		if (config_.keybinds.is_pressed(InputAction::SpeedDecrease, window)) {
			set_uniform_speed(config_.free_fly.forward_speed * 0.95);
		}
		if (config_.keybinds.is_pressed(InputAction::SpeedIncrease, window)) {
			set_uniform_speed(config_.free_fly.forward_speed * 1.05);
		}
		if (config_.keybinds.is_pressed(InputAction::SnapEquatorialFront, window)) {
			snap_to_equatorial_front(50.0);
		}
		if (config_.keybinds.is_pressed(InputAction::SnapEquatorialSide, window)) {
			snap_to_equatorial_side(50.0);
		}
		if (config_.keybinds.is_pressed(InputAction::SnapNorthPole, window)) {
			snap_to_north_pole(50.0);
		}
		if (config_.keybinds.is_pressed(InputAction::SnapSouthPole, window)) {
			snap_to_south_pole(50.0);
		}
		if (config_.keybinds.is_pressed(InputAction::SnapIsco, window)) {
			snap_to_isco();
		}
	}

	void update_free_fly_mode(GLFWwindow* window, double dt, double boost_multiplier, bool is_hovered) noexcept {
		auto& cam = orchestrator_.camera();
		const auto& prof = config_.free_fly;
		const double pitch_rad = cam.pitch * (std::numbers::pi / 180.0);
		const double yaw_rad = cam.yaw * (std::numbers::pi / 180.0);

		const double cos_p = prof.ignore_pitch_roll_for_movement ? 1.0 : std::cos(pitch_rad);
		const double sin_p = prof.ignore_pitch_roll_for_movement ? 0.0 : std::sin(pitch_rad);
		const double cos_y = std::cos(yaw_rad);
		const double sin_y = std::sin(yaw_rad);

		const std::array<double, 3> forward = {cos_p * cos_y, cos_p * sin_y, sin_p};
		const std::array<double, 3> right = {sin_y, -cos_y, 0.0};
		const std::array<double, 3> up = {0.0, 0.0, 1.0};

		const double vert_sign = prof.invert_vertical ? -1.0 : 1.0;
		const double lat_sign = prof.invert_lateral ? -1.0 : 1.0;
		const double fwd_sign = prof.invert_forward ? -1.0 : 1.0;

		std::array<double, 3> accum{0.0, 0.0, 0.0};

		if (config_.keybinds.is_pressed(InputAction::MoveForward, window)) {
			for (size_t i = 0; i < 3; ++i) accum[i] += forward[i] * prof.forward_speed * fwd_sign;
		}
		if (config_.keybinds.is_pressed(InputAction::MoveBackward, window)) {
			for (size_t i = 0; i < 3; ++i) accum[i] -= forward[i] * prof.forward_speed * fwd_sign;
		}
		if (config_.keybinds.is_pressed(InputAction::MoveRight, window)) {
			for (size_t i = 0; i < 3; ++i) accum[i] += right[i] * prof.lateral_speed * lat_sign;
		}
		if (config_.keybinds.is_pressed(InputAction::MoveLeft, window)) {
			for (size_t i = 0; i < 3; ++i) accum[i] -= right[i] * prof.lateral_speed * lat_sign;
		}
		if (config_.keybinds.is_pressed(InputAction::MoveUp, window)) {
			for (size_t i = 0; i < 3; ++i) accum[i] += up[i] * prof.vertical_speed * vert_sign;
		}
		if (config_.keybinds.is_pressed(InputAction::MoveDown, window)) {
			for (size_t i = 0; i < 3; ++i) accum[i] -= up[i] * prof.vertical_speed * vert_sign;
		}

		if (config_.keybinds.is_pressed(InputAction::RollLeft, window)) {
			cam.roll -= prof.roll_speed_deg_s * dt;
		}
		if (config_.keybinds.is_pressed(InputAction::RollRight, window)) {
			cam.roll += prof.roll_speed_deg_s * dt;
		}

		cam.position[0] += accum[0] * boost_multiplier * dt;
		cam.position[1] += accum[1] * boost_multiplier * dt;
		cam.position[2] += accum[2] * boost_multiplier * dt;

		if (accum[0] != 0.0 || accum[1] != 0.0 || accum[2] != 0.0) {
			sync_spherical_from_cartesian();
		}

		handle_mouse_look(window, is_hovered);
	}

	void release_surface_walk_state() noexcept {
		walker_state_ = Observer::SurfaceWalkerState{};
		walk_telemetry_ = SurfaceWalkTelemetry{};
		auto& cam = orchestrator_.camera();
		cam.roll = 0.0;
		cam.pitch = std::clamp(cam.pitch, -89.0, 89.0);
		cam.velocity = {0.0, 0.0, 0.0};
		orchestrator_.notify_state_changed();
	}

	[[nodiscard]] std::optional<Observer::SurfaceWalkerEnvironment> resolve_walker_environment(uint32_t body_id) const noexcept {
		if (body_id == 0U) {
			return std::nullopt;
		}
		auto& system = orchestrator_.nbody_system();
		std::lock_guard<std::recursive_mutex> lock(system.bodies_mutex());
		for (const auto& body : system.bodies()) {
			if (body.id != body_id || !body.enabled || body.is_spacetime_source) {
				continue;
			}
			Observer::SurfaceWalkerEnvironment environment;
			environment.center = body.position;
			environment.axes = Observer::SurfaceGeometry::body_semi_axes(body);
			environment.mass = body.mass;
			environment.gravitational_constant = orchestrator_.physical_gravitational_constant();
			environment.surface_rotation_rate = static_cast<double>(body.rotation_speed_3d);
			environment.logical_time = orchestrator_.scheduler().snapshot().logical_time;
			return environment;
		}
		return std::nullopt;
	}

	[[nodiscard]] uint32_t nearest_walkable_body_id(const std::array<double, 3>& position) const noexcept {
		auto& system = orchestrator_.nbody_system();
		std::lock_guard<std::recursive_mutex> lock(system.bodies_mutex());
		uint32_t best_id = 0U;
		double best_distance = std::numeric_limits<double>::max();
		for (const auto& body : system.bodies()) {
			if (!body.enabled || body.is_spacetime_source) {
				continue;
			}
			const double distance = Observer::SurfaceGeometry::length(Observer::SurfaceGeometry::subtract(position, body.position))
				- Observer::SurfaceWalker::mean_radius(Observer::SurfaceGeometry::body_semi_axes(body));
			if (distance < best_distance) {
				best_distance = distance;
				best_id = body.id;
			}
		}
		return best_id;
	}

	void update_surface_walk_mode(GLFWwindow* window, double dt, bool is_hovered) noexcept {
		auto& params = orchestrator_.parameters();
		auto& cam = orchestrator_.camera();
		auto& profile = config_.surface_walk;
		profile.sanitize();

		auto environment = resolve_walker_environment(params.surface_walk_body_id);
		if (!environment.has_value()) {
			const uint32_t fallback_id = nearest_walkable_body_id(cam.position);
			if (fallback_id != 0U) {
				params.surface_walk_body_id = fallback_id;
				walker_state_.active = false;
				environment = resolve_walker_environment(fallback_id);
			}
		}
		if (!environment.has_value()) {
			Core::log_warning("Surface walk requires at least one enabled celestial body; returning to free fly navigation.");
			set_navigation_mode(CameraNavigationMode::FreeFly6DOF);
			return;
		}

		const double reference_length = Observer::SurfaceWalker::mean_radius(environment->axes);
		const auto dimensions = profile.resolve(reference_length);

		if (!walker_state_.active || walker_state_.body_id != params.surface_walk_body_id) {
			Observer::SurfaceWalker::initialize(walker_state_, *environment, profile, dimensions, params.surface_walk_body_id, cam.position, cam.orientation_basis().forward);
		}

		handle_mouse_look(window, is_hovered);
		if (config_.keybinds.is_pressed(InputAction::ResetRoll, window)) {
			walker_state_.look_pitch_deg = 0.0;
		}

		const auto& keys = config_.keybinds;
		const auto& free_fly = config_.free_fly;
		const auto key_down = [window](int key) noexcept { return window != nullptr && glfwGetKey(window, key) == GLFW_PRESS; };

		Observer::SurfaceWalkerInput input;
		const double move_fwd = (keys.is_pressed(InputAction::MoveForward, window) || key_down(GLFW_KEY_UP) ? 1.0 : 0.0)
			- (keys.is_pressed(InputAction::MoveBackward, window) || key_down(GLFW_KEY_DOWN) ? 1.0 : 0.0);
		const double move_rgt = (keys.is_pressed(InputAction::MoveRight, window) || key_down(GLFW_KEY_RIGHT) ? 1.0 : 0.0)
			- (keys.is_pressed(InputAction::MoveLeft, window) || key_down(GLFW_KEY_LEFT) ? 1.0 : 0.0);
		input.move_forward = free_fly.invert_forward ? -move_fwd : move_fwd;
		input.move_right = free_fly.invert_lateral ? -move_rgt : move_rgt;
		input.jump = keys.is_pressed(InputAction::MoveUp, window) || key_down(GLFW_KEY_SPACE);
		input.crouch = keys.is_pressed(InputAction::MoveDown, window) || key_down(GLFW_KEY_C);
		input.sprint = keys.is_active(InputAction::Sprint, window) || key_down(GLFW_KEY_LEFT_SHIFT) || key_down(GLFW_KEY_RIGHT_SHIFT);
		input.crawl = keys.is_active(InputAction::Crawl, window) || key_down(GLFW_KEY_LEFT_CONTROL) || key_down(GLFW_KEY_RIGHT_CONTROL);
		input.look_yaw_delta_deg = look_delta_yaw_deg_;
		input.look_pitch_delta_deg = look_delta_pitch_deg_;

		Observer::SurfaceWalker::advance(walker_state_, *environment, profile, dimensions, input, dt);
		const auto pose = Observer::SurfaceWalker::evaluate(walker_state_, *environment, profile, dimensions);
		if (!std::isfinite(pose.position[0]) || !std::isfinite(pose.position[1]) || !std::isfinite(pose.position[2])) {
			walker_state_.active = false;
			return;
		}

		walker_state_.last_yaw_deg = pose.angles.yaw_deg;
		Observer::SurfaceGeometry::FrameAngles walk_angles = pose.angles;
		walk_angles.pitch_deg = std::clamp(walk_angles.pitch_deg, -89.0, 89.0);
		apply_follow_pose(pose.position, walk_angles, environment->center, reference_length);

		walk_telemetry_.active = true;
		walk_telemetry_.grounded = pose.grounded;
		walk_telemetry_.body_id = walker_state_.body_id;
		walk_telemetry_.gravity = pose.gravity;
		walk_telemetry_.altitude = pose.altitude;
		walk_telemetry_.speed = std::hypot(walker_state_.velocity_forward, walker_state_.velocity_right);
		walk_telemetry_.surface_radius = pose.surface_radius;
		walk_telemetry_.reference_length = reference_length;
	}

	void apply_view_collision(const std::array<double, 3>& previous_position) noexcept {
		auto& cam = orchestrator_.camera();
		collision_field_.rebuild(orchestrator_);
		const auto resolution = collision_field_.resolve(previous_position, cam.position, orchestrator_.parameters().camera_collision_clearance);
		if (!resolution.adjusted) {
			return;
		}
		cam.position = resolution.position;
		const double inward = Observer::SurfaceGeometry::dot(cam.velocity, resolution.normal);
		if (inward < 0.0) {
			for (size_t i = 0; i < 3; ++i) {
				cam.velocity[i] -= inward * resolution.normal[i];
			}
		}
		if (navigation_mode_ == CameraNavigationMode::OrbitCenter) {
			cam.orbit_distance = Observer::SurfaceGeometry::length(Observer::SurfaceGeometry::subtract(cam.position, cam.target));
		}
		sync_spherical_from_cartesian();
	}

	void update_rocket_mode(GLFWwindow* window, double dt, bool is_hovered) noexcept {
		auto& cam = orchestrator_.camera();
		const auto& prof = config_.rocket;
		const bool time_running = !orchestrator_.scheduler().is_paused();
		const bool can_thrust = !prof.requires_time_running || time_running;

		const double pitch_rad = cam.pitch * (std::numbers::pi / 180.0);
		const double yaw_rad = cam.yaw * (std::numbers::pi / 180.0);
		const double cos_p = std::cos(pitch_rad);
		const double sin_p = std::sin(pitch_rad);
		const double cos_y = std::cos(yaw_rad);
		const double sin_y = std::sin(yaw_rad);

		const std::array<double, 3> forward = {cos_p * cos_y, cos_p * sin_y, sin_p};
		const std::array<double, 3> right = {sin_y, -cos_y, 0.0};
		const std::array<double, 3> up = {0.0, 0.0, 1.0};

		if (can_thrust) {
			const double lat_sign = prof.invert_lateral ? -1.0 : 1.0;
			const double vert_sign = prof.invert_vertical ? -1.0 : 1.0;
			std::array<double, 3> thrust{0.0, 0.0, 0.0};

			if (config_.keybinds.is_pressed(InputAction::MoveForward, window)) {
				for (size_t i = 0; i < 3; ++i) thrust[i] += forward[i] * prof.main_thrust_accel;
			}
			if (config_.keybinds.is_pressed(InputAction::MoveBackward, window)) {
				for (size_t i = 0; i < 3; ++i) thrust[i] -= forward[i] * prof.main_thrust_accel;
			}
			if (config_.keybinds.is_pressed(InputAction::MoveRight, window)) {
				for (size_t i = 0; i < 3; ++i) thrust[i] += right[i] * prof.lateral_thrust_accel * lat_sign;
			}
			if (config_.keybinds.is_pressed(InputAction::MoveLeft, window)) {
				for (size_t i = 0; i < 3; ++i) thrust[i] -= right[i] * prof.lateral_thrust_accel * lat_sign;
			}
			if (config_.keybinds.is_pressed(InputAction::MoveUp, window)) {
				for (size_t i = 0; i < 3; ++i) thrust[i] += up[i] * prof.vertical_thrust_accel * vert_sign;
			}
			if (config_.keybinds.is_pressed(InputAction::MoveDown, window)) {
				for (size_t i = 0; i < 3; ++i) thrust[i] -= up[i] * prof.vertical_thrust_accel * vert_sign;
			}

			const double thrust_mag = std::sqrt(thrust[0] * thrust[0] + thrust[1] * thrust[1] + thrust[2] * thrust[2]);
			if (thrust_mag > prof.max_proper_acceleration && thrust_mag > 1e-12) {
				const double scale = prof.max_proper_acceleration / thrust_mag;
				for (auto& c : thrust) c *= scale;
			}

			for (size_t i = 0; i < 3; ++i) {
				cam.velocity[i] += thrust[i] * dt;
			}
		}

		if (time_running) {
			if (orchestrator_.parameters().mass > 0.0) {
				const double gm = orchestrator_.parameters().mass;
				const double r2 = cam.position[0] * cam.position[0] + cam.position[1] * cam.position[1] + cam.position[2] * cam.position[2];
				const double r = std::sqrt(std::max(r2, 1e-4));
				const double a_grav = gm / r2;
				cam.velocity[0] -= a_grav * (cam.position[0] / r) * dt;
				cam.velocity[1] -= a_grav * (cam.position[1] / r) * dt;
				cam.velocity[2] -= a_grav * (cam.position[2] / r) * dt;
			}
			cam.position[0] += cam.velocity[0] * dt;
			cam.position[1] += cam.velocity[1] * dt;
			cam.position[2] += cam.velocity[2] * dt;
			sync_spherical_from_cartesian();
		}

		if (config_.keybinds.is_pressed(InputAction::RollLeft, window)) {
			cam.roll -= prof.angular_rate_deg_s * dt;
		}
		if (config_.keybinds.is_pressed(InputAction::RollRight, window)) {
			cam.roll += prof.angular_rate_deg_s * dt;
		}

		handle_mouse_look(window, is_hovered);
	}

	void update_orbit_mode(GLFWwindow* window, double dt, bool is_hovered) noexcept {
		auto& cam = orchestrator_.camera();
		const auto& prof = config_.orbit;

		if (config_.keybinds.is_pressed(InputAction::MoveForward, window)) {
			cam.orbit_distance = std::max(2.0, cam.orbit_distance - prof.orbit_distance_speed * dt);
		}
		if (config_.keybinds.is_pressed(InputAction::MoveBackward, window)) {
			cam.orbit_distance = std::min(5000.0, cam.orbit_distance + prof.orbit_distance_speed * dt);
		}
		if (config_.keybinds.is_pressed(InputAction::MoveLeft, window)) {
			cam.yaw -= prof.yaw_speed_deg_s * dt;
		}
		if (config_.keybinds.is_pressed(InputAction::MoveRight, window)) {
			cam.yaw += prof.yaw_speed_deg_s * dt;
		}

		const double pitch_sign = prof.invert_pitch ? -1.0 : 1.0;
		if (config_.keybinds.is_pressed(InputAction::MoveUp, window)) {
			cam.pitch = std::clamp(cam.pitch - prof.pitch_speed_deg_s * dt * pitch_sign, -89.0, 89.0);
		}
		if (config_.keybinds.is_pressed(InputAction::MoveDown, window)) {
			cam.pitch = std::clamp(cam.pitch + prof.pitch_speed_deg_s * dt * pitch_sign, -89.0, 89.0);
		}

		handle_mouse_look(window, is_hovered);

		const double p_rad = cam.pitch * (std::numbers::pi / 180.0);
		const double y_rad = cam.yaw * (std::numbers::pi / 180.0);
		cam.position[0] = cam.target[0] - cam.orbit_distance * std::cos(p_rad) * std::cos(y_rad);
		cam.position[1] = cam.target[1] - cam.orbit_distance * std::cos(p_rad) * std::sin(y_rad);
		cam.position[2] = cam.target[2] - cam.orbit_distance * std::sin(p_rad);

		sync_spherical_from_cartesian();
	}

	void update_spherical_mode(GLFWwindow* window, double dt, double boost_multiplier, bool is_hovered) noexcept {
		auto& cam = orchestrator_.camera();
		const double speed = config_.free_fly.forward_speed * boost_multiplier;

		if (config_.keybinds.is_pressed(InputAction::MoveForward, window)) {
			cam.radius = std::max(2.5, cam.radius - speed * dt);
		}
		if (config_.keybinds.is_pressed(InputAction::MoveBackward, window)) {
			cam.radius = std::min(5000.0, cam.radius + speed * dt);
		}
		if (config_.keybinds.is_pressed(InputAction::MoveLeft, window)) {
			cam.phi -= (speed / std::max(cam.radius, 1e-3)) * dt;
		}
		if (config_.keybinds.is_pressed(InputAction::MoveRight, window)) {
			cam.phi += (speed / std::max(cam.radius, 1e-3)) * dt;
		}
		if (config_.keybinds.is_pressed(InputAction::MoveUp, window)) {
			cam.theta = std::clamp(cam.theta - (speed / std::max(cam.radius, 1e-3)) * dt, 0.01, std::numbers::pi - 0.01);
		}
		if (config_.keybinds.is_pressed(InputAction::MoveDown, window)) {
			cam.theta = std::clamp(cam.theta + (speed / std::max(cam.radius, 1e-3)) * dt, 0.01, std::numbers::pi - 0.01);
		}

		cam.position[0] = cam.radius * std::sin(cam.theta) * std::cos(cam.phi);
		cam.position[1] = cam.radius * std::sin(cam.theta) * std::sin(cam.phi);
		cam.position[2] = cam.radius * std::cos(cam.theta);

		look_at_origin();
		handle_mouse_look(window, is_hovered);
	}

	void handle_mouse_look(GLFWwindow* window, bool is_hovered) noexcept {
		look_delta_yaw_deg_ = 0.0;
		look_delta_pitch_deg_ = 0.0;
		if (window == nullptr) {
			is_dragging_ = false;
			drag_threshold_exceeded_ = false;
			mouse_delta_x_ = 0.0;
			mouse_delta_y_ = 0.0;
			return;
		}
		double mx = 0.0, my = 0.0;
		glfwGetCursorPos(window, &mx, &my);

		const bool right_down = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);
		const bool left_down = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);

		if ((right_down || left_down) && (is_hovered || is_dragging_)) {
			if (!is_dragging_) {
				is_dragging_ = true;
				drag_threshold_exceeded_ = false;
				last_mouse_x_ = mx;
				last_mouse_y_ = my;
				press_mouse_x_ = mx;
				press_mouse_y_ = my;
			} else {
				const double dx = mx - last_mouse_x_;
				const double dy = my - last_mouse_y_;
				last_mouse_x_ = mx;
				last_mouse_y_ = my;

				const double total_dx = mx - press_mouse_x_;
				const double total_dy = my - press_mouse_y_;
				if (!drag_threshold_exceeded_ && std::sqrt(total_dx * total_dx + total_dy * total_dy) > kClickDragThresholdPixels) {
					drag_threshold_exceeded_ = true;
				}

				mouse_delta_x_ = dx;
				mouse_delta_y_ = dy;

				if (drag_threshold_exceeded_) {
					auto& cam = orchestrator_.camera();
					const auto& prof = config_.free_fly;
					const double x_sign = prof.invert_mouse_x ? -1.0 : 1.0;
					const double y_sign = prof.invert_mouse_y ? -1.0 : 1.0;
					look_delta_yaw_deg_ = -dx * prof.mouse_sensitivity * x_sign;
					look_delta_pitch_deg_ = -dy * prof.mouse_sensitivity * y_sign;
					if (navigation_mode_ != CameraNavigationMode::SurfaceWalk && navigation_mode_ != CameraNavigationMode::PlanetOrbit) {
						cam.yaw += look_delta_yaw_deg_;
						cam.pitch = std::clamp(cam.pitch + look_delta_pitch_deg_, -89.0, 89.0);
					}
				}
			}
		} else {
			is_dragging_ = false;
			drag_threshold_exceeded_ = false;
		}
	}

	void release_planet_orbit_state() noexcept {
		planet_orbit_state_ = Observer::PlanetOrbitState{};
		auto& cam = orchestrator_.camera();
		cam.roll = 0.0;
		cam.pitch = std::clamp(cam.pitch, -89.0, 89.0);
		cam.velocity = {0.0, 0.0, 0.0};
		orchestrator_.notify_state_changed();
	}

	[[nodiscard]] std::optional<Observer::PlanetOrbitEnvironment> resolve_planet_orbit_environment(uint32_t body_id) const noexcept {
		if (body_id == 0U) {
			return std::nullopt;
		}
		auto& system = orchestrator_.nbody_system();
		std::lock_guard<std::recursive_mutex> lock(system.bodies_mutex());
		for (const auto& body : system.bodies()) {
			if (body.id != body_id || !body.enabled || body.is_spacetime_source) {
				continue;
			}
			Observer::PlanetOrbitEnvironment environment;
			environment.center = body.position;
			environment.axes = Observer::SurfaceGeometry::body_semi_axes(body);
			return environment;
		}
		return std::nullopt;
	}

	[[nodiscard]] uint32_t nearest_orbitable_body_id(const std::array<double, 3>& position) const noexcept {
		auto& system = orchestrator_.nbody_system();
		std::lock_guard<std::recursive_mutex> lock(system.bodies_mutex());
		uint32_t best_id = 0U;
		double best_distance = std::numeric_limits<double>::max();
		for (const auto& body : system.bodies()) {
			if (!body.enabled || body.is_spacetime_source) {
				continue;
			}
			const double dx = body.position[0] - position[0];
			const double dy = body.position[1] - position[1];
			const double dz = body.position[2] - position[2];
			const double distance = dx * dx + dy * dy + dz * dz;
			if (distance < best_distance) {
				best_distance = distance;
				best_id = body.id;
			}
		}
		return best_id;
	}

	void update_planet_orbit_mode(GLFWwindow* window, double dt, bool is_hovered) noexcept {
		auto& params = orchestrator_.parameters();
		auto& cam = orchestrator_.camera();
		auto& profile = config_.planet_orbit;
		profile.sanitize();

		auto environment = resolve_planet_orbit_environment(params.surface_walk_body_id);
		if (!environment.has_value()) {
			const uint32_t fallback_id = nearest_orbitable_body_id(cam.position);
			if (fallback_id != 0U) {
				params.surface_walk_body_id = fallback_id;
				planet_orbit_state_.active = false;
				environment = resolve_planet_orbit_environment(fallback_id);
			}
		}
		if (!environment.has_value()) {
			Core::log_warning("Planet orbit requires at least one enabled celestial body; returning to free fly navigation.");
			set_navigation_mode(CameraNavigationMode::FreeFly6DOF);
			return;
		}

		if (!planet_orbit_state_.active || planet_orbit_state_.body_id != params.surface_walk_body_id) {
			Observer::PlanetOrbit::initialize(planet_orbit_state_, *environment, profile, params.surface_walk_body_id, cam.position);
		}

		Observer::PlanetOrbitInput input;
		if (window != nullptr) {
			const auto& keybinds = config_.keybinds;
			const auto key_down = [window](int key) noexcept { return glfwGetKey(window, key) == GLFW_PRESS; };
			if (keybinds.is_pressed(InputAction::MoveRight, window) || key_down(GLFW_KEY_RIGHT)) input.orbit_east += 1.0;
			if (keybinds.is_pressed(InputAction::MoveLeft, window) || key_down(GLFW_KEY_LEFT)) input.orbit_east -= 1.0;
			if (keybinds.is_pressed(InputAction::MoveForward, window) || key_down(GLFW_KEY_UP)) input.orbit_north += 1.0;
			if (keybinds.is_pressed(InputAction::MoveBackward, window) || key_down(GLFW_KEY_DOWN)) input.orbit_north -= 1.0;
			if (key_down(GLFW_KEY_PAGE_UP)) input.zoom -= 1.0;
			if (key_down(GLFW_KEY_PAGE_DOWN)) input.zoom += 1.0;
			input.sprint = keybinds.is_active(InputAction::Sprint, window) || key_down(GLFW_KEY_LEFT_SHIFT) || key_down(GLFW_KEY_RIGHT_SHIFT);
			input.crawl = keybinds.is_active(InputAction::Crawl, window) || key_down(GLFW_KEY_LEFT_CONTROL) || key_down(GLFW_KEY_RIGHT_CONTROL);

			handle_mouse_look(window, is_hovered);
			if (is_dragging_ && drag_threshold_exceeded_) {
				const double direction = profile.invert_drag ? -1.0 : 1.0;
				input.drag_east_pixels = -mouse_delta_x_ * direction;
				input.drag_north_pixels = mouse_delta_y_ * direction;
			}
			input.zoom_steps = pending_scroll_steps_;
			pending_scroll_steps_ = 0.0;
		}

		Observer::PlanetOrbit::advance(planet_orbit_state_, profile, input, dt);
		const Observer::PlanetOrbitPose pose = Observer::PlanetOrbit::evaluate(planet_orbit_state_, *environment, profile);
		if (!std::isfinite(pose.position[0]) || !std::isfinite(pose.position[1]) || !std::isfinite(pose.position[2])) {
			planet_orbit_state_.active = false;
			return;
		}

		Observer::SurfaceGeometry::FrameAngles orbit_angles = pose.angles;
		orbit_angles.pitch_deg = std::clamp(orbit_angles.pitch_deg, -89.0, 89.0);
		apply_follow_pose(pose.position, orbit_angles, environment->center, Observer::PlanetOrbit::mean_radius(environment->axes));
	}

	void sync_spherical_from_cartesian() noexcept {
		orchestrator_.camera().synchronize_spherical();
	}
};

}

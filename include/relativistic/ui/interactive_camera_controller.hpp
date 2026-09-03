#pragma once

#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/ui/camera_control_config.hpp"
#include <GLFW/glfw3.h>
#include <cmath>
#include <numbers>
#include <array>
#include <algorithm>

namespace Relativistic::UI {

enum class CameraNavigationMode : uint32_t {
	FreeFly6DOF = 0,
	OrbitCenter = 1,
	SphericalBoyerLindquist = 2,
	RocketThrust = 3
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

	void snap_to_equatorial_front(double distance = 50.0) noexcept {
		auto& cam = orchestrator_.camera();
		cam.position = {0.0, distance, 0.0};
		cam.velocity = {0.0, 0.0, 0.0};
		cam.pitch = 0.0;
		cam.yaw = 180.0;
		cam.roll = 0.0;
		cam.orbit_distance = distance;
		sync_spherical_from_cartesian();
	}

	void snap_to_equatorial_side(double distance = 50.0) noexcept {
		auto& cam = orchestrator_.camera();
		cam.position = {distance, 0.0, 0.0};
		cam.velocity = {0.0, 0.0, 0.0};
		cam.pitch = 0.0;
		cam.yaw = -90.0;
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
			cam.yaw = std::atan2(dx, -dy) * (180.0 / std::numbers::pi);
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

	void update(GLFWwindow* window, double dt, bool is_hovered) noexcept {
		if (window == nullptr || dt <= 0.0) return;

		handle_global_shortcuts(window);

		navigation_mode_ = static_cast<CameraNavigationMode>(orchestrator_.parameters().camera_mode);

		double boost_multiplier = 1.0;
		if (config_.keybinds.is_pressed(CameraAction::Sprint, window)) {
			boost_multiplier = config_.free_fly.sprint_multiplier;
		} else if (config_.keybinds.is_pressed(CameraAction::Crawl, window)) {
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
			case CameraNavigationMode::FreeFly6DOF:
			default:
				update_free_fly_mode(window, dt, boost_multiplier, is_hovered);
				break;
		}
	}

	void handle_scroll(double yoffset) noexcept {
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
	void handle_global_shortcuts(GLFWwindow* window) noexcept {
		if (config_.keybinds.is_pressed(CameraAction::ResetRoll, window)) {
			reset_roll();
		}
		if (config_.keybinds.is_pressed(CameraAction::LookAtOrigin, window)) {
			look_at_origin();
		}
		if (config_.keybinds.is_pressed(CameraAction::SpeedDecrease, window)) {
			set_uniform_speed(config_.free_fly.forward_speed * 0.95);
		}
		if (config_.keybinds.is_pressed(CameraAction::SpeedIncrease, window)) {
			set_uniform_speed(config_.free_fly.forward_speed * 1.05);
		}
		if (glfwGetKey(window, GLFW_KEY_KP_1) == GLFW_PRESS || (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS)) {
			snap_to_equatorial_front(50.0);
		}
		if (glfwGetKey(window, GLFW_KEY_KP_3) == GLFW_PRESS || (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS)) {
			snap_to_equatorial_side(50.0);
		}
		if (glfwGetKey(window, GLFW_KEY_KP_7) == GLFW_PRESS || (glfwGetKey(window, GLFW_KEY_7) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS)) {
			snap_to_north_pole(50.0);
		}
		if (glfwGetKey(window, GLFW_KEY_KP_9) == GLFW_PRESS || (glfwGetKey(window, GLFW_KEY_9) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS)) {
			snap_to_south_pole(50.0);
		}
		if (glfwGetKey(window, GLFW_KEY_KP_5) == GLFW_PRESS || (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS)) {
			snap_to_isco();
		}
	}

	void update_free_fly_mode(GLFWwindow* window, double dt, double boost_multiplier, bool is_hovered) noexcept {
		auto& cam = orchestrator_.camera();
		const auto& prof = config_.free_fly;
		const double pitch_rad = cam.pitch * (std::numbers::pi / 180.0);
		const double yaw_rad = cam.yaw * (std::numbers::pi / 180.0);

		const double cos_p = std::cos(pitch_rad);
		const double sin_p = std::sin(pitch_rad);
		const double cos_y = std::cos(yaw_rad);
		const double sin_y = std::sin(yaw_rad);

		const std::array<double, 3> forward = {cos_p * sin_y, cos_p * cos_y, sin_p};
		const std::array<double, 3> right = {cos_y, -sin_y, 0.0};
		const std::array<double, 3> up = {0.0, 0.0, 1.0};

		const double vert_sign = prof.invert_vertical ? -1.0 : 1.0;
		const double lat_sign = prof.invert_lateral ? -1.0 : 1.0;
		const double fwd_sign = prof.invert_forward ? -1.0 : 1.0;

		std::array<double, 3> accum{0.0, 0.0, 0.0};

		if (config_.keybinds.is_pressed(CameraAction::MoveForward, window)) {
			for (size_t i = 0; i < 3; ++i) accum[i] += forward[i] * prof.forward_speed * fwd_sign;
		}
		if (config_.keybinds.is_pressed(CameraAction::MoveBackward, window)) {
			for (size_t i = 0; i < 3; ++i) accum[i] -= forward[i] * prof.forward_speed * fwd_sign;
		}
		if (config_.keybinds.is_pressed(CameraAction::MoveRight, window)) {
			for (size_t i = 0; i < 3; ++i) accum[i] += right[i] * prof.lateral_speed * lat_sign;
		}
		if (config_.keybinds.is_pressed(CameraAction::MoveLeft, window)) {
			for (size_t i = 0; i < 3; ++i) accum[i] -= right[i] * prof.lateral_speed * lat_sign;
		}
		if (config_.keybinds.is_pressed(CameraAction::MoveUp, window)) {
			for (size_t i = 0; i < 3; ++i) accum[i] += up[i] * prof.vertical_speed * vert_sign;
		}
		if (config_.keybinds.is_pressed(CameraAction::MoveDown, window)) {
			for (size_t i = 0; i < 3; ++i) accum[i] -= up[i] * prof.vertical_speed * vert_sign;
		}

		if (config_.keybinds.is_pressed(CameraAction::RollLeft, window)) {
			cam.roll -= prof.roll_speed_deg_s * dt;
		}
		if (config_.keybinds.is_pressed(CameraAction::RollRight, window)) {
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

		const std::array<double, 3> forward = {cos_p * sin_y, cos_p * cos_y, sin_p};
		const std::array<double, 3> right = {cos_y, -sin_y, 0.0};
		const std::array<double, 3> up = {0.0, 0.0, 1.0};

		if (can_thrust) {
			const double lat_sign = prof.invert_lateral ? -1.0 : 1.0;
			const double vert_sign = prof.invert_vertical ? -1.0 : 1.0;
			std::array<double, 3> thrust{0.0, 0.0, 0.0};

			if (config_.keybinds.is_pressed(CameraAction::MoveForward, window)) {
				for (size_t i = 0; i < 3; ++i) thrust[i] += forward[i] * prof.main_thrust_accel;
			}
			if (config_.keybinds.is_pressed(CameraAction::MoveBackward, window)) {
				for (size_t i = 0; i < 3; ++i) thrust[i] -= forward[i] * prof.main_thrust_accel;
			}
			if (config_.keybinds.is_pressed(CameraAction::MoveRight, window)) {
				for (size_t i = 0; i < 3; ++i) thrust[i] += right[i] * prof.lateral_thrust_accel * lat_sign;
			}
			if (config_.keybinds.is_pressed(CameraAction::MoveLeft, window)) {
				for (size_t i = 0; i < 3; ++i) thrust[i] -= right[i] * prof.lateral_thrust_accel * lat_sign;
			}
			if (config_.keybinds.is_pressed(CameraAction::MoveUp, window)) {
				for (size_t i = 0; i < 3; ++i) thrust[i] += up[i] * prof.vertical_thrust_accel * vert_sign;
			}
			if (config_.keybinds.is_pressed(CameraAction::MoveDown, window)) {
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
			cam.position[0] += cam.velocity[0] * dt;
			cam.position[1] += cam.velocity[1] * dt;
			cam.position[2] += cam.velocity[2] * dt;
			sync_spherical_from_cartesian();
		}

		if (config_.keybinds.is_pressed(CameraAction::RollLeft, window)) {
			cam.roll -= prof.angular_rate_deg_s * dt;
		}
		if (config_.keybinds.is_pressed(CameraAction::RollRight, window)) {
			cam.roll += prof.angular_rate_deg_s * dt;
		}

		handle_mouse_look(window, is_hovered);
	}

	void update_orbit_mode(GLFWwindow* window, double dt, bool is_hovered) noexcept {
		auto& cam = orchestrator_.camera();
		const auto& prof = config_.orbit;

		if (config_.keybinds.is_pressed(CameraAction::MoveForward, window)) {
			cam.orbit_distance = std::max(2.0, cam.orbit_distance - prof.orbit_distance_speed * dt);
		}
		if (config_.keybinds.is_pressed(CameraAction::MoveBackward, window)) {
			cam.orbit_distance = std::min(5000.0, cam.orbit_distance + prof.orbit_distance_speed * dt);
		}
		if (config_.keybinds.is_pressed(CameraAction::MoveLeft, window)) {
			cam.yaw += prof.yaw_speed_deg_s * dt;
		}
		if (config_.keybinds.is_pressed(CameraAction::MoveRight, window)) {
			cam.yaw -= prof.yaw_speed_deg_s * dt;
		}

		const double pitch_sign = prof.invert_pitch ? -1.0 : 1.0;
		if (config_.keybinds.is_pressed(CameraAction::MoveUp, window)) {
			cam.pitch = std::clamp(cam.pitch + prof.pitch_speed_deg_s * dt * pitch_sign, -89.0, 89.0);
		}
		if (config_.keybinds.is_pressed(CameraAction::MoveDown, window)) {
			cam.pitch = std::clamp(cam.pitch - prof.pitch_speed_deg_s * dt * pitch_sign, -89.0, 89.0);
		}

		handle_mouse_look(window, is_hovered);

		const double p_rad = cam.pitch * (std::numbers::pi / 180.0);
		const double y_rad = cam.yaw * (std::numbers::pi / 180.0);
		cam.position[0] = cam.target[0] - cam.orbit_distance * std::cos(p_rad) * std::sin(y_rad);
		cam.position[1] = cam.target[1] - cam.orbit_distance * std::cos(p_rad) * std::cos(y_rad);
		cam.position[2] = cam.target[2] + cam.orbit_distance * std::sin(p_rad);

		sync_spherical_from_cartesian();
	}

	void update_spherical_mode(GLFWwindow* window, double dt, double boost_multiplier, bool is_hovered) noexcept {
		auto& cam = orchestrator_.camera();
		const double speed = config_.free_fly.forward_speed * boost_multiplier;

		if (config_.keybinds.is_pressed(CameraAction::MoveForward, window)) {
			cam.radius = std::max(2.5, cam.radius - speed * dt);
		}
		if (config_.keybinds.is_pressed(CameraAction::MoveBackward, window)) {
			cam.radius = std::min(5000.0, cam.radius + speed * dt);
		}
		if (config_.keybinds.is_pressed(CameraAction::MoveLeft, window)) {
			cam.phi -= (speed / std::max(cam.radius, 1e-3)) * dt;
		}
		if (config_.keybinds.is_pressed(CameraAction::MoveRight, window)) {
			cam.phi += (speed / std::max(cam.radius, 1e-3)) * dt;
		}
		if (config_.keybinds.is_pressed(CameraAction::MoveUp, window)) {
			cam.theta = std::clamp(cam.theta - (speed / std::max(cam.radius, 1e-3)) * dt, 0.01, std::numbers::pi - 0.01);
		}
		if (config_.keybinds.is_pressed(CameraAction::MoveDown, window)) {
			cam.theta = std::clamp(cam.theta + (speed / std::max(cam.radius, 1e-3)) * dt, 0.01, std::numbers::pi - 0.01);
		}

		cam.position[0] = cam.radius * std::sin(cam.theta) * std::cos(cam.phi);
		cam.position[1] = cam.radius * std::sin(cam.theta) * std::sin(cam.phi);
		cam.position[2] = cam.radius * std::cos(cam.theta);

		look_at_origin();
		handle_mouse_look(window, is_hovered);
	}

	void handle_mouse_look(GLFWwindow* window, bool is_hovered) noexcept {
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

				if (drag_threshold_exceeded_) {
					auto& cam = orchestrator_.camera();
					const auto& prof = config_.free_fly;
					const double x_sign = prof.invert_mouse_x ? -1.0 : 1.0;
					const double y_sign = prof.invert_mouse_y ? -1.0 : 1.0;
					cam.yaw += dx * prof.mouse_sensitivity * x_sign;
					cam.pitch = std::clamp(cam.pitch - dy * prof.mouse_sensitivity * y_sign, -89.0, 89.0);
				}
			}
		} else {
			is_dragging_ = false;
			drag_threshold_exceeded_ = false;
		}
	}

	void sync_spherical_from_cartesian() noexcept {
		auto& cam = orchestrator_.camera();
		const double x = cam.position[0];
		const double y = cam.position[1];
		const double z = cam.position[2];
		const double r = std::sqrt(x * x + y * y + z * z);
		cam.radius = std::max(r, 1e-4);
		cam.theta = (r > 0.0) ? std::acos(std::clamp(z / r, -1.0, 1.0)) : (std::numbers::pi / 2.0);
		cam.phi = std::atan2(y, x);
	}
};

}

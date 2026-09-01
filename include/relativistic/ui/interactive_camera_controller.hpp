#pragma once

#include "relativistic/orchestrator/simulation_orchestrator.hpp"
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
	CockpitFlight = 3
};

class InteractiveCameraController {
private:
	Orchestrator::SimulationOrchestrator<1024>& orchestrator_;
	CameraNavigationMode navigation_mode_{CameraNavigationMode::FreeFly6DOF};
	bool is_dragging_{false};
	bool cursor_captured_{false};
	double last_mouse_x_{0.0};
	double last_mouse_y_{0.0};
	double mouse_sensitivity_{0.15};
	double move_speed_{10.0};
	double boost_multiplier_{4.0};
	double crawl_multiplier_{0.2};
	bool invert_y_{false};
	bool smooth_interpolation_{true};

public:
	explicit InteractiveCameraController(Orchestrator::SimulationOrchestrator<1024>& orchestrator) noexcept
		: orchestrator_(orchestrator) {}

	void set_navigation_mode(CameraNavigationMode mode) noexcept {
		navigation_mode_ = mode;
		orchestrator_.parameters().camera_mode = static_cast<uint32_t>(mode);
	}

	[[nodiscard]] CameraNavigationMode navigation_mode() const noexcept {
		return navigation_mode_;
	}

	void set_mouse_sensitivity(double sens) noexcept {
		mouse_sensitivity_ = std::clamp(sens, 0.01, 2.0);
	}

	[[nodiscard]] double mouse_sensitivity() const noexcept {
		return mouse_sensitivity_;
	}

	void set_move_speed(double speed) noexcept {
		move_speed_ = std::max(speed, 0.01);
		orchestrator_.camera().speed = move_speed_;
	}

	[[nodiscard]] double move_speed() const noexcept {
		return move_speed_;
	}

	void set_invert_y(bool inv) noexcept {
		invert_y_ = inv;
	}

	[[nodiscard]] bool invert_y() const noexcept {
		return invert_y_;
	}

	void set_smooth_interpolation(bool smooth) noexcept {
		smooth_interpolation_ = smooth;
	}

	[[nodiscard]] bool smooth_interpolation() const noexcept {
		return smooth_interpolation_;
	}

	void snap_to_equatorial_front(double distance = 50.0) noexcept {
		auto& cam = orchestrator_.camera();
		cam.position = {0.0, distance, 0.0};
		cam.pitch = 0.0;
		cam.yaw = 0.0;
		cam.roll = 0.0;
		cam.orbit_distance = distance;
		sync_spherical_from_cartesian();
	}

	void snap_to_equatorial_side(double distance = 50.0) noexcept {
		auto& cam = orchestrator_.camera();
		cam.position = {distance, 0.0, 0.0};
		cam.pitch = 0.0;
		cam.yaw = -90.0;
		cam.roll = 0.0;
		cam.orbit_distance = distance;
		sync_spherical_from_cartesian();
	}

	void snap_to_north_pole(double distance = 50.0) noexcept {
		auto& cam = orchestrator_.camera();
		cam.position = {0.0, 0.001, distance};
		cam.pitch = -89.0;
		cam.yaw = 0.0;
		cam.roll = 0.0;
		cam.orbit_distance = distance;
		sync_spherical_from_cartesian();
	}

	void snap_to_south_pole(double distance = 50.0) noexcept {
		auto& cam = orchestrator_.camera();
		cam.position = {0.0, 0.001, -distance};
		cam.pitch = 89.0;
		cam.yaw = 0.0;
		cam.roll = 0.0;
		cam.orbit_distance = distance;
		sync_spherical_from_cartesian();
	}

	void snap_to_isco(double margin_factor = 1.2) noexcept {
		const double mass = orchestrator_.parameters().mass;
		const double isco_r = 6.0 * mass * margin_factor;
		snap_to_equatorial_front(isco_r);
	}

	void snap_to_photon_sphere(double margin_factor = 1.1) noexcept {
		const double mass = orchestrator_.parameters().mass;
		const double photon_r = 3.0 * mass * margin_factor;
		snap_to_equatorial_front(photon_r);
	}

	void look_at_target(const std::array<double, 3>& target_pos) noexcept {
		auto& cam = orchestrator_.camera();
		const double dx = target_pos[0] - cam.position[0];
		const double dy = target_pos[1] - cam.position[1];
		const double dz = target_pos[2] - cam.position[2];
		const double d_xy = std::sqrt(dx * dx + dy * dy);
		const double d_tot = std::sqrt(d_xy * d_xy + dz * dz);
		if (d_tot > 1e-6) {
			cam.yaw = std::atan2(dx, dy) * (180.0 / std::numbers::pi);
			cam.pitch = std::atan2(dz, d_xy) * (180.0 / std::numbers::pi);
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
		if (!window || dt <= 0.0) return;

		handle_global_shortcuts(window);

		navigation_mode_ = static_cast<CameraNavigationMode>(orchestrator_.parameters().camera_mode);

		double current_speed = move_speed_;
		if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS) {
			current_speed *= boost_multiplier_;
		} else if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS) {
			current_speed *= crawl_multiplier_;
		}

		switch (navigation_mode_) {
			case CameraNavigationMode::OrbitCenter:
				update_orbit_mode(window, dt, current_speed, is_hovered);
				break;
			case CameraNavigationMode::SphericalBoyerLindquist:
				update_spherical_mode(window, dt, current_speed, is_hovered);
				break;
			case CameraNavigationMode::CockpitFlight:
				update_cockpit_flight_mode(window, dt, current_speed, is_hovered);
				break;
			case CameraNavigationMode::FreeFly6DOF:
			default:
				update_free_fly_mode(window, dt, current_speed, is_hovered);
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
	}

private:
	void handle_global_shortcuts(GLFWwindow* window) noexcept {
		if (glfwGetKey(window, GLFW_KEY_HOME) == GLFW_PRESS) {
			reset_roll();
		}
		if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
			look_at_origin();
		}
		if (glfwGetKey(window, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS) {
			set_move_speed(move_speed_ * 0.95);
		}
		if (glfwGetKey(window, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS) {
			set_move_speed(move_speed_ * 1.05);
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

	void update_free_fly_mode(GLFWwindow* window, double dt, double speed, bool is_hovered) noexcept {
		auto& cam = orchestrator_.camera();
		const double pitch_rad = cam.pitch * (std::numbers::pi / 180.0);
		const double yaw_rad = cam.yaw * (std::numbers::pi / 180.0);

		const double cos_p = std::cos(pitch_rad);
		const double sin_p = std::sin(pitch_rad);
		const double cos_y = std::cos(yaw_rad);
		const double sin_y = std::sin(yaw_rad);

		const std::array<double, 3> forward = {cos_p * sin_y, cos_p * cos_y, sin_p};
		const std::array<double, 3> right = {cos_y, -sin_y, 0.0};
		const std::array<double, 3> up = {0.0, 0.0, 1.0};

		std::array<double, 3> move_dir = {0.0, 0.0, 0.0};

		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
			move_dir[0] += forward[0]; move_dir[1] += forward[1]; move_dir[2] += forward[2];
		}
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
			move_dir[0] -= forward[0]; move_dir[1] -= forward[1]; move_dir[2] -= forward[2];
		}
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
			move_dir[0] += right[0]; move_dir[1] += right[1]; move_dir[2] += right[2];
		}
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
			move_dir[0] -= right[0]; move_dir[1] -= right[1]; move_dir[2] -= right[2];
		}
		if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
			move_dir[0] += up[0]; move_dir[1] += up[1]; move_dir[2] += up[2];
		}
		if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
			move_dir[0] -= up[0]; move_dir[1] -= up[1]; move_dir[2] -= up[2];
		}

		if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_PAGE_UP) == GLFW_PRESS) {
			cam.roll -= 45.0 * dt;
		}
		if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS) {
			cam.roll += 45.0 * dt;
		}

		const double move_len = std::sqrt(move_dir[0] * move_dir[0] + move_dir[1] * move_dir[1] + move_dir[2] * move_dir[2]);
		if (move_len > 0.0) {
			const double factor = (speed * dt) / move_len;
			cam.position[0] += move_dir[0] * factor;
			cam.position[1] += move_dir[1] * factor;
			cam.position[2] += move_dir[2] * factor;
			sync_spherical_from_cartesian();
		}

		handle_mouse_look(window, is_hovered);
	}

	void update_orbit_mode(GLFWwindow* window, double dt, double speed, bool is_hovered) noexcept {
		auto& cam = orchestrator_.camera();

		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
			cam.orbit_distance = std::max(2.0, cam.orbit_distance - speed * dt);
		}
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
			cam.orbit_distance = std::min(500.0, cam.orbit_distance + speed * dt);
		}
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
			cam.yaw += 40.0 * dt;
		}
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
			cam.yaw -= 40.0 * dt;
		}

		handle_mouse_look(window, is_hovered);

		const double p_rad = cam.pitch * (std::numbers::pi / 180.0);
		const double y_rad = cam.yaw * (std::numbers::pi / 180.0);
		cam.position[0] = cam.target[0] - cam.orbit_distance * std::cos(p_rad) * std::sin(y_rad);
		cam.position[1] = cam.target[1] - cam.orbit_distance * std::cos(p_rad) * std::cos(y_rad);
		cam.position[2] = cam.target[2] + cam.orbit_distance * std::sin(p_rad);

		sync_spherical_from_cartesian();
	}

	void update_spherical_mode(GLFWwindow* window, double dt, double speed, bool is_hovered) noexcept {
		auto& cam = orchestrator_.camera();

		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
			cam.radius = std::max(2.5, cam.radius - speed * dt);
		}
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
			cam.radius = std::min(500.0, cam.radius + speed * dt);
		}
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
			cam.phi -= (speed / cam.radius) * dt;
		}
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
			cam.phi += (speed / cam.radius) * dt;
		}
		if (glfwGetKey(window, GLFW_KEY_PAGE_UP) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
			cam.theta = std::clamp(cam.theta - (speed / cam.radius) * dt, 0.01, std::numbers::pi - 0.01);
		}
		if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
			cam.theta = std::clamp(cam.theta + (speed / cam.radius) * dt, 0.01, std::numbers::pi - 0.01);
		}

		cam.position[0] = cam.radius * std::sin(cam.theta) * std::cos(cam.phi);
		cam.position[1] = cam.radius * std::sin(cam.theta) * std::sin(cam.phi);
		cam.position[2] = cam.radius * std::cos(cam.theta);

		look_at_origin();
		handle_mouse_look(window, is_hovered);
	}

	void update_cockpit_flight_mode(GLFWwindow* window, double dt, double speed, bool is_hovered) noexcept {
		auto& cam = orchestrator_.camera();
		double throttle = 0.0;
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) throttle += 1.0;
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) throttle -= 1.0;

		const double pitch_rad = cam.pitch * (std::numbers::pi / 180.0);
		const double yaw_rad = cam.yaw * (std::numbers::pi / 180.0);

		const double cos_p = std::cos(pitch_rad);
		const double sin_p = std::sin(pitch_rad);
		const double cos_y = std::cos(yaw_rad);
		const double sin_y = std::sin(yaw_rad);

		const std::array<double, 3> forward = {cos_p * sin_y, cos_p * cos_y, sin_p};
		cam.position[0] += forward[0] * throttle * speed * dt;
		cam.position[1] += forward[1] * throttle * speed * dt;
		cam.position[2] += forward[2] * throttle * speed * dt;

		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) cam.yaw += 40.0 * dt;
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) cam.yaw -= 40.0 * dt;
		if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) cam.pitch = std::clamp(cam.pitch + 40.0 * dt, -89.0, 89.0);
		if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) cam.pitch = std::clamp(cam.pitch - 40.0 * dt, -89.0, 89.0);

		sync_spherical_from_cartesian();
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
				last_mouse_x_ = mx;
				last_mouse_y_ = my;
			} else {
				const double dx = mx - last_mouse_x_;
				const double dy = my - last_mouse_y_;
				last_mouse_x_ = mx;
				last_mouse_y_ = my;

				auto& cam = orchestrator_.camera();
				cam.yaw += dx * mouse_sensitivity_;
				const double pitch_delta = (invert_y_ ? -dy : dy) * mouse_sensitivity_;
				cam.pitch = std::clamp(cam.pitch - pitch_delta, -89.0, 89.0);
			}
		} else {
			is_dragging_ = false;
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

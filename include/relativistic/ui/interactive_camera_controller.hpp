#pragma once

#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include <GLFW/glfw3.h>
#include <cmath>
#include <numbers>
#include <array>
#include <algorithm>

namespace Relativistic::UI {

class InteractiveCameraController {
private:
	Orchestrator::SimulationOrchestrator<1024>& orchestrator_;
	bool is_dragging_{false};
	bool cursor_captured_{false};
	double last_mouse_x_{0.0};
	double last_mouse_y_{0.0};
	double mouse_sensitivity_{0.15};
	double move_speed_{10.0};
	double boost_multiplier_{4.0};
	double crawl_multiplier_{0.2};

public:
	explicit InteractiveCameraController(Orchestrator::SimulationOrchestrator<1024>& orchestrator) noexcept
		: orchestrator_(orchestrator) {}

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

	void update(GLFWwindow* window, double dt) noexcept {
		if (!window || dt <= 0.0) return;

		auto& cam = orchestrator_.camera();
		double current_speed = move_speed_;

		if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS) {
			current_speed *= boost_multiplier_;
		} else if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS) {
			current_speed *= crawl_multiplier_;
		}

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
			move_dir[0] += forward[0];
			move_dir[1] += forward[1];
			move_dir[2] += forward[2];
		}
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
			move_dir[0] -= forward[0];
			move_dir[1] -= forward[1];
			move_dir[2] -= forward[2];
		}
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
			move_dir[0] += right[0];
			move_dir[1] += right[1];
			move_dir[2] += right[2];
		}
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
			move_dir[0] -= right[0];
			move_dir[1] -= right[1];
			move_dir[2] -= right[2];
		}
		if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
			move_dir[0] += up[0];
			move_dir[1] += up[1];
			move_dir[2] += up[2];
		}
		if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
			move_dir[0] -= up[0];
			move_dir[1] -= up[1];
			move_dir[2] -= up[2];
		}

		const double move_len = std::sqrt(move_dir[0] * move_dir[0] + move_dir[1] * move_dir[1] + move_dir[2] * move_dir[2]);
		if (move_len > 0.0) {
			const double factor = (current_speed * dt) / move_len;
			cam.position[0] += move_dir[0] * factor;
			cam.position[1] += move_dir[1] * factor;
			cam.position[2] += move_dir[2] * factor;

			const double r = std::sqrt(cam.position[0] * cam.position[0] + cam.position[1] * cam.position[1] + cam.position[2] * cam.position[2]);
			cam.radius = std::max(r, 1e-4);
			cam.theta = (r > 0.0) ? std::acos(std::clamp(cam.position[2] / r, -1.0, 1.0)) : (std::numbers::pi / 2.0);
			cam.phi = std::atan2(cam.position[1], cam.position[0]);
		}

		handle_mouse_look(window);
	}

	void handle_mouse_look(GLFWwindow* window) noexcept {
		double mx = 0.0, my = 0.0;
		glfwGetCursorPos(window, &mx, &my);

		const bool right_down = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);
		const bool left_down = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS);

		if (right_down || left_down || cursor_captured_) {
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
				cam.pitch = std::clamp(cam.pitch - dy * mouse_sensitivity_, -89.0, 89.0);
			}
		} else {
			is_dragging_ = false;
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
};

}

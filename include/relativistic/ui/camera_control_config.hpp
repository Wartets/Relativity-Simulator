#pragma once

#include <GLFW/glfw3.h>
#include <array>
#include <cstdint>

namespace Relativistic::UI {

enum class CameraAction : uint32_t {
	MoveForward = 0,
	MoveBackward,
	MoveLeft,
	MoveRight,
	MoveUp,
	MoveDown,
	RollLeft,
	RollRight,
	Sprint,
	Crawl,
	LookAtOrigin,
	ResetRoll,
	SpeedDecrease,
	SpeedIncrease,
	Count
};

struct KeybindEntry {
	int primary_key{GLFW_KEY_UNKNOWN};
	int secondary_key{GLFW_KEY_UNKNOWN};

	[[nodiscard]] bool matches(GLFWwindow* window) const noexcept {
		if (window == nullptr) return false;
		if (primary_key != GLFW_KEY_UNKNOWN && glfwGetKey(window, primary_key) == GLFW_PRESS) return true;
		if (secondary_key != GLFW_KEY_UNKNOWN && glfwGetKey(window, secondary_key) == GLFW_PRESS) return true;
		return false;
	}
};

class CameraKeybindMap {
private:
	std::array<KeybindEntry, static_cast<size_t>(CameraAction::Count)> bindings_{};

public:
	CameraKeybindMap() noexcept {
		set(CameraAction::MoveForward, GLFW_KEY_W, GLFW_KEY_UP);
		set(CameraAction::MoveBackward, GLFW_KEY_S, GLFW_KEY_DOWN);
		set(CameraAction::MoveLeft, GLFW_KEY_A, GLFW_KEY_LEFT);
		set(CameraAction::MoveRight, GLFW_KEY_D, GLFW_KEY_RIGHT);
		set(CameraAction::MoveUp, GLFW_KEY_SPACE, GLFW_KEY_E);
		set(CameraAction::MoveDown, GLFW_KEY_C, GLFW_KEY_LEFT_CONTROL);
		set(CameraAction::RollLeft, GLFW_KEY_J, GLFW_KEY_PAGE_UP);
		set(CameraAction::RollRight, GLFW_KEY_K, GLFW_KEY_PAGE_DOWN);
		set(CameraAction::Sprint, GLFW_KEY_LEFT_SHIFT, GLFW_KEY_RIGHT_SHIFT);
		set(CameraAction::Crawl, GLFW_KEY_LEFT_ALT, GLFW_KEY_RIGHT_ALT);
		set(CameraAction::LookAtOrigin, GLFW_KEY_F, GLFW_KEY_UNKNOWN);
		set(CameraAction::ResetRoll, GLFW_KEY_HOME, GLFW_KEY_UNKNOWN);
		set(CameraAction::SpeedDecrease, GLFW_KEY_LEFT_BRACKET, GLFW_KEY_UNKNOWN);
		set(CameraAction::SpeedIncrease, GLFW_KEY_RIGHT_BRACKET, GLFW_KEY_UNKNOWN);
	}

	void set(CameraAction action, int primary, int secondary = GLFW_KEY_UNKNOWN) noexcept {
		bindings_[static_cast<size_t>(action)] = KeybindEntry{primary, secondary};
	}

	[[nodiscard]] const KeybindEntry& get(CameraAction action) const noexcept {
		return bindings_[static_cast<size_t>(action)];
	}

	[[nodiscard]] bool is_pressed(CameraAction action, GLFWwindow* window) const noexcept {
		return bindings_[static_cast<size_t>(action)].matches(window);
	}

	[[nodiscard]] static const char* key_name(int key) noexcept {
		if (key == GLFW_KEY_UNKNOWN) return "---";
		const char* name = glfwGetKeyName(key, 0);
		if (name != nullptr) return name;
		switch (key) {
			case GLFW_KEY_SPACE: return "Space";
			case GLFW_KEY_LEFT_SHIFT: return "L-Shift";
			case GLFW_KEY_RIGHT_SHIFT: return "R-Shift";
			case GLFW_KEY_LEFT_CONTROL: return "L-Ctrl";
			case GLFW_KEY_RIGHT_CONTROL: return "R-Ctrl";
			case GLFW_KEY_LEFT_ALT: return "L-Alt";
			case GLFW_KEY_RIGHT_ALT: return "R-Alt";
			case GLFW_KEY_UP: return "Up";
			case GLFW_KEY_DOWN: return "Down";
			case GLFW_KEY_LEFT: return "Left";
			case GLFW_KEY_RIGHT: return "Right";
			case GLFW_KEY_PAGE_UP: return "PgUp";
			case GLFW_KEY_PAGE_DOWN: return "PgDn";
			case GLFW_KEY_HOME: return "Home";
			default: return "?";
		}
	}
};

struct AxisSpeedProfile {
	double forward_speed{10.0};
	double lateral_speed{10.0};
	double vertical_speed{10.0};
	double yaw_speed_deg_s{40.0};
	double pitch_speed_deg_s{40.0};
	double roll_speed_deg_s{45.0};
	double sprint_multiplier{4.0};
	double crawl_multiplier{0.2};
	double orbit_distance_speed{10.0};
	bool invert_forward{false};
	bool invert_lateral{false};
	bool invert_vertical{false};
	bool invert_pitch{false};
	bool invert_mouse_x{false};
	bool invert_mouse_y{false};
	double mouse_sensitivity{0.15};
};

struct RocketControlProfile {
	double main_thrust_accel{20.0};
	double lateral_thrust_accel{10.0};
	double vertical_thrust_accel{10.0};
	double angular_rate_deg_s{60.0};
	double max_proper_acceleration{100.0};
	bool invert_lateral{false};
	bool invert_vertical{false};
	bool requires_time_running{true};
};

struct ZoomConfig {
	bool zoom_center_on_cursor{true};
	double zoom_scroll_sensitivity{0.18};
	double min_zoom{1.0};
	double max_zoom{8.0};
};

struct CameraControlConfig {
	CameraKeybindMap keybinds{};
	AxisSpeedProfile free_fly{};
	AxisSpeedProfile orbit{};
	RocketControlProfile rocket{};
	ZoomConfig zoom{};
};

}

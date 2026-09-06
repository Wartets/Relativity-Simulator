#pragma once

#include <GLFW/glfw3.h>
#include <array>
#include <cstdint>
#include <string_view>

namespace Relativistic::UI {

enum class KeyboardLayout : uint32_t {
	Qwerty = 0,
	Azerty = 1
};

enum class InputAction : uint32_t {
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
	ZoomModifier,
	SnapEquatorialFront,
	SnapEquatorialSide,
	SnapNorthPole,
	SnapSouthPole,
	SnapIsco,
	TogglePausePlay,
	SingleStepTick,
	ResetClock,
	ToggleControlPanel,
	LayoutMultiWindow,
	LayoutDocked,
	LayoutViewportFocus,
	ToggleBodyManager,
	CycleCameraMode,
	ToggleTelemetryWindow,
	TogglePerformanceWindow,
	CaptureScreenshot,
	ToggleHudManager,
	ToggleKeybindSettings,
	Count
};

[[nodiscard]] constexpr std::string_view input_action_name(InputAction action) noexcept {
	switch (action) {
		case InputAction::MoveForward: return "Move Forward";
		case InputAction::MoveBackward: return "Move Backward";
		case InputAction::MoveLeft: return "Move Left";
		case InputAction::MoveRight: return "Move Right";
		case InputAction::MoveUp: return "Move Up";
		case InputAction::MoveDown: return "Move Down";
		case InputAction::RollLeft: return "Roll Left";
		case InputAction::RollRight: return "Roll Right";
		case InputAction::Sprint: return "Sprint";
		case InputAction::Crawl: return "Crawl";
		case InputAction::LookAtOrigin: return "Look At Origin";
		case InputAction::ResetRoll: return "Reset Roll";
		case InputAction::SpeedDecrease: return "Decrease Navigation Speed";
		case InputAction::SpeedIncrease: return "Increase Navigation Speed";
		case InputAction::ZoomModifier: return "Hold To Zoom (Scroll)";
		case InputAction::SnapEquatorialFront: return "Snap: Equatorial Front";
		case InputAction::SnapEquatorialSide: return "Snap: Equatorial Side";
		case InputAction::SnapNorthPole: return "Snap: North Pole";
		case InputAction::SnapSouthPole: return "Snap: South Pole";
		case InputAction::SnapIsco: return "Snap: ISCO Orbit";
		case InputAction::TogglePausePlay: return "Pause / Resume Simulation";
		case InputAction::SingleStepTick: return "Single Step Tick";
		case InputAction::ResetClock: return "Reset Simulation Clock";
		case InputAction::ToggleControlPanel: return "Toggle Control Panel";
		case InputAction::LayoutMultiWindow: return "Layout: Multi-Window Detached";
		case InputAction::LayoutDocked: return "Layout: Docked Workspace";
		case InputAction::LayoutViewportFocus: return "Layout: Viewport Focus";
		case InputAction::ToggleBodyManager: return "Toggle Body Manager";
		case InputAction::CycleCameraMode: return "Cycle Camera Navigation Mode";
		case InputAction::ToggleTelemetryWindow: return "Toggle Telemetry Window";
		case InputAction::TogglePerformanceWindow: return "Toggle Performance Window";
		case InputAction::CaptureScreenshot: return "Capture Screenshot";
		case InputAction::ToggleHudManager: return "Toggle HUD Manager";
		case InputAction::ToggleKeybindSettings: return "Toggle Keybind Settings";
		default: return "Unknown Action";
	}
}

struct KeyBinding {
	int primary_key{GLFW_KEY_UNKNOWN};
	int secondary_key{GLFW_KEY_UNKNOWN};

	[[nodiscard]] bool matches(GLFWwindow* window) const noexcept {
		if (window == nullptr) return false;
		if (primary_key != GLFW_KEY_UNKNOWN && glfwGetKey(window, primary_key) == GLFW_PRESS) return true;
		if (secondary_key != GLFW_KEY_UNKNOWN && glfwGetKey(window, secondary_key) == GLFW_PRESS) return true;
		return false;
	}

	[[nodiscard]] bool contains_key(int key) const noexcept {
		return key != GLFW_KEY_UNKNOWN && (key == primary_key || key == secondary_key);
	}
};

[[nodiscard]] inline const char* glfw_key_display_name(int key) noexcept {
	if (key == GLFW_KEY_UNKNOWN) return "---";
	const char* name = glfwGetKeyName(key, 0);
	if (name != nullptr) return name;
	switch (key) {
		case GLFW_KEY_SPACE: return "Space";
		case GLFW_KEY_TAB: return "Tab";
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
		case GLFW_KEY_END: return "End";
		case GLFW_KEY_DELETE: return "Del";
		case GLFW_KEY_INSERT: return "Ins";
		case GLFW_KEY_CAPS_LOCK: return "CapsLock";
		case GLFW_KEY_ESCAPE: return "Esc";
		case GLFW_KEY_F1: return "F1";
		case GLFW_KEY_F2: return "F2";
		case GLFW_KEY_F3: return "F3";
		case GLFW_KEY_F4: return "F4";
		case GLFW_KEY_F5: return "F5";
		case GLFW_KEY_F6: return "F6";
		case GLFW_KEY_F7: return "F7";
		case GLFW_KEY_F8: return "F8";
		case GLFW_KEY_F9: return "F9";
		case GLFW_KEY_F10: return "F10";
		case GLFW_KEY_F11: return "F11";
		case GLFW_KEY_F12: return "F12";
		case GLFW_KEY_KP_0: return "Num0";
		case GLFW_KEY_KP_1: return "Num1";
		case GLFW_KEY_KP_2: return "Num2";
		case GLFW_KEY_KP_3: return "Num3";
		case GLFW_KEY_KP_4: return "Num4";
		case GLFW_KEY_KP_5: return "Num5";
		case GLFW_KEY_KP_6: return "Num6";
		case GLFW_KEY_KP_7: return "Num7";
		case GLFW_KEY_KP_8: return "Num8";
		case GLFW_KEY_KP_9: return "Num9";
		case GLFW_KEY_KP_ENTER: return "NumEnter";
		default: return "?";
	}
}

class ActionKeybindMap {
private:
	std::array<KeyBinding, static_cast<size_t>(InputAction::Count)> bindings_{};
	KeyboardLayout layout_{KeyboardLayout::Qwerty};

	void apply_layout_defaults(KeyboardLayout layout) noexcept {
		layout_ = layout;
		const bool azerty = (layout == KeyboardLayout::Azerty);

		set(InputAction::MoveForward, azerty ? GLFW_KEY_Z : GLFW_KEY_W, GLFW_KEY_UP);
		set(InputAction::MoveBackward, GLFW_KEY_S, GLFW_KEY_DOWN);
		set(InputAction::MoveLeft, azerty ? GLFW_KEY_Q : GLFW_KEY_A, GLFW_KEY_LEFT);
		set(InputAction::MoveRight, GLFW_KEY_D, GLFW_KEY_RIGHT);
		set(InputAction::MoveUp, GLFW_KEY_SPACE, GLFW_KEY_E);
		set(InputAction::MoveDown, GLFW_KEY_C, GLFW_KEY_LEFT_CONTROL);
		set(InputAction::RollLeft, GLFW_KEY_J, GLFW_KEY_PAGE_UP);
		set(InputAction::RollRight, GLFW_KEY_K, GLFW_KEY_PAGE_DOWN);
		set(InputAction::Sprint, GLFW_KEY_LEFT_SHIFT, GLFW_KEY_RIGHT_SHIFT);
		set(InputAction::Crawl, GLFW_KEY_LEFT_ALT, GLFW_KEY_RIGHT_ALT);
		set(InputAction::LookAtOrigin, GLFW_KEY_F, GLFW_KEY_UNKNOWN);
		set(InputAction::ResetRoll, GLFW_KEY_HOME, GLFW_KEY_UNKNOWN);
		set(InputAction::SpeedDecrease, GLFW_KEY_LEFT_BRACKET, GLFW_KEY_UNKNOWN);
		set(InputAction::SpeedIncrease, GLFW_KEY_RIGHT_BRACKET, GLFW_KEY_UNKNOWN);
		set(InputAction::ZoomModifier, azerty ? GLFW_KEY_W : GLFW_KEY_Z, GLFW_KEY_UNKNOWN);
		set(InputAction::SnapEquatorialFront, GLFW_KEY_KP_1, GLFW_KEY_1);
		set(InputAction::SnapEquatorialSide, GLFW_KEY_KP_3, GLFW_KEY_3);
		set(InputAction::SnapNorthPole, GLFW_KEY_KP_7, GLFW_KEY_7);
		set(InputAction::SnapSouthPole, GLFW_KEY_KP_9, GLFW_KEY_9);
		set(InputAction::SnapIsco, GLFW_KEY_KP_5, GLFW_KEY_5);
		set(InputAction::TogglePausePlay, GLFW_KEY_F5, GLFW_KEY_P);
		set(InputAction::SingleStepTick, GLFW_KEY_F6, GLFW_KEY_UNKNOWN);
		set(InputAction::ResetClock, GLFW_KEY_F7, GLFW_KEY_UNKNOWN);
		set(InputAction::ToggleControlPanel, GLFW_KEY_F1, GLFW_KEY_UNKNOWN);
		set(InputAction::LayoutMultiWindow, GLFW_KEY_F2, GLFW_KEY_UNKNOWN);
		set(InputAction::LayoutDocked, GLFW_KEY_F3, GLFW_KEY_UNKNOWN);
		set(InputAction::LayoutViewportFocus, GLFW_KEY_F4, GLFW_KEY_UNKNOWN);
		set(InputAction::ToggleBodyManager, GLFW_KEY_F8, GLFW_KEY_UNKNOWN);
		set(InputAction::CycleCameraMode, GLFW_KEY_F9, GLFW_KEY_UNKNOWN);
		set(InputAction::ToggleTelemetryWindow, GLFW_KEY_F10, GLFW_KEY_UNKNOWN);
		set(InputAction::TogglePerformanceWindow, GLFW_KEY_F11, GLFW_KEY_UNKNOWN);
		set(InputAction::CaptureScreenshot, GLFW_KEY_F12, GLFW_KEY_UNKNOWN);
		set(InputAction::ToggleHudManager, GLFW_KEY_H, GLFW_KEY_UNKNOWN);
		set(InputAction::ToggleKeybindSettings, GLFW_KEY_B, GLFW_KEY_UNKNOWN);
	}

public:
	ActionKeybindMap() noexcept {
		apply_layout_defaults(KeyboardLayout::Qwerty);
	}

	void reset_to_layout_defaults(KeyboardLayout layout) noexcept {
		apply_layout_defaults(layout);
	}

	[[nodiscard]] KeyboardLayout layout() const noexcept {
		return layout_;
	}

	void set(InputAction action, int primary, int secondary = GLFW_KEY_UNKNOWN) noexcept {
		bindings_[static_cast<size_t>(action)] = KeyBinding{primary, secondary};
	}

	void set_primary(InputAction action, int key) noexcept {
		bindings_[static_cast<size_t>(action)].primary_key = key;
	}

	void set_secondary(InputAction action, int key) noexcept {
		bindings_[static_cast<size_t>(action)].secondary_key = key;
	}

	[[nodiscard]] const KeyBinding& get(InputAction action) const noexcept {
		return bindings_[static_cast<size_t>(action)];
	}

	[[nodiscard]] bool is_pressed(InputAction action, GLFWwindow* window) const noexcept {
		return bindings_[static_cast<size_t>(action)].matches(window);
	}

	[[nodiscard]] bool is_key_used_elsewhere(int key, InputAction excluding) const noexcept {
		for (size_t i = 0; i < bindings_.size(); ++i) {
			if (static_cast<InputAction>(i) == excluding) continue;
			if (bindings_[i].contains_key(key)) return true;
		}
		return false;
	}

	[[nodiscard]] std::array<KeyBinding, static_cast<size_t>(InputAction::Count)>& raw_bindings() noexcept {
		return bindings_;
	}

	[[nodiscard]] const std::array<KeyBinding, static_cast<size_t>(InputAction::Count)>& raw_bindings() const noexcept {
		return bindings_;
	}
};

class ActionEdgeTracker {
private:
	std::array<bool, static_cast<size_t>(InputAction::Count)> previous_state_{};

public:
	[[nodiscard]] bool just_pressed(const ActionKeybindMap& map, InputAction action, GLFWwindow* window) noexcept {
		const bool current = map.is_pressed(action, window);
		const size_t idx = static_cast<size_t>(action);
		const bool fired = current && !previous_state_[idx];
		previous_state_[idx] = current;
		return fired;
	}

	void reset() noexcept {
		previous_state_.fill(false);
	}
};

}

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
	CycleMetric,
	CycleIntegrator,
	CycleProjectionMode,
	CycleTonemapper,
	CycleSkyboxStyle,
	ToggleGpuCompute,
	ToggleSpaceSkipping,
	ToggleLodSystem,
	IncreaseExposure,
	DecreaseExposure,
	IncreaseTimeWarp,
	DecreaseTimeWarp,
	QuickSaveScenario,
	QuickLoadScenario,
	ToggleFullscreenViewport,
	ToggleWorkDistributionTiling,
	CycleStepController,
	ResetToDefaultPerformance,
	ToggleScenarioWindow,
	ToggleDiagnosticsWindow,
	ToggleSpectrographWindow,
	Count
};

enum class InputActionCategory : uint32_t {
	Movement = 0,
	CameraOrientation,
	CameraFraming,
	QuickSnap,
	SimulationControl,
	SpacetimeModel,
	RenderingQuality,
	InterfaceWindows
};

[[nodiscard]] constexpr std::string_view input_action_category_name(InputActionCategory category) noexcept {
	switch (category) {
		case InputActionCategory::Movement: return "Movement";
		case InputActionCategory::CameraOrientation: return "Camera Orientation";
		case InputActionCategory::CameraFraming: return "Camera Framing";
		case InputActionCategory::QuickSnap: return "Quick Snap Positions";
		case InputActionCategory::SimulationControl: return "Simulation Control";
		case InputActionCategory::SpacetimeModel: return "Spacetime Model";
		case InputActionCategory::RenderingQuality: return "Rendering Quality";
		case InputActionCategory::InterfaceWindows: return "Interface Windows";
		default: return "Other";
	}
}

[[nodiscard]] constexpr InputActionCategory input_action_category(InputAction action) noexcept {
	switch (action) {
		case InputAction::MoveForward:
		case InputAction::MoveBackward:
		case InputAction::MoveLeft:
		case InputAction::MoveRight:
		case InputAction::MoveUp:
		case InputAction::MoveDown:
		case InputAction::RollLeft:
		case InputAction::RollRight:
		case InputAction::Sprint:
		case InputAction::Crawl:
			return InputActionCategory::Movement;
		case InputAction::LookAtOrigin:
		case InputAction::ResetRoll:
		case InputAction::ZoomModifier:
			return InputActionCategory::CameraOrientation;
		case InputAction::SpeedDecrease:
		case InputAction::SpeedIncrease:
		case InputAction::IncreaseExposure:
		case InputAction::DecreaseExposure:
		case InputAction::CycleProjectionMode:
		case InputAction::CycleTonemapper:
		case InputAction::CycleSkyboxStyle:
			return InputActionCategory::CameraFraming;
		case InputAction::SnapEquatorialFront:
		case InputAction::SnapEquatorialSide:
		case InputAction::SnapNorthPole:
		case InputAction::SnapSouthPole:
		case InputAction::SnapIsco:
			return InputActionCategory::QuickSnap;
		case InputAction::TogglePausePlay:
		case InputAction::SingleStepTick:
		case InputAction::ResetClock:
		case InputAction::CycleCameraMode:
		case InputAction::IncreaseTimeWarp:
		case InputAction::DecreaseTimeWarp:
		case InputAction::QuickSaveScenario:
		case InputAction::QuickLoadScenario:
			return InputActionCategory::SimulationControl;
		case InputAction::CycleMetric:
		case InputAction::CycleIntegrator:
			return InputActionCategory::SpacetimeModel;
		case InputAction::ToggleGpuCompute:
		case InputAction::ToggleSpaceSkipping:
		case InputAction::ToggleLodSystem:
		case InputAction::ToggleWorkDistributionTiling:
		case InputAction::CycleStepController:
		case InputAction::ResetToDefaultPerformance:
			return InputActionCategory::RenderingQuality;
		case InputAction::ToggleControlPanel:
		case InputAction::LayoutMultiWindow:
		case InputAction::LayoutDocked:
		case InputAction::LayoutViewportFocus:
		case InputAction::ToggleBodyManager:
		case InputAction::ToggleTelemetryWindow:
		case InputAction::TogglePerformanceWindow:
		case InputAction::CaptureScreenshot:
		case InputAction::ToggleHudManager:
		case InputAction::ToggleKeybindSettings:
		case InputAction::ToggleFullscreenViewport:
		case InputAction::ToggleScenarioWindow:
		case InputAction::ToggleDiagnosticsWindow:
		case InputAction::ToggleSpectrographWindow:
		default:
			return InputActionCategory::InterfaceWindows;
	}
}

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
		case InputAction::CycleMetric: return "Cycle Spacetime Metric";
		case InputAction::CycleIntegrator: return "Cycle ODE Integrator";
		case InputAction::CycleProjectionMode: return "Cycle Projection Mode";
		case InputAction::CycleTonemapper: return "Cycle HDR Tonemapper";
		case InputAction::CycleSkyboxStyle: return "Cycle Skybox Style";
		case InputAction::ToggleGpuCompute: return "Toggle GPU Compute Offload";
		case InputAction::ToggleSpaceSkipping: return "Toggle Adaptive Space-Skipping";
		case InputAction::ToggleLodSystem: return "Toggle Distance-Based LOD";
		case InputAction::IncreaseExposure: return "Increase Exposure (EV)";
		case InputAction::DecreaseExposure: return "Decrease Exposure (EV)";
		case InputAction::IncreaseTimeWarp: return "Increase Time Warp Factor";
		case InputAction::DecreaseTimeWarp: return "Decrease Time Warp Factor";
		case InputAction::QuickSaveScenario: return "Quick Save Scenario";
		case InputAction::QuickLoadScenario: return "Quick Load Scenario";
		case InputAction::ToggleFullscreenViewport: return "Toggle Fullscreen Viewport";
		case InputAction::ToggleWorkDistributionTiling: return "Toggle Tiled Work Distribution";
		case InputAction::CycleStepController: return "Cycle Adaptive Step Controller";
		case InputAction::ResetToDefaultPerformance: return "Reset Performance To Balanced Preset";
		case InputAction::ToggleScenarioWindow: return "Toggle Scenario Catalog Window";
		case InputAction::ToggleDiagnosticsWindow: return "Toggle Curvature Diagnostics Window";
		case InputAction::ToggleSpectrographWindow: return "Toggle Spectrograph Window";
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

[[nodiscard]] constexpr const char* qwerty_reference_key_name(int key) noexcept {
	switch (key) {
		case GLFW_KEY_A: return "A";
		case GLFW_KEY_B: return "B";
		case GLFW_KEY_C: return "C";
		case GLFW_KEY_D: return "D";
		case GLFW_KEY_E: return "E";
		case GLFW_KEY_F: return "F";
		case GLFW_KEY_G: return "G";
		case GLFW_KEY_H: return "H";
		case GLFW_KEY_I: return "I";
		case GLFW_KEY_J: return "J";
		case GLFW_KEY_K: return "K";
		case GLFW_KEY_L: return "L";
		case GLFW_KEY_M: return "M";
		case GLFW_KEY_N: return "N";
		case GLFW_KEY_O: return "O";
		case GLFW_KEY_P: return "P";
		case GLFW_KEY_Q: return "Q";
		case GLFW_KEY_R: return "R";
		case GLFW_KEY_S: return "S";
		case GLFW_KEY_T: return "T";
		case GLFW_KEY_U: return "U";
		case GLFW_KEY_V: return "V";
		case GLFW_KEY_W: return "W";
		case GLFW_KEY_X: return "X";
		case GLFW_KEY_Y: return "Y";
		case GLFW_KEY_Z: return "Z";
		case GLFW_KEY_0: return "0";
		case GLFW_KEY_1: return "1";
		case GLFW_KEY_2: return "2";
		case GLFW_KEY_3: return "3";
		case GLFW_KEY_4: return "4";
		case GLFW_KEY_5: return "5";
		case GLFW_KEY_6: return "6";
		case GLFW_KEY_7: return "7";
		case GLFW_KEY_8: return "8";
		case GLFW_KEY_9: return "9";
		default: return nullptr;
	}
}

[[nodiscard]] inline const char* glfw_key_display_name(int key) noexcept {
	if (key == GLFW_KEY_UNKNOWN) return "---";
	if (const char* canonical = qwerty_reference_key_name(key)) return canonical;
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
		set(InputAction::SnapEquatorialFront, GLFW_KEY_KP_1);
		set(InputAction::SnapEquatorialSide, GLFW_KEY_KP_3);
		set(InputAction::SnapNorthPole, GLFW_KEY_KP_7);
		set(InputAction::SnapSouthPole, GLFW_KEY_KP_9);
		set(InputAction::SnapIsco, GLFW_KEY_KP_5);
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
		set(InputAction::CycleMetric, GLFW_KEY_M, GLFW_KEY_UNKNOWN);
		set(InputAction::CycleIntegrator, GLFW_KEY_I, GLFW_KEY_UNKNOWN);
		set(InputAction::CycleProjectionMode, GLFW_KEY_V, GLFW_KEY_UNKNOWN);
		set(InputAction::CycleTonemapper, GLFW_KEY_T, GLFW_KEY_UNKNOWN);
		set(InputAction::CycleSkyboxStyle, GLFW_KEY_G, GLFW_KEY_UNKNOWN);
		set(InputAction::ToggleGpuCompute, GLFW_KEY_U, GLFW_KEY_UNKNOWN);
		set(InputAction::ToggleSpaceSkipping, GLFW_KEY_N, GLFW_KEY_UNKNOWN);
		set(InputAction::ToggleLodSystem, GLFW_KEY_L, GLFW_KEY_UNKNOWN);
		set(InputAction::IncreaseExposure, GLFW_KEY_EQUAL, GLFW_KEY_UNKNOWN);
		set(InputAction::DecreaseExposure, GLFW_KEY_MINUS, GLFW_KEY_UNKNOWN);
		set(InputAction::IncreaseTimeWarp, GLFW_KEY_PERIOD, GLFW_KEY_UNKNOWN);
		set(InputAction::DecreaseTimeWarp, GLFW_KEY_COMMA, GLFW_KEY_UNKNOWN);
		set(InputAction::QuickSaveScenario, GLFW_KEY_INSERT, GLFW_KEY_UNKNOWN);
		set(InputAction::QuickLoadScenario, GLFW_KEY_DELETE, GLFW_KEY_UNKNOWN);
		set(InputAction::ToggleFullscreenViewport, GLFW_KEY_GRAVE_ACCENT, GLFW_KEY_UNKNOWN);
		set(InputAction::ToggleWorkDistributionTiling, GLFW_KEY_R, GLFW_KEY_UNKNOWN);
		set(InputAction::CycleStepController, GLFW_KEY_0, GLFW_KEY_UNKNOWN);
		set(InputAction::ResetToDefaultPerformance, GLFW_KEY_9, GLFW_KEY_UNKNOWN);
		set(InputAction::ToggleScenarioWindow, GLFW_KEY_O, GLFW_KEY_UNKNOWN);
		set(InputAction::ToggleDiagnosticsWindow, GLFW_KEY_Y, GLFW_KEY_UNKNOWN);
		set(InputAction::ToggleSpectrographWindow, GLFW_KEY_X, GLFW_KEY_UNKNOWN);
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

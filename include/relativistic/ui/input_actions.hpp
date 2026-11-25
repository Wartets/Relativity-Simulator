#pragma once

#include <GLFW/glfw3.h>
#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace Relativistic::UI {

enum class KeyboardLayout : uint32_t {
	Qwerty = 0,
	Azerty = 1
};

enum class InputActivationMode : uint32_t {
	Hold = 0,
	Toggle = 1,
	Tap = 2,
	DoubleTap = 3
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
	TogglePerformanceAnalysisWindow,
	StartStopBenchmarkCapture,
	QuickSaveBenchmarkRun,
	ToggleConstantsWindow,
	BulkInvertAllVelocities,
	BulkScatterBodyPositions,
	BulkSnapBodiesToGrid,
	BulkCullBodiesOutsideView,
	BulkEqualizeBodyMasses,
	BulkAverageBodyMasses,
	BulkZeroAllSpins,
	ToggleCaptureStudio,
	ToggleRayProbeFreeze,
	ToggleArrowKeysMode,
	LookUp,
	LookDown,
	LookLeft,
	LookRight,
	FovIncrease,
	FovDecrease,
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
		case InputAction::ToggleArrowKeysMode:
		case InputAction::LookUp:
		case InputAction::LookDown:
		case InputAction::LookLeft:
		case InputAction::LookRight:
			return InputActionCategory::CameraOrientation;
		case InputAction::SpeedDecrease:
		case InputAction::SpeedIncrease:
		case InputAction::IncreaseExposure:
		case InputAction::DecreaseExposure:
		case InputAction::CycleProjectionMode:
		case InputAction::CycleTonemapper:
		case InputAction::CycleSkyboxStyle:
		case InputAction::FovIncrease:
		case InputAction::FovDecrease:
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
		case InputAction::BulkInvertAllVelocities:
		case InputAction::BulkScatterBodyPositions:
		case InputAction::BulkSnapBodiesToGrid:
		case InputAction::BulkCullBodiesOutsideView:
		case InputAction::BulkEqualizeBodyMasses:
		case InputAction::BulkAverageBodyMasses:
		case InputAction::BulkZeroAllSpins:
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
		case InputAction::TogglePerformanceAnalysisWindow:
		case InputAction::ToggleConstantsWindow:
		case InputAction::ToggleCaptureStudio:
		case InputAction::ToggleRayProbeFreeze:
			return InputActionCategory::InterfaceWindows;
		case InputAction::StartStopBenchmarkCapture:
		case InputAction::QuickSaveBenchmarkRun:
			return InputActionCategory::RenderingQuality;
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
		case InputAction::TogglePerformanceAnalysisWindow: return "Toggle Performance Analysis Window";
		case InputAction::StartStopBenchmarkCapture: return "Start/Stop Benchmark Capture";
		case InputAction::QuickSaveBenchmarkRun: return "Quick Save Live Window As Benchmark Run";
		case InputAction::ToggleConstantsWindow: return "Toggle Physical Constants Engine";
		case InputAction::ToggleCaptureStudio: return "Toggle Capture Studio";
		case InputAction::ToggleRayProbeFreeze: return "Freeze / Unfreeze Ray Probe";
		case InputAction::ToggleArrowKeysMode: return "Toggle Arrow Keys Mode (Move vs Look)";
		case InputAction::LookUp: return "Look Up";
		case InputAction::LookDown: return "Look Down";
		case InputAction::LookLeft: return "Look Left";
		case InputAction::LookRight: return "Look Right";
		case InputAction::FovIncrease: return "Field of View Increase";
		case InputAction::FovDecrease: return "Field of View Decrease";
		case InputAction::BulkInvertAllVelocities: return "Bulk: Invert All Body Velocities";
		case InputAction::BulkScatterBodyPositions: return "Bulk: Scatter Body Positions";
		case InputAction::BulkSnapBodiesToGrid: return "Bulk: Snap Bodies To Grid";
		case InputAction::BulkCullBodiesOutsideView: return "Bulk: Cull Bodies Outside View";
		case InputAction::BulkEqualizeBodyMasses: return "Bulk: Equalize Body Masses";
		case InputAction::BulkAverageBodyMasses: return "Bulk: Average Body Masses";
		case InputAction::BulkZeroAllSpins: return "Bulk: Zero All Body Spins";
		default: return "Unknown Action";
	}
}

struct KeyBinding {
	int primary_key{GLFW_KEY_UNKNOWN};
	int secondary_key{GLFW_KEY_UNKNOWN};
	int primary_modifier{GLFW_KEY_UNKNOWN};
	int primary_modifier2{GLFW_KEY_UNKNOWN};
	int secondary_modifier{GLFW_KEY_UNKNOWN};
	int secondary_modifier2{GLFW_KEY_UNKNOWN};
	InputActivationMode mode{InputActivationMode::Hold};
	float tap_duration_seconds{0.25f};
	float double_tap_window_seconds{0.30f};

	constexpr KeyBinding() noexcept = default;

	constexpr KeyBinding(int prim, int sec = GLFW_KEY_UNKNOWN, InputActivationMode m = InputActivationMode::Hold) noexcept
		: primary_key(prim), secondary_key(sec), mode(m) {}

	constexpr KeyBinding(int prim, int prim_mod, int sec, int sec_mod, InputActivationMode m) noexcept
		: primary_key(prim), secondary_key(sec), primary_modifier(prim_mod), secondary_modifier(sec_mod), mode(m) {}

	[[nodiscard]] bool matches(GLFWwindow* window) const noexcept {
		if (window == nullptr) return false;
		if (primary_key != GLFW_KEY_UNKNOWN && glfwGetKey(window, primary_key) == GLFW_PRESS) {
			const bool m1 = (primary_modifier == GLFW_KEY_UNKNOWN) || (glfwGetKey(window, primary_modifier) == GLFW_PRESS);
			const bool m2 = (primary_modifier2 == GLFW_KEY_UNKNOWN) || (glfwGetKey(window, primary_modifier2) == GLFW_PRESS);
			if (m1 && m2) return true;
		}
		if (secondary_key != GLFW_KEY_UNKNOWN && glfwGetKey(window, secondary_key) == GLFW_PRESS) {
			const bool m1 = (secondary_modifier == GLFW_KEY_UNKNOWN) || (glfwGetKey(window, secondary_modifier) == GLFW_PRESS);
			const bool m2 = (secondary_modifier2 == GLFW_KEY_UNKNOWN) || (glfwGetKey(window, secondary_modifier2) == GLFW_PRESS);
			if (m1 && m2) return true;
		}
		return false;
	}

	[[nodiscard]] bool contains_key(int key) const noexcept {
		if (key == GLFW_KEY_UNKNOWN) return false;
		return key == primary_key || key == secondary_key ||
			   key == primary_modifier || key == primary_modifier2 ||
			   key == secondary_modifier || key == secondary_modifier2;
	}

	[[nodiscard]] bool has_modifiers() const noexcept {
		return primary_modifier != GLFW_KEY_UNKNOWN || primary_modifier2 != GLFW_KEY_UNKNOWN ||
			   secondary_modifier != GLFW_KEY_UNKNOWN || secondary_modifier2 != GLFW_KEY_UNKNOWN;
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

[[nodiscard]] constexpr const char* azerty_display_override(int key) noexcept {
	switch (key) {
		case GLFW_KEY_Q: return "A";
		case GLFW_KEY_A: return "Q";
		case GLFW_KEY_Z: return "W";
		case GLFW_KEY_W: return "Z";
		case GLFW_KEY_M: return ";";
		case GLFW_KEY_SEMICOLON: return "M";
		case GLFW_KEY_0: return "\xc3\xa0";
		case GLFW_KEY_1: return "&";
		case GLFW_KEY_2: return "\xc3\xa9";
		case GLFW_KEY_3: return "\"";
		case GLFW_KEY_4: return "'";
		case GLFW_KEY_5: return "(";
		case GLFW_KEY_6: return "-";
		case GLFW_KEY_7: return "\xc3\xa8";
		case GLFW_KEY_8: return "_";
		case GLFW_KEY_9: return "\xc3\xa7";
		case GLFW_KEY_COMMA: return ";";
		case GLFW_KEY_PERIOD: return ":";
		case GLFW_KEY_SLASH: return "!";
		case GLFW_KEY_MINUS: return ")";
		case GLFW_KEY_EQUAL: return "=";
		case GLFW_KEY_LEFT_BRACKET: return "^";
		case GLFW_KEY_RIGHT_BRACKET: return "$";
		case GLFW_KEY_APOSTROPHE: return "\xc3\xb9";
		case GLFW_KEY_GRAVE_ACCENT: return "\xc2\xb2";
		case GLFW_KEY_BACKSLASH: return "*";
		default: return nullptr;
	}
}

[[nodiscard]] inline const char* glfw_key_display_name(int key, KeyboardLayout layout = KeyboardLayout::Qwerty) noexcept {
	if (key == GLFW_KEY_UNKNOWN) return "---";
	if (layout == KeyboardLayout::Azerty) {
		if (const char* azerty_name = azerty_display_override(key)) return azerty_name;
	}
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
		case GLFW_KEY_LEFT_BRACKET: return "[";
		case GLFW_KEY_RIGHT_BRACKET: return "]";
		case GLFW_KEY_MINUS: return "-";
		case GLFW_KEY_EQUAL: return "=";
		case GLFW_KEY_COMMA: return ",";
		case GLFW_KEY_PERIOD: return ".";
		case GLFW_KEY_SLASH: return "/";
		case GLFW_KEY_BACKSLASH: return "\\";
		case GLFW_KEY_SEMICOLON: return ";";
		case GLFW_KEY_APOSTROPHE: return "'";
		case GLFW_KEY_GRAVE_ACCENT: return "`";
		default: return "?";
	}
}

[[nodiscard]] inline std::string format_single_chord(int key, int mod1, int mod2, KeyboardLayout layout = KeyboardLayout::Qwerty) noexcept {
	if (key == GLFW_KEY_UNKNOWN && mod1 == GLFW_KEY_UNKNOWN && mod2 == GLFW_KEY_UNKNOWN) return "";
	std::string s;
	if (mod1 != GLFW_KEY_UNKNOWN) {
		s += glfw_key_display_name(mod1, layout);
		s += "+";
	}
	if (mod2 != GLFW_KEY_UNKNOWN) {
		s += glfw_key_display_name(mod2, layout);
		s += "+";
	}
	if (key != GLFW_KEY_UNKNOWN) {
		s += glfw_key_display_name(key, layout);
	}
	return s;
}

[[nodiscard]] inline std::string format_key_binding(const KeyBinding& binding, KeyboardLayout layout = KeyboardLayout::Qwerty) noexcept {
	const std::string prim_str = format_single_chord(binding.primary_key, binding.primary_modifier, binding.primary_modifier2, layout);
	const std::string sec_str = format_single_chord(binding.secondary_key, binding.secondary_modifier, binding.secondary_modifier2, layout);
	if (prim_str.empty() && sec_str.empty()) return "---";
	if (!prim_str.empty() && !sec_str.empty()) return prim_str + " / " + sec_str;
	return !prim_str.empty() ? prim_str : sec_str;
}

class ActionKeybindMap {
private:
	std::array<KeyBinding, static_cast<size_t>(InputAction::Count)> bindings_{};
	KeyboardLayout layout_{KeyboardLayout::Qwerty};
	mutable std::array<bool, static_cast<size_t>(InputAction::Count)> toggle_state_{};
	mutable std::array<bool, static_cast<size_t>(InputAction::Count)> toggle_prev_raw_{};
	mutable std::array<double, static_cast<size_t>(InputAction::Count)> press_time_{};
	mutable std::array<double, static_cast<size_t>(InputAction::Count)> release_time_{};
	mutable std::array<uint8_t, static_cast<size_t>(InputAction::Count)> tap_count_{};

	void apply_layout_defaults(KeyboardLayout layout) noexcept {
		layout_ = layout;
		for (size_t i = 0; i < static_cast<size_t>(InputAction::Count); ++i) {
			const auto action = static_cast<InputAction>(i);
			const InputActivationMode preserved_mode = bindings_[i].mode;
			bindings_[i] = layout_default(action, layout);
			bindings_[i].mode = preserved_mode;
		}
	}

public:
	[[nodiscard]] static constexpr KeyBinding layout_default(InputAction action, KeyboardLayout layout) noexcept {
		const bool azerty = (layout == KeyboardLayout::Azerty);
		switch (action) {
			case InputAction::MoveForward: return KeyBinding{azerty ? GLFW_KEY_Z : GLFW_KEY_W, GLFW_KEY_UP};
			case InputAction::MoveBackward: return KeyBinding{GLFW_KEY_S, GLFW_KEY_DOWN};
			case InputAction::MoveLeft: return KeyBinding{azerty ? GLFW_KEY_Q : GLFW_KEY_A, GLFW_KEY_LEFT};
			case InputAction::MoveRight: return KeyBinding{GLFW_KEY_D, GLFW_KEY_RIGHT};
			case InputAction::MoveUp: return KeyBinding{GLFW_KEY_SPACE, GLFW_KEY_E};
			case InputAction::MoveDown: return KeyBinding{GLFW_KEY_C, GLFW_KEY_LEFT_CONTROL};
			case InputAction::RollLeft: return KeyBinding{GLFW_KEY_J, GLFW_KEY_PAGE_UP};
			case InputAction::RollRight: return KeyBinding{GLFW_KEY_K, GLFW_KEY_PAGE_DOWN};
			case InputAction::Sprint: return KeyBinding{GLFW_KEY_LEFT_SHIFT, GLFW_KEY_RIGHT_SHIFT};
			case InputAction::Crawl: return KeyBinding{GLFW_KEY_LEFT_ALT, GLFW_KEY_RIGHT_ALT};
			case InputAction::LookAtOrigin: return KeyBinding{GLFW_KEY_F, GLFW_KEY_UNKNOWN};
			case InputAction::ResetRoll: return KeyBinding{GLFW_KEY_HOME, GLFW_KEY_UNKNOWN};
			case InputAction::SpeedDecrease: return KeyBinding{GLFW_KEY_LEFT_BRACKET, GLFW_KEY_UNKNOWN};
			case InputAction::SpeedIncrease: return KeyBinding{GLFW_KEY_RIGHT_BRACKET, GLFW_KEY_UNKNOWN};
			case InputAction::ZoomModifier: return KeyBinding{azerty ? GLFW_KEY_W : GLFW_KEY_Z, GLFW_KEY_UNKNOWN};
			case InputAction::SnapEquatorialFront: return KeyBinding{GLFW_KEY_KP_1, GLFW_KEY_UNKNOWN};
			case InputAction::SnapEquatorialSide: return KeyBinding{GLFW_KEY_KP_3, GLFW_KEY_UNKNOWN};
			case InputAction::SnapNorthPole: return KeyBinding{GLFW_KEY_KP_7, GLFW_KEY_UNKNOWN};
			case InputAction::SnapSouthPole: return KeyBinding{GLFW_KEY_KP_9, GLFW_KEY_UNKNOWN};
			case InputAction::SnapIsco: return KeyBinding{GLFW_KEY_KP_5, GLFW_KEY_UNKNOWN};
			case InputAction::TogglePausePlay: return KeyBinding{GLFW_KEY_F5, GLFW_KEY_P};
			case InputAction::SingleStepTick: return KeyBinding{GLFW_KEY_F6, GLFW_KEY_UNKNOWN};
			case InputAction::ResetClock: return KeyBinding{GLFW_KEY_F7, GLFW_KEY_UNKNOWN};
			case InputAction::ToggleControlPanel: return KeyBinding{GLFW_KEY_F1, GLFW_KEY_UNKNOWN};
			case InputAction::LayoutMultiWindow: return KeyBinding{GLFW_KEY_F2, GLFW_KEY_UNKNOWN};
			case InputAction::LayoutDocked: return KeyBinding{GLFW_KEY_F3, GLFW_KEY_UNKNOWN};
			case InputAction::LayoutViewportFocus: return KeyBinding{GLFW_KEY_F4, GLFW_KEY_UNKNOWN};
			case InputAction::ToggleBodyManager: return KeyBinding{GLFW_KEY_F8, GLFW_KEY_UNKNOWN};
			case InputAction::CycleCameraMode: return KeyBinding{GLFW_KEY_F9, GLFW_KEY_UNKNOWN};
			case InputAction::ToggleTelemetryWindow: return KeyBinding{GLFW_KEY_F10, GLFW_KEY_UNKNOWN};
			case InputAction::TogglePerformanceWindow: return KeyBinding{GLFW_KEY_F11, GLFW_KEY_UNKNOWN};
			case InputAction::CaptureScreenshot: return KeyBinding{GLFW_KEY_F12, GLFW_KEY_UNKNOWN};
			case InputAction::ToggleHudManager: return KeyBinding{GLFW_KEY_H, GLFW_KEY_UNKNOWN};
			case InputAction::ToggleKeybindSettings: return KeyBinding{GLFW_KEY_B, GLFW_KEY_UNKNOWN};
			case InputAction::CycleMetric: return KeyBinding{GLFW_KEY_M, GLFW_KEY_UNKNOWN};
			case InputAction::CycleIntegrator: return KeyBinding{GLFW_KEY_I, GLFW_KEY_UNKNOWN};
			case InputAction::CycleProjectionMode: return KeyBinding{GLFW_KEY_V, GLFW_KEY_UNKNOWN};
			case InputAction::CycleTonemapper: return KeyBinding{GLFW_KEY_T, GLFW_KEY_UNKNOWN};
			case InputAction::CycleSkyboxStyle: return KeyBinding{GLFW_KEY_G, GLFW_KEY_UNKNOWN};
			case InputAction::ToggleGpuCompute: return KeyBinding{GLFW_KEY_U, GLFW_KEY_UNKNOWN};
			case InputAction::ToggleSpaceSkipping: return KeyBinding{GLFW_KEY_N, GLFW_KEY_UNKNOWN};
			case InputAction::ToggleLodSystem: return KeyBinding{GLFW_KEY_L, GLFW_KEY_UNKNOWN};
			case InputAction::IncreaseExposure: return KeyBinding{GLFW_KEY_EQUAL, GLFW_KEY_UNKNOWN};
			case InputAction::DecreaseExposure: return KeyBinding{GLFW_KEY_MINUS, GLFW_KEY_UNKNOWN};
			case InputAction::IncreaseTimeWarp: return KeyBinding{GLFW_KEY_PERIOD, GLFW_KEY_UNKNOWN};
			case InputAction::DecreaseTimeWarp: return KeyBinding{GLFW_KEY_COMMA, GLFW_KEY_UNKNOWN};
			case InputAction::QuickSaveScenario: return KeyBinding{GLFW_KEY_INSERT, GLFW_KEY_UNKNOWN};
			case InputAction::QuickLoadScenario: return KeyBinding{GLFW_KEY_DELETE, GLFW_KEY_UNKNOWN};
			case InputAction::ToggleFullscreenViewport: return KeyBinding{GLFW_KEY_GRAVE_ACCENT, GLFW_KEY_UNKNOWN};
			case InputAction::ToggleWorkDistributionTiling: return KeyBinding{GLFW_KEY_R, GLFW_KEY_UNKNOWN};
			case InputAction::CycleStepController: return KeyBinding{GLFW_KEY_0, GLFW_KEY_UNKNOWN};
			case InputAction::ResetToDefaultPerformance: return KeyBinding{GLFW_KEY_9, GLFW_KEY_UNKNOWN};
			case InputAction::ToggleScenarioWindow: return KeyBinding{GLFW_KEY_O, GLFW_KEY_UNKNOWN};
			case InputAction::ToggleDiagnosticsWindow: return KeyBinding{GLFW_KEY_Y, GLFW_KEY_UNKNOWN};
			case InputAction::ToggleSpectrographWindow: return KeyBinding{GLFW_KEY_X, GLFW_KEY_UNKNOWN};
			case InputAction::TogglePerformanceAnalysisWindow: return KeyBinding{GLFW_KEY_UNKNOWN, GLFW_KEY_UNKNOWN};
			case InputAction::StartStopBenchmarkCapture: return KeyBinding{GLFW_KEY_UNKNOWN, GLFW_KEY_UNKNOWN};
			case InputAction::QuickSaveBenchmarkRun: return KeyBinding{GLFW_KEY_UNKNOWN, GLFW_KEY_UNKNOWN};
			case InputAction::ToggleConstantsWindow: return KeyBinding{GLFW_KEY_SEMICOLON, GLFW_KEY_UNKNOWN};
			case InputAction::ToggleRayProbeFreeze: return KeyBinding{GLFW_KEY_BACKSLASH, GLFW_KEY_UNKNOWN};
			case InputAction::ToggleArrowKeysMode: return KeyBinding{GLFW_KEY_BACKSLASH, GLFW_KEY_RIGHT_ALT, GLFW_KEY_UNKNOWN, GLFW_KEY_UNKNOWN, InputActivationMode::Hold};
			case InputAction::LookUp: return KeyBinding{GLFW_KEY_UNKNOWN, GLFW_KEY_UNKNOWN};
			case InputAction::LookDown: return KeyBinding{GLFW_KEY_UNKNOWN, GLFW_KEY_UNKNOWN};
			case InputAction::LookLeft: return KeyBinding{GLFW_KEY_UNKNOWN, GLFW_KEY_UNKNOWN};
			case InputAction::LookRight: return KeyBinding{GLFW_KEY_UNKNOWN, GLFW_KEY_UNKNOWN};
			case InputAction::FovIncrease: return KeyBinding{GLFW_KEY_UNKNOWN, GLFW_KEY_UNKNOWN};
			case InputAction::FovDecrease: return KeyBinding{GLFW_KEY_UNKNOWN, GLFW_KEY_UNKNOWN};
			default: return KeyBinding{GLFW_KEY_UNKNOWN, GLFW_KEY_UNKNOWN};
		}
	}

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

	void set_primary_chord(InputAction action, int key, int mod1 = GLFW_KEY_UNKNOWN, int mod2 = GLFW_KEY_UNKNOWN) noexcept {
		auto& b = bindings_[static_cast<size_t>(action)];
		b.primary_key = key;
		b.primary_modifier = mod1;
		b.primary_modifier2 = mod2;
	}

	void set_secondary_chord(InputAction action, int key, int mod1 = GLFW_KEY_UNKNOWN, int mod2 = GLFW_KEY_UNKNOWN) noexcept {
		auto& b = bindings_[static_cast<size_t>(action)];
		b.secondary_key = key;
		b.secondary_modifier = mod1;
		b.secondary_modifier2 = mod2;
	}

	void set_timing(InputAction action, float tap_duration, float double_tap_window) noexcept {
		auto& b = bindings_[static_cast<size_t>(action)];
		b.tap_duration_seconds = tap_duration;
		b.double_tap_window_seconds = double_tap_window;
	}

	[[nodiscard]] const KeyBinding& get(InputAction action) const noexcept {
		return bindings_[static_cast<size_t>(action)];
	}

	[[nodiscard]] bool is_pressed(InputAction action, GLFWwindow* window) const noexcept {
		return bindings_[static_cast<size_t>(action)].matches(window);
	}

	[[nodiscard]] bool is_active(InputAction action, GLFWwindow* window, double current_time = 0.0) const noexcept {
		const size_t idx = static_cast<size_t>(action);
		const auto& b = bindings_[idx];
		const bool raw = b.matches(window);
		const bool prev = toggle_prev_raw_[idx];

		if (raw && !prev) {
			press_time_[idx] = current_time;
		}

		bool triggered = false;
		switch (b.mode) {
			case InputActivationMode::Toggle:
				if (raw && !prev) {
					toggle_state_[idx] = !toggle_state_[idx];
				}
				triggered = toggle_state_[idx];
				break;
			case InputActivationMode::Tap:
				if (!raw && prev) {
					const double duration = current_time - press_time_[idx];
					if (duration <= static_cast<double>(b.tap_duration_seconds)) {
						triggered = true;
					}
				}
				break;
			case InputActivationMode::DoubleTap:
				if (!raw && prev) {
					const double duration = current_time - press_time_[idx];
					if (duration <= static_cast<double>(b.tap_duration_seconds)) {
						const double since_last = current_time - release_time_[idx];
						if (since_last <= static_cast<double>(b.double_tap_window_seconds)) {
							triggered = true;
							tap_count_[idx] = 0;
						} else {
							tap_count_[idx] = 1;
						}
					}
					release_time_[idx] = current_time;
				}
				break;
			case InputActivationMode::Hold:
			default:
				triggered = raw;
				break;
		}

		if (!raw && prev) {
			release_time_[idx] = current_time;
		}
		toggle_prev_raw_[idx] = raw;
		return triggered;
	}

	void set_mode(InputAction action, InputActivationMode mode) noexcept {
		bindings_[static_cast<size_t>(action)].mode = mode;
	}

	[[nodiscard]] InputActivationMode mode(InputAction action) const noexcept {
		return bindings_[static_cast<size_t>(action)].mode;
	}

	void reset_to_default(InputAction action, KeyboardLayout layout) noexcept {
		const InputActivationMode preserved_mode = bindings_[static_cast<size_t>(action)].mode;
		bindings_[static_cast<size_t>(action)] = layout_default(action, layout);
		bindings_[static_cast<size_t>(action)].mode = preserved_mode;
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

[[nodiscard]] constexpr const char* input_activation_mode_name(InputActivationMode mode) noexcept {
	switch (mode) {
		case InputActivationMode::Hold: return "Hold (Continuous)";
		case InputActivationMode::Toggle: return "Toggle (Flip On/Off)";
		case InputActivationMode::Tap: return "Tap (Quick Press)";
		case InputActivationMode::DoubleTap: return "Double-Tap";
		default: return "Hold";
	}
}

[[nodiscard]] constexpr bool action_supports_activation_mode(InputAction action, InputActivationMode mode) noexcept {
	if (mode == InputActivationMode::Hold) return true;
	switch (action) {
		case InputAction::MoveForward:
		case InputAction::MoveBackward:
		case InputAction::MoveLeft:
		case InputAction::MoveRight:
		case InputAction::MoveUp:
		case InputAction::MoveDown:
		case InputAction::RollLeft:
		case InputAction::RollRight:
		case InputAction::LookUp:
		case InputAction::LookDown:
		case InputAction::LookLeft:
		case InputAction::LookRight:
			return true;
		case InputAction::Sprint:
		case InputAction::Crawl:
		case InputAction::ZoomModifier:
			return mode == InputActivationMode::Hold || mode == InputActivationMode::Toggle;
		case InputAction::SpeedDecrease:
		case InputAction::SpeedIncrease:
		case InputAction::IncreaseExposure:
		case InputAction::DecreaseExposure:
		case InputAction::IncreaseTimeWarp:
		case InputAction::DecreaseTimeWarp:
		case InputAction::FovIncrease:
		case InputAction::FovDecrease:
			return mode == InputActivationMode::Hold || mode == InputActivationMode::Tap || mode == InputActivationMode::DoubleTap;
		case InputAction::LookAtOrigin:
		case InputAction::ResetRoll:
		case InputAction::SnapEquatorialFront:
		case InputAction::SnapEquatorialSide:
		case InputAction::SnapNorthPole:
		case InputAction::SnapSouthPole:
		case InputAction::SnapIsco:
		case InputAction::TogglePausePlay:
		case InputAction::SingleStepTick:
		case InputAction::ResetClock:
		case InputAction::CycleCameraMode:
		case InputAction::CaptureScreenshot:
		case InputAction::ToggleControlPanel:
		case InputAction::LayoutMultiWindow:
		case InputAction::LayoutDocked:
		case InputAction::LayoutViewportFocus:
		case InputAction::ToggleBodyManager:
		case InputAction::ToggleTelemetryWindow:
		case InputAction::TogglePerformanceWindow:
		case InputAction::ToggleHudManager:
		case InputAction::ToggleKeybindSettings:
		case InputAction::CycleMetric:
		case InputAction::CycleIntegrator:
		case InputAction::CycleProjectionMode:
		case InputAction::CycleTonemapper:
		case InputAction::CycleSkyboxStyle:
		case InputAction::ToggleGpuCompute:
		case InputAction::ToggleSpaceSkipping:
		case InputAction::ToggleLodSystem:
		case InputAction::ToggleFullscreenViewport:
		case InputAction::ToggleScenarioWindow:
		case InputAction::ToggleDiagnosticsWindow:
		case InputAction::ToggleSpectrographWindow:
		case InputAction::TogglePerformanceAnalysisWindow:
		case InputAction::ToggleConstantsWindow:
		case InputAction::ToggleCaptureStudio:
		case InputAction::ToggleRayProbeFreeze:
		case InputAction::ToggleArrowKeysMode:
		case InputAction::BulkInvertAllVelocities:
		case InputAction::BulkScatterBodyPositions:
		case InputAction::BulkSnapBodiesToGrid:
		case InputAction::BulkCullBodiesOutsideView:
		case InputAction::BulkEqualizeBodyMasses:
		case InputAction::BulkAverageBodyMasses:
		case InputAction::BulkZeroAllSpins:
			return mode == InputActivationMode::Tap || mode == InputActivationMode::DoubleTap || mode == InputActivationMode::Toggle;
		default:
			return mode == InputActivationMode::Hold || mode == InputActivationMode::Toggle;
	}
}

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

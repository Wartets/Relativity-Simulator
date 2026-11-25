#pragma once

#include <GLFW/glfw3.h>
#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <string_view>
#include <algorithm>

namespace Relativistic::UI {

enum class GamepadButton : uint8_t {
	A = GLFW_GAMEPAD_BUTTON_A,
	B = GLFW_GAMEPAD_BUTTON_B,
	X = GLFW_GAMEPAD_BUTTON_X,
	Y = GLFW_GAMEPAD_BUTTON_Y,
	LeftBumper = GLFW_GAMEPAD_BUTTON_LEFT_BUMPER,
	RightBumper = GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER,
	Back = GLFW_GAMEPAD_BUTTON_BACK,
	Start = GLFW_GAMEPAD_BUTTON_START,
	Guide = GLFW_GAMEPAD_BUTTON_GUIDE,
	LeftThumb = GLFW_GAMEPAD_BUTTON_LEFT_THUMB,
	RightThumb = GLFW_GAMEPAD_BUTTON_RIGHT_THUMB,
	DpadUp = GLFW_GAMEPAD_BUTTON_DPAD_UP,
	DpadRight = GLFW_GAMEPAD_BUTTON_DPAD_RIGHT,
	DpadDown = GLFW_GAMEPAD_BUTTON_DPAD_DOWN,
	DpadLeft = GLFW_GAMEPAD_BUTTON_DPAD_LEFT,
	None = 255
};

enum class GamepadAxis : uint8_t {
	LeftX = GLFW_GAMEPAD_AXIS_LEFT_X,
	LeftY = GLFW_GAMEPAD_AXIS_LEFT_Y,
	RightX = GLFW_GAMEPAD_AXIS_RIGHT_X,
	RightY = GLFW_GAMEPAD_AXIS_RIGHT_Y,
	LeftTrigger = GLFW_GAMEPAD_AXIS_LEFT_TRIGGER,
	RightTrigger = GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER
};

inline constexpr int kGamepadVirtualLeftStickUp = 15;
inline constexpr int kGamepadVirtualLeftStickDown = 16;
inline constexpr int kGamepadVirtualLeftStickLeft = 17;
inline constexpr int kGamepadVirtualLeftStickRight = 18;
inline constexpr int kGamepadVirtualRightStickUp = 19;
inline constexpr int kGamepadVirtualRightStickDown = 20;
inline constexpr int kGamepadVirtualRightStickLeft = 21;
inline constexpr int kGamepadVirtualRightStickRight = 22;
inline constexpr int kGamepadVirtualLeftTrigger = 23;
inline constexpr int kGamepadVirtualRightTrigger = 24;
inline constexpr size_t kGamepadButtonTotalCount = 25;

[[nodiscard]] constexpr const char* gamepad_button_name(int button) noexcept {
	switch (button) {
		case GLFW_GAMEPAD_BUTTON_A: return "A";
		case GLFW_GAMEPAD_BUTTON_B: return "B";
		case GLFW_GAMEPAD_BUTTON_X: return "X";
		case GLFW_GAMEPAD_BUTTON_Y: return "Y";
		case GLFW_GAMEPAD_BUTTON_LEFT_BUMPER: return "LB";
		case GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER: return "RB";
		case GLFW_GAMEPAD_BUTTON_BACK: return "View/Back";
		case GLFW_GAMEPAD_BUTTON_START: return "Start";
		case GLFW_GAMEPAD_BUTTON_GUIDE: return "Guide/Xbox";
		case GLFW_GAMEPAD_BUTTON_LEFT_THUMB: return "LS Click";
		case GLFW_GAMEPAD_BUTTON_RIGHT_THUMB: return "RS Click";
		case GLFW_GAMEPAD_BUTTON_DPAD_UP: return "D-Pad Up";
		case GLFW_GAMEPAD_BUTTON_DPAD_RIGHT: return "D-Pad Right";
		case GLFW_GAMEPAD_BUTTON_DPAD_DOWN: return "D-Pad Down";
		case GLFW_GAMEPAD_BUTTON_DPAD_LEFT: return "D-Pad Left";
		case kGamepadVirtualLeftStickUp: return "LSB Up";
		case kGamepadVirtualLeftStickDown: return "LSB Down";
		case kGamepadVirtualLeftStickLeft: return "LSB Left";
		case kGamepadVirtualLeftStickRight: return "LSB Right";
		case kGamepadVirtualRightStickUp: return "RSB Up";
		case kGamepadVirtualRightStickDown: return "RSB Down";
		case kGamepadVirtualRightStickLeft: return "RSB Left";
		case kGamepadVirtualRightStickRight: return "RSB Right";
		case kGamepadVirtualLeftTrigger: return "LT";
		case kGamepadVirtualRightTrigger: return "RT";
		default: return "---";
	}
}

[[nodiscard]] constexpr const char* gamepad_axis_name(GamepadAxis axis) noexcept {
	switch (axis) {
		case GamepadAxis::LeftX: return "Left Stick X";
		case GamepadAxis::LeftY: return "Left Stick Y";
		case GamepadAxis::RightX: return "Right Stick X";
		case GamepadAxis::RightY: return "Right Stick Y";
		case GamepadAxis::LeftTrigger: return "Left Trigger";
		case GamepadAxis::RightTrigger: return "Right Trigger";
		default: return "---";
	}
}

struct GamepadAxesConfig {
	float left_stick_deadzone{0.14f};
	float right_stick_deadzone{0.14f};
	float trigger_deadzone{0.08f};
	float look_sensitivity_x{80.0f};
	float look_sensitivity_y{65.0f};
	float move_sensitivity{1.0f};
	float roll_sensitivity{45.0f};
	bool invert_look_y{false};
	bool invert_look_x{false};
	bool invert_roll{false};
	bool invert_vertical{false};
};

struct GamepadBinding {
	int button{static_cast<int>(GamepadButton::None)};
	int modifier_button{static_cast<int>(GamepadButton::None)};
	uint32_t mode{0};
	float tap_duration_seconds{0.25f};
	float double_tap_window_seconds{0.30f};

	[[nodiscard]] bool empty() const noexcept {
		return button == static_cast<int>(GamepadButton::None) && modifier_button == static_cast<int>(GamepadButton::None);
	}

	[[nodiscard]] bool contains(int btn) const noexcept {
		return btn != static_cast<int>(GamepadButton::None) && (btn == button || btn == modifier_button);
	}
};

struct GamepadMotionDeltas {
	float move_forward{0.0f};
	float move_right{0.0f};
	float move_up{0.0f};
	float look_yaw_deg{0.0f};
	float look_pitch_deg{0.0f};
	float roll_deg{0.0f};
	bool sprint{false};
	bool crawl{false};
	bool zoom{false};
};

struct GamepadConfig {
	bool enabled{true};
	int selected_joystick{-1};
	GamepadAxesConfig axes{};
	std::array<GamepadBinding, static_cast<size_t>(InputAction::Count)> bindings{};

	GamepadConfig() noexcept {
		reset_to_defaults();
	}

	void reset_to_defaults() noexcept;
};

class GamepadState {
public:
	bool connected{false};
	int joystick_id{-1};
	std::string name{"Disconnected"};
	GLFWgamepadstate raw_state{};
	std::array<float, 6> filtered_axes{};
	std::array<bool, kGamepadButtonTotalCount> button_pressed{};
	std::array<bool, kGamepadButtonTotalCount> button_prev{};
	std::array<double, kGamepadButtonTotalCount> button_press_time{};
	std::array<double, kGamepadButtonTotalCount> button_release_time{};
	std::array<bool, static_cast<size_t>(InputAction::Count)> action_toggle_state{};
	std::array<bool, static_cast<size_t>(InputAction::Count)> action_prev_raw{};
	std::array<double, static_cast<size_t>(InputAction::Count)> action_press_time{};
	std::array<double, static_cast<size_t>(InputAction::Count)> action_release_time{};
	std::array<uint8_t, static_cast<size_t>(InputAction::Count)> action_tap_count{};
	std::array<bool, static_cast<size_t>(InputAction::Count)> action_active{};

	[[nodiscard]] static float apply_deadzone(float raw, float deadzone) noexcept {
		const float abs_val = std::abs(raw);
		if (abs_val <= deadzone) return 0.0f;
		const float sign = (raw > 0.0f) ? 1.0f : -1.0f;
		const float scaled = (abs_val - deadzone) / (1.0f - deadzone);
		return sign * (scaled * scaled);
	}

	void update(int preferred_jid, const GamepadAxesConfig& cfg, double current_time) noexcept {
		joystick_id = -1;
		connected = false;

		if (preferred_jid >= GLFW_JOYSTICK_1 && preferred_jid <= GLFW_JOYSTICK_LAST) {
			if (glfwJoystickPresent(preferred_jid) && glfwJoystickIsGamepad(preferred_jid)) {
				joystick_id = preferred_jid;
			}
		}
		if (joystick_id < 0) {
			for (int jid = GLFW_JOYSTICK_1; jid <= GLFW_JOYSTICK_LAST; ++jid) {
				if (glfwJoystickPresent(jid) && glfwJoystickIsGamepad(jid)) {
					joystick_id = jid;
					break;
				}
			}
		}

		if (joystick_id < 0) {
			name = "Disconnected";
			filtered_axes.fill(0.0f);
			button_pressed.fill(false);
			return;
		}

		connected = true;
		const char* jname = glfwGetGamepadName(joystick_id);
		name = (jname != nullptr) ? jname : "Gamepad Controller";

		button_prev = button_pressed;
		if (glfwGetGamepadState(joystick_id, &raw_state)) {
			filtered_axes[GLFW_GAMEPAD_AXIS_LEFT_X] = apply_deadzone(raw_state.axes[GLFW_GAMEPAD_AXIS_LEFT_X], cfg.left_stick_deadzone);
			filtered_axes[GLFW_GAMEPAD_AXIS_LEFT_Y] = apply_deadzone(raw_state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y], cfg.left_stick_deadzone);
			filtered_axes[GLFW_GAMEPAD_AXIS_RIGHT_X] = apply_deadzone(raw_state.axes[GLFW_GAMEPAD_AXIS_RIGHT_X], cfg.right_stick_deadzone);
			filtered_axes[GLFW_GAMEPAD_AXIS_RIGHT_Y] = apply_deadzone(raw_state.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y], cfg.right_stick_deadzone);

			const float lt_norm = (raw_state.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER] + 1.0f) * 0.5f;
			const float rt_norm = (raw_state.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER] + 1.0f) * 0.5f;
			filtered_axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER] = apply_deadzone(lt_norm, cfg.trigger_deadzone);
			filtered_axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER] = apply_deadzone(rt_norm, cfg.trigger_deadzone);

			for (int b = 0; b <= GLFW_GAMEPAD_BUTTON_LAST; ++b) {
				button_pressed[static_cast<size_t>(b)] = (raw_state.buttons[b] == GLFW_PRESS);
			}
			constexpr float stick_threshold = 0.40f;
			constexpr float trigger_threshold = 0.40f;
			button_pressed[kGamepadVirtualLeftStickUp] = (filtered_axes[GLFW_GAMEPAD_AXIS_LEFT_Y] < -stick_threshold);
			button_pressed[kGamepadVirtualLeftStickDown] = (filtered_axes[GLFW_GAMEPAD_AXIS_LEFT_Y] > stick_threshold);
			button_pressed[kGamepadVirtualLeftStickLeft] = (filtered_axes[GLFW_GAMEPAD_AXIS_LEFT_X] < -stick_threshold);
			button_pressed[kGamepadVirtualLeftStickRight] = (filtered_axes[GLFW_GAMEPAD_AXIS_LEFT_X] > stick_threshold);
			button_pressed[kGamepadVirtualRightStickUp] = (filtered_axes[GLFW_GAMEPAD_AXIS_RIGHT_Y] < -stick_threshold);
			button_pressed[kGamepadVirtualRightStickDown] = (filtered_axes[GLFW_GAMEPAD_AXIS_RIGHT_Y] > stick_threshold);
			button_pressed[kGamepadVirtualRightStickLeft] = (filtered_axes[GLFW_GAMEPAD_AXIS_RIGHT_X] < -stick_threshold);
			button_pressed[kGamepadVirtualRightStickRight] = (filtered_axes[GLFW_GAMEPAD_AXIS_RIGHT_X] > stick_threshold);
			button_pressed[kGamepadVirtualLeftTrigger] = (filtered_axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER] > trigger_threshold);
			button_pressed[kGamepadVirtualRightTrigger] = (filtered_axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER] > trigger_threshold);

			for (size_t b = 0; b < button_pressed.size(); ++b) {
				const bool down = button_pressed[b];
				if (down && !button_prev[b]) {
					button_press_time[b] = current_time;
				} else if (!down && button_prev[b]) {
					button_release_time[b] = current_time;
				}
			}
		}
	}

	[[nodiscard]] bool is_button_pressed(int button) const noexcept {
		if (button < 0 || button >= static_cast<int>(button_pressed.size())) return false;
		return button_pressed[static_cast<size_t>(button)];
	}

	[[nodiscard]] bool matches_binding(const GamepadBinding& binding) const noexcept {
		if (binding.empty() || !connected) return false;
		if (binding.button != static_cast<int>(GamepadButton::None) && !is_button_pressed(binding.button)) return false;
		if (binding.modifier_button != static_cast<int>(GamepadButton::None) && !is_button_pressed(binding.modifier_button)) return false;
		return true;
	}

	[[nodiscard]] bool evaluate_action(size_t action_idx, const GamepadBinding& binding, double current_time) noexcept {
		if (action_idx >= action_toggle_state.size() || binding.empty() || !connected) return false;
		const bool raw = matches_binding(binding);
		const bool prev = action_prev_raw[action_idx];

		if (raw && !prev) {
			action_press_time[action_idx] = current_time;
		}

		bool triggered = false;
		switch (binding.mode) {
			case 1:
				if (raw && !prev) {
					action_toggle_state[action_idx] = !action_toggle_state[action_idx];
				}
				triggered = action_toggle_state[action_idx];
				break;
			case 2:
				if (!raw && prev) {
					const double duration = current_time - action_press_time[action_idx];
					if (duration <= static_cast<double>(binding.tap_duration_seconds)) {
						triggered = true;
					}
				}
				break;
			case 3:
				if (!raw && prev) {
					const double duration = current_time - action_press_time[action_idx];
					if (duration <= static_cast<double>(binding.tap_duration_seconds)) {
						const double since_last = current_time - action_release_time[action_idx];
						if (since_last <= static_cast<double>(binding.double_tap_window_seconds)) {
							triggered = true;
							action_tap_count[action_idx] = 0;
						} else {
							action_tap_count[action_idx] = 1;
						}
					}
					action_release_time[action_idx] = current_time;
				}
				break;
			case 0:
			default:
				triggered = raw;
				break;
		}

		if (!raw && prev) {
			action_release_time[action_idx] = current_time;
		}
		action_prev_raw[action_idx] = raw;
		action_active[action_idx] = triggered;
		return triggered;
	}

	[[nodiscard]] bool is_action_active(size_t action_idx) const noexcept {
		if (action_idx >= action_active.size()) return false;
		return action_active[action_idx];
	}

	[[nodiscard]] GamepadMotionDeltas calculate_motion(const GamepadAxesConfig& cfg, float dt) const noexcept {
		GamepadMotionDeltas deltas{};
		if (!connected) return deltas;

		const float lx = filtered_axes[GLFW_GAMEPAD_AXIS_LEFT_X];
		const float ly = filtered_axes[GLFW_GAMEPAD_AXIS_LEFT_Y];
		const float rx = filtered_axes[GLFW_GAMEPAD_AXIS_RIGHT_X];
		const float ry = filtered_axes[GLFW_GAMEPAD_AXIS_RIGHT_Y];
		const float lt = filtered_axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER];
		const float rt = filtered_axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER];

		deltas.move_right = lx * cfg.move_sensitivity;
		deltas.move_forward = -ly * cfg.move_sensitivity;

		const float vert_mult = cfg.invert_vertical ? -1.0f : 1.0f;
		deltas.move_up = (rt - lt) * vert_mult * cfg.move_sensitivity;

		const float yaw_mult = cfg.invert_look_x ? -1.0f : 1.0f;
		const float pitch_mult = cfg.invert_look_y ? -1.0f : 1.0f;
		deltas.look_yaw_deg = -rx * cfg.look_sensitivity_x * yaw_mult * dt;
		deltas.look_pitch_deg = -ry * cfg.look_sensitivity_y * pitch_mult * dt;

		float roll_val = 0.0f;
		if (is_button_pressed(GLFW_GAMEPAD_BUTTON_LEFT_BUMPER)) roll_val -= 1.0f;
		if (is_button_pressed(GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER)) roll_val += 1.0f;
		const float roll_mult = cfg.invert_roll ? -1.0f : 1.0f;
		deltas.roll_deg = roll_val * cfg.roll_sensitivity * roll_mult * dt;

		deltas.sprint = is_button_pressed(GLFW_GAMEPAD_BUTTON_LEFT_THUMB);
		deltas.crawl = is_button_pressed(GLFW_GAMEPAD_BUTTON_RIGHT_THUMB);
		deltas.zoom = is_button_pressed(GLFW_GAMEPAD_BUTTON_X);

		return deltas;
	}
};

inline void GamepadConfig::reset_to_defaults() noexcept {
	bindings.fill(GamepadBinding{});
	bindings[static_cast<size_t>(InputAction::MoveForward)] = GamepadBinding{kGamepadVirtualLeftStickUp, static_cast<int>(GamepadButton::None), 0};
	bindings[static_cast<size_t>(InputAction::MoveBackward)] = GamepadBinding{kGamepadVirtualLeftStickDown, static_cast<int>(GamepadButton::None), 0};
	bindings[static_cast<size_t>(InputAction::MoveLeft)] = GamepadBinding{kGamepadVirtualLeftStickLeft, static_cast<int>(GamepadButton::None), 0};
	bindings[static_cast<size_t>(InputAction::MoveRight)] = GamepadBinding{kGamepadVirtualLeftStickRight, static_cast<int>(GamepadButton::None), 0};
	bindings[static_cast<size_t>(InputAction::LookUp)] = GamepadBinding{kGamepadVirtualRightStickUp, static_cast<int>(GamepadButton::None), 0};
	bindings[static_cast<size_t>(InputAction::LookDown)] = GamepadBinding{kGamepadVirtualRightStickDown, static_cast<int>(GamepadButton::None), 0};
	bindings[static_cast<size_t>(InputAction::LookLeft)] = GamepadBinding{kGamepadVirtualRightStickLeft, static_cast<int>(GamepadButton::None), 0};
	bindings[static_cast<size_t>(InputAction::LookRight)] = GamepadBinding{kGamepadVirtualRightStickRight, static_cast<int>(GamepadButton::None), 0};
	bindings[static_cast<size_t>(InputAction::RollLeft)] = GamepadBinding{GLFW_GAMEPAD_BUTTON_LEFT_BUMPER, static_cast<int>(GamepadButton::None), 0};
	bindings[static_cast<size_t>(InputAction::RollRight)] = GamepadBinding{GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER, static_cast<int>(GamepadButton::None), 0};
	bindings[static_cast<size_t>(InputAction::MoveUp)] = GamepadBinding{kGamepadVirtualRightTrigger, static_cast<int>(GamepadButton::None), 0};
	bindings[static_cast<size_t>(InputAction::MoveDown)] = GamepadBinding{kGamepadVirtualLeftTrigger, static_cast<int>(GamepadButton::None), 0};
	bindings[static_cast<size_t>(InputAction::Sprint)] = GamepadBinding{GLFW_GAMEPAD_BUTTON_LEFT_THUMB, static_cast<int>(GamepadButton::None), 0};
	bindings[static_cast<size_t>(InputAction::Crawl)] = GamepadBinding{GLFW_GAMEPAD_BUTTON_RIGHT_THUMB, static_cast<int>(GamepadButton::None), 0};
	bindings[static_cast<size_t>(InputAction::ResetRoll)] = GamepadBinding{GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER, GLFW_GAMEPAD_BUTTON_LEFT_BUMPER, 2};
	bindings[static_cast<size_t>(InputAction::TogglePausePlay)] = GamepadBinding{GLFW_GAMEPAD_BUTTON_START, static_cast<int>(GamepadButton::None), 2};
	bindings[static_cast<size_t>(InputAction::SingleStepTick)] = GamepadBinding{GLFW_GAMEPAD_BUTTON_A, static_cast<int>(GamepadButton::None), 2};
	bindings[static_cast<size_t>(InputAction::CycleProjectionMode)] = GamepadBinding{GLFW_GAMEPAD_BUTTON_X, static_cast<int>(GamepadButton::None), 2};
	bindings[static_cast<size_t>(InputAction::FovIncrease)] = GamepadBinding{GLFW_GAMEPAD_BUTTON_DPAD_UP, static_cast<int>(GamepadButton::None), 0};
	bindings[static_cast<size_t>(InputAction::FovDecrease)] = GamepadBinding{GLFW_GAMEPAD_BUTTON_DPAD_DOWN, static_cast<int>(GamepadButton::None), 0};
}

[[nodiscard]] inline std::string format_gamepad_binding(const GamepadBinding& binding) noexcept {
	if (binding.empty()) return "---";
	std::string s;
	if (binding.modifier_button != static_cast<int>(GamepadButton::None)) {
		s += gamepad_button_name(binding.modifier_button);
		s += "+";
	}
	s += gamepad_button_name(binding.button);
	return s;
}

}

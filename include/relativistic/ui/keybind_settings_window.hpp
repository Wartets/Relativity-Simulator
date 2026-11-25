#pragma once

#include "relativistic/ui/camera_control_config.hpp"
#include "relativistic/ui/gamepad_input.hpp"
#include "relativistic/ui/hud/hud_layout_config.hpp"
#include "relativistic/ui/tooltip_utils.hpp"
#include <imgui.h>
#include <GLFW/glfw3.h>
#include <string>
#include <string_view>
#include <vector>
#include <array>
#include <algorithm>
#include <cctype>
#include <cstring>

namespace Relativistic::UI {

class KeybindSettingsWindow {
private:
	bool is_open_{false};
	CameraControlConfig* config_{nullptr};
	HudLayoutConfig* hud_layout_{nullptr};
	int listening_action_{-1};
	int listening_slot_{0};
	int listening_gamepad_action_{-1};
	int chord_modifier_{GLFW_KEY_UNKNOWN};
	int chord_modifier2_{GLFW_KEY_UNKNOWN};
	std::string conflict_message_{};
	char search_buffer_[64]{};
	int category_filter_{-1};
	int sort_mode_{0};
	int settings_tab_{0};
	GamepadState live_gamepad_{};

	static constexpr int kBindableKeys[] = {
		GLFW_KEY_A, GLFW_KEY_B, GLFW_KEY_C, GLFW_KEY_D, GLFW_KEY_E, GLFW_KEY_F, GLFW_KEY_G, GLFW_KEY_H,
		GLFW_KEY_I, GLFW_KEY_J, GLFW_KEY_K, GLFW_KEY_L, GLFW_KEY_M, GLFW_KEY_N, GLFW_KEY_O, GLFW_KEY_P,
		GLFW_KEY_Q, GLFW_KEY_R, GLFW_KEY_S, GLFW_KEY_T, GLFW_KEY_U, GLFW_KEY_V, GLFW_KEY_W, GLFW_KEY_X,
		GLFW_KEY_Y, GLFW_KEY_Z,
		GLFW_KEY_0, GLFW_KEY_1, GLFW_KEY_2, GLFW_KEY_3, GLFW_KEY_4, GLFW_KEY_5, GLFW_KEY_6, GLFW_KEY_7,
		GLFW_KEY_8, GLFW_KEY_9,
		GLFW_KEY_F1, GLFW_KEY_F2, GLFW_KEY_F3, GLFW_KEY_F4, GLFW_KEY_F5, GLFW_KEY_F6, GLFW_KEY_F7, GLFW_KEY_F8,
		GLFW_KEY_F9, GLFW_KEY_F10, GLFW_KEY_F11, GLFW_KEY_F12,
		GLFW_KEY_SPACE, GLFW_KEY_TAB, GLFW_KEY_LEFT_SHIFT, GLFW_KEY_RIGHT_SHIFT,
		GLFW_KEY_LEFT_CONTROL, GLFW_KEY_RIGHT_CONTROL, GLFW_KEY_LEFT_ALT, GLFW_KEY_RIGHT_ALT,
		GLFW_KEY_UP, GLFW_KEY_DOWN, GLFW_KEY_LEFT, GLFW_KEY_RIGHT,
		GLFW_KEY_PAGE_UP, GLFW_KEY_PAGE_DOWN, GLFW_KEY_HOME, GLFW_KEY_END,
		GLFW_KEY_LEFT_BRACKET, GLFW_KEY_RIGHT_BRACKET, GLFW_KEY_SEMICOLON, GLFW_KEY_APOSTROPHE,
		GLFW_KEY_COMMA, GLFW_KEY_PERIOD, GLFW_KEY_SLASH, GLFW_KEY_BACKSLASH, GLFW_KEY_MINUS, GLFW_KEY_EQUAL,
		GLFW_KEY_GRAVE_ACCENT, GLFW_KEY_CAPS_LOCK,
		GLFW_KEY_KP_0, GLFW_KEY_KP_1, GLFW_KEY_KP_2, GLFW_KEY_KP_3, GLFW_KEY_KP_4,
		GLFW_KEY_KP_5, GLFW_KEY_KP_6, GLFW_KEY_KP_7, GLFW_KEY_KP_8, GLFW_KEY_KP_9,
		GLFW_KEY_KP_ENTER, GLFW_KEY_DELETE, GLFW_KEY_INSERT
	};

	struct ConflictInfo {
		std::array<bool, static_cast<size_t>(InputAction::Count)> primary_conflict{};
		std::array<bool, static_cast<size_t>(InputAction::Count)> secondary_conflict{};
		size_t total_conflicts{0};
	};

	[[nodiscard]] ConflictInfo compute_conflicts() const noexcept {
		ConflictInfo info{};
		const auto& bindings = config_->keybinds.raw_bindings();
		for (size_t i = 0; i < bindings.size(); ++i) {
			const int prim = bindings[i].primary_key;
			if (prim != GLFW_KEY_UNKNOWN) {
				for (size_t j = 0; j < bindings.size(); ++j) {
					if (i == j) continue;
					if ((bindings[j].primary_key == prim && bindings[j].primary_modifier == bindings[i].primary_modifier && bindings[j].primary_modifier2 == bindings[i].primary_modifier2) ||
						(bindings[j].secondary_key == prim && bindings[j].secondary_modifier == bindings[i].primary_modifier && bindings[j].secondary_modifier2 == bindings[i].primary_modifier2)) {
						info.primary_conflict[i] = true;
						break;
					}
				}
			}
			const int sec = bindings[i].secondary_key;
			if (sec != GLFW_KEY_UNKNOWN) {
				for (size_t j = 0; j < bindings.size(); ++j) {
					if (i == j) continue;
					if ((bindings[j].primary_key == sec && bindings[j].primary_modifier == bindings[i].secondary_modifier && bindings[j].primary_modifier2 == bindings[i].secondary_modifier2) ||
						(bindings[j].secondary_key == sec && bindings[j].secondary_modifier == bindings[i].secondary_modifier && bindings[j].secondary_modifier2 == bindings[i].secondary_modifier2)) {
						info.secondary_conflict[i] = true;
						break;
					}
				}
			}
			if (info.primary_conflict[i] || info.secondary_conflict[i]) {
				++info.total_conflicts;
			}
		}
		return info;
	}

	[[nodiscard]] static bool matches_search(InputAction action, std::string_view query) noexcept {
		if (query.empty()) return true;
		std::string haystack(input_action_name(action));
		std::string needle(query);
		std::transform(haystack.begin(), haystack.end(), haystack.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		std::transform(needle.begin(), needle.end(), needle.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return haystack.find(needle) != std::string::npos;
	}

	[[nodiscard]] static bool action_supports_activation_mode(InputAction action, InputActivationMode mode) noexcept {
		return Relativistic::UI::action_supports_activation_mode(action, mode);
	}

	[[nodiscard]] static bool is_modifier_key(int key) noexcept {
		return key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_RIGHT_CONTROL ||
			   key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT ||
			   key == GLFW_KEY_LEFT_ALT || key == GLFW_KEY_RIGHT_ALT;
	}

	void render_capture_banner(GLFWwindow* window) noexcept {
		if (listening_action_ < 0) return;

		std::string chord_prompt;
		if (chord_modifier_ != GLFW_KEY_UNKNOWN) {
			chord_prompt += glfw_key_display_name(chord_modifier_, config_->keyboard_layout);
			chord_prompt += "+";
		}
		if (chord_modifier2_ != GLFW_KEY_UNKNOWN) {
			chord_prompt += glfw_key_display_name(chord_modifier2_, config_->keyboard_layout);
			chord_prompt += "+";
		}

		ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f),
			"Binding '%s' (%s slot): Hold modifiers and press a key. Current: [%s...]. ESC to cancel.",
			std::string(input_action_name(static_cast<InputAction>(listening_action_))).c_str(),
			listening_slot_ == 0 ? "primary" : "secondary",
			chord_prompt.empty() ? "" : chord_prompt.c_str());

		if (window == nullptr) return;

		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			listening_action_ = -1;
			chord_modifier_ = GLFW_KEY_UNKNOWN;
			chord_modifier2_ = GLFW_KEY_UNKNOWN;
			return;
		}

		const bool ctrl = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;
		const bool shift = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
		const bool alt = glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS;

		int m1 = GLFW_KEY_UNKNOWN;
		int m2 = GLFW_KEY_UNKNOWN;
		if (ctrl) m1 = GLFW_KEY_LEFT_CONTROL;
		if (shift) {
			if (m1 == GLFW_KEY_UNKNOWN) m1 = GLFW_KEY_LEFT_SHIFT;
			else m2 = GLFW_KEY_LEFT_SHIFT;
		}
		if (alt) {
			if (m1 == GLFW_KEY_UNKNOWN) m1 = GLFW_KEY_LEFT_ALT;
			else if (m2 == GLFW_KEY_UNKNOWN) m2 = GLFW_KEY_LEFT_ALT;
		}
		chord_modifier_ = m1;
		chord_modifier2_ = m2;

		for (int key : kBindableKeys) {
			if (is_modifier_key(key)) continue;
			if (glfwGetKey(window, key) != GLFW_PRESS) continue;

			const auto action = static_cast<InputAction>(listening_action_);
			if (listening_slot_ == 0) {
				config_->keybinds.set_primary_chord(action, key, chord_modifier_, chord_modifier2_);
			} else {
				config_->keybinds.set_secondary_chord(action, key, chord_modifier_, chord_modifier2_);
			}
			conflict_message_ = "Bound combination: " + format_single_chord(key, chord_modifier_, chord_modifier2_, config_->keyboard_layout);
			listening_action_ = -1;
			chord_modifier_ = GLFW_KEY_UNKNOWN;
			chord_modifier2_ = GLFW_KEY_UNKNOWN;
			break;
		}
	}

	void render_action_row(InputAction action, const ConflictInfo& conflicts) noexcept {
		const size_t idx = static_cast<size_t>(action);
		const auto& binding = config_->keybinds.get(action);
		const bool has_conflict = conflicts.primary_conflict[idx] || conflicts.secondary_conflict[idx];

		ImGui::PushID(static_cast<int>(idx));

		if (has_conflict) {
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.55f, 0.35f, 1.0f));
		}
		ImGui::TextUnformatted(std::string(input_action_name(action)).c_str());
		if (has_conflict) {
			ImGui::PopStyleColor();
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(1.0f, 0.55f, 0.35f, 1.0f), "[!]");
			render_setting_tooltip_warning(
				"This action shares a key with at least one other action.",
				"Rebind one of the conflicting actions to avoid unpredictable input handling."
			);
		}

		ImGui::SameLine(260.0f);
		const bool prim_conflict = conflicts.primary_conflict[idx];
		if (prim_conflict) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.55f, 0.25f, 0.15f, 1.0f));
		const std::string primary_label = std::string(glfw_key_display_name(binding.primary_key, config_->keyboard_layout)) + "##primary";
		if (ImGui::Button(primary_label.c_str(), ImVec2(100.0f, 0.0f))) {
			listening_action_ = static_cast<int>(idx);
			listening_slot_ = 0;
			conflict_message_.clear();
		}
		if (prim_conflict) ImGui::PopStyleColor();

		ImGui::SameLine();
		const bool sec_conflict = conflicts.secondary_conflict[idx];
		if (sec_conflict) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.55f, 0.25f, 0.15f, 1.0f));
		const std::string secondary_label = std::string(glfw_key_display_name(binding.secondary_key, config_->keyboard_layout)) + "##secondary";
		if (ImGui::Button(secondary_label.c_str(), ImVec2(100.0f, 0.0f))) {
			listening_action_ = static_cast<int>(idx);
			listening_slot_ = 1;
			conflict_message_.clear();
		}
		if (sec_conflict) ImGui::PopStyleColor();

		ImGui::SameLine();
		if (ImGui::SmallButton("Clear")) {
			config_->keybinds.set(action, GLFW_KEY_UNKNOWN, GLFW_KEY_UNKNOWN);
		}

		ImGui::SameLine();
		if (ImGui::SmallButton("Default")) {
			config_->keybinds.reset_to_default(action, config_->keyboard_layout);
		}

		if (hud_layout_ != nullptr) {
			ImGui::SameLine();
			bool visible_in_hud = hud_layout_->keybind_summary_visible[idx];
			if (ImGui::Checkbox("Show In HUD", &visible_in_hud)) {
				hud_layout_->keybind_summary_visible[idx] = visible_in_hud;
			}
		}

		ImGui::SameLine();
		const char* all_mode_names[] = {"Hold", "Toggle", "Tap", "Double-Tap"};
		std::vector<const char*> available_names;
		std::vector<InputActivationMode> available_modes;
		for (uint32_t m = 0; m <= 3; ++m) {
			const auto mode_candidate = static_cast<InputActivationMode>(m);
			if (action_supports_activation_mode(action, mode_candidate)) {
				available_names.push_back(all_mode_names[m]);
				available_modes.push_back(mode_candidate);
			}
		}

		int current_mode_pos = 0;
		for (size_t m = 0; m < available_modes.size(); ++m) {
			if (available_modes[m] == binding.mode) {
				current_mode_pos = static_cast<int>(m);
				break;
			}
		}

		ImGui::SetNextItemWidth(105.0f);
		if (ImGui::Combo("##activation_mode", &current_mode_pos, available_names.data(), static_cast<int>(available_names.size()))) {
			config_->keybinds.set_mode(action, available_modes[static_cast<size_t>(current_mode_pos)]);
		}
		render_setting_tooltip("Activation policy: Hold requires holding the key continuously. Toggle keeps the action active until pressed again. Tap triggers on a brief click. Double-Tap requires two quick presses.");

		if (binding.mode == InputActivationMode::Tap || binding.mode == InputActivationMode::DoubleTap) {
			ImGui::SameLine();
			float tap_d = binding.tap_duration_seconds;
			ImGui::SetNextItemWidth(70.0f);
			if (ImGui::SliderFloat("##tap_dur", &tap_d, 0.05f, 0.60f, "%.2fs")) {
				config_->keybinds.set_timing(action, tap_d, binding.double_tap_window_seconds);
			}
			render_setting_tooltip("Maximum duration in seconds for a key press to be registered as a quick tap.");
		}

		if (binding.mode == InputActivationMode::DoubleTap) {
			ImGui::SameLine();
			float dbl_w = binding.double_tap_window_seconds;
			ImGui::SetNextItemWidth(70.0f);
			if (ImGui::SliderFloat("##dbl_win", &dbl_w, 0.10f, 0.80f, "%.2fs")) {
				config_->keybinds.set_timing(action, binding.tap_duration_seconds, dbl_w);
			}
			render_setting_tooltip("Maximum time window between consecutive taps to trigger a double-tap.");
		}

		ImGui::PopID();
	}

void render_gamepad_capture_banner() noexcept {
		if (listening_gamepad_action_ < 0) return;
		int held_mod = static_cast<int>(GamepadButton::None);
		for (size_t b = 0; b < live_gamepad_.button_pressed.size(); ++b) {
			if (live_gamepad_.button_pressed[b] && live_gamepad_.button_prev[b]) {
				held_mod = static_cast<int>(b);
				break;
			}
		}
		if (held_mod != static_cast<int>(GamepadButton::None)) {
			ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f),
				"Chord: Holding [%s] + press another button for '%s'. Click 'Cancel' or press ESC.",
				gamepad_button_name(held_mod),
				std::string(input_action_name(static_cast<InputAction>(listening_gamepad_action_))).c_str());
		} else {
			ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f),
				"Binding Gamepad for '%s': Press any button, stick direction, trigger, or hold for chord. Click 'Cancel' or press ESC.",
				std::string(input_action_name(static_cast<InputAction>(listening_gamepad_action_))).c_str());
		}

		ImGui::SameLine();
		if (ImGui::Button("Cancel")) {
			listening_gamepad_action_ = -1;
			return;
		}

		if (!live_gamepad_.connected) return;

		for (size_t b = 0; b < live_gamepad_.button_pressed.size(); ++b) {
			if (live_gamepad_.button_pressed[b] && !live_gamepad_.button_prev[b]) {
				const size_t act_idx = static_cast<size_t>(listening_gamepad_action_);
				if (act_idx < config_->gamepad.bindings.size()) {
					config_->gamepad.bindings[act_idx].button = static_cast<int>(b);
					config_->gamepad.bindings[act_idx].modifier_button = (held_mod != static_cast<int>(b)) ? held_mod : static_cast<int>(GamepadButton::None);
				}
				listening_gamepad_action_ = -1;
				break;
			}
		}
	}

	void render_gamepad_tab() noexcept {
		ImGui::TextColored(ImVec4(0.3f, 0.9f, 1.0f, 1.0f), "Gamepad & Space-Flight Controller Support");
		ImGui::Checkbox("Enable Gamepad Input", &config_->gamepad.enabled);
		render_setting_tooltip("Enables analogue stick navigation, trigger thrusters, and controller keybind evaluation.");

		ImGui::Separator();
		if (live_gamepad_.connected) {
			ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "Connected: %s (ID: %d)", live_gamepad_.name.c_str(), live_gamepad_.joystick_id);
		} else {
			ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "No gamepad detected. Connect an Xbox, PlayStation, or generic controller via USB or Bluetooth.");
		}

		std::vector<int> joystick_ids;
		std::vector<std::string> joystick_labels;
		joystick_ids.push_back(-1);
		joystick_labels.push_back("Auto-Select (First Active Controller)");
		for (int jid = GLFW_JOYSTICK_1; jid <= GLFW_JOYSTICK_LAST; ++jid) {
			if (glfwJoystickPresent(jid) && glfwJoystickIsGamepad(jid)) {
				joystick_ids.push_back(jid);
				const char* jname = glfwGetGamepadName(jid);
				joystick_labels.push_back(std::string("Controller ") + std::to_string(jid) + ": " + (jname ? jname : "Gamepad"));
			}
		}
		int current_joystick_index = 0;
		for (size_t j = 0; j < joystick_ids.size(); ++j) {
			if (joystick_ids[j] == config_->gamepad.selected_joystick) {
				current_joystick_index = static_cast<int>(j);
				break;
			}
		}
		std::vector<const char*> joystick_label_ptrs;
		joystick_label_ptrs.reserve(joystick_labels.size());
		for (const auto& label : joystick_labels) joystick_label_ptrs.push_back(label.c_str());
		if (ImGui::Combo("Assigned Controller Device", &current_joystick_index, joystick_label_ptrs.data(), static_cast<int>(joystick_label_ptrs.size()))) {
			config_->gamepad.selected_joystick = joystick_ids[static_cast<size_t>(current_joystick_index)];
		}
		render_setting_tooltip("Locks input listening and navigation to a specific plugged-in gamepad to prevent multi-controller crosstalk.");

		if (hud_layout_ != nullptr) {
			ImGui::Checkbox("Show Controller Keybinds In Viewport HUD Summary", &hud_layout_->show_gamepad_in_summary);
			render_setting_tooltip("Includes active gamepad bindings in the floating HUD Navigation Controls panel.");
		}

		if (ImGui::Button("Reset Gamepad To Defaults", ImVec2(260.0f, 26.0f))) {
			config_->gamepad.reset_to_defaults();
		}
		render_setting_tooltip("Restores all gamepad bindings to default settings.");

		ImGui::Separator();
		if (ImGui::CollapsingHeader("Controller Calibration & Deadzones", ImGuiTreeNodeFlags_DefaultOpen)) {
			auto& axes = config_->gamepad.axes;
			ImGui::SliderFloat("Left Stick Deadzone", &axes.left_stick_deadzone, 0.02f, 0.40f, "%.2f");
			ImGui::SliderFloat("Right Stick Deadzone", &axes.right_stick_deadzone, 0.02f, 0.40f, "%.2f");
			ImGui::SliderFloat("Triggers Deadzone", &axes.trigger_deadzone, 0.02f, 0.30f, "%.2f");
			ImGui::SliderFloat("Look Sensitivity X", &axes.look_sensitivity_x, 10.0f, 200.0f, "%.1f deg/s");
			ImGui::SliderFloat("Look Sensitivity Y", &axes.look_sensitivity_y, 10.0f, 200.0f, "%.1f deg/s");
			ImGui::SliderFloat("Roll Sensitivity", &axes.roll_sensitivity, 10.0f, 180.0f, "%.1f deg/s");
			ImGui::SliderFloat("Movement Sensitivity", &axes.move_sensitivity, 0.2f, 3.0f, "%.2fx");
			ImGui::Checkbox("Invert Pitch (Y)", &axes.invert_look_y);
			ImGui::SameLine();
			ImGui::Checkbox("Invert Yaw (X)", &axes.invert_look_x);
			ImGui::SameLine();
			ImGui::Checkbox("Invert Roll", &axes.invert_roll);
			ImGui::SameLine();
			ImGui::Checkbox("Invert Vertical Thrust", &axes.invert_vertical);

			if (live_gamepad_.connected) {
				ImGui::Text("Live Inputs: LS (%.2f, %.2f) | RS (%.2f, %.2f) | LT: %.2f | RT: %.2f",
					static_cast<double>(live_gamepad_.filtered_axes[GLFW_GAMEPAD_AXIS_LEFT_X]),
					static_cast<double>(live_gamepad_.filtered_axes[GLFW_GAMEPAD_AXIS_LEFT_Y]),
					static_cast<double>(live_gamepad_.filtered_axes[GLFW_GAMEPAD_AXIS_RIGHT_X]),
					static_cast<double>(live_gamepad_.filtered_axes[GLFW_GAMEPAD_AXIS_RIGHT_Y]),
					static_cast<double>(live_gamepad_.filtered_axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER]),
					static_cast<double>(live_gamepad_.filtered_axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER]));
			}
		}

		ImGui::Separator();
		render_gamepad_capture_banner();

		ImGui::TextColored(ImVec4(0.5f, 0.85f, 1.0f, 1.0f), "Gamepad Action Bindings:");
		for (size_t i = 0; i < static_cast<size_t>(InputAction::Count); ++i) {
			if (i >= config_->gamepad.bindings.size()) break;
			const auto action = static_cast<InputAction>(i);
			auto& binding = config_->gamepad.bindings[i];

			ImGui::PushID(static_cast<int>(i) + 50000);
			ImGui::TextUnformatted(std::string(input_action_name(action)).c_str());
			ImGui::SameLine(260.0f);

			const std::string btn_label = format_gamepad_binding(binding) + "##gpbtn";
			if (ImGui::Button(btn_label.c_str(), ImVec2(130.0f, 0.0f))) {
				listening_gamepad_action_ = static_cast<int>(i);
			}

			ImGui::SameLine();
			if (ImGui::SmallButton("Clear")) {
				binding.button = static_cast<int>(GamepadButton::None);
				binding.modifier_button = static_cast<int>(GamepadButton::None);
			}

			if (hud_layout_ != nullptr) {
				ImGui::SameLine();
				bool visible_in_hud = hud_layout_->gamepad_summary_visible[i];
				if (ImGui::Checkbox("Show In HUD", &visible_in_hud)) {
					hud_layout_->gamepad_summary_visible[i] = visible_in_hud;
				}
			}

			ImGui::SameLine();
			const char* all_mode_names[] = {"Hold", "Toggle", "Tap", "Double-Tap"};
			std::vector<const char*> available_names;
			std::vector<InputActivationMode> available_modes;
			for (uint32_t m = 0; m <= 3; ++m) {
				const auto candidate = static_cast<InputActivationMode>(m);
				if (action_supports_activation_mode(action, candidate)) {
					available_names.push_back(all_mode_names[m]);
					available_modes.push_back(candidate);
				}
			}

			int current_mode_pos = 0;
			for (size_t m = 0; m < available_modes.size(); ++m) {
				if (static_cast<uint32_t>(available_modes[m]) == binding.mode) {
					current_mode_pos = static_cast<int>(m);
					break;
				}
			}

			ImGui::SetNextItemWidth(105.0f);
			if (ImGui::Combo("##gpmode", &current_mode_pos, available_names.data(), static_cast<int>(available_names.size()))) {
				binding.mode = static_cast<uint32_t>(available_modes[static_cast<size_t>(current_mode_pos)]);
			}

			if (binding.mode == static_cast<uint32_t>(InputActivationMode::Tap) || binding.mode == static_cast<uint32_t>(InputActivationMode::DoubleTap)) {
				ImGui::SameLine();
				float tap_d = binding.tap_duration_seconds;
				ImGui::SetNextItemWidth(70.0f);
				if (ImGui::SliderFloat("##gptap", &tap_d, 0.05f, 0.60f, "%.2fs")) {
					binding.tap_duration_seconds = tap_d;
				}
			}

			if (binding.mode == static_cast<uint32_t>(InputActivationMode::DoubleTap)) {
				ImGui::SameLine();
				float dbl_w = binding.double_tap_window_seconds;
				ImGui::SetNextItemWidth(70.0f);
				if (ImGui::SliderFloat("##gpdbl", &dbl_w, 0.10f, 0.80f, "%.2fs")) {
					binding.double_tap_window_seconds = dbl_w;
				}
			}

			ImGui::PopID();
		}
	}

public:
	explicit KeybindSettingsWindow(CameraControlConfig& config) noexcept
		: config_(&config) {}

	void attach_hud_layout(HudLayoutConfig& hud_layout) noexcept {
		hud_layout_ = &hud_layout;
	}

	[[nodiscard]] bool& open_state() noexcept {
		return is_open_;
	}

	[[nodiscard]] bool is_open() const noexcept {
		return is_open_;
	}

	void render(GLFWwindow* window) {
		if (!is_open_) return;

		const double cur_time = ImGui::GetTime();
		live_gamepad_.update(config_->gamepad.selected_joystick, config_->gamepad.axes, cur_time);

		ImGui::SetNextWindowSize(ImVec2(740.0f, 660.0f), ImGuiCond_FirstUseEver);
		if (!ImGui::Begin("Keybind Settings", &is_open_)) {
			ImGui::End();
			return;
		}

		if (ImGui::BeginTabBar("KeybindTabs")) {
			if (ImGui::BeginTabItem("Keyboard Chords")) {
				settings_tab_ = 0;
				render_keyboard_tab(window);
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Gamepad Controller")) {
				settings_tab_ = 1;
				render_gamepad_tab();
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}

		ImGui::End();
	}

private:
	void render_keyboard_tab(GLFWwindow* window) noexcept {
		ImGui::TextColored(ImVec4(0.3f, 0.9f, 1.0f, 1.0f), "Keyboard Layout");
		int layout_idx = static_cast<int>(config_->keyboard_layout);
		const char* layouts[] = {"QWERTY", "AZERTY"};
		if (ImGui::Combo("Layout Preset", &layout_idx, layouts, IM_ARRAYSIZE(layouts))) {
			config_->apply_keyboard_layout(static_cast<KeyboardLayout>(layout_idx));
			listening_action_ = -1;
			conflict_message_.clear();
		}
		render_setting_tooltip("Applies the default movement bindings for the selected physical keyboard layout. Custom rebinds made afterward are preserved independently until this preset is reapplied.");
		ImGui::TextWrapped("Switching layout resets every binding below to that layout's defaults. Individual bindings can still be customized afterward and are saved independently.");

		ImGui::Separator();

		render_capture_banner(window);
		if (!conflict_message_.empty()) {
			ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "%s", conflict_message_.c_str());
		}

		ImGui::Separator();

		ImGui::SetNextItemWidth(220.0f);
		ImGui::InputTextWithHint("##KeybindSearch", "Search actions or keys...", search_buffer_, sizeof(search_buffer_));
		ImGui::SameLine();

		const char* category_options[] = {
			"All Categories", "Movement", "Camera Orientation", "Camera Framing",
			"Quick Snap Positions", "Simulation Control", "Spacetime Model",
			"Rendering Quality", "Interface Windows"
		};
		int category_combo_idx = category_filter_ + 1;
		ImGui::SetNextItemWidth(200.0f);
		if (ImGui::Combo("##CategoryFilter", &category_combo_idx, category_options, IM_ARRAYSIZE(category_options))) {
			category_filter_ = category_combo_idx - 1;
		}
		ImGui::SameLine();

		const char* sort_options[] = {"Sort: By Category", "Sort: Alphabetical", "Sort: Conflicts First"};
		ImGui::SetNextItemWidth(190.0f);
		ImGui::Combo("##SortMode", &sort_mode_, sort_options, IM_ARRAYSIZE(sort_options));

		const auto conflicts = compute_conflicts();
		if (conflicts.total_conflicts > 0) {
			ImGui::TextColored(ImVec4(1.0f, 0.55f, 0.35f, 1.0f), "%zu action(s) have conflicting key assignments.", conflicts.total_conflicts);
		} else {
			ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.5f, 1.0f), "No key conflicts detected.");
		}

		ImGui::Separator();

		std::vector<InputAction> visible_actions;
		visible_actions.reserve(static_cast<size_t>(InputAction::Count));
		const std::string_view search_view(search_buffer_);
		for (size_t i = 0; i < static_cast<size_t>(InputAction::Count); ++i) {
			const auto action = static_cast<InputAction>(i);
			if (category_filter_ >= 0 && static_cast<int>(input_action_category(action)) != category_filter_) continue;
			if (!matches_search(action, search_view)) continue;
			visible_actions.push_back(action);
		}

		if (sort_mode_ == 1) {
			std::sort(visible_actions.begin(), visible_actions.end(), [](InputAction a, InputAction b) {
				return input_action_name(a) < input_action_name(b);
			});
		} else if (sort_mode_ == 2) {
			std::sort(visible_actions.begin(), visible_actions.end(), [&](InputAction a, InputAction b) {
				const size_t ia = static_cast<size_t>(a);
				const size_t ib = static_cast<size_t>(b);
				const bool ca = conflicts.primary_conflict[ia] || conflicts.secondary_conflict[ia];
				const bool cb = conflicts.primary_conflict[ib] || conflicts.secondary_conflict[ib];
				if (ca != cb) return ca && !cb;
				return input_action_name(a) < input_action_name(b);
			});
		}

		if (sort_mode_ == 0) {
			for (uint32_t cat_idx = 0; cat_idx < static_cast<uint32_t>(InputActionCategory::InterfaceWindows) + 1; ++cat_idx) {
				const auto category = static_cast<InputActionCategory>(cat_idx);
				std::vector<InputAction> in_category;
				for (auto action : visible_actions) {
					if (input_action_category(action) == category) in_category.push_back(action);
				}
				if (in_category.empty()) continue;

				const std::string header = std::string(input_action_category_name(category)) + " (" + std::to_string(in_category.size()) + ")";
				if (ImGui::CollapsingHeader(header.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
					for (auto action : in_category) {
						render_action_row(action, conflicts);
					}
				}
			}
		} else {
			for (auto action : visible_actions) {
				render_action_row(action, conflicts);
			}
		}

		if (visible_actions.empty()) {
			ImGui::TextDisabled("No actions match the current search and filter.");
		}

		ImGui::Separator();
		if (ImGui::Button("Reset All To Layout Defaults", ImVec2(240.0f, 28.0f))) {
			config_->keybinds.reset_to_layout_defaults(config_->keyboard_layout);
			listening_action_ = -1;
			conflict_message_.clear();
		}
		ImGui::SameLine();
		if (ImGui::Button("Clear All Bindings", ImVec2(160.0f, 28.0f))) {
			auto& all_bindings = config_->keybinds.raw_bindings();
			for (auto& b : all_bindings) {
				b.primary_key = GLFW_KEY_UNKNOWN;
				b.secondary_key = GLFW_KEY_UNKNOWN;
				b.primary_modifier = GLFW_KEY_UNKNOWN;
				b.primary_modifier2 = GLFW_KEY_UNKNOWN;
				b.secondary_modifier = GLFW_KEY_UNKNOWN;
				b.secondary_modifier2 = GLFW_KEY_UNKNOWN;
			}
		}
	}
};

}

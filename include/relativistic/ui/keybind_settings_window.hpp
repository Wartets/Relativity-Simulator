#pragma once

#include "relativistic/ui/camera_control_config.hpp"
#include <imgui.h>
#include <GLFW/glfw3.h>
#include <string>

namespace Relativistic::UI {

class KeybindSettingsWindow {
private:
	bool is_open_{false};
	CameraControlConfig* config_{nullptr};
	int listening_action_{-1};
	int listening_slot_{0};
	std::string conflict_message_{};

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

public:
	explicit KeybindSettingsWindow(CameraControlConfig& config) noexcept
		: config_(&config) {}

	[[nodiscard]] bool& open_state() noexcept {
		return is_open_;
	}

	void render(GLFWwindow* window) {
		if (!is_open_) return;

		ImGui::SetNextWindowSize(ImVec2(640.0f, 660.0f), ImGuiCond_FirstUseEver);
		if (!ImGui::Begin("Keybind Settings", &is_open_)) {
			ImGui::End();
			return;
		}

		ImGui::TextColored(ImVec4(0.3f, 0.9f, 1.0f, 1.0f), "Keyboard Layout");
		int layout_idx = static_cast<int>(config_->keyboard_layout);
		const char* layouts[] = {"QWERTY", "AZERTY"};
		if (ImGui::Combo("Layout Preset", &layout_idx, layouts, IM_ARRAYSIZE(layouts))) {
			config_->apply_keyboard_layout(static_cast<KeyboardLayout>(layout_idx));
			listening_action_ = -1;
			conflict_message_.clear();
		}
		ImGui::TextWrapped("Switching layout resets every binding below to that layout's defaults. Individual bindings can still be customized afterward and are saved independently.");

		ImGui::Separator();

		if (listening_action_ >= 0) {
			ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "Press any key to bind '%s' (%s slot). Press ESC to cancel.",
				std::string(input_action_name(static_cast<InputAction>(listening_action_))).c_str(),
				listening_slot_ == 0 ? "primary" : "secondary");

			if (window != nullptr) {
				if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
					listening_action_ = -1;
				} else {
					for (int key : kBindableKeys) {
						if (glfwGetKey(window, key) == GLFW_PRESS) {
							const auto action = static_cast<InputAction>(listening_action_);
							if (config_->keybinds.is_key_used_elsewhere(key, action)) {
								auto& all_bindings = config_->keybinds.raw_bindings();
								for (size_t i = 0; i < all_bindings.size(); ++i) {
									if (static_cast<int>(i) == listening_action_) continue;
									if (all_bindings[i].primary_key == key) all_bindings[i].primary_key = GLFW_KEY_UNKNOWN;
									if (all_bindings[i].secondary_key == key) all_bindings[i].secondary_key = GLFW_KEY_UNKNOWN;
								}
								conflict_message_ = std::string("Reassigned '") + glfw_key_display_name(key) + "' from its previous action.";
							} else {
								conflict_message_.clear();
							}
							if (listening_slot_ == 0) {
								config_->keybinds.set_primary(action, key);
							} else {
								config_->keybinds.set_secondary(action, key);
							}
							listening_action_ = -1;
							break;
						}
					}
				}
			}
		}

		if (!conflict_message_.empty()) {
			ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "%s", conflict_message_.c_str());
		}

		ImGui::Separator();

		if (ImGui::BeginTable("KeybindTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0.0f, 480.0f))) {
			ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("Primary", ImGuiTableColumnFlags_WidthFixed, 110.0f);
			ImGui::TableSetupColumn("Secondary", ImGuiTableColumnFlags_WidthFixed, 110.0f);
			ImGui::TableSetupColumn("Clear", ImGuiTableColumnFlags_WidthFixed, 60.0f);
			ImGui::TableHeadersRow();

			for (size_t i = 0; i < static_cast<size_t>(InputAction::Count); ++i) {
				const auto action = static_cast<InputAction>(i);
				const auto& binding = config_->keybinds.get(action);

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::TextUnformatted(std::string(input_action_name(action)).c_str());

				ImGui::TableSetColumnIndex(1);
				ImGui::PushID(static_cast<int>(i) * 2);
				if (ImGui::Button(glfw_key_display_name(binding.primary_key), ImVec2(-1.0f, 0.0f))) {
					listening_action_ = static_cast<int>(i);
					listening_slot_ = 0;
					conflict_message_.clear();
				}
				ImGui::PopID();

				ImGui::TableSetColumnIndex(2);
				ImGui::PushID(static_cast<int>(i) * 2 + 1);
				if (ImGui::Button(glfw_key_display_name(binding.secondary_key), ImVec2(-1.0f, 0.0f))) {
					listening_action_ = static_cast<int>(i);
					listening_slot_ = 1;
					conflict_message_.clear();
				}
				ImGui::PopID();

				ImGui::TableSetColumnIndex(3);
				ImGui::PushID(static_cast<int>(i) + 10000);
				if (ImGui::Button("X")) {
					config_->keybinds.set(action, GLFW_KEY_UNKNOWN, GLFW_KEY_UNKNOWN);
				}
				ImGui::PopID();
			}

			ImGui::EndTable();
		}

		ImGui::Separator();
		if (ImGui::Button("Reset All To Layout Defaults", ImVec2(240.0f, 28.0f))) {
			config_->keybinds.reset_to_layout_defaults(config_->keyboard_layout);
			listening_action_ = -1;
			conflict_message_.clear();
		}

		ImGui::End();
	}
};

}

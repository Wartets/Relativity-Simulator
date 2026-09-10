#pragma once

#include "relativistic/ui/camera_control_config.hpp"
#include "relativistic/ui/hud_layout_config.hpp"
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
	std::string conflict_message_{};
	char search_buffer_[64]{};
	int category_filter_{-1};
	int sort_mode_{0};

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
					if (bindings[j].primary_key == prim || bindings[j].secondary_key == prim) {
						info.primary_conflict[i] = true;
						break;
					}
				}
			}
			const int sec = bindings[i].secondary_key;
			if (sec != GLFW_KEY_UNKNOWN) {
				for (size_t j = 0; j < bindings.size(); ++j) {
					if (i == j) continue;
					if (bindings[j].primary_key == sec || bindings[j].secondary_key == sec) {
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

	[[nodiscard]] static bool action_supports_activation_mode(InputAction action) noexcept {
		return action == InputAction::ZoomModifier || action == InputAction::Sprint || action == InputAction::Crawl;
	}

	void render_capture_banner(GLFWwindow* window) noexcept {
		if (listening_action_ < 0) return;

		ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "Press any key to bind '%s' (%s slot). Press ESC to cancel.",
			std::string(input_action_name(static_cast<InputAction>(listening_action_))).c_str(),
			listening_slot_ == 0 ? "primary" : "secondary");

		if (window == nullptr) return;

		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			listening_action_ = -1;
			return;
		}

		for (int key : kBindableKeys) {
			if (glfwGetKey(window, key) != GLFW_PRESS) continue;
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

		if (action_supports_activation_mode(action)) {
			ImGui::SameLine();
			const char* mode_names[] = {"Hold", "Toggle"};
			int mode_idx = (binding.mode == InputActivationMode::Toggle) ? 1 : 0;
			ImGui::SetNextItemWidth(90.0f);
			if (ImGui::Combo("##activation_mode", &mode_idx, mode_names, IM_ARRAYSIZE(mode_names))) {
				config_->keybinds.set_mode(action, mode_idx == 1 ? InputActivationMode::Toggle : InputActivationMode::Hold);
			}
			render_setting_tooltip("Hold keeps this action active only while the key is held down. Toggle flips the action on and off each time the key is pressed.");
		}

		ImGui::PopID();
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

		ImGui::SetNextWindowSize(ImVec2(660.0f, 620.0f), ImGuiCond_FirstUseEver);
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
			}
		}

		ImGui::End();
	}
};

}

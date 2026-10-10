#pragma once

#include <imgui.h>

namespace Relativistic::UI {

inline void render_setting_tooltip(const char* text) noexcept {
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 28.0f);
		ImGui::TextUnformatted(text);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

inline void render_wrapped_colored_text(const ImVec4& color, const char* text) noexcept {
	ImGui::PushStyleColor(ImGuiCol_Text, color);
	ImGui::PushTextWrapPos(ImGui::GetContentRegionAvail().x + ImGui::GetCursorPosX());
	ImGui::TextUnformatted(text);
	ImGui::PopTextWrapPos();
	ImGui::PopStyleColor();
}

inline void render_setting_tooltip_warning(const char* text, const char* warning) noexcept {
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 28.0f);
		ImGui::TextUnformatted(text);
		if (warning != nullptr && warning[0] != '\0') {
			ImGui::Spacing();
			ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "%s", warning);
		}
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

class TabTooltipTracker {
private:
	int hovered_tab_id_{-1};
	ImVec2 last_mouse_pos_{0.0f, 0.0f};
	double stationary_start_time_{0.0};

public:
	[[nodiscard]] bool update_and_check(int tab_id, bool is_hovered, float delay_seconds, float movement_threshold_px) noexcept {
		if (!is_hovered) {
			if (hovered_tab_id_ == tab_id) {
				hovered_tab_id_ = -1;
			}
			return false;
		}

		const double current_time = ImGui::GetTime();
		const ImVec2 current_mouse = ImGui::GetMousePos();

		if (hovered_tab_id_ != tab_id) {
			hovered_tab_id_ = tab_id;
			last_mouse_pos_ = current_mouse;
			stationary_start_time_ = current_time;
			return false;
		}

		const float dx = current_mouse.x - last_mouse_pos_.x;
		const float dy = current_mouse.y - last_mouse_pos_.y;
		if ((dx * dx + dy * dy) > (movement_threshold_px * movement_threshold_px)) {
			last_mouse_pos_ = current_mouse;
			stationary_start_time_ = current_time;
			return false;
		}

		return (current_time - stationary_start_time_) >= static_cast<double>(delay_seconds);
	}

	void reset() noexcept {
		hovered_tab_id_ = -1;
	}
};

inline void render_tab_tooltip(
	TabTooltipTracker& tracker,
	int tab_id,
	bool is_hovered,
	const char* full_label,
	const char* tooltip_text,
	bool allow_reorder,
	bool is_popup_open,
	bool has_active_drag,
	float delay_seconds = 0.85f,
	float movement_threshold_px = 1.5f
) noexcept {
	if (is_popup_open || has_active_drag) {
		static_cast<void>(tracker.update_and_check(tab_id, false, delay_seconds, movement_threshold_px));
		return;
	}

	if (!tracker.update_and_check(tab_id, is_hovered, delay_seconds, movement_threshold_px)) {
		return;
	}

	ImGui::BeginTooltip();
	ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", full_label);
	if (tooltip_text != nullptr && tooltip_text[0] != '\0') {
		ImGui::Spacing();
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 24.0f);
		ImGui::TextColored(ImVec4(0.75f, 0.82f, 0.9f, 1.0f), "%s", tooltip_text);
		ImGui::PopTextWrapPos();
	}
	if (allow_reorder) {
		ImGui::Spacing();
		ImGui::TextDisabled("Right-click or drag to reorder tabs");
	}
	ImGui::EndTooltip();
}

}

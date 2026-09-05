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

}

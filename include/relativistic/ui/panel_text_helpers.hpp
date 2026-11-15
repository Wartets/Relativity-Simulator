#pragma once

#include <imgui.h>
#include "relativistic/ui/tooltip_utils.hpp"

namespace Relativistic::UI {

inline void render_hint_text(const char* text) noexcept {
	render_wrapped_colored_text(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled), text);
}

inline void render_caution_text(const char* text) noexcept {
	render_wrapped_colored_text(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), text);
}

template <typename... Args>
inline void render_wrapped_disabled_format(const char* fmt, Args&&... args) noexcept {
	char buf[512];
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
#endif
	std::snprintf(buf, sizeof(buf), fmt, std::forward<Args>(args)...);
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
	render_wrapped_colored_text(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled), buf);
}

inline void render_group_title(const char* text, const ImVec4& color) noexcept {
	ImGui::Spacing();
	ImGui::TextColored(color, "%s", text);
	ImGui::Separator();
}

}

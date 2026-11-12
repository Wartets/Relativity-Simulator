#pragma once

#include <imgui.h>
#include <algorithm>

namespace Relativistic::UI {

struct SplitPaneState {
	float primary_width{260.0f};
	float default_width{260.0f};
	float height{0.0f};
};

inline constexpr float kSplitPaneHandleWidth = 6.0f;

inline void split_pane_begin_primary(const char* id, SplitPaneState& state, float minimum_primary, float minimum_secondary) noexcept {
	const ImVec2 available = ImGui::GetContentRegionAvail();
	const float maximum_primary = std::max(minimum_primary, available.x - minimum_secondary - kSplitPaneHandleWidth);
	state.primary_width = std::clamp(state.primary_width, minimum_primary, maximum_primary);
	state.height = std::max(available.y, 180.0f);
	ImGui::BeginChild(id, ImVec2(state.primary_width, state.height), true);
}

inline void split_pane_end_primary(SplitPaneState& state) noexcept {
	ImGui::EndChild();
	ImGui::SameLine(0.0f, 0.0f);
	ImGui::PushID(&state);
	ImGui::InvisibleButton("##SplitPaneHandle", ImVec2(kSplitPaneHandleWidth, state.height));
	const bool hovered = ImGui::IsItemHovered();
	const bool active = ImGui::IsItemActive();
	if (active) {
		state.primary_width += ImGui::GetIO().MouseDelta.x;
	}
	if (hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
		state.primary_width = state.default_width;
	}
	if (hovered || active) {
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
	}
	const ImVec2 handle_min = ImGui::GetItemRectMin();
	const ImVec2 handle_max = ImGui::GetItemRectMax();
	const ImU32 color = ImGui::GetColorU32(active ? ImGuiCol_SeparatorActive : (hovered ? ImGuiCol_SeparatorHovered : ImGuiCol_Separator));
	const float center_x = 0.5f * (handle_min.x + handle_max.x);
	ImGui::GetWindowDrawList()->AddLine(ImVec2(center_x, handle_min.y + 4.0f), ImVec2(center_x, handle_max.y - 4.0f), color, active ? 2.0f : 1.0f);
	ImGui::PopID();
	ImGui::SameLine(0.0f, 0.0f);
}

inline void split_pane_begin_secondary(const char* id, const SplitPaneState& state) noexcept {
	ImGui::BeginChild(id, ImVec2(0.0f, state.height), true);
}

inline void split_pane_end_secondary() noexcept {
	ImGui::EndChild();
}

}

#pragma once

#include <imgui.h>
#include <string>
#include <utility>

namespace Relativistic::UI {

class SecondaryViewWindow {
private:
	std::string name_;
	bool is_open_{true};
	uint32_t texture_id_{0};

public:
	explicit SecondaryViewWindow(std::string name) : name_(std::move(name)) {}

	void render() {
		if (!is_open_) return;

		ImGui::SetNextWindowPos(ImVec2(400.0f, 100.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(480.0f, 320.0f), ImGuiCond_FirstUseEver);

		if (ImGui::Begin(name_.c_str(), &is_open_)) {
			ImVec2 size = ImGui::GetContentRegionAvail();
			if (size.x > 0 && size.y > 0) {
				if (texture_id_ != 0) {
					ImGui::Image(reinterpret_cast<void*>(static_cast<intptr_t>(texture_id_)), size);
				} else {
					ImGui::GetWindowDrawList()->AddRectFilled(
						ImGui::GetCursorScreenPos(),
						ImVec2(ImGui::GetCursorScreenPos().x + size.x, ImGui::GetCursorScreenPos().y + size.y),
						0xFF323232U
					);
					ImGui::SetCursorScreenPos(ImVec2(ImGui::GetCursorScreenPos().x + size.x * 0.5f - 50.0f, ImGui::GetCursorScreenPos().y + size.y * 0.5f));
					ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Secondary Camera View");
				}
			}
		}
		ImGui::End();
	}
};

}

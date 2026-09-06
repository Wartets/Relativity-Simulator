#pragma once

#include "relativistic/ui/hud_layout_config.hpp"
#include <imgui.h>

namespace Relativistic::UI {

class HudManagerWindow {
private:
	bool is_open_{false};
	HudLayoutConfig* config_{nullptr};

public:
	explicit HudManagerWindow(HudLayoutConfig& config) noexcept
		: config_(&config) {}

	[[nodiscard]] bool& open_state() noexcept {
		return is_open_;
	}

	void render() {
		if (!is_open_) return;

		ImGui::SetNextWindowSize(ImVec2(580.0f, 640.0f), ImGuiCond_FirstUseEver);
		if (!ImGui::Begin("HUD Manager", &is_open_)) {
			ImGui::End();
			return;
		}

		ImGui::Checkbox("Master HUD Visibility", &config_->master_enabled);
		ImGui::TextDisabled("Governs the telemetry and navigation overlay elements only; toolbar and loading indicator have their own toggles below.");
		ImGui::Separator();

		const char* anchor_names[] = {"Top Left", "Top Right", "Bottom Left", "Bottom Right", "Top Center", "Bottom Center"};

		for (size_t i = 0; i < static_cast<size_t>(HudElementId::Count); ++i) {
			const auto id = static_cast<HudElementId>(i);
			auto& style = config_->element(id);

			ImGui::PushID(static_cast<int>(i));
			if (ImGui::CollapsingHeader(hud_element_name(id))) {
				ImGui::Checkbox("Enabled", &style.enabled);

				int anchor_idx = static_cast<int>(style.anchor);
				if (ImGui::Combo("Anchor Corner", &anchor_idx, anchor_names, IM_ARRAYSIZE(anchor_names))) {
					style.anchor = static_cast<HudAnchor>(anchor_idx);
				}

				ImGui::DragFloat("Offset X", &style.offset_x, 1.0f, 0.0f, 2400.0f, "%.0f px");
				ImGui::DragFloat("Offset Y", &style.offset_y, 1.0f, 0.0f, 2400.0f, "%.0f px");
				ImGui::SliderFloat("Text Scale", &style.scale, 0.5f, 3.0f, "%.2fx");
				ImGui::ColorEdit4("Text Color", style.text_color.data());
				ImGui::Checkbox("Show Background Panel", &style.show_background);
				if (style.show_background) {
					ImGui::SliderFloat("Background Opacity", &style.background_opacity, 0.0f, 1.0f, "%.2f");
				}
			}
			ImGui::PopID();
		}

		ImGui::Separator();
		if (ImGui::Button("Restore All HUD Defaults", ImVec2(220.0f, 28.0f))) {
			*config_ = HudLayoutConfig{};
		}

		ImGui::End();
	}
};

}

#pragma once

#include "relativistic/ui/secondary_view_window.hpp"
#include "relativistic/ui/tooltip_utils.hpp"
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include <imgui.h>
#include <vector>
#include <memory>
#include <string>
#include <cmath>
#include <cstdio>
#include <algorithm>

namespace Relativistic::UI {

class SecondaryViewportManager {
private:
	Orchestrator::SimulationOrchestrator<1024>& orchestrator_;
	std::vector<std::unique_ptr<SecondaryViewWindow>> views_{};
	uint32_t next_id_{1};
	bool auto_throttle_enabled_{true};
	float throttle_scale_per_extra_view_{0.85f};

public:
	explicit SecondaryViewportManager(Orchestrator::SimulationOrchestrator<1024>& orchestrator) noexcept
		: orchestrator_(orchestrator) {}

	SecondaryViewWindow& add_view(const std::string& base_name = "Secondary Observer") {
		char label[96];
		std::snprintf(label, sizeof(label), "%s %u", base_name.c_str(), next_id_++);
		views_.push_back(std::make_unique<SecondaryViewWindow>(std::string(label), orchestrator_));
		views_.back()->open_state() = true;
		return *views_.back();
	}

	void remove_closed_views() noexcept {
		std::erase_if(views_, [](const std::unique_ptr<SecondaryViewWindow>& v) { return !v->open_state(); });
	}

	[[nodiscard]] size_t active_view_count() const noexcept {
		size_t count = 0;
		for (const auto& v : views_) {
			if (v->open_state()) ++count;
		}
		return count;
	}

	[[nodiscard]] std::vector<std::unique_ptr<SecondaryViewWindow>>& views() noexcept {
		return views_;
	}

	void render_all() {
		if (auto_throttle_enabled_) {
			const size_t active = active_view_count();
			const float suggested_scale = (active > 1)
				? std::pow(throttle_scale_per_extra_view_, static_cast<float>(active - 1))
				: 1.0f;
			for (auto& v : views_) {
				if (v->open_state()) {
					v->set_performance_budget_scale(std::max(suggested_scale, 0.15f));
				}
			}
		} else {
			for (auto& v : views_) {
				v->set_performance_budget_scale(1.0f);
			}
		}

		for (auto& v : views_) {
			v->render();
		}
		remove_closed_views();
	}

	void render_management_panel(bool& panel_open) {
		if (!panel_open) return;

		ImGui::SetNextWindowSize(ImVec2(420.0f, 320.0f), ImGuiCond_FirstUseEver);
		if (!ImGui::Begin("Secondary Viewports Manager", &panel_open)) {
			ImGui::End();
			return;
		}

		ImGui::TextColored(ImVec4(0.3f, 0.9f, 1.0f, 1.0f), "Active Secondary Observers: %zu", views_.size());
		ImGui::Checkbox("Auto-Throttle Resolution With Multiple Views", &auto_throttle_enabled_);
		render_setting_tooltip("Automatically reduces the internal render resolution scale of each secondary viewport as more of them are opened simultaneously, keeping overall frame time bounded regardless of how many observers are active.");
		if (auto_throttle_enabled_) {
			ImGui::SliderFloat("Per-View Scale Falloff", &throttle_scale_per_extra_view_, 0.4f, 1.0f, "%.2f");
		}

		ImGui::Separator();
		if (ImGui::Button("Add Secondary Viewport", ImVec2(-1.0f, 28.0f))) {
			add_view();
		}

		ImGui::Separator();
		for (size_t i = 0; i < views_.size(); ++i) {
			ImGui::PushID(static_cast<int>(i));
			ImGui::Checkbox("##ViewOpen", &views_[i]->open_state());
			ImGui::SameLine();
			ImGui::TextUnformatted(views_[i]->name().c_str());
			ImGui::SameLine();
			if (ImGui::SmallButton("Close")) {
				views_[i]->open_state() = false;
			}
			ImGui::PopID();
		}
		if (views_.empty()) {
			ImGui::TextDisabled("No secondary viewports active.");
		}

		ImGui::End();
	}
};

}

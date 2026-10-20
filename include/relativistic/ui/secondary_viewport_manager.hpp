#pragma once

#include "relativistic/ui/secondary_view_window.hpp"
#include "relativistic/ui/secondary_view_roles.hpp"
#include "relativistic/ui/tooltip_utils.hpp"
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/render/gpu_types.hpp"
#include <imgui.h>
#include <vector>
#include <memory>
#include <string>
#include <array>
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
	SecondaryTargetSettings shared_target_{};
	bool target_sync_enabled_{false};
	bool beacons_visible_in_primary_{true};

	SecondaryViewWindow& register_view(std::unique_ptr<SecondaryViewWindow> view, SecondaryViewRole role, bool open) {
		view->set_view_id(next_id_++);
		view->set_role(role);
		view->open_state() = open;
		views_.push_back(std::move(view));
		return *views_.back();
	}

public:
	explicit SecondaryViewportManager(Orchestrator::SimulationOrchestrator<1024>& orchestrator) noexcept
		: orchestrator_(orchestrator) {}

	SecondaryViewWindow& add_view(const std::string& base_name = "Secondary Observer", SecondaryViewRole role = SecondaryViewRole::FreeObserver) {
		char label[96];
		std::snprintf(label, sizeof(label), "%s %u", base_name.c_str(), next_id_);
		return register_view(std::make_unique<SecondaryViewWindow>(std::string(label), orchestrator_), role, true);
	}

	SecondaryViewWindow& add_view_with_role(SecondaryViewRole role) {
		const std::string base = std::string("Secondary ") + kSecondaryViewRoleLabels[static_cast<size_t>(role)];
		return add_view(base, role);
	}

	SecondaryViewWindow& add_view_with_state(const std::string& exact_name, bool initial_open) {
		return register_view(std::make_unique<SecondaryViewWindow>(exact_name, orchestrator_), SecondaryViewRole::FreeObserver, initial_open);
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

	void append_beacon_bodies(std::vector<Render::GpuBodyData>& bodies, const std::array<double, 3>& viewer) const {
		if (!beacons_visible_in_primary_) {
			return;
		}
		for (const auto& v : views_) {
			v->append_beacon_bodies(bodies, viewer);
		}
	}

	void store_manager_state(SecondaryStateIo::Entries& entries) const {
		SecondaryStateIo::put_unsigned(entries, "secondary_manager_sync", target_sync_enabled_ ? 1U : 0U);
		SecondaryStateIo::put_unsigned(entries, "secondary_manager_beacons", beacons_visible_in_primary_ ? 1U : 0U);
		SecondaryStateIo::write_target(entries, "secondary_manager_", shared_target_);
	}

	void restore_manager_state(const SecondaryStateIo::Entries& entries) {
		target_sync_enabled_ = SecondaryStateIo::read_unsigned(entries, "secondary_manager_sync", target_sync_enabled_ ? 1U : 0U) != 0U;
		beacons_visible_in_primary_ = SecondaryStateIo::read_unsigned(entries, "secondary_manager_beacons", beacons_visible_in_primary_ ? 1U : 0U) != 0U;
		SecondaryStateIo::read_target(entries, "secondary_manager_", shared_target_);
	}

	void render_all() {
		for (auto& v : views_) {
			v->bind_shared_target(&shared_target_, target_sync_enabled_);
		}

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

		ImGui::SetNextWindowSize(ImVec2(460.0f, 520.0f), ImGuiCond_FirstUseEver);
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
		ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Multi-View Synchronization");
		ImGui::Checkbox("Synchronize Observation Targets", &target_sync_enabled_);
		render_setting_tooltip("Forces every secondary view to look at and orbit the shared target below, so the same phenomenon is studied from several reference frames at once.");
		if (target_sync_enabled_) {
			static_cast<void>(render_secondary_target_editor(shared_target_, orchestrator_));
		}
		ImGui::Checkbox("Show Observer Beacons In Primary Viewport", &beacons_visible_in_primary_);
		render_setting_tooltip("Renders a 3D marker at the position of every secondary observer inside the primary raytraced view. Markers are lensed with the rest of the scene and each view can opt out individually.");

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Create Views");
		for (size_t role_index = 0; role_index < kSecondaryViewRoleCount; ++role_index) {
			const std::string label = std::string("Add ") + kSecondaryViewRoleNames[role_index];
			if (ImGui::Button(label.c_str(), ImVec2(-1.0f, 26.0f))) {
				add_view_with_role(static_cast<SecondaryViewRole>(role_index));
			}
		}
		if (ImGui::Button("Add Comoving Probe + Asymptotic Pair", ImVec2(-1.0f, 28.0f))) {
			add_view_with_role(SecondaryViewRole::ComovingProbe);
			add_view_with_role(SecondaryViewRole::AsymptoticGlobal);
			target_sync_enabled_ = true;
		}
		render_setting_tooltip("Creates a free-fall cockpit view and a global view from far away and synchronizes their targets, for simultaneous study from the accelerated frame and from spatial infinity.");

		ImGui::Separator();
		for (size_t i = 0; i < views_.size(); ++i) {
			ImGui::PushID(static_cast<int>(i));
			ImGui::Checkbox("##ViewOpen", &views_[i]->open_state());
			ImGui::SameLine();
			ImGui::Text("%s [%s]", views_[i]->name().c_str(), kSecondaryViewRoleLabels[static_cast<size_t>(views_[i]->role())]);
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

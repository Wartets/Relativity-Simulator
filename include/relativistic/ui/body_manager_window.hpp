#pragma once

#include <imgui.h>
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/io/user_settings.hpp"
#include "relativistic/ui/body_manager/body_manager_types.hpp"
#include "relativistic/ui/body_manager/body_manager_context.hpp"
#include "relativistic/ui/body_manager/body_catalog_tab.hpp"
#include "relativistic/ui/body_manager/source_catalog_tab.hpp"
#include "relativistic/ui/body_manager/body_creation_tab.hpp"
#include "relativistic/ui/body_manager/source_creation_tab.hpp"
#include "relativistic/ui/body_manager/system_dynamics_tab.hpp"
#include "relativistic/ui/body_manager/interactions_tab.hpp"
#include "relativistic/ui/tooltip_utils.hpp"
#include <algorithm>
#include <cfloat>
#include <cstddef>
#include <cstring>
#include <optional>

namespace Relativistic::UI {

class BodyManagerWindow {
private:
	bool is_open_{false};
	Orchestrator::SimulationOrchestrator<1024>& orchestrator_;
	IO::UserSettings* persisted_settings_{nullptr};
	BodyManagerContext context_;
	BodyCatalogTab object_catalog_tab_{};
	SourceCatalogTab source_catalog_tab_{};
	BodyCreationTab object_creation_tab_{};
	SourceCreationTab source_creation_tab_{};
	SystemDynamicsTab system_dynamics_tab_{};
	InteractionsTab interactions_tab_{};

	[[nodiscard]] static ImGuiTabItemFlags tab_flags(const std::optional<BodyManagerTab>& requested, BodyManagerTab tab) noexcept {
		return (requested.has_value() && *requested == tab) ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None;
	}

public:
	explicit BodyManagerWindow(Orchestrator::SimulationOrchestrator<1024>& orchestrator)
		: orchestrator_(orchestrator), context_(orchestrator) {
		object_creation_tab_.reset_draft(context_);
		source_creation_tab_.reset_draft(context_);
	}

	void attach_persisted_settings(IO::UserSettings& settings) noexcept {
		persisted_settings_ = &settings;
		auto& filter = object_catalog_tab_.filter();
		filter.sort_mode = static_cast<int>(settings.body_manager_sort_mode);
		filter.descending = settings.body_manager_sort_descending;
		std::strncpy(filter.search, settings.body_manager_search_filter.c_str(), sizeof(filter.search) - 1);
		filter.search[sizeof(filter.search) - 1] = '\0';
		auto& source_filter = source_catalog_tab_.filter();
		source_filter.sort_mode = static_cast<int>(settings.body_manager_source_sort_mode);
		source_filter.descending = settings.body_manager_source_sort_descending;
		std::strncpy(source_filter.search, settings.body_manager_source_search_filter.c_str(), sizeof(source_filter.search) - 1);
		source_filter.search[sizeof(source_filter.search) - 1] = '\0';
		auto& object_split = object_catalog_tab_.split();
		object_split.primary_width = settings.body_manager_list_pane_width;
		source_catalog_tab_.split().primary_width = settings.body_manager_source_list_pane_width;
		system_dynamics_tab_.set_bulk_parameter_index(static_cast<int>(settings.body_manager_bulk_parameter_index));
		object_creation_tab_.view().mass_log_mode = settings.body_manager_new_body_mass_log_mode;
		object_creation_tab_.view().radius_log_mode = settings.body_manager_new_body_radius_log_mode;
		source_creation_tab_.view().mass_log_mode = settings.body_manager_new_source_mass_log_mode;
	}

	void sync_persisted_settings() noexcept {
		if (persisted_settings_ == nullptr) return;
		const auto& filter = object_catalog_tab_.filter();
		persisted_settings_->body_manager_sort_mode = static_cast<uint32_t>(filter.sort_mode);
		persisted_settings_->body_manager_sort_descending = filter.descending;
		persisted_settings_->body_manager_search_filter = filter.search;
		persisted_settings_->body_manager_list_pane_width = object_catalog_tab_.split().primary_width;
		const auto& source_filter = source_catalog_tab_.filter();
		persisted_settings_->body_manager_source_sort_mode = static_cast<uint32_t>(source_filter.sort_mode);
		persisted_settings_->body_manager_source_sort_descending = source_filter.descending;
		persisted_settings_->body_manager_source_search_filter = source_filter.search;
		persisted_settings_->body_manager_source_list_pane_width = source_catalog_tab_.split().primary_width;
		persisted_settings_->body_manager_bulk_parameter_index = static_cast<uint32_t>(system_dynamics_tab_.bulk_parameter_index());
		persisted_settings_->body_manager_new_body_mass_log_mode = object_creation_tab_.view().mass_log_mode;
		persisted_settings_->body_manager_new_body_radius_log_mode = object_creation_tab_.view().radius_log_mode;
		persisted_settings_->body_manager_new_source_mass_log_mode = source_creation_tab_.view().mass_log_mode;
	}

	[[nodiscard]] bool& open_state() noexcept {
		return is_open_;
	}

	void render() {
		if (!is_open_) {
			sync_persisted_settings();
			return;
		}

		ImGui::SetNextWindowPos(ImVec2(15.0f, 400.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(820.0f, 700.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSizeConstraints(ImVec2(520.0f, 420.0f), ImVec2(FLT_MAX, FLT_MAX));

		if (!ImGui::Begin("Celestial Body & N-Body Manager", &is_open_)) {
			ImGui::End();
			return;
		}

		context_.update_tracking();

		size_t objects = 0;
		size_t enabled_objects = 0;
		size_t sources = 0;
		size_t enabled_sources = 0;
		context_.count_bodies(objects, enabled_objects, sources, enabled_sources);
		ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "Objects: %zu (%zu enabled)", objects, enabled_objects);
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(1.0f, 0.65f, 0.4f, 1.0f), "Sources: %zu (%zu enabled)", sources, enabled_sources);
		render_setting_tooltip("Objects are ordinary bodies integrated by the post-Newtonian N-body solver. Sources are independent black holes with their own event horizons.");
		ImGui::SameLine();
		if (ImGui::Button("Clear All Bodies")) {
			context_.clear_all_bodies();
		}
		render_setting_tooltip("Permanently removes every object and independent black hole from the system. The primary central mass at the origin is unaffected.");
		ImGui::SameLine();
		if (ImGui::Button("Spawn Reference System")) {
			context_.populate_reference_system();
		}
		render_setting_tooltip("Populates the system with a simplified two-planet configuration to quickly exercise N-body dynamics.");
		ImGui::SameLine();
		if (ImGui::Button("Spawn Companion BH")) {
			context_.spawn_companion_source();
		}
		render_setting_tooltip("Creates an independent black hole roughly half the mass of the primary source on a wide circular orbit.");

		ImGui::Separator();

		const auto requested_tab = context_.take_requested_tab();

		if (ImGui::BeginTabBar("BodyManagerTabs", ImGuiTabBarFlags_None)) {
			if (ImGui::BeginTabItem("Object Catalog", nullptr, tab_flags(requested_tab, BodyManagerTab::ObjectCatalog))) {
				object_catalog_tab_.render(context_);
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Source Catalog", nullptr, tab_flags(requested_tab, BodyManagerTab::SourceCatalog))) {
				source_catalog_tab_.render(context_);
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Create Object", nullptr, tab_flags(requested_tab, BodyManagerTab::ObjectCreation))) {
				object_creation_tab_.render(context_);
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Create Source", nullptr, tab_flags(requested_tab, BodyManagerTab::SourceCreation))) {
				source_creation_tab_.render(context_);
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("System Dynamics", nullptr, tab_flags(requested_tab, BodyManagerTab::SystemDynamics))) {
				system_dynamics_tab_.render(context_);
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Interactions", nullptr, tab_flags(requested_tab, BodyManagerTab::Interactions))) {
				interactions_tab_.render(context_);
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}

		sync_persisted_settings();
		ImGui::End();
	}
};

}

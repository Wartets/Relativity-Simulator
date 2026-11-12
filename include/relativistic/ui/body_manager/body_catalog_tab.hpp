#pragma once

#include <imgui.h>
#include "relativistic/ui/body_manager/body_manager_context.hpp"
#include "relativistic/ui/body_manager/body_catalog_list.hpp"
#include "relativistic/ui/body_manager/body_property_editor.hpp"
#include "relativistic/ui/split_pane_layout.hpp"
#include "relativistic/ui/panel_text_helpers.hpp"
#include "relativistic/ui/tooltip_utils.hpp"
#include "relativistic/orchestrator/command.hpp"
#include <mutex>
#include <span>

namespace Relativistic::UI {

class BodyCatalogTab {
private:
	CatalogFilterState filter_{};
	SplitPaneState split_{300.0f, 300.0f, 0.0f};
	BodyEditorViewState view_{};

	void render_empty_state(BodyManagerContext& ctx) noexcept {
		render_hint_text("No ordinary object exists yet. Objects are orbiting bodies such as planets, moons, stars or test particles integrated by the post-Newtonian solver.");
		ImGui::Spacing();
		if (ImGui::Button("Create Object", ImVec2(-1.0f, 28.0f))) {
			ctx.request_tab(BodyManagerTab::ObjectCreation);
		}
		render_setting_tooltip("Opens the Create Object tab so you can configure and spawn a new orbiting body.");
		ImGui::Spacing();
		if (ImGui::Button("Spawn Reference System", ImVec2(-1.0f, 26.0f))) {
			ctx.populate_reference_system();
		}
		render_setting_tooltip("Replaces the catalog with a simplified two-planet configuration to quickly exercise N-body dynamics.");
	}

	void render_object_detail(BodyManagerContext& ctx, Dynamics::PostNewtonianSystem& sys, Dynamics::PostNewtonianBody& b, CatalogListResult& pending) noexcept {
		const uint32_t body_id = b.id;
		ImGui::PushID(static_cast<int>(body_id));
		ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%s", BodyManagerContext::display_name(b).c_str());
		ImGui::TextDisabled("Identifier: #%u", body_id);
		const std::string facts = catalog_info_line(ctx, b);
		ImGui::TextDisabled("%s", facts.c_str());

		if (ImGui::Button("Look At", ImVec2(70.0f, 24.0f))) {
			ctx.look_at(b.position);
		}
		render_setting_tooltip("Rotates the camera to face this body without moving the camera position.");
		ImGui::SameLine();
		bool tracking_this = ctx.is_tracking(static_cast<int>(body_id));
		if (ImGui::Checkbox("Track", &tracking_this)) {
			ctx.set_tracking(static_cast<int>(body_id), tracking_this);
		}
		render_setting_tooltip("Keeps the camera pointed at this body while it moves.");
		ImGui::SameLine();
		if (ImGui::Button("Duplicate", ImVec2(80.0f, 24.0f))) {
			pending.action = CatalogRowAction::Duplicate;
			pending.action_id = static_cast<int>(body_id);
		}
		render_setting_tooltip("Creates a copy of this body offset along X, keeping every physical parameter, the appearance and all texture layers.");
		ImGui::SameLine();
		if (ImGui::Button("Delete", ImVec2(70.0f, 24.0f))) {
			pending.action = CatalogRowAction::Delete;
			pending.action_id = static_cast<int>(body_id);
		}
		render_setting_tooltip("Permanently removes this body from the N-body system.");
		if (b.enabled) {
			ImGui::SameLine();
			if (ImGui::Button("Walk On Surface", ImVec2(120.0f, 24.0f))) {
				ctx.orchestrator().parameters().surface_walk_body_id = body_id;
				static_cast<void>(ctx.orchestrator().enqueue_command(Orchestrator::Command::make_set_camera_mode(4U)));
			}
			render_setting_tooltip("Places the camera on the surface of this body and switches to Surface Walk navigation.");
		}
		ImGui::Separator();

		Dynamics::BodySurfaceLayerSet layers = ctx.orchestrator().surface_layers().get(body_id);
		const BodyEditResult result = render_body_editor(ctx, b, &layers, view_, BodyEditorSection::Complete);
		if (result.layers_changed) {
			ctx.orchestrator().surface_layers().set(body_id, layers);
		}
		if (result.body_changed || result.layers_changed) {
			sys.update_accelerations();
			ctx.orchestrator().notify_state_changed();
		}
		if (b.is_spacetime_source) {
			ctx.select_source(static_cast<int>(body_id));
			ctx.select_object(BodyManagerContext::kNoSelection);
			ctx.request_tab(BodyManagerTab::SourceCatalog);
		}
		ImGui::PopID();
	}

public:
	[[nodiscard]] CatalogFilterState& filter() noexcept {
		return filter_;
	}

	[[nodiscard]] SplitPaneState& split() noexcept {
		return split_;
	}

	void render(BodyManagerContext& ctx) noexcept {
		auto& sys = ctx.orchestrator().nbody_system();
		std::lock_guard<std::recursive_mutex> lock(sys.bodies_mutex());
		auto bodies = sys.bodies();

		size_t object_count = 0;
		size_t enabled_count = 0;
		for (const auto& body : bodies) {
			if (!body.is_spacetime_source) {
				++object_count;
				if (body.enabled) ++enabled_count;
			}
		}

		const float font = ImGui::GetFontSize();
		CatalogListResult pending;

		split_pane_begin_primary("ObjectCatalogList", split_, font * 15.0f, font * 26.0f);
		ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "Objects: %zu (%zu enabled)", object_count, enabled_count);
		render_setting_tooltip("Ordinary bodies integrated by the post-Newtonian N-body solver. Independent black holes are listed in the Source Catalog tab.");
		ImGui::Separator();
		if (object_count == 0) {
			render_empty_state(ctx);
		} else {
			pending = render_body_catalog_list(ctx, bodies, false, filter_, ctx.selected_object_id());
			if (pending.clicked_id >= 0) {
				ctx.select_object(pending.clicked_id);
			}
		}
		split_pane_end_primary(split_);

		split_pane_begin_secondary("ObjectCatalogDetail", split_);
		if (ctx.selected_object_id() == BodyManagerContext::kNoSelection && object_count > 0) {
			for (const auto& body : bodies) {
				if (!body.is_spacetime_source) {
					ctx.select_object(static_cast<int>(body.id));
					break;
				}
			}
		}
		Dynamics::PostNewtonianBody* selected = nullptr;
		for (auto& body : bodies) {
			if (!body.is_spacetime_source && static_cast<int>(body.id) == ctx.selected_object_id()) {
				selected = &body;
				break;
			}
		}
		if (selected != nullptr) {
			render_object_detail(ctx, sys, *selected, pending);
		} else {
			render_hint_text("Select an object in the list to inspect or edit its parameters. Right-click an entry for quick actions such as look at, track, enable, duplicate and delete.");
		}
		split_pane_end_secondary();

		apply_catalog_row_action(ctx, pending);
	}
};

}

#pragma once

#include <imgui.h>
#include "relativistic/ui/body_manager/body_manager_context.hpp"
#include "relativistic/ui/body_manager/body_catalog_list.hpp"
#include "relativistic/ui/body_manager/body_property_editor.hpp"
#include "relativistic/ui/spacetime_source_panel.hpp"
#include "relativistic/ui/accretion_disk_editor.hpp"
#include "relativistic/ui/jet_magnetosphere_editor.hpp"
#include "relativistic/ui/compatibility_notes.hpp"
#include "relativistic/ui/split_pane_layout.hpp"
#include "relativistic/ui/panel_text_helpers.hpp"
#include "relativistic/ui/tooltip_utils.hpp"
#include "relativistic/orchestrator/command.hpp"
#include <algorithm>
#include <cstdint>
#include <mutex>

namespace Relativistic::UI {

class SourceCatalogTab {
private:
	CatalogFilterState filter_{};
	SplitPaneState split_{300.0f, 300.0f, 0.0f};
	BodyEditorViewState view_{};
	SpacetimeSourceEditState central_edit_{true};
	SpacetimeSourceValues central_values_{};
	uint64_t central_synced_version_{0};
	bool central_lock_{true};

	void render_primary_source_detail(BodyManagerContext& ctx) noexcept {
		auto& orchestrator = ctx.orchestrator();
		const auto& params = orchestrator.parameters();

		const uint64_t version = orchestrator.state_version();
		if (version != central_synced_version_ || !ImGui::IsAnyItemActive()) {
			central_values_ = SpacetimeSourceValues{params.mass, params.spin, params.charge, params.cosmological_lambda};
			central_synced_version_ = version;
		}

		ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Primary Metric Source");
		ImGui::TextDisabled("Metric: %s", orchestrator.active_metric_name().c_str());
		render_hint_text("The primary spacetime source curves the rendered background and the lensing, and is always fixed at the coordinate origin. Its parameters are the same ones edited in the Spacetime & Metrics tab of the Master Simulation Controls.");

		if (ImGui::Button("Look At", ImVec2(70.0f, 24.0f))) {
			ctx.look_at({0.0, 0.0, 0.0});
		}
		render_setting_tooltip("Orients the camera to face the central compact object without changing its position.");
		ImGui::SameLine();
		bool tracking_central = ctx.is_tracking(BodyManagerContext::kCentralSourceId);
		if (ImGui::Checkbox("Track", &tracking_central)) {
			ctx.set_tracking(BodyManagerContext::kCentralSourceId, tracking_central);
		}
		render_setting_tooltip("Keeps the camera pointed at the central object.");
		ImGui::Separator();

		const Metrics::HorizonConstraintSet constraints = Metrics::HorizonConstraintSet::from_metric_name(orchestrator.active_metric_name());
		SpacetimeSourcePanelOptions options;
		options.constraints = &constraints;
		options.guarantee_horizon = &central_lock_;
		options.metric_name = &orchestrator.active_metric_name();
		options.render_lock_controls = true;
		options.mass_label = "Central Mass (M)";
		const std::string& active_metric = orchestrator.active_metric_name();
		options.show_mass = metric_uses_central_mass(active_metric);
		options.show_spin = active_metric.find("Kerr") != std::string::npos;
		options.show_charge = active_metric.find("Reissner") != std::string::npos || active_metric.find("Newman") != std::string::npos;
		options.show_lambda = active_metric.find("de Sitter") != std::string::npos
			|| active_metric.find("DeSitter") != std::string::npos
			|| active_metric.find("FLRW") != std::string::npos
			|| active_metric.find("Quintessence") != std::string::npos
			|| active_metric.find("Chaplygin") != std::string::npos;
		options.mass_slider_minimum = 0.001;
		options.mass_slider_maximum = 1.0e6;
		options.mass_log_minimum = 1e-12;
		options.mass_log_maximum = 1e60;
		const SpacetimeSourcePanelResult panel = render_spacetime_source_panel(orchestrator, central_edit_, central_values_, options);
		if (panel.mass_changed) {
			static_cast<void>(orchestrator.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::Mass, std::max(0.01, central_values_.mass))));
		}
		if (panel.spin_changed) {
			static_cast<void>(orchestrator.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::Spin, central_values_.spin)));
		}
		if (panel.charge_changed) {
			static_cast<void>(orchestrator.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::Charge, central_values_.charge)));
		}
		if (panel.cosmological_constant_changed) {
			static_cast<void>(orchestrator.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::CosmologicalLambda, central_values_.cosmological_constant)));
		}

		if (options.show_spin && params.mass > 0.0) {
			float spin_ratio = static_cast<float>(std::abs(params.spin) / params.mass);
			const double charge_ratio = params.charge / params.mass;
			const float extremal_ratio = static_cast<float>(std::sqrt(std::max(1.0 - charge_ratio * charge_ratio, 0.0)));
			const float allowed_ratio = central_lock_ ? std::max(std::min(extremal_ratio, 0.999f), 1e-3f) : 1.5f;
			if (slider_float_with_input("Dimensionless Spin (a/M)", &spin_ratio, 0.0f, allowed_ratio, "%.4f")) {
				const double sign = (params.spin < 0.0) ? -1.0 : 1.0;
				static_cast<void>(orchestrator.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::Spin, sign * static_cast<double>(spin_ratio) * params.mass)));
			}
			render_setting_tooltip("Specific angular momentum ratio a/M, identical to the control in the Spacetime & Metrics tab. The sign of the spin is preserved.");
		}
		if (!options.show_mass && !options.show_spin && !options.show_charge && !options.show_lambda) {
			render_hint_text("The active spacetime has no central mass, spin or charge parameter; its specific parameters are edited in the Spacetime & Metrics tab.");
		} else if (active_metric.find("Morris") != std::string::npos || active_metric.find("Wormhole") != std::string::npos || active_metric.find("Alcubierre") != std::string::npos || active_metric.find("Warp") != std::string::npos || active_metric.find("TeVeS") != std::string::npos) {
			render_hint_text("Throat, bubble and scalar-field parameters of the active spacetime are edited in the Spacetime & Metrics tab.");
		}

		ImGui::Spacing();
		if (ImGui::CollapsingHeader("Accretion Disk", ImGuiTreeNodeFlags_DefaultOpen)) {
			render_primary_accretion_disk_editor(orchestrator);
		}
		if (ImGui::CollapsingHeader("Blandford-Znajek Jet Magnetosphere")) {
			render_jet_magnetosphere_editor(orchestrator);
		}
		render_group_title("Primary Geometric Landmarks", ImVec4(1.0f, 0.75f, 0.35f, 1.0f));
		render_primary_geometric_landmarks(orchestrator);
	}

	void render_source_detail(BodyManagerContext& ctx, Dynamics::PostNewtonianSystem& sys, Dynamics::PostNewtonianBody& b, CatalogListResult& pending) noexcept {
		const uint32_t body_id = b.id;
		ImGui::PushID(static_cast<int>(body_id) + 500000);
		ImGui::TextColored(ImVec4(1.0f, 0.65f, 0.4f, 1.0f), "%s", BodyManagerContext::display_name(b).c_str());
		ImGui::TextDisabled("Independent black hole  |  Identifier: #%u", body_id);
		const std::string facts = catalog_info_line(ctx, b);
		ImGui::TextDisabled("%s", facts.c_str());

		if (ImGui::Button("Look At", ImVec2(70.0f, 24.0f))) {
			ctx.look_at(b.position);
		}
		render_setting_tooltip("Rotates the camera to face this black hole without moving the camera position.");
		ImGui::SameLine();
		bool tracking_this = ctx.is_tracking(static_cast<int>(body_id));
		if (ImGui::Checkbox("Track", &tracking_this)) {
			ctx.set_tracking(static_cast<int>(body_id), tracking_this);
		}
		render_setting_tooltip("Keeps the camera pointed at this black hole while it moves.");
		ImGui::SameLine();
		if (ImGui::Button("Duplicate", ImVec2(80.0f, 24.0f))) {
			pending.action = CatalogRowAction::Duplicate;
			pending.action_id = static_cast<int>(body_id);
		}
		render_setting_tooltip("Creates a copy of this black hole offset along X and moves it to a safe separation from every other horizon.");
		ImGui::SameLine();
		bool reverted = false;
		if (ImGui::Button("Revert To Ordinary Body", ImVec2(170.0f, 24.0f))) {
			b.is_spacetime_source = false;
			apply_body_appearance_preset(b, Dynamics::Body3DPreset::Metallic);
			reverted = true;
		}
		render_setting_tooltip("Removes this body's gravitating-source status; it becomes an ordinary body without its own event horizon, no longer able to absorb or merge with other bodies, and moves to the Object Catalog tab.");
		ImGui::SameLine();
		if (ImGui::Button("Delete", ImVec2(70.0f, 24.0f))) {
			pending.action = CatalogRowAction::Delete;
			pending.action_id = static_cast<int>(body_id);
		}
		render_setting_tooltip("Permanently removes this black hole from the N-body system.");
		ImGui::Separator();

		const BodyEditResult result = render_body_editor(ctx, b, nullptr, view_, BodyEditorSection::SpacetimeSource);
		if (result.body_changed || reverted) {
			sys.update_accelerations();
			ctx.orchestrator().notify_state_changed();
		}
		if (reverted || !b.is_spacetime_source) {
			ctx.select_object(static_cast<int>(body_id));
			ctx.select_source(BodyManagerContext::kNoSelection);
			ctx.request_tab(BodyManagerTab::ObjectCatalog);
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

		size_t source_count = 0;
		size_t enabled_count = 0;
		for (const auto& body : bodies) {
			if (body.is_spacetime_source) {
				++source_count;
				if (body.enabled) ++enabled_count;
			}
		}

		const float font = ImGui::GetFontSize();
		CatalogListResult pending;

		split_pane_begin_primary("SourceCatalogList", split_, font * 15.0f, font * 26.0f);
		ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.35f, 1.0f), "Primary Metric Source");
		const auto& params = ctx.orchestrator().parameters();
		const bool central_tracked = ctx.is_tracking(BodyManagerContext::kCentralSourceId);
		std::string primary_label = "Central Object  (M " + Units::format_mass(params.mass * ctx.mass_scale(), ctx.orchestrator().unit_preferences().mass, 3) + ", a " + std::to_string(params.spin).substr(0, 5) + ")";
		if (central_tracked) {
			primary_label += "  [TRK]";
		}
		if (ImGui::Selectable(primary_label.c_str(), ctx.selected_source_id() == BodyManagerContext::kCentralSourceId)) {
			ctx.select_source(BodyManagerContext::kCentralSourceId);
		}
		render_setting_tooltip("The metric-generating central mass fixed at the origin. Select it to edit the mass, spin and charge that shape the background spacetime.");

		ImGui::Spacing();
		ImGui::TextColored(ImVec4(0.95f, 0.5f, 0.5f, 1.0f), "Independent Black Holes: %zu (%zu enabled)", source_count, enabled_count);
		render_setting_tooltip("Fully gravitating Kerr sources participating in N-body dynamics: they attract and are attracted by every other body, merge with other sources on contact, and absorb ordinary bodies crossing their horizon.");
		ImGui::Separator();
		if (source_count == 0) {
			render_hint_text("No independent black hole exists yet.");
			ImGui::Spacing();
			if (ImGui::Button("Create Source", ImVec2(-1.0f, 28.0f))) {
				ctx.request_tab(BodyManagerTab::SourceCreation);
			}
			render_setting_tooltip("Opens the Create Source tab so you can configure and spawn an independent black hole.");
			ImGui::Spacing();
			if (ImGui::Button("Spawn Companion In Orbit", ImVec2(-1.0f, 26.0f))) {
				ctx.spawn_companion_source();
			}
			render_setting_tooltip("Creates a black hole roughly half the mass of the primary source on a wide, randomly inclined circular orbit.");
		} else {
			pending = render_body_catalog_list(ctx, bodies, true, filter_, ctx.selected_source_id());
			if (pending.clicked_id >= 0) {
				ctx.select_source(pending.clicked_id);
			}
		}
		split_pane_end_primary(split_);

		split_pane_begin_secondary("SourceCatalogDetail", split_);
		if (ctx.selected_source_id() == BodyManagerContext::kCentralSourceId) {
			render_primary_source_detail(ctx);
		} else {
			Dynamics::PostNewtonianBody* selected = nullptr;
			for (auto& body : bodies) {
				if (body.is_spacetime_source && static_cast<int>(body.id) == ctx.selected_source_id()) {
					selected = &body;
					break;
				}
			}
			if (selected != nullptr) {
				render_source_detail(ctx, sys, *selected, pending);
			} else {
				render_hint_text("Select the primary metric source or an independent black hole in the list to inspect or edit it. Right-click an entry for quick actions.");
			}
		}
		split_pane_end_secondary();

		apply_catalog_row_action(ctx, pending);
	}
};

}

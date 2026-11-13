#pragma once

#include <imgui.h>
#include "relativistic/ui/body_manager/body_manager_context.hpp"
#include "relativistic/ui/body_manager/body_property_editor.hpp"
#include "relativistic/ui/split_pane_layout.hpp"
#include "relativistic/ui/panel_text_helpers.hpp"
#include "relativistic/ui/tooltip_utils.hpp"
#include "relativistic/units/unit_system.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <string>

namespace Relativistic::UI {

class SourceCreationTab {
private:
	Dynamics::PostNewtonianBody draft_{};
	BodyEditorViewState view_{};
	SplitPaneState split_{320.0f, 320.0f, 0.0f};
	int template_index_{0};
	bool pristine_{true};
	double reference_mass_{-1.0};
	std::string last_spawned_name_{};
	uint32_t last_spawned_id_{0};
	bool spawn_notification_active_{false};

	void apply_template(BodyManagerContext& ctx, SourcePresetTemplate preset) noexcept {
		if (preset == SourcePresetTemplate::Custom) {
			reset_draft(ctx);
			return;
		}
		const size_t index = static_cast<size_t>(preset) - 1U;
		if (index >= kSourceTemplateSpecs.size()) return;
		const auto& spec = kSourceTemplateSpecs[index];
		apply_source_template_preset(draft_, preset, ctx.mass_scale(), ctx.orchestrator().parameters().mass);
		const double central_mass = std::max(ctx.orchestrator().parameters().mass, 1.0);
		const double orbit_radius = central_mass * spec.orbit_in_central_masses;
		const double inclination = spec.inclination_rad;
		const double theta = std::numbers::pi * 0.5 - inclination;
		draft_.position = {orbit_radius * std::sin(theta), 0.0, orbit_radius * std::cos(theta)};
		draft_.velocity = ctx.compute_circular_orbit_velocity(draft_.position, central_mass + draft_.mass);
		draft_.set_name(ctx.unique_name(spec.name));
		ctx.resolve_source_placement(draft_);
		pristine_ = false;
	}

	void render_setup_pane(BodyManagerContext& ctx) noexcept {
		ImGui::TextColored(ImVec4(0.95f, 0.5f, 0.5f, 1.0f), "Black Hole Draft");
		render_hint_text("Configure an independent Kerr black hole in the editor on the right, then spawn it. It is fully N-body integrated: it can orbit, drift, merge with other sources on contact and absorb ordinary bodies crossing its horizon.");

		render_group_title("Starting Point", ImVec4(0.5f, 0.85f, 1.0f, 1.0f));
		int template_idx = template_index_;
		ImGui::SetNextItemWidth(-1.0f);
		if (ImGui::Combo("##SourceTemplate", &template_idx, kSourceTemplateNames.data(), static_cast<int>(kSourceTemplateNames.size()))) {
			template_index_ = template_idx;
			apply_template(ctx, static_cast<SourcePresetTemplate>(template_index_));
		}
		render_setting_tooltip("Fills the draft with realistic mass, spin, charge, and accretion disk profile for observed black holes.");
		if (template_index_ > 0 && static_cast<size_t>(template_index_ - 1) < kSourceTemplateSpecs.size()) {
			render_hint_text(kSourceTemplateSpecs[static_cast<size_t>(template_index_ - 1)].description.data());
		}

		if (ImGui::Button("Reset Draft To Defaults", ImVec2(-1.0f, 24.0f))) {
			template_index_ = static_cast<int>(SourcePresetTemplate::Custom);
			reset_draft(ctx);
		}
		render_setting_tooltip("Restores a quarter-mass black hole on a wide circular orbit around the primary source.");

		const Dynamics::PostNewtonianBody* selected = nullptr;
		{
			auto& sys = ctx.orchestrator().nbody_system();
			std::lock_guard<std::recursive_mutex> lock(sys.bodies_mutex());
			if (ctx.selected_source_id() >= 0) {
				const Dynamics::PostNewtonianBody* candidate = ctx.find_body(static_cast<uint32_t>(ctx.selected_source_id()));
				if (candidate != nullptr && candidate->is_spacetime_source) {
					selected = candidate;
				}
			}
			ImGui::BeginDisabled(selected == nullptr);
			if (ImGui::Button("Copy Settings From Selected Source", ImVec2(-1.0f, 24.0f)) && selected != nullptr) {
				draft_ = *selected;
				draft_.id = 0;
				draft_.acceleration = {0.0, 0.0, 0.0};
				draft_.set_name(ctx.unique_name(BodyManagerContext::display_name(*selected)));
				pristine_ = false;
			}
			ImGui::EndDisabled();
		}
		render_setting_tooltip("Loads every property and the accretion disk appearance of the black hole selected in the Source Catalog into the draft.");

		if (ImGui::Button("Spawn Companion In Wide Orbit (Randomized)", ImVec2(-1.0f, 24.0f))) {
			ctx.spawn_companion_source();
			ctx.request_tab(BodyManagerTab::SourceCatalog);
		}
		render_setting_tooltip("Immediately creates a black hole roughly half the mass of the primary source on a wide, randomly inclined circular orbit.");

		render_group_title("Draft vs System Comparator", ImVec4(1.0f, 0.75f, 0.35f, 1.0f));
		const auto& prefs = ctx.orchestrator().unit_preferences();
		const double length_scale_m = ctx.length_scale();
		const double central_m = std::max(ctx.orchestrator().parameters().mass, 1e-12);
		const double mass_ratio = draft_.mass / central_m;
		const double mass_kg = draft_.mass * ctx.mass_scale();
		const double draft_dist = BodyManagerContext::distance_from_center(draft_);
		const double horizon = draft_.kerr_outer_horizon_radius();
		const double primary_h = ctx.primary_horizon_radius();

		ImGui::TextWrapped("Name: %s", BodyManagerContext::display_name(draft_).c_str());
		ImGui::TextDisabled("Mass Ratio (M2 / M1): %.5f (%.3f%% of primary)", mass_ratio, mass_ratio * 100.0);
		ImGui::TextDisabled("Astrophysical Mass: %.3e M_sun", mass_kg / 1.98847e30);
		ImGui::TextDisabled("Outer Horizon: %s (%.4f M)", Units::format_distance(horizon * length_scale_m, prefs.distance, 3).c_str(), horizon / draft_.mass);
		if (primary_h > 0.0) {
			ImGui::TextDisabled("Horizon Scale Ratio (r_h,2 / r_h,1): %.4f", horizon / primary_h);
		}
		ImGui::TextDisabled("Orbital Separation: %s (%.2f r_g,1)", Units::format_distance(draft_dist * length_scale_m, prefs.distance, 3).c_str(), draft_dist / central_m);
		ImGui::TextDisabled("Coordinate Speed: %s", Units::format_velocity(draft_.speed() * ctx.speed_scale(), prefs.velocity).c_str());

		const auto draft_binary = ctx.compute_binary_configuration(draft_);
		if (draft_binary.valid) {
			ImGui::TextDisabled("Binary Metrics: q = %.4f | eta = %.4f | GW Coalescence: %s", draft_binary.mass_ratio, draft_binary.symmetric_mass_ratio, Units::format_time(draft_binary.peters_coalescence_time_seconds, prefs.time).c_str());
		}

		render_group_title("Anti-Collision & Separation Diagnostics", ImVec4(1.0f, 0.45f, 0.45f, 1.0f));
		const auto collision_diag = ctx.check_placement_collision(draft_.position, horizon, true);
		if (collision_diag.has_collision) {
			if (collision_diag.inside_primary_horizon) {
				render_caution_text("Separation Hazard: Draft horizon overlaps the primary central event horizon!");
			} else if (collision_diag.inside_source_horizon) {
				render_caution_text(("Separation Hazard: Horizon overlaps independent black hole " + collision_diag.colliding_body_name + "!").c_str());
			} else {
				render_caution_text(("Separation Hazard: Horizon intersects celestial body " + collision_diag.colliding_body_name + "!").c_str());
			}
			if (ImGui::Button("Move to Safe Separation", ImVec2(-1.0f, 26.0f))) {
				ctx.resolve_safe_placement(draft_.position, horizon, true);
				draft_.velocity = ctx.compute_circular_orbit_velocity(draft_.position, central_m + draft_.mass);
				pristine_ = false;
			}
			render_setting_tooltip("Pushes the black hole outward until its horizon is completely separated from the primary source and every other body by a safe margin, and recomputes Keplerian circular velocity.");
		} else {
			ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.5f, 1.0f), "Separation Status: Clear (No event horizon overlap)");
		}

		const auto draft_hazard = ctx.evaluate_trajectory_hazard(draft_.position, draft_.velocity, draft_.mass, draft_.radius, central_m + draft_.mass);
		if (draft_hazard.plunge_hazard) {
			render_caution_text("Trajectory Hazard: Periapsis falls inside the primary event horizon (direct plunge orbit).");
		}

		render_group_title("Spawn", ImVec4(0.4f, 0.9f, 0.6f, 1.0f));
		if (ImGui::Button("Spawn Black Hole", ImVec2(-1.0f, 32.0f))) {
			draft_.is_spacetime_source = true;
			last_spawned_name_ = BodyManagerContext::display_name(draft_);
			const uint32_t new_id = ctx.spawn_body(draft_, Dynamics::BodySurfaceLayerSet{});
			last_spawned_id_ = new_id;
			spawn_notification_active_ = true;
			ctx.select_source(static_cast<int>(new_id));
			reset_draft(ctx);
		}
		render_setting_tooltip("Adds a fully N-body integrated independent black hole with its own Kerr event horizon to the running simulation, using the mass, spin, charge, position and velocity configured in the editor.");

		if (spawn_notification_active_) {
			ImGui::Spacing();
			ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.2f, 0.85f, 0.45f, 0.8f));
			ImGui::BeginChild("##SpawnSourceNotice", ImVec2(0.0f, 56.0f), true);
			ImGui::TextColored(ImVec4(0.25f, 0.95f, 0.5f, 1.0f), "Black hole created successfully: %s (#%u)", last_spawned_name_.c_str(), last_spawned_id_);
			if (ImGui::SmallButton("Inspect in Source Catalog")) {
				ctx.select_source(static_cast<int>(last_spawned_id_));
				ctx.request_tab(BodyManagerTab::SourceCatalog);
			}
			ImGui::SameLine();
			if (ImGui::SmallButton("Look At")) {
				const auto* b = ctx.find_body(last_spawned_id_);
				if (b != nullptr) ctx.look_at(b->position);
			}
			ImGui::SameLine();
			if (ImGui::SmallButton("Track")) {
				ctx.set_tracking(static_cast<int>(last_spawned_id_), true);
			}
			ImGui::EndChild();
			ImGui::PopStyleColor();
		}
	}

public:
	[[nodiscard]] BodyEditorViewState& view() noexcept {
		return view_;
	}

	[[nodiscard]] SplitPaneState& split() noexcept {
		return split_;
	}

	void reset_draft(BodyManagerContext& ctx) noexcept {
		auto& d = draft_;
		d = Dynamics::PostNewtonianBody{};
		const double central_mass = std::max(ctx.orchestrator().parameters().mass, 1.0);
		d.mass = 0.25 * central_mass;
		d.radius = 2.0 * d.mass;
		d.reference_radius = d.radius;
		d.position = {30.0 * central_mass, 0.0, 0.0};
		d.velocity = ctx.compute_circular_orbit_velocity(d.position, central_mass + d.mass);
		pristine_ = true;
		reference_mass_ = ctx.orchestrator().parameters().mass;
		d.is_spacetime_source = true;
		apply_body_appearance_preset(d, Dynamics::Body3DPreset::BlackHole);
		d.set_name(ctx.unique_name("New Black Hole"));
	}

	void render(BodyManagerContext& ctx) noexcept {
		if (pristine_ && std::abs(ctx.orchestrator().parameters().mass - reference_mass_) > 1e-9 * std::max(std::abs(reference_mass_), 1.0)) {
			reset_draft(ctx);
		}
		const float font = ImGui::GetFontSize();

		split_pane_begin_primary("SourceCreationSetup", split_, font * 16.0f, font * 28.0f);
		render_setup_pane(ctx);
		split_pane_end_primary(split_);

		split_pane_begin_secondary("SourceCreationEditor", split_);
		ImGui::PushID("BlackHoleCreationEditor");
		draft_.is_spacetime_source = true;
		const BodyEditResult edit = render_body_editor(ctx, draft_, nullptr, view_, BodyEditorSection::SpacetimeSource);
		if (edit.body_changed) {
			pristine_ = false;
		}
		ImGui::PopID();
		split_pane_end_secondary();
	}
};

}

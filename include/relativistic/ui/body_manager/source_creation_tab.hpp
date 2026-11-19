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
	bool sec_template_open_{true};
	bool sec_comparator_open_{false};
	bool sec_plane_open_{false};
	bool sec_hazard_open_{false};

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
		const auto& prefs = ctx.orchestrator().unit_preferences();
		const double length_scale_m = ctx.length_scale();
		const double central_m = std::max(ctx.orchestrator().parameters().mass, 1e-12);
		const double horizon = draft_.kerr_outer_horizon_radius();
		ImGui::TextColored(ImVec4(0.95f, 0.5f, 0.5f, 1.0f), "Black Hole Draft");
		render_hint_text("Configure an independent Kerr black hole in the editor on the right, then spawn it. It is fully N-body integrated: it can orbit, drift, merge with other sources on contact and absorb ordinary bodies crossing its horizon.");

		ImGui::SetNextItemOpen(sec_template_open_);
		sec_template_open_ = ImGui::CollapsingHeader("Starting Point & Astrophysical Template###SourceCreationTemplate");
		if (sec_template_open_) {
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
		}

		ImGui::SetNextItemOpen(sec_comparator_open_);
		sec_comparator_open_ = ImGui::CollapsingHeader("Draft vs System Comparator###SourceCreationComparator");
		if (sec_comparator_open_) {
		const double mass_ratio = draft_.mass / central_m;
		const double mass_kg = draft_.mass * ctx.mass_scale();
		const double draft_dist = BodyManagerContext::distance_from_center(draft_);
		const double primary_h = ctx.primary_horizon_radius();

		char src_draft_name_buf[32]{};
		const std::string_view cur_src_name = draft_.name_view();
		std::memcpy(src_draft_name_buf, cur_src_name.data(), std::min(cur_src_name.size(), sizeof(src_draft_name_buf) - 1));
		const float reroll_btn_w = 58.0f;
		const float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
		ImGui::SetNextItemWidth(std::max(ImGui::GetContentRegionAvail().x - reroll_btn_w - spacing - 48.0f, 60.0f));
		if (ImGui::InputTextWithHint("##SourceDraftNameInput", "Unnamed", src_draft_name_buf, sizeof(src_draft_name_buf))) {
			draft_.set_name(std::string_view(src_draft_name_buf));
			pristine_ = false;
		}
		render_setting_tooltip("Name of the draft black hole to be spawned.");
		ImGui::SameLine(0.0f, spacing);
		if (ImGui::Button("Reroll##SourceDraftNameBtn", ImVec2(reroll_btn_w, 0.0f))) {
			draft_.set_name(ctx.unique_name(ctx.generate_procedural_name(draft_)));
			pristine_ = false;
		}
		render_setting_tooltip("Generates another realistic procedural name for this draft black hole.");
		ImGui::SameLine();
		ImGui::TextUnformatted("Name");

		render_wrapped_disabled_format("Mass Ratio (M2 / M1): %.5f (%.3f%% of primary)", mass_ratio, mass_ratio * 100.0);
		render_wrapped_disabled_format("Astrophysical Mass: %.3e M_sun", mass_kg / 1.98847e30);
		render_wrapped_disabled_format("Outer Horizon: %s (%.4f M)", Units::format_distance(horizon * length_scale_m, prefs.distance, 3).c_str(), horizon / draft_.mass);
		if (primary_h > 0.0) {
			render_wrapped_disabled_format("Horizon Scale Ratio (r_h,2 / r_h,1): %.4f", horizon / primary_h);
		}
		render_wrapped_disabled_format("Orbital Separation: %s (%.2f r_g,1)", Units::format_distance(draft_dist * length_scale_m, prefs.distance, 3).c_str(), draft_dist / central_m);
		render_wrapped_disabled_format("Coordinate Speed: %s", Units::format_velocity(draft_.speed() * ctx.speed_scale(), prefs.velocity).c_str());

		const auto draft_binary = ctx.compute_binary_configuration(draft_);
		if (draft_binary.valid) {
			render_wrapped_disabled_format("Binary Metrics: q = %.4f | eta = %.4f | GW Coalescence: %s", draft_binary.mass_ratio, draft_binary.symmetric_mass_ratio, Units::format_time(draft_binary.peters_coalescence_time_seconds, prefs.time).c_str());
		}
		}

		ImGui::SetNextItemOpen(sec_plane_open_);
		sec_plane_open_ = ImGui::CollapsingHeader("Orbital Plane & Parent Alignment###SourceCreationPlane");
		if (sec_plane_open_) {
			render_hint_text("Specify the reference parent attractor and tilt the black hole orbital plane relative to the central equator or a parent's orbit.");

		std::vector<int> src_creation_ref_ids{BodyManagerContext::kCentralSourceId};
		std::vector<std::string> src_creation_ref_labels{"Central Spacetime Attractor (Origin #0)"};
		{
			auto& sys = ctx.orchestrator().nbody_system();
			std::lock_guard<std::recursive_mutex> lock(sys.bodies_mutex());
			for (const auto& b_item : sys.bodies()) {
				if (!b_item.enabled) continue;
				src_creation_ref_ids.push_back(static_cast<int>(b_item.id));
				src_creation_ref_labels.push_back(BodyManagerContext::display_name(b_item) + " (#" + std::to_string(b_item.id) + ")");
			}
		}

		int src_creation_ref_idx = 0;
		for (size_t i = 0; i < src_creation_ref_ids.size(); ++i) {
			if (src_creation_ref_ids[i] == view_.reference_frame_body_id) {
				src_creation_ref_idx = static_cast<int>(i);
				break;
			}
		}

		std::vector<const char*> src_creation_ref_ptrs;
		src_creation_ref_ptrs.reserve(src_creation_ref_labels.size());
		for (const auto& s : src_creation_ref_labels) src_creation_ref_ptrs.push_back(s.c_str());

		ImGui::TextUnformatted("Reference Parent Attractor:");
		ImGui::SetNextItemWidth(-1.0f);
		if (ImGui::Combo("##SourceDraftRefCombo", &src_creation_ref_idx, src_creation_ref_ptrs.data(), static_cast<int>(src_creation_ref_ptrs.size()))) {
			view_.reference_frame_body_id = src_creation_ref_ids[static_cast<size_t>(src_creation_ref_idx)];
			view_.plane_angles_initialized = false;
		}

		const auto draft_src_plane = ctx.compute_relative_orbital_plane_state(draft_, view_.reference_frame_body_id);
		if (draft_src_plane.valid) {
			const double signed_tilt = draft_src_plane.signed_inclination_offset_deg;
			const ImVec4 tilt_badge_col = (signed_tilt >= 0.0)
				? ImVec4(0.35f, 0.95f, 0.45f, 1.0f)
				: ImVec4(1.0f, 0.35f, 0.35f, 1.0f);
			char tilt_buf[128];
			std::snprintf(tilt_buf, sizeof(tilt_buf), "%+.*f deg", 2, signed_tilt);
			render_wrapped_colored_text(tilt_badge_col, ("Tilt: " + std::string(tilt_buf) + " relative to " + draft_src_plane.reference_name).c_str());
			render_wrapped_disabled_format("Mutual Plane Angle: %.2f deg | Alignment: %s", draft_src_plane.relative_inclination_deg, draft_src_plane.alignment_category);

			if (draft_src_plane.is_in_kozai_zone) {
				render_caution_text(draft_src_plane.kozai_warning);
			}

			if (view_.plane_initialized_for_body != draft_.id || !view_.plane_angles_initialized) {
				view_.plane_target_inclination_deg = static_cast<float>(draft_src_plane.relative_inclination_deg);
				view_.plane_target_node_deg = static_cast<float>(draft_src_plane.relative_node_deg);
				view_.plane_target_periapsis_deg = static_cast<float>(draft_src_plane.relative_periapsis_deg);
				view_.plane_angles_initialized = true;
				view_.plane_initialized_for_body = draft_.id;
			}

			bool src_plane_changed = false;
			float d_inc = view_.plane_target_inclination_deg;
			if (slider_float_with_input("Draft Inclination (i_rel)##SourceDraft", &d_inc, 0.0f, 180.0f, "%.2f deg")) {
				view_.plane_target_inclination_deg = d_inc;
				src_plane_changed = true;
			}
			float d_node = view_.plane_target_node_deg;
			if (slider_float_with_input("Draft Node (Omega_rel)##SourceDraft", &d_node, 0.0f, 360.0f, "%.2f deg")) {
				view_.plane_target_node_deg = d_node;
				src_plane_changed = true;
			}
			float d_peri = view_.plane_target_periapsis_deg;
			if (slider_float_with_input("Draft Periapsis (omega_rel)##SourceDraft", &d_peri, 0.0f, 360.0f, "%.2f deg")) {
				view_.plane_target_periapsis_deg = d_peri;
				src_plane_changed = true;
			}

			if (src_plane_changed) {
				ctx.apply_relative_orbital_plane_rotation(
					draft_, view_.reference_frame_body_id,
					static_cast<double>(view_.plane_target_inclination_deg),
					static_cast<double>(view_.plane_target_node_deg),
					static_cast<double>(view_.plane_target_periapsis_deg)
				);
				pristine_ = false;
			}

			const float half_btn_w = std::max((ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f, 60.0f);
			if (ImGui::Button("Coplanar (0 deg)##SourceDraft", ImVec2(half_btn_w, 24.0f))) {
				view_.plane_target_inclination_deg = 0.0f;
				ctx.apply_relative_orbital_plane_rotation(draft_, view_.reference_frame_body_id, 0.0, static_cast<double>(view_.plane_target_node_deg), static_cast<double>(view_.plane_target_periapsis_deg));
				pristine_ = false;
			}
			ImGui::SameLine();
			if (ImGui::Button("Polar (90 deg)##SourceDraft", ImVec2(half_btn_w, 24.0f))) {
				view_.plane_target_inclination_deg = 90.0f;
				ctx.apply_relative_orbital_plane_rotation(draft_, view_.reference_frame_body_id, 90.0, static_cast<double>(view_.plane_target_node_deg), static_cast<double>(view_.plane_target_periapsis_deg));
				pristine_ = false;
			}
			if (ImGui::Button("Retrograde (180 deg)##SourceDraft", ImVec2(half_btn_w, 24.0f))) {
				view_.plane_target_inclination_deg = 180.0f;
				ctx.apply_relative_orbital_plane_rotation(draft_, view_.reference_frame_body_id, 180.0, static_cast<double>(view_.plane_target_node_deg), static_cast<double>(view_.plane_target_periapsis_deg));
				pristine_ = false;
			}
			ImGui::SameLine();
			if (ImGui::Button("Circularize##SourceDraft", ImVec2(half_btn_w, 24.0f))) {
				ctx.circularize_around_reference(draft_, view_.reference_frame_body_id);
				view_.plane_angles_initialized = false;
				pristine_ = false;
			}
		}
		}

		ImGui::SetNextItemOpen(sec_hazard_open_);
		sec_hazard_open_ = ImGui::CollapsingHeader("Anti-Collision & Separation Diagnostics###SourceCreationHazard");
		if (sec_hazard_open_) {
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
			render_wrapped_colored_text(ImVec4(0.3f, 0.9f, 0.5f, 1.0f), "Separation Status: Clear (No event horizon overlap)");
		}

		const auto draft_hazard = ctx.evaluate_trajectory_hazard(draft_.position, draft_.velocity, draft_.mass, draft_.radius, central_m + draft_.mass);
		if (draft_hazard.plunge_hazard) {
			render_caution_text("Trajectory Hazard: Periapsis falls inside the primary event horizon (direct plunge orbit).");
		}
		}

		render_group_title("Spawn", ImVec4(0.4f, 0.9f, 0.6f, 1.0f));
		if (ImGui::Button("Spawn Black Hole", ImVec2(-1.0f, 32.0f))) {
			draft_.is_spacetime_source = true;
			const uint32_t new_id = ctx.spawn_body(draft_, Dynamics::BodySurfaceLayerSet{});
			const auto* spawned = ctx.find_body(new_id);
			last_spawned_name_ = (spawned != nullptr) ? BodyManagerContext::display_name(*spawned) : BodyManagerContext::display_name(draft_);
			last_spawned_id_ = new_id;
			spawn_notification_active_ = true;
			ctx.select_source(static_cast<int>(new_id));
			reset_draft(ctx);
		}
		render_setting_tooltip("Adds a fully N-body integrated independent black hole with its own Kerr event horizon to the running simulation, using the mass, spin, charge, position and velocity configured in the editor.");

		if (spawn_notification_active_) {
			ImGui::Spacing();
			ImGui::Separator();
			render_wrapped_colored_text(ImVec4(0.25f, 0.95f, 0.5f, 1.0f), ("Black hole created: " + last_spawned_name_ + " (#" + std::to_string(last_spawned_id_) + ")").c_str());
			if (ImGui::SmallButton("Inspect in Catalog##SrcNotice")) {
				ctx.select_source(static_cast<int>(last_spawned_id_));
				ctx.request_tab(BodyManagerTab::SourceCatalog);
			}
			ImGui::SameLine();
			if (ImGui::SmallButton("Look At##SrcNotice")) {
				const auto* b = ctx.find_body(last_spawned_id_);
				if (b != nullptr) ctx.look_at(b->position);
			}
			ImGui::SameLine();
			if (ImGui::SmallButton("Track##SrcNotice")) {
				ctx.set_tracking(static_cast<int>(last_spawned_id_), true);
			}
			ImGui::Separator();
		}
	}

public:
	[[nodiscard]] BodyEditorViewState& view() noexcept {
		return view_;
	}

	[[nodiscard]] SplitPaneState& split() noexcept {
		return split_;
	}

	[[nodiscard]] bool& sec_template_open() noexcept { return sec_template_open_; }
	[[nodiscard]] bool& sec_comparator_open() noexcept { return sec_comparator_open_; }
	[[nodiscard]] bool& sec_plane_open() noexcept { return sec_plane_open_; }
	[[nodiscard]] bool& sec_hazard_open() noexcept { return sec_hazard_open_; }

	void reset_draft(BodyManagerContext& ctx) noexcept {
		auto& d = draft_;
		d = Dynamics::PostNewtonianBody{};
		const auto& rnd = ctx.randomizer_config();
		const double central_mass = std::max(ctx.orchestrator().parameters().mass, 1.0);
		const double mass_ratio = ctx.sample_uniform(rnd.min_source_mass_ratio, rnd.max_source_mass_ratio);
		d.mass = std::max(central_mass * mass_ratio, 1e-6);
		const double spin = ctx.sample_uniform(rnd.min_source_spin, rnd.max_source_spin);
		d.charge = ctx.sample_uniform(rnd.min_source_charge, rnd.max_source_charge) * d.mass;
		const double orbit_rg = ctx.sample_uniform(rnd.min_source_orbit_rg, rnd.max_source_orbit_rg);
		const double orbit_r = central_mass * orbit_rg;
		const double tilt_rad = ctx.sample_uniform(0.0, rnd.max_source_tilt_deg * (std::numbers::pi / 180.0));
		const double theta = std::numbers::pi * 0.5 - tilt_rad;
		const double phi = ctx.random_real(0.0, 2.0 * std::numbers::pi);
		const double sin_t = std::sin(theta);
		d.position = {orbit_r * sin_t * std::cos(phi), orbit_r * sin_t * std::sin(phi), orbit_r * std::cos(theta)};
		d.velocity = ctx.compute_circular_orbit_velocity(d.position, central_mass + d.mass);
		const std::array<double, 3> spin_axis = (ctx.random_real(0.0, 1.0) < 0.7)
			? std::array<double, 3>{0.0, 0.0, 1.0}
			: std::array<double, 3>{ctx.random_real(-1.0, 1.0), ctx.random_real(-1.0, 1.0), ctx.random_real(0.1, 1.0)};
		d.set_spin_state(spin, spin_axis);
		d.radius = d.kerr_outer_horizon_radius();
		d.reference_radius = d.radius;
		d.is_spacetime_source = true;
		apply_body_appearance_preset(d, Dynamics::Body3DPreset::BlackHole);
		d.accretion_disk.enabled = (ctx.random_real(0.0, 1.0) < rnd.source_disk_probability);
		if (d.accretion_disk.enabled) {
			d.accretion_disk.peak_temperature_k = static_cast<float>(ctx.sample_uniform(rnd.min_source_disk_temp_k, rnd.max_source_disk_temp_k));
			d.accretion_disk.outer_radius_mass_units = static_cast<float>(ctx.sample_uniform(rnd.min_source_disk_outer_rg, rnd.max_source_disk_outer_rg));
			d.accretion_disk.brightness = static_cast<float>(ctx.sample_uniform(rnd.min_source_disk_brightness, rnd.max_source_disk_brightness));
			d.accretion_disk.inner_radius_scale = 1.0f;
		}
		pristine_ = true;
		reference_mass_ = ctx.orchestrator().parameters().mass;
		d.set_name(ctx.unique_name(ctx.generate_source_name()));
		ctx.resolve_source_placement(d);
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

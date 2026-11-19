#pragma once

#include <imgui.h>
#include "relativistic/ui/body_manager/body_manager_context.hpp"
#include "relativistic/ui/body_manager/body_property_editor.hpp"
#include "relativistic/ui/body_manager/body_randomizer_config.hpp"
#include "relativistic/ui/body_manager/system_presets.hpp"
#include "relativistic/ui/split_pane_layout.hpp"
#include "relativistic/ui/panel_text_helpers.hpp"
#include "relativistic/ui/tooltip_utils.hpp"
#include "relativistic/units/unit_system.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <string>

namespace Relativistic::UI {

class BodyCreationTab {
private:
	enum class CreationSubMode : int {
		SingleBodyDraft = 0,
		GeometricPatternGenerator = 1
	};
	CreationSubMode creation_sub_mode_{CreationSubMode::SingleBodyDraft};
	Dynamics::PostNewtonianBody draft_{};
	Dynamics::BodySurfaceLayerSet layers_{};
	BodyEditorViewState view_{};
	SplitPaneState split_{320.0f, 320.0f, 0.0f};
	BodyRandomizerConfig randomizer_config_{};
	int template_index_{0};
	bool pristine_{true};
	double reference_mass_{-1.0};
	bool randomize_after_spawn_{true};
	std::string last_spawned_name_{};
	uint32_t last_spawned_id_{0};
	bool spawn_notification_active_{false};
	bool sec_template_open_{true};
	bool sec_comparator_open_{false};
	bool sec_plane_open_{false};
	bool sec_hazard_open_{false};

	GeometricPatternConfig pattern_config_{};
	int pattern_preset_index_{0};
	bool pattern_radial_thickness_log_mode_{false};
	bool pattern_scale_height_log_mode_{false};
	bool pattern_min_mass_log_mode_{true};
	bool pattern_max_mass_log_mode_{true};
	bool pattern_fixed_mass_log_mode_{true};
	bool pattern_density_log_mode_{false};
	std::string last_pattern_notification_{};
	bool pattern_notification_active_{false};

	void apply_template(BodyManagerContext& ctx, BodyPresetTemplate preset) noexcept {
		if (preset == BodyPresetTemplate::Custom) {
			reset_draft(ctx);
			return;
		}
		const size_t index = static_cast<size_t>(preset) - 1U;
		if (index >= kBodyTemplateSpecs.size()) return;
		const auto& spec = kBodyTemplateSpecs[index];
		auto& d = draft_;
		const std::array<double, 3> kept_position = d.position;
		const std::array<double, 3> kept_velocity = d.velocity;
		d = Dynamics::PostNewtonianBody{};
		apply_body_appearance_preset(d, spec.preset);
		d.position = kept_position;
		d.velocity = kept_velocity;
		d.mass = spec.native_units ? spec.mass : spec.mass / ctx.mass_scale();
		d.radius = spec.native_units ? spec.radius : spec.radius / ctx.length_scale();
		d.reference_radius = d.radius;

		if (!spec.native_units && spec.semi_major_axis_m > 0.0) {
			const double dist_sim = spec.semi_major_axis_m / ctx.length_scale();
			const double cur_dist = std::sqrt(d.position[0] * d.position[0] + d.position[1] * d.position[1] + d.position[2] * d.position[2]);
			if (cur_dist > 1e-6) {
				const double scale = dist_sim / cur_dist;
				d.position[0] *= scale;
				d.position[1] *= scale;
				d.position[2] *= scale;
			} else {
				d.position = {dist_sim, 0.0, 0.0};
			}
		}

		d.j2 = spec.j2;
		d.j4 = spec.j4;
		d.temperature = spec.temperature_kelvin;
		d.color = spec.color;
		d.color_secondary = spec.color_secondary;
		d.color_tertiary = spec.color_tertiary;
		d.surface_noise_scale = spec.noise_scale;
		d.surface_roughness = spec.surface_roughness;
		d.atmosphere_thickness = spec.atmosphere_thickness;
		d.emission_intensity = spec.emission_intensity;
		d.rotation_speed_3d = spec.rotation_speed;
		d.polar_cap_strength = spec.polar_cap_strength;
		d.night_side_light_intensity = spec.night_side_light_intensity;
		d.ring_system_enabled = spec.ring_system;
		d.set_name(ctx.unique_name(spec.name));

		layers_ = Dynamics::BodySurfaceLayerSet{};
		for (const int layer_template : {spec.layer_template_first, spec.layer_template_second}) {
			if (layer_template >= 0) {
				static_cast<void>(layers_.add(Dynamics::SurfaceLayerDefinition::from_template(static_cast<Dynamics::SurfaceLayerTemplate>(layer_template))));
			}
		}
		d.velocity = ctx.compute_circular_orbit_velocity(d.position, std::max(ctx.orchestrator().parameters().mass, 1e-12));
		pristine_ = false;
	}

	void render_setup_pane(BodyManagerContext& ctx) noexcept {
		const auto& prefs = ctx.orchestrator().unit_preferences();
		const double central_m = std::max(ctx.orchestrator().parameters().mass, 1e-12);
		ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.6f, 1.0f), "Object Draft");
		render_hint_text("Configure every property of a new orbiting body in the editor on the right, then spawn it into the running system. Independent black holes are created in the Create Source tab.");

		ImGui::SetNextItemOpen(sec_template_open_);
		sec_template_open_ = ImGui::CollapsingHeader("Starting Point & Archetype Preset###BodyCreationTemplate");
		if (sec_template_open_) {
			int template_idx = template_index_;
			ImGui::SetNextItemWidth(-1.0f);
			if (ImGui::Combo("##ObjectTemplate", &template_idx, kBodyTemplateNames.data(), static_cast<int>(kBodyTemplateNames.size()))) {
				template_index_ = template_idx;
				apply_template(ctx, static_cast<BodyPresetTemplate>(template_index_));
			}
			render_setting_tooltip("Fills the draft with realistic physical values, appearance and texture layers for a known body type. Real-world values are converted to the active simulation units. Position and velocity are kept.");
			render_hint_text("Template presets load realistic values; Custom Body generates a random coherent draft.");
			if (template_index_ == 7 || template_index_ == 8) {
				render_caution_text("Notice: This template creates an ordinary body rendered as a black hole visual artifact. To create an active Kerr spacetime source with its own event horizon that participates in N-body gravity, absorbs matter and merges with other sources, use the 'Create Source' tab.");
				if (ImGui::SmallButton("Switch To Create Source Tab")) {
					ctx.request_tab(BodyManagerTab::SourceCreation);
				}
			}

			if (ImGui::Button("Randomize Intelligent Defaults", ImVec2(-1.0f, 24.0f))) {
				reset_draft(ctx);
			}
			render_setting_tooltip("Generates a coherent body profile by sampling correlated mass, radius, orbit, spin, multipoles, appearance and name.");

			const Dynamics::PostNewtonianBody* selected = nullptr;
			{
				auto& sys = ctx.orchestrator().nbody_system();
				std::lock_guard<std::recursive_mutex> lock(sys.bodies_mutex());
				const Dynamics::PostNewtonianBody* candidate = ctx.find_body(static_cast<uint32_t>(std::max(ctx.selected_object_id(), 0)));
				if (ctx.selected_object_id() >= 0 && candidate != nullptr && !candidate->is_spacetime_source) {
					selected = candidate;
				}
				ImGui::BeginDisabled(selected == nullptr);
				if (ImGui::Button("Copy Settings From Selected Object", ImVec2(-1.0f, 24.0f)) && selected != nullptr) {
					layers_ = ctx.orchestrator().surface_layers().get(selected->id);
					draft_ = *selected;
					draft_.id = 0;
					draft_.acceleration = {0.0, 0.0, 0.0};
					draft_.set_name(ctx.unique_name(BodyManagerContext::display_name(*selected)));
					template_index_ = static_cast<int>(BodyPresetTemplate::Custom);
					pristine_ = false;
				}
				ImGui::EndDisabled();
			}
			render_setting_tooltip("Loads every property, the appearance and all texture layers of the object selected in the Object Catalog into the draft.");
		}

		ImGui::SetNextItemOpen(sec_comparator_open_);
		sec_comparator_open_ = ImGui::CollapsingHeader("Draft vs System Comparator###BodyCreationComparator");
		if (sec_comparator_open_) {
		const double mass_ratio = draft_.mass / central_m;
		const double mass_kg = draft_.mass * ctx.mass_scale();
		const double radius_m = draft_.radius * ctx.length_scale();
		const double dist_m = BodyManagerContext::distance_from_center(draft_) * ctx.length_scale();
		const double draft_dist = BodyManagerContext::distance_from_center(draft_);

		char draft_name_buf[32]{};
		const std::string_view cur_draft_name = draft_.name_view();
		std::memcpy(draft_name_buf, cur_draft_name.data(), std::min(cur_draft_name.size(), sizeof(draft_name_buf) - 1));
		const float reroll_btn_w = 58.0f;
		const float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
		ImGui::SetNextItemWidth(std::max(ImGui::GetContentRegionAvail().x - reroll_btn_w - spacing - 48.0f, 60.0f));
		if (ImGui::InputTextWithHint("##DraftNameInput", "Unnamed", draft_name_buf, sizeof(draft_name_buf))) {
			draft_.set_name(std::string_view(draft_name_buf));
			pristine_ = false;
		}
		render_setting_tooltip("Name of the draft body to be spawned.");
		ImGui::SameLine(0.0f, spacing);
		if (ImGui::Button("Reroll##DraftNameBtn", ImVec2(reroll_btn_w, 0.0f))) {
			draft_.set_name(ctx.unique_name(ctx.generate_procedural_name(draft_)));
			pristine_ = false;
		}
		render_setting_tooltip("Generates another realistic procedural name for this draft.");
		ImGui::SameLine();
		ImGui::TextUnformatted("Name");

		render_wrapped_disabled_format("Mass Ratio (M / M_central): %.4e (%.4f%%)", mass_ratio, mass_ratio * 100.0);
		render_wrapped_disabled_format("Astrophysical Mass: %.3e M_sun | %.3e M_earth", mass_kg / 1.98847e30, mass_kg / 5.9722e24);
		render_wrapped_disabled_format("Radius vs Reference: %.3f R_earth | %.4f R_jup | %.5f R_sun", radius_m / 6.378137e6, radius_m / 7.1492e7, radius_m / 6.9634e8);
		render_wrapped_disabled_format("Orbital Separation: %s (%.2f r_g)", Units::format_distance(dist_m, prefs.distance, 3).c_str(), draft_dist / central_m);
		render_wrapped_disabled_format("Instantaneous Speed: %s", Units::format_velocity(draft_.speed() * ctx.speed_scale(), prefs.velocity).c_str());
		render_wrapped_disabled_format("Texture Layers Assigned: %u", layers_.count);

		const auto draft_orbit = ctx.compute_keplerian_elements(draft_.position, draft_.velocity, central_m);
		if (draft_orbit.valid) {
			render_wrapped_disabled_format("Orbital Type: %s (%s) | e = %.4f | i = %.1f deg", draft_orbit.type_name, draft_orbit.is_prograde ? "Prograde" : "Retrograde", draft_orbit.eccentricity, draft_orbit.inclination_deg);
			if (draft_orbit.is_bound) {
				render_wrapped_disabled_format("Keplerian Period: %s", Units::format_time(draft_orbit.period_seconds, prefs.time).c_str());
			}
		}
		}

		ImGui::SetNextItemOpen(sec_plane_open_);
		sec_plane_open_ = ImGui::CollapsingHeader("Orbital Plane & Parent Alignment###BodyCreationPlane");
		if (sec_plane_open_) {
			render_hint_text("Specify the reference parent attractor and tilt the draft orbital plane relative to the central equator or a parent's orbit.");

		std::vector<int> creation_ref_ids{BodyManagerContext::kCentralSourceId};
		std::vector<std::string> creation_ref_labels{"Central Spacetime Attractor (Origin #0)"};
		{
			auto& sys = ctx.orchestrator().nbody_system();
			std::lock_guard<std::recursive_mutex> lock(sys.bodies_mutex());
			for (const auto& b_item : sys.bodies()) {
				if (!b_item.enabled) continue;
				creation_ref_ids.push_back(static_cast<int>(b_item.id));
				creation_ref_labels.push_back(BodyManagerContext::display_name(b_item) + " (#" + std::to_string(b_item.id) + ")");
			}
		}

		int creation_ref_idx = 0;
		for (size_t i = 0; i < creation_ref_ids.size(); ++i) {
			if (creation_ref_ids[i] == view_.reference_frame_body_id) {
				creation_ref_idx = static_cast<int>(i);
				break;
			}
		}

		std::vector<const char*> creation_ref_ptrs;
		creation_ref_ptrs.reserve(creation_ref_labels.size());
		for (const auto& s : creation_ref_labels) creation_ref_ptrs.push_back(s.c_str());

		ImGui::TextUnformatted("Reference Parent Attractor:");
		ImGui::SetNextItemWidth(-1.0f);
		if (ImGui::Combo("##DraftRefParentCombo", &creation_ref_idx, creation_ref_ptrs.data(), static_cast<int>(creation_ref_ptrs.size()))) {
			view_.reference_frame_body_id = creation_ref_ids[static_cast<size_t>(creation_ref_idx)];
			view_.plane_angles_initialized = false;
		}

		const auto draft_plane = ctx.compute_relative_orbital_plane_state(draft_, view_.reference_frame_body_id);
		if (draft_plane.valid) {
			const double signed_tilt = draft_plane.signed_inclination_offset_deg;
			const ImVec4 tilt_badge_col = (signed_tilt >= 0.0)
				? ImVec4(0.35f, 0.95f, 0.45f, 1.0f)
				: ImVec4(1.0f, 0.35f, 0.35f, 1.0f);
			char tilt_buf[128];
			std::snprintf(tilt_buf, sizeof(tilt_buf), "%+.*f deg", 2, signed_tilt);
			render_wrapped_colored_text(tilt_badge_col, ("Tilt: " + std::string(tilt_buf) + " relative to " + draft_plane.reference_name).c_str());
			render_wrapped_disabled_format("Mutual Plane Angle: %.2f deg | Alignment: %s", draft_plane.relative_inclination_deg, draft_plane.alignment_category);

			if (draft_plane.is_in_kozai_zone) {
				render_caution_text(draft_plane.kozai_warning);
			}

			if (view_.plane_initialized_for_body != draft_.id || !view_.plane_angles_initialized) {
				view_.plane_target_inclination_deg = static_cast<float>(draft_plane.relative_inclination_deg);
				view_.plane_target_node_deg = static_cast<float>(draft_plane.relative_node_deg);
				view_.plane_target_periapsis_deg = static_cast<float>(draft_plane.relative_periapsis_deg);
				view_.plane_angles_initialized = true;
				view_.plane_initialized_for_body = draft_.id;
			}

			bool draft_plane_changed = false;
			float d_inc = view_.plane_target_inclination_deg;
			if (slider_float_with_input("Draft Inclination (i_rel)", &d_inc, 0.0f, 180.0f, "%.2f deg")) {
				view_.plane_target_inclination_deg = d_inc;
				draft_plane_changed = true;
			}
			float d_node = view_.plane_target_node_deg;
			if (slider_float_with_input("Draft Node (Omega_rel)", &d_node, 0.0f, 360.0f, "%.2f deg")) {
				view_.plane_target_node_deg = d_node;
				draft_plane_changed = true;
			}
			float d_peri = view_.plane_target_periapsis_deg;
			if (slider_float_with_input("Draft Periapsis (omega_rel)", &d_peri, 0.0f, 360.0f, "%.2f deg")) {
				view_.plane_target_periapsis_deg = d_peri;
				draft_plane_changed = true;
			}

			if (draft_plane_changed) {
				ctx.apply_relative_orbital_plane_rotation(
					draft_, view_.reference_frame_body_id,
					static_cast<double>(view_.plane_target_inclination_deg),
					static_cast<double>(view_.plane_target_node_deg),
					static_cast<double>(view_.plane_target_periapsis_deg)
				);
				pristine_ = false;
			}

			const float half_btn_w = std::max((ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f, 60.0f);
			if (ImGui::Button("Coplanar (0 deg)##Draft", ImVec2(half_btn_w, 24.0f))) {
				view_.plane_target_inclination_deg = 0.0f;
				ctx.apply_relative_orbital_plane_rotation(draft_, view_.reference_frame_body_id, 0.0, static_cast<double>(view_.plane_target_node_deg), static_cast<double>(view_.plane_target_periapsis_deg));
				pristine_ = false;
			}
			ImGui::SameLine();
			if (ImGui::Button("Polar (90 deg)##Draft", ImVec2(half_btn_w, 24.0f))) {
				view_.plane_target_inclination_deg = 90.0f;
				ctx.apply_relative_orbital_plane_rotation(draft_, view_.reference_frame_body_id, 90.0, static_cast<double>(view_.plane_target_node_deg), static_cast<double>(view_.plane_target_periapsis_deg));
				pristine_ = false;
			}
			if (ImGui::Button("Retrograde (180 deg)##Draft", ImVec2(half_btn_w, 24.0f))) {
				view_.plane_target_inclination_deg = 180.0f;
				ctx.apply_relative_orbital_plane_rotation(draft_, view_.reference_frame_body_id, 180.0, static_cast<double>(view_.plane_target_node_deg), static_cast<double>(view_.plane_target_periapsis_deg));
				pristine_ = false;
			}
			ImGui::SameLine();
			if (ImGui::Button("Circularize##Draft", ImVec2(half_btn_w, 24.0f))) {
				ctx.circularize_around_reference(draft_, view_.reference_frame_body_id);
				view_.plane_angles_initialized = false;
				pristine_ = false;
			}
		}
		}

		ImGui::SetNextItemOpen(sec_hazard_open_);
		sec_hazard_open_ = ImGui::CollapsingHeader("Anti-Collision & Hazard Diagnostics###BodyCreationHazard");
		if (sec_hazard_open_) {
		const auto collision_diag = ctx.check_placement_collision(draft_.position, draft_.radius, false);
		if (collision_diag.has_collision) {
			if (collision_diag.inside_primary_horizon) {
				render_caution_text("Placement Hazard: Draft lies inside the central event horizon!");
			} else if (collision_diag.inside_source_horizon) {
				render_caution_text(("Placement Hazard: Draft lies inside the horizon of " + collision_diag.colliding_body_name + "!").c_str());
			} else {
				render_caution_text(("Placement Hazard: Direct physical collision with " + collision_diag.colliding_body_name + "!").c_str());
			}
			if (ImGui::Button("Move to Safe Orbit", ImVec2(-1.0f, 26.0f))) {
				ctx.resolve_safe_placement(draft_.position, draft_.radius, false);
				draft_.velocity = ctx.compute_circular_orbit_velocity(draft_.position, central_m);
				pristine_ = false;
			}
			render_setting_tooltip("Shifts the draft's orbital distance radially outward until it is completely clear of all celestial bodies and event horizons, and recomputes circular orbital velocity.");
		} else {
			render_wrapped_colored_text(ImVec4(0.3f, 0.9f, 0.5f, 1.0f), "Placement Status: Clear (No collision or horizon conflict)");
		}

		const auto draft_hazard = ctx.evaluate_trajectory_hazard(draft_.position, draft_.velocity, draft_.mass, draft_.radius, central_m);
		if (draft_hazard.plunge_hazard) {
			render_caution_text("Trajectory Hazard: Periapsis falls inside the central event horizon (plunge orbit).");
		} else if (draft_hazard.roche_hazard) {
			render_caution_text("Trajectory Hazard: Periapsis crosses the central Roche tidal disruption limit.");
		}
		}

		render_group_title("Spawn", ImVec4(0.4f, 0.9f, 0.6f, 1.0f));
		ImGui::Checkbox("Randomize Draft After Spawn", &randomize_after_spawn_);
		render_setting_tooltip("When enabled, a new random draft is generated after each spawn. When disabled, the draft is kept so several similar bodies can be spawned in a row.");
		if (ImGui::Button("Spawn and Inject into System", ImVec2(-1.0f, 32.0f))) {
			const uint32_t new_id = ctx.spawn_body(draft_, layers_);
			const auto* spawned = ctx.find_body(new_id);
			last_spawned_name_ = (spawned != nullptr) ? BodyManagerContext::display_name(*spawned) : BodyManagerContext::display_name(draft_);
			last_spawned_id_ = new_id;
			spawn_notification_active_ = true;
			ctx.select_object(static_cast<int>(new_id));
			if (randomize_after_spawn_) {
				reset_draft(ctx);
			} else {
				draft_.set_name(ctx.unique_name(BodyManagerContext::display_name(draft_)));
			}
		}
		render_setting_tooltip("Adds the configured body, with its full appearance and texture layers, to the running N-body system and selects it in the Object Catalog.");

		if (spawn_notification_active_) {
			ImGui::Spacing();
			ImGui::Separator();
			render_wrapped_colored_text(ImVec4(0.25f, 0.95f, 0.5f, 1.0f), ("Body created successfully: " + last_spawned_name_ + " (#" + std::to_string(last_spawned_id_) + ")").c_str());
			if (ImGui::SmallButton("Inspect in Catalog##Notice")) {
				ctx.select_object(static_cast<int>(last_spawned_id_));
				ctx.request_tab(BodyManagerTab::ObjectCatalog);
			}
			ImGui::SameLine();
			if (ImGui::SmallButton("Look At##Notice")) {
				const auto* b = ctx.find_body(last_spawned_id_);
				if (b != nullptr) ctx.look_at(b->position);
			}
			ImGui::SameLine();
			if (ImGui::SmallButton("Track##Notice")) {
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
	[[nodiscard]] BodyRandomizerConfig& randomizer_config() noexcept { return randomizer_config_; }
	[[nodiscard]] const BodyRandomizerConfig& randomizer_config() const noexcept { return randomizer_config_; }

	void reset_draft(BodyManagerContext& ctx) noexcept {
		randomizer_config_.sanitize();
		std::vector<Dynamics::Body3DPreset> allowed_presets;
		if (randomizer_config_.archetypes.allow_terrestrial) allowed_presets.push_back(Dynamics::Body3DPreset::TerrestrialPlanet);
		if (randomizer_config_.archetypes.allow_gas_giant) allowed_presets.push_back(Dynamics::Body3DPreset::GasGiant);
		if (randomizer_config_.archetypes.allow_ice_giant) allowed_presets.push_back(Dynamics::Body3DPreset::IceGiant);
		if (randomizer_config_.archetypes.allow_star) allowed_presets.push_back(Dynamics::Body3DPreset::Star);
		if (randomizer_config_.archetypes.allow_compact) allowed_presets.push_back(Dynamics::Body3DPreset::NeutronStar);
		if (randomizer_config_.archetypes.allow_asteroid) allowed_presets.push_back(Dynamics::Body3DPreset::Asteroid);
		if (allowed_presets.empty()) allowed_presets.push_back(Dynamics::Body3DPreset::TerrestrialPlanet);

		const auto chosen_preset = allowed_presets[static_cast<size_t>(ctx.random_int(0, static_cast<int>(allowed_presets.size()) - 1))];
		const double central_mass = std::max(ctx.orchestrator().parameters().mass, 1e-12);
		const double length_scale_m = ctx.length_scale();
		const double mass_scale_kg = ctx.mass_scale();

		auto& d = draft_;
		d = Dynamics::PostNewtonianBody{};
		apply_body_appearance_preset(d, chosen_preset);

		double mass_ratio = 1e-6;
		if (randomizer_config_.mass_distribution == RandomizerDistributionLaw::LogUniform) {
			mass_ratio = ctx.sample_log_uniform(randomizer_config_.min_mass_ratio, randomizer_config_.max_mass_ratio);
		} else if (randomizer_config_.mass_distribution == RandomizerDistributionLaw::Uniform) {
			mass_ratio = ctx.sample_uniform(randomizer_config_.min_mass_ratio, randomizer_config_.max_mass_ratio);
		} else if (randomizer_config_.mass_distribution == RandomizerDistributionLaw::PowerLaw) {
			mass_ratio = ctx.sample_power_law(randomizer_config_.min_mass_ratio, randomizer_config_.max_mass_ratio, randomizer_config_.mass_power_law_alpha);
		} else {
			const double mean_ratio = 0.5 * (randomizer_config_.min_mass_ratio + randomizer_config_.max_mass_ratio);
			const double sigma = 0.25 * (randomizer_config_.max_mass_ratio - randomizer_config_.min_mass_ratio);
			mass_ratio = std::clamp(ctx.sample_gaussian(mean_ratio, sigma), randomizer_config_.min_mass_ratio, randomizer_config_.max_mass_ratio);
		}
		d.mass = central_mass * mass_ratio;

		if (randomizer_config_.radius_mode == RandomizerRadiusMode::PhysicalDensity) {
			const double density = ctx.sample_uniform(randomizer_config_.min_density_kg_m3, randomizer_config_.max_density_kg_m3);
			const double mass_kg = d.mass * mass_scale_kg;
			const double volume_m3 = mass_kg / std::max(density, 1.0);
			const double radius_m = std::cbrt(3.0 * volume_m3 / (4.0 * std::numbers::pi));
			d.radius = std::max(radius_m / length_scale_m, 1e-6);
		} else if (randomizer_config_.radius_mode == RandomizerRadiusMode::ExplicitBounds) {
			const double rad_rg = ctx.sample_uniform(randomizer_config_.min_explicit_radius_rg, randomizer_config_.max_explicit_radius_rg);
			d.radius = std::max(rad_rg * central_mass, 1e-6);
		} else {
			d.radius = std::max(4.0 * d.mass, 1e-6);
		}
		d.reference_radius = d.radius;

		double orbit_rg = 30.0;
		if (randomizer_config_.orbit_distribution == RandomizerDistributionLaw::LogUniform) {
			orbit_rg = ctx.sample_log_uniform(randomizer_config_.min_orbit_rg, randomizer_config_.max_orbit_rg);
		} else if (randomizer_config_.orbit_distribution == RandomizerDistributionLaw::Uniform) {
			orbit_rg = ctx.sample_uniform(randomizer_config_.min_orbit_rg, randomizer_config_.max_orbit_rg);
		} else {
			orbit_rg = std::clamp(ctx.sample_gaussian(0.5 * (randomizer_config_.min_orbit_rg + randomizer_config_.max_orbit_rg), 0.25 * (randomizer_config_.max_orbit_rg - randomizer_config_.min_orbit_rg)), randomizer_config_.min_orbit_rg, randomizer_config_.max_orbit_rg);
		}
		const double orbit_radius = central_mass * orbit_rg;

		const double max_inc_rad = randomizer_config_.max_inclination_deg * (std::numbers::pi / 180.0);
		const double inclination = ctx.sample_signed_uniform(0.0, max_inc_rad);
		const double theta = std::numbers::pi * 0.5 - inclination;
		const double phi = ctx.sample_uniform(0.0, 2.0 * std::numbers::pi);
		const double sin_theta = std::sin(theta);
		d.position = {orbit_radius * sin_theta * std::cos(phi), orbit_radius * sin_theta * std::sin(phi), orbit_radius * std::cos(theta)};

		const bool is_prograde = (ctx.random_real(0.0, 1.0) < randomizer_config_.prograde_fraction);
		auto velocity = is_prograde ? ctx.compute_circular_orbit_velocity(d.position, central_mass) : ctx.compute_retrograde_circular_orbit_velocity(d.position, central_mass);
		const double ecc = ctx.sample_uniform(randomizer_config_.min_eccentricity, randomizer_config_.max_eccentricity);
		if (ecc > 0.001) {
			velocity = ctx.compute_target_eccentricity_velocity(d.position, velocity, central_mass, ecc);
		}
		d.velocity = velocity;

		const double tilt_rad = ctx.sample_uniform(0.0, randomizer_config_.max_axial_tilt_deg * (std::numbers::pi / 180.0));
		const double spin_h = ctx.sample_uniform(randomizer_config_.min_spin_period_hours, randomizer_config_.max_spin_period_hours);
		const double spin_rate = (2.0 * std::numbers::pi) / (spin_h * 3600.0) * (length_scale_m / ctx.speed_scale());
		d.rotation_speed = spin_rate;
		d.rotation_speed_3d = static_cast<float>(spin_rate);
		d.set_orientation_euler_degrees(0.0, tilt_rad * (180.0 / std::numbers::pi), ctx.random_real(0.0, 360.0));

		if (randomizer_config_.radiative_equilibrium_temperature) {
			const double t_ref = 300.0;
			const double r_ref = 50.0;
			d.temperature = std::clamp(t_ref * std::sqrt(r_ref / std::max(orbit_rg, 1.0)), randomizer_config_.min_temperature_k, randomizer_config_.max_temperature_k);
		} else {
			d.temperature = ctx.sample_uniform(randomizer_config_.min_temperature_k, randomizer_config_.max_temperature_k);
		}

		apply_body_material_preset(d, chosen_preset);

		d.friction_coefficient = ctx.sample_uniform(randomizer_config_.min_friction, randomizer_config_.max_friction);
		d.restitution = ctx.sample_uniform(randomizer_config_.min_restitution, randomizer_config_.max_restitution);
		d.elasticity = ctx.sample_uniform(randomizer_config_.min_elasticity, randomizer_config_.max_elasticity);
		d.integrity = ctx.sample_uniform(randomizer_config_.min_integrity, randomizer_config_.max_integrity);
		d.youngs_modulus_cold = ctx.sample_uniform(randomizer_config_.min_youngs_modulus, randomizer_config_.max_youngs_modulus);
		d.youngs_modulus_hot = d.youngs_modulus_cold;
		d.charge = ctx.sample_uniform(randomizer_config_.min_charge, randomizer_config_.max_charge);
		d.magnetic_moment = ctx.sample_uniform(randomizer_config_.min_magnetic_moment, randomizer_config_.max_magnetic_moment);
		d.quadrupole_moment = ctx.sample_uniform(randomizer_config_.min_quadrupole, randomizer_config_.max_quadrupole);
		d.j2 = ctx.sample_uniform(randomizer_config_.min_j2, randomizer_config_.max_j2);
		d.j3 = ctx.sample_uniform(randomizer_config_.min_j3, randomizer_config_.max_j3);
		d.j4 = ctx.sample_uniform(randomizer_config_.min_j4, randomizer_config_.max_j4);
		const double obl = ctx.sample_uniform(randomizer_config_.min_oblateness, randomizer_config_.max_oblateness);
		if (obl < 0.98) {
			d.geometry_model = Dynamics::Body3DGeometryModel::OblateSpheroid;
		} else if (obl > 1.02) {
			d.geometry_model = Dynamics::Body3DGeometryModel::ProlateSpheroid;
		}

		const float jitter = static_cast<float>(randomizer_config_.appearance_color_jitter);
		if (jitter > 0.0f) {
			const float base_hue = static_cast<float>(ctx.random_real(0.0, 1.0));
			const auto rgb = body_hsv_to_rgb(base_hue, 0.65f, 0.85f);
			d.color[0] = std::clamp(d.color[0] * (1.0f - jitter) + rgb[0] * jitter, 0.0f, 1.0f);
			d.color[1] = std::clamp(d.color[1] * (1.0f - jitter) + rgb[1] * jitter, 0.0f, 1.0f);
			d.color[2] = std::clamp(d.color[2] * (1.0f - jitter) + rgb[2] * jitter, 0.0f, 1.0f);
		}

		d.surface_noise_scale = static_cast<float>(ctx.sample_uniform(randomizer_config_.min_noise_scale, randomizer_config_.max_noise_scale));
		d.surface_roughness = static_cast<float>(ctx.sample_uniform(randomizer_config_.min_roughness, randomizer_config_.max_roughness));
		d.texture_detail_scale = static_cast<float>(ctx.sample_uniform(randomizer_config_.min_texture_detail, randomizer_config_.max_texture_detail));
		d.specular_roughness = static_cast<float>(ctx.sample_uniform(randomizer_config_.min_specular_roughness, randomizer_config_.max_specular_roughness));
		d.emission_intensity = (chosen_preset == Dynamics::Body3DPreset::Star) ? static_cast<float>(ctx.sample_uniform(std::max(1.5, randomizer_config_.min_emission), std::max(3.5, randomizer_config_.max_emission))) : static_cast<float>(ctx.sample_uniform(randomizer_config_.min_emission, randomizer_config_.max_emission));

		const bool is_terrestrial = (chosen_preset == Dynamics::Body3DPreset::TerrestrialPlanet);
		const bool is_giant = (chosen_preset == Dynamics::Body3DPreset::GasGiant || chosen_preset == Dynamics::Body3DPreset::IceGiant);

		if (randomizer_config_.allow_atmospheres && (ctx.random_real(0.0, 1.0) < randomizer_config_.atmosphere_probability)) {
			d.atmosphere_thickness = static_cast<float>(ctx.sample_uniform(randomizer_config_.min_atmosphere_thickness, randomizer_config_.max_atmosphere_thickness));
		} else {
			d.atmosphere_thickness = 0.0f;
			d.atmosphere_mode = Dynamics::Body3DAtmosphereMode::Off;
		}

		d.polar_cap_strength = (is_terrestrial || is_giant) ? static_cast<float>(ctx.sample_uniform(randomizer_config_.min_polar_cap, randomizer_config_.max_polar_cap)) : 0.0f;
		d.night_side_light_intensity = is_terrestrial ? static_cast<float>(ctx.sample_uniform(randomizer_config_.min_night_lights, randomizer_config_.max_night_lights)) : 0.0f;
		d.ring_system_enabled = is_giant && randomizer_config_.allow_ring_systems && (ctx.random_real(0.0, 1.0) < randomizer_config_.ring_system_probability);

		d.set_name(ctx.unique_name(ctx.generate_object_name(chosen_preset)));
		pristine_ = true;
		reference_mass_ = ctx.orchestrator().parameters().mass;
		layers_ = Dynamics::BodySurfaceLayerSet{};
		template_index_ = static_cast<int>(BodyPresetTemplate::Custom);
		view_.mass_log_mode = true;
		view_.radius_log_mode = true;
		view_.reference_radius_log_mode = true;
	}

	void render_pattern_generator(BodyManagerContext& ctx) noexcept {
		const float font = ImGui::GetFontSize();
		auto& orchestrator = ctx.orchestrator();
		const auto& prefs = orchestrator.unit_preferences();
		const double length_scale_m = ctx.length_scale();
		const double mass_scale_kg = ctx.mass_scale();

		split_pane_begin_primary("PatternGeneratorSetup", split_, font * 17.0f, font * 30.0f);

		ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Pattern Geometry Architecture");
		int geom_type_idx = static_cast<int>(pattern_config_.pattern_type);
		if (ImGui::Combo("Pattern Type", &geom_type_idx, kPatternGeometryNames.data(), static_cast<int>(kPatternGeometryNames.size()))) {
			pattern_config_.pattern_type = static_cast<PatternGeometryType>(geom_type_idx);
		}
		render_setting_tooltip("Selects the geometric macro-structure to generate: annular toroidal belts, coplanar orbital debris swarms, or 3D spherical test formations.");

		int preset_idx = static_cast<int>(pattern_config_.preset);
		if (ImGui::Combo("Pattern Presets", &preset_idx, kPatternPresetNames.data(), static_cast<int>(kPatternPresetNames.size()))) {
			pattern_config_.apply_preset(static_cast<PatternPreset>(preset_idx));
		}
		render_setting_tooltip("Applies coherent astrophysical defaults tailored to real astronomical systems.");

		ImGui::Separator();
		render_group_title("Centroid & Attractor Origin", ImVec4(1.0f, 0.75f, 0.35f, 1.0f));

		auto& sys = orchestrator.nbody_system();
		std::vector<int> parent_ids{-2};
		std::vector<std::string> parent_labels{"Central Spacetime Attractor (Origin)"};

		for (const auto& b : sys.bodies()) {
			if (!b.enabled) continue;
			parent_ids.push_back(static_cast<int>(b.id));
			parent_labels.push_back(BodyManagerContext::display_name(b) + " (#" + std::to_string(b.id) + ")");
		}

		int parent_combo_idx = 0;
		for (size_t i = 0; i < parent_ids.size(); ++i) {
			if (parent_ids[i] == pattern_config_.parent_body_id) {
				parent_combo_idx = static_cast<int>(i);
				break;
			}
		}

		std::vector<const char*> parent_ptrs;
		parent_ptrs.reserve(parent_labels.size());
		for (const auto& s : parent_labels) parent_ptrs.push_back(s.c_str());

		if (ImGui::Combo("Parent Attractor", &parent_combo_idx, parent_ptrs.data(), static_cast<int>(parent_ptrs.size()))) {
			pattern_config_.parent_body_id = parent_ids[static_cast<size_t>(parent_combo_idx)];
		}
		render_setting_tooltip("Select which body acts as the gravitational center of the generated formation and defines the orbital reference frame.");

		slider_int_with_input("Object Count", &pattern_config_.object_count, 2, 2500);
		render_setting_tooltip("Total number of celestial bodies to generate and inject into the simulation in a single batch.");

		if (coordinate_aware_input_position("Center Position Offset", pattern_config_.center_position, orchestrator)) {
		}
		render_setting_tooltip("Cartesian offset added to the parent attractor position to shift the formation centroid.");

		render_group_title("Batch Execution & Management", ImVec4(0.35f, 0.95f, 0.45f, 1.0f));

		if (ImGui::Button("Generate and Inject Pattern Bodies", ImVec2(-1.0f, 32.0f))) {
			const auto res = ctx.inject_geometric_pattern(pattern_config_);
			if (res.success) {
				char buf[128];
				std::snprintf(buf, sizeof(buf), "Successfully injected %zu bodies (%zu filtered)", res.generated_count, res.rejected_count);
				last_pattern_notification_ = buf;
				pattern_notification_active_ = true;
			}
		}
		render_setting_tooltip("Executes random distribution sampling and spawns all generated bodies into the post-Newtonian N-body system.");

		if (ctx.has_last_pattern_batch()) {
			if (ImGui::Button("Select Last Injected Batch", ImVec2(-1.0f, 24.0f))) {
				ctx.select_last_pattern_batch();
			}
			render_setting_tooltip("Selects all bodies belonging to the last pattern injection for batch editing in the Object Catalog tab.");

			if (ImGui::Button("Undo / Delete Last Injected Batch", ImVec2(-1.0f, 24.0f))) {
				ctx.undo_last_pattern_injection();
				last_pattern_notification_ = "Last injected batch deleted successfully";
				pattern_notification_active_ = true;
			}
			render_setting_tooltip("Permanently removes every body spawned during the last pattern injection.");
		}

		if (pattern_notification_active_) {
			ImGui::Spacing();
			ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.2f, 0.85f, 0.45f, 0.8f));
			ImGui::BeginChild("##PatternNotice", ImVec2(0.0f, 48.0f), true);
			ImGui::TextColored(ImVec4(0.25f, 0.95f, 0.5f, 1.0f), "%s", last_pattern_notification_.c_str());
			ImGui::EndChild();
			ImGui::PopStyleColor();
		}

		if (ctx.has_last_pattern_batch()) {
			const auto& r = ctx.last_generation_result();
			ImGui::Spacing();
			render_group_title("Last Generation Summary", ImVec4(0.5f, 0.85f, 1.0f, 1.0f));
			ImGui::TextDisabled("Bodies Spawned: %zu / %zu", r.generated_count, r.requested_count);
			ImGui::TextDisabled("Total Mass: %s", Units::format_mass(r.total_mass * mass_scale_kg, prefs.mass, 3).c_str());
			ImGui::TextDisabled("Radial Span: %s .. %s", Units::format_distance(r.min_distance * length_scale_m, prefs.distance, 2).c_str(), Units::format_distance(r.max_distance * length_scale_m, prefs.distance, 2).c_str());
			ImGui::TextDisabled("Velocity Span: %s .. %s", Units::format_velocity(r.min_speed * ctx.speed_scale(), prefs.velocity).c_str(), Units::format_velocity(r.max_speed * ctx.speed_scale(), prefs.velocity).c_str());
		}

		split_pane_end_primary(split_);

		split_pane_begin_secondary("PatternGeneratorDetails", split_);
		ImGui::PushID("PatternDetailsTab");

		if (ImGui::BeginTabBar("PatternConfigTabs")) {
			if (ImGui::BeginTabItem("Spatial Geometry")) {
				render_group_title("Radial Geometry & Dispersion", ImVec4(0.4f, 0.85f, 1.0f, 1.0f));

				if (pattern_config_.pattern_type != PatternGeometryType::SphericalFormation) {
					double semi_a_m = pattern_config_.mean_semi_major_axis * length_scale_m;
					if (unit_aware_slider_double("Mean Semi-Major Axis", &semi_a_m, 1.0 * length_scale_m, 500.0 * length_scale_m, UnitCategory::Distance, prefs, "%.2f")) {
						pattern_config_.mean_semi_major_axis = std::max(semi_a_m / length_scale_m, 1.0);
					}
					render_setting_tooltip("Mean orbital radius or semi-major axis of the annular ring / debris swarm.");

					double rad_thick_m = pattern_config_.radial_thickness * length_scale_m;
					if (unit_aware_slider_double("Radial Thickness", &rad_thick_m, 0.1 * length_scale_m, 200.0 * length_scale_m, UnitCategory::Distance, prefs, "%.2f", &pattern_radial_thickness_log_mode_)) {
						pattern_config_.radial_thickness = std::clamp(rad_thick_m / length_scale_m, 0.01, pattern_config_.mean_semi_major_axis * 1.95);
					}
					render_setting_tooltip("Radial belt width / swarm radial thickness.");

					int rad_dist_idx = static_cast<int>(pattern_config_.radial_distribution);
					if (ImGui::Combo("Radial Distribution Law", &rad_dist_idx, kRadialDistributionNames.data(), static_cast<int>(kRadialDistributionNames.size()))) {
						pattern_config_.radial_distribution = static_cast<RadialDistributionLaw>(rad_dist_idx);
					}
					render_setting_tooltip("Spatial probability distribution governing radial placement of bodies.");

					if (pattern_config_.radial_distribution == RadialDistributionLaw::Gaussian) {
						slider_double_with_input("Radial Gaussian Sigma", &pattern_config_.radial_gaussian_sigma, 0.01, pattern_config_.radial_thickness);
					} else if (pattern_config_.radial_distribution == RadialDistributionLaw::PowerLaw) {
						slider_double_with_input("Power-Law Exponent (alpha)", &pattern_config_.radial_power_law_index, -4.0, 4.0, "%.2f");
					} else if (pattern_config_.radial_distribution == RadialDistributionLaw::KirkwoodResonant) {
						ImGui::Checkbox("Simulate Kirkwood Resonant Gaps", &pattern_config_.kirkwood_gaps_enabled);
						if (pattern_config_.kirkwood_gaps_enabled) {
							slider_int_with_input("Gap Count", &pattern_config_.kirkwood_gap_count, 1, 8);
							slider_double_with_input("Resonance Depletion Depth", &pattern_config_.kirkwood_resonance_depth, 0.1, 1.0, "%.2f");
						}
					}

					render_group_title("Vertical Scale Height", ImVec4(0.5f, 0.9f, 0.85f, 1.0f));
					double v_scale_m = pattern_config_.vertical_scale_height * length_scale_m;
					if (unit_aware_slider_double("Vertical Thickness / Scale Height", &v_scale_m, 0.01 * length_scale_m, 50.0 * length_scale_m, UnitCategory::Distance, prefs, "%.2f", &pattern_scale_height_log_mode_)) {
						pattern_config_.vertical_scale_height = std::max(v_scale_m / length_scale_m, 0.0);
					}

					render_group_title("Common Plane Orientation", ImVec4(0.8f, 0.6f, 1.0f, 1.0f));
					slider_double_with_input("Plane Inclination (i)", &pattern_config_.plane_inclination_deg, -90.0, 90.0, "%.1f deg");
					slider_double_with_input("Longitude of Ascending Node (Omega)", &pattern_config_.plane_longitude_node_deg, 0.0, 360.0, "%.1f deg");
					slider_double_with_input("Argument of Periapsis (omega)", &pattern_config_.plane_argument_periapsis_deg, 0.0, 360.0, "%.1f deg");

					if (ImGui::SmallButton("Equatorial (0 deg)")) {
						pattern_config_.plane_inclination_deg = 0.0;
						pattern_config_.plane_longitude_node_deg = 0.0;
					}
					ImGui::SameLine();
					if (ImGui::SmallButton("Polar (90 deg)")) {
						pattern_config_.plane_inclination_deg = 90.0;
					}
					ImGui::SameLine();
					if (ImGui::SmallButton("Retrograde (180 deg)")) {
						pattern_config_.plane_inclination_deg = 180.0;
					}
				} else {
					int sph_type_idx = static_cast<int>(pattern_config_.spherical_type);
					if (ImGui::Combo("Spherical Geometry Model", &sph_type_idx, kSphericalFormationNames.data(), static_cast<int>(kSphericalFormationNames.size()))) {
						pattern_config_.spherical_type = static_cast<SphericalFormationType>(sph_type_idx);
					}
					render_setting_tooltip("Selects the 3D spherical configuration: Plummer star cluster, uniform solid sphere, Dyson shell, or expanding blast.");

					if (pattern_config_.spherical_type == SphericalFormationType::PlummerSphere) {
						slider_double_with_input("Plummer Scale Radius", &pattern_config_.plummer_scale_radius, 0.5, 200.0, "%.2f");
						render_setting_tooltip("Scale parameter 'b' of the Plummer sphere density profile rho(r) = (3M / 4pi b^3) * (1 + r^2/b^2)^(-5/2).");
					} else if (pattern_config_.spherical_type == SphericalFormationType::UniformSphere) {
						slider_double_with_input("Uniform Sphere Radius", &pattern_config_.uniform_sphere_radius, 0.5, 200.0, "%.2f");
					} else if (pattern_config_.spherical_type == SphericalFormationType::ThinShell || pattern_config_.spherical_type == SphericalFormationType::FibonacciShell) {
						slider_double_with_input("Shell Radius", &pattern_config_.thin_shell_radius, 0.5, 200.0, "%.2f");
					} else {
						slider_double_with_input("Inner Shell Radius", &pattern_config_.shell_inner_radius, 0.5, 200.0, "%.2f");
						slider_double_with_input("Outer Shell Radius", &pattern_config_.shell_outer_radius, pattern_config_.shell_inner_radius * 1.05, 500.0, "%.2f");
						slider_double_with_input("Radial Density Falloff (gamma)", &pattern_config_.spherical_power_law_gamma, -4.0, 4.0, "%.2f");
					}

					render_group_title("Spherical Formation Rotation", ImVec4(0.8f, 0.6f, 1.0f, 1.0f));
					slider_double_with_input("Solid-Body Angular Speed", &pattern_config_.solid_body_rotation_speed, -1.0, 1.0, "%.3f rad/s");
					slider_double_with_input("Rotation Axis Tilt", &pattern_config_.solid_body_axis_tilt_deg, 0.0, 180.0, "%.1f deg");
					slider_double_with_input("Rotation Axis Azimuth", &pattern_config_.solid_body_axis_azimuth_deg, 0.0, 360.0, "%.1f deg");
				}

				if (pattern_config_.pattern_type == PatternGeometryType::DebrisSwarm) {
					render_group_title("Debris Sector & Anomaly Spread", ImVec4(1.0f, 0.75f, 0.35f, 1.0f));
					slider_double_with_input("Angular Opening (Spread)", &pattern_config_.angular_opening_deg, 2.0, 360.0, "%.1f deg");
					slider_double_with_input("Angular Center Position", &pattern_config_.angular_center_deg, 0.0, 360.0, "%.1f deg");
				}

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Kinematics & Orbits")) {
				render_group_title("Orbital Velocity Injection", ImVec4(0.4f, 0.9f, 0.6f, 1.0f));
				int vel_mode_idx = static_cast<int>(pattern_config_.velocity_mode);
				if (ImGui::Combo("Velocity Mode", &vel_mode_idx, kVelocityInjectionModeNames.data(), static_cast<int>(kVelocityInjectionModeNames.size()))) {
					pattern_config_.velocity_mode = static_cast<VelocityInjectionMode>(vel_mode_idx);
				}
				render_setting_tooltip("Specifies how initial velocity vectors are computed for each member of the formation.");

				slider_double_with_input("Mean Velocity Multiplier", &pattern_config_.mean_velocity_multiplier, 0.0, 3.0, "%.3fx");
				render_setting_tooltip("Multiplier applied to the ideal orbital velocity (1.0 = exact equilibrium circular orbit).");

				slider_double_with_input("Velocity Dispersion Fraction", &pattern_config_.velocity_dispersion_fraction, 0.0, 0.5, "%.3f");
				render_setting_tooltip("Fractional spread delta_v / v_mean representing random kinematic velocity dispersion.");

				int vel_dist_idx = static_cast<int>(pattern_config_.velocity_distribution);
				if (ImGui::Combo("Velocity Dispersion Law", &vel_dist_idx, kVelocityDistributionNames.data(), static_cast<int>(kVelocityDistributionNames.size()))) {
					pattern_config_.velocity_distribution = static_cast<VelocityDistributionLaw>(vel_dist_idx);
				}

				slider_double_with_input("Prograde Velocity Fraction", &pattern_config_.prograde_fraction, 0.0, 1.0, "%.2f");
				render_setting_tooltip("Fraction of bodies orbiting prograde vs retrograde (1.0 = 100% prograde, 0.0 = 100% retrograde, 0.5 = counter-rotating).");

				if (pattern_config_.velocity_mode == VelocityInjectionMode::RadialExpansion) {
					slider_double_with_input("Radial Blast Speed", &pattern_config_.expansion_speed, 0.001, 2.0, "%.4f c");
				}

				render_group_title("Eccentricity & Inclination Spreads", ImVec4(1.0f, 0.7f, 0.35f, 1.0f));
				slider_double_with_input("Mean Orbital Eccentricity", &pattern_config_.mean_eccentricity, 0.0, 0.98, "%.3f");
				slider_double_with_input("Eccentricity Dispersion", &pattern_config_.eccentricity_dispersion, 0.0, 0.4, "%.3f");

				int ecc_dist_idx = static_cast<int>(pattern_config_.eccentricity_distribution);
				if (ImGui::Combo("Eccentricity Distribution Law", &ecc_dist_idx, kDispersionDistributionNames.data(), static_cast<int>(kDispersionDistributionNames.size()))) {
					pattern_config_.eccentricity_distribution = static_cast<DispersionDistributionLaw>(ecc_dist_idx);
				}

				slider_double_with_input("Inclination Dispersion", &pattern_config_.mean_inclination_dispersion_deg, 0.0, 45.0, "%.2f deg");
				int inc_dist_idx = static_cast<int>(pattern_config_.inclination_distribution);
				if (ImGui::Combo("Inclination Distribution Law", &inc_dist_idx, kDispersionDistributionNames.data(), static_cast<int>(kDispersionDistributionNames.size()))) {
					pattern_config_.inclination_distribution = static_cast<DispersionDistributionLaw>(inc_dist_idx);
				}

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Mass & Material")) {
				render_group_title("Population Archetype & Appearance", ImVec4(0.6f, 0.85f, 1.0f, 1.0f));

				int arch_idx = static_cast<int>(pattern_config_.archetype);
				if (ImGui::Combo("Base Population Archetype", &arch_idx, kBodySurfacePresetNames.data(), static_cast<int>(kBodySurfacePresetNames.size()))) {
					pattern_config_.archetype = static_cast<Dynamics::Body3DPreset>(arch_idx);
				}
				render_setting_tooltip("Physical material and surface shading archetype assigned across the generated batch.");

				char name_prefix_buf[32];
				std::strncpy(name_prefix_buf, pattern_config_.name_prefix.c_str(), sizeof(name_prefix_buf) - 1);
				name_prefix_buf[sizeof(name_prefix_buf) - 1] = '\0';
				if (ImGui::InputText("Name Prefix", name_prefix_buf, sizeof(name_prefix_buf))) {
					pattern_config_.name_prefix = name_prefix_buf;
				}

				slider_double_with_input("Color Jitter (Hue Variance)", &pattern_config_.color_jitter, 0.0, 1.0, "%.2f");
				render_setting_tooltip("Subtle chromatic variation applied across generated body surface palettes.");

				render_group_title("Mass Distribution", ImVec4(1.0f, 0.75f, 0.35f, 1.0f));
				int mass_dist_idx = static_cast<int>(pattern_config_.mass_distribution);
				if (ImGui::Combo("Mass Distribution Law", &mass_dist_idx, kMassDistributionNames.data(), static_cast<int>(kMassDistributionNames.size()))) {
					pattern_config_.mass_distribution = static_cast<MassDistributionLaw>(mass_dist_idx);
				}

				if (pattern_config_.mass_distribution == MassDistributionLaw::Fixed) {
					double m_fix_kg = pattern_config_.fixed_mass * mass_scale_kg;
					if (unit_aware_slider_double("Fixed Body Mass", &m_fix_kg, 1e-12 * mass_scale_kg, 1e6 * mass_scale_kg, UnitCategory::Mass, prefs, "%.4e", &pattern_fixed_mass_log_mode_)) {
						pattern_config_.fixed_mass = std::max(m_fix_kg / mass_scale_kg, 0.0);
					}
				} else {
					double m_min_kg = pattern_config_.min_mass * mass_scale_kg;
					if (unit_aware_slider_double("Minimum Mass", &m_min_kg, 1e-14 * mass_scale_kg, 1e4 * mass_scale_kg, UnitCategory::Mass, prefs, "%.4e", &pattern_min_mass_log_mode_)) {
						pattern_config_.min_mass = std::max(m_min_kg / mass_scale_kg, 0.0);
					}
					double m_max_kg = pattern_config_.max_mass * mass_scale_kg;
					if (unit_aware_slider_double("Maximum Mass", &m_max_kg, pattern_config_.min_mass * mass_scale_kg, 1e6 * mass_scale_kg, UnitCategory::Mass, prefs, "%.4e", &pattern_max_mass_log_mode_)) {
						pattern_config_.max_mass = std::max(m_max_kg / mass_scale_kg, pattern_config_.min_mass);
					}
					if (pattern_config_.mass_distribution == MassDistributionLaw::PowerLaw) {
						slider_double_with_input("Mass Power-Law Exponent (alpha)", &pattern_config_.mass_power_law_alpha, 0.5, 4.0, "%.3f");
						render_setting_tooltip("Differential mass function dN/dm ~ m^-alpha (Dohnanyi collisional cascade = 1.833, Salpeter IMF = 2.35).");
					}
				}

				render_group_title("Physical Radius & Density Coupling", ImVec4(0.4f, 0.9f, 0.6f, 1.0f));
				ImGui::Checkbox("Derive Physical Radius from Material Density", &pattern_config_.radius_from_density);
				render_setting_tooltip("Computes physical body radius dynamically from mass and volumetric mass density via V = 4/3 pi r^3.");

				if (pattern_config_.radius_from_density) {
					slider_double_with_input("Mean Density (kg/m^3)", &pattern_config_.mean_density, 50.0, 20000.0, "%.0f", &pattern_density_log_mode_);
					slider_double_with_input("Density Spread (+/- %)", &pattern_config_.density_dispersion_pct, 0.0, 50.0, "%.1f%%");
				} else {
					slider_double_with_input("Minimum Radius", &pattern_config_.min_radius, 1e-4, 10.0, "%.4f");
					slider_double_with_input("Maximum Radius", &pattern_config_.max_radius, pattern_config_.min_radius, 50.0, "%.4f");
				}

				render_group_title("Thermodynamics", ImVec4(1.0f, 0.5f, 0.4f, 1.0f));
				ImGui::Checkbox("Equilibrium Temperature from Central Radiation", &pattern_config_.temperature_equilibrium);
				if (pattern_config_.temperature_equilibrium) {
					slider_double_with_input("Reference Temperature at Mean Radius (K)", &pattern_config_.reference_temperature_k, 5.0, 20000.0, "%.1f K");
					slider_double_with_input("Radial Falloff Exponent (beta)", &pattern_config_.temperature_falloff_exponent, 0.1, 1.0, "%.2f");
				}

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Hazards & Seeds")) {
				render_group_title("Anti-Collision & Horizon Safeguards", ImVec4(1.0f, 0.45f, 0.45f, 1.0f));
				ImGui::Checkbox("Avoid Central Event Horizon Overlap (r > 1.35 r_h)", &pattern_config_.avoid_horizon_overlap);
				render_setting_tooltip("Rejects generated positions that fall within 1.35 horizon radii of the central black hole.");

				ImGui::Checkbox("Avoid Mutual Batch Collisions", &pattern_config_.avoid_mutual_collisions);
				render_setting_tooltip("Ensures generated members do not spawn physically overlapping one another.");

				render_group_title("Deterministic PRNG Seed", ImVec4(0.7f, 0.7f, 1.0f, 1.0f));
				int seed_i = static_cast<int>(pattern_config_.seed);
				if (ImGui::InputInt("Pattern Generator Seed", &seed_i)) {
					pattern_config_.seed = static_cast<uint64_t>(std::max(seed_i, 1));
				}
				if (ImGui::Button("Randomize Seed", ImVec2(160.0f, 24.0f))) {
					pattern_config_.seed = static_cast<uint64_t>(ctx.random_int(1, 1000000));
				}

				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}

		ImGui::PopID();
		split_pane_end_secondary();
	}

	void render(BodyManagerContext& ctx) noexcept {
		if (ctx.consume_pattern_generator_request()) {
			creation_sub_mode_ = CreationSubMode::GeometricPatternGenerator;
		}

		ImGui::PushID("CreationSubModeBar");
		const char* sub_mode_names[] = {"Single Body Draft", "Multi-Body Geometric Pattern Generator"};
		int mode_idx = static_cast<int>(creation_sub_mode_);
		if (ImGui::Combo("Creation Workflow", &mode_idx, sub_mode_names, IM_ARRAYSIZE(sub_mode_names))) {
			creation_sub_mode_ = static_cast<CreationSubMode>(mode_idx);
		}
		render_setting_tooltip("Switch between drafting an individual celestial body or mass-generating structured collections of bodies (annular asteroid belts, coplanar debris swarms, spherical test formations).");
		ImGui::PopID();
		ImGui::Separator();

		if (creation_sub_mode_ == CreationSubMode::GeometricPatternGenerator) {
			render_pattern_generator(ctx);
			return;
		}

		if (pristine_ && std::abs(ctx.orchestrator().parameters().mass - reference_mass_) > 1e-9 * std::max(std::abs(reference_mass_), 1.0)) {
			reset_draft(ctx);
		}
		const float font = ImGui::GetFontSize();

		split_pane_begin_primary("ObjectCreationSetup", split_, font * 16.0f, font * 28.0f);
		render_setup_pane(ctx);
		split_pane_end_primary(split_);

		split_pane_begin_secondary("ObjectCreationEditor", split_);
		ImGui::PushID("BodyCreationEditor");
		const BodyEditResult edit = render_body_editor(ctx, draft_, &layers_, view_, BodyEditorSection::ObjectCreation);
		if (edit.body_changed || edit.layers_changed) {
			pristine_ = false;
		}
		ImGui::PopID();
		split_pane_end_secondary();
	}
};

}

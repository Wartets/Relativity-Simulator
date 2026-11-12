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

class BodyCreationTab {
private:
	Dynamics::PostNewtonianBody draft_{};
	Dynamics::BodySurfaceLayerSet layers_{};
	BodyEditorViewState view_{};
	SplitPaneState split_{320.0f, 320.0f, 0.0f};
	int template_index_{0};
	bool pristine_{true};
	double reference_mass_{-1.0};
	bool randomize_after_spawn_{true};
	std::string last_spawned_name_{};

	[[nodiscard]] std::string synthesize_name(BodyManagerContext& ctx, std::string_view label, double mass, double radius, double orbit_radius) {
		static constexpr std::array<std::string_view, 28> prefixes{
			"Astra", "Boreal", "Cinder", "Drift", "Echo", "Eon", "Flux", "Halo",
			"Ion", "Kestrel", "Lumen", "Nova", "Nyx", "Orbit", "Quasar", "Rift",
			"Solace", "Spectrum", "Titan", "Umbra", "Vanta", "Velvet", "Vesper",
			"Warden", "Zenith", "Auric", "Kepler", "Morrow"
		};
		static constexpr std::array<std::string_view, 8> accents{
			"Prime", "I", "II", "III", "Arc", "Node", "Field", "Halo"
		};

		const std::string_view prefix = prefixes[static_cast<size_t>(ctx.random_int(0, static_cast<int>(prefixes.size() - 1)))];
		const std::string_view accent = accents[static_cast<size_t>(ctx.random_int(0, static_cast<int>(accents.size() - 1)))];
		const uint32_t suffix = static_cast<uint32_t>(ctx.random_int(10, 999));

		std::string name = std::string(prefix) + " " + std::string(label);
		if (mass > 0.0 && radius > 0.0) {
			if (orbit_radius > radius * 100.0) {
				name += " Deep";
			} else if (orbit_radius < radius * 20.0) {
				name += " Inner";
			}
		}
		name += " ";
		name += std::string(accent);
		name += " ";
		name += std::to_string(suffix);
		return name;
	}

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
		ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.6f, 1.0f), "Object Draft");
		render_hint_text("Configure every property of a new orbiting body in the editor on the right, then spawn it into the running system. Independent black holes are created in the Create Source tab.");

		render_group_title("Starting Point", ImVec4(0.5f, 0.85f, 1.0f, 1.0f));
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

		render_group_title("Draft Summary", ImVec4(1.0f, 0.75f, 0.35f, 1.0f));
		const auto& prefs = ctx.orchestrator().unit_preferences();
		ImGui::TextWrapped("Name: %s", BodyManagerContext::display_name(draft_).c_str());
		ImGui::TextWrapped("Mass: %s", Units::format_mass(draft_.mass * ctx.mass_scale(), prefs.mass, 3).c_str());
		ImGui::TextWrapped("Radius: %s", Units::format_distance(draft_.radius * ctx.length_scale(), prefs.distance, 3).c_str());
		ImGui::TextWrapped("Distance From Center: %s", Units::format_distance(BodyManagerContext::distance_from_center(draft_) * ctx.length_scale(), prefs.distance, 3).c_str());
		ImGui::TextWrapped("Speed: %s", Units::format_velocity(draft_.speed() * ctx.speed_scale(), prefs.velocity).c_str());
		ImGui::TextWrapped("Texture Layers: %u", layers_.count);

		render_group_title("Spawn", ImVec4(0.4f, 0.9f, 0.6f, 1.0f));
		ImGui::Checkbox("Randomize Draft After Spawn", &randomize_after_spawn_);
		render_setting_tooltip("When enabled, a new random draft is generated after each spawn. When disabled, the draft is kept so several similar bodies can be spawned in a row.");
		if (ImGui::Button("Spawn and Inject into System", ImVec2(-1.0f, 32.0f))) {
			last_spawned_name_ = BodyManagerContext::display_name(draft_);
			const uint32_t new_id = ctx.spawn_body(draft_, layers_);
			ctx.select_object(static_cast<int>(new_id));
			if (randomize_after_spawn_) {
				reset_draft(ctx);
			} else {
				draft_.set_name(ctx.unique_name(BodyManagerContext::display_name(draft_)));
			}
		}
		render_setting_tooltip("Adds the configured body, with its full appearance and texture layers, to the running N-body system and selects it in the Object Catalog.");
		if (!last_spawned_name_.empty()) {
			ImGui::TextWrapped("Last spawned: %s", last_spawned_name_.c_str());
			if (ImGui::SmallButton("Show In Object Catalog")) {
				ctx.request_tab(BodyManagerTab::ObjectCatalog);
			}
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
		const auto& profile = kCreationArchetypes[static_cast<size_t>(ctx.random_int(0, static_cast<int>(kCreationArchetypes.size()) - 1))];
		const double central_mass = std::max(ctx.orchestrator().parameters().mass, 1e-12);
		auto& d = draft_;
		d = Dynamics::PostNewtonianBody{};
		apply_body_appearance_preset(d, profile.preset);

		const double reference_mass = std::max(central_mass, 1.0);
		const double orbit_radius = reference_mass * ctx.sample_log_uniform(profile.orbit_gravitational_radii_min, profile.orbit_gravitational_radii_max);
		const double mass_ratio = ctx.sample_log_uniform(profile.mass_ratio_min, profile.mass_ratio_max);
		const double mass_percentile = std::clamp(std::log(mass_ratio / profile.mass_ratio_min) / std::log(profile.mass_ratio_max / profile.mass_ratio_min), 0.0, 1.0);
		d.mass = reference_mass * mass_ratio;
		if (profile.preset == Dynamics::Body3DPreset::NeutronStar) {
			d.radius = d.mass * ctx.random_real(profile.size_factor_min, profile.size_factor_max);
		} else {
			const double size_fraction = profile.size_factor_min * std::pow(profile.size_factor_max / profile.size_factor_min, mass_percentile) * ctx.random_real(0.85, 1.15);
			d.radius = std::max(orbit_radius * size_fraction, 4.0 * d.mass);
		}
		d.radius = std::max(d.radius, 1e-6);
		d.reference_radius = d.radius;
		d.quadrupole_moment = ctx.sample_signed_uniform(profile.quadrupole_min, profile.quadrupole_max);
		d.j2 = ctx.sample_log_uniform(profile.j2_min, profile.j2_max);
		d.j3 = ctx.sample_signed_uniform(profile.j3_min, profile.j3_max);
		d.j4 = ctx.sample_signed_uniform(profile.j4_min, profile.j4_max);

		const double inclination = ctx.random_real(-0.35, 0.35);
		const double theta = std::numbers::pi * 0.5 - inclination;
		const double phi = ctx.random_real(0.0, 2.0 * std::numbers::pi);
		const double sin_theta = std::sin(theta);
		d.position = {orbit_radius * sin_theta * std::cos(phi), orbit_radius * sin_theta * std::sin(phi), orbit_radius * std::cos(theta)};

		auto velocity = ctx.compute_circular_orbit_velocity(d.position, central_mass);
		if (std::abs(velocity[0]) < 1e-12 && std::abs(velocity[1]) < 1e-12 && std::abs(velocity[2]) < 1e-12) {
			velocity = {0.0, std::sqrt(central_mass / std::max(orbit_radius, 1e-9)), 0.0};
		}
		const double speed_scale = ctx.random_real(0.82, 1.18);
		d.velocity = {velocity[0] * speed_scale, velocity[1] * speed_scale, velocity[2] * speed_scale};
		const double breakup_rate = std::sqrt(std::max(ctx.orchestrator().physical_gravitational_constant() * d.mass / (d.radius * d.radius * d.radius), 0.0));
		const double spin_rate = breakup_rate * profile.breakup_spin_fraction * ctx.random_real(0.1, 1.0);
		const double axis_tilt = ctx.random_real(0.0, 0.5);
		const double axis_azimuth = ctx.random_real(0.0, 2.0 * std::numbers::pi);
		const double angular_momentum = 0.4 * d.mass * d.radius * d.radius * spin_rate;
		d.rotation_speed = spin_rate;
		d.spin = {
			angular_momentum * std::sin(axis_tilt) * std::cos(axis_azimuth),
			angular_momentum * std::sin(axis_tilt) * std::sin(axis_azimuth),
			angular_momentum * std::cos(axis_tilt)
		};
		apply_body_material_preset(d, profile.preset);

		const float primary_hue = static_cast<float>(std::fmod(profile.hue_center + ctx.random_real(-profile.hue_spread, profile.hue_spread) + 1.0, 1.0));
		const auto primary_rgb = body_hsv_to_rgb(primary_hue, static_cast<float>(profile.saturation), static_cast<float>(profile.value));
		d.color = {primary_rgb[0], primary_rgb[1], primary_rgb[2], 1.0f};
		const float secondary_hue = std::fmod(primary_hue + static_cast<float>(ctx.random_real(0.03, 0.12)) + 1.0f, 1.0f);
		const auto secondary_rgb = body_hsv_to_rgb(secondary_hue, static_cast<float>(std::clamp(profile.saturation * 1.2, 0.0, 1.0)), static_cast<float>(std::clamp(profile.value * 0.7, 0.0, 1.0)));
		d.color_secondary = {secondary_rgb[0], secondary_rgb[1], secondary_rgb[2], 1.0f};
		const float tertiary_hue = std::fmod(primary_hue + static_cast<float>(ctx.random_real(0.25, 0.55)) + 1.0f, 1.0f);
		const auto tertiary_rgb = body_hsv_to_rgb(tertiary_hue, static_cast<float>(std::clamp(profile.saturation * 0.6, 0.0, 1.0)), static_cast<float>(std::clamp(profile.value * 1.1, 0.0, 1.0)));
		d.color_tertiary = {tertiary_rgb[0], tertiary_rgb[1], tertiary_rgb[2], 1.0f};

		d.surface_noise_scale = static_cast<float>(ctx.random_real(profile.noise_min, profile.noise_max));
		d.surface_roughness = static_cast<float>(ctx.random_real(profile.roughness_min, profile.roughness_max));
		d.atmosphere_thickness = static_cast<float>(ctx.random_real(profile.atmosphere_min, profile.atmosphere_max));
		d.emission_intensity = static_cast<float>(ctx.random_real(profile.emission_min, profile.emission_max));
		d.rotation_speed_3d = static_cast<float>(ctx.random_real(profile.rotation_min, profile.rotation_max));
		d.texture_detail_scale = static_cast<float>(ctx.random_real(0.6, 2.2));
		const bool is_world = profile.preset == Dynamics::Body3DPreset::TerrestrialPlanet;
		const bool is_giant = profile.preset == Dynamics::Body3DPreset::GasGiant;
		d.polar_cap_strength = (is_world || is_giant) ? static_cast<float>(ctx.random_real(0.1, 0.5)) : 0.0f;
		d.night_side_light_intensity = is_world ? static_cast<float>(ctx.random_real(0.0, 0.6)) : 0.0f;
		d.ring_system_enabled = is_giant && (ctx.random_int(0, 1) == 0);
		d.set_orientation_euler_degrees(ctx.random_real(-8.0, 8.0), ctx.random_real(-30.0, 30.0), ctx.random_real(-180.0, 180.0));

		d.set_name(ctx.unique_name(synthesize_name(ctx, profile.label, d.mass, d.radius, orbit_radius)));
		pristine_ = true;
		reference_mass_ = ctx.orchestrator().parameters().mass;
		layers_ = Dynamics::BodySurfaceLayerSet{};
		template_index_ = static_cast<int>(BodyPresetTemplate::Custom);
		view_.mass_log_mode = true;
		view_.radius_log_mode = true;
		view_.reference_radius_log_mode = true;
	}

	void render(BodyManagerContext& ctx) noexcept {
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

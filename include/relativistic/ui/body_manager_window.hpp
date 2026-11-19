#pragma once

#include <imgui.h>
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/io/user_settings.hpp"
#include "relativistic/ui/body_manager/body_manager_types.hpp"
#include "relativistic/ui/body_manager/body_manager_context.hpp"
#include "relativistic/ui/body_manager/body_randomizer_config.hpp"
#include "relativistic/ui/body_manager/system_presets.hpp"
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
		object_catalog_tab_.view().reference_frame_body_id = settings.body_manager_reference_body_id;
		object_catalog_tab_.view().show_relative_coordinates = settings.body_manager_relative_coords_mode;
		source_catalog_tab_.view().reference_frame_body_id = settings.body_manager_reference_body_id;
		source_catalog_tab_.view().show_relative_coordinates = settings.body_manager_relative_coords_mode;
		object_catalog_tab_.view().sec_identity_open = settings.body_sec_identity_open;
		object_catalog_tab_.view().sec_motion_open = settings.body_sec_motion_open;
		object_catalog_tab_.view().sec_orbital_plane_open = settings.body_sec_orbital_plane_open;
		object_catalog_tab_.view().sec_frenet_open = settings.body_sec_frenet_open;
		object_catalog_tab_.view().sec_orientation_open = settings.body_sec_orientation_open;
		object_catalog_tab_.view().sec_multipoles_open = settings.body_sec_multipoles_open;
		object_catalog_tab_.view().sec_material_open = settings.body_sec_material_open;
		object_catalog_tab_.view().sec_surface_open = settings.body_sec_surface_open;
		object_catalog_tab_.view().sec_accretion_disk_open = settings.body_sec_accretion_disk_open;
		source_catalog_tab_.view().sec_identity_open = settings.body_sec_identity_open;
		source_catalog_tab_.view().sec_motion_open = settings.body_sec_motion_open;
		source_catalog_tab_.view().sec_orbital_plane_open = settings.body_sec_orbital_plane_open;
		source_catalog_tab_.view().sec_frenet_open = settings.body_sec_frenet_open;
		source_catalog_tab_.view().sec_accretion_disk_open = settings.body_sec_accretion_disk_open;
		object_creation_tab_.sec_template_open() = settings.body_creation_sec_template_open;
		object_creation_tab_.sec_comparator_open() = settings.body_creation_sec_comparator_open;
		object_creation_tab_.sec_plane_open() = settings.body_creation_sec_plane_open;
		object_creation_tab_.sec_hazard_open() = settings.body_creation_sec_hazard_open;
		source_creation_tab_.sec_template_open() = settings.source_creation_sec_template_open;
		source_creation_tab_.sec_comparator_open() = settings.source_creation_sec_comparator_open;
		source_creation_tab_.sec_plane_open() = settings.source_creation_sec_plane_open;
		source_creation_tab_.sec_hazard_open() = settings.source_creation_sec_hazard_open;
		system_dynamics_tab_.sec_bulk_actions_open() = settings.sys_dyn_sec_bulk_actions_open;
		system_dynamics_tab_.sec_plane_align_open() = settings.sys_dyn_sec_plane_align_open;
		system_dynamics_tab_.sec_generalized_open() = settings.sys_dyn_sec_generalized_open;
		system_dynamics_tab_.sec_sources_open() = settings.sys_dyn_sec_sources_open;
		interactions_tab_.sec_em_open() = settings.interactions_sec_em_open;
		interactions_tab_.sec_collisions_open() = settings.interactions_sec_collisions_open;
		interactions_tab_.sec_thermo_open() = settings.interactions_sec_thermo_open;
		interactions_tab_.sec_frag_open() = settings.interactions_sec_frag_open;
		interactions_tab_.sec_annihilation_open() = settings.interactions_sec_annihilation_open;
		object_catalog_tab_.filter().physical_ranges_open = settings.catalog_filter_ranges_open;
		object_creation_tab_.randomizer_config() = settings.body_randomizer;
		context_.randomizer_config() = settings.body_randomizer;
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
		persisted_settings_->body_manager_reference_body_id = object_catalog_tab_.view().reference_frame_body_id;
		persisted_settings_->body_manager_relative_coords_mode = object_catalog_tab_.view().show_relative_coordinates;
		persisted_settings_->body_sec_identity_open = object_catalog_tab_.view().sec_identity_open;
		persisted_settings_->body_sec_motion_open = object_catalog_tab_.view().sec_motion_open;
		persisted_settings_->body_sec_orbital_plane_open = object_catalog_tab_.view().sec_orbital_plane_open;
		persisted_settings_->body_sec_frenet_open = object_catalog_tab_.view().sec_frenet_open;
		persisted_settings_->body_sec_orientation_open = object_catalog_tab_.view().sec_orientation_open;
		persisted_settings_->body_sec_multipoles_open = object_catalog_tab_.view().sec_multipoles_open;
		persisted_settings_->body_sec_material_open = object_catalog_tab_.view().sec_material_open;
		persisted_settings_->body_sec_surface_open = object_catalog_tab_.view().sec_surface_open;
		persisted_settings_->body_sec_accretion_disk_open = object_catalog_tab_.view().sec_accretion_disk_open;
		persisted_settings_->body_creation_sec_template_open = object_creation_tab_.sec_template_open();
		persisted_settings_->body_creation_sec_comparator_open = object_creation_tab_.sec_comparator_open();
		persisted_settings_->body_creation_sec_plane_open = object_creation_tab_.sec_plane_open();
		persisted_settings_->body_creation_sec_hazard_open = object_creation_tab_.sec_hazard_open();
		persisted_settings_->source_creation_sec_template_open = source_creation_tab_.sec_template_open();
		persisted_settings_->source_creation_sec_comparator_open = source_creation_tab_.sec_comparator_open();
		persisted_settings_->source_creation_sec_plane_open = source_creation_tab_.sec_plane_open();
		persisted_settings_->source_creation_sec_hazard_open = source_creation_tab_.sec_hazard_open();
		persisted_settings_->sys_dyn_sec_bulk_actions_open = system_dynamics_tab_.sec_bulk_actions_open();
		persisted_settings_->sys_dyn_sec_plane_align_open = system_dynamics_tab_.sec_plane_align_open();
		persisted_settings_->sys_dyn_sec_generalized_open = system_dynamics_tab_.sec_generalized_open();
		persisted_settings_->sys_dyn_sec_sources_open = system_dynamics_tab_.sec_sources_open();
		persisted_settings_->interactions_sec_em_open = interactions_tab_.sec_em_open();
		persisted_settings_->interactions_sec_collisions_open = interactions_tab_.sec_collisions_open();
		persisted_settings_->interactions_sec_thermo_open = interactions_tab_.sec_thermo_open();
		persisted_settings_->interactions_sec_frag_open = interactions_tab_.sec_frag_open();
		persisted_settings_->interactions_sec_annihilation_open = interactions_tab_.sec_annihilation_open();
		persisted_settings_->catalog_filter_ranges_open = object_catalog_tab_.filter().physical_ranges_open;
		context_.randomizer_config() = object_creation_tab_.randomizer_config();
		persisted_settings_->body_randomizer = object_creation_tab_.randomizer_config();
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

		if (context_.tracking_active()) {
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.25f, 1.0f), "[TRACKING: %s]", context_.tracked_target_name().c_str());
			ImGui::SameLine();
			if (ImGui::SmallButton("Release Camera")) {
				context_.stop_tracking();
			}
			render_setting_tooltip("Stops active camera tracking and uncouples the gaze orientation from the current target.");
		}

		ImGui::SameLine();
		if (ImGui::Button("Clear All Bodies")) {
			context_.clear_all_bodies();
		}
		render_setting_tooltip("Permanently removes every object and independent black hole from the system. The primary central mass at the origin is unaffected.");
		ImGui::SameLine();
		const auto& all_presets = SystemPresetCatalog::presets();
		static int top_preset_idx = 0;
		std::vector<const char*> top_preset_labels;
		top_preset_labels.reserve(all_presets.size());
		for (const auto& p : all_presets) top_preset_labels.push_back(p.name.c_str());

		ImGui::SetNextItemWidth(260.0f);
		ImGui::Combo("##TopSystemPresetCombo", &top_preset_idx, top_preset_labels.data(), static_cast<int>(top_preset_labels.size()));
		ImGui::SameLine();
		if (ImGui::Button("Load System Preset")) {
			if (top_preset_idx >= 0 && static_cast<size_t>(top_preset_idx) < all_presets.size()) {
				apply_system_preset(context_, all_presets[static_cast<size_t>(top_preset_idx)]);
			}
		}
		render_setting_tooltip("Spawns the selected system preset (including the full real Solar System) with exact physical scales converted through the active constants engine.");
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
			if (ImGui::BeginTabItem("Randomizer Controls", nullptr, tab_flags(requested_tab, BodyManagerTab::RandomizerSettings))) {
				render_randomizer_controls_tab();
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}

		sync_persisted_settings();
		ImGui::End();
	}

private:
	void render_randomizer_controls_tab() noexcept {
		auto& cfg = object_creation_tab_.randomizer_config();

		ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Procedural Randomizer Bounds & Distribution Controls");
		render_hint_text("Configure the physical bounds, probability distributions, and mass-radius models that govern the 'Randomize Intelligent Defaults' button.");
		ImGui::Separator();

		if (ImGui::CollapsingHeader("Mass & Volumetric Density Models", ImGuiTreeNodeFlags_DefaultOpen)) {
			int mass_dist_idx = static_cast<int>(cfg.mass_distribution);
			if (ImGui::Combo("Mass Distribution Law", &mass_dist_idx, kRandomizerDistributionNames.data(), static_cast<int>(kRandomizerDistributionNames.size()))) {
				cfg.mass_distribution = static_cast<RandomizerDistributionLaw>(mass_dist_idx);
			}

			static bool min_m_log = true;
			static bool max_m_log = true;
			float min_m_flt = static_cast<float>(cfg.min_mass_ratio);
			if (slider_float_with_input("Min Mass Ratio (M / M_central)", &min_m_flt, 1e-15f, 1e-1f, "%.4e", &min_m_log, 1e-15f, 1.0f)) {
				cfg.min_mass_ratio = static_cast<double>(min_m_flt);
			}
			float max_m_flt = static_cast<float>(cfg.max_mass_ratio);
			if (slider_float_with_input("Max Mass Ratio (M / M_central)", &max_m_flt, 1e-12f, 1.0f, "%.4e", &max_m_log, 1e-12f, 1.0f)) {
				cfg.max_mass_ratio = static_cast<double>(max_m_flt);
			}

			if (cfg.mass_distribution == RandomizerDistributionLaw::PowerLaw) {
				float alpha = static_cast<float>(cfg.mass_power_law_alpha);
				if (ImGui::SliderFloat("Mass Power-Law Exponent (alpha)", &alpha, 0.5f, 4.0f, "%.2f")) {
					cfg.mass_power_law_alpha = static_cast<double>(alpha);
				}
			}

			ImGui::Separator();
			int rad_mode_idx = static_cast<int>(cfg.radius_mode);
			if (ImGui::Combo("Radius Determination Model", &rad_mode_idx, kRandomizerRadiusModeNames.data(), static_cast<int>(kRandomizerRadiusModeNames.size()))) {
				cfg.radius_mode = static_cast<RandomizerRadiusMode>(rad_mode_idx);
			}

			if (cfg.radius_mode == RandomizerRadiusMode::PhysicalDensity) {
				float min_rho = static_cast<float>(cfg.min_density_kg_m3);
				float max_rho = static_cast<float>(cfg.max_density_kg_m3);
				if (ImGui::DragFloatRange2("Volumetric Density Range (kg/m^3)", &min_rho, &max_rho, 10.0f, 50.0f, 25000.0f, "Min: %.0f", "Max: %.0f")) {
					cfg.min_density_kg_m3 = static_cast<double>(min_rho);
					cfg.max_density_kg_m3 = static_cast<double>(max_rho);
				}
			} else if (cfg.radius_mode == RandomizerRadiusMode::ExplicitBounds) {
				float min_rad_flt = static_cast<float>(cfg.min_explicit_radius_rg);
				float max_rad_flt = static_cast<float>(cfg.max_explicit_radius_rg);
				if (ImGui::DragFloatRange2("Radius Range (Gravitational Radii r_g)", &min_rad_flt, &max_rad_flt, 0.05f, 1e-4f, 50.0f, "Min: %.3f", "Max: %.3f")) {
					cfg.min_explicit_radius_rg = static_cast<double>(min_rad_flt);
					cfg.max_explicit_radius_rg = static_cast<double>(max_rad_flt);
				}
			}
		}

		if (ImGui::CollapsingHeader("Orbital Geometry & Kinematics Bounds", ImGuiTreeNodeFlags_DefaultOpen)) {
			int orb_dist_idx = static_cast<int>(cfg.orbit_distribution);
			if (ImGui::Combo("Orbital Distance Distribution", &orb_dist_idx, kRandomizerDistributionNames.data(), static_cast<int>(kRandomizerDistributionNames.size()))) {
				cfg.orbit_distribution = static_cast<RandomizerDistributionLaw>(orb_dist_idx);
			}

			float min_orb_flt = static_cast<float>(cfg.min_orbit_rg);
			float max_orb_flt = static_cast<float>(cfg.max_orbit_rg);
			if (ImGui::DragFloatRange2("Orbital Distance Range (r_g)", &min_orb_flt, &max_orb_flt, 1.0f, 2.5f, 10000.0f, "Min: %.1f", "Max: %.1f")) {
				cfg.min_orbit_rg = static_cast<double>(min_orb_flt);
				cfg.max_orbit_rg = static_cast<double>(max_orb_flt);
			}

			float min_ecc_flt = static_cast<float>(cfg.min_eccentricity);
			float max_ecc_flt = static_cast<float>(cfg.max_eccentricity);
			if (ImGui::DragFloatRange2("Orbital Eccentricity Range (e)", &min_ecc_flt, &max_ecc_flt, 0.01f, 0.0f, 0.99f, "Min: %.3f", "Max: %.3f")) {
				cfg.min_eccentricity = static_cast<double>(min_ecc_flt);
				cfg.max_eccentricity = static_cast<double>(max_ecc_flt);
			}

			float max_inc_flt = static_cast<float>(cfg.max_inclination_deg);
			if (ImGui::SliderFloat("Maximum Inclination (deg)", &max_inc_flt, 0.0f, 90.0f, "%.1f deg")) {
				cfg.max_inclination_deg = static_cast<double>(max_inc_flt);
			}

			float prog_frac = static_cast<float>(cfg.prograde_fraction);
			if (ImGui::SliderFloat("Prograde Probability", &prog_frac, 0.0f, 1.0f, "%.2f")) {
				cfg.prograde_fraction = static_cast<double>(prog_frac);
			}
		}

		if (ImGui::CollapsingHeader("Thermodynamics & Rotation Bounds")) {
			ImGui::Checkbox("Derive Equilibrium Temperature From Central Attractor", &cfg.radiative_equilibrium_temperature);
			if (!cfg.radiative_equilibrium_temperature) {
				float min_t = static_cast<float>(cfg.min_temperature_k);
				float max_t = static_cast<float>(cfg.max_temperature_k);
				if (ImGui::DragFloatRange2("Temperature Range (K)", &min_t, &max_t, 10.0f, 2.7f, 50000.0f, "Min: %.0f K", "Max: %.0f K")) {
					cfg.min_temperature_k = static_cast<double>(min_t);
					cfg.max_temperature_k = static_cast<double>(max_t);
				}
			}

			float min_spin = static_cast<float>(cfg.min_spin_period_hours);
			float max_spin = static_cast<float>(cfg.max_spin_period_hours);
			if (ImGui::DragFloatRange2("Rotation Period Range (hours)", &min_spin, &max_spin, 1.0f, 0.1f, 10000.0f, "Min: %.1f h", "Max: %.1f h")) {
				cfg.min_spin_period_hours = static_cast<double>(min_spin);
				cfg.max_spin_period_hours = static_cast<double>(max_spin);
			}

			float max_tilt = static_cast<float>(cfg.max_axial_tilt_deg);
			if (ImGui::SliderFloat("Maximum Axial Tilt", &max_tilt, 0.0f, 90.0f, "%.1f deg")) {
				cfg.max_axial_tilt_deg = static_cast<double>(max_tilt);
			}
		}

		if (ImGui::CollapsingHeader("Allowed Population Archetypes")) {
			ImGui::Checkbox("Terrestrial Worlds", &cfg.archetypes.allow_terrestrial);
			ImGui::SameLine();
			ImGui::Checkbox("Gas Giants", &cfg.archetypes.allow_gas_giant);
			ImGui::SameLine();
			ImGui::Checkbox("Ice Giants", &cfg.archetypes.allow_ice_giant);
			ImGui::Checkbox("Stars", &cfg.archetypes.allow_star);
			ImGui::SameLine();
			ImGui::Checkbox("Compact Remnants", &cfg.archetypes.allow_compact);
			ImGui::SameLine();
			ImGui::Checkbox("Asteroids / Moons", &cfg.archetypes.allow_asteroid);
		}

		if (ImGui::CollapsingHeader("Material & Mechanical Bounds")) {
			float min_fric = static_cast<float>(cfg.min_friction);
			float max_fric = static_cast<float>(cfg.max_friction);
			if (ImGui::DragFloatRange2("Friction Range", &min_fric, &max_fric, 0.01f, 0.0f, 1.0f, "Min: %.2f", "Max: %.2f")) {
				cfg.min_friction = min_fric;
				cfg.max_friction = max_fric;
			}
			float min_rest = static_cast<float>(cfg.min_restitution);
			float max_rest = static_cast<float>(cfg.max_restitution);
			if (ImGui::DragFloatRange2("Restitution Range", &min_rest, &max_rest, 0.01f, 0.0f, 1.0f, "Min: %.2f", "Max: %.2f")) {
				cfg.min_restitution = min_rest;
				cfg.max_restitution = max_rest;
			}
			float min_integ = static_cast<float>(cfg.min_integrity);
			float max_integ = static_cast<float>(cfg.max_integrity);
			if (ImGui::DragFloatRange2("Integrity Range", &min_integ, &max_integ, 0.05f, 0.0f, 10.0f, "Min: %.2f", "Max: %.2f")) {
				cfg.min_integrity = min_integ;
				cfg.max_integrity = max_integ;
			}
		}

		if (ImGui::CollapsingHeader("Multipole Moments & Zonal Harmonics Bounds")) {
			float min_j2 = static_cast<float>(cfg.min_j2);
			float max_j2 = static_cast<float>(cfg.max_j2);
			if (ImGui::DragFloatRange2("J2 Moment Range", &min_j2, &max_j2, 0.0001f, 0.0f, 0.05f, "Min: %.4f", "Max: %.4f")) {
				cfg.min_j2 = min_j2;
				cfg.max_j2 = max_j2;
			}
			float min_obl = static_cast<float>(cfg.min_oblateness);
			float max_obl = static_cast<float>(cfg.max_oblateness);
			if (ImGui::DragFloatRange2("Oblateness Range", &min_obl, &max_obl, 0.01f, 0.5f, 2.0f, "Min: %.2f", "Max: %.2f")) {
				cfg.min_oblateness = min_obl;
				cfg.max_oblateness = max_obl;
			}
		}

		if (ImGui::CollapsingHeader("Surface Appearance & Texture Bounds")) {
			float min_noise = static_cast<float>(cfg.min_noise_scale);
			float max_noise = static_cast<float>(cfg.max_noise_scale);
			if (ImGui::DragFloatRange2("Noise Scale Range", &min_noise, &max_noise, 0.1f, 0.5f, 50.0f, "Min: %.1f", "Max: %.1f")) {
				cfg.min_noise_scale = min_noise;
				cfg.max_noise_scale = max_noise;
			}
			float min_rough = static_cast<float>(cfg.min_roughness);
			float max_rough = static_cast<float>(cfg.max_roughness);
			if (ImGui::DragFloatRange2("Roughness Range", &min_rough, &max_rough, 0.01f, 0.01f, 1.0f, "Min: %.2f", "Max: %.2f")) {
				cfg.min_roughness = min_rough;
				cfg.max_roughness = max_rough;
			}
			float min_emit = static_cast<float>(cfg.min_emission);
			float max_emit = static_cast<float>(cfg.max_emission);
			if (ImGui::DragFloatRange2("Emission Range", &min_emit, &max_emit, 0.05f, 0.0f, 10.0f, "Min: %.2f", "Max: %.2f")) {
				cfg.min_emission = min_emit;
				cfg.max_emission = max_emit;
			}
		}

		if (ImGui::CollapsingHeader("Spacetime Source Randomizer Bounds")) {
			float min_smr = static_cast<float>(cfg.min_source_mass_ratio);
			float max_smr = static_cast<float>(cfg.max_source_mass_ratio);
			if (ImGui::DragFloatRange2("Source Mass Ratio (M_bh / M_central)", &min_smr, &max_smr, 0.01f, 0.001f, 5.0f, "Min: %.3f", "Max: %.3f")) {
				cfg.min_source_mass_ratio = min_smr;
				cfg.max_source_mass_ratio = max_smr;
			}
			float min_s_orb = static_cast<float>(cfg.min_source_orbit_rg);
			float max_s_orb = static_cast<float>(cfg.max_source_orbit_rg);
			if (ImGui::DragFloatRange2("Source Orbit Radius (r_g)", &min_s_orb, &max_s_orb, 1.0f, 4.0f, 500.0f, "Min: %.1f", "Max: %.1f")) {
				cfg.min_source_orbit_rg = min_s_orb;
				cfg.max_source_orbit_rg = max_s_orb;
			}
			float min_s_spin = static_cast<float>(cfg.min_source_spin);
			float max_s_spin = static_cast<float>(cfg.max_source_spin);
			if (ImGui::DragFloatRange2("Kerr Dimensionless Spin (a/M)", &min_s_spin, &max_s_spin, 0.01f, 0.0f, 0.999f, "Min: %.3f", "Max: %.3f")) {
				cfg.min_source_spin = min_s_spin;
				cfg.max_source_spin = max_s_spin;
			}
			float disk_prob = static_cast<float>(cfg.source_disk_probability);
			if (ImGui::SliderFloat("Accretion Disk Probability", &disk_prob, 0.0f, 1.0f, "%.2f")) {
				cfg.source_disk_probability = disk_prob;
			}
		}

		ImGui::Spacing();
		if (ImGui::Button("Reset Randomizer Bounds To Astrophysical Defaults", ImVec2(-1.0f, 26.0f))) {
			cfg = BodyRandomizerConfig{};
		}
		ImGui::SameLine();
		if (ImGui::Button("Test Random Draft Generation", ImVec2(-1.0f, 26.0f))) {
			context_.randomizer_config() = cfg;
			object_creation_tab_.reset_draft(context_);
			source_creation_tab_.reset_draft(context_);
		}

		cfg.sanitize();
		context_.randomizer_config() = cfg;
	}
};

}

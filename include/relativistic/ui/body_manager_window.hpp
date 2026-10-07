#pragma once

#include <imgui.h>
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/dynamics/pn/pn_body.hpp"
#include "relativistic/dynamics/pn/pn_nbody_system.hpp"
#include "relativistic/core/constants.hpp"
#include "relativistic/ui/numeric_slider_utils.hpp"
#include "relativistic/ui/tooltip_utils.hpp"
#include "relativistic/dynamics/bulk_body_actions.hpp"
#include "relativistic/dynamics/interaction_compatibility.hpp"
#include "relativistic/dynamics/body_surface_layers.hpp"
#include "relativistic/units/unit_system.hpp"
#include "relativistic/units/unit_aware_widgets.hpp"
#include "relativistic/io/user_settings.hpp"
#include "relativistic/optics/disk_thermal_profile.hpp"
#include "relativistic/ui/accretion_disk_editor.hpp"
#include <vector>
#include <string>
#include <string_view>
#include <array>
#include <span>
#include <cmath>
#include <cctype>
#include <numbers>
#include <algorithm>
#include <cstring>
#include <cfloat>
#include <cstdint>
#include <random>
#include <mutex>

namespace Relativistic::UI {

enum class BodyPresetTemplate : uint32_t {
	Custom = 0,
	Sun = 1,
	Earth = 2,
	Moon = 3,
	Jupiter = 4,
	Mars = 5,
	NeutronStar = 6,
	SupermassiveBlackHole = 7,
	StellarBlackHole = 8,
	TestParticle = 9
};

enum class BodyCatalogSortMode : uint32_t {
	CreationOrder = 0,
	Name = 1,
	Mass = 2,
	Distance = 3,
	Speed = 4
};

namespace BodyEditorSection {
	inline constexpr uint32_t Identity = 1U << 0;
	inline constexpr uint32_t SourceToggle = 1U << 1;
	inline constexpr uint32_t Physical = 1U << 2;
	inline constexpr uint32_t Multipoles = 1U << 3;
	inline constexpr uint32_t Material = 1U << 4;
	inline constexpr uint32_t Surface = 1U << 5;
	inline constexpr uint32_t AccretionDisk = 1U << 6;
	inline constexpr uint32_t Complete = Identity | SourceToggle | Physical | Multipoles | Material | Surface | AccretionDisk;
	inline constexpr uint32_t SpacetimeSource = Identity | Physical | AccretionDisk;
}

struct BodyEditorViewState {
	bool mass_log_mode{true};
	bool radius_log_mode{true};
	bool reference_radius_log_mode{true};
	bool magnetic_moment_log_mode{false};
	bool rotation_speed_log_mode{false};
	bool lifetime_log_mode{false};
	bool heat_capacity_log_mode{false};
	bool youngs_modulus_log_mode{true};
	bool resistance_log_mode{false};
	int layer_template_choice{0};
};

struct BodyEditResult {
	bool body_changed{false};
	bool layers_changed{false};
};

struct BodyTemplateSpec {
	std::string_view name;
	bool native_units;
	double mass;
	double radius;
	double j2;
	double j4;
	double temperature_kelvin;
	Dynamics::Body3DPreset preset;
	std::array<float, 4> color;
	std::array<float, 4> color_secondary;
	std::array<float, 4> color_tertiary;
	float noise_scale;
	float surface_roughness;
	float atmosphere_thickness;
	float emission_intensity;
	float rotation_speed;
	float polar_cap_strength;
	float night_side_light_intensity;
	bool ring_system;
	int layer_template_first;
	int layer_template_second;
};

inline constexpr std::array<BodyTemplateSpec, 9> kBodyTemplateSpecs{{
	BodyTemplateSpec{"Sun", false, 1.98847e30, 6.9634e8, 2.2e-7, 0.0, 5772.0, Dynamics::Body3DPreset::Star,
		{1.0f, 0.85f, 0.5f, 1.0f}, {1.0f, 0.55f, 0.15f, 1.0f}, {1.0f, 0.95f, 0.7f, 1.0f},
		8.0f, 0.6f, 0.0f, 2.5f, 0.05f, 0.0f, 0.0f, false, -1, -1},
	BodyTemplateSpec{"Earth", false, 5.9722e24, 6.378137e6, 1.08263e-3, 0.0, 288.0, Dynamics::Body3DPreset::TerrestrialPlanet,
		{0.10f, 0.35f, 0.65f, 1.0f}, {0.20f, 0.50f, 0.22f, 1.0f}, {0.95f, 0.95f, 0.98f, 1.0f},
		5.0f, 0.45f, 0.18f, 0.0f, 0.12f, 0.45f, 0.4f, false, 2, -1},
	BodyTemplateSpec{"Moon", false, 7.342e22, 1.7374e6, 2.0335e-4, 0.0, 250.0, Dynamics::Body3DPreset::Metallic,
		{0.55f, 0.55f, 0.58f, 1.0f}, {0.32f, 0.32f, 0.35f, 1.0f}, {0.75f, 0.75f, 0.78f, 1.0f},
		7.0f, 0.75f, 0.0f, 0.0f, 0.03f, 0.0f, 0.0f, false, 3, -1},
	BodyTemplateSpec{"Jupiter", false, 1.89813e27, 7.1492e7, 1.469657e-2, -5.86609e-4, 165.0, Dynamics::Body3DPreset::GasGiant,
		{0.82f, 0.65f, 0.45f, 1.0f}, {0.62f, 0.40f, 0.24f, 1.0f}, {0.92f, 0.85f, 0.75f, 1.0f},
		3.5f, 0.3f, 0.32f, 0.0f, 0.35f, 0.2f, 0.0f, true, 5, -1},
	BodyTemplateSpec{"Mars", false, 6.4171e23, 3.3895e6, 1.96045e-3, 0.0, 210.0, Dynamics::Body3DPreset::TerrestrialPlanet,
		{0.72f, 0.35f, 0.20f, 1.0f}, {0.48f, 0.24f, 0.15f, 1.0f}, {0.95f, 0.95f, 0.98f, 1.0f},
		6.0f, 0.65f, 0.05f, 0.0f, 0.11f, 0.0f, 0.0f, false, 0, 6},
	BodyTemplateSpec{"Neutron Star", false, 2.8e30, 12000.0, 0.0, 0.0, 1.0e6, Dynamics::Body3DPreset::NeutronStar,
		{0.85f, 0.90f, 1.0f, 1.0f}, {0.55f, 0.72f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f},
		1.5f, 0.15f, 0.0f, 3.0f, 1.8f, 0.0f, 0.0f, false, -1, -1},
	BodyTemplateSpec{"Supermassive BH", false, 8.0e36, 1.2e10, 0.0, 0.0, 0.0, Dynamics::Body3DPreset::BlackHole,
		{0.02f, 0.02f, 0.03f, 1.0f}, {0.06f, 0.05f, 0.08f, 1.0f}, {0.04f, 0.04f, 0.03f, 1.0f},
		1.0f, 0.1f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, false, -1, -1},
	BodyTemplateSpec{"Stellar Black Hole", false, 2.0e31, 30000.0, 0.0, 0.0, 0.0, Dynamics::Body3DPreset::BlackHole,
		{0.02f, 0.02f, 0.03f, 1.0f}, {0.06f, 0.05f, 0.08f, 1.0f}, {0.04f, 0.04f, 0.03f, 1.0f},
		1.0f, 0.1f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, false, -1, -1},
	BodyTemplateSpec{"Test Particle", true, 0.0, 1.0, 0.0, 0.0, 0.0, Dynamics::Body3DPreset::Asteroid,
		{1.0f, 1.0f, 1.0f, 1.0f}, {0.7f, 0.7f, 0.7f, 1.0f}, {0.85f, 0.85f, 0.85f, 1.0f},
		4.0f, 0.5f, 0.0f, 0.0f, 0.1f, 0.0f, 0.0f, false, -1, -1}
}};

struct BodyCreationArchetype {
	std::string_view label;
	double mass_ratio_min;
	double mass_ratio_max;
	double size_factor_min;
	double size_factor_max;
	double orbit_gravitational_radii_min;
	double orbit_gravitational_radii_max;
	double breakup_spin_fraction;
	double quadrupole_min;
	double quadrupole_max;
	double j2_min;
	double j2_max;
	double j3_min;
	double j3_max;
	double j4_min;
	double j4_max;
	Dynamics::Body3DPreset preset;
	double hue_center;
	double hue_spread;
	double saturation;
	double value;
	double noise_min;
	double noise_max;
	double roughness_min;
	double roughness_max;
	double atmosphere_min;
	double atmosphere_max;
	double emission_min;
	double emission_max;
	double rotation_min;
	double rotation_max;
};

inline constexpr std::array<BodyCreationArchetype, 6> kCreationArchetypes{{
	BodyCreationArchetype{"Probe", 1e-12, 1e-9, 4e-4, 2e-3, 12.0, 120.0, 0.05, 1e-12, 1e-8, 1e-10, 1e-6, 1e-12, 1e-7, 1e-12, 1e-7,
		Dynamics::Body3DPreset::Asteroid, 0.55, 0.08, 0.10, 0.75, 6.0, 14.0, 0.6, 0.95, 0.0, 0.0, 0.0, 0.0, 0.05, 0.4},
	BodyCreationArchetype{"Shard", 1e-10, 1e-7, 1e-3, 5e-3, 10.0, 150.0, 0.1, 1e-10, 1e-6, 1e-8, 1e-4, 1e-10, 1e-5, 1e-10, 1e-5,
		Dynamics::Body3DPreset::Metallic, 0.08, 0.06, 0.30, 0.55, 4.0, 10.0, 0.5, 0.85, 0.0, 0.0, 0.0, 0.0, 0.05, 0.35},
	BodyCreationArchetype{"World", 1e-7, 1e-5, 4e-3, 1.5e-2, 10.0, 200.0, 0.05, 1e-9, 1e-5, 1e-4, 3e-3, 1e-8, 1e-5, 1e-9, 1e-5,
		Dynamics::Body3DPreset::TerrestrialPlanet, 0.42, 0.14, 0.55, 0.75, 3.0, 8.0, 0.3, 0.6, 0.08, 0.3, 0.0, 0.0, 0.05, 0.2},
	BodyCreationArchetype{"Giant", 1e-5, 1e-3, 1.2e-2, 4e-2, 15.0, 250.0, 0.25, 1e-8, 1e-4, 1e-3, 2e-2, 1e-7, 1e-4, 1e-7, 1e-4,
		Dynamics::Body3DPreset::GasGiant, 0.10, 0.16, 0.45, 0.85, 2.0, 6.0, 0.2, 0.5, 0.2, 0.4, 0.0, 0.0, 0.1, 0.5},
	BodyCreationArchetype{"Compact", 1e-3, 2e-2, 4.0, 8.0, 12.0, 250.0, 0.4, 1e-12, 1e-7, 1e-10, 1e-6, 1e-12, 1e-7, 1e-12, 1e-7,
		Dynamics::Body3DPreset::NeutronStar, 0.58, 0.05, 0.08, 0.9, 1.5, 5.0, 0.15, 0.4, 0.0, 0.0, 0.5, 2.5, 0.5, 2.0},
	BodyCreationArchetype{"Astral", 2e-3, 4e-2, 1.5e-2, 4e-2, 25.0, 300.0, 0.05, 1e-8, 1e-3, 1e-7, 1e-4, 1e-7, 1e-4, 1e-7, 1e-4,
		Dynamics::Body3DPreset::Star, 0.13, 0.10, 0.35, 1.0, 3.0, 12.0, 0.4, 0.85, 0.0, 0.0, 1.5, 3.5, 0.02, 0.2}
}};

inline constexpr std::array<const char*, 10> kBodyTemplateNames{
	"Custom Body", "Sun (Solar Mass & Radius)", "Earth (Terrestrial Planet)", "Moon (Natural Satellite)",
	"Jupiter (Gas Giant)", "Mars (Telluric Planet)", "Neutron Star (Compact)", "Supermassive Black Hole",
	"Stellar Mass Black Hole", "Test Particle (Zero Mass)"
};

inline constexpr std::array<const char*, 4> kBodyGeometryNames{
	"Oblate Spheroid (Spin/J2 Deformed)", "Rigid Sphere", "Prolate Spheroid", "Triaxial Ellipsoid"
};

inline constexpr std::array<const char*, 10> kBodySurfacePresetNames{
	"Star", "Terrestrial Planet", "Gas Giant", "Ice Giant", "Metallic / Moon",
	"Asteroid", "Neutron Star", "Pulsar", "Black Hole", "Custom"
};

inline constexpr std::array<const char*, 14> kBodyTextureModeNames{
	"Procedural Noise Shader", "Solid Color", "Color Palette Blend", "Banded Gas Giant",
	"Cratered Terrestrial", "Stellar Granulation", "Accretion Flow", "Marbled Stone",
	"Ringed Gas Giant (Bands + Polar Caps)", "Icy Cracked Surface", "Volcanic Magma",
	"City Lights (Night Side)", "Nebulous Gas Cloud", "Earth Photographic Map (Blue Marble)"
};

inline constexpr std::array<const char*, 3> kEarthMapVariantNames{
	"Day Map", "Night Map", "Automatic Day/Night Blend"
};

inline constexpr std::array<const char*, 2> kEarthMapQualityNames{
	"1K (1024x512, lowest GPU cost)", "2K (2048x1024, highest detail)"
};

inline constexpr std::array<const char*, 6> kBodyAtmosphereModeNames{
	"Rayleigh Limb Shell", "Volumetric Scattering", "Off", "Thick Haze", "Volumetric Mie", "Glowing Corona"
};

class BodyManagerWindow {
private:
	static constexpr int kCentralObjectIndex = -2;

	bool is_open_{false};
	Orchestrator::SimulationOrchestrator<1024>& orchestrator_;
	IO::UserSettings* persisted_settings_{nullptr};

	int selected_body_index_{-1};
	int creation_preset_{0};
	bool request_focus_creation_tab_{false};
	bool randomize_after_spawn_{true};
	Dynamics::PostNewtonianBody creation_draft_{};
	Dynamics::BodySurfaceLayerSet creation_layers_{};
	Dynamics::PostNewtonianBody black_hole_draft_{};
	bool creation_draft_pristine_{true};
	double creation_reference_mass_{-1.0};
	bool black_hole_draft_pristine_{true};
	double black_hole_reference_mass_{-1.0};
	BodyEditorViewState selected_view_{};
	BodyEditorViewState creation_view_{};
	BodyEditorViewState source_view_{};
	bool central_mass_log_mode_{true};
	std::mt19937_64 creation_rng_{std::random_device{}()};

	char search_filter_[64]{};
	int sort_mode_{static_cast<int>(BodyCatalogSortMode::CreationOrder)};
	bool sort_descending_{false};
	float list_pane_width_{230.0f};
	int tracked_body_id_{-1};
	bool tracking_enabled_{false};
	float global_velocity_[3]{0.0f, 0.0f, 0.0f};
	float global_spin_[3]{0.0f, 0.0f, 0.0f};
	float grid_spacing_{1.0f};
	int bulk_parameter_index_{0};
	float bulk_parameter_value_{1.0f};
	bool bulk_parameter_value_log_mode_{false};
	bool em_permittivity_log_mode_{true};
	bool em_permeability_log_mode_{true};
	bool collision_stiffness_log_mode_{true};
	bool fragmentation_min_mass_log_mode_{true};
	bool fragmentation_energy_integrity_log_mode_{true};
	bool fragmentation_tidal_integrity_log_mode_{true};

public:
	explicit BodyManagerWindow(Orchestrator::SimulationOrchestrator<1024>& orchestrator)
		: orchestrator_(orchestrator) {
		randomize_creation_defaults();
		reset_black_hole_draft();
	}

	void attach_persisted_settings(IO::UserSettings& settings) noexcept {
		persisted_settings_ = &settings;
		sort_mode_ = static_cast<int>(settings.body_manager_sort_mode);
		sort_descending_ = settings.body_manager_sort_descending;
		std::strncpy(search_filter_, settings.body_manager_search_filter.c_str(), sizeof(search_filter_) - 1);
		search_filter_[sizeof(search_filter_) - 1] = '\0';
		list_pane_width_ = settings.body_manager_list_pane_width;
		bulk_parameter_index_ = static_cast<int>(settings.body_manager_bulk_parameter_index);
		creation_view_.mass_log_mode = settings.body_manager_new_body_mass_log_mode;
		creation_view_.radius_log_mode = settings.body_manager_new_body_radius_log_mode;
	}

	void sync_persisted_settings() noexcept {
		if (persisted_settings_ == nullptr) return;
		persisted_settings_->body_manager_sort_mode = static_cast<uint32_t>(sort_mode_);
		persisted_settings_->body_manager_sort_descending = sort_descending_;
		persisted_settings_->body_manager_search_filter = search_filter_;
		persisted_settings_->body_manager_list_pane_width = list_pane_width_;
		persisted_settings_->body_manager_bulk_parameter_index = static_cast<uint32_t>(bulk_parameter_index_);
		persisted_settings_->body_manager_new_body_mass_log_mode = creation_view_.mass_log_mode;
		persisted_settings_->body_manager_new_body_radius_log_mode = creation_view_.radius_log_mode;
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
		ImGui::SetNextWindowSize(ImVec2(600.0f, 680.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSizeConstraints(ImVec2(440.0f, 380.0f), ImVec2(FLT_MAX, FLT_MAX));

		if (!ImGui::Begin("Celestial Body & N-Body Manager", &is_open_)) {
			ImGui::End();
			return;
		}

		auto& sys = orchestrator_.nbody_system();
		update_tracking(sys);
		const size_t body_count = sys.body_count();

		ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "Active Bodies in System: %zu", body_count);
		render_setting_tooltip("Total number of orbiting bodies currently tracked by the post-Newtonian N-body integrator, excluding the central spacetime source itself.");
		ImGui::SameLine();
		if (ImGui::Button("Clear All Bodies")) {
			orchestrator_.surface_layers().clear();
			sys.clear_bodies();
			selected_body_index_ = -1;
			orchestrator_.notify_state_changed();
		}
		render_setting_tooltip("Removes every orbiting body from the system. The central spacetime source and its mass/spin/charge are unaffected.");

		ImGui::Separator();

		if (ImGui::BeginTabBar("BodyManagerTabs")) {
			ImGuiTabItemFlags creation_flags = ImGuiTabItemFlags_None;
			if (request_focus_creation_tab_) {
				creation_flags |= ImGuiTabItemFlags_SetSelected;
				request_focus_creation_tab_ = false;
			}
			if (ImGui::BeginTabItem("Body Catalog")) {
				render_body_list_tab();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Create Body", nullptr, creation_flags)) {
				render_creation_tab();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Spacetime Sources")) {
				render_spacetime_sources_tab();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("System Dynamics")) {
				render_system_dynamics_tab();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Interactions")) {
				render_interactions_tab();
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}

		sync_persisted_settings();
		ImGui::End();
	}

private:
	template <typename Engine>
	[[nodiscard]] static double get_speed_scale(const Engine& engine) noexcept {
		if constexpr (requires { engine.speed_scale(); }) {
			return static_cast<double>(engine.speed_scale());
		} else if constexpr (requires { engine.velocity_scale(); }) {
			return static_cast<double>(engine.velocity_scale());
		} else if constexpr (requires { engine.time_scale(); }) {
			const double ts = static_cast<double>(engine.time_scale());
			return (ts > 0.0) ? (static_cast<double>(engine.length_scale()) / ts) : 299792458.0;
		} else if constexpr (requires { engine.speed_of_light(); }) {
			return static_cast<double>(engine.speed_of_light());
		} else if constexpr (requires { engine.c(); }) {
			return static_cast<double>(engine.c());
		} else {
			return 299792458.0;
		}
	}

	[[nodiscard]] double mass_scale() const noexcept {
		return std::max(orchestrator_.constants_engine().mass_scale(), 1e-300);
	}

	[[nodiscard]] double length_scale() const noexcept {
		return std::max(orchestrator_.constants_engine().length_scale(), 1e-300);
	}

	void look_at(const std::array<double, 3>& target) noexcept {
		auto& camera = orchestrator_.camera();
		const double dx = target[0] - camera.position[0];
		const double dy = target[1] - camera.position[1];
		const double dz = target[2] - camera.position[2];
		const double distance = std::sqrt(dx * dx + dy * dy + dz * dz);
		if (distance <= 1e-9) return;
		camera.target = target;
		camera.yaw = std::atan2(dy, dx) * (180.0 / std::numbers::pi);
		camera.pitch = std::asin(std::clamp(dz / distance, -1.0, 1.0)) * (180.0 / std::numbers::pi);
		camera.roll = 0.0;
	}

	void update_tracking(Dynamics::PostNewtonianSystem& sys) noexcept {
		if (!tracking_enabled_) return;
		std::lock_guard<std::recursive_mutex> tracking_lock(sys.bodies_mutex());
		if (tracked_body_id_ == kCentralObjectIndex) {
			look_at({0.0, 0.0, 0.0});
			return;
		}
		for (const auto& body : sys.bodies()) {
			if (body.id == static_cast<uint32_t>(tracked_body_id_) && body.enabled) {
				look_at(body.position);
				return;
			}
		}
		tracking_enabled_ = false;
		tracked_body_id_ = -1;
	}

	[[nodiscard]] std::string unique_name(std::string_view base) const {
		std::string original = base.empty() ? "Body" : std::string(base);
		std::lock_guard<std::recursive_mutex> naming_lock(orchestrator_.nbody_system().bodies_mutex());
		const auto& bodies = orchestrator_.nbody_system().bodies();
		auto exists = [&](std::string_view name) {
			return std::any_of(bodies.begin(), bodies.end(), [&](const auto& body) { return body.name_view() == name; });
		};

		std::string stem = original;
		uint32_t start_suffix = 2;
		const size_t space_pos = original.find_last_of(' ');
		if (space_pos != std::string::npos && space_pos + 1 < original.size()) {
			const std::string_view tail = std::string_view(original).substr(space_pos + 1);
			if (!tail.empty() && std::all_of(tail.begin(), tail.end(), [](unsigned char c) { return std::isdigit(c) != 0; })) {
				const uint32_t parsed = static_cast<uint32_t>(std::strtoul(std::string(tail).c_str(), nullptr, 10));
				if (parsed > 0) {
					stem = original.substr(0, space_pos);
					start_suffix = parsed + 1;
				}
			}
		}

		if (!exists(original)) return original;
		for (uint32_t suffix = start_suffix; suffix < 1000000; ++suffix) {
			const std::string numbered = stem + " " + std::to_string(suffix);
			if (!exists(numbered)) return numbered;
		}
		return original;
	}

	[[nodiscard]] static std::string display_name(const Dynamics::PostNewtonianBody& body) {
		if (body.has_name()) {
			return std::string(body.name_view());
		}
		return "Body #" + std::to_string(body.id);
	}

	[[nodiscard]] static double distance_from_center(const Dynamics::PostNewtonianBody& body) noexcept {
		return std::sqrt(body.position[0] * body.position[0] + body.position[1] * body.position[1] + body.position[2] * body.position[2]);
	}

	[[nodiscard]] std::array<double, 3> compute_circular_orbit_velocity(const std::array<double, 3>& pos, double central_mass) const noexcept {
		const double r2 = pos[0] * pos[0] + pos[1] * pos[1] + pos[2] * pos[2];
		const double r = std::sqrt(std::max(r2, 1e-12));
		const double speed = std::sqrt(std::max(orchestrator_.physical_gravitational_constant() * central_mass, 0.0) / r);
		const std::array<double, 3> up{0.0, 0.0, 1.0};
		std::array<double, 3> tangent{
			up[1] * pos[2] - up[2] * pos[1],
			up[2] * pos[0] - up[0] * pos[2],
			up[0] * pos[1] - up[1] * pos[0]
		};
		const double t_len = std::sqrt(tangent[0] * tangent[0] + tangent[1] * tangent[1] + tangent[2] * tangent[2]);
		if (t_len < 1e-9) {
			return {0.0, speed, 0.0};
		}
		return {tangent[0] / t_len * speed, tangent[1] / t_len * speed, tangent[2] / t_len * speed};
	}

	[[nodiscard]] double random_real(double min_val, double max_val) noexcept {
		std::uniform_real_distribution<double> dist(min_val, std::max(min_val, max_val));
		return dist(creation_rng_);
	}

	[[nodiscard]] int random_int(int min_val, int max_val) noexcept {
		std::uniform_int_distribution<int> dist(min_val, max_val);
		return dist(creation_rng_);
	}

	[[nodiscard]] double sample_log_uniform(double min_val, double max_val) noexcept {
		return std::pow(10.0, random_real(std::log10(min_val), std::log10(max_val)));
	}

	[[nodiscard]] double sample_signed_uniform(double min_abs, double max_abs) noexcept {
		return (random_int(0, 1) == 0 ? -1.0 : 1.0) * random_real(min_abs, max_abs);
	}

	[[nodiscard]] static std::array<float, 3> hsv_to_rgb(float h, float s, float v) noexcept {
		const float c = v * s;
		const float hp = std::fmod(h * 6.0f, 6.0f);
		const float x = c * (1.0f - std::abs(std::fmod(hp, 2.0f) - 1.0f));
		float r = 0.0f, g = 0.0f, b = 0.0f;
		if (hp < 1.0f) { r = c; g = x; }
		else if (hp < 2.0f) { r = x; g = c; }
		else if (hp < 3.0f) { g = c; b = x; }
		else if (hp < 4.0f) { g = x; b = c; }
		else if (hp < 5.0f) { r = x; b = c; }
		else { r = c; b = x; }
		const float m = v - c;
		return {r + m, g + m, b + m};
	}

	[[nodiscard]] std::string synthesize_creation_name(std::string_view label, double mass, double radius, double orbit_radius) {
		static constexpr std::array<std::string_view, 28> prefixes{
			"Astra", "Boreal", "Cinder", "Drift", "Echo", "Eon", "Flux", "Halo",
			"Ion", "Kestrel", "Lumen", "Nova", "Nyx", "Orbit", "Quasar", "Rift",
			"Solace", "Spectrum", "Titan", "Umbra", "Vanta", "Velvet", "Vesper",
			"Warden", "Zenith", "Auric", "Kepler", "Morrow"
		};
		static constexpr std::array<std::string_view, 8> accents{
			"Prime", "I", "II", "III", "Arc", "Node", "Field", "Halo"
		};

		const std::string_view prefix = prefixes[static_cast<size_t>(random_int(0, static_cast<int>(prefixes.size() - 1)))];
		const std::string_view accent = accents[static_cast<size_t>(random_int(0, static_cast<int>(accents.size() - 1)))];
		const uint32_t suffix = static_cast<uint32_t>(random_int(10, 999));

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

	static void apply_material_defaults(Dynamics::PostNewtonianBody& b, Dynamics::Body3DPreset preset) noexcept {
		double specific_heat = 900.0;
		b.friction_coefficient = 0.4;
		b.restitution = 0.5;
		b.absorption_factor = 0.7;
		b.integrity = 1.0;
		switch (preset) {
			case Dynamics::Body3DPreset::Star:
				b.temperature = 5772.0;
				b.absorption_factor = 1.0;
				b.restitution = 0.0;
				b.friction_coefficient = 0.0;
				specific_heat = 1.2e4;
				break;
			case Dynamics::Body3DPreset::TerrestrialPlanet:
				b.temperature = 288.0;
				specific_heat = 1000.0;
				break;
			case Dynamics::Body3DPreset::GasGiant:
			case Dynamics::Body3DPreset::IceGiant:
				b.temperature = 130.0;
				b.restitution = 0.1;
				specific_heat = 1.2e4;
				break;
			case Dynamics::Body3DPreset::Metallic:
				b.temperature = 220.0;
				specific_heat = 600.0;
				break;
			case Dynamics::Body3DPreset::NeutronStar:
			case Dynamics::Body3DPreset::Pulsar:
				b.temperature = 1.0e6;
				b.restitution = 0.0;
				b.friction_coefficient = 0.0;
				specific_heat = 1.0e3;
				break;
			default:
				b.temperature = 180.0;
				specific_heat = 800.0;
				break;
		}
		b.heat_capacity = std::max(b.mass, 1e-9) * specific_heat;
	}

	void reset_creation_view() noexcept {
		creation_view_.mass_log_mode = true;
		creation_view_.radius_log_mode = true;
		creation_view_.reference_radius_log_mode = true;
	}

	void randomize_creation_defaults() noexcept {
		const auto& profile = kCreationArchetypes[static_cast<size_t>(random_int(0, static_cast<int>(kCreationArchetypes.size()) - 1))];
		const double central_mass = std::max(orchestrator_.parameters().mass, 1e-12);
		auto& d = creation_draft_;
		d = Dynamics::PostNewtonianBody{};
		apply_body_preset_defaults(d, profile.preset);

		const double reference_mass = std::max(central_mass, 1.0);
		const double orbit_radius = reference_mass * sample_log_uniform(profile.orbit_gravitational_radii_min, profile.orbit_gravitational_radii_max);
		const double mass_ratio = sample_log_uniform(profile.mass_ratio_min, profile.mass_ratio_max);
		const double mass_percentile = std::clamp(std::log(mass_ratio / profile.mass_ratio_min) / std::log(profile.mass_ratio_max / profile.mass_ratio_min), 0.0, 1.0);
		d.mass = reference_mass * mass_ratio;
		if (profile.preset == Dynamics::Body3DPreset::NeutronStar) {
			d.radius = d.mass * random_real(profile.size_factor_min, profile.size_factor_max);
		} else {
			const double size_fraction = profile.size_factor_min * std::pow(profile.size_factor_max / profile.size_factor_min, mass_percentile) * random_real(0.85, 1.15);
			d.radius = std::max(orbit_radius * size_fraction, 4.0 * d.mass);
		}
		d.radius = std::max(d.radius, 1e-6);
		d.reference_radius = d.radius;
		d.quadrupole_moment = sample_signed_uniform(profile.quadrupole_min, profile.quadrupole_max);
		d.j2 = sample_log_uniform(profile.j2_min, profile.j2_max);
		d.j3 = sample_signed_uniform(profile.j3_min, profile.j3_max);
		d.j4 = sample_signed_uniform(profile.j4_min, profile.j4_max);

		const double inclination = random_real(-0.35, 0.35);
		const double theta = std::numbers::pi * 0.5 - inclination;
		const double phi = random_real(0.0, 2.0 * std::numbers::pi);
		const double sin_theta = std::sin(theta);
		d.position = {orbit_radius * sin_theta * std::cos(phi), orbit_radius * sin_theta * std::sin(phi), orbit_radius * std::cos(theta)};

		auto velocity = compute_circular_orbit_velocity(d.position, central_mass);
		if (std::abs(velocity[0]) < 1e-12 && std::abs(velocity[1]) < 1e-12 && std::abs(velocity[2]) < 1e-12) {
			velocity = {0.0, std::sqrt(central_mass / std::max(orbit_radius, 1e-9)), 0.0};
		}
		const double speed_scale = random_real(0.82, 1.18);
		d.velocity = {velocity[0] * speed_scale, velocity[1] * speed_scale, velocity[2] * speed_scale};
		const double breakup_rate = std::sqrt(std::max(orchestrator_.physical_gravitational_constant() * d.mass / (d.radius * d.radius * d.radius), 0.0));
		const double spin_rate = breakup_rate * profile.breakup_spin_fraction * random_real(0.1, 1.0);
		const double axis_tilt = random_real(0.0, 0.5);
		const double axis_azimuth = random_real(0.0, 2.0 * std::numbers::pi);
		const double angular_momentum = 0.4 * d.mass * d.radius * d.radius * spin_rate;
		d.rotation_speed = spin_rate;
		d.spin = {
			angular_momentum * std::sin(axis_tilt) * std::cos(axis_azimuth),
			angular_momentum * std::sin(axis_tilt) * std::sin(axis_azimuth),
			angular_momentum * std::cos(axis_tilt)
		};
		apply_material_defaults(d, profile.preset);

		const float primary_hue = static_cast<float>(std::fmod(profile.hue_center + random_real(-profile.hue_spread, profile.hue_spread) + 1.0, 1.0));
		const auto primary_rgb = hsv_to_rgb(primary_hue, static_cast<float>(profile.saturation), static_cast<float>(profile.value));
		d.color = {primary_rgb[0], primary_rgb[1], primary_rgb[2], 1.0f};
		const float secondary_hue = std::fmod(primary_hue + static_cast<float>(random_real(0.03, 0.12)) + 1.0f, 1.0f);
		const auto secondary_rgb = hsv_to_rgb(secondary_hue, static_cast<float>(std::clamp(profile.saturation * 1.2, 0.0, 1.0)), static_cast<float>(std::clamp(profile.value * 0.7, 0.0, 1.0)));
		d.color_secondary = {secondary_rgb[0], secondary_rgb[1], secondary_rgb[2], 1.0f};
		const float tertiary_hue = std::fmod(primary_hue + static_cast<float>(random_real(0.25, 0.55)) + 1.0f, 1.0f);
		const auto tertiary_rgb = hsv_to_rgb(tertiary_hue, static_cast<float>(std::clamp(profile.saturation * 0.6, 0.0, 1.0)), static_cast<float>(std::clamp(profile.value * 1.1, 0.0, 1.0)));
		d.color_tertiary = {tertiary_rgb[0], tertiary_rgb[1], tertiary_rgb[2], 1.0f};

		d.surface_noise_scale = static_cast<float>(random_real(profile.noise_min, profile.noise_max));
		d.surface_roughness = static_cast<float>(random_real(profile.roughness_min, profile.roughness_max));
		d.atmosphere_thickness = static_cast<float>(random_real(profile.atmosphere_min, profile.atmosphere_max));
		d.emission_intensity = static_cast<float>(random_real(profile.emission_min, profile.emission_max));
		d.rotation_speed_3d = static_cast<float>(random_real(profile.rotation_min, profile.rotation_max));
		d.texture_detail_scale = static_cast<float>(random_real(0.6, 2.2));
		const bool is_world = profile.preset == Dynamics::Body3DPreset::TerrestrialPlanet;
		const bool is_giant = profile.preset == Dynamics::Body3DPreset::GasGiant;
		d.polar_cap_strength = (is_world || is_giant) ? static_cast<float>(random_real(0.1, 0.5)) : 0.0f;
		d.night_side_light_intensity = is_world ? static_cast<float>(random_real(0.0, 0.6)) : 0.0f;
		d.ring_system_enabled = is_giant && (random_int(0, 1) == 0);

		d.set_name(unique_name(synthesize_creation_name(profile.label, d.mass, d.radius, orbit_radius)));
		creation_draft_pristine_ = true;
		creation_reference_mass_ = orchestrator_.parameters().mass;
		creation_layers_ = Dynamics::BodySurfaceLayerSet{};
		creation_preset_ = static_cast<int>(BodyPresetTemplate::Custom);
		reset_creation_view();
	}

	void reset_black_hole_draft() noexcept {
		auto& d = black_hole_draft_;
		d = Dynamics::PostNewtonianBody{};
		const double central_mass = std::max(orchestrator_.parameters().mass, 1.0);
		d.mass = 0.25 * central_mass;
		d.radius = 2.0 * d.mass;
		d.reference_radius = d.radius;
		d.position = {30.0 * central_mass, 0.0, 0.0};
		d.velocity = compute_circular_orbit_velocity(d.position, central_mass + d.mass);
		black_hole_draft_pristine_ = true;
		black_hole_reference_mass_ = orchestrator_.parameters().mass;
		d.is_spacetime_source = true;
		apply_body_preset_defaults(d, Dynamics::Body3DPreset::BlackHole);
		d.set_name(unique_name("New Black Hole"));
	}

	void apply_template_preset(BodyPresetTemplate preset) noexcept {
		if (preset == BodyPresetTemplate::Custom) {
			randomize_creation_defaults();
			return;
		}
		const size_t index = static_cast<size_t>(preset) - 1U;
		if (index >= kBodyTemplateSpecs.size()) return;
		const auto& spec = kBodyTemplateSpecs[index];
		auto& d = creation_draft_;
		const std::array<double, 3> kept_position = d.position;
		const std::array<double, 3> kept_velocity = d.velocity;
		d = Dynamics::PostNewtonianBody{};
		apply_body_preset_defaults(d, spec.preset);
		d.position = kept_position;
		d.velocity = kept_velocity;
		d.mass = spec.native_units ? spec.mass : spec.mass / mass_scale();
		d.radius = spec.native_units ? spec.radius : spec.radius / length_scale();
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
		d.set_name(unique_name(spec.name));

		creation_layers_ = Dynamics::BodySurfaceLayerSet{};
		for (const int layer_template : {spec.layer_template_first, spec.layer_template_second}) {
			if (layer_template >= 0) {
				static_cast<void>(creation_layers_.add(Dynamics::SurfaceLayerDefinition::from_template(static_cast<Dynamics::SurfaceLayerTemplate>(layer_template))));
			}
		}
		d.velocity = compute_circular_orbit_velocity(d.position, std::max(orchestrator_.parameters().mass, 1e-12));
		creation_draft_pristine_ = false;
		reset_creation_view();
	}

	static void apply_body_preset_defaults(Dynamics::PostNewtonianBody& b, Dynamics::Body3DPreset preset) noexcept {
		switch (preset) {
			case Dynamics::Body3DPreset::Star:
				b.atmosphere_mode = Dynamics::Body3DAtmosphereMode::GlowingCorona;
				b.surface_texture_mode = Dynamics::Body3DSurfaceTextureMode::StellarGranulation;
				b.color = {1.0f, 0.85f, 0.5f, 1.0f};
				b.color_secondary = {1.0f, 0.0f, 0.0f, 1.0f};
				b.color_tertiary = {1.0f, 0.95f, 0.7f, 1.0f};
				b.surface_noise_scale = 20.0f;
				b.surface_roughness = 0.82f;
				b.atmosphere_thickness = 0.0f;
				b.emission_intensity = 2.11f;
				b.rotation_speed_3d = 0.05f;
				b.specular_roughness = 1.0f;
				b.polar_cap_strength = 0.0f;
				b.night_side_light_intensity = 0.0f;
				b.ring_system_enabled = false;
				break;
			case Dynamics::Body3DPreset::TerrestrialPlanet:
				b.atmosphere_mode = Dynamics::Body3DAtmosphereMode::RayleighLimbShell;
				b.surface_texture_mode = Dynamics::Body3DSurfaceTextureMode::ProceduralNoise;
				b.color = {0.10f, 0.35f, 0.65f, 1.0f};
				b.color_secondary = {0.20f, 0.50f, 0.22f, 1.0f};
				b.color_tertiary = {0.95f, 0.95f, 0.98f, 1.0f};
				b.surface_noise_scale = 10.9f;
				b.surface_roughness = 0.05f;
				b.atmosphere_thickness = 0.18f;
				b.emission_intensity = 0.0f;
				b.rotation_speed_3d = 0.12f;
				b.specular_roughness = 0.2f;
				b.polar_cap_strength = 0.45f;
				b.night_side_light_intensity = 0.4f;
				b.ring_system_enabled = false;
				break;
			case Dynamics::Body3DPreset::GasGiant:
				b.atmosphere_mode = Dynamics::Body3DAtmosphereMode::ThickHaze;
				b.surface_texture_mode = Dynamics::Body3DSurfaceTextureMode::RingedGasGiant;
				b.color = {0.82f, 0.65f, 0.45f, 1.0f};
				b.color_secondary = {0.62f, 0.40f, 0.24f, 1.0f};
				b.color_tertiary = {0.58f, 0.29f, 0.0f, 1.0f};
				b.surface_noise_scale = 20.0f;
				b.surface_roughness = 0.3f;
				b.atmosphere_thickness = 0.32f;
				b.emission_intensity = 0.0f;
				b.rotation_speed_3d = 0.35f;
				b.specular_roughness = 0.5f;
				b.polar_cap_strength = 0.2f;
				b.night_side_light_intensity = 0.0f;
				b.ring_system_enabled = true;
				break;
			case Dynamics::Body3DPreset::IceGiant:
				b.atmosphere_mode = Dynamics::Body3DAtmosphereMode::VolumetricMie;
				b.surface_texture_mode = Dynamics::Body3DSurfaceTextureMode::IcyCracked;
				b.color = {0.55f, 0.75f, 0.90f, 1.0f};
				b.color_secondary = {0.35f, 0.55f, 0.80f, 1.0f};
				b.color_tertiary = {0.90f, 0.97f, 1.0f, 1.0f};
				b.surface_noise_scale = 4.0f;
				b.surface_roughness = 1.0f;
				b.atmosphere_thickness = 0.22f;
				b.emission_intensity = 0.0f;
				b.rotation_speed_3d = 0.28f;
				b.specular_roughness = 0.15f;
				b.polar_cap_strength = 0.1f;
				b.night_side_light_intensity = 0.0f;
				b.ring_system_enabled = false;
				break;
			case Dynamics::Body3DPreset::Metallic:
				b.atmosphere_mode = Dynamics::Body3DAtmosphereMode::Off;
				b.surface_texture_mode = Dynamics::Body3DSurfaceTextureMode::CrateredTerrestrial;
				b.color = {0.55f, 0.55f, 0.58f, 1.0f};
				b.color_secondary = {0.32f, 0.32f, 0.35f, 1.0f};
				b.color_tertiary = {0.75f, 0.75f, 0.78f, 1.0f};
				b.surface_noise_scale = 7.0f;
				b.surface_roughness = 0.75f;
				b.atmosphere_thickness = 0.0f;
				b.emission_intensity = 0.0f;
				b.rotation_speed_3d = 0.03f;
				b.specular_roughness = 0.35f;
				b.polar_cap_strength = 0.0f;
				b.night_side_light_intensity = 0.0f;
				b.ring_system_enabled = false;
				break;
			case Dynamics::Body3DPreset::Asteroid:
				b.atmosphere_mode = Dynamics::Body3DAtmosphereMode::Off;
				b.surface_texture_mode = Dynamics::Body3DSurfaceTextureMode::CrateredTerrestrial;
				b.color = {0.45f, 0.42f, 0.38f, 1.0f};
				b.color_secondary = {0.28f, 0.26f, 0.23f, 1.0f};
				b.color_tertiary = {0.60f, 0.56f, 0.50f, 1.0f};
				b.surface_noise_scale = 20.0f;
				b.surface_roughness = 1.0f;
				b.atmosphere_thickness = 0.0f;
				b.emission_intensity = 0.0f;
				b.rotation_speed_3d = 0.4f;
				b.specular_roughness = 0.85f;
				b.polar_cap_strength = 0.0f;
				b.night_side_light_intensity = 0.0f;
				b.ring_system_enabled = false;
				break;
			case Dynamics::Body3DPreset::NeutronStar:
				b.atmosphere_mode = Dynamics::Body3DAtmosphereMode::Off;
				b.surface_texture_mode = Dynamics::Body3DSurfaceTextureMode::StellarGranulation;
				b.color = {0.85f, 0.90f, 1.0f, 1.0f};
				b.color_secondary = {0.55f, 0.72f, 1.0f, 1.0f};
				b.color_tertiary = {1.0f, 1.0f, 1.0f, 1.0f};
				b.surface_noise_scale = 15.0f;
				b.surface_roughness = 0.15f;
				b.atmosphere_thickness = 0.0f;
				b.emission_intensity = 3.0f;
				b.rotation_speed_3d = 1.8f;
				b.specular_roughness = 1.0f;
				b.polar_cap_strength = 0.0f;
				b.night_side_light_intensity = 0.0f;
				b.ring_system_enabled = false;
				break;
			case Dynamics::Body3DPreset::Pulsar:
				b.atmosphere_mode = Dynamics::Body3DAtmosphereMode::Off;
				b.surface_texture_mode = Dynamics::Body3DSurfaceTextureMode::StellarGranulation;
				b.color = {0.65f, 0.80f, 1.0f, 1.0f};
				b.color_secondary = {0.40f, 0.55f, 1.0f, 1.0f};
				b.color_tertiary = {1.0f, 1.0f, 1.0f, 1.0f};
				b.surface_noise_scale = 1.2f;
				b.surface_roughness = 0.1f;
				b.atmosphere_thickness = 0.0f;
				b.emission_intensity = 4.0f;
				b.rotation_speed_3d = 6.0f;
				b.specular_roughness = 1.0f;
				b.polar_cap_strength = 0.0f;
				b.night_side_light_intensity = 0.0f;
				b.ring_system_enabled = false;
				break;
			case Dynamics::Body3DPreset::BlackHole:
				b.atmosphere_mode = Dynamics::Body3DAtmosphereMode::Off;
				b.surface_texture_mode = Dynamics::Body3DSurfaceTextureMode::SolidColor;
				b.color = {0.0f, 0.0f, 0.0f, 1.0f};
				b.color_secondary = {0.03f, 0.03f, 0.04f, 1.0f};
				b.color_tertiary = {0.04f, 0.04f, 0.03f, 1.0f};
				b.surface_noise_scale = 15.0f;
				b.surface_roughness = 0.79f;
				b.atmosphere_thickness = 0.0f;
				b.emission_intensity = 0.0f;
				b.rotation_speed_3d = 0.0f;
				b.specular_roughness = 0.0f;
				b.polar_cap_strength = 0.0f;
				b.night_side_light_intensity = 0.0f;
				b.ring_system_enabled = false;
				break;
			case Dynamics::Body3DPreset::Custom:
			default:
				b.preset_3d = Dynamics::Body3DPreset::Custom;
				return;
		}
		b.preset_3d = preset;
	}

	[[nodiscard]] uint32_t spawn_body(Dynamics::PostNewtonianBody body, const Dynamics::BodySurfaceLayerSet& layers) noexcept {
		auto& sys = orchestrator_.nbody_system();
		body.id = 0;
		body.acceleration = {0.0, 0.0, 0.0};
		body.set_name(unique_name(body.has_name() ? body.name_view() : std::string_view("Body")));
		if (body.is_spacetime_source) {
			body.enforce_spacetime_source_invariants();
			resolve_source_placement(body);
		}
		const uint32_t id = sys.add_body(std::move(body));
		orchestrator_.surface_layers().set(id, layers);
		sys.update_accelerations();
		orchestrator_.notify_state_changed();
		return id;
	}

	void remove_body_by_id(uint32_t id) noexcept {
		auto& sys = orchestrator_.nbody_system();
		orchestrator_.surface_layers().erase(id);
		if (sys.remove_body(id)) {
			sys.update_accelerations();
		}
		if (tracked_body_id_ == static_cast<int>(id)) {
			tracking_enabled_ = false;
			tracked_body_id_ = -1;
		}
		if (selected_body_index_ == static_cast<int>(id)) {
			selected_body_index_ = -1;
		}
		orchestrator_.notify_state_changed();
	}

	[[nodiscard]] static bool edit_double_slider(const char* label, double& value, float min_value, float max_value, const char* format, bool* log_mode = nullptr, float log_min = 0.0f, float log_max = 0.0f) noexcept {
		float scratch = static_cast<float>(value);
		const float effective_log_min = (log_min > 0.0f) ? log_min : min_value;
		const float effective_log_max = (log_max > 0.0f) ? log_max : max_value;
		if (slider_float_with_input(label, &scratch, min_value, max_value, format, log_mode, effective_log_min, effective_log_max)) {
			value = static_cast<double>(scratch);
			return true;
		}
		return false;
	}

	[[nodiscard]] static const char* texture_mode_description(uint32_t mode) noexcept {
		switch (mode) {
			case 0U: return "Procedural noise: oceans below the noise threshold, land blended from primary to secondary color. Uses Noise Scale, Roughness and Texture Detail Scale.";
			case 1U: return "Solid color: flat primary color. Layers, polar caps, city lights and ring band still apply on top.";
			case 2U: return "Palette blend: smooth gradient primary, midpoint, secondary driven by noise. Uses Noise Scale and Texture Detail Scale.";
			case 3U: return "Banded gas giant: latitude bands warped by noise between primary and secondary colors.";
			case 4U: return "Cratered terrain: crater darkening and rims with secondary color speckle.";
			case 5U: return "Stellar granulation: limb-darkened granular surface, strongly driven by Emission Intensity.";
			case 6U: return "Accretion flow: hot streaks along longitude, driven by Emission Intensity and Noise Scale.";
			case 7U: return "Marbled stone: primary/secondary base with tertiary color veins scaled by Texture Detail Scale.";
			case 8U: return "Ringed gas giant: bands, tertiary polar caps and equatorial ring shadow driven by Polar Cap Strength and Ring System.";
			case 9U: return "Icy cracked surface: secondary color cracks with tertiary color shimmer.";
			case 10U: return "Volcanic magma: tertiary color glowing cracks over primary crust with secondary patches.";
			case 11U: return "City lights: primary/secondary land with a speckled night side driven by Night Side City Lights.";
			case 12U: return "Nebulous cloud: three-color wisps mixing primary, secondary and tertiary colors.";
			case 13U: return "Earth photographic map: real day and night imagery with an automatic lit-side mask. Only the texture layers stay editable on top, and the images are loaded only while a body uses this mode.";
			default: return "Unknown texture mode.";
		}
	}

	[[nodiscard]] bool render_identity_section(Dynamics::PostNewtonianBody& b, bool allow_source_toggle) noexcept {
		bool changed = false;
		char name_buffer[32]{};
		const std::string_view current_name = b.name_view();
		std::memcpy(name_buffer, current_name.data(), std::min(current_name.size(), sizeof(name_buffer) - 1));
		if (ImGui::InputTextWithHint("Name", "Unnamed", name_buffer, sizeof(name_buffer))) {
			b.set_name(std::string_view(name_buffer));
			changed = true;
		}
		render_setting_tooltip("Human-readable label shown in the catalog, tags and saved scenarios instead of the numeric identifier.");
		if (ImGui::Checkbox("Enabled", &b.enabled)) changed = true;
		render_setting_tooltip("Disabled bodies remain in the catalog and scenario, but are excluded from rendering, prediction, gravity, integration, and horizon absorption.");
		if (allow_source_toggle) {
			bool is_source = b.is_spacetime_source;
			if (ImGui::Checkbox("Spacetime Source (Independent Black Hole)", &is_source)) {
				b.is_spacetime_source = is_source;
				if (is_source) {
					apply_body_preset_defaults(b, Dynamics::Body3DPreset::BlackHole);
				} else if (b.preset_3d == Dynamics::Body3DPreset::BlackHole) {
					apply_body_preset_defaults(b, Dynamics::Body3DPreset::Metallic);
				}
				b.enforce_spacetime_source_invariants();
				changed = true;
			}
			render_setting_tooltip("Marks this body as its own gravitating compact object with a Kerr event horizon derived from its mass and spin. It participates fully in N-body dynamics, can absorb ordinary bodies crossing its horizon, and merges with other spacetime sources on contact. The rendered lensing still follows the primary central object only.");
		}
		if (b.is_spacetime_source) {
			ImGui::TextDisabled("Dimensionless spin a/M = %.4f | Horizon radius = %.4f", b.kerr_spin_parameter(), b.kerr_outer_horizon_radius());
		}
		return changed;
	}

	[[nodiscard]] bool render_physical_section(Dynamics::PostNewtonianBody& b, BodyEditorViewState& view) noexcept {
		bool changed = false;
		const auto& prefs = orchestrator_.unit_preferences();
		const double mass_scale_kg = mass_scale();
		const double length_scale_m = length_scale();
		const double speed_scale_mps = get_speed_scale(orchestrator_.constants_engine());

		double mass_kg = b.mass * mass_scale_kg;
		if (unit_aware_slider_double("Mass", &mass_kg, 0.001 * mass_scale_kg, 1.0e6 * mass_scale_kg, UnitCategory::Mass, prefs, "%.4f", &view.mass_log_mode, 1e-12 * mass_scale_kg, 1e60 * mass_scale_kg)) {
			b.mass = std::max(0.0, mass_kg / mass_scale_kg);
			changed = true;
		}
		render_setting_tooltip(("Gravitating mass of this body, displayed in " + std::string(Units::mass_unit_suffix(prefs.mass)) + ".").c_str());

		double radius_m = b.radius * length_scale_m;
		if (unit_aware_slider_double("Physical Radius", &radius_m, 0.001 * length_scale_m, 1.0e5 * length_scale_m, UnitCategory::Distance, prefs, "%.4f", &view.radius_log_mode, 1e-6 * length_scale_m, 1e50 * length_scale_m)) {
			b.radius = std::max(1e-6, radius_m / length_scale_m);
			changed = true;
		}
		render_setting_tooltip(("Visual and collision radius, displayed in " + std::string(Units::distance_unit_suffix(prefs.distance)) + ".").c_str());

		double charge = b.charge;
		if (unit_aware_slider_double("Charge", &charge, -10.0, 10.0, UnitCategory::Charge, prefs, "%.4e")) {
			b.charge = charge;
			changed = true;
		}
		render_setting_tooltip(("Net electric charge, displayed in " + std::string(Units::charge_unit_suffix(prefs.charge)) + ".").c_str());

		double position_m[3] = {b.position[0] * length_scale_m, b.position[1] * length_scale_m, b.position[2] * length_scale_m};
		if (unit_aware_input_double3("Position (x, y, z)", position_m, UnitCategory::Distance, prefs)) {
			b.position = {position_m[0] / length_scale_m, position_m[1] / length_scale_m, position_m[2] / length_scale_m};
			changed = true;
		}
		render_setting_tooltip(("Cartesian position relative to the central object, displayed in " + std::string(Units::distance_unit_suffix(prefs.distance)) + ".").c_str());

		double velocity_mps[3] = {b.velocity[0] * speed_scale_mps, b.velocity[1] * speed_scale_mps, b.velocity[2] * speed_scale_mps};
		if (unit_aware_input_double3("Velocity (vx, vy, vz)", velocity_mps, UnitCategory::Velocity, prefs)) {
			b.velocity = {velocity_mps[0] / speed_scale_mps, velocity_mps[1] / speed_scale_mps, velocity_mps[2] / speed_scale_mps};
			changed = true;
		}
		render_setting_tooltip(("Instantaneous coordinate velocity of this body, displayed in " + std::string(Units::velocity_unit_suffix(prefs.velocity)) + ".").c_str());

		if (ImGui::Button("Set Circular Orbit Velocity", ImVec2(-1.0f, 24.0f))) {
			const double central_mass = orchestrator_.parameters().mass + (b.is_spacetime_source ? b.mass : 0.0);
			b.velocity = compute_circular_orbit_velocity(b.position, central_mass);
			changed = true;
		}
		render_setting_tooltip("Overwrites the velocity above with the Keplerian circular-orbit velocity for the current distance from the central mass. Independent black holes include their own mass in the effective central mass.");

		if (ImGui::InputScalarN("Spin Vector", ImGuiDataType_Double, b.spin.data(), 3, nullptr, nullptr, "%.6g")) {
			changed = true;
		}
		render_setting_tooltip("Intrinsic angular momentum vector, feeding spin-orbit and spin-spin post-Newtonian coupling terms and the Kerr parameter of spacetime sources.");

		const double kinetic_energy_joules = b.kinetic_energy() * mass_scale_kg * speed_scale_mps * speed_scale_mps;
		ImGui::TextDisabled("Speed: %s | Kinetic Energy: %s", Units::format_velocity(b.speed() * speed_scale_mps, prefs.velocity).c_str(), Units::format_energy(kinetic_energy_joules, prefs.energy).c_str());
		return changed;
	}

	[[nodiscard]] double primary_horizon_radius() const noexcept {
		const auto& p = orchestrator_.parameters();
		if (p.mass <= 0.0) return 0.0;
		const double a = std::clamp(p.spin, -0.999 * p.mass, 0.999 * p.mass);
		return p.mass + std::sqrt(std::max(p.mass * p.mass - a * a, 0.0));
	}

	[[nodiscard]] std::string source_overlap_warning(const Dynamics::PostNewtonianBody& b) const {
		const double horizon = b.kerr_outer_horizon_radius();
		const double primary_horizon = primary_horizon_radius();
		if (primary_horizon > 0.0 && distance_from_center(b) < primary_horizon + horizon) {
			return "This horizon overlaps the primary source horizon; overlapping sources merge when the simulation runs, and new sources are moved to a safe separation when spawned.";
		}
		auto& sys = orchestrator_.nbody_system();
		{
			std::lock_guard<std::recursive_mutex> lock(sys.bodies_mutex());
			for (const auto& other : sys.bodies()) {
				if (!other.enabled || !other.is_spacetime_source || other.id == b.id) continue;
				const double dx = b.position[0] - other.position[0];
				const double dy = b.position[1] - other.position[1];
				const double dz = b.position[2] - other.position[2];
				if (std::sqrt(dx * dx + dy * dy + dz * dz) < horizon + other.kerr_outer_horizon_radius()) {
					return "This horizon overlaps another independent black hole; overlapping sources merge when the simulation runs, and new sources are moved to a safe separation when spawned.";
				}
			}
		}
		const auto& camera = orchestrator_.camera();
		const double cx = b.position[0] - camera.position[0];
		const double cy = b.position[1] - camera.position[1];
		const double cz = b.position[2] - camera.position[2];
		if (std::sqrt(cx * cx + cy * cy + cz * cz) < horizon) {
			return "The observer is located inside this event horizon.";
		}
		return {};
	}

	void resolve_source_placement(Dynamics::PostNewtonianBody& body) noexcept {
		constexpr double separation_margin = 1.25;
		const double horizon = body.kerr_outer_horizon_radius();
		const double primary_horizon = primary_horizon_radius();
		auto& sys = orchestrator_.nbody_system();
		std::lock_guard<std::recursive_mutex> lock(sys.bodies_mutex());
		const auto separate_from = [&body](const std::array<double, 3>& center, double required_distance) noexcept -> bool {
			const std::array<double, 3> offset{body.position[0] - center[0], body.position[1] - center[1], body.position[2] - center[2]};
			const double distance = std::sqrt(offset[0] * offset[0] + offset[1] * offset[1] + offset[2] * offset[2]);
			if (distance >= required_distance) return false;
			const std::array<double, 3> direction = (distance > 1e-9)
				? std::array<double, 3>{offset[0] / distance, offset[1] / distance, offset[2] / distance}
				: std::array<double, 3>{1.0, 0.0, 0.0};
			body.position = {center[0] + direction[0] * required_distance, center[1] + direction[1] * required_distance, center[2] + direction[2] * required_distance};
			return true;
		};
		for (int pass = 0; pass < 16; ++pass) {
			bool moved = false;
			if (primary_horizon > 0.0) {
				moved = separate_from({0.0, 0.0, 0.0}, (primary_horizon + horizon) * separation_margin);
			}
			for (const auto& other : sys.bodies()) {
				if (!other.enabled || !other.is_spacetime_source || other.id == body.id) continue;
				moved = separate_from(other.position, (other.kerr_outer_horizon_radius() + horizon) * separation_margin) || moved;
			}
			if (!moved) break;
		}
	}

	[[nodiscard]] bool render_spacetime_source_physical_section(Dynamics::PostNewtonianBody& b, BodyEditorViewState& view) noexcept {
		bool changed = false;
		const auto& prefs = orchestrator_.unit_preferences();
		const double mass_scale_kg = mass_scale();
		const double length_scale_m = length_scale();
		const double speed_scale_mps = get_speed_scale(orchestrator_.constants_engine());

		const double previous_spin_parameter = b.kerr_spin_parameter();
		double mass_kg = b.mass * mass_scale_kg;
		if (unit_aware_slider_double("Mass", &mass_kg, 0.001 * mass_scale_kg, 1.0e6 * mass_scale_kg, UnitCategory::Mass, prefs, "%.4f", &view.mass_log_mode, 1e-12 * mass_scale_kg, 1e60 * mass_scale_kg)) {
			const std::array<double, 3> preserved_axis = b.spin_axis_unit();
			b.mass = std::max(1e-9, mass_kg / mass_scale_kg);
			b.set_spin_state(previous_spin_parameter, preserved_axis);
			changed = true;
		}
		render_setting_tooltip(("Gravitating mass of the black hole, displayed in " + std::string(Units::mass_unit_suffix(prefs.mass)) + ". It sets the event horizon; the dimensionless spin is preserved when the mass changes.").c_str());

		float spin_parameter = static_cast<float>(b.kerr_spin_parameter());
		if (slider_float_with_input("Dimensionless Spin (a/M)", &spin_parameter, 0.0f, 0.999f, "%.4f")) {
			b.set_spin_state(static_cast<double>(spin_parameter), b.spin_axis_unit());
			changed = true;
		}
		render_setting_tooltip("Kerr spin parameter a/M in [0, 0.999]. It flattens the horizon, shrinks the ISCO and drives frame dragging and spin-orbit coupling; its direction is set by the spin axis below.");

		const bool spinning = b.kerr_spin_parameter() > 1e-9;
		const std::array<double, 3> axis = b.spin_axis_unit();
		constexpr double radians_to_degrees = 180.0 / std::numbers::pi;
		float polar_degrees = static_cast<float>(std::acos(std::clamp(axis[2], -1.0, 1.0)) * radians_to_degrees);
		float azimuth_degrees = static_cast<float>(std::atan2(axis[1], axis[0]) * radians_to_degrees);
		ImGui::BeginDisabled(!spinning);
		bool orientation_changed = ImGui::SliderFloat("Spin Axis Polar Angle", &polar_degrees, 0.0f, 180.0f, "%.1f deg");
		render_setting_tooltip("Angle between the black hole rotation axis and the world +Z axis. It orients the flattened horizon, the accretion disk plane and the frame dragging direction.");
		orientation_changed = ImGui::SliderFloat("Spin Axis Azimuth", &azimuth_degrees, -180.0f, 180.0f, "%.1f deg") || orientation_changed;
		render_setting_tooltip("Azimuth of the rotation axis around the world Z axis.");
		if (orientation_changed) {
			const double polar = static_cast<double>(polar_degrees) / radians_to_degrees;
			const double azimuth = static_cast<double>(azimuth_degrees) / radians_to_degrees;
			b.set_spin_state(b.kerr_spin_parameter(), {std::sin(polar) * std::cos(azimuth), std::sin(polar) * std::sin(azimuth), std::cos(polar)});
			changed = true;
		}
		if (ImGui::SmallButton("Align With Orbital Angular Momentum")) {
			const std::array<double, 3> angular_momentum{
				b.position[1] * b.velocity[2] - b.position[2] * b.velocity[1],
				b.position[2] * b.velocity[0] - b.position[0] * b.velocity[2],
				b.position[0] * b.velocity[1] - b.position[1] * b.velocity[0]
			};
			if (std::sqrt(angular_momentum[0] * angular_momentum[0] + angular_momentum[1] * angular_momentum[1] + angular_momentum[2] * angular_momentum[2]) > 1e-12) {
				b.set_spin_state(b.kerr_spin_parameter(), angular_momentum);
				changed = true;
			}
		}
		render_setting_tooltip("Points the spin axis along the orbital angular momentum of this black hole around the origin, producing an aligned (prograde) configuration.");
		ImGui::SameLine();
		if (ImGui::SmallButton("Align With Primary Spin")) {
			b.set_spin_state(b.kerr_spin_parameter(), {0.0, 0.0, (orchestrator_.parameters().spin >= 0.0) ? 1.0 : -1.0});
			changed = true;
		}
		render_setting_tooltip("Points the spin axis along the rotation axis of the primary central source.");
		ImGui::SameLine();
		if (ImGui::SmallButton("Reset Axis To +Z")) {
			b.set_spin_state(b.kerr_spin_parameter(), {0.0, 0.0, 1.0});
			changed = true;
		}
		ImGui::EndDisabled();
		if (!spinning) {
			ImGui::TextDisabled("The spin axis only has an effect when the dimensionless spin is non-zero.");
		}

		const double charge_limit = std::max(b.mass, 1e-6);
		double charge = b.charge;
		if (unit_aware_slider_double("Charge", &charge, -charge_limit, charge_limit, UnitCategory::Charge, prefs, "%.4e")) {
			b.charge = charge;
			changed = true;
		}
		render_setting_tooltip(("Net electric charge, displayed in " + std::string(Units::charge_unit_suffix(prefs.charge)) + ", limited to the extremal bound |Q| <= M. It only acts through the electromagnetic interactions between bodies.").c_str());

		double position_m[3] = {b.position[0] * length_scale_m, b.position[1] * length_scale_m, b.position[2] * length_scale_m};
		if (unit_aware_input_double3("Position (x, y, z)", position_m, UnitCategory::Distance, prefs)) {
			b.position = {position_m[0] / length_scale_m, position_m[1] / length_scale_m, position_m[2] / length_scale_m};
			changed = true;
		}
		render_setting_tooltip(("Cartesian position of the black hole relative to the primary source, displayed in " + std::string(Units::distance_unit_suffix(prefs.distance)) + ".").c_str());

		double velocity_mps[3] = {b.velocity[0] * speed_scale_mps, b.velocity[1] * speed_scale_mps, b.velocity[2] * speed_scale_mps};
		if (unit_aware_input_double3("Velocity (vx, vy, vz)", velocity_mps, UnitCategory::Velocity, prefs)) {
			b.velocity = {velocity_mps[0] / speed_scale_mps, velocity_mps[1] / speed_scale_mps, velocity_mps[2] / speed_scale_mps};
			changed = true;
		}
		render_setting_tooltip(("Coordinate velocity of the black hole, displayed in " + std::string(Units::velocity_unit_suffix(prefs.velocity)) + ".").c_str());

		if (ImGui::Button("Set Circular Orbit Velocity", ImVec2(-1.0f, 24.0f))) {
			b.velocity = compute_circular_orbit_velocity(b.position, orchestrator_.parameters().mass + b.mass);
			changed = true;
		}
		render_setting_tooltip("Overwrites the velocity with the Keplerian circular-orbit velocity around the primary source, including the mass of this black hole.");

		if (ImGui::Button("Move To Safe Separation", ImVec2(-1.0f, 24.0f))) {
			resolve_source_placement(b);
			changed = true;
		}
		render_setting_tooltip("Pushes the black hole outward until its horizon no longer overlaps the primary source or any other independent black hole, keeping a 25% margin.");

		const std::string overlap_warning = source_overlap_warning(b);
		if (!overlap_warning.empty()) {
			render_wrapped_colored_text(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), overlap_warning.c_str());
		}

		const double horizon = b.kerr_outer_horizon_radius();
		const double spin_length = b.kerr_spin_parameter() * b.mass;
		const double equatorial_extent = std::sqrt(horizon * horizon + spin_length * spin_length);
		const double isco = Optics::DiskThermalProfile::kerr_isco_radius(b.mass, spin_length);
		ImGui::TextDisabled("Outer Horizon: %s | Equatorial Extent: %s", Units::format_distance(horizon * length_scale_m, prefs.distance).c_str(), Units::format_distance(equatorial_extent * length_scale_m, prefs.distance).c_str());
		ImGui::TextDisabled("Equatorial Ergosphere: %s | ISCO (Prograde): %s", Units::format_distance(2.0 * b.mass * length_scale_m, prefs.distance).c_str(), Units::format_distance(isco * length_scale_m, prefs.distance).c_str());
		const double kinetic_energy_joules = b.kinetic_energy() * mass_scale_kg * speed_scale_mps * speed_scale_mps;
		ImGui::TextDisabled("Speed: %s | Kinetic Energy: %s", Units::format_velocity(b.speed() * speed_scale_mps, prefs.velocity).c_str(), Units::format_energy(kinetic_energy_joules, prefs.energy).c_str());

		if (changed) {
			b.enforce_spacetime_source_invariants();
		}
		return changed;
	}

	[[nodiscard]] bool render_multipole_section(Dynamics::PostNewtonianBody& b, BodyEditorViewState& view) noexcept {
		bool changed = false;
		changed = edit_double_slider("Quadrupole Moment (Q)", b.quadrupole_moment, -1e-2f, 1e-2f, "%.6e") || changed;
		render_setting_tooltip("Quadrupole deformation parameter for tidal and multipolar force models.");
		changed = edit_double_slider("Zonal J2 Moment", b.j2, -1e-2f, 1e-2f, "%.6e") || changed;
		render_setting_tooltip("Dominant oblateness harmonic coefficient, also flattening the rendered spheroid and producing nodal precession on other bodies passing nearby.");
		changed = edit_double_slider("Zonal J3 Moment", b.j3, -1e-3f, 1e-3f, "%.6e") || changed;
		render_setting_tooltip("Third-degree zonal harmonic coefficient, primarily contributing a north-south asymmetric perturbation.");
		changed = edit_double_slider("Zonal J4 Moment", b.j4, -1e-3f, 1e-3f, "%.6e") || changed;
		render_setting_tooltip("Fourth-degree zonal harmonic coefficient, a smaller correction to the oblateness perturbation.");

		const auto& prefs = orchestrator_.unit_preferences();
		const double length_scale_m = length_scale();
		double reference_radius_m = b.reference_radius * length_scale_m;
		if (unit_aware_slider_double("Multipole Reference Radius", &reference_radius_m, 0.001 * length_scale_m, 1.0e5 * length_scale_m, UnitCategory::Distance, prefs, "%.4f", &view.reference_radius_log_mode, 1e-6 * length_scale_m, 1e50 * length_scale_m)) {
			b.reference_radius = std::max(1e-6, reference_radius_m / length_scale_m);
			changed = true;
		}
		render_setting_tooltip(("Reference radius, displayed in " + std::string(Units::distance_unit_suffix(prefs.distance)) + ", at which the zonal harmonic coefficients are defined, typically the body's equatorial radius.").c_str());
		return changed;
	}

	[[nodiscard]] bool render_material_section(Dynamics::PostNewtonianBody& b, BodyEditorViewState& view) noexcept {
		bool changed = false;
		const auto& prefs = orchestrator_.unit_preferences();

		changed = edit_double_slider("Magnetic Moment", b.magnetic_moment, 1e-6f, 1.0e6f, "%.4e", &view.magnetic_moment_log_mode, 1e-9f, 1e9f) || changed;
		render_setting_tooltip("Magnetic dipole moment used by the dipole-dipole force when magnetism is enabled.");

		if (unit_aware_slider_double("Rotation Speed", &b.rotation_speed, 1e-6, 1.0e6, UnitCategory::AngularVelocity, prefs, "%.4e", &view.rotation_speed_log_mode, 1e-9, 1e9)) {
			changed = true;
		}
		render_setting_tooltip("Physical spin rate used by collision friction torque, independent from the visual 3D rotation speed.");

		changed = edit_double_slider("Friction Coefficient", b.friction_coefficient, 0.0f, 1.0f, "%.3f") || changed;
		changed = edit_double_slider("Restitution", b.restitution, 0.0f, 1.0f, "%.3f") || changed;
		changed = edit_double_slider("Elasticity", b.elasticity, 0.0f, 1.0f, "%.3f") || changed;
		changed = edit_double_slider("Integrity", b.integrity, 0.0f, 1.0f, "%.3f") || changed;
		b.integrity = std::max(0.0, b.integrity);
		render_setting_tooltip("Structural integrity reserve. Collision impacts and tidal stress erode it, and the body fragments once it reaches zero.");

		if (unit_aware_slider_double("Lifetime", &b.lifetime, 1e-6, 1.0e9, UnitCategory::Time, prefs, "%.4e", &view.lifetime_log_mode, 1e-6, 1e12)) {
			b.lifetime = std::max(0.0, b.lifetime);
			changed = true;
		}

		ImGui::Separator();
		ImGui::TextDisabled("Thermal Properties");
		if (unit_aware_slider_double("Temperature", &b.temperature, 0.0, 50000.0, UnitCategory::Temperature, prefs, "%.2f")) {
			b.temperature = std::max(0.0, b.temperature);
			changed = true;
		}
		changed = edit_double_slider("Heat Capacity", b.heat_capacity, 1e-6f, 1.0e9f, "%.4e", &view.heat_capacity_log_mode, 1e-6f, 1e12f) || changed;
		b.heat_capacity = std::max(0.0, b.heat_capacity);
		changed = edit_double_slider("Absorption Factor", b.absorption_factor, 0.0f, 1.0f, "%.3f") || changed;
		render_setting_tooltip("Fraction of incident radiative energy absorbed by the body, scaling the Stefan-Boltzmann exchange with the ambient temperature.");
		changed = edit_double_slider("Transmission Factor", b.transmission_factor, 0.0f, 1.0f, "%.3f") || changed;
		if (unit_aware_slider_double("Critical Temperature", &b.critical_temperature, 0.0, 50000.0, UnitCategory::Temperature, prefs, "%.2f")) {
			b.critical_temperature = std::max(0.0, b.critical_temperature);
			changed = true;
		}
		render_setting_tooltip("Temperature above which the hot Young's modulus applies instead of the cold one. Zero disables the transition.");
		changed = edit_double_slider("Cold Resistance", b.cold_resistance, 0.0f, 1.0e6f, "%.4e", &view.resistance_log_mode, 1e-6f, 1e9f) || changed;
		changed = edit_double_slider("Hot Resistance", b.hot_resistance, 0.0f, 1.0e6f, "%.4e", &view.resistance_log_mode, 1e-6f, 1e9f) || changed;

		ImGui::Separator();
		ImGui::TextDisabled("Mechanical Stiffness");
		changed = edit_double_slider("Young's Modulus (Cold)", b.youngs_modulus_cold, 0.0f, 1.0e12f, "%.4e", &view.youngs_modulus_log_mode, 1e-3f, 1e15f) || changed;
		changed = edit_double_slider("Young's Modulus (Hot)", b.youngs_modulus_hot, 0.0f, 1.0e12f, "%.4e", &view.youngs_modulus_log_mode, 1e-3f, 1e15f) || changed;
		render_setting_tooltip("Material stiffness driving the Hertzian contact repulsion when collisions are enabled. A value of zero disables stiffness pushback for the body.");

		char composition[32]{};
		std::memcpy(composition, b.composition.data(), b.composition.size() - 1);
		if (ImGui::InputText("Composition", composition, sizeof(composition))) {
			b.set_composition(composition);
			changed = true;
		}
		render_setting_tooltip("Free-form material tag used by the physical intelligence color coding.");
		return changed;
	}

	[[nodiscard]] bool render_earth_texture_controls(Dynamics::PostNewtonianBody& b) noexcept {
		bool changed = false;

		int variant_idx = std::min(static_cast<int>(b.earth_map_variant), static_cast<int>(kEarthMapVariantNames.size()) - 1);
		if (ImGui::Combo("Earth Map Version", &variant_idx, kEarthMapVariantNames.data(), static_cast<int>(kEarthMapVariantNames.size()))) {
			b.earth_map_variant = Optics::earth_map_variant_from_index(static_cast<uint32_t>(variant_idx));
			changed = true;
		}
		render_setting_tooltip("Day shows the sunlit photographic map, Night shows the city-lights map as self-illumination, and Automatic masks both by the lit side of the body for a continuous blue-marble transition.");

		int quality_idx = std::min(static_cast<int>(b.earth_map_quality), static_cast<int>(kEarthMapQualityNames.size()) - 1);
		if (ImGui::Combo("Earth Map Quality", &quality_idx, kEarthMapQualityNames.data(), static_cast<int>(kEarthMapQualityNames.size()))) {
			b.earth_map_quality = Optics::earth_map_quality_from_index(static_cast<uint32_t>(quality_idx));
			changed = true;
		}
		render_setting_tooltip("Resolution of the Earth images decoded and uploaded to the GPU. When several Earth bodies request different qualities, the highest one is used on the GPU.");

		if (b.earth_map_variant == Optics::EarthMapVariant::Automatic) {
			if (ImGui::SliderFloat("Terminator Blend Softness", &b.earth_terminator_softness, 0.0f, 1.0f, "%.2f")) {
				changed = true;
			}
			render_setting_tooltip("Width of the transition between the day and night images around the terminator. Low values give a sharp line, high values hide the mask with a wide twilight blend.");
		}

		render_wrapped_colored_text(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled), "While the Earth texture is active, colors, noise, roughness, detail, polar caps, city lights and ring shadow are locked. Surface texture layers remain fully editable so clouds and other details can still be added.");
		return changed;
	}

	[[nodiscard]] bool render_base_surface_tab(Dynamics::PostNewtonianBody& b) noexcept {
		bool changed = false;

		int geometry_idx = std::min(static_cast<int>(b.geometry_model), static_cast<int>(kBodyGeometryNames.size()) - 1);
		if (ImGui::Combo("3D Geometry Model", &geometry_idx, kBodyGeometryNames.data(), static_cast<int>(kBodyGeometryNames.size()))) {
			b.geometry_model = static_cast<Dynamics::Body3DGeometryModel>(geometry_idx);
			changed = true;
		}
		render_setting_tooltip("Defines the ray-traced 3D shape. Oblate and prolate spheroids deform from the J2 moment, and the triaxial ellipsoid adds a flattened second axis.");

		const bool earth_texture_active = (b.surface_texture_mode == Dynamics::Body3DSurfaceTextureMode::EarthBlueMarble);
		ImGui::BeginDisabled(earth_texture_active);
		int preset_idx = std::min(static_cast<int>(b.preset_3d), static_cast<int>(kBodySurfacePresetNames.size()) - 1);
		if (ImGui::Combo("Surface Preset", &preset_idx, kBodySurfacePresetNames.data(), static_cast<int>(kBodySurfacePresetNames.size()))) {
			apply_body_preset_defaults(b, static_cast<Dynamics::Body3DPreset>(preset_idx));
			changed = true;
		}
		render_setting_tooltip("Applies a matched base texture, palette, roughness, emission and atmosphere profile for the chosen body family. Surface texture layers are preserved.");
		ImGui::SameLine();
		if (ImGui::SmallButton("Reapply")) {
			apply_body_preset_defaults(b, b.preset_3d);
			changed = true;
		}
		render_setting_tooltip("Restores the base appearance parameters of the current preset without touching texture layers.");
		ImGui::EndDisabled();
		if (earth_texture_active) {
			ImGui::TextDisabled("Surface presets are locked while the Earth texture is active; change the Base Texture Mode to leave it.");
		}

		int texture_idx = std::min(static_cast<int>(b.surface_texture_mode), static_cast<int>(kBodyTextureModeNames.size()) - 1);
		if (ImGui::Combo("Base Texture Mode", &texture_idx, kBodyTextureModeNames.data(), static_cast<int>(kBodyTextureModeNames.size()))) {
			b.surface_texture_mode = static_cast<Dynamics::Body3DSurfaceTextureMode>(texture_idx);
			changed = true;
		}
		render_wrapped_colored_text(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled), texture_mode_description(static_cast<uint32_t>(texture_idx)));

		if (earth_texture_active) {
			return render_earth_texture_controls(b) || changed;
		}

		if (ImGui::ColorEdit4("Primary Color", b.color.data())) changed = true;
		if (ImGui::ColorEdit4("Secondary Color", b.color_secondary.data())) changed = true;
		if (ImGui::ColorEdit4("Tertiary Color", b.color_tertiary.data())) changed = true;
		render_setting_tooltip("Third accent color used by Marbled, Ringed, Icy, Volcanic and Nebulous modes and by polar caps.");
		if (ImGui::SliderFloat("Surface Noise Scale", &b.surface_noise_scale, 0.5f, 20.0f, "%.2f")) changed = true;
		if (ImGui::SliderFloat("Surface Roughness", &b.surface_roughness, 0.05f, 1.0f, "%.2f")) changed = true;
		if (ImGui::SliderFloat("Texture Detail Scale", &b.texture_detail_scale, 0.1f, 5.0f, "%.2fx")) changed = true;
		render_setting_tooltip("Multiplies the frequency of secondary surface patterns such as marble veining or gas giant banding.");
		if (ImGui::SliderFloat("Polar Cap Strength", &b.polar_cap_strength, 0.0f, 1.0f, "%.2f")) changed = true;
		render_setting_tooltip("Blends the tertiary color over the poles to depict ice caps or polar storm bands.");
		if (ImGui::SliderFloat("Night Side City Lights", &b.night_side_light_intensity, 0.0f, 2.0f, "%.2f")) changed = true;
		render_setting_tooltip("Adds a speckled glow on the unlit hemisphere.");
		if (ImGui::Checkbox("Ring System (Equatorial Shadow Band)", &b.ring_system_enabled)) changed = true;
		render_setting_tooltip("Darkens a thin equatorial band to suggest a shadow cast by an orbiting ring plane.");
		return changed;
	}

	[[nodiscard]] bool render_atmosphere_lighting_tab(Dynamics::PostNewtonianBody& b) noexcept {
		bool changed = false;
		int atmosphere_idx = std::min(static_cast<int>(b.atmosphere_mode), static_cast<int>(kBodyAtmosphereModeNames.size()) - 1);
		if (ImGui::Combo("Atmosphere Mode", &atmosphere_idx, kBodyAtmosphereModeNames.data(), static_cast<int>(kBodyAtmosphereModeNames.size()))) {
			b.atmosphere_mode = static_cast<Dynamics::Body3DAtmosphereMode>(atmosphere_idx);
			changed = true;
		}
		render_setting_tooltip("Selects the rim-lit atmospheric glow drawn around the silhouette. Off disables the effect entirely.");
		if (ImGui::SliderFloat("Atmosphere Thickness", &b.atmosphere_thickness, 0.0f, 0.5f, "%.3f")) changed = true;
		if (ImGui::ColorEdit4("Atmosphere Color", b.atmosphere_color.data())) changed = true;
		if (ImGui::SliderFloat("Emission Intensity", &b.emission_intensity, 0.0f, 5.0f, "%.2f")) changed = true;
		render_setting_tooltip("Self-illumination added on top of the lit surface, required for stars and compact remnants.");
		if (ImGui::SliderFloat("Specular Roughness", &b.specular_roughness, 0.05f, 1.0f, "%.2f")) changed = true;
		if (ImGui::SliderFloat("3D Rotation Speed", &b.rotation_speed_3d, 0.0f, 2.0f, "%.3f rad/s")) changed = true;
		render_setting_tooltip("Visual rotation rate of the surface texture and layer drift.");
		return changed;
	}

	[[nodiscard]] bool render_surface_layer_editor(Dynamics::BodySurfaceLayerSet& set, BodyEditorViewState& view) noexcept {
		bool edited = false;
		const bool full = set.count >= Render::kMaxSurfaceLayers;
		ImGui::TextDisabled("Layers: %u / %zu (drawn first to last, on top of the base texture)", set.count, Render::kMaxSurfaceLayers);
		ImGui::SetNextItemWidth(220.0f);
		ImGui::Combo("Template", &view.layer_template_choice, Dynamics::kSurfaceLayerTemplateNames.data(), static_cast<int>(Dynamics::kSurfaceLayerTemplateNames.size()));
		if (full) ImGui::BeginDisabled(true);
		if (ImGui::Button("Add Template Layer", ImVec2(160.0f, 24.0f))) {
			edited = set.add(Dynamics::SurfaceLayerDefinition::from_template(static_cast<Dynamics::SurfaceLayerTemplate>(view.layer_template_choice))) || edited;
		}
		ImGui::SameLine();
		if (ImGui::Button("Add Blank Layer", ImVec2(140.0f, 24.0f))) {
			edited = set.add(Dynamics::SurfaceLayerDefinition{}) || edited;
		}
		if (full) ImGui::EndDisabled();
		ImGui::SameLine();
		if (ImGui::Button("Clear All Layers", ImVec2(130.0f, 24.0f)) && set.count > 0U) {
			set = Dynamics::BodySurfaceLayerSet{};
			edited = true;
		}
		render_setting_tooltip("Adds a procedural texture layer blended over the base surface. Each layer has its own pattern, blend mode, region mask, color, scale, drift and emission.");

		int remove_index = -1;
		int move_index = -1;
		int move_delta = 0;
		for (uint32_t i = 0; i < set.count; ++i) {
			auto& layer = set.layers[i];
			ImGui::PushID(static_cast<int>(i));
			const std::string header = std::to_string(i + 1U) + ". " + Dynamics::kSurfaceLayerPatternNames[static_cast<size_t>(layer.pattern)] + " - " + Dynamics::kSurfaceLayerMaskNames[static_cast<size_t>(layer.mask)] + "###surface_layer_header";
			if (ImGui::CollapsingHeader(header.c_str())) {
				if (ImGui::Checkbox("Enabled", &layer.enabled)) edited = true;
				ImGui::SameLine();
				if (ImGui::SmallButton("Move Up")) { move_index = static_cast<int>(i); move_delta = -1; }
				ImGui::SameLine();
				if (ImGui::SmallButton("Move Down")) { move_index = static_cast<int>(i); move_delta = 1; }
				ImGui::SameLine();
				if (ImGui::SmallButton("Remove")) { remove_index = static_cast<int>(i); }

				int pattern_idx = static_cast<int>(layer.pattern);
				if (ImGui::Combo("Pattern", &pattern_idx, Dynamics::kSurfaceLayerPatternNames.data(), static_cast<int>(Dynamics::kSurfaceLayerPatternNames.size()))) {
					layer.pattern = static_cast<Render::SurfaceLayerPattern>(pattern_idx);
					edited = true;
				}
				int blend_idx = static_cast<int>(layer.blend);
				if (ImGui::Combo("Blend Mode", &blend_idx, Dynamics::kSurfaceLayerBlendNames.data(), static_cast<int>(Dynamics::kSurfaceLayerBlendNames.size()))) {
					layer.blend = static_cast<Render::SurfaceLayerBlend>(blend_idx);
					edited = true;
				}
				int mask_idx = static_cast<int>(layer.mask);
				if (ImGui::Combo("Region Mask", &mask_idx, Dynamics::kSurfaceLayerMaskNames.data(), static_cast<int>(Dynamics::kSurfaceLayerMaskNames.size()))) {
					layer.mask = static_cast<Render::SurfaceLayerMask>(mask_idx);
					edited = true;
				}
				if (ImGui::ColorEdit3("Layer Color", layer.color.data())) edited = true;
				if (ImGui::SliderFloat("Opacity", &layer.opacity, 0.0f, 1.0f, "%.2f")) edited = true;
				if (ImGui::SliderFloat("Pattern Scale", &layer.scale, 0.5f, 30.0f, "%.2f")) edited = true;
				if (ImGui::SliderFloat("Contrast", &layer.contrast, 0.2f, 4.0f, "%.2f")) edited = true;
				if (ImGui::SliderFloat("Threshold", &layer.threshold, 0.0f, 0.95f, "%.2f")) edited = true;
				if (ImGui::SliderFloat("Edge Softness", &layer.softness, 0.01f, 1.0f, "%.2f")) edited = true;
				if (layer.mask != Render::SurfaceLayerMask::Global) {
					if (ImGui::SliderFloat("Mask Width", &layer.mask_width, 0.05f, 1.0f, "%.2f")) edited = true;
				}
				if (ImGui::SliderFloat("Drift Speed", &layer.rotation_factor, -2.0f, 2.0f, "%.2f")) edited = true;
				if (ImGui::SliderFloat("Emission", &layer.emission, 0.0f, 4.0f, "%.2f")) edited = true;
				int octave_value = static_cast<int>(layer.octaves);
				if (ImGui::SliderInt("Detail Octaves", &octave_value, 1, 6)) {
					layer.octaves = static_cast<uint32_t>(octave_value);
					edited = true;
				}
				int seed_value = static_cast<int>(layer.seed);
				if (ImGui::InputInt("Seed", &seed_value)) {
					layer.seed = static_cast<uint32_t>(std::max(seed_value, 0));
					edited = true;
				}
			}
			ImGui::PopID();
		}

		if (move_index >= 0) {
			edited = set.move(static_cast<uint32_t>(move_index), move_delta) || edited;
		}
		if (remove_index >= 0) {
			edited = set.remove(static_cast<uint32_t>(remove_index)) || edited;
		}
		return edited;
	}

	void render_surface_section(Dynamics::PostNewtonianBody& b, Dynamics::BodySurfaceLayerSet& layers, BodyEditorViewState& view, BodyEditResult& result) noexcept {
		const size_t texture_idx = std::min(static_cast<size_t>(b.surface_texture_mode), kBodyTextureModeNames.size() - 1);
		ImGui::TextDisabled("Composition: %s, then %u layer(s), atmosphere and relativistic effects", kBodyTextureModeNames[texture_idx], layers.count);
		if (ImGui::BeginTabBar("SurfaceEditorTabs")) {
			if (ImGui::BeginTabItem("Base Surface")) {
				result.body_changed = render_base_surface_tab(b) || result.body_changed;
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Atmosphere & Lighting")) {
				result.body_changed = render_atmosphere_lighting_tab(b) || result.body_changed;
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Texture Layers")) {
				result.layers_changed = render_surface_layer_editor(layers, view) || result.layers_changed;
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
	}

	[[nodiscard]] BodyEditResult render_body_editor(Dynamics::PostNewtonianBody& b, Dynamics::BodySurfaceLayerSet* layers, BodyEditorViewState& view, uint32_t sections) noexcept {
		BodyEditResult result;
		if (b.is_spacetime_source) {
			sections &= ~(BodyEditorSection::Multipoles | BodyEditorSection::Material | BodyEditorSection::Surface);
		} else {
			sections &= ~BodyEditorSection::AccretionDisk;
		}
		if ((sections & BodyEditorSection::Identity) != 0U && ImGui::CollapsingHeader("Identity & State", ImGuiTreeNodeFlags_DefaultOpen)) {
			result.body_changed = render_identity_section(b, (sections & BodyEditorSection::SourceToggle) != 0U) || result.body_changed;
		}
		if ((sections & BodyEditorSection::Physical) != 0U && ImGui::CollapsingHeader("Physical State & Motion", ImGuiTreeNodeFlags_DefaultOpen)) {
			result.body_changed = (b.is_spacetime_source ? render_spacetime_source_physical_section(b, view) : render_physical_section(b, view)) || result.body_changed;
		}
		if ((sections & BodyEditorSection::AccretionDisk) != 0U && ImGui::CollapsingHeader("Accretion Disk Appearance")) {
			result.body_changed = render_accretion_disk_editor(b.accretion_disk, true) || result.body_changed;
		}
		if ((sections & BodyEditorSection::Multipoles) != 0U && ImGui::CollapsingHeader("Gravitational Multipoles")) {
			result.body_changed = render_multipole_section(b, view) || result.body_changed;
		}
		if ((sections & BodyEditorSection::Material) != 0U && ImGui::CollapsingHeader("Material, Thermal & Electromagnetic")) {
			result.body_changed = render_material_section(b, view) || result.body_changed;
		}
		if ((sections & BodyEditorSection::Surface) != 0U && layers != nullptr && ImGui::CollapsingHeader("Surface Appearance & Layers")) {
			render_surface_section(b, *layers, view, result);
		}
		return result;
	}

	void render_body_list_tab() noexcept {
		auto& sys = orchestrator_.nbody_system();
		std::lock_guard<std::recursive_mutex> body_list_lock(sys.bodies_mutex());
		auto bodies = sys.bodies();
		const size_t n = bodies.size();

		const float total_width = ImGui::GetContentRegionAvail().x;
		list_pane_width_ = std::clamp(list_pane_width_, 190.0f, std::max(210.0f, total_width - 220.0f));

		ImGui::BeginChild("BodyCatalogListPane", ImVec2(list_pane_width_, ImGui::GetContentRegionAvail().y), true);

		ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "Spacetime Source");
		{
			const bool is_selected_central = (selected_body_index_ == kCentralObjectIndex);
			const auto& params = orchestrator_.parameters();
			const std::string central_label = "Central Object (M=" + std::to_string(params.mass).substr(0, 5) + ", a=" + std::to_string(params.spin).substr(0, 5) + ")";
			if (ImGui::Selectable(central_label.c_str(), is_selected_central)) {
				selected_body_index_ = kCentralObjectIndex;
			}
			render_setting_tooltip("The metric-generating central mass. Select this to edit mass, spin, and charge parameters that shape the background spacetime.");
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "N-Body Catalog");

		ImGui::SetNextItemWidth(-1.0f);
		ImGui::InputTextWithHint("##BodySearch", "Filter by name or ID...", search_filter_, sizeof(search_filter_));
		render_setting_tooltip("Narrows the catalog below to bodies whose name or numeric identifier contains this text.");

		const char* sort_options[] = {"Creation Order", "Name", "Mass", "Distance From Center", "Speed"};
		ImGui::SetNextItemWidth(-30.0f);
		ImGui::Combo("##BodySortMode", &sort_mode_, sort_options, IM_ARRAYSIZE(sort_options));
		ImGui::SameLine();
		if (ImGui::ArrowButton("##BodySortDirection", sort_descending_ ? ImGuiDir_Down : ImGuiDir_Up)) {
			sort_descending_ = !sort_descending_;
		}
		render_setting_tooltip("Chooses how the catalog list below is ordered, and toggles between ascending and descending order.");

		std::vector<size_t> visible_indices;
		visible_indices.reserve(n);
		const std::string_view filter_view(search_filter_);
		for (size_t i = 0; i < n; ++i) {
			if (!filter_view.empty()) {
				const std::string name = display_name(bodies[i]);
				const std::string id_str = std::to_string(bodies[i].id);
				if (name.find(filter_view) == std::string::npos && id_str.find(filter_view) == std::string::npos) {
					continue;
				}
			}
			visible_indices.push_back(i);
		}

		std::sort(visible_indices.begin(), visible_indices.end(), [&](size_t a, size_t b) noexcept {
			bool less;
			switch (static_cast<BodyCatalogSortMode>(sort_mode_)) {
				case BodyCatalogSortMode::Name:
					less = display_name(bodies[a]) < display_name(bodies[b]);
					break;
				case BodyCatalogSortMode::Mass:
					less = bodies[a].mass < bodies[b].mass;
					break;
				case BodyCatalogSortMode::Distance:
					less = distance_from_center(bodies[a]) < distance_from_center(bodies[b]);
					break;
				case BodyCatalogSortMode::Speed:
					less = bodies[a].speed() < bodies[b].speed();
					break;
				case BodyCatalogSortMode::CreationOrder:
				default:
					less = a < b;
					break;
			}
			return sort_descending_ ? !less : less;
		});

		ImGui::Spacing();

		if (n == 0) {
			ImGui::TextDisabled("No orbiting bodies populated.");
			ImGui::Spacing();
			if (ImGui::Button("Add Body", ImVec2(-1.0f, 28.0f))) {
				request_focus_creation_tab_ = true;
			}
			render_setting_tooltip("Switches to the Create Body tab so you can configure and spawn a new orbiting body.");
			ImGui::Spacing();
			if (ImGui::Button("Spawn Solar System Archetype", ImVec2(-1.0f, 26.0f))) {
				populate_solar_system_archetype();
			}
			render_setting_tooltip("Populates the system with a simplified two-planet configuration to quickly exercise N-body dynamics.");
		} else if (visible_indices.empty()) {
			ImGui::TextDisabled("No bodies match the current filter.");
		} else {
			for (const size_t i : visible_indices) {
				const bool is_selected = (selected_body_index_ == static_cast<int>(bodies[i].id));
				const double dist = distance_from_center(bodies[i]);
				const std::string label = display_name(bodies[i]) + "  (M=" + std::to_string(bodies[i].mass).substr(0, 4) + ", r=" + std::to_string(dist).substr(0, 5) + ")";
				if (ImGui::Selectable(label.c_str(), is_selected)) {
					selected_body_index_ = static_cast<int>(bodies[i].id);
				}
				if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
					ImGui::BeginTooltip();
					ImGui::Text("Identifier: #%u", bodies[i].id);
					ImGui::Text("Mass: %.6e", bodies[i].mass);
					ImGui::Text("Distance from center: %.6f", dist);
					ImGui::Text("Speed: %.6e", bodies[i].speed());
					ImGui::EndTooltip();
				}
			}
		}
		ImGui::EndChild();

		ImGui::SameLine();
		ImGui::InvisibleButton("BodyCatalogSplitter", ImVec2(6.0f, ImGui::GetContentRegionAvail().y));
		if (ImGui::IsItemActive()) {
			list_pane_width_ += ImGui::GetIO().MouseDelta.x;
		}
		if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
			ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
		}
		ImGui::SameLine();

		ImGui::BeginChild("BodyCatalogDetailPane", ImVec2(0.0f, ImGui::GetContentRegionAvail().y), true);
		if (selected_body_index_ == kCentralObjectIndex) {
			render_central_object_panel();
		} else {
			size_t resolved_index = n;
			for (size_t k = 0; k < n; ++k) {
				if (static_cast<int>(bodies[k].id) == selected_body_index_) {
					resolved_index = k;
					break;
				}
			}
			if (resolved_index < n) {
				render_selected_nbody_panel(sys, bodies, resolved_index);
			} else {
				ImGui::TextDisabled("Select a body from the catalog to inspect or edit its parameters.");
			}
		}
		ImGui::EndChild();
	}

	void render_central_object_panel() noexcept {
		auto& params = orchestrator_.parameters();

		ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Central Spacetime Source");
		ImGui::TextDisabled("Metric: %s", orchestrator_.active_metric_name().c_str());
		ImGui::Separator();

		{
			const double central_mass_scale_kg = mass_scale();
			double central_mass_kg = static_cast<double>(params.mass) * central_mass_scale_kg;
			if (unit_aware_slider_double("Central Mass (M)", &central_mass_kg, 0.001 * central_mass_scale_kg, 1.0e6 * central_mass_scale_kg, UnitCategory::Mass, orchestrator_.unit_preferences(), "%.4f", &central_mass_log_mode_, 1e-12 * central_mass_scale_kg, 1e60 * central_mass_scale_kg)) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::Mass, std::max(0.01, central_mass_kg / central_mass_scale_kg))));
			}
		}
		render_setting_tooltip(("Central gravitating mass, displayed in " + std::string(Units::mass_unit_suffix(orchestrator_.unit_preferences().mass)) + ". Governs the Schwarzschild radius rs = 2M and the overall curvature strength.").c_str());

		float spin = static_cast<float>(params.spin);
		const float spin_bound = static_cast<float>(0.999 * params.mass);
		if (slider_float_with_input("Spin Parameter (a)", &spin, -spin_bound, spin_bound, "%.4f")) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::Spin, std::clamp(static_cast<double>(spin), -0.999 * params.mass, 0.999 * params.mass))));
		}
		render_setting_tooltip("Specific angular momentum a = J / M, clamped to the subextremal range. Only meaningful for Kerr-family metrics.");

		{
			double charge_disp = static_cast<double>(params.charge);
			if (unit_aware_slider_double("Electric Charge (Q)", &charge_disp, -10.0, 10.0, UnitCategory::Charge, orchestrator_.unit_preferences(), "%.4f")) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::Charge, charge_disp)));
			}
		}
		render_setting_tooltip(("Net electrostatic charge, displayed in " + std::string(Units::charge_unit_suffix(orchestrator_.unit_preferences().charge)) + ". Only meaningful for Reissner-Nordstrom and Kerr-Newman metrics.").c_str());

		ImGui::Spacing();
		if (ImGui::Button("Look At Central Object", ImVec2(-1.0f, 26.0f))) {
			look_at({0.0, 0.0, 0.0});
		}
		render_setting_tooltip("Orients the camera to face the central compact object without changing its position.");
		bool tracking_central = tracking_enabled_ && tracked_body_id_ == kCentralObjectIndex;
		if (ImGui::Checkbox("Track Central Object", &tracking_central)) {
			tracking_enabled_ = tracking_central;
			tracked_body_id_ = tracking_central ? kCentralObjectIndex : -1;
		}

		ImGui::Separator();
		if (ImGui::CollapsingHeader("Accretion Disk Appearance", ImGuiTreeNodeFlags_DefaultOpen)) {
			render_primary_accretion_disk_editor(orchestrator_);
		}
	}

	void render_selected_nbody_panel(Dynamics::PostNewtonianSystem& sys, std::span<Dynamics::PostNewtonianBody> bodies, size_t index) noexcept {
		auto& b = bodies[index];
		const uint32_t body_id = b.id;
		bool duplicate_requested = false;
		bool remove_requested = false;

		ImGui::PushID(static_cast<int>(body_id));
		ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%s", display_name(b).c_str());
		ImGui::TextDisabled("Identifier: #%u", body_id);

		if (ImGui::Button("Look At", ImVec2(70.0f, 24.0f))) {
			look_at(b.position);
		}
		render_setting_tooltip("Rotates the camera to face this body without moving the camera position.");
		ImGui::SameLine();
		bool tracking_this = tracking_enabled_ && tracked_body_id_ == static_cast<int>(body_id);
		if (ImGui::Checkbox("Track", &tracking_this)) {
			tracking_enabled_ = tracking_this;
			tracked_body_id_ = tracking_this ? static_cast<int>(body_id) : -1;
		}
		ImGui::SameLine();
		if (ImGui::Button("Duplicate", ImVec2(80.0f, 24.0f))) {
			duplicate_requested = true;
		}
		render_setting_tooltip("Creates a copy of this body offset along X, keeping every physical parameter, the appearance and all texture layers.");
		ImGui::SameLine();
		if (ImGui::Button("Delete", ImVec2(70.0f, 24.0f))) {
			remove_requested = true;
		}
		render_setting_tooltip("Permanently removes this body from the N-body system.");
		if (b.enabled && !b.is_spacetime_source) {
			ImGui::SameLine();
			if (ImGui::Button("Walk On Surface", ImVec2(120.0f, 24.0f))) {
				orchestrator_.parameters().surface_walk_body_id = body_id;
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_camera_mode(4U)));
			}
			render_setting_tooltip("Places the camera on the surface of this body and switches to Surface Walk navigation.");
		}
		ImGui::Separator();

		Dynamics::BodySurfaceLayerSet layers = orchestrator_.surface_layers().get(body_id);
		const BodyEditResult result = render_body_editor(b, &layers, selected_view_, BodyEditorSection::Complete);
		if (result.layers_changed) {
			orchestrator_.surface_layers().set(body_id, layers);
		}
		if (result.body_changed || result.layers_changed) {
			sys.update_accelerations();
			orchestrator_.notify_state_changed();
		}

		if (duplicate_requested) {
			Dynamics::PostNewtonianBody clone = b;
			clone.position[0] += std::max(clone.radius, 1e-6) * 4.0;
			selected_body_index_ = static_cast<int>(spawn_body(clone, layers));
		} else if (remove_requested) {
			remove_body_by_id(body_id);
		}
		ImGui::PopID();
	}

	void render_creation_tab() noexcept {
		if (creation_draft_pristine_ && std::abs(orchestrator_.parameters().mass - creation_reference_mass_) > 1e-9 * std::max(std::abs(creation_reference_mass_), 1.0)) {
			randomize_creation_defaults();
		}
		ImGui::TextWrapped("Every property of a body is configured here with the same editor used for existing bodies, then spawned into the running system.");

		int template_idx = creation_preset_;
		if (ImGui::Combo("Template Preset", &template_idx, kBodyTemplateNames.data(), static_cast<int>(kBodyTemplateNames.size()))) {
			creation_preset_ = template_idx;
			apply_template_preset(static_cast<BodyPresetTemplate>(creation_preset_));
		}
		render_setting_tooltip("Fills the draft with realistic physical values, appearance and texture layers for a known body type. Real-world values are converted to the active simulation units. Position and velocity are kept.");

		if (ImGui::Button("Randomize Intelligent Defaults", ImVec2(-1.0f, 24.0f))) {
			randomize_creation_defaults();
		}
		render_setting_tooltip("Generates a coherent body profile by sampling correlated mass, radius, orbit, spin, multipoles, appearance and name.");

		{
			auto& sys = orchestrator_.nbody_system();
			std::lock_guard<std::recursive_mutex> lock(sys.bodies_mutex());
			const Dynamics::PostNewtonianBody* selected = nullptr;
			for (const auto& body : sys.bodies()) {
				if (static_cast<int>(body.id) == selected_body_index_) {
					selected = &body;
					break;
				}
			}
			ImGui::BeginDisabled(selected == nullptr);
			if (ImGui::Button("Copy Settings From Selected Body", ImVec2(-1.0f, 24.0f)) && selected != nullptr) {
				creation_layers_ = orchestrator_.surface_layers().get(selected->id);
				creation_draft_ = *selected;
				creation_draft_.id = 0;
				creation_draft_.acceleration = {0.0, 0.0, 0.0};
				creation_draft_.set_name(unique_name(display_name(*selected)));
				creation_preset_ = static_cast<int>(BodyPresetTemplate::Custom);
				creation_draft_pristine_ = false;
			}
			ImGui::EndDisabled();
			render_setting_tooltip("Loads every property, the appearance and all texture layers of the body selected in the catalog into the draft.");
		}

		ImGui::Separator();
		ImGui::PushID("BodyCreationEditor");
		const BodyEditResult creation_edit = render_body_editor(creation_draft_, &creation_layers_, creation_view_, BodyEditorSection::Complete);
		if (creation_edit.body_changed || creation_edit.layers_changed) {
			creation_draft_pristine_ = false;
		}
		ImGui::PopID();

		ImGui::Separator();
		ImGui::Checkbox("Randomize Draft After Spawn", &randomize_after_spawn_);
		render_setting_tooltip("When enabled, a new random draft is generated after each spawn. When disabled, the draft is kept so several similar bodies can be spawned in a row.");
		if (ImGui::Button("Spawn and Inject into System", ImVec2(-1.0f, 32.0f))) {
			const uint32_t new_id = spawn_body(creation_draft_, creation_layers_);
			selected_body_index_ = static_cast<int>(new_id);
			if (randomize_after_spawn_) {
				randomize_creation_defaults();
			} else {
				creation_draft_.set_name(unique_name(display_name(creation_draft_)));
			}
		}
		render_setting_tooltip("Adds the configured body, with its full appearance and texture layers, to the running N-body system and selects it in the catalog.");
	}

	void render_spacetime_sources_tab() noexcept {
		auto& sys = orchestrator_.nbody_system();
		std::lock_guard<std::recursive_mutex> spacetime_sources_lock(sys.bodies_mutex());

		ImGui::TextColored(ImVec4(0.95f, 0.5f, 0.5f, 1.0f), "Primary Metric Source (Fixed At Origin)");
		ImGui::TextWrapped("The primary spacetime source that curves the rendered background metric and lensing is always fixed at the coordinate origin. Independent black holes below are fully N-body integrated and can move, orbit, drift, and merge.");
		auto& params = orchestrator_.parameters();
		float primary_mass = static_cast<float>(params.mass);
		if (slider_float_with_input("Primary Mass (M)", &primary_mass, 0.01f, 1.0e6f, "%.4e")) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::Mass, static_cast<double>(primary_mass))));
		}
		float primary_spin = static_cast<float>(params.spin);
		const float primary_spin_bound = static_cast<float>(0.999 * params.mass);
		if (slider_float_with_input("Primary Spin (a)", &primary_spin, -primary_spin_bound, primary_spin_bound, "%.4f")) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::Spin, std::clamp(static_cast<double>(primary_spin), -0.999 * params.mass, 0.999 * params.mass))));
		}

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.95f, 0.5f, 0.5f, 1.0f), "Independent Black Holes (N-Body Integrated)");
		ImGui::TextWrapped("Each entry below is a fully gravitating Kerr spacetime source participating in N-body dynamics: it attracts and is attracted by every other body, can orbit or drift freely, merges with other black holes on contact, and absorbs ordinary bodies crossing its horizon.");

		bool any_source = false;
		bool delete_requested = false;
		uint32_t delete_id = 0;
		for (auto& body : sys.bodies()) {
			if (!body.is_spacetime_source) continue;
			any_source = true;
			ImGui::PushID(static_cast<int>(body.id) + 500000);
			const std::string header = display_name(body) + " (#" + std::to_string(body.id) + ")###source_header";
			if (ImGui::CollapsingHeader(header.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
				bool changed = render_body_editor(body, nullptr, source_view_, BodyEditorSection::SpacetimeSource).body_changed;
				if (ImGui::Button("Look At")) {
					look_at(body.position);
				}
				ImGui::SameLine();
				bool tracking_this = tracking_enabled_ && tracked_body_id_ == static_cast<int>(body.id);
				if (ImGui::Checkbox("Track", &tracking_this)) {
					tracking_enabled_ = tracking_this;
					tracked_body_id_ = tracking_this ? static_cast<int>(body.id) : -1;
				}
				ImGui::SameLine();
				if (ImGui::Button("Revert To Ordinary Body")) {
					body.is_spacetime_source = false;
					apply_body_preset_defaults(body, Dynamics::Body3DPreset::Metallic);
					changed = true;
				}
				render_setting_tooltip("Removes this body's gravitating-source status; it becomes an ordinary body without its own event horizon, no longer able to absorb or merge with other bodies.");
				ImGui::SameLine();
				if (ImGui::Button("Delete This Black Hole")) {
					delete_requested = true;
					delete_id = body.id;
				}
				if (changed) {
					sys.update_accelerations();
					orchestrator_.notify_state_changed();
				}
			}
			ImGui::PopID();
		}
		if (!any_source) {
			ImGui::TextDisabled("No independent black holes yet. Create one below.");
		}
		if (delete_requested) {
			remove_body_by_id(delete_id);
		}

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.6f, 1.0f), "Create New Black Hole");
		if (black_hole_draft_pristine_ && std::abs(orchestrator_.parameters().mass - black_hole_reference_mass_) > 1e-9 * std::max(std::abs(black_hole_reference_mass_), 1.0)) {
			reset_black_hole_draft();
		}
		ImGui::PushID("BlackHoleCreationEditor");
		const BodyEditResult black_hole_edit = render_body_editor(black_hole_draft_, nullptr, source_view_, BodyEditorSection::SpacetimeSource);
		if (black_hole_edit.body_changed) {
			black_hole_draft_pristine_ = false;
		}
		ImGui::PopID();
		if (ImGui::Button("Spawn Black Hole", ImVec2(-1.0f, 30.0f))) {
			black_hole_draft_.is_spacetime_source = true;
			static_cast<void>(spawn_body(black_hole_draft_, Dynamics::BodySurfaceLayerSet{}));
			selected_body_index_ = -1;
			reset_black_hole_draft();
		}
		render_setting_tooltip("Adds a fully N-body integrated independent black hole with its own Kerr event horizon to the running simulation, using the mass, spin, charge, position and velocity configured above.");

		ImGui::Spacing();
		if (ImGui::Button("Spawn Companion In Wide Circular Orbit (Randomized)", ImVec2(-1.0f, 26.0f))) {
			spawn_orbiting_spacetime_source();
		}
		render_setting_tooltip("Creates a new independent black hole roughly half the mass of the primary central object on a wide, randomly inclined circular orbit.");
	}

	void render_system_dynamics_tab() noexcept {
		auto& sys = orchestrator_.nbody_system();
		std::lock_guard<std::recursive_mutex> system_dynamics_lock(sys.bodies_mutex());
		const size_t n = sys.body_count();

		if (n == 0) {
			ImGui::TextDisabled("System is empty.");
			return;
		}

		if (ImGui::Button("Recompute System State", ImVec2(-1.0f, 26.0f))) {
			sys.update_accelerations();
			orchestrator_.notify_state_changed();
		}
		render_setting_tooltip("Forces an immediate recomputation of accelerations and gravitational-wave emission from the current body states.");
		if (ImGui::CollapsingHeader("Global Body Actions", ImGuiTreeNodeFlags_DefaultOpen)) {
			static_cast<void>(unit_aware_input_float3("Velocity For All", global_velocity_, UnitCategory::Velocity, orchestrator_.unit_preferences()));
			if (ImGui::Button("Set Velocity For All")) {
				for (auto& body : sys.bodies()) if (body.enabled) body.velocity = std::array<double, 3>{global_velocity_[0], global_velocity_[1], global_velocity_[2]};
				sys.update_accelerations();
			}
			ImGui::SameLine();
			if (ImGui::Button("Invert All Velocities")) {
				for (auto& body : sys.bodies()) if (body.enabled) for (double& component : body.velocity) component = -component;
				sys.update_accelerations();
			}
			ImGui::InputFloat3("Spin For All", global_spin_);
			if (ImGui::Button("Set Spin For All")) {
				for (auto& body : sys.bodies()) if (body.enabled) body.spin = std::array<double, 3>{global_spin_[0], global_spin_[1], global_spin_[2]};
				sys.update_accelerations();
			}
			ImGui::InputFloat("Grid Precision", &grid_spacing_, 0.1f, 1.0f, "%.4g");
			if (ImGui::Button("Snap Positions To Grid")) {
				Dynamics::BulkBodyActions::snap_to_grid(sys, static_cast<double>(grid_spacing_));
			}
			ImGui::SameLine();
			if (ImGui::Button("Scatter Positions")) {
				Dynamics::BulkBodyActions::scatter_positions(sys);
			}
			if (ImGui::Button("Equalize Mass For All")) {
				Dynamics::BulkBodyActions::equalize_masses(sys);
			}
			ImGui::SameLine();
			if (ImGui::Button("Zero All Spins")) {
				Dynamics::BulkBodyActions::zero_all_spins(sys);
			}
			if (ImGui::Button("Cull Bodies Outside Render Distance")) {
				const auto& p = orchestrator_.parameters();
				const double limit = (p.render_distance_scale > 0.0) ? (p.render_distance_scale * std::max(p.mass, 1e-6)) : 1.0e7;
				Dynamics::BulkBodyActions::cull_outside_radius(sys, limit);
			}
			render_setting_tooltip("Removes every body whose distance from the origin exceeds the current render distance, approximating a view-frustum cull.");
			ImGui::SameLine();
			if (ImGui::Button("Cull Bodies Outside Current View")) {
				const auto& cam = orchestrator_.camera();
				const double pitch_rad = cam.pitch * (std::numbers::pi / 180.0);
				const double yaw_rad = cam.yaw * (std::numbers::pi / 180.0);
				const std::array<double, 3> forward{std::cos(pitch_rad) * std::cos(yaw_rad), std::cos(pitch_rad) * std::sin(yaw_rad), std::sin(pitch_rad)};
				const double half_fov_rad = std::clamp(cam.fov_deg, 5.0, 175.0) * (std::numbers::pi / 360.0);
				const auto& p = orchestrator_.parameters();
				const double max_distance = (p.render_distance_scale > 0.0) ? (p.render_distance_scale * std::max(p.mass, 1e-6)) : 1.0e7;
				Dynamics::BulkBodyActions::cull_outside_camera_frustum(sys, cam.position, forward, half_fov_rad, max_distance);
			}
			render_setting_tooltip("Removes every body that falls outside the primary camera's current field-of-view cone, using its live position, orientation, and FOV.");

			ImGui::Spacing();
			ImGui::TextColored(ImVec4(0.6f, 0.85f, 1.0f, 1.0f), "Generalized Parameter Actions");
			const char* bulk_param_names[] = {
				"Mass", "Radius", "Charge", "Magnetic Moment", "Rotation Speed", "Friction Coefficient",
				"Restitution", "Integrity", "Lifetime", "Temperature", "Heat Capacity",
				"Quadrupole Moment", "Zonal J2", "Zonal J3", "Zonal J4", "Multipole Reference Radius"
			};
			ImGui::SetNextItemWidth(240.0f);
			ImGui::Combo("Target Parameter", &bulk_parameter_index_, bulk_param_names, IM_ARRAYSIZE(bulk_param_names));
			const auto bulk_param = static_cast<Dynamics::BulkScalarParameter>(bulk_parameter_index_);
			slider_float_with_input("Value", &bulk_parameter_value_, -1.0e6f, 1.0e6f, "%.4e", &bulk_parameter_value_log_mode_, 1e-9f, 1e12f);
			if (ImGui::Button("Set For All Bodies")) {
				Dynamics::BulkBodyActions::set_parameter_for_all(sys, bulk_param, static_cast<double>(bulk_parameter_value_));
			}
			ImGui::SameLine();
			if (ImGui::Button("Equalize (Set To Average)")) {
				Dynamics::BulkBodyActions::equalize_parameter(sys, bulk_param);
			}
			ImGui::SameLine();
			if (ImGui::Button("Average (Same As Equalize)")) {
				Dynamics::BulkBodyActions::average_parameter(sys, bulk_param);
			}
			render_setting_tooltip("Applies to the selected physical parameter across every enabled body: sets an explicit value, or replaces every value with the current mean.");
		}

		ImGui::Separator();
		if (ImGui::CollapsingHeader("Spacetime Sources (Black Holes)", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::TextColored(ImVec4(0.95f, 0.5f, 0.5f, 1.0f), "Primary Metric Source");
			ImGui::Text("Mass=%.6e  Spin a=%.4f  Position=(0,0,0)", orchestrator_.parameters().mass, orchestrator_.parameters().spin);
			render_setting_tooltip("The single spacetime source that curves the rendered background metric and lensing. It is always fixed at the coordinate origin; use Spacetime & Metrics to change its mass and spin.");

			ImGui::Spacing();
			ImGui::TextColored(ImVec4(0.95f, 0.5f, 0.5f, 1.0f), "Independent N-Body Spacetime Sources");
			bool any_source = false;
			for (auto& body : sys.bodies()) {
				if (!body.is_spacetime_source) continue;
				any_source = true;
				ImGui::PushID(static_cast<int>(body.id));
				const double dist = distance_from_center(body);
				const std::string label = display_name(body) + " (M=" + std::to_string(body.mass).substr(0, 5) + ", a/M=" + std::to_string(body.kerr_spin_parameter()).substr(0, 5) + ", r_h=" + std::to_string(body.kerr_outer_horizon_radius()).substr(0, 5) + ", dist=" + std::to_string(dist).substr(0, 6) + ")";
				if (ImGui::Selectable(label.c_str(), false)) {
					selected_body_index_ = static_cast<int>(body.id);
				}
				ImGui::SameLine();
				if (ImGui::SmallButton("Look At")) {
					look_at(body.position);
				}
				ImGui::PopID();
			}
			if (!any_source) {
				ImGui::TextDisabled("No independent spacetime sources yet. Create one from the Spacetime Sources tab or enable the Spacetime Source option on any body.");
			}
			render_setting_tooltip("Bodies flagged as spacetime sources are fully integrated N-body gravitating objects with their own Kerr event horizon. They attract and are attracted by every other body and each other, merge with each other on contact, and absorb ordinary bodies crossing their horizon, letting several black holes coexist and orbit within the same simulation.");

			ImGui::Spacing();
			if (ImGui::Button("Spawn A Second Black Hole In Orbit", ImVec2(-1.0f, 28.0f))) {
				spawn_orbiting_spacetime_source();
			}
			render_setting_tooltip("Creates a new independent black hole roughly half the mass of the primary central object, placed on a wide circular orbit with a small random inclination, ready for binary or multi-black-hole dynamics.");
		}
		ImGui::Separator();

		ImGui::Text("Total System Mass:     %.6e", sys.total_mass());
		const auto cm = sys.center_of_mass();
		ImGui::Text("Center of Mass (x,y,z): (%.2e, %.2e, %.2e)", cm[0], cm[1], cm[2]);
		render_setting_tooltip("Mass-weighted average position of all orbiting bodies.");

		const auto p_tot = sys.total_linear_momentum();
		ImGui::Text("Total Linear Momentum: (%.2e, %.2e, %.2e)", p_tot[0], p_tot[1], p_tot[2]);

		const auto l_tot = sys.total_angular_momentum();
		ImGui::Text("Total Angular Momentum:(%.2e, %.2e, %.2e)", l_tot[0], l_tot[1], l_tot[2]);
		render_setting_tooltip("Includes the 1PN spin-orbit correction when enabled and available for a two-body configuration.");

		ImGui::Text("Total Mechanical Energy: %.6e", sys.compute_total_energy());
		ImGui::TextDisabled("%s", Units::format_energy(sys.compute_total_energy(), orchestrator_.unit_preferences().energy).c_str());

		const auto& gw = sys.latest_gw_emission();
		ImGui::Text("GW Radiated Power:       %.6e W", gw.radiated_power);
		render_setting_tooltip("Quadrupole-formula gravitational-wave luminosity computed from the current body configuration.");
	}

	void render_interactions_tab() noexcept {
		auto& cfg = orchestrator_.interaction_config();
		std::lock_guard<std::recursive_mutex> interactions_lock(orchestrator_.nbody_system().bodies_mutex());
		const auto bodies_span = orchestrator_.nbody_system().bodies();

		ImGui::TextColored(ImVec4(0.3f, 0.9f, 1.0f, 1.0f), "Electromagnetic Interactions");
		bool electricity = cfg.electromagnetic.electricity_enabled;
		if (ImGui::Checkbox("Enable Electricity (Coulomb Force)", &electricity)) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionElectricityEnabled, electricity ? 1.0 : 0.0)));
		}
		render_setting_tooltip("Enables attraction and repulsion between charged bodies following Coulomb's law, scaled by the medium permittivity below. Charge is set per body in the Body Catalog tab.");
		bool magnetism = cfg.electromagnetic.magnetism_enabled;
		if (ImGui::Checkbox("Enable Magnetism (Dipole Force)", &magnetism)) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionMagnetismEnabled, magnetism ? 1.0 : 0.0)));
		}
		render_setting_tooltip("Enables dipole-dipole magnetic forces derived from each body's magnetic moment value. Both interacting bodies must have a non-zero magnetic moment for a force to appear.");
		{
			const auto warn = Dynamics::magnetism_without_moments_warning(bodies_span, cfg.electromagnetic);
			if (!warn.empty()) render_wrapped_colored_text(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), std::string(warn).c_str());
		}
		float permittivity = static_cast<float>(cfg.electromagnetic.vacuum_permittivity);
		if (slider_float_with_input("Medium Permittivity (epsilon)", &permittivity, 1e-14f, 1.0f, "%.4e", &em_permittivity_log_mode_, 1e-15f, 1e2f)) {
			cfg.electromagnetic.vacuum_permittivity = static_cast<double>(permittivity);
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionVacuumPermittivity, static_cast<double>(permittivity))));
		}
		render_setting_tooltip("Electrical permittivity of the medium the bodies interact through. Reduces the Coulomb force below its vacuum strength as this value grows. Real vacuum permittivity is roughly 8.85e-12; enable Log for practical control at that scale.");
		float permeability = static_cast<float>(cfg.electromagnetic.vacuum_permeability);
		if (slider_float_with_input("Medium Permeability (mu)", &permeability, 1e-10f, 10.0f, "%.4e", &em_permeability_log_mode_, 1e-11f, 1e3f)) {
			cfg.electromagnetic.vacuum_permeability = static_cast<double>(permeability);
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionVacuumPermeability, static_cast<double>(permeability))));
		}
		render_setting_tooltip("Magnetic permeability of the medium the bodies interact through, scaling the dipole-dipole magnetic force.");
		ImGui::Spacing();
		if (ImGui::Button("Sync To Vacuum Values From Constants Engine", ImVec2(-1.0f, 26.0f))) {
			const auto& engine = orchestrator_.constants_engine();
			cfg.electromagnetic.vacuum_permittivity = engine.sim_vacuum_permittivity();
			cfg.electromagnetic.vacuum_permeability = engine.sim_vacuum_permeability();
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionVacuumPermittivity, cfg.electromagnetic.vacuum_permittivity)));
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionVacuumPermeability, cfg.electromagnetic.vacuum_permeability)));
		}
		render_setting_tooltip("Overwrites the two fields above with the true vacuum permittivity and permeability derived from the fundamental constants c, G, h, kB currently active in the Physical Constants Engine window, keeping both subsystems consistent.");

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.2f, 1.0f), "Collisions");
		bool collisions = cfg.collisions.enabled;
		if (ImGui::Checkbox("Enable Collisions", &collisions)) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionCollisionsEnabled, collisions ? 1.0 : 0.0)));
		}
		render_setting_tooltip("Detects physical contact between overlapping bodies and resolves it with a Hertzian material-stiffness repulsion plus an impulse-based restitution and friction response. See docs/other/COLLISION_MODEL.md for the underlying derivation.");
		{
			const auto warn = Dynamics::collisions_without_radius_warning(bodies_span, cfg.collisions);
			if (!warn.empty()) render_wrapped_colored_text(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), std::string(warn).c_str());
		}
		if (collisions) {
			int response_idx = static_cast<int>(cfg.collisions.response_model);
			const char* response_names[] = {"Elastic (Restitution-Based)", "Inelastic (Perfectly Damped)"};
			if (ImGui::Combo("Response Model", &response_idx, response_names, IM_ARRAYSIZE(response_names))) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionCollisionResponseModel, static_cast<double>(response_idx))));
			}
			render_setting_tooltip("Elastic uses each body's Restitution property to bounce apart; Inelastic removes all normal-direction relative velocity on contact.");
			bool consider_rotation = cfg.collisions.consider_rotation;
			if (ImGui::Checkbox("Consider Rotation", &consider_rotation)) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionCollisionConsiderRotation, consider_rotation ? 1.0 : 0.0)));
			}
			render_setting_tooltip("Lets tangential friction impulses spin bodies up around their rotation axis, approximated as solid spheres (I = 0.4 * m * r^2).");
			ImGui::SameLine();
			bool consider_friction = cfg.collisions.consider_friction;
			if (ImGui::Checkbox("Consider Friction", &consider_friction)) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionCollisionConsiderFriction, consider_friction ? 1.0 : 0.0)));
			}
			render_setting_tooltip("Applies a Coulomb-clamped tangential impulse opposing contact-point sliding velocity, using the average of both bodies' Friction Coefficient property.");
			float restitution_mult = static_cast<float>(cfg.collisions.restitution_multiplier);
			if (slider_float_with_input("Restitution Multiplier", &restitution_mult, 0.0f, 4.0f, "%.2fx")) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionCollisionRestitutionMultiplier, static_cast<double>(restitution_mult))));
			}
			render_setting_tooltip("Global multiplier applied on top of each body's own Restitution property before it is clamped back into the physical [0, 1] range.");
			float stiffness = static_cast<float>(cfg.collisions.contact_stiffness_scale);
			if (slider_float_with_input("Contact Stiffness Scale", &stiffness, 1e-15f, 1.0f, "%.6e", &collision_stiffness_log_mode_, 1e-15f, 1e3f)) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionCollisionStiffnessScale, static_cast<double>(stiffness))));
			}
			render_setting_tooltip("Scales the Hertzian penetration repulsion force computed from each body's Young's Modulus property and the current overlap depth. Zero disables material-stiffness pushback and relies only on the instantaneous impulse response below.");
			float position_correction = static_cast<float>(cfg.collisions.position_correction_factor);
			if (slider_float_with_input("Position Correction Factor", &position_correction, 0.0f, 1.0f, "%.2f")) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionCollisionPositionCorrectionFactor, static_cast<double>(position_correction))));
			}
			render_setting_tooltip("Baumgarte-style geometric correction that nudges deeply overlapping bodies apart each step, mass-weighted, preventing residual sinking when the material stiffness above is too soft to fully separate stiff bodies in a single step.");
		}

		ImGui::Separator();
		ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.3f, 1.0f), "Thermodynamics");
		bool thermo = cfg.thermodynamics.enabled;
		if (ImGui::Checkbox("Enable Thermodynamics", &thermo)) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionThermodynamicsEnabled, thermo ? 1.0 : 0.0)));
		}
		render_setting_tooltip("Tracks per-body Temperature and Heat Capacity properties and radiates energy toward the ambient temperature below via the Stefan-Boltzmann law, scaled by each body's Absorption Factor.");
		{
			const auto warn = Dynamics::thermodynamics_disabled_ambient_note(cfg.thermodynamics);
			if (!warn.empty()) render_wrapped_colored_text(ImVec4(0.6f, 0.75f, 1.0f, 1.0f), std::string(warn).c_str());
		}
		if (thermo) {
			float ambient = static_cast<float>(cfg.thermodynamics.ambient_temperature_kelvin);
			if (slider_float_with_input("Ambient Temperature (K, -1 = none)", &ambient, -1.0f, 6000.0f, "%.1f")) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionAmbientTemperature, static_cast<double>(ambient))));
			}
			render_setting_tooltip("Background radiative temperature bodies cool toward or heat toward via Stefan-Boltzmann emission. Set to -1 to disable ambient radiative coupling entirely while keeping per-body heat capacity bookkeeping active.");
			if (ambient >= 0.0f) {
				ImGui::TextDisabled("%s", Units::format_temperature(static_cast<double>(ambient), orchestrator_.unit_preferences().temperature).c_str());
			}
			float coupling = static_cast<float>(cfg.thermodynamics.radiative_coupling_scale);
			if (slider_float_with_input("Radiative Coupling Scale", &coupling, 0.0f, 10.0f, "%.2fx")) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionRadiativeCouplingScale, static_cast<double>(coupling))));
			}
			render_setting_tooltip("Multiplies the Stefan-Boltzmann radiative exchange rate, letting the ambient coupling be sped up or slowed down without touching the physical Stefan-Boltzmann constant itself.");
		}

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.9f, 0.4f, 0.4f, 1.0f), "Fragmentation");
		bool fragmentation = cfg.fragmentation.enabled;
		if (ImGui::Checkbox("Enable Fragmentation", &fragmentation)) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionFragmentationEnabled, fragmentation ? 1.0 : 0.0)));
		}
		render_setting_tooltip("Allows bodies to shatter into smaller fragments once their Integrity property is depleted by high-energy collisions and, optionally, sustained tidal stress from the central spacetime source.");
		{
			const auto warn = Dynamics::fragmentation_requires_source_warning(cfg.fragmentation, cfg.collisions);
			if (!warn.empty()) render_wrapped_colored_text(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), std::string(warn).c_str());
		}
		if (fragmentation) {
			float min_fragment_mass = static_cast<float>(cfg.fragmentation.minimum_fragment_mass);
			if (slider_float_with_input("Minimum Fragment Mass", &min_fragment_mass, 1e-9f, 1e3f, "%.4e", &fragmentation_min_mass_log_mode_, 1e-12f, 1e6f)) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionMinimumFragmentMass, static_cast<double>(min_fragment_mass))));
			}
			render_setting_tooltip("Fragments whose computed mass would fall below this floor are dispersed entirely rather than spawned as new bodies, preventing runaway fragment counts.");
			int max_fragments = static_cast<int>(cfg.fragmentation.max_fragments_per_event);
			if (slider_int_with_input("Max Fragments Per Event", &max_fragments, 1, 8)) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionFragmentationMaxFragments, static_cast<double>(max_fragments))));
			}
			float energy_to_integrity = static_cast<float>(cfg.fragmentation.collision_energy_to_integrity_loss);
			if (slider_float_with_input("Collision Energy To Integrity Loss", &energy_to_integrity, 1e-12f, 1.0f, "%.4e", &fragmentation_energy_integrity_log_mode_, 1e-12f, 1e2f)) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionCollisionEnergyToIntegrityLoss, static_cast<double>(energy_to_integrity))));
			}
			render_setting_tooltip("Conversion factor from an impact's kinetic energy along the collision normal into lost Integrity for both colliding bodies.");
			bool tidal_stress = cfg.fragmentation.enable_tidal_stress;
			if (ImGui::Checkbox("Enable Tidal Disruption From Central Source", &tidal_stress)) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionFragmentationTidalStressEnabled, tidal_stress ? 1.0 : 0.0)));
			}
			render_setting_tooltip("Continuously erodes Integrity for bodies close to the central mass based on the differential gravitational acceleration across their physical radius, approximating tidal stretching near the Roche limit.");
			if (tidal_stress) {
				float tidal_to_integrity = static_cast<float>(cfg.fragmentation.tidal_stress_to_integrity_loss);
				if (slider_float_with_input("Tidal Stress To Integrity Loss", &tidal_to_integrity, 1e-12f, 1.0f, "%.4e", &fragmentation_tidal_integrity_log_mode_, 1e-12f, 1e2f)) {
					static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionFragmentationTidalStressToIntegrityLoss, static_cast<double>(tidal_to_integrity))));
				}
				render_setting_tooltip("Conversion factor from tidal stress energy density into lost Integrity per second.");
			}
		}

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.7f, 0.5f, 1.0f, 1.0f), "Annihilation");
		bool annihilation = cfg.annihilation.enabled;
		if (ImGui::Checkbox("Enable Annihilation", &annihilation)) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionAnnihilationEnabled, annihilation ? 1.0 : 0.0)));
		}
		render_setting_tooltip("Removes both colliding bodies entirely once they satisfy the contact and charge conditions below, modeling a complete matter-antimatter style annihilation event.");
		{
			const auto warn = Dynamics::annihilation_requires_charge_warning(cfg.annihilation, cfg.electromagnetic);
			if (!warn.empty()) render_wrapped_colored_text(ImVec4(0.6f, 0.75f, 1.0f, 1.0f), std::string(warn).c_str());
		}
		if (annihilation) {
			bool opposite_charge = cfg.annihilation.require_opposite_charge;
			if (ImGui::Checkbox("Require Opposite Charge", &opposite_charge)) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionAnnihilationRequireOppositeCharge, opposite_charge ? 1.0 : 0.0)));
			}
			float contact_scale = static_cast<float>(cfg.annihilation.contact_distance_scale);
			if (slider_float_with_input("Contact Distance Scale", &contact_scale, 0.01f, 4.0f, "%.2fx")) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionAnnihilationContactScale, static_cast<double>(contact_scale))));
			}
			render_setting_tooltip("Fraction of the combined radii within which annihilation triggers; values below 1 require deeper overlap than plain collision contact.");
		}
	}

	void spawn_orbiting_spacetime_source() noexcept {
		const double primary_mass = std::max(orchestrator_.parameters().mass, 1e-6);
		const double companion_mass = primary_mass * 0.5;
		const double orbit_radius = std::max(primary_mass * 40.0, 20.0);
		const double theta = random_real(std::numbers::pi * 0.35, std::numbers::pi * 0.65);
		const double phi = random_real(0.0, 2.0 * std::numbers::pi);
		const std::array<double, 3> position{
			orbit_radius * std::sin(theta) * std::cos(phi),
			orbit_radius * std::sin(theta) * std::sin(phi),
			orbit_radius * std::cos(theta)
		};
		const auto velocity = compute_circular_orbit_velocity(position, primary_mass + companion_mass);

		Dynamics::PostNewtonianBody body(
			0, companion_mass, companion_mass * 2.0,
			position, velocity,
			{0.0, 0.0, companion_mass * 0.3}
		);
		body.set_name("Companion Black Hole");
		body.is_spacetime_source = true;
		apply_body_preset_defaults(body, Dynamics::Body3DPreset::BlackHole);
		selected_body_index_ = -1;
		static_cast<void>(spawn_body(body, Dynamics::BodySurfaceLayerSet{}));
	}

	void populate_solar_system_archetype() noexcept {
		auto& sys = orchestrator_.nbody_system();
		orchestrator_.surface_layers().clear();
		sys.clear_bodies();

		const double central_mass = std::max(orchestrator_.parameters().mass, 1.0);
		const auto spawn_planet = [&](std::string_view name, Dynamics::Body3DPreset preset, double mass_ratio, double size_fraction, double orbit_in_central_masses) noexcept {
			const double mass = central_mass * mass_ratio;
			const double orbit_radius = central_mass * orbit_in_central_masses;
			const double radius = std::max(orbit_radius * size_fraction, 4.0 * mass);
			const std::array<double, 3> position{orbit_radius, 0.0, 0.0};
			Dynamics::PostNewtonianBody planet(0, mass, radius, position, compute_circular_orbit_velocity(position, central_mass), {0.0, 0.0, 0.0});
			apply_body_preset_defaults(planet, preset);
			apply_material_defaults(planet, preset);
			planet.set_name(name);
			static_cast<void>(spawn_body(planet, Dynamics::BodySurfaceLayerSet{}));
		};
		spawn_planet("Inner Planet", Dynamics::Body3DPreset::TerrestrialPlanet, 3e-7, 0.006, 14.0);
		spawn_planet("Outer Planet", Dynamics::Body3DPreset::GasGiant, 4e-5, 0.02, 32.0);
		selected_body_index_ = -1;
	}
};

}

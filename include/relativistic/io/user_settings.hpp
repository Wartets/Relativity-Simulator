#pragma once

#include "relativistic/io/scenario/scenario_locator.hpp"
#include "relativistic/ui/camera_control_config.hpp"
#include "relativistic/ui/hud/hud_layout_config.hpp"
#include "relativistic/ui/schematic/schematic_view_config.hpp"
#include "relativistic/ui/spatial_reference/spatial_reference_config.hpp"
#include "relativistic/io/capture/capture_settings_io.hpp"
#include "relativistic/ui/window_chrome.hpp"
#include "relativistic/ui/interface_persistence.hpp"
#include "relativistic/core/physical_constants_engine.hpp"
#include "relativistic/units/unit_system.hpp"
#include "relativistic/observer/coordinate_systems.hpp"
#include "relativistic/ui/body_manager/body_randomizer_config.hpp"
#include <cstdint>
#include <cstdlib>
#include <string>
#include <fstream>
#include <filesystem>
#include <unordered_map>
#include <array>

namespace Relativistic::IO {

inline constexpr uint32_t USER_SETTINGS_FORMAT_VERSION = 7;

struct SecondaryViewPersistedState {
	bool active{false};
	bool open{false};
	std::string name{};
	double radius{60.0};
	double theta{1.5707963267948966};
	double phi{0.0};
	double fov_deg{60.0};
	double exposure{0.0};
	int tonemapping_mode{1};
	int projection_mode{0};
	int max_steps{768};
	float resolution_scale{0.5f};
	bool follow_primary{false};
	double follow_offset_theta{0.35};
	double follow_offset_phi{1.5707963267948966};
};

enum class SettingsLoadPolicy : uint32_t {
	AlwaysResetToDefaults = 0,
	RestorePreviousSession = 1
};

struct UserSettings {
	uint32_t format_version{USER_SETTINGS_FORMAT_VERSION};
	SettingsLoadPolicy load_policy{SettingsLoadPolicy::RestorePreviousSession};

	uint32_t default_camera_mode{0};
	uint32_t default_performance_preset{1};
	bool default_use_gpu_compute{true};
	std::string default_scenario_path{ScenarioLocator::kBuiltInStartupScenario};
	bool load_scenario_on_startup{true};
	std::string screenshot_output_directory{"./screenshots"};
	std::string screenshot_filename_pattern{"relativistic_%metric%_%Y%m%d_%H%M%S"};
	uint32_t screenshot_format{0};
	bool screenshot_watermark_enabled{false};
	std::string screenshot_watermark_text{"Relativistic Engine"};
	uint32_t screenshot_overwrite_policy{1};
	bool show_system_console{true};
	bool window_log_console_open{false};
	bool sky_rendering_enabled{true};
	bool sky_seed_locked{false};
	Render::ProceduralSkySettings sky{};
	static constexpr size_t kMaxSecondaryViews = 8;
	std::array<SecondaryViewPersistedState, kMaxSecondaryViews> secondary_views{};

	uint32_t body_manager_sort_mode{0};
	bool body_manager_sort_descending{false};
	std::string body_manager_search_filter{};
	float body_manager_list_pane_width{230.0f};
	uint32_t body_manager_source_sort_mode{0};
	bool body_manager_source_sort_descending{false};
	std::string body_manager_source_search_filter{};
	float body_manager_source_list_pane_width{300.0f};
	uint32_t body_manager_bulk_parameter_index{0};
	bool body_manager_new_body_mass_log_mode{true};
	bool body_manager_new_body_radius_log_mode{true};
	bool body_manager_new_source_mass_log_mode{true};
	int body_manager_reference_body_id{-2};
	bool body_manager_relative_coords_mode{false};

	bool body_sec_identity_open{true};
	bool body_sec_motion_open{true};
	bool body_sec_orbital_plane_open{false};
	bool body_sec_frenet_open{false};
	bool body_sec_orientation_open{false};
	bool body_sec_multipoles_open{false};
	bool body_sec_material_open{false};
	bool body_sec_surface_open{false};
	bool body_sec_accretion_disk_open{false};
	bool body_creation_sec_template_open{true};
	bool body_creation_sec_comparator_open{false};
	bool body_creation_sec_plane_open{false};
	bool body_creation_sec_hazard_open{false};
	bool source_creation_sec_template_open{true};
	bool source_creation_sec_comparator_open{false};
	bool source_creation_sec_plane_open{false};
	bool source_creation_sec_hazard_open{false};
	bool sys_dyn_sec_bulk_actions_open{true};
	bool sys_dyn_sec_plane_align_open{false};
	bool sys_dyn_sec_generalized_open{false};
	bool sys_dyn_sec_sources_open{false};
	bool interactions_sec_em_open{true};
	bool interactions_sec_collisions_open{true};
	bool interactions_sec_thermo_open{false};
	bool interactions_sec_frag_open{false};
	bool interactions_sec_annihilation_open{false};
	bool catalog_filter_ranges_open{false};

	UI::WindowChromeSettings window_chrome{};
	std::unordered_map<std::string, std::string> session_values{};

	UI::CameraControlConfig camera_controls{};
	UI::HudLayoutConfig hud_layout{};
	UI::SchematicViewConfig schematic_view{};
	UI::SpatialReferenceConfig spatial_reference{};
	Units::UnitDisplayPreferences unit_preferences{};
	Observer::CoordinatePreferences coordinate_preferences{};

	uint32_t last_window_layout{0};
	bool multi_window_mode{true};

	bool window_control_panel_open{true};
	bool window_performance_open{true};
	bool window_scenario_open{true};
	bool window_telemetry_open{false};
	bool window_spectrograph_open{false};
	bool window_diagnostics_open{false};
	bool window_body_manager_open{false};
	bool window_performance_analysis_open{false};
	bool window_hud_manager_open{false};
	bool window_keybind_settings_open{false};
	bool window_constants_open{false};
	bool window_capture_studio_open{false};
	uint32_t constants_preset{1};
	double constants_c{1.0};
	double constants_g{1.0};
	double constants_h{2.0 * std::numbers::pi};
	double constants_kb{1.0};
	double constants_na{Core::SIReferenceConstants::AVOGADRO_CONSTANT};
	double constants_ke{1.0};
	double constants_kcd{1.0};
	bool interaction_electricity_enabled{false};
	bool interaction_magnetism_enabled{false};
	double interaction_vacuum_permittivity{8.8541878128e-12};
	double interaction_vacuum_permeability{1.25663706212e-6};
	bool interaction_collisions_enabled{false};
	uint32_t interaction_collision_response_model{0};
	bool interaction_collision_consider_rotation{true};
	bool interaction_collision_consider_friction{true};
	double interaction_collision_restitution_multiplier{1.0};
	bool interaction_thermodynamics_enabled{false};
	double interaction_ambient_temperature{-1.0};
	double interaction_radiative_coupling_scale{1.0};
	bool interaction_fragmentation_enabled{false};
	double interaction_minimum_fragment_mass{1e-6};
	uint32_t interaction_fragmentation_max_fragments{2};
	double interaction_collision_energy_to_integrity_loss{1e-6};
	bool interaction_annihilation_enabled{false};
	double interaction_annihilation_contact_scale{1.0};
	bool interaction_annihilation_require_opposite_charge{true};
	double interaction_collision_stiffness_scale{1.0e-9};
	double interaction_collision_position_correction_factor{0.2};
	bool interaction_fragmentation_tidal_stress_enabled{false};
	std::string system_preset_selected{"solar_system_real"};
	UI::BodyRandomizerConfig body_randomizer{};
	bool cluster_enabled{false};
	uint32_t cluster_node_role{1};
	uint32_t cluster_listen_port{42425};
	bool cluster_discovery_multicast_enabled{true};
	bool cluster_dynamic_load_balancing{true};
	double cluster_latency_penalty_alpha{1.0};
	bool cluster_work_stealing_enabled{true};
	uint32_t cluster_tile_timeout_ms{120};
	uint32_t cluster_compression_mode{2};
	bool cluster_require_encryption{true};
	std::string cluster_shared_token_hex{};
	bool cluster_route_interactive_viewport{true};
	bool cluster_route_capture_studio{true};
	bool cluster_route_monte_carlo_ensembles{true};
	bool cluster_route_nbody_interactions{false};
	bool cluster_route_exact_spacetime_metrics{true};

	[[nodiscard]] static std::filesystem::path settings_file_path() {
		return std::filesystem::path("config") / "user_settings.cfg";
	}

	[[nodiscard]] static std::filesystem::path preferences_directory() {
		return std::filesystem::path("config") / "preferences";
	}

	[[nodiscard]] static std::filesystem::path general_settings_path() {
		return preferences_directory() / "general.cfg";
	}

	[[nodiscard]] static std::filesystem::path camera_settings_path() {
		return preferences_directory() / "camera.cfg";
	}

	[[nodiscard]] static std::filesystem::path render_sky_settings_path() {
		return preferences_directory() / "render_sky.cfg";
	}

	[[nodiscard]] static std::filesystem::path interface_settings_path() {
		return preferences_directory() / "interface.cfg";
	}

	[[nodiscard]] static std::filesystem::path units_settings_path() {
		return preferences_directory() / "units.cfg";
	}

	[[nodiscard]] static std::filesystem::path physics_settings_path() {
		return preferences_directory() / "physics.cfg";
	}

	[[nodiscard]] static std::filesystem::path bodies_settings_path() {
		return preferences_directory() / "celestial_bodies.cfg";
	}

	[[nodiscard]] static std::filesystem::path capture_settings_path() {
		return preferences_directory() / "capture.cfg";
	}

	[[nodiscard]] static std::filesystem::path cluster_settings_path() {
		return preferences_directory() / "cluster.cfg";
	}

	[[nodiscard]] static std::filesystem::path crash_guard_path() {
		return std::filesystem::path("config") / ".session_active";
	}

	static std::unordered_map<std::string, std::string> parse_kv_file(const std::filesystem::path& path) {
		std::unordered_map<std::string, std::string> kv;
		std::ifstream file(path);
		if (!file.is_open()) {
			return kv;
		}
		std::string line;
		while (std::getline(file, line)) {
			if (!line.empty() && line.back() == '\r') {
				line.pop_back();
			}
			const size_t eq = line.find('=');
			if (eq == std::string::npos) continue;
			kv[line.substr(0, eq)] = line.substr(eq + 1);
		}
		return kv;
	}

	template <typename Writer>
	static void write_kv_file(const std::filesystem::path& path, Writer&& writer) {
		std::error_code ec;
		std::filesystem::create_directories(path.parent_path(), ec);
		std::ofstream out(path, std::ios::trunc);
		if (!out.is_open()) return;
		writer(out);
	}

	static std::string get_str(const std::unordered_map<std::string, std::string>& kv, const char* key, const std::string& fallback) {
		auto it = kv.find(key);
		return (it != kv.end()) ? it->second : fallback;
	}

	static uint32_t get_u32(const std::unordered_map<std::string, std::string>& kv, const char* key, uint32_t fallback) {
		auto it = kv.find(key);
		return (it != kv.end()) ? static_cast<uint32_t>(std::strtoul(it->second.c_str(), nullptr, 10)) : fallback;
	}

	static int get_i32(const std::unordered_map<std::string, std::string>& kv, const char* key, int fallback) {
		auto it = kv.find(key);
		return (it != kv.end()) ? static_cast<int>(std::strtol(it->second.c_str(), nullptr, 10)) : fallback;
	}

	static double get_dbl(const std::unordered_map<std::string, std::string>& kv, const char* key, double fallback) {
		auto it = kv.find(key);
		return (it != kv.end()) ? std::strtod(it->second.c_str(), nullptr) : fallback;
	}

	static bool get_bool(const std::unordered_map<std::string, std::string>& kv, const char* key, bool fallback) {
		auto it = kv.find(key);
		if (it == kv.end()) return fallback;
		if (it->second == "true" || it->second == "True" || it->second == "1") return true;
		if (it->second == "false" || it->second == "False" || it->second == "0") return false;
		return std::strtoul(it->second.c_str(), nullptr, 10) != 0;
	}

	void read_general(const std::unordered_map<std::string, std::string>& kv) {
		format_version = get_u32(kv, "format_version", format_version);
		load_policy = static_cast<SettingsLoadPolicy>(get_u32(kv, "load_policy", static_cast<uint32_t>(load_policy)));
		default_performance_preset = get_u32(kv, "default_performance_preset", default_performance_preset);
		default_use_gpu_compute = get_bool(kv, "default_use_gpu_compute", default_use_gpu_compute);
		default_scenario_path = get_str(kv, "default_scenario_path", default_scenario_path);
		load_scenario_on_startup = get_bool(kv, "load_scenario_on_startup", load_scenario_on_startup);
		show_system_console = get_bool(kv, "show_system_console", show_system_console);
		last_window_layout = get_u32(kv, "last_window_layout", last_window_layout);
		multi_window_mode = get_bool(kv, "multi_window_mode", multi_window_mode);
	}

	void read_camera(const std::unordered_map<std::string, std::string>& kv) {
		default_camera_mode = get_u32(kv, "default_camera_mode", default_camera_mode);
		camera_controls.free_fly.forward_speed = get_dbl(kv, "cam_ff_forward_speed", camera_controls.free_fly.forward_speed);
		camera_controls.free_fly.lateral_speed = get_dbl(kv, "cam_ff_lateral_speed", camera_controls.free_fly.lateral_speed);
		camera_controls.free_fly.vertical_speed = get_dbl(kv, "cam_ff_vertical_speed", camera_controls.free_fly.vertical_speed);
		camera_controls.free_fly.invert_vertical = get_bool(kv, "cam_ff_invert_vertical", camera_controls.free_fly.invert_vertical);
		camera_controls.free_fly.invert_forward = get_bool(kv, "cam_ff_invert_forward", camera_controls.free_fly.invert_forward);
		camera_controls.free_fly.invert_lateral = get_bool(kv, "cam_ff_invert_lateral", camera_controls.free_fly.invert_lateral);
		camera_controls.free_fly.invert_mouse_y = get_bool(kv, "cam_ff_invert_mouse_y", camera_controls.free_fly.invert_mouse_y);
		camera_controls.free_fly.invert_mouse_x = get_bool(kv, "cam_ff_invert_mouse_x", camera_controls.free_fly.invert_mouse_x);
		camera_controls.free_fly.mouse_sensitivity = get_dbl(kv, "cam_ff_mouse_sensitivity", camera_controls.free_fly.mouse_sensitivity);
		camera_controls.free_fly.sprint_multiplier = get_dbl(kv, "cam_ff_sprint_multiplier", camera_controls.free_fly.sprint_multiplier);
		camera_controls.free_fly.crawl_multiplier = get_dbl(kv, "cam_ff_crawl_multiplier", camera_controls.free_fly.crawl_multiplier);
		camera_controls.free_fly.ignore_pitch_roll_for_movement = get_bool(kv, "cam_ff_ignore_pitch_roll", camera_controls.free_fly.ignore_pitch_roll_for_movement);

		camera_controls.zoom.zoom_center_on_cursor = get_bool(kv, "cam_zoom_center_on_cursor", camera_controls.zoom.zoom_center_on_cursor);
		camera_controls.zoom.zoom_scroll_sensitivity = get_dbl(kv, "cam_zoom_scroll_sensitivity", camera_controls.zoom.zoom_scroll_sensitivity);
		camera_controls.zoom.max_zoom = get_dbl(kv, "cam_zoom_max", camera_controls.zoom.max_zoom);
		camera_controls.zoom.min_zoom = get_dbl(kv, "cam_zoom_min", camera_controls.zoom.min_zoom);

		camera_controls.orbit.orbit_distance_speed = get_dbl(kv, "cam_orbit_distance_speed", camera_controls.orbit.orbit_distance_speed);
		camera_controls.orbit.pitch_speed_deg_s = get_dbl(kv, "cam_orbit_pitch_speed", camera_controls.orbit.pitch_speed_deg_s);
		camera_controls.orbit.yaw_speed_deg_s = get_dbl(kv, "cam_orbit_yaw_speed", camera_controls.orbit.yaw_speed_deg_s);
		camera_controls.orbit.invert_pitch = get_bool(kv, "cam_orbit_invert_pitch", camera_controls.orbit.invert_pitch);

		camera_controls.rocket.main_thrust_accel = get_dbl(kv, "cam_rocket_main_thrust", camera_controls.rocket.main_thrust_accel);
		camera_controls.rocket.lateral_thrust_accel = get_dbl(kv, "cam_rocket_lateral_thrust", camera_controls.rocket.lateral_thrust_accel);
		camera_controls.rocket.vertical_thrust_accel = get_dbl(kv, "cam_rocket_vertical_thrust", camera_controls.rocket.vertical_thrust_accel);
		camera_controls.rocket.angular_rate_deg_s = get_dbl(kv, "cam_rocket_angular_rate", camera_controls.rocket.angular_rate_deg_s);
		camera_controls.rocket.invert_vertical = get_bool(kv, "cam_rocket_invert_vertical", camera_controls.rocket.invert_vertical);
		camera_controls.rocket.invert_lateral = get_bool(kv, "cam_rocket_invert_lateral", camera_controls.rocket.invert_lateral);
		camera_controls.rocket.requires_time_running = get_bool(kv, "cam_rocket_requires_time", camera_controls.rocket.requires_time_running);

		auto& walk = camera_controls.surface_walk;
		walk.scale_with_body = get_bool(kv, "cam_walk_scale_with_body", walk.scale_with_body);
		walk.walker_height = get_dbl(kv, "cam_walk_height", walk.walker_height);
		walk.eye_height_fraction = get_dbl(kv, "cam_walk_eye_fraction", walk.eye_height_fraction);
		walk.crouch_fraction = get_dbl(kv, "cam_walk_crouch_fraction", walk.crouch_fraction);
		walk.crouch_speed_multiplier = get_dbl(kv, "cam_walk_crouch_speed", walk.crouch_speed_multiplier);
		walk.walk_speed = get_dbl(kv, "cam_walk_speed", walk.walk_speed);
		walk.sprint_multiplier = get_dbl(kv, "cam_walk_sprint", walk.sprint_multiplier);
		walk.crawl_multiplier = get_dbl(kv, "cam_walk_crawl", walk.crawl_multiplier);
		walk.step_length = get_dbl(kv, "cam_walk_step_length", walk.step_length);
		walk.jump_height = get_dbl(kv, "cam_walk_jump_height", walk.jump_height);
		walk.automatic_gravity = get_bool(kv, "cam_walk_auto_gravity", walk.automatic_gravity);
		walk.gravity_scale = get_dbl(kv, "cam_walk_gravity_scale", walk.gravity_scale);
		walk.manual_gravity = get_dbl(kv, "cam_walk_manual_gravity", walk.manual_gravity);
		walk.maximum_airtime_seconds = get_dbl(kv, "cam_walk_max_airtime", walk.maximum_airtime_seconds);
		walk.ground_response_seconds = get_dbl(kv, "cam_walk_ground_response", walk.ground_response_seconds);
		walk.air_control = get_dbl(kv, "cam_walk_air_control", walk.air_control);
		walk.head_bob_enabled = get_bool(kv, "cam_walk_head_bob", walk.head_bob_enabled);
		walk.head_bob_amplitude = get_dbl(kv, "cam_walk_head_bob_amplitude", walk.head_bob_amplitude);
		walk.follow_surface_rotation = get_bool(kv, "cam_walk_follow_rotation", walk.follow_surface_rotation);
		walk.maximum_look_pitch_deg = get_dbl(kv, "cam_walk_max_look_pitch", walk.maximum_look_pitch_deg);
		walk.maximum_drop_heights = get_dbl(kv, "cam_walk_max_drop_heights", walk.maximum_drop_heights);
		walk.sanitize();

		auto& planet = camera_controls.planet_orbit;
		planet.angular_speed_deg_s = get_dbl(kv, "cam_planet_speed", planet.angular_speed_deg_s);
		planet.drag_sensitivity_deg_per_pixel = get_dbl(kv, "cam_planet_drag", planet.drag_sensitivity_deg_per_pixel);
		planet.zoom_rate = get_dbl(kv, "cam_planet_zoom_rate", planet.zoom_rate);
		planet.wheel_zoom_factor = get_dbl(kv, "cam_planet_wheel", planet.wheel_zoom_factor);
		planet.minimum_altitude_ratio = get_dbl(kv, "cam_planet_min_altitude", planet.minimum_altitude_ratio);
		planet.maximum_altitude_ratio = get_dbl(kv, "cam_planet_max_altitude", planet.maximum_altitude_ratio);
		planet.smoothing_seconds = get_dbl(kv, "cam_planet_smoothing", planet.smoothing_seconds);
		planet.maximum_pitch_deg = get_dbl(kv, "cam_planet_max_pitch", planet.maximum_pitch_deg);
		planet.sprint_multiplier = get_dbl(kv, "cam_planet_sprint", planet.sprint_multiplier);
		planet.crawl_multiplier = get_dbl(kv, "cam_planet_crawl", planet.crawl_multiplier);
		planet.invert_drag = get_bool(kv, "cam_planet_invert_drag", planet.invert_drag);
		planet.sanitize();

		const auto loaded_layout = static_cast<UI::KeyboardLayout>(get_u32(kv, "cam_keyboard_layout", static_cast<uint32_t>(UI::KeyboardLayout::Qwerty)));
		camera_controls.apply_keyboard_layout(loaded_layout);

		for (size_t i = 0; i < static_cast<size_t>(UI::InputAction::Count); ++i) {
			const std::string primary_key = "kb_" + std::to_string(i) + "_primary";
			const std::string secondary_key = "kb_" + std::to_string(i) + "_secondary";
			auto it_primary = kv.find(primary_key);
			auto it_secondary = kv.find(secondary_key);
			if (it_primary != kv.end() || it_secondary != kv.end()) {
				const auto action = static_cast<UI::InputAction>(i);
				const auto& current_bind = camera_controls.keybinds.get(action);
				const int primary = (it_primary != kv.end()) ? get_i32(kv, primary_key.c_str(), current_bind.primary_key) : current_bind.primary_key;
				const int secondary = (it_secondary != kv.end()) ? get_i32(kv, secondary_key.c_str(), current_bind.secondary_key) : current_bind.secondary_key;
				camera_controls.keybinds.set(action, primary, secondary);
			}
			const std::string mode_key = "kb_" + std::to_string(i) + "_mode";
			auto it_mode = kv.find(mode_key);
			if (it_mode != kv.end()) {
				const auto action = static_cast<UI::InputAction>(i);
				camera_controls.keybinds.set_mode(action, static_cast<UI::InputActivationMode>(get_u32(kv, mode_key.c_str(), 0)));
			}
		}
	}

	[[nodiscard]] static bool previous_session_crashed() noexcept {
		std::error_code ec;
		return std::filesystem::exists(crash_guard_path(), ec);
	}

	static void mark_session_started() {
		std::error_code ec;
		std::filesystem::create_directories(crash_guard_path().parent_path(), ec);
		std::ofstream marker(crash_guard_path(), std::ios::trunc);
		marker << "1";
	}

	static void mark_session_ended_cleanly() noexcept {
		std::error_code ec;
		std::filesystem::remove(crash_guard_path(), ec);
	}

	void read_render_sky(const std::unordered_map<std::string, std::string>& kv) {
		sky_rendering_enabled = get_bool(kv, "sky_rendering_enabled", sky_rendering_enabled);
		sky_seed_locked = get_bool(kv, "sky_seed_locked", sky_seed_locked);
		sky.enabled = sky_rendering_enabled;
		sky.seed_locked = sky_seed_locked;
		sky.seed = get_u32(kv, "sky_seed", sky.seed);
		sky.master_intensity = get_dbl(kv, "sky_master_intensity", sky.master_intensity);
		sky.quality_factor = get_dbl(kv, "sky_quality_factor", sky.quality_factor);
		sky.adaptive_fov_lod = get_bool(kv, "sky_adaptive_fov_lod", sky.adaptive_fov_lod);
		sky.adaptive_fov_quality = get_bool(kv, "sky_adaptive_fov_quality", sky.adaptive_fov_quality);
		sky.adaptive_fov_reference = get_dbl(kv, "sky_adaptive_fov_reference", sky.adaptive_fov_reference);
		sky.adaptive_fov_sensitivity = get_dbl(kv, "sky_adaptive_fov_sensitivity", sky.adaptive_fov_sensitivity);
		sky.max_star_octaves = get_u32(kv, "sky_max_star_octaves", sky.max_star_octaves);
		sky.star_quality_factor = get_dbl(kv, "sky_star_quality_factor", sky.star_quality_factor);
		sky.nebula_quality_factor = get_dbl(kv, "sky_nebula_quality_factor", sky.nebula_quality_factor);
		sky.cluster_quality_factor = get_dbl(kv, "sky_cluster_quality_factor", sky.cluster_quality_factor);
		sky.dust_quality_factor = get_dbl(kv, "sky_dust_quality_factor", sky.dust_quality_factor);
		sky.dynamic_motion = get_bool(kv, "sky_dynamic_motion", sky.dynamic_motion);
		sky.dynamic_rotation = get_bool(kv, "sky_dynamic_rotation", sky.dynamic_rotation);
		sky.motion_speed = get_dbl(kv, "sky_motion_speed", sky.motion_speed);
		sky.rotation_rate_deg_s = get_dbl(kv, "sky_rotation_rate_deg_s", sky.rotation_rate_deg_s);
		sky.stars_enabled = get_bool(kv, "sky_stars_enabled", sky.stars_enabled);
		sky.star_profile_model = get_u32(kv, "sky_star_profile_model", sky.star_profile_model);
		sky.star_profile_param = get_dbl(kv, "sky_star_profile_param", sky.star_profile_param);
		sky.star_spike_count = get_u32(kv, "sky_star_spike_count", sky.star_spike_count);
		sky.star_spike_length = get_dbl(kv, "sky_star_spike_length", sky.star_spike_length);
		sky.star_spike_intensity = get_dbl(kv, "sky_star_spike_intensity", sky.star_spike_intensity);
		sky.star_spike_angle_deg = get_dbl(kv, "sky_star_spike_angle_deg", sky.star_spike_angle_deg);
		sky.star_profile_softness = get_dbl(kv, "sky_star_profile_softness", sky.star_profile_softness);
		sky.star_placement_model = get_u32(kv, "sky_star_placement_model", sky.star_placement_model);
		sky.star_galactic_concentration = get_dbl(kv, "sky_star_galactic_concentration", sky.star_galactic_concentration);
		sky.nebula_color_separation = get_dbl(kv, "sky_nebula_color_separation", sky.nebula_color_separation);
		sky.nebula_ionization_glow = get_dbl(kv, "sky_nebula_ionization_glow", sky.nebula_ionization_glow);
		sky.nebula_gas_contrast = get_dbl(kv, "sky_nebula_gas_contrast", sky.nebula_gas_contrast);
		sky.cluster_placement_model = get_u32(kv, "sky_cluster_placement_model", sky.cluster_placement_model);
		sky.cluster_halo_radius = get_dbl(kv, "sky_cluster_halo_radius", sky.cluster_halo_radius);
		sky.cluster_plane_scale_height = get_dbl(kv, "sky_cluster_plane_scale_height", sky.cluster_plane_scale_height);
		sky.cluster_stars_enabled = get_bool(kv, "sky_cluster_stars_enabled", sky.cluster_stars_enabled);
		sky.cluster_member_count = get_u32(kv, "sky_cluster_member_count", sky.cluster_member_count);
		sky.cluster_member_size = get_dbl(kv, "sky_cluster_member_size", sky.cluster_member_size);
		sky.cluster_member_brightness = get_dbl(kv, "sky_cluster_member_brightness", sky.cluster_member_brightness);
		sky.cluster_star_profile_model = get_u32(kv, "sky_cluster_star_profile_model", sky.cluster_star_profile_model);
		sky.star_profile_param2 = get_dbl(kv, "sky_star_profile_param2", sky.star_profile_param2);
		sky.star_dirac_concentration = get_dbl(kv, "sky_star_dirac_concentration", sky.star_dirac_concentration);
		sky.star_bloom_radius = get_dbl(kv, "sky_star_bloom_radius", sky.star_bloom_radius);
		sky.star_bloom_intensity = get_dbl(kv, "sky_star_bloom_intensity", sky.star_bloom_intensity);
		sky.quasar_quality_factor = get_dbl(kv, "sky_quasar_quality_factor", sky.quasar_quality_factor);
		sky.pulsar_quality_factor = get_dbl(kv, "sky_pulsar_quality_factor", sky.pulsar_quality_factor);
		sky.snr_quality_factor = get_dbl(kv, "sky_snr_quality_factor", sky.snr_quality_factor);
		sky.quasar_jet_length = get_dbl(kv, "sky_quasar_jet_length", sky.quasar_jet_length);
		sky.quasar_jet_intensity = get_dbl(kv, "sky_quasar_jet_intensity", sky.quasar_jet_intensity);
		sky.pulsar_beam_width_deg = get_dbl(kv, "sky_pulsar_beam_width_deg", sky.pulsar_beam_width_deg);
		sky.snr_expansion_age = get_dbl(kv, "sky_snr_expansion_age", sky.snr_expansion_age);
		sky.grid_show_ticks = get_bool(kv, "sky_grid_show_ticks", sky.grid_show_ticks);
		sky.grid_tick_interval_deg = get_dbl(kv, "sky_grid_tick_interval_deg", sky.grid_tick_interval_deg);
		sky.grid_tick_length = get_dbl(kv, "sky_grid_tick_length", sky.grid_tick_length);
		sky.star_density = get_dbl(kv, "sky_star_density", sky.star_density);
		sky.star_brightness = get_dbl(kv, "sky_star_brightness", sky.star_brightness);
		sky.star_size_min = get_dbl(kv, "sky_star_size_min", sky.star_size_min);
		sky.star_size_max = get_dbl(kv, "sky_star_size_max", sky.star_size_max);
		sky.star_size_distribution = get_u32(kv, "sky_star_size_distribution", sky.star_size_distribution);
		sky.star_size_param = get_dbl(kv, "sky_star_size_param", sky.star_size_param);
		sky.star_luminosity_distribution = get_u32(kv, "sky_star_lum_distribution", sky.star_luminosity_distribution);
		sky.star_luminosity_param = get_dbl(kv, "sky_star_lum_param", sky.star_luminosity_param);
		sky.star_color_variation = get_dbl(kv, "sky_star_color_variation", sky.star_color_variation);
		sky.star_temperature_bias = get_dbl(kv, "sky_star_temperature_bias", sky.star_temperature_bias);
		sky.star_brightness_variation = get_dbl(kv, "sky_star_brightness_variation", sky.star_brightness_variation);
		sky.star_scintillation = get_dbl(kv, "sky_star_scintillation", sky.star_scintillation);
		sky.star_magnitude_limit = get_dbl(kv, "sky_star_magnitude_limit", sky.star_magnitude_limit);
		sky.nebula_enabled = get_bool(kv, "sky_nebula_enabled", sky.nebula_enabled);
		sky.nebula_intensity = get_dbl(kv, "sky_nebula_intensity", sky.nebula_intensity);
		sky.nebula_roughness = get_dbl(kv, "sky_nebula_roughness", sky.nebula_roughness);
		sky.nebula_octaves = get_u32(kv, "sky_nebula_octaves", sky.nebula_octaves);
		sky.nebula_frequency = get_dbl(kv, "sky_nebula_frequency", sky.nebula_frequency);
		sky.nebula_plane_thickness = get_dbl(kv, "sky_nebula_plane_thickness", sky.nebula_plane_thickness);
		sky.nebula_plane_falloff = get_dbl(kv, "sky_nebula_plane_falloff", sky.nebula_plane_falloff);
		sky.nebula_asymmetry = get_dbl(kv, "sky_nebula_asymmetry", sky.nebula_asymmetry);
		sky.nebula_absorption_density = get_dbl(kv, "sky_nebula_absorption_density", sky.nebula_absorption_density);
		sky.nebula_layer_count = get_u32(kv, "sky_nebula_layer_count", sky.nebula_layer_count);
		sky.nebula_layer_spread = get_dbl(kv, "sky_nebula_layer_spread", sky.nebula_layer_spread);
		sky.nebula_core_glow = get_dbl(kv, "sky_nebula_core_glow", sky.nebula_core_glow);
		sky.nebula_curl_strength = get_dbl(kv, "sky_nebula_curl_strength", sky.nebula_curl_strength);
		sky.galaxies_enabled = get_bool(kv, "sky_galaxies_enabled", sky.galaxies_enabled);
		sky.galaxy_density = get_dbl(kv, "sky_galaxy_density", sky.galaxy_density);
		sky.galaxy_brightness = get_dbl(kv, "sky_galaxy_brightness", sky.galaxy_brightness);
		sky.galaxy_size_scale = get_dbl(kv, "sky_galaxy_size_scale", sky.galaxy_size_scale);
		sky.galaxy_size_min = get_dbl(kv, "sky_galaxy_size_min", sky.galaxy_size_min);
		sky.galaxy_size_max = get_dbl(kv, "sky_galaxy_size_max", sky.galaxy_size_max);
		sky.galaxy_size_distribution = get_u32(kv, "sky_galaxy_size_distribution", sky.galaxy_size_distribution);
		sky.galaxy_size_param = get_dbl(kv, "sky_galaxy_size_param", sky.galaxy_size_param);
		sky.galaxy_spiral_arms = get_dbl(kv, "sky_galaxy_spiral_arms", sky.galaxy_spiral_arms);
		sky.galaxy_pitch_angle = get_dbl(kv, "sky_galaxy_pitch_angle", sky.galaxy_pitch_angle);
		sky.galaxy_core_ratio = get_dbl(kv, "sky_galaxy_core_ratio", sky.galaxy_core_ratio);
		sky.galaxy_ellipticity_variance = get_dbl(kv, "sky_galaxy_ellipticity_variance", sky.galaxy_ellipticity_variance);
		sky.clusters_enabled = get_bool(kv, "sky_clusters_enabled", sky.clusters_enabled);
		sky.cluster_density = get_dbl(kv, "sky_cluster_density", sky.cluster_density);
		sky.cluster_brightness = get_dbl(kv, "sky_cluster_brightness", sky.cluster_brightness);
		sky.cluster_size_scale = get_dbl(kv, "sky_cluster_size_scale", sky.cluster_size_scale);
		sky.cluster_size_min = get_dbl(kv, "sky_cluster_size_min", sky.cluster_size_min);
		sky.cluster_size_max = get_dbl(kv, "sky_cluster_size_max", sky.cluster_size_max);
		sky.cluster_size_distribution = get_u32(kv, "sky_cluster_size_distribution", sky.cluster_size_distribution);
		sky.cluster_concentration = get_dbl(kv, "sky_cluster_concentration", sky.cluster_concentration);
		sky.cluster_star_density_boost = get_dbl(kv, "sky_cluster_star_density_boost", sky.cluster_star_density_boost);
		sky.dust_enabled = get_bool(kv, "sky_dust_enabled", sky.dust_enabled);
		sky.dust_density = get_dbl(kv, "sky_dust_density", sky.dust_density);
		sky.dust_intensity = get_dbl(kv, "sky_dust_intensity", sky.dust_intensity);
		sky.dust_scale = get_dbl(kv, "sky_dust_scale", sky.dust_scale);
		sky.dust_absorption_ratio = get_dbl(kv, "sky_dust_absorption_ratio", sky.dust_absorption_ratio);
		sky.dust_fractal_octaves = get_u32(kv, "sky_dust_fractal_octaves", sky.dust_fractal_octaves);
		sky.dust_contrast = get_dbl(kv, "sky_dust_contrast", sky.dust_contrast);
		sky.quasar_density = get_dbl(kv, "sky_quasar_density", sky.quasar_density);
		sky.quasar_brightness = get_dbl(kv, "sky_quasar_brightness", sky.quasar_brightness);
		sky.quasar_core_size = get_dbl(kv, "sky_quasar_core_size", sky.quasar_core_size);
		sky.pulsar_pulse_rate = get_dbl(kv, "sky_pulsar_pulse_rate", sky.pulsar_pulse_rate);
		sky.pulsar_pulse_duty = get_dbl(kv, "sky_pulsar_pulse_duty", sky.pulsar_pulse_duty);
		sky.grid_enabled = get_bool(kv, "sky_grid_enabled", sky.grid_enabled);
		sky.grid_opacity = get_dbl(kv, "sky_grid_opacity", sky.grid_opacity);
		sky.grid_line_width = get_dbl(kv, "sky_grid_line_width", sky.grid_line_width);
		sky.grid_spacing_deg = get_dbl(kv, "sky_grid_spacing_deg", sky.grid_spacing_deg);
		sky.grid_subdivisions = get_u32(kv, "sky_grid_subdivisions", sky.grid_subdivisions);
		sky.grid_glow = get_dbl(kv, "sky_grid_glow", sky.grid_glow);
		sky.grid_major_color[0] = static_cast<float>(get_dbl(kv, "sky_grid_major_color_r", sky.grid_major_color[0]));
		sky.grid_major_color[1] = static_cast<float>(get_dbl(kv, "sky_grid_major_color_g", sky.grid_major_color[1]));
		sky.grid_major_color[2] = static_cast<float>(get_dbl(kv, "sky_grid_major_color_b", sky.grid_major_color[2]));
		sky.grid_minor_color[0] = static_cast<float>(get_dbl(kv, "sky_grid_minor_color_r", sky.grid_minor_color[0]));
		sky.grid_minor_color[1] = static_cast<float>(get_dbl(kv, "sky_grid_minor_color_g", sky.grid_minor_color[1]));
		sky.grid_minor_color[2] = static_cast<float>(get_dbl(kv, "sky_grid_minor_color_b", sky.grid_minor_color[2]));
		sky.grid_show_polar_caps = get_bool(kv, "sky_grid_show_polar_caps", sky.grid_show_polar_caps);
		sky.grid_show_equator = get_bool(kv, "sky_grid_show_equator", sky.grid_show_equator);
		sky.grid_show_meridians = get_bool(kv, "sky_grid_show_meridians", sky.grid_show_meridians);
		sky.grid_cell_fill_mode = get_u32(kv, "sky_grid_cell_fill_mode", sky.grid_cell_fill_mode);
		sky.grid_cell_opacity = get_dbl(kv, "sky_grid_cell_opacity", sky.grid_cell_opacity);
		sky.grid_tick_width = get_dbl(kv, "sky_grid_tick_width", sky.grid_tick_width);
		sky.grid_equator_width = get_dbl(kv, "sky_grid_equator_width", sky.grid_equator_width);
		sky.grid_equator_opacity = get_dbl(kv, "sky_grid_equator_opacity", sky.grid_equator_opacity);
		sky.grid_polar_width = get_dbl(kv, "sky_grid_polar_width", sky.grid_polar_width);
		sky.grid_polar_opacity = get_dbl(kv, "sky_grid_polar_opacity", sky.grid_polar_opacity);
		sky.grid_meridian_width = get_dbl(kv, "sky_grid_meridian_width", sky.grid_meridian_width);
		sky.grid_meridian_opacity = get_dbl(kv, "sky_grid_meridian_opacity", sky.grid_meridian_opacity);
		sky.grid_major_width = get_dbl(kv, "sky_grid_major_width", sky.grid_major_width);
		sky.grid_minor_width = get_dbl(kv, "sky_grid_minor_width", sky.grid_minor_width);
		sky.grid_equator_color[0] = static_cast<float>(get_dbl(kv, "sky_grid_equator_color_r", sky.grid_equator_color[0]));
		sky.grid_equator_color[1] = static_cast<float>(get_dbl(kv, "sky_grid_equator_color_g", sky.grid_equator_color[1]));
		sky.grid_equator_color[2] = static_cast<float>(get_dbl(kv, "sky_grid_equator_color_b", sky.grid_equator_color[2]));
		sky.grid_polar_color[0] = static_cast<float>(get_dbl(kv, "sky_grid_polar_color_r", sky.grid_polar_color[0]));
		sky.grid_polar_color[1] = static_cast<float>(get_dbl(kv, "sky_grid_polar_color_g", sky.grid_polar_color[1]));
		sky.grid_polar_color[2] = static_cast<float>(get_dbl(kv, "sky_grid_polar_color_b", sky.grid_polar_color[2]));
		sky.grid_meridian_color[0] = static_cast<float>(get_dbl(kv, "sky_grid_meridian_color_r", sky.grid_meridian_color[0]));
		sky.grid_meridian_color[1] = static_cast<float>(get_dbl(kv, "sky_grid_meridian_color_g", sky.grid_meridian_color[1]));
		sky.grid_meridian_color[2] = static_cast<float>(get_dbl(kv, "sky_grid_meridian_color_b", sky.grid_meridian_color[2]));
		sky.grid_tick_color[0] = static_cast<float>(get_dbl(kv, "sky_grid_tick_color_r", sky.grid_tick_color[0]));
		sky.grid_tick_color[1] = static_cast<float>(get_dbl(kv, "sky_grid_tick_color_g", sky.grid_tick_color[1]));
		sky.grid_tick_color[2] = static_cast<float>(get_dbl(kv, "sky_grid_tick_color_b", sky.grid_tick_color[2]));
		sky.grid_cell_color_count = get_u32(kv, "sky_grid_cell_color_count", sky.grid_cell_color_count);
		for (size_t c = 0; c < Render::ProceduralSkySettings::kMaxGridCellColors; ++c) {
			const std::string pr = "sky_grid_cell_color_" + std::to_string(c) + "_r";
			const std::string pg = "sky_grid_cell_color_" + std::to_string(c) + "_g";
			const std::string pb = "sky_grid_cell_color_" + std::to_string(c) + "_b";
			sky.grid_cell_colors[c][0] = static_cast<float>(get_dbl(kv, pr.c_str(), sky.grid_cell_colors[c][0]));
			sky.grid_cell_colors[c][1] = static_cast<float>(get_dbl(kv, pg.c_str(), sky.grid_cell_colors[c][1]));
			sky.grid_cell_colors[c][2] = static_cast<float>(get_dbl(kv, pb.c_str(), sky.grid_cell_colors[c][2]));
		}
		sky.grid_orientation_yaw_deg = get_dbl(kv, "sky_grid_orientation_yaw_deg", sky.grid_orientation_yaw_deg);
		sky.grid_orientation_pitch_deg = get_dbl(kv, "sky_grid_orientation_pitch_deg", sky.grid_orientation_pitch_deg);
		sky.grid_orientation_roll_deg = get_dbl(kv, "sky_grid_orientation_roll_deg", sky.grid_orientation_roll_deg);
		sky.cluster_halo_intensity = get_dbl(kv, "sky_cluster_halo_intensity", sky.cluster_halo_intensity);
		sky.cluster_inherit_star_profile = get_bool(kv, "sky_cluster_inherit_star_profile", sky.cluster_inherit_star_profile);
		sky.cluster_spread_factor = get_dbl(kv, "sky_cluster_spread_factor", sky.cluster_spread_factor);
		sky.galaxy_morphology_model = get_u32(kv, "sky_galaxy_morphology_model", sky.galaxy_morphology_model);
		sky.galaxy_bar_strength = get_dbl(kv, "sky_galaxy_bar_strength", sky.galaxy_bar_strength);
		sky.galaxy_dust_lane_opacity = get_dbl(kv, "sky_galaxy_dust_lane_opacity", sky.galaxy_dust_lane_opacity);
		sky.galaxy_clumpiness = get_dbl(kv, "sky_galaxy_clumpiness", sky.galaxy_clumpiness);
		sky.galaxy_disk_thickness = get_dbl(kv, "sky_galaxy_disk_thickness", sky.galaxy_disk_thickness);
		sky.galaxy_inclination_variance = get_dbl(kv, "sky_galaxy_inclination_variance", sky.galaxy_inclination_variance);
		sky.galaxy_bulge_sersic_index = get_dbl(kv, "sky_galaxy_bulge_sersic_index", sky.galaxy_bulge_sersic_index);
		sky.galaxy_weight_grand_design = get_dbl(kv, "sky_galaxy_weight_grand_design", sky.galaxy_weight_grand_design);
		sky.galaxy_weight_flocculent = get_dbl(kv, "sky_galaxy_weight_flocculent", sky.galaxy_weight_flocculent);
		sky.galaxy_weight_barred = get_dbl(kv, "sky_galaxy_weight_barred", sky.galaxy_weight_barred);
		sky.galaxy_weight_elliptical = get_dbl(kv, "sky_galaxy_weight_elliptical", sky.galaxy_weight_elliptical);
		sky.galaxy_weight_lenticular = get_dbl(kv, "sky_galaxy_weight_lenticular", sky.galaxy_weight_lenticular);
		sky.galaxy_weight_ring = get_dbl(kv, "sky_galaxy_weight_ring", sky.galaxy_weight_ring);
		sky.quasar_enabled = get_bool(kv, "sky_quasar_enabled", sky.quasar_enabled);
		sky.quasar_jet_opening_angle_deg = get_dbl(kv, "sky_quasar_jet_opening_angle_deg", sky.quasar_jet_opening_angle_deg);
		sky.quasar_jet_lorentz_factor = get_dbl(kv, "sky_quasar_jet_lorentz_factor", sky.quasar_jet_lorentz_factor);
		sky.quasar_accretion_disk_glow = get_dbl(kv, "sky_quasar_accretion_disk_glow", sky.quasar_accretion_disk_glow);
		sky.quasar_precession_angle_deg = get_dbl(kv, "sky_quasar_precession_angle_deg", sky.quasar_precession_angle_deg);
		sky.pulsar_enabled = get_bool(kv, "sky_pulsar_enabled", sky.pulsar_enabled);
		sky.pulsar_magnetic_tilt_deg = get_dbl(kv, "sky_pulsar_magnetic_tilt_deg", sky.pulsar_magnetic_tilt_deg);
		sky.pulsar_wind_nebula_scale = get_dbl(kv, "sky_pulsar_wind_nebula_scale", sky.pulsar_wind_nebula_scale);
		sky.pulsar_wind_nebula_intensity = get_dbl(kv, "sky_pulsar_wind_nebula_intensity", sky.pulsar_wind_nebula_intensity);
		sky.pulsar_frequency_spread = get_dbl(kv, "sky_pulsar_frequency_spread", sky.pulsar_frequency_spread);
		sky.snr_enabled = get_bool(kv, "sky_snr_enabled", sky.snr_enabled);
		sky.snr_asymmetry = get_dbl(kv, "sky_snr_asymmetry", sky.snr_asymmetry);
		sky.snr_filament_scale = get_dbl(kv, "sky_snr_filament_scale", sky.snr_filament_scale);
		sky.snr_interior_glow = get_dbl(kv, "sky_snr_interior_glow", sky.snr_interior_glow);
		sky.snr_color_variation = get_dbl(kv, "sky_snr_color_variation", sky.snr_color_variation);
		sky.snr_morphology_variety = get_dbl(kv, "sky_snr_morphology_variety", sky.snr_morphology_variety);
		sky.snr_shell_color[0] = static_cast<float>(get_dbl(kv, "sky_snr_shell_color_r", sky.snr_shell_color[0]));
		sky.snr_shell_color[1] = static_cast<float>(get_dbl(kv, "sky_snr_shell_color_g", sky.snr_shell_color[1]));
		sky.snr_shell_color[2] = static_cast<float>(get_dbl(kv, "sky_snr_shell_color_b", sky.snr_shell_color[2]));
		sky.snr_interior_color[0] = static_cast<float>(get_dbl(kv, "sky_snr_interior_color_r", sky.snr_interior_color[0]));
		sky.snr_interior_color[1] = static_cast<float>(get_dbl(kv, "sky_snr_interior_color_g", sky.snr_interior_color[1]));
		sky.snr_interior_color[2] = static_cast<float>(get_dbl(kv, "sky_snr_interior_color_b", sky.snr_interior_color[2]));
		sky.star_size_scale = get_dbl(kv, "sky_star_size_scale", sky.star_size_scale);
		sky.pulsar_brightness = get_dbl(kv, "sky_pulsar_brightness", sky.pulsar_brightness);
		sky.snr_size = get_dbl(kv, "sky_snr_size", sky.snr_size);
		sky.hue_shift_deg = get_dbl(kv, "sky_hue_shift_deg", sky.hue_shift_deg);
		sky.saturation = get_dbl(kv, "sky_saturation", sky.saturation);
		sky.background_color[0] = static_cast<float>(get_dbl(kv, "sky_background_color_r", sky.background_color[0]));
		sky.background_color[1] = static_cast<float>(get_dbl(kv, "sky_background_color_g", sky.background_color[1]));
		sky.background_color[2] = static_cast<float>(get_dbl(kv, "sky_background_color_b", sky.background_color[2]));
		sky.orientation_yaw_deg = get_dbl(kv, "sky_orientation_yaw_deg", sky.orientation_yaw_deg);
		sky.orientation_pitch_deg = get_dbl(kv, "sky_orientation_pitch_deg", sky.orientation_pitch_deg);
		sky.orientation_roll_deg = get_dbl(kv, "sky_orientation_roll_deg", sky.orientation_roll_deg);
	}

	void read_interface(const std::unordered_map<std::string, std::string>& kv) {
		window_log_console_open = get_bool(kv, "window_log_console_open", window_log_console_open);
		window_control_panel_open = get_bool(kv, "window_control_panel_open", window_control_panel_open);
		window_performance_open = get_bool(kv, "window_performance_open", window_performance_open);
		window_scenario_open = get_bool(kv, "window_scenario_open", window_scenario_open);
		window_telemetry_open = get_bool(kv, "window_telemetry_open", window_telemetry_open);
		window_spectrograph_open = get_bool(kv, "window_spectrograph_open", window_spectrograph_open);
		window_diagnostics_open = get_bool(kv, "window_diagnostics_open", window_diagnostics_open);
		window_body_manager_open = get_bool(kv, "window_body_manager_open", window_body_manager_open);
		window_performance_analysis_open = get_bool(kv, "window_performance_analysis_open", window_performance_analysis_open);
		window_hud_manager_open = get_bool(kv, "window_hud_manager_open", window_hud_manager_open);
		window_keybind_settings_open = get_bool(kv, "window_keybind_settings_open", window_keybind_settings_open);
		window_constants_open = get_bool(kv, "window_constants_open", window_constants_open);
		window_capture_studio_open = get_bool(kv, "window_capture_studio_open", window_capture_studio_open);

		hud_layout.master_enabled = get_bool(kv, "hud_master_enabled", hud_layout.master_enabled);
		for (size_t i = 0; i < static_cast<size_t>(UI::HudElementId::Count); ++i) {
			const std::string prefix = "hud_el_" + std::to_string(i) + "_";
			auto& style = hud_layout.element(static_cast<UI::HudElementId>(i));
			style.enabled = get_bool(kv, (prefix + "enabled").c_str(), style.enabled);
			style.anchor = static_cast<UI::HudAnchor>(get_u32(kv, (prefix + "anchor").c_str(), static_cast<uint32_t>(style.anchor)));
			style.offset_x = static_cast<float>(get_dbl(kv, (prefix + "offset_x").c_str(), style.offset_x));
			style.offset_y = static_cast<float>(get_dbl(kv, (prefix + "offset_y").c_str(), style.offset_y));
			style.scale = static_cast<float>(get_dbl(kv, (prefix + "scale").c_str(), style.scale));
			style.text_color[0] = static_cast<float>(get_dbl(kv, (prefix + "color_r").c_str(), style.text_color[0]));
			style.text_color[1] = static_cast<float>(get_dbl(kv, (prefix + "color_g").c_str(), style.text_color[1]));
			style.text_color[2] = static_cast<float>(get_dbl(kv, (prefix + "color_b").c_str(), style.text_color[2]));
			style.text_color[3] = static_cast<float>(get_dbl(kv, (prefix + "color_a").c_str(), style.text_color[3]));
			style.show_background = get_bool(kv, (prefix + "show_background").c_str(), style.show_background);
			style.background_opacity = static_cast<float>(get_dbl(kv, (prefix + "background_opacity").c_str(), style.background_opacity));
		}

		window_chrome.read(kv);
		UI::InterfacePersistence::read(kv, hud_layout, schematic_view);
		for (const auto& [key, value] : kv) {
			if (key.rfind("session_", 0) == 0) {
				session_values[key.substr(8)] = value;
			}
		}

		for (size_t i = 0; i < kMaxSecondaryViews; ++i) {
			const std::string prefix = "secview_" + std::to_string(i) + "_";
			auto& slot = secondary_views[i];
			slot.active = get_bool(kv, (prefix + "active").c_str(), slot.active);
			if (!slot.active) continue;
			slot.open = get_bool(kv, (prefix + "open").c_str(), slot.open);
			slot.name = get_str(kv, (prefix + "name").c_str(), slot.name);
			slot.radius = get_dbl(kv, (prefix + "radius").c_str(), slot.radius);
			slot.theta = get_dbl(kv, (prefix + "theta").c_str(), slot.theta);
			slot.phi = get_dbl(kv, (prefix + "phi").c_str(), slot.phi);
			slot.fov_deg = get_dbl(kv, (prefix + "fov_deg").c_str(), slot.fov_deg);
			slot.exposure = get_dbl(kv, (prefix + "exposure").c_str(), slot.exposure);
			slot.tonemapping_mode = get_i32(kv, (prefix + "tonemapping_mode").c_str(), slot.tonemapping_mode);
			slot.projection_mode = get_i32(kv, (prefix + "projection_mode").c_str(), slot.projection_mode);
			slot.max_steps = get_i32(kv, (prefix + "max_steps").c_str(), slot.max_steps);
			slot.resolution_scale = static_cast<float>(get_dbl(kv, (prefix + "resolution_scale").c_str(), slot.resolution_scale));
			slot.follow_primary = get_bool(kv, (prefix + "follow_primary").c_str(), slot.follow_primary);
			slot.follow_offset_theta = get_dbl(kv, (prefix + "follow_offset_theta").c_str(), slot.follow_offset_theta);
			slot.follow_offset_phi = get_dbl(kv, (prefix + "follow_offset_phi").c_str(), slot.follow_offset_phi);
		}

		const SettingsReader spatial_reader(kv);
		spatial_reference.read_settings(spatial_reader);
	}

	void read_units(const std::unordered_map<std::string, std::string>& kv) {
		unit_preferences.distance = static_cast<Units::DistanceUnit>(get_u32(kv, "unit_distance", static_cast<uint32_t>(unit_preferences.distance)));
		unit_preferences.velocity = static_cast<Units::VelocityUnit>(get_u32(kv, "unit_velocity", static_cast<uint32_t>(unit_preferences.velocity)));
		unit_preferences.mass = static_cast<Units::MassUnit>(get_u32(kv, "unit_mass", static_cast<uint32_t>(unit_preferences.mass)));
		unit_preferences.energy = static_cast<Units::EnergyUnit>(get_u32(kv, "unit_energy", static_cast<uint32_t>(unit_preferences.energy)));
		unit_preferences.temperature = static_cast<Units::TemperatureUnit>(get_u32(kv, "unit_temperature", static_cast<uint32_t>(unit_preferences.temperature)));
		unit_preferences.angle = static_cast<Units::AngleUnit>(get_u32(kv, "unit_angle", static_cast<uint32_t>(unit_preferences.angle)));
		unit_preferences.charge = static_cast<Units::ChargeUnit>(get_u32(kv, "unit_charge", static_cast<uint32_t>(unit_preferences.charge)));
		unit_preferences.current = static_cast<Units::CurrentUnit>(get_u32(kv, "unit_current", static_cast<uint32_t>(unit_preferences.current)));
		unit_preferences.time = static_cast<Units::TimeUnit>(get_u32(kv, "unit_time", static_cast<uint32_t>(unit_preferences.time)));
		unit_preferences.acceleration = static_cast<Units::AccelerationUnit>(get_u32(kv, "unit_acceleration", static_cast<uint32_t>(unit_preferences.acceleration)));
		unit_preferences.angular_velocity = static_cast<Units::AngularVelocityUnit>(get_u32(kv, "unit_angular_velocity", static_cast<uint32_t>(unit_preferences.angular_velocity)));
		unit_preferences.density = static_cast<Units::DensityUnit>(get_u32(kv, "unit_density", static_cast<uint32_t>(unit_preferences.density)));
		unit_preferences.pressure = static_cast<Units::PressureUnit>(get_u32(kv, "unit_pressure", static_cast<uint32_t>(unit_preferences.pressure)));
		unit_preferences.power = static_cast<Units::PowerUnit>(get_u32(kv, "unit_power", static_cast<uint32_t>(unit_preferences.power)));
		unit_preferences.frequency = static_cast<Units::FrequencyUnit>(get_u32(kv, "unit_frequency", static_cast<uint32_t>(unit_preferences.frequency)));
		unit_preferences.force = static_cast<Units::ForceUnit>(get_u32(kv, "unit_force", static_cast<uint32_t>(unit_preferences.force)));
		unit_preferences.magnetic_field = static_cast<Units::MagneticFieldUnit>(get_u32(kv, "unit_magnetic_field", static_cast<uint32_t>(unit_preferences.magnetic_field)));
		unit_preferences.voltage = static_cast<Units::VoltageUnit>(get_u32(kv, "unit_voltage", static_cast<uint32_t>(unit_preferences.voltage)));
		unit_preferences.frame_rate = static_cast<Units::FrameRateUnit>(get_u32(kv, "unit_frame_rate", static_cast<uint32_t>(unit_preferences.frame_rate)));
		coordinate_preferences.widget_default = Observer::coordinate_system_from_index(get_u32(kv, "coord_widget_default", static_cast<uint32_t>(coordinate_preferences.widget_default)));
	}

	void read_physics(const std::unordered_map<std::string, std::string>& kv) {
		constants_preset = get_u32(kv, "constants_preset", constants_preset);
		constants_c = get_dbl(kv, "constants_c", constants_c);
		constants_g = get_dbl(kv, "constants_g", constants_g);
		constants_h = get_dbl(kv, "constants_h", constants_h);
		constants_kb = get_dbl(kv, "constants_kb", constants_kb);
		constants_na = get_dbl(kv, "constants_na", constants_na);
		constants_ke = get_dbl(kv, "constants_ke", constants_ke);
		constants_kcd = get_dbl(kv, "constants_kcd", constants_kcd);
		interaction_electricity_enabled = get_bool(kv, "interaction_electricity_enabled", interaction_electricity_enabled);
		interaction_magnetism_enabled = get_bool(kv, "interaction_magnetism_enabled", interaction_magnetism_enabled);
		interaction_vacuum_permittivity = get_dbl(kv, "interaction_vacuum_permittivity", interaction_vacuum_permittivity);
		interaction_vacuum_permeability = get_dbl(kv, "interaction_vacuum_permeability", interaction_vacuum_permeability);
		interaction_collisions_enabled = get_bool(kv, "interaction_collisions_enabled", interaction_collisions_enabled);
		interaction_collision_response_model = get_u32(kv, "interaction_collision_response_model", interaction_collision_response_model);
		interaction_collision_consider_rotation = get_bool(kv, "interaction_collision_consider_rotation", interaction_collision_consider_rotation);
		interaction_collision_consider_friction = get_bool(kv, "interaction_collision_consider_friction", interaction_collision_consider_friction);
		interaction_collision_restitution_multiplier = get_dbl(kv, "interaction_collision_restitution_multiplier", interaction_collision_restitution_multiplier);
		interaction_thermodynamics_enabled = get_bool(kv, "interaction_thermodynamics_enabled", interaction_thermodynamics_enabled);
		interaction_ambient_temperature = get_dbl(kv, "interaction_ambient_temperature", interaction_ambient_temperature);
		interaction_radiative_coupling_scale = get_dbl(kv, "interaction_radiative_coupling_scale", interaction_radiative_coupling_scale);
		interaction_fragmentation_enabled = get_bool(kv, "interaction_fragmentation_enabled", interaction_fragmentation_enabled);
		interaction_minimum_fragment_mass = get_dbl(kv, "interaction_minimum_fragment_mass", interaction_minimum_fragment_mass);
		interaction_fragmentation_max_fragments = get_u32(kv, "interaction_fragmentation_max_fragments", interaction_fragmentation_max_fragments);
		interaction_collision_energy_to_integrity_loss = get_dbl(kv, "interaction_collision_energy_to_integrity_loss", interaction_collision_energy_to_integrity_loss);
		interaction_annihilation_enabled = get_bool(kv, "interaction_annihilation_enabled", interaction_annihilation_enabled);
		interaction_annihilation_contact_scale = get_dbl(kv, "interaction_annihilation_contact_scale", interaction_annihilation_contact_scale);
		interaction_annihilation_require_opposite_charge = get_bool(kv, "interaction_annihilation_require_opposite_charge", interaction_annihilation_require_opposite_charge);
		interaction_collision_stiffness_scale = get_dbl(kv, "interaction_collision_stiffness_scale", interaction_collision_stiffness_scale);
		interaction_collision_position_correction_factor = get_dbl(kv, "interaction_collision_position_correction_factor", interaction_collision_position_correction_factor);
		interaction_fragmentation_tidal_stress_enabled = get_bool(kv, "interaction_fragmentation_tidal_stress_enabled", interaction_fragmentation_tidal_stress_enabled);
	}

	void read_bodies(const std::unordered_map<std::string, std::string>& kv) {
		system_preset_selected = get_str(kv, "system_preset_selected", system_preset_selected);
		body_randomizer.read_settings(kv);
		body_manager_sort_mode = get_u32(kv, "body_manager_sort_mode", body_manager_sort_mode);
		body_manager_sort_descending = get_bool(kv, "body_manager_sort_descending", body_manager_sort_descending);
		body_manager_search_filter = get_str(kv, "body_manager_search_filter", body_manager_search_filter);
		body_manager_list_pane_width = static_cast<float>(get_dbl(kv, "body_manager_list_pane_width", body_manager_list_pane_width));
		body_manager_source_sort_mode = get_u32(kv, "body_manager_source_sort_mode", body_manager_source_sort_mode);
		body_manager_source_sort_descending = get_bool(kv, "body_manager_source_sort_descending", body_manager_source_sort_descending);
		body_manager_source_search_filter = get_str(kv, "body_manager_source_search_filter", body_manager_source_search_filter);
		body_manager_source_list_pane_width = static_cast<float>(get_dbl(kv, "body_manager_source_list_pane_width", body_manager_source_list_pane_width));
		body_manager_bulk_parameter_index = get_u32(kv, "body_manager_bulk_parameter_index", body_manager_bulk_parameter_index);
		body_manager_new_body_mass_log_mode = get_bool(kv, "body_manager_new_body_mass_log_mode", body_manager_new_body_mass_log_mode);
		body_manager_new_body_radius_log_mode = get_bool(kv, "body_manager_new_body_radius_log_mode", body_manager_new_body_radius_log_mode);
		body_manager_new_source_mass_log_mode = get_bool(kv, "body_manager_new_source_mass_log_mode", body_manager_new_source_mass_log_mode);
		body_manager_reference_body_id = get_i32(kv, "body_manager_reference_body_id", body_manager_reference_body_id);
		body_manager_relative_coords_mode = get_bool(kv, "body_manager_relative_coords_mode", body_manager_relative_coords_mode);

		body_sec_identity_open = get_bool(kv, "body_sec_identity_open", body_sec_identity_open);
		body_sec_motion_open = get_bool(kv, "body_sec_motion_open", body_sec_motion_open);
		body_sec_orbital_plane_open = get_bool(kv, "body_sec_orbital_plane_open", body_sec_orbital_plane_open);
		body_sec_frenet_open = get_bool(kv, "body_sec_frenet_open", body_sec_frenet_open);
		body_sec_orientation_open = get_bool(kv, "body_sec_orientation_open", body_sec_orientation_open);
		body_sec_multipoles_open = get_bool(kv, "body_sec_multipoles_open", body_sec_multipoles_open);
		body_sec_material_open = get_bool(kv, "body_sec_material_open", body_sec_material_open);
		body_sec_surface_open = get_bool(kv, "body_sec_surface_open", body_sec_surface_open);
		body_sec_accretion_disk_open = get_bool(kv, "body_sec_accretion_disk_open", body_sec_accretion_disk_open);
		body_creation_sec_template_open = get_bool(kv, "body_creation_sec_template_open", body_creation_sec_template_open);
		body_creation_sec_comparator_open = get_bool(kv, "body_creation_sec_comparator_open", body_creation_sec_comparator_open);
		body_creation_sec_plane_open = get_bool(kv, "body_creation_sec_plane_open", body_creation_sec_plane_open);
		body_creation_sec_hazard_open = get_bool(kv, "body_creation_sec_hazard_open", body_creation_sec_hazard_open);
		source_creation_sec_template_open = get_bool(kv, "source_creation_sec_template_open", source_creation_sec_template_open);
		source_creation_sec_comparator_open = get_bool(kv, "source_creation_sec_comparator_open", source_creation_sec_comparator_open);
		source_creation_sec_plane_open = get_bool(kv, "source_creation_sec_plane_open", source_creation_sec_plane_open);
		source_creation_sec_hazard_open = get_bool(kv, "source_creation_sec_hazard_open", source_creation_sec_hazard_open);
		sys_dyn_sec_bulk_actions_open = get_bool(kv, "sys_dyn_sec_bulk_actions_open", sys_dyn_sec_bulk_actions_open);
		sys_dyn_sec_plane_align_open = get_bool(kv, "sys_dyn_sec_plane_align_open", sys_dyn_sec_plane_align_open);
		sys_dyn_sec_generalized_open = get_bool(kv, "sys_dyn_sec_generalized_open", sys_dyn_sec_generalized_open);
		sys_dyn_sec_sources_open = get_bool(kv, "sys_dyn_sec_sources_open", sys_dyn_sec_sources_open);
		interactions_sec_em_open = get_bool(kv, "interactions_sec_em_open", interactions_sec_em_open);
		interactions_sec_collisions_open = get_bool(kv, "interactions_sec_collisions_open", interactions_sec_collisions_open);
		interactions_sec_thermo_open = get_bool(kv, "interactions_sec_thermo_open", interactions_sec_thermo_open);
		interactions_sec_frag_open = get_bool(kv, "interactions_sec_frag_open", interactions_sec_frag_open);
		interactions_sec_annihilation_open = get_bool(kv, "interactions_sec_annihilation_open", interactions_sec_annihilation_open);
		catalog_filter_ranges_open = get_bool(kv, "catalog_filter_ranges_open", catalog_filter_ranges_open);
	}

	void read_capture(const std::unordered_map<std::string, std::string>& kv) {
		screenshot_output_directory = get_str(kv, "screenshot_output_directory", screenshot_output_directory);
		screenshot_filename_pattern = get_str(kv, "screenshot_filename_pattern", screenshot_filename_pattern);
		screenshot_format = get_u32(kv, "screenshot_format", screenshot_format);
		screenshot_watermark_enabled = get_bool(kv, "screenshot_watermark_enabled", screenshot_watermark_enabled);
		screenshot_watermark_text = get_str(kv, "screenshot_watermark_text", screenshot_watermark_text);
		screenshot_overwrite_policy = get_u32(kv, "screenshot_overwrite_policy", screenshot_overwrite_policy);
	}

	void read_cluster(const std::unordered_map<std::string, std::string>& kv) {
		cluster_enabled = get_bool(kv, "cluster_enabled", cluster_enabled);
		cluster_node_role = get_u32(kv, "cluster_node_role", cluster_node_role);
		cluster_listen_port = get_u32(kv, "cluster_listen_port", cluster_listen_port);
		cluster_discovery_multicast_enabled = get_bool(kv, "cluster_discovery_multicast_enabled", cluster_discovery_multicast_enabled);
		cluster_dynamic_load_balancing = get_bool(kv, "cluster_dynamic_load_balancing", cluster_dynamic_load_balancing);
		cluster_latency_penalty_alpha = get_dbl(kv, "cluster_latency_penalty_alpha", cluster_latency_penalty_alpha);
		cluster_work_stealing_enabled = get_bool(kv, "cluster_work_stealing_enabled", cluster_work_stealing_enabled);
		cluster_tile_timeout_ms = get_u32(kv, "cluster_tile_timeout_ms", cluster_tile_timeout_ms);
		cluster_compression_mode = get_u32(kv, "cluster_compression_mode", cluster_compression_mode);
		cluster_require_encryption = get_bool(kv, "cluster_require_encryption", cluster_require_encryption);
		cluster_shared_token_hex = get_str(kv, "cluster_shared_token_hex", cluster_shared_token_hex);
		cluster_route_interactive_viewport = get_bool(kv, "cluster_route_interactive_viewport", cluster_route_interactive_viewport);
		cluster_route_capture_studio = get_bool(kv, "cluster_route_capture_studio", cluster_route_capture_studio);
		cluster_route_monte_carlo_ensembles = get_bool(kv, "cluster_route_monte_carlo_ensembles", cluster_route_monte_carlo_ensembles);
		cluster_route_nbody_interactions = get_bool(kv, "cluster_route_nbody_interactions", cluster_route_nbody_interactions);
		cluster_route_exact_spacetime_metrics = get_bool(kv, "cluster_route_exact_spacetime_metrics", cluster_route_exact_spacetime_metrics);
	}

	void write_general(std::ostream& out) const {
		out << "format_version=" << format_version << "\n";
		out << "load_policy=" << static_cast<uint32_t>(load_policy) << "\n";
		out << "default_performance_preset=" << default_performance_preset << "\n";
		out << "default_use_gpu_compute=" << (default_use_gpu_compute ? 1 : 0) << "\n";
		out << "load_scenario_on_startup=" << (load_scenario_on_startup ? 1 : 0) << "\n";
		out << "default_scenario_path=" << default_scenario_path << "\n";
		out << "show_system_console=" << (show_system_console ? 1 : 0) << "\n";
		out << "last_window_layout=" << last_window_layout << "\n";
		out << "multi_window_mode=" << (multi_window_mode ? 1 : 0) << "\n";
	}

	void write_camera(std::ostream& out) const {
		out << "default_camera_mode=" << default_camera_mode << "\n";
		out << "cam_ff_forward_speed=" << camera_controls.free_fly.forward_speed << "\n";
		out << "cam_ff_lateral_speed=" << camera_controls.free_fly.lateral_speed << "\n";
		out << "cam_ff_vertical_speed=" << camera_controls.free_fly.vertical_speed << "\n";
		out << "cam_ff_invert_vertical=" << (camera_controls.free_fly.invert_vertical ? 1 : 0) << "\n";
		out << "cam_ff_invert_forward=" << (camera_controls.free_fly.invert_forward ? 1 : 0) << "\n";
		out << "cam_ff_invert_lateral=" << (camera_controls.free_fly.invert_lateral ? 1 : 0) << "\n";
		out << "cam_ff_invert_mouse_y=" << (camera_controls.free_fly.invert_mouse_y ? 1 : 0) << "\n";
		out << "cam_ff_invert_mouse_x=" << (camera_controls.free_fly.invert_mouse_x ? 1 : 0) << "\n";
		out << "cam_ff_mouse_sensitivity=" << camera_controls.free_fly.mouse_sensitivity << "\n";
		out << "cam_ff_sprint_multiplier=" << camera_controls.free_fly.sprint_multiplier << "\n";
		out << "cam_ff_crawl_multiplier=" << camera_controls.free_fly.crawl_multiplier << "\n";
		out << "cam_ff_ignore_pitch_roll=" << (camera_controls.free_fly.ignore_pitch_roll_for_movement ? 1 : 0) << "\n";

		out << "cam_zoom_center_on_cursor=" << (camera_controls.zoom.zoom_center_on_cursor ? 1 : 0) << "\n";
		out << "cam_zoom_scroll_sensitivity=" << camera_controls.zoom.zoom_scroll_sensitivity << "\n";
		out << "cam_zoom_max=" << camera_controls.zoom.max_zoom << "\n";
		out << "cam_zoom_min=" << camera_controls.zoom.min_zoom << "\n";

		out << "cam_orbit_distance_speed=" << camera_controls.orbit.orbit_distance_speed << "\n";
		out << "cam_orbit_pitch_speed=" << camera_controls.orbit.pitch_speed_deg_s << "\n";
		out << "cam_orbit_yaw_speed=" << camera_controls.orbit.yaw_speed_deg_s << "\n";
		out << "cam_orbit_invert_pitch=" << (camera_controls.orbit.invert_pitch ? 1 : 0) << "\n";
		out << "cam_rocket_main_thrust=" << camera_controls.rocket.main_thrust_accel << "\n";
		out << "cam_rocket_lateral_thrust=" << camera_controls.rocket.lateral_thrust_accel << "\n";
		out << "cam_rocket_vertical_thrust=" << camera_controls.rocket.vertical_thrust_accel << "\n";
		out << "cam_rocket_angular_rate=" << camera_controls.rocket.angular_rate_deg_s << "\n";
		out << "cam_rocket_invert_vertical=" << (camera_controls.rocket.invert_vertical ? 1 : 0) << "\n";
		out << "cam_rocket_invert_lateral=" << (camera_controls.rocket.invert_lateral ? 1 : 0) << "\n";
		out << "cam_rocket_requires_time=" << (camera_controls.rocket.requires_time_running ? 1 : 0) << "\n";
		out << "cam_walk_scale_with_body=" << (camera_controls.surface_walk.scale_with_body ? 1 : 0) << "\n";
		out << "cam_walk_height=" << camera_controls.surface_walk.walker_height << "\n";
		out << "cam_walk_eye_fraction=" << camera_controls.surface_walk.eye_height_fraction << "\n";
		out << "cam_walk_crouch_fraction=" << camera_controls.surface_walk.crouch_fraction << "\n";
		out << "cam_walk_crouch_speed=" << camera_controls.surface_walk.crouch_speed_multiplier << "\n";
		out << "cam_walk_speed=" << camera_controls.surface_walk.walk_speed << "\n";
		out << "cam_walk_sprint=" << camera_controls.surface_walk.sprint_multiplier << "\n";
		out << "cam_walk_crawl=" << camera_controls.surface_walk.crawl_multiplier << "\n";
		out << "cam_walk_step_length=" << camera_controls.surface_walk.step_length << "\n";
		out << "cam_walk_jump_height=" << camera_controls.surface_walk.jump_height << "\n";
		out << "cam_walk_auto_gravity=" << (camera_controls.surface_walk.automatic_gravity ? 1 : 0) << "\n";
		out << "cam_walk_gravity_scale=" << camera_controls.surface_walk.gravity_scale << "\n";
		out << "cam_walk_manual_gravity=" << camera_controls.surface_walk.manual_gravity << "\n";
		out << "cam_walk_max_airtime=" << camera_controls.surface_walk.maximum_airtime_seconds << "\n";
		out << "cam_walk_ground_response=" << camera_controls.surface_walk.ground_response_seconds << "\n";
		out << "cam_walk_air_control=" << camera_controls.surface_walk.air_control << "\n";
		out << "cam_walk_head_bob=" << (camera_controls.surface_walk.head_bob_enabled ? 1 : 0) << "\n";
		out << "cam_walk_head_bob_amplitude=" << camera_controls.surface_walk.head_bob_amplitude << "\n";
		out << "cam_walk_follow_rotation=" << (camera_controls.surface_walk.follow_surface_rotation ? 1 : 0) << "\n";
		out << "cam_walk_max_look_pitch=" << camera_controls.surface_walk.maximum_look_pitch_deg << "\n";
		out << "cam_walk_max_drop_heights=" << camera_controls.surface_walk.maximum_drop_heights << "\n";
		out << "cam_planet_speed=" << camera_controls.planet_orbit.angular_speed_deg_s << "\n";
		out << "cam_planet_drag=" << camera_controls.planet_orbit.drag_sensitivity_deg_per_pixel << "\n";
		out << "cam_planet_zoom_rate=" << camera_controls.planet_orbit.zoom_rate << "\n";
		out << "cam_planet_wheel=" << camera_controls.planet_orbit.wheel_zoom_factor << "\n";
		out << "cam_planet_min_altitude=" << camera_controls.planet_orbit.minimum_altitude_ratio << "\n";
		out << "cam_planet_max_altitude=" << camera_controls.planet_orbit.maximum_altitude_ratio << "\n";
		out << "cam_planet_smoothing=" << camera_controls.planet_orbit.smoothing_seconds << "\n";
		out << "cam_planet_max_pitch=" << camera_controls.planet_orbit.maximum_pitch_deg << "\n";
		out << "cam_planet_sprint=" << camera_controls.planet_orbit.sprint_multiplier << "\n";
		out << "cam_planet_crawl=" << camera_controls.planet_orbit.crawl_multiplier << "\n";
		out << "cam_planet_invert_drag=" << (camera_controls.planet_orbit.invert_drag ? 1 : 0) << "\n";
		out << "cam_keyboard_layout=" << static_cast<uint32_t>(camera_controls.keyboard_layout) << "\n";
		for (size_t i = 0; i < static_cast<size_t>(UI::InputAction::Count); ++i) {
			const auto& binding = camera_controls.keybinds.get(static_cast<UI::InputAction>(i));
			out << "kb_" << i << "_primary=" << binding.primary_key << "\n";
			out << "kb_" << i << "_secondary=" << binding.secondary_key << "\n";
			out << "kb_" << i << "_mode=" << static_cast<uint32_t>(binding.mode) << "\n";
		}
	}

	void write_render_sky(std::ostream& out) const {
		out << "sky_rendering_enabled=" << (sky_rendering_enabled ? 1 : 0) << "\n";
		out << "sky_seed_locked=" << (sky_seed_locked ? 1 : 0) << "\n";
		out << "sky_seed=" << sky.seed << "\n";
		out << "sky_master_intensity=" << sky.master_intensity << "\n";
		out << "sky_quality_factor=" << sky.quality_factor << "\n";
		out << "sky_adaptive_fov_lod=" << (sky.adaptive_fov_lod ? 1 : 0) << "\n";
		out << "sky_adaptive_fov_quality=" << (sky.adaptive_fov_quality ? 1 : 0) << "\n";
		out << "sky_adaptive_fov_reference=" << sky.adaptive_fov_reference << "\n";
		out << "sky_adaptive_fov_sensitivity=" << sky.adaptive_fov_sensitivity << "\n";
		out << "sky_max_star_octaves=" << sky.max_star_octaves << "\n";
		out << "sky_star_quality_factor=" << sky.star_quality_factor << "\n";
		out << "sky_nebula_quality_factor=" << sky.nebula_quality_factor << "\n";
		out << "sky_cluster_quality_factor=" << sky.cluster_quality_factor << "\n";
		out << "sky_dust_quality_factor=" << sky.dust_quality_factor << "\n";
		out << "sky_dynamic_motion=" << (sky.dynamic_motion ? 1 : 0) << "\n";
		out << "sky_dynamic_rotation=" << (sky.dynamic_rotation ? 1 : 0) << "\n";
		out << "sky_motion_speed=" << sky.motion_speed << "\n";
		out << "sky_rotation_rate_deg_s=" << sky.rotation_rate_deg_s << "\n";
		out << "sky_stars_enabled=" << (sky.stars_enabled ? 1 : 0) << "\n";
		out << "sky_star_profile_model=" << sky.star_profile_model << "\n";
		out << "sky_star_profile_param=" << sky.star_profile_param << "\n";
		out << "sky_star_spike_count=" << sky.star_spike_count << "\n";
		out << "sky_star_spike_length=" << sky.star_spike_length << "\n";
		out << "sky_star_spike_intensity=" << sky.star_spike_intensity << "\n";
		out << "sky_star_spike_angle_deg=" << sky.star_spike_angle_deg << "\n";
		out << "sky_star_profile_softness=" << sky.star_profile_softness << "\n";
		out << "sky_star_placement_model=" << sky.star_placement_model << "\n";
		out << "sky_star_galactic_concentration=" << sky.star_galactic_concentration << "\n";
		out << "sky_nebula_color_separation=" << sky.nebula_color_separation << "\n";
		out << "sky_nebula_ionization_glow=" << sky.nebula_ionization_glow << "\n";
		out << "sky_nebula_gas_contrast=" << sky.nebula_gas_contrast << "\n";
		out << "sky_cluster_placement_model=" << sky.cluster_placement_model << "\n";
		out << "sky_cluster_halo_radius=" << sky.cluster_halo_radius << "\n";
		out << "sky_cluster_plane_scale_height=" << sky.cluster_plane_scale_height << "\n";
		out << "sky_cluster_stars_enabled=" << (sky.cluster_stars_enabled ? 1 : 0) << "\n";
		out << "sky_cluster_member_count=" << sky.cluster_member_count << "\n";
		out << "sky_cluster_member_size=" << sky.cluster_member_size << "\n";
		out << "sky_cluster_member_brightness=" << sky.cluster_member_brightness << "\n";
		out << "sky_cluster_star_profile_model=" << sky.cluster_star_profile_model << "\n";
		out << "sky_star_profile_param2=" << sky.star_profile_param2 << "\n";
		out << "sky_star_dirac_concentration=" << sky.star_dirac_concentration << "\n";
		out << "sky_star_bloom_radius=" << sky.star_bloom_radius << "\n";
		out << "sky_star_bloom_intensity=" << sky.star_bloom_intensity << "\n";
		out << "sky_quasar_quality_factor=" << sky.quasar_quality_factor << "\n";
		out << "sky_pulsar_quality_factor=" << sky.pulsar_quality_factor << "\n";
		out << "sky_snr_quality_factor=" << sky.snr_quality_factor << "\n";
		out << "sky_quasar_jet_length=" << sky.quasar_jet_length << "\n";
		out << "sky_quasar_jet_intensity=" << sky.quasar_jet_intensity << "\n";
		out << "sky_pulsar_beam_width_deg=" << sky.pulsar_beam_width_deg << "\n";
		out << "sky_snr_expansion_age=" << sky.snr_expansion_age << "\n";
		out << "sky_grid_show_ticks=" << (sky.grid_show_ticks ? 1 : 0) << "\n";
		out << "sky_grid_tick_interval_deg=" << sky.grid_tick_interval_deg << "\n";
		out << "sky_grid_tick_length=" << sky.grid_tick_length << "\n";
		out << "sky_star_density=" << sky.star_density << "\n";
		out << "sky_star_brightness=" << sky.star_brightness << "\n";
		out << "sky_star_size_min=" << sky.star_size_min << "\n";
		out << "sky_star_size_max=" << sky.star_size_max << "\n";
		out << "sky_star_size_distribution=" << sky.star_size_distribution << "\n";
		out << "sky_star_size_param=" << sky.star_size_param << "\n";
		out << "sky_star_lum_distribution=" << sky.star_luminosity_distribution << "\n";
		out << "sky_star_lum_param=" << sky.star_luminosity_param << "\n";
		out << "sky_star_color_variation=" << sky.star_color_variation << "\n";
		out << "sky_star_temperature_bias=" << sky.star_temperature_bias << "\n";
		out << "sky_star_brightness_variation=" << sky.star_brightness_variation << "\n";
		out << "sky_star_scintillation=" << sky.star_scintillation << "\n";
		out << "sky_star_magnitude_limit=" << sky.star_magnitude_limit << "\n";
		out << "sky_nebula_enabled=" << (sky.nebula_enabled ? 1 : 0) << "\n";
		out << "sky_nebula_intensity=" << sky.nebula_intensity << "\n";
		out << "sky_nebula_roughness=" << sky.nebula_roughness << "\n";
		out << "sky_nebula_octaves=" << sky.nebula_octaves << "\n";
		out << "sky_nebula_frequency=" << sky.nebula_frequency << "\n";
		out << "sky_nebula_plane_thickness=" << sky.nebula_plane_thickness << "\n";
		out << "sky_nebula_plane_falloff=" << sky.nebula_plane_falloff << "\n";
		out << "sky_nebula_asymmetry=" << sky.nebula_asymmetry << "\n";
		out << "sky_nebula_absorption_density=" << sky.nebula_absorption_density << "\n";
		out << "sky_nebula_layer_count=" << sky.nebula_layer_count << "\n";
		out << "sky_nebula_layer_spread=" << sky.nebula_layer_spread << "\n";
		out << "sky_nebula_core_glow=" << sky.nebula_core_glow << "\n";
		out << "sky_nebula_curl_strength=" << sky.nebula_curl_strength << "\n";
		out << "sky_galaxies_enabled=" << (sky.galaxies_enabled ? 1 : 0) << "\n";
		out << "sky_galaxy_density=" << sky.galaxy_density << "\n";
		out << "sky_galaxy_brightness=" << sky.galaxy_brightness << "\n";
		out << "sky_galaxy_size_scale=" << sky.galaxy_size_scale << "\n";
		out << "sky_galaxy_size_min=" << sky.galaxy_size_min << "\n";
		out << "sky_galaxy_size_max=" << sky.galaxy_size_max << "\n";
		out << "sky_galaxy_size_distribution=" << sky.galaxy_size_distribution << "\n";
		out << "sky_galaxy_size_param=" << sky.galaxy_size_param << "\n";
		out << "sky_galaxy_spiral_arms=" << sky.galaxy_spiral_arms << "\n";
		out << "sky_galaxy_pitch_angle=" << sky.galaxy_pitch_angle << "\n";
		out << "sky_galaxy_core_ratio=" << sky.galaxy_core_ratio << "\n";
		out << "sky_galaxy_ellipticity_variance=" << sky.galaxy_ellipticity_variance << "\n";
		out << "sky_clusters_enabled=" << (sky.clusters_enabled ? 1 : 0) << "\n";
		out << "sky_cluster_density=" << sky.cluster_density << "\n";
		out << "sky_cluster_brightness=" << sky.cluster_brightness << "\n";
		out << "sky_cluster_size_scale=" << sky.cluster_size_scale << "\n";
		out << "sky_cluster_size_min=" << sky.cluster_size_min << "\n";
		out << "sky_cluster_size_max=" << sky.cluster_size_max << "\n";
		out << "sky_cluster_size_distribution=" << sky.cluster_size_distribution << "\n";
		out << "sky_cluster_concentration=" << sky.cluster_concentration << "\n";
		out << "sky_cluster_star_density_boost=" << sky.cluster_star_density_boost << "\n";
		out << "sky_dust_enabled=" << (sky.dust_enabled ? 1 : 0) << "\n";
		out << "sky_dust_density=" << sky.dust_density << "\n";
		out << "sky_dust_intensity=" << sky.dust_intensity << "\n";
		out << "sky_dust_scale=" << sky.dust_scale << "\n";
		out << "sky_dust_absorption_ratio=" << sky.dust_absorption_ratio << "\n";
		out << "sky_dust_fractal_octaves=" << sky.dust_fractal_octaves << "\n";
		out << "sky_dust_contrast=" << sky.dust_contrast << "\n";
		out << "sky_quasar_density=" << sky.quasar_density << "\n";
		out << "sky_quasar_brightness=" << sky.quasar_brightness << "\n";
		out << "sky_quasar_core_size=" << sky.quasar_core_size << "\n";
		out << "sky_pulsar_pulse_rate=" << sky.pulsar_pulse_rate << "\n";
		out << "sky_pulsar_pulse_duty=" << sky.pulsar_pulse_duty << "\n";
		out << "sky_grid_enabled=" << (sky.grid_enabled ? 1 : 0) << "\n";
		out << "sky_grid_opacity=" << sky.grid_opacity << "\n";
		out << "sky_grid_line_width=" << sky.grid_line_width << "\n";
		out << "sky_grid_spacing_deg=" << sky.grid_spacing_deg << "\n";
		out << "sky_grid_subdivisions=" << sky.grid_subdivisions << "\n";
		out << "sky_grid_glow=" << sky.grid_glow << "\n";
		out << "sky_grid_major_color_r=" << sky.grid_major_color[0] << "\n";
		out << "sky_grid_major_color_g=" << sky.grid_major_color[1] << "\n";
		out << "sky_grid_major_color_b=" << sky.grid_major_color[2] << "\n";
		out << "sky_grid_minor_color_r=" << sky.grid_minor_color[0] << "\n";
		out << "sky_grid_minor_color_g=" << sky.grid_minor_color[1] << "\n";
		out << "sky_grid_minor_color_b=" << sky.grid_minor_color[2] << "\n";
		out << "sky_grid_show_polar_caps=" << (sky.grid_show_polar_caps ? 1 : 0) << "\n";
		out << "sky_grid_show_equator=" << (sky.grid_show_equator ? 1 : 0) << "\n";
		out << "sky_grid_show_meridians=" << (sky.grid_show_meridians ? 1 : 0) << "\n";
		out << "sky_grid_cell_fill_mode=" << sky.grid_cell_fill_mode << "\n";
		out << "sky_grid_cell_opacity=" << sky.grid_cell_opacity << "\n";
		out << "sky_grid_tick_width=" << sky.grid_tick_width << "\n";
		out << "sky_grid_equator_width=" << sky.grid_equator_width << "\n";
		out << "sky_grid_equator_opacity=" << sky.grid_equator_opacity << "\n";
		out << "sky_grid_polar_width=" << sky.grid_polar_width << "\n";
		out << "sky_grid_polar_opacity=" << sky.grid_polar_opacity << "\n";
		out << "sky_grid_meridian_width=" << sky.grid_meridian_width << "\n";
		out << "sky_grid_meridian_opacity=" << sky.grid_meridian_opacity << "\n";
		out << "sky_grid_major_width=" << sky.grid_major_width << "\n";
		out << "sky_grid_minor_width=" << sky.grid_minor_width << "\n";
		out << "sky_grid_equator_color_r=" << sky.grid_equator_color[0] << "\n";
		out << "sky_grid_equator_color_g=" << sky.grid_equator_color[1] << "\n";
		out << "sky_grid_equator_color_b=" << sky.grid_equator_color[2] << "\n";
		out << "sky_grid_polar_color_r=" << sky.grid_polar_color[0] << "\n";
		out << "sky_grid_polar_color_g=" << sky.grid_polar_color[1] << "\n";
		out << "sky_grid_polar_color_b=" << sky.grid_polar_color[2] << "\n";
		out << "sky_grid_meridian_color_r=" << sky.grid_meridian_color[0] << "\n";
		out << "sky_grid_meridian_color_g=" << sky.grid_meridian_color[1] << "\n";
		out << "sky_grid_meridian_color_b=" << sky.grid_meridian_color[2] << "\n";
		out << "sky_grid_tick_color_r=" << sky.grid_tick_color[0] << "\n";
		out << "sky_grid_tick_color_g=" << sky.grid_tick_color[1] << "\n";
		out << "sky_grid_tick_color_b=" << sky.grid_tick_color[2] << "\n";
		out << "sky_grid_cell_color_count=" << sky.grid_cell_color_count << "\n";
		for (size_t c = 0; c < Render::ProceduralSkySettings::kMaxGridCellColors; ++c) {
			out << "sky_grid_cell_color_" << c << "_r=" << sky.grid_cell_colors[c][0] << "\n";
			out << "sky_grid_cell_color_" << c << "_g=" << sky.grid_cell_colors[c][1] << "\n";
			out << "sky_grid_cell_color_" << c << "_b=" << sky.grid_cell_colors[c][2] << "\n";
		}
		out << "sky_grid_orientation_yaw_deg=" << sky.grid_orientation_yaw_deg << "\n";
		out << "sky_grid_orientation_pitch_deg=" << sky.grid_orientation_pitch_deg << "\n";
		out << "sky_grid_orientation_roll_deg=" << sky.grid_orientation_roll_deg << "\n";
		out << "sky_cluster_halo_intensity=" << sky.cluster_halo_intensity << "\n";
		out << "sky_cluster_inherit_star_profile=" << (sky.cluster_inherit_star_profile ? 1 : 0) << "\n";
		out << "sky_cluster_spread_factor=" << sky.cluster_spread_factor << "\n";
		out << "sky_galaxy_morphology_model=" << sky.galaxy_morphology_model << "\n";
		out << "sky_galaxy_bar_strength=" << sky.galaxy_bar_strength << "\n";
		out << "sky_galaxy_dust_lane_opacity=" << sky.galaxy_dust_lane_opacity << "\n";
		out << "sky_galaxy_clumpiness=" << sky.galaxy_clumpiness << "\n";
		out << "sky_galaxy_disk_thickness=" << sky.galaxy_disk_thickness << "\n";
		out << "sky_galaxy_inclination_variance=" << sky.galaxy_inclination_variance << "\n";
		out << "sky_galaxy_bulge_sersic_index=" << sky.galaxy_bulge_sersic_index << "\n";
		out << "sky_galaxy_weight_grand_design=" << sky.galaxy_weight_grand_design << "\n";
		out << "sky_galaxy_weight_flocculent=" << sky.galaxy_weight_flocculent << "\n";
		out << "sky_galaxy_weight_barred=" << sky.galaxy_weight_barred << "\n";
		out << "sky_galaxy_weight_elliptical=" << sky.galaxy_weight_elliptical << "\n";
		out << "sky_galaxy_weight_lenticular=" << sky.galaxy_weight_lenticular << "\n";
		out << "sky_galaxy_weight_ring=" << sky.galaxy_weight_ring << "\n";
		out << "sky_quasar_enabled=" << (sky.quasar_enabled ? 1 : 0) << "\n";
		out << "sky_quasar_jet_opening_angle_deg=" << sky.quasar_jet_opening_angle_deg << "\n";
		out << "sky_quasar_jet_lorentz_factor=" << sky.quasar_jet_lorentz_factor << "\n";
		out << "sky_quasar_accretion_disk_glow=" << sky.quasar_accretion_disk_glow << "\n";
		out << "sky_quasar_precession_angle_deg=" << sky.quasar_precession_angle_deg << "\n";
		out << "sky_pulsar_enabled=" << (sky.pulsar_enabled ? 1 : 0) << "\n";
		out << "sky_pulsar_magnetic_tilt_deg=" << sky.pulsar_magnetic_tilt_deg << "\n";
		out << "sky_pulsar_wind_nebula_scale=" << sky.pulsar_wind_nebula_scale << "\n";
		out << "sky_pulsar_wind_nebula_intensity=" << sky.pulsar_wind_nebula_intensity << "\n";
		out << "sky_pulsar_frequency_spread=" << sky.pulsar_frequency_spread << "\n";
		out << "sky_snr_enabled=" << (sky.snr_enabled ? 1 : 0) << "\n";
		out << "sky_snr_asymmetry=" << sky.snr_asymmetry << "\n";
		out << "sky_snr_filament_scale=" << sky.snr_filament_scale << "\n";
		out << "sky_snr_interior_glow=" << sky.snr_interior_glow << "\n";
		out << "sky_snr_color_variation=" << sky.snr_color_variation << "\n";
		out << "sky_snr_morphology_variety=" << sky.snr_morphology_variety << "\n";
		out << "sky_snr_shell_color_r=" << sky.snr_shell_color[0] << "\n";
		out << "sky_snr_shell_color_g=" << sky.snr_shell_color[1] << "\n";
		out << "sky_snr_shell_color_b=" << sky.snr_shell_color[2] << "\n";
		out << "sky_snr_interior_color_r=" << sky.snr_interior_color[0] << "\n";
		out << "sky_snr_interior_color_g=" << sky.snr_interior_color[1] << "\n";
		out << "sky_snr_interior_color_b=" << sky.snr_interior_color[2] << "\n";
		out << "sky_star_size_scale=" << sky.star_size_scale << "\n";
		out << "sky_pulsar_brightness=" << sky.pulsar_brightness << "\n";
		out << "sky_snr_size=" << sky.snr_size << "\n";
		out << "sky_hue_shift_deg=" << sky.hue_shift_deg << "\n";
		out << "sky_saturation=" << sky.saturation << "\n";
		out << "sky_background_color_r=" << sky.background_color[0] << "\n";
		out << "sky_background_color_g=" << sky.background_color[1] << "\n";
		out << "sky_background_color_b=" << sky.background_color[2] << "\n";
		out << "sky_orientation_yaw_deg=" << sky.orientation_yaw_deg << "\n";
		out << "sky_orientation_pitch_deg=" << sky.orientation_pitch_deg << "\n";
		out << "sky_orientation_roll_deg=" << sky.orientation_roll_deg << "\n";
	}

	void write_interface(std::ostream& out) const {
		out << "window_log_console_open=" << (window_log_console_open ? 1 : 0) << "\n";
		out << "window_control_panel_open=" << (window_control_panel_open ? 1 : 0) << "\n";
		out << "window_performance_open=" << (window_performance_open ? 1 : 0) << "\n";
		out << "window_scenario_open=" << (window_scenario_open ? 1 : 0) << "\n";
		out << "window_telemetry_open=" << (window_telemetry_open ? 1 : 0) << "\n";
		out << "window_spectrograph_open=" << (window_spectrograph_open ? 1 : 0) << "\n";
		out << "window_diagnostics_open=" << (window_diagnostics_open ? 1 : 0) << "\n";
		out << "window_body_manager_open=" << (window_body_manager_open ? 1 : 0) << "\n";
		out << "window_performance_analysis_open=" << (window_performance_analysis_open ? 1 : 0) << "\n";
		out << "window_hud_manager_open=" << (window_hud_manager_open ? 1 : 0) << "\n";
		out << "window_keybind_settings_open=" << (window_keybind_settings_open ? 1 : 0) << "\n";
		out << "window_constants_open=" << (window_constants_open ? 1 : 0) << "\n";
		out << "window_capture_studio_open=" << (window_capture_studio_open ? 1 : 0) << "\n";
		out << "hud_master_enabled=" << (hud_layout.master_enabled ? 1 : 0) << "\n";
		for (size_t i = 0; i < static_cast<size_t>(UI::HudElementId::Count); ++i) {
			const auto& style = hud_layout.element(static_cast<UI::HudElementId>(i));
			const std::string prefix = "hud_el_" + std::to_string(i) + "_";
			out << prefix << "enabled=" << (style.enabled ? 1 : 0) << "\n";
			out << prefix << "anchor=" << static_cast<uint32_t>(style.anchor) << "\n";
			out << prefix << "offset_x=" << style.offset_x << "\n";
			out << prefix << "offset_y=" << style.offset_y << "\n";
			out << prefix << "scale=" << style.scale << "\n";
			out << prefix << "color_r=" << style.text_color[0] << "\n";
			out << prefix << "color_g=" << style.text_color[1] << "\n";
			out << prefix << "color_b=" << style.text_color[2] << "\n";
			out << prefix << "color_a=" << style.text_color[3] << "\n";
			out << prefix << "show_background=" << (style.show_background ? 1 : 0) << "\n";
			out << prefix << "background_opacity=" << style.background_opacity << "\n";
		}
		window_chrome.write(out);
		UI::InterfacePersistence::write(out, hud_layout, schematic_view);
		for (const auto& [key, value] : session_values) {
			out << "session_" << key << "=" << value << "\n";
		}
		for (size_t i = 0; i < kMaxSecondaryViews; ++i) {
			const auto& slot = secondary_views[i];
			const std::string prefix = "secview_" + std::to_string(i) + "_";
			out << prefix << "active=" << (slot.active ? 1 : 0) << "\n";
			if (!slot.active) continue;
			out << prefix << "open=" << (slot.open ? 1 : 0) << "\n";
			out << prefix << "name=" << slot.name << "\n";
			out << prefix << "radius=" << slot.radius << "\n";
			out << prefix << "theta=" << slot.theta << "\n";
			out << prefix << "phi=" << slot.phi << "\n";
			out << prefix << "fov_deg=" << slot.fov_deg << "\n";
			out << prefix << "exposure=" << slot.exposure << "\n";
			out << prefix << "tonemapping_mode=" << slot.tonemapping_mode << "\n";
			out << prefix << "projection_mode=" << slot.projection_mode << "\n";
			out << prefix << "max_steps=" << slot.max_steps << "\n";
			out << prefix << "resolution_scale=" << slot.resolution_scale << "\n";
			out << prefix << "follow_primary=" << (slot.follow_primary ? 1 : 0) << "\n";
			out << prefix << "follow_offset_theta=" << slot.follow_offset_theta << "\n";
			out << prefix << "follow_offset_phi=" << slot.follow_offset_phi << "\n";
		}
		SettingsWriter spatial_writer(out);
		spatial_reference.write_settings(spatial_writer);
	}

	void write_units(std::ostream& out) const {
		out << "unit_distance=" << static_cast<uint32_t>(unit_preferences.distance) << "\n";
		out << "unit_velocity=" << static_cast<uint32_t>(unit_preferences.velocity) << "\n";
		out << "unit_mass=" << static_cast<uint32_t>(unit_preferences.mass) << "\n";
		out << "unit_energy=" << static_cast<uint32_t>(unit_preferences.energy) << "\n";
		out << "unit_temperature=" << static_cast<uint32_t>(unit_preferences.temperature) << "\n";
		out << "unit_angle=" << static_cast<uint32_t>(unit_preferences.angle) << "\n";
		out << "unit_charge=" << static_cast<uint32_t>(unit_preferences.charge) << "\n";
		out << "unit_current=" << static_cast<uint32_t>(unit_preferences.current) << "\n";
		out << "unit_time=" << static_cast<uint32_t>(unit_preferences.time) << "\n";
		out << "unit_acceleration=" << static_cast<uint32_t>(unit_preferences.acceleration) << "\n";
		out << "unit_angular_velocity=" << static_cast<uint32_t>(unit_preferences.angular_velocity) << "\n";
		out << "unit_density=" << static_cast<uint32_t>(unit_preferences.density) << "\n";
		out << "unit_pressure=" << static_cast<uint32_t>(unit_preferences.pressure) << "\n";
		out << "unit_power=" << static_cast<uint32_t>(unit_preferences.power) << "\n";
		out << "unit_frequency=" << static_cast<uint32_t>(unit_preferences.frequency) << "\n";
		out << "unit_force=" << static_cast<uint32_t>(unit_preferences.force) << "\n";
		out << "unit_magnetic_field=" << static_cast<uint32_t>(unit_preferences.magnetic_field) << "\n";
		out << "unit_voltage=" << static_cast<uint32_t>(unit_preferences.voltage) << "\n";
		out << "unit_frame_rate=" << static_cast<uint32_t>(unit_preferences.frame_rate) << "\n";
		out << "coord_widget_default=" << static_cast<uint32_t>(coordinate_preferences.widget_default) << "\n";
	}

	void write_physics(std::ostream& out) const {
		out << "constants_preset=" << constants_preset << "\n";
		out << "constants_c=" << constants_c << "\n";
		out << "constants_g=" << constants_g << "\n";
		out << "constants_h=" << constants_h << "\n";
		out << "constants_kb=" << constants_kb << "\n";
		out << "constants_na=" << constants_na << "\n";
		out << "constants_ke=" << constants_ke << "\n";
		out << "constants_kcd=" << constants_kcd << "\n";
		out << "interaction_electricity_enabled=" << (interaction_electricity_enabled ? 1 : 0) << "\n";
		out << "interaction_magnetism_enabled=" << (interaction_magnetism_enabled ? 1 : 0) << "\n";
		out << "interaction_vacuum_permittivity=" << interaction_vacuum_permittivity << "\n";
		out << "interaction_vacuum_permeability=" << interaction_vacuum_permeability << "\n";
		out << "interaction_collisions_enabled=" << (interaction_collisions_enabled ? 1 : 0) << "\n";
		out << "interaction_collision_response_model=" << interaction_collision_response_model << "\n";
		out << "interaction_collision_consider_rotation=" << (interaction_collision_consider_rotation ? 1 : 0) << "\n";
		out << "interaction_collision_consider_friction=" << (interaction_collision_consider_friction ? 1 : 0) << "\n";
		out << "interaction_collision_restitution_multiplier=" << interaction_collision_restitution_multiplier << "\n";
		out << "interaction_thermodynamics_enabled=" << (interaction_thermodynamics_enabled ? 1 : 0) << "\n";
		out << "interaction_ambient_temperature=" << interaction_ambient_temperature << "\n";
		out << "interaction_radiative_coupling_scale=" << interaction_radiative_coupling_scale << "\n";
		out << "interaction_fragmentation_enabled=" << (interaction_fragmentation_enabled ? 1 : 0) << "\n";
		out << "interaction_minimum_fragment_mass=" << interaction_minimum_fragment_mass << "\n";
		out << "interaction_fragmentation_max_fragments=" << interaction_fragmentation_max_fragments << "\n";
		out << "interaction_collision_energy_to_integrity_loss=" << interaction_collision_energy_to_integrity_loss << "\n";
		out << "interaction_annihilation_enabled=" << (interaction_annihilation_enabled ? 1 : 0) << "\n";
		out << "interaction_annihilation_contact_scale=" << interaction_annihilation_contact_scale << "\n";
		out << "interaction_annihilation_require_opposite_charge=" << (interaction_annihilation_require_opposite_charge ? 1 : 0) << "\n";
		out << "interaction_collision_stiffness_scale=" << interaction_collision_stiffness_scale << "\n";
		out << "interaction_collision_position_correction_factor=" << interaction_collision_position_correction_factor << "\n";
		out << "interaction_fragmentation_tidal_stress_enabled=" << (interaction_fragmentation_tidal_stress_enabled ? 1 : 0) << "\n";
	}

	void write_bodies(std::ostream& out) const {
		out << "system_preset_selected=" << system_preset_selected << "\n";
		{
			std::unordered_map<std::string, std::string> rnd_kv;
			body_randomizer.write_settings(rnd_kv);
			for (const auto& [rk, rv] : rnd_kv) {
				out << rk << "=" << rv << "\n";
			}
		}
		out << "body_manager_sort_mode=" << body_manager_sort_mode << "\n";
		out << "body_manager_sort_descending=" << (body_manager_sort_descending ? 1 : 0) << "\n";
		out << "body_manager_search_filter=" << body_manager_search_filter << "\n";
		out << "body_manager_list_pane_width=" << body_manager_list_pane_width << "\n";
		out << "body_manager_source_sort_mode=" << body_manager_source_sort_mode << "\n";
		out << "body_manager_source_sort_descending=" << (body_manager_source_sort_descending ? 1 : 0) << "\n";
		out << "body_manager_source_search_filter=" << body_manager_source_search_filter << "\n";
		out << "body_manager_source_list_pane_width=" << body_manager_source_list_pane_width << "\n";
		out << "body_manager_bulk_parameter_index=" << body_manager_bulk_parameter_index << "\n";
		out << "body_manager_new_body_mass_log_mode=" << (body_manager_new_body_mass_log_mode ? 1 : 0) << "\n";
		out << "body_manager_new_body_radius_log_mode=" << (body_manager_new_body_radius_log_mode ? 1 : 0) << "\n";
		out << "body_manager_new_source_mass_log_mode=" << (body_manager_new_source_mass_log_mode ? 1 : 0) << "\n";
		out << "body_manager_reference_body_id=" << body_manager_reference_body_id << "\n";
		out << "body_manager_relative_coords_mode=" << (body_manager_relative_coords_mode ? 1 : 0) << "\n";
		out << "body_sec_identity_open=" << (body_sec_identity_open ? 1 : 0) << "\n";
		out << "body_sec_motion_open=" << (body_sec_motion_open ? 1 : 0) << "\n";
		out << "body_sec_orbital_plane_open=" << (body_sec_orbital_plane_open ? 1 : 0) << "\n";
		out << "body_sec_frenet_open=" << (body_sec_frenet_open ? 1 : 0) << "\n";
		out << "body_sec_orientation_open=" << (body_sec_orientation_open ? 1 : 0) << "\n";
		out << "body_sec_multipoles_open=" << (body_sec_multipoles_open ? 1 : 0) << "\n";
		out << "body_sec_material_open=" << (body_sec_material_open ? 1 : 0) << "\n";
		out << "body_sec_surface_open=" << (body_sec_surface_open ? 1 : 0) << "\n";
		out << "body_sec_accretion_disk_open=" << (body_sec_accretion_disk_open ? 1 : 0) << "\n";
		out << "body_creation_sec_template_open=" << (body_creation_sec_template_open ? 1 : 0) << "\n";
		out << "body_creation_sec_comparator_open=" << (body_creation_sec_comparator_open ? 1 : 0) << "\n";
		out << "body_creation_sec_plane_open=" << (body_creation_sec_plane_open ? 1 : 0) << "\n";
		out << "body_creation_sec_hazard_open=" << (body_creation_sec_hazard_open ? 1 : 0) << "\n";
		out << "source_creation_sec_template_open=" << (source_creation_sec_template_open ? 1 : 0) << "\n";
		out << "source_creation_sec_comparator_open=" << (source_creation_sec_comparator_open ? 1 : 0) << "\n";
		out << "source_creation_sec_plane_open=" << (source_creation_sec_plane_open ? 1 : 0) << "\n";
		out << "source_creation_sec_hazard_open=" << (source_creation_sec_hazard_open ? 1 : 0) << "\n";
		out << "sys_dyn_sec_bulk_actions_open=" << (sys_dyn_sec_bulk_actions_open ? 1 : 0) << "\n";
		out << "sys_dyn_sec_plane_align_open=" << (sys_dyn_sec_plane_align_open ? 1 : 0) << "\n";
		out << "sys_dyn_sec_generalized_open=" << (sys_dyn_sec_generalized_open ? 1 : 0) << "\n";
		out << "sys_dyn_sec_sources_open=" << (sys_dyn_sec_sources_open ? 1 : 0) << "\n";
		out << "interactions_sec_em_open=" << (interactions_sec_em_open ? 1 : 0) << "\n";
		out << "interactions_sec_collisions_open=" << (interactions_sec_collisions_open ? 1 : 0) << "\n";
		out << "interactions_sec_thermo_open=" << (interactions_sec_thermo_open ? 1 : 0) << "\n";
		out << "interactions_sec_frag_open=" << (interactions_sec_frag_open ? 1 : 0) << "\n";
		out << "interactions_sec_annihilation_open=" << (interactions_sec_annihilation_open ? 1 : 0) << "\n";
		out << "catalog_filter_ranges_open=" << (catalog_filter_ranges_open ? 1 : 0) << "\n";
	}

	void write_capture(std::ostream& out) const {
		out << "screenshot_output_directory=" << screenshot_output_directory << "\n";
		out << "screenshot_filename_pattern=" << screenshot_filename_pattern << "\n";
		out << "screenshot_format=" << screenshot_format << "\n";
		out << "screenshot_watermark_enabled=" << (screenshot_watermark_enabled ? 1 : 0) << "\n";
		out << "screenshot_watermark_text=" << screenshot_watermark_text << "\n";
		out << "screenshot_overwrite_policy=" << screenshot_overwrite_policy << "\n";
	}

	void write_cluster(std::ostream& out) const {
		out << "cluster_enabled=" << (cluster_enabled ? 1 : 0) << "\n";
		out << "cluster_node_role=" << cluster_node_role << "\n";
		out << "cluster_listen_port=" << cluster_listen_port << "\n";
		out << "cluster_discovery_multicast_enabled=" << (cluster_discovery_multicast_enabled ? 1 : 0) << "\n";
		out << "cluster_dynamic_load_balancing=" << (cluster_dynamic_load_balancing ? 1 : 0) << "\n";
		out << "cluster_latency_penalty_alpha=" << cluster_latency_penalty_alpha << "\n";
		out << "cluster_work_stealing_enabled=" << (cluster_work_stealing_enabled ? 1 : 0) << "\n";
		out << "cluster_tile_timeout_ms=" << cluster_tile_timeout_ms << "\n";
		out << "cluster_compression_mode=" << cluster_compression_mode << "\n";
		out << "cluster_require_encryption=" << (cluster_require_encryption ? 1 : 0) << "\n";
		out << "cluster_shared_token_hex=" << cluster_shared_token_hex << "\n";
		out << "cluster_route_interactive_viewport=" << (cluster_route_interactive_viewport ? 1 : 0) << "\n";
		out << "cluster_route_capture_studio=" << (cluster_route_capture_studio ? 1 : 0) << "\n";
		out << "cluster_route_monte_carlo_ensembles=" << (cluster_route_monte_carlo_ensembles ? 1 : 0) << "\n";
		out << "cluster_route_nbody_interactions=" << (cluster_route_nbody_interactions ? 1 : 0) << "\n";
		out << "cluster_route_exact_spacetime_metrics=" << (cluster_route_exact_spacetime_metrics ? 1 : 0) << "\n";
	}

	[[nodiscard]] static UserSettings load_or_default() {
		UserSettings result{};
		std::error_code ec;

		if (std::filesystem::exists(settings_file_path(), ec)) {
			std::filesystem::remove(settings_file_path(), ec);
		}

		if (std::filesystem::exists(general_settings_path(), ec)) {
			result.read_general(parse_kv_file(general_settings_path()));
		}
		if (result.load_policy == SettingsLoadPolicy::AlwaysResetToDefaults) {
			const auto preserved_policy = result.load_policy;
			if (std::filesystem::exists(capture_settings_path(), ec)) {
				result.read_capture(parse_kv_file(capture_settings_path()));
			}
			const auto preserved_dir = result.screenshot_output_directory;
			result = UserSettings{};
			result.load_policy = preserved_policy;
			result.screenshot_output_directory = preserved_dir;
			return result;
		}
		if (std::filesystem::exists(camera_settings_path(), ec)) {
			result.read_camera(parse_kv_file(camera_settings_path()));
		}
		if (std::filesystem::exists(render_sky_settings_path(), ec)) {
			result.read_render_sky(parse_kv_file(render_sky_settings_path()));
		}
		if (std::filesystem::exists(interface_settings_path(), ec)) {
			result.read_interface(parse_kv_file(interface_settings_path()));
		}
		if (std::filesystem::exists(units_settings_path(), ec)) {
			result.read_units(parse_kv_file(units_settings_path()));
		}
		if (std::filesystem::exists(physics_settings_path(), ec)) {
			result.read_physics(parse_kv_file(physics_settings_path()));
		}
		if (std::filesystem::exists(bodies_settings_path(), ec)) {
			result.read_bodies(parse_kv_file(bodies_settings_path()));
		}
		if (std::filesystem::exists(capture_settings_path(), ec)) {
			result.read_capture(parse_kv_file(capture_settings_path()));
		}
		if (std::filesystem::exists(cluster_settings_path(), ec)) {
			result.read_cluster(parse_kv_file(cluster_settings_path()));
		}
		return result;
	}

	void save() const {
		std::error_code ec;
		std::filesystem::create_directories(preferences_directory(), ec);

		write_kv_file(general_settings_path(), [this](std::ostream& out) { write_general(out); });
		write_kv_file(camera_settings_path(), [this](std::ostream& out) { write_camera(out); });
		write_kv_file(render_sky_settings_path(), [this](std::ostream& out) { write_render_sky(out); });
		write_kv_file(interface_settings_path(), [this](std::ostream& out) { write_interface(out); });
		write_kv_file(units_settings_path(), [this](std::ostream& out) { write_units(out); });
		write_kv_file(physics_settings_path(), [this](std::ostream& out) { write_physics(out); });
		write_kv_file(bodies_settings_path(), [this](std::ostream& out) { write_bodies(out); });
		write_kv_file(capture_settings_path(), [this](std::ostream& out) { write_capture(out); });
		write_kv_file(cluster_settings_path(), [this](std::ostream& out) { write_cluster(out); });

		if (std::filesystem::exists(settings_file_path(), ec)) {
			std::filesystem::remove(settings_file_path(), ec);
		}
	}
};

}

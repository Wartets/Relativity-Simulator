#pragma once

#include "relativistic/ui/camera_control_config.hpp"
#include "relativistic/ui/hud_layout_config.hpp"
#include "relativistic/ui/schematic_view_config.hpp"
#include <cstdint>
#include <cstdlib>
#include <string>
#include <fstream>
#include <filesystem>
#include <unordered_map>

namespace Relativistic::IO {

inline constexpr uint32_t USER_SETTINGS_FORMAT_VERSION = 2;

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
	std::string default_scenario_path{};
	std::string screenshot_output_directory{"./screenshots"};
	std::string screenshot_filename_pattern{"relativistic_%Y%m%d_%H%M%S"};
	uint32_t screenshot_format{0};

	UI::CameraControlConfig camera_controls{};
	UI::HudLayoutConfig hud_layout{};
	UI::SchematicViewConfig schematic_view{};

	uint32_t last_window_layout{0};
	bool multi_window_mode{true};

	[[nodiscard]] static std::filesystem::path settings_file_path() {
		return std::filesystem::path("config") / "user_settings.cfg";
	}

	[[nodiscard]] static std::filesystem::path crash_guard_path() {
		return std::filesystem::path("config") / ".session_active";
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

	[[nodiscard]] static UserSettings load_or_default() {
		UserSettings result{};
		const bool crashed = previous_session_crashed();

		std::ifstream file(settings_file_path());
		if (!file.is_open()) {
			return result;
		}

		std::unordered_map<std::string, std::string> kv;
		std::string line;
		while (std::getline(file, line)) {
			const size_t eq = line.find('=');
			if (eq == std::string::npos) continue;
			kv[line.substr(0, eq)] = line.substr(eq + 1);
		}

		auto get_str = [&](const char* key, const std::string& fallback) -> std::string {
			auto it = kv.find(key);
			return (it != kv.end()) ? it->second : fallback;
		};
		auto get_u32 = [&](const char* key, uint32_t fallback) -> uint32_t {
			auto it = kv.find(key);
			return (it != kv.end()) ? static_cast<uint32_t>(std::strtoul(it->second.c_str(), nullptr, 10)) : fallback;
		};
		auto get_i32 = [&](const char* key, int fallback) -> int {
			auto it = kv.find(key);
			return (it != kv.end()) ? static_cast<int>(std::strtol(it->second.c_str(), nullptr, 10)) : fallback;
		};
		auto get_dbl = [&](const char* key, double fallback) -> double {
			auto it = kv.find(key);
			return (it != kv.end()) ? std::strtod(it->second.c_str(), nullptr) : fallback;
		};
		auto get_bool = [&](const char* key, bool fallback) -> bool {
			auto it = kv.find(key);
			return (it != kv.end()) ? (std::strtoul(it->second.c_str(), nullptr, 10) != 0) : fallback;
		};

		const uint32_t file_version = get_u32("format_version", 0);
		if (file_version == 0 || file_version > USER_SETTINGS_FORMAT_VERSION) {
			return result;
		}

		result.format_version = file_version;
		result.load_policy = static_cast<SettingsLoadPolicy>(get_u32("load_policy", static_cast<uint32_t>(SettingsLoadPolicy::RestorePreviousSession)));

		if (result.load_policy == SettingsLoadPolicy::AlwaysResetToDefaults || crashed) {
			const auto preserved_policy = result.load_policy;
			const auto preserved_dir = get_str("screenshot_output_directory", result.screenshot_output_directory);
			result = UserSettings{};
			result.load_policy = preserved_policy;
			result.screenshot_output_directory = preserved_dir;
			return result;
		}

		result.default_camera_mode = get_u32("default_camera_mode", result.default_camera_mode);
		result.default_performance_preset = get_u32("default_performance_preset", result.default_performance_preset);
		result.default_use_gpu_compute = get_u32("default_use_gpu_compute", result.default_use_gpu_compute ? 1 : 0) != 0;
		result.default_scenario_path = get_str("default_scenario_path", result.default_scenario_path);
		result.screenshot_output_directory = get_str("screenshot_output_directory", result.screenshot_output_directory);
		result.screenshot_filename_pattern = get_str("screenshot_filename_pattern", result.screenshot_filename_pattern);
		result.screenshot_format = get_u32("screenshot_format", result.screenshot_format);
		result.last_window_layout = get_u32("last_window_layout", result.last_window_layout);
		result.multi_window_mode = get_u32("multi_window_mode", result.multi_window_mode ? 1 : 0) != 0;

		result.camera_controls.free_fly.forward_speed = get_dbl("cam_ff_forward_speed", result.camera_controls.free_fly.forward_speed);
		result.camera_controls.free_fly.lateral_speed = get_dbl("cam_ff_lateral_speed", result.camera_controls.free_fly.lateral_speed);
		result.camera_controls.free_fly.vertical_speed = get_dbl("cam_ff_vertical_speed", result.camera_controls.free_fly.vertical_speed);
		result.camera_controls.free_fly.invert_vertical = get_u32("cam_ff_invert_vertical", 0) != 0;
		result.camera_controls.free_fly.invert_lateral = get_u32("cam_ff_invert_lateral", 0) != 0;
		result.camera_controls.free_fly.invert_mouse_y = get_u32("cam_ff_invert_mouse_y", 0) != 0;
		result.camera_controls.free_fly.invert_mouse_x = get_u32("cam_ff_invert_mouse_x", 0) != 0;
		result.camera_controls.free_fly.mouse_sensitivity = get_dbl("cam_ff_mouse_sensitivity", result.camera_controls.free_fly.mouse_sensitivity);
		result.camera_controls.free_fly.sprint_multiplier = get_dbl("cam_ff_sprint_multiplier", result.camera_controls.free_fly.sprint_multiplier);
		result.camera_controls.free_fly.crawl_multiplier = get_dbl("cam_ff_crawl_multiplier", result.camera_controls.free_fly.crawl_multiplier);

		result.camera_controls.orbit.orbit_distance_speed = get_dbl("cam_orbit_distance_speed", result.camera_controls.orbit.orbit_distance_speed);
		result.camera_controls.orbit.pitch_speed_deg_s = get_dbl("cam_orbit_pitch_speed", result.camera_controls.orbit.pitch_speed_deg_s);
		result.camera_controls.orbit.yaw_speed_deg_s = get_dbl("cam_orbit_yaw_speed", result.camera_controls.orbit.yaw_speed_deg_s);
		result.camera_controls.orbit.invert_pitch = get_u32("cam_orbit_invert_pitch", 0) != 0;

		result.camera_controls.rocket.main_thrust_accel = get_dbl("cam_rocket_main_thrust", result.camera_controls.rocket.main_thrust_accel);
		result.camera_controls.rocket.lateral_thrust_accel = get_dbl("cam_rocket_lateral_thrust", result.camera_controls.rocket.lateral_thrust_accel);
		result.camera_controls.rocket.vertical_thrust_accel = get_dbl("cam_rocket_vertical_thrust", result.camera_controls.rocket.vertical_thrust_accel);
		result.camera_controls.rocket.angular_rate_deg_s = get_dbl("cam_rocket_angular_rate", result.camera_controls.rocket.angular_rate_deg_s);
		result.camera_controls.rocket.invert_vertical = get_u32("cam_rocket_invert_vertical", 0) != 0;
		result.camera_controls.rocket.invert_lateral = get_u32("cam_rocket_invert_lateral", 0) != 0;
		result.camera_controls.rocket.requires_time_running = get_u32("cam_rocket_requires_time", 1) != 0;

		const auto loaded_layout = static_cast<UI::KeyboardLayout>(get_u32("cam_keyboard_layout", static_cast<uint32_t>(UI::KeyboardLayout::Qwerty)));
		result.camera_controls.apply_keyboard_layout(loaded_layout);

		for (size_t i = 0; i < static_cast<size_t>(UI::InputAction::Count); ++i) {
			const std::string primary_key = "kb_" + std::to_string(i) + "_primary";
			const std::string secondary_key = "kb_" + std::to_string(i) + "_secondary";
			auto it_primary = kv.find(primary_key);
			auto it_secondary = kv.find(secondary_key);
			if (it_primary != kv.end() || it_secondary != kv.end()) {
				const auto action = static_cast<UI::InputAction>(i);
				const auto& current_bind = result.camera_controls.keybinds.get(action);
				const int primary = (it_primary != kv.end()) ? get_i32(primary_key.c_str(), current_bind.primary_key) : current_bind.primary_key;
				const int secondary = (it_secondary != kv.end()) ? get_i32(secondary_key.c_str(), current_bind.secondary_key) : current_bind.secondary_key;
				result.camera_controls.keybinds.set(action, primary, secondary);
			}
		}

		result.hud_layout.master_enabled = get_bool("hud_master_enabled", result.hud_layout.master_enabled);
		for (size_t i = 0; i < static_cast<size_t>(UI::HudElementId::Count); ++i) {
			const std::string prefix = "hud_el_" + std::to_string(i) + "_";
			auto& style = result.hud_layout.element(static_cast<UI::HudElementId>(i));
			style.enabled = get_bool((prefix + "enabled").c_str(), style.enabled);
			style.anchor = static_cast<UI::HudAnchor>(get_u32((prefix + "anchor").c_str(), static_cast<uint32_t>(style.anchor)));
			style.offset_x = static_cast<float>(get_dbl((prefix + "offset_x").c_str(), style.offset_x));
			style.offset_y = static_cast<float>(get_dbl((prefix + "offset_y").c_str(), style.offset_y));
			style.scale = static_cast<float>(get_dbl((prefix + "scale").c_str(), style.scale));
			style.text_color[0] = static_cast<float>(get_dbl((prefix + "color_r").c_str(), style.text_color[0]));
			style.text_color[1] = static_cast<float>(get_dbl((prefix + "color_g").c_str(), style.text_color[1]));
			style.text_color[2] = static_cast<float>(get_dbl((prefix + "color_b").c_str(), style.text_color[2]));
			style.text_color[3] = static_cast<float>(get_dbl((prefix + "color_a").c_str(), style.text_color[3]));
			style.show_background = get_bool((prefix + "show_background").c_str(), style.show_background);
			style.background_opacity = static_cast<float>(get_dbl((prefix + "background_opacity").c_str(), style.background_opacity));
		}

		return result;
	}

	void save() const {
		std::error_code ec;
		std::filesystem::create_directories(settings_file_path().parent_path(), ec);
		std::ofstream out(settings_file_path(), std::ios::trunc);
		if (!out.is_open()) return;

		out << "format_version=" << format_version << "\n";
		out << "load_policy=" << static_cast<uint32_t>(load_policy) << "\n";
		out << "default_camera_mode=" << default_camera_mode << "\n";
		out << "default_performance_preset=" << default_performance_preset << "\n";
		out << "default_use_gpu_compute=" << (default_use_gpu_compute ? 1 : 0) << "\n";
		out << "default_scenario_path=" << default_scenario_path << "\n";
		out << "screenshot_output_directory=" << screenshot_output_directory << "\n";
		out << "screenshot_filename_pattern=" << screenshot_filename_pattern << "\n";
		out << "screenshot_format=" << screenshot_format << "\n";
		out << "last_window_layout=" << last_window_layout << "\n";
		out << "multi_window_mode=" << (multi_window_mode ? 1 : 0) << "\n";
		out << "cam_ff_forward_speed=" << camera_controls.free_fly.forward_speed << "\n";
		out << "cam_ff_lateral_speed=" << camera_controls.free_fly.lateral_speed << "\n";
		out << "cam_ff_vertical_speed=" << camera_controls.free_fly.vertical_speed << "\n";
		out << "cam_ff_invert_vertical=" << (camera_controls.free_fly.invert_vertical ? 1 : 0) << "\n";
		out << "cam_ff_invert_lateral=" << (camera_controls.free_fly.invert_lateral ? 1 : 0) << "\n";
		out << "cam_ff_invert_mouse_y=" << (camera_controls.free_fly.invert_mouse_y ? 1 : 0) << "\n";
		out << "cam_ff_invert_mouse_x=" << (camera_controls.free_fly.invert_mouse_x ? 1 : 0) << "\n";
		out << "cam_ff_mouse_sensitivity=" << camera_controls.free_fly.mouse_sensitivity << "\n";
		out << "cam_ff_sprint_multiplier=" << camera_controls.free_fly.sprint_multiplier << "\n";
		out << "cam_ff_crawl_multiplier=" << camera_controls.free_fly.crawl_multiplier << "\n";
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
		out << "cam_keyboard_layout=" << static_cast<uint32_t>(camera_controls.keyboard_layout) << "\n";

		for (size_t i = 0; i < static_cast<size_t>(UI::InputAction::Count); ++i) {
			const auto& binding = camera_controls.keybinds.get(static_cast<UI::InputAction>(i));
			out << "kb_" << i << "_primary=" << binding.primary_key << "\n";
			out << "kb_" << i << "_secondary=" << binding.secondary_key << "\n";
		}

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
	}
};

}

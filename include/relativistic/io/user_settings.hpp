#pragma once

#include "relativistic/ui/camera_control_config.hpp"
#include <cstdint>
#include <cstdlib>
#include <string>
#include <fstream>
#include <filesystem>
#include <unordered_map>

namespace Relativistic::IO {

inline constexpr uint32_t USER_SETTINGS_FORMAT_VERSION = 1;

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
		auto get_dbl = [&](const char* key, double fallback) -> double {
			auto it = kv.find(key);
			return (it != kv.end()) ? std::strtod(it->second.c_str(), nullptr) : fallback;
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
	}
};

}

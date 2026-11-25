#pragma once

#include "relativistic/ui/input_actions.hpp"
#include "relativistic/ui/gamepad_input.hpp"
#include "relativistic/observer/surface_walker.hpp"
#include "relativistic/observer/planet_orbit.hpp"
#include <cstdint>

namespace Relativistic::UI {

enum class ArrowKeysMode : uint32_t {
	Movement = 0,
	LookPitchYaw = 1,
	LookPitchRoll = 2,
	LookRollYaw = 3
};

[[nodiscard]] constexpr const char* arrow_keys_mode_name(ArrowKeysMode mode) noexcept {
	switch (mode) {
		case ArrowKeysMode::LookPitchYaw: return "Look Inclination (Pitch & Yaw)";
		case ArrowKeysMode::LookPitchRoll: return "Look Inclination (Pitch & Roll)";
		case ArrowKeysMode::LookRollYaw: return "Look Inclination (Roll & Yaw)";
		case ArrowKeysMode::Movement:
		default: return "Translational Movement";
	}
}

struct AxisSpeedProfile {
	double forward_speed{10.0};
	double lateral_speed{10.0};
	double vertical_speed{10.0};
	double yaw_speed_deg_s{40.0};
	double pitch_speed_deg_s{40.0};
	double roll_speed_deg_s{45.0};
	double sprint_multiplier{4.0};
	double crawl_multiplier{0.2};
	double orbit_distance_speed{10.0};
	bool invert_forward{false};
	bool invert_lateral{false};
	bool invert_vertical{false};
	bool invert_pitch{false};
	bool invert_mouse_x{false};
	bool invert_mouse_y{false};
	double mouse_sensitivity{0.15};
	bool ignore_pitch_roll_for_movement{true};
};

struct RocketControlProfile {
	double main_thrust_accel{5.0};
	double lateral_thrust_accel{2.0};
	double vertical_thrust_accel{2.0};
	double angular_rate_deg_s{15.0};
	double max_proper_acceleration{20.0};
	bool invert_lateral{false};
	bool invert_vertical{false};
	bool requires_time_running{true};
};

struct ZoomConfig {
	bool zoom_center_on_cursor{true};
	double zoom_scroll_sensitivity{0.18};
	double min_zoom{1.0};
	double max_zoom{8.0};
};

struct CameraControlConfig {
	KeyboardLayout keyboard_layout{KeyboardLayout::Qwerty};
	ActionKeybindMap keybinds{};
	AxisSpeedProfile free_fly{};
	AxisSpeedProfile orbit{};
	RocketControlProfile rocket{};
	Observer::SurfaceWalkerParameters surface_walk{};
	Observer::PlanetOrbitParameters planet_orbit{};
	ZoomConfig zoom{};
	ArrowKeysMode arrow_keys_mode{ArrowKeysMode::Movement};
	double arrow_look_sensitivity{1.0};
	bool arrow_look_invert_pitch{false};
	bool arrow_look_invert_yaw{false};
	bool arrow_look_invert_roll{false};
	GamepadConfig gamepad{};

	void apply_keyboard_layout(KeyboardLayout layout) noexcept {
		keyboard_layout = layout;
		keybinds.reset_to_layout_defaults(layout);
	}
};

}

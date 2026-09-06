#pragma once

#include "relativistic/ui/input_actions.hpp"
#include <cstdint>

namespace Relativistic::UI {

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
};

struct RocketControlProfile {
	double main_thrust_accel{20.0};
	double lateral_thrust_accel{10.0};
	double vertical_thrust_accel{10.0};
	double angular_rate_deg_s{60.0};
	double max_proper_acceleration{100.0};
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
	ZoomConfig zoom{};

	void apply_keyboard_layout(KeyboardLayout layout) noexcept {
		keyboard_layout = layout;
		keybinds.reset_to_layout_defaults(layout);
	}
};

}

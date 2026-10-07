#pragma once

namespace Relativistic::UI {

struct HudPreferences {
	bool show_hud{true};
	bool show_viewport_toolbar{true};
	bool show_loading_indicator{true};
	bool show_frame_time{true};
	bool show_rolling_average_fps{true};
	bool show_camera_distance{true};
	bool show_camera_angles{true};
	bool show_camera_orientation{true};
	bool show_metric_summary{true};
	bool show_ray_statistics{true};
	bool show_navigation_controls{true};
};

}

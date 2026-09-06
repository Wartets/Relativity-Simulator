#pragma once

#include <imgui.h>
#include "relativistic/ui/input_actions.hpp"
#include <array>
#include <cstdint>
#include <algorithm>

namespace Relativistic::UI {

enum class HudAnchor : uint32_t {
	TopLeft = 0,
	TopRight = 1,
	BottomLeft = 2,
	BottomRight = 3,
	TopCenter = 4,
	BottomCenter = 5
};

enum class HudDisplayMode : uint32_t {
	Compact = 0,
	Standard = 1,
	Extended = 2
};

enum class HudElementId : uint32_t {
	FrameTimeReadout = 0,
	CameraDistanceReadout,
	CameraAnglesReadout,
	CameraOrientationReadout,
	MetricSummaryReadout,
	RayStatisticsReadout,
	NavigationControlsPanel,
	ViewportToolbar,
	LoadingIndicator,
	SimulationClockReadout,
	WarpFactorReadout,
	PerformancePresetReadout,
	TelemetryQuickReadout,
	SpectrographQuickReadout,
	DiagnosticsQuickReadout,
	Count
};

[[nodiscard]] constexpr const char* hud_element_name(HudElementId id) noexcept {
	switch (id) {
		case HudElementId::FrameTimeReadout: return "Frame Time / FPS";
		case HudElementId::CameraDistanceReadout: return "Camera Distance";
		case HudElementId::CameraAnglesReadout: return "Camera Angles";
		case HudElementId::CameraOrientationReadout: return "Camera Orientation";
		case HudElementId::MetricSummaryReadout: return "Metric Summary";
		case HudElementId::RayStatisticsReadout: return "Ray Statistics";
		case HudElementId::NavigationControlsPanel: return "Navigation Controls Panel";
		case HudElementId::ViewportToolbar: return "Viewport Toolbar";
		case HudElementId::LoadingIndicator: return "Loading Indicator";
		case HudElementId::SimulationClockReadout: return "Simulation Clock";
		case HudElementId::WarpFactorReadout: return "Warp Factor";
		case HudElementId::PerformancePresetReadout: return "Performance Preset";
		case HudElementId::TelemetryQuickReadout: return "Telemetry Quick Readout";
		case HudElementId::SpectrographQuickReadout: return "Spectrograph Quick Readout";
		case HudElementId::DiagnosticsQuickReadout: return "Diagnostics Quick Readout";
		default: return "Unknown Element";
	}
}

struct HudElementStyle {
	bool enabled{true};
	HudAnchor anchor{HudAnchor::TopLeft};
	float offset_x{16.0f};
	float offset_y{48.0f};
	float scale{1.0f};
	std::array<float, 4> text_color{0.9f, 0.9f, 0.9f, 1.0f};
	bool show_background{false};
	float background_opacity{0.55f};
	HudDisplayMode display_mode{HudDisplayMode::Standard};
	bool show_label{true};
	bool horizontal_layout{false};
	int draw_priority{0};
};

struct ToolbarButtonVisibility {
	bool play_pause{true};
	bool step{true};
	bool reset_view{true};
	bool look_at_target_combo{true};
	bool jump_to_target{true};
	bool camera_mode_combo{true};
	bool hud_master_toggle{true};
};

struct HudLayoutConfig {
	bool master_enabled{true};
	std::array<HudElementStyle, static_cast<size_t>(HudElementId::Count)> elements{};
	ToolbarButtonVisibility toolbar_buttons{};
	std::array<bool, static_cast<size_t>(InputAction::Count)> keybind_summary_visible{};

	HudLayoutConfig() noexcept {
		for (size_t i = 0; i < static_cast<size_t>(InputAction::Count); ++i) {
			keybind_summary_visible[i] = (input_action_category(static_cast<InputAction>(i)) == InputActionCategory::Movement)
				|| (static_cast<InputAction>(i) == InputAction::ZoomModifier);
		}
		element(HudElementId::FrameTimeReadout).offset_x = 16.0f;
		element(HudElementId::FrameTimeReadout).offset_y = 48.0f;
		element(HudElementId::FrameTimeReadout).text_color = {0.2f, 1.0f, 0.4f, 1.0f};

		element(HudElementId::CameraDistanceReadout).offset_x = 16.0f;
		element(HudElementId::CameraDistanceReadout).offset_y = 70.0f;

		element(HudElementId::CameraAnglesReadout).offset_x = 16.0f;
		element(HudElementId::CameraAnglesReadout).offset_y = 92.0f;

		element(HudElementId::CameraOrientationReadout).offset_x = 16.0f;
		element(HudElementId::CameraOrientationReadout).offset_y = 114.0f;

		element(HudElementId::MetricSummaryReadout).offset_x = 16.0f;
		element(HudElementId::MetricSummaryReadout).offset_y = 136.0f;

		element(HudElementId::RayStatisticsReadout).offset_x = 16.0f;
		element(HudElementId::RayStatisticsReadout).offset_y = 158.0f;
		element(HudElementId::RayStatisticsReadout).text_color = {0.7f, 0.7f, 1.0f, 0.9f};

		auto& nav_panel = element(HudElementId::NavigationControlsPanel);
		nav_panel.anchor = HudAnchor::TopRight;
		nav_panel.offset_x = 16.0f;
		nav_panel.offset_y = 16.0f;

		element(HudElementId::ViewportToolbar).offset_x = 16.0f;
		element(HudElementId::ViewportToolbar).offset_y = 16.0f;

		auto& loading = element(HudElementId::LoadingIndicator);
		loading.anchor = HudAnchor::BottomRight;
		loading.offset_x = 16.0f;
		loading.offset_y = 16.0f;

		auto& sim_clock = element(HudElementId::SimulationClockReadout);
		sim_clock.enabled = false;
		sim_clock.anchor = HudAnchor::BottomLeft;
		sim_clock.offset_x = 16.0f;
		sim_clock.offset_y = 16.0f;

		auto& warp = element(HudElementId::WarpFactorReadout);
		warp.enabled = false;
		warp.anchor = HudAnchor::BottomLeft;
		warp.offset_x = 16.0f;
		warp.offset_y = 38.0f;

		auto& preset = element(HudElementId::PerformancePresetReadout);
		preset.enabled = false;
		preset.anchor = HudAnchor::BottomLeft;
		preset.offset_x = 16.0f;
		preset.offset_y = 60.0f;

		auto& telemetry_ro = element(HudElementId::TelemetryQuickReadout);
		telemetry_ro.enabled = false;
		telemetry_ro.anchor = HudAnchor::BottomLeft;
		telemetry_ro.offset_x = 16.0f;
		telemetry_ro.offset_y = 82.0f;

		auto& spectro_ro = element(HudElementId::SpectrographQuickReadout);
		spectro_ro.enabled = false;
		spectro_ro.anchor = HudAnchor::BottomLeft;
		spectro_ro.offset_x = 16.0f;
		spectro_ro.offset_y = 104.0f;

		auto& diag_ro = element(HudElementId::DiagnosticsQuickReadout);
		diag_ro.enabled = false;
		diag_ro.anchor = HudAnchor::BottomLeft;
		diag_ro.offset_x = 16.0f;
		diag_ro.offset_y = 126.0f;
	}

	[[nodiscard]] HudElementStyle& element(HudElementId id) noexcept {
		return elements[static_cast<size_t>(id)];
	}

	[[nodiscard]] const HudElementStyle& element(HudElementId id) const noexcept {
		return elements[static_cast<size_t>(id)];
	}
};

[[nodiscard]] inline ImVec2 hud_anchor_resolve(HudAnchor anchor, const ImVec2& avail, const ImVec2& element_size, float offset_x, float offset_y) noexcept {
	float x = 0.0f;
	float y = 0.0f;
	switch (anchor) {
		case HudAnchor::TopLeft:
			x = offset_x;
			y = offset_y;
			break;
		case HudAnchor::TopRight:
			x = avail.x - element_size.x - offset_x;
			y = offset_y;
			break;
		case HudAnchor::BottomLeft:
			x = offset_x;
			y = avail.y - element_size.y - offset_y;
			break;
		case HudAnchor::BottomRight:
			x = avail.x - element_size.x - offset_x;
			y = avail.y - element_size.y - offset_y;
			break;
		case HudAnchor::TopCenter:
			x = (avail.x - element_size.x) * 0.5f + offset_x;
			y = offset_y;
			break;
		case HudAnchor::BottomCenter:
			x = (avail.x - element_size.x) * 0.5f + offset_x;
			y = avail.y - element_size.y - offset_y;
			break;
	}
	return ImVec2(std::max(x, 0.0f), std::max(y, 0.0f));
}

}

#pragma once

#include <imgui.h>
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/ui/spatial_reference/spatial_reference_config.hpp"
#include "relativistic/ui/spatial_reference/spatial_reference_renderer.hpp"
#include "relativistic/ui/tooltip_utils.hpp"
#include "relativistic/ui/numeric_slider_utils.hpp"
#include "relativistic/units/unit_system.hpp"
#include "relativistic/units/unit_aware_widgets.hpp"
#include <algorithm>
#include <array>
#include <mutex>
#include <string>
#include <vector>

namespace Relativistic::UI {

inline bool render_spatial_reference_editor(SpatialReferenceConfig& cfg, Orchestrator::SimulationOrchestrator<1024>& orchestrator) {
	bool changed = false;
	const auto& prefs = orchestrator.unit_preferences();
	const double length_scale = std::max(orchestrator.constants_engine().length_scale(), 1.0e-300);
	static std::array<bool, 8> log_modes{true, true, true, true, true, true, true, true};

	const auto length_slider = [&](const char* label, double& value, double minimum, double maximum, size_t log_slot) {
		double meters = value * length_scale;
		const double min_m = minimum * length_scale;
		const double max_m = maximum * length_scale;
		if (unit_aware_slider_double(label, &meters, min_m, max_m, UnitCategory::Distance, prefs, "%.4f", &log_modes[log_slot], min_m, max_m)) {
			value = meters / length_scale;
			return true;
		}
		return false;
	};

	const auto size_mode_combo = [&](const char* label, SpatialSizeMode& mode) {
		int index = static_cast<int>(mode);
		static constexpr std::array<const char*, 2> names{"Automatic (Adapts To Camera Distance)", "Fixed"};
		if (ImGui::Combo(label, &index, names.data(), static_cast<int>(names.size()))) {
			mode = static_cast<SpatialSizeMode>(index);
			return true;
		}
		return false;
	};

	const auto line_style = [&](const char* label, SpatialLineStyle& style) {
		bool edited = false;
		ImGui::PushID(label);
		edited |= ImGui::Checkbox(label, &style.enabled);
		if (style.enabled) {
			edited |= ImGui::ColorEdit4("Color", style.color.data());
			edited |= ImGui::SliderFloat("Thickness (px)", &style.thickness, 0.25f, 12.0f, "%.2f");
			edited |= ImGui::SliderFloat("Opacity", &style.opacity, 0.0f, 1.0f, "%.2f");
		}
		ImGui::PopID();
		return edited;
	};

	ImGui::TextColored(ImVec4(0.3f, 0.9f, 1.0f, 1.0f), "3D Spatial Reference");
	ImGui::TextDisabled("Lengths are displayed in %s and automatic spacing snaps to round values in that unit.", std::string(Units::distance_unit_suffix(prefs.distance)).c_str());
	changed |= ImGui::Checkbox("Enable Spatial Reference", &cfg.enabled);
	render_setting_tooltip("Draws a 3D coordinate reference (axes, grid planes, rings, ticks and scale bar) over the viewport. Disabled by default.");
	changed |= ImGui::Checkbox("Show In Ray-Traced View", &cfg.show_in_raytraced_view);
	ImGui::SameLine();
	changed |= ImGui::Checkbox("Show In Schematic View", &cfg.show_in_schematic_view);
	changed |= ImGui::SliderFloat("Global Opacity", &cfg.global_opacity, 0.0f, 1.0f, "%.2f");

	if (ImGui::CollapsingHeader("Reference Point", ImGuiTreeNodeFlags_DefaultOpen)) {
		int mode = static_cast<int>(cfg.center_mode);
		static constexpr std::array<const char*, 4> modes{"World Origin", "Custom Point", "Follow Body", "Snapped To Camera (Local Frame)"};
		if (ImGui::Combo("Reference Origin", &mode, modes.data(), static_cast<int>(modes.size()))) {
			cfg.center_mode = static_cast<SpatialCenterMode>(mode);
			changed = true;
		}
		render_setting_tooltip("Chooses where the reference frame is anchored: the world origin, a chosen point, a moving body, or a grid-snapped point under the camera that keeps a local frame always in view.");
		if (cfg.center_mode == SpatialCenterMode::CustomPoint) {
			double meters[3] = {cfg.custom_center[0] * length_scale, cfg.custom_center[1] * length_scale, cfg.custom_center[2] * length_scale};
			if (unit_aware_input_double3("Custom Point (x, y, z)", meters, UnitCategory::Distance, prefs)) {
				cfg.custom_center = {meters[0] / length_scale, meters[1] / length_scale, meters[2] / length_scale};
				changed = true;
			}
			if (ImGui::Button("Use Camera Position")) {
				cfg.custom_center = orchestrator.camera().position;
				changed = true;
			}
		} else if (cfg.center_mode == SpatialCenterMode::Body) {
			std::vector<uint32_t> ids;
			std::vector<std::string> labels;
			{
				std::lock_guard<std::recursive_mutex> lock(orchestrator.nbody_system().bodies_mutex());
				for (const auto& body : orchestrator.nbody_system().bodies()) {
					ids.push_back(body.id);
					labels.push_back((body.has_name() ? std::string(body.name_view()) : std::string("Body")) + " (#" + std::to_string(body.id) + ")");
				}
			}
			if (ids.empty()) {
				ImGui::TextDisabled("No body available; the world origin is used.");
			} else {
				int selected = 0;
				for (size_t i = 0; i < ids.size(); ++i) {
					if (ids[i] == cfg.center_body_id) {
						selected = static_cast<int>(i);
					}
				}
				std::vector<const char*> pointers;
				pointers.reserve(labels.size());
				for (const std::string& label : labels) {
					pointers.push_back(label.c_str());
				}
				if (ImGui::Combo("Followed Body", &selected, pointers.data(), static_cast<int>(pointers.size()))) {
					cfg.center_body_id = ids[static_cast<size_t>(selected)];
					changed = true;
				} else if (cfg.center_body_id == 0U) {
					cfg.center_body_id = ids.front();
				}
			}
		}
	}

	if (ImGui::CollapsingHeader("Visibility And Quality")) {
		changed |= ImGui::Checkbox("Hide Behind Event Horizon", &cfg.hide_behind_horizon);
		render_setting_tooltip("Hides reference geometry that lies behind the primary event horizon as seen from the camera.");
		changed |= ImGui::Checkbox("Apply Lens Approximation", &cfg.lens_approximation);
		render_setting_tooltip("Bends the reference geometry with the same weak-field deflection model used by the body overlays in the ray-traced view.");
		changed |= ImGui::Checkbox("Fade With Distance", &cfg.depth_fade_enabled);
		if (cfg.depth_fade_enabled) {
			changed |= size_mode_combo("Fade Reference Mode", cfg.depth_fade_mode);
			if (cfg.depth_fade_mode == SpatialSizeMode::Fixed) {
				changed |= length_slider("Fade Reference Distance", cfg.depth_fade_reference, 1.0e-3, 1.0e6, 0);
			}
			changed |= ImGui::SliderFloat("Minimum Fade Opacity", &cfg.depth_fade_minimum, 0.0f, 1.0f, "%.2f");
		}
		int subdivisions = static_cast<int>(cfg.curve_subdivisions);
		if (ImGui::SliderInt("Curve Subdivisions", &subdivisions, 2, 128)) {
			cfg.curve_subdivisions = static_cast<uint32_t>(subdivisions);
			changed = true;
		}
		render_setting_tooltip("Number of pieces each line is split into when the projection or the lens approximation bends straight lines.");
		int max_lines = static_cast<int>(cfg.max_lines_per_direction);
		if (ImGui::SliderInt("Maximum Grid Lines Per Direction", &max_lines, 2, 400)) {
			cfg.max_lines_per_direction = static_cast<uint32_t>(max_lines);
			changed = true;
		}
		int max_ticks = static_cast<int>(cfg.max_ticks_per_axis);
		if (ImGui::SliderInt("Maximum Ticks Per Axis", &max_ticks, 1, 400)) {
			cfg.max_ticks_per_axis = static_cast<uint32_t>(max_ticks);
			changed = true;
		}
	}

	if (ImGui::CollapsingHeader("Axes")) {
		static constexpr std::array<const char*, 3> names{"X Axis", "Y Axis", "Z Axis"};
		for (size_t i = 0; i < 3; ++i) {
			changed |= line_style(names[i], cfg.axes[i]);
		}
		changed |= ImGui::Checkbox("Show Negative Half", &cfg.axes_show_negative);
		changed |= size_mode_combo("Axis Length Mode", cfg.axis_length_mode);
		if (cfg.axis_length_mode == SpatialSizeMode::Fixed) {
			changed |= length_slider("Axis Length", cfg.axis_length, 1.0e-3, 1.0e6, 1);
		}
		changed |= ImGui::Checkbox("Arrowheads", &cfg.axis_arrowheads);
		if (cfg.axis_arrowheads) {
			changed |= ImGui::SliderFloat("Arrowhead Size (px)", &cfg.axis_arrow_size_px, 2.0f, 60.0f, "%.1f");
		}
		changed |= ImGui::Checkbox("Axis Name Labels", &cfg.axis_labels);
	}

	if (ImGui::CollapsingHeader("Ticks")) {
		changed |= ImGui::Checkbox("Axis Ticks", &cfg.ticks_enabled);
		if (cfg.ticks_enabled) {
			changed |= size_mode_combo("Tick Spacing Mode", cfg.tick_spacing_mode);
			if (cfg.tick_spacing_mode == SpatialSizeMode::Fixed) {
				changed |= length_slider("Tick Spacing", cfg.tick_spacing, 1.0e-4, 1.0e5, 2);
			}
			changed |= ImGui::SliderFloat("Tick Length (px)", &cfg.tick_length_px, 1.0f, 60.0f, "%.1f");
			changed |= ImGui::SliderFloat("Tick Thickness (px)", &cfg.tick_thickness_px, 0.25f, 10.0f, "%.2f");
			changed |= ImGui::Checkbox("Tick Labels", &cfg.tick_labels);
			int interval = static_cast<int>(cfg.tick_label_interval);
			if (ImGui::SliderInt("Label Every N Ticks", &interval, 1, 20)) {
				cfg.tick_label_interval = static_cast<uint32_t>(interval);
				changed = true;
			}
		}
	}

	if (ImGui::CollapsingHeader("Grid Planes")) {
		changed |= ImGui::Checkbox("XY Plane", &cfg.grid_planes[0]);
		ImGui::SameLine();
		changed |= ImGui::Checkbox("XZ Plane", &cfg.grid_planes[1]);
		ImGui::SameLine();
		changed |= ImGui::Checkbox("YZ Plane", &cfg.grid_planes[2]);
		changed |= size_mode_combo("Grid Spacing Mode", cfg.grid_spacing_mode);
		if (cfg.grid_spacing_mode == SpatialSizeMode::Fixed) {
			changed |= length_slider("Grid Spacing", cfg.grid_spacing, 1.0e-4, 1.0e5, 3);
		}
		changed |= size_mode_combo("Grid Extent Mode", cfg.grid_extent_mode);
		if (cfg.grid_extent_mode == SpatialSizeMode::Fixed) {
			changed |= length_slider("Grid Half Extent", cfg.grid_extent, 1.0e-3, 1.0e6, 4);
		}
		int major = static_cast<int>(cfg.grid_major_interval);
		if (ImGui::SliderInt("Major Line Every N Lines", &major, 1, 50)) {
			cfg.grid_major_interval = static_cast<uint32_t>(major);
			changed = true;
		}
		changed |= line_style("Minor Lines", cfg.grid_minor);
		changed |= line_style("Major Lines", cfg.grid_major);
		changed |= ImGui::Checkbox("Radial Edge Fade", &cfg.grid_radial_fade);
		render_setting_tooltip("Fades grid lines smoothly toward the outer edge of the grid so it does not end abruptly.");
		static constexpr std::array<const char*, 3> offset_labels{"XY Plane Offset (Z)", "XZ Plane Offset (Y)", "YZ Plane Offset (X)"};
		static constexpr std::array<size_t, 3> slots{5, 6, 7};
		for (size_t i = 0; i < 3; ++i) {
			if (cfg.grid_planes[i]) {
				double meters = cfg.grid_plane_offsets[i] * length_scale;
				const double limit = 1.0e6 * length_scale;
				if (unit_aware_slider_double(offset_labels[i], &meters, -limit, limit, UnitCategory::Distance, prefs, "%.4f")) {
					cfg.grid_plane_offsets[i] = meters / length_scale;
					changed = true;
				}
			}
		}
		static_cast<void>(slots);
	}

	if (ImGui::CollapsingHeader("Distance Rings")) {
		changed |= ImGui::Checkbox("Enable Rings", &cfg.rings_enabled);
		if (cfg.rings_enabled) {
			int plane = static_cast<int>(cfg.ring_plane);
			static constexpr std::array<const char*, 3> planes{"XY Plane", "XZ Plane", "YZ Plane"};
			if (ImGui::Combo("Ring Plane", &plane, planes.data(), static_cast<int>(planes.size()))) {
				cfg.ring_plane = static_cast<SpatialPlane>(plane);
				changed = true;
			}
			changed |= size_mode_combo("Ring Spacing Mode", cfg.ring_spacing_mode);
			if (cfg.ring_spacing_mode == SpatialSizeMode::Fixed) {
				changed |= length_slider("Ring Spacing", cfg.ring_spacing, 1.0e-4, 1.0e5, 2);
			}
			int count = static_cast<int>(cfg.ring_count);
			if (ImGui::SliderInt("Ring Count", &count, 1, 64)) {
				cfg.ring_count = static_cast<uint32_t>(count);
				changed = true;
			}
			changed |= line_style("Ring Lines", cfg.ring_style);
			changed |= ImGui::Checkbox("Ring Distance Labels", &cfg.ring_labels);
		}
	}

	if (ImGui::CollapsingHeader("Origin Marker")) {
		changed |= ImGui::Checkbox("Show Origin Marker", &cfg.origin_marker_enabled);
		if (cfg.origin_marker_enabled) {
			int shape = static_cast<int>(cfg.origin_marker_shape);
			static constexpr std::array<const char*, 4> shapes{"Cross", "Dot", "Diamond", "Ring"};
			if (ImGui::Combo("Marker Shape", &shape, shapes.data(), static_cast<int>(shapes.size()))) {
				cfg.origin_marker_shape = static_cast<SpatialMarkerShape>(shape);
				changed = true;
			}
			changed |= ImGui::SliderFloat("Marker Size (px)", &cfg.origin_marker_size_px, 2.0f, 80.0f, "%.1f");
			changed |= ImGui::ColorEdit4("Marker Color", cfg.origin_marker_color.data());
		}
	}

	if (ImGui::CollapsingHeader("Scale Bar")) {
		changed |= ImGui::Checkbox("Show Scale Bar", &cfg.scale_bar_enabled);
		if (cfg.scale_bar_enabled) {
			int corner = static_cast<int>(cfg.scale_bar_corner);
			static constexpr std::array<const char*, 4> corners{"Top Left", "Top Right", "Bottom Left", "Bottom Right"};
			if (ImGui::Combo("Scale Bar Corner", &corner, corners.data(), static_cast<int>(corners.size()))) {
				cfg.scale_bar_corner = static_cast<SpatialCorner>(corner);
				changed = true;
			}
			changed |= ImGui::SliderFloat("Margin (px)", &cfg.scale_bar_margin_px, 0.0f, 400.0f, "%.0f");
			changed |= ImGui::SliderFloat("Target Length (px)", &cfg.scale_bar_target_px, 40.0f, 1200.0f, "%.0f");
			changed |= ImGui::SliderFloat("Bar Thickness (px)", &cfg.scale_bar_thickness_px, 0.5f, 12.0f, "%.1f");
			changed |= ImGui::ColorEdit4("Bar Color", cfg.scale_bar_color.data());
			changed |= ImGui::Checkbox("Bar Label", &cfg.scale_bar_label);
		}
	}

	if (ImGui::CollapsingHeader("Labels")) {
		changed |= ImGui::SliderFloat("Label Scale", &cfg.label_scale, 0.5f, 3.0f, "%.2fx");
		int decimals = static_cast<int>(cfg.label_decimals);
		if (ImGui::SliderInt("Decimals (-1 = Automatic)", &decimals, -1, 9)) {
			cfg.label_decimals = decimals;
			changed = true;
		}
		changed |= ImGui::Checkbox("Show Unit Suffix", &cfg.label_unit_suffix);
		changed |= ImGui::ColorEdit4("Label Color", cfg.label_color.data());
		changed |= ImGui::Checkbox("Label Background Panel", &cfg.label_background);
		if (cfg.label_background) {
			changed |= ImGui::SliderFloat("Background Opacity", &cfg.label_background_opacity, 0.0f, 1.0f, "%.2f");
		}
	}

	ImGui::Separator();
	if (ImGui::Button("Reset Spatial Reference Settings", ImVec2(260.0f, 26.0f))) {
		const bool was_enabled = cfg.enabled;
		cfg = SpatialReferenceConfig{};
		cfg.enabled = was_enabled;
		changed = true;
	}
	if (changed) {
		cfg.sanitize();
	}
	return changed;
}

}

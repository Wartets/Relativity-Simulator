#pragma once

#include "relativistic/io/capture/capture_settings_io.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <string>

namespace Relativistic::UI {

enum class SpatialSizeMode : uint32_t {
	Automatic = 0,
	Fixed = 1
};

enum class SpatialCenterMode : uint32_t {
	WorldOrigin = 0,
	CustomPoint = 1,
	Body = 2,
	CameraSnapped = 3
};

enum class SpatialMarkerShape : uint32_t {
	Cross = 0,
	Dot = 1,
	Diamond = 2,
	Ring = 3
};

enum class SpatialPlane : uint32_t {
	XY = 0,
	XZ = 1,
	YZ = 2
};

enum class SpatialCorner : uint32_t {
	TopLeft = 0,
	TopRight = 1,
	BottomLeft = 2,
	BottomRight = 3
};

namespace SpatialPersistence {

inline void write_color(IO::SettingsWriter& writer, const std::string& key, const std::array<float, 4>& color) {
	writer.real(key + ".r", color[0]);
	writer.real(key + ".g", color[1]);
	writer.real(key + ".b", color[2]);
	writer.real(key + ".a", color[3]);
}

inline void read_color(const IO::SettingsReader& reader, const std::string& key, std::array<float, 4>& color) {
	for (size_t i = 0; i < 4; ++i) {
		static constexpr std::array<const char*, 4> suffixes{".r", ".g", ".b", ".a"};
		color[i] = std::clamp(static_cast<float>(reader.real(key + suffixes[i], color[i])), 0.0f, 1.0f);
	}
}

inline void write_vector(IO::SettingsWriter& writer, const std::string& key, const std::array<double, 3>& value) {
	writer.real(key + ".x", value[0]);
	writer.real(key + ".y", value[1]);
	writer.real(key + ".z", value[2]);
}

inline void read_vector(const IO::SettingsReader& reader, const std::string& key, std::array<double, 3>& value) {
	value[0] = reader.real(key + ".x", value[0]);
	value[1] = reader.real(key + ".y", value[1]);
	value[2] = reader.real(key + ".z", value[2]);
}

}

struct SpatialLineStyle {
	bool enabled{true};
	std::array<float, 4> color{1.0f, 1.0f, 1.0f, 1.0f};
	float thickness{1.0f};
	float opacity{1.0f};

	void write(IO::SettingsWriter& writer, const std::string& prefix) const {
		writer.flag(prefix + "enabled", enabled);
		SpatialPersistence::write_color(writer, prefix + "color", color);
		writer.real(prefix + "thickness", thickness);
		writer.real(prefix + "opacity", opacity);
	}

	void read(const IO::SettingsReader& reader, const std::string& prefix) {
		enabled = reader.flag(prefix + "enabled", enabled);
		SpatialPersistence::read_color(reader, prefix + "color", color);
		thickness = std::clamp(static_cast<float>(reader.real(prefix + "thickness", thickness)), 0.25f, 16.0f);
		opacity = std::clamp(static_cast<float>(reader.real(prefix + "opacity", opacity)), 0.0f, 1.0f);
	}
};

struct SpatialReferenceConfig {
	bool enabled{false};
	bool show_in_raytraced_view{true};
	bool show_in_schematic_view{true};
	float global_opacity{1.0f};

	SpatialCenterMode center_mode{SpatialCenterMode::WorldOrigin};
	std::array<double, 3> custom_center{0.0, 0.0, 0.0};
	uint32_t center_body_id{0};

	bool hide_behind_horizon{true};
	bool lens_approximation{true};
	bool depth_fade_enabled{true};
	SpatialSizeMode depth_fade_mode{SpatialSizeMode::Automatic};
	double depth_fade_reference{500.0};
	float depth_fade_minimum{0.2f};
	uint32_t curve_subdivisions{24};
	uint32_t max_lines_per_direction{160};
	uint32_t max_ticks_per_axis{120};

	std::array<SpatialLineStyle, 3> axes{};
	bool axes_show_negative{true};
	SpatialSizeMode axis_length_mode{SpatialSizeMode::Automatic};
	double axis_length{50.0};
	bool axis_arrowheads{true};
	float axis_arrow_size_px{10.0f};
	bool axis_labels{true};

	bool ticks_enabled{true};
	SpatialSizeMode tick_spacing_mode{SpatialSizeMode::Automatic};
	double tick_spacing{5.0};
	float tick_length_px{7.0f};
	float tick_thickness_px{1.4f};
	bool tick_labels{true};
	uint32_t tick_label_interval{2};

	std::array<bool, 3> grid_planes{true, false, false};
	SpatialLineStyle grid_minor{};
	SpatialLineStyle grid_major{};
	SpatialSizeMode grid_extent_mode{SpatialSizeMode::Automatic};
	double grid_extent{100.0};
	SpatialSizeMode grid_spacing_mode{SpatialSizeMode::Automatic};
	double grid_spacing{10.0};
	uint32_t grid_major_interval{5};
	std::array<double, 3> grid_plane_offsets{0.0, 0.0, 0.0};
	bool grid_radial_fade{true};

	bool rings_enabled{false};
	SpatialPlane ring_plane{SpatialPlane::XY};
	SpatialSizeMode ring_spacing_mode{SpatialSizeMode::Automatic};
	double ring_spacing{10.0};
	uint32_t ring_count{8};
	SpatialLineStyle ring_style{};
	bool ring_labels{true};

	bool origin_marker_enabled{true};
	SpatialMarkerShape origin_marker_shape{SpatialMarkerShape::Cross};
	float origin_marker_size_px{8.0f};
	std::array<float, 4> origin_marker_color{1.0f, 1.0f, 1.0f, 1.0f};

	bool scale_bar_enabled{false};
	SpatialCorner scale_bar_corner{SpatialCorner::BottomLeft};
	float scale_bar_margin_px{28.0f};
	float scale_bar_target_px{160.0f};
	float scale_bar_thickness_px{2.5f};
	std::array<float, 4> scale_bar_color{0.95f, 0.95f, 1.0f, 1.0f};
	bool scale_bar_label{true};

	float label_scale{1.0f};
	int32_t label_decimals{-1};
	bool label_unit_suffix{true};
	bool label_background{true};
	float label_background_opacity{0.55f};
	std::array<float, 4> label_color{0.92f, 0.95f, 1.0f, 1.0f};

	SpatialReferenceConfig() noexcept {
		axes[0].color = {1.0f, 0.32f, 0.32f, 1.0f};
		axes[1].color = {0.40f, 0.90f, 0.40f, 1.0f};
		axes[2].color = {0.40f, 0.60f, 1.0f, 1.0f};
		for (SpatialLineStyle& axis : axes) {
			axis.thickness = 2.0f;
		}
		grid_minor.color = {0.55f, 0.65f, 0.80f, 1.0f};
		grid_minor.opacity = 0.18f;
		grid_minor.thickness = 1.0f;
		grid_major.color = {0.65f, 0.78f, 0.95f, 1.0f};
		grid_major.opacity = 0.45f;
		grid_major.thickness = 1.3f;
		ring_style.color = {1.0f, 0.80f, 0.40f, 1.0f};
		ring_style.opacity = 0.5f;
		ring_style.thickness = 1.2f;
	}

	void sanitize() noexcept {
		global_opacity = std::clamp(global_opacity, 0.0f, 1.0f);
		depth_fade_reference = std::max(depth_fade_reference, 1.0e-6);
		depth_fade_minimum = std::clamp(depth_fade_minimum, 0.0f, 1.0f);
		curve_subdivisions = std::clamp<uint32_t>(curve_subdivisions, 1U, 256U);
		max_lines_per_direction = std::clamp<uint32_t>(max_lines_per_direction, 2U, 1000U);
		max_ticks_per_axis = std::clamp<uint32_t>(max_ticks_per_axis, 1U, 1000U);
		axis_length = std::max(axis_length, 1.0e-6);
		axis_arrow_size_px = std::clamp(axis_arrow_size_px, 2.0f, 60.0f);
		tick_spacing = std::max(tick_spacing, 1.0e-9);
		tick_length_px = std::clamp(tick_length_px, 1.0f, 60.0f);
		tick_thickness_px = std::clamp(tick_thickness_px, 0.25f, 10.0f);
		tick_label_interval = std::clamp<uint32_t>(tick_label_interval, 1U, 100U);
		grid_extent = std::max(grid_extent, 1.0e-6);
		grid_spacing = std::max(grid_spacing, 1.0e-9);
		grid_major_interval = std::clamp<uint32_t>(grid_major_interval, 1U, 100U);
		ring_spacing = std::max(ring_spacing, 1.0e-9);
		ring_count = std::clamp<uint32_t>(ring_count, 1U, 64U);
		origin_marker_size_px = std::clamp(origin_marker_size_px, 2.0f, 80.0f);
		scale_bar_margin_px = std::clamp(scale_bar_margin_px, 0.0f, 400.0f);
		scale_bar_target_px = std::clamp(scale_bar_target_px, 40.0f, 1200.0f);
		scale_bar_thickness_px = std::clamp(scale_bar_thickness_px, 0.5f, 12.0f);
		label_scale = std::clamp(label_scale, 0.5f, 3.0f);
		label_decimals = std::clamp<int32_t>(label_decimals, -1, 9);
		label_background_opacity = std::clamp(label_background_opacity, 0.0f, 1.0f);
	}

	void write_settings(IO::SettingsWriter& writer) const {
		const std::string p = "spatial_ref_";
		writer.flag(p + "enabled", enabled);
		writer.flag(p + "show_raytraced", show_in_raytraced_view);
		writer.flag(p + "show_schematic", show_in_schematic_view);
		writer.real(p + "opacity", global_opacity);
		writer.enumeration(p + "center_mode", center_mode);
		SpatialPersistence::write_vector(writer, p + "custom_center", custom_center);
		writer.unsigned_value(p + "center_body", center_body_id);
		writer.flag(p + "hide_horizon", hide_behind_horizon);
		writer.flag(p + "lens", lens_approximation);
		writer.flag(p + "depth_fade", depth_fade_enabled);
		writer.enumeration(p + "depth_fade_mode", depth_fade_mode);
		writer.real(p + "depth_fade_reference", depth_fade_reference);
		writer.real(p + "depth_fade_minimum", depth_fade_minimum);
		writer.unsigned_value(p + "subdivisions", curve_subdivisions);
		writer.unsigned_value(p + "max_lines", max_lines_per_direction);
		writer.unsigned_value(p + "max_ticks", max_ticks_per_axis);
		for (size_t i = 0; i < axes.size(); ++i) {
			axes[i].write(writer, p + "axis" + std::to_string(i) + ".");
		}
		writer.flag(p + "axis_negative", axes_show_negative);
		writer.enumeration(p + "axis_length_mode", axis_length_mode);
		writer.real(p + "axis_length", axis_length);
		writer.flag(p + "axis_arrows", axis_arrowheads);
		writer.real(p + "axis_arrow_size", axis_arrow_size_px);
		writer.flag(p + "axis_labels", axis_labels);
		writer.flag(p + "ticks", ticks_enabled);
		writer.enumeration(p + "tick_spacing_mode", tick_spacing_mode);
		writer.real(p + "tick_spacing", tick_spacing);
		writer.real(p + "tick_length", tick_length_px);
		writer.real(p + "tick_thickness", tick_thickness_px);
		writer.flag(p + "tick_labels", tick_labels);
		writer.unsigned_value(p + "tick_label_interval", tick_label_interval);
		for (size_t i = 0; i < grid_planes.size(); ++i) {
			writer.flag(p + "grid_plane" + std::to_string(i), grid_planes[i]);
		}
		grid_minor.write(writer, p + "grid_minor.");
		grid_major.write(writer, p + "grid_major.");
		writer.enumeration(p + "grid_extent_mode", grid_extent_mode);
		writer.real(p + "grid_extent", grid_extent);
		writer.enumeration(p + "grid_spacing_mode", grid_spacing_mode);
		writer.real(p + "grid_spacing", grid_spacing);
		writer.unsigned_value(p + "grid_major_interval", grid_major_interval);
		SpatialPersistence::write_vector(writer, p + "grid_offsets", grid_plane_offsets);
		writer.flag(p + "grid_radial_fade", grid_radial_fade);
		writer.flag(p + "rings", rings_enabled);
		writer.enumeration(p + "ring_plane", ring_plane);
		writer.enumeration(p + "ring_spacing_mode", ring_spacing_mode);
		writer.real(p + "ring_spacing", ring_spacing);
		writer.unsigned_value(p + "ring_count", ring_count);
		ring_style.write(writer, p + "ring_style.");
		writer.flag(p + "ring_labels", ring_labels);
		writer.flag(p + "origin_marker", origin_marker_enabled);
		writer.enumeration(p + "origin_shape", origin_marker_shape);
		writer.real(p + "origin_size", origin_marker_size_px);
		SpatialPersistence::write_color(writer, p + "origin_color", origin_marker_color);
		writer.flag(p + "scale_bar", scale_bar_enabled);
		writer.enumeration(p + "scale_bar_corner", scale_bar_corner);
		writer.real(p + "scale_bar_margin", scale_bar_margin_px);
		writer.real(p + "scale_bar_target", scale_bar_target_px);
		writer.real(p + "scale_bar_thickness", scale_bar_thickness_px);
		SpatialPersistence::write_color(writer, p + "scale_bar_color", scale_bar_color);
		writer.flag(p + "scale_bar_label", scale_bar_label);
		writer.real(p + "label_scale", label_scale);
		writer.signed_value(p + "label_decimals", label_decimals);
		writer.flag(p + "label_unit", label_unit_suffix);
		writer.flag(p + "label_background", label_background);
		writer.real(p + "label_background_opacity", label_background_opacity);
		SpatialPersistence::write_color(writer, p + "label_color", label_color);
	}

	void read_settings(const IO::SettingsReader& reader) {
		const std::string p = "spatial_ref_";
		enabled = reader.flag(p + "enabled", enabled);
		show_in_raytraced_view = reader.flag(p + "show_raytraced", show_in_raytraced_view);
		show_in_schematic_view = reader.flag(p + "show_schematic", show_in_schematic_view);
		global_opacity = static_cast<float>(reader.real(p + "opacity", global_opacity));
		center_mode = reader.enumeration(p + "center_mode", center_mode, SpatialCenterMode::CameraSnapped);
		SpatialPersistence::read_vector(reader, p + "custom_center", custom_center);
		center_body_id = reader.unsigned_value(p + "center_body", center_body_id);
		hide_behind_horizon = reader.flag(p + "hide_horizon", hide_behind_horizon);
		lens_approximation = reader.flag(p + "lens", lens_approximation);
		depth_fade_enabled = reader.flag(p + "depth_fade", depth_fade_enabled);
		depth_fade_mode = reader.enumeration(p + "depth_fade_mode", depth_fade_mode, SpatialSizeMode::Fixed);
		depth_fade_reference = reader.real(p + "depth_fade_reference", depth_fade_reference);
		depth_fade_minimum = static_cast<float>(reader.real(p + "depth_fade_minimum", depth_fade_minimum));
		curve_subdivisions = reader.unsigned_value(p + "subdivisions", curve_subdivisions);
		max_lines_per_direction = reader.unsigned_value(p + "max_lines", max_lines_per_direction);
		max_ticks_per_axis = reader.unsigned_value(p + "max_ticks", max_ticks_per_axis);
		for (size_t i = 0; i < axes.size(); ++i) {
			axes[i].read(reader, p + "axis" + std::to_string(i) + ".");
		}
		axes_show_negative = reader.flag(p + "axis_negative", axes_show_negative);
		axis_length_mode = reader.enumeration(p + "axis_length_mode", axis_length_mode, SpatialSizeMode::Fixed);
		axis_length = reader.real(p + "axis_length", axis_length);
		axis_arrowheads = reader.flag(p + "axis_arrows", axis_arrowheads);
		axis_arrow_size_px = static_cast<float>(reader.real(p + "axis_arrow_size", axis_arrow_size_px));
		axis_labels = reader.flag(p + "axis_labels", axis_labels);
		ticks_enabled = reader.flag(p + "ticks", ticks_enabled);
		tick_spacing_mode = reader.enumeration(p + "tick_spacing_mode", tick_spacing_mode, SpatialSizeMode::Fixed);
		tick_spacing = reader.real(p + "tick_spacing", tick_spacing);
		tick_length_px = static_cast<float>(reader.real(p + "tick_length", tick_length_px));
		tick_thickness_px = static_cast<float>(reader.real(p + "tick_thickness", tick_thickness_px));
		tick_labels = reader.flag(p + "tick_labels", tick_labels);
		tick_label_interval = reader.unsigned_value(p + "tick_label_interval", tick_label_interval);
		for (size_t i = 0; i < grid_planes.size(); ++i) {
			grid_planes[i] = reader.flag(p + "grid_plane" + std::to_string(i), grid_planes[i]);
		}
		grid_minor.read(reader, p + "grid_minor.");
		grid_major.read(reader, p + "grid_major.");
		grid_extent_mode = reader.enumeration(p + "grid_extent_mode", grid_extent_mode, SpatialSizeMode::Fixed);
		grid_extent = reader.real(p + "grid_extent", grid_extent);
		grid_spacing_mode = reader.enumeration(p + "grid_spacing_mode", grid_spacing_mode, SpatialSizeMode::Fixed);
		grid_spacing = reader.real(p + "grid_spacing", grid_spacing);
		grid_major_interval = reader.unsigned_value(p + "grid_major_interval", grid_major_interval);
		SpatialPersistence::read_vector(reader, p + "grid_offsets", grid_plane_offsets);
		grid_radial_fade = reader.flag(p + "grid_radial_fade", grid_radial_fade);
		rings_enabled = reader.flag(p + "rings", rings_enabled);
		ring_plane = reader.enumeration(p + "ring_plane", ring_plane, SpatialPlane::YZ);
		ring_spacing_mode = reader.enumeration(p + "ring_spacing_mode", ring_spacing_mode, SpatialSizeMode::Fixed);
		ring_spacing = reader.real(p + "ring_spacing", ring_spacing);
		ring_count = reader.unsigned_value(p + "ring_count", ring_count);
		ring_style.read(reader, p + "ring_style.");
		ring_labels = reader.flag(p + "ring_labels", ring_labels);
		origin_marker_enabled = reader.flag(p + "origin_marker", origin_marker_enabled);
		origin_marker_shape = reader.enumeration(p + "origin_shape", origin_marker_shape, SpatialMarkerShape::Ring);
		origin_marker_size_px = static_cast<float>(reader.real(p + "origin_size", origin_marker_size_px));
		SpatialPersistence::read_color(reader, p + "origin_color", origin_marker_color);
		scale_bar_enabled = reader.flag(p + "scale_bar", scale_bar_enabled);
		scale_bar_corner = reader.enumeration(p + "scale_bar_corner", scale_bar_corner, SpatialCorner::BottomRight);
		scale_bar_margin_px = static_cast<float>(reader.real(p + "scale_bar_margin", scale_bar_margin_px));
		scale_bar_target_px = static_cast<float>(reader.real(p + "scale_bar_target", scale_bar_target_px));
		scale_bar_thickness_px = static_cast<float>(reader.real(p + "scale_bar_thickness", scale_bar_thickness_px));
		SpatialPersistence::read_color(reader, p + "scale_bar_color", scale_bar_color);
		scale_bar_label = reader.flag(p + "scale_bar_label", scale_bar_label);
		label_scale = static_cast<float>(reader.real(p + "label_scale", label_scale));
		label_decimals = reader.signed_value(p + "label_decimals", label_decimals);
		label_unit_suffix = reader.flag(p + "label_unit", label_unit_suffix);
		label_background = reader.flag(p + "label_background", label_background);
		label_background_opacity = static_cast<float>(reader.real(p + "label_background_opacity", label_background_opacity));
		SpatialPersistence::read_color(reader, p + "label_color", label_color);
		sanitize();
	}
};

}

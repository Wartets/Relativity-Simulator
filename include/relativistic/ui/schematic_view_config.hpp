#pragma once

#include <array>
#include <cstdint>
#include <algorithm>
#include <cmath>

namespace Relativistic::UI {

enum class SchematicObjectShape : uint32_t {
	Point = 0,
	SphereFixedRadius = 1,
	SphereByParameter = 2
};

enum class SchematicSphereStyle : uint32_t {
	Opaque = 0,
	Translucent = 1,
	Wireframe = 2
};

enum class SchematicColorCodingMode : uint32_t {
	Uniform = 0,
	ByMass = 1,
	BySpeed = 2,
	BySpinMagnitude = 3,
	ByDistanceFromCenter = 4,
	ByKineticEnergy = 5
};

enum class SchematicSphereParameterSource : uint32_t {
	Mass = 0,
	Speed = 1,
	KineticEnergy = 2,
	SpinMagnitude = 3,
	PhysicalRadius = 4
};

enum class SchematicVectorKind : uint32_t {
	Velocity = 0,
	TotalForce = 1,
	Spin = 2,
	RotationAxis = 3
};

inline constexpr size_t SCHEMATIC_VECTOR_KIND_COUNT = 4;

enum class SchematicVectorPlacement : uint32_t {
	AtCenter = 0,
	AtSurface = 1
};

enum class SchematicVectorOrientationMode : uint32_t {
	FromPhysicalQuantity = 0,
	FixedWorldAxis = 1
};

struct SchematicVectorStyle {
	bool enabled{false};
	SchematicVectorPlacement placement{SchematicVectorPlacement::AtCenter};
	SchematicVectorOrientationMode orientation_mode{SchematicVectorOrientationMode::FromPhysicalQuantity};
	std::array<double, 3> fixed_direction{0.0, 0.0, 1.0};
	double length_scale{1.0};
	double min_pixel_length{10.0};
	double max_pixel_length{140.0};
	double head_size_px{7.0};
	double line_thickness_px{2.0};
	bool use_automatic_color{true};
	std::array<float, 4> manual_color{1.0f, 1.0f, 1.0f, 1.0f};
};

struct SchematicObjectDisplayConfig {
	SchematicObjectShape shape{SchematicObjectShape::SphereFixedRadius};
	SchematicSphereStyle sphere_style{SchematicSphereStyle::Opaque};
	SchematicColorCodingMode color_mode{SchematicColorCodingMode::Uniform};
	SchematicSphereParameterSource parameter_source{SchematicSphereParameterSource::Mass};
	std::array<float, 4> uniform_color{0.62f, 0.75f, 1.0f, 1.0f};
	double radius_scale{1.0};
	double point_pixel_radius{3.5};
	double parameter_pixel_scale{1.0};
	double sphere_min_pixel_radius{2.5};
	double sphere_max_pixel_radius{220.0};
	double translucency_alpha{0.35};
	int wireframe_rings{5};
	int wireframe_segments{20};
	bool show_tag{true};
	bool show_id_in_tag{true};
	bool show_mass_in_tag{false};
	bool show_speed_in_tag{false};
};

struct SchematicViewConfig {
	bool respect_active_projection_mode{true};

	bool show_central_object{true};
	bool show_bodies{true};
	bool show_background_grid{true};
	bool show_field_lines{false};
	bool show_trails{true};
	bool show_orbit_predictions{false};
	bool show_vectors{true};
	bool show_tags{true};

	double grid_opacity{0.55};
	int grid_latitude_lines{6};
	int grid_longitude_lines{12};
	int grid_segments{48};
	double grid_radius_scale{40.0};

	int field_line_count{28};
	double field_line_opacity{0.32};
	double field_line_extent_scale{28.0};
	bool field_line_inward_arrows{true};

	double trail_duration_seconds{14.0};
	int trail_max_points{260};
	double trail_sample_interval_seconds{0.05};
	double trail_fade_power{1.4};
	double trail_line_thickness{1.6};

	int orbit_prediction_segments{160};
	double orbit_prediction_opacity{0.45};
	double orbit_prediction_thickness{1.4};
	double orbit_prediction_max_eccentricity{0.98};

	SchematicObjectDisplayConfig central_object_style{};
	SchematicObjectDisplayConfig body_style{};

	std::array<SchematicVectorStyle, SCHEMATIC_VECTOR_KIND_COUNT> vectors{
		SchematicVectorStyle{},
		SchematicVectorStyle{},
		SchematicVectorStyle{},
		SchematicVectorStyle{}
	};

	SchematicViewConfig() noexcept {
		vectors[static_cast<size_t>(SchematicVectorKind::Velocity)].manual_color = {0.3f, 0.95f, 0.5f, 1.0f};
		vectors[static_cast<size_t>(SchematicVectorKind::TotalForce)].manual_color = {1.0f, 0.55f, 0.25f, 1.0f};
		vectors[static_cast<size_t>(SchematicVectorKind::Spin)].manual_color = {0.6f, 0.7f, 1.0f, 1.0f};
		vectors[static_cast<size_t>(SchematicVectorKind::RotationAxis)].manual_color = {1.0f, 0.85f, 0.35f, 1.0f};
		vectors[static_cast<size_t>(SchematicVectorKind::RotationAxis)].placement = SchematicVectorPlacement::AtSurface;
		vectors[static_cast<size_t>(SchematicVectorKind::Spin)].placement = SchematicVectorPlacement::AtCenter;
		body_style.uniform_color = {0.62f, 0.75f, 1.0f, 1.0f};
		central_object_style.uniform_color = {0.92f, 0.92f, 0.96f, 1.0f};
		central_object_style.color_mode = SchematicColorCodingMode::Uniform;
	}

	[[nodiscard]] SchematicVectorStyle& vector_style(SchematicVectorKind kind) noexcept {
		return vectors[static_cast<size_t>(kind)];
	}

	[[nodiscard]] const SchematicVectorStyle& vector_style(SchematicVectorKind kind) const noexcept {
		return vectors[static_cast<size_t>(kind)];
	}
};

[[nodiscard]] inline std::array<float, 3> schematic_heatmap_gradient(double t) noexcept {
	const double c = std::clamp(t, 0.0, 1.0);
	static constexpr std::array<std::array<float, 3>, 5> stops{{
		{0.16f, 0.32f, 0.85f},
		{0.20f, 0.75f, 0.85f},
		{0.35f, 0.85f, 0.30f},
		{0.95f, 0.80f, 0.20f},
		{0.95f, 0.25f, 0.20f}
	}};
	const double scaled = c * static_cast<double>(stops.size() - 1);
	const size_t idx0 = std::min(static_cast<size_t>(scaled), stops.size() - 2);
	const size_t idx1 = idx0 + 1;
	const float frac = static_cast<float>(scaled - static_cast<double>(idx0));
	return {
		stops[idx0][0] + (stops[idx1][0] - stops[idx0][0]) * frac,
		stops[idx0][1] + (stops[idx1][1] - stops[idx0][1]) * frac,
		stops[idx0][2] + (stops[idx1][2] - stops[idx0][2]) * frac
	};
}

}

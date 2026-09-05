#pragma once

#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/dynamics/pn_body.hpp"
#include "relativistic/observer/camera_projections.hpp"
#include "relativistic/ui/schematic_view_config.hpp"
#include <imgui.h>
#include <array>
#include <vector>
#include <deque>
#include <unordered_map>
#include <span>
#include <utility>
#include <optional>
#include <cmath>
#include <numbers>
#include <algorithm>
#include <string>
#include <limits>

namespace Relativistic::UI {

class SchematicViewRenderer {
private:
	struct ProjectedPoint {
		bool visible{false};
		ImVec2 screen{0.0f, 0.0f};
		double forward_depth{0.0};
	};

	struct TrailSample {
		std::array<double, 3> position{};
		double timestamp{0.0};
	};

	std::array<double, 3> tetrad_forward_{1.0, 0.0, 0.0};
	std::array<double, 3> tetrad_right_{0.0, 1.0, 0.0};
	std::array<double, 3> tetrad_up_{0.0, 0.0, 1.0};
	std::array<double, 3> camera_position_{0.0, 0.0, 0.0};
	double fov_rad_{1.0471975511965976};
	double aspect_{1.0};
	ImVec2 rect_min_{};
	ImVec2 rect_size_{};
	Observer::ProjectionMode projection_mode_{Observer::ProjectionMode::Pinhole};
	std::unordered_map<uint32_t, std::deque<TrailSample>> body_trails_{};

	[[nodiscard]] static int clamp8(double value) noexcept {
		return static_cast<int>(std::clamp(value, 0.0, 255.0));
	}

	[[nodiscard]] std::optional<std::pair<double, double>> direction_to_screen_uv(double fwd, double right, double up) const noexcept {
		constexpr double eps = 1e-12;
		switch (projection_mode_) {
			case Observer::ProjectionMode::FisheyeEquidistant: {
				const double n_len = std::sqrt(fwd * fwd + right * right + up * up);
				if (n_len <= eps) return std::nullopt;
				const double n1 = std::clamp(fwd / n_len, -1.0, 1.0);
				const double theta = std::acos(n1);
				const double sin_t = std::sin(theta);
				if (sin_t < eps) return std::make_pair(0.0, 0.0);
				if (fov_rad_ < eps) return std::nullopt;
				const double r = (2.0 * theta) / fov_rad_;
				return std::make_pair((right / n_len) * r / sin_t, -(up / n_len) * r / sin_t);
			}
			case Observer::ProjectionMode::FisheyeStereographic: {
				const double n_len = std::sqrt(fwd * fwd + right * right + up * up);
				if (n_len <= eps) return std::nullopt;
				const double n1 = std::clamp(fwd / n_len, -1.0, 1.0);
				const double theta = std::acos(n1);
				const double max_fov = std::clamp(fov_rad_ * 1.6, 0.1, 260.0 * std::numbers::pi_v<double> / 180.0);
				const double tan_half_max = std::tan(max_fov * 0.25);
				if (tan_half_max < eps) return std::nullopt;
				const double sin_t = std::sin(theta);
				if (sin_t < eps) return std::make_pair(0.0, 0.0);
				const double r = std::tan(theta * 0.5) / tan_half_max;
				return std::make_pair((right / n_len) * r / sin_t, -(up / n_len) * r / sin_t);
			}
			case Observer::ProjectionMode::FisheyeOrthographic: {
				const double n_len = std::sqrt(fwd * fwd + right * right + up * up);
				if (n_len <= eps) return std::nullopt;
				const double n1 = std::clamp(fwd / n_len, -1.0, 1.0);
				if (n1 < 0.0) return std::nullopt;
				const double sin_t = std::sqrt(std::max(1.0 - n1 * n1, 0.0));
				const double sin_half_fov = std::sin(std::clamp(fov_rad_ * 0.5, 0.01, std::numbers::pi_v<double> * 0.499));
				if (sin_half_fov < eps) return std::nullopt;
				if (sin_t < eps) return std::make_pair(0.0, 0.0);
				const double r = sin_t / sin_half_fov;
				return std::make_pair((right / n_len) * r / sin_t, -(up / n_len) * r / sin_t);
			}
			case Observer::ProjectionMode::Equirectangular360: {
				const double n_len = std::sqrt(fwd * fwd + right * right + up * up);
				if (n_len <= eps) return std::nullopt;
				const double n1 = fwd / n_len;
				const double n2 = std::clamp(up / n_len, -1.0, 1.0);
				const double n3 = right / n_len;
				const double theta = std::acos(n2);
				const double phi = std::atan2(n3, n1);
				const double fov_scale = fov_rad_ / (60.0 * std::numbers::pi_v<double> / 180.0);
				if (fov_scale < eps) return std::nullopt;
				const double u = phi / (std::numbers::pi_v<double> * fov_scale);
				const double v = (std::numbers::pi_v<double> * 0.5 - theta) / (std::numbers::pi_v<double> * 0.5 * fov_scale);
				return std::make_pair(u, v);
			}
			case Observer::ProjectionMode::Pinhole:
			case Observer::ProjectionMode::AutoZoomAberration:
			case Observer::ProjectionMode::PaniniCylindrical:
			case Observer::ProjectionMode::HammerAitoff:
			default: {
				if (fwd <= eps) return std::nullopt;
				const double tan_half_fov = std::tan(fov_rad_ * 0.5);
				if (tan_half_fov < eps) return std::nullopt;
				return std::make_pair((right / fwd) / tan_half_fov, -(up / fwd) / tan_half_fov);
			}
		}
	}

	[[nodiscard]] ProjectedPoint project(const std::array<double, 3>& world_pos) const noexcept {
		const double dx = world_pos[0] - camera_position_[0];
		const double dy = world_pos[1] - camera_position_[1];
		const double dz = world_pos[2] - camera_position_[2];

		const double fwd = dx * tetrad_forward_[0] + dy * tetrad_forward_[1] + dz * tetrad_forward_[2];
		const double right = dx * tetrad_right_[0] + dy * tetrad_right_[1] + dz * tetrad_right_[2];
		const double up = dx * tetrad_up_[0] + dy * tetrad_up_[1] + dz * tetrad_up_[2];

		ProjectedPoint pt;
		pt.forward_depth = fwd;

		const auto uv = direction_to_screen_uv(fwd, right, up);
		if (!uv.has_value()) {
			return pt;
		}

		const bool is_allsky = (projection_mode_ == Observer::ProjectionMode::Equirectangular360 || projection_mode_ == Observer::ProjectionMode::HammerAitoff);
		const double u_screen = is_allsky ? uv->first : (uv->first / std::max(aspect_, 1e-6));
		const double v_screen = uv->second;

		if (!std::isfinite(u_screen) || !std::isfinite(v_screen)) {
			return pt;
		}

		const double rect_width = static_cast<double>(rect_size_.x);
		const double rect_height = static_cast<double>(rect_size_.y);

		pt.screen.x = rect_min_.x + static_cast<float>((u_screen * 0.5 + 0.5) * rect_width);
		pt.screen.y = rect_min_.y + static_cast<float>((v_screen * 0.5 + 0.5) * rect_height);
		pt.visible = true;
		return pt;
	}

	[[nodiscard]] double compute_screen_radius(const std::array<double, 3>& world_center, double physical_radius) const noexcept {
		if (physical_radius <= 0.0) return 0.0;
		const auto center_proj = project(world_center);
		if (!center_proj.visible) return 0.0;

		const std::array<double, 3> offset_point{
			world_center[0] + tetrad_right_[0] * physical_radius,
			world_center[1] + tetrad_right_[1] * physical_radius,
			world_center[2] + tetrad_right_[2] * physical_radius
		};
		const auto edge_proj = project(offset_point);
		if (!edge_proj.visible) return 0.0;

		const double dx = static_cast<double>(edge_proj.screen.x) - static_cast<double>(center_proj.screen.x);
		const double dy = static_cast<double>(edge_proj.screen.y) - static_cast<double>(center_proj.screen.y);
		return std::sqrt(dx * dx + dy * dy);
	}

	void draw_projected_sphere_wireframe(
		ImDrawList* draw_list,
		const std::array<double, 3>& center,
		double radius,
		int lat_lines,
		int lon_lines,
		int segments,
		ImU32 major_color,
		ImU32 minor_color,
		float thickness
	) const {
		lat_lines = std::max(lat_lines, 1);
		lon_lines = std::max(lon_lines, 1);
		segments = std::max(segments, 8);

		for (int lat = 1; lat < lat_lines; ++lat) {
			const double theta = std::numbers::pi_v<double> * static_cast<double>(lat) / static_cast<double>(lat_lines);
			const bool major = (lat == lat_lines / 2);
			std::vector<ImVec2> pts;
			pts.reserve(static_cast<size_t>(segments) + 1);
			bool any_visible = false;
			for (int i = 0; i <= segments; ++i) {
				const double phi = 2.0 * std::numbers::pi_v<double> * static_cast<double>(i) / static_cast<double>(segments);
				const std::array<double, 3> p{
					center[0] + radius * std::sin(theta) * std::cos(phi),
					center[1] + radius * std::sin(theta) * std::sin(phi),
					center[2] + radius * std::cos(theta)
				};
				const auto proj = project(p);
				if (proj.visible) {
					any_visible = true;
					pts.push_back(proj.screen);
				}
			}
			if (any_visible && pts.size() >= 2) {
				draw_list->AddPolyline(pts.data(), static_cast<int>(pts.size()), major ? major_color : minor_color, ImDrawFlags_None, major ? thickness * 1.4f : thickness);
			}
		}

		for (int lon = 0; lon < lon_lines; ++lon) {
			const double phi = 2.0 * std::numbers::pi_v<double> * static_cast<double>(lon) / static_cast<double>(lon_lines);
			std::vector<ImVec2> pts;
			pts.reserve(static_cast<size_t>(segments) + 1);
			bool any_visible = false;
			for (int i = 0; i <= segments; ++i) {
				const double theta = std::numbers::pi_v<double> * static_cast<double>(i) / static_cast<double>(segments);
				const std::array<double, 3> p{
					center[0] + radius * std::sin(theta) * std::cos(phi),
					center[1] + radius * std::sin(theta) * std::sin(phi),
					center[2] + radius * std::cos(theta)
				};
				const auto proj = project(p);
				if (proj.visible) {
					any_visible = true;
					pts.push_back(proj.screen);
				}
			}
			if (any_visible && pts.size() >= 2) {
				draw_list->AddPolyline(pts.data(), static_cast<int>(pts.size()), minor_color, ImDrawFlags_None, thickness);
			}
		}
	}

	void draw_polyline_3d(ImDrawList* draw_list, const std::vector<std::array<double, 3>>& points, ImU32 color, float thickness, bool closed) const {
		if (points.size() < 2) return;
		std::vector<ImVec2> screen_pts;
		screen_pts.reserve(points.size());
		for (const auto& p : points) {
			const auto proj = project(p);
			if (!proj.visible) continue;
			screen_pts.push_back(proj.screen);
		}
		if (screen_pts.size() < 2) return;
		draw_list->AddPolyline(screen_pts.data(), static_cast<int>(screen_pts.size()), color, closed ? ImDrawFlags_Closed : ImDrawFlags_None, thickness);
	}

	void draw_offscreen_indicator(ImDrawList* draw_list, const std::array<double, 3>& world_position, const std::string& label) const {
		if (rect_size_.x <= 0.0f || rect_size_.y <= 0.0f) return;

		const double dx = world_position[0] - camera_position_[0];
		const double dy = world_position[1] - camera_position_[1];
		const double dz = world_position[2] - camera_position_[2];

		const double cam_right = dx * tetrad_right_[0] + dy * tetrad_right_[1] + dz * tetrad_right_[2];
		const double cam_up = dx * tetrad_up_[0] + dy * tetrad_up_[1] + dz * tetrad_up_[2];

		const double bearing_x = cam_right;
		const double bearing_y = -cam_up;
		const double bearing_length = std::sqrt(bearing_x * bearing_x + bearing_y * bearing_y);

		ImVec2 dir{};
		if (bearing_length > 1e-12) {
			dir.x = static_cast<float>(bearing_x / bearing_length);
			dir.y = static_cast<float>(bearing_y / bearing_length);
		} else {
			dir = ImVec2(0.0f, -1.0f);
		}

		const ImVec2 center(rect_min_.x + rect_size_.x * 0.5f, rect_min_.y + rect_size_.y * 0.5f);

		constexpr float margin = 40.0f;
		const float half_w = std::max(rect_size_.x * 0.5f - margin, 1.0f);
		const float half_h = std::max(rect_size_.y * 0.5f - margin, 1.0f);

		float scale_x = std::numeric_limits<float>::max();
		float scale_y = std::numeric_limits<float>::max();
		if (std::abs(dir.x) > 1e-6f) scale_x = half_w / std::abs(dir.x);
		if (std::abs(dir.y) > 1e-6f) scale_y = half_h / std::abs(dir.y);
		const float scale = std::min(scale_x, scale_y);
		if (!std::isfinite(scale)) return;

		const ImVec2 edge_point(center.x + dir.x * scale, center.y + dir.y * scale);
		const ImVec2 perp{-dir.y, dir.x};
		constexpr float arrow_size = 9.0f;
		const ImVec2 tip(edge_point.x + dir.x * arrow_size, edge_point.y + dir.y * arrow_size);
		const ImVec2 base_a(edge_point.x - dir.x * arrow_size + perp.x * arrow_size * 0.6f, edge_point.y - dir.y * arrow_size + perp.y * arrow_size * 0.6f);
		const ImVec2 base_b(edge_point.x - dir.x * arrow_size - perp.x * arrow_size * 0.6f, edge_point.y - dir.y * arrow_size - perp.y * arrow_size * 0.6f);

		draw_list->AddTriangleFilled(tip, base_a, base_b, IM_COL32(255, 210, 90, 220));

		const ImVec2 text_position(edge_point.x + dir.x * 14.0f, edge_point.y + dir.y * 14.0f - 6.0f);
		draw_list->AddText(text_position, IM_COL32(255, 220, 150, 220), label.c_str());
	}

	[[nodiscard]] static double body_scalar_value(const Dynamics::PostNewtonianBody& body, SchematicColorCodingMode mode) noexcept {
		switch (mode) {
			case SchematicColorCodingMode::ByMass:
				return body.mass;
			case SchematicColorCodingMode::BySpeed:
				return body.speed();
			case SchematicColorCodingMode::BySpinMagnitude:
				return body.spin_magnitude();
			case SchematicColorCodingMode::ByKineticEnergy:
				return body.kinetic_energy();
			case SchematicColorCodingMode::ByDistanceFromCenter: {
				const double dx = body.position[0];
				const double dy = body.position[1];
				const double dz = body.position[2];
				return std::sqrt(dx * dx + dy * dy + dz * dz);
			}
			case SchematicColorCodingMode::Uniform:
			default:
				return 0.0;
		}
	}

	[[nodiscard]] static double body_parameter_source_value(const Dynamics::PostNewtonianBody& body, SchematicSphereParameterSource source) noexcept {
		switch (source) {
			case SchematicSphereParameterSource::Speed:
				return body.speed();
			case SchematicSphereParameterSource::KineticEnergy:
				return body.kinetic_energy();
			case SchematicSphereParameterSource::SpinMagnitude:
				return body.spin_magnitude();
			case SchematicSphereParameterSource::PhysicalRadius:
				return body.radius;
			case SchematicSphereParameterSource::Mass:
			default:
				return body.mass;
		}
	}

	[[nodiscard]] static ImU32 compute_coded_color(SchematicColorCodingMode mode, double value, double min_v, double max_v, const std::array<float, 4>& fallback_color) noexcept {
		if (mode == SchematicColorCodingMode::Uniform || max_v <= min_v) {
			return ImGui::ColorConvertFloat4ToU32(ImVec4(fallback_color[0], fallback_color[1], fallback_color[2], fallback_color[3]));
		}
		const double t = std::clamp((value - min_v) / (max_v - min_v), 0.0, 1.0);
		const auto rgb = schematic_heatmap_gradient(t);
		return ImGui::ColorConvertFloat4ToU32(ImVec4(rgb[0], rgb[1], rgb[2], fallback_color[3]));
	}

	void draw_object_shape(
		ImDrawList* draw_list,
		const std::array<double, 3>& center_world,
		double physical_radius,
		const SchematicObjectDisplayConfig& style,
		ImU32 color,
		double pixel_radius_override
	) const {
		const auto center_proj = project(center_world);
		if (!center_proj.visible) return;

		if (style.shape == SchematicObjectShape::Point) {
			draw_list->AddCircleFilled(center_proj.screen, static_cast<float>(style.point_pixel_radius), color, 20);
			return;
		}

		const double px_radius = (pixel_radius_override >= 0.0)
			? pixel_radius_override
			: std::clamp(compute_screen_radius(center_world, physical_radius), style.sphere_min_pixel_radius, style.sphere_max_pixel_radius);

		const bool can_render_true_wireframe = (style.shape == SchematicObjectShape::SphereFixedRadius) && (pixel_radius_override < 0.0);

		switch (style.sphere_style) {
			case SchematicSphereStyle::Opaque:
				draw_list->AddCircleFilled(center_proj.screen, static_cast<float>(px_radius), color, 40);
				draw_list->AddCircle(center_proj.screen, static_cast<float>(px_radius), IM_COL32(8, 10, 18, 210), 40, 1.2f);
				break;
			case SchematicSphereStyle::Translucent: {
				const ImVec4 col4 = ImGui::ColorConvertU32ToFloat4(color);
				const ImU32 faded = ImGui::ColorConvertFloat4ToU32(ImVec4(col4.x, col4.y, col4.z, static_cast<float>(style.translucency_alpha)));
				draw_list->AddCircleFilled(center_proj.screen, static_cast<float>(px_radius), faded, 40);
				draw_list->AddCircle(center_proj.screen, static_cast<float>(px_radius), color, 40, 1.3f);
				break;
			}
			case SchematicSphereStyle::Wireframe:
			default:
				if (can_render_true_wireframe) {
					draw_projected_sphere_wireframe(draw_list, center_world, physical_radius, style.wireframe_rings, style.wireframe_segments, style.wireframe_segments, color, color, 1.2f);
				} else {
					const ImVec4 col4 = ImGui::ColorConvertU32ToFloat4(color);
					const ImU32 faded = ImGui::ColorConvertFloat4ToU32(ImVec4(col4.x, col4.y, col4.z, 0.28f));
					draw_list->AddCircleFilled(center_proj.screen, static_cast<float>(px_radius), faded, 40);
					draw_list->AddCircle(center_proj.screen, static_cast<float>(px_radius), color, 40, 1.4f);
				}
				break;
		}
	}

	void draw_body_vector(
		ImDrawList* draw_list,
		const std::array<double, 3>& origin_world,
		const std::array<double, 3>& unit_direction_world,
		double magnitude,
		const SchematicVectorStyle& style,
		ImU32 color
	) const {
		if (magnitude <= 1e-15) return;

		const auto origin_proj = project(origin_world);
		if (!origin_proj.visible) return;

		const double dx = origin_world[0] - camera_position_[0];
		const double dy = origin_world[1] - camera_position_[1];
		const double dz = origin_world[2] - camera_position_[2];
		const double distance_to_camera = std::max(std::sqrt(dx * dx + dy * dy + dz * dz), 1e-6);
		const double probe_distance = distance_to_camera * 0.02;

		const std::array<double, 3> probe_world{
			origin_world[0] + unit_direction_world[0] * probe_distance,
			origin_world[1] + unit_direction_world[1] * probe_distance,
			origin_world[2] + unit_direction_world[2] * probe_distance
		};
		const auto probe_proj = project(probe_world);
		if (!probe_proj.visible) return;

		ImVec2 screen_dir{probe_proj.screen.x - origin_proj.screen.x, probe_proj.screen.y - origin_proj.screen.y};
		const float dir_len = std::sqrt(screen_dir.x * screen_dir.x + screen_dir.y * screen_dir.y);
		if (dir_len < 1e-5f) return;
		screen_dir.x /= dir_len;
		screen_dir.y /= dir_len;

		const double pixel_length = std::clamp(magnitude * style.length_scale, style.min_pixel_length, style.max_pixel_length);

		const ImVec2 end_point{
			origin_proj.screen.x + screen_dir.x * static_cast<float>(pixel_length),
			origin_proj.screen.y + screen_dir.y * static_cast<float>(pixel_length)
		};

		draw_list->AddLine(origin_proj.screen, end_point, color, static_cast<float>(style.line_thickness_px));

		const ImVec2 perp{-screen_dir.y, screen_dir.x};
		const float head = static_cast<float>(style.head_size_px);
		const ImVec2 base{end_point.x - screen_dir.x * head, end_point.y - screen_dir.y * head};
		draw_list->AddTriangleFilled(
			end_point,
			ImVec2(base.x + perp.x * head * 0.5f, base.y + perp.y * head * 0.5f),
			ImVec2(base.x - perp.x * head * 0.5f, base.y - perp.y * head * 0.5f),
			color
		);
	}

	void draw_all_body_vectors(ImDrawList* draw_list, const Dynamics::PostNewtonianBody& body, const SchematicViewConfig& cfg, ImU32 body_color) const {
		const double effective_radius = std::max(body.radius, 1e-4);

		const auto emit = [&](SchematicVectorKind kind, const std::array<double, 3>& raw_vec, double magnitude) {
			const auto& style = cfg.vector_style(kind);
			if (!style.enabled) return;

			std::array<double, 3> direction;
			if (style.orientation_mode == SchematicVectorOrientationMode::FixedWorldAxis) {
				direction = style.fixed_direction;
			} else {
				direction = raw_vec;
			}
			const double dir_len = std::sqrt(direction[0] * direction[0] + direction[1] * direction[1] + direction[2] * direction[2]);
			if (dir_len < 1e-12) return;
			direction = {direction[0] / dir_len, direction[1] / dir_len, direction[2] / dir_len};

			std::array<double, 3> origin = body.position;
			if (style.placement == SchematicVectorPlacement::AtSurface) {
				origin = {
					body.position[0] + direction[0] * effective_radius,
					body.position[1] + direction[1] * effective_radius,
					body.position[2] + direction[2] * effective_radius
				};
			}

			const ImU32 color = style.use_automatic_color
				? body_color
				: ImGui::ColorConvertFloat4ToU32(ImVec4(style.manual_color[0], style.manual_color[1], style.manual_color[2], style.manual_color[3]));

			draw_body_vector(draw_list, origin, direction, magnitude, style, color);
		};

		const double force_magnitude = body.mass * std::sqrt(
			body.acceleration[0] * body.acceleration[0] +
			body.acceleration[1] * body.acceleration[1] +
			body.acceleration[2] * body.acceleration[2]
		);

		emit(SchematicVectorKind::Velocity, body.velocity, body.speed());
		emit(SchematicVectorKind::TotalForce, body.acceleration, force_magnitude);
		emit(SchematicVectorKind::Spin, body.spin, body.spin_magnitude());
		emit(SchematicVectorKind::RotationAxis, body.spin, body.spin_magnitude());
	}

	[[nodiscard]] static std::string build_object_tag(const Dynamics::PostNewtonianBody& body, const SchematicObjectDisplayConfig& style) {
		std::string tag;
		if (style.show_id_in_tag) {
			tag += "#" + std::to_string(body.id);
		}
		if (style.show_mass_in_tag) {
			if (!tag.empty()) tag += " ";
			tag += "M=" + std::to_string(body.mass).substr(0, 6);
		}
		if (style.show_speed_in_tag) {
			if (!tag.empty()) tag += " ";
			tag += "v=" + std::to_string(body.speed()).substr(0, 6);
		}
		return tag;
	}

	void draw_body(ImDrawList* draw_list, const Dynamics::PostNewtonianBody& body, const SchematicViewConfig& cfg, double min_val, double max_val) const {
		const auto& style = cfg.body_style;
		const auto proj = project(body.position);

		if (!proj.visible) {
			if (cfg.show_tags) {
				draw_offscreen_indicator(draw_list, body.position, "#" + std::to_string(body.id));
			}
			return;
		}

		const double color_value = body_scalar_value(body, style.color_mode);
		const ImU32 color = compute_coded_color(style.color_mode, color_value, min_val, max_val, style.uniform_color);

		double pixel_radius_override = -1.0;
		if (style.shape == SchematicObjectShape::SphereByParameter) {
			const double param_value = body_parameter_source_value(body, style.parameter_source);
			pixel_radius_override = std::clamp(param_value * style.parameter_pixel_scale, style.sphere_min_pixel_radius, style.sphere_max_pixel_radius);
		}

		const double physical_radius = std::max(body.radius, 1e-4) * style.radius_scale;
		draw_object_shape(draw_list, body.position, physical_radius, style, color, pixel_radius_override);

		if (cfg.show_vectors) {
			draw_all_body_vectors(draw_list, body, cfg, color);
		}

		if (cfg.show_tags && style.show_tag) {
			const std::string tag = build_object_tag(body, style);
			if (!tag.empty()) {
				double px_radius_for_tag;
				if (style.shape == SchematicObjectShape::Point) {
					px_radius_for_tag = style.point_pixel_radius;
				} else if (pixel_radius_override >= 0.0) {
					px_radius_for_tag = pixel_radius_override;
				} else {
					px_radius_for_tag = compute_screen_radius(body.position, physical_radius);
				}
				draw_list->AddText(
					ImVec2(proj.screen.x + static_cast<float>(std::max(px_radius_for_tag, style.point_pixel_radius)) + 5.0f, proj.screen.y - 8.0f),
					IM_COL32(225, 232, 250, 235),
					tag.c_str()
				);
			}
		}
	}

	void draw_central_object(ImDrawList* draw_list, const Orchestrator::PhysicalParameters& params, double central_radius, const SchematicViewConfig& cfg) const {
		const std::array<double, 3> center{0.0, 0.0, 0.0};
		const auto proj = project(center);
		const auto& style = cfg.central_object_style;

		if (!proj.visible) {
			if (cfg.show_tags) {
				draw_offscreen_indicator(draw_list, center, "Central Object");
			}
			return;
		}

		const ImU32 color = ImGui::ColorConvertFloat4ToU32(ImVec4(style.uniform_color[0], style.uniform_color[1], style.uniform_color[2], style.uniform_color[3]));
		const double physical_radius = central_radius * style.radius_scale;
		draw_object_shape(draw_list, center, physical_radius, style, color, -1.0);

		if (cfg.show_vectors && std::abs(params.spin) > 1e-12) {
			const double effective_radius = std::max(physical_radius, 1e-4);
			const std::array<double, 3> reference_axis{0.0, 0.0, params.spin >= 0.0 ? 1.0 : -1.0};

			const auto emit_central = [&](SchematicVectorKind kind) {
				const auto& vstyle = cfg.vector_style(kind);
				if (!vstyle.enabled) return;

				std::array<double, 3> direction = (vstyle.orientation_mode == SchematicVectorOrientationMode::FixedWorldAxis)
					? vstyle.fixed_direction
					: reference_axis;
				const double dir_len = std::sqrt(direction[0] * direction[0] + direction[1] * direction[1] + direction[2] * direction[2]);
				if (dir_len < 1e-12) return;
				direction = {direction[0] / dir_len, direction[1] / dir_len, direction[2] / dir_len};

				std::array<double, 3> origin = center;
				if (vstyle.placement == SchematicVectorPlacement::AtSurface) {
					origin = {direction[0] * effective_radius, direction[1] * effective_radius, direction[2] * effective_radius};
				}

				const ImU32 vcolor = vstyle.use_automatic_color
					? color
					: ImGui::ColorConvertFloat4ToU32(ImVec4(vstyle.manual_color[0], vstyle.manual_color[1], vstyle.manual_color[2], vstyle.manual_color[3]));

				draw_body_vector(draw_list, origin, direction, std::abs(params.spin), vstyle, vcolor);
			};

			emit_central(SchematicVectorKind::Spin);
			emit_central(SchematicVectorKind::RotationAxis);
		}

		if (cfg.show_tags && style.show_tag) {
			std::string tag = "Central Object";
			if (style.show_mass_in_tag) {
				tag += " M=" + std::to_string(params.mass).substr(0, 6);
			}
			const double px_radius_for_tag = compute_screen_radius(center, physical_radius);
			draw_list->AddText(
				ImVec2(proj.screen.x + static_cast<float>(std::max(px_radius_for_tag, style.point_pixel_radius)) + 5.0f, proj.screen.y - 8.0f),
				IM_COL32(235, 225, 190, 240),
				tag.c_str()
			);
		}
	}

	void draw_field_lines(ImDrawList* draw_list, double central_radius, const SchematicViewConfig& cfg) const {
		const int count = std::max(cfg.field_line_count, 1);
		const double golden_angle = std::numbers::pi_v<double> * (3.0 - std::sqrt(5.0));
		const double inner = central_radius * 1.15;
		const double outer = central_radius * cfg.field_line_extent_scale;
		const ImU32 color = IM_COL32(90, 140, 200, clamp8(255.0 * cfg.field_line_opacity));

		for (int i = 0; i < count; ++i) {
			const double t = (static_cast<double>(i) + 0.5) / static_cast<double>(count);
			const double theta = std::acos(1.0 - 2.0 * t);
			const double phi = golden_angle * static_cast<double>(i);
			const double sin_t = std::sin(theta);
			const double cos_t = std::cos(theta);
			const double cos_p = std::cos(phi);
			const double sin_p = std::sin(phi);

			const std::array<double, 3> dir{sin_t * cos_p, sin_t * sin_p, cos_t};
			const std::array<double, 3> p0{dir[0] * inner, dir[1] * inner, dir[2] * inner};
			const std::array<double, 3> p1{dir[0] * outer, dir[1] * outer, dir[2] * outer};

			const auto proj0 = project(p0);
			const auto proj1 = project(p1);
			if (!proj0.visible || !proj1.visible) continue;

			draw_list->AddLine(proj0.screen, proj1.screen, color, 1.0f);

			if (cfg.field_line_inward_arrows) {
				const ImVec2 dir2d{proj0.screen.x - proj1.screen.x, proj0.screen.y - proj1.screen.y};
				const float len2d = std::sqrt(dir2d.x * dir2d.x + dir2d.y * dir2d.y);
				if (len2d > 1e-3f) {
					const ImVec2 n{dir2d.x / len2d, dir2d.y / len2d};
					const ImVec2 perp{-n.y, n.x};
					constexpr float head = 5.0f;
					const ImVec2 mid{(proj0.screen.x + proj1.screen.x) * 0.5f, (proj0.screen.y + proj1.screen.y) * 0.5f};
					const ImVec2 tip{mid.x + n.x * head, mid.y + n.y * head};
					const ImVec2 base{mid.x - n.x * head, mid.y - n.y * head};
					draw_list->AddTriangleFilled(
						tip,
						ImVec2(base.x + perp.x * head * 0.6f, base.y + perp.y * head * 0.6f),
						ImVec2(base.x - perp.x * head * 0.6f, base.y - perp.y * head * 0.6f),
						color
					);
				}
			}
		}
	}

	void update_trails(std::span<const Dynamics::PostNewtonianBody> bodies, const SchematicViewConfig& cfg) {
		const double now = ImGui::GetTime();
		for (const auto& body : bodies) {
			auto& trail = body_trails_[body.id];
			if (trail.empty() || (now - trail.back().timestamp) >= cfg.trail_sample_interval_seconds) {
				trail.push_back(TrailSample{body.position, now});
			}
			while (!trail.empty() && (now - trail.front().timestamp) > cfg.trail_duration_seconds) {
				trail.pop_front();
			}
			while (static_cast<int>(trail.size()) > cfg.trail_max_points) {
				trail.pop_front();
			}
		}

		std::erase_if(body_trails_, [&](const auto& entry) {
			return std::none_of(bodies.begin(), bodies.end(), [&](const auto& b) { return b.id == entry.first; });
		});
	}

	void draw_trails(ImDrawList* draw_list, const SchematicViewConfig& cfg) const {
		const double now = ImGui::GetTime();
		for (const auto& [id, trail] : body_trails_) {
			static_cast<void>(id);
			if (trail.size() < 2) continue;
			for (size_t i = 1; i < trail.size(); ++i) {
				const auto& a = trail[i - 1];
				const auto& b = trail[i];
				const auto proj_a = project(a.position);
				const auto proj_b = project(b.position);
				if (!proj_a.visible || !proj_b.visible) continue;
				const double age = now - b.timestamp;
				const double t = std::clamp(1.0 - age / std::max(cfg.trail_duration_seconds, 1e-6), 0.0, 1.0);
				const float alpha = static_cast<float>(std::pow(t, cfg.trail_fade_power));
				const ImU32 color = IM_COL32(140, 190, 255, clamp8(static_cast<double>(alpha) * 220.0));
				draw_list->AddLine(proj_a.screen, proj_b.screen, color, static_cast<float>(cfg.trail_line_thickness));
			}
		}
	}

	[[nodiscard]] static std::vector<std::array<double, 3>> compute_orbit_ellipse_points(
		const std::array<double, 3>& r_vec,
		const std::array<double, 3>& v_vec,
		double mu,
		int segments,
		double max_eccentricity
	) {
		std::vector<std::array<double, 3>> pts;
		const double r = std::sqrt(r_vec[0] * r_vec[0] + r_vec[1] * r_vec[1] + r_vec[2] * r_vec[2]);
		if (r < 1e-9 || mu <= 0.0) return pts;

		const std::array<double, 3> h_vec{
			r_vec[1] * v_vec[2] - r_vec[2] * v_vec[1],
			r_vec[2] * v_vec[0] - r_vec[0] * v_vec[2],
			r_vec[0] * v_vec[1] - r_vec[1] * v_vec[0]
		};
		const double h_mag = std::sqrt(h_vec[0] * h_vec[0] + h_vec[1] * h_vec[1] + h_vec[2] * h_vec[2]);
		if (h_mag < 1e-12) return pts;

		const double v2 = v_vec[0] * v_vec[0] + v_vec[1] * v_vec[1] + v_vec[2] * v_vec[2];
		const double inv_a = (2.0 / r - v2 / mu);
		if (inv_a <= 1e-12) return pts;
		const double a = 1.0 / inv_a;

		std::array<double, 3> e_vec{
			(v_vec[1] * h_vec[2] - v_vec[2] * h_vec[1]) / mu - r_vec[0] / r,
			(v_vec[2] * h_vec[0] - v_vec[0] * h_vec[2]) / mu - r_vec[1] / r,
			(v_vec[0] * h_vec[1] - v_vec[1] * h_vec[0]) / mu - r_vec[2] / r
		};
		const double ecc = std::sqrt(e_vec[0] * e_vec[0] + e_vec[1] * e_vec[1] + e_vec[2] * e_vec[2]);
		if (ecc >= max_eccentricity) return pts;

		const std::array<double, 3> periapsis_dir = (ecc > 1e-8)
			? std::array<double, 3>{e_vec[0] / ecc, e_vec[1] / ecc, e_vec[2] / ecc}
			: std::array<double, 3>{r_vec[0] / r, r_vec[1] / r, r_vec[2] / r};

		const std::array<double, 3> normal{h_vec[0] / h_mag, h_vec[1] / h_mag, h_vec[2] / h_mag};
		const std::array<double, 3> in_plane_perp{
			normal[1] * periapsis_dir[2] - normal[2] * periapsis_dir[1],
			normal[2] * periapsis_dir[0] - normal[0] * periapsis_dir[2],
			normal[0] * periapsis_dir[1] - normal[1] * periapsis_dir[0]
		};

		const double p = a * (1.0 - ecc * ecc);
		const int seg_count = std::max(segments, 8);
		pts.reserve(static_cast<size_t>(seg_count) + 1);
		for (int i = 0; i <= seg_count; ++i) {
			const double nu = (2.0 * std::numbers::pi_v<double> * static_cast<double>(i)) / static_cast<double>(seg_count);
			const double denom = 1.0 + ecc * std::cos(nu);
			if (denom <= 1e-9) continue;
			const double radius_at_nu = p / denom;
			const double cos_nu = std::cos(nu);
			const double sin_nu = std::sin(nu);
			pts.push_back({
				periapsis_dir[0] * radius_at_nu * cos_nu + in_plane_perp[0] * radius_at_nu * sin_nu,
				periapsis_dir[1] * radius_at_nu * cos_nu + in_plane_perp[1] * radius_at_nu * sin_nu,
				periapsis_dir[2] * radius_at_nu * cos_nu + in_plane_perp[2] * radius_at_nu * sin_nu
			});
		}
		return pts;
	}

public:
	void configure(
		const Orchestrator::CameraState& cam,
		Observer::ProjectionMode projection_mode,
		double fov_rad,
		const ImVec2& rect_min,
		const ImVec2& rect_size
	) noexcept {
		camera_position_ = cam.position;
		projection_mode_ = projection_mode;
		rect_min_ = rect_min;
		rect_size_ = ImVec2(std::max(rect_size.x, 0.0f), std::max(rect_size.y, 0.0f));
		aspect_ = (rect_size_.y > 0.0f) ? static_cast<double>(rect_size_.x) / static_cast<double>(rect_size_.y) : 1.0;

		constexpr double min_fov = 1e-6;
		constexpr double max_fov = std::numbers::pi_v<double> - 1e-6;
		fov_rad_ = std::clamp(fov_rad, min_fov, max_fov);

		const double pitch_r = cam.pitch * (std::numbers::pi / 180.0);
		const double yaw_r = cam.yaw * (std::numbers::pi / 180.0);
		const double roll_r = cam.roll * (std::numbers::pi / 180.0);
		const double cp = std::cos(pitch_r), sp = std::sin(pitch_r);
		const double cy = std::cos(yaw_r), sy = std::sin(yaw_r);
		const double cr = std::cos(roll_r), sr = std::sin(roll_r);

		tetrad_forward_ = {cp * cy, cp * sy, sp};
		tetrad_right_ = {cr * (-sy) + sr * (-sp * cy), cr * cy + sr * (-sp * sy), sr * cp};
		tetrad_up_ = {-sr * (-sy) + cr * (-sp * cy), -sr * cy + cr * (-sp * sy), cr * cp};
	}

	void render(ImDrawList* draw_list, const Orchestrator::SimulationOrchestrator<1024>& orchestrator, const SchematicViewConfig& cfg) {
		draw_list->AddRectFilled(rect_min_, ImVec2(rect_min_.x + rect_size_.x, rect_min_.y + rect_size_.y), IM_COL32(5, 6, 10, 255));

		const auto& params = orchestrator.parameters();
		const auto& sys = orchestrator.nbody_system();
		const auto bodies = sys.bodies();
		const double mu = std::max(params.mass, 1e-6);
		const double central_radius = std::max(2.0 * params.mass, 1e-3);

		if (cfg.show_background_grid) {
			draw_projected_sphere_wireframe(
				draw_list, camera_position_, std::max(central_radius * cfg.grid_radius_scale, 1.0),
				cfg.grid_latitude_lines, cfg.grid_longitude_lines, cfg.grid_segments,
				IM_COL32(60, 92, 124, clamp8(220.0 * cfg.grid_opacity)),
				IM_COL32(32, 52, 72, clamp8(170.0 * cfg.grid_opacity)),
				1.0f
			);
		}

		if (cfg.show_field_lines) {
			draw_field_lines(draw_list, central_radius, cfg);
		}

		update_trails(bodies, cfg);
		if (cfg.show_trails) {
			draw_trails(draw_list, cfg);
		}

		if (cfg.show_orbit_predictions) {
			for (const auto& body : bodies) {
				const auto pts = compute_orbit_ellipse_points(body.position, body.velocity, mu, cfg.orbit_prediction_segments, cfg.orbit_prediction_max_eccentricity);
				draw_polyline_3d(draw_list, pts, IM_COL32(170, 200, 255, clamp8(255.0 * cfg.orbit_prediction_opacity)), static_cast<float>(cfg.orbit_prediction_thickness), true);
			}
		}

		double min_val = std::numeric_limits<double>::max();
		double max_val = std::numeric_limits<double>::lowest();
		if (cfg.body_style.color_mode != SchematicColorCodingMode::Uniform) {
			for (const auto& body : bodies) {
				const double v = body_scalar_value(body, cfg.body_style.color_mode);
				min_val = std::min(min_val, v);
				max_val = std::max(max_val, v);
			}
		}

		if (cfg.show_central_object) {
			draw_central_object(draw_list, params, central_radius, cfg);
		}

		if (cfg.show_bodies) {
			for (const auto& body : bodies) {
				draw_body(draw_list, body, cfg, min_val, max_val);
			}
		}
	}
};

}

#pragma once

#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/dynamics/pn_body.hpp"
#include "relativistic/observer/camera_projections.hpp"
#include "relativistic/observer/direction_projection.hpp"
#include "relativistic/ui/schematic_view_config.hpp"
#include "relativistic/optics/disk_thermal_profile.hpp"
#include "relativistic/capture/path_preview.hpp"
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
	double lensing_mass_{0.0};
	bool apply_lensing_{false};
	std::unordered_map<uint32_t, std::deque<TrailSample>> body_trails_{};
	Capture::PathPreview path_preview_{};

	[[nodiscard]] static int clamp8(double value) noexcept {
		return static_cast<int>(std::clamp(value, 0.0, 255.0));
	}

	[[nodiscard]] std::optional<std::pair<double, double>> direction_to_screen_uv(double fwd, double right, double up) const noexcept {
		return Observer::direction_to_screen_uv(projection_mode_, fwd, right, up, fov_rad_);
	}

	[[nodiscard]] ProjectedPoint project(const std::array<double, 3>& world_pos) const noexcept {
		const double dx = world_pos[0] - camera_position_[0];
		const double dy = world_pos[1] - camera_position_[1];
		const double dz = world_pos[2] - camera_position_[2];

		double fwd = dx * tetrad_forward_[0] + dy * tetrad_forward_[1] + dz * tetrad_forward_[2];
		double right = dx * tetrad_right_[0] + dy * tetrad_right_[1] + dz * tetrad_right_[2];
		double up = dx * tetrad_up_[0] + dy * tetrad_up_[1] + dz * tetrad_up_[2];

		if (apply_lensing_ && lensing_mass_ > 0.0) {
			const double camera_r = std::sqrt(camera_position_[0] * camera_position_[0] + camera_position_[1] * camera_position_[1] + camera_position_[2] * camera_position_[2]);
			const double ray_r = std::sqrt(dx * dx + dy * dy + dz * dz);
			if (camera_r > 1e-6 && ray_r > 1e-6) {
				const double lens_fwd = (-camera_position_[0] * tetrad_forward_[0] - camera_position_[1] * tetrad_forward_[1] - camera_position_[2] * tetrad_forward_[2]) / camera_r;
				const double lens_right = (-camera_position_[0] * tetrad_right_[0] - camera_position_[1] * tetrad_right_[1] - camera_position_[2] * tetrad_right_[2]) / camera_r;
				const double lens_up = (-camera_position_[0] * tetrad_up_[0] - camera_position_[1] * tetrad_up_[1] - camera_position_[2] * tetrad_up_[2]) / camera_r;
				const double ray_dot_lens = std::clamp((fwd * lens_fwd + right * lens_right + up * lens_up) / ray_r, -1.0, 1.0);
				const double impact = std::max(camera_r * std::sqrt(std::max(1.0 - ray_dot_lens * ray_dot_lens, 0.0)), 2.05 * lensing_mass_);
				const double deflection = std::clamp(4.0 * lensing_mass_ / impact, 0.0, 0.35);
				fwd += ray_r * deflection * lens_fwd;
				right += ray_r * deflection * lens_right;
				up += ray_r * deflection * lens_up;
			}
		}

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

	[[nodiscard]] bool is_occluded_by_sphere(const std::array<double, 3>& target, const std::array<double, 3>& sphere_center, double sphere_radius) const noexcept {
		const double dx = target[0] - camera_position_[0];
		const double dy = target[1] - camera_position_[1];
		const double dz = target[2] - camera_position_[2];
		const double ox = camera_position_[0] - sphere_center[0];
		const double oy = camera_position_[1] - sphere_center[1];
		const double oz = camera_position_[2] - sphere_center[2];
		const double a_coeff = dx * dx + dy * dy + dz * dz;
		if (a_coeff <= 1e-18) return false;
		const double b_coeff = 2.0 * (ox * dx + oy * dy + oz * dz);
		const double c_coeff = ox * ox + oy * oy + oz * oz - sphere_radius * sphere_radius;
		const double discriminant = b_coeff * b_coeff - 4.0 * a_coeff * c_coeff;
		if (discriminant < 0.0) return false;
		const double entry = (-b_coeff - std::sqrt(discriminant)) / (2.0 * a_coeff);
		return entry >= 0.0 && entry < 1.0;
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

	void draw_offscreen_indicator(
		ImDrawList* draw_list,
		const std::array<double, 3>& world_position,
		const std::string& label,
		const OffscreenIndicatorConfig& style = OffscreenIndicatorConfig{},
		ImU32 color_override = 0,
		double size_override = -1.0
	) const {
		if (rect_size_.x <= 0.0f || rect_size_.y <= 0.0f || !style.enabled) return;

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

		const float margin = static_cast<float>(style.edge_margin_px);
		const float half_w = std::max(rect_size_.x * 0.5f - margin, 1.0f);
		const float half_h = std::max(rect_size_.y * 0.5f - margin, 1.0f);

		float scale_x = std::numeric_limits<float>::max();
		float scale_y = std::numeric_limits<float>::max();
		if (std::abs(dir.x) > 1e-6f) scale_x = half_w / std::abs(dir.x);
		if (std::abs(dir.y) > 1e-6f) scale_y = half_h / std::abs(dir.y);
		const float scale = std::min(scale_x, scale_y);
		if (!std::isfinite(scale)) return;

		const double distance = std::sqrt(dx * dx + dy * dy + dz * dz);
		const float arrow_size = static_cast<float>(size_override >= 0.0
			? size_override
			: (style.scale_with_distance
				? std::clamp(style.base_size_px * (60.0 / std::max(distance, 1.0)), style.min_size_px, style.max_size_px)
				: style.base_size_px));

		ImU32 indicator_color = (color_override != 0) ? color_override : ImGui::ColorConvertFloat4ToU32(ImVec4(style.fixed_color[0], style.fixed_color[1], style.fixed_color[2], style.fixed_color[3]));
		if (style.fade_with_distance) {
			const float fade = static_cast<float>(std::clamp(style.fade_reference_distance / std::max(distance, 1.0), 0.15, 1.0));
			const ImVec4 base = ImGui::ColorConvertU32ToFloat4(indicator_color);
			indicator_color = ImGui::ColorConvertFloat4ToU32(ImVec4(base.x, base.y, base.z, base.w * fade));
		}

		const ImVec2 edge_point(center.x + dir.x * scale, center.y + dir.y * scale);
		const ImVec2 perp{-dir.y, dir.x};

		switch (style.shape) {
			case OffscreenIndicatorShape::Diamond: {
				const ImVec2 top(edge_point.x + dir.x * arrow_size, edge_point.y + dir.y * arrow_size);
				const ImVec2 bottom(edge_point.x - dir.x * arrow_size, edge_point.y - dir.y * arrow_size);
				const ImVec2 left(edge_point.x + perp.x * arrow_size, edge_point.y + perp.y * arrow_size);
				const ImVec2 right(edge_point.x - perp.x * arrow_size, edge_point.y - perp.y * arrow_size);
				draw_list->AddQuadFilled(top, left, bottom, right, indicator_color);
				break;
			}
			case OffscreenIndicatorShape::Dot: {
				draw_list->AddCircleFilled(edge_point, arrow_size * 0.6f, indicator_color, 16);
				break;
			}
			case OffscreenIndicatorShape::Chevron: {
				const ImVec2 tip(edge_point.x + dir.x * arrow_size, edge_point.y + dir.y * arrow_size);
				const ImVec2 base_a(edge_point.x - dir.x * arrow_size * 0.2f + perp.x * arrow_size * 0.7f, edge_point.y - dir.y * arrow_size * 0.2f + perp.y * arrow_size * 0.7f);
				const ImVec2 base_b(edge_point.x - dir.x * arrow_size * 0.2f - perp.x * arrow_size * 0.7f, edge_point.y - dir.y * arrow_size * 0.2f - perp.y * arrow_size * 0.7f);
				draw_list->AddLine(base_a, tip, indicator_color, 2.5f);
				draw_list->AddLine(base_b, tip, indicator_color, 2.5f);
				break;
			}
			case OffscreenIndicatorShape::Triangle:
			default: {
				const ImVec2 tip(edge_point.x + dir.x * arrow_size, edge_point.y + dir.y * arrow_size);
				const ImVec2 base_a(edge_point.x - dir.x * arrow_size + perp.x * arrow_size * 0.6f, edge_point.y - dir.y * arrow_size + perp.y * arrow_size * 0.6f);
				const ImVec2 base_b(edge_point.x - dir.x * arrow_size - perp.x * arrow_size * 0.6f, edge_point.y - dir.y * arrow_size - perp.y * arrow_size * 0.6f);
				draw_list->AddTriangleFilled(tip, base_a, base_b, indicator_color);
				break;
			}
		}

		if (style.show_label) {
			std::string full_label = label;
			if (style.show_distance_in_label) {
				full_label += " (" + std::to_string(distance).substr(0, 6) + ")";
			}
			const ImVec2 text_position(edge_point.x + dir.x * 14.0f, edge_point.y + dir.y * 14.0f - 6.0f);
			draw_list->AddText(text_position, indicator_color, full_label.c_str());
		}
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
			case SchematicColorCodingMode::ByTemperature:
				return body.temperature;
			case SchematicColorCodingMode::ByChargeMagnitude:
				return std::abs(body.charge);
			case SchematicColorCodingMode::ByDensity: {
				const double volume = (4.0 / 3.0) * std::numbers::pi_v<double> * std::pow(std::max(body.radius, 1e-12), 3.0);
				return body.mass / volume;
			}
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

	struct ProjectedEllipse {
		bool visible{false};
		ImVec2 center{0.0f, 0.0f};
		float rx{0.0f};
		float ry{0.0f};
	};

	[[nodiscard]] ProjectedEllipse compute_screen_ellipse(const std::array<double, 3>& world_center, double physical_radius) const noexcept {
		ProjectedEllipse res;
		if (physical_radius <= 0.0) return res;
		const auto center_proj = project(world_center);
		if (!center_proj.visible) return res;

		res.center = center_proj.screen;
		res.visible = true;

		const std::array<double, 3> offset_right{
			world_center[0] + tetrad_right_[0] * physical_radius,
			world_center[1] + tetrad_right_[1] * physical_radius,
			world_center[2] + tetrad_right_[2] * physical_radius
		};
		const auto edge_right = project(offset_right);

		const std::array<double, 3> offset_up{
			world_center[0] + tetrad_up_[0] * physical_radius,
			world_center[1] + tetrad_up_[1] * physical_radius,
			world_center[2] + tetrad_up_[2] * physical_radius
		};
		const auto edge_up = project(offset_up);

		if (edge_right.visible) {
			const float dx = edge_right.screen.x - center_proj.screen.x;
			const float dy = edge_right.screen.y - center_proj.screen.y;
			res.rx = std::sqrt(dx * dx + dy * dy);
		} else {
			res.rx = 0.0f;
		}

		if (edge_up.visible) {
			const float dx = edge_up.screen.x - center_proj.screen.x;
			const float dy = edge_up.screen.y - center_proj.screen.y;
			res.ry = std::sqrt(dx * dx + dy * dy);
		} else {
			res.ry = res.rx;
		}

		if (res.rx <= 0.0f && res.ry <= 0.0f) res.visible = false;
		if (res.rx <= 0.0f) res.rx = res.ry;
		if (res.ry <= 0.0f) res.ry = res.rx;
		return res;
	}

	[[nodiscard]] static std::array<float, 3> blackbody_to_rgb(double temp_k) noexcept {
		struct Stop { double temp; std::array<float, 3> rgb; };
		static constexpr std::array<Stop, 8> table{{
			{ 800.0,   {1.00f, 0.22f, 0.00f} },
			{ 1500.0,  {1.00f, 0.45f, 0.10f} },
			{ 3000.0,  {1.00f, 0.70f, 0.40f} },
			{ 5778.0,  {1.00f, 0.97f, 0.90f} },
			{ 8000.0,  {0.85f, 0.90f, 1.00f} },
			{ 15000.0, {0.65f, 0.80f, 1.00f} },
			{ 30000.0, {0.40f, 0.60f, 1.00f} },
			{ 40000.0, {0.30f, 0.45f, 1.00f} }
		}};

		const double T = std::clamp(temp_k, 800.0, 40000.0);
		if (T <= table.front().temp) return table.front().rgb;
		if (T >= table.back().temp) return table.back().rgb;

		for (size_t i = 0; i < table.size() - 1; ++i) {
			if (T >= table[i].temp && T <= table[i + 1].temp) {
				const float frac = static_cast<float>((T - table[i].temp) / (table[i + 1].temp - table[i].temp));
				return {
					table[i].rgb[0] + frac * (table[i + 1].rgb[0] - table[i].rgb[0]),
					table[i].rgb[1] + frac * (table[i + 1].rgb[1] - table[i].rgb[1]),
					table[i].rgb[2] + frac * (table[i + 1].rgb[2] - table[i].rgb[2])
				};
			}
		}
		return table.back().rgb;
	}

	[[nodiscard]] static ImU32 compute_intelligent_color(const Dynamics::PostNewtonianBody& body) noexcept {
		const double temp = (body.temperature > 0.0) ? body.temperature : 5778.0;
		auto bb_rgb = blackbody_to_rgb(temp);

		std::array<float, 3> comp_shift{0.0f, 0.0f, 0.0f};
		float brightness_boost = 0.0f;
		const char comp_char = static_cast<char>(std::toupper(static_cast<unsigned char>(body.composition[0])));
		switch (comp_char) {
			case 'H': comp_shift = { -0.02f,  0.02f,  0.06f }; break;
			case 'C': comp_shift = {  0.12f, -0.05f, -0.05f }; break;
			case 'R': comp_shift = {  0.12f,  0.07f, -0.05f }; break;
			case 'M': comp_shift = {  0.00f,  0.00f,  0.00f }; brightness_boost = 0.12f; break;
			case 'G': comp_shift = {  0.08f,  0.05f, -0.02f }; break;
			case 'N': comp_shift = { -0.05f,  0.05f,  0.20f }; break;
			default: break;
		}

		const double charge_n = std::clamp(body.charge / 5.0, -1.0, 1.0);
		float charge_r = 0.0f, charge_g = 0.0f, charge_b = 0.0f;
		if (charge_n > 0.0) {
			charge_r = static_cast<float>(charge_n * 0.15);
			charge_g = static_cast<float>(charge_n * 0.10);
		} else if (charge_n < 0.0) {
			charge_r = static_cast<float>(-charge_n * 0.08);
			charge_b = static_cast<float>(-charge_n * 0.15);
		}

		const double spin_n = std::clamp(body.spin_magnitude() / std::max(body.mass, 1e-9), 0.0, 1.0);
		const float spin_blue_add = static_cast<float>(spin_n * 0.14);

		const double vol = (4.0 / 3.0) * std::numbers::pi_v<double> * std::pow(std::max(body.radius, 1e-6), 3.0);
		const double density = body.mass / std::max(vol, 1e-12);
		const double density_n = std::clamp(std::log10(density + 1.0) / 8.0, 0.0, 1.0);
		const float brightness_mult = static_cast<float>(0.65 + 0.35 * density_n) + brightness_boost;

		const double compactness = std::clamp(2.0 * body.mass / std::max(body.radius, 1e-6), 0.0, 0.95);
		const float redshift_r_add = static_cast<float>(compactness * 0.25);
		const float redshift_b_sub = static_cast<float>(compactness * 0.15);

		float r = std::clamp((bb_rgb[0] + comp_shift[0] + charge_r + redshift_r_add) * brightness_mult, 0.0f, 1.0f);
		float g = std::clamp((bb_rgb[1] + comp_shift[1] + charge_g) * brightness_mult, 0.0f, 1.0f);
		float b = std::clamp((bb_rgb[2] + comp_shift[2] + charge_b + spin_blue_add - redshift_b_sub) * brightness_mult, 0.0f, 1.0f);

		return IM_COL32(clamp8(r * 255.0f), clamp8(g * 255.0f), clamp8(b * 255.0f), 255);
	}

	[[nodiscard]] static ImU32 compute_coded_color(SchematicColorCodingMode mode, double value, double min_v, double max_v, const std::array<float, 4>& fallback_color) noexcept {
		if (mode == SchematicColorCodingMode::Uniform || max_v <= min_v) {
			return ImGui::ColorConvertFloat4ToU32(ImVec4(fallback_color[0], fallback_color[1], fallback_color[2], fallback_color[3]));
		}
		const double t = std::clamp((value - min_v) / (max_v - min_v), 0.0, 1.0);
		const auto rgb = schematic_heatmap_gradient(t);
		return ImGui::ColorConvertFloat4ToU32(ImVec4(rgb[0], rgb[1], rgb[2], fallback_color[3]));
	}

	void draw_body_halo(
		ImDrawList* draw_list,
		ImVec2 center,
		float px_rx,
		float px_ry,
		const std::array<float, 4>& halo_color,
		float halo_strength,
		float halo_radius_factor
	) const {
		if (halo_strength <= 0.0f || px_rx <= 0.0f) return;
		const float max_halo_r = std::max(px_rx, px_ry) * halo_radius_factor;
		constexpr int passes = 6;
		for (int i = passes; i >= 1; --i) {
			const float t = static_cast<float>(i) / static_cast<float>(passes);
			const float cur_r_x = px_rx + (max_halo_r - px_rx) * t;
			const float cur_r_y = px_ry + (max_halo_r - px_ry) * t;
			const float alpha = halo_color[3] * halo_strength * (1.0f - t * 0.85f) * 0.35f;
			const ImU32 col = ImGui::ColorConvertFloat4ToU32(ImVec4(halo_color[0], halo_color[1], halo_color[2], alpha));
			if (std::abs(cur_r_x - cur_r_y) < 1.0f) {
				draw_list->AddCircleFilled(center, cur_r_x, col, 36);
			} else {
				draw_list->AddEllipseFilled(center, ImVec2(cur_r_x, cur_r_y), col, 0.0f, 36);
			}
		}
	}

	void draw_body_outline(
		ImDrawList* draw_list,
		ImVec2 center,
		float px_rx,
		float px_ry,
		const SchematicBodyOutlineStyle& outline
	) const {
		if (!outline.enabled || px_rx <= 0.0f) return;

		if (outline.glow_enabled && outline.glow_radius > 0.0f) {
			constexpr int glow_passes = 5;
			for (int i = glow_passes; i >= 1; --i) {
				const float t = static_cast<float>(i) / static_cast<float>(glow_passes);
				const float glow_r_x = px_rx + outline.glow_radius * t;
				const float glow_r_y = px_ry + outline.glow_radius * t;
				const float alpha = outline.glow_color[3] * outline.glow_alpha * (1.0f - t * 0.7f) * 0.4f;
				const ImU32 glow_col = ImGui::ColorConvertFloat4ToU32(ImVec4(outline.glow_color[0], outline.glow_color[1], outline.glow_color[2], alpha));
				if (std::abs(glow_r_x - glow_r_y) < 1.0f) {
					draw_list->AddCircle(center, glow_r_x, glow_col, 40, outline.thickness + outline.glow_radius * t * 0.5f);
				} else {
					draw_list->AddEllipse(center, ImVec2(glow_r_x, glow_r_y), glow_col, 0.0f, 40, outline.thickness + outline.glow_radius * t * 0.5f);
				}
			}
		}

		const ImU32 outline_col = ImGui::ColorConvertFloat4ToU32(ImVec4(outline.color[0], outline.color[1], outline.color[2], outline.color[3]));
		if (std::abs(px_rx - px_ry) < 1.0f) {
			draw_list->AddCircle(center, px_rx, outline_col, 48, outline.thickness);
		} else {
			draw_list->AddEllipse(center, ImVec2(px_rx, px_ry), outline_col, 0.0f, 48, outline.thickness);
		}
	}

	void draw_shaded_sphere(
		ImDrawList* draw_list,
		ImVec2 center,
		float px_rx,
		float px_ry,
		ImU32 primary_color,
		const std::array<float, 4>& secondary_color,
		const SchematicBodyShadingConfig& shading
	) const {
		const ImVec4 base_col = ImGui::ColorConvertU32ToFloat4(primary_color);

		float shadow_r = base_col.x * shading.ambient_strength;
		float shadow_g = base_col.y * shading.ambient_strength;
		float shadow_b = base_col.z * shading.ambient_strength;
		if (shading.use_secondary_color_as_shadow) {
			shadow_r = (shadow_r + secondary_color[0] * shading.ambient_strength) * 0.5f;
			shadow_g = (shadow_g + secondary_color[1] * shading.ambient_strength) * 0.5f;
			shadow_b = (shadow_b + secondary_color[2] * shading.ambient_strength) * 0.5f;
		}
		const ImU32 shadow_col = ImGui::ColorConvertFloat4ToU32(ImVec4(shadow_r, shadow_g, shadow_b, base_col.w));

		if (std::abs(px_rx - px_ry) < 1.0f) {
			draw_list->AddCircleFilled(center, px_rx, shadow_col, 48);
		} else {
			draw_list->AddEllipseFilled(center, ImVec2(px_rx, px_ry), shadow_col, 0.0f, 48);
		}

		const float lx = shading.light_direction[0];
		const float ly = shading.light_direction[1];
		const float l_len = std::sqrt(lx * lx + ly * ly);
		ImVec2 light_offset{0.0f, 0.0f};
		if (l_len > 1e-4f) {
			light_offset.x = (lx / l_len) * px_rx * 0.25f;
			light_offset.y = (-ly / l_len) * px_ry * 0.25f;
		}

		const float diff_r = std::clamp(base_col.x * shading.diffuse_strength, 0.0f, 1.0f);
		const float diff_g = std::clamp(base_col.y * shading.diffuse_strength, 0.0f, 1.0f);
		const float diff_b = std::clamp(base_col.z * shading.diffuse_strength, 0.0f, 1.0f);
		const ImU32 diff_col = ImGui::ColorConvertFloat4ToU32(ImVec4(diff_r, diff_g, diff_b, base_col.w));

		const ImVec2 lit_center(center.x + light_offset.x, center.y + light_offset.y);
		if (std::abs(px_rx - px_ry) < 1.0f) {
			draw_list->AddCircleFilled(lit_center, px_rx * 0.82f, diff_col, 44);
		} else {
			draw_list->AddEllipseFilled(lit_center, ImVec2(px_rx * 0.82f, px_ry * 0.82f), diff_col, 0.0f, 44);
		}

		const ImVec2 highlight_center(center.x + light_offset.x * 1.4f, center.y + light_offset.y * 1.4f);
		if (std::abs(px_rx - px_ry) < 1.0f) {
			draw_list->AddCircleFilled(highlight_center, px_rx * 0.52f, primary_color, 40);
		} else {
			draw_list->AddEllipseFilled(highlight_center, ImVec2(px_rx * 0.52f, px_ry * 0.52f), primary_color, 0.0f, 40);
		}

		if (shading.specular_strength > 0.001f) {
			const ImVec2 spec_center(center.x + light_offset.x * 1.8f, center.y + light_offset.y * 1.8f);
			const float spec_alpha = std::clamp(shading.specular_strength, 0.0f, 1.0f) * base_col.w;
			const ImU32 spec_col = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, spec_alpha));
			const float spec_r_x = std::max(px_rx * (0.22f - std::min(shading.specular_shininess * 0.004f, 0.15f)), 1.0f);
			const float spec_r_y = std::max(px_ry * (0.22f - std::min(shading.specular_shininess * 0.004f, 0.15f)), 1.0f);
			if (std::abs(spec_r_x - spec_r_y) < 1.0f) {
				draw_list->AddCircleFilled(spec_center, spec_r_x, spec_col, 24);
			} else {
				draw_list->AddEllipseFilled(spec_center, ImVec2(spec_r_x, spec_r_y), spec_col, 0.0f, 24);
			}
		}

		if (shading.limb_darkening_power > 0.01f) {
			const float limb_alpha = std::clamp(shading.limb_darkening_power * 0.65f, 0.0f, 0.95f);
			const ImU32 limb_col = IM_COL32(8, 10, 18, clamp8(255.0f * limb_alpha));
			if (std::abs(px_rx - px_ry) < 1.0f) {
				draw_list->AddCircle(center, px_rx, limb_col, 48, std::max(px_rx * 0.15f, 1.2f));
			} else {
				draw_list->AddEllipse(center, ImVec2(px_rx, px_ry), limb_col, 0.0f, 48, std::max(px_rx * 0.15f, 1.2f));
			}
		}
	}

	void draw_gradient_sphere(
		ImDrawList* draw_list,
		ImVec2 center,
		float px_rx,
		float px_ry,
		const std::vector<SchematicGradientStop>& stops,
		bool radial,
		float angle_deg
	) const {
		if (stops.empty() || px_rx <= 0.0f) return;

		if (radial) {
			for (int i = static_cast<int>(stops.size()) - 1; i >= 0; --i) {
				const auto& stop = stops[static_cast<size_t>(i)];
				const float factor = std::clamp(stop.position, 0.0f, 1.0f);
				const float cur_rx = std::max(px_rx * (1.0f - factor * 0.85f), 1.0f);
				const float cur_ry = std::max(px_ry * (1.0f - factor * 0.85f), 1.0f);
				const ImU32 col = ImGui::ColorConvertFloat4ToU32(ImVec4(stop.color[0], stop.color[1], stop.color[2], stop.color[3]));
				if (std::abs(cur_rx - cur_ry) < 1.0f) {
					draw_list->AddCircleFilled(center, cur_rx, col, 40);
				} else {
					draw_list->AddEllipseFilled(center, ImVec2(cur_rx, cur_ry), col, 0.0f, 40);
				}
			}
		} else {
			const float rad = angle_deg * (std::numbers::pi_v<float> / 180.0f);
			const ImVec2 dir{std::cos(rad), std::sin(rad)};
			const size_t stop_count = stops.size();
			for (size_t i = 0; i < stop_count; ++i) {
				const auto& stop = stops[i];
				const float pos_offset = (stop.position - 0.5f) * px_rx * 0.8f;
				const ImVec2 pos_center(center.x + dir.x * pos_offset, center.y + dir.y * pos_offset);
				const float scale = 1.0f - static_cast<float>(i) * (0.6f / static_cast<float>(std::max(stop_count, size_t(1))));
				const ImU32 col = ImGui::ColorConvertFloat4ToU32(ImVec4(stop.color[0], stop.color[1], stop.color[2], stop.color[3]));
				if (std::abs(px_rx - px_ry) < 1.0f) {
					draw_list->AddCircleFilled(pos_center, px_rx * scale, col, 36);
				} else {
					draw_list->AddEllipseFilled(pos_center, ImVec2(px_rx * scale, px_ry * scale), col, 0.0f, 36);
				}
			}
		}
	}

	void draw_object_shape(
		ImDrawList* draw_list,
		const std::array<double, 3>& center_world,
		double physical_radius,
		const SchematicObjectDisplayConfig& style,
		ImU32 color,
		double pixel_radius_override,
		const std::array<float, 4>& secondary_color = {0.18f, 0.30f, 0.75f, 1.0f}
	) const {
		const auto center_proj = project(center_world);
		if (!center_proj.visible) return;

		if (style.shape == SchematicObjectShape::Point) {
			draw_list->AddCircleFilled(center_proj.screen, static_cast<float>(style.point_pixel_radius), color, 20);
			return;
		}

		const auto ellipse = compute_screen_ellipse(center_world, physical_radius);
		float px_rx = (pixel_radius_override >= 0.0) ? static_cast<float>(pixel_radius_override) : ellipse.rx;
		float px_ry = (pixel_radius_override >= 0.0) ? static_cast<float>(pixel_radius_override) : ellipse.ry;
		px_rx = static_cast<float>(std::clamp(static_cast<double>(px_rx), style.sphere_min_pixel_radius, style.sphere_max_pixel_radius));
		px_ry = static_cast<float>(std::clamp(static_cast<double>(px_ry), style.sphere_min_pixel_radius, style.sphere_max_pixel_radius));

		draw_body_halo(draw_list, center_proj.screen, px_rx, px_ry, style.halo_color, style.halo_strength, style.halo_radius_factor);

		const bool can_render_true_wireframe = (style.shape == SchematicObjectShape::SphereFixedRadius) && (pixel_radius_override < 0.0);

		switch (style.sphere_style) {
			case SchematicSphereStyle::RealisticShaded: {
				draw_shaded_sphere(draw_list, center_proj.screen, px_rx, px_ry, color, secondary_color, style.shading);
				break;
			}
			case SchematicSphereStyle::GradientFill: {
				draw_gradient_sphere(draw_list, center_proj.screen, px_rx, px_ry, style.gradient_stops, style.gradient_radial, style.gradient_angle_deg);
				break;
			}
			case SchematicSphereStyle::Opaque: {
				const ImVec4 base_col4 = ImGui::ColorConvertU32ToFloat4(color);
				const ImU32 shadow_col = ImGui::ColorConvertFloat4ToU32(ImVec4(base_col4.x * 0.35f, base_col4.y * 0.35f, base_col4.z * 0.35f, base_col4.w));
				const ImU32 mid_col = ImGui::ColorConvertFloat4ToU32(ImVec4(base_col4.x * 0.7f, base_col4.y * 0.7f, base_col4.z * 0.7f, base_col4.w));
				if (std::abs(px_rx - px_ry) < 1.0f) {
					draw_list->AddCircleFilled(center_proj.screen, px_rx, shadow_col, 48);
					const ImVec2 mid_center(center_proj.screen.x - px_rx * 0.12f, center_proj.screen.y - px_rx * 0.12f);
					draw_list->AddCircleFilled(mid_center, px_rx * 0.88f, mid_col, 44);
					const ImVec2 highlight_center(center_proj.screen.x - px_rx * 0.32f, center_proj.screen.y - px_rx * 0.32f);
					draw_list->AddCircleFilled(highlight_center, px_rx * 0.55f, color, 40);
					const ImVec2 specular_center(center_proj.screen.x - px_rx * 0.42f, center_proj.screen.y - px_rx * 0.42f);
					draw_list->AddCircleFilled(specular_center, std::max(px_rx * 0.18f, 1.0f), IM_COL32(255, 255, 255, 90), 24);
					draw_list->AddCircle(center_proj.screen, px_rx, IM_COL32(8, 10, 18, 210), 48, 1.2f);
				} else {
					draw_list->AddEllipseFilled(center_proj.screen, ImVec2(px_rx, px_ry), shadow_col, 0.0f, 48);
					const ImVec2 mid_center(center_proj.screen.x - px_rx * 0.12f, center_proj.screen.y - px_ry * 0.12f);
					draw_list->AddEllipseFilled(mid_center, ImVec2(px_rx * 0.88f, px_ry * 0.88f), mid_col, 0.0f, 44);
					const ImVec2 highlight_center(center_proj.screen.x - px_rx * 0.32f, center_proj.screen.y - px_ry * 0.32f);
					draw_list->AddEllipseFilled(highlight_center, ImVec2(px_rx * 0.55f, px_ry * 0.55f), color, 0.0f, 40);
					const ImVec2 specular_center(center_proj.screen.x - px_rx * 0.42f, center_proj.screen.y - px_ry * 0.42f);
					draw_list->AddEllipseFilled(specular_center, ImVec2(std::max(px_rx * 0.18f, 1.0f), std::max(px_ry * 0.18f, 1.0f)), IM_COL32(255, 255, 255, 90), 0.0f, 24);
					draw_list->AddEllipse(center_proj.screen, ImVec2(px_rx, px_ry), IM_COL32(8, 10, 18, 210), 0.0f, 48, 1.2f);
				}
				break;
			}
			case SchematicSphereStyle::Translucent: {
				const ImVec4 col4 = ImGui::ColorConvertU32ToFloat4(color);
				const ImU32 faded = ImGui::ColorConvertFloat4ToU32(ImVec4(col4.x, col4.y, col4.z, static_cast<float>(style.translucency_alpha)));
				if (std::abs(px_rx - px_ry) < 1.0f) {
					draw_list->AddCircleFilled(center_proj.screen, px_rx, faded, 40);
					draw_list->AddCircle(center_proj.screen, px_rx, color, 40, 1.3f);
				} else {
					draw_list->AddEllipseFilled(center_proj.screen, ImVec2(px_rx, px_ry), faded, 0.0f, 40);
					draw_list->AddEllipse(center_proj.screen, ImVec2(px_rx, px_ry), color, 0.0f, 40, 1.3f);
				}
				break;
			}
			case SchematicSphereStyle::Wireframe:
			default:
				if (can_render_true_wireframe) {
					draw_projected_sphere_wireframe(draw_list, center_world, physical_radius, style.wireframe_rings, style.wireframe_segments, style.wireframe_segments, color, color, 1.2f);
				} else {
					const ImVec4 col4 = ImGui::ColorConvertU32ToFloat4(color);
					const ImU32 faded = ImGui::ColorConvertFloat4ToU32(ImVec4(col4.x, col4.y, col4.z, 0.28f));
					if (std::abs(px_rx - px_ry) < 1.0f) {
						draw_list->AddCircleFilled(center_proj.screen, px_rx, faded, 40);
						draw_list->AddCircle(center_proj.screen, px_rx, color, 40, 1.4f);
					} else {
						draw_list->AddEllipseFilled(center_proj.screen, ImVec2(px_rx, px_ry), faded, 0.0f, 40);
						draw_list->AddEllipse(center_proj.screen, ImVec2(px_rx, px_ry), color, 0.0f, 40, 1.4f);
					}
				}
				break;
		}

		// Outline pass (drawn after body)
		draw_body_outline(draw_list, center_proj.screen, px_rx, px_ry, style.outline);
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
		const double effective_radius = std::max(body.effective_radius(), 1e-4);

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

	[[nodiscard]] static std::string build_object_tag(const Dynamics::PostNewtonianBody& body, const SchematicObjectDisplayConfig& style, const Units::UnitDisplayPreferences* unit_prefs = nullptr) {
		std::string tag;
		if (style.show_id_in_tag) {
			tag += "#" + std::to_string(body.id);
		}
		if (style.show_mass_in_tag) {
			if (!tag.empty()) tag += " ";
			if (unit_prefs) {
				tag += "M=" + Units::format_mass(body.mass, unit_prefs->mass);
			} else {
				tag += "M=" + std::to_string(body.mass).substr(0, 6);
			}
		}
		if (style.show_speed_in_tag) {
			if (!tag.empty()) tag += " ";
			if (unit_prefs) {
				tag += "v=" + Units::format_velocity(body.speed(), unit_prefs->velocity);
			} else {
				tag += "v=" + std::to_string(body.speed()).substr(0, 6);
			}
		}
		return tag;
	}

	void draw_orbit_ring(
		ImDrawList* draw_list,
		const std::array<double, 3>& center,
		const std::array<double, 3>& axis,
		double radius,
		ImU32 color,
		float thickness,
		double center_depth,
		bool front_half
	) const {
		const std::array<double, 3> helper = (std::abs(axis[2]) < 0.9)
			? std::array<double, 3>{0.0, 0.0, 1.0}
			: std::array<double, 3>{1.0, 0.0, 0.0};
		std::array<double, 3> u{
			helper[1] * axis[2] - helper[2] * axis[1],
			helper[2] * axis[0] - helper[0] * axis[2],
			helper[0] * axis[1] - helper[1] * axis[0]
		};
		const double u_length = std::sqrt(u[0] * u[0] + u[1] * u[1] + u[2] * u[2]);
		for (double& component : u) component /= u_length;
		const std::array<double, 3> v{
			axis[1] * u[2] - axis[2] * u[1],
			axis[2] * u[0] - axis[0] * u[2],
			axis[0] * u[1] - axis[1] * u[0]
		};
		constexpr int segments = 96;
		std::vector<ImVec2> run;
		run.reserve(static_cast<size_t>(segments) + 1);
		const auto flush = [&]() {
			if (run.size() >= 2) {
				draw_list->AddPolyline(run.data(), static_cast<int>(run.size()), color, ImDrawFlags_None, thickness);
			}
			run.clear();
		};
		for (int i = 0; i <= segments; ++i) {
			const double angle = 2.0 * std::numbers::pi_v<double> * static_cast<double>(i) / static_cast<double>(segments);
			const double c = std::cos(angle);
			const double s = std::sin(angle);
			const std::array<double, 3> point{
				center[0] + radius * (c * u[0] + s * v[0]),
				center[1] + radius * (c * u[1] + s * v[1]),
				center[2] + radius * (c * u[2] + s * v[2])
			};
			const auto projected = project(point);
			const bool in_front = projected.forward_depth < center_depth;
			if (projected.visible && in_front == front_half) {
				run.push_back(projected.screen);
			} else {
				flush();
			}
		}
		flush();
	}

	void draw_spacetime_source(
		ImDrawList* draw_list,
		const Dynamics::PostNewtonianBody& body,
		const SchematicViewConfig& cfg,
		const Units::UnitDisplayPreferences* unit_prefs,
		bool overlay_pass
	) const {
		const auto center_proj = project(body.position);
		if (!center_proj.visible) return;

		const auto& style = cfg.effective_body_style(body.id);
		const double horizon = body.kerr_outer_horizon_radius();
		const double spin_parameter = body.kerr_spin_parameter();
		const double spin_length = spin_parameter * body.mass;
		const bool spinning = spin_parameter > 1e-6;
		const std::array<double, 3> axis = body.spin_axis_unit();
		const auto ellipse = compute_screen_ellipse(body.position, horizon);
		const float px_rx = static_cast<float>(std::clamp(static_cast<double>(ellipse.rx), style.sphere_min_pixel_radius, style.sphere_max_pixel_radius));
		const float px_ry = static_cast<float>(std::clamp(static_cast<double>(ellipse.ry), style.sphere_min_pixel_radius, style.sphere_max_pixel_radius));
		const ImVec2 center = center_proj.screen;
		const bool circular = std::abs(px_rx - px_ry) < 1.0f;

		if (!overlay_pass) {
			const double isco = Optics::DiskThermalProfile::kerr_isco_radius(body.mass, spin_length);
			const double disk_outer = Optics::DiskThermalProfile::disk_outer_radius(body.mass);
			const ImU32 isco_color = IM_COL32(255, 170, 70, 210);
			const ImU32 outer_color = IM_COL32(170, 105, 60, 95);

			if (disk_outer > 0.0) draw_orbit_ring(draw_list, body.position, axis, disk_outer, outer_color, 1.0f, center_proj.forward_depth, false);
			if (isco > 0.0) draw_orbit_ring(draw_list, body.position, axis, isco, isco_color, 1.6f, center_proj.forward_depth, false);

			draw_body_halo(draw_list, center, px_rx, px_ry, std::array<float, 4>{1.0f, 0.72f, 0.38f, 0.55f}, 0.6f, 1.7f);

			const ImU32 horizon_color = IM_COL32(1, 1, 3, 255);
			const ImU32 rim_color = IM_COL32(255, 210, 140, 235);
			if (circular) {
				draw_list->AddCircleFilled(center, px_rx, horizon_color, 64);
				draw_list->AddCircle(center, px_rx, rim_color, 64, 1.6f);
			} else {
				draw_list->AddEllipseFilled(center, ImVec2(px_rx, px_ry), horizon_color, 0.0f, 64);
				draw_list->AddEllipse(center, ImVec2(px_rx, px_ry), rim_color, 0.0f, 64, 1.6f);
			}

			if (disk_outer > 0.0) draw_orbit_ring(draw_list, body.position, axis, disk_outer, outer_color, 1.0f, center_proj.forward_depth, true);
			if (isco > 0.0) draw_orbit_ring(draw_list, body.position, axis, isco, isco_color, 1.6f, center_proj.forward_depth, true);
		}

		if (spinning) {
			SchematicVectorStyle axis_style{};
			axis_style.enabled = true;
			axis_style.length_scale = 1.0;
			axis_style.min_pixel_length = static_cast<double>(std::max(px_rx, px_ry) * 2.4f);
			axis_style.max_pixel_length = axis_style.min_pixel_length;
			axis_style.head_size_px = 8.0;
			axis_style.line_thickness_px = 2.0;
			const std::array<double, 3> axis_origin{
				body.position[0] + axis[0] * horizon,
				body.position[1] + axis[1] * horizon,
				body.position[2] + axis[2] * horizon
			};
			draw_body_vector(draw_list, axis_origin, axis, 1.0, axis_style, IM_COL32(120, 200, 255, 230));
		}

		if (cfg.show_vectors) {
			draw_all_body_vectors(draw_list, body, cfg, IM_COL32(120, 200, 255, 255));
		}

		if (cfg.show_tags && style.show_tag) {
			const std::string tag = build_object_tag(body, style, unit_prefs);
			if (!tag.empty()) {
				draw_list->AddText(
					ImVec2(center.x + std::max(px_rx, px_ry) + 5.0f, center.y - 8.0f),
					IM_COL32(225, 232, 250, 235),
					tag.c_str()
				);
			}
		}
	}

	void draw_body(ImDrawList* draw_list, const Dynamics::PostNewtonianBody& body, const SchematicViewConfig& cfg, double min_val, double max_val, const Units::UnitDisplayPreferences* unit_prefs = nullptr, bool overlay_pass = false) const {
		if (!body.enabled) return;
		const auto& style = cfg.effective_body_style(body.id);
		const auto proj = project(body.position);

		if (!proj.visible) {
			if (cfg.show_tags) {
				const auto& indicator_style = cfg.offscreen_indicator;
				ImU32 mapped_color = 0;
				switch (indicator_style.color_source) {
					case OffscreenIndicatorColorSource::ByMass:
						mapped_color = compute_coded_color(SchematicColorCodingMode::ByMass, body.mass, 0.0, std::max(body.mass * 4.0, 1.0), style.uniform_color);
						break;
					case OffscreenIndicatorColorSource::ByDistance:
						mapped_color = compute_coded_color(SchematicColorCodingMode::ByDistanceFromCenter, body_scalar_value(body, SchematicColorCodingMode::ByDistanceFromCenter), 0.0, 500.0, style.uniform_color);
						break;
					case OffscreenIndicatorColorSource::BySpeed:
						mapped_color = compute_coded_color(SchematicColorCodingMode::BySpeed, body.speed(), 0.0, std::max(body.speed() * 2.0, 0.01), style.uniform_color);
						break;
					case OffscreenIndicatorColorSource::ByTemperature:
						mapped_color = compute_coded_color(SchematicColorCodingMode::ByTemperature, body.temperature, 0.0, 20000.0, style.uniform_color);
						break;
					case OffscreenIndicatorColorSource::Fixed:
					default:
						break;
				}
				draw_offscreen_indicator(draw_list, body.position, "#" + std::to_string(body.id), indicator_style, mapped_color);
			}
			return;
		}

		if (body.is_spacetime_source) {
			draw_spacetime_source(draw_list, body, cfg, unit_prefs, overlay_pass);
			return;
		}

		const double color_value = body_scalar_value(body, style.color_mode);
		const auto fallback_color = (style.color_mode == SchematicColorCodingMode::Uniform) ? body.color : style.uniform_color;
		const ImU32 color = (style.color_mode == SchematicColorCodingMode::ByPhysicalIntelligent)
			? compute_intelligent_color(body)
			: compute_coded_color(style.color_mode, color_value, min_val, max_val, fallback_color);

		double pixel_radius_override = -1.0;
		if (style.shape == SchematicObjectShape::SphereByParameter) {
			const double param_value = body_parameter_source_value(body, style.parameter_source);
			pixel_radius_override = std::clamp(param_value * style.parameter_pixel_scale, style.sphere_min_pixel_radius, style.sphere_max_pixel_radius);
		}

		const double physical_radius = std::max(body.radius, 1e-4) * style.radius_scale;
		draw_object_shape(draw_list, body.position, physical_radius, style, color, pixel_radius_override, body.color_secondary);

		if (cfg.show_vectors) {
			draw_all_body_vectors(draw_list, body, cfg, color);
		}

		if (cfg.show_tags && style.show_tag) {
			const std::string tag = build_object_tag(body, style, unit_prefs);
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
				draw_offscreen_indicator(draw_list, center, "Central Object", cfg.offscreen_indicator);
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
			if (!body.enabled) continue;
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

	void draw_trails(ImDrawList* draw_list, const SchematicViewConfig& cfg, std::span<const Dynamics::PostNewtonianBody> bodies) const {
		const double now = ImGui::GetTime();
		for (const auto& [id, trail] : body_trails_) {
			if (trail.size() < 2) continue;
			std::array<float, 4> base_color{0.55f, 0.75f, 1.0f, 1.0f};
			for (const auto& b : bodies) {
				if (b.id == id) {
					base_color = b.color;
					break;
				}
			}
			for (size_t i = 1; i < trail.size(); ++i) {
				const auto& a = trail[i - 1];
				const auto& b = trail[i];
				const auto proj_a = project(a.position);
				const auto proj_b = project(b.position);
				if (!proj_a.visible || !proj_b.visible) continue;
				const double age = now - b.timestamp;
				const double t = std::clamp(1.0 - age / std::max(cfg.trail_duration_seconds, 1e-6), 0.0, 1.0);
				const float alpha = static_cast<float>(std::pow(t, cfg.trail_fade_power));
				const ImU32 color = ImGui::ColorConvertFloat4ToU32(ImVec4(base_color[0], base_color[1], base_color[2], alpha));
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

	[[nodiscard]] static std::vector<std::array<double, 3>> apply_uncertainty_offset(
		const std::vector<std::array<double, 3>>& base_points,
		double growth_rate,
		double sign
	) {
		std::vector<std::array<double, 3>> result;
		result.reserve(base_points.size());
		for (size_t i = 0; i < base_points.size(); ++i) {
			const auto& p = base_points[i];
			const double r = std::sqrt(p[0] * p[0] + p[1] * p[1] + p[2] * p[2]);
			const double offset = sign * growth_rate * static_cast<double>(i);
			if (r > 1e-9) {
				result.push_back({
					p[0] * (1.0 + offset / r),
					p[1] * (1.0 + offset / r),
					p[2] * (1.0 + offset / r)
				});
			} else {
				result.push_back(p);
			}
		}
		return result;
	}

	[[nodiscard]] static std::vector<std::array<double, 3>> compute_orbit_prediction_points(
		const Dynamics::PostNewtonianBody& body, double mu, int segments, double duration, int substeps
	) {
		std::vector<std::array<double, 3>> points;
		const int count = std::max(segments, 2);
		const int steps = std::clamp(substeps, 1, 32);
		const double dt = std::max(duration, 1e-6) / static_cast<double>(count * steps);
		std::array<double, 3> position = body.position;
		std::array<double, 3> velocity = body.velocity;
		auto acceleration = [mu](const std::array<double, 3>& p) {
			const double r2 = p[0] * p[0] + p[1] * p[1] + p[2] * p[2];
			const double inv_r3 = 1.0 / (std::max(r2, 1e-12) * std::sqrt(std::max(r2, 1e-12)));
			return std::array<double, 3>{-mu * p[0] * inv_r3, -mu * p[1] * inv_r3, -mu * p[2] * inv_r3};
		};
		points.reserve(static_cast<size_t>(count) + 1);
		points.push_back(position);
		for (int sample = 0; sample < count; ++sample) {
			for (int step = 0; step < steps; ++step) {
				const auto a0 = acceleration(position);
				for (size_t axis = 0; axis < 3; ++axis) position[axis] += velocity[axis] * dt + 0.5 * a0[axis] * dt * dt;
				const auto a1 = acceleration(position);
				for (size_t axis = 0; axis < 3; ++axis) velocity[axis] += 0.5 * (a0[axis] + a1[axis]) * dt;
			}
			if (!std::isfinite(position[0]) || !std::isfinite(position[1]) || !std::isfinite(position[2])) break;
			points.push_back(position);
		}
		return points;
	}

struct DepthSphere {
		std::array<double, 3> center{0.0, 0.0, 0.0};
		double radius{0.0};
		bool covering{false};
		ImVec2 screen_center{0.0f, 0.0f};
		float screen_rx{0.0f};
		float screen_ry{0.0f};
		double front_depth{0.0};

		[[nodiscard]] bool covers(const ProjectedPoint& point) const noexcept {
			if (!covering || screen_rx <= 0.0f || screen_ry <= 0.0f || point.forward_depth <= front_depth) {
				return false;
			}
			const float nx = (point.screen.x - screen_center.x) / screen_rx;
			const float ny = (point.screen.y - screen_center.y) / screen_ry;
			return nx * nx + ny * ny <= 1.0f;
		}
	};

	enum class PathCommandKind : uint8_t { Segment, Marker, FrustumEdge, FrustumApex };

	static constexpr size_t kHiddenSlot = std::numeric_limits<size_t>::max();

	struct PathCommand {
		PathCommandKind kind{PathCommandKind::Segment};
		size_t index{0};
		size_t slot{0};
		double depth{0.0};
		float depth_factor{0.0f};
		bool arrow{false};
		uint32_t segment{0};
		ImVec2 from{0.0f, 0.0f};
		ImVec2 to{0.0f, 0.0f};
	};

	struct PathScene {
		std::vector<PathCommand> commands{};
		std::vector<ProjectedPoint> vertices{};
		std::vector<ProjectedPoint> markers{};
		std::array<ProjectedPoint, 5> frustum{};
		size_t cursor{0};
	};

	[[nodiscard]] DepthSphere make_depth_sphere(
		const std::array<double, 3>& center,
		double physical_radius,
		double minimum_pixel_radius,
		double maximum_pixel_radius,
		double pixel_radius_override
	) const noexcept {
		DepthSphere sphere{center, physical_radius};
		if (physical_radius <= 0.0) {
			return sphere;
		}
		const auto projected = project(center);
		if (!projected.visible) {
			return sphere;
		}
		const auto ellipse = compute_screen_ellipse(center, physical_radius);
		double radius_x = (pixel_radius_override >= 0.0) ? pixel_radius_override : static_cast<double>(ellipse.rx);
		double radius_y = (pixel_radius_override >= 0.0) ? pixel_radius_override : static_cast<double>(ellipse.ry);
		radius_x = std::clamp(radius_x, minimum_pixel_radius, maximum_pixel_radius);
		radius_y = std::clamp(radius_y, minimum_pixel_radius, maximum_pixel_radius);
		sphere.covering = true;
		sphere.screen_center = projected.screen;
		sphere.screen_rx = static_cast<float>(radius_x);
		sphere.screen_ry = static_cast<float>(radius_y);
		sphere.front_depth = projected.forward_depth - physical_radius;
		return sphere;
	}

	[[nodiscard]] DepthSphere make_central_sphere(double central_radius, const SchematicViewConfig& cfg) const noexcept {
		const auto& style = cfg.central_object_style;
		if (style.shape == SchematicObjectShape::Point) {
			return DepthSphere{{0.0, 0.0, 0.0}, 0.0};
		}
		return make_depth_sphere({0.0, 0.0, 0.0}, central_radius * style.radius_scale, style.sphere_min_pixel_radius, style.sphere_max_pixel_radius, -1.0);
	}

	[[nodiscard]] DepthSphere make_body_sphere(const Dynamics::PostNewtonianBody& body, const SchematicViewConfig& cfg) const noexcept {
		const auto& style = cfg.effective_body_style(body.id);
		if (body.is_spacetime_source) {
			return make_depth_sphere(body.position, body.kerr_outer_horizon_radius(), style.sphere_min_pixel_radius, style.sphere_max_pixel_radius, -1.0);
		}
		if (style.shape == SchematicObjectShape::Point) {
			return DepthSphere{body.position, 0.0};
		}
		double pixel_radius_override = -1.0;
		if (style.shape == SchematicObjectShape::SphereByParameter) {
			pixel_radius_override = std::clamp(body_parameter_source_value(body, style.parameter_source) * style.parameter_pixel_scale, style.sphere_min_pixel_radius, style.sphere_max_pixel_radius);
		}
		return make_depth_sphere(body.position, std::max(body.radius, 1e-4) * style.radius_scale, style.sphere_min_pixel_radius, style.sphere_max_pixel_radius, pixel_radius_override);
	}

	[[nodiscard]] PathScene build_path_scene(std::span<const DepthSphere> spheres, double hidden_radius) const {
		PathScene scene;
		const auto& preview = path_preview_;
		if (preview.empty()) return scene;

		const std::array<double, 3> origin{0.0, 0.0, 0.0};
		const auto slot_for = [&](const std::array<double, 3>& world, const ProjectedPoint& projected) -> size_t {
			if (hidden_radius > 0.0 && is_occluded_by_sphere(world, origin, hidden_radius)) return kHiddenSlot;
			for (size_t i = 0; i < spheres.size(); ++i) {
				if (spheres[i].covers(projected)) return i;
			}
			return spheres.size();
		};

		scene.vertices.reserve(preview.vertices.size());
		double depth_min = std::numeric_limits<double>::max();
		double depth_max = std::numeric_limits<double>::lowest();
		for (const auto& vertex : preview.vertices) {
			const auto projected = project(vertex.position);
			scene.vertices.push_back(projected);
			if (projected.visible) {
				depth_min = std::min(depth_min, projected.forward_depth);
				depth_max = std::max(depth_max, projected.forward_depth);
			}
		}
		const double depth_span = std::max(depth_max - depth_min, 1e-9);

		constexpr double arrow_spacing = 110.0;
		constexpr int max_split_depth = 14;
		constexpr float min_split_pixels = 0.6f;
		double arrow_distance = 0.0;

		struct PathPiece {
			double t0;
			double t1;
			size_t slot;
			ProjectedPoint from;
			ProjectedPoint to;
		};
		struct PendingSpan {
			double t0;
			double t1;
			size_t slot0;
			size_t slot1;
			ProjectedPoint from;
			ProjectedPoint to;
			int level;
		};
		std::vector<PathPiece> pieces;
		std::vector<PendingSpan> pending;

		for (size_t i = 1; i < scene.vertices.size(); ++i) {
			if (preview.vertices[i].segment != preview.vertices[i - 1].segment) {
				arrow_distance = 0.0;
				continue;
			}
			const ProjectedPoint& start = scene.vertices[i - 1];
			const ProjectedPoint& end = scene.vertices[i];
			if (!start.visible || !end.visible) continue;
			const auto& wa = preview.vertices[i - 1].position;
			const auto& wb = preview.vertices[i].position;
			const auto world_at = [&](double t) noexcept -> std::array<double, 3> {
				return {wa[0] + (wb[0] - wa[0]) * t, wa[1] + (wb[1] - wa[1]) * t, wa[2] + (wb[2] - wa[2]) * t};
			};

			pieces.clear();
			pending.clear();
			pending.push_back(PendingSpan{0.0, 1.0, slot_for(wa, start), slot_for(wb, end), start, end, 0});
			while (!pending.empty()) {
				const PendingSpan span = pending.back();
				pending.pop_back();
				const double tm = 0.5 * (span.t0 + span.t1);
				const auto wm = world_at(tm);
				const ProjectedPoint pm = project(wm);
				const size_t sm = pm.visible ? slot_for(wm, pm) : span.slot0;
				const float dx = span.to.screen.x - span.from.screen.x;
				const float dy = span.to.screen.y - span.from.screen.y;
				const bool uniform = (span.slot0 == span.slot1) && (sm == span.slot0);
				const bool tiny = (dx * dx + dy * dy) <= min_split_pixels * min_split_pixels;
				if (uniform || tiny || span.level >= max_split_depth || !pm.visible) {
					pieces.push_back(PathPiece{span.t0, span.t1, uniform ? span.slot0 : sm, span.from, span.to});
					continue;
				}
				pending.push_back(PendingSpan{tm, span.t1, sm, span.slot1, pm, span.to, span.level + 1});
				pending.push_back(PendingSpan{span.t0, tm, span.slot0, sm, span.from, pm, span.level + 1});
			}

			std::sort(pieces.begin(), pieces.end(), [](const PathPiece& lhs, const PathPiece& rhs) noexcept { return lhs.t0 < rhs.t0; });
			size_t merged = 0;
			for (size_t k = 0; k < pieces.size(); ++k) {
				if (merged > 0 && pieces[merged - 1].slot == pieces[k].slot) {
					pieces[merged - 1].to = pieces[k].to;
					pieces[merged - 1].t1 = pieces[k].t1;
				} else {
					pieces[merged++] = pieces[k];
				}
			}
			pieces.erase(pieces.begin() + static_cast<ptrdiff_t>(merged), pieces.end());

			for (const PathPiece& piece : pieces) {
				if (piece.slot == kHiddenSlot) continue;
				const double dx = static_cast<double>(piece.to.screen.x - piece.from.screen.x);
				const double dy = static_cast<double>(piece.to.screen.y - piece.from.screen.y);
				arrow_distance += std::sqrt(dx * dx + dy * dy);

				PathCommand command;
				command.kind = PathCommandKind::Segment;
				command.index = i;
				command.segment = preview.vertices[i].segment;
				command.slot = piece.slot;
				command.from = piece.from.screen;
				command.to = piece.to.screen;
				command.depth = 0.5 * (piece.from.forward_depth + piece.to.forward_depth);
				command.depth_factor = static_cast<float>(std::clamp((command.depth - depth_min) / depth_span, 0.0, 1.0));
				if (arrow_distance >= arrow_spacing) {
					command.arrow = preview.show_direction;
					arrow_distance = 0.0;
				}
				scene.commands.push_back(command);
			}
		}

		if (preview.show_markers) {
			scene.markers.reserve(preview.markers.size());
			for (size_t i = 0; i < preview.markers.size(); ++i) {
				const auto projected = project(preview.markers[i].position);
				scene.markers.push_back(projected);
				if (!projected.visible) continue;
				const size_t slot = slot_for(preview.markers[i].position, projected);
				if (slot == kHiddenSlot) continue;
				PathCommand command;
				command.kind = PathCommandKind::Marker;
				command.index = i;
				command.slot = slot;
				command.depth = projected.forward_depth;
				scene.commands.push_back(command);
			}
		}

		if (preview.show_frustum && preview.cursor.valid) {
			const auto& cursor = preview.cursor;
			const double length = preview.frustum_length;
			const double half_width = std::tan(std::clamp(cursor.fov_rad, 0.02, 3.0) * 0.5) * length;
			const double half_height = half_width * 9.0 / 16.0;
			const auto corner = [&](double sx, double sy) noexcept {
				return std::array<double, 3>{
					cursor.position[0] + cursor.forward[0] * length + cursor.right[0] * sx * half_width + cursor.up[0] * sy * half_height,
					cursor.position[1] + cursor.forward[1] * length + cursor.right[1] * sx * half_width + cursor.up[1] * sy * half_height,
					cursor.position[2] + cursor.forward[2] * length + cursor.right[2] * sx * half_width + cursor.up[2] * sy * half_height
				};
			};
			const std::array<std::array<double, 3>, 5> world{cursor.position, corner(-1.0, 1.0), corner(1.0, 1.0), corner(1.0, -1.0), corner(-1.0, -1.0)};
			for (size_t k = 0; k < world.size(); ++k) {
				scene.frustum[k] = project(world[k]);
			}

			static constexpr std::array<std::pair<size_t, size_t>, 8> edges{{{0, 1}, {0, 2}, {0, 3}, {0, 4}, {1, 2}, {2, 3}, {3, 4}, {4, 1}}};
			for (size_t e = 0; e < edges.size(); ++e) {
				const auto& pa = scene.frustum[edges[e].first];
				const auto& pb = scene.frustum[edges[e].second];
				if (!pa.visible || !pb.visible) continue;
				const auto& wa = world[edges[e].first];
				const auto& wb = world[edges[e].second];
				const std::array<double, 3> mid{(wa[0] + wb[0]) * 0.5, (wa[1] + wb[1]) * 0.5, (wa[2] + wb[2]) * 0.5};
				const size_t slot = slot_for(mid, project(mid));
				if (slot == kHiddenSlot) continue;
				PathCommand command;
				command.kind = PathCommandKind::FrustumEdge;
				command.index = e;
				command.slot = slot;
				command.depth = 0.5 * (pa.forward_depth + pb.forward_depth);
				scene.commands.push_back(command);
			}
			const size_t apex_slot = scene.frustum[0].visible ? slot_for(world[0], scene.frustum[0]) : kHiddenSlot;
			if (apex_slot != kHiddenSlot) {
				PathCommand command;
				command.kind = PathCommandKind::FrustumApex;
				command.slot = apex_slot;
				command.depth = scene.frustum[0].forward_depth;
				scene.commands.push_back(command);
			}
		}

		std::stable_sort(scene.commands.begin(), scene.commands.end(), [](const PathCommand& a, const PathCommand& b) noexcept {
			if (a.slot != b.slot) return a.slot < b.slot;
			return a.depth > b.depth;
		});
		return scene;
	}

	void draw_path_command(ImDrawList* draw_list, const PathScene& scene, const PathCommand& command) const {
		const auto& preview = path_preview_;
		const float near_factor = 1.0f - command.depth_factor;
		const ImU32 frustum_color = IM_COL32(255, 245, 140, 240);
		switch (command.kind) {
			case PathCommandKind::Segment: {
				const ProjectedPoint from{true, command.from, 0.0};
				const ProjectedPoint to{true, command.to, 0.0};
				const uint32_t segment = command.segment;
				const auto& rgb = Capture::kPathPreviewPalette[segment % Capture::kPathPreviewPalette.size()];
				const bool highlighted = preview.highlighted_segment >= 0 && static_cast<uint32_t>(preview.highlighted_segment) == segment;
				const float thickness = (highlighted ? 3.6f : 2.0f) * (0.7f + 0.3f * near_factor);
				const int alpha = static_cast<int>(150.0f + 95.0f * near_factor);
				const ImU32 color = IM_COL32(rgb[0], rgb[1], rgb[2], alpha);
				draw_list->AddLine(from.screen, to.screen, IM_COL32(6, 8, 14, alpha * 3 / 5), thickness + 2.0f);
				draw_list->AddLine(from.screen, to.screen, color, thickness);
				if (preview.show_samples) {
					draw_list->AddCircleFilled(to.screen, 1.8f, IM_COL32(255, 255, 255, 170), 8);
				}
				if (command.arrow) {
					const float dx = to.screen.x - from.screen.x;
					const float dy = to.screen.y - from.screen.y;
					const float length = std::sqrt(dx * dx + dy * dy);
					if (length > 1e-3f) {
						const ImVec2 n{dx / length, dy / length};
						const ImVec2 perp{-n.y, n.x};
						const float size = 5.0f + 2.5f * near_factor;
						const ImVec2 tip{to.screen.x + n.x * size, to.screen.y + n.y * size};
						const ImVec2 base{to.screen.x - n.x * size, to.screen.y - n.y * size};
						draw_list->AddTriangleFilled(tip, ImVec2(base.x + perp.x * size * 0.8f, base.y + perp.y * size * 0.8f), ImVec2(base.x - perp.x * size * 0.8f, base.y - perp.y * size * 0.8f), color);
					}
				}
				break;
			}
			case PathCommandKind::Marker: {
				const auto& marker = preview.markers[command.index];
				const ImVec2 c = scene.markers[command.index].screen;
				switch (marker.kind) {
					case Capture::PathPreviewMarkerKind::Start:
						draw_list->AddCircleFilled(c, 6.0f, IM_COL32(90, 235, 120, 245), 16);
						draw_list->AddCircle(c, 6.0f, IM_COL32(6, 8, 14, 220), 16, 1.4f);
						break;
					case Capture::PathPreviewMarkerKind::End:
						draw_list->AddCircleFilled(c, 6.0f, IM_COL32(245, 90, 90, 245), 16);
						draw_list->AddCircle(c, 6.0f, IM_COL32(6, 8, 14, 220), 16, 1.4f);
						break;
					case Capture::PathPreviewMarkerKind::Event:
						draw_list->AddQuadFilled(ImVec2(c.x, c.y - 7.0f), ImVec2(c.x + 7.0f, c.y), ImVec2(c.x, c.y + 7.0f), ImVec2(c.x - 7.0f, c.y), IM_COL32(255, 220, 70, 245));
						break;
					case Capture::PathPreviewMarkerKind::SegmentBoundary:
					default:
						draw_list->AddCircle(c, 5.0f, IM_COL32(240, 245, 255, 235), 16, 1.6f);
						break;
				}
				if (preview.show_labels && !marker.label.empty()) {
					const ImVec2 text_position(c.x + 9.0f, c.y - 7.0f);
					const ImVec2 text_size = ImGui::CalcTextSize(marker.label.c_str());
					draw_list->AddRectFilled(ImVec2(text_position.x - 2.0f, text_position.y - 1.0f), ImVec2(text_position.x + text_size.x + 2.0f, text_position.y + text_size.y + 1.0f), IM_COL32(8, 10, 18, 150), 3.0f);
					draw_list->AddText(text_position, IM_COL32(230, 236, 250, 235), marker.label.c_str());
				}
				break;
			}
			case PathCommandKind::FrustumEdge: {
				static constexpr std::array<std::pair<size_t, size_t>, 8> edges{{{0, 1}, {0, 2}, {0, 3}, {0, 4}, {1, 2}, {2, 3}, {3, 4}, {4, 1}}};
				const auto& pa = scene.frustum[edges[command.index].first];
				const auto& pb = scene.frustum[edges[command.index].second];
				draw_list->AddLine(pa.screen, pb.screen, frustum_color, command.index < 4 ? 1.3f : 1.6f);
				break;
			}
			case PathCommandKind::FrustumApex:
				draw_list->AddCircleFilled(scene.frustum[0].screen, 4.0f, frustum_color, 12);
				break;
		}
	}

	void flush_path_scene(ImDrawList* draw_list, PathScene& scene, size_t slot_limit) const {
		while (scene.cursor < scene.commands.size() && scene.commands[scene.cursor].slot < slot_limit) {
			draw_path_command(draw_list, scene, scene.commands[scene.cursor]);
			++scene.cursor;
		}
	}

public:
	void set_path_preview(const Capture::PathPreview& preview) {
		path_preview_ = preview;
	}

	void clear_path_preview() noexcept {
		path_preview_ = Capture::PathPreview{};
	}

	[[nodiscard]] bool has_path_preview() const noexcept {
		return !path_preview_.empty();
	}

	void render_path_preview_only(ImDrawList* draw_list) const {
		PathScene scene = build_path_scene(std::span<const DepthSphere>{}, 2.0 * lensing_mass_);
		flush_path_scene(draw_list, scene, std::numeric_limits<size_t>::max());
	}

	void configure(
		const Orchestrator::CameraState& cam,
		Observer::ProjectionMode projection_mode,
		double fov_rad,
		const ImVec2& rect_min,
		const ImVec2& rect_size,
		double lensing_mass = 0.0,
		bool apply_lensing = false
	) noexcept {
		camera_position_ = cam.position;
		projection_mode_ = projection_mode;
		lensing_mass_ = std::max(lensing_mass, 0.0);
		apply_lensing_ = apply_lensing;
		rect_min_ = rect_min;
		rect_size_ = ImVec2(std::max(rect_size.x, 0.0f), std::max(rect_size.y, 0.0f));
		aspect_ = (rect_size_.y > 0.0f) ? static_cast<double>(rect_size_.x) / static_cast<double>(rect_size_.y) : 1.0;

		constexpr double min_fov = 1e-6;
		constexpr double max_fov = std::numbers::pi_v<double> - 1e-6;
		fov_rad_ = std::clamp(fov_rad, min_fov, max_fov);

		const auto orientation = cam.orientation_basis();
		tetrad_forward_ = orientation.forward;
		tetrad_right_ = orientation.right;
		tetrad_up_ = orientation.up;
	}

	void render_overlay(ImDrawList* draw_list, const Orchestrator::SimulationOrchestrator<1024>& orchestrator, const SchematicViewConfig& cfg) {
		const auto& sys = orchestrator.nbody_system();
		const auto bodies = sys.bodies();
		if (bodies.empty()) {
			PathScene empty_scene = build_path_scene(std::span<const DepthSphere>{}, 2.0 * lensing_mass_);
			flush_path_scene(draw_list, empty_scene, std::numeric_limits<size_t>::max());
			return;
		}

		const auto& params = orchestrator.parameters();
		const double mu = std::max(params.mass, 1e-6);

		update_trails(bodies, cfg);
		if (cfg.show_trails) {
			draw_trails(draw_list, cfg, bodies);
		}

		if (cfg.show_orbit_predictions) {
			for (const auto& body : bodies) {
				if (!body.enabled) continue;
				const auto pts = compute_orbit_prediction_points(body, mu, cfg.orbit_prediction_segments, cfg.orbit_prediction_duration, cfg.orbit_prediction_substeps);
				if (cfg.show_orbit_prediction_uncertainty) {
					const auto upper_pts = apply_uncertainty_offset(pts, cfg.orbit_prediction_uncertainty_growth, 1.0);
					const auto lower_pts = apply_uncertainty_offset(pts, cfg.orbit_prediction_uncertainty_growth, -1.0);
					const ImU32 uncertainty_color = IM_COL32(170, 200, 255, clamp8(255.0 * cfg.orbit_prediction_uncertainty_opacity));
					draw_polyline_3d(draw_list, upper_pts, uncertainty_color, 1.0f, false);
					draw_polyline_3d(draw_list, lower_pts, uncertainty_color, 1.0f, false);
				}
				draw_polyline_3d(draw_list, pts, IM_COL32(170, 200, 255, clamp8(255.0 * cfg.orbit_prediction_opacity)), static_cast<float>(cfg.orbit_prediction_thickness), false);
			}
		}

		double min_val = std::numeric_limits<double>::max();
		double max_val = std::numeric_limits<double>::lowest();
		if (cfg.body_style.color_mode != SchematicColorCodingMode::Uniform) {
			for (const auto& body : bodies) {
				if (!body.enabled) continue;
				const double v = body_scalar_value(body, cfg.body_style.color_mode);
				min_val = std::min(min_val, v);
				max_val = std::max(max_val, v);
			}
		}

		const double primary_mass = std::max(params.mass, 0.0);
		const double primary_spin = std::clamp(params.spin, -primary_mass, primary_mass);
		const double primary_horizon = (primary_mass > 0.0)
			? (primary_mass + std::sqrt(std::max(primary_mass * primary_mass - primary_spin * primary_spin, 0.0)))
			: 0.0;

		struct OverlayEntry {
			double depth;
			const Dynamics::PostNewtonianBody* body;
		};
		std::vector<OverlayEntry> overlay_entries;
		if (cfg.show_bodies) {
			overlay_entries.reserve(bodies.size());
			for (const auto& body : bodies) {
				if (primary_horizon > 0.0 && is_occluded_by_sphere(body.position, std::array<double, 3>{0.0, 0.0, 0.0}, primary_horizon)) continue;
				overlay_entries.push_back(OverlayEntry{project(body.position).forward_depth, &body});
			}
			std::stable_sort(overlay_entries.begin(), overlay_entries.end(), [](const OverlayEntry& a, const OverlayEntry& b) noexcept {
				return a.depth > b.depth;
			});
		}

		std::vector<DepthSphere> overlay_spheres;
		overlay_spheres.reserve(overlay_entries.size());
		for (const auto& entry : overlay_entries) {
			overlay_spheres.push_back(make_body_sphere(*entry.body, cfg));
		}
		PathScene overlay_scene = build_path_scene(overlay_spheres, 2.0 * lensing_mass_);
		for (size_t i = 0; i < overlay_entries.size(); ++i) {
			flush_path_scene(draw_list, overlay_scene, i + 1);
			draw_body(draw_list, *overlay_entries[i].body, cfg, min_val, max_val, nullptr, true);
		}
		flush_path_scene(draw_list, overlay_scene, std::numeric_limits<size_t>::max());
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
			draw_trails(draw_list, cfg, bodies);
		}

		if (cfg.show_orbit_predictions) {
			for (const auto& body : bodies) {
				if (!body.enabled) continue;
				const auto pts = compute_orbit_prediction_points(body, mu, cfg.orbit_prediction_segments, cfg.orbit_prediction_duration, cfg.orbit_prediction_substeps);
				if (cfg.show_orbit_prediction_uncertainty) {
					const auto upper_pts = apply_uncertainty_offset(pts, cfg.orbit_prediction_uncertainty_growth, 1.0);
					const auto lower_pts = apply_uncertainty_offset(pts, cfg.orbit_prediction_uncertainty_growth, -1.0);
					const ImU32 uncertainty_color = IM_COL32(170, 200, 255, clamp8(255.0 * cfg.orbit_prediction_uncertainty_opacity));
					draw_polyline_3d(draw_list, upper_pts, uncertainty_color, 1.0f, false);
					draw_polyline_3d(draw_list, lower_pts, uncertainty_color, 1.0f, false);
				}
				draw_polyline_3d(draw_list, pts, IM_COL32(170, 200, 255, clamp8(255.0 * cfg.orbit_prediction_opacity)), static_cast<float>(cfg.orbit_prediction_thickness), false);
			}
		}

		double min_val = std::numeric_limits<double>::max();
		double max_val = std::numeric_limits<double>::lowest();
		if (cfg.body_style.color_mode != SchematicColorCodingMode::Uniform) {
			for (const auto& body : bodies) {
				if (!body.enabled) continue;
				const double v = body_scalar_value(body, cfg.body_style.color_mode);
				min_val = std::min(min_val, v);
				max_val = std::max(max_val, v);
			}
		}

		struct SceneDrawEntry {
			double depth;
			const Dynamics::PostNewtonianBody* body;
		};
		std::vector<SceneDrawEntry> scene_entries;
		scene_entries.reserve(bodies.size() + 1);
		if (cfg.show_central_object) {
			scene_entries.push_back(SceneDrawEntry{project(std::array<double, 3>{0.0, 0.0, 0.0}).forward_depth, nullptr});
		}
		if (cfg.show_bodies) {
			for (const auto& body : bodies) {
				if (!body.enabled) continue;
				scene_entries.push_back(SceneDrawEntry{project(body.position).forward_depth, &body});
			}
		}
		std::stable_sort(scene_entries.begin(), scene_entries.end(), [](const SceneDrawEntry& a, const SceneDrawEntry& b) noexcept {
			return a.depth > b.depth;
		});

		std::vector<DepthSphere> depth_spheres;
		depth_spheres.reserve(scene_entries.size());
		for (const auto& entry : scene_entries) {
			depth_spheres.push_back(entry.body == nullptr ? make_central_sphere(central_radius, cfg) : make_body_sphere(*entry.body, cfg));
		}
		PathScene path_scene = build_path_scene(depth_spheres, 0.0);

		const auto& unit_prefs = orchestrator.unit_preferences();
		for (size_t i = 0; i < scene_entries.size(); ++i) {
			flush_path_scene(draw_list, path_scene, i + 1);
			const auto& entry = scene_entries[i];
			if (entry.body == nullptr) {
				draw_central_object(draw_list, params, central_radius, cfg);
			} else {
				draw_body(draw_list, *entry.body, cfg, min_val, max_val, &unit_prefs);
			}
		}
		flush_path_scene(draw_list, path_scene, std::numeric_limits<size_t>::max());
	}
};

}

#pragma once

#include "relativistic/observer/camera_projections.hpp"
#include "relativistic/observer/direction_projection.hpp"
#include "relativistic/ui/spatial_reference/spatial_reference_config.hpp"
#include "relativistic/units/unit_system.hpp"
#include <imgui.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <optional>
#include <string>

namespace Relativistic::UI {

namespace SpatialUnits {

[[nodiscard]] inline double meters_per_distance_unit(Units::DistanceUnit unit) noexcept {
	static constexpr std::array<double, 10> kMeters{
		1.0, 1.0e3, 0.3048, 1609.344, 1852.0, 1.495978707e11, 9.4607304725808e15, 3.0856775814913673e16, 3.0856775814913673e19, 6.957e8
	};
	const size_t index = static_cast<size_t>(unit);
	return (index < kMeters.size()) ? kMeters[index] : 1.0;
}

[[nodiscard]] inline double nice_step(double value) noexcept {
	if (!(value > 0.0) || !std::isfinite(value)) {
		return 1.0;
	}
	const double base = std::pow(10.0, std::floor(std::log10(value)));
	const double mantissa = value / base;
	const double nice = (mantissa < 1.5) ? 1.0 : ((mantissa < 3.5) ? 2.0 : ((mantissa < 7.5) ? 5.0 : 10.0));
	return nice * base;
}

}

struct SpatialReferenceView {
	std::array<double, 3> camera_position{0.0, 0.0, 0.0};
	std::array<double, 3> forward{1.0, 0.0, 0.0};
	std::array<double, 3> right{0.0, -1.0, 0.0};
	std::array<double, 3> up{0.0, 0.0, 1.0};
	Observer::ProjectionMode projection_mode{Observer::ProjectionMode::Pinhole};
	double fov_rad{1.0471975511965976};
	ImVec2 rect_min{0.0f, 0.0f};
	ImVec2 rect_size{0.0f, 0.0f};
	double lensing_mass{0.0};
	double horizon_radius{0.0};
	double length_scale_meters{1.0};
	Units::DistanceUnit distance_unit{};
	std::optional<std::array<double, 3>> body_center{};
};

class SpatialReferenceRenderer {
private:
	using Vec3 = std::array<double, 3>;

	struct Projected {
		bool visible{false};
		ImVec2 screen{0.0f, 0.0f};
		double depth{0.0};
	};

	struct RadialFade {
		Vec3 center{0.0, 0.0, 0.0};
		double radius{1.0};
	};

	ImDrawList* list_{nullptr};
	const SpatialReferenceConfig* cfg_{nullptr};
	const SpatialReferenceView* view_{nullptr};
	double aspect_{1.0};
	bool all_sky_{false};
	bool clip_front_{false};
	bool lens_active_{false};
	double unit_meters_{1.0};
	double length_scale_{1.0};
	double focus_distance_{1.0};
	double grid_spacing_{1.0};
	double grid_extent_{1.0};
	int64_t grid_lines_{1};
	double axis_length_{1.0};
	double tick_spacing_{1.0};
	double ring_spacing_{1.0};
	double depth_reference_{1.0};
	Vec3 center_{0.0, 0.0, 0.0};

	[[nodiscard]] static double dot(const Vec3& a, const Vec3& b) noexcept {
		return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
	}

	[[nodiscard]] static double norm(const Vec3& a) noexcept {
		return std::sqrt(dot(a, a));
	}

	[[nodiscard]] static Vec3 offset(const Vec3& origin, const Vec3& direction, double distance) noexcept {
		return {origin[0] + direction[0] * distance, origin[1] + direction[1] * distance, origin[2] + direction[2] * distance};
	}

	[[nodiscard]] static Vec3 lerp(const Vec3& a, const Vec3& b, double t) noexcept {
		return {a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t, a[2] + (b[2] - a[2]) * t};
	}

	[[nodiscard]] static ImU32 make_color(const std::array<float, 4>& color, float alpha) noexcept {
		return ImGui::ColorConvertFloat4ToU32(ImVec4(color[0], color[1], color[2], std::clamp(alpha, 0.0f, 1.0f)));
	}

	[[nodiscard]] double resolve_spacing(SpatialSizeMode mode, double fixed_value, double desired) const noexcept {
		if (mode == SpatialSizeMode::Fixed) {
			return std::max(fixed_value, 1.0e-9);
		}
		const double simulation_per_unit = unit_meters_ / length_scale_;
		return SpatialUnits::nice_step(desired / simulation_per_unit) * simulation_per_unit;
	}

	[[nodiscard]] uint32_t line_subdivisions(bool needs_gradient) const noexcept {
		if (!clip_front_ || lens_active_) {
			return std::max<uint32_t>(cfg_->curve_subdivisions, 2U);
		}
		return needs_gradient ? 16U : 1U;
	}

	void prepare() {
		const SpatialReferenceConfig& cfg = *cfg_;
		const SpatialReferenceView& view = *view_;
		aspect_ = static_cast<double>(view.rect_size.x) / std::max(static_cast<double>(view.rect_size.y), 1.0);
		const Observer::ProjectionMode mode = view.projection_mode;
		all_sky_ = (mode == Observer::ProjectionMode::Equirectangular360 || mode == Observer::ProjectionMode::HammerAitoff);
		clip_front_ = (mode == Observer::ProjectionMode::Pinhole || mode == Observer::ProjectionMode::AutoZoomAberration
			|| mode == Observer::ProjectionMode::PaniniCylindrical || mode == Observer::ProjectionMode::FisheyeOrthographic);
		lens_active_ = cfg.lens_approximation && view.lensing_mass > 0.0;
		length_scale_ = std::max(view.length_scale_meters, 1.0e-300);
		unit_meters_ = SpatialUnits::meters_per_distance_unit(view.distance_unit);

		Vec3 anchor{0.0, 0.0, 0.0};
		if (cfg.center_mode == SpatialCenterMode::CustomPoint) {
			anchor = cfg.custom_center;
		} else if (cfg.center_mode == SpatialCenterMode::Body && view.body_center.has_value()) {
			anchor = *view.body_center;
		}
		const Vec3 to_anchor{anchor[0] - view.camera_position[0], anchor[1] - view.camera_position[1], anchor[2] - view.camera_position[2]};
		focus_distance_ = std::max(norm(to_anchor), 1.0e-3);
		const double scale = (cfg.center_mode == SpatialCenterMode::CameraSnapped)
			? std::max(norm(view.camera_position) * 0.35, 1.0e-2)
			: focus_distance_;

		grid_spacing_ = resolve_spacing(cfg.grid_spacing_mode, cfg.grid_spacing, scale / 8.0);
		const double requested_extent = (cfg.grid_extent_mode == SpatialSizeMode::Fixed) ? std::max(cfg.grid_extent, grid_spacing_) : grid_spacing_ * 14.0;
		const int64_t line_limit = std::max<int64_t>(static_cast<int64_t>(cfg.max_lines_per_direction / 2U), 1);
		grid_lines_ = std::clamp<int64_t>(static_cast<int64_t>(std::floor(requested_extent / grid_spacing_)), 1, line_limit);
		grid_extent_ = static_cast<double>(grid_lines_) * grid_spacing_;
		axis_length_ = (cfg.axis_length_mode == SpatialSizeMode::Fixed) ? std::max(cfg.axis_length, 1.0e-9) : resolve_spacing(SpatialSizeMode::Automatic, 0.0, scale * 1.6);
		tick_spacing_ = resolve_spacing(cfg.tick_spacing_mode, cfg.tick_spacing, scale / 6.0);
		ring_spacing_ = resolve_spacing(cfg.ring_spacing_mode, cfg.ring_spacing, scale / 6.0);
		depth_reference_ = (cfg.depth_fade_mode == SpatialSizeMode::Fixed) ? std::max(cfg.depth_fade_reference, 1.0e-9) : std::max(scale * 2.5, 1.0e-6);

		center_ = anchor;
		if (cfg.center_mode == SpatialCenterMode::CameraSnapped) {
			for (size_t i = 0; i < 3; ++i) {
				center_[i] = std::round(view.camera_position[i] / grid_spacing_) * grid_spacing_;
			}
		}
	}

	[[nodiscard]] Projected project(const Vec3& world) const noexcept {
		const SpatialReferenceView& view = *view_;
		const Vec3 delta{world[0] - view.camera_position[0], world[1] - view.camera_position[1], world[2] - view.camera_position[2]};
		double fwd = dot(delta, view.forward);
		double right = dot(delta, view.right);
		double up = dot(delta, view.up);

		if (lens_active_) {
			const double camera_r = norm(view.camera_position);
			const double ray_r = norm(delta);
			if (camera_r > 1.0e-6 && ray_r > 1.0e-6) {
				const Vec3& cam = view.camera_position;
				const double lens_fwd = -(cam[0] * view.forward[0] + cam[1] * view.forward[1] + cam[2] * view.forward[2]) / camera_r;
				const double lens_right = -(cam[0] * view.right[0] + cam[1] * view.right[1] + cam[2] * view.right[2]) / camera_r;
				const double lens_up = -(cam[0] * view.up[0] + cam[1] * view.up[1] + cam[2] * view.up[2]) / camera_r;
				const double ray_dot_lens = std::clamp((fwd * lens_fwd + right * lens_right + up * lens_up) / ray_r, -1.0, 1.0);
				const double impact = std::max(camera_r * std::sqrt(std::max(1.0 - ray_dot_lens * ray_dot_lens, 0.0)), 2.05 * view.lensing_mass);
				const double deflection = std::clamp(4.0 * view.lensing_mass / impact, 0.0, 0.35);
				fwd += ray_r * deflection * lens_fwd;
				right += ray_r * deflection * lens_right;
				up += ray_r * deflection * lens_up;
			}
		}

		Projected result;
		result.depth = fwd;
		const auto uv = Observer::direction_to_screen_uv(view.projection_mode, fwd, right, up, view.fov_rad);
		if (!uv.has_value()) {
			return result;
		}
		const double u_screen = all_sky_ ? uv->first : (uv->first / std::max(aspect_, 1.0e-6));
		const double v_screen = uv->second;
		if (!std::isfinite(u_screen) || !std::isfinite(v_screen)) {
			return result;
		}
		constexpr double limit = 2.0e5;
		const double x = std::clamp((u_screen * 0.5 + 0.5) * static_cast<double>(view.rect_size.x), -limit, limit);
		const double y = std::clamp((v_screen * 0.5 + 0.5) * static_cast<double>(view.rect_size.y), -limit, limit);
		result.screen = ImVec2(view.rect_min.x + static_cast<float>(x), view.rect_min.y + static_cast<float>(y));
		result.visible = true;
		return result;
	}

	[[nodiscard]] bool clip_to_front(Vec3& a, Vec3& b) const noexcept {
		constexpr double near_depth = 1.0e-3;
		const SpatialReferenceView& view = *view_;
		const auto depth_of = [&view](const Vec3& p) noexcept {
			return (p[0] - view.camera_position[0]) * view.forward[0] + (p[1] - view.camera_position[1]) * view.forward[1] + (p[2] - view.camera_position[2]) * view.forward[2];
		};
		const double da = depth_of(a);
		const double db = depth_of(b);
		if (da <= near_depth && db <= near_depth) {
			return false;
		}
		if (da < near_depth) {
			a = lerp(a, b, (near_depth - da) / (db - da));
		} else if (db < near_depth) {
			b = lerp(b, a, (near_depth - db) / (da - db));
		}
		return true;
	}

	[[nodiscard]] bool occluded_by_horizon(const Vec3& target) const noexcept {
		const SpatialReferenceView& view = *view_;
		if (view.horizon_radius <= 0.0) {
			return false;
		}
		const Vec3 direction{target[0] - view.camera_position[0], target[1] - view.camera_position[1], target[2] - view.camera_position[2]};
		const double a = dot(direction, direction);
		if (a <= 1.0e-18) {
			return false;
		}
		const double b = 2.0 * dot(view.camera_position, direction);
		const double c = dot(view.camera_position, view.camera_position) - view.horizon_radius * view.horizon_radius;
		const double discriminant = b * b - 4.0 * a * c;
		if (discriminant < 0.0) {
			return false;
		}
		const double entry = (-b - std::sqrt(discriminant)) / (2.0 * a);
		return entry >= 0.0 && entry < 1.0;
	}

	[[nodiscard]] float piece_alpha(const Vec3& world, float base_alpha, const RadialFade* radial) const noexcept {
		const SpatialReferenceConfig& cfg = *cfg_;
		float alpha = base_alpha * cfg.global_opacity;
		if (cfg.depth_fade_enabled) {
			const Vec3 delta{world[0] - view_->camera_position[0], world[1] - view_->camera_position[1], world[2] - view_->camera_position[2]};
			const double distance = std::max(norm(delta), 1.0e-9);
			alpha *= static_cast<float>(std::clamp(depth_reference_ / distance, static_cast<double>(cfg.depth_fade_minimum), 1.0));
		}
		if (radial != nullptr) {
			const Vec3 delta{world[0] - radial->center[0], world[1] - radial->center[1], world[2] - radial->center[2]};
			const double ratio = norm(delta) / std::max(radial->radius, 1.0e-9);
			alpha *= static_cast<float>(std::clamp(1.0 - ratio * ratio, 0.0, 1.0));
		}
		if (cfg.hide_behind_horizon && occluded_by_horizon(world)) {
			return 0.0f;
		}
		return alpha;
	}

	[[nodiscard]] bool segment_outside(const ImVec2& a, const ImVec2& b) const noexcept {
		const SpatialReferenceView& view = *view_;
		constexpr float margin = 64.0f;
		const float left = view.rect_min.x - margin;
		const float right = view.rect_min.x + view.rect_size.x + margin;
		const float top = view.rect_min.y - margin;
		const float bottom = view.rect_min.y + view.rect_size.y + margin;
		return (a.x < left && b.x < left) || (a.x > right && b.x > right) || (a.y < top && b.y < top) || (a.y > bottom && b.y > bottom);
	}

	void draw_segment(const Vec3& a, const Vec3& b, const std::array<float, 4>& color, float opacity, float thickness, uint32_t subdivisions, const RadialFade* radial) {
		Vec3 start = a;
		Vec3 end = b;
		if (clip_front_ && !clip_to_front(start, end)) {
			return;
		}
		const uint32_t pieces = std::max<uint32_t>(subdivisions, 1U);
		const float base_alpha = color[3] * opacity;
		Vec3 previous_world = start;
		Projected previous = project(start);
		for (uint32_t i = 1; i <= pieces; ++i) {
			const Vec3 current_world = lerp(start, end, static_cast<double>(i) / static_cast<double>(pieces));
			const Projected current = project(current_world);
			if (previous.visible && current.visible && !segment_outside(previous.screen, current.screen)) {
				const float alpha = piece_alpha(lerp(previous_world, current_world, 0.5), base_alpha, radial);
				if (alpha > 0.004f) {
					list_->AddLine(previous.screen, current.screen, make_color(color, alpha), thickness);
				}
			}
			previous_world = current_world;
			previous = current;
		}
	}

	[[nodiscard]] std::string format_number(double value) const {
		char buffer[48];
		if (cfg_->label_decimals >= 0) {
			std::snprintf(buffer, sizeof(buffer), "%.*f", std::min(cfg_->label_decimals, 9), value);
			return buffer;
		}
		const double magnitude = std::abs(value);
		if (magnitude < 1.0e-12) {
			return "0";
		}
		if (magnitude >= 1.0e6 || magnitude < 1.0e-3) {
			std::snprintf(buffer, sizeof(buffer), "%.3g", value);
			return buffer;
		}
		const int decimals = std::clamp(3 - static_cast<int>(std::floor(std::log10(magnitude))), 0, 6);
		std::snprintf(buffer, sizeof(buffer), "%.*f", decimals, value);
		std::string text(buffer);
		if (text.find('.') != std::string::npos) {
			while (!text.empty() && text.back() == '0') {
				text.pop_back();
			}
			if (!text.empty() && text.back() == '.') {
				text.pop_back();
			}
		}
		return text;
	}

	[[nodiscard]] std::string format_length(double simulation_length) const {
		std::string text = format_number(simulation_length * length_scale_ / unit_meters_);
		if (cfg_->label_unit_suffix) {
			text += " ";
			text += std::string(Units::distance_unit_suffix(view_->distance_unit));
		}
		return text;
	}

	void draw_text(const ImVec2& anchor, const std::string& text, const std::array<float, 4>& color, float alpha) {
		if (alpha <= 0.004f || text.empty()) {
			return;
		}
		const float scale = cfg_->label_scale;
		const ImVec2 base = ImGui::CalcTextSize(text.c_str());
		const ImVec2 size(base.x * scale, base.y * scale);
		const ImVec2 origin(anchor.x - size.x * 0.5f, anchor.y - size.y * 0.5f);
		if (cfg_->label_background) {
			const int background_alpha = static_cast<int>(std::clamp(alpha * cfg_->label_background_opacity, 0.0f, 1.0f) * 255.0f);
			list_->AddRectFilled(ImVec2(origin.x - 3.0f, origin.y - 1.0f), ImVec2(origin.x + size.x + 3.0f, origin.y + size.y + 1.0f), IM_COL32(8, 10, 18, background_alpha), 3.0f);
		}
		list_->AddText(nullptr, ImGui::GetFontSize() * scale, origin, make_color(color, alpha), text.c_str());
	}

	void draw_grids() {
		const SpatialReferenceConfig& cfg = *cfg_;
		static constexpr std::array<std::array<size_t, 3>, 3> kPlaneAxes{{{0, 1, 2}, {0, 2, 1}, {1, 2, 0}}};
		const int64_t interval = static_cast<int64_t>(std::max<uint32_t>(cfg.grid_major_interval, 1U));
		for (size_t plane = 0; plane < 3; ++plane) {
			if (!cfg.grid_planes[plane]) {
				continue;
			}
			const size_t u_axis = kPlaneAxes[plane][0];
			const size_t v_axis = kPlaneAxes[plane][1];
			const size_t n_axis = kPlaneAxes[plane][2];
			Vec3 origin = center_;
			origin[n_axis] += cfg.grid_plane_offsets[plane];
			Vec3 u{0.0, 0.0, 0.0};
			Vec3 v{0.0, 0.0, 0.0};
			u[u_axis] = 1.0;
			v[v_axis] = 1.0;
			const RadialFade fade{origin, grid_extent_};
			const RadialFade* radial = cfg.grid_radial_fade ? &fade : nullptr;
			const uint32_t subdivisions = line_subdivisions(radial != nullptr);
			for (int64_t j = -grid_lines_; j <= grid_lines_; ++j) {
				const SpatialLineStyle& style = (j % interval == 0) ? cfg.grid_major : cfg.grid_minor;
				if (!style.enabled) {
					continue;
				}
				const double position = static_cast<double>(j) * grid_spacing_;
				const Vec3 u_line_origin = offset(origin, v, position);
				draw_segment(offset(u_line_origin, u, -grid_extent_), offset(u_line_origin, u, grid_extent_), style.color, style.opacity, style.thickness, subdivisions, radial);
				const Vec3 v_line_origin = offset(origin, u, position);
				draw_segment(offset(v_line_origin, v, -grid_extent_), offset(v_line_origin, v, grid_extent_), style.color, style.opacity, style.thickness, subdivisions, radial);
			}
		}
	}

	void draw_rings() {
		const SpatialReferenceConfig& cfg = *cfg_;
		if (!cfg.rings_enabled || !cfg.ring_style.enabled) {
			return;
		}
		static constexpr std::array<std::array<size_t, 2>, 3> kPlaneAxes{{{0, 1}, {0, 2}, {1, 2}}};
		const size_t plane = static_cast<size_t>(cfg.ring_plane);
		Vec3 u{0.0, 0.0, 0.0};
		Vec3 v{0.0, 0.0, 0.0};
		u[kPlaneAxes[plane][0]] = 1.0;
		v[kPlaneAxes[plane][1]] = 1.0;
		constexpr int segments = 96;
		constexpr double two_pi = 6.283185307179586;
		for (uint32_t ring = 1; ring <= cfg.ring_count; ++ring) {
			const double radius = static_cast<double>(ring) * ring_spacing_;
			Vec3 previous = offset(center_, u, radius);
			for (int i = 1; i <= segments; ++i) {
				const double angle = two_pi * static_cast<double>(i) / static_cast<double>(segments);
				const Vec3 current = offset(offset(center_, u, radius * std::cos(angle)), v, radius * std::sin(angle));
				draw_segment(previous, current, cfg.ring_style.color, cfg.ring_style.opacity, cfg.ring_style.thickness, 1U, nullptr);
				previous = current;
			}
			if (cfg.ring_labels) {
				const Vec3 label_point = offset(center_, u, radius);
				const Projected projected = project(label_point);
				if (projected.visible) {
					const float alpha = piece_alpha(label_point, cfg.label_color[3], nullptr);
					draw_text(ImVec2(projected.screen.x, projected.screen.y - 10.0f * cfg.label_scale), format_length(radius), cfg.label_color, alpha);
				}
			}
		}
	}

	void draw_axis_ticks(size_t axis, const Vec3& direction, const SpatialLineStyle& style) {
		const SpatialReferenceConfig& cfg = *cfg_;
		const int64_t limit = std::min<int64_t>(static_cast<int64_t>(std::floor(axis_length_ / tick_spacing_)), static_cast<int64_t>(cfg.max_ticks_per_axis));
		if (limit < 1) {
			return;
		}
		const int64_t first = cfg.axes_show_negative ? -limit : 1;
		const double probe = tick_spacing_ * 0.05;
		const float half_length = cfg.tick_length_px * 0.5f;
		const int64_t label_interval = static_cast<int64_t>(std::max<uint32_t>(cfg.tick_label_interval, 1U));
		static_cast<void>(axis);
		for (int64_t k = first; k <= limit; ++k) {
			if (k == 0) {
				continue;
			}
			const Vec3 point = offset(center_, direction, static_cast<double>(k) * tick_spacing_);
			const Projected here = project(point);
			const Projected ahead = project(offset(point, direction, probe));
			if (!here.visible || !ahead.visible) {
				continue;
			}
			float dx = ahead.screen.x - here.screen.x;
			float dy = ahead.screen.y - here.screen.y;
			const float length = std::sqrt(dx * dx + dy * dy);
			if (length < 1.0e-4f) {
				continue;
			}
			dx /= length;
			dy /= length;
			float px = -dy;
			float py = dx;
			if (py < 0.0f) {
				px = -px;
				py = -py;
			}
			const float alpha = piece_alpha(point, style.color[3] * style.opacity, nullptr);
			if (alpha <= 0.004f) {
				continue;
			}
			list_->AddLine(ImVec2(here.screen.x - px * half_length, here.screen.y - py * half_length), ImVec2(here.screen.x + px * half_length, here.screen.y + py * half_length), make_color(style.color, alpha), cfg.tick_thickness_px);
			if (cfg.tick_labels && (k % label_interval == 0)) {
				const float label_offset = half_length + 9.0f * cfg.label_scale;
				draw_text(ImVec2(here.screen.x + px * label_offset, here.screen.y + py * label_offset), format_length(static_cast<double>(k) * tick_spacing_), cfg.label_color, alpha * cfg.label_color[3]);
			}
		}
	}

	void draw_arrowhead(const Vec3& tip_world, const Vec3& direction, const SpatialLineStyle& style) {
		const Projected tip = project(tip_world);
		const Projected behind = project(offset(tip_world, direction, -axis_length_ * 0.01));
		if (!tip.visible || !behind.visible) {
			return;
		}
		float dx = tip.screen.x - behind.screen.x;
		float dy = tip.screen.y - behind.screen.y;
		const float length = std::sqrt(dx * dx + dy * dy);
		if (length < 1.0e-4f) {
			return;
		}
		dx /= length;
		dy /= length;
		const float alpha = piece_alpha(tip_world, style.color[3] * style.opacity, nullptr);
		if (alpha <= 0.004f) {
			return;
		}
		const float size = cfg_->axis_arrow_size_px;
		const ImVec2 apex(tip.screen.x + dx * size * 0.5f, tip.screen.y + dy * size * 0.5f);
		const ImVec2 base(tip.screen.x - dx * size * 0.5f, tip.screen.y - dy * size * 0.5f);
		list_->AddTriangleFilled(apex, ImVec2(base.x - dy * size * 0.35f, base.y + dx * size * 0.35f), ImVec2(base.x + dy * size * 0.35f, base.y - dx * size * 0.35f), make_color(style.color, alpha));
	}

	void draw_axes() {
		const SpatialReferenceConfig& cfg = *cfg_;
		static constexpr std::array<const char*, 3> kNames{"X", "Y", "Z"};
		for (size_t axis = 0; axis < 3; ++axis) {
			const SpatialLineStyle& style = cfg.axes[axis];
			if (!style.enabled) {
				continue;
			}
			Vec3 direction{0.0, 0.0, 0.0};
			direction[axis] = 1.0;
			const Vec3 positive = offset(center_, direction, axis_length_);
			const Vec3 negative = cfg.axes_show_negative ? offset(center_, direction, -axis_length_) : center_;
			draw_segment(negative, positive, style.color, style.opacity, style.thickness, line_subdivisions(false), nullptr);
			if (cfg.ticks_enabled) {
				draw_axis_ticks(axis, direction, style);
			}
			if (cfg.axis_arrowheads) {
				draw_arrowhead(positive, direction, style);
			}
			if (cfg.axis_labels) {
				const Projected tip = project(positive);
				const Projected behind = project(offset(positive, direction, -axis_length_ * 0.01));
				if (tip.visible && behind.visible) {
					float dx = tip.screen.x - behind.screen.x;
					float dy = tip.screen.y - behind.screen.y;
					const float length = std::sqrt(dx * dx + dy * dy);
					if (length > 1.0e-4f) {
						dx /= length;
						dy /= length;
						const float distance = cfg.axis_arrow_size_px + 10.0f * cfg.label_scale;
						draw_text(ImVec2(tip.screen.x + dx * distance, tip.screen.y + dy * distance), kNames[axis], style.color, piece_alpha(positive, style.color[3], nullptr));
					}
				}
			}
		}
	}

	void draw_origin_marker() {
		const SpatialReferenceConfig& cfg = *cfg_;
		if (!cfg.origin_marker_enabled) {
			return;
		}
		const Projected projected = project(center_);
		if (!projected.visible) {
			return;
		}
		const float alpha = piece_alpha(center_, cfg.origin_marker_color[3], nullptr);
		if (alpha <= 0.004f) {
			return;
		}
		const ImU32 color = make_color(cfg.origin_marker_color, alpha);
		const ImVec2 c = projected.screen;
		const float s = cfg.origin_marker_size_px;
		switch (cfg.origin_marker_shape) {
			case SpatialMarkerShape::Dot:
				list_->AddCircleFilled(c, s * 0.5f, color, 20);
				break;
			case SpatialMarkerShape::Diamond:
				list_->AddQuadFilled(ImVec2(c.x, c.y - s), ImVec2(c.x + s, c.y), ImVec2(c.x, c.y + s), ImVec2(c.x - s, c.y), color);
				break;
			case SpatialMarkerShape::Ring:
				list_->AddCircle(c, s, color, 28, 1.8f);
				break;
			case SpatialMarkerShape::Cross:
			default:
				list_->AddLine(ImVec2(c.x - s, c.y), ImVec2(c.x + s, c.y), color, 1.8f);
				list_->AddLine(ImVec2(c.x, c.y - s), ImVec2(c.x, c.y + s), color, 1.8f);
				break;
		}
	}

	void draw_scale_bar() {
		const SpatialReferenceConfig& cfg = *cfg_;
		const SpatialReferenceView& view = *view_;
		if (!cfg.scale_bar_enabled) {
			return;
		}
		const Vec3 focus = offset(view.camera_position, view.forward, focus_distance_);
		const double probe = focus_distance_ * 0.05;
		const Projected a = project(focus);
		const Projected b = project(offset(focus, view.right, probe));
		if (!a.visible || !b.visible) {
			return;
		}
		const double dx = static_cast<double>(b.screen.x - a.screen.x);
		const double dy = static_cast<double>(b.screen.y - a.screen.y);
		const double pixels_per_unit = std::sqrt(dx * dx + dy * dy) / probe;
		if (!(pixels_per_unit > 1.0e-9) || !std::isfinite(pixels_per_unit)) {
			return;
		}
		const double length = resolve_spacing(SpatialSizeMode::Automatic, 0.0, static_cast<double>(cfg.scale_bar_target_px) / pixels_per_unit);
		const float bar_pixels = static_cast<float>(length * pixels_per_unit);
		if (bar_pixels < 4.0f || bar_pixels > view.rect_size.x * 0.9f) {
			return;
		}
		const float margin = cfg.scale_bar_margin_px;
		const bool left = (cfg.scale_bar_corner == SpatialCorner::TopLeft || cfg.scale_bar_corner == SpatialCorner::BottomLeft);
		const bool top = (cfg.scale_bar_corner == SpatialCorner::TopLeft || cfg.scale_bar_corner == SpatialCorner::TopRight);
		const float x0 = left ? (view.rect_min.x + margin) : (view.rect_min.x + view.rect_size.x - margin - bar_pixels);
		const float y = top ? (view.rect_min.y + margin + 14.0f) : (view.rect_min.y + view.rect_size.y - margin);
		const ImU32 color = make_color(cfg.scale_bar_color, cfg.scale_bar_color[3] * cfg.global_opacity);
		list_->AddLine(ImVec2(x0, y), ImVec2(x0 + bar_pixels, y), color, cfg.scale_bar_thickness_px);
		list_->AddLine(ImVec2(x0, y - 6.0f), ImVec2(x0, y + 6.0f), color, cfg.scale_bar_thickness_px);
		list_->AddLine(ImVec2(x0 + bar_pixels, y - 6.0f), ImVec2(x0 + bar_pixels, y + 6.0f), color, cfg.scale_bar_thickness_px);
		if (cfg.scale_bar_label) {
			draw_text(ImVec2(x0 + bar_pixels * 0.5f, y - 12.0f * cfg.label_scale), format_length(length), cfg.scale_bar_color, cfg.global_opacity * cfg.scale_bar_color[3]);
		}
	}

public:
	void render(ImDrawList* draw_list, const SpatialReferenceConfig& config, const SpatialReferenceView& view) {
		if (draw_list == nullptr || !config.enabled || view.rect_size.x <= 1.0f || view.rect_size.y <= 1.0f) {
			return;
		}
		list_ = draw_list;
		cfg_ = &config;
		view_ = &view;
		prepare();
		draw_grids();
		draw_rings();
		draw_axes();
		draw_origin_marker();
		draw_scale_bar();
		list_ = nullptr;
		cfg_ = nullptr;
		view_ = nullptr;
	}
};

}

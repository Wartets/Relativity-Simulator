#pragma once

#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/dynamics/pn_body.hpp"
#include <imgui.h>
#include <array>
#include <vector>
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
		double depth{0.0};
	};

	std::array<double, 3> tetrad_e1_{1.0, 0.0, 0.0};
	std::array<double, 3> tetrad_e2_{0.0, 1.0, 0.0};
	std::array<double, 3> tetrad_e3_{0.0, 0.0, 1.0};
	std::array<double, 3> camera_position_{0.0, 0.0, 0.0};
	double fov_rad_{1.0471975511965976};
	ImVec2 rect_min_{};
	ImVec2 rect_size_{};

	[[nodiscard]] ProjectedPoint project(const std::array<double, 3>& world_pos) const noexcept {
		const double dx = world_pos[0] - camera_position_[0];
		const double dy = world_pos[1] - camera_position_[1];
		const double dz = world_pos[2] - camera_position_[2];

		const double cam_x = dx * tetrad_e1_[0] + dy * tetrad_e1_[1] + dz * tetrad_e1_[2];
		const double cam_y = dx * tetrad_e2_[0] + dy * tetrad_e2_[1] + dz * tetrad_e2_[2];
		const double cam_z = dx * tetrad_e3_[0] + dy * tetrad_e3_[1] + dz * tetrad_e3_[2];

		ProjectedPoint pt;
		pt.depth = cam_x;
		if (cam_x <= 1e-6) {
			pt.visible = false;
			return pt;
		}

		const double tan_half_fov = std::tan(fov_rad_ * 0.5);
		const double aspect = static_cast<double>(rect_size_.x) / static_cast<double>(std::max(rect_size_.y, 1.0f));
		const double u = (cam_z / cam_x) / (tan_half_fov * aspect);
		const double v = -(cam_y / cam_x) / tan_half_fov;

		const double rect_width = static_cast<double>(rect_size_.x);
		const double rect_height = static_cast<double>(rect_size_.y);

		pt.screen.x = rect_min_.x + static_cast<float>((u * 0.5 + 0.5) * rect_width);
		pt.screen.y = rect_min_.y + static_cast<float>((v * 0.5 + 0.5) * rect_height);
		pt.visible = true;
		return pt;
	}

	[[nodiscard]] double angular_radius_to_pixels(double object_radius, double distance) const noexcept {
		if (object_radius <= 0.0 || distance <= 1e-9 || rect_size_.y <= 0.0f) {
			return 0.0;
		}

		const double half_fov = fov_rad_ * 0.5;
		if (!(half_fov > 0.0) || half_fov >= std::numbers::pi_v<double> * 0.5) {
			return 0.0;
		}

		const double tan_half_fov = std::tan(half_fov);
		if (!(tan_half_fov > 1e-12) || !std::isfinite(tan_half_fov)) {
			return 0.0;
		}

		const double angular = std::atan(object_radius / distance);
		return (angular / tan_half_fov) * (static_cast<double>(rect_size_.y) * 0.5);
	}

	void draw_grid_sphere(ImDrawList* draw_list, double radius) const {
		constexpr int LAT_LINES = 6;
		constexpr int LON_LINES = 12;
		constexpr int SEGMENTS = 48;
		const ImU32 minor_col = IM_COL32(35, 55, 75, 160);
		const ImU32 major_col = IM_COL32(60, 90, 120, 200);

		for (int lat = 1; lat < LAT_LINES; ++lat) {
			const double theta = std::numbers::pi_v<double> * static_cast<double>(lat) / static_cast<double>(LAT_LINES);
			const bool major = (lat == LAT_LINES / 2);
			std::vector<ImVec2> pts;
			pts.reserve(SEGMENTS + 1);
			bool any_visible = false;
			for (int i = 0; i <= SEGMENTS; ++i) {
				const double phi = 2.0 * std::numbers::pi_v<double> * static_cast<double>(i) / static_cast<double>(SEGMENTS);
				const std::array<double, 3> p{
					camera_position_[0] + radius * std::sin(theta) * std::cos(phi),
					camera_position_[1] + radius * std::sin(theta) * std::sin(phi),
					camera_position_[2] + radius * std::cos(theta)
				};
				const auto proj = project(p);
				if (proj.visible) {
					any_visible = true;
					pts.push_back(proj.screen);
				}
			}
			if (any_visible && pts.size() >= 2) {
				draw_list->AddPolyline(pts.data(), static_cast<int>(pts.size()), major ? major_col : minor_col, ImDrawFlags_None, major ? 1.6f : 1.0f);
			}
		}

		for (int lon = 0; lon < LON_LINES; ++lon) {
			const double phi = 2.0 * std::numbers::pi_v<double> * static_cast<double>(lon) / static_cast<double>(LON_LINES);
			std::vector<ImVec2> pts;
			pts.reserve(SEGMENTS + 1);
			bool any_visible = false;
			for (int i = 0; i <= SEGMENTS; ++i) {
				const double theta = std::numbers::pi_v<double> * static_cast<double>(i) / static_cast<double>(SEGMENTS);
				const std::array<double, 3> p{
					camera_position_[0] + radius * std::sin(theta) * std::cos(phi),
					camera_position_[1] + radius * std::sin(theta) * std::sin(phi),
					camera_position_[2] + radius * std::cos(theta)
				};
				const auto proj = project(p);
				if (proj.visible) {
					any_visible = true;
					pts.push_back(proj.screen);
				}
			}
			if (any_visible && pts.size() >= 2) {
				draw_list->AddPolyline(pts.data(), static_cast<int>(pts.size()), minor_col, ImDrawFlags_None, 1.0f);
			}
		}
	}

	void draw_orientation_gizmo(
		ImDrawList* draw_list,
		const std::array<double, 3>& center,
		const std::array<double, 3>& spin_vector,
		double body_radius
	) const {
		const double spin_mag = std::sqrt(spin_vector[0] * spin_vector[0] + spin_vector[1] * spin_vector[1] + spin_vector[2] * spin_vector[2]);
		std::array<double, 3> axis{0.0, 0.0, 1.0};
		if (spin_mag > 1e-12) {
			axis = {spin_vector[0] / spin_mag, spin_vector[1] / spin_mag, spin_vector[2] / spin_mag};
		}

		const double axis_length = body_radius * 2.2;
		const std::array<double, 3> tip{
			center[0] + axis[0] * axis_length,
			center[1] + axis[1] * axis_length,
			center[2] + axis[2] * axis_length
		};
		const std::array<double, 3> tail{
			center[0] - axis[0] * axis_length * 0.6,
			center[1] - axis[1] * axis_length * 0.6,
			center[2] - axis[2] * axis_length * 0.6
		};

		const auto p_center = project(center);
		const auto p_tip = project(tip);
		const auto p_tail = project(tail);
		if (!p_center.visible || !p_tip.visible) return;

		const ImU32 axis_col = IM_COL32(255, 200, 80, 230);
		if (p_tail.visible) {
			draw_list->AddLine(p_tail.screen, p_center.screen, axis_col, 1.5f);
		}
		draw_list->AddLine(p_center.screen, p_tip.screen, axis_col, 2.0f);

		const ImVec2 dir{p_tip.screen.x - p_center.screen.x, p_tip.screen.y - p_center.screen.y};
		const float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
		if (len > 1e-3f) {
			const ImVec2 n{dir.x / len, dir.y / len};
			const ImVec2 perp{-n.y, n.x};
			constexpr float head = 8.0f;
			const ImVec2 base{p_tip.screen.x - n.x * head, p_tip.screen.y - n.y * head};
			draw_list->AddTriangleFilled(
				p_tip.screen,
				ImVec2(base.x + perp.x * head * 0.5f, base.y + perp.y * head * 0.5f),
				ImVec2(base.x - perp.x * head * 0.5f, base.y - perp.y * head * 0.5f),
				axis_col
			);
		}

		if (spin_mag > 1e-12) {
			constexpr int ring_segments = 24;
			std::vector<ImVec2> ring;
			ring.reserve(ring_segments + 1);
			std::array<double, 3> ref{0.0, 1.0, 0.0};
			if (std::abs(axis[1]) > 0.9) ref = {1.0, 0.0, 0.0};
			std::array<double, 3> u_vec{
				ref[1] * axis[2] - ref[2] * axis[1],
				ref[2] * axis[0] - ref[0] * axis[2],
				ref[0] * axis[1] - ref[1] * axis[0]
			};
			const double u_len = std::sqrt(u_vec[0] * u_vec[0] + u_vec[1] * u_vec[1] + u_vec[2] * u_vec[2]);
			if (u_len > 1e-9) {
				u_vec = {u_vec[0] / u_len, u_vec[1] / u_len, u_vec[2] / u_len};
				const std::array<double, 3> v_vec{
					axis[1] * u_vec[2] - axis[2] * u_vec[1],
					axis[2] * u_vec[0] - axis[0] * u_vec[2],
					axis[0] * u_vec[1] - axis[1] * u_vec[0]
				};
				const double ring_radius = body_radius * 1.35;
				bool any_visible = false;
				for (int i = 0; i <= ring_segments; ++i) {
					const double a = 2.0 * std::numbers::pi_v<double> * static_cast<double>(i) / static_cast<double>(ring_segments);
					const std::array<double, 3> p{
						center[0] + ring_radius * (u_vec[0] * std::cos(a) + v_vec[0] * std::sin(a)),
						center[1] + ring_radius * (u_vec[1] * std::cos(a) + v_vec[1] * std::sin(a)),
						center[2] + ring_radius * (u_vec[2] * std::cos(a) + v_vec[2] * std::sin(a))
					};
					const auto proj = project(p);
					if (proj.visible) any_visible = true;
					ring.push_back(proj.screen);
				}
				if (any_visible) {
					draw_list->AddPolyline(ring.data(), static_cast<int>(ring.size()), IM_COL32(120, 200, 255, 200), ImDrawFlags_None, 1.4f);
				}
			}
		}
	}

	void draw_offscreen_indicator(
		ImDrawList* draw_list,
		const std::array<double, 3>& world_position,
		const std::string& label
	) const {
		if (rect_size_.x <= 0.0f || rect_size_.y <= 0.0f) {
			return;
		}

		const double dx = world_position[0] - camera_position_[0];
		const double dy = world_position[1] - camera_position_[1];
		const double dz = world_position[2] - camera_position_[2];

		const double cam_y = dx * tetrad_e2_[0] + dy * tetrad_e2_[1] + dz * tetrad_e2_[2];
		const double cam_z = dx * tetrad_e3_[0] + dy * tetrad_e3_[1] + dz * tetrad_e3_[2];

		const double bearing_x = cam_z;
		const double bearing_y = -cam_y;
		const double bearing_length = std::sqrt(
			bearing_x * bearing_x +
			bearing_y * bearing_y
		);

		ImVec2 dir{};
		if (bearing_length > 1e-12) {
			dir.x = static_cast<float>(bearing_x / bearing_length);
			dir.y = static_cast<float>(bearing_y / bearing_length);
		} else {
			dir = ImVec2(0.0f, -1.0f);
		}

		const ImVec2 center(
			rect_min_.x + rect_size_.x * 0.5f,
			rect_min_.y + rect_size_.y * 0.5f
		);

		constexpr float margin = 40.0f;
		const float half_w = std::max(rect_size_.x * 0.5f - margin, 1.0f);
		const float half_h = std::max(rect_size_.y * 0.5f - margin, 1.0f);

		float scale_x = std::numeric_limits<float>::max();
		float scale_y = std::numeric_limits<float>::max();

		if (std::abs(dir.x) > 1e-6f) {
			scale_x = half_w / std::abs(dir.x);
		}

		if (std::abs(dir.y) > 1e-6f) {
			scale_y = half_h / std::abs(dir.y);
		}

		const float scale = std::min(scale_x, scale_y);
		if (!std::isfinite(scale)) {
			return;
		}

		const ImVec2 edge_point(
			center.x + dir.x * scale,
			center.y + dir.y * scale
		);

		const ImVec2 perp{-dir.y, dir.x};
		constexpr float arrow_size = 9.0f;

		const ImVec2 tip(
			edge_point.x + dir.x * arrow_size,
			edge_point.y + dir.y * arrow_size
		);

		const ImVec2 base_a(
			edge_point.x - dir.x * arrow_size + perp.x * arrow_size * 0.6f,
			edge_point.y - dir.y * arrow_size + perp.y * arrow_size * 0.6f
		);

		const ImVec2 base_b(
			edge_point.x - dir.x * arrow_size - perp.x * arrow_size * 0.6f,
			edge_point.y - dir.y * arrow_size - perp.y * arrow_size * 0.6f
		);

		draw_list->AddTriangleFilled(
			tip,
			base_a,
			base_b,
			IM_COL32(255, 210, 90, 220)
		);

		const ImVec2 text_position(
			edge_point.x + dir.x * 14.0f,
			edge_point.y + dir.y * 14.0f - 6.0f
		);

		draw_list->AddText(
			text_position,
			IM_COL32(255, 220, 150, 220),
			label.c_str()
		);
	}

public:
	void configure(
		const Orchestrator::CameraState& cam,
		double fov_rad,
		const ImVec2& rect_min,
		const ImVec2& rect_size
	) noexcept {
		camera_position_ = cam.position;
		fov_rad_ = fov_rad;
		rect_min_ = rect_min;
		rect_size_ = ImVec2(
			std::max(rect_size.x, 0.0f),
			std::max(rect_size.y, 0.0f)
		);

		constexpr double min_fov = 1e-6;
		constexpr double max_fov = std::numbers::pi_v<double> - 1e-6;
		fov_rad_ = std::clamp(fov_rad, min_fov, max_fov);

		const double pitch_r = cam.pitch * (std::numbers::pi / 180.0);
		const double yaw_r = cam.yaw * (std::numbers::pi / 180.0);
		const double roll_r = cam.roll * (std::numbers::pi / 180.0);
		const double cp = std::cos(pitch_r), sp = std::sin(pitch_r);
		const double cy = std::cos(yaw_r), sy = std::sin(yaw_r);
		const double cr = std::cos(roll_r), sr = std::sin(roll_r);

		tetrad_e1_ = {cp * cy, cp * sy, sp};
		tetrad_e2_ = {cr * (-sy) + sr * (-sp * cy), cr * cy + sr * (-sp * sy), sr * cp};
		tetrad_e3_ = {-sr * (-sy) + cr * (-sp * cy), -sr * cy + cr * (-sp * sy), cr * cp};
	}

	void render(
		ImDrawList* draw_list,
		const Orchestrator::SimulationOrchestrator<1024>& orchestrator
	) const {
		draw_list->AddRectFilled(rect_min_, ImVec2(rect_min_.x + rect_size_.x, rect_min_.y + rect_size_.y), IM_COL32(4, 4, 8, 255));

		const auto& params = orchestrator.parameters();
		const double central_radius = std::max(2.0 * params.mass, 1e-3);
		draw_grid_sphere(draw_list, std::max(central_radius * 40.0, 40.0));

		const auto central_dist = std::sqrt(
			camera_position_[0] * camera_position_[0] +
			camera_position_[1] * camera_position_[1] +
			camera_position_[2] * camera_position_[2]
		);
		const auto p_center = project({0.0, 0.0, 0.0});
		if (p_center.visible) {
			const double px_radius = std::max(angular_radius_to_pixels(central_radius, central_dist), 2.0);
			draw_list->AddCircleFilled(p_center.screen, static_cast<float>(px_radius), IM_COL32(230, 230, 245, 255), 48);
			draw_orientation_gizmo(draw_list, {0.0, 0.0, 0.0}, {0.0, 0.0, params.spin}, central_radius);
		} else {
			draw_offscreen_indicator(draw_list, {0.0, 0.0, 0.0}, "Central Object");
		}

		const auto& sys = orchestrator.nbody_system();
		for (const auto& body : sys.bodies()) {
			const double dx = body.position[0] - camera_position_[0];
			const double dy = body.position[1] - camera_position_[1];
			const double dz = body.position[2] - camera_position_[2];
			const double dist = std::sqrt(dx * dx + dy * dy + dz * dz);
			const auto proj = project(body.position);
			if (proj.visible) {
				const double px_radius = std::max(angular_radius_to_pixels(std::max(body.radius, 1e-4), dist), 2.0);
				draw_list->AddCircleFilled(proj.screen, static_cast<float>(px_radius), IM_COL32(140, 190, 255, 255), 32);
				draw_list->AddCircle(proj.screen, static_cast<float>(px_radius), IM_COL32(20, 30, 50, 255), 32, 1.2f);
				draw_orientation_gizmo(draw_list, body.position, body.spin, std::max(body.radius, 1e-4));
				draw_list->AddText(ImVec2(proj.screen.x + static_cast<float>(px_radius) + 4.0f, proj.screen.y - 8.0f), IM_COL32(220, 230, 255, 220), ("#" + std::to_string(body.id)).c_str());
			} else {
				draw_offscreen_indicator(draw_list, body.position, "#" + std::to_string(body.id));
			}
		}
	}
};

}

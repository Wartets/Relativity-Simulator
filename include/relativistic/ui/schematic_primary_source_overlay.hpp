#pragma once

#include "relativistic/observer/camera_collision.hpp"
#include "relativistic/observer/camera_projections.hpp"
#include "relativistic/observer/direction_projection.hpp"
#include "relativistic/optics/cie_observer.hpp"
#include "relativistic/optics/disk_thermal_profile.hpp"
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/render/gpu_types.hpp"
#include "relativistic/ui/schematic_view_config.hpp"
#include "relativistic/units/unit_system.hpp"
#include <imgui.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Relativistic::UI {

class SchematicPrimarySourceOverlay {
public:
	using OrchestratorType = Orchestrator::SimulationOrchestrator<1024>;

	struct ViewRegion {
		ImVec2 origin;
		ImVec2 size;
		Observer::ProjectionMode projection;
		double field_of_view_rad;
	};

	static void draw(ImDrawList* draw_list, const OrchestratorType& orchestrator, const SchematicViewConfig& config, const ViewRegion& region) {
		const auto& parameters = orchestrator.parameters();
		const double horizon = Observer::CameraCollisionField::primary_horizon_radius(parameters, orchestrator.active_metric_name());
		if (draw_list == nullptr || horizon <= 0.0 || region.size.x < 1.0f || region.size.y < 1.0f) {
			return;
		}

		const Projector projector(orchestrator.camera(), region);
		const auto& prefs = orchestrator.unit_preferences();
		const double length_scale = orchestrator.constants_engine().length_scale();
		const double mass = std::max(parameters.mass, 1e-4);
		const std::string& metric_name = orchestrator.active_metric_name();

		const auto& outline = config.central_object_style.outline;
		const ImU32 outline_color = outline.enabled
			? ImGui::ColorConvertFloat4ToU32(ImVec4(outline.color[0], outline.color[1], outline.color[2], outline.color[3]))
			: IM_COL32(242, 204, 128, 255);
		const float outline_thickness = outline.enabled ? std::max(outline.thickness, 1.0f) : 2.0f;

		draw_horizon(draw_list, projector, horizon, outline_color, outline_thickness);

		bool disk_drawn = false;
		double inner_radius = 0.0;
		double outer_radius = 0.0;
		double peak_temperature = 0.0;
		if (metric_hosts_disk(metric_name)) {
			Render::GpuCameraPushConstants constants{};
			orchestrator.apply_primary_disk_constants(constants);
			const Render::GpuDiskProfile& disk = constants.primary_disk;
			if (disk.enabled > 0.5f) {
				const double spin = metric_has_spin(metric_name) ? std::clamp(parameters.spin, -0.999 * mass, 0.999 * mass) : 0.0;
				const double isco = Optics::DiskThermalProfile::kerr_isco_radius(mass, spin);
				inner_radius = isco * static_cast<double>(std::max(disk.inner_radius_scale, 1.0f));
				outer_radius = std::max(static_cast<double>(disk.outer_radius_mass_units) * mass, inner_radius * 1.05);
				peak_temperature = static_cast<double>(disk.peak_temperature_k);
				draw_disk_rings(draw_list, projector, disk, horizon, inner_radius, outer_radius);
				disk_drawn = true;
			}
		}

		if (!config.show_tags) {
			return;
		}
		const ImU32 text_color = IM_COL32(235, 240, 250, 255);
		char buffer[192];
		std::snprintf(buffer, sizeof(buffer), "Event horizon r+ = %s (%.3f M)", Units::format_distance(horizon * length_scale, prefs.distance).c_str(), horizon / mass);
		draw_label(draw_list, projector, 0.0, 0.0, horizon, 0.7, buffer, text_color);
		if (disk_drawn) {
			std::snprintf(buffer, sizeof(buffer), "Disk inner edge = %s (%.3f M)", Units::format_distance(inner_radius * length_scale, prefs.distance).c_str(), inner_radius / mass);
			draw_label(draw_list, projector, horizon, inner_radius, inner_radius, 2.3, buffer, text_color);
			std::snprintf(buffer, sizeof(buffer), "Disk outer edge = %s (%.3f M)", Units::format_distance(outer_radius * length_scale, prefs.distance).c_str(), outer_radius / mass);
			draw_label(draw_list, projector, horizon, outer_radius, outer_radius, 3.9, buffer, text_color);
			std::snprintf(buffer, sizeof(buffer), "Disk peak temperature = %s", Units::format_temperature(peak_temperature, prefs.temperature).c_str());
			draw_label(draw_list, projector, horizon, std::sqrt(inner_radius * outer_radius), std::sqrt(inner_radius * outer_radius), 5.4, buffer, text_color);
		}
	}

private:
	using Vec3 = std::array<double, 3>;

	static constexpr int kRingSegments = 160;
	static constexpr int kDiskRingCount = 18;

	class Projector {
	public:
		Projector(const Orchestrator::CameraState& camera, const ViewRegion& region)
			: position_(camera.position), region_(region) {
			const auto basis = camera.orientation_basis();
			forward_ = basis.forward;
			right_ = basis.right;
			up_ = basis.up;
			aspect_ = static_cast<double>(region.size.x) / std::max(static_cast<double>(region.size.y), 1.0);
			all_sky_ = (region.projection == Observer::ProjectionMode::Equirectangular360 || region.projection == Observer::ProjectionMode::HammerAitoff);
		}

		[[nodiscard]] const Vec3& camera_position() const noexcept {
			return position_;
		}

		[[nodiscard]] std::optional<ImVec2> project(const Vec3& point) const noexcept {
			const Vec3 delta{point[0] - position_[0], point[1] - position_[1], point[2] - position_[2]};
			const auto uv = Observer::direction_to_screen_uv(region_.projection, dot(delta, forward_), dot(delta, right_), dot(delta, up_), region_.field_of_view_rad);
			if (!uv.has_value()) {
				return std::nullopt;
			}
			const double u = all_sky_ ? uv->first : (uv->first / aspect_);
			if (!std::isfinite(u) || !std::isfinite(uv->second)) {
				return std::nullopt;
			}
			return ImVec2(
				region_.origin.x + static_cast<float>((u * 0.5 + 0.5) * static_cast<double>(region_.size.x)),
				region_.origin.y + static_cast<float>((uv->second * 0.5 + 0.5) * static_cast<double>(region_.size.y))
			);
		}

	private:
		Vec3 position_;
		Vec3 forward_{};
		Vec3 right_{};
		Vec3 up_{};
		ViewRegion region_;
		double aspect_{1.0};
		bool all_sky_{false};
	};

	[[nodiscard]] static double dot(const Vec3& a, const Vec3& b) noexcept {
		return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
	}

	[[nodiscard]] static Vec3 cross(const Vec3& a, const Vec3& b) noexcept {
		return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
	}

	[[nodiscard]] static bool contains(std::string_view text, std::string_view token) noexcept {
		return text.find(token) != std::string_view::npos;
	}

	[[nodiscard]] static bool metric_hosts_disk(std::string_view name) noexcept {
		if (contains(name, "Minkowski") || contains(name, "FLRW") || contains(name, "Morris") || contains(name, "Wormhole") || contains(name, "Alcubierre") || contains(name, "Warp") || contains(name, "BSSN")) {
			return false;
		}
		if (contains(name, "de Sitter")) {
			return false;
		}
		return contains(name, "Schwarzschild") || contains(name, "Kerr") || contains(name, "Reissner");
	}

	[[nodiscard]] static bool metric_has_spin(std::string_view name) noexcept {
		return contains(name, "Kerr");
	}

	[[nodiscard]] static bool hidden_by_horizon(const Vec3& from, const Vec3& to, double radius) noexcept {
		const Vec3 d{to[0] - from[0], to[1] - from[1], to[2] - from[2]};
		const double a = dot(d, d);
		if (a <= 1e-30) {
			return false;
		}
		const double c = dot(from, from) - radius * radius;
		if (c <= 0.0) {
			return false;
		}
		const double b = dot(from, d);
		const double discriminant = b * b - a * c;
		if (discriminant < 0.0) {
			return false;
		}
		const double t = (-b - std::sqrt(discriminant)) / a;
		return t > 0.0 && t < 1.0;
	}

	[[nodiscard]] static ImU32 temperature_color(double kelvin, int alpha) noexcept {
		const double temperature = std::clamp(kelvin, 800.0, 60000.0);
		constexpr double second_radiation_constant = 0.0143877688;
		constexpr int samples = 32;
		Optics::ColorXYZ xyz{};
		for (int i = 0; i <= samples; ++i) {
			const double lambda_nm = 380.0 + static_cast<double>(i) * (400.0 / static_cast<double>(samples));
			const double lambda_m = lambda_nm * 1e-9;
			const double exponent = second_radiation_constant / (lambda_m * temperature);
			if (exponent > 700.0) {
				continue;
			}
			const double lambda_um = lambda_nm * 1e-3;
			const double radiance = 1.0 / (std::pow(lambda_um, 5.0) * std::expm1(exponent));
			const double weight = (i == 0 || i == samples) ? 0.5 : 1.0;
			xyz.x += weight * radiance * Optics::CIE1931Observer::x_bar(lambda_nm);
			xyz.y += weight * radiance * Optics::CIE1931Observer::y_bar(lambda_nm);
			xyz.z += weight * radiance * Optics::CIE1931Observer::z_bar(lambda_nm);
		}
		Optics::ColorRGB linear = Optics::CIE1931Observer::xyz_to_linear_srgb(xyz);
		const double peak = std::max({linear.r, linear.g, linear.b, 1e-30});
		linear = Optics::ColorRGB{linear.r / peak, linear.g / peak, linear.b / peak};
		const Optics::ColorRGB srgb = Optics::CIE1931Observer::linear_to_srgb(linear);
		const auto channel = [](double value) noexcept {
			return static_cast<int>(std::clamp(0.2 + 0.8 * value, 0.0, 1.0) * 255.0 + 0.5);
		};
		return IM_COL32(channel(srgb.r), channel(srgb.g), channel(srgb.b), alpha);
	}

	static void draw_horizon(ImDrawList* draw_list, const Projector& projector, double radius, ImU32 outline_color, float thickness) {
		const Vec3& camera = projector.camera_position();
		const double distance = std::sqrt(dot(camera, camera));
		if (distance <= radius * 1.0001) {
			return;
		}
		const Vec3 normal{camera[0] / distance, camera[1] / distance, camera[2] / distance};
		const Vec3 helper = (std::abs(normal[2]) < 0.9) ? Vec3{0.0, 0.0, 1.0} : Vec3{1.0, 0.0, 0.0};
		Vec3 e1 = cross(normal, helper);
		const double e1_length = std::sqrt(dot(e1, e1));
		e1 = {e1[0] / e1_length, e1[1] / e1_length, e1[2] / e1_length};
		const Vec3 e2 = cross(normal, e1);
		const double offset = radius * radius / distance;
		const double silhouette_radius = radius * std::sqrt(std::max(1.0 - (radius / distance) * (radius / distance), 0.0));

		std::vector<ImVec2> points;
		points.reserve(kRingSegments);
		bool complete = true;
		for (int i = 0; i < kRingSegments; ++i) {
			const double angle = 2.0 * 3.14159265358979323846 * static_cast<double>(i) / static_cast<double>(kRingSegments);
			const double c = std::cos(angle) * silhouette_radius;
			const double s = std::sin(angle) * silhouette_radius;
			const Vec3 point{
				normal[0] * offset + e1[0] * c + e2[0] * s,
				normal[1] * offset + e1[1] * c + e2[1] * s,
				normal[2] * offset + e1[2] * c + e2[2] * s
			};
			const auto projected = projector.project(point);
			if (!projected.has_value()) {
				complete = false;
				break;
			}
			points.push_back(*projected);
		}
		if (!complete || points.size() < 3) {
			return;
		}
		draw_list->AddConvexPolyFilled(points.data(), static_cast<int>(points.size()), IM_COL32(0, 0, 0, 238));
		draw_list->AddPolyline(points.data(), static_cast<int>(points.size()), outline_color, ImDrawFlags_Closed, thickness);
	}

	static void draw_ring(ImDrawList* draw_list, const Projector& projector, double radius, double horizon, ImU32 color, float thickness) {
		const Vec3& camera = projector.camera_position();
		std::vector<ImVec2> run;
		run.reserve(kRingSegments + 1);
		const auto flush = [&]() {
			if (run.size() >= 2) {
				draw_list->AddPolyline(run.data(), static_cast<int>(run.size()), color, ImDrawFlags_None, thickness);
			}
			run.clear();
		};
		for (int i = 0; i <= kRingSegments; ++i) {
			const double angle = 2.0 * 3.14159265358979323846 * static_cast<double>(i) / static_cast<double>(kRingSegments);
			const Vec3 point{radius * std::cos(angle), radius * std::sin(angle), 0.0};
			const auto projected = projector.project(point);
			if (projected.has_value() && !hidden_by_horizon(camera, point, horizon)) {
				run.push_back(*projected);
			} else {
				flush();
			}
		}
		flush();
	}

	static void draw_disk_rings(ImDrawList* draw_list, const Projector& projector, const Render::GpuDiskProfile& disk, double horizon, double inner, double outer) {
		const double ratio = outer / inner;
		for (int k = 0; k < kDiskRingCount; ++k) {
			const double t = static_cast<double>(k) / static_cast<double>(kDiskRingCount - 1);
			const double radius = inner * std::pow(ratio, t);
			const bool boundary = (k == 0 || k == kDiskRingCount - 1);
			draw_ring(draw_list, projector, radius, horizon, temperature_color(local_temperature(disk, inner, radius), boundary ? 255 : 150), boundary ? 2.4f : 1.5f);
		}
	}

	[[nodiscard]] static double local_temperature(const Render::GpuDiskProfile& disk, double inner, double radius) noexcept {
		const double u = std::clamp(inner / std::max(radius, 1e-9), 1e-4, 1.0);
		const double boundary = std::max(1.0 - static_cast<double>(disk.zero_torque_strength) * std::sqrt(u), 0.0);
		const double base = std::pow(u, static_cast<double>(disk.temperature_exponent)) * std::pow(boundary, 0.25) * static_cast<double>(disk.temperature_normalization);
		const double span = std::max(static_cast<double>(disk.peak_temperature_k) - static_cast<double>(disk.floor_temperature_k), 0.0);
		return static_cast<double>(disk.floor_temperature_k) + span * base;
	}

	static void draw_label(ImDrawList* draw_list, const Projector& projector, double, double radius_hint, double radius, double angle, const char* text, ImU32 color) {
		const double r = (radius_hint > 0.0) ? radius : radius;
		const Vec3 anchor{r * std::cos(angle), r * std::sin(angle), 0.0};
		const auto projected = projector.project(anchor);
		if (!projected.has_value()) {
			return;
		}
		const ImVec2 text_size = ImGui::CalcTextSize(text);
		const ImVec2 position(projected->x + 6.0f, projected->y - text_size.y * 0.5f);
		draw_list->AddRectFilled(ImVec2(position.x - 3.0f, position.y - 1.0f), ImVec2(position.x + text_size.x + 3.0f, position.y + text_size.y + 1.0f), IM_COL32(8, 10, 16, 190), 3.0f);
		draw_list->AddText(position, color, text);
	}
};

}

#pragma once

#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/render/gpu_types.hpp"
#include "relativistic/render/software_compute_engine.hpp"
#include "relativistic/ui/tooltip_utils.hpp"
#include <imgui.h>
#if defined(__APPLE__)
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif
#include <vector>
#include <string>
#include <cmath>
#include <numbers>
#include <algorithm>
#include <cstdint>

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

#ifndef GL_RGBA32F
#define GL_RGBA32F 0x8814
#endif

namespace Relativistic::UI {

class SecondaryViewWindow {
private:
	std::string name_;
	bool is_open_{false};
	Orchestrator::SimulationOrchestrator<1024>* orchestrator_{nullptr};

	uint32_t gl_texture_id_{0};
	uint32_t allocated_texture_w_{0};
	uint32_t allocated_texture_h_{0};
	std::vector<float> color_upload_buffer_{};

	double radius_{60.0};
	double theta_{1.5707963267948966};
	double phi_{0.0};
	double fov_deg_{55.0};
	double exposure_{0.0};
	int tonemapping_mode_{1};
	int projection_mode_{0};
	int max_steps_{768};
	float resolution_scale_{0.5f};
	float external_budget_scale_{1.0f};
	bool auto_refresh_{true};
	bool follow_primary_camera_{false};
	double follow_offset_theta_{0.35};
	double follow_offset_phi_{1.5707963267948966};

	Render::GpuCameraPushConstants last_consts_{};
	bool has_rendered_once_{false};

	[[nodiscard]] static uint32_t resolve_metric_id(std::string_view name) noexcept {
		if (name.find("Minkowski") != std::string_view::npos) return 0;
		if (name.find("Schwarzschild") != std::string_view::npos && name.find("de Sitter") != std::string_view::npos) return 6;
		if (name.find("Schwarzschild") != std::string_view::npos) return 1;
		if (name.find("Kerr-Newman") != std::string_view::npos) return 5;
		if (name.find("Kerr") != std::string_view::npos) return 2;
		if (name.find("Reissner") != std::string_view::npos) return 4;
		if (name.find("FLRW") != std::string_view::npos) return 7;
		if (name.find("Morris") != std::string_view::npos || name.find("Wormhole") != std::string_view::npos) return 8;
		if (name.find("Alcubierre") != std::string_view::npos || name.find("Warp") != std::string_view::npos) return 9;
		return 1;
	}

	void ensure_texture() noexcept {
		if (gl_texture_id_ == 0) {
			glGenTextures(1, &gl_texture_id_);
			glBindTexture(GL_TEXTURE_2D, gl_texture_id_);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}
	}

	[[nodiscard]] Render::GpuCameraPushConstants build_push_constants(uint32_t width, uint32_t height) const noexcept {
		const auto& params = orchestrator_->parameters();

		double eff_theta = theta_;
		double eff_phi = phi_;
		if (follow_primary_camera_) {
			const auto& main_cam = orchestrator_->camera();
			eff_theta = std::clamp(main_cam.theta + follow_offset_theta_, 0.02, std::numbers::pi_v<double> - 0.02);
			eff_phi = main_cam.phi + follow_offset_phi_;
		}

		const double x = radius_ * std::sin(eff_theta) * std::cos(eff_phi);
		const double y = radius_ * std::sin(eff_theta) * std::sin(eff_phi);
		const double z = radius_ * std::cos(eff_theta);

		const double dist = std::max(radius_, 1e-6);
		const double yaw_rad = std::atan2(-y, -x);
		const double pitch_rad = std::asin(std::clamp(-z / dist, -0.9999, 0.9999));

		const double cp = std::cos(pitch_rad), sp = std::sin(pitch_rad);
		const double cy = std::cos(yaw_rad), sy = std::sin(yaw_rad);

		Render::GpuCameraPushConstants consts{};
		consts.screen_width = width;
		consts.screen_height = height;
		consts.field_of_view_rad = fov_deg_ * (std::numbers::pi / 180.0);
		consts.metric_type = resolve_metric_id(orchestrator_->active_metric_name());
		consts.metric_mass = params.mass;
		consts.metric_spin = params.spin;
		consts.metric_charge = params.charge;
		consts.cosmological_lambda = params.cosmological_lambda;
		consts.wormhole_throat = params.wormhole_throat;
		consts.warp_velocity = params.warp_velocity;
		consts.camera_exposure = exposure_;
		consts.tonemapping_mode = static_cast<uint32_t>(tonemapping_mode_);
		consts.horizon_radius = 2.0 * params.mass;
		consts.escape_radius = std::max(radius_ * 4.0, 50.0);
		consts.projection_mode = static_cast<uint32_t>(projection_mode_);
		consts.max_integration_steps = static_cast<uint32_t>(std::clamp(max_steps_, 32, 8192));
		consts.render_flags = params.visual_overlays_flags;
		consts.space_skip_radius_scale = params.space_skip_radius_scale;
		consts.pole_guard_precision_scale = params.pole_guard_precision_scale;
		consts.sky_rotation_rad = params.sky_rotation_deg * (std::numbers::pi / 180.0);
		consts.sky_hue_shift_rad = params.sky_hue_shift_deg * (std::numbers::pi / 180.0);
		consts.sky_saturation = params.sky_saturation;
		consts.sky_star_density = params.sky_star_density;
		consts.sky_star_brightness = params.sky_star_brightness;
		consts.sky_nebula_intensity = params.sky_nebula_intensity;
		consts.sky_grid_opacity = params.sky_grid_opacity;
		consts.sky_background_r = params.sky_background_r;
		consts.sky_background_g = params.sky_background_g;
		consts.sky_background_b = params.sky_background_b;
		consts.observer_position = {0.0, radius_, eff_theta, eff_phi};
		consts.tetrad_e0 = {1.0, 0.0, 0.0, 0.0};
		consts.tetrad_e1 = {0.0, cp * cy, cp * sy, sp};
		consts.tetrad_e2 = {0.0, -sy, cy, 0.0};
		consts.tetrad_e3 = {0.0, -sp * cy, -sp * sy, cp};
		return consts;
	}

	void render_frame(const ImVec2& avail) {
		ensure_texture();

		const float effective_scale = std::clamp(resolution_scale_ * external_budget_scale_, 0.05f, 2.0f);
		const uint32_t width = std::clamp(static_cast<uint32_t>(avail.x * effective_scale), 32u, 1920u);
		const uint32_t height = std::clamp(static_cast<uint32_t>(avail.y * effective_scale), 32u, 1080u);
		if (width == 0 || height == 0) {
			return;
		}

		const auto consts = build_push_constants(width, height);
		if (has_rendered_once_ && !auto_refresh_ && consts == last_consts_) {
			return;
		}

		const size_t pixel_count = static_cast<size_t>(width) * static_cast<size_t>(height);
		std::vector<Render::GpuPixelOutput> fb(pixel_count, Render::GpuPixelOutput{});
		Render::SoftwareComputeEngine::dispatch_fp64(consts, fb, nullptr, nullptr);

		if (color_upload_buffer_.size() < pixel_count * 4) {
			color_upload_buffer_.assign(pixel_count * 4, 0.0f);
		}
		for (size_t i = 0; i < pixel_count; ++i) {
			color_upload_buffer_[i * 4 + 0] = fb[i].r;
			color_upload_buffer_[i * 4 + 1] = fb[i].g;
			color_upload_buffer_[i * 4 + 2] = fb[i].b;
			color_upload_buffer_[i * 4 + 3] = fb[i].a;
		}

		glBindTexture(GL_TEXTURE_2D, gl_texture_id_);
		if (width != allocated_texture_w_ || height != allocated_texture_h_) {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, static_cast<GLsizei>(width), static_cast<GLsizei>(height), 0, GL_RGBA, GL_FLOAT, color_upload_buffer_.data());
			allocated_texture_w_ = width;
			allocated_texture_h_ = height;
		} else {
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height), GL_RGBA, GL_FLOAT, color_upload_buffer_.data());
		}

		last_consts_ = consts;
		has_rendered_once_ = true;
	}

public:
	SecondaryViewWindow(std::string name, Orchestrator::SimulationOrchestrator<1024>& orchestrator)
		: name_(std::move(name)), orchestrator_(&orchestrator) {}

	~SecondaryViewWindow() {
		if (gl_texture_id_ != 0) {
			glDeleteTextures(1, &gl_texture_id_);
			gl_texture_id_ = 0;
		}
	}

	SecondaryViewWindow(const SecondaryViewWindow&) = delete;
	SecondaryViewWindow& operator=(const SecondaryViewWindow&) = delete;
	SecondaryViewWindow(SecondaryViewWindow&&) = default;
	SecondaryViewWindow& operator=(SecondaryViewWindow&&) = default;

	[[nodiscard]] bool& open_state() noexcept {
		return is_open_;
	}

	[[nodiscard]] const std::string& name() const noexcept {
		return name_;
	}

	[[nodiscard]] double radius() const noexcept { return radius_; }
	[[nodiscard]] double theta() const noexcept { return theta_; }
	[[nodiscard]] double phi() const noexcept { return phi_; }
	[[nodiscard]] double fov_deg() const noexcept { return fov_deg_; }
	[[nodiscard]] double exposure() const noexcept { return exposure_; }
	[[nodiscard]] int tonemapping_mode() const noexcept { return tonemapping_mode_; }
	[[nodiscard]] int projection_mode() const noexcept { return projection_mode_; }
	[[nodiscard]] int max_steps() const noexcept { return max_steps_; }
	[[nodiscard]] float resolution_scale() const noexcept { return resolution_scale_; }
	[[nodiscard]] bool follow_primary_camera() const noexcept { return follow_primary_camera_; }
	[[nodiscard]] double follow_offset_theta() const noexcept { return follow_offset_theta_; }
	[[nodiscard]] double follow_offset_phi() const noexcept { return follow_offset_phi_; }

	void apply_saved_state(
		double saved_radius, double saved_theta, double saved_phi, double saved_fov_deg, double saved_exposure,
		int saved_tonemapping_mode, int saved_projection_mode, int saved_max_steps, float saved_resolution_scale,
		bool saved_follow_primary, double saved_follow_offset_theta, double saved_follow_offset_phi
	) noexcept {
		radius_ = saved_radius;
		theta_ = saved_theta;
		phi_ = saved_phi;
		fov_deg_ = saved_fov_deg;
		exposure_ = saved_exposure;
		tonemapping_mode_ = saved_tonemapping_mode;
		projection_mode_ = saved_projection_mode;
		max_steps_ = saved_max_steps;
		resolution_scale_ = saved_resolution_scale;
		follow_primary_camera_ = saved_follow_primary;
		follow_offset_theta_ = saved_follow_offset_theta;
		follow_offset_phi_ = saved_follow_offset_phi;
	}

	void set_performance_budget_scale(float scale) noexcept {
		external_budget_scale_ = std::clamp(scale, 0.1f, 1.0f);
	}

	void render() {
		if (!is_open_ || orchestrator_ == nullptr) return;

		ImGui::SetNextWindowPos(ImVec2(400.0f, 100.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(560.0f, 520.0f), ImGuiCond_FirstUseEver);

		if (ImGui::Begin(name_.c_str(), &is_open_)) {
			ImGui::BeginChild("SecondaryViewportImage", ImVec2(0.0f, ImGui::GetContentRegionAvail().y * 0.62f), true);
			const ImVec2 size = ImGui::GetContentRegionAvail();
			if (size.x > 4.0f && size.y > 4.0f) {
				render_frame(size);
				ImGui::Image(reinterpret_cast<void*>(static_cast<intptr_t>(gl_texture_id_)), size);

				if (!follow_primary_camera_ && ImGui::IsItemHovered()) {
					ImGuiIO& drag_io = ImGui::GetIO();
					if (ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0f)) {
						phi_ -= static_cast<double>(drag_io.MouseDelta.x) * 0.0045;
						theta_ = std::clamp(theta_ - static_cast<double>(drag_io.MouseDelta.y) * 0.0045, 0.02, std::numbers::pi_v<double> - 0.02);
					}
					if (drag_io.MouseWheel != 0.0f) {
						radius_ = std::clamp(radius_ * (1.0 - static_cast<double>(drag_io.MouseWheel) * 0.12), 1.5, 5000.0);
					}
				}
			}
			ImGui::EndChild();

			ImGui::Separator();
			ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Secondary Observer Placement");

			ImGui::Checkbox("Follow Primary Camera Orientation", &follow_primary_camera_);
			render_setting_tooltip("When enabled, this observer's angular position is derived from the primary viewport camera plus a fixed offset below, keeping the two views correlated as the primary camera moves.");

			if (follow_primary_camera_) {
				float off_theta = static_cast<float>(follow_offset_theta_);
				if (ImGui::SliderFloat("Offset Theta", &off_theta, -1.5f, 1.5f, "%.3f rad")) follow_offset_theta_ = off_theta;
				float off_phi = static_cast<float>(follow_offset_phi_);
				if (ImGui::SliderFloat("Offset Phi", &off_phi, -3.1416f, 3.1416f, "%.3f rad")) follow_offset_phi_ = off_phi;
			} else {
				ImGui::TextDisabled("Quick View Direction:");
				const double half_pi = std::numbers::pi_v<double> / 2.0;
				const double pi_val = std::numbers::pi_v<double>;
				if (ImGui::Button("Front", ImVec2(60.0f, 22.0f))) { theta_ = half_pi; phi_ = 0.0; }
				ImGui::SameLine();
				if (ImGui::Button("Back", ImVec2(60.0f, 22.0f))) { theta_ = half_pi; phi_ = pi_val; }
				ImGui::SameLine();
				if (ImGui::Button("Left", ImVec2(60.0f, 22.0f))) { theta_ = half_pi; phi_ = -half_pi; }
				ImGui::SameLine();
				if (ImGui::Button("Right", ImVec2(60.0f, 22.0f))) { theta_ = half_pi; phi_ = half_pi; }
				if (ImGui::Button("Top", ImVec2(60.0f, 22.0f))) { theta_ = 0.05; }
				ImGui::SameLine();
				if (ImGui::Button("Bottom", ImVec2(60.0f, 22.0f))) { theta_ = pi_val - 0.05; }
				ImGui::SameLine();
				if (ImGui::Button("45deg Elevated", ImVec2(110.0f, 22.0f))) { theta_ = half_pi - 0.7853981634; }
				render_setting_tooltip("Snaps this observer to a common cardinal viewing direction relative to the coordinate origin. Left-drag the image above to orbit freely, and scroll over it to zoom.");

				float r = static_cast<float>(radius_);
				if (ImGui::SliderFloat("Radius", &r, 2.0f, 500.0f, "%.2f M", ImGuiSliderFlags_Logarithmic)) radius_ = r;
				float th = static_cast<float>(theta_);
				if (ImGui::SliderAngle("Polar Angle (theta)", &th, 0.5f, 179.5f)) theta_ = th;
				float ph = static_cast<float>(phi_);
				if (ImGui::SliderAngle("Azimuthal Angle (phi)", &ph, -180.0f, 180.0f)) phi_ = ph;
				if (ImGui::Button("Reset To Equatorial View", ImVec2(-1.0f, 24.0f))) {
					theta_ = std::numbers::pi_v<double> / 2.0;
					phi_ = 0.0;
					radius_ = 60.0;
				}
			}

			ImGui::Separator();
			ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Optics");
			float fov = static_cast<float>(fov_deg_);
			if (ImGui::SliderFloat("Field of View", &fov, 10.0f, 150.0f, "%.1f")) fov_deg_ = fov;
			float exposure = static_cast<float>(exposure_);
			if (ImGui::SliderFloat("Exposure", &exposure, -6.0f, 6.0f, "%.2f EV")) exposure_ = exposure;
			const char* tonemappers[] = {"Linear Unclamped", "ACES Filmic Curve", "Logarithmic Extended HDR", "Reinhard Modified"};
			ImGui::Combo("Tonemapper", &tonemapping_mode_, tonemappers, IM_ARRAYSIZE(tonemappers));
			const char* projections[] = {"Pinhole", "Auto-Zoom", "Fisheye Stereographic", "Equirectangular 360", "Fisheye Equidistant", "Fisheye Orthographic", "Panini Cylindrical", "Hammer-Aitoff"};
			ImGui::Combo("Projection", &projection_mode_, projections, IM_ARRAYSIZE(projections));

			ImGui::Separator();
			ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Rendering Quality");
			ImGui::SliderFloat("Internal Resolution Scale", &resolution_scale_, 0.1f, 1.0f, "%.2fx");
			ImGui::SliderInt("Max Integration Steps", &max_steps_, 32, 4096);
			ImGui::Checkbox("Auto-Refresh Every Frame", &auto_refresh_);
			render_setting_tooltip("When disabled, this secondary view only re-renders when a placement, optics, or quality parameter changes, reducing CPU load while docked or unused.");
			if (!auto_refresh_) {
				ImGui::SameLine();
				if (ImGui::Button("Render Now")) {
					has_rendered_once_ = false;
				}
			}
		}
		ImGui::End();
	}
};

}

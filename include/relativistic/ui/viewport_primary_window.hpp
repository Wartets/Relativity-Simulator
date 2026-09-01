#pragma once

#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/render/geodesic_compute_pipeline.hpp"
#include "relativistic/ui/interactive_camera_controller.hpp"
#include <imgui.h>
#include <GLFW/glfw3.h>
#if defined(__APPLE__)
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif
#include <vector>
#include <cmath>
#include <algorithm>
#include <string>

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

#ifndef GL_RGBA32F
#define GL_RGBA32F 0x8814
#endif

namespace Relativistic::UI {

class ViewportPrimaryWindow {
private:
	Orchestrator::SimulationOrchestrator<1024>& orchestrator_;
	InteractiveCameraController& camera_controller_;
	Render::GeodesicComputePipeline pipeline_;
	uint32_t gl_texture_id_{0};
	uint32_t current_width_{1280};
	uint32_t current_height_{720};
	float resolution_scale_{1.0f};
	bool is_hovered_{false};
	bool is_focused_{false};
	bool show_telemetry_overlay_{true};

	void init_gl_texture() noexcept {
		if (gl_texture_id_ == 0) {
			glGenTextures(1, &gl_texture_id_);
			glBindTexture(GL_TEXTURE_2D, gl_texture_id_);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}
	}

public:
	ViewportPrimaryWindow(
		Orchestrator::SimulationOrchestrator<1024>& orchestrator,
		InteractiveCameraController& cam_ctrl
	) : orchestrator_(orchestrator),
	    camera_controller_(cam_ctrl),
	    pipeline_(Render::GeodesicPipelineConfig{
	        .width = 1280,
	        .height = 720,
	        .precision = Render::PrecisionMode::NativeFloat64,
	        .metric = Render::MetricId::Schwarzschild
	    }) {
		init_gl_texture();
	}

	~ViewportPrimaryWindow() {
		if (gl_texture_id_ != 0) {
			glDeleteTextures(1, &gl_texture_id_);
			gl_texture_id_ = 0;
		}
	}

	void render(GLFWwindow* window, double dt) {
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		if (ImGui::Begin("Primary Relativistic Viewport", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
			is_hovered_ = ImGui::IsWindowHovered();
			is_focused_ = ImGui::IsWindowFocused();

			const ImVec2 avail = ImGui::GetContentRegionAvail();
			const uint32_t target_w = std::max(static_cast<uint32_t>(avail.x * resolution_scale_), 64u);
			const uint32_t target_h = std::max(static_cast<uint32_t>(avail.y * resolution_scale_), 64u);

			if (target_w != current_width_ || target_h != current_height_) {
				current_width_ = target_w;
				current_height_ = target_h;
				pipeline_.resize(current_width_, current_height_);
			}

			if (is_hovered_ || is_focused_) {
				camera_controller_.update(window, dt);
			}

			const auto& cam = orchestrator_.camera();
			const auto& params = orchestrator_.parameters();

			Render::GpuCameraPushConstants cam_consts{};
			cam_consts.screen_width = current_width_;
			cam_consts.screen_height = current_height_;
			cam_consts.field_of_view_rad = cam.fov_deg * (std::numbers::pi / 180.0);
			cam_consts.metric_mass = params.mass;
			cam_consts.metric_spin = params.spin;
			cam_consts.metric_charge = params.charge;
			cam_consts.horizon_radius = 2.0 * params.mass;
			cam_consts.escape_radius = 100.0 * params.mass;
			cam_consts.projection_mode = params.projection_mode;
			cam_consts.observer_position = {0.0, cam.radius, cam.theta, cam.phi};

			const double pitch_r = cam.pitch * (std::numbers::pi / 180.0);
			const double yaw_r = cam.yaw * (std::numbers::pi / 180.0);
			const double cp = std::cos(pitch_r), sp = std::sin(pitch_r);
			const double cy = std::cos(yaw_r), sy = std::sin(yaw_r);

			cam_consts.tetrad_e0 = {1.0, 0.0, 0.0, 0.0};
			cam_consts.tetrad_e1 = {0.0, cp * cy, cp * sy, sp};
			cam_consts.tetrad_e2 = {0.0, -sy, cy, 0.0};
			cam_consts.tetrad_e3 = {0.0, -sp * cy, -sp * sy, cp};

			pipeline_.set_projection_mode(static_cast<Observer::ProjectionMode>(params.projection_mode));
			pipeline_.dispatch(cam_consts);

			const auto fb = pipeline_.framebuffer();
			glBindTexture(GL_TEXTURE_2D, gl_texture_id_);
			glTexImage2D(
				GL_TEXTURE_2D, 0, GL_RGBA32F,
				static_cast<GLsizei>(current_width_), static_cast<GLsizei>(current_height_),
				0, GL_RGBA, GL_FLOAT, fb.data()
			);

			ImGui::Image(
				reinterpret_cast<void*>(static_cast<intptr_t>(gl_texture_id_)),
				avail, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f)
			);

			if (show_telemetry_overlay_) {
				render_hud_overlay(avail);
			}
		}
		ImGui::End();
		ImGui::PopStyleVar();
	}

private:
	void render_hud_overlay(const ImVec2& avail) noexcept {
		const auto& cam = orchestrator_.camera();
		const auto& params = orchestrator_.parameters();
		const auto& tel = pipeline_.telemetry();

		ImGui::SetCursorPos(ImVec2(16.0f, 16.0f));
		ImGui::BeginGroup();
		ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "FPS: %.1f (%.2f ms)", tel.frame_rate_fps, tel.execution_time_ms);
		ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.9f, 0.9f), "Resolution: %ux%u", current_width_, current_height_);
		ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.9f, 0.9f), "Position (r, theta, phi): (%.2f, %.2f, %.2f)", cam.radius, cam.theta, cam.phi);
		ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.9f, 0.9f), "Camera Pitch/Yaw: %.1f deg / %.1f deg", cam.pitch, cam.yaw);
		ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.9f, 0.9f), "Active Metric: %s (M=%.2f, a=%.2f)", orchestrator_.active_metric_name().c_str(), params.mass, params.spin);
		ImGui::EndGroup();

		ImGui::SetCursorPos(ImVec2(avail.x - 220.0f, 16.0f));
		ImGui::BeginGroup();
		ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Controls (ZQSD / WASD):");
		ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 0.8f), "Z/W: Forward | S: Back");
		ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 0.8f), "Q/A: Left    | D: Right");
		ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 0.8f), "Space: Up    | Ctrl: Down");
		ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 0.8f), "Shift: Sprint | Mouse: Look");
		ImGui::EndGroup();
	}
};

}

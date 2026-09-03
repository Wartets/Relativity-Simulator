#pragma once

#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/render/geodesic_compute_pipeline.hpp"
#include "relativistic/ui/interactive_camera_controller.hpp"
#include "relativistic/io/screenshot_exporter.hpp"
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
	std::vector<float> color_upload_buffer_{};
	bool is_hovered_{false};
	bool is_focused_{false};
	bool show_telemetry_overlay_{true};
	bool window_visible_{true};
	uint32_t allocated_texture_w_{0};
	uint32_t allocated_texture_h_{0};
	Render::GpuCameraPushConstants last_camera_constants_{};
	double last_logical_time_{-1.0};
	double last_precision_selector_{-1.0};
	uint64_t last_synced_version_{0};
	bool force_rerender_{true};
	bool has_received_frame_{false};
	std::vector<double> frame_times_history_{};
	double current_frame_time_ms_{0.0};
	double rolling_average_time_ms_{0.0};
	bool has_sufficient_rolling_frames_{false};

	struct DynamicLookAtTarget {
		std::string label;
		std::array<double, 3> position{0.0, 0.0, 0.0};
		double recommended_distance{25.0};
	};

	[[nodiscard]] static uint32_t get_metric_id_from_name(std::string_view name) noexcept {
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
	        .metric = Render::MetricId::Schwarzschild,
	        .field_of_view_deg = 60.0,
	        .max_steps = 2048,
	        .initial_step = -0.05,
	        .headless = false,
	        .projection_mode = Observer::ProjectionMode::Equirectangular360
	    }) {
		init_gl_texture();
	}

	~ViewportPrimaryWindow() {
		if (gl_texture_id_ != 0) {
			glDeleteTextures(1, &gl_texture_id_);
			gl_texture_id_ = 0;
		}
	}

	void request_rerender() noexcept {
		force_rerender_ = true;
	}

	void request_screenshot(const std::string& output_directory, const std::string& filename_pattern, IO::ScreenshotFormat format) {
		std::vector<Render::GpuPixelOutput> fb;
		uint32_t fb_w = 0, fb_h = 0;
		pipeline_.copy_framebuffer(fb, fb_w, fb_h);
		if (fb_w == 0 || fb_h == 0 || fb.empty()) {
			return;
		}
		const std::string stem = IO::ScreenshotExporter::expand_filename_pattern(filename_pattern);
		IO::ScreenshotExporter::export_async(std::move(fb), fb_w, fb_h, output_directory, stem, format);
	}

	[[nodiscard]] bool is_hovered() const noexcept {
		return is_hovered_;
	}

	void render(GLFWwindow* window, double dt, bool fullscreen_bg) {
		if (fullscreen_bg) {
			ImGui::SetNextWindowPos(ImGui::GetMainViewport()->WorkPos);
			ImGui::SetNextWindowSize(ImGui::GetMainViewport()->WorkSize);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			window_visible_ = ImGui::Begin("Primary Relativistic Viewport", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground);
		} else {
			ImGui::SetNextWindowPos(ImVec2(340.0f, 35.0f), ImGuiCond_FirstUseEver);
			ImGui::SetNextWindowSize(ImVec2(1130.0f, 705.0f), ImGuiCond_FirstUseEver);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			window_visible_ = ImGui::Begin("Primary Relativistic Viewport", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		}

		if (!window_visible_) {
			ImGui::End();
			if (fullscreen_bg) {
				ImGui::PopStyleVar(2);
			} else {
				ImGui::PopStyleVar(1);
			}
			return;
		}

		is_hovered_ = ImGui::IsWindowHovered();
		is_focused_ = ImGui::IsWindowFocused();

			const auto& params = orchestrator_.parameters();
			resolution_scale_ = static_cast<float>(params.resolution_scale);

			const bool is_navigating = (is_hovered_ || is_focused_) && (
				camera_controller_.is_actively_navigating() ||
				glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS ||
				glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS ||
				glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS ||
				glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS
			);

			float active_scale = resolution_scale_;
			if (is_navigating) {
				active_scale = std::clamp(resolution_scale_ * 0.65f, 0.25f, 1.0f);
			}

			const ImVec2 avail = ImGui::GetContentRegionAvail();
			const uint32_t target_w = std::clamp(static_cast<uint32_t>(avail.x * active_scale), 64u, 3840u);
			const uint32_t target_h = std::clamp(static_cast<uint32_t>(avail.y * active_scale), 64u, 2160u);

			if (std::abs(static_cast<int>(target_w) - static_cast<int>(current_width_)) > 2 || 
			    std::abs(static_cast<int>(target_h) - static_cast<int>(current_height_)) > 2) {
				current_width_ = target_w;
				current_height_ = target_h;
				pipeline_.resize(current_width_, current_height_);
				color_upload_buffer_.assign(static_cast<size_t>(current_width_) * static_cast<size_t>(current_height_) * 4, 0.0f);
				force_rerender_ = true;
			}

			if (is_hovered_ || is_focused_) {
				camera_controller_.update(window, dt, is_hovered_);
			}

			const auto& cam = orchestrator_.camera();

			Render::GpuCameraPushConstants cam_consts{};
			cam_consts.screen_width = current_width_;
			cam_consts.screen_height = current_height_;
			cam_consts.field_of_view_rad = cam.fov_deg * (std::numbers::pi / 180.0);
			cam_consts.metric_type = get_metric_id_from_name(orchestrator_.active_metric_name());
			cam_consts.metric_mass = params.mass;
			cam_consts.metric_spin = params.spin;
			cam_consts.metric_charge = params.charge;
			cam_consts.cosmological_lambda = params.cosmological_lambda;
			cam_consts.wormhole_throat = params.wormhole_throat;
			cam_consts.warp_velocity = params.warp_velocity;
			cam_consts.camera_exposure = params.camera_exposure;
			cam_consts.tonemapping_mode = params.tonemapping_mode;
			cam_consts.horizon_radius = 2.0 * params.mass;
			{
				constexpr double kUnboundedRenderDistance = 1.0e7;
				const double configured_distance = (params.render_distance_scale > 0.0) ? (params.render_distance_scale * params.mass) : kUnboundedRenderDistance;
				cam_consts.escape_radius = std::max(configured_distance, cam.radius * 2.0);
			}
			cam_consts.projection_mode = params.projection_mode;
			cam_consts.max_integration_steps = params.max_ray_steps;
			cam_consts.render_flags = params.visual_overlays_flags;
			if (params.lod_enabled) {
				cam_consts.render_flags |= Render::RenderFlags::USE_LOD_SYSTEM;
			}
			cam_consts.lod_distance_threshold = params.lod_distance_scale * params.mass;
			cam_consts.lod_reduced_steps = params.lod_reduced_ray_steps;
			cam_consts.sky_rotation_rad = params.sky_rotation_deg * (std::numbers::pi / 180.0);
			cam_consts.sky_hue_shift_rad = params.sky_hue_shift_deg * (std::numbers::pi / 180.0);
			cam_consts.sky_saturation = params.sky_saturation;
			cam_consts.sky_star_density = params.sky_star_density;
			cam_consts.sky_star_brightness = params.sky_star_brightness;
			cam_consts.sky_nebula_intensity = params.sky_nebula_intensity;
			cam_consts.sky_grid_opacity = params.sky_grid_opacity;
			cam_consts.sky_background_r = params.sky_background_r;
			cam_consts.sky_background_g = params.sky_background_g;
			cam_consts.sky_background_b = params.sky_background_b;
			cam_consts.observer_position = {0.0, cam.radius, cam.theta, cam.phi};

			const double pitch_r = cam.pitch * (std::numbers::pi / 180.0);
			const double yaw_r = cam.yaw * (std::numbers::pi / 180.0);
			const double roll_r = cam.roll * (std::numbers::pi / 180.0);
			const double cp = std::cos(pitch_r), sp = std::sin(pitch_r);
			const double cy = std::cos(yaw_r), sy = std::sin(yaw_r);
			const double cr = std::cos(roll_r), sr = std::sin(roll_r);

			cam_consts.tetrad_e0 = {1.0, 0.0, 0.0, 0.0};
			cam_consts.tetrad_e1 = {0.0, cp * cy, cp * sy, sp};
			cam_consts.tetrad_e2 = {0.0, cr * (-sy) + sr * (-sp * cy), cr * cy + sr * (-sp * sy), sr * cp};
			cam_consts.tetrad_e3 = {0.0, -sr * (-sy) + cr * (-sp * cy), -sr * cy + cr * (-sp * sy), cr * cp};

			const double precision_selector = orchestrator_.get_custom_param("precision_mode", 0.0);
			const bool precision_changed = (precision_selector != last_precision_selector_);

			const uint64_t current_ver = orchestrator_.state_version();
			if (current_ver != last_synced_version_) {
				force_rerender_ = true;
				last_synced_version_ = current_ver;
			}
			const auto snap = orchestrator_.scheduler().snapshot();
			const bool is_time_progressing = !snap.is_paused || snap.remaining_steps > 0;
			const bool time_changed = (snap.logical_time != last_logical_time_);
			const bool params_changed = !(cam_consts == last_camera_constants_);
			const bool is_dirty = force_rerender_ || params_changed || precision_changed || (is_time_progressing && time_changed);

			if (is_dirty) {
				pipeline_.set_precision_mode(precision_selector > 0.5 ? Render::PrecisionMode::DoubleSingleEmulation : Render::PrecisionMode::NativeFloat64);
				pipeline_.set_projection_mode(static_cast<Observer::ProjectionMode>(params.projection_mode));
				pipeline_.dispatch(cam_consts);
				last_camera_constants_ = cam_consts;
				last_logical_time_ = snap.logical_time;
				last_precision_selector_ = precision_selector;
				force_rerender_ = false;
			}

			if (pipeline_.check_and_clear_new_frame()) {
				std::vector<Render::GpuPixelOutput> fb;
				uint32_t fb_w = 0, fb_h = 0;
				pipeline_.copy_framebuffer(fb, fb_w, fb_h);
				const size_t pixel_count = static_cast<size_t>(fb_w) * static_cast<size_t>(fb_h);
				if (pixel_count > 0 && fb.size() == pixel_count) {
					if (color_upload_buffer_.size() < pixel_count * 4) {
						color_upload_buffer_.assign(pixel_count * 4, 0.0f);
					}
					for (size_t i = 0; i < pixel_count; ++i) {
					color_upload_buffer_[i * 4 + 0] = fb[i].r;
					color_upload_buffer_[i * 4 + 1] = fb[i].g;
					color_upload_buffer_[i * 4 + 2] = fb[i].b;
					color_upload_buffer_[i * 4 + 3] = fb[i].a;
					}
					has_received_frame_ = true;
					const bool force_realloc = (params.visual_overlays_flags & Render::RenderFlags::FORCE_TEXTURE_REALLOCATION) != 0U;
					glBindTexture(GL_TEXTURE_2D, gl_texture_id_);
					if (force_realloc || fb_w != allocated_texture_w_ || fb_h != allocated_texture_h_) {
						glTexImage2D(
							GL_TEXTURE_2D, 0, GL_RGBA32F,
							static_cast<GLsizei>(fb_w), static_cast<GLsizei>(fb_h),
							0, GL_RGBA, GL_FLOAT, color_upload_buffer_.data()
						);
						allocated_texture_w_ = fb_w;
						allocated_texture_h_ = fb_h;
					} else {
						glTexSubImage2D(
							GL_TEXTURE_2D, 0, 0, 0,
							static_cast<GLsizei>(fb_w), static_cast<GLsizei>(fb_h),
							GL_RGBA, GL_FLOAT, color_upload_buffer_.data()
						);
					}
				}
			}

			ImGui::Image(
				reinterpret_cast<void*>(static_cast<intptr_t>(gl_texture_id_)),
				avail, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f)
			);

			render_viewport_toolbar();
			render_loading_indicator(avail);

			if (show_telemetry_overlay_) {
				render_hud_overlay(avail, window);
			}
		
		ImGui::End();
		if (fullscreen_bg) {
			ImGui::PopStyleVar(2);
		} else {
			ImGui::PopStyleVar(1);
		}
	}

	[[nodiscard]] std::vector<DynamicLookAtTarget> build_dynamic_look_at_targets() const noexcept {
		std::vector<DynamicLookAtTarget> targets;
		const auto& p = orchestrator_.parameters();
		const double m = std::max(p.mass, 1e-4);
		const double a = p.spin;
		const std::string& metric = orchestrator_.active_metric_name();

		targets.push_back(DynamicLookAtTarget{"Spacetime Singularity / Origin (r=0)", {0.0, 0.0, 0.0}, 25.0 * m});
		const double r_h = (std::abs(a) > 1e-6) ? (m + std::sqrt(std::max(m * m - a * a, 0.0))) : (2.0 * m);

		if (metric.find("Morris") != std::string::npos || metric.find("Wormhole") != std::string::npos) {
			const double b0 = std::max(p.wormhole_throat, 0.1);
			targets.push_back(DynamicLookAtTarget{"Wormhole Throat (b0=" + std::to_string(b0).substr(0, 4) + ")", {0.0, b0, 0.0}, b0 * 2.5});
			targets.push_back(DynamicLookAtTarget{"Alternate Universe Mouth (+l)", {0.0, b0 * 2.0, 0.0}, b0 * 3.5});
		} else if (metric.find("Alcubierre") != std::string::npos) {
			targets.push_back(DynamicLookAtTarget{"Warp Bubble Center", {0.0, 0.0, 0.0}, 30.0});
			targets.push_back(DynamicLookAtTarget{"Forward Contraction Boundary", {15.0, 0.0, 0.0}, 25.0});
			targets.push_back(DynamicLookAtTarget{"Rear Expansion Boundary", {-15.0, 0.0, 0.0}, 25.0});
		} else {
			targets.push_back(DynamicLookAtTarget{"Event Horizon (r=" + std::to_string(r_h).substr(0, 4) + "M)", {0.0, r_h * 1.05, 0.0}, r_h * 2.2});
		}

		const double r_ph = (std::abs(a) > 1e-6) ? (2.0 * m * (1.0 + std::cos(2.0 / 3.0 * std::acos(-std::clamp(a / m, -1.0, 1.0))))) : (3.0 * m);
		targets.push_back(DynamicLookAtTarget{"Photon Sphere (r=" + std::to_string(r_ph).substr(0, 4) + "M)", {0.0, r_ph, 0.0}, r_ph * 2.0});

		const double r_isco = (std::abs(a) > 1e-6) ? std::max(r_h * 1.05, 6.0 * m - 4.0 * a) : (6.0 * m);
		targets.push_back(DynamicLookAtTarget{"Accretion ISCO (r=" + std::to_string(r_isco).substr(0, 4) + "M)", {0.0, r_isco, 0.0}, r_isco * 1.8});

		targets.push_back(DynamicLookAtTarget{"Accretion Outer Edge (r=24M)", {0.0, 24.0 * m, 0.0}, 35.0 * m});
		targets.push_back(DynamicLookAtTarget{"North Polar Axis (+Z)", {0.0, 0.001, 20.0 * m}, 30.0 * m});
		targets.push_back(DynamicLookAtTarget{"South Polar Axis (-Z)", {0.0, 0.001, -20.0 * m}, 30.0 * m});

		const auto& sys = orchestrator_.nbody_system();
		for (const auto& b : sys.bodies()) {
			targets.push_back(DynamicLookAtTarget{
				"Celestial Body #" + std::to_string(b.id) + " (M=" + std::to_string(b.mass).substr(0, 4) + ")",
				b.position,
				std::max(b.radius * 4.0, 10.0)
			});
		}

		return targets;
	}

	void render_viewport_toolbar() noexcept {
		ImGui::SetCursorPos(ImVec2(16.0f, 16.0f));
		ImGui::BeginGroup();
		
		if (orchestrator_.scheduler().is_paused()) {
			if (ImGui::Button("Play (P)", ImVec2(75.0f, 24.0f))) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_resume()));
			}
		} else {
			if (ImGui::Button("Pause (P)", ImVec2(75.0f, 24.0f))) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_pause()));
			}
		}

		ImGui::SameLine();
		if (ImGui::Button("Step (F6)", ImVec2(68.0f, 24.0f))) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_step(1)));
		}

		ImGui::SameLine();
		if (ImGui::Button("Reset View", ImVec2(78.0f, 24.0f))) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_camera_reset()));
			camera_controller_.snap_to_equatorial_front(33.24);
		}

		ImGui::SameLine();
		const auto dynamic_targets = build_dynamic_look_at_targets();
		static int selected_target_idx = 0;
		if (selected_target_idx >= static_cast<int>(dynamic_targets.size())) {
			selected_target_idx = 0;
		}

		std::vector<const char*> target_labels;
		target_labels.reserve(dynamic_targets.size());
		for (const auto& t : dynamic_targets) {
			target_labels.push_back(t.label.c_str());
		}

		ImGui::SetNextItemWidth(210.0f);
		ImGui::Combo("##AimTargetCombo", &selected_target_idx, target_labels.data(), static_cast<int>(target_labels.size()));

		ImGui::SameLine();
		if (ImGui::Button("Look At Object", ImVec2(105.0f, 24.0f)) && selected_target_idx < static_cast<int>(dynamic_targets.size())) {
			camera_controller_.look_at_target(dynamic_targets[selected_target_idx].position);
		}

		ImGui::SameLine();
		if (ImGui::Button("Jump to Target", ImVec2(100.0f, 24.0f)) && selected_target_idx < static_cast<int>(dynamic_targets.size())) {
			const auto& tgt = dynamic_targets[selected_target_idx];
			camera_controller_.look_at_target(tgt.position);
			auto& c = orchestrator_.camera();
			c.position = {tgt.position[0], tgt.position[1] + tgt.recommended_distance, tgt.position[2]};
			c.orbit_distance = tgt.recommended_distance;
			c.radius = tgt.recommended_distance;
		}

		ImGui::SameLine();
		const char* cam_modes[] = {"Free Fly", "Orbit Center", "Spherical", "Rocket"};
		int cur_mode = static_cast<int>(orchestrator_.parameters().camera_mode);
		ImGui::SetNextItemWidth(95.0f);
		if (ImGui::Combo("##CamModeCombo", &cur_mode, cam_modes, IM_ARRAYSIZE(cam_modes))) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_camera_mode(static_cast<uint32_t>(cur_mode))));
		}

		ImGui::SameLine();
		ImGui::Checkbox("HUD", &show_telemetry_overlay_);

		ImGui::EndGroup();
	}

	void render_loading_indicator(const ImVec2& avail) noexcept {
		const bool is_loading = pipeline_.is_rendering() || !has_received_frame_;
		if (!is_loading) return;

		const char* label = "Calculating Geodesics...";
		const ImVec2 text_size = ImGui::CalcTextSize(label);
		const float radius = 14.0f;
		const float spinner_diameter = radius * 2.0f;
		const float gap = 12.0f;
		const float padding = 10.0f;

		const float panel_w = spinner_diameter + gap + text_size.x + padding * 2.0f;
		const float panel_h = std::max(spinner_diameter, text_size.y) + padding * 2.0f;

		const ImVec2 panel_top_left(avail.x - panel_w - 16.0f, avail.y - panel_h - 16.0f);
		const ImVec2 window_pos = ImGui::GetWindowPos();
		const ImVec2 draw_panel_top_left(window_pos.x + panel_top_left.x, window_pos.y + panel_top_left.y);

		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		draw_list->AddRectFilled(
			draw_panel_top_left,
			ImVec2(draw_panel_top_left.x + panel_w, draw_panel_top_left.y + panel_h),
			IM_COL32(15, 15, 25, 200),
			6.0f
		);

		const ImVec2 spinner_center(
			draw_panel_top_left.x + padding + radius,
			draw_panel_top_left.y + panel_h * 0.5f
		);

		const double time = ImGui::GetTime();
		const int num_segments = 24;
		const float start_angle = static_cast<float>(time * 8.0);
		const float arc_len = static_cast<float>(std::numbers::pi * 1.3);

		for (int i = 0; i < num_segments; ++i) {
			const float a1 = start_angle + (static_cast<float>(i) / static_cast<float>(num_segments)) * arc_len;
			const float a2 = start_angle + (static_cast<float>(i + 1) / static_cast<float>(num_segments)) * arc_len;
			const float alpha = static_cast<float>(i + 1) / static_cast<float>(num_segments);
			const ImU32 col = IM_COL32(50 + static_cast<int>(180 * alpha), 150 + static_cast<int>(105 * alpha), 255, static_cast<int>(255 * alpha));
			draw_list->AddLine(
				ImVec2(spinner_center.x + std::cos(a1) * radius, spinner_center.y + std::sin(a1) * radius),
				ImVec2(spinner_center.x + std::cos(a2) * radius, spinner_center.y + std::sin(a2) * radius),
				col, 3.0f
			);
		}

		const ImVec2 text_pos(
			spinner_center.x + radius + gap,
			draw_panel_top_left.y + (panel_h - text_size.y) * 0.5f
		);
		draw_list->AddText(text_pos, IM_COL32(200, 225, 255, 240), label);
	}

private:
	void render_hud_overlay(const ImVec2& avail, GLFWwindow* window) noexcept {
		const auto& cam = orchestrator_.camera();
		const auto& params = orchestrator_.parameters();
		const auto& tel = pipeline_.telemetry();

		current_frame_time_ms_ = tel.execution_time_ms;
		if (current_frame_time_ms_ > 0.0) {
			frame_times_history_.push_back(current_frame_time_ms_);
		}
		const size_t target_samples = std::max(uint32_t{2}, params.rolling_average_frame_count);
		while (frame_times_history_.size() > target_samples) {
			frame_times_history_.erase(frame_times_history_.begin());
		}

		has_sufficient_rolling_frames_ = (frame_times_history_.size() >= target_samples);
		if (has_sufficient_rolling_frames_) {
			double sum = 0.0;
			for (double ft : frame_times_history_) sum += ft;
			rolling_average_time_ms_ = sum / static_cast<double>(frame_times_history_.size());
		}

		ImGui::SetCursorPos(ImVec2(16.0f, 48.0f));
		ImGui::BeginGroup();
		if (has_sufficient_rolling_frames_) {
			ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "Frame Time: %.2f ms (Avg[%u]: %.2f ms | %.1f FPS)", current_frame_time_ms_, static_cast<unsigned int>(target_samples), rolling_average_time_ms_, 1000.0 / rolling_average_time_ms_);
		} else {
			ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "Frame Time: %.2f ms (Avg: warming up %zu/%u...)", current_frame_time_ms_, frame_times_history_.size(), static_cast<unsigned int>(target_samples));
		}
		ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.9f, 0.9f), "Camera Distance (r): %.2f M | Angles (theta, phi): (%.2f, %.2f)", cam.radius, cam.theta, cam.phi);
		ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.9f, 0.9f), "Orientation (Pitch, Yaw, Roll): (%.1f, %.1f, %.1f) deg", cam.pitch, cam.yaw, cam.roll);
		ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.9f, 0.9f), "Metric: %s (Mass=%.2f, Spin=%.2f, Charge=%.2f)", orchestrator_.active_metric_name().c_str(), params.mass, params.spin, params.charge);
		ImGui::TextColored(ImVec4(0.7f, 0.7f, 1.0f, 0.9f), "Absorbed Rays: %llu | Celestial Rays: %llu", static_cast<unsigned long long>(tel.horizon_pixels_absorbed), static_cast<unsigned long long>(tel.celestial_pixels_hit));
		ImGui::EndGroup();

		ImGui::SetCursorPos(ImVec2(avail.x - 240.0f, 16.0f));
		ImGui::BeginGroup();
		ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Navigation Controls:");

		auto draw_keybind = [&](const char* label, bool is_active) noexcept {
			if (is_active) {
				ImGui::TextColored(ImVec4(1.0f, 0.95f, 0.2f, 1.0f), "> %s <", label);
			} else {
				ImGui::TextColored(ImVec4(0.7f, 0.75f, 0.8f, 0.8f), "  %s", label);
			}
		};

		const bool fwd = window && (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS);
		const bool back = window && (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS);
		const bool left = window && (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS);
		const bool right = window && (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS);
		const bool up = window && (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS);
		const bool down = window && (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS);
		const bool roll = window && (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_PAGE_UP) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS);
		const bool sprint = window && (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS);
		const bool rmb = window && (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS || glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);

		draw_keybind("Z/W: Forward", fwd);
		draw_keybind("S: Back", back);
		draw_keybind("Q/A: Left", left);
		draw_keybind("D: Right", right);
		draw_keybind("Space: Up", up);
		draw_keybind("Ctrl/C: Down", down);
		draw_keybind("J/K: Roll", roll);
		draw_keybind("Shift: Sprint", sprint);
		draw_keybind("Mouse Drag: Look", rmb);
		ImGui::EndGroup();
	}
};

}

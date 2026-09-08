#pragma once

#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/render/geodesic_compute_pipeline.hpp"
#include "relativistic/ui/interactive_camera_controller.hpp"
#include "relativistic/ui/hud_layout_config.hpp"
#include "relativistic/ui/input_actions.hpp"
#include "relativistic/ui/schematic_view_renderer.hpp"
#include "relativistic/ui/tooltip_utils.hpp"
#include "relativistic/metrics/kerr.hpp"
#include "relativistic/io/screenshot_exporter.hpp"
#include <imgui.h>
#include <GLFW/glfw3.h>
#if defined(__APPLE__)
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif
#include <vector>
#include <string>
#include <cstdio>
#include <cmath>
#include <algorithm>

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

#ifndef GL_RGBA32F
#define GL_RGBA32F 0x8814
#endif

namespace Relativistic::UI {

class ViewportPrimaryWindow {
private:
	struct HudTextLine {
		std::string text;
		ImU32 color{0};
	};

	Orchestrator::SimulationOrchestrator<1024>& orchestrator_;
	InteractiveCameraController& camera_controller_;
	HudLayoutConfig& hud_layout_;
	SchematicViewConfig& schematic_cfg_;
	Render::GeodesicComputePipeline pipeline_;
	SchematicViewRenderer schematic_renderer_{};
	uint32_t gl_texture_id_{0};
	uint32_t current_width_{1280};
	uint32_t current_height_{720};
	float resolution_scale_{1.0f};
	std::vector<float> color_upload_buffer_{};
	bool is_hovered_{false};
	bool is_focused_{false};
	double zoom_level_{1.0};
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

	static void draw_hud_block(ImDrawList* draw_list, const ImVec2& window_pos, const ImVec2& avail, const HudElementStyle& style, const std::vector<HudTextLine>& lines) noexcept {
		if (!style.enabled || lines.empty()) return;

		const float font_size = ImGui::GetFontSize() * style.scale;
		const float line_height = font_size + 3.0f;
		constexpr float horizontal_gap = 18.0f;

		std::vector<ImVec2> line_sizes;
		line_sizes.reserve(lines.size());
		float max_width = 0.0f;
		float total_width = 0.0f;
		for (const auto& line : lines) {
			const ImVec2 sz = ImGui::CalcTextSize(line.text.c_str());
			const ImVec2 scaled(sz.x * style.scale, sz.y * style.scale);
			line_sizes.push_back(scaled);
			max_width = std::max(max_width, scaled.x);
			total_width += scaled.x;
		}
		if (style.horizontal_layout && lines.size() > 1) {
			total_width += horizontal_gap * static_cast<float>(lines.size() - 1);
		}

		const ImVec2 block_size = style.horizontal_layout
			? ImVec2(total_width, line_height)
			: ImVec2(max_width, line_height * static_cast<float>(lines.size()));
		const ImVec2 local_pos = hud_anchor_resolve(style.anchor, avail, block_size, style.offset_x, style.offset_y);
		const ImVec2 screen_pos(window_pos.x + local_pos.x, window_pos.y + local_pos.y);

		if (style.show_background) {
			draw_list->AddRectFilled(
				ImVec2(screen_pos.x - 6.0f, screen_pos.y - 4.0f),
				ImVec2(screen_pos.x + block_size.x + 6.0f, screen_pos.y + block_size.y + 4.0f),
				IM_COL32(10, 12, 18, static_cast<int>(std::clamp(style.background_opacity, 0.0f, 1.0f) * 255.0f)),
				4.0f
			);
		}

		float cursor_x = screen_pos.x;
		for (size_t i = 0; i < lines.size(); ++i) {
			const ImU32 col = (lines[i].color != 0)
				? lines[i].color
				: ImGui::ColorConvertFloat4ToU32(ImVec4(style.text_color[0], style.text_color[1], style.text_color[2], style.text_color[3]));
			if (style.horizontal_layout) {
				const ImVec2 line_pos(cursor_x, screen_pos.y);
				draw_list->AddText(nullptr, font_size, line_pos, col, lines[i].text.c_str());
				cursor_x += line_sizes[i].x + horizontal_gap;
			} else {
				const ImVec2 line_pos(screen_pos.x, screen_pos.y + line_height * static_cast<float>(i));
				draw_list->AddText(nullptr, font_size, line_pos, col, lines[i].text.c_str());
			}
		}
	}

public:
	ViewportPrimaryWindow(
		Orchestrator::SimulationOrchestrator<1024>& orchestrator,
		InteractiveCameraController& cam_ctrl,
		HudLayoutConfig& hud_layout,
		SchematicViewConfig& schematic_cfg
	) : orchestrator_(orchestrator),
	    camera_controller_(cam_ctrl),
	    hud_layout_(hud_layout),
	    schematic_cfg_(schematic_cfg),
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

	void handle_zoom_scroll(double yoffset) noexcept {
		const auto& zoom_cfg = camera_controller_.config().zoom;
		zoom_level_ = std::clamp(zoom_level_ + yoffset * zoom_cfg.zoom_scroll_sensitivity * zoom_level_, zoom_cfg.min_zoom, zoom_cfg.max_zoom);
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

	[[nodiscard]] Render::GeodesicComputePipeline& pipeline_ref() noexcept {
		return pipeline_;
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
			pipeline_.set_gpu_compute_enabled(params.use_gpu_compute);

			const auto& active_keybinds = camera_controller_.config().keybinds;
			const bool is_navigating = (is_hovered_ || is_focused_) && (
				camera_controller_.is_actively_navigating() ||
				active_keybinds.is_pressed(InputAction::MoveForward, window) ||
				active_keybinds.is_pressed(InputAction::MoveBackward, window) ||
				active_keybinds.is_pressed(InputAction::MoveLeft, window) ||
				active_keybinds.is_pressed(InputAction::MoveRight, window)
			);

			float active_scale = resolution_scale_;
			if (is_navigating) {
				switch (static_cast<Orchestrator::MotionQualityMode>(params.motion_quality_mode)) {
					case Orchestrator::MotionQualityMode::Disabled:
						active_scale = resolution_scale_;
						break;
					case Orchestrator::MotionQualityMode::Fixed:
						active_scale = static_cast<float>(params.motion_quality_scale);
						break;
					case Orchestrator::MotionQualityMode::Automatic:
					default:
						active_scale = std::clamp(resolution_scale_ * 0.65f, 0.1f, resolution_scale_);
						break;
				}
				active_scale = std::clamp(active_scale, 0.1f, 2.0f);
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

			if (params.schematic_mode_enabled) {
				const ImVec2 schematic_pos = ImGui::GetCursorScreenPos();
				const auto schematic_projection_mode = schematic_cfg_.respect_active_projection_mode
					? static_cast<Observer::ProjectionMode>(params.projection_mode)
					: Observer::ProjectionMode::Pinhole;
				schematic_renderer_.configure(cam, schematic_projection_mode, cam.fov_deg * (std::numbers::pi / 180.0), schematic_pos, avail);
				schematic_renderer_.render(ImGui::GetWindowDrawList(), orchestrator_, schematic_cfg_);
				ImGui::Dummy(avail);

				if (hud_layout_.element(HudElementId::ViewportToolbar).enabled) {
					render_viewport_toolbar(avail);
				}
				render_hud_overlay(avail, window);

				ImGui::End();
				if (fullscreen_bg) {
					ImGui::PopStyleVar(2);
				} else {
					ImGui::PopStyleVar(1);
				}
				return;
			}

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
			if (params.space_skipping_enabled) {
				cam_consts.render_flags |= Render::RenderFlags::SPACE_SKIP_ENABLED;
			}
			cam_consts.space_skip_radius_scale = params.space_skip_radius_scale;
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

			const ImVec2 viewport_image_pos = ImGui::GetCursorScreenPos();
			const bool zoom_key_down = (window != nullptr) && (is_hovered_ || is_focused_) && camera_controller_.config().keybinds.is_active(InputAction::ZoomModifier, window);
			if (!zoom_key_down) {
				zoom_level_ = 1.0;
			}
			ImVec2 zoom_uv0(0.0f, 0.0f);
			ImVec2 zoom_uv1(1.0f, 1.0f);
			if (zoom_level_ > 1.0001) {
				const auto& zoom_cfg = camera_controller_.config().zoom;
				ImVec2 zoom_focus(0.5f, 0.5f);
				if (zoom_cfg.zoom_center_on_cursor && avail.x > 0.0f && avail.y > 0.0f) {
					const ImVec2 mouse_pos = ImGui::GetMousePos();
					zoom_focus.x = (mouse_pos.x - viewport_image_pos.x) / avail.x;
					zoom_focus.y = (mouse_pos.y - viewport_image_pos.y) / avail.y;
				}
				const float half_w = static_cast<float>(0.5 / zoom_level_);
				const float half_h = static_cast<float>(0.5 / zoom_level_);
				zoom_focus.x = std::clamp(zoom_focus.x, half_w, 1.0f - half_w);
				zoom_focus.y = std::clamp(zoom_focus.y, half_h, 1.0f - half_h);
				zoom_uv0 = ImVec2(zoom_focus.x - half_w, zoom_focus.y - half_h);
				zoom_uv1 = ImVec2(zoom_focus.x + half_w, zoom_focus.y + half_h);
			}

			ImGui::Image(
				reinterpret_cast<void*>(static_cast<intptr_t>(gl_texture_id_)),
				avail, zoom_uv0, zoom_uv1
			);

			if (hud_layout_.element(HudElementId::ViewportToolbar).enabled) {
				render_viewport_toolbar(avail);
			}
			render_loading_indicator(avail);
			render_hud_overlay(avail, window);
		
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

	void render_viewport_toolbar(const ImVec2& avail) noexcept {
		const auto& style = hud_layout_.element(HudElementId::ViewportToolbar);
		if (!style.enabled || !hud_layout_.master_enabled) return;

		const auto& tb = hud_layout_.toolbar_buttons;
		const auto& params = orchestrator_.parameters();
		const bool schematic_locked = params.schematic_mode_enabled && !params.schematic_allow_simulation;

		const ImVec2 estimated_size(900.0f, 34.0f);
		const ImVec2 pos = hud_anchor_resolve(style.anchor, avail, estimated_size, style.offset_x, style.offset_y);
		ImGui::SetCursorPos(pos);
		ImGui::BeginGroup();

		const auto& toolbar_keybinds = camera_controller_.config().keybinds;
		auto toolbar_key_hint = [&](InputAction action) noexcept -> std::string {
			const auto& b = toolbar_keybinds.get(action);
			return (b.primary_key != GLFW_KEY_UNKNOWN) ? glfw_key_display_name(b.primary_key) : std::string(glfw_key_display_name(b.secondary_key));
		};

		if (tb.play_pause) {
			if (schematic_locked) ImGui::BeginDisabled(true);
			if (orchestrator_.scheduler().is_paused()) {
				const std::string label = "Play (" + toolbar_key_hint(InputAction::TogglePausePlay) + ")";
				if (ImGui::Button(label.c_str(), ImVec2(75.0f, 24.0f))) {
					static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_resume()));
				}
			} else {
				const std::string label = "Pause (" + toolbar_key_hint(InputAction::TogglePausePlay) + ")";
				if (ImGui::Button(label.c_str(), ImVec2(75.0f, 24.0f))) {
					static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_pause()));
				}
			}
			if (schematic_locked) {
				ImGui::EndDisabled();
				render_setting_tooltip_warning("Simulation playback controls.", "Disabled while Schematic Orbital View is active. Enable 'Allow Simulation Clock To Run In Schematic View' in the Schematic View tab to unlock.");
			}
			ImGui::SameLine();
		}

		if (tb.step) {
			if (schematic_locked) ImGui::BeginDisabled(true);
			const std::string step_label = "Step (" + toolbar_key_hint(InputAction::SingleStepTick) + ")";
			if (ImGui::Button(step_label.c_str(), ImVec2(68.0f, 24.0f))) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_step(1)));
			}
			if (schematic_locked) {
				ImGui::EndDisabled();
				render_setting_tooltip_warning("Advance the simulation by one tick.", "Disabled while Schematic Orbital View is active. Enable 'Allow Simulation Clock To Run In Schematic View' in the Schematic View tab to unlock.");
			}
			ImGui::SameLine();
		}

		if (tb.reset_view) {
			if (ImGui::Button("Reset View", ImVec2(78.0f, 24.0f))) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_camera_reset()));
				camera_controller_.snap_to_equatorial_front(33.24);
			}
			ImGui::SameLine();
		}

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

		if (tb.look_at_target_combo || tb.jump_to_target) {
			ImGui::SetNextItemWidth(210.0f);
			ImGui::Combo("##AimTargetCombo", &selected_target_idx, target_labels.data(), static_cast<int>(target_labels.size()));
			ImGui::SameLine();
		}

		if (tb.look_at_target_combo) {
			if (ImGui::Button("Look At Object", ImVec2(105.0f, 24.0f)) && selected_target_idx < static_cast<int>(dynamic_targets.size())) {
				camera_controller_.look_at_target(dynamic_targets[selected_target_idx].position);
			}
			ImGui::SameLine();
		}

		if (tb.jump_to_target) {
			if (ImGui::Button("Jump to Target", ImVec2(100.0f, 24.0f)) && selected_target_idx < static_cast<int>(dynamic_targets.size())) {
				const auto& tgt = dynamic_targets[selected_target_idx];
				camera_controller_.look_at_target(tgt.position);
				auto& c = orchestrator_.camera();
				c.position = {tgt.position[0], tgt.position[1] + tgt.recommended_distance, tgt.position[2]};
				c.orbit_distance = tgt.recommended_distance;
				c.radius = tgt.recommended_distance;
			}
			ImGui::SameLine();
		}

		if (tb.camera_mode_combo) {
			const char* cam_modes[] = {"Free Fly", "Orbit Center", "Spherical", "Rocket"};
			int cur_mode = static_cast<int>(orchestrator_.parameters().camera_mode);
			ImGui::SetNextItemWidth(95.0f);
			if (ImGui::Combo("##CamModeCombo", &cur_mode, cam_modes, IM_ARRAYSIZE(cam_modes))) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_camera_mode(static_cast<uint32_t>(cur_mode))));
			}
			ImGui::SameLine();
		}

		if (tb.hud_master_toggle) {
			ImGui::Checkbox("HUD", &hud_layout_.master_enabled);
		}

		ImGui::EndGroup();
	}

	void render_loading_indicator(const ImVec2& avail) noexcept {
		const auto& style = hud_layout_.element(HudElementId::LoadingIndicator);
		if (!style.enabled) return;

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

		const ImVec2 panel_top_left = hud_anchor_resolve(style.anchor, avail, ImVec2(panel_w, panel_h), style.offset_x, style.offset_y);
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
		if (!hud_layout_.master_enabled) return;

		const auto& cam = orchestrator_.camera();
		const auto& params = orchestrator_.parameters();
		const auto& tel = pipeline_.telemetry();
		const auto snap = orchestrator_.scheduler().snapshot();
		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		const ImVec2 window_pos = ImGui::GetWindowPos();

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

		{
			const auto& ft_style = hud_layout_.element(HudElementId::FrameTimeReadout);
			char buf[192];
			const double instant_fps = (current_frame_time_ms_ > 0.0) ? (1000.0 / current_frame_time_ms_) : 0.0;
			if (ft_style.display_mode == HudDisplayMode::Compact) {
				std::snprintf(buf, sizeof(buf), "%.0f FPS", has_sufficient_rolling_frames_ ? (1000.0 / rolling_average_time_ms_) : instant_fps);
			} else if (ft_style.display_mode == HudDisplayMode::Extended) {
				if (has_sufficient_rolling_frames_) {
					std::snprintf(buf, sizeof(buf), "Frame Time: %.3f ms | Instant: %.1f FPS | Avg[%u]: %.3f ms (%.1f FPS) | Samples: %zu", current_frame_time_ms_, instant_fps, static_cast<unsigned int>(target_samples), rolling_average_time_ms_, 1000.0 / rolling_average_time_ms_, frame_times_history_.size());
				} else {
					std::snprintf(buf, sizeof(buf), "Frame Time: %.3f ms | Instant: %.1f FPS | Avg: warming up %zu/%u", current_frame_time_ms_, instant_fps, frame_times_history_.size(), static_cast<unsigned int>(target_samples));
				}
			} else {
				if (has_sufficient_rolling_frames_) {
					std::snprintf(buf, sizeof(buf), "Frame Time: %.2f ms (Avg[%u]: %.2f ms | %.1f FPS)", current_frame_time_ms_, static_cast<unsigned int>(target_samples), rolling_average_time_ms_, 1000.0 / rolling_average_time_ms_);
				} else {
					std::snprintf(buf, sizeof(buf), "Frame Time: %.2f ms (Avg: warming up %zu/%u...)", current_frame_time_ms_, frame_times_history_.size(), static_cast<unsigned int>(target_samples));
				}
			}
			draw_hud_block(draw_list, window_pos, avail, ft_style, {HudTextLine{buf}});
		}

		{
			char buf[96];
			std::snprintf(buf, sizeof(buf), "Camera Distance (r): %.2f M", cam.radius);
			draw_hud_block(draw_list, window_pos, avail, hud_layout_.element(HudElementId::CameraDistanceReadout), {HudTextLine{buf}});
		}

		{
			char buf[96];
			std::snprintf(buf, sizeof(buf), "Angles (theta, phi): (%.2f, %.2f)", cam.theta, cam.phi);
			draw_hud_block(draw_list, window_pos, avail, hud_layout_.element(HudElementId::CameraAnglesReadout), {HudTextLine{buf}});
		}

		{
			char buf[128];
			std::snprintf(buf, sizeof(buf), "Orientation (Pitch, Yaw, Roll): (%.1f, %.1f, %.1f) deg", cam.pitch, cam.yaw, cam.roll);
			draw_hud_block(draw_list, window_pos, avail, hud_layout_.element(HudElementId::CameraOrientationReadout), {HudTextLine{buf}});
		}

		{
			char buf[160];
			std::snprintf(buf, sizeof(buf), "Metric: %s (Mass=%.2f, Spin=%.2f, Charge=%.2f)", orchestrator_.active_metric_name().c_str(), params.mass, params.spin, params.charge);
			draw_hud_block(draw_list, window_pos, avail, hud_layout_.element(HudElementId::MetricSummaryReadout), {HudTextLine{buf}});
		}

		{
			char buf[128];
			std::snprintf(buf, sizeof(buf), "Absorbed Rays: %llu | Celestial Rays: %llu", static_cast<unsigned long long>(tel.horizon_pixels_absorbed), static_cast<unsigned long long>(tel.celestial_pixels_hit));
			draw_hud_block(draw_list, window_pos, avail, hud_layout_.element(HudElementId::RayStatisticsReadout), {HudTextLine{buf}});
		}

		{
			char buf[80];
			std::snprintf(buf, sizeof(buf), "Logical Time: %.3f s | Tick #%llu", snap.logical_time, static_cast<unsigned long long>(snap.tick_index));
			draw_hud_block(draw_list, window_pos, avail, hud_layout_.element(HudElementId::SimulationClockReadout), {HudTextLine{buf}});
		}

		{
			char buf[48];
			std::snprintf(buf, sizeof(buf), "Warp Factor: %.2fx", snap.warp_factor);
			draw_hud_block(draw_list, window_pos, avail, hud_layout_.element(HudElementId::WarpFactorReadout), {HudTextLine{buf}});
		}

		{
			char buf[48];
			std::snprintf(buf, sizeof(buf), "Performance Preset: %u", params.performance_preset);
			draw_hud_block(draw_list, window_pos, avail, hud_layout_.element(HudElementId::PerformancePresetReadout), {HudTextLine{buf}});
		}

		{
			const double r_safe_link = std::max(cam.radius, 2.05 * params.mass);
			Metrics::KerrMetric<double> tel_metric(params.mass, params.spin, 1.0, 1.0);
			Core::FourVector<double> tel_pos(0.0, r_safe_link, cam.theta, cam.phi);
			const auto tel_g = tel_metric.metric_tensor(tel_pos);
			const double lapse = std::sqrt(std::max(-tel_g(0, 0), 1e-30));
			char buf[128];
			std::snprintf(buf, sizeof(buf), "Lapse (alpha): %.4f | Time Dilation: %.3fx", lapse, 1.0 / std::max(lapse, 1e-12));
			draw_hud_block(draw_list, window_pos, avail, hud_layout_.element(HudElementId::TelemetryQuickReadout), {HudTextLine{buf}});
		}

		{
			char buf[96];
			std::snprintf(buf, sizeof(buf), "Doppler g: 1.000 | Exposure: %.2f EV", params.camera_exposure);
			draw_hud_block(draw_list, window_pos, avail, hud_layout_.element(HudElementId::SpectrographQuickReadout), {HudTextLine{buf}});
		}

		{
			char buf[96];
			std::snprintf(buf, sizeof(buf), "Metric: %s | Integrator: %s", orchestrator_.active_metric_name().c_str(), orchestrator_.active_integrator_name().c_str());
			draw_hud_block(draw_list, window_pos, avail, hud_layout_.element(HudElementId::DiagnosticsQuickReadout), {HudTextLine{buf}});
		}

		{
			const auto& kb = camera_controller_.config().keybinds;
			auto bind_str = [&](InputAction action) noexcept -> std::string {
				const auto& b = kb.get(action);
				if (b.primary_key == GLFW_KEY_UNKNOWN && b.secondary_key == GLFW_KEY_UNKNOWN) {
					return "---";
				}
				std::string s = glfw_key_display_name(b.primary_key);
				if (b.secondary_key != GLFW_KEY_UNKNOWN) {
					s += "/";
					s += glfw_key_display_name(b.secondary_key);
				}
				return s;
			};

			std::vector<HudTextLine> nav_lines;
			nav_lines.push_back(HudTextLine{"Keybind Summary:", IM_COL32(102, 204, 255, 255)});
			for (size_t i = 0; i < static_cast<size_t>(InputAction::Count); ++i) {
				const auto action = static_cast<InputAction>(i);
				const auto& b = kb.get(action);
				const bool bound = (b.primary_key != GLFW_KEY_UNKNOWN) || (b.secondary_key != GLFW_KEY_UNKNOWN);
				if (!bound || !hud_layout_.keybind_summary_visible[i]) {
					continue;
				}
				const bool active = (window != nullptr) && kb.is_pressed(action, window);
				const ImU32 col = active ? IM_COL32(255, 242, 51, 255) : IM_COL32(178, 191, 204, 204);
				nav_lines.push_back(HudTextLine{bind_str(action) + ": " + std::string(input_action_name(action)), col});
			}

			if (nav_lines.size() > 1) {
				draw_hud_block(draw_list, window_pos, avail, hud_layout_.element(HudElementId::NavigationControlsPanel), nav_lines);
			}
		}
	}
};

}

#pragma once

#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/orchestrator/performance_profiler.hpp"
#include "relativistic/render/geodesic_compute_pipeline.hpp"
#include "relativistic/optics/textures/earth_texture_image.hpp"
#include "relativistic/ui/interactive_camera_controller.hpp"
#include "relativistic/ui/hud/hud_layout_config.hpp"
#include "relativistic/ui/input_actions.hpp"
#include "relativistic/ui/schematic/schematic_view_renderer.hpp"
#include "relativistic/ui/schematic/schematic_primary_source_overlay.hpp"
#include "relativistic/ui/spatial_reference/spatial_reference_renderer.hpp"
#include "relativistic/ui/tooltip_utils.hpp"
#include "relativistic/metrics/kerr.hpp"
#include "relativistic/metrics/kerr_invariants.hpp"
#include "relativistic/optics/disk_thermal_profile.hpp"
#include "relativistic/optics/geodesic_ray_probe.hpp"
#include "relativistic/observer/camera_projections.hpp"
#include "relativistic/interferometry/visibility_synthesis.hpp"
#include "relativistic/ui/hud/hud_linked_readouts.hpp"
#include "relativistic/ui/coordinate_display.hpp"
#include "relativistic/units/unit_system.hpp"
#include "relativistic/io/capture/screenshot_exporter.hpp"
#include "relativistic/io/capture/screenshot_capture_settings.hpp"
#include "relativistic/io/capture/video_capture_settings.hpp"
#include "relativistic/capture/capture_coordinator.hpp"
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
#include <numbers>
#include <algorithm>
#include <chrono>
#include <functional>
#include <limits>
#include <thread>
#include <atomic>
#include <mutex>

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
	SpatialReferenceConfig& spatial_cfg_;
	Render::GeodesicComputePipeline pipeline_;
	SchematicViewRenderer schematic_renderer_{};
	SpatialReferenceRenderer spatial_renderer_{};
	uint32_t gl_texture_id_{0};
	uint32_t current_width_{1280};
	uint32_t current_height_{720};
	float resolution_scale_{1.0f};
	std::vector<float> color_upload_buffer_{};
	std::vector<Render::GpuPixelOutput> display_framebuffer_{};
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
	uint64_t last_earth_texture_revision_{0};
	bool force_rerender_{true};
	bool has_received_frame_{false};
	uint32_t interlace_phase_{0};
	double last_dispatch_clock_{0.0};
	float dynamic_resolution_multiplier_{1.0f};
	std::vector<double> frame_times_history_{};
	double current_frame_time_ms_{0.0};
	double rolling_average_time_ms_{0.0};
	bool has_sufficient_rolling_frames_{false};
	std::function<void()> fullscreen_toggle_callback_{};
	std::function<void()> open_screenshot_settings_callback_{};
	std::unique_ptr<Capture::CaptureCoordinator> capture_coordinator_{};
	bool capture_constants_valid_{false};
	ImVec2 viewport_content_size_{0.0f, 0.0f};
	ImVec2 viewport_window_origin_{0.0f, 0.0f};
	ImVec2 viewport_window_extent_{0.0f, 0.0f};
	Optics::RayProbeResult ray_probe_result_{};
	Optics::RayProbeQuery ray_probe_query_{};
	bool ray_probe_has_query_{false};
	bool ray_probe_active_{false};
	bool ray_probe_frozen_{false};
	int ray_probe_source_{0};
	ImVec2 ray_probe_texture_position_{0.5f, 0.5f};
	uint32_t display_frame_width_{0};
	uint32_t display_frame_height_{0};
	HudLinkedReadouts linked_readouts_{};
	bool cursor_inside_{false};
	uint32_t cursor_pixel_x_{0};
	uint32_t cursor_pixel_y_{0};
	ImVec2 cursor_uv_{0.0f, 0.0f};
	ImVec2 cursor_viewport_position_{0.0f, 0.0f};
	Optics::RayProbeResult cursor_probe_result_{};
	Optics::RayProbeQuery cursor_probe_query_{};
	bool cursor_probe_has_query_{false};

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

	[[nodiscard]] static ImVec2 measure_hud_block(const HudElementStyle& style, const std::vector<HudTextLine>& lines) noexcept {
		const float font_size = ImGui::GetFontSize() * style.scale;
		const float line_height = font_size + 3.0f;
		constexpr float horizontal_gap = 18.0f;
		float max_width = 0.0f;
		float total_width = 0.0f;
		for (const auto& line : lines) {
			const ImVec2 sz = ImGui::CalcTextSize(line.text.c_str());
			max_width = std::max(max_width, sz.x * style.scale);
			total_width += sz.x * style.scale;
		}
		if (style.horizontal_layout && lines.size() > 1) {
			total_width += horizontal_gap * static_cast<float>(lines.size() - 1);
		}
		return style.horizontal_layout
			? ImVec2(total_width, line_height)
			: ImVec2(max_width, line_height * static_cast<float>(lines.size()));
	}

	static void draw_hud_block_at(ImDrawList* draw_list, const ImVec2& screen_pos, const HudElementStyle& style, const std::vector<HudTextLine>& lines) noexcept {
		if (lines.empty()) return;

		const float font_size = ImGui::GetFontSize() * style.scale;
		const float line_height = font_size + 3.0f;
		constexpr float horizontal_gap = 18.0f;
		const ImVec2 block_size = measure_hud_block(style, lines);

		std::vector<ImVec2> line_sizes;
		line_sizes.reserve(lines.size());
		for (const auto& line : lines) {
			const ImVec2 sz = ImGui::CalcTextSize(line.text.c_str());
			line_sizes.push_back(ImVec2(sz.x * style.scale, sz.y * style.scale));
		}

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

	[[nodiscard]] bool follows_moving_target() const noexcept {
		const uint32_t mode = orchestrator_.parameters().camera_mode;
		return mode == static_cast<uint32_t>(CameraNavigationMode::SurfaceWalk);
	}

public:
	ViewportPrimaryWindow(
		Orchestrator::SimulationOrchestrator<1024>& orchestrator,
		InteractiveCameraController& cam_ctrl,
		HudLayoutConfig& hud_layout,
		SchematicViewConfig& schematic_cfg,
		SpatialReferenceConfig& spatial_cfg
	) : orchestrator_(orchestrator),
		camera_controller_(cam_ctrl),
		hud_layout_(hud_layout),
		schematic_cfg_(schematic_cfg),
		spatial_cfg_(spatial_cfg),
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
		capture_coordinator_ = std::make_unique<Capture::CaptureCoordinator>(orchestrator_, pipeline_);
		capture_coordinator_->bind_frame_sources(
			[this]() -> std::optional<Render::GpuCameraPushConstants> { return build_capture_constants(); },
			[this]() -> std::vector<Render::GpuBodyData> { return collect_capture_bodies(); }
		);
	}

	~ViewportPrimaryWindow() {
		capture_coordinator_.reset();
		if (gl_texture_id_ != 0) {
			glDeleteTextures(1, &gl_texture_id_);
			gl_texture_id_ = 0;
		}
	}

	void request_rerender() noexcept {
		force_rerender_ = true;
	}

	void set_path_preview(const Capture::PathPreview* preview) {
		if (preview != nullptr) {
			schematic_renderer_.set_path_preview(*preview);
		} else {
			schematic_renderer_.clear_path_preview();
		}
	}

	void set_fullscreen_toggle_callback(std::function<void()> callback) noexcept {
		fullscreen_toggle_callback_ = std::move(callback);
	}

	void set_open_screenshot_settings_callback(std::function<void()> callback) noexcept {
		open_screenshot_settings_callback_ = std::move(callback);
	}

	void handle_zoom_scroll(double yoffset) noexcept {
		const auto& zoom_cfg = camera_controller_.config().zoom;
		zoom_level_ = std::clamp(zoom_level_ + yoffset * zoom_cfg.zoom_scroll_sensitivity * zoom_level_, zoom_cfg.min_zoom, zoom_cfg.max_zoom);
	}

	[[nodiscard]] Capture::CaptureCoordinator& capture_coordinator() noexcept {
		return *capture_coordinator_;
	}

	[[nodiscard]] const Capture::CaptureCoordinator& capture_coordinator() const noexcept {
		return *capture_coordinator_;
	}

	[[nodiscard]] ImVec2 content_size() const noexcept {
		if (viewport_content_size_.x < 1.0f || viewport_content_size_.y < 1.0f) {
			return ImVec2(1280.0f, 720.0f);
		}
		return viewport_content_size_;
	}

	[[nodiscard]] ImVec2 window_center() const noexcept {
		if (viewport_window_extent_.x < 1.0f || viewport_window_extent_.y < 1.0f) {
			return ImGui::GetMainViewport()->GetCenter();
		}
		return ImVec2(viewport_window_origin_.x + viewport_window_extent_.x * 0.5f, viewport_window_origin_.y + viewport_window_extent_.y * 0.5f);
	}

	[[nodiscard]] std::optional<Render::GpuCameraPushConstants> build_capture_constants() const {
		if (!capture_constants_valid_) {
			return std::nullopt;
		}
		Render::GpuCameraPushConstants constants = last_camera_constants_;
		const auto& params = orchestrator_.parameters();
		const auto& cam = orchestrator_.camera();
		const auto obs_sph = cam.spherical_coordinates();
		const auto orientation = cam.orientation_basis();

		constants.observer_position = {0.0, obs_sph[0], obs_sph[1], obs_sph[2]};
		constants.tetrad_e1 = {0.0, orientation.forward[0], orientation.forward[1], orientation.forward[2]};
		constants.tetrad_e2 = {0.0, orientation.right[0], orientation.right[1], orientation.right[2]};
		constants.tetrad_e3 = {0.0, orientation.up[0], orientation.up[1], orientation.up[2]};
		constants.field_of_view_rad = cam.fov_deg * (std::numbers::pi / 180.0);
		constants.camera_exposure = params.camera_exposure;
		constants.metric_mass = params.mass;
		constants.metric_spin = params.spin;
		constants.metric_charge = params.charge;
		constants.horizon_radius = 2.0 * params.mass;
		constants.time = orchestrator_.scheduler().snapshot().logical_time;
		orchestrator_.apply_dark_matter_constants(constants);
		constants.disk_temperature_scale_k = params.disk_temperature_scale_k;
		constants.disk_temperature_floor_k = params.disk_temperature_floor_k;
		constants.disk_doppler_beaming_exponent = params.disk_doppler_beaming_exponent;
		constants.disk_color_saturation = params.disk_color_saturation;
		orchestrator_.apply_primary_disk_constants(constants);

		constexpr double kUnboundedRenderDistance = 1.0e7;
		const double configured_distance = (params.render_distance_scale > 0.0) ? (params.render_distance_scale * params.mass) : kUnboundedRenderDistance;
		constants.escape_radius = std::max(configured_distance, obs_sph[0] * 2.0);
		return constants;
	}

	[[nodiscard]] std::vector<Render::GpuBodyData> collect_capture_bodies() const {
		std::vector<Render::GpuBodyData> bodies;
		std::lock_guard<std::recursive_mutex> body_lock(orchestrator_.nbody_system().bodies_mutex());
		const auto& nbody_sys = orchestrator_.nbody_system().bodies();
		bodies.reserve(nbody_sys.size());
		for (const auto& b : nbody_sys) {
			if (b.enabled) {
				bodies.push_back(orchestrator_.make_gpu_body_data(b));
			}
		}
		return bodies;
	}

	void render_capture_overlay(const ImVec2& avail) noexcept {
		const auto& progress = capture_coordinator_->progress();
		const float panel_w = std::min(360.0f, std::max(avail.x - 40.0f, 120.0f));
		const float panel_h = 22.0f;
		const ImVec2 window_pos = ImGui::GetWindowPos();
		const ImVec2 top_left(window_pos.x + (avail.x - panel_w) * 0.5f, window_pos.y + avail.y - panel_h - 16.0f);
		const float fraction = std::clamp(static_cast<float>(progress.fraction()), 0.0f, 1.0f);
		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		draw_list->AddRectFilled(top_left, ImVec2(top_left.x + panel_w, top_left.y + panel_h), IM_COL32(15, 15, 25, 210), 5.0f);
		draw_list->AddRectFilled(top_left, ImVec2(top_left.x + panel_w * fraction, top_left.y + panel_h), IM_COL32(60, 170, 255, 200), 5.0f);
		char text[96];
		std::snprintf(text, sizeof(text), "%s  %.0f%%", Capture::capture_phase_name(progress.phase()), static_cast<double>(fraction) * 100.0);
		draw_list->AddText(ImVec2(top_left.x + 8.0f, top_left.y + 3.0f), IM_COL32(235, 245, 255, 255), text);
	}

	[[nodiscard]] bool is_hovered() const noexcept {
		return is_hovered_;
	}

	[[nodiscard]] Render::GeodesicComputePipeline& pipeline_ref() noexcept {
		return pipeline_;
	}

	void configure_ray_probe(bool active, int source, bool frozen) noexcept {
		ray_probe_active_ = active;
		ray_probe_source_ = source;
		ray_probe_frozen_ = frozen;
	}

	[[nodiscard]] const Optics::RayProbeResult& ray_probe_result() const noexcept {
		return ray_probe_result_;
	}

	void set_linked_readouts(const HudLinkedReadouts& readouts) noexcept {
		linked_readouts_ = readouts;
	}

	[[nodiscard]] bool capture_intensity_image(Interferometry::IntensityImage& image, uint32_t size) const {
		const uint32_t frame_w = display_frame_width_;
		const uint32_t frame_h = display_frame_height_;
		if (size < 8U || frame_w == 0U || frame_h == 0U || display_framebuffer_.size() < static_cast<size_t>(frame_w) * static_cast<size_t>(frame_h)) {
			return false;
		}
		static const std::array<double, 4096> decode_table = [] {
			std::array<double, 4096> table{};
			for (size_t i = 0; i < table.size(); ++i) {
				const double encoded = static_cast<double>(i) / 4095.0;
				table[i] = (encoded <= 0.04045) ? (encoded / 12.92) : std::pow((encoded + 0.055) / 1.055, 2.4);
			}
			return table;
		}();
		const auto linearize = [](float value) noexcept -> size_t {
			return static_cast<size_t>(std::clamp(value, 0.0f, 1.0f) * 4095.0f + 0.5f);
		};

		const uint32_t side = std::min(frame_w, frame_h);
		const uint32_t origin_x = (frame_w - side) / 2U;
		const uint32_t origin_y = (frame_h - side) / 2U;
		image.size = size;
		image.pixels.assign(static_cast<size_t>(size) * size, 0.0);
		for (uint32_t j = 0; j < size; ++j) {
			const uint32_t y_begin = origin_y + static_cast<uint32_t>(static_cast<uint64_t>(j) * side / size);
			const uint32_t y_end = std::min(origin_y + side, std::max(y_begin + 1U, origin_y + static_cast<uint32_t>(static_cast<uint64_t>(j + 1U) * side / size)));
			for (uint32_t i = 0; i < size; ++i) {
				const uint32_t x_begin = origin_x + static_cast<uint32_t>(static_cast<uint64_t>(i) * side / size);
				const uint32_t x_end = std::min(origin_x + side, std::max(x_begin + 1U, origin_x + static_cast<uint32_t>(static_cast<uint64_t>(i + 1U) * side / size)));
				double sum = 0.0;
				uint32_t samples = 0;
				for (uint32_t y = y_begin; y < y_end; ++y) {
					for (uint32_t x = x_begin; x < x_end; ++x) {
						const auto& px = display_framebuffer_[static_cast<size_t>(y) * frame_w + x];
						sum += 0.2126 * decode_table[linearize(px.r)] + 0.7152 * decode_table[linearize(px.g)] + 0.0722 * decode_table[linearize(px.b)];
						++samples;
					}
				}
				image.pixels[static_cast<size_t>(j) * size + i] = (samples > 0U) ? (sum / static_cast<double>(samples)) : 0.0;
			}
		}

		const auto& cam = orchestrator_.camera();
		const auto& params = orchestrator_.parameters();
		const double mass = std::max(params.mass, 1e-9);
		const double fov_rad = cam.fov_deg * (std::numbers::pi / 180.0);
		const double vertical_extent = (params.projection_mode <= 1U) ? (2.0 * std::tan(0.5 * fov_rad)) : fov_rad;
		const double extent = vertical_extent * static_cast<double>(side) / static_cast<double>(frame_h);
		image.observer_radius = cam.radius;
		image.simulation_pixel_scale_rad = cam.radius * extent / mass / static_cast<double>(size);
		return true;
	}

	[[nodiscard]] Optics::RayProbeQuery build_ray_probe_query(uint32_t pixel_x, uint32_t pixel_y, uint32_t frame_w, uint32_t frame_h, double escape_radius, const Render::GpuDiskProfile& disk_profile) const {
		constexpr uint32_t kProbeMaxSteps = 16384U;
		const auto& params = orchestrator_.parameters();
		const auto& cam = orchestrator_.camera();
		const auto spherical = cam.spherical_coordinates();
		const auto basis = cam.orientation_basis();
		const double aspect = static_cast<double>(frame_w) / static_cast<double>(frame_h);
		const double v_norm = 1.0 - (static_cast<double>(pixel_y) + 0.5) / static_cast<double>(frame_h) * 2.0;
		const double u_raw = (static_cast<double>(pixel_x) + 0.5) / static_cast<double>(frame_w) * 2.0 - 1.0;
		const bool all_sky = (params.projection_mode == 3U || params.projection_mode == 7U);
		const auto n_local = Observer::CameraProjector<double>::compute_ray_direction(
			static_cast<Observer::ProjectionMode>(params.projection_mode),
			all_sky ? u_raw : (u_raw * aspect), v_norm, cam.fov_deg * (std::numbers::pi / 180.0)
		);

		Optics::RayProbeQuery query;
		query.metric_type = get_metric_id_from_name(orchestrator_.active_metric_name());
		query.mass = params.mass;
		query.spin = params.spin;
		query.charge = params.charge;
		query.cosmological_lambda = params.cosmological_lambda;
		query.observer_radius = spherical[0];
		query.observer_theta = spherical[1];
		query.observer_phi = spherical[2];
		for (size_t i = 0; i < 3; ++i) {
			query.direction[i] = n_local[0] * basis.forward[i] + n_local[2] * basis.right[i] + n_local[1] * basis.up[i];
		}
		query.escape_radius = escape_radius;
		query.max_steps = std::min(params.max_ray_steps, kProbeMaxSteps);
		query.step_size_factor = params.integration_step_factor;
		query.min_step_size = params.integration_min_step;
		query.max_step_size = params.integration_max_step;
		query.far_field_step_scale = params.far_field_step_scale;
		query.pole_guard_precision_scale = params.pole_guard_precision_scale;
		query.disk_enabled = disk_profile.enabled > 0.5f;
		query.disk_inner_radius_scale = disk_profile.inner_radius_scale;
		query.disk_outer_radius_mass_units = disk_profile.outer_radius_mass_units;
		query.disk_peak_temperature_k = disk_profile.peak_temperature_k;
		query.disk_floor_temperature_k = disk_profile.floor_temperature_k;
		query.disk_temperature_exponent = disk_profile.temperature_exponent;
		query.disk_zero_torque_strength = disk_profile.zero_torque_strength;
		query.disk_temperature_normalization = disk_profile.temperature_normalization;
		query.pixel_x = pixel_x;
		query.pixel_y = pixel_y;
		return query;
	}

	void update_cursor_state(bool image_hovered, const ImVec2& image_pos, const ImVec2& avail, const ImVec2& uv0, const ImVec2& uv1, double escape_radius, const Render::GpuDiskProfile& disk_profile) {
		cursor_inside_ = false;
		const auto& style = hud_layout_.element(HudElementId::CursorReadout);
		if (!image_hovered || !hud_layout_.master_enabled || !style.enabled || avail.x < 1.0f || avail.y < 1.0f) {
			return;
		}
		const uint32_t frame_w = (display_frame_width_ > 0U) ? display_frame_width_ : current_width_;
		const uint32_t frame_h = (display_frame_height_ > 0U) ? display_frame_height_ : current_height_;
		if (frame_w == 0U || frame_h == 0U) {
			return;
		}
		const ImVec2 mouse = ImGui::GetMousePos();
		const float fx = std::clamp((mouse.x - image_pos.x) / avail.x, 0.0f, 1.0f);
		const float fy = std::clamp((mouse.y - image_pos.y) / avail.y, 0.0f, 1.0f);
		cursor_viewport_position_ = ImVec2(mouse.x - image_pos.x, mouse.y - image_pos.y);
		cursor_uv_ = ImVec2(uv0.x + fx * (uv1.x - uv0.x), uv0.y + fy * (uv1.y - uv0.y));
		cursor_pixel_x_ = std::min(frame_w - 1U, static_cast<uint32_t>(std::clamp(cursor_uv_.x, 0.0f, 1.0f) * static_cast<float>(frame_w)));
		cursor_pixel_y_ = std::min(frame_h - 1U, static_cast<uint32_t>(std::clamp(cursor_uv_.y, 0.0f, 1.0f) * static_cast<float>(frame_h)));
		cursor_inside_ = true;

		const Optics::RayProbeQuery query = build_ray_probe_query(cursor_pixel_x_, cursor_pixel_y_, frame_w, frame_h, escape_radius, disk_profile);
		if (cursor_probe_has_query_ && query == cursor_probe_query_) {
			return;
		}
		cursor_probe_query_ = query;
		cursor_probe_has_query_ = true;
		cursor_probe_result_ = Optics::GeodesicRayProbe::trace(query);
	}

	void update_ray_probe(bool image_hovered, const ImVec2& image_pos, const ImVec2& avail, const ImVec2& uv0, const ImVec2& uv1, double escape_radius, const Render::GpuDiskProfile& disk_profile) {
		if (!ray_probe_active_ || ray_probe_frozen_ || avail.x < 1.0f || avail.y < 1.0f) {
			return;
		}
		if (ray_probe_source_ == 1) {
			ray_probe_texture_position_ = ImVec2(0.5f * (uv0.x + uv1.x), 0.5f * (uv0.y + uv1.y));
		} else if (image_hovered) {
			const ImVec2 mouse = ImGui::GetMousePos();
			const float fx = std::clamp((mouse.x - image_pos.x) / avail.x, 0.0f, 1.0f);
			const float fy = std::clamp((mouse.y - image_pos.y) / avail.y, 0.0f, 1.0f);
			ray_probe_texture_position_ = ImVec2(uv0.x + fx * (uv1.x - uv0.x), uv0.y + fy * (uv1.y - uv0.y));
		} else if (ray_probe_has_query_) {
			return;
		} else {
			ray_probe_texture_position_ = ImVec2(0.5f * (uv0.x + uv1.x), 0.5f * (uv0.y + uv1.y));
		}

		const uint32_t frame_w = (display_frame_width_ > 0U) ? display_frame_width_ : current_width_;
		const uint32_t frame_h = (display_frame_height_ > 0U) ? display_frame_height_ : current_height_;
		if (frame_w == 0U || frame_h == 0U) {
			return;
		}
		const uint32_t pixel_x = std::min(frame_w - 1U, static_cast<uint32_t>(std::clamp(ray_probe_texture_position_.x, 0.0f, 1.0f) * static_cast<float>(frame_w)));
		const uint32_t pixel_y = std::min(frame_h - 1U, static_cast<uint32_t>(std::clamp(ray_probe_texture_position_.y, 0.0f, 1.0f) * static_cast<float>(frame_h)));

		const Optics::RayProbeQuery query = build_ray_probe_query(pixel_x, pixel_y, frame_w, frame_h, escape_radius, disk_profile);

		if (ray_probe_has_query_ && query == ray_probe_query_) {
			return;
		}
		ray_probe_query_ = query;
		ray_probe_has_query_ = true;
		ray_probe_result_ = Optics::GeodesicRayProbe::trace(query);
	}

	void draw_ray_probe_marker(ImDrawList* draw_list, const ImVec2& image_pos, const ImVec2& avail, const ImVec2& uv0, const ImVec2& uv1) const {
		if (!ray_probe_active_ || !ray_probe_result_.valid) {
			return;
		}
		const float span_x = uv1.x - uv0.x;
		const float span_y = uv1.y - uv0.y;
		if (span_x <= 0.0f || span_y <= 0.0f) {
			return;
		}
		const float fx = (ray_probe_texture_position_.x - uv0.x) / span_x;
		const float fy = (ray_probe_texture_position_.y - uv0.y) / span_y;
		if (fx < 0.0f || fx > 1.0f || fy < 0.0f || fy > 1.0f) {
			return;
		}
		const ImVec2 center(image_pos.x + fx * avail.x, image_pos.y + fy * avail.y);
		ImU32 color = IM_COL32(120, 220, 255, 235);
		if (ray_probe_result_.disk_hit) color = IM_COL32(255, 180, 60, 235);
		else if (ray_probe_result_.termination == Optics::RayTermination::HorizonAbsorbed) color = IM_COL32(255, 80, 70, 235);
		draw_list->AddCircle(center, 9.0f, color, 24, 1.8f);
		draw_list->AddLine(ImVec2(center.x - 15.0f, center.y), ImVec2(center.x - 5.0f, center.y), color, 1.6f);
		draw_list->AddLine(ImVec2(center.x + 5.0f, center.y), ImVec2(center.x + 15.0f, center.y), color, 1.6f);
		draw_list->AddLine(ImVec2(center.x, center.y - 15.0f), ImVec2(center.x, center.y - 5.0f), color, 1.6f);
		draw_list->AddLine(ImVec2(center.x, center.y + 5.0f), ImVec2(center.x, center.y + 15.0f), color, 1.6f);
		char label[64];
		std::snprintf(label, sizeof(label), "g=%.4f%s", ray_probe_result_.spectral_shift_g, ray_probe_frozen_ ? " [frozen]" : "");
		draw_list->AddText(ImVec2(center.x + 14.0f, center.y + 10.0f), color, label);
	}

	void render(GLFWwindow* window, double dt, bool fullscreen_bg) {
		const auto frame_render_start_ = std::chrono::steady_clock::now();
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
		viewport_window_origin_ = ImGui::GetWindowPos();
		viewport_window_extent_ = ImGui::GetWindowSize();

			const auto& params = orchestrator_.parameters();
			resolution_scale_ = static_cast<float>(params.resolution_scale);
			if (params.use_gpu_compute && !pipeline_.gpu_compute_available()) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::UseGpuCompute, 0.0)));
			}
			pipeline_.set_gpu_compute_enabled(params.use_gpu_compute);

			const auto& active_keybinds = camera_controller_.config().keybinds;
			const bool is_navigating = (is_hovered_ || is_focused_) && (
				camera_controller_.is_in_motion() ||
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

			if (params.dynamic_resolution_enabled) {
				const double target_frame_ms = 1000.0 / std::max(params.dynamic_resolution_target_fps, 1.0);
				const double last_frame_ms = pipeline_.telemetry().execution_time_ms;
				if (last_frame_ms > target_frame_ms * 1.08) {
					dynamic_resolution_multiplier_ = std::max(dynamic_resolution_multiplier_ * 0.94f, 0.25f);
				} else if (last_frame_ms > 0.0 && last_frame_ms < target_frame_ms * 0.82) {
					dynamic_resolution_multiplier_ = std::min(dynamic_resolution_multiplier_ * 1.03f, 1.0f);
				}
			} else {
				dynamic_resolution_multiplier_ = 1.0f;
			}
			active_scale = std::clamp(active_scale * dynamic_resolution_multiplier_, 0.1f, 2.0f);
			if (capture_coordinator_->locks_live_resolution()) {
				active_scale = std::clamp(resolution_scale_ * capture_coordinator_->live_resolution_multiplier(), 0.1f, 4.0f);
			}

			const ImVec2 avail = ImGui::GetContentRegionAvail();
			viewport_content_size_ = avail;
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

			std::unique_lock<std::recursive_mutex> body_snapshot_lock(orchestrator_.nbody_system().bodies_mutex());
			if (!capture_coordinator_->drives_camera()) {
				if (is_hovered_ || is_focused_) {
					const auto camera_update_stage_timer = orchestrator_.profiler().scoped_stage(Orchestrator::ProfilerTaskStage::CameraUpdate);
					camera_controller_.update(window, dt, is_hovered_);
				} else {
					camera_controller_.follow_active_target(dt);
				}
			} else {
				camera_controller_.reset_follow_state();
			}

			const auto& cam = orchestrator_.camera();
			const auto obs_sph = cam.spherical_coordinates();

			if (params.schematic_mode_enabled) {
				cursor_inside_ = false;
				const ImVec2 schematic_pos = ImGui::GetCursorScreenPos();
				const auto schematic_projection_mode = schematic_cfg_.human_perspective_mode
					? Observer::ProjectionMode::Pinhole
					: schematic_cfg_.projection_mode;
				{
					const auto schematic_stage_timer = orchestrator_.profiler().scoped_stage(Orchestrator::ProfilerTaskStage::SchematicOverlay);
					schematic_renderer_.configure(cam, schematic_projection_mode, cam.fov_deg * (std::numbers::pi / 180.0), schematic_pos, avail);
					if (schematic_cfg_.show_central_object) {
						const double primary_horizon = Observer::CameraCollisionField::primary_horizon_radius(params, orchestrator_.active_metric_name());
						const SchematicPrimarySourceOverlay::ViewRegion primary_region{schematic_pos, avail, schematic_projection_mode, cam.fov_deg * (std::numbers::pi / 180.0)};
						schematic_renderer_.set_primary_source_painter(primary_horizon, [this, primary_region](ImDrawList* list) {
							SchematicPrimarySourceOverlay::draw(list, orchestrator_, schematic_cfg_, primary_region);
						});
					}
					schematic_renderer_.render(ImGui::GetWindowDrawList(), orchestrator_, schematic_cfg_);
					schematic_renderer_.clear_primary_source_painter();
				}
				if (spatial_cfg_.show_in_schematic_view) {
					render_spatial_reference(schematic_pos, avail, schematic_projection_mode, 0.0);
				}
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
			{
			const auto camera_constants_stage_timer = orchestrator_.profiler().scoped_stage(Orchestrator::ProfilerTaskStage::CameraConstantsBuild);
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
				cam_consts.escape_radius = std::max(configured_distance, obs_sph[0] * 2.0);
			}
			cam_consts.projection_mode = params.projection_mode;
			cam_consts.max_integration_steps = params.max_ray_steps;
			cam_consts.step_size_factor = params.integration_step_factor;
			cam_consts.max_step_size = params.integration_max_step;
			cam_consts.min_step_size = std::min(params.integration_min_step, params.integration_max_step);
			cam_consts.render_flags = params.visual_overlays_flags;
			if (params.lod_enabled) {
				cam_consts.render_flags |= Render::RenderFlags::USE_LOD_SYSTEM;
			}
			if (params.adaptive_tile_prepass_enabled) {
				cam_consts.render_flags |= Render::RenderFlags::ADAPTIVE_TILE_PREPASS;
			}
			cam_consts.lod_distance_threshold = params.lod_distance_scale * params.mass;
			cam_consts.lod_reduced_steps = params.lod_reduced_ray_steps;
			if (params.space_skipping_enabled) {
				cam_consts.render_flags |= Render::RenderFlags::SPACE_SKIP_ENABLED;
			}
			cam_consts.space_skip_radius_scale = params.space_skip_radius_scale;
			cam_consts.pole_guard_precision_scale = params.pole_guard_precision_scale;
			cam_consts.far_field_step_scale = params.far_field_step_scale;
			cam_consts.body_atmosphere_global_intensity = params.body_atmosphere_global_intensity;
			cam_consts.body_render_lod_pixel_threshold = params.body_render_lod_pixel_threshold;
			cam_consts.body_render_low_power_mode = params.body_render_low_power_mode ? 1U : 0U;
			cam_consts.body_noise_octaves = params.body_noise_octaves;
			cam_consts.body_render_point_pixel_threshold = params.body_render_point_pixel_threshold;
			if (params.body_shadows_enabled) {
				cam_consts.render_flags |= Render::RenderFlags::ENABLE_BODY_SHADOWS;
			}
			if (params.bodies_only_render_mode) {
				cam_consts.render_flags |= Render::RenderFlags::BODIES_ONLY_MODE;
			}
			if (params.body_disk_occlusion_enabled) {
				cam_consts.render_flags |= Render::RenderFlags::ENABLE_BODY_DISK_OCCLUSION;
			}
			cam_consts.interlace_mode = params.interlace_rendering_enabled ? 1U : 0U;
			cam_consts.interlace_phase = interlace_phase_;
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
			cam_consts.sky_star_brightness_variation = params.sky_star_brightness_variation;
			cam_consts.sky_star_size_variation = params.sky_star_size_variation;
			cam_consts.sky_star_color_variation = params.sky_star_color_variation;
			cam_consts.sky_star_temperature_bias = params.sky_star_temperature_bias;
			cam_consts.sky_procedural_seed = params.sky_procedural_seed;
			cam_consts.sky_background_source = params.sky_background_source;
			cam_consts.sky_panorama_id = params.sky_panorama_id;
			cam_consts.sky_panorama_quality = params.sky_panorama_quality;
			cam_consts.sky_galaxy_density = params.sky_galaxy_density;
			cam_consts.sky_galaxy_brightness = params.sky_galaxy_brightness;
			cam_consts.sky_galaxy_size_scale = params.sky_galaxy_size_scale;
			cam_consts.sky_dust_density = params.sky_dust_density;
			cam_consts.sky_dust_intensity = params.sky_dust_intensity;
			cam_consts.sky_dust_scale = params.sky_dust_scale;
			cam_consts.sky_cluster_density = params.sky_cluster_density;
			cam_consts.sky_cluster_brightness = params.sky_cluster_brightness;
			cam_consts.sky_cluster_size_scale = params.sky_cluster_size_scale;
			cam_consts.disk_temperature_scale_k = params.disk_temperature_scale_k;
			cam_consts.disk_temperature_floor_k = params.disk_temperature_floor_k;
			cam_consts.disk_doppler_beaming_exponent = params.disk_doppler_beaming_exponent;
			cam_consts.disk_color_saturation = params.disk_color_saturation;
			cam_consts.observer_position = {0.0, obs_sph[0], obs_sph[1], obs_sph[2]};
			orchestrator_.apply_lighting_constants(cam_consts);
			orchestrator_.apply_primary_disk_constants(cam_consts);

			const auto orientation = cam.orientation_basis();

			cam_consts.tetrad_e0 = {1.0, 0.0, 0.0, 0.0};
			cam_consts.tetrad_e1 = {0.0, orientation.forward[0], orientation.forward[1], orientation.forward[2]};
			cam_consts.tetrad_e2 = {0.0, orientation.right[0], orientation.right[1], orientation.right[2]};
			cam_consts.tetrad_e3 = {0.0, orientation.up[0], orientation.up[1], orientation.up[2]};
			cam_consts.time = orchestrator_.scheduler().snapshot().logical_time;
			orchestrator_.apply_dark_matter_constants(cam_consts);
			}

			uint32_t total_enabled_bodies_this_frame = 0;
			std::vector<Render::GpuBodyData> gpu_bodies;
			if ((params.visual_overlays_flags & Render::RenderFlags::ENABLE_3D_BODY_RAYTRACING) != 0U) {
				const auto& nbody_sys = orchestrator_.nbody_system().bodies();
				gpu_bodies.reserve(nbody_sys.size());
				const bool lensing_present = params.mass > 0.0 && cam_consts.metric_type != 0U;
				const bool perspective_view = (params.projection_mode == 0U || params.projection_mode == 1U);
				const bool lighting_needs_all_bodies = params.body_shadows_enabled
					|| params.body_emission_lighting_enabled
					|| params.light_source_mode == static_cast<uint32_t>(Render::LightSourceMode::NearestEmissiveBody)
					|| params.light_source_mode == static_cast<uint32_t>(Render::LightSourceMode::SpecificBody);
				const bool cone_culling_enabled = perspective_view && !lensing_present && !lighting_needs_all_bodies;
				const double frame_aspect = static_cast<double>(current_width_) / static_cast<double>(std::max(current_height_, 1U));
				const double frustum_half_angle = std::atan(std::tan(cam_consts.field_of_view_rad * 0.5) * std::sqrt(1.0 + frame_aspect * frame_aspect));
				for (const auto& b : nbody_sys) {
					if (!b.enabled) continue;
					++total_enabled_bodies_this_frame;
					const double dx = b.position[0] - cam.position[0];
					const double dy = b.position[1] - cam.position[1];
					const double dz = b.position[2] - cam.position[2];
					const double dist = std::sqrt(dx * dx + dy * dy + dz * dz);
					if (dist > cam_consts.escape_radius) continue;
					if (cone_culling_enabled && dist > 1e-9) {
						const double cos_angle = std::clamp((dx * cam_consts.tetrad_e1[1] + dy * cam_consts.tetrad_e1[2] + dz * cam_consts.tetrad_e1[3]) / dist, -1.0, 1.0);
						const double angular_radius = std::asin(std::clamp(b.radius / dist, 0.0, 1.0));
						if (std::acos(cos_angle) > frustum_half_angle + angular_radius) continue;
					}
					gpu_bodies.push_back(orchestrator_.make_gpu_body_data(b));
				}
			}
			cam_consts.body_count = static_cast<uint32_t>(gpu_bodies.size());

			const bool is_surface_walking_motion = (params.camera_mode == static_cast<uint32_t>(CameraNavigationMode::SurfaceWalk)) && camera_controller_.is_in_motion();
			const bool walk_refresh_suppressed = is_surface_walking_motion && !params.camera_motion_live_refresh;

			const double precision_selector = orchestrator_.get_custom_param("precision_mode", 0.0);
			const bool precision_changed = (precision_selector != last_precision_selector_);

			const uint64_t current_ver = orchestrator_.state_version();
			if (current_ver != last_synced_version_) {
				if (!walk_refresh_suppressed) {
					force_rerender_ = true;
				}
				last_synced_version_ = current_ver;
			}
			const uint64_t earth_texture_revision = Optics::EarthTextureLoader::instance().revision();
			if (earth_texture_revision != last_earth_texture_revision_) {
				force_rerender_ = true;
				last_earth_texture_revision_ = earth_texture_revision;
			}
			const auto snap = orchestrator_.scheduler().snapshot();
			const bool is_time_progressing = !snap.is_paused || snap.remaining_steps > 0;
			const bool time_changed = (snap.logical_time != last_logical_time_);
			Render::GpuCameraPushConstants comparable_consts = cam_consts;
			comparable_consts.time = last_camera_constants_.time;
			const bool params_changed = !(comparable_consts == last_camera_constants_);
			const double dispatch_clock = ImGui::GetTime();
			const double minimum_time_interval = std::clamp(pipeline_.telemetry().execution_time_ms * 0.0012, 1.0 / 60.0, 0.25);
			const bool time_refresh_due = is_time_progressing && time_changed && (is_navigating || !pipeline_.is_rendering()) && (dispatch_clock - last_dispatch_clock_) >= minimum_time_interval;
			const bool camera_pose_changed = comparable_consts.observer_position != last_camera_constants_.observer_position
				|| comparable_consts.tetrad_e1 != last_camera_constants_.tetrad_e1
				|| comparable_consts.tetrad_e2 != last_camera_constants_.tetrad_e2
				|| comparable_consts.tetrad_e3 != last_camera_constants_.tetrad_e3
				|| comparable_consts.field_of_view_rad != last_camera_constants_.field_of_view_rad;
			const bool preserve_in_flight_render = params.camera_motion_live_refresh
				&& (is_navigating || camera_pose_changed || camera_controller_.is_in_motion());
			const bool is_dirty = (!walk_refresh_suppressed && (force_rerender_ || params_changed)) || precision_changed || time_refresh_due;

			if (is_dirty && !capture_coordinator_->suppresses_live_render()) {
				pipeline_.set_precision_mode(precision_selector > 0.5 ? Render::PrecisionMode::DoubleSingleEmulation : Render::PrecisionMode::NativeFloat64);
				pipeline_.set_projection_mode(static_cast<Observer::ProjectionMode>(params.projection_mode));
				pipeline_.dispatch(cam_consts, gpu_bodies, total_enabled_bodies_this_frame, preserve_in_flight_render);
				last_camera_constants_ = cam_consts;
				last_dispatch_clock_ = dispatch_clock;
				capture_constants_valid_ = true;
				last_logical_time_ = snap.logical_time;
				last_precision_selector_ = precision_selector;
				force_rerender_ = false;
				if (params.interlace_rendering_enabled) {
					interlace_phase_ ^= 1U;
				}
			}

			uint32_t fb_w = 0, fb_h = 0;
			bool got_new_frame = false;
			{
				const auto framebuffer_readback_stage_timer = orchestrator_.profiler().scoped_stage(Orchestrator::ProfilerTaskStage::FramebufferReadback);
				got_new_frame = pipeline_.swap_display_framebuffer(display_framebuffer_, fb_w, fb_h);
			}
			if (got_new_frame) {
				const auto& fb = display_framebuffer_;
				display_frame_width_ = fb_w;
				display_frame_height_ = fb_h;
				const size_t pixel_count = static_cast<size_t>(fb_w) * static_cast<size_t>(fb_h);
				if (pixel_count > 0 && fb.size() >= pixel_count) {
					const auto post_processing_stage_timer = orchestrator_.profiler().scoped_stage(Orchestrator::ProfilerTaskStage::PostProcessing);
					if (color_upload_buffer_.size() < pixel_count * 4) {
						color_upload_buffer_.assign(pixel_count * 4, 0.0f);
					}
					const auto& grading = orchestrator_.parameters();
					const float grade_contrast = static_cast<float>(grading.post_contrast);
					const float grade_saturation = static_cast<float>(grading.post_saturation);
					const float grade_lift = static_cast<float>(grading.post_lift);
					const float grade_inv_gamma = 1.0f / static_cast<float>(std::max(grading.post_gamma, 0.01));
					const float grade_gain = static_cast<float>(grading.post_gain);
					const float grade_highlights = static_cast<float>(grading.post_highlights);
					const float grade_shadows = static_cast<float>(grading.post_shadows);
					const float grade_vignette = static_cast<float>(grading.post_vignette_strength);
					const bool needs_grading = (grade_contrast != 1.0f) || (grade_saturation != 1.0f) || (grade_lift != 0.0f) || (grade_inv_gamma != 1.0f) || (grade_gain != 1.0f) || (grade_highlights != 0.0f) || (grade_shadows != 0.0f) || (grade_vignette > 0.0f);
					if (!needs_grading) {
						for (size_t i = 0; i < pixel_count; ++i) {
							color_upload_buffer_[i * 4 + 0] = fb[i].r;
							color_upload_buffer_[i * 4 + 1] = fb[i].g;
							color_upload_buffer_[i * 4 + 2] = fb[i].b;
							color_upload_buffer_[i * 4 + 3] = fb[i].a;
						}
					} else {
						const float grade_inv_w = (fb_w > 0) ? (1.0f / static_cast<float>(fb_w)) : 0.0f;
						const float grade_inv_h = (fb_h > 0) ? (1.0f / static_cast<float>(fb_h)) : 0.0f;
						auto grade_channel = [&](float c) noexcept -> float {
							c = std::clamp(c + grade_lift * (1.0f - c), 0.0f, 4.0f);
							c = (c - 0.5f) * grade_contrast + 0.5f;
							c = std::max(c, 0.0f);
							c = std::pow(c, grade_inv_gamma) * grade_gain;
							if (grade_highlights != 0.0f) {
								const float w = std::clamp((c - 0.6f) / 0.4f, 0.0f, 1.0f);
								c += grade_highlights * w * (1.0f - c) * 0.5f;
							}
							if (grade_shadows != 0.0f) {
								const float w = std::clamp(1.0f - c / 0.4f, 0.0f, 1.0f);
								c += grade_shadows * w * c * 0.5f;
							}
							return std::clamp(c, 0.0f, 1.0f);
						};
						for (size_t y = 0; y < fb_h; ++y) {
							const float py = (static_cast<float>(y) + 0.5f) * grade_inv_h - 0.5f;
							const float py2 = py * py;
							const size_t row_offset = y * fb_w;
							for (size_t x = 0; x < fb_w; ++x) {
								const size_t i = row_offset + x;
								float gr = grade_channel(fb[i].r);
								float gg = grade_channel(fb[i].g);
								float gb = grade_channel(fb[i].b);
								const float luma = 0.2126f * gr + 0.7152f * gg + 0.0722f * gb;
								gr = std::clamp(luma + (gr - luma) * grade_saturation, 0.0f, 1.0f);
								gg = std::clamp(luma + (gg - luma) * grade_saturation, 0.0f, 1.0f);
								gb = std::clamp(luma + (gb - luma) * grade_saturation, 0.0f, 1.0f);
								if (grade_vignette > 0.0f) {
									const float px = (static_cast<float>(x) + 0.5f) * grade_inv_w - 0.5f;
									const float dist_sq = std::min((px * px + py2) * 2.0f, 1.0f);
									const float falloff = 1.0f - grade_vignette * dist_sq;
									gr *= falloff;
									gg *= falloff;
									gb *= falloff;
								}
								color_upload_buffer_[i * 4 + 0] = gr;
								color_upload_buffer_[i * 4 + 1] = gg;
								color_upload_buffer_[i * 4 + 2] = gb;
								color_upload_buffer_[i * 4 + 3] = fb[i].a;
							}
						}
					}
					has_received_frame_ = true;
					const auto texture_upload_stage_timer = orchestrator_.profiler().scoped_stage(Orchestrator::ProfilerTaskStage::TextureUpload);
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
			const bool probe_image_hovered = ImGui::IsItemHovered();
			update_ray_probe(probe_image_hovered, viewport_image_pos, avail, zoom_uv0, zoom_uv1, cam_consts.escape_radius, cam_consts.primary_disk);
			draw_ray_probe_marker(ImGui::GetWindowDrawList(), viewport_image_pos, avail, zoom_uv0, zoom_uv1);
			update_cursor_state(probe_image_hovered, viewport_image_pos, avail, zoom_uv0, zoom_uv1, cam_consts.escape_radius, cam_consts.primary_disk);

			if (schematic_cfg_.show_overlay_in_raytraced_view) {
				const auto schematic_stage_timer = orchestrator_.profiler().scoped_stage(Orchestrator::ProfilerTaskStage::SchematicOverlay);
				const auto proj_mode = static_cast<Observer::ProjectionMode>(params.projection_mode);
				schematic_renderer_.configure(
					cam, proj_mode, cam.fov_deg * (std::numbers::pi / 180.0), viewport_image_pos, avail,
					params.mass, schematic_cfg_.lens_body_overlays_in_raytraced_view
				);
				schematic_renderer_.render_overlay(ImGui::GetWindowDrawList(), orchestrator_, schematic_cfg_);
			}

			if (spatial_cfg_.show_in_raytraced_view) {
				render_spatial_reference(viewport_image_pos, avail, static_cast<Observer::ProjectionMode>(params.projection_mode), params.mass);
			}

			if (hud_layout_.element(HudElementId::ViewportToolbar).enabled) {
				render_viewport_toolbar(avail);
			}
			if (!schematic_cfg_.show_overlay_in_raytraced_view && schematic_renderer_.has_path_preview()) {
				const auto preview_projection = static_cast<Observer::ProjectionMode>(params.projection_mode);
				schematic_renderer_.configure(
					cam, preview_projection, cam.fov_deg * (std::numbers::pi / 180.0), viewport_image_pos, avail,
					params.mass, schematic_cfg_.lens_body_overlays_in_raytraced_view
				);
				schematic_renderer_.render_path_preview_only(ImGui::GetWindowDrawList());
			}
			render_loading_indicator(avail);
			{
				const auto hud_overlay_stage_timer = orchestrator_.profiler().scoped_stage(Orchestrator::ProfilerTaskStage::HudOverlay);
				render_hud_overlay(avail, window);
			}

			{
				const auto& tel = pipeline_.telemetry();
				const uint64_t total_pixels_for_ratio = std::max<uint64_t>(tel.total_pixels_processed, uint64_t{1});
				Orchestrator::FrameSampleInput frame_input;
				frame_input.timestamp_seconds = ImGui::GetTime();
				frame_input.frame_time_ms = tel.execution_time_ms;
				frame_input.fps = tel.frame_rate_fps;
				frame_input.used_gpu_path = tel.used_gpu_path;
				frame_input.screen_width = current_width_;
				frame_input.screen_height = current_height_;
				frame_input.horizon_pixels = tel.horizon_pixels_absorbed;
				frame_input.celestial_pixels = tel.celestial_pixels_hit;
				frame_input.disk_pixels = tel.accretion_disk_pixels_hit;
				frame_input.average_iterations = tel.average_iterations_used;
				frame_input.max_iteration_ratio = static_cast<double>(tel.saturated_ray_pixels) / static_cast<double>(total_pixels_for_ratio);
				frame_input.resolution_scale = params.resolution_scale;
				frame_input.max_ray_steps = params.max_ray_steps;
				frame_input.precision_mode = static_cast<uint32_t>(orchestrator_.get_custom_param("precision_mode", 0.0));
				frame_input.performance_preset = params.performance_preset;
				frame_input.tiled_distribution = (params.visual_overlays_flags & Render::RenderFlags::USE_TILED_DISTRIBUTION) != 0U;
				frame_input.simd_pipeline = (params.visual_overlays_flags & Render::RenderFlags::USE_SCALAR_PIPELINE) == 0U;
				frame_input.gpu_compute_enabled = params.use_gpu_compute;
				frame_input.step_controller_mode = params.step_controller_mode;
				frame_input.motion_quality_mode = params.motion_quality_mode;
				frame_input.motion_quality_scale = params.motion_quality_scale;
				frame_input.space_skipping_enabled = params.space_skipping_enabled;
				frame_input.space_skip_radius_scale = params.space_skip_radius_scale;
				frame_input.pole_guard_precision_scale = params.pole_guard_precision_scale;
				frame_input.far_field_step_scale = params.far_field_step_scale;
				frame_input.lod_enabled = params.lod_enabled;
				frame_input.lod_distance_scale = params.lod_distance_scale;
				frame_input.lod_reduced_ray_steps = params.lod_reduced_ray_steps;
				frame_input.render_distance_scale = params.render_distance_scale;
				frame_input.interlace_rendering_enabled = params.interlace_rendering_enabled;
				frame_input.dynamic_resolution_enabled = params.dynamic_resolution_enabled;
				frame_input.dynamic_resolution_target_fps = params.dynamic_resolution_target_fps;
				frame_input.adaptive_tile_prepass_enabled = params.adaptive_tile_prepass_enabled;
				frame_input.rolling_average_frame_count = params.rolling_average_frame_count;
				frame_input.integration_rtol = params.integration_rtol;
				frame_input.integration_atol = params.integration_atol;
				frame_input.metric_name = orchestrator_.active_metric_name();
				frame_input.integrator_name = orchestrator_.active_integrator_name();

				const auto frame_render_end = std::chrono::steady_clock::now();
				const double frame_total_ms = std::chrono::duration<double, std::milli>(frame_render_end - frame_render_start_).count();
				orchestrator_.profiler().record_stage_duration(Orchestrator::ProfilerTaskStage::FrameTotal, frame_total_ms);
				orchestrator_.profiler().record_stage_duration(Orchestrator::ProfilerTaskStage::RenderDispatch, tel.execution_time_ms);
				orchestrator_.profiler().record_stage_duration(tel.used_gpu_path ? Orchestrator::ProfilerTaskStage::GpuDispatchExecution : Orchestrator::ProfilerTaskStage::CpuDispatchExecution, tel.execution_time_ms);
				orchestrator_.profiler().record_stage_duration(Orchestrator::ProfilerTaskStage::AdaptiveTilePrepassSky, tel.tile_prepass_skip_ms);
				orchestrator_.profiler().record_stage_duration(Orchestrator::ProfilerTaskStage::PixelClassification, tel.pixel_classification_ms);
				orchestrator_.profiler().record_frame(frame_input);
			}

			if (capture_coordinator_->is_busy()) {
				render_capture_overlay(avail);
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
			if (!b.enabled) continue;
			targets.push_back(DynamicLookAtTarget{
				(b.is_spacetime_source ? "Independent Black Hole #" : "Celestial Body #") + std::to_string(b.id) + " (M=" + std::to_string(b.mass).substr(0, 4) + ")",
				b.position,
				std::max(b.effective_radius() * 4.0, 10.0)
			});
		}

		return targets;
	}

	void render_spatial_reference(const ImVec2& origin, const ImVec2& size, Observer::ProjectionMode projection, double lensing_mass) {
		if (!spatial_cfg_.enabled) {
			return;
		}
		const auto& camera = orchestrator_.camera();
		const auto basis = camera.orientation_basis();
		const double horizon = Observer::CameraCollisionField::primary_horizon_radius(orchestrator_.parameters(), orchestrator_.active_metric_name());

		SpatialReferenceView view;
		view.camera_position = camera.position;
		view.forward = basis.forward;
		view.right = basis.right;
		view.up = basis.up;
		view.projection_mode = projection;
		view.fov_rad = camera.fov_deg * (std::numbers::pi / 180.0);
		view.rect_min = origin;
		view.rect_size = size;
		view.horizon_radius = horizon;
		view.lensing_mass = (horizon > 0.0) ? lensing_mass : 0.0;
		view.length_scale_meters = orchestrator_.constants_engine().length_scale();
		view.distance_unit = orchestrator_.unit_preferences().distance;

		if (spatial_cfg_.center_mode == SpatialCenterMode::Body) {
			std::lock_guard<std::recursive_mutex> lock(orchestrator_.nbody_system().bodies_mutex());
			for (const auto& body : orchestrator_.nbody_system().bodies()) {
				if (body.enabled && body.id == spatial_cfg_.center_body_id) {
					view.body_center = body.position;
					break;
				}
			}
		}
		spatial_renderer_.render(ImGui::GetWindowDrawList(), spatial_cfg_, view);
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
		const float toolbar_pad_scale = std::clamp(hud_layout_.toolbar_padding_scale, 0.4f, 3.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.0f * toolbar_pad_scale, 4.0f * toolbar_pad_scale));
		ImGui::BeginGroup();
		ImGui::Dummy(ImVec2(0.0f, 0.0f));
		ImGui::SameLine(0.0f, 0.0f);

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
				render_setting_tooltip("Resumes simulation clock progression.");
			} else {
				const std::string label = "Pause (" + toolbar_key_hint(InputAction::TogglePausePlay) + ")";
				if (ImGui::Button(label.c_str(), ImVec2(75.0f, 24.0f))) {
					static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_pause()));
				}
				render_setting_tooltip("Pauses simulation clock progression.");
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
			render_setting_tooltip("Advances logical simulation state by exactly 1 tick.");
			if (schematic_locked) {
				ImGui::EndDisabled();
				render_setting_tooltip_warning("Advance the simulation by one tick.", "Disabled while Schematic Orbital View is active. Enable 'Allow Simulation Clock To Run In Schematic View' in the Schematic View tab to unlock.");
			}
			ImGui::SameLine();
		}

		if (tb.reset_view) {
			if (ImGui::Button("Reset View", ImVec2(78.0f, 24.0f))) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_camera_reset()));
			}
			render_setting_tooltip("Resets camera coordinates and orientation to default front view.");
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
			if (ImGui::Combo("##AimTargetCombo", &selected_target_idx, target_labels.data(), static_cast<int>(target_labels.size()))) {
			}
			render_setting_tooltip("Select a physical landmark or orbiting celestial body to look at or jump toward.");
			ImGui::SameLine();
		}

		if (tb.look_at_target_combo) {
			if (ImGui::Button("Look At Object", ImVec2(105.0f, 24.0f)) && selected_target_idx < static_cast<int>(dynamic_targets.size())) {
				camera_controller_.look_at_target(dynamic_targets[selected_target_idx].position);
			}
			render_setting_tooltip("Reorients camera gaze vector directly toward the selected target.");
			ImGui::SameLine();
		}

		if (tb.jump_to_target) {
			if (ImGui::Button("Jump to Target", ImVec2(100.0f, 24.0f)) && selected_target_idx < static_cast<int>(dynamic_targets.size())) {
				const auto& tgt = dynamic_targets[selected_target_idx];
				auto& c = orchestrator_.camera();
				c.position = {tgt.position[0], tgt.position[1] + tgt.recommended_distance, tgt.position[2]};
				c.orbit_distance = tgt.recommended_distance;
				c.synchronize_spherical();
				camera_controller_.look_at_target(tgt.position);
			}
			render_setting_tooltip("Teleports observer to a safe framing distance and looks at target.");
			ImGui::SameLine();
		}

		if (tb.camera_mode_combo) {
			const char* cam_modes[] = {"Free Fly", "Orbit Center", "Spherical", "Rocket", "Surface Walk", "Planet Orbit"};
			int cur_mode = static_cast<int>(orchestrator_.parameters().camera_mode);
			ImGui::SetNextItemWidth(115.0f);
			if (ImGui::Combo("##CamModeCombo", &cur_mode, cam_modes, IM_ARRAYSIZE(cam_modes))) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_camera_mode(static_cast<uint32_t>(cur_mode))));
			}
			render_setting_tooltip("Switches observer navigation model between 6-DOF Free Fly, Center Orbit, Spherical, and Rocket flight.");
			ImGui::SameLine();
		}

		if (tb.hud_master_toggle) {
			ImGui::Checkbox("HUD", &hud_layout_.master_enabled);
			render_setting_tooltip("Toggles master visibility for all viewport overlay readouts and panels.");
		}

		if (tb.screenshot) {
			ImGui::SameLine();
			if (ImGui::Button("Capture", ImVec2(70.0f, 24.0f)) && open_screenshot_settings_callback_) {
				open_screenshot_settings_callback_();
			}
			render_setting_tooltip("Opens the Capture Studio to configure and take high-resolution screenshots or video sequences.");
			if (capture_coordinator_->is_busy()) {
				ImGui::SameLine();
				ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "Capturing %.0f%%", capture_coordinator_->progress().fraction() * 100.0);
			}
		}

		if (tb.fullscreen_toggle) {
			ImGui::SameLine();
			if (ImGui::Button("Fullscreen", ImVec2(86.0f, 24.0f)) && fullscreen_toggle_callback_) {
				fullscreen_toggle_callback_();
			}
			render_setting_tooltip("Toggles fullscreen borderless presentation for the primary viewport.");
		}

		if (tb.gpu_compute_toggle) {
			ImGui::SameLine();
			bool gpu_on = orchestrator_.parameters().use_gpu_compute;
			if (ImGui::Checkbox("GPU", &gpu_on)) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::UseGpuCompute, gpu_on ? 1.0 : 0.0)));
			}
			render_setting_tooltip("Toggles Vulkan GPU compute dispatch offload.");
		}

		if (tb.space_skip_toggle) {
			ImGui::SameLine();
			bool skip_on = orchestrator_.parameters().space_skipping_enabled;
			if (ImGui::Checkbox("Skip", &skip_on)) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SpaceSkippingEnabled, skip_on ? 1.0 : 0.0)));
			}
			render_setting_tooltip("Toggles asymptotic space-skipping for weak-field rays.");
		}

		if (tb.lod_toggle) {
			ImGui::SameLine();
			bool lod_on = orchestrator_.parameters().lod_enabled;
			if (ImGui::Checkbox("LOD", &lod_on)) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::LodEnabled, lod_on ? 1.0 : 0.0)));
			}
			render_setting_tooltip("Toggles distance-based level of detail step budget reduction.");
		}

		if (tb.exposure_controls) {
			ImGui::SameLine();
			if (ImGui::Button("EV-", ImVec2(34.0f, 24.0f))) {
				const double next_exposure = std::clamp(orchestrator_.parameters().camera_exposure - 0.25, -6.0, 6.0);
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::CameraExposure, next_exposure)));
			}
			render_setting_tooltip("Decreases optical exposure compensation by 0.25 EV.");
			ImGui::SameLine();
			if (ImGui::Button("EV+", ImVec2(34.0f, 24.0f))) {
				const double next_exposure = std::clamp(orchestrator_.parameters().camera_exposure + 0.25, -6.0, 6.0);
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::CameraExposure, next_exposure)));
			}
			render_setting_tooltip("Increases optical exposure compensation by 0.25 EV.");
		}

		if (tb.warp_controls) {
			ImGui::SameLine();
			if (ImGui::Button("Warp-", ImVec2(50.0f, 24.0f))) {
				const double next_warp = std::max(orchestrator_.scheduler().warp_factor() / 1.5, 0.05);
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_warp(next_warp)));
			}
			render_setting_tooltip("Slows simulation time warp rate by 1.5x.");
			ImGui::SameLine();
			if (ImGui::Button("Warp+", ImVec2(50.0f, 24.0f))) {
				const double next_warp = orchestrator_.scheduler().warp_factor() * 1.5;
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_warp(next_warp)));
			}
			render_setting_tooltip("Accelerates simulation time warp rate by 1.5x.");
		}

		if (tb.tonemapper_cycle) {
			ImGui::SameLine();
			if (ImGui::Button("Tonemap", ImVec2(78.0f, 24.0f))) {
				const uint32_t next_mode = (orchestrator_.parameters().tonemapping_mode + 1) % 4;
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::TonemappingMode, static_cast<double>(next_mode))));
			}
			render_setting_tooltip("Cycles through Linear, ACES Filmic, Logarithmic, and Reinhard HDR tonemappers.");
		}

		if (tb.projection_cycle) {
			ImGui::SameLine();
			if (ImGui::Button("Projection", ImVec2(86.0f, 24.0f))) {
				const uint32_t next_mode = (orchestrator_.parameters().projection_mode + 1) % 8;
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::ProjectionMode, static_cast<double>(next_mode))));
			}
			render_setting_tooltip("Cycles camera projection geometry (Pinhole, Fisheye, 360, Panini, Hammer-Aitoff).");
		}

		if (tb.skybox_cycle) {
			ImGui::SameLine();
			if (ImGui::Button("Skybox", ImVec2(66.0f, 24.0f))) {
				const uint32_t current_style = orchestrator_.parameters().visual_overlays_flags & Render::RenderFlags::SKYBOX_MODE_MASK;
				const uint32_t next_style = (current_style + 1) % 6;
				const uint32_t next_flags = (orchestrator_.parameters().visual_overlays_flags & ~Render::RenderFlags::SKYBOX_MODE_MASK) | next_style;
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::VisualOverlays, static_cast<double>(next_flags))));
			}
			render_setting_tooltip("Cycles celestial background modes (Stars, Grid, Composite, Void, Grid+Stars).");
		}

		if (tb.metric_cycle) {
			ImGui::SameLine();
			if (ImGui::Button("Metric", ImVec2(64.0f, 24.0f))) {
				static constexpr const char* kToolbarMetricCycle[] = {
					"Flat Minkowski", "Schwarzschild Black Hole", "Kerr Rotating Black Hole",
					"Reissner-Nordstrom Charged", "Kerr-Newman Charged Rotating",
					"Schwarzschild-de Sitter (Lambda)", "FLRW Cosmological Expansion",
					"Morris-Thorne Traversable Wormhole", "Alcubierre Warp Drive Bubble", "BSSN 3+1 Numerical Grid"
				};
				constexpr int metric_count = static_cast<int>(sizeof(kToolbarMetricCycle) / sizeof(kToolbarMetricCycle[0]));
				int current_idx = 0;
				for (int i = 0; i < metric_count; ++i) {
					if (orchestrator_.active_metric_name() == kToolbarMetricCycle[i]) {
						current_idx = i;
						break;
					}
				}
				const int next_idx = (current_idx + 1) % metric_count;
				orchestrator_.set_active_metric_name(kToolbarMetricCycle[next_idx]);
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_metric(kToolbarMetricCycle[next_idx])));
			}
			render_setting_tooltip("Cycles sequentially through all available spacetime solutions.");
		}

		if (tb.integrator_cycle) {
			ImGui::SameLine();
			if (ImGui::Button("Integrator", ImVec2(80.0f, 24.0f))) {
				static constexpr const char* kToolbarIntegratorCycle[] = {
					"Dormand-Prince RK45 (Adaptive)", "Cash-Karp 5(4) (Adaptive)", "Vernier 9(8) High-Order",
					"Symplectic Gauss-Legendre 4th", "Symplectic Gauss-Legendre 6th", "Hermite 4th-Order (Aarseth)"
				};
				constexpr int integrator_count = static_cast<int>(sizeof(kToolbarIntegratorCycle) / sizeof(kToolbarIntegratorCycle[0]));
				int current_idx = 0;
				for (int i = 0; i < integrator_count; ++i) {
					if (orchestrator_.active_integrator_name() == kToolbarIntegratorCycle[i]) {
						current_idx = i;
						break;
					}
				}
				const int next_idx = (current_idx + 1) % integrator_count;
				orchestrator_.set_active_integrator_name(kToolbarIntegratorCycle[next_idx]);
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_integrator(kToolbarIntegratorCycle[next_idx])));
			}
			render_setting_tooltip("Cycles through numerical differential equation integration schemes.");
		}

		if (tb.performance_preset_combo) {
			ImGui::SameLine();
			const char* toolbar_presets[] = {"Potato", "Perf", "Balanced", "High", "Ultra", "Extreme", "Custom"};
			int preset_idx = static_cast<int>(std::min<uint32_t>(orchestrator_.parameters().performance_preset, 6U));
			ImGui::SetNextItemWidth(90.0f);
			if (ImGui::Combo("##ToolbarPresetCombo", &preset_idx, toolbar_presets, IM_ARRAYSIZE(toolbar_presets)) && preset_idx < 6) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_performance_preset(static_cast<uint32_t>(preset_idx))));
			}
			render_setting_tooltip("Quickly applies a performance preset profile.");
		}

		if (tb.quicksave_quickload) {
			ImGui::SameLine();
			if (ImGui::Button("QSave", ImVec2(56.0f, 24.0f))) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_save_scenario("scenarios/quicksave.yaml")));
			}
			render_setting_tooltip("Quick-saves the active simulation state to scenarios/quicksave.yaml.");
			ImGui::SameLine();
			if (ImGui::Button("QLoad", ImVec2(56.0f, 24.0f))) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_load_scenario("scenarios/quicksave.yaml")));
			}
			render_setting_tooltip("Quick-loads the simulation state from scenarios/quicksave.yaml.");
		}

		if (tb.step_controller_cycle) {
			ImGui::SameLine();
			if (ImGui::Button("StepCtrl", ImVec2(74.0f, 24.0f))) {
				const uint32_t next_mode = (orchestrator_.parameters().step_controller_mode + 1) % 3;
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::StepControllerMode, static_cast<double>(next_mode))));
			}
			render_setting_tooltip("Cycles adaptive step-size controller strategy (Standard, PI-30, PID-42).");
		}

		if (tb.render_distance_toggle) {
			ImGui::SameLine();
			const bool unbounded = orchestrator_.parameters().render_distance_scale <= 0.0;
			if (ImGui::Button(unbounded ? "Dist: Inf" : "Dist: 100M", ImVec2(84.0f, 24.0f))) {
				const double next_distance = unbounded ? 100.0 : 0.0;
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::RenderDistanceScale, next_distance)));
			}
			render_setting_tooltip("Toggles between unbounded render distance and 100 M cutoff radius.");
		}

		if (tb.pole_precision_nudge) {
			ImGui::SameLine();
			if (ImGui::Button("Pole-", ImVec2(48.0f, 24.0f))) {
				const double next_val = std::clamp(orchestrator_.parameters().pole_guard_precision_scale - 0.25, 0.05, 8.0);
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::PoleGuardPrecisionScale, next_val)));
			}
			render_setting_tooltip("Reduces polar region integration damping precision scale.");
			ImGui::SameLine();
			if (ImGui::Button("Pole+", ImVec2(48.0f, 24.0f))) {
				const double next_val = std::clamp(orchestrator_.parameters().pole_guard_precision_scale + 0.25, 0.05, 8.0);
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::PoleGuardPrecisionScale, next_val)));
			}
			render_setting_tooltip("Increases polar region integration damping precision scale to eliminate axis artifacts.");
		}

		ImGui::EndGroup();
		ImGui::PopStyleVar();
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
		const Observer::CoordinateFrame coordinate_frame = make_coordinate_frame(orchestrator_);
		const double coordinate_length_scale = orchestrator_.constants_engine().length_scale();
		const HudCoordinateDisplay& coordinate_display = hud_layout_.coordinates;

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

		struct PendingHudBlock {
			const HudElementStyle* style;
			std::vector<HudTextLine> lines;
		};
		std::vector<PendingHudBlock> pending;
		pending.reserve(static_cast<size_t>(HudElementId::Count));

		auto push_block = [&](HudElementId id, std::vector<HudTextLine> lines) {
			const auto& style = hud_layout_.element(id);
			if (!style.enabled || lines.empty()) return;
			pending.push_back(PendingHudBlock{&style, std::move(lines)});
		};

		{
			const auto& ft_style = hud_layout_.element(HudElementId::FrameTimeReadout);
			char buf[192];
			const double instant_fps = (current_frame_time_ms_ > 0.0) ? (1000.0 / current_frame_time_ms_) : 0.0;
			const double effective_fps = has_sufficient_rolling_frames_ ? (1000.0 / rolling_average_time_ms_) : instant_fps;
			const int prec = std::clamp(ft_style.decimal_precision, 0, 6);
			const auto frame_rate_unit_pref = orchestrator_.unit_preferences().frame_rate;
			if (ft_style.display_mode == HudDisplayMode::Compact) {
				const std::string fps_display = Units::format_frame_time(has_sufficient_rolling_frames_ ? rolling_average_time_ms_ : current_frame_time_ms_, frame_rate_unit_pref, prec);
				std::snprintf(buf, sizeof(buf), "%s", fps_display.c_str());
			} else if (ft_style.display_mode == HudDisplayMode::Extended) {
				if (has_sufficient_rolling_frames_) {
					std::snprintf(buf, sizeof(buf), "Frame Time: %.*f ms | Instant: %.1f FPS | Avg[%u]: %.*f ms (%.1f FPS) | Samples: %zu", prec, current_frame_time_ms_, instant_fps, static_cast<unsigned int>(target_samples), prec, rolling_average_time_ms_, 1000.0 / rolling_average_time_ms_, frame_times_history_.size());
				} else {
					std::snprintf(buf, sizeof(buf), "Frame Time: %.*f ms | Instant: %.1f FPS | Avg: warming up %zu/%u", prec, current_frame_time_ms_, instant_fps, frame_times_history_.size(), static_cast<unsigned int>(target_samples));
				}
			} else {
				if (has_sufficient_rolling_frames_) {
					std::snprintf(buf, sizeof(buf), "Frame Time: %.*f ms (Avg[%u]: %.*f ms | %.1f FPS)", prec, current_frame_time_ms_, static_cast<unsigned int>(target_samples), prec, rolling_average_time_ms_, 1000.0 / rolling_average_time_ms_);
				} else {
					std::snprintf(buf, sizeof(buf), "Frame Time: %.*f ms (Avg: warming up %zu/%u...)", prec, current_frame_time_ms_, frame_times_history_.size(), static_cast<unsigned int>(target_samples));
				}
			}
			const ImU32 base_col = ImGui::ColorConvertFloat4ToU32(ImVec4(ft_style.text_color[0], ft_style.text_color[1], ft_style.text_color[2], ft_style.text_color[3]));
			const ImU32 dyn_col = hud_resolve_dynamic_color(ft_style, effective_fps, base_col);
			push_block(HudElementId::FrameTimeReadout, {HudTextLine{buf, dyn_col}});
		}

		{
			const auto& style = hud_layout_.element(HudElementId::CameraDistanceReadout);
			char buf[160];
			const int prec = std::clamp(style.decimal_precision, 0, 6);
			const auto& unit_prefs = orchestrator_.unit_preferences();
			const double meters_per_geo_unit = orchestrator_.constants_engine().length_scale();
			const std::string radius_display = Units::format_distance(cam.radius * meters_per_geo_unit, unit_prefs.distance, prec);
			if (style.display_mode == HudDisplayMode::Compact) {
				std::snprintf(buf, sizeof(buf), "r=%s", radius_display.c_str());
			} else if (style.display_mode == HudDisplayMode::Extended) {
				const std::string orbit_display = Units::format_distance(cam.orbit_distance * meters_per_geo_unit, unit_prefs.distance, prec);
				std::snprintf(buf, sizeof(buf), "Camera Distance (r): %s | Orbit Distance: %s", radius_display.c_str(), orbit_display.c_str());
			} else {
				std::snprintf(buf, sizeof(buf), "Camera Distance (r): %s", radius_display.c_str());
			}
			push_block(HudElementId::CameraDistanceReadout, {HudTextLine{buf}});
		}

		{
			const auto& style = hud_layout_.element(HudElementId::CameraAnglesReadout);
			char buf[176];
			const int prec = std::clamp(style.decimal_precision, 0, 6);
			const auto angle_unit = orchestrator_.unit_preferences().angle;
			const double theta_disp = Units::convert_angle_from_radians(cam.theta, angle_unit);
			const double phi_disp = Units::convert_angle_from_radians(cam.phi, angle_unit);
			const char* angle_suffix = Units::angle_unit_suffix(angle_unit);
			if (style.display_mode == HudDisplayMode::Compact) {
				std::snprintf(buf, sizeof(buf), "(t,p)=(%.*f, %.*f) %s", prec, theta_disp, prec, phi_disp, angle_suffix);
			} else if (style.display_mode == HudDisplayMode::Extended) {
				std::snprintf(buf, sizeof(buf), "Angles (theta, phi): (%.*f, %.*f) %s | (%.4f, %.4f) rad", prec, theta_disp, prec, phi_disp, angle_suffix, cam.theta, cam.phi);
			} else {
				std::snprintf(buf, sizeof(buf), "Angles (theta, phi): (%.*f, %.*f) %s", prec, theta_disp, prec, phi_disp, angle_suffix);
			}
			push_block(HudElementId::CameraAnglesReadout, {HudTextLine{buf}});
		}

		{
			const auto& style = hud_layout_.element(HudElementId::CameraOrientationReadout);
			char buf[176];
			const int prec = std::clamp(style.decimal_precision, 0, 6);
			const auto angle_unit = orchestrator_.unit_preferences().angle;
			const double deg_to_rad = std::numbers::pi / 180.0;
			const double pitch_disp = Units::convert_angle_from_radians(cam.pitch * deg_to_rad, angle_unit);
			const double yaw_disp = Units::convert_angle_from_radians(cam.yaw * deg_to_rad, angle_unit);
			const double roll_disp = Units::convert_angle_from_radians(cam.roll * deg_to_rad, angle_unit);
			const char* angle_suffix = Units::angle_unit_suffix(angle_unit);
			if (style.display_mode == HudDisplayMode::Compact) {
				std::snprintf(buf, sizeof(buf), "P/Y/R: %.0f/%.0f/%.0f %s", pitch_disp, yaw_disp, roll_disp, angle_suffix);
			} else {
				std::snprintf(buf, sizeof(buf), "Orientation (Pitch, Yaw, Roll): (%.*f, %.*f, %.*f) %s", prec, pitch_disp, prec, yaw_disp, prec, roll_disp, angle_suffix);
			}
			push_block(HudElementId::CameraOrientationReadout, {HudTextLine{buf}});
		}

		{
			const auto& style = hud_layout_.element(HudElementId::MetricSummaryReadout);
			char buf[192];
			const int prec = std::clamp(style.decimal_precision, 0, 6);
			const auto& unit_prefs = orchestrator_.unit_preferences();
			const std::string mass_disp = Units::format_mass(params.mass, unit_prefs.mass, prec);
			const std::string charge_disp = Units::format_charge(params.charge, unit_prefs.charge, prec);
			if (style.display_mode == HudDisplayMode::Compact) {
				std::snprintf(buf, sizeof(buf), "%s", orchestrator_.active_metric_name().c_str());
			} else if (style.display_mode == HudDisplayMode::Extended) {
				std::snprintf(buf, sizeof(buf), "Metric: %s (Mass=%s, Spin=%.3f, Charge=%s) | Integrator: %s", orchestrator_.active_metric_name().c_str(), mass_disp.c_str(), params.spin, charge_disp.c_str(), orchestrator_.active_integrator_name().c_str());
			} else {
				std::snprintf(buf, sizeof(buf), "Metric: %s (Mass=%s, Spin=%.2f, Charge=%s)", orchestrator_.active_metric_name().c_str(), mass_disp.c_str(), params.spin, charge_disp.c_str());
			}
			push_block(HudElementId::MetricSummaryReadout, {HudTextLine{buf}});
		}

		{
			const auto& style = hud_layout_.element(HudElementId::RayStatisticsReadout);
			char buf[160];
			if (style.display_mode == HudDisplayMode::Compact) {
				std::snprintf(buf, sizeof(buf), "Abs %llu | Esc %llu", static_cast<unsigned long long>(tel.horizon_pixels_absorbed), static_cast<unsigned long long>(tel.celestial_pixels_hit));
			} else if (style.display_mode == HudDisplayMode::Extended) {
				std::snprintf(buf, sizeof(buf), "Absorbed Rays: %llu | Celestial Rays: %llu | Disk Hits: %llu | Avg Iterations: %.1f", static_cast<unsigned long long>(tel.horizon_pixels_absorbed), static_cast<unsigned long long>(tel.celestial_pixels_hit), static_cast<unsigned long long>(tel.accretion_disk_pixels_hit), tel.average_iterations_used);
			} else {
				std::snprintf(buf, sizeof(buf), "Absorbed Rays: %llu | Celestial Rays: %llu", static_cast<unsigned long long>(tel.horizon_pixels_absorbed), static_cast<unsigned long long>(tel.celestial_pixels_hit));
			}
			push_block(HudElementId::RayStatisticsReadout, {HudTextLine{buf}});
		}

		{
			const auto& style = hud_layout_.element(HudElementId::SimulationClockReadout);
			char buf[128];
			const int prec = std::clamp(style.decimal_precision, 0, 6);
			const auto& unit_prefs = orchestrator_.unit_preferences();
			const std::string time_display = Units::format_time(snap.logical_time, unit_prefs.time, prec);
			if (style.display_mode == HudDisplayMode::Compact) {
				std::snprintf(buf, sizeof(buf), "t=%s", time_display.c_str());
			} else if (style.display_mode == HudDisplayMode::Extended) {
				std::snprintf(buf, sizeof(buf), "Logical Time: %s | Tick #%llu | Rate: %.0f Hz", time_display.c_str(), static_cast<unsigned long long>(snap.tick_index), snap.tick_rate_hz);
			} else {
				std::snprintf(buf, sizeof(buf), "Logical Time: %s | Tick #%llu", time_display.c_str(), static_cast<unsigned long long>(snap.tick_index));
			}
			push_block(HudElementId::SimulationClockReadout, {HudTextLine{buf}});
		}

		{
			const auto& style = hud_layout_.element(HudElementId::WarpFactorReadout);
			char buf[80];
			if (style.display_mode == HudDisplayMode::Extended) {
				std::snprintf(buf, sizeof(buf), "Warp Factor: %.2fx | Paused: %s", snap.warp_factor, snap.is_paused ? "Yes" : "No");
			} else {
				std::snprintf(buf, sizeof(buf), "Warp Factor: %.2fx", snap.warp_factor);
			}
			push_block(HudElementId::WarpFactorReadout, {HudTextLine{buf}});
		}

		{
			const auto& style = hud_layout_.element(HudElementId::PerformancePresetReadout);
			char buf[64];
			static constexpr const char* kPresetNames[] = {"Potato", "Performance", "Balanced", "High", "Ultra", "Extreme", "Custom"};
			const uint32_t preset_idx = std::min<uint32_t>(params.performance_preset, 6U);
			if (style.display_mode == HudDisplayMode::Compact) {
				std::snprintf(buf, sizeof(buf), "%s", kPresetNames[preset_idx]);
			} else {
				std::snprintf(buf, sizeof(buf), "Performance Preset: %s", kPresetNames[preset_idx]);
			}
			push_block(HudElementId::PerformancePresetReadout, {HudTextLine{buf}});
		}

		{
			const auto& style = hud_layout_.element(HudElementId::TelemetryQuickReadout);
			const double r_safe_link = std::max(cam.radius, 2.05 * params.mass);
			Metrics::KerrMetric<double> tel_metric(params.mass, params.spin, 1.0, 1.0);
			Core::FourVector<double> tel_pos(0.0, r_safe_link, cam.theta, cam.phi);
			const auto tel_g = tel_metric.metric_tensor(tel_pos);
			const double lapse = std::sqrt(std::max(-tel_g(0, 0), 1e-30));
			char buf[160];
			if (style.display_mode == HudDisplayMode::Compact) {
				std::snprintf(buf, sizeof(buf), "alpha=%.3f", lapse);
			} else if (style.display_mode == HudDisplayMode::Extended) {
				const double omega_zamo = (std::abs(params.spin) > 1e-9) ? Metrics::compute_zamo_angular_velocity(tel_metric, tel_pos) : 0.0;
				std::snprintf(buf, sizeof(buf), "Lapse (alpha): %.4f | Time Dilation: %.3fx | ZAMO Omega: %.4f rad/M", lapse, 1.0 / std::max(lapse, 1e-12), omega_zamo);
			} else {
				std::snprintf(buf, sizeof(buf), "Lapse (alpha): %.4f | Time Dilation: %.3fx", lapse, 1.0 / std::max(lapse, 1e-12));
			}
			push_block(HudElementId::TelemetryQuickReadout, {HudTextLine{buf}});
		}

		{
			const auto& style = hud_layout_.element(HudElementId::SpectrographQuickReadout);
			const bool on_disk = Optics::DiskThermalProfile::radius_within_disk(params.mass, params.spin, cam.radius);
			const double g_link = on_disk ? Optics::DiskThermalProfile::circular_orbit_redshift_factor(params.mass, cam.radius) : 1.0;
			char buf[144];
			if (style.display_mode == HudDisplayMode::Extended) {
				std::snprintf(buf, sizeof(buf), "Doppler g: %.3f (%s) | Exposure: %.2f EV | Tonemap: %u", g_link, on_disk ? "on disk band" : "off disk band", params.camera_exposure, params.tonemapping_mode);
			} else {
				std::snprintf(buf, sizeof(buf), "Doppler g: %.3f | Exposure: %.2f EV", g_link, params.camera_exposure);
			}
			push_block(HudElementId::SpectrographQuickReadout, {HudTextLine{buf}});
		}

		{
			const auto& style = hud_layout_.element(HudElementId::DiagnosticsQuickReadout);
			char buf[128];
			if (style.display_mode == HudDisplayMode::Compact) {
				std::snprintf(buf, sizeof(buf), "%s", orchestrator_.active_metric_name().c_str());
			} else {
				std::snprintf(buf, sizeof(buf), "Metric: %s | Integrator: %s", orchestrator_.active_metric_name().c_str(), orchestrator_.active_integrator_name().c_str());
			}
			push_block(HudElementId::DiagnosticsQuickReadout, {HudTextLine{buf}});
		}

		{
			const auto& profiler = orchestrator_.profiler();
			if (!profiler.history().empty()) {
				const auto& style = hud_layout_.element(HudElementId::ProfilerFrameTimeReadout);
				const auto& latest = profiler.history().back();
				char buf[176];
				if (style.display_mode == HudDisplayMode::Compact) {
					std::snprintf(buf, sizeof(buf), "%.2f ms", latest.frame_time_ms);
				} else if (style.display_mode == HudDisplayMode::Extended) {
					std::snprintf(buf, sizeof(buf), "Frame: %.2f ms | Dispatch: %.2f ms | Upload: %.2f ms | HUD: %.2f ms", latest.frame_time_ms, latest.stage_time_ms[static_cast<size_t>(Orchestrator::ProfilerTaskStage::RenderDispatch)], latest.stage_time_ms[static_cast<size_t>(Orchestrator::ProfilerTaskStage::TextureUpload)], latest.stage_time_ms[static_cast<size_t>(Orchestrator::ProfilerTaskStage::HudOverlay)]);
				} else {
					std::snprintf(buf, sizeof(buf), "Frame: %.2f ms | Dispatch: %.2f ms | Upload: %.2f ms", latest.frame_time_ms, latest.stage_time_ms[static_cast<size_t>(Orchestrator::ProfilerTaskStage::RenderDispatch)], latest.stage_time_ms[static_cast<size_t>(Orchestrator::ProfilerTaskStage::TextureUpload)]);
				}
				const ImU32 base_col = ImGui::ColorConvertFloat4ToU32(ImVec4(style.text_color[0], style.text_color[1], style.text_color[2], style.text_color[3]));
				const ImU32 dyn_col = hud_resolve_dynamic_color(style, latest.frame_time_ms, base_col);
				push_block(HudElementId::ProfilerFrameTimeReadout, {HudTextLine{buf, dyn_col}});
			}
		}

		{
			const auto& profiler = orchestrator_.profiler();
			const auto& style = hud_layout_.element(HudElementId::ProfilerBottleneckReadout);
			const auto report = profiler.analyze_bottleneck(120);
			char buf[176];
			if (style.display_mode == HudDisplayMode::Compact) {
				std::snprintf(buf, sizeof(buf), "%s", Orchestrator::profiler_stage_name(report.dominant_stage));
			} else {
				std::snprintf(buf, sizeof(buf), "Bottleneck: %s (%.0f%%)%s", Orchestrator::profiler_stage_name(report.dominant_stage), report.dominant_share * 100.0, report.ray_step_saturated ? " | Step-Saturated" : "");
			}
			const ImU32 base_col = ImGui::ColorConvertFloat4ToU32(ImVec4(style.text_color[0], style.text_color[1], style.text_color[2], style.text_color[3]));
			const ImU32 dyn_col = hud_resolve_dynamic_color(style, report.dominant_share * 100.0, base_col);
			push_block(HudElementId::ProfilerBottleneckReadout, {HudTextLine{buf, dyn_col}});
		}

		{
			const auto& profiler = orchestrator_.profiler();
			if (!profiler.history().empty()) {
				const auto& latest = profiler.history().back();
				char buf[224];
				std::snprintf(
					buf, sizeof(buf),
					"HUD Overlay: %.3f ms | Camera: %.3f ms | Schematic: %.3f ms | Post-Process: %.3f ms | Readback: %.3f ms",
					latest.stage_time_ms[static_cast<size_t>(Orchestrator::ProfilerTaskStage::HudOverlay)],
					latest.stage_time_ms[static_cast<size_t>(Orchestrator::ProfilerTaskStage::CameraUpdate)],
					latest.stage_time_ms[static_cast<size_t>(Orchestrator::ProfilerTaskStage::SchematicOverlay)],
					latest.stage_time_ms[static_cast<size_t>(Orchestrator::ProfilerTaskStage::PostProcessing)],
					latest.stage_time_ms[static_cast<size_t>(Orchestrator::ProfilerTaskStage::FramebufferReadback)]
				);
				push_block(HudElementId::ProfilerStageBreakdownReadout, {HudTextLine{buf}});
			}
		}

		{
			const auto& profiler = orchestrator_.profiler();
			const auto& history = profiler.history();
			if (!history.empty()) {
				const auto& style = hud_layout_.element(HudElementId::ProfilerGpuCpuSplitReadout);
				const size_t window_count = std::min<size_t>(history.size(), 120);
				size_t gpu_frames = 0;
				for (size_t i = history.size() - window_count; i < history.size(); ++i) {
					if (history[i].used_gpu_path) ++gpu_frames;
				}
				const double gpu_ratio = static_cast<double>(gpu_frames) / static_cast<double>(window_count) * 100.0;
				char buf[128];
				if (style.display_mode == HudDisplayMode::Extended) {
					std::snprintf(buf, sizeof(buf), "GPU Path: %.0f%% | CPU Path: %.0f%% (last %zu frames)", gpu_ratio, 100.0 - gpu_ratio, window_count);
				} else {
					std::snprintf(buf, sizeof(buf), "GPU Path: %.0f%% | CPU Path: %.0f%%", gpu_ratio, 100.0 - gpu_ratio);
				}
				push_block(HudElementId::ProfilerGpuCpuSplitReadout, {HudTextLine{buf}});
			}
		}

		{
			const auto& profiler = orchestrator_.profiler();
			if (!profiler.history().empty()) {
				const auto& style = hud_layout_.element(HudElementId::ProfilerRayClassificationReadout);
				const auto& latest = profiler.history().back();
				char buf[176];
				if (style.display_mode == HudDisplayMode::Extended) {
					const uint64_t total = std::max<uint64_t>(latest.horizon_pixels + latest.celestial_pixels + latest.disk_pixels, uint64_t{1});
					std::snprintf(buf, sizeof(buf), "Horizon: %llu (%.1f%%) | Celestial: %llu (%.1f%%) | Disk: %llu (%.1f%%)", static_cast<unsigned long long>(latest.horizon_pixels), static_cast<double>(latest.horizon_pixels) / static_cast<double>(total) * 100.0, static_cast<unsigned long long>(latest.celestial_pixels), static_cast<double>(latest.celestial_pixels) / static_cast<double>(total) * 100.0, static_cast<unsigned long long>(latest.disk_pixels), static_cast<double>(latest.disk_pixels) / static_cast<double>(total) * 100.0);
				} else {
					std::snprintf(buf, sizeof(buf), "Horizon: %llu | Celestial: %llu | Disk: %llu", static_cast<unsigned long long>(latest.horizon_pixels), static_cast<unsigned long long>(latest.celestial_pixels), static_cast<unsigned long long>(latest.disk_pixels));
				}
				push_block(HudElementId::ProfilerRayClassificationReadout, {HudTextLine{buf}});
			}
		}

		{
			const auto& style = hud_layout_.element(HudElementId::ProfilerIterationRangeReadout);
			char buf[112];
			if (style.display_mode == HudDisplayMode::Extended) {
				std::snprintf(buf, sizeof(buf), "Ray Iterations: %u - %u | Avg: %.1f", tel.min_iterations_used, tel.max_iterations_used, tel.average_iterations_used);
			} else {
				std::snprintf(buf, sizeof(buf), "Ray Iterations: %u - %u", tel.min_iterations_used, tel.max_iterations_used);
			}
			push_block(HudElementId::ProfilerIterationRangeReadout, {HudTextLine{buf}});
		}

		{
			char buf[64];
			std::snprintf(buf, sizeof(buf), "N-Body Count: %zu", orchestrator_.nbody_system().body_count());
			push_block(HudElementId::BodyCountReadout, {HudTextLine{buf}});
		}

		{
			const auto& style = hud_layout_.element(HudElementId::GpuComputeStatusReadout);
			char buf[112];
			if (style.display_mode == HudDisplayMode::Extended) {
				std::snprintf(buf, sizeof(buf), "GPU Path: %s | Enabled: %s | Available: %s", tel.used_gpu_path ? "Active" : "Idle", params.use_gpu_compute ? "Yes" : "No", pipeline_.gpu_compute_available() ? "Yes" : "No");
			} else {
				std::snprintf(buf, sizeof(buf), "GPU Path: %s", tel.used_gpu_path ? "Active" : "Idle");
			}
			push_block(HudElementId::GpuComputeStatusReadout, {HudTextLine{buf}});
		}

		{
			const auto& style = hud_layout_.element(HudElementId::IntegratorStatsReadout);
			char buf[144];
			if (style.display_mode == HudDisplayMode::Extended) {
				std::snprintf(buf, sizeof(buf), "Integrator: %s | rtol: %.1e | atol: %.1e", orchestrator_.active_integrator_name().c_str(), params.integration_rtol, params.integration_atol);
			} else {
				std::snprintf(buf, sizeof(buf), "Integrator: %s", orchestrator_.active_integrator_name().c_str());
			}
			push_block(HudElementId::IntegratorStatsReadout, {HudTextLine{buf}});
		}

		{
			const auto& style = hud_layout_.element(HudElementId::ConstantsQuickReadout);
			const auto& constants = orchestrator_.constants_engine();
			const auto& unit_prefs = orchestrator_.unit_preferences();
			static constexpr const char* kPresetLabels[] = {"SI", "Planck", "Custom"};
			const uint32_t preset_idx = std::min<uint32_t>(static_cast<uint32_t>(constants.active_preset()), 2U);
			char buf[192];
			if (style.display_mode == HudDisplayMode::Extended) {
				const std::string c_disp = Units::format_velocity(constants.sim_speed_of_light(), unit_prefs.velocity);
				const std::string l0_disp = Units::format_distance(constants.length_scale(), unit_prefs.distance);
				std::snprintf(buf, sizeof(buf), "Constants: %s | c=%s | G=%.3e | L0=%s", kPresetLabels[preset_idx], c_disp.c_str(), constants.sim_gravitational_constant(), l0_disp.c_str());
			} else {
				std::snprintf(buf, sizeof(buf), "Constants: %s Preset", kPresetLabels[preset_idx]);
			}
			push_block(HudElementId::ConstantsQuickReadout, {HudTextLine{buf}});
		}

		{
			const auto& style = hud_layout_.element(HudElementId::RayProbeReadout);
			const auto& probe = ray_probe_result_;
			if (style.enabled && probe.valid) {
				const double mass_scale = std::max(params.mass, 1e-9);
				const int prec = std::clamp(style.decimal_precision, 0, 6);
				const char* fate = probe.disk_hit ? "Disk" : Optics::ray_termination_name(probe.termination);
				char buf[224];
				if (style.display_mode == HudDisplayMode::Compact) {
					std::snprintf(buf, sizeof(buf), "%s | g=%.*f", fate, prec, probe.spectral_shift_g);
				} else if (style.display_mode == HudDisplayMode::Extended) {
					std::snprintf(buf, sizeof(buf), "Probe (%u, %u): %s | g=%.*f | b=%.*f M | Q=%.*e | Iterations: %u", probe.pixel_x, probe.pixel_y, fate, prec, probe.spectral_shift_g, prec, probe.impact_parameter / mass_scale, prec, probe.carter_constant, probe.iterations);
				} else {
					std::snprintf(buf, sizeof(buf), "Probe: %s | g=%.*f | b=%.*f M", fate, prec, probe.spectral_shift_g, prec, probe.impact_parameter / mass_scale);
				}
				const ImU32 base_col = ImGui::ColorConvertFloat4ToU32(ImVec4(style.text_color[0], style.text_color[1], style.text_color[2], style.text_color[3]));
				push_block(HudElementId::RayProbeReadout, {HudTextLine{buf, hud_resolve_dynamic_color(style, probe.spectral_shift_g, base_col)}});
			}
		}

		{
			const auto& style = hud_layout_.element(HudElementId::RayProbeEmissionReadout);
			const auto& probe = ray_probe_result_;
			if (style.enabled && probe.valid) {
				const double mass_scale = std::max(params.mass, 1e-9);
				const int prec = std::clamp(style.decimal_precision, 0, 6);
				const auto& unit_prefs = orchestrator_.unit_preferences();
				const double length_scale = orchestrator_.constants_engine().length_scale();
				const double emission_sin_theta = std::sin(probe.emission_theta);
				const Observer::CoordinateVector emission_cartesian{
					probe.emission_r * emission_sin_theta * std::cos(probe.emission_phi),
					probe.emission_r * emission_sin_theta * std::sin(probe.emission_phi),
					probe.emission_r * std::cos(probe.emission_theta)
				};
				const auto emission_lines = describe_position_lines(
					"Emission", emission_cartesian, coordinate_frame, length_scale, unit_prefs,
					coordinate_display.primary, coordinate_display.secondary, coordinate_display.secondary_enabled,
					prec, style.display_mode == HudDisplayMode::Compact, coordinate_display.show_axis_labels
				);
				std::vector<HudTextLine> emission_block;
				for (const auto& emission_line : emission_lines) {
					emission_block.push_back(HudTextLine{emission_line});
				}
				if (style.display_mode == HudDisplayMode::Extended) {
					char buf[96];
					std::snprintf(buf, sizeof(buf), "Closest Approach: %.*f M | r_e=%.*f M", prec, probe.minimum_radius / mass_scale, prec, probe.emission_r / mass_scale);
					emission_block.push_back(HudTextLine{buf});
				}
				push_block(HudElementId::RayProbeEmissionReadout, std::move(emission_block));
			}
		}

		{
			const auto& style = hud_layout_.element(HudElementId::RayProbeGeometryReadout);
			const auto& probe = ray_probe_result_;
			if (style.enabled && probe.valid) {
				const double mass_scale = std::max(params.mass, 1e-9);
				const int prec = std::clamp(style.decimal_precision, 0, 6);
				const auto& unit_prefs = orchestrator_.unit_preferences();
				const std::string deflection_text = Units::format_angle(probe.deflection_angle, unit_prefs.angle);
				char buf[256];
				if (style.display_mode == HudDisplayMode::Compact) {
					std::snprintf(buf, sizeof(buf), "Deflection %s | Order %u", deflection_text.c_str(), probe.image_order);
				} else if (style.display_mode == HudDisplayMode::Extended) {
					const std::string temperature_text = probe.disk_hit ? Units::format_temperature(probe.disk_observed_temperature_k, unit_prefs.temperature) : std::string("n/a");
					std::snprintf(buf, sizeof(buf), "Deflection: %s | Disk Image Order: %u | Closest Approach: %.*f M | E=%.*f | Observed Disk Temperature: %s", deflection_text.c_str(), probe.image_order, prec, probe.minimum_radius / mass_scale, prec, probe.energy, temperature_text.c_str());
				} else {
					std::snprintf(buf, sizeof(buf), "Deflection: %s | Disk Crossings: %u | Closest Approach: %.*f M", deflection_text.c_str(), probe.disk_crossings, prec, probe.minimum_radius / mass_scale);
				}
				push_block(HudElementId::RayProbeGeometryReadout, {HudTextLine{buf}});
			}
		}

		{
			const auto& style = hud_layout_.element(HudElementId::PolarizationQuickReadout);
			const auto& pol = linked_readouts_.polarization;
			if (style.enabled && pol.valid) {
				const int prec = std::clamp(style.decimal_precision, 0, 6);
				char buf[224];
				if (style.display_mode == HudDisplayMode::Compact) {
					std::snprintf(buf, sizeof(buf), "DoLP %.*f%% | EVPA %.*f deg", prec, 100.0 * pol.dolp, prec, pol.evpa_deg);
				} else if (style.display_mode == HudDisplayMode::Extended) {
					std::snprintf(buf, sizeof(buf), "Polarization @ %.4g nm: DoLP %.*f%% | DoCP %.*f%% | EVPA %.*f deg | I=%.*e", pol.wavelength_nm, prec, 100.0 * pol.dolp, prec, 100.0 * pol.docp, prec, pol.evpa_deg, prec, pol.intensity);
				} else {
					std::snprintf(buf, sizeof(buf), "Polarization @ %.4g nm: DoLP %.*f%% | DoCP %.*f%% | EVPA %.*f deg", pol.wavelength_nm, prec, 100.0 * pol.dolp, prec, 100.0 * pol.docp, prec, pol.evpa_deg);
				}
				const ImU32 base_col = ImGui::ColorConvertFloat4ToU32(ImVec4(style.text_color[0], style.text_color[1], style.text_color[2], style.text_color[3]));
				push_block(HudElementId::PolarizationQuickReadout, {HudTextLine{buf, hud_resolve_dynamic_color(style, pol.dolp * 100.0, base_col)}});
			}
		}

		{
			const auto& style = hud_layout_.element(HudElementId::InterferometryQuickReadout);
			const auto& vlbi = linked_readouts_.interferometry;
			if (style.enabled && vlbi.valid) {
				const int prec = std::clamp(style.decimal_precision, 0, 6);
				char buf[224];
				if (style.display_mode == HudDisplayMode::Compact) {
					std::snprintf(buf, sizeof(buf), "VLBI %.*f uas", prec, vlbi.resolution_uas);
				} else if (style.display_mode == HudDisplayMode::Extended) {
					std::snprintf(buf, sizeof(buf), "VLBI: %zu visibilities | %zu closure phases | B_max %.*f Glambda | Resolution %.*f uas | Mean SNR %.*f", vlbi.visibility_count, vlbi.closure_count, prec, vlbi.maximum_baseline_glambda, prec, vlbi.resolution_uas, prec, vlbi.mean_snr);
				} else {
					std::snprintf(buf, sizeof(buf), "VLBI: %zu vis | B_max %.*f Glambda | Resolution %.*f uas", vlbi.visibility_count, prec, vlbi.maximum_baseline_glambda, prec, vlbi.resolution_uas);
				}
				const ImU32 base_col = ImGui::ColorConvertFloat4ToU32(ImVec4(style.text_color[0], style.text_color[1], style.text_color[2], style.text_color[3]));
				push_block(HudElementId::InterferometryQuickReadout, {HudTextLine{buf, hud_resolve_dynamic_color(style, vlbi.mean_snr, base_col)}});
			}
		}

		{
			const auto& style = hud_layout_.element(HudElementId::DarkMatterQuickReadout);
			const auto& dm_field = params.dark_matter;
			if (style.enabled && dm_field.enabled) {
				uint32_t active_halos = 0;
				for (uint32_t i = 0; i < dm_field.count; ++i) {
					if (dm_field.halos[i].enabled) ++active_halos;
				}
				const int prec = std::clamp(style.decimal_precision, 0, 6);
				const size_t mode_index = std::min<size_t>(dm_field.visualization_mode, DarkMatter::kDarkMatterVisualizationModeCount - 1);
				char buf[224];
				if (style.display_mode == HudDisplayMode::Compact) {
					std::snprintf(buf, sizeof(buf), "DM %u/%u", active_halos, dm_field.count);
				} else if (style.display_mode == HudDisplayMode::Extended) {
					std::snprintf(buf, sizeof(buf), "Dark Matter: %u/%u halos | Mass %.*f M | Lensing %s | Glow %s (%s) | N-Body %s", active_halos, dm_field.count, prec, dm_field.total_halo_mass(), dm_field.lensing_enabled ? "On" : "Off", dm_field.visualization_enabled ? "On" : "Off", DarkMatter::kDarkMatterVisualizationModeNames[mode_index], dm_field.affects_bodies ? "On" : "Off");
				} else {
					std::snprintf(buf, sizeof(buf), "Dark Matter: %u/%u halos | Mass %.*f M | Lensing %s", active_halos, dm_field.count, prec, dm_field.total_halo_mass(), dm_field.lensing_enabled ? "On" : "Off");
				}
				push_block(HudElementId::DarkMatterQuickReadout, {HudTextLine{buf}});
			}
		}

		{
			const auto& style = hud_layout_.element(HudElementId::CameraPositionReadout);
			if (style.enabled) {
				const int prec = std::clamp(style.decimal_precision, 0, 6);
				const auto position_lines = describe_position_lines(
					"Camera", cam.position, coordinate_frame, coordinate_length_scale, orchestrator_.unit_preferences(),
					coordinate_display.primary, coordinate_display.secondary, coordinate_display.secondary_enabled,
					prec, style.display_mode == HudDisplayMode::Compact, coordinate_display.show_axis_labels
				);
				std::vector<HudTextLine> position_block;
				for (const auto& position_line : position_lines) {
					position_block.push_back(HudTextLine{position_line});
				}
				push_block(HudElementId::CameraPositionReadout, std::move(position_block));
			}
		}

		{
			const auto& style = hud_layout_.element(HudElementId::CursorReadout);
			if (style.enabled && cursor_inside_) {
				const int prec = std::clamp(style.decimal_precision, 0, 6);
				const ImVec2 mouse_screen = ImGui::GetMousePos();
				const uint32_t cursor_frame_w = (display_frame_width_ > 0U) ? display_frame_width_ : current_width_;
				const uint32_t cursor_frame_h = (display_frame_height_ > 0U) ? display_frame_height_ : current_height_;
				const auto& unit_prefs = orchestrator_.unit_preferences();
				std::vector<HudTextLine> cursor_block;
				char buf[256];
				if (style.display_mode == HudDisplayMode::Compact) {
					std::snprintf(buf, sizeof(buf), "Cursor px (%u, %u)", cursor_pixel_x_, cursor_pixel_y_);
				} else if (style.display_mode == HudDisplayMode::Extended) {
					std::snprintf(buf, sizeof(buf), "Cursor: screen (%.0f, %.0f) | viewport (%.0f, %.0f) | pixel (%u, %u) / %ux%u | UV (%.*f, %.*f)", static_cast<double>(mouse_screen.x), static_cast<double>(mouse_screen.y), static_cast<double>(cursor_viewport_position_.x), static_cast<double>(cursor_viewport_position_.y), cursor_pixel_x_, cursor_pixel_y_, cursor_frame_w, cursor_frame_h, prec, static_cast<double>(cursor_uv_.x), prec, static_cast<double>(cursor_uv_.y));
				} else {
					std::snprintf(buf, sizeof(buf), "Cursor: screen (%.0f, %.0f) | pixel (%u, %u) / %ux%u", static_cast<double>(mouse_screen.x), static_cast<double>(mouse_screen.y), cursor_pixel_x_, cursor_pixel_y_, cursor_frame_w, cursor_frame_h);
				}
				cursor_block.push_back(HudTextLine{buf});

				const auto& cursor_probe = cursor_probe_result_;
				if (cursor_probe.valid && style.display_mode != HudDisplayMode::Compact) {
					const double cursor_mass_scale = std::max(params.mass, 1e-9);
					const char* cursor_fate = cursor_probe.disk_hit ? "Disk" : Optics::ray_termination_name(cursor_probe.termination);
					std::snprintf(buf, sizeof(buf), "Cursor Ray: %s | g=%.*f | b=%.*f M", cursor_fate, prec, cursor_probe.spectral_shift_g, prec, cursor_probe.impact_parameter / cursor_mass_scale);
					cursor_block.push_back(HudTextLine{buf});

					const double cursor_sin_theta = std::sin(cursor_probe.emission_theta);
					const Observer::CoordinateVector cursor_emission{
						cursor_probe.emission_r * cursor_sin_theta * std::cos(cursor_probe.emission_phi),
						cursor_probe.emission_r * cursor_sin_theta * std::sin(cursor_probe.emission_phi),
						cursor_probe.emission_r * std::cos(cursor_probe.emission_theta)
					};
					const auto cursor_lines = describe_position_lines(
						"Cursor Ray Emission", cursor_emission, coordinate_frame, coordinate_length_scale, unit_prefs,
						coordinate_display.primary, coordinate_display.secondary, coordinate_display.secondary_enabled,
						prec, false, coordinate_display.show_axis_labels
					);
					for (const auto& cursor_line : cursor_lines) {
						cursor_block.push_back(HudTextLine{cursor_line});
					}

					if (style.display_mode == HudDisplayMode::Extended) {
						const auto& direction = cursor_probe_query_.direction;
						const double direction_length = std::sqrt(direction[0] * direction[0] + direction[1] * direction[1] + direction[2] * direction[2]);
						const double direction_theta = (direction_length > 1e-12) ? std::acos(std::clamp(direction[2] / direction_length, -1.0, 1.0)) : 0.0;
						const double direction_phi = std::atan2(direction[1], direction[0]);
						const char* angle_suffix = Units::angle_unit_suffix(unit_prefs.angle);
						std::snprintf(buf, sizeof(buf), "Ray Direction: theta=%.*f %s | phi=%.*f %s", prec, Units::convert_angle_from_radians(direction_theta, unit_prefs.angle), angle_suffix, prec, Units::convert_angle_from_radians(direction_phi, unit_prefs.angle), angle_suffix);
						cursor_block.push_back(HudTextLine{buf});
					}
				}
				push_block(HudElementId::CursorReadout, std::move(cursor_block));
			}
		}

		{
			const auto& kb = camera_controller_.config().keybinds;
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
				nav_lines.push_back(HudTextLine{format_key_binding(b) + ": " + std::string(input_action_name(action)), col});
			}
			if (nav_lines.size() > 1) {
				push_block(HudElementId::NavigationControlsPanel, std::move(nav_lines));
			}
		}

		std::stable_sort(pending.begin(), pending.end(), [](const PendingHudBlock& a, const PendingHudBlock& b) noexcept {
			return a.style->draw_priority > b.style->draw_priority;
		});

		HudAutoArranger arranger;
		if (hud_layout_.auto_arrange_enabled) {
			const auto& toolbar_style = hud_layout_.element(HudElementId::ViewportToolbar);
			if (toolbar_style.enabled) {
				arranger.reserve(toolbar_style.anchor, ImVec2(900.0f, 34.0f), hud_layout_.auto_arrange_spacing);
			}
		}
		for (const auto& block : pending) {
			const auto& style = *block.style;
			const ImVec2 block_size = measure_hud_block(style, block.lines);

			ImVec2 local_pos;
			if (hud_layout_.auto_arrange_enabled) {
				local_pos = arranger.place(style.anchor, avail, block_size, hud_layout_.auto_arrange_spacing);
				local_pos.x = std::max(local_pos.x + style.nudge_x, 0.0f);
				local_pos.y = std::max(local_pos.y + style.nudge_y, 0.0f);
			} else {
				local_pos = hud_anchor_resolve(style.anchor, avail, block_size, style.offset_x, style.offset_y);
			}

			const ImVec2 screen_pos(window_pos.x + local_pos.x, window_pos.y + local_pos.y);
			draw_hud_block_at(draw_list, screen_pos, style, block.lines);
		}
	}
};

}

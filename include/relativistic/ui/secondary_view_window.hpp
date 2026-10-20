#pragma once

#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/render/gpu_types.hpp"
#include "relativistic/render/software_compute_engine.hpp"
#include "relativistic/ui/secondary_view_roles.hpp"
#include "relativistic/ui/tooltip_utils.hpp"
#include "relativistic/units/unit_aware_widgets.hpp"
#include <imgui.h>
#if defined(__APPLE__)
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif
#include <vector>
#include <string>
#include <string_view>
#include <array>
#include <cmath>
#include <numbers>
#include <algorithm>
#include <cstdint>
#include <cstddef>
#include <mutex>

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

#ifndef GL_RGBA32F
#define GL_RGBA32F 0x8814
#endif

namespace Relativistic::UI {

[[nodiscard]] inline std::string secondary_view_state_prefix(size_t slot) {
	return "secondary_view_" + std::to_string(slot) + "_";
}

inline bool render_secondary_target_editor(SecondaryTargetSettings& settings, Orchestrator::SimulationOrchestrator<1024>& orchestrator) {
	bool changed = false;
	int mode = static_cast<int>(settings.mode);
	if (ImGui::Combo("Observation Target", &mode, kSecondaryTargetModeNames.data(), static_cast<int>(kSecondaryTargetModeNames.size()))) {
		settings.mode = static_cast<SecondaryTargetMode>(mode);
		changed = true;
	}
	render_setting_tooltip("Point every observer looks at and orbits around: the central object, the primary camera or its focus point, a tracked body, or a custom position.");

	if (settings.mode == SecondaryTargetMode::TrackedBody) {
		std::vector<uint32_t> ids;
		std::vector<std::string> labels;
		{
			std::lock_guard<std::recursive_mutex> lock(orchestrator.nbody_system().bodies_mutex());
			for (const auto& body : orchestrator.nbody_system().bodies()) {
				if (!body.enabled) continue;
				ids.push_back(body.id);
				labels.push_back((body.has_name() ? std::string(body.name_view()) : std::string("Body")) + " (#" + std::to_string(body.id) + ")");
			}
		}
		if (ids.empty()) {
			ImGui::TextDisabled("No enabled body available; the central object is used.");
		} else {
			int selected = 0;
			for (size_t i = 0; i < ids.size(); ++i) {
				if (ids[i] == settings.body_id) {
					selected = static_cast<int>(i);
				}
			}
			std::vector<const char*> pointers;
			pointers.reserve(labels.size());
			for (const std::string& label : labels) {
				pointers.push_back(label.c_str());
			}
			if (ImGui::Combo("Tracked Body", &selected, pointers.data(), static_cast<int>(pointers.size()))) {
				settings.body_id = ids[static_cast<size_t>(selected)];
				changed = true;
			} else if (settings.body_id == 0U || std::find(ids.begin(), ids.end(), settings.body_id) == ids.end()) {
				settings.body_id = ids.front();
				changed = true;
			}
		}
	} else if (settings.mode == SecondaryTargetMode::CustomPoint) {
		const double length_scale = std::max(orchestrator.constants_engine().length_scale(), 1.0e-300);
		const double limit = 1.0e6 * length_scale;
		static constexpr std::array<const char*, 3> labels{"Target X", "Target Y", "Target Z"};
		for (size_t i = 0; i < 3; ++i) {
			double meters = settings.custom_point[i] * length_scale;
			if (unit_aware_slider_double(labels[i], &meters, -limit, limit, UnitCategory::Distance, orchestrator.unit_preferences(), "%.4f")) {
				settings.custom_point[i] = meters / length_scale;
				changed = true;
			}
		}
	}
	return changed;
}

class SecondaryViewWindow {
public:
	using Vec3 = std::array<double, 3>;
	using StateEntries = SecondaryStateIo::Entries;

	struct ObserverFrame {
		Vec3 position{0.0, 0.0, 0.0};
		Vec3 forward{1.0, 0.0, 0.0};
		Vec3 right{0.0, -1.0, 0.0};
		Vec3 up{0.0, 0.0, 1.0};
		std::array<double, 4> boost{0.0, 0.0, 0.0, 0.0};
		double fov_rad{1.0471975511965976};
		uint32_t projection{0};
		double escape_radius{50.0};
	};

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

	SecondaryViewRole role_{SecondaryViewRole::FreeObserver};
	SecondaryTargetSettings local_target_{};
	const SecondaryTargetSettings* shared_target_{nullptr};
	bool use_shared_target_{false};
	bool force_shared_target_{false};
	bool beacon_enabled_{true};
	bool probe_inbound_{true};
	double probe_speed_fraction_{1.0};
	double look_yaw_deg_{0.0};
	double look_pitch_deg_{0.0};
	double asymptotic_distance_{1500.0};
	double asymptotic_half_extent_{40.0};
	uint32_t view_id_{0};

	Render::GpuCameraPushConstants last_consts_{};
	bool has_rendered_once_{false};

	[[nodiscard]] static double dot3(const Vec3& a, const Vec3& b) noexcept {
		return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
	}

	[[nodiscard]] static Vec3 cross3(const Vec3& a, const Vec3& b) noexcept {
		return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
	}

	[[nodiscard]] static Vec3 normalized(const Vec3& v, const Vec3& fallback) noexcept {
		const double length = std::sqrt(dot3(v, v));
		if (length < 1.0e-12) {
			return fallback;
		}
		return {v[0] / length, v[1] / length, v[2] / length};
	}

	[[nodiscard]] static std::array<float, 3> hue_to_rgb(float hue) noexcept {
		const float scaled = std::fmod(hue, 1.0f) * 6.0f;
		const float x = 1.0f - std::abs(std::fmod(scaled, 2.0f) - 1.0f);
		const float saturation = 0.8f;
		const float chroma = saturation;
		const float base = 1.0f - chroma;
		float r = 0.0f;
		float g = 0.0f;
		float b = 0.0f;
		if (scaled < 1.0f) { r = chroma; g = chroma * x; }
		else if (scaled < 2.0f) { r = chroma * x; g = chroma; }
		else if (scaled < 3.0f) { g = chroma; b = chroma * x; }
		else if (scaled < 4.0f) { g = chroma * x; b = chroma; }
		else if (scaled < 5.0f) { r = chroma * x; b = chroma; }
		else { r = chroma; b = chroma * x; }
		return {r + base, g + base, b + base};
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

	[[nodiscard]] const SecondaryTargetSettings& active_target() const noexcept {
		return ((use_shared_target_ || force_shared_target_) && shared_target_ != nullptr) ? *shared_target_ : local_target_;
	}

	[[nodiscard]] Vec3 resolve_target_position() const {
		const SecondaryTargetSettings& settings = active_target();
		switch (settings.mode) {
			case SecondaryTargetMode::PrimaryCameraPosition:
				return orchestrator_->camera().position;
			case SecondaryTargetMode::PrimaryCameraFocus: {
				const auto& camera = orchestrator_->camera();
				const auto basis = camera.orientation_basis();
				const double distance = std::max(camera.orbit_distance, 1.0e-3);
				return {
					camera.position[0] + basis.forward[0] * distance,
					camera.position[1] + basis.forward[1] * distance,
					camera.position[2] + basis.forward[2] * distance
				};
			}
			case SecondaryTargetMode::TrackedBody: {
				std::lock_guard<std::recursive_mutex> lock(orchestrator_->nbody_system().bodies_mutex());
				for (const auto& body : orchestrator_->nbody_system().bodies()) {
					if (body.enabled && body.id == settings.body_id) {
						return body.position;
					}
				}
				return {0.0, 0.0, 0.0};
			}
			case SecondaryTargetMode::CustomPoint:
				return settings.custom_point;
			case SecondaryTargetMode::CentralObject:
			default:
				return {0.0, 0.0, 0.0};
		}
	}

	[[nodiscard]] Render::GpuBodyData make_beacon_body(uint32_t id_offset, const Vec3& position, double radius, const std::array<float, 3>& color) const noexcept {
		Render::GpuBodyData body{};
		body.position = {position[0], position[1], position[2], 0.0};
		body.velocity = {0.0, 0.0, 0.0, 0.0};
		body.color_primary = {static_cast<double>(color[0]), static_cast<double>(color[1]), static_cast<double>(color[2]), 1.0};
		body.color_secondary = body.color_primary;
		body.atmosphere_color = {0.0, 0.0, 0.0, 0.0};
		body.radius = radius;
		body.mass = 0.0;
		body.charge = 0.0;
		body.temperature = 0.0;
		body.atmosphere_thickness = 0.0;
		body.specular_roughness = 1.0;
		body.emission_intensity = 0.0;
		body.rotation_speed = 0.0;
		body.oblateness_ratio = 1.0;
		body.spin_parameter = 0.0;
		body.body_id = Render::kBeaconBodyIdBase + id_offset;
		body.geometry_model = 1U;
		body.surface_texture_mode = 1U;
		body.atmosphere_mode = 2U;
		body.preset_3d = 9U;
		body.polar_cap_strength = 0.0;
		body.ring_system_enabled = 0.0;
		body.night_side_light_intensity = 0.0;
		body.surface_layer_count = 0U;
		return body;
	}

	[[nodiscard]] Render::GpuCameraPushConstants build_push_constants(uint32_t width, uint32_t height, const ObserverFrame& frame) const {
		const auto& params = orchestrator_->parameters();
		using Orchestrator_t = Orchestrator::SimulationOrchestrator<1024>;

		const double mass = params.mass;
		const double spin_limit = 0.9999 * std::max(mass, 0.0);
		const double spin = std::clamp(params.spin, -spin_limit, spin_limit);
		const double horizon = (mass > 0.0) ? (mass + std::sqrt(std::max(mass * mass - spin * spin, 0.0))) : 0.0;

		const double observer_radius = std::max(std::sqrt(dot3(frame.position, frame.position)), 1.0e-6);
		const double observer_theta = std::acos(std::clamp(frame.position[2] / observer_radius, -1.0, 1.0));
		const double observer_phi = std::atan2(frame.position[1], frame.position[0]);

		Render::GpuCameraPushConstants consts{};
		consts.screen_width = width;
		consts.screen_height = height;
		consts.field_of_view_rad = frame.fov_rad;
		consts.metric_type = Orchestrator_t::metric_name_to_id(orchestrator_->active_metric_name());
		consts.metric_mass = params.mass;
		consts.metric_spin = params.spin;
		consts.metric_charge = params.charge;
		consts.cosmological_lambda = params.cosmological_lambda;
		consts.wormhole_throat = params.wormhole_throat;
		consts.warp_velocity = params.warp_velocity;
		consts.warp_bubble_radius = params.warp_bubble_radius;
		consts.warp_wall_sharpness = params.warp_wall_sharpness;
		consts.wormhole_tidal_potential = params.wormhole_tidal_potential;
		consts.teves_critical_acceleration = params.teves_critical_acceleration;
		consts.speed_of_light = orchestrator_->physical_speed_of_light();
		consts.gravitational_constant = orchestrator_->physical_gravitational_constant();
		consts.camera_exposure = exposure_;
		consts.tonemapping_mode = static_cast<uint32_t>(tonemapping_mode_);
		consts.horizon_radius = horizon;
		consts.escape_radius = frame.escape_radius;
		consts.projection_mode = frame.projection;
		consts.max_integration_steps = static_cast<uint32_t>(std::clamp(max_steps_, 32, 8192));
		consts.step_size_factor = params.integration_step_factor;
		consts.max_step_size = params.integration_max_step;
		consts.min_step_size = std::min(params.integration_min_step, params.integration_max_step);
		consts.far_field_step_scale = params.far_field_step_scale;
		consts.space_skip_radius_scale = params.space_skip_radius_scale;
		consts.pole_guard_precision_scale = params.pole_guard_precision_scale;
		consts.time = orchestrator_->scheduler().snapshot().logical_time;

		consts.render_flags = params.visual_overlays_flags;
		if (params.space_skipping_enabled) consts.render_flags |= Render::RenderFlags::SPACE_SKIP_ENABLED;
		if (params.body_shadows_enabled) consts.render_flags |= Render::RenderFlags::ENABLE_BODY_SHADOWS;
		if (params.body_disk_occlusion_enabled) consts.render_flags |= Render::RenderFlags::ENABLE_BODY_DISK_OCCLUSION;

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
		consts.sky_star_brightness_variation = params.sky_star_brightness_variation;
		consts.sky_star_size_variation = params.sky_star_size_variation;
		consts.sky_star_color_variation = params.sky_star_color_variation;
		consts.sky_star_temperature_bias = params.sky_star_temperature_bias;
		consts.sky_procedural_seed = params.sky_procedural_seed;
		consts.sky_background_source = params.sky_background_source;
		consts.sky_panorama_id = params.sky_panorama_id;
		consts.sky_panorama_quality = params.sky_panorama_quality;
		consts.sky_galaxy_density = params.sky_galaxy_density;
		consts.sky_galaxy_brightness = params.sky_galaxy_brightness;
		consts.sky_galaxy_size_scale = params.sky_galaxy_size_scale;
		consts.sky_dust_density = params.sky_dust_density;
		consts.sky_dust_intensity = params.sky_dust_intensity;
		consts.sky_dust_scale = params.sky_dust_scale;
		consts.sky_cluster_density = params.sky_cluster_density;
		consts.sky_cluster_brightness = params.sky_cluster_brightness;
		consts.sky_cluster_size_scale = params.sky_cluster_size_scale;

		consts.body_atmosphere_global_intensity = params.body_atmosphere_global_intensity;
		consts.body_render_lod_pixel_threshold = params.body_render_lod_pixel_threshold;
		consts.body_render_low_power_mode = params.body_render_low_power_mode ? 1U : 0U;
		consts.body_noise_octaves = params.body_noise_octaves;
		consts.body_render_point_pixel_threshold = params.body_render_point_pixel_threshold;

		consts.disk_temperature_scale_k = params.disk_temperature_scale_k;
		consts.disk_temperature_floor_k = params.disk_temperature_floor_k;
		consts.disk_doppler_beaming_exponent = params.disk_doppler_beaming_exponent;
		consts.disk_color_saturation = params.disk_color_saturation;

		consts.observer_position = {0.0, observer_radius, observer_theta, observer_phi};
		consts.tetrad_e0 = {1.0, 0.0, 0.0, 0.0};
		consts.tetrad_e1 = {0.0, frame.forward[0], frame.forward[1], frame.forward[2]};
		consts.tetrad_e2 = {0.0, frame.right[0], frame.right[1], frame.right[2]};
		consts.tetrad_e3 = {0.0, frame.up[0], frame.up[1], frame.up[2]};
		consts.observer_boost = frame.boost;

		orchestrator_->apply_lighting_constants(consts);
		orchestrator_->apply_primary_disk_constants(consts);
		orchestrator_->apply_dark_matter_constants(consts);
		orchestrator_->apply_cosmology_constants(consts);
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

		const ObserverFrame frame = resolve_frame();
		auto consts = build_push_constants(width, height, frame);
		std::vector<Render::GpuBodyData> gpu_bodies;
		{
			std::lock_guard<std::recursive_mutex> lock(orchestrator_->nbody_system().bodies_mutex());
			const auto nbody_sys = orchestrator_->nbody_system().bodies();
			gpu_bodies.reserve(nbody_sys.size());
			for (const auto& b : nbody_sys) {
				if (b.enabled) gpu_bodies.push_back(orchestrator_->make_gpu_body_data(b));
			}
		}
		if (!gpu_bodies.empty()) {
			consts.render_flags |= Render::RenderFlags::ENABLE_3D_BODY_RAYTRACING;
		}
		consts.body_count = static_cast<uint32_t>(gpu_bodies.size());

		if (has_rendered_once_ && !auto_refresh_ && consts == last_consts_) {
			return;
		}

		const size_t pixel_count = static_cast<size_t>(width) * static_cast<size_t>(height);
		std::vector<Render::GpuPixelOutput> fb(pixel_count, Render::GpuPixelOutput{});
		Render::SoftwareComputeEngine::dispatch_fp64(consts, fb, gpu_bodies, nullptr, nullptr);

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
	[[nodiscard]] SecondaryViewRole role() const noexcept { return role_; }
	[[nodiscard]] bool beacon_enabled() const noexcept { return beacon_enabled_; }
	[[nodiscard]] uint32_t view_id() const noexcept { return view_id_; }

	void set_role(SecondaryViewRole role) noexcept {
		role_ = role;
		has_rendered_once_ = false;
	}

	void set_view_id(uint32_t id) noexcept {
		view_id_ = id;
	}

	void bind_shared_target(const SecondaryTargetSettings* shared, bool force) noexcept {
		shared_target_ = shared;
		force_shared_target_ = force;
	}

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

	void store_extended_state(StateEntries& entries, const std::string& prefix) const {
		SecondaryStateIo::put_unsigned(entries, prefix + "role", static_cast<uint64_t>(role_));
		SecondaryStateIo::write_target(entries, prefix, local_target_);
		SecondaryStateIo::put_unsigned(entries, prefix + "use_shared", use_shared_target_ ? 1U : 0U);
		SecondaryStateIo::put_unsigned(entries, prefix + "beacon", beacon_enabled_ ? 1U : 0U);
		SecondaryStateIo::put_unsigned(entries, prefix + "probe_inbound", probe_inbound_ ? 1U : 0U);
		SecondaryStateIo::put_real(entries, prefix + "probe_speed", probe_speed_fraction_);
		SecondaryStateIo::put_real(entries, prefix + "look_yaw", look_yaw_deg_);
		SecondaryStateIo::put_real(entries, prefix + "look_pitch", look_pitch_deg_);
		SecondaryStateIo::put_real(entries, prefix + "asymptotic_distance", asymptotic_distance_);
		SecondaryStateIo::put_real(entries, prefix + "asymptotic_extent", asymptotic_half_extent_);
	}

	void restore_extended_state(const StateEntries& entries, const std::string& prefix) {
		const uint64_t role = SecondaryStateIo::read_unsigned(entries, prefix + "role", static_cast<uint64_t>(role_));
		role_ = static_cast<SecondaryViewRole>(role < kSecondaryViewRoleCount ? role : 0U);
		SecondaryStateIo::read_target(entries, prefix, local_target_);
		use_shared_target_ = SecondaryStateIo::read_unsigned(entries, prefix + "use_shared", use_shared_target_ ? 1U : 0U) != 0U;
		beacon_enabled_ = SecondaryStateIo::read_unsigned(entries, prefix + "beacon", beacon_enabled_ ? 1U : 0U) != 0U;
		probe_inbound_ = SecondaryStateIo::read_unsigned(entries, prefix + "probe_inbound", probe_inbound_ ? 1U : 0U) != 0U;
		probe_speed_fraction_ = std::clamp(SecondaryStateIo::read_real(entries, prefix + "probe_speed", probe_speed_fraction_), 0.0, 1.0);
		look_yaw_deg_ = std::clamp(SecondaryStateIo::read_real(entries, prefix + "look_yaw", look_yaw_deg_), -180.0, 180.0);
		look_pitch_deg_ = std::clamp(SecondaryStateIo::read_real(entries, prefix + "look_pitch", look_pitch_deg_), -89.0, 89.0);
		asymptotic_distance_ = std::clamp(SecondaryStateIo::read_real(entries, prefix + "asymptotic_distance", asymptotic_distance_), 100.0, 20000.0);
		asymptotic_half_extent_ = std::clamp(SecondaryStateIo::read_real(entries, prefix + "asymptotic_extent", asymptotic_half_extent_), 1.0, 2000.0);
		has_rendered_once_ = false;
	}

	void set_performance_budget_scale(float scale) noexcept {
		external_budget_scale_ = std::clamp(scale, 0.1f, 1.0f);
	}

	[[nodiscard]] ObserverFrame resolve_frame() const {
		const auto& params = orchestrator_->parameters();
		double effective_theta = theta_;
		double effective_phi = phi_;
		if (follow_primary_camera_) {
			const auto& main_camera = orchestrator_->camera();
			effective_theta = std::clamp(main_camera.theta + follow_offset_theta_, 0.02, std::numbers::pi_v<double> - 0.02);
			effective_phi = main_camera.phi + follow_offset_phi_;
		}

		const double mass = std::max(params.mass, 1.0e-4);
		const Vec3 target = resolve_target_position();
		const double sin_theta = std::sin(effective_theta);
		const Vec3 direction{sin_theta * std::cos(effective_phi), sin_theta * std::sin(effective_phi), std::cos(effective_theta)};
		const bool asymptotic = (role_ == SecondaryViewRole::AsymptoticGlobal);
		const double distance = asymptotic ? (asymptotic_distance_ * mass) : std::max(radius_, 1.0e-6);

		ObserverFrame frame;
		frame.position = {target[0] + direction[0] * distance, target[1] + direction[1] * distance, target[2] + direction[2] * distance};

		Vec3 forward{-direction[0], -direction[1], -direction[2]};
		Vec3 right = normalized(cross3(forward, Vec3{0.0, 0.0, 1.0}), normalized(cross3(forward, Vec3{0.0, 1.0, 0.0}), Vec3{1.0, 0.0, 0.0}));
		Vec3 up = cross3(right, forward);

		if (!asymptotic) {
			const double yaw = look_yaw_deg_ * (std::numbers::pi / 180.0);
			const double pitch = look_pitch_deg_ * (std::numbers::pi / 180.0);
			const double cy = std::cos(yaw);
			const double sy = std::sin(yaw);
			const double cp = std::cos(pitch);
			const double sp = std::sin(pitch);
			const Vec3 yawed_forward{forward[0] * cy + right[0] * sy, forward[1] * cy + right[1] * sy, forward[2] * cy + right[2] * sy};
			const Vec3 yawed_right{right[0] * cy - forward[0] * sy, right[1] * cy - forward[1] * sy, right[2] * cy - forward[2] * sy};
			forward = {yawed_forward[0] * cp + up[0] * sp, yawed_forward[1] * cp + up[1] * sp, yawed_forward[2] * cp + up[2] * sp};
			up = {up[0] * cp - yawed_forward[0] * sp, up[1] * cp - yawed_forward[1] * sp, up[2] * cp - yawed_forward[2] * sp};
			right = yawed_right;
		}
		frame.forward = forward;
		frame.right = right;
		frame.up = up;

		if (role_ == SecondaryViewRole::Equirectangular360) {
			frame.projection = 3U;
			frame.fov_rad = 60.0 * (std::numbers::pi / 180.0);
		} else if (asymptotic) {
			frame.projection = 0U;
			const double extent = asymptotic_half_extent_ * mass;
			frame.fov_rad = std::clamp(2.0 * std::atan(extent / std::max(distance, 1.0e-9)), 1.0e-4, 2.0);
		} else {
			frame.projection = static_cast<uint32_t>(std::clamp(projection_mode_, 0, 7));
			frame.fov_rad = fov_deg_ * (std::numbers::pi / 180.0);
		}

		const double origin_distance = std::max(std::sqrt(dot3(frame.position, frame.position)), 1.0e-6);
		frame.escape_radius = std::max(origin_distance * (asymptotic ? 1.5 : 4.0), 50.0);

		if (role_ == SecondaryViewRole::ComovingProbe) {
			const double free_fall_speed = std::sqrt(std::min(2.0 * mass / origin_distance, 0.999 * 0.999));
			const double speed = free_fall_speed * std::clamp(probe_speed_fraction_, 0.0, 1.0);
			const Vec3 radial = normalized(frame.position, direction);
			const double sign = probe_inbound_ ? -1.0 : 1.0;
			const Vec3 velocity_direction{sign * radial[0], sign * radial[1], sign * radial[2]};
			frame.boost = {
				speed * dot3(velocity_direction, frame.forward),
				speed * dot3(velocity_direction, frame.up),
				speed * dot3(velocity_direction, frame.right),
				0.0
			};
		}
		return frame;
	}

	void append_beacon_bodies(std::vector<Render::GpuBodyData>& bodies, const Vec3& viewer) const {
		if (orchestrator_ == nullptr || !beacon_enabled_ || !is_open_) {
			return;
		}
		const ObserverFrame frame = resolve_frame();
		const Vec3 offset{frame.position[0] - viewer[0], frame.position[1] - viewer[1], frame.position[2] - viewer[2]};
		const double distance = std::sqrt(dot3(offset, offset));
		const double beacon_radius = std::max(distance * 0.007, 0.02);
		if (distance < beacon_radius * 4.0) {
			return;
		}
		const auto color = hue_to_rgb(static_cast<float>(view_id_) * 0.61803398875f);
		bodies.push_back(make_beacon_body(view_id_ * 2U, frame.position, beacon_radius, color));
		const double marker_distance = beacon_radius * 2.4;
		const Vec3 marker_position{
			frame.position[0] + frame.forward[0] * marker_distance,
			frame.position[1] + frame.forward[1] * marker_distance,
			frame.position[2] + frame.forward[2] * marker_distance
		};
		bodies.push_back(make_beacon_body(view_id_ * 2U + 1U, marker_position, beacon_radius * 0.4, {1.0f, 1.0f, 1.0f}));
	}

	void render() {
		if (!is_open_ || orchestrator_ == nullptr) return;

		ImGui::SetNextWindowPos(ImVec2(400.0f, 100.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(560.0f, 560.0f), ImGuiCond_FirstUseEver);

		if (ImGui::Begin(name_.c_str(), &is_open_)) {
			const bool asymptotic = (role_ == SecondaryViewRole::AsymptoticGlobal);
			ImGui::BeginChild("SecondaryViewportImage", ImVec2(0.0f, ImGui::GetContentRegionAvail().y * 0.55f), true);
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
						if (asymptotic) {
							asymptotic_half_extent_ = std::clamp(asymptotic_half_extent_ * (1.0 - static_cast<double>(drag_io.MouseWheel) * 0.12), 1.0, 2000.0);
						} else {
							radius_ = std::clamp(radius_ * (1.0 - static_cast<double>(drag_io.MouseWheel) * 0.12), 1.5, 5000.0);
						}
					}
				}
			}
			ImGui::EndChild();

			ImGui::Separator();
			ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Observer Role");
			int role_index = static_cast<int>(role_);
			if (ImGui::Combo("Role", &role_index, kSecondaryViewRoleNames.data(), static_cast<int>(kSecondaryViewRoleNames.size()))) {
				role_ = static_cast<SecondaryViewRole>(role_index);
				has_rendered_once_ = false;
			}
			render_setting_tooltip("Free Observer orbits the target; Comoving Probe sees the sky aberrated by the velocity of a radially free-falling probe; Asymptotic Global observes from very far away with a narrow field for a quasi-orthogonal projection; Equirectangular 360 renders the full sky around the observer.");

			ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Observation Target");
			if (force_shared_target_ && shared_target_ != nullptr) {
				ImGui::TextDisabled("Target synchronized by the Secondary Viewports Manager: %s", kSecondaryTargetModeNames[static_cast<size_t>(shared_target_->mode)]);
			} else {
				if (shared_target_ != nullptr) {
					ImGui::Checkbox("Use Shared Target", &use_shared_target_);
					render_setting_tooltip("Follows the target configured in the Secondary Viewports Manager so several views look at the same point.");
				}
				if (use_shared_target_ && shared_target_ != nullptr) {
					ImGui::TextDisabled("Shared target: %s", kSecondaryTargetModeNames[static_cast<size_t>(shared_target_->mode)]);
				} else {
					static_cast<void>(render_secondary_target_editor(local_target_, *orchestrator_));
				}
			}

			ImGui::Checkbox("Show Beacon In Primary View", &beacon_enabled_);
			render_setting_tooltip("Renders a colored marker with a white heading dot at this observer position inside the primary raytraced viewport. It is lensed like any other body and requires the manager option to be enabled.");

			ImGui::Separator();
			ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Secondary Observer Placement");

			ImGui::Checkbox("Follow Primary Camera Orientation", &follow_primary_camera_);
			render_setting_tooltip("When enabled, this observer's angular position is derived from the primary viewport camera plus a fixed offset below, keeping the two views correlated as the primary camera moves.");

			if (follow_primary_camera_) {
				static_cast<void>(unit_aware_slider_double("Offset Theta", &follow_offset_theta_, -1.5, 1.5, UnitCategory::Angle, orchestrator_->unit_preferences(), "%.3f"));
				static_cast<void>(unit_aware_slider_double("Offset Phi", &follow_offset_phi_, -std::numbers::pi_v<double>, std::numbers::pi_v<double>, UnitCategory::Angle, orchestrator_->unit_preferences(), "%.3f"));
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
				render_setting_tooltip("Snaps this observer to a common cardinal viewing direction relative to the target. Left-drag the image above to orbit freely, and scroll over it to zoom.");

				const double dist_scale = std::max(orchestrator_->constants_engine().length_scale(), 1.0e-300);
				if (!asymptotic) {
					double r_meters = radius_ * dist_scale;
					if (unit_aware_slider_double("Radius", &r_meters, 2.0 * dist_scale, 500.0 * dist_scale, UnitCategory::Distance, orchestrator_->unit_preferences(), "%.2f")) {
						radius_ = r_meters / dist_scale;
					}
				}
				static_cast<void>(unit_aware_slider_double("Polar Angle (theta)", &theta_, 0.005, std::numbers::pi_v<double> - 0.005, UnitCategory::Angle, orchestrator_->unit_preferences(), "%.2f"));
				static_cast<void>(unit_aware_slider_double("Azimuthal Angle (phi)", &phi_, -std::numbers::pi_v<double>, std::numbers::pi_v<double>, UnitCategory::Angle, orchestrator_->unit_preferences(), "%.2f"));
				if (ImGui::Button("Reset To Equatorial View", ImVec2(-1.0f, 24.0f))) {
					theta_ = std::numbers::pi_v<double> / 2.0;
					phi_ = 0.0;
					radius_ = 60.0;
				}
			}

			if (asymptotic) {
				float distance_value = static_cast<float>(asymptotic_distance_);
				if (ImGui::SliderFloat("Observer Distance (M)", &distance_value, 100.0f, 20000.0f, "%.0f M", ImGuiSliderFlags_Logarithmic)) {
					asymptotic_distance_ = static_cast<double>(distance_value);
				}
				render_setting_tooltip("Distance of the asymptotic observer from the target in units of the central mass. Larger values make the viewing rays closer to parallel.");
				float extent_value = static_cast<float>(asymptotic_half_extent_);
				if (ImGui::SliderFloat("Half Extent (M)", &extent_value, 1.0f, 2000.0f, "%.1f M", ImGuiSliderFlags_Logarithmic)) {
					asymptotic_half_extent_ = static_cast<double>(extent_value);
				}
				render_setting_tooltip("Half height of the region framed around the target, in units of the central mass. The field of view is derived from it automatically; the mouse wheel adjusts it.");
			} else {
				float yaw_value = static_cast<float>(look_yaw_deg_);
				if (ImGui::SliderFloat("Look Yaw Offset", &yaw_value, -180.0f, 180.0f, "%.1f deg")) {
					look_yaw_deg_ = static_cast<double>(yaw_value);
				}
				float pitch_value = static_cast<float>(look_pitch_deg_);
				if (ImGui::SliderFloat("Look Pitch Offset", &pitch_value, -89.0f, 89.0f, "%.1f deg")) {
					look_pitch_deg_ = static_cast<double>(pitch_value);
				}
				render_setting_tooltip("Rotates the viewing direction away from the target. Combined with the Comoving Probe role it lets the cockpit look in any direction relative to its velocity.");
			}

			if (role_ == SecondaryViewRole::ComovingProbe) {
				ImGui::Separator();
				ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Comoving Probe Kinematics");
				ImGui::Checkbox("Inbound Free Fall", &probe_inbound_);
				render_setting_tooltip("Selects whether the probe falls toward the central object or moves away from it along the radial direction.");
				float fraction_value = static_cast<float>(probe_speed_fraction_);
				if (ImGui::SliderFloat("Fraction Of Escape Speed", &fraction_value, 0.0f, 1.0f, "%.3f")) {
					probe_speed_fraction_ = static_cast<double>(fraction_value);
				}
				render_setting_tooltip("Probe speed relative to the local static observer as a fraction of the free-fall speed from infinity, sqrt(2M/r). The rendered sky is relativistically aberrated accordingly.");
				const ObserverFrame frame = resolve_frame();
				const double beta = std::sqrt(frame.boost[0] * frame.boost[0] + frame.boost[1] * frame.boost[1] + frame.boost[2] * frame.boost[2]);
				ImGui::TextDisabled("Probe speed: %.4f c | Lorentz factor: %.4f", beta, 1.0 / std::sqrt(std::max(1.0 - beta * beta, 1.0e-12)));
			}

			ImGui::Separator();
			ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Optics");
			const bool optics_locked = (role_ == SecondaryViewRole::Equirectangular360) || asymptotic;
			ImGui::BeginDisabled(optics_locked);
			double fov_rad = fov_deg_ * (std::numbers::pi / 180.0);
			if (unit_aware_slider_double("Field of View", &fov_rad, 10.0 * (std::numbers::pi / 180.0), 150.0 * (std::numbers::pi / 180.0), UnitCategory::Angle, orchestrator_->unit_preferences(), "%.1f")) {
				fov_deg_ = fov_rad * (180.0 / std::numbers::pi);
			}
			const char* projections[] = {"Pinhole", "Auto-Zoom", "Fisheye Stereographic", "Equirectangular 360", "Fisheye Equidistant", "Fisheye Orthographic", "Panini Cylindrical", "Hammer-Aitoff"};
			ImGui::Combo("Projection", &projection_mode_, projections, IM_ARRAYSIZE(projections));
			ImGui::EndDisabled();
			if (optics_locked) {
				ImGui::TextDisabled("Projection and field of view are fixed by the selected role.");
			}
			float exposure = static_cast<float>(exposure_);
			if (ImGui::SliderFloat("Exposure", &exposure, -6.0f, 6.0f, "%.2f EV")) exposure_ = exposure;
			const char* tonemappers[] = {"Linear Unclamped", "ACES Filmic Curve", "Logarithmic Extended HDR", "Reinhard Modified"};
			ImGui::Combo("Tonemapper", &tonemapping_mode_, tonemappers, IM_ARRAYSIZE(tonemappers));

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

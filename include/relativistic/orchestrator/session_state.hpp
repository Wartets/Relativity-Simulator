#pragma once

#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/render/bodies/body_lighting.hpp"
#include "relativistic/render/gpu_types.hpp"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unordered_map>

namespace Relativistic::Orchestrator {

class SessionStateStore {
public:
	using Entries = std::unordered_map<std::string, std::string>;
	using OrchestratorType = SimulationOrchestrator<1024>;

	static void capture(const OrchestratorType& orchestrator, Entries& out) {
		const PhysicalParameters& p = orchestrator.parameters();
		for (const RealField& field : kRealFields) {
			out[field.key] = format_real(p.*(field.member));
		}
		for (const UnsignedField& field : kUnsignedFields) {
			out[field.key] = std::to_string(p.*(field.member));
		}
		for (const BoolField& field : kBoolFields) {
			out[field.key] = (p.*(field.member)) ? "1" : "0";
		}
		out["overlay_flags"] = std::to_string(p.visual_overlays_flags & kPersistedOverlayFlags);
		const Render::HydroDiskSettings& hydro = p.hydro_disk;
		out["hydro_disk_model"] = std::to_string(static_cast<uint32_t>(hydro.model));
		out["hydro_disk_accretion_rate_scale"] = format_real(static_cast<double>(hydro.accretion_rate_scale));
		out["hydro_disk_torus_inner_radius"] = format_real(static_cast<double>(hydro.torus_inner_radius));
		out["hydro_disk_torus_center_radius"] = format_real(static_cast<double>(hydro.torus_center_radius));
		out["hydro_disk_adiabatic_index"] = format_real(static_cast<double>(hydro.adiabatic_index));
		out["hydro_disk_optical_depth_scale"] = format_real(static_cast<double>(hydro.optical_depth_scale));
		out["hydro_disk_sampling_density"] = format_real(static_cast<double>(hydro.sampling_density));
		p.jet.store(out);
		p.dark_matter.store(out);
		const CameraState& camera = orchestrator.camera();
		out["cam_x"] = format_real(camera.position[0]);
		out["cam_y"] = format_real(camera.position[1]);
		out["cam_z"] = format_real(camera.position[2]);
		out["cam_pitch"] = format_real(camera.pitch);
		out["cam_yaw"] = format_real(camera.yaw);
		out["cam_roll"] = format_real(camera.roll);
		out["cam_fov"] = format_real(camera.fov_deg);
		out["cam_speed"] = format_real(camera.speed);
		out["warp_factor"] = format_real(orchestrator.scheduler().warp_factor());
		out["metric_name"] = orchestrator.active_metric_name();
		out["integrator_name"] = orchestrator.active_integrator_name();
	}

	static void apply(OrchestratorType& orchestrator, const Entries& in) {
		PhysicalParameters& p = orchestrator.parameters();
		for (const RealField& field : kRealFields) {
			if (const auto it = in.find(field.key); it != in.end()) {
				const double value = std::strtod(it->second.c_str(), nullptr);
				if (std::isfinite(value)) {
					p.*(field.member) = std::clamp(value, field.minimum, field.maximum);
				}
			}
		}
		for (const UnsignedField& field : kUnsignedFields) {
			if (const auto it = in.find(field.key); it != in.end()) {
				const auto value = static_cast<uint32_t>(std::min<unsigned long long>(std::strtoull(it->second.c_str(), nullptr, 10), field.maximum));
				p.*(field.member) = std::max(value, field.minimum);
			}
		}
		for (const BoolField& field : kBoolFields) {
			if (const auto it = in.find(field.key); it != in.end()) {
				p.*(field.member) = std::strtoul(it->second.c_str(), nullptr, 10) != 0UL;
			}
		}
		if (const auto it = in.find("overlay_flags"); it != in.end()) {
			const auto saved = static_cast<uint32_t>(std::strtoull(it->second.c_str(), nullptr, 10));
			p.visual_overlays_flags = (p.visual_overlays_flags & ~kPersistedOverlayFlags) | (saved & kPersistedOverlayFlags);
		}
		p.spin = std::clamp(p.spin, -0.999 * p.mass, 0.999 * p.mass);

		const auto read_hydro_value = [&in](const char* key, float fallback) noexcept -> float {
			const auto it = in.find(key);
			if (it == in.end()) {
				return fallback;
			}
			const double value = std::strtod(it->second.c_str(), nullptr);
			return std::isfinite(value) ? static_cast<float>(value) : fallback;
		};
		Render::HydroDiskSettings& hydro = p.hydro_disk;
		if (const auto it = in.find("hydro_disk_model"); it != in.end()) {
			hydro.model = static_cast<Render::HydroDiskModel>(std::min<unsigned long long>(std::strtoull(it->second.c_str(), nullptr, 10), static_cast<unsigned long long>(Render::kHydroDiskModelCount - 1)));
		}
		hydro.accretion_rate_scale = read_hydro_value("hydro_disk_accretion_rate_scale", hydro.accretion_rate_scale);
		hydro.torus_inner_radius = read_hydro_value("hydro_disk_torus_inner_radius", hydro.torus_inner_radius);
		hydro.torus_center_radius = read_hydro_value("hydro_disk_torus_center_radius", hydro.torus_center_radius);
		hydro.adiabatic_index = read_hydro_value("hydro_disk_adiabatic_index", hydro.adiabatic_index);
		hydro.optical_depth_scale = read_hydro_value("hydro_disk_optical_depth_scale", hydro.optical_depth_scale);
		hydro.sampling_density = read_hydro_value("hydro_disk_sampling_density", hydro.sampling_density);
		hydro.sanitize();
		p.jet.restore(in);
		p.dark_matter.restore(in);

		CameraState& camera = orchestrator.camera();
		const auto real = [&in](const char* key, double fallback) noexcept {
			const auto it = in.find(key);
			if (it == in.end()) {
				return fallback;
			}
			const double value = std::strtod(it->second.c_str(), nullptr);
			return std::isfinite(value) ? value : fallback;
		};
		if (in.contains("cam_x") && in.contains("cam_y") && in.contains("cam_z")) {
			camera.position = {real("cam_x", camera.position[0]), real("cam_y", camera.position[1]), real("cam_z", camera.position[2])};
			camera.pitch = std::clamp(real("cam_pitch", camera.pitch), -89.0, 89.0);
			camera.yaw = real("cam_yaw", camera.yaw);
			camera.roll = real("cam_roll", camera.roll);
			camera.velocity = {0.0, 0.0, 0.0};
			camera.synchronize_spherical();
			camera.orbit_distance = camera.radius;
		}
		camera.fov_deg = std::clamp(real("cam_fov", camera.fov_deg), 5.0, 175.0);
		camera.speed = std::max(real("cam_speed", camera.speed), 0.001);
		p.camera_fov_deg = camera.fov_deg;
		p.camera_speed = camera.speed;

		const double warp = real("warp_factor", orchestrator.scheduler().warp_factor());
		orchestrator.scheduler().set_warp_factor(warp);
		if (const auto it = in.find("metric_name"); it != in.end() && !it->second.empty()) {
			orchestrator.set_active_metric_name(it->second);
		}
		if (const auto it = in.find("integrator_name"); it != in.end() && !it->second.empty()) {
			orchestrator.set_active_integrator_name(it->second);
		}
		orchestrator.notify_state_changed();
	}

private:
	struct RealField {
		const char* key;
		double PhysicalParameters::* member;
		double minimum;
		double maximum;
	};

	struct UnsignedField {
		const char* key;
		uint32_t PhysicalParameters::* member;
		uint32_t minimum;
		uint32_t maximum;
	};

	struct BoolField {
		const char* key;
		bool PhysicalParameters::* member;
	};

	static constexpr uint32_t kPersistedOverlayFlags =
		Render::RenderFlags::SKYBOX_MODE_MASK
		| Render::RenderFlags::USE_GRID_SKYBOX
		| Render::RenderFlags::ENABLE_3D_BODY_RAYTRACING
		| Render::RenderFlags::ENABLE_BODY_DOPPLER_BEAMING
		| Render::RenderFlags::ENABLE_BODY_GRAV_REDSHIFT
		| Render::RenderFlags::ENABLE_ATMOSPHERE_SCATTERING;

	static inline constexpr RealField kRealFields[] = {
		{"mass", &PhysicalParameters::mass, 0.0, 1.0e12},
		{"spin", &PhysicalParameters::spin, -1.0e12, 1.0e12},
		{"charge", &PhysicalParameters::charge, -1.0e12, 1.0e12},
		{"cosmological_lambda", &PhysicalParameters::cosmological_lambda, 0.0, 1.0},
		{"wormhole_throat", &PhysicalParameters::wormhole_throat, 0.01, 1.0e6},
		{"warp_velocity", &PhysicalParameters::warp_velocity, 0.0, 100.0},
		{"camera_exposure", &PhysicalParameters::camera_exposure, -6.0, 6.0},
		{"camera_collision_clearance", &PhysicalParameters::camera_collision_clearance, 0.0, 1.0e3},
		{"post_contrast", &PhysicalParameters::post_contrast, 0.1, 3.0},
		{"post_saturation", &PhysicalParameters::post_saturation, 0.0, 3.0},
		{"post_lift", &PhysicalParameters::post_lift, -0.5, 0.5},
		{"post_gamma", &PhysicalParameters::post_gamma, 0.2, 3.0},
		{"post_gain", &PhysicalParameters::post_gain, 0.1, 3.0},
		{"post_vignette_strength", &PhysicalParameters::post_vignette_strength, 0.0, 1.0},
		{"post_highlights", &PhysicalParameters::post_highlights, -1.0, 1.0},
		{"post_shadows", &PhysicalParameters::post_shadows, -1.0, 1.0},
		{"sky_star_density", &PhysicalParameters::sky_star_density, 0.0, 100.0},
		{"sky_star_brightness", &PhysicalParameters::sky_star_brightness, 0.0, 100.0},
		{"sky_nebula_intensity", &PhysicalParameters::sky_nebula_intensity, 0.0, 100.0},
		{"sky_grid_opacity", &PhysicalParameters::sky_grid_opacity, 0.0, 100.0},
		{"sky_rotation_deg", &PhysicalParameters::sky_rotation_deg, -3600.0, 3600.0},
		{"sky_hue_shift_deg", &PhysicalParameters::sky_hue_shift_deg, -3600.0, 3600.0},
		{"sky_saturation", &PhysicalParameters::sky_saturation, 0.0, 100.0},
		{"sky_background_r", &PhysicalParameters::sky_background_r, 0.0, 1.0},
		{"sky_background_g", &PhysicalParameters::sky_background_g, 0.0, 1.0},
		{"sky_background_b", &PhysicalParameters::sky_background_b, 0.0, 1.0},
		{"sky_star_brightness_variation", &PhysicalParameters::sky_star_brightness_variation, 0.0, 1.0},
		{"sky_star_size_variation", &PhysicalParameters::sky_star_size_variation, 0.0, 1.0},
		{"sky_star_color_variation", &PhysicalParameters::sky_star_color_variation, 0.0, 2.0},
		{"sky_star_temperature_bias", &PhysicalParameters::sky_star_temperature_bias, -1.0, 1.0},
		{"sky_galaxy_density", &PhysicalParameters::sky_galaxy_density, 0.0, 4.0},
		{"sky_galaxy_brightness", &PhysicalParameters::sky_galaxy_brightness, 0.0, 4.0},
		{"sky_galaxy_size_scale", &PhysicalParameters::sky_galaxy_size_scale, 0.05, 8.0},
		{"sky_dust_density", &PhysicalParameters::sky_dust_density, 0.0, 4.0},
		{"sky_dust_intensity", &PhysicalParameters::sky_dust_intensity, 0.0, 4.0},
		{"sky_dust_scale", &PhysicalParameters::sky_dust_scale, 0.05, 8.0},
		{"sky_cluster_density", &PhysicalParameters::sky_cluster_density, 0.0, 4.0},
		{"sky_cluster_brightness", &PhysicalParameters::sky_cluster_brightness, 0.0, 4.0},
		{"sky_cluster_size_scale", &PhysicalParameters::sky_cluster_size_scale, 0.05, 8.0},
		{"disk_temperature_scale_k", &PhysicalParameters::disk_temperature_scale_k, 1000.0, 60000.0},
		{"disk_temperature_floor_k", &PhysicalParameters::disk_temperature_floor_k, 0.0, 20000.0},
		{"disk_doppler_beaming_exponent", &PhysicalParameters::disk_doppler_beaming_exponent, -2.0, 8.0},
		{"disk_color_saturation", &PhysicalParameters::disk_color_saturation, 0.0, 3.0},
		{"light_intensity", &PhysicalParameters::light_intensity, 0.0, 50.0},
		{"light_color_r", &PhysicalParameters::light_color_r, 0.0, 4.0},
		{"light_color_g", &PhysicalParameters::light_color_g, 0.0, 4.0},
		{"light_color_b", &PhysicalParameters::light_color_b, 0.0, 4.0},
		{"light_ambient", &PhysicalParameters::light_ambient, 0.0, 1.0},
		{"light_terminator_softness", &PhysicalParameters::light_terminator_softness, 0.0, 1.0},
		{"light_specular_scale", &PhysicalParameters::light_specular_scale, 0.0, 8.0},
		{"light_direction_azimuth_deg", &PhysicalParameters::light_direction_azimuth_deg, -360.0, 360.0},
		{"light_direction_elevation_deg", &PhysicalParameters::light_direction_elevation_deg, -90.0, 90.0},
		{"light_position_x", &PhysicalParameters::light_position_x, -1.0e9, 1.0e9},
		{"light_position_y", &PhysicalParameters::light_position_y, -1.0e9, 1.0e9},
		{"light_position_z", &PhysicalParameters::light_position_z, -1.0e9, 1.0e9},
		{"light_reference_distance", &PhysicalParameters::light_reference_distance, 1.0e-3, 1.0e7},
		{"body_emission_lighting_gain", &PhysicalParameters::body_emission_lighting_gain, 0.0, 1.0e5},
		{"body_emission_lighting_reference_distance", &PhysicalParameters::body_emission_lighting_reference_distance, 1.0e-3, 1.0e7},
		{"body_atmosphere_global_intensity", &PhysicalParameters::body_atmosphere_global_intensity, 0.0, 3.0}
	};

	static inline constexpr UnsignedField kUnsignedFields[] = {
		{"projection_mode", &PhysicalParameters::projection_mode, 0U, 7U},
		{"time_flow_mode", &PhysicalParameters::time_flow_mode, 0U, 1U},
		{"tonemapping_mode", &PhysicalParameters::tonemapping_mode, 0U, 3U},
		{"sky_procedural_seed", &PhysicalParameters::sky_procedural_seed, 0U, 0xFFFFFFFFU},
		{"sky_background_source", &PhysicalParameters::sky_background_source, 0U, 1U},
		{"sky_panorama_id", &PhysicalParameters::sky_panorama_id, 0U, 2U},
		{"sky_panorama_quality", &PhysicalParameters::sky_panorama_quality, 0U, 2U},
		{"light_source_mode", &PhysicalParameters::light_source_mode, 0U, static_cast<uint32_t>(Render::kLightSourceModeCount) - 1U},
		{"light_attenuation_mode", &PhysicalParameters::light_attenuation_mode, 0U, static_cast<uint32_t>(Render::kLightAttenuationModeCount) - 1U},
		{"light_source_body_id", &PhysicalParameters::light_source_body_id, 0U, 0xFFFFFFFFU},
		{"rolling_average_frame_count", &PhysicalParameters::rolling_average_frame_count, 2U, 120U}
	};

	static inline constexpr BoolField kBoolFields[] = {
		{"camera_collision_enabled", &PhysicalParameters::camera_collision_enabled},
		{"schematic_mode_enabled", &PhysicalParameters::schematic_mode_enabled},
		{"schematic_allow_simulation", &PhysicalParameters::schematic_allow_simulation},
		{"body_emission_lighting_enabled", &PhysicalParameters::body_emission_lighting_enabled}
	};

	[[nodiscard]] static std::string format_real(double value) {
		char buffer[40];
		std::snprintf(buffer, sizeof(buffer), "%.17g", value);
		return buffer;
	}
};

}

#pragma once

#include <imgui.h>
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/io/user_settings.hpp"
#include "relativistic/render/skybox_renderer.hpp"
#include "relativistic/optics/textures/sky_panorama_catalog.hpp"
#include "relativistic/ui/numeric_slider_utils.hpp"
#include "relativistic/ui/tooltip_utils.hpp"
#include <algorithm>
#include <chrono>
#include <cstdint>

namespace Relativistic::UI {

inline void render_skybox_editor(Orchestrator::SimulationOrchestrator<1024>& orchestrator, IO::UserSettings& user_settings) noexcept {
	auto& params = orchestrator.parameters();
	auto& sky = params.sky;
	bool changed = false;

	ImGui::PushID("SkyboxEnvironmentEditor");

	changed = ImGui::Checkbox("Enable Skybox & Celestial Background", &sky.enabled) || changed;
	render_setting_tooltip("Master switch for all celestial background rendering. When unchecked, background rendering is completely disabled.");

	if (!sky.enabled) {
		ImGui::Spacing();
		ImGui::TextColored(ImVec4(0.7f, 0.4f, 0.4f, 1.0f), "Skybox and celestial background rendering is disabled.");
		user_settings.sky_rendering_enabled = false;
		if (changed) {
			params.sync_sky_fields();
			orchestrator.notify_state_changed();
		}
		ImGui::PopID();
		return;
	}

	user_settings.sky_rendering_enabled = true;
	ImGui::Separator();

	ImGui::TextColored(ImVec4(0.3f, 0.9f, 1.0f, 1.0f), "Sky Background Source");
	const char* sky_sources[] = {"Procedural Sky Generator", "Imported Real Sky Panorama"};
	int background_source = static_cast<int>(params.sky_background_source);
	if (ImGui::Combo("Background Source", &background_source, sky_sources, IM_ARRAYSIZE(sky_sources))) {
		params.sky_background_source = static_cast<uint32_t>(background_source);
		changed = true;
	}
	render_setting_tooltip("Select between the spherical procedural celestial background generator and an imported equirectangular sky panorama.");

	ImGui::Separator();

	if (params.sky_background_source == 1U) {
		static constexpr std::array<const char*, 3> kPanoramaNames{"Night Sky HDRI 001 (ambientCG)", "Night Sky HDRI 008 (ambientCG)", "ESO 0932a (ESO Observatory Photograph)"};
		int panorama_id = static_cast<int>(params.sky_panorama_id);
		if (ImGui::Combo("Sky Panorama", &panorama_id, kPanoramaNames.data(), static_cast<int>(kPanoramaNames.size()))) {
			params.sky_panorama_id = static_cast<uint32_t>(panorama_id);
			changed = true;
		}
		render_setting_tooltip("Chooses the photographic celestial sphere dataset projected as the equirectangular background.");

		const auto p_id = static_cast<Optics::SkyPanoramaId>(params.sky_panorama_id);
		const auto& entry = Optics::sky_panorama_catalog_entry(p_id);

		if (entry.has_quality_variants) {
			const char* quality_names[] = {"1K (1024x512, Fast)", "2K (2048x1024, Balanced)", "4K (4096x2048, High Detail)"};
			int panorama_quality = static_cast<int>(params.sky_panorama_quality);
			if (ImGui::Combo("Panorama Resolution", &panorama_quality, quality_names, IM_ARRAYSIZE(quality_names))) {
				params.sky_panorama_quality = static_cast<uint32_t>(panorama_quality);
				changed = true;
			}
			render_setting_tooltip("Resolution level decoded from the panorama package.");
		} else {
			ImGui::TextDisabled("This panorama is supplied in its native photographic resolution (6000x3000).");
		}

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.5f, 0.75f, 1.0f, 1.0f), "Panorama Color Adjustments");
		changed = slider_double_with_input("Panorama Rotation (deg)", &sky.orientation_yaw_deg, -180.0, 180.0, "%.1f") || changed;
		changed = slider_double_with_input("Panorama Hue Shift (deg)", &sky.hue_shift_deg, -180.0, 180.0, "%.1f") || changed;
		changed = slider_double_with_input("Panorama Saturation", &sky.saturation, 0.0, 2.0, "%.2f") || changed;
		changed = ImGui::ColorEdit3("Panorama Ambient Pedestal", sky.background_color.data()) || changed;

		if (changed) {
			params.sync_sky_fields();
			orchestrator.notify_state_changed();
		}
		ImGui::PopID();
		return;
	}

	ImGui::TextColored(ImVec4(0.5f, 0.85f, 1.0f, 1.0f), "Procedural Seed & Session Locking");
	changed = ImGui::Checkbox("Lock Procedural Seed Across Sessions", &sky.seed_locked) || changed;
	render_setting_tooltip("When locked, the procedural seed is saved and restored across application restarts. When unlocked, a new random seed is generated at each application launch.");
	user_settings.sky_seed_locked = sky.seed_locked;

	ImGui::SameLine();
	if (ImGui::Button("Randomize Seed")) {
		sky.seed = static_cast<uint32_t>(std::chrono::steady_clock::now().time_since_epoch().count() & 0xFFFFFFFFU);
		params.sky_procedural_seed = sky.seed;
		changed = true;
	}
	render_setting_tooltip("Generates a new random seed for the procedural generator immediately.");

	int current_seed = static_cast<int>(sky.seed);
	if (ImGui::InputInt("Seed", &current_seed)) {
		sky.seed = static_cast<uint32_t>(std::max(0, current_seed));
		params.sky_procedural_seed = sky.seed;
		changed = true;
	}

	changed = slider_double_with_input("Master Sky Intensity", &sky.master_intensity, 0.0, 10.0, "%.3f") || changed;
	render_setting_tooltip("Global scalar multiplier for all background celestial radiance.");

	ImGui::Spacing();
	if (ImGui::CollapsingHeader("Quality & Dynamic Motion", ImGuiTreeNodeFlags_DefaultOpen)) {
		changed = slider_double_with_input("Quality Factor", &sky.quality_factor, 0.1, 4.0, "%.2f") || changed;
		render_setting_tooltip("Scales spatial subdivision density and sampling fidelity for celestial point sources and extended structures.");

		changed = ImGui::Checkbox("Adaptive FOV Quality Scaling (Dynamic LOD)", &sky.adaptive_fov_lod) || changed;
		render_setting_tooltip("Dynamically scales high-frequency procedural detail as camera FOV decreases, keeping star flux constant and revealing finer structure without popping.");

		if (sky.adaptive_fov_lod) {
			changed = slider_double_with_input("Adaptive Reference FOV (deg)", &sky.adaptive_fov_reference, 10.0, 120.0, "%.1f") || changed;
			render_setting_tooltip("Reference field of view above which the base LOD is applied and below which high-frequency octaves are introduced.");
		}

		changed = ImGui::Checkbox("Dynamic Celestial Motion", &sky.dynamic_motion) || changed;
		render_setting_tooltip("Enables subtle time-dependent proper motion and stellar pulsation, strictly deterministic with simulation clock.");

		if (sky.dynamic_motion) {
			changed = slider_double_with_input("Motion Speed", &sky.motion_speed, 0.0, 1.0, "%.3f") || changed;
		}
	}

	if (ImGui::CollapsingHeader("Stars & Stellar Population", ImGuiTreeNodeFlags_DefaultOpen)) {
		changed = ImGui::Checkbox("Enable Stars", &sky.stars_enabled) || changed;
		if (sky.stars_enabled) {
			changed = slider_double_with_input("Star Density", &sky.star_density, 0.01, 5.0, "%.3f") || changed;
			changed = slider_double_with_input("Star Brightness", &sky.star_brightness, 0.0, 10.0, "%.2f") || changed;
			changed = slider_double_with_input("Minimum Angular Size (rad)", &sky.star_size_min, 0.0001, 0.005, "%.5f") || changed;
			changed = slider_double_with_input("Maximum Angular Size (rad)", &sky.star_size_max, 0.0005, 0.015, "%.5f") || changed;

			static const char* kSizeDistributions[] = {"Uniform", "Salpeter Power-Law", "Exponential", "Log-Normal", "Gaussian"};
			int current_size_dist = static_cast<int>(std::min(sky.star_size_distribution, 4U));
			if (ImGui::Combo("Size Distribution", &current_size_dist, kSizeDistributions, IM_ARRAYSIZE(kSizeDistributions))) {
				sky.star_size_distribution = static_cast<uint32_t>(current_size_dist);
				changed = true;
			}
			render_setting_tooltip("Statistical probability distribution governing stellar radii.");

			if (sky.star_size_distribution != 0U) {
				changed = slider_double_with_input("Size Distribution Parameter", &sky.star_size_param, 0.1, 5.0, "%.2f") || changed;
			}

			static const char* kLumDistributions[] = {"Uniform", "Power-Law", "Exponential", "Log-Normal", "Gaussian"};
			int current_lum_dist = static_cast<int>(std::min(sky.star_luminosity_distribution, 4U));
			if (ImGui::Combo("Luminosity Distribution", &current_lum_dist, kLumDistributions, IM_ARRAYSIZE(kLumDistributions))) {
				sky.star_luminosity_distribution = static_cast<uint32_t>(current_lum_dist);
				changed = true;
			}
			render_setting_tooltip("Statistical probability distribution governing stellar luminosities.");

			if (sky.star_luminosity_distribution != 0U) {
				changed = slider_double_with_input("Luminosity Distribution Parameter", &sky.star_luminosity_param, 0.1, 5.0, "%.2f") || changed;
			}

			changed = slider_double_with_input("Color Variation", &sky.star_color_variation, 0.0, 3.0, "%.2f") || changed;
			changed = slider_double_with_input("Temperature Bias", &sky.star_temperature_bias, -1.0, 1.0, "%.2f") || changed;
			render_setting_tooltip("Shifts the stellar population toward cooler red stars (-1) or hotter blue stars (+1).");

			changed = slider_double_with_input("Brightness Variation", &sky.star_brightness_variation, 0.0, 1.0, "%.2f") || changed;
			changed = slider_double_with_input("Magnitude Limit", &sky.star_magnitude_limit, 4.0, 22.0, "%.1f") || changed;
			changed = slider_double_with_input("Scintillation / Twinkle", &sky.star_scintillation, 0.0, 1.0, "%.2f") || changed;
		}
	}

	if (ImGui::CollapsingHeader("Volumetric Nebulae & Galactic Plane", ImGuiTreeNodeFlags_DefaultOpen)) {
		changed = ImGui::Checkbox("Enable Nebulae", &sky.nebula_enabled) || changed;
		if (sky.nebula_enabled) {
			changed = slider_double_with_input("Nebula Glow Intensity", &sky.nebula_intensity, 0.0, 5.0, "%.2f") || changed;
			changed = slider_double_with_input("Roughness", &sky.nebula_roughness, 0.1, 0.95, "%.2f") || changed;
			int octaves = static_cast<int>(sky.nebula_octaves);
			if (ImGui::SliderInt("Fractal Octaves", &octaves, 1, 8)) {
				sky.nebula_octaves = static_cast<uint32_t>(octaves);
				changed = true;
			}
			changed = slider_double_with_input("Base Frequency", &sky.nebula_frequency, 0.5, 10.0, "%.2f") || changed;

			float plane_norm[3] = {static_cast<float>(sky.nebula_plane_normal[0]), static_cast<float>(sky.nebula_plane_normal[1]), static_cast<float>(sky.nebula_plane_normal[2])};
			if (ImGui::InputFloat3("Galactic Plane Normal", plane_norm, "%.3f")) {
				sky.nebula_plane_normal = {plane_norm[0], plane_norm[1], plane_norm[2]};
				changed = true;
			}
			render_setting_tooltip("Direction vector perpendicular to the central galactic plane.");

			changed = slider_double_with_input("Plane Thickness", &sky.nebula_plane_thickness, 0.02, 1.0, "%.3f") || changed;
			changed = slider_double_with_input("Plane Falloff Power", &sky.nebula_plane_falloff, 1.0, 6.0, "%.2f") || changed;
			changed = slider_double_with_input("Directional Asymmetry", &sky.nebula_asymmetry, 0.0, 2.0, "%.2f") || changed;

			float asym_dir[3] = {static_cast<float>(sky.nebula_asymmetry_dir[0]), static_cast<float>(sky.nebula_asymmetry_dir[1]), static_cast<float>(sky.nebula_asymmetry_dir[2])};
			if (ImGui::InputFloat3("Asymmetry Direction Vector", asym_dir, "%.3f")) {
				sky.nebula_asymmetry_dir = {asym_dir[0], asym_dir[1], asym_dir[2]};
				changed = true;
			}

			changed = ImGui::ColorEdit3("Primary Emission Color", sky.nebula_emission_color.data()) || changed;
			changed = ImGui::ColorEdit3("Secondary Gas Color", sky.nebula_secondary_color.data()) || changed;
			changed = ImGui::ColorEdit3("Tertiary Gas Color", sky.nebula_tertiary_color.data()) || changed;
			changed = slider_double_with_input("Dust Extinction Density", &sky.nebula_absorption_density, 0.0, 3.0, "%.2f") || changed;

			int layers = static_cast<int>(sky.nebula_layer_count);
			if (ImGui::SliderInt("Volumetric Layer Count", &layers, 1, 5)) {
				sky.nebula_layer_count = static_cast<uint32_t>(layers);
				changed = true;
			}
			changed = slider_double_with_input("Layer Separation Spread", &sky.nebula_layer_spread, 0.1, 2.0, "%.2f") || changed;
			changed = slider_double_with_input("Galactic Core Glow", &sky.nebula_core_glow, 0.0, 3.0, "%.2f") || changed;
			changed = slider_double_with_input("Curl Turbulence Strength", &sky.nebula_curl_strength, 0.0, 1.0, "%.2f") || changed;
		}
	}

	if (ImGui::CollapsingHeader("Deep Field Galaxies")) {
		changed = ImGui::Checkbox("Enable Distant Galaxies", &sky.galaxies_enabled) || changed;
		if (sky.galaxies_enabled) {
			changed = slider_double_with_input("Galaxy Density", &sky.galaxy_density, 0.0, 2.0, "%.3f") || changed;
			changed = slider_double_with_input("Galaxy Brightness", &sky.galaxy_brightness, 0.0, 5.0, "%.2f") || changed;
			changed = slider_double_with_input("Size Scale", &sky.galaxy_size_scale, 0.05, 4.0, "%.2f") || changed;
			changed = slider_double_with_input("Size Min (rad)", &sky.galaxy_size_min, 0.0005, 0.02, "%.4f") || changed;
			changed = slider_double_with_input("Size Max (rad)", &sky.galaxy_size_max, 0.005, 0.08, "%.4f") || changed;
			static const char* kGalDist[] = {"Uniform", "Power-Law", "Exponential", "Log-Normal", "Gaussian"};
			int gal_dist_idx = static_cast<int>(std::min(sky.galaxy_size_distribution, 4U));
			if (ImGui::Combo("Size Distribution##Gal", &gal_dist_idx, kGalDist, IM_ARRAYSIZE(kGalDist))) {
				sky.galaxy_size_distribution = static_cast<uint32_t>(gal_dist_idx);
				changed = true;
			}
			changed = slider_double_with_input("Spiral Arms Count", &sky.galaxy_spiral_arms, 1.0, 6.0, "%.1f") || changed;
			changed = slider_double_with_input("Arm Pitch Angle", &sky.galaxy_pitch_angle, 0.2, 3.0, "%.2f") || changed;
			changed = slider_double_with_input("Core to Disk Ratio", &sky.galaxy_core_ratio, 0.0, 1.0, "%.2f") || changed;
			changed = slider_double_with_input("Ellipticity Variance", &sky.galaxy_ellipticity_variance, 0.0, 2.0, "%.2f") || changed;
		}
	}

	if (ImGui::CollapsingHeader("Star Clusters")) {
		changed = ImGui::Checkbox("Enable Star Clusters", &sky.clusters_enabled) || changed;
		if (sky.clusters_enabled) {
			changed = slider_double_with_input("Cluster Density", &sky.cluster_density, 0.0, 3.0, "%.3f") || changed;
			changed = slider_double_with_input("Cluster Brightness", &sky.cluster_brightness, 0.0, 5.0, "%.2f") || changed;
			changed = slider_double_with_input("Cluster Size Scale", &sky.cluster_size_scale, 0.05, 4.0, "%.2f") || changed;
			changed = slider_double_with_input("Cluster Size Min (rad)", &sky.cluster_size_min, 0.001, 0.03, "%.4f") || changed;
			changed = slider_double_with_input("Cluster Size Max (rad)", &sky.cluster_size_max, 0.005, 0.1, "%.4f") || changed;
			changed = slider_double_with_input("Plummer Concentration", &sky.cluster_concentration, 0.5, 3.5, "%.2f") || changed;
			changed = slider_double_with_input("Core Star Density Boost", &sky.cluster_star_density_boost, 0.0, 5.0, "%.2f") || changed;
			changed = ImGui::ColorEdit3("Core Tint Color", sky.cluster_core_tint.data()) || changed;
		}
	}

	if (ImGui::CollapsingHeader("Interstellar Dust Filaments")) {
		changed = ImGui::Checkbox("Enable Dust Filaments", &sky.dust_enabled) || changed;
		if (sky.dust_enabled) {
			changed = slider_double_with_input("Dust Density", &sky.dust_density, 0.0, 5.0, "%.2f") || changed;
			changed = slider_double_with_input("Dust Intensity", &sky.dust_intensity, 0.0, 5.0, "%.2f") || changed;
			changed = slider_double_with_input("Spatial Scale", &sky.dust_scale, 0.2, 8.0, "%.2f") || changed;
			changed = slider_double_with_input("Absorption Ratio", &sky.dust_absorption_ratio, 0.0, 1.0, "%.2f") || changed;
			changed = slider_double_with_input("Contrast", &sky.dust_contrast, 0.5, 4.0, "%.2f") || changed;
			changed = ImGui::ColorEdit3("Dust Tint", sky.dust_tint.data()) || changed;
			int octaves = static_cast<int>(sky.dust_fractal_octaves);
			if (ImGui::SliderInt("Fractal Octaves##Dust", &octaves, 1, 6)) {
				sky.dust_fractal_octaves = static_cast<uint32_t>(octaves);
				changed = true;
			}
		}
	}

	if (ImGui::CollapsingHeader("Exotic Sources (Quasars & Pulsars)")) {
		changed = ImGui::Checkbox("Enable Exotic Objects", &sky.exotic_objects_enabled) || changed;
		if (sky.exotic_objects_enabled) {
			changed = slider_double_with_input("Quasar Density", &sky.quasar_density, 0.0, 0.5, "%.3f") || changed;
			changed = slider_double_with_input("Quasar Brightness", &sky.quasar_brightness, 0.0, 15.0, "%.2f") || changed;
			changed = slider_double_with_input("Quasar Core Size", &sky.quasar_core_size, 0.0001, 0.002, "%.5f") || changed;
			changed = slider_double_with_input("Pulsar Pulse Frequency (Hz)", &sky.pulsar_pulse_rate, 0.1, 20.0, "%.2f") || changed;
			changed = slider_double_with_input("Pulsar Pulse Duty Cycle", &sky.pulsar_pulse_duty, 0.05, 0.95, "%.2f") || changed;
		}
	}

	if (ImGui::CollapsingHeader("Celestial Coordinate Grid (Seed-Independent)")) {
		changed = ImGui::Checkbox("Enable Celestial Grid", &sky.grid_enabled) || changed;
		ImGui::TextDisabled("Geometric celestial sphere coordinate grid (RA & Dec), strictly independent of procedural seed.");
		if (sky.grid_enabled) {
			changed = slider_double_with_input("Grid Opacity", &sky.grid_opacity, 0.0, 1.0, "%.2f") || changed;
			changed = slider_double_with_input("Line Width (rad)", &sky.grid_line_width, 0.0005, 0.015, "%.4f") || changed;

			static const double kSpacings[] = {5.0, 10.0, 15.0, 30.0};
			static const char* kSpacingLabels[] = {"5 deg", "10 deg", "15 deg (1h RA)", "30 deg (2h RA)"};
			int cur_spacing_idx = 2;
			for (int s = 0; s < 4; ++s) {
				if (std::abs(sky.grid_spacing_deg - kSpacings[s]) < 1.0) cur_spacing_idx = s;
			}
			if (ImGui::Combo("Angular Spacing", &cur_spacing_idx, kSpacingLabels, 4)) {
				sky.grid_spacing_deg = kSpacings[cur_spacing_idx];
				changed = true;
			}

			changed = ImGui::ColorEdit3("Major Lines (Equator/Meridian)", sky.grid_major_color.data()) || changed;
			changed = ImGui::ColorEdit3("Minor Grid Lines", sky.grid_minor_color.data()) || changed;
			changed = ImGui::Checkbox("Show Polar Cap Rings", &sky.grid_show_polar_caps) || changed;
			changed = ImGui::Checkbox("Show Equator", &sky.grid_show_equator) || changed;
			changed = ImGui::Checkbox("Show Prime Meridians", &sky.grid_show_meridians) || changed;
		}
	}

	if (ImGui::CollapsingHeader("Color Grading & Orientation")) {
		changed = ImGui::ColorEdit3("Deep Space Base Color", sky.background_color.data()) || changed;
		changed = slider_double_with_input("Hue Shift (deg)", &sky.hue_shift_deg, -180.0, 180.0, "%.1f") || changed;
		changed = slider_double_with_input("Saturation", &sky.saturation, 0.0, 3.0, "%.2f") || changed;

		ImGui::Spacing();
		ImGui::TextColored(ImVec4(0.5f, 0.85f, 1.0f, 1.0f), "Celestial Sphere Orientation (Euler Angles)");
		changed = slider_double_with_input("Yaw (deg)", &sky.orientation_yaw_deg, -180.0, 180.0, "%.1f") || changed;
		changed = slider_double_with_input("Pitch (deg)", &sky.orientation_pitch_deg, -90.0, 90.0, "%.1f") || changed;
		changed = slider_double_with_input("Roll (deg)", &sky.orientation_roll_deg, -180.0, 180.0, "%.1f") || changed;
	}

	if (ImGui::Button("Reset All Sky Settings to Defaults", ImVec2(280.0f, 26.0f))) {
		sky = Render::ProceduralSkySettings{};
		params.sky_background_source = 0U;
		params.sky_panorama_id = 0U;
		params.sky_panorama_quality = 1U;
		changed = true;
	}
	render_setting_tooltip("Restores all procedural skybox attributes to physical defaults.");

	if (changed) {
		user_settings.sky = sky;
		user_settings.sky_rendering_enabled = sky.enabled;
		user_settings.sky_seed_locked = sky.seed_locked;
		params.sync_sky_fields();
		orchestrator.notify_state_changed();
	}

	ImGui::PopID();
}

inline void render_skybox_editor_window(bool& open, Orchestrator::SimulationOrchestrator<1024>& orchestrator, IO::UserSettings& user_settings) noexcept {
	if (!open) return;
	ImGui::SetNextWindowSize(ImVec2(540.0f, 700.0f), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Skybox & Environment Editor", &open)) {
		render_skybox_editor(orchestrator, user_settings);
	}
	ImGui::End();
}

}

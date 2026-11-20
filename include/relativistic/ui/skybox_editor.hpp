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
	if (ImGui::CollapsingHeader("Sky Quality & Continuous Multi-Scale LOD", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.6f, 1.0f), "Quality Preset Profiles:");
		if (ImGui::Button("Fast (0.1x)", ImVec2(80.0f, 22.0f))) {
			sky.quality_factor = 0.10;
			sky.star_quality_factor = 0.15;
			sky.nebula_quality_factor = 0.20;
			sky.cluster_quality_factor = 0.15;
			sky.dust_quality_factor = 0.20;
			sky.adaptive_fov_quality = false;
			changed = true;
		}
		render_setting_tooltip("Minimal sampling workload for low-power devices and fast preview rendering.");
		ImGui::SameLine();
		if (ImGui::Button("Low (0.35x)", ImVec2(85.0f, 22.0f))) {
			sky.quality_factor = 0.35;
			sky.star_quality_factor = 0.40;
			sky.nebula_quality_factor = 0.50;
			sky.cluster_quality_factor = 0.40;
			sky.dust_quality_factor = 0.50;
			sky.adaptive_fov_quality = false;
			changed = true;
		}
		render_setting_tooltip("Reduced density and layer counts for steady interactive frame rates.");
		ImGui::SameLine();
		if (ImGui::Button("Balanced (1.0x)", ImVec2(100.0f, 22.0f))) {
			sky.quality_factor = 1.0;
			sky.star_quality_factor = 1.0;
			sky.nebula_quality_factor = 1.0;
			sky.cluster_quality_factor = 1.0;
			sky.dust_quality_factor = 1.0;
			changed = true;
		}
		render_setting_tooltip("Standard balanced configuration matching physical cosmic density.");
		ImGui::SameLine();
		if (ImGui::Button("High (2.0x)", ImVec2(85.0f, 22.0f))) {
			sky.quality_factor = 2.0;
			sky.star_quality_factor = 1.8;
			sky.nebula_quality_factor = 1.8;
			sky.cluster_quality_factor = 1.8;
			sky.dust_quality_factor = 1.8;
			changed = true;
		}
		render_setting_tooltip("Double sampling fidelity and finer spatial partition resolution.");
		ImGui::SameLine();
		if (ImGui::Button("Fractal Infinite", ImVec2(110.0f, 22.0f))) {
			sky.quality_factor = 2.0;
			sky.adaptive_fov_quality = true;
			sky.adaptive_fov_sensitivity = 1.0;
			sky.max_star_octaves = 6;
			changed = true;
		}
		render_setting_tooltip("Engages continuous multi-scale fractal zoom where narrowing the camera FOV smoothly reveals deeper octaves of fainter stars and finer filaments.");

		changed = slider_double_with_input("Master Quality Factor", &sky.quality_factor, 0.01, 10.0, "%.3fx") || changed;
		render_setting_tooltip("Global scalar governing celestial sphere sampling density. Values below 1 reduce computational work; values above 1 increase structural detail.");

		changed = ImGui::Checkbox("Adaptive FOV Quality Scaling (Fractal Zoom)", &sky.adaptive_fov_quality) || changed;
		render_setting_tooltip("When enabled, zooming in via camera FOV automatically recalculates the sky with deeper hierarchical octaves of fainter stars without popping or position shifts.");

		if (sky.adaptive_fov_quality) {
			changed = slider_double_with_input("Reference Full-View FOV (deg)", &sky.adaptive_fov_reference, 10.0, 140.0, "%.1f") || changed;
			render_setting_tooltip("Base field of view at which base stellar magnitude and base nebular layers are displayed.");
			changed = slider_double_with_input("Zoom Octave Sensitivity", &sky.adaptive_fov_sensitivity, 0.2, 3.0, "%.2f") || changed;
			render_setting_tooltip("Rate at which deeper octaves fade into view as FOV decreases.");
			int oct_count = static_cast<int>(sky.max_star_octaves);
			if (ImGui::SliderInt("Maximum Zoom Octaves", &oct_count, 1, 8)) {
				sky.max_star_octaves = static_cast<uint32_t>(oct_count);
				changed = true;
			}
			render_setting_tooltip("Upper limit of hierarchical octave subdivisions calculated at extreme telescope zoom.");
		}

		if (ImGui::TreeNode("Individual Subsystem Quality Scales")) {
			changed = slider_double_with_input("Stars Quality Multiplier", &sky.star_quality_factor, 0.01, 5.0, "%.2fx") || changed;
			changed = slider_double_with_input("Nebulae Quality Multiplier", &sky.nebula_quality_factor, 0.01, 5.0, "%.2fx") || changed;
			changed = slider_double_with_input("Clusters Quality Multiplier", &sky.cluster_quality_factor, 0.01, 5.0, "%.2fx") || changed;
			changed = slider_double_with_input("Dust Quality Multiplier", &sky.dust_quality_factor, 0.01, 5.0, "%.2fx") || changed;
			ImGui::TreePop();
		}
	}

	if (ImGui::CollapsingHeader("Stars & Stellar Population", ImGuiTreeNodeFlags_DefaultOpen)) {
		changed = ImGui::Checkbox("Enable Stars", &sky.stars_enabled) || changed;
		if (sky.stars_enabled) {
			const char* kProfileModels[] = {
				"Gaussian (Smooth Astronomical Bell Curve)",
				"Lorentzian / Cauchy (Broad Wings & Halo)",
				"Moffat (Telescopic Power-Law Profile)",
				"Airy Diffraction Disk (Concentric Bessel Rings)",
				"Hard Disc (Solid Anti-Aliased Circle)",
				"Diffraction Spikes (Telescopic Cross Pattern)"
			};
			int profile_idx = static_cast<int>(std::min(sky.star_profile_model, 5U));
			if (ImGui::Combo("Star Profile Model (PSF)", &profile_idx, kProfileModels, IM_ARRAYSIZE(kProfileModels))) {
				sky.star_profile_model = static_cast<uint32_t>(profile_idx);
				changed = true;
			}
			render_setting_tooltip("Mathematical point spread function governing stellar optical radiance and appearance.");

			if (sky.star_profile_model == static_cast<uint32_t>(Render::StarProfileModel::Moffat)) {
				changed = slider_double_with_input("Moffat Power-Law Index (beta)", &sky.star_profile_param, 1.1, 6.0, "%.2f") || changed;
				render_setting_tooltip("Controls the slope of outer stellar wings. Lower values yield diffuse halos; higher values approach Gaussian falloff.");
			} else if (sky.star_profile_model == static_cast<uint32_t>(Render::StarProfileModel::Lorentzian)) {
				changed = slider_double_with_input("Lorentzian Wing Scale", &sky.star_profile_param, 0.1, 5.0, "%.2f") || changed;
				render_setting_tooltip("Relative intensity and spread of heavy Lorentzian stellar wings.");
			} else if (sky.star_profile_model == static_cast<uint32_t>(Render::StarProfileModel::HardDisc)) {
				changed = slider_double_with_input("Disc Edge Softness", &sky.star_profile_softness, 0.01, 0.5, "%.3f") || changed;
				render_setting_tooltip("Sub-pixel anti-aliasing feathering width around the disc boundary.");
			} else if (sky.star_profile_model == static_cast<uint32_t>(Render::StarProfileModel::DiffractionSpikes)) {
				int spikes_choice = (sky.star_spike_count == 6U) ? 1 : 0;
				const char* kSpikeChoices[] = {"4-Point Spikes (Reflector)", "6-Point Spikes (Hexagonal / JWST)"};
				if (ImGui::Combo("Spike Geometry", &spikes_choice, kSpikeChoices, IM_ARRAYSIZE(kSpikeChoices))) {
					sky.star_spike_count = (spikes_choice == 1) ? 6U : 4U;
					changed = true;
				}
				changed = slider_double_with_input("Spike Length Multiplier", &sky.star_spike_length, 0.5, 12.0, "%.2fx") || changed;
				changed = slider_double_with_input("Spike Intensity", &sky.star_spike_intensity, 0.0, 3.0, "%.2f") || changed;
				changed = slider_double_with_input("Spike Orientation (deg)", &sky.star_spike_angle_deg, 0.0, 180.0, "%.1f") || changed;
			}

			ImGui::Separator();
			const char* kPlacementModels[] = {
				"Uniform Equal-Area (Isotropic Sphere)",
				"Galactic Disk (Plane Concentrated)",
				"Fractal Clustered (Stellar Associations)",
				"Dual Population (Disk + Isotropic Halo)"
			};
			int placement_idx = static_cast<int>(std::min(sky.star_placement_model, 3U));
			if (ImGui::Combo("Star Placement Model", &placement_idx, kPlacementModels, IM_ARRAYSIZE(kPlacementModels))) {
				sky.star_placement_model = static_cast<uint32_t>(placement_idx);
				changed = true;
			}
			render_setting_tooltip("Spatial density distribution of stars across the celestial sphere.");

			if (sky.star_placement_model == static_cast<uint32_t>(Render::StarPlacementModel::GalacticDisk) ||
				sky.star_placement_model == static_cast<uint32_t>(Render::StarPlacementModel::DualPopulation)) {
				changed = slider_double_with_input("Galactic Plane Concentration", &sky.star_galactic_concentration, 0.5, 10.0, "%.2f") || changed;
				render_setting_tooltip("Degree of vertical confinement toward the defined galactic plane.");
			}

			ImGui::Separator();
			changed = slider_double_with_input("Star Density", &sky.star_density, 0.01, 8.0, "%.3f") || changed;
			changed = slider_double_with_input("Star Brightness", &sky.star_brightness, 0.0, 15.0, "%.2f") || changed;

			static bool star_min_size_log = true;
			changed = slider_double_with_input("Minimum Angular Size (rad)", &sky.star_size_min, 0.000001, 0.005, "%.6f", &star_min_size_log, 0.000001, 0.005) || changed;
			render_setting_tooltip("Smallest angular radius for distant point-like stars, down to 1e-6 radians for sub-pixel pinpricks.");

			changed = slider_double_with_input("Maximum Angular Size (rad)", &sky.star_size_max, 0.0001, 0.025, "%.5f") || changed;

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
			changed = slider_double_with_input("Magnitude Limit", &sky.star_magnitude_limit, 4.0, 26.0, "%.1f") || changed;
			changed = slider_double_with_input("Scintillation / Twinkle", &sky.star_scintillation, 0.0, 1.0, "%.2f") || changed;
		}
	}

	if (ImGui::CollapsingHeader("Volumetric Nebulae & Galactic Plane", ImGuiTreeNodeFlags_DefaultOpen)) {
		changed = ImGui::Checkbox("Enable Nebulae", &sky.nebula_enabled) || changed;
		if (sky.nebula_enabled) {
			changed = slider_double_with_input("Nebula Glow Intensity", &sky.nebula_intensity, 0.0, 8.0, "%.2f") || changed;
			changed = slider_double_with_input("Gas Color Separation", &sky.nebula_color_separation, 0.5, 3.0, "%.2f") || changed;
			render_setting_tooltip("Spatial differentiation between H-alpha, O-III, and reflection gas filaments.");
			changed = slider_double_with_input("Gas Filament Contrast", &sky.nebula_gas_contrast, 0.5, 3.0, "%.2f") || changed;
			render_setting_tooltip("Enhances contrast between glowing emission ridges and darker background regions.");
			changed = slider_double_with_input("Ionization Core Glow", &sky.nebula_ionization_glow, 0.0, 3.0, "%.2f") || changed;

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

			changed = ImGui::ColorEdit3("H-Alpha Emission (Crimson)", sky.nebula_emission_color.data()) || changed;
			changed = ImGui::ColorEdit3("O-III Ionized Gas (Cyan/Teal)", sky.nebula_secondary_color.data()) || changed;
			changed = ImGui::ColorEdit3("S-II / Reflection Gas (Amber/Gold)", sky.nebula_tertiary_color.data()) || changed;
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

	if (ImGui::CollapsingHeader("Star Clusters & Member Stars", ImGuiTreeNodeFlags_DefaultOpen)) {
		changed = ImGui::Checkbox("Enable Star Clusters", &sky.clusters_enabled) || changed;
		if (sky.clusters_enabled) {
			const char* kClusterPlacement[] = {
				"Globular Cluster Halo (Galactocentric R^-3.5)",
				"Open Clusters (Galactic Disk Plane)",
				"Uniform Isotropic (Full Sky Poisson)",
				"Supercluster Complex (Filamentary Nodes)"
			};
			int cluster_place_idx = static_cast<int>(std::min(sky.cluster_placement_model, 3U));
			if (ImGui::Combo("Cluster Placement Geometry", &cluster_place_idx, kClusterPlacement, IM_ARRAYSIZE(kClusterPlacement))) {
				sky.cluster_placement_model = static_cast<uint32_t>(cluster_place_idx);
				changed = true;
			}

			if (sky.cluster_placement_model == static_cast<uint32_t>(Render::ClusterPlacementModel::GlobularHalo)) {
				changed = slider_double_with_input("Halo Scale Radius (rad)", &sky.cluster_halo_radius, 0.1, 3.0, "%.2f") || changed;
			} else if (sky.cluster_placement_model == static_cast<uint32_t>(Render::ClusterPlacementModel::OpenClusterPlane)) {
				changed = slider_double_with_input("Plane Scale Height (rad)", &sky.cluster_plane_scale_height, 0.02, 0.8, "%.3f") || changed;
			}

			changed = slider_double_with_input("Cluster Density", &sky.cluster_density, 0.0, 4.0, "%.3f") || changed;
			changed = slider_double_with_input("Cluster Core Brightness", &sky.cluster_brightness, 0.0, 8.0, "%.2f") || changed;
			changed = slider_double_with_input("Cluster Size Scale", &sky.cluster_size_scale, 0.05, 4.0, "%.2f") || changed;
			changed = slider_double_with_input("Cluster Size Min (rad)", &sky.cluster_size_min, 0.001, 0.03, "%.4f") || changed;
			changed = slider_double_with_input("Cluster Size Max (rad)", &sky.cluster_size_max, 0.005, 0.1, "%.4f") || changed;
			changed = slider_double_with_input("Plummer Core Concentration", &sky.cluster_concentration, 0.5, 4.5, "%.2f") || changed;
			changed = ImGui::ColorEdit3("Cluster Core Tint", sky.cluster_core_tint.data()) || changed;

			ImGui::Separator();
			changed = ImGui::Checkbox("Resolve Member Stars Inside Clusters", &sky.cluster_stars_enabled) || changed;
			render_setting_tooltip("Synthesizes individual member stars orbiting each cluster center according to a Plummer potential.");
			if (sky.cluster_stars_enabled) {
				int members = static_cast<int>(sky.cluster_member_count);
				if (ImGui::SliderInt("Cluster Member Stars Count", &members, 4, 256)) {
					sky.cluster_member_count = static_cast<uint32_t>(members);
					changed = true;
				}
				changed = slider_double_with_input("Member Star Brightness", &sky.cluster_member_brightness, 0.1, 5.0, "%.2f") || changed;
				changed = slider_double_with_input("Member Star Size (rad)", &sky.cluster_member_size, 0.0001, 0.003, "%.5f") || changed;

				const char* kMemberProfiles[] = {"Gaussian", "Lorentzian", "Moffat", "Airy Diffraction", "Hard Disc", "Diffraction Spikes"};
				int member_prof_idx = static_cast<int>(std::min(sky.cluster_star_profile_model, 5U));
				if (ImGui::Combo("Member Star Profile", &member_prof_idx, kMemberProfiles, IM_ARRAYSIZE(kMemberProfiles))) {
					sky.cluster_star_profile_model = static_cast<uint32_t>(member_prof_idx);
					changed = true;
				}
			}
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

	if (ImGui::CollapsingHeader("Celestial Coordinate Grid (Seed-Independent)", ImGuiTreeNodeFlags_DefaultOpen)) {
		changed = ImGui::Checkbox("Enable Celestial Coordinate Grid", &sky.grid_enabled) || changed;
		ImGui::TextDisabled("Geometric celestial sphere coordinate grid (RA & Dec), strictly independent of procedural seed.");
		if (sky.grid_enabled) {
			changed = slider_double_with_input("Grid Opacity", &sky.grid_opacity, 0.0, 1.0, "%.2f") || changed;
			changed = slider_double_with_input("Line Width (rad)", &sky.grid_line_width, 0.0002, 0.02, "%.5f") || changed;
			render_setting_tooltip("Angular line width on the celestial sphere. Automatically scales with viewport pixel footprint to remain sharp and anti-aliased at any resolution.");

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
			ImGui::SameLine();
			changed = ImGui::Checkbox("Show Equator", &sky.grid_show_equator) || changed;
			ImGui::SameLine();
			changed = ImGui::Checkbox("Show Prime Meridians", &sky.grid_show_meridians) || changed;
			changed = ImGui::Checkbox("Show Sub-Degree Ticks", &sky.grid_show_ticks) || changed;
			if (sky.grid_show_ticks) {
				changed = slider_double_with_input("Tick Interval (deg)", &sky.grid_tick_interval_deg, 0.5, 10.0, "%.1f") || changed;
				changed = slider_double_with_input("Tick Length (rad)", &sky.grid_tick_length, 0.0005, 0.015, "%.4f") || changed;
			}
		}
	}

	if (ImGui::CollapsingHeader("Color Grading, Orientation & Rotation")) {
		changed = ImGui::ColorEdit3("Deep Space Base Color", sky.background_color.data()) || changed;
		changed = slider_double_with_input("Hue Shift (deg)", &sky.hue_shift_deg, -180.0, 180.0, "%.1f") || changed;
		changed = slider_double_with_input("Saturation", &sky.saturation, 0.0, 3.0, "%.2f") || changed;

		ImGui::Spacing();
		ImGui::TextColored(ImVec4(0.5f, 0.85f, 1.0f, 1.0f), "Celestial Sphere Orientation (Euler Angles)");
		changed = slider_double_with_input("Yaw (deg)", &sky.orientation_yaw_deg, -180.0, 180.0, "%.1f") || changed;
		changed = slider_double_with_input("Pitch (deg)", &sky.orientation_pitch_deg, -90.0, 90.0, "%.1f") || changed;
		changed = slider_double_with_input("Roll (deg)", &sky.orientation_roll_deg, -180.0, 180.0, "%.1f") || changed;

		ImGui::Spacing();
		changed = ImGui::Checkbox("Continuous Celestial Sphere Rotation", &sky.dynamic_rotation) || changed;
		render_setting_tooltip("Slow deterministic axial rotation of the entire celestial sphere coordinated with the simulation clock.");
		if (sky.dynamic_rotation) {
			changed = slider_double_with_input("Rotation Speed (deg/s)", &sky.rotation_rate_deg_s, -1.0, 1.0, "%.3f") || changed;
		}
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

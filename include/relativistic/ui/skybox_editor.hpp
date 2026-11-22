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
			changed = slider_double_with_input("Quasars Quality Multiplier", &sky.quasar_quality_factor, 0.01, 5.0, "%.2fx") || changed;
			changed = slider_double_with_input("Pulsars Quality Multiplier", &sky.pulsar_quality_factor, 0.01, 5.0, "%.2fx") || changed;
			changed = slider_double_with_input("Supernova Remnants Quality Multiplier", &sky.snr_quality_factor, 0.01, 5.0, "%.2fx") || changed;
			ImGui::TreePop();
		}
	}

	if (ImGui::CollapsingHeader("Stars & Stellar Population", ImGuiTreeNodeFlags_DefaultOpen)) {
		changed = ImGui::Checkbox("Enable Stars", &sky.stars_enabled) || changed;
		if (sky.stars_enabled) {
			const char* kProfileModels[] = {
				"Gaussian (Astronomical Seeing Disk)",
				"Lorentzian / Cauchy (Broad Wings & Halos)",
				"Moffat (Telescopic Power-Law Profile)",
				"Airy Diffraction Disk (Concentric Bessel Rings)",
				"Hard Disc (Solid Anti-Aliased Circle)",
				"Diffraction Spikes (Telescopic Vane Pattern)",
				"Pinpoint Dirac (Pure Sub-Pixel Point Stars)",
				"Cinematic Bloom (Core + Glare Radiance)"
			};
			int profile_idx = static_cast<int>(std::min(sky.star_profile_model, 7U));
			if (ImGui::Combo("Star Profile Model (PSF)", &profile_idx, kProfileModels, IM_ARRAYSIZE(kProfileModels))) {
				sky.star_profile_model = static_cast<uint32_t>(profile_idx);
				changed = true;
			}
			render_setting_tooltip("Mathematical point spread function governing stellar optical radiance and profile geometry.");

			if (sky.star_profile_model == static_cast<uint32_t>(Render::StarProfileModel::PinpointDirac)) {
				changed = slider_double_with_input("Sub-Pixel Concentration", &sky.star_dirac_concentration, 0.5, 0.999, "%.3f") || changed;
				render_setting_tooltip("Concentrates star radiance directly into the hit direction with minimal geometric spread.");
			} else if (sky.star_profile_model == static_cast<uint32_t>(Render::StarProfileModel::CinematicBloom)) {
				changed = slider_double_with_input("Bloom Radius Multiplier", &sky.star_bloom_radius, 1.0, 10.0, "%.2fx") || changed;
				changed = slider_double_with_input("Bloom Glare Intensity", &sky.star_bloom_intensity, 0.0, 2.0, "%.2f") || changed;
			} else if (sky.star_profile_model == static_cast<uint32_t>(Render::StarProfileModel::Moffat)) {
				changed = slider_double_with_input("Moffat Power-Law Index (beta)", &sky.star_profile_param, 1.1, 6.0, "%.2f") || changed;
				render_setting_tooltip("Controls the slope of outer stellar wings. Lower values yield diffuse halos; higher values approach Gaussian falloff.");
			} else if (sky.star_profile_model == static_cast<uint32_t>(Render::StarProfileModel::Lorentzian)) {
				changed = slider_double_with_input("Lorentzian Wing Scale", &sky.star_profile_param, 0.1, 5.0, "%.2f") || changed;
				render_setting_tooltip("Relative intensity and spread of heavy Lorentzian stellar wings.");
			} else if (sky.star_profile_model == static_cast<uint32_t>(Render::StarProfileModel::HardDisc)) {
				changed = slider_double_with_input("Disc Edge Softness", &sky.star_profile_softness, 0.005, 0.5, "%.3f") || changed;
				render_setting_tooltip("Sub-pixel anti-aliasing feathering width around the disc boundary.");
			} else if (sky.star_profile_model == static_cast<uint32_t>(Render::StarProfileModel::DiffractionSpikes)) {
				int spikes_choice = (sky.star_spike_count == 6U) ? 1 : ((sky.star_spike_count == 8U) ? 2 : 0);
				const char* kSpikeChoices[] = {"4-Point Spikes (Newtonian)", "6-Point Spikes (Hexagonal / JWST)", "8-Point Spikes (Complex Spider)"};
				if (ImGui::Combo("Spike Geometry", &spikes_choice, kSpikeChoices, IM_ARRAYSIZE(kSpikeChoices))) {
					sky.star_spike_count = (spikes_choice == 1) ? 6U : ((spikes_choice == 2) ? 8U : 4U);
					changed = true;
				}
				changed = slider_double_with_input("Spike Length Multiplier", &sky.star_spike_length, 0.5, 16.0, "%.2fx") || changed;
				changed = slider_double_with_input("Spike Intensity", &sky.star_spike_intensity, 0.0, 4.0, "%.2f") || changed;
				changed = slider_double_with_input("Spike Orientation (deg)", &sky.star_spike_angle_deg, 0.0, 180.0, "%.1f") || changed;
			}

			ImGui::Separator();
			const char* kPlacementModels[] = {
				"Uniform Equal-Area (Isotropic Poisson on S2)",
				"Galactic Disk (Plane Concentrated)",
				"Fractal Clustered (Stellar Associations)",
				"Dual Population (Disk + Isotropic Halo)",
				"Spiral Arm Density (Logarithmic Arms)",
				"Constellation Associations (Loose Groups)"
			};
			int placement_idx = static_cast<int>(std::min(sky.star_placement_model, 5U));
			if (ImGui::Combo("Star Placement Model", &placement_idx, kPlacementModels, IM_ARRAYSIZE(kPlacementModels))) {
				sky.star_placement_model = static_cast<uint32_t>(placement_idx);
				changed = true;
			}
			render_setting_tooltip("Spatial density distribution of stars across the celestial sphere.");

			if (sky.star_placement_model == static_cast<uint32_t>(Render::StarPlacementModel::GalacticDisk) ||
				sky.star_placement_model == static_cast<uint32_t>(Render::StarPlacementModel::DualPopulation) ||
				sky.star_placement_model == static_cast<uint32_t>(Render::StarPlacementModel::SpiralArmDensity)) {
				changed = slider_double_with_input("Galactic Plane Concentration", &sky.star_galactic_concentration, 0.5, 12.0, "%.2f") || changed;
				render_setting_tooltip("Degree of vertical confinement toward the defined galactic plane.");
			}

			ImGui::Separator();
			changed = slider_double_with_input("Star Density", &sky.star_density, 0.01, 100.0, "%.3f") || changed;
			changed = slider_double_with_input("Star Brightness", &sky.star_brightness, 0.0, 50.0, "%.2f") || changed;
			changed = slider_double_with_input("Star Size Scale", &sky.star_size_scale, 0.05, 50.0, "%.3fx") || changed;
			render_setting_tooltip("Uniform multiplier on all stellar angular radii, allowing tiny needle pinpoints up to giant luminous discs.");

			static bool star_min_size_log = true;
			changed = slider_double_with_input("Minimum Angular Size (rad)", &sky.star_size_min, 0.0, 0.05, "%.7f", &star_min_size_log, 0.0, 0.05) || changed;
			render_setting_tooltip("Smallest angular radius for distant stars, allowing true zero for pure sub-pixel point sources.");

			static bool star_max_size_log = true;
			changed = slider_double_with_input("Maximum Angular Size (rad)", &sky.star_size_max, 0.00001, 0.15, "%.5f", &star_max_size_log, 0.00001, 0.15) || changed;

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
			changed = slider_double_with_input("Cluster Halo Intensity", &sky.cluster_halo_intensity, 0.0, 2.0, "%.2f") || changed;
			render_setting_tooltip("Controls the brightness of the diffuse ambient halo surrounding each star cluster. Set lower to avoid over-concentrated bright white cores.");
			changed = slider_double_with_input("Cluster Spread Factor", &sky.cluster_spread_factor, 0.1, 5.0, "%.2f") || changed;
			render_setting_tooltip("Controls spatial member dispersion scale across the cluster volume.");
			changed = slider_double_with_input("Cluster Size Scale", &sky.cluster_size_scale, 0.05, 4.0, "%.2f") || changed;
			changed = slider_double_with_input("Cluster Size Min (rad)", &sky.cluster_size_min, 0.001, 0.03, "%.4f") || changed;
			changed = slider_double_with_input("Cluster Size Max (rad)", &sky.cluster_size_max, 0.005, 0.1, "%.4f") || changed;
			changed = slider_double_with_input("Plummer Core Concentration", &sky.cluster_concentration, 0.5, 4.5, "%.2f") || changed;
			changed = ImGui::ColorEdit3("Cluster Core Tint", sky.cluster_core_tint.data()) || changed;

			ImGui::Separator();
			changed = ImGui::Checkbox("Resolve Member Stars Inside Clusters", &sky.cluster_stars_enabled) || changed;
			render_setting_tooltip("Synthesizes individual member stars orbiting each cluster center according to a Plummer potential.");
			if (sky.cluster_stars_enabled) {
				changed = ImGui::Checkbox("Inherit Global Star Profile & PSF Style", &sky.cluster_inherit_star_profile) || changed;
				render_setting_tooltip("When enabled, member stars follow the globally selected star point-spread function, spike count, and diffraction style.");
				int members = static_cast<int>(sky.cluster_member_count);
				if (ImGui::SliderInt("Cluster Member Stars Count", &members, 4, 256)) {
					sky.cluster_member_count = static_cast<uint32_t>(members);
					changed = true;
				}
				changed = slider_double_with_input("Member Star Brightness", &sky.cluster_member_brightness, 0.1, 5.0, "%.2f") || changed;
				changed = slider_double_with_input("Member Star Size (rad)", &sky.cluster_member_size, 0.0001, 0.003, "%.5f") || changed;

				if (!sky.cluster_inherit_star_profile) {
					const char* kMemberProfiles[] = {"Gaussian", "Lorentzian", "Moffat", "Airy Diffraction", "Hard Disc", "Diffraction Spikes"};
					int member_prof_idx = static_cast<int>(std::min(sky.cluster_star_profile_model, 5U));
					if (ImGui::Combo("Member Star Profile", &member_prof_idx, kMemberProfiles, IM_ARRAYSIZE(kMemberProfiles))) {
						sky.cluster_star_profile_model = static_cast<uint32_t>(member_prof_idx);
						changed = true;
					}
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
			const char* kMorphologies[] = {
				"Mixed Catalog (Weighted Custom Mix)",
				"Grand Design Spiral (Prominent 2-Arm)",
				"Flocculent Spiral (Fragmented Arms)",
				"Barred Spiral (Central Bar Structure)",
				"Giant Elliptical (Sersic Profile)",
				"Lenticular S0 (Smooth Disc + Bulge)",
				"Ring Galaxy (Hoag-Type Detached Ring)"
			};
			int morph_idx = static_cast<int>(std::min(sky.galaxy_morphology_model, 6U));
			if (ImGui::Combo("Morphological Type", &morph_idx, kMorphologies, IM_ARRAYSIZE(kMorphologies))) {
				sky.galaxy_morphology_model = static_cast<uint32_t>(morph_idx);
				changed = true;
			}
			render_setting_tooltip("Governs galaxy structure and light distribution models across the celestial sphere.");

			if (sky.galaxy_morphology_model == 0U) {
				ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Mixed Catalog Morphology Presence Weights:");
				changed = slider_double_with_input("Grand Design Spiral Weight", &sky.galaxy_weight_grand_design, 0.0, 5.0, "%.2f") || changed;
				changed = slider_double_with_input("Flocculent Spiral Weight", &sky.galaxy_weight_flocculent, 0.0, 5.0, "%.2f") || changed;
				changed = slider_double_with_input("Barred Spiral Weight", &sky.galaxy_weight_barred, 0.0, 5.0, "%.2f") || changed;
				changed = slider_double_with_input("Giant Elliptical Weight", &sky.galaxy_weight_elliptical, 0.0, 5.0, "%.2f") || changed;
				changed = slider_double_with_input("Lenticular S0 Weight", &sky.galaxy_weight_lenticular, 0.0, 5.0, "%.2f") || changed;
				changed = slider_double_with_input("Ring Galaxy Weight", &sky.galaxy_weight_ring, 0.0, 5.0, "%.2f") || changed;
			}

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

			ImGui::Separator();
			ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Morphological & Structural Tuning:");
			changed = slider_double_with_input("Spiral Arms Count", &sky.galaxy_spiral_arms, 1.0, 8.0, "%.1f") || changed;
			changed = slider_double_with_input("Arm Pitch Angle", &sky.galaxy_pitch_angle, 0.2, 3.0, "%.2f") || changed;
			changed = slider_double_with_input("Core to Disk Ratio", &sky.galaxy_core_ratio, 0.0, 1.0, "%.2f") || changed;
			changed = slider_double_with_input("Central Bar Strength", &sky.galaxy_bar_strength, 0.0, 2.0, "%.2f") || changed;
			changed = slider_double_with_input("Near-Side Dust Lane Opacity", &sky.galaxy_dust_lane_opacity, 0.0, 1.0, "%.2f") || changed;
			render_setting_tooltip("Simulates realistic asymmetric dust lane absorption on the near side of inclined disk galaxies.");
			changed = slider_double_with_input("Starburst Clumpiness", &sky.galaxy_clumpiness, 0.0, 2.0, "%.2f") || changed;
			changed = slider_double_with_input("Disk Thickness Scale Height", &sky.galaxy_disk_thickness, 0.05, 0.8, "%.2f") || changed;
			changed = slider_double_with_input("3D Inclination Variance", &sky.galaxy_inclination_variance, 0.0, 1.0, "%.2f") || changed;
			changed = slider_double_with_input("Bulge Sersic Index (n)", &sky.galaxy_bulge_sersic_index, 0.5, 8.0, "%.2f") || changed;
			changed = slider_double_with_input("Ellipticity Variance", &sky.galaxy_ellipticity_variance, 0.0, 2.0, "%.2f") || changed;
		}
	}

	if (ImGui::CollapsingHeader("Active Galactic Nuclei & Quasars")) {
		changed = ImGui::Checkbox("Enable Quasars", &sky.quasar_enabled) || changed;
		if (sky.quasar_enabled) {
			changed = slider_double_with_input("Quasar Density", &sky.quasar_density, 0.001, 5.0, "%.3f") || changed;
			changed = slider_double_with_input("Quasar Brightness", &sky.quasar_brightness, 0.01, 10.0, "%.2f") || changed;
			static bool quasar_size_log = true;
			changed = slider_double_with_input("Quasar Core Angular Radius (rad)", &sky.quasar_core_size, 0.00005, 0.01, "%.5f", &quasar_size_log, 0.00005, 0.01) || changed;
			changed = slider_double_with_input("Relativistic Jet Length (rad)", &sky.quasar_jet_length, 0.002, 0.20, "%.4f") || changed;
			changed = slider_double_with_input("Relativistic Jet Intensity", &sky.quasar_jet_intensity, 0.0, 5.0, "%.2f") || changed;
			changed = slider_double_with_input("Jet Opening Angle (deg)", &sky.quasar_jet_opening_angle_deg, 1.0, 25.0, "%.1f") || changed;
			changed = slider_double_with_input("Jet Lorentz Factor (gamma)", &sky.quasar_jet_lorentz_factor, 1.05, 20.0, "%.2f") || changed;
			changed = slider_double_with_input("Accretion Disk Glow", &sky.quasar_accretion_disk_glow, 0.0, 3.0, "%.2f") || changed;
			changed = slider_double_with_input("Jet Precession Angle (deg)", &sky.quasar_precession_angle_deg, 0.0, 45.0, "%.1f") || changed;
		}
	}

	if (ImGui::CollapsingHeader("Pulsars & Magnetars")) {
		changed = ImGui::Checkbox("Enable Pulsars", &sky.pulsar_enabled) || changed;
		if (sky.pulsar_enabled) {
			changed = slider_double_with_input("Pulsar Density", &sky.pulsar_density, 0.001, 5.0, "%.3f") || changed;
			changed = slider_double_with_input("Pulsar Brightness", &sky.pulsar_brightness, 0.01, 10.0, "%.2f") || changed;
			changed = slider_double_with_input("Pulse Frequency (Hz)", &sky.pulsar_pulse_rate, 0.1, 50.0, "%.2f") || changed;
			changed = slider_double_with_input("Frequency Desynchronization Spread", &sky.pulsar_frequency_spread, 0.0, 2.0, "%.2f") || changed;
			render_setting_tooltip("Introduces individualized frequency scatter across pulsars so each rotates at its own independent rate.");
			changed = slider_double_with_input("Pulse Duty Cycle", &sky.pulsar_pulse_duty, 0.01, 0.5, "%.2f") || changed;
			changed = slider_double_with_input("Beaming Cone Half-Angle (deg)", &sky.pulsar_beam_width_deg, 1.0, 30.0, "%.1f") || changed;
			changed = slider_double_with_input("Magnetic Axis Tilt (deg)", &sky.pulsar_magnetic_tilt_deg, 0.0, 90.0, "%.1f") || changed;
			changed = slider_double_with_input("Wind Nebula Scale", &sky.pulsar_wind_nebula_scale, 0.0005, 0.02, "%.4f") || changed;
			changed = slider_double_with_input("Wind Nebula Intensity", &sky.pulsar_wind_nebula_intensity, 0.0, 2.0, "%.2f") || changed;
		}
	}

	if (ImGui::CollapsingHeader("Supernova Remnants (SNR)")) {
		changed = ImGui::Checkbox("Enable Supernova Remnant Shells", &sky.snr_enabled) || changed;
		if (sky.snr_enabled) {
			changed = slider_double_with_input("SNR Density", &sky.snr_density, 0.005, 5.0, "%.3f") || changed;
			changed = slider_double_with_input("SNR Shell Brightness", &sky.snr_brightness, 0.01, 10.0, "%.2f") || changed;
			changed = slider_double_with_input("Shell Angular Radius (rad)", &sky.snr_size, 0.002, 0.08, "%.4f") || changed;
			changed = slider_double_with_input("Shell Thickness Fraction", &sky.snr_shell_thickness, 0.01, 0.3, "%.3f") || changed;
			changed = slider_double_with_input("Expansion Age Phase", &sky.snr_expansion_age, 0.0, 2.0, "%.2f") || changed;
			changed = slider_double_with_input("Shell Asymmetry", &sky.snr_asymmetry, 0.0, 1.0, "%.2f") || changed;
			changed = slider_double_with_input("Morphology Variety", &sky.snr_morphology_variety, 0.0, 2.0, "%.2f") || changed;
			render_setting_tooltip("Varies axial elongation and structural shape across individual remnants.");
			changed = slider_double_with_input("Color & Spectral Variation", &sky.snr_color_variation, 0.0, 2.0, "%.2f") || changed;
			render_setting_tooltip("Varies spectral composition across remnants between oxygen-rich, hydrogen-rich, and synchrotron emission.");
			changed = ImGui::ColorEdit3("Shell Base Color", sky.snr_shell_color.data()) || changed;
			changed = ImGui::ColorEdit3("Interior Synchrotron Color", sky.snr_interior_color.data()) || changed;
			changed = slider_double_with_input("Filament Turbulence Scale", &sky.snr_filament_scale, 0.5, 8.0, "%.2f") || changed;
			changed = slider_double_with_input("Interior Synchrotron Glow", &sky.snr_interior_glow, 0.0, 2.0, "%.2f") || changed;
		}
	}

	if (ImGui::CollapsingHeader("Celestial Coordinate Grid (Seed-Independent)", ImGuiTreeNodeFlags_DefaultOpen)) {
		changed = ImGui::Checkbox("Enable Celestial Coordinate Grid", &sky.grid_enabled) || changed;
		ImGui::TextDisabled("Geometric celestial sphere coordinate grid (RA & Dec) with independent 3D orientation.");

		if (sky.grid_enabled) {
			if (ImGui::TreeNode("Independent 3D Grid Orientation")) {
				changed = slider_double_with_input("Grid Yaw (deg)", &sky.grid_orientation_yaw_deg, -180.0, 180.0, "%.1f") || changed;
				changed = slider_double_with_input("Grid Pitch (deg)", &sky.grid_orientation_pitch_deg, -90.0, 90.0, "%.1f") || changed;
				changed = slider_double_with_input("Grid Roll (deg)", &sky.grid_orientation_roll_deg, -180.0, 180.0, "%.1f") || changed;
				if (ImGui::Button("Reset Grid Orientation")) {
					sky.grid_orientation_yaw_deg = 0.0;
					sky.grid_orientation_pitch_deg = 0.0;
					sky.grid_orientation_roll_deg = 0.0;
					changed = true;
				}
				ImGui::TreePop();
			}

			if (ImGui::TreeNode("Grid Dimensions & Line Style")) {
				changed = slider_double_with_input("Grid Spacing (deg)", &sky.grid_spacing_deg, 1.0, 90.0, "%.1f") || changed;
				changed = slider_double_with_input("Grid Lines Opacity", &sky.grid_opacity, 0.0, 1.0, "%.2f") || changed;
				changed = slider_double_with_input("Grid Glow Intensity", &sky.grid_glow, 0.0, 5.0, "%.2f") || changed;

				static bool grid_width_log = true;
				changed = slider_double_with_input("Major Line Width (rad)", &sky.grid_major_width, 0.00001, 0.01, "%.5f", &grid_width_log, 0.00001, 0.01) || changed;
				changed = ImGui::ColorEdit3("Major Lines Color", sky.grid_major_color.data()) || changed;

				int subdivisions = static_cast<int>(sky.grid_subdivisions);
				if (ImGui::SliderInt("Minor Subdivisions", &subdivisions, 0, 10)) {
					sky.grid_subdivisions = static_cast<uint32_t>(subdivisions);
					changed = true;
				}
				if (sky.grid_subdivisions > 0U) {
					changed = slider_double_with_input("Minor Line Width (rad)", &sky.grid_minor_width, 0.00001, 0.01, "%.5f", &grid_width_log, 0.00001, 0.01) || changed;
					changed = ImGui::ColorEdit3("Minor Lines Color", sky.grid_minor_color.data()) || changed;
				}
				ImGui::TreePop();
			}

			if (ImGui::TreeNode("Cell Fill & Dynamic Color Palette")) {
				const char* kFillModes[] = {
					"None (Transparent Cells)",
					"Checkerboard (Alternating 2 Colors)",
					"Cyclic Palette Across Grid",
					"Checkerboard Pattern Across Palette",
					"Random Hash Fill"
				};
				int fill_idx = static_cast<int>(std::min(sky.grid_cell_fill_mode, 4U));
				if (ImGui::Combo("Fill Mode", &fill_idx, kFillModes, IM_ARRAYSIZE(kFillModes))) {
					sky.grid_cell_fill_mode = static_cast<uint32_t>(fill_idx);
					changed = true;
				}

				if (sky.grid_cell_fill_mode != 0U) {
					changed = slider_double_with_input("Cell Fill Opacity", &sky.grid_cell_opacity, 0.0, 1.0, "%.2f") || changed;

					int active_color_count = static_cast<int>(std::clamp(sky.grid_cell_color_count, 1U, static_cast<uint32_t>(Render::ProceduralSkySettings::kMaxGridCellColors)));
					if (ImGui::SliderInt("Number of Fill Colors", &active_color_count, 1, static_cast<int>(Render::ProceduralSkySettings::kMaxGridCellColors))) {
						sky.grid_cell_color_count = static_cast<uint32_t>(active_color_count);
						changed = true;
					}

					ImGui::SameLine();
					if (ImGui::Button("Add Color") && sky.grid_cell_color_count < Render::ProceduralSkySettings::kMaxGridCellColors) {
						++sky.grid_cell_color_count;
						changed = true;
					}
					ImGui::SameLine();
					if (ImGui::Button("Remove Color") && sky.grid_cell_color_count > 1U) {
						--sky.grid_cell_color_count;
						changed = true;
					}

					for (uint32_t c = 0; c < sky.grid_cell_color_count; ++c) {
						std::string color_label = "Color #" + std::to_string(c + 1);
						changed = ImGui::ColorEdit3(color_label.c_str(), sky.grid_cell_colors[c].data()) || changed;
					}
				}
				ImGui::TreePop();
			}

			if (ImGui::TreeNode("Equator, Poles & Meridians")) {
				changed = ImGui::Checkbox("Show Equator", &sky.grid_show_equator) || changed;
				if (sky.grid_show_equator) {
					changed = slider_double_with_input("Equator Width (rad)", &sky.grid_equator_width, 0.00001, 0.02, "%.5f") || changed;
					changed = slider_double_with_input("Equator Opacity", &sky.grid_equator_opacity, 0.0, 1.0, "%.2f") || changed;
					changed = ImGui::ColorEdit3("Equator Color", sky.grid_equator_color.data()) || changed;
				}
				changed = ImGui::Checkbox("Show Polar Cap Rings", &sky.grid_show_polar_caps) || changed;
				if (sky.grid_show_polar_caps) {
					changed = slider_double_with_input("Polar Width (rad)", &sky.grid_polar_width, 0.00001, 0.02, "%.5f") || changed;
					changed = slider_double_with_input("Polar Opacity", &sky.grid_polar_opacity, 0.0, 1.0, "%.2f") || changed;
					changed = ImGui::ColorEdit3("Polar Color", sky.grid_polar_color.data()) || changed;
				}
				changed = ImGui::Checkbox("Show Prime Meridians", &sky.grid_show_meridians) || changed;
				if (sky.grid_show_meridians) {
					changed = slider_double_with_input("Meridian Width (rad)", &sky.grid_meridian_width, 0.00001, 0.02, "%.5f") || changed;
					changed = slider_double_with_input("Meridian Opacity", &sky.grid_meridian_opacity, 0.0, 1.0, "%.2f") || changed;
					changed = ImGui::ColorEdit3("Meridian Color", sky.grid_meridian_color.data()) || changed;
				}
				ImGui::TreePop();
			}

			if (ImGui::TreeNode("Axis Ticks & Graduations")) {
				changed = ImGui::Checkbox("Show Sub-Degree Ticks", &sky.grid_show_ticks) || changed;
				if (sky.grid_show_ticks) {
					changed = slider_double_with_input("Tick Interval (deg)", &sky.grid_tick_interval_deg, 0.5, 10.0, "%.1f") || changed;
					changed = slider_double_with_input("Tick Length (rad)", &sky.grid_tick_length, 0.0005, 0.03, "%.4f") || changed;
					changed = slider_double_with_input("Tick Width (rad)", &sky.grid_tick_width, 0.00001, 0.005, "%.5f") || changed;
					changed = ImGui::ColorEdit3("Tick Marks Color", sky.grid_tick_color.data()) || changed;
				}
				ImGui::TreePop();
			}
		}
	}

	if (ImGui::CollapsingHeader("Color Grading, Orientation & Rotation", ImGuiTreeNodeFlags_DefaultOpen)) {
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

		changed = ImGui::Checkbox("Dynamic Stellar & Nebular Micro-Motion", &sky.dynamic_motion) || changed;
		render_setting_tooltip("Deterministic time-coordinated stellar scintillation, filamentary gas drift, and pulsar emission cycles.");
		if (sky.dynamic_motion) {
			changed = slider_double_with_input("Micro-Motion Speed Multiplier", &sky.motion_speed, 0.001, 1.0, "%.3f") || changed;
		}
	}

	ImGui::Spacing();
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

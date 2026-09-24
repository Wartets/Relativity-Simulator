#pragma once

#include <imgui.h>
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/orchestrator/command.hpp"
#include "relativistic/render/gpu_types.hpp"
#include "relativistic/ui/interactive_camera_controller.hpp"
#include "relativistic/ui/hud_layout_config.hpp"
#include "relativistic/ui/schematic_view_config.hpp"
#include "relativistic/render/geodesic_compute_pipeline.hpp"
#include "relativistic/ui/tooltip_utils.hpp"
#include "relativistic/ui/numeric_slider_utils.hpp"
#include "relativistic/ui/compatibility_notes.hpp"
#include "relativistic/units/unit_system.hpp"
#include "relativistic/units/unit_aware_widgets.hpp"
#include "relativistic/optics/sky_panorama_catalog.hpp"
#include <string>
#include <string_view>
#include <vector>
#include <cctype>
#include <array>
#include <cmath>
#include <numbers>
#include <algorithm>

namespace Relativistic::UI {

class ControlPanelWindow {
private:
	bool is_open_{true};
	Orchestrator::SimulationOrchestrator<1024>& orchestrator_;
	InteractiveCameraController& camera_controller_;
	HudLayoutConfig& hud_layout_;
	SchematicViewConfig& schematic_cfg_;
	bool& hud_manager_open_;
	bool& keybind_settings_open_;

	float mass_{1.0f};
	float spin_{0.0f};
	float charge_{0.0f};
	float lambda_{0.0f};
	float throat_{1.0f};
	float warp_vel_{1.0f};
	bool mass_log_mode_{false};
	bool lambda_log_mode_{true};
	bool warp_log_mode_{false};
	double mass_quantity_kg_{0.0};
	std::string mass_expr_error_{};

	float camera_speed_{10.0f};
	float camera_fov_{60.0f};
	float camera_exposure_{0.0f};
	int projection_mode_{0};
	int tonemapping_mode_{0};

	int camera_coord_system_{0};
	float manual_cartesian_position_[3]{0.0f, 32.0f, 0.0f};
	float manual_spherical_position_[3]{32.0f, 1.5707963267948966f, 1.5707963267948966f};
	float manual_orientation_[3]{0.0f, 180.0f, 0.0f};
	bool manual_placement_dirty_{false};

	int metric_selection_{1};
	int integrator_selection_{0};
	bool integrator_rtol_log_mode_{true};
	bool integrator_atol_log_mode_{true};

	float rocket_thrust_x_{0.0f};
	float rocket_thrust_y_{0.0f};
	float rocket_thrust_z_{0.0f};
	float rocket_throttle_{0.0f};
	int timeflow_mode_{0};

	float sky_star_density_{1.0f};
	float sky_star_brightness_{1.0f};
	float sky_nebula_intensity_{1.0f};
	float sky_grid_opacity_{1.0f};
	float sky_rotation_{0.0f};
	float sky_hue_shift_{0.0f};
	float sky_saturation_{1.0f};
	float sky_background_[3]{0.0f, 0.0f, 0.0f};
	float sky_star_brightness_variation_{0.5f};
	float sky_star_size_variation_{0.5f};
	float sky_star_color_variation_{1.0f};
	float sky_star_temperature_bias_{0.0f};
	int sky_procedural_seed_{12345};
	float sky_galaxy_density_{0.0f};
	float sky_galaxy_brightness_{1.0f};
	float sky_galaxy_size_scale_{1.0f};
	float sky_dust_density_{0.0f};
	float sky_dust_intensity_{1.0f};
	float sky_dust_scale_{1.0f};
	float sky_cluster_density_{0.0f};
	float sky_cluster_brightness_{1.0f};
	float sky_cluster_size_scale_{1.0f};
	int sky_background_source_{0};
	int sky_panorama_id_{0};
	int sky_panorama_quality_{1};
	uint64_t last_synced_version_{0};
	Render::GeodesicComputePipeline* render_pipeline_{nullptr};

public:
	explicit ControlPanelWindow(Orchestrator::SimulationOrchestrator<1024>& orchestrator, InteractiveCameraController& camera_controller, HudLayoutConfig& hud_layout, SchematicViewConfig& schematic_cfg, bool& hud_manager_open, bool& keybind_settings_open)
		: orchestrator_(orchestrator), camera_controller_(camera_controller), hud_layout_(hud_layout), schematic_cfg_(schematic_cfg), hud_manager_open_(hud_manager_open), keybind_settings_open_(keybind_settings_open) {
		sync_from_orchestrator();
	}

	void attach_render_pipeline(Render::GeodesicComputePipeline& pipeline) noexcept {
		render_pipeline_ = &pipeline;
	}

	[[nodiscard]] bool& open_state() noexcept {
		return is_open_;
	}

	void sync_from_orchestrator(bool include_manual_placement = true) noexcept {
		const auto& p = orchestrator_.parameters();
		mass_ = static_cast<float>(p.mass);
		spin_ = static_cast<float>(p.spin);
		charge_ = static_cast<float>(p.charge);
		lambda_ = static_cast<float>(p.cosmological_lambda);
		throat_ = static_cast<float>(p.wormhole_throat);
		warp_vel_ = static_cast<float>(p.warp_velocity);
		camera_speed_ = static_cast<float>(p.camera_speed);
		camera_fov_ = static_cast<float>(p.camera_fov_deg);
		camera_exposure_ = static_cast<float>(p.camera_exposure);
		projection_mode_ = static_cast<int>(p.projection_mode);
		tonemapping_mode_ = static_cast<int>(p.tonemapping_mode);
		timeflow_mode_ = static_cast<int>(p.time_flow_mode);
		const std::string& active_m = orchestrator_.active_metric_name();
		if (active_m.find("Minkowski") != std::string::npos) metric_selection_ = 0;
		else if (active_m.find("de Sitter") != std::string::npos) metric_selection_ = 5;
		else if (active_m.find("Schwarzschild") != std::string::npos) metric_selection_ = 1;
		else if (active_m.find("Newman") != std::string::npos) metric_selection_ = 4;
		else if (active_m.find("Kerr") != std::string::npos) metric_selection_ = 2;
		else if (active_m.find("Reissner") != std::string::npos) metric_selection_ = 3;
		else if (active_m.find("FLRW") != std::string::npos) metric_selection_ = 6;
		else if (active_m.find("Morris") != std::string::npos || active_m.find("Wormhole") != std::string::npos) metric_selection_ = 7;
		else if (active_m.find("Alcubierre") != std::string::npos || active_m.find("Warp") != std::string::npos) metric_selection_ = 8;
		else if (active_m.find("BSSN") != std::string::npos) metric_selection_ = 9;

		const std::string& active_i = orchestrator_.active_integrator_name();
		if (active_i.find("Cash") != std::string::npos) integrator_selection_ = 1;
		else if (active_i.find("Vernier") != std::string::npos) integrator_selection_ = 2;
		else if (active_i.find("Gauss") != std::string::npos && active_i.find("6") != std::string::npos) integrator_selection_ = 4;
		else if (active_i.find("Gauss") != std::string::npos) integrator_selection_ = 3;
		else if (active_i.find("Hermite") != std::string::npos) integrator_selection_ = 5;
		else integrator_selection_ = 0;

		if (include_manual_placement) {
			const auto& cam = orchestrator_.camera();
			manual_cartesian_position_[0] = static_cast<float>(cam.position[0]);
			manual_cartesian_position_[1] = static_cast<float>(cam.position[1]);
			manual_cartesian_position_[2] = static_cast<float>(cam.position[2]);
			manual_spherical_position_[0] = static_cast<float>(cam.radius);
			manual_spherical_position_[1] = static_cast<float>(cam.theta);
			manual_spherical_position_[2] = static_cast<float>(cam.phi);
			manual_orientation_[0] = static_cast<float>(cam.pitch);
			manual_orientation_[1] = static_cast<float>(cam.yaw);
			manual_orientation_[2] = static_cast<float>(cam.roll);
		}

		sky_star_density_ = static_cast<float>(p.sky_star_density);
		sky_star_brightness_ = static_cast<float>(p.sky_star_brightness);
		sky_nebula_intensity_ = static_cast<float>(p.sky_nebula_intensity);
		sky_grid_opacity_ = static_cast<float>(p.sky_grid_opacity);
		sky_rotation_ = static_cast<float>(p.sky_rotation_deg);
		sky_hue_shift_ = static_cast<float>(p.sky_hue_shift_deg);
		sky_saturation_ = static_cast<float>(p.sky_saturation);
		sky_background_[0] = static_cast<float>(p.sky_background_r);
		sky_background_[1] = static_cast<float>(p.sky_background_g);
		sky_background_[2] = static_cast<float>(p.sky_background_b);
		sky_star_brightness_variation_ = static_cast<float>(p.sky_star_brightness_variation);
		sky_star_size_variation_ = static_cast<float>(p.sky_star_size_variation);
		sky_star_color_variation_ = static_cast<float>(p.sky_star_color_variation);
		sky_star_temperature_bias_ = static_cast<float>(p.sky_star_temperature_bias);
		sky_procedural_seed_ = static_cast<int>(p.sky_procedural_seed);
		sky_galaxy_density_ = static_cast<float>(p.sky_galaxy_density);
		sky_galaxy_brightness_ = static_cast<float>(p.sky_galaxy_brightness);
		sky_galaxy_size_scale_ = static_cast<float>(p.sky_galaxy_size_scale);
		sky_dust_density_ = static_cast<float>(p.sky_dust_density);
		sky_dust_intensity_ = static_cast<float>(p.sky_dust_intensity);
		sky_dust_scale_ = static_cast<float>(p.sky_dust_scale);
		sky_cluster_density_ = static_cast<float>(p.sky_cluster_density);
		sky_cluster_brightness_ = static_cast<float>(p.sky_cluster_brightness);
		sky_cluster_size_scale_ = static_cast<float>(p.sky_cluster_size_scale);
		sky_background_source_ = static_cast<int>(p.sky_background_source);
		sky_panorama_id_ = static_cast<int>(p.sky_panorama_id);
		sky_panorama_quality_ = static_cast<int>(p.sky_panorama_quality);
	}

	void render() {
		if (!is_open_) return;

		const uint64_t current_ver = orchestrator_.state_version();
		if (current_ver != last_synced_version_) {
			sync_from_orchestrator();
			last_synced_version_ = current_ver;
		} else if (!ImGui::IsAnyItemActive() && !manual_placement_dirty_) {
			sync_from_orchestrator(true);
		}

		ImGui::SetNextWindowPos(ImVec2(1450.0f, 30.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(455.0f, 720.0f), ImGuiCond_FirstUseEver);

		if (ImGui::Begin("Master Simulation Controls", &is_open_)) {
			if (ImGui::BeginTabBar("ControlTabs")) {
				if (ImGui::BeginTabItem("Spacetime & Metrics")) {
					render_metrics_tab();
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Optics & Camera")) {
					render_camera_tab();
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Camera Controls")) {
					render_camera_controls_tab();
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Skybox & Environment")) {
					render_skybox_tab();
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Solvers & Integrators")) {
					render_integrators_tab();
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Relativistic Rocket (6-DOF)")) {
					render_rocket_tab();
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Time & Execution")) {
					render_execution_tab();
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("HUD & Overlay")) {
					render_hud_tab();
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Schematic View")) {
					render_schematic_tab();
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("3D Body Render")) {
					render_body_3d_render_tab();
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Units & Scales")) {
					render_units_tab();
					ImGui::EndTabItem();
				}
				ImGui::EndTabBar();
			}
		}
		ImGui::End();
	}

private:
	void render_metrics_tab() noexcept {
		const char* metric_names[] = {
			"Flat Minkowski",
			"Schwarzschild Black Hole",
			"Kerr Rotating Black Hole",
			"Reissner-Nordstrom Charged",
			"Kerr-Newman Charged Rotating",
			"Schwarzschild-de Sitter (Lambda)",
			"FLRW Cosmological Expansion",
			"Morris-Thorne Traversable Wormhole",
			"Alcubierre Warp Drive Bubble",
			"BSSN 3+1 Numerical Grid"
		};

		if (ImGui::Combo("Spacetime Metric", &metric_selection_, metric_names, IM_ARRAYSIZE(metric_names))) {
			orchestrator_.set_active_metric_name(metric_names[metric_selection_]);
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_metric(metric_names[metric_selection_])));
		}
		{
			const auto warning = metric_integrator_incompatibility(orchestrator_.active_metric_name(), orchestrator_.active_integrator_name());
			if (!warning.empty()) {
				render_setting_tooltip_warning("Select the background Riemannian manifold or exact vacuum/electrovacuum spacetime solution to simulate.", std::string(warning).c_str());
			} else {
				render_setting_tooltip("Select the background Riemannian manifold or exact vacuum/electrovacuum spacetime solution to simulate.");
			}
		}

		ImGui::Separator();

		const bool needs_mass = (metric_selection_ == 1 || metric_selection_ == 2 || metric_selection_ == 3 || metric_selection_ == 4 || metric_selection_ == 5);
		const bool needs_spin = (metric_selection_ == 2 || metric_selection_ == 4);
		const bool needs_charge = (metric_selection_ == 3 || metric_selection_ == 4);
		const bool needs_lambda = (metric_selection_ == 5);
		const bool needs_throat = (metric_selection_ == 7);
		const bool needs_warp_velocity = (metric_selection_ == 8);
		const bool has_any_param = needs_mass || needs_spin || needs_charge || needs_lambda || needs_throat || needs_warp_velocity;

		if (needs_mass) {
			const double active_mass_scale_disp = orchestrator_.constants_engine().mass_scale();
			double mass_kg_disp = static_cast<double>(mass_) * active_mass_scale_disp;
			const double mass_min_kg_disp = 0.01 * active_mass_scale_disp;
			const double mass_max_kg_disp = 100.0 * active_mass_scale_disp;
			if (unit_aware_slider_double("Central Mass (M)", &mass_kg_disp, mass_min_kg_disp, mass_max_kg_disp, UnitCategory::Mass, orchestrator_.unit_preferences(), "%.3f", &mass_log_mode_)) {
				mass_ = static_cast<float>(mass_kg_disp / active_mass_scale_disp);
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::Mass, static_cast<double>(mass_))));
			}
			render_setting_tooltip(("Central gravitating mass, displayed in " + std::string(Units::mass_unit_suffix(orchestrator_.unit_preferences().mass)) + ". Governs Schwarzschild radius rs = 2M and spacetime curvature strength. Enable Log for finer control across small or very large magnitudes.").c_str());

			const double active_mass_scale = orchestrator_.constants_engine().mass_scale();
			mass_quantity_kg_ = static_cast<double>(mass_) * active_mass_scale;
			if (smart_quantity_input("M  [dim: Mass]", &mass_quantity_kg_, Units::Dimensions::Mass, mass_expr_error_)) {
				if (active_mass_scale > 0.0) {
					const double new_mass = mass_quantity_kg_ / active_mass_scale;
					mass_ = static_cast<float>(new_mass);
					static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::Mass, new_mass)));
				}
			}
			render_setting_tooltip("Enter the central mass as a plain value or as a unit-aware expression, for example \"1.989e30 kg\" or \"1 msun\". The value must have dimensions of mass and is converted through the simulation's current Mass Scale factor from the Physical Constants Engine.");
		}

		if (needs_spin) {
			if (slider_float_with_input("Spin Parameter (a)", &spin_, -0.999f, 0.999f, "%.4f")) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::Spin, static_cast<double>(spin_))));
			}
			const auto spin_warning = metric_spin_incompatibility(orchestrator_.active_metric_name(), static_cast<double>(spin_), static_cast<double>(mass_));
			if (!spin_warning.empty()) {
				render_setting_tooltip_warning("Specific angular momentum a = J / M. Deforms the event horizon into an oblate spheroid and induces Lense-Thirring frame-dragging.", std::string(spin_warning).c_str());
			} else {
				render_setting_tooltip("Specific angular momentum a = J / M. Deforms the event horizon into an oblate spheroid and induces Lense-Thirring frame-dragging.");
			}
		}

		if (needs_charge) {
			double charge_disp = static_cast<double>(charge_);
			if (unit_aware_slider_double("Electric Charge (Q)", &charge_disp, -1.0, 1.0, UnitCategory::Charge, orchestrator_.unit_preferences(), "%.3f")) {
				charge_ = static_cast<float>(charge_disp);
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::Charge, static_cast<double>(charge_))));
			}
			render_setting_tooltip(("Net electrostatic charge, displayed in " + std::string(Units::charge_unit_suffix(orchestrator_.unit_preferences().charge)) + ". Creates an inner Cauchy horizon and counteracts gravitational attraction.").c_str());
		}

		if (needs_lambda) {
			if (slider_float_with_input("Cosmological Lambda", &lambda_, 1e-8f, 1e-2f, "%.2e", &lambda_log_mode_)) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::CosmologicalLambda, static_cast<double>(lambda_))));
			}
			render_setting_tooltip("Cosmological constant responsible for large-scale cosmic acceleration and cosmological horizon creation. Logarithmic mode is on by default since this value typically spans many orders of magnitude.");
		}

		if (needs_throat) {
			if (ImGui::SliderFloat("Wormhole Throat (b0)", &throat_, 0.1f, 20.0f, "%.2f")) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::WormholeThroat, static_cast<double>(throat_))));
			}
			render_setting_tooltip("Radius of the non-singular throat b0 for the Morris-Thorne wormhole connecting two distinct asymptotically flat universes.");
		}

		if (needs_warp_velocity) {
			if (ImGui::SliderFloat("Warp Bubble Velocity (vs)", &warp_vel_, 0.0f, 10.0f, "%.2f c")) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::WarpVelocity, static_cast<double>(warp_vel_))));
			}
			render_setting_tooltip("Apparent transluminal velocity of the Alcubierre spacetime bubble contracting space ahead and expanding behind.");
		}

		if (!has_any_param) {
			ImGui::TextDisabled("This spacetime model exposes no adjustable parameters here.");
		}
	}

	void render_camera_tab() noexcept {
		const char* projections[] = {
			"Standard Perspective (Pinhole)",
			"Auto-Zoom (Aberration Comp.)",
			"Fisheye Stereographic (Conformal)",
			"Equirectangular 360 Panorama",
			"Fisheye Equidistant (All-Sky)",
			"Fisheye Orthographic (Hemisphere)",
			"Panini Cylindrical (Wide-Angle)",
			"Hammer-Aitoff (Equal-Area)"
		};

		if (ImGui::Combo("Projection Mode", &projection_mode_, projections, IM_ARRAYSIZE(projections))) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::ProjectionMode, static_cast<double>(projection_mode_))));
		}
		render_setting_tooltip("Optical projection geometry used to map the celestial sphere onto the screen (Pinhole, Panoramas, Fisheyes, Hammer-Aitoff).");

		const char* tonemappers[] = {
			"Linear Unclamped",
			"ACES Filmic Curve",
			"Logarithmic Extended HDR",
			"Reinhard Modified"
		};

		if (ImGui::Combo("HDR Tonemapper", &tonemapping_mode_, tonemappers, IM_ARRAYSIZE(tonemappers))) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::TonemappingMode, static_cast<double>(tonemapping_mode_))));
		}
		render_setting_tooltip("Tone mapping operator compressing high dynamic range extreme radiant flux down to standard 8-bit sRGB display gamuts.");

		ImGui::Spacing();
		ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.35f, 1.0f), "Accretion Disk Spectral Color Model:");
		auto& disk_params = orchestrator_.parameters();
		float disk_temp_scale = static_cast<float>(disk_params.disk_temperature_scale_k);
		if (ImGui::SliderFloat("Disk Peak Temperature Scale (K)", &disk_temp_scale, 2000.0f, 40000.0f, "%.0f K")) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::DiskTemperatureScale, static_cast<double>(disk_temp_scale))));
		}
		render_setting_tooltip("Blackbody temperature at the innermost edge of the accretion disk before Doppler shifting. The visible disk color is computed by integrating this exact blackbody spectrum through CIE 1931 color matching functions, so higher values push the inner disk toward blue-white and lower values toward orange-red.");
		float disk_temp_floor = static_cast<float>(disk_params.disk_temperature_floor_k);
		if (ImGui::SliderFloat("Disk Outer Edge Temperature Floor (K)", &disk_temp_floor, 0.0f, 6000.0f, "%.0f K")) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::DiskTemperatureFloor, static_cast<double>(disk_temp_floor))));
		}
		render_setting_tooltip("Minimum blackbody temperature retained at the outer disk edge, preventing the coolest visible disk radius from going fully dark.");
		float disk_beaming = static_cast<float>(disk_params.disk_doppler_beaming_exponent);
		if (ImGui::SliderFloat("Doppler Beaming Exponent", &disk_beaming, 0.0f, 8.0f, "%.2f")) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::DiskDopplerBeamingExponent, static_cast<double>(disk_beaming))));
		}
		render_setting_tooltip("Power applied to the relativistic Doppler factor g when brightening the approaching side of the disk and dimming the receding side. The physically exact value for specific intensity is 4; lower values soften the asymmetry for a more balanced look.");
		float disk_saturation = static_cast<float>(disk_params.disk_color_saturation);
		if (ImGui::SliderFloat("Disk Color Saturation", &disk_saturation, 0.0f, 3.0f, "%.2fx")) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::DiskColorSaturation, static_cast<double>(disk_saturation))));
		}
		render_setting_tooltip("Post-spectral chroma multiplier applied around the disk's computed luminance. 1.0 keeps the physically derived CIE color; higher values intensify the color contrast between hot and cool disk regions.");

		ImGui::Spacing();
		ImGui::TextColored(ImVec4(0.85f, 0.6f, 1.0f, 1.0f), "Color Grading (Post-Tonemap):");
		auto& grading_params = orchestrator_.parameters();
		float post_contrast = static_cast<float>(grading_params.post_contrast);
		if (ImGui::SliderFloat("Contrast", &post_contrast, 0.1f, 3.0f, "%.2fx")) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::PostContrast, static_cast<double>(post_contrast))));
		}
		render_setting_tooltip("Scales pixel values around mid-gray (0.5) after tonemapping. Above 1 increases separation between shadows and highlights; below 1 flattens the image.");
		float post_saturation = static_cast<float>(grading_params.post_saturation);
		if (ImGui::SliderFloat("Saturation", &post_saturation, 0.0f, 3.0f, "%.2fx")) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::PostSaturation, static_cast<double>(post_saturation))));
		}
		render_setting_tooltip("Blends each pixel with its luminance. 0 produces a grayscale image, 1 leaves color unchanged, above 1 exaggerates color intensity.");
		float post_lift = static_cast<float>(grading_params.post_lift);
		if (ImGui::SliderFloat("Lift (Shadows Offset)", &post_lift, -0.5f, 0.5f, "%.3f")) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::PostLift, static_cast<double>(post_lift))));
		}
		render_setting_tooltip("Adds a constant offset to darker tones, raising or crushing black levels without affecting highlights as strongly.");
		float post_gamma = static_cast<float>(grading_params.post_gamma);
		if (ImGui::SliderFloat("Gamma (Midtones)", &post_gamma, 0.2f, 3.0f, "%.3f")) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::PostGamma, static_cast<double>(post_gamma))));
		}
		render_setting_tooltip("Applies a power curve to midtones. Below 1 brightens midtones, above 1 darkens them, leaving pure black and white unaffected.");
		float post_gain = static_cast<float>(grading_params.post_gain);
		if (ImGui::SliderFloat("Gain (Highlights)", &post_gain, 0.1f, 3.0f, "%.3f")) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::PostGain, static_cast<double>(post_gain))));
		}
		render_setting_tooltip("Multiplies the overall signal after lift and gamma are applied, primarily affecting bright highlights.");
		float post_highlights = static_cast<float>(grading_params.post_highlights);
		if (ImGui::SliderFloat("Highlights Recovery", &post_highlights, -1.0f, 1.0f, "%.2f")) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::PostHighlights, static_cast<double>(post_highlights))));
		}
		render_setting_tooltip("Negative values compress the brightest regions to reveal clipped detail near the accretion disk core; positive values push highlights brighter.");
		float post_shadows = static_cast<float>(grading_params.post_shadows);
		if (ImGui::SliderFloat("Shadows Recovery", &post_shadows, -1.0f, 1.0f, "%.2f")) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::PostShadows, static_cast<double>(post_shadows))));
		}
		render_setting_tooltip("Positive values lift detail out of dark regions near the event horizon; negative values deepen shadows for more contrast.");
		float post_vignette = static_cast<float>(grading_params.post_vignette_strength);
		if (ImGui::SliderFloat("Vignette Strength", &post_vignette, 0.0f, 1.0f, "%.2f")) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::PostVignetteStrength, static_cast<double>(post_vignette))));
		}
		render_setting_tooltip("Darkens the corners of the frame radially from the center, applied after every other color grading step. Zero disables the effect entirely.");
		if (ImGui::Button("Reset Color Grading", ImVec2(200.0f, 26.0f))) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::PostContrast, 1.0)));
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::PostSaturation, 1.0)));
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::PostLift, 0.0)));
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::PostGamma, 1.0)));
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::PostGain, 1.0)));
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::PostHighlights, 0.0)));
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::PostShadows, 0.0)));
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::PostVignetteStrength, 0.0)));
		}

		ImGui::Separator();

		const char* cam_modes[] = {"Free Fly 6-DOF", "Orbit Center Target", "Spherical (Boyer-Lindquist)", "Rocket 6-DOF Thrust"};
		int mode = static_cast<int>(orchestrator_.parameters().camera_mode);
		if (ImGui::Combo("Camera Mode", &mode, cam_modes, IM_ARRAYSIZE(cam_modes))) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_camera_mode(static_cast<uint32_t>(mode))));
		}
		render_setting_tooltip("Observer navigation paradigm (6-DOF Free Fly, Spherical Boyer-Lindquist Orbit, Cockpit Flight).");

		{
			double fov_rad_disp = static_cast<double>(camera_fov_) * (std::numbers::pi / 180.0);
			const double fov_min_rad_disp = 10.0 * (std::numbers::pi / 180.0);
			const double fov_max_rad_disp = 160.0 * (std::numbers::pi / 180.0);
			if (unit_aware_slider_double("Field of View (FOV)", &fov_rad_disp, fov_min_rad_disp, fov_max_rad_disp, UnitCategory::Angle, orchestrator_.unit_preferences(), "%.1f")) {
				camera_fov_ = static_cast<float>(fov_rad_disp * (180.0 / std::numbers::pi));
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_camera_set_fov(static_cast<double>(camera_fov_))));
			}
		}
		render_setting_tooltip(("Horizontal angular aperture, displayed in " + std::string(Units::angle_unit_suffix(orchestrator_.unit_preferences().angle)) + ". Can also be dynamically zoomed using mouse wheel scroll.").c_str());

		{
			const auto& speed_ce = orchestrator_.constants_engine();
			const double speed_scale_mps = speed_ce.length_scale() / speed_ce.time_scale();
			double speed_disp = static_cast<double>(camera_speed_) * speed_scale_mps;
			const double speed_min_disp = 0.1 * speed_scale_mps;
			const double speed_max_disp = 100.0 * speed_scale_mps;
			if (unit_aware_slider_double("Navigation Speed", &speed_disp, speed_min_disp, speed_max_disp, UnitCategory::Velocity, orchestrator_.unit_preferences(), "%.1f")) {
				camera_speed_ = static_cast<float>(speed_disp / speed_scale_mps);
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_camera_set_speed(static_cast<double>(camera_speed_))));
				camera_controller_.set_uniform_speed(static_cast<double>(camera_speed_));
			}
		}
		render_setting_tooltip(("Translational observer traversal speed, displayed in " + std::string(Units::velocity_unit_suffix(orchestrator_.unit_preferences().velocity)) + ". Hold Shift to sprint, Ctrl to crawl.").c_str());

		if (ImGui::SliderFloat("Exposure Compensation (EV)", &camera_exposure_, -6.0f, 6.0f, "%.2f EV")) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::CameraExposure, static_cast<double>(camera_exposure_))));
		}
		render_setting_tooltip("Logarithmic optical sensitivity compensation in Exposure Values (EV). Higher values brighten dim accretion emission.");

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.6f, 1.0f), "Manual Camera Placement:");

		const char* coord_systems[] = {"Cartesian (x, y, z)", "Spherical (r, theta, phi)"};
		ImGui::Combo("Coordinate System", &camera_coord_system_, coord_systems, IM_ARRAYSIZE(coord_systems));

		if (camera_coord_system_ == 0) {
			if (ImGui::InputFloat3("Position (x, y, z)", manual_cartesian_position_)) {
				manual_placement_dirty_ = true;
			}
		} else {
			if (ImGui::InputFloat("Radius (r)", &manual_spherical_position_[0])) {
				manual_placement_dirty_ = true;
			}
			if (ImGui::SliderAngle("Polar Angle (theta)", &manual_spherical_position_[1], 0.1f, 179.9f)) {
				manual_placement_dirty_ = true;
			}
			if (ImGui::SliderAngle("Azimuthal Angle (phi)", &manual_spherical_position_[2], -180.0f, 180.0f)) {
				manual_placement_dirty_ = true;
			}
		}

		if (ImGui::InputFloat3("Orientation (pitch, yaw, roll)", manual_orientation_)) {
			manual_placement_dirty_ = true;
		}

		if (ImGui::Button("Apply Camera Placement", ImVec2(220.0f, 28.0f))) {
			apply_manual_camera_placement();
			manual_placement_dirty_ = false;
		}
	}

	void render_camera_controls_tab() noexcept {
		auto& cfg = camera_controller_.config();

		ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.6f, 1.0f), "Mouse Look:");
		float mouse_sens = static_cast<float>(cfg.free_fly.mouse_sensitivity);
		if (ImGui::SliderFloat("Mouse Sensitivity", &mouse_sens, 0.01f, 1.0f, "%.3f")) {
			cfg.free_fly.mouse_sensitivity = static_cast<double>(mouse_sens);
		}
		bool invert_mouse_y = cfg.free_fly.invert_mouse_y;
		if (ImGui::Checkbox("Invert Mouse Y", &invert_mouse_y)) {
			cfg.free_fly.invert_mouse_y = invert_mouse_y;
		}
		ImGui::SameLine();
		bool invert_mouse_x = cfg.free_fly.invert_mouse_x;
		if (ImGui::Checkbox("Invert Mouse X", &invert_mouse_x)) {
			cfg.free_fly.invert_mouse_x = invert_mouse_x;
		}

		ImGui::Separator();
		bool ignore_pitch_roll = cfg.free_fly.ignore_pitch_roll_for_movement;
		if (ImGui::Checkbox("Horizontal Movement (Ignore Look Pitch/Roll)", &ignore_pitch_roll)) {
			cfg.free_fly.ignore_pitch_roll_for_movement = ignore_pitch_roll;
		}
		render_setting_tooltip("When enabled, forward/back and strafe keys always move along the horizontal plane regardless of where the camera is currently looking, matching the conventional first-person navigation used in most video games. Vertical keys still move straight up or down along the world axis.");

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.6f, 1.0f), "Hold-to-Zoom (W / Z Key):");
		bool zoom_on_cursor = cfg.zoom.zoom_center_on_cursor;
		if (ImGui::Checkbox("Zoom Toward Cursor Position", &zoom_on_cursor)) {
			cfg.zoom.zoom_center_on_cursor = zoom_on_cursor;
		}
		render_setting_tooltip("When enabled, holding the zoom key magnifies the region under the mouse cursor. When disabled, zoom is always centered on the middle of the viewport.");
		float zoom_sens = static_cast<float>(cfg.zoom.zoom_scroll_sensitivity);
		if (ImGui::SliderFloat("Zoom Scroll Sensitivity", &zoom_sens, 0.02f, 1.0f, "%.2f")) {
			cfg.zoom.zoom_scroll_sensitivity = static_cast<double>(zoom_sens);
		}
		float zoom_max = static_cast<float>(cfg.zoom.max_zoom);
		if (ImGui::SliderFloat("Maximum Zoom Level", &zoom_max, 1.5f, 16.0f, "%.1fx")) {
			cfg.zoom.max_zoom = static_cast<double>(zoom_max);
		}

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.6f, 1.0f), "Free Fly 6-DOF Axis Speeds:");
		const double ff_speed_scale_mps = orchestrator_.constants_engine().length_scale() / orchestrator_.constants_engine().time_scale();
		double ff_fwd = cfg.free_fly.forward_speed * ff_speed_scale_mps;
		if (unit_aware_slider_double("Forward/Back Speed", &ff_fwd, 0.1 * ff_speed_scale_mps, 200.0 * ff_speed_scale_mps, UnitCategory::Velocity, orchestrator_.unit_preferences(), "%.1f")) cfg.free_fly.forward_speed = ff_fwd / ff_speed_scale_mps;
		double ff_lat = cfg.free_fly.lateral_speed * ff_speed_scale_mps;
		if (unit_aware_slider_double("Left/Right Speed", &ff_lat, 0.1 * ff_speed_scale_mps, 200.0 * ff_speed_scale_mps, UnitCategory::Velocity, orchestrator_.unit_preferences(), "%.1f")) cfg.free_fly.lateral_speed = ff_lat / ff_speed_scale_mps;
		double ff_vert = cfg.free_fly.vertical_speed * ff_speed_scale_mps;
		if (unit_aware_slider_double("Up/Down Speed", &ff_vert, 0.1 * ff_speed_scale_mps, 200.0 * ff_speed_scale_mps, UnitCategory::Velocity, orchestrator_.unit_preferences(), "%.1f")) cfg.free_fly.vertical_speed = ff_vert / ff_speed_scale_mps;
		bool ff_invert_vert = cfg.free_fly.invert_vertical;
		if (ImGui::Checkbox("Invert Up/Down Keys", &ff_invert_vert)) cfg.free_fly.invert_vertical = ff_invert_vert;
		ImGui::SameLine();
		bool ff_invert_lat = cfg.free_fly.invert_lateral;
		if (ImGui::Checkbox("Invert Left/Right Keys", &ff_invert_lat)) cfg.free_fly.invert_lateral = ff_invert_lat;
		float ff_sprint = static_cast<float>(cfg.free_fly.sprint_multiplier);
		if (ImGui::SliderFloat("Sprint Multiplier", &ff_sprint, 1.0f, 20.0f, "%.1fx")) cfg.free_fly.sprint_multiplier = static_cast<double>(ff_sprint);
		float ff_crawl = static_cast<float>(cfg.free_fly.crawl_multiplier);
		if (ImGui::SliderFloat("Crawl Multiplier", &ff_crawl, 0.01f, 1.0f, "%.2fx")) cfg.free_fly.crawl_multiplier = static_cast<double>(ff_crawl);

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.6f, 1.0f), "Orbit Center Mode:");
		const double orbit_speed_scale_mps = orchestrator_.constants_engine().length_scale() / orchestrator_.constants_engine().time_scale();
		double orb_dist = cfg.orbit.orbit_distance_speed * orbit_speed_scale_mps;
		if (unit_aware_slider_double("Zoom Speed", &orb_dist, 0.1 * orbit_speed_scale_mps, 200.0 * orbit_speed_scale_mps, UnitCategory::Velocity, orchestrator_.unit_preferences(), "%.1f")) cfg.orbit.orbit_distance_speed = orb_dist / orbit_speed_scale_mps;
		float orb_pitch = static_cast<float>(cfg.orbit.pitch_speed_deg_s);
		if (ImGui::SliderFloat("Pitch Speed", &orb_pitch, 1.0f, 180.0f, "%.1f deg/s")) cfg.orbit.pitch_speed_deg_s = static_cast<double>(orb_pitch);
		float orb_yaw = static_cast<float>(cfg.orbit.yaw_speed_deg_s);
		if (ImGui::SliderFloat("Yaw Speed", &orb_yaw, 1.0f, 180.0f, "%.1f deg/s")) cfg.orbit.yaw_speed_deg_s = static_cast<double>(orb_yaw);
		bool orb_invert_pitch = cfg.orbit.invert_pitch;
		if (ImGui::Checkbox("Invert Orbit Pitch Keys", &orb_invert_pitch)) cfg.orbit.invert_pitch = orb_invert_pitch;

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.6f, 1.0f), "Rocket 6-DOF Thrust Mode:");
		ImGui::TextDisabled("Thrust integrates only while simulation time is running (unpaused).");
		float rk_main = static_cast<float>(cfg.rocket.main_thrust_accel);
		if (ImGui::SliderFloat("Main Thrust Accel", &rk_main, 0.1f, 500.0f, "%.1f")) cfg.rocket.main_thrust_accel = static_cast<double>(rk_main);
		float rk_lat = static_cast<float>(cfg.rocket.lateral_thrust_accel);
		if (ImGui::SliderFloat("Lateral Thrust Accel", &rk_lat, 0.1f, 500.0f, "%.1f")) cfg.rocket.lateral_thrust_accel = static_cast<double>(rk_lat);
		float rk_vert = static_cast<float>(cfg.rocket.vertical_thrust_accel);
		if (ImGui::SliderFloat("Vertical Thrust Accel", &rk_vert, 0.1f, 500.0f, "%.1f")) cfg.rocket.vertical_thrust_accel = static_cast<double>(rk_vert);
		float rk_ang = static_cast<float>(cfg.rocket.angular_rate_deg_s);
		if (ImGui::SliderFloat("Roll Rate", &rk_ang, 1.0f, 360.0f, "%.1f deg/s")) cfg.rocket.angular_rate_deg_s = static_cast<double>(rk_ang);
		bool rk_invert_vert = cfg.rocket.invert_vertical;
		if (ImGui::Checkbox("Invert Rocket Up/Down", &rk_invert_vert)) cfg.rocket.invert_vertical = rk_invert_vert;
		ImGui::SameLine();
		bool rk_invert_lat = cfg.rocket.invert_lateral;
		if (ImGui::Checkbox("Invert Rocket Left/Right", &rk_invert_lat)) cfg.rocket.invert_lateral = rk_invert_lat;
		bool rk_requires_time = cfg.rocket.requires_time_running;
		if (ImGui::Checkbox("Require Unpaused Time For Thrust", &rk_requires_time)) cfg.rocket.requires_time_running = rk_requires_time;

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.6f, 1.0f), "Keybind Configuration:");
		ImGui::TextDisabled("Full rebinding, conflict handling, and keyboard layout presets (QWERTY/AZERTY) are managed in the dedicated Keybind Settings window.");
		const bool keybind_window_already_open = keybind_settings_open_;
		if (keybind_window_already_open) {
			ImGui::BeginDisabled(true);
		}
		if (ImGui::Button(keybind_window_already_open ? "Keybind Settings Open" : "Open Keybind Settings", ImVec2(220.0f, 28.0f))) {
			keybind_settings_open_ = true;
		}
		if (keybind_window_already_open) {
			ImGui::EndDisabled();
			ImGui::SameLine();
			if (ImGui::Button("Focus Window", ImVec2(120.0f, 28.0f))) {
				ImGui::SetWindowFocus("Keybind Settings");
			}
		}
	}

	void apply_manual_camera_placement() noexcept {
		auto& cam = orchestrator_.camera();

		if (camera_coord_system_ == 0) {
			cam.position = {
				static_cast<double>(manual_cartesian_position_[0]),
				static_cast<double>(manual_cartesian_position_[1]),
				static_cast<double>(manual_cartesian_position_[2])
			};
		} else {
			const double r = static_cast<double>(manual_spherical_position_[0]);
			const double theta = static_cast<double>(manual_spherical_position_[1]);
			const double phi = static_cast<double>(manual_spherical_position_[2]);
			cam.position = {
				r * std::sin(theta) * std::cos(phi),
				r * std::sin(theta) * std::sin(phi),
				r * std::cos(theta)
			};
		}

		cam.pitch = std::clamp(static_cast<double>(manual_orientation_[0]), -89.0, 89.0);
		cam.yaw = static_cast<double>(manual_orientation_[1]);
		cam.roll = static_cast<double>(manual_orientation_[2]);

		const double x = cam.position[0];
		const double y = cam.position[1];
		const double z = cam.position[2];
		const double r_new = std::sqrt(x * x + y * y + z * z);
		cam.radius = std::max(r_new, 1e-6);
		cam.theta = (r_new > 0.0) ? std::acos(std::clamp(z / r_new, -1.0, 1.0)) : (std::numbers::pi_v<double> / 2.0);
		cam.phi = std::atan2(y, x);
		cam.orbit_distance = cam.radius;
		orchestrator_.notify_state_changed();
	}

	void render_skybox_tab() noexcept {
		ImGui::TextColored(ImVec4(0.3f, 0.9f, 1.0f, 1.0f), "Sky Background Source");
		const char* sky_sources[] = {"Procedural Generation", "Imported Real Sky Panorama"};
		if (ImGui::Combo("Background Source", &sky_background_source_, sky_sources, IM_ARRAYSIZE(sky_sources))) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SkyBackgroundSource, static_cast<double>(sky_background_source_))));
			if (sky_background_source_ == 1 && !orchestrator_.parameters().use_gpu_compute && render_pipeline_ != nullptr && render_pipeline_->gpu_compute_available()) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::UseGpuCompute, 1.0)));
			}
		}
		render_setting_tooltip("Switches the background between the fully procedural starfield engine and a real equirectangular sky panorama loaded from disk.");
		ImGui::Separator();

		if (sky_background_source_ == 1) {
			render_sky_panorama_controls();
			return;
		}

		const char* sky_modes[] = {
			"Full Starfield",
			"Grid Sphere",
			"Composite Overlay (Starfield + Grid Overlay)",
			"Dark Cosmic Void",
			"Starfield without Nebula",
			"Grid Sphere with Stars"
		};

		int current_sky = static_cast<int>(orchestrator_.parameters().visual_overlays_flags & Render::RenderFlags::SKYBOX_MODE_MASK);
		if (current_sky < 0 || current_sky > 5) {
			current_sky = static_cast<int>(Render::RenderFlags::SKYBOX_STARS);
		}
		if (ImGui::Combo("Skybox Style", &current_sky, sky_modes, IM_ARRAYSIZE(sky_modes))) {
			uint32_t flags = orchestrator_.parameters().visual_overlays_flags & ~Render::RenderFlags::SKYBOX_MODE_MASK;
			flags |= static_cast<uint32_t>(current_sky);
			orchestrator_.parameters().visual_overlays_flags = flags;
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::VisualOverlays, static_cast<double>(flags))));
		}
		render_setting_tooltip("Background celestial radiance model: celestial starfields, spherical coordinate grids, or void absorption.");

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Skybox Presets:");
		auto apply_sky_mode = [&](uint32_t mode) noexcept {
			uint32_t flags = (orchestrator_.parameters().visual_overlays_flags & ~Render::RenderFlags::SKYBOX_MODE_MASK) | mode;
			orchestrator_.parameters().visual_overlays_flags = flags;
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::VisualOverlays, static_cast<double>(flags))));
		};
		if (ImGui::Button("Full Starfield", ImVec2(120.0f, 26.0f))) {
			apply_sky_mode(Render::RenderFlags::SKYBOX_STARS);
		}
		ImGui::SameLine();
		if (ImGui::Button("Grid Sphere", ImVec2(110.0f, 26.0f))) {
			apply_sky_mode(Render::RenderFlags::SKYBOX_GRID);
		}
		ImGui::SameLine();
		if (ImGui::Button("Composite Overlay", ImVec2(130.0f, 26.0f))) {
			apply_sky_mode(Render::RenderFlags::SKYBOX_COMPOSITE);
		}
		if (ImGui::Button("Dark Void", ImVec2(120.0f, 26.0f))) {
			apply_sky_mode(Render::RenderFlags::SKYBOX_VOID);
		}
		ImGui::SameLine();
		if (ImGui::Button("Starfield (No Nebula)", ImVec2(170.0f, 26.0f))) {
			apply_sky_mode(Render::RenderFlags::SKYBOX_STARS_NO_NEBULA);
		}
		ImGui::SameLine();
		if (ImGui::Button("Grid + Stars", ImVec2(110.0f, 26.0f))) {
			apply_sky_mode(Render::RenderFlags::SKYBOX_GRID_STARS);
		}

		ImGui::Separator();
		ImGui::Text("Camera Quick Viewpoints:");
		if (ImGui::Button("Equatorial View (r=50)")) {
			camera_controller_.snap_to_equatorial_front(50.0);
			orchestrator_.notify_state_changed();
		}
		ImGui::SameLine();
		if (ImGui::Button("Top Polar View (z=50)")) {
			camera_controller_.snap_to_north_pole(50.0);
			orchestrator_.notify_state_changed();
		}
		ImGui::SameLine();
		if (ImGui::Button("Close-up ISCO (r=8)")) {
			camera_controller_.snap_to_equatorial_front(8.0);
			orchestrator_.notify_state_changed();
		}

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Sky Style Customization:");

		const bool has_stars = (current_sky == static_cast<int>(Render::RenderFlags::SKYBOX_STARS)) ||
			(current_sky == static_cast<int>(Render::RenderFlags::SKYBOX_COMPOSITE)) ||
			(current_sky == static_cast<int>(Render::RenderFlags::SKYBOX_STARS_NO_NEBULA)) ||
			(current_sky == static_cast<int>(Render::RenderFlags::SKYBOX_GRID_STARS));
		const bool has_nebula = (current_sky == static_cast<int>(Render::RenderFlags::SKYBOX_STARS)) ||
			(current_sky == static_cast<int>(Render::RenderFlags::SKYBOX_COMPOSITE));
		const bool has_grid = (current_sky == static_cast<int>(Render::RenderFlags::SKYBOX_GRID)) ||
			(current_sky == static_cast<int>(Render::RenderFlags::SKYBOX_COMPOSITE)) ||
			(current_sky == static_cast<int>(Render::RenderFlags::SKYBOX_GRID_STARS));
		const bool has_rotation_hue = (current_sky != static_cast<int>(Render::RenderFlags::SKYBOX_VOID));

		if (has_stars) {
			if (ImGui::SliderFloat("Star Density", &sky_star_density_, 0.0f, 4.0f, "%.2fx")) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SkyStarDensity, static_cast<double>(sky_star_density_))));
			}
			if (ImGui::SliderFloat("Star Brightness", &sky_star_brightness_, 0.0f, 4.0f, "%.2fx")) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SkyStarBrightness, static_cast<double>(sky_star_brightness_))));
			}
		}
		if (has_nebula) {
			if (ImGui::SliderFloat("Nebula Glow Intensity", &sky_nebula_intensity_, 0.0f, 4.0f, "%.2fx")) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SkyNebulaIntensity, static_cast<double>(sky_nebula_intensity_))));
			}
		}
		if (has_grid) {
			if (ImGui::SliderFloat("Coordinate Grid Opacity", &sky_grid_opacity_, 0.0f, 2.0f, "%.2fx")) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SkyGridOpacity, static_cast<double>(sky_grid_opacity_))));
			}
		}
		if (has_rotation_hue) {
			if (ImGui::SliderFloat("Sky Rotation", &sky_rotation_, -180.0f, 180.0f, "%.1f deg")) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SkyRotation, static_cast<double>(sky_rotation_))));
			}
			if (ImGui::SliderFloat("Sky Hue Shift", &sky_hue_shift_, -180.0f, 180.0f, "%.1f deg")) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SkyHueShift, static_cast<double>(sky_hue_shift_))));
			}
			if (ImGui::SliderFloat("Sky Saturation", &sky_saturation_, 0.0f, 2.0f, "%.2fx")) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SkySaturation, static_cast<double>(sky_saturation_))));
			}
		}
		if (ImGui::ColorEdit3("Background Tint", sky_background_)) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SkyBackgroundR, static_cast<double>(sky_background_[0]))));
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SkyBackgroundG, static_cast<double>(sky_background_[1]))));
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SkyBackgroundB, static_cast<double>(sky_background_[2]))));
		}
		if (ImGui::Button("Reset Sky Style to Defaults", ImVec2(220.0f, 26.0f))) {
			sky_star_density_ = 1.0f;
			sky_star_brightness_ = 1.0f;
			sky_nebula_intensity_ = 1.0f;
			sky_grid_opacity_ = 1.0f;
			sky_rotation_ = 0.0f;
			sky_hue_shift_ = 0.0f;
			sky_saturation_ = 1.0f;
			sky_background_[0] = 0.0f;
			sky_background_[1] = 0.0f;
			sky_background_[2] = 0.0f;
			sky_star_brightness_variation_ = 0.5f;
			sky_star_size_variation_ = 0.5f;
			sky_star_color_variation_ = 1.0f;
			sky_star_temperature_bias_ = 0.0f;
			sky_galaxy_density_ = 0.0f;
			sky_galaxy_brightness_ = 1.0f;
			sky_galaxy_size_scale_ = 1.0f;
			sky_dust_density_ = 0.0f;
			sky_dust_intensity_ = 1.0f;
			sky_dust_scale_ = 1.0f;
			sky_cluster_density_ = 0.0f;
			sky_cluster_brightness_ = 1.0f;
			sky_cluster_size_scale_ = 1.0f;
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SkyStarDensity, 1.0)));
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SkyStarBrightness, 1.0)));
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SkyNebulaIntensity, 1.0)));
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SkyGridOpacity, 1.0)));
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SkyRotation, 0.0)));
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SkyHueShift, 0.0)));
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SkySaturation, 1.0)));
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SkyBackgroundR, 0.0)));
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SkyBackgroundG, 0.0)));
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SkyBackgroundB, 0.0)));
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SkyStarBrightnessVariation, 0.5)));
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SkyStarSizeVariation, 0.5)));
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SkyStarColorVariation, 1.0)));
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SkyStarTemperatureBias, 0.0)));
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SkyGalaxyDensity, 0.0)));
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SkyGalaxyBrightness, 1.0)));
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SkyGalaxySizeScale, 1.0)));
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SkyDustDensity, 0.0)));
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SkyDustIntensity, 1.0)));
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SkyDustScale, 1.0)));
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SkyClusterDensity, 0.0)));
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SkyClusterBrightness, 1.0)));
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SkyClusterSizeScale, 1.0)));
		}

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.5f, 0.75f, 1.0f, 1.0f), "Star Field Randomization:");
		if (has_stars) {
			if (ImGui::SliderFloat("Brightness Variation", &sky_star_brightness_variation_, 0.0f, 1.0f, "%.2f")) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SkyStarBrightnessVariation, static_cast<double>(sky_star_brightness_variation_))));
			}
			render_setting_tooltip("Controls how strongly individual star brightness deviates from the average. Higher values produce a sparser population of very bright stars against many faint ones.");
			if (ImGui::SliderFloat("Size Variation", &sky_star_size_variation_, 0.0f, 1.0f, "%.2f")) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SkyStarSizeVariation, static_cast<double>(sky_star_size_variation_))));
			}
			render_setting_tooltip("Controls the spread of apparent star disc sizes across the field.");
			if (ImGui::SliderFloat("Color Variation", &sky_star_color_variation_, 0.0f, 2.0f, "%.2f")) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SkyStarColorVariation, static_cast<double>(sky_star_color_variation_))));
			}
			render_setting_tooltip("Scales how far individual star tints drift from neutral white, from monochrome (0) to strongly saturated red/blue extremes (2).");
			if (ImGui::SliderFloat("Temperature Bias", &sky_star_temperature_bias_, -1.0f, 1.0f, "%.2f")) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SkyStarTemperatureBias, static_cast<double>(sky_star_temperature_bias_))));
			}
			render_setting_tooltip("Shifts the overall star population toward cooler red stars (negative) or hotter blue stars (positive).");
		}
		if (ImGui::InputInt("Procedural Seed", &sky_procedural_seed_)) {
			sky_procedural_seed_ = std::max(sky_procedural_seed_, 0);
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SkyProceduralSeed, static_cast<double>(sky_procedural_seed_))));
		}
		render_setting_tooltip("Seed controlling the procedural placement of every background element below, letting the entire deep-field layout be reshuffled deterministically.");

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.85f, 0.6f, 1.0f, 1.0f), "Deep Field Background Elements:");
		ImGui::TextDisabled("Procedurally scattered background objects layered on top of the skybox above, independent of the selected style.");

		if (ImGui::SliderFloat("Distant Galaxy Density", &sky_galaxy_density_, 0.0f, 4.0f, "%.2f")) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SkyGalaxyDensity, static_cast<double>(sky_galaxy_density_))));
		}
		render_setting_tooltip("Number of procedurally generated background micro-galaxies scattered across the sky. Zero disables them entirely.");
		if (sky_galaxy_density_ > 0.0f) {
			if (ImGui::SliderFloat("Galaxy Brightness", &sky_galaxy_brightness_, 0.0f, 4.0f, "%.2fx")) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SkyGalaxyBrightness, static_cast<double>(sky_galaxy_brightness_))));
			}
			if (ImGui::SliderFloat("Galaxy Size Scale", &sky_galaxy_size_scale_, 0.1f, 4.0f, "%.2fx")) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SkyGalaxySizeScale, static_cast<double>(sky_galaxy_size_scale_))));
			}
		}

		if (ImGui::SliderFloat("Dust Cloud Density", &sky_dust_density_, 0.0f, 4.0f, "%.2f")) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SkyDustDensity, static_cast<double>(sky_dust_density_))));
		}
		render_setting_tooltip("Coverage of procedurally generated interstellar dust and gas cloud wisps across the sky. Zero disables them entirely.");
		if (sky_dust_density_ > 0.0f) {
			if (ImGui::SliderFloat("Dust Cloud Intensity", &sky_dust_intensity_, 0.0f, 4.0f, "%.2fx")) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SkyDustIntensity, static_cast<double>(sky_dust_intensity_))));
			}
			if (ImGui::SliderFloat("Dust Cloud Scale", &sky_dust_scale_, 0.1f, 4.0f, "%.2fx")) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SkyDustScale, static_cast<double>(sky_dust_scale_))));
			}
		}

		if (ImGui::SliderFloat("Star Cluster Density", &sky_cluster_density_, 0.0f, 4.0f, "%.2f")) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SkyClusterDensity, static_cast<double>(sky_cluster_density_))));
		}
		render_setting_tooltip("Number of tight procedurally generated star clusters scattered across the sky, each composed of several close-set stars. Zero disables them entirely.");
		if (sky_cluster_density_ > 0.0f) {
			if (ImGui::SliderFloat("Cluster Brightness", &sky_cluster_brightness_, 0.0f, 4.0f, "%.2fx")) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SkyClusterBrightness, static_cast<double>(sky_cluster_brightness_))));
			}
			if (ImGui::SliderFloat("Cluster Size Scale", &sky_cluster_size_scale_, 0.1f, 4.0f, "%.2fx")) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SkyClusterSizeScale, static_cast<double>(sky_cluster_size_scale_))));
			}
		}
	}

	void render_sky_panorama_controls() noexcept {
		static constexpr std::array<const char*, 3> kPanoramaNames{"Night Sky HDRI 001 (ambientCG)", "Night Sky HDRI 008 (ambientCG)", "ESO 0932a (ESO Observatory Photograph)"};
		if (ImGui::Combo("Sky Panorama", &sky_panorama_id_, kPanoramaNames.data(), static_cast<int>(kPanoramaNames.size()))) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SkyPanoramaId, static_cast<double>(sky_panorama_id_))));
		}
		render_setting_tooltip("Selects which real captured sky panorama is projected as the equirectangular background instead of the procedural starfield.");

		const auto panorama_id = static_cast<Optics::SkyPanoramaId>(sky_panorama_id_);
		const auto& entry = Optics::sky_panorama_catalog_entry(panorama_id);

		if (entry.has_quality_variants) {
			const char* quality_names[] = {"1K (1024x512, fastest load)", "2K (2048x1024, balanced)", "4K (4096x2048, highest detail)"};
			if (ImGui::Combo("Panorama Quality", &sky_panorama_quality_, quality_names, IM_ARRAYSIZE(quality_names))) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SkyPanoramaQuality, static_cast<double>(sky_panorama_quality_))));
			}
			render_setting_tooltip("Chooses which resolution variant of the selected panorama is decoded and sampled. Higher resolutions cost more memory and a longer one-time decode when switching.");
		} else {
			ImGui::TextDisabled("This panorama is only available in its native captured resolution (6000x3000).");
		}

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.5f, 0.75f, 1.0f, 1.0f), "Panorama Color Adjustments:");
		if (ImGui::SliderFloat("Sky Rotation", &sky_rotation_, -180.0f, 180.0f, "%.1f deg")) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SkyRotation, static_cast<double>(sky_rotation_))));
		}
		if (ImGui::SliderFloat("Sky Hue Shift", &sky_hue_shift_, -180.0f, 180.0f, "%.1f deg")) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SkyHueShift, static_cast<double>(sky_hue_shift_))));
		}
		if (ImGui::SliderFloat("Sky Saturation", &sky_saturation_, 0.0f, 2.0f, "%.2fx")) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SkySaturation, static_cast<double>(sky_saturation_))));
		}
		if (ImGui::ColorEdit3("Background Tint", sky_background_)) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SkyBackgroundR, static_cast<double>(sky_background_[0]))));
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SkyBackgroundG, static_cast<double>(sky_background_[1]))));
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SkyBackgroundB, static_cast<double>(sky_background_[2]))));
		}

		ImGui::Separator();
		ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.35f, 1.0f), "Rendering Path:");
		render_wrapped_colored_text(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled), "Imported sky panoramas are decoded once on a background thread and cached, then sampled directly by the Vulkan compute shader when GPU offload is enabled. The render keeps showing the procedural sky with no stall while a new panorama decodes. The CPU path remains the fallback for unsupported metrics or precision modes. If a panorama file cannot be decoded, the procedural sky is used and the error is written to the engine log.");
	}

	void render_integrators_tab() noexcept {
		const char* integrators[] = {
			"Dormand-Prince RK45 (Adaptive)",
			"Cash-Karp 5(4) (Adaptive)",
			"Vernier 9(8) High-Order",
			"Symplectic Gauss-Legendre 4th",
			"Symplectic Gauss-Legendre 6th",
			"Hermite 4th-Order (Aarseth)"
		};

		if (ImGui::Combo("ODE Integrator", &integrator_selection_, integrators, IM_ARRAYSIZE(integrators))) {
			orchestrator_.set_active_integrator_name(integrators[integrator_selection_]);
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_integrator(integrators[integrator_selection_])));
		}
		{
			const auto warning = metric_integrator_incompatibility(orchestrator_.active_metric_name(), orchestrator_.active_integrator_name());
			if (!warning.empty()) {
				render_setting_tooltip_warning("Numerical differential solver scheme: adaptive Runge-Kutta Dormand-Prince, high-order Vernier 9(8), or symplectic Gauss-Legendre.", std::string(warning).c_str());
			} else {
				render_setting_tooltip("Numerical differential solver scheme: adaptive Runge-Kutta Dormand-Prince, high-order Vernier 9(8), or symplectic Gauss-Legendre.");
			}
		}

		ImGui::Separator();

		float rtol = static_cast<float>(orchestrator_.parameters().integration_rtol);
		if (slider_float_with_input("Relative Tolerance (rtol)", &rtol, 1e-16f, 1e-1f, "%.2e", &integrator_rtol_log_mode_, 1e-16f, 1e-1f)) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::IntegrationRtol, static_cast<double>(rtol))));
		}
		ImGui::SameLine();
		if (ImGui::SmallButton("rtol /10")) {
			rtol = std::max(rtol * 0.1f, 1e-16f);
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::IntegrationRtol, static_cast<double>(rtol))));
		}
		ImGui::SameLine();
		if (ImGui::SmallButton("rtol x10")) {
			rtol = std::min(rtol * 10.0f, 1e-1f);
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::IntegrationRtol, static_cast<double>(rtol))));
		}
		render_setting_tooltip("Local relative error tolerance threshold controlling adaptive step-size regulation. The /10 and x10 buttons jump by a full order of magnitude.");

		float atol = static_cast<float>(orchestrator_.parameters().integration_atol);
		if (slider_float_with_input("Absolute Tolerance (atol)", &atol, 1e-20f, 1e-4f, "%.2e", &integrator_atol_log_mode_, 1e-20f, 1e-4f)) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::IntegrationAtol, static_cast<double>(atol))));
		}
		ImGui::SameLine();
		if (ImGui::SmallButton("atol /10")) {
			atol = std::max(atol * 0.1f, 1e-20f);
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::IntegrationAtol, static_cast<double>(atol))));
		}
		ImGui::SameLine();
		if (ImGui::SmallButton("atol x10")) {
			atol = std::min(atol * 10.0f, 1e-4f);
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::IntegrationAtol, static_cast<double>(atol))));
		}
		render_setting_tooltip("Absolute error tolerance floor preventing step-size collapse near null-coordinate vanishing states. The /10 and x10 buttons jump by a full order of magnitude.");
	}

	void render_rocket_tab() noexcept {
		const char* flows[] = {"Proper Time Comobile (tau)", "Coordinate Time (t)"};
		if (ImGui::Combo("Clock Flow", &timeflow_mode_, flows, IM_ARRAYSIZE(flows))) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::TimeFlowMode, static_cast<double>(timeflow_mode_))));
		}
		render_setting_tooltip("Select whether simulation time advances along observer comobile proper time (tau) or asymptotic coordinate time (t).");

		ImGui::Separator();
		ImGui::SliderFloat("Main Throttle", &rocket_throttle_, 0.0f, 1.0f, "%.2f");
		render_setting_tooltip("Throttle percentage regulating main forward relativistic engine thrust.");
		ImGui::SliderFloat("Thrust X (Longitudinal)", &rocket_thrust_x_, -100.0f, 100.0f, "%.1f m/s^2");
		render_setting_tooltip("Proper thrust component directed along the vehicle forward longitudinal tetrad axis.");
		ImGui::SliderFloat("Thrust Y (Lateral)", &rocket_thrust_y_, -50.0f, 50.0f, "%.1f m/s^2");
		render_setting_tooltip("Proper thrust component directed along the vehicle horizontal lateral tetrad axis.");
		ImGui::SliderFloat("Thrust Z (Normal)", &rocket_thrust_z_, -50.0f, 50.0f, "%.1f m/s^2");
		render_setting_tooltip("Proper thrust component directed along the vehicle vertical normal tetrad axis.");
	}

	void render_execution_tab() noexcept {
		const auto snap = orchestrator_.scheduler().snapshot();
		const auto& params = orchestrator_.parameters();
		const bool schematic_locked = params.schematic_mode_enabled && !params.schematic_allow_simulation;

		ImGui::Text("Simulation Cycle: #%llu", static_cast<unsigned long long>(snap.tick_index));
		ImGui::Text("Logical Time:     %.4f s", snap.logical_time);

		if (schematic_locked) {
			ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.35f, 1.0f), "Simulation clock is frozen: Schematic Orbital View is active and 'Allow Simulation Clock To Run In Schematic View' is disabled.");
		}

		float warp = static_cast<float>(snap.warp_factor);
		if (slider_float_with_input("Warp Factor", &warp, 0.01f, 100.0f, "%.4f", &warp_log_mode_, 1e-6f, 1e9f)) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_warp(static_cast<double>(warp))));
		}
		render_setting_tooltip("Temporal acceleration multiplier applied to the logical simulation clock, spanning extreme slow motion to extreme fast forward. Enable Log for precise control across the full range.");
		ImGui::Spacing();
		if (ImGui::SmallButton("Warp x10")) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_warp(std::min(static_cast<double>(warp) * 10.0, 1e9))));
		}
		ImGui::SameLine(0.0f, 14.0f);
		if (ImGui::SmallButton("Warp /10")) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_warp(std::max(static_cast<double>(warp) * 0.1, 1e-6))));
		}
		ImGui::SameLine(0.0f, 14.0f);
		if (ImGui::SmallButton("Warp Reset (1x)")) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_warp(1.0)));
		}

		float rate = static_cast<float>(snap.tick_rate_hz);
		if (slider_float_with_input("Scheduler Rate", &rate, 10.0f, 1000.0f, "%.0f")) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_tickrate(static_cast<double>(rate))));
		}
		render_setting_tooltip("Fixed logical simulation clock frequency decoupled from display frame rates (10 Hz to 1000 Hz).");

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.7f, 1.0f), "Adaptive Render Performance:");
		bool dynamic_res = orchestrator_.parameters().dynamic_resolution_enabled;
		if (ImGui::Checkbox("Dynamic Resolution Scaling", &dynamic_res)) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::DynamicResolutionEnabled, dynamic_res ? 1.0 : 0.0)));
		}
		render_setting_tooltip("Automatically shrinks and grows the internal render resolution frame by frame to hold the target frame rate below, multiplying on top of the manual resolution scale above. Disabled by default so the currently tuned performance defaults stay untouched.");
		if (dynamic_res) {
			float target_fps = static_cast<float>(orchestrator_.parameters().dynamic_resolution_target_fps);
			if (ImGui::SliderFloat("Target Frame Rate", &target_fps, 15.0f, 240.0f, "%.0f fps")) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::DynamicResolutionTargetFps, static_cast<double>(target_fps))));
			}
		}
		bool tile_prepass = orchestrator_.parameters().adaptive_tile_prepass_enabled;
		if (ImGui::Checkbox("Adaptive Tile Sky Prepass", &tile_prepass)) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::AdaptiveTilePrepassEnabled, tile_prepass ? 1.0 : 0.0)));
		}
		render_setting_tooltip("Tests only the four corner rays of every 32x32 CPU render tile; when the whole tile is confirmed to be untouched empty sky far from the black hole, it is filled from a single analytic sky sample instead of fully integrating every one of its pixels. Only affects the CPU fallback path. Disabled by default.");

		ImGui::Separator();

		if (schematic_locked) ImGui::BeginDisabled(true);

		if (snap.is_paused) {
			if (ImGui::Button("Resume (F5)", ImVec2(110.0f, 28.0f))) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_resume()));
			}
		} else {
			if (ImGui::Button("Pause (F5)", ImVec2(110.0f, 28.0f))) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_pause()));
			}
		}

		ImGui::SameLine();
		if (ImGui::Button("Step 1 Tick (F6)", ImVec2(120.0f, 28.0f))) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_step(1)));
		}

		ImGui::SameLine();
		if (ImGui::Button("Reset Clock", ImVec2(110.0f, 28.0f))) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_reset()));
		}

		if (schematic_locked) {
			ImGui::EndDisabled();
			render_setting_tooltip_warning("Simulation clock controls.", "Disabled while Schematic Orbital View is active. Enable 'Allow Simulation Clock To Run In Schematic View' in the Schematic View tab to unlock.");
		}
	}

	void render_schematic_object_style(const char* label, SchematicObjectDisplayConfig& style) noexcept {
		ImGui::PushID(label);
		if (ImGui::CollapsingHeader(label, ImGuiTreeNodeFlags_DefaultOpen)) {
			const char* shapes[] = {"Point Marker", "Sphere (Fixed Radius)", "Sphere (By Parameter)"};
			int shape_idx = static_cast<int>(style.shape);
			if (ImGui::Combo("Render Shape", &shape_idx, shapes, IM_ARRAYSIZE(shapes))) {
				style.shape = static_cast<SchematicObjectShape>(shape_idx);
			}

			if (style.shape == SchematicObjectShape::Point) {
				float pr = static_cast<float>(style.point_pixel_radius);
				if (ImGui::SliderFloat("Marker Pixel Radius", &pr, 1.0f, 20.0f, "%.1f")) style.point_pixel_radius = pr;
			} else {
				const char* sphere_styles[] = {"Opaque", "Translucent", "Wireframe Cage", "Realistic Shaded", "Gradient Fill"};
				int sphere_idx = static_cast<int>(style.sphere_style);
				if (ImGui::Combo("Sphere Style", &sphere_idx, sphere_styles, IM_ARRAYSIZE(sphere_styles))) {
					style.sphere_style = static_cast<SchematicSphereStyle>(sphere_idx);
				}
				if (style.sphere_style == SchematicSphereStyle::Translucent) {
					float alpha = static_cast<float>(style.translucency_alpha);
					if (ImGui::SliderFloat("Translucency Alpha", &alpha, 0.05f, 0.95f, "%.2f")) style.translucency_alpha = alpha;
				}
				if (style.sphere_style == SchematicSphereStyle::Wireframe) {
					int rings = style.wireframe_rings;
					if (ImGui::SliderInt("Wireframe Rings", &rings, 2, 16)) style.wireframe_rings = rings;
					int segs = style.wireframe_segments;
					if (ImGui::SliderInt("Wireframe Segments", &segs, 8, 64)) style.wireframe_segments = segs;
				}
				if (style.sphere_style == SchematicSphereStyle::RealisticShaded) {
					if (ImGui::TreeNode("Shading Pipeline Configuration")) {
						ImGui::SliderFloat("Ambient Strength", &style.shading.ambient_strength, 0.0f, 1.0f, "%.2f");
						ImGui::SliderFloat("Diffuse Strength", &style.shading.diffuse_strength, 0.0f, 2.0f, "%.2f");
						ImGui::SliderFloat("Specular Strength", &style.shading.specular_strength, 0.0f, 2.0f, "%.2f");
						ImGui::SliderFloat("Specular Shininess", &style.shading.specular_shininess, 1.0f, 128.0f, "%.1f");
						ImGui::SliderFloat("Limb Darkening Power", &style.shading.limb_darkening_power, 0.0f, 1.0f, "%.2f");
						ImGui::InputFloat3("Light Direction Vector", style.shading.light_direction.data());
						ImGui::Checkbox("Use Secondary Color as Shadow Tint", &style.shading.use_secondary_color_as_shadow);
						ImGui::TreePop();
					}
				}
				if (style.sphere_style == SchematicSphereStyle::GradientFill) {
					if (ImGui::TreeNode("Gradient Configuration")) {
						ImGui::Checkbox("Radial Gradient", &style.gradient_radial);
						if (!style.gradient_radial) {
							ImGui::SliderFloat("Gradient Angle", &style.gradient_angle_deg, 0.0f, 360.0f, "%.1f deg");
						}
						for (size_t s = 0; s < style.gradient_stops.size(); ++s) {
							ImGui::PushID(static_cast<int>(s));
							std::string stop_label = "Stop #" + std::to_string(s + 1) + " Pos";
							ImGui::SliderFloat(stop_label.c_str(), &style.gradient_stops[s].position, 0.0f, 1.0f, "%.2f");
							std::string col_label = "Stop #" + std::to_string(s + 1) + " Color";
							ImGui::ColorEdit4(col_label.c_str(), style.gradient_stops[s].color.data());
							ImGui::PopID();
						}
						ImGui::TreePop();
					}
				}

				if (style.shape == SchematicObjectShape::SphereFixedRadius) {
					float rscale = static_cast<float>(style.radius_scale);
					if (ImGui::SliderFloat("Physical Radius Scale", &rscale, 0.1f, 10.0f, "%.2fx")) style.radius_scale = rscale;
				} else {
					const char* sources[] = {"Mass", "Speed", "Kinetic Energy", "Spin Magnitude", "Physical Radius"};
					int src_idx = static_cast<int>(style.parameter_source);
					if (ImGui::Combo("Radius Parameter Source", &src_idx, sources, IM_ARRAYSIZE(sources))) {
						style.parameter_source = static_cast<SchematicSphereParameterSource>(src_idx);
					}
					float pscale = static_cast<float>(style.parameter_pixel_scale);
					if (ImGui::SliderFloat("Parameter Pixel Scale", &pscale, 0.001f, 50.0f, "%.4f", ImGuiSliderFlags_Logarithmic)) style.parameter_pixel_scale = pscale;
				}

				float min_px = static_cast<float>(style.sphere_min_pixel_radius);
				if (ImGui::SliderFloat("Min Pixel Radius", &min_px, 1.0f, 50.0f, "%.1f")) style.sphere_min_pixel_radius = min_px;
				float max_px = static_cast<float>(style.sphere_max_pixel_radius);
				if (ImGui::SliderFloat("Max Pixel Radius", &max_px, 10.0f, 400.0f, "%.1f")) style.sphere_max_pixel_radius = max_px;
			}

			if (ImGui::TreeNode("Body Outline & Glow Style")) {
				ImGui::Checkbox("Enable Perimeter Outline", &style.outline.enabled);
				if (style.outline.enabled) {
					ImGui::ColorEdit4("Outline Color", style.outline.color.data());
					ImGui::SliderFloat("Outline Thickness", &style.outline.thickness, 0.5f, 10.0f, "%.1f px");
					ImGui::Checkbox("Enable Outer Edge Glow", &style.outline.glow_enabled);
					if (style.outline.glow_enabled) {
						ImGui::SliderFloat("Glow Expansion Radius", &style.outline.glow_radius, 1.0f, 30.0f, "%.1f px");
						ImGui::ColorEdit4("Glow Color", style.outline.glow_color.data());
						ImGui::SliderFloat("Glow Alpha Multiplier", &style.outline.glow_alpha, 0.05f, 1.0f, "%.2f");
					}
				}
				ImGui::TreePop();
			}

			if (ImGui::TreeNode("Body Outer Ambient Halo")) {
				ImGui::SliderFloat("Halo Strength", &style.halo_strength, 0.0f, 2.0f, "%.2f");
				if (style.halo_strength > 0.0f) {
					ImGui::ColorEdit4("Halo Color", style.halo_color.data());
					ImGui::SliderFloat("Halo Radius Multiplier", &style.halo_radius_factor, 1.1f, 5.0f, "%.2fx");
				}
				ImGui::TreePop();
			}

			const char* color_modes[] = {"Body Color", "By Mass", "By Speed", "By Spin Magnitude", "By Distance From Center", "By Kinetic Energy", "Physical: Temperature", "Physical: Charge", "Physical: Density", "Physical: Intelligent Composite"};
			int color_idx = static_cast<int>(style.color_mode);
			if (ImGui::Combo("Color Coding", &color_idx, color_modes, IM_ARRAYSIZE(color_modes))) {
				style.color_mode = static_cast<SchematicColorCodingMode>(color_idx);
			}
			ImGui::ColorEdit4("Base / Fallback Color", style.uniform_color.data());

			if (style.color_mode == SchematicColorCodingMode::ByPhysicalIntelligent) {
				if (ImGui::TreeNode("Physical Intelligence Color Mode Info")) {
					ImGui::TextWrapped("PhysColorize engine calculates body surface color dynamically from physical attributes:");
					ImGui::BulletText("Temperature: Blackbody radiation lookup (Wien's law from 800K to 40,000K).");
					ImGui::BulletText("Composition: Shift based on material type ('H'=cyan, 'C'=reddish, 'R'=ochre, 'M'=silver, 'N'=violet).");
					ImGui::BulletText("Charge: Positive charge shifts to warm gold; negative charge shifts to cool violet.");
					ImGui::BulletText("Spin: Relativistic polarization blue boost proportional to angular momentum.");
					ImGui::BulletText("Density: Scales perceptual brightness (high density = high luminosity).");
					ImGui::BulletText("Gravitational Redshift: Compactness ratio (2M/r) shifts spectrum towards red.");
					ImGui::TreePop();
				}
			}

			ImGui::Separator();
			ImGui::Checkbox("Show Tag", &style.show_tag);
			if (style.show_tag) {
				ImGui::Checkbox("Tag: Show ID", &style.show_id_in_tag);
				ImGui::SameLine();
				ImGui::Checkbox("Tag: Show Mass", &style.show_mass_in_tag);
				ImGui::SameLine();
				ImGui::Checkbox("Tag: Show Speed", &style.show_speed_in_tag);
			}
		}
		ImGui::PopID();
	}

	void render_schematic_vector_style(const char* label, SchematicVectorStyle& style) noexcept {
		ImGui::PushID(label);
		if (ImGui::CollapsingHeader(label)) {
			ImGui::Checkbox("Enabled", &style.enabled);
			if (style.enabled) {
				const char* placements[] = {"At Object Center", "At Object Surface"};
				int p_idx = static_cast<int>(style.placement);
				if (ImGui::Combo("Placement", &p_idx, placements, IM_ARRAYSIZE(placements))) {
					style.placement = static_cast<SchematicVectorPlacement>(p_idx);
				}
				const char* orient_modes[] = {"From Physical Quantity", "Fixed World Axis"};
				int o_idx = static_cast<int>(style.orientation_mode);
				if (ImGui::Combo("Orientation Source", &o_idx, orient_modes, IM_ARRAYSIZE(orient_modes))) {
					style.orientation_mode = static_cast<SchematicVectorOrientationMode>(o_idx);
				}
				if (style.orientation_mode == SchematicVectorOrientationMode::FixedWorldAxis) {
					float dir[3] = {static_cast<float>(style.fixed_direction[0]), static_cast<float>(style.fixed_direction[1]), static_cast<float>(style.fixed_direction[2])};
					if (ImGui::InputFloat3("Fixed Direction", dir)) {
						style.fixed_direction = {dir[0], dir[1], dir[2]};
					}
				}
				float length_scale = static_cast<float>(style.length_scale);
				if (ImGui::SliderFloat("Length Scale", &length_scale, 0.001f, 20.0f, "%.4f", ImGuiSliderFlags_Logarithmic)) style.length_scale = length_scale;
				float min_len = static_cast<float>(style.min_pixel_length);
				if (ImGui::SliderFloat("Min Pixel Length", &min_len, 0.0f, 100.0f, "%.1f")) style.min_pixel_length = min_len;
				float max_len = static_cast<float>(style.max_pixel_length);
				if (ImGui::SliderFloat("Max Pixel Length", &max_len, 10.0f, 400.0f, "%.1f")) style.max_pixel_length = max_len;
				float head_size = static_cast<float>(style.head_size_px);
				if (ImGui::SliderFloat("Arrowhead Size", &head_size, 2.0f, 24.0f, "%.1f")) style.head_size_px = head_size;
				float thickness = static_cast<float>(style.line_thickness_px);
				if (ImGui::SliderFloat("Line Thickness", &thickness, 0.5f, 8.0f, "%.1f")) style.line_thickness_px = thickness;
				ImGui::Checkbox("Automatic Color (Match Object Coding)", &style.use_automatic_color);
				if (!style.use_automatic_color) {
					ImGui::ColorEdit4("Manual Color", style.manual_color.data());
				}
			}
		}
		ImGui::PopID();
	}

	void render_body_3d_render_tab() noexcept {
		auto& params = orchestrator_.parameters();
		bool enable_3d = (params.visual_overlays_flags & Render::RenderFlags::ENABLE_3D_BODY_RAYTRACING) != 0U;
		if (ImGui::Checkbox("Enable Realistic 3D Celestial Body Ray-Tracing Pipeline", &enable_3d)) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_visual_overlay(Render::RenderFlags::ENABLE_3D_BODY_RAYTRACING, enable_3d)));
		}
		render_setting_tooltip("Toggles the 3D ray-traced rendering pipeline for all active celestial bodies. When enabled, photons intersect 3D oblate spheroid surface geometry and procedural shaders in real-time. Disabling this hides every 3D body regardless of the catalog contents.");

		bool bodies_only = params.bodies_only_render_mode;
		if (ImGui::Checkbox("Render Bodies Only (Skip Black Hole & Lensing)", &bodies_only)) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::BodiesOnlyRenderMode, bodies_only ? 1.0 : 0.0)));
		}
		render_setting_tooltip("Skips gravitational lensing, the event horizon, and the accretion disk for the current frame, keeping only the celestial bodies and the sky. Requires the pipeline checkbox above to be enabled to have any visible effect. Useful for a fast, low-latency preview while editing a body catalog.");

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.6f, 1.0f), "Relativistic Surface Effects:");

		bool doppler = (params.visual_overlays_flags & Render::RenderFlags::ENABLE_BODY_DOPPLER_BEAMING) != 0U;
		if (ImGui::Checkbox("Doppler Beaming & Relativistic Aberration", &doppler)) {
			if (doppler) params.visual_overlays_flags |= Render::RenderFlags::ENABLE_BODY_DOPPLER_BEAMING;
			else params.visual_overlays_flags &= ~Render::RenderFlags::ENABLE_BODY_DOPPLER_BEAMING;
		}
		render_setting_tooltip("Applies relativistic Doppler boosting (g^3) and searchlight beaming to body surface radiance based on body orbital velocity.");

		bool redshift = (params.visual_overlays_flags & Render::RenderFlags::ENABLE_BODY_GRAV_REDSHIFT) != 0U;
		if (ImGui::Checkbox("Gravitational Redshift Surface Attenuation", &redshift)) {
			if (redshift) params.visual_overlays_flags |= Render::RenderFlags::ENABLE_BODY_GRAV_REDSHIFT;
			else params.visual_overlays_flags &= ~Render::RenderFlags::ENABLE_BODY_GRAV_REDSHIFT;
		}
		render_setting_tooltip("Attenuates surface radiance according to metric gravitational time dilation near compact gravitating objects.");

		bool atmos = (params.visual_overlays_flags & Render::RenderFlags::ENABLE_ATMOSPHERE_SCATTERING) != 0U;
		if (ImGui::Checkbox("Atmospheric Rayleigh Rim Scattering", &atmos)) {
			if (atmos) params.visual_overlays_flags |= Render::RenderFlags::ENABLE_ATMOSPHERE_SCATTERING;
			else params.visual_overlays_flags &= ~Render::RenderFlags::ENABLE_ATMOSPHERE_SCATTERING;
		}
		render_setting_tooltip("Enables analytical Rayleigh limb shell rim scattering around planetary atmospheres.");

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.6f, 0.85f, 1.0f, 1.0f), "Performance & Level Of Detail:");

		int lod_threshold = static_cast<int>(params.body_render_lod_pixel_threshold);
		if (slider_int_with_input("LOD Pixel Threshold", &lod_threshold, 2, 64)) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::BodyRenderLodPixelThreshold, static_cast<double>(lod_threshold))));
		}
		render_setting_tooltip("Bodies whose apparent on-screen radius falls below this many pixels render with simplified flat shading and no procedural noise or atmosphere, keeping distant or small bodies cheap to draw.");

		int point_lod_threshold = static_cast<int>(params.body_render_point_pixel_threshold);
		if (slider_int_with_input("Point LOD Threshold (px)", &point_lod_threshold, 1, 16)) {
			point_lod_threshold = std::min(point_lod_threshold, lod_threshold);
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::BodyRenderPointPixelThreshold, static_cast<double>(point_lod_threshold))));
		}
		render_setting_tooltip("Bodies whose apparent on-screen radius falls below this even smaller pixel threshold skip lighting and shading entirely and render as a flat lit dot, the cheapest possible representation for distant or numerous small bodies such as asteroid fields. Always kept at or below the LOD Pixel Threshold above.");

		int noise_octaves = static_cast<int>(params.body_noise_octaves);
		if (slider_int_with_input("Surface Noise Octaves", &noise_octaves, 1, 6)) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::BodyNoiseOctaves, static_cast<double>(noise_octaves))));
		}
		render_setting_tooltip("Number of fractal noise layers combined to generate a body's procedural surface detail. Higher values add finer detail at a proportional rendering cost; lower values are considerably cheaper for busy scenes with many bodies.");

		bool low_power = params.body_render_low_power_mode;
		if (ImGui::Checkbox("Low-Power Mode (Force Simple Shading On All Bodies)", &low_power)) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::BodyRenderLowPowerMode, low_power ? 1.0 : 0.0)));
		}
		render_setting_tooltip("Forces every celestial body to render with flat Lambert shading regardless of apparent size, for maximum performance on dense body catalogs or low-end hardware.");

		bool shadows = params.body_shadows_enabled;
		if (ImGui::Checkbox("Central-Source Directional Shading (Day/Night Terminator)", &shadows)) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::BodyShadowsEnabled, shadows ? 1.0 : 0.0)));
		}
		render_setting_tooltip("Lights each body's surface from the direction of the central spacetime source instead of the camera, producing a genuine day/night terminator that rotates with orbital position instead of always facing the viewer. This does not make bodies occlude the accretion disk; see the option below for that.");

		bool disk_occlusion = params.body_disk_occlusion_enabled;
		if (ImGui::Checkbox("Bodies Occlude The Accretion Disk", &disk_occlusion)) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::BodyDiskOcclusionEnabled, disk_occlusion ? 1.0 : 0.0)));
		}
		render_setting_tooltip("When a celestial body sits between the observer and a point of the accretion disk along a given ray, its surface takes priority over the disk's radiative contribution at that point, instead of the disk always being visible through the body.");

		float atmo_intensity = static_cast<float>(params.body_atmosphere_global_intensity);
		if (slider_float_with_input("Global Atmosphere Intensity", &atmo_intensity, 0.0f, 3.0f, "%.2fx")) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::BodyAtmosphereGlobalIntensity, static_cast<double>(atmo_intensity))));
		}
		render_setting_tooltip("Global multiplier applied on top of every body's individual atmosphere thickness, letting the overall rim-scattering strength be tuned or disabled without editing each body.");

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.5f, 0.85f, 1.0f, 1.0f), "Live Body Rendering Diagnostics:");
		if (render_pipeline_ != nullptr) {
			const auto& tel = render_pipeline_->telemetry();
			ImGui::Text("Bodies Sent This Frame: %u / %u (after culling)", tel.bodies_sent_this_frame, tel.bodies_total_enabled_this_frame);
			render_setting_tooltip("Number of bodies actually forwarded to the renderer this frame after field-of-view and render-distance culling, compared to the total number of enabled bodies in the catalog.");
			ImGui::Text("Body Tiles This Frame: %llu / %llu (%.1f%%)", static_cast<unsigned long long>(tel.body_tile_count), static_cast<unsigned long long>(tel.body_tile_total_count), tel.body_tile_total_count > 0 ? (100.0 * static_cast<double>(tel.body_tile_count) / static_cast<double>(tel.body_tile_total_count)) : 0.0);
			render_setting_tooltip("Fraction of 32x32 screen tiles that had to be tested or shaded for celestial bodies during the most recently completed frame.");
			ImGui::Text("Bodies Rendered On GPU: %s", tel.bodies_rendered_on_gpu ? "Yes" : "No");
			render_setting_tooltip("Whether the celestial bodies visible this frame were intersected and shaded directly by the Vulkan compute shader instead of the CPU fallback path.");
		} else {
			size_t enabled_body_count = 0;
			for (const auto& b : orchestrator_.nbody_system().bodies()) {
				if (b.enabled) ++enabled_body_count;
			}
			ImGui::Text("Active Bodies In Catalog: %zu", enabled_body_count);
			render_setting_tooltip("Total number of enabled bodies currently in the N-Body catalog. Bodies outside the camera's field of view or beyond the render distance are culled before being sent to the renderer each frame.");
		}
		ImGui::Text("3D Ray-Tracing Pipeline: %s", enable_3d ? "Enabled" : "Disabled");
		ImGui::Text("Bodies-Only Mode: %s", bodies_only ? "Enabled" : "Disabled");
	}

	void render_schematic_tab() noexcept {
		ImGui::TextColored(ImVec4(0.3f, 0.9f, 1.0f, 1.0f), "Schematic Orbital View Configuration");
		ImGui::TextDisabled("Controls the simplified non-lensed projection view used when Schematic Mode is active.");
		ImGui::Separator();

		bool schematic_mode = orchestrator_.parameters().schematic_mode_enabled;
		if (ImGui::Checkbox("Enable Schematic Orbital View (No Lensing)", &schematic_mode)) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SchematicModeEnabled, schematic_mode ? 1.0 : 0.0)));
		}
		render_setting_tooltip("Replaces gravitational ray tracing with a simplified projection showing every body as a plain sphere against a coordinate grid backdrop, with orientation arrows for spin axes.");

		bool allow_sim_in_schematic = orchestrator_.parameters().schematic_allow_simulation;
		if (ImGui::Checkbox("Allow Simulation Clock To Run In Schematic View", &allow_sim_in_schematic)) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SchematicAllowSimulation, allow_sim_in_schematic ? 1.0 : 0.0)));
		}
		render_setting_tooltip("Disabled by default. When left unchecked, entering Schematic View freezes the simulation clock and disables Play, Step, and Reset controls, since the simplified projection is intended as a static structural overview rather than a live playback mode. Enable this to keep bodies orbiting and the clock advancing while viewing the schematic projection.");

		ImGui::Separator();
		const char* schematic_projections[] = {"Human Perspective", "Auto-Zoom", "Fisheye Stereographic", "Equirectangular 360", "Fisheye Equidistant", "Fisheye Orthographic", "Panini Cylindrical", "Hammer-Aitoff"};
		int schematic_projection = static_cast<int>(schematic_cfg_.projection_mode);
		if (ImGui::Combo("Schematic Projection", &schematic_projection, schematic_projections, IM_ARRAYSIZE(schematic_projections))) {
			schematic_cfg_.projection_mode = static_cast<Observer::ProjectionMode>(schematic_projection);
		}
		ImGui::Checkbox("Human Perspective Schematic Rendering", &schematic_cfg_.human_perspective_mode);
		render_setting_tooltip("Uses an ordinary 3D pinhole view for the schematic scene while retaining bodies, gravity vectors, trails, and predictions. The normal ray-traced viewport keeps its separate projection choice.");
		ImGui::Checkbox("Show Body & Orbit Overlays In Raytraced View", &schematic_cfg_.show_overlay_in_raytraced_view);
		render_setting_tooltip("When enabled, projects orbiting bodies, trails, tags, and vectors on top of the raytraced 3D viewport so you can see them orbiting the black hole without switching to Schematic View.");
		ImGui::Checkbox("Apply Lens Approximation To Normal-View Body Overlays", &schematic_cfg_.lens_body_overlays_in_raytraced_view);

		ImGui::Separator();
		ImGui::Checkbox("Show Central Object", &schematic_cfg_.show_central_object);
		ImGui::SameLine();
		ImGui::Checkbox("Show N-Body Bodies", &schematic_cfg_.show_bodies);
		ImGui::Checkbox("Show Background Grid", &schematic_cfg_.show_background_grid);
		ImGui::SameLine();
		ImGui::Checkbox("Show Field Lines", &schematic_cfg_.show_field_lines);
		ImGui::Checkbox("Show Trajectory Trails", &schematic_cfg_.show_trails);
		ImGui::SameLine();
		ImGui::Checkbox("Show Orbit Predictions", &schematic_cfg_.show_orbit_predictions);
		ImGui::Checkbox("Show Vectors", &schematic_cfg_.show_vectors);
		ImGui::SameLine();
		ImGui::Checkbox("Show Object Tags", &schematic_cfg_.show_tags);

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Off-Screen Indicators:");
		auto& offscreen = schematic_cfg_.offscreen_indicator;
		ImGui::Checkbox("Enable Off-Screen Indicators", &offscreen.enabled);
		if (offscreen.enabled) {
			const char* shapes[] = {"Triangle", "Chevron", "Diamond", "Dot"};
			int shape_idx = static_cast<int>(offscreen.shape);
			if (ImGui::Combo("Indicator Shape", &shape_idx, shapes, IM_ARRAYSIZE(shapes))) offscreen.shape = static_cast<OffscreenIndicatorShape>(shape_idx);
			const char* color_sources[] = {"Fixed Color", "By Mass", "By Distance From Center", "By Speed", "By Temperature"};
			int color_idx = static_cast<int>(offscreen.color_source);
			if (ImGui::Combo("Color Source", &color_idx, color_sources, IM_ARRAYSIZE(color_sources))) offscreen.color_source = static_cast<OffscreenIndicatorColorSource>(color_idx);
			if (offscreen.color_source == OffscreenIndicatorColorSource::Fixed) {
				ImGui::ColorEdit4("Fixed Indicator Color", offscreen.fixed_color.data());
			}
			float base_size = static_cast<float>(offscreen.base_size_px);
			if (slider_float_with_input("Base Size (px)", &base_size, 2.0f, 40.0f, "%.1f")) offscreen.base_size_px = base_size;
			ImGui::Checkbox("Scale Size With Distance", &offscreen.scale_with_distance);
			if (offscreen.scale_with_distance) {
				float min_size = static_cast<float>(offscreen.min_size_px);
				if (slider_float_with_input("Min Size (px)", &min_size, 1.0f, 40.0f, "%.1f")) offscreen.min_size_px = min_size;
				float max_size = static_cast<float>(offscreen.max_size_px);
				if (slider_float_with_input("Max Size (px)", &max_size, 1.0f, 80.0f, "%.1f")) offscreen.max_size_px = max_size;
			}
			ImGui::Checkbox("Fade Opacity With Distance", &offscreen.fade_with_distance);
			if (offscreen.fade_with_distance) {
				float fade_ref = static_cast<float>(offscreen.fade_reference_distance);
				if (slider_float_with_input("Fade Reference Distance", &fade_ref, 1.0f, 2000.0f, "%.1f", nullptr, 1.0f, 2000.0f)) offscreen.fade_reference_distance = fade_ref;
			}
			ImGui::Checkbox("Show Label", &offscreen.show_label);
			if (offscreen.show_label) {
				ImGui::SameLine();
				ImGui::Checkbox("Include Distance In Label", &offscreen.show_distance_in_label);
			}
			float edge_margin = static_cast<float>(offscreen.edge_margin_px);
			if (slider_float_with_input("Screen Edge Margin (px)", &edge_margin, 5.0f, 150.0f, "%.1f")) offscreen.edge_margin_px = edge_margin;
		}

		if (schematic_cfg_.show_background_grid) {
			ImGui::Separator();
			ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Background Grid:");
			float grid_opacity = static_cast<float>(schematic_cfg_.grid_opacity);
			if (ImGui::SliderFloat("Grid Opacity", &grid_opacity, 0.0f, 1.0f, "%.2f")) schematic_cfg_.grid_opacity = grid_opacity;
			int lat = schematic_cfg_.grid_latitude_lines;
			if (ImGui::SliderInt("Latitude Lines", &lat, 1, 24)) schematic_cfg_.grid_latitude_lines = lat;
			int lon = schematic_cfg_.grid_longitude_lines;
			if (ImGui::SliderInt("Longitude Lines", &lon, 1, 36)) schematic_cfg_.grid_longitude_lines = lon;
			int segs = schematic_cfg_.grid_segments;
			if (ImGui::SliderInt("Grid Segments", &segs, 8, 128)) schematic_cfg_.grid_segments = segs;
			float grid_scale = static_cast<float>(schematic_cfg_.grid_radius_scale);
			if (ImGui::SliderFloat("Grid Radius Scale", &grid_scale, 2.0f, 200.0f, "%.1fx")) schematic_cfg_.grid_radius_scale = grid_scale;
		}

		if (schematic_cfg_.show_field_lines) {
			ImGui::Separator();
			ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Field Lines:");
			int fl_count = schematic_cfg_.field_line_count;
			if (ImGui::SliderInt("Field Line Count", &fl_count, 4, 200)) schematic_cfg_.field_line_count = fl_count;
			float fl_opacity = static_cast<float>(schematic_cfg_.field_line_opacity);
			if (ImGui::SliderFloat("Field Line Opacity", &fl_opacity, 0.0f, 1.0f, "%.2f")) schematic_cfg_.field_line_opacity = fl_opacity;
			float fl_extent = static_cast<float>(schematic_cfg_.field_line_extent_scale);
			if (ImGui::SliderFloat("Field Line Extent Scale", &fl_extent, 2.0f, 200.0f, "%.1fx")) schematic_cfg_.field_line_extent_scale = fl_extent;
			ImGui::Checkbox("Inward Direction Arrows", &schematic_cfg_.field_line_inward_arrows);
		}

		if (schematic_cfg_.show_trails) {
			ImGui::Separator();
			ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Trajectory Trails:");
			float duration = static_cast<float>(schematic_cfg_.trail_duration_seconds);
			if (ImGui::SliderFloat("Trail Duration (s)", &duration, 1.0f, 120.0f, "%.1f")) schematic_cfg_.trail_duration_seconds = duration;
			int max_pts = schematic_cfg_.trail_max_points;
			if (ImGui::SliderInt("Max Trail Points", &max_pts, 16, 2000)) schematic_cfg_.trail_max_points = max_pts;
			float interval = static_cast<float>(schematic_cfg_.trail_sample_interval_seconds);
			if (ImGui::SliderFloat("Sample Interval (s)", &interval, 0.01f, 2.0f, "%.3f")) schematic_cfg_.trail_sample_interval_seconds = interval;
			float fade_power = static_cast<float>(schematic_cfg_.trail_fade_power);
			if (ImGui::SliderFloat("Fade Curve Power", &fade_power, 0.2f, 5.0f, "%.2f")) schematic_cfg_.trail_fade_power = fade_power;
			float thickness = static_cast<float>(schematic_cfg_.trail_line_thickness);
			if (ImGui::SliderFloat("Trail Thickness", &thickness, 0.5f, 6.0f, "%.1f")) schematic_cfg_.trail_line_thickness = thickness;
		}

		if (schematic_cfg_.show_orbit_predictions) {
			ImGui::Separator();
			ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Orbit Predictions:");
			ImGui::Checkbox("Show Uncertainty Band", &schematic_cfg_.show_orbit_prediction_uncertainty);
			if (schematic_cfg_.show_orbit_prediction_uncertainty) {
				float uncertainty_growth = static_cast<float>(schematic_cfg_.orbit_prediction_uncertainty_growth);
				if (slider_float_with_input("Uncertainty Growth Rate", &uncertainty_growth, 0.0f, 0.5f, "%.4f")) schematic_cfg_.orbit_prediction_uncertainty_growth = uncertainty_growth;
				float uncertainty_opacity = static_cast<float>(schematic_cfg_.orbit_prediction_uncertainty_opacity);
				if (slider_float_with_input("Uncertainty Band Opacity", &uncertainty_opacity, 0.0f, 1.0f, "%.2f")) schematic_cfg_.orbit_prediction_uncertainty_opacity = uncertainty_opacity;
			}
			int segs = schematic_cfg_.orbit_prediction_segments;
			if (ImGui::SliderInt("Prediction Segments", &segs, 16, 2000)) schematic_cfg_.orbit_prediction_segments = segs;
			float prediction_duration = static_cast<float>(schematic_cfg_.orbit_prediction_duration);
			if (ImGui::SliderFloat("Prediction Horizon", &prediction_duration, 1.0f, 3600.0f, "%.1f s", ImGuiSliderFlags_Logarithmic)) schematic_cfg_.orbit_prediction_duration = prediction_duration;
			int prediction_substeps = schematic_cfg_.orbit_prediction_substeps;
			if (ImGui::SliderInt("Prediction Substeps", &prediction_substeps, 1, 32)) schematic_cfg_.orbit_prediction_substeps = prediction_substeps;
			float op_opacity = static_cast<float>(schematic_cfg_.orbit_prediction_opacity);
			if (ImGui::SliderFloat("Orbit Line Opacity", &op_opacity, 0.05f, 1.0f, "%.2f")) schematic_cfg_.orbit_prediction_opacity = op_opacity;
			float op_thick = static_cast<float>(schematic_cfg_.orbit_prediction_thickness);
			if (ImGui::SliderFloat("Orbit Line Thickness", &op_thick, 0.5f, 6.0f, "%.1f")) schematic_cfg_.orbit_prediction_thickness = op_thick;
			float max_ecc = static_cast<float>(schematic_cfg_.orbit_prediction_max_eccentricity);
			if (ImGui::SliderFloat("Max Eccentricity Shown", &max_ecc, 0.5f, 0.999f, "%.3f")) schematic_cfg_.orbit_prediction_max_eccentricity = max_ecc;
		}

		ImGui::Separator();
		render_schematic_object_style("Central Object Appearance", schematic_cfg_.central_object_style);
		render_schematic_object_style("Orbiting Bodies Appearance", schematic_cfg_.body_style);

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Vector Overlays:");
		render_schematic_vector_style("Velocity Vector", schematic_cfg_.vector_style(SchematicVectorKind::Velocity));
		render_schematic_vector_style("Total Force Vector", schematic_cfg_.vector_style(SchematicVectorKind::TotalForce));
		render_schematic_vector_style("Spin Vector", schematic_cfg_.vector_style(SchematicVectorKind::Spin));
		render_schematic_vector_style("Rotation Axis Vector (Surface)", schematic_cfg_.vector_style(SchematicVectorKind::RotationAxis));

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.9f, 0.75f, 0.3f, 1.0f), "Per-Body Appearance Overrides:");
		ImGui::TextDisabled("Assign a distinct display style to any individual body currently present in the N-Body system, overriding the shared 'Orbiting Bodies Appearance' style above for that body only.");

		auto& sys = orchestrator_.nbody_system();
		const auto bodies = sys.bodies();

		static int override_target_body_id = -1;
		std::vector<uint32_t> body_ids;
		body_ids.reserve(bodies.size());
		for (const auto& b : bodies) body_ids.push_back(b.id);

		if (body_ids.empty()) {
			ImGui::TextDisabled("No bodies present in the N-Body system. Add bodies from the Celestial Body & N-Body Manager to enable per-body overrides.");
		} else {
			std::vector<std::string> combo_labels;
			combo_labels.reserve(body_ids.size());
			for (const uint32_t id : body_ids) {
				const bool has_override = schematic_cfg_.body_style_overrides.contains(id);
				combo_labels.push_back("#" + std::to_string(id) + (has_override ? " (overridden)" : ""));
			}

			int selected_idx = 0;
			for (size_t i = 0; i < body_ids.size(); ++i) {
				if (static_cast<int>(body_ids[i]) == override_target_body_id) {
					selected_idx = static_cast<int>(i);
					break;
				}
			}

			std::vector<const char*> label_ptrs;
			label_ptrs.reserve(combo_labels.size());
			for (const auto& s : combo_labels) label_ptrs.push_back(s.c_str());

			if (ImGui::Combo("Target Body", &selected_idx, label_ptrs.data(), static_cast<int>(label_ptrs.size()))) {
				override_target_body_id = static_cast<int>(body_ids[static_cast<size_t>(selected_idx)]);
			}
			if (override_target_body_id < 0 && !body_ids.empty()) {
				override_target_body_id = static_cast<int>(body_ids.front());
			}

			const uint32_t target_id = static_cast<uint32_t>(override_target_body_id);
			const bool has_override = schematic_cfg_.body_style_overrides.contains(target_id);

			if (!has_override) {
				if (ImGui::Button("Create Override For This Body", ImVec2(240.0f, 26.0f))) {
					schematic_cfg_.body_style_overrides[target_id] = schematic_cfg_.body_style;
				}
			} else {
				if (ImGui::Button("Remove Override (Use Shared Style)", ImVec2(260.0f, 26.0f))) {
					schematic_cfg_.body_style_overrides.erase(target_id);
				}
				ImGui::Spacing();
				render_schematic_object_style(("Override Style For Body #" + std::to_string(target_id)).c_str(), schematic_cfg_.body_style_overrides[target_id]);
			}
		}
	}

	void render_units_tab() noexcept {
		auto& prefs = orchestrator_.unit_preferences();

		ImGui::TextColored(ImVec4(0.3f, 0.9f, 1.0f, 1.0f), "Display Units");
		ImGui::TextWrapped("Choose the units used to display every physical quantity throughout the HUD, viewport, and every widget panel.");
		ImGui::Separator();

		const char* distance_units[] = {"Meters", "Kilometers", "Feet", "Miles", "Nautical Miles", "Astronomical Units", "Light Years", "Parsecs", "Kiloparsecs", "Solar Radii"};
		int distance_idx = static_cast<int>(prefs.distance);
		if (ImGui::Combo("Distance Unit", &distance_idx, distance_units, IM_ARRAYSIZE(distance_units))) {
			prefs.distance = static_cast<Units::DistanceUnit>(distance_idx);
		}

		const char* mass_units[] = {"Kilograms", "Grams", "Pounds", "Metric Tonnes", "Solar Masses", "Earth Masses", "Jupiter Masses", "Turkeys (~11 kg)"};
		int mass_idx = static_cast<int>(prefs.mass);
		if (ImGui::Combo("Mass Unit", &mass_idx, mass_units, IM_ARRAYSIZE(mass_units))) {
			prefs.mass = static_cast<Units::MassUnit>(mass_idx);
		}

		const char* velocity_units[] = {"Meters/Second", "Kilometers/Hour", "Miles/Hour", "Kilometers/Second", "Fraction of c", "Parsecs/Year", "Astronomical Units/Day"};
		int velocity_idx = static_cast<int>(prefs.velocity);
		if (ImGui::Combo("Velocity Unit", &velocity_idx, velocity_units, IM_ARRAYSIZE(velocity_units))) {
			prefs.velocity = static_cast<Units::VelocityUnit>(velocity_idx);
		}

		const char* energy_units[] = {"Joules", "Kilojoules", "Megajoules", "Electronvolts", "Kilowatt-Hours", "Ergs", "Foot-Pounds", "Calories"};
		int energy_idx = static_cast<int>(prefs.energy);
		if (ImGui::Combo("Energy Unit", &energy_idx, energy_units, IM_ARRAYSIZE(energy_units))) {
			prefs.energy = static_cast<Units::EnergyUnit>(energy_idx);
		}

		const char* angle_units[] = {"Radians", "Degrees", "Arcminutes", "Arcseconds", "Gradians", "Revolutions", "Milliradians"};
		int angle_idx = static_cast<int>(prefs.angle);
		if (ImGui::Combo("Angle Unit", &angle_idx, angle_units, IM_ARRAYSIZE(angle_units))) {
			prefs.angle = static_cast<Units::AngleUnit>(angle_idx);
		}

		const char* temperature_units[] = {"Kelvin", "Celsius", "Fahrenheit", "Rankine"};
		int temperature_idx = static_cast<int>(prefs.temperature);
		if (ImGui::Combo("Temperature Unit", &temperature_idx, temperature_units, IM_ARRAYSIZE(temperature_units))) {
			prefs.temperature = static_cast<Units::TemperatureUnit>(temperature_idx);
		}

		const char* charge_units[] = {"Coulombs", "Millicoulombs", "Microcoulombs", "Elementary Charges", "Ampere-Hours", "Statcoulombs"};
		int charge_idx = static_cast<int>(prefs.charge);
		if (ImGui::Combo("Charge Unit", &charge_idx, charge_units, IM_ARRAYSIZE(charge_units))) {
			prefs.charge = static_cast<Units::ChargeUnit>(charge_idx);
		}

		const char* current_units[] = {"Amperes", "Milliamperes", "Microamperes", "Kiloamperes"};
		int current_idx = static_cast<int>(prefs.current);
		if (ImGui::Combo("Current Unit", &current_idx, current_units, IM_ARRAYSIZE(current_units))) {
			prefs.current = static_cast<Units::CurrentUnit>(current_idx);
		}

		const char* frame_rate_units[] = {"Frames Per Second", "Milliseconds", "Microseconds", "Hertz"};
		int frame_rate_idx = static_cast<int>(prefs.frame_rate);
		if (ImGui::Combo("Frame Rate Unit", &frame_rate_idx, frame_rate_units, IM_ARRAYSIZE(frame_rate_units))) {
			prefs.frame_rate = static_cast<Units::FrameRateUnit>(frame_rate_idx);
		}
		render_setting_tooltip("Controls how the Frame Time / FPS HUD readout presents its headline number.");

		const char* time_units[] = {"Seconds", "Milliseconds", "Microseconds", "Nanoseconds", "Minutes", "Hours", "Days", "Years"};
		int time_idx = static_cast<int>(prefs.time);
		if (ImGui::Combo("Time Unit", &time_idx, time_units, IM_ARRAYSIZE(time_units))) {
			prefs.time = static_cast<Units::TimeUnit>(time_idx);
		}

		const char* accel_units[] = {"Meters/Second^2", "Centimeters/Second^2", "Feet/Second^2", "Standard Gravity (g)", "Kilometers/Second^2"};
		int accel_idx = static_cast<int>(prefs.acceleration);
		if (ImGui::Combo("Acceleration Unit", &accel_idx, accel_units, IM_ARRAYSIZE(accel_units))) {
			prefs.acceleration = static_cast<Units::AccelerationUnit>(accel_idx);
		}

		const char* ang_vel_units[] = {"Radians/Second", "Degrees/Second", "Revolutions/Minute (RPM)", "Hertz (Hz)"};
		int ang_vel_idx = static_cast<int>(prefs.angular_velocity);
		if (ImGui::Combo("Angular Velocity Unit", &ang_vel_idx, ang_vel_units, IM_ARRAYSIZE(ang_vel_units))) {
			prefs.angular_velocity = static_cast<Units::AngularVelocityUnit>(ang_vel_idx);
		}

		const char* density_units[] = {"Kilograms/Meter^3", "Grams/Centimeter^3", "Pounds/Foot^3", "Solar Masses/Parsec^3"};
		int density_idx = static_cast<int>(prefs.density);
		if (ImGui::Combo("Density Unit", &density_idx, density_units, IM_ARRAYSIZE(density_units))) {
			prefs.density = static_cast<Units::DensityUnit>(density_idx);
		}

		const char* pressure_units[] = {"Pascals", "Kilopascals", "Megapascals", "Gigapascals", "Bars", "Atmospheres", "PSI"};
		int pressure_idx = static_cast<int>(prefs.pressure);
		if (ImGui::Combo("Pressure Unit", &pressure_idx, pressure_units, IM_ARRAYSIZE(pressure_units))) {
			prefs.pressure = static_cast<Units::PressureUnit>(pressure_idx);
		}

		const char* power_units[] = {"Watts", "Kilowatts", "Megawatts", "Solar Luminosities", "Horsepower"};
		int power_idx = static_cast<int>(prefs.power);
		if (ImGui::Combo("Power Unit", &power_idx, power_units, IM_ARRAYSIZE(power_units))) {
			prefs.power = static_cast<Units::PowerUnit>(power_idx);
		}

		const char* frequency_units[] = {"Hertz", "Kilohertz", "Megahertz", "Gigahertz"};
		int freq_idx = static_cast<int>(prefs.frequency);
		if (ImGui::Combo("Frequency Unit", &freq_idx, frequency_units, IM_ARRAYSIZE(frequency_units))) {
			prefs.frequency = static_cast<Units::FrequencyUnit>(freq_idx);
		}

		const char* force_units[] = {"Newtons", "Kilonewtons", "Dynes", "Pounds-Force"};
		int force_idx = static_cast<int>(prefs.force);
		if (ImGui::Combo("Force Unit", &force_idx, force_units, IM_ARRAYSIZE(force_units))) {
			prefs.force = static_cast<Units::ForceUnit>(force_idx);
		}

		const char* magnetic_units[] = {"Teslas", "Gauss", "Microteslas"};
		int mag_idx = static_cast<int>(prefs.magnetic_field);
		if (ImGui::Combo("Magnetic Field Unit", &mag_idx, magnetic_units, IM_ARRAYSIZE(magnetic_units))) {
			prefs.magnetic_field = static_cast<Units::MagneticFieldUnit>(mag_idx);
		}

		const char* voltage_units[] = {"Volts", "Millivolts", "Kilovolts", "Megavolts"};
		int volt_idx = static_cast<int>(prefs.voltage);
		if (ImGui::Combo("Voltage Unit", &volt_idx, voltage_units, IM_ARRAYSIZE(voltage_units))) {
			prefs.voltage = static_cast<Units::VoltageUnit>(volt_idx);
		}

		ImGui::Separator();
		ImGui::TextDisabled("Live Previews:");
		const auto& cam = orchestrator_.camera();
		const double distance_meters = cam.radius * orchestrator_.constants_engine().length_scale();
		ImGui::Text("Camera Distance: %s", Units::format_distance(distance_meters, prefs.distance).c_str());
		ImGui::Text("Camera Polar Angle: %s", Units::format_angle(cam.theta, prefs.angle).c_str());
		ImGui::Text("Central Charge: %s", Units::format_charge(orchestrator_.parameters().charge, prefs.charge).c_str());
		ImGui::Text("Reference Frame Time (16.67 ms): %s", Units::format_frame_time(16.67, prefs.frame_rate).c_str());
		ImGui::Text("Reference Temperature (5778 K): %s", Units::format_temperature(5778.0, prefs.temperature).c_str());
		ImGui::Text("Simulation Current Scale: %s", Units::format_current(orchestrator_.constants_engine().current_scale(), prefs.current).c_str());
		ImGui::Text("Reference Energy (1 Joule): %s", Units::format_energy(1.0, prefs.energy).c_str());
		ImGui::Text("Reference Time (3600 s): %s", Units::format_time(3600.0, prefs.time).c_str());
		ImGui::Text("Standard Gravity Acceleration: %s", Units::format_acceleration(9.80665, prefs.acceleration).c_str());
		ImGui::Text("ZAMO Angular Velocity: %s", Units::format_angular_velocity(1.0, prefs.angular_velocity).c_str());
		ImGui::Text("Reference Power (100 W): %s", Units::format_power(100.0, prefs.power).c_str());
		ImGui::Text("Reference Pressure (1 atm): %s", Units::format_pressure(101325.0, prefs.pressure).c_str());

		if (ImGui::Button("Reset To SI Defaults", ImVec2(200.0f, 26.0f))) {
			prefs = Units::UnitDisplayPreferences{};
		}
	}

	void render_hud_tab() noexcept {
		ImGui::TextColored(ImVec4(0.3f, 0.9f, 1.0f, 1.0f), "Heads-Up Display Configuration");
		ImGui::TextDisabled("Enable, position, resize, recolor, and style every individual HUD element directly from this tab.");
		ImGui::Separator();

		ImGui::Checkbox("Master HUD Visibility", &hud_layout_.master_enabled);
		render_setting_tooltip("Master switch for every overlay element, including the viewport toolbar and loading indicator. Disabling this hides the entire HUD.");

		ImGui::SameLine();
		if (ImGui::Button("Restore All HUD Defaults", ImVec2(200.0f, 24.0f))) {
			hud_layout_ = HudLayoutConfig{};
		}

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.3f, 1.0f), "Global Refresh & Averaging Settings:");

		int rolling_count = static_cast<int>(orchestrator_.parameters().rolling_average_frame_count);
		if (ImGui::SliderInt("Frame Time Rolling Average Window (N)", &rolling_count, 2, 120)) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::RollingAverageFrameCount, static_cast<double>(rolling_count))));
		}
		render_setting_tooltip("Number of historical frame times averaged into the HUD frame-time and FPS readouts. Larger windows produce smoother, slower-reacting numbers.");

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.5f, 0.85f, 1.0f, 1.0f), "Viewport Toolbar Buttons:");
		auto& tb = hud_layout_.toolbar_buttons;
		ImGui::Checkbox("Play / Pause Button", &tb.play_pause);
		ImGui::SameLine();
		ImGui::Checkbox("Step Button", &tb.step);
		ImGui::SameLine();
		ImGui::Checkbox("Reset View Button", &tb.reset_view);
		ImGui::Checkbox("Look-At Target Combo + Button", &tb.look_at_target_combo);
		ImGui::SameLine();
		ImGui::Checkbox("Jump-To-Target Button", &tb.jump_to_target);
		ImGui::Checkbox("Camera Mode Combo", &tb.camera_mode_combo);
		ImGui::SameLine();
		ImGui::Checkbox("HUD Master Toggle Checkbox", &tb.hud_master_toggle);
		render_setting_tooltip("Individually enable or disable each control exposed in the floating viewport toolbar without affecting the rest of the HUD.");

		ImGui::Spacing();
		ImGui::TextColored(ImVec4(0.4f, 0.85f, 0.65f, 1.0f), "Extended Toolbar Shortcuts:");
		ImGui::Checkbox("Screenshot Capture Button", &tb.screenshot);
		ImGui::SameLine();
		ImGui::Checkbox("Fullscreen Toggle Button", &tb.fullscreen_toggle);
		ImGui::Checkbox("GPU Compute Toggle", &tb.gpu_compute_toggle);
		ImGui::SameLine();
		ImGui::Checkbox("Space-Skip Toggle", &tb.space_skip_toggle);
		ImGui::SameLine();
		ImGui::Checkbox("LOD Toggle", &tb.lod_toggle);
		ImGui::Checkbox("Exposure +/- Buttons", &tb.exposure_controls);
		ImGui::SameLine();
		ImGui::Checkbox("Warp +/- Buttons", &tb.warp_controls);
		ImGui::Checkbox("Tonemapper Cycle Button", &tb.tonemapper_cycle);
		ImGui::SameLine();
		ImGui::Checkbox("Projection Cycle Button", &tb.projection_cycle);
		ImGui::Checkbox("Skybox Cycle Button", &tb.skybox_cycle);
		ImGui::SameLine();
		ImGui::Checkbox("Metric Cycle Button", &tb.metric_cycle);
		ImGui::Checkbox("Integrator Cycle Button", &tb.integrator_cycle);
		ImGui::SameLine();
		ImGui::Checkbox("Performance Preset Combo", &tb.performance_preset_combo);
		ImGui::Checkbox("Quick Save/Load Buttons", &tb.quicksave_quickload);
		ImGui::SameLine();
		ImGui::Checkbox("Step Controller Cycle Button", &tb.step_controller_cycle);
		ImGui::Checkbox("Render Distance Toggle Button", &tb.render_distance_toggle);
		ImGui::SameLine();
		ImGui::Checkbox("Pole Precision Nudge Buttons", &tb.pole_precision_nudge);
		render_setting_tooltip("Adds shortcuts to the floating toolbar for the same controls exposed elsewhere in this panel, so frequently used settings stay reachable without opening a tab.");

		ImGui::SliderFloat("Toolbar Button Padding Scale", &hud_layout_.toolbar_padding_scale, 0.4f, 3.0f, "%.2fx");
		render_setting_tooltip("Uniformly scales the padding of every button and control in the floating viewport toolbar, letting the toolbar be shrunk down for a minimal footprint or enlarged for easier touch or high-DPI use.");

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.5f, 0.85f, 1.0f, 1.0f), "Individual HUD Elements:");

		static char hud_search[64] = "";
		ImGui::SetNextItemWidth(220.0f);
		ImGui::InputTextWithHint("##HudSearch", "Search HUD elements...", hud_search, sizeof(hud_search));
		ImGui::SameLine();
		static int hud_sort_mode = 0;
		const char* hud_sort_options[] = {"Sort: Default Order", "Sort: Alphabetical", "Sort: Enabled First"};
		ImGui::SetNextItemWidth(200.0f);
		ImGui::Combo("##HudSortMode", &hud_sort_mode, hud_sort_options, IM_ARRAYSIZE(hud_sort_options));
		ImGui::SameLine();
		static bool hud_show_disabled_only = false;
		ImGui::Checkbox("Disabled Only", &hud_show_disabled_only);

		std::vector<HudElementId> visible_elements;
		visible_elements.reserve(static_cast<size_t>(HudElementId::Count));
		const std::string_view search_view(hud_search);
		auto to_lower = [](std::string s) {
			std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
			return s;
		};
		const std::string needle = to_lower(std::string(search_view));

		for (size_t i = 0; i < static_cast<size_t>(HudElementId::Count); ++i) {
			const auto id = static_cast<HudElementId>(i);
			if (!needle.empty()) {
				const std::string haystack = to_lower(std::string(hud_element_name(id)));
				if (haystack.find(needle) == std::string::npos) continue;
			}
			if (hud_show_disabled_only && hud_layout_.element(id).enabled) continue;
			visible_elements.push_back(id);
		}

		if (hud_sort_mode == 1) {
			std::sort(visible_elements.begin(), visible_elements.end(), [](HudElementId a, HudElementId b) {
				return std::string_view(hud_element_name(a)) < std::string_view(hud_element_name(b));
			});
		} else if (hud_sort_mode == 2) {
			std::sort(visible_elements.begin(), visible_elements.end(), [&](HudElementId a, HudElementId b) {
				const bool ea = hud_layout_.element(a).enabled;
				const bool eb = hud_layout_.element(b).enabled;
				if (ea != eb) return ea && !eb;
				return std::string_view(hud_element_name(a)) < std::string_view(hud_element_name(b));
			});
		}

		if (visible_elements.empty()) {
			ImGui::TextDisabled("No HUD elements match the current search and filter.");
		}

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.5f, 0.9f, 0.7f, 1.0f), "Layout Engine:");
		ImGui::Checkbox("Automatic Gap-Free Stacked Layout", &hud_layout_.auto_arrange_enabled);
		render_setting_tooltip("When enabled, elements sharing the same anchor corner automatically stack one after another in draw-priority order with no overlap or gaps. Offset X/Y then act as a fine-tuning nudge applied on top of the stacked position. When disabled, every element uses its raw Offset X/Y measured from the chosen anchor corner.");
		if (hud_layout_.auto_arrange_enabled) {
			ImGui::SliderFloat("Stacking Spacing", &hud_layout_.auto_arrange_spacing, 0.0f, 40.0f, "%.0f px");
		}

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.9f, 0.75f, 0.4f, 1.0f), "Bulk Actions On Visible Elements:");
		if (ImGui::Button("Enable All Visible", ImVec2(150.0f, 24.0f))) {
			for (const auto id : visible_elements) hud_layout_.element(id).enabled = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Disable All Visible", ImVec2(150.0f, 24.0f))) {
			for (const auto id : visible_elements) hud_layout_.element(id).enabled = false;
		}
		ImGui::SameLine();
		static int bulk_anchor_idx = 0;
		const char* bulk_anchor_names[] = {"Top Left", "Top Right", "Bottom Left", "Bottom Right", "Top Center", "Bottom Center"};
		ImGui::SetNextItemWidth(140.0f);
		ImGui::Combo("##BulkAnchor", &bulk_anchor_idx, bulk_anchor_names, IM_ARRAYSIZE(bulk_anchor_names));
		ImGui::SameLine();
		if (ImGui::Button("Apply Anchor To Visible", ImVec2(180.0f, 24.0f))) {
			for (const auto id : visible_elements) hud_layout_.element(id).anchor = static_cast<HudAnchor>(bulk_anchor_idx);
		}
		render_setting_tooltip("Bulk actions apply only to elements currently matched by the search filter above, letting you target either the entire HUD or a chosen selection.");

		const char* anchor_names[] = {"Top Left", "Top Right", "Bottom Left", "Bottom Right", "Top Center", "Bottom Center"};
		const char* display_mode_names[] = {"Compact", "Standard", "Extended"};
		const char* comparison_names[] = {"Above Threshold", "Below Threshold"};

		for (const auto id : visible_elements) {
			auto& elem = hud_layout_.element(id);
			ImGui::PushID(static_cast<int>(id));
			const std::string header_id = std::string(hud_element_name(id)) + "###hud_elem_header";
			const bool header_open = ImGui::CollapsingHeader(header_id.c_str());
			if (!elem.enabled) {
				ImGui::SameLine();
				ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.4f, 1.0f), "(disabled)");
			}
			if (header_open) {
				ImGui::Checkbox("Enabled", &elem.enabled);

				int anchor_idx = static_cast<int>(elem.anchor);
				if (ImGui::Combo("Anchor Corner", &anchor_idx, anchor_names, IM_ARRAYSIZE(anchor_names))) {
					elem.anchor = static_cast<HudAnchor>(anchor_idx);
				}

				if (hud_layout_.auto_arrange_enabled) {
					ImGui::DragFloat("Fine Nudge X", &elem.nudge_x, 1.0f, -400.0f, 400.0f, "%.0f px");
					ImGui::DragFloat("Fine Nudge Y", &elem.nudge_y, 1.0f, -400.0f, 400.0f, "%.0f px");
				} else {
					ImGui::DragFloat("Offset X", &elem.offset_x, 1.0f, 0.0f, 2400.0f, "%.0f px");
					ImGui::DragFloat("Offset Y", &elem.offset_y, 1.0f, 0.0f, 2400.0f, "%.0f px");
				}
				ImGui::SliderFloat("Text Scale", &elem.scale, 0.5f, 3.0f, "%.2fx");
				ImGui::ColorEdit4("Text Color", elem.text_color.data());
				ImGui::Checkbox("Show Background Panel", &elem.show_background);
				if (elem.show_background) {
					ImGui::SliderFloat("Background Opacity", &elem.background_opacity, 0.0f, 1.0f, "%.2f");
				}

				int display_mode_idx = static_cast<int>(elem.display_mode);
				if (ImGui::Combo("Display Format", &display_mode_idx, display_mode_names, IM_ARRAYSIZE(display_mode_names))) {
					elem.display_mode = static_cast<HudDisplayMode>(display_mode_idx);
				}
				render_setting_tooltip("Compact shows the minimal essential value, Standard shows the default readout, Extended shows the full breakdown with additional derived figures. Every readout in the HUD honors this setting.");

				ImGui::SliderInt("Decimal Precision", &elem.decimal_precision, 0, 6);
				render_setting_tooltip("Number of decimal digits shown for numeric readouts belonging to this element.");

				ImGui::Checkbox("Show Descriptive Label", &elem.show_label);
				ImGui::Checkbox("Lay Out Lines Horizontally", &elem.horizontal_layout);
				render_setting_tooltip("Only applies to multi-line panels such as Navigation Controls; arranges entries side by side instead of stacked vertically.");

				ImGui::InputInt("Draw Priority", &elem.draw_priority);
				render_setting_tooltip("Elements with a higher priority are placed first when using the automatic stacked layout, and are listed first within a shared anchor corner.");
				ImGui::SameLine();
				if (ImGui::SmallButton("Move Earlier")) {
					elem.draw_priority += 1;
				}
				ImGui::SameLine();
				if (ImGui::SmallButton("Move Later")) {
					elem.draw_priority -= 1;
				}

				ImGui::SliderFloat("Refresh Interval", &elem.refresh_interval_seconds, 0.0f, 5.0f, "%.2f s");
				render_setting_tooltip("Minimum time between visual updates for this element. Zero means the element refreshes every frame.");

				if (ImGui::TreeNode("Dynamic Warning Color Rule")) {
					ImGui::Checkbox("Enable Warning Rule", &elem.warning_rule.enabled);
					if (elem.warning_rule.enabled) {
						int cmp_idx = static_cast<int>(elem.warning_rule.comparison);
						if (ImGui::Combo("Trigger When Value Is", &cmp_idx, comparison_names, IM_ARRAYSIZE(comparison_names))) {
							elem.warning_rule.comparison = static_cast<HudColorRuleComparison>(cmp_idx);
						}
						double threshold = elem.warning_rule.threshold;
						if (ImGui::InputDouble("Threshold", &threshold, 0.1, 1.0, "%.3f")) {
							elem.warning_rule.threshold = threshold;
						}
						ImGui::ColorEdit4("Warning Color", elem.warning_rule.color.data());
					}
					ImGui::TreePop();
				}

				if (ImGui::TreeNode("Dynamic Critical Color Rule")) {
					ImGui::Checkbox("Enable Critical Rule", &elem.critical_rule.enabled);
					if (elem.critical_rule.enabled) {
						int cmp_idx = static_cast<int>(elem.critical_rule.comparison);
						if (ImGui::Combo("Trigger When Value Is", &cmp_idx, comparison_names, IM_ARRAYSIZE(comparison_names))) {
							elem.critical_rule.comparison = static_cast<HudColorRuleComparison>(cmp_idx);
						}
						double threshold = elem.critical_rule.threshold;
						if (ImGui::InputDouble("Threshold", &threshold, 0.1, 1.0, "%.3f")) {
							elem.critical_rule.threshold = threshold;
						}
						ImGui::ColorEdit4("Critical Color", elem.critical_rule.color.data());
					}
					ImGui::TreePop();
				}
				render_setting_tooltip("Critical takes precedence over Warning when both trigger. Currently drives the Frame Time / FPS readout and the Profiler frame time readout, which expose a numeric value to compare against the threshold.");
			}
			ImGui::PopID();
		}

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.4f, 1.0f), "Linked Widget Readouts:");
		ImGui::TextDisabled("These HUD elements mirror a summary of live data from their corresponding analysis windows directly onto the viewport, without needing to open those windows.");
		ImGui::BulletText("Telemetry Quick Readout mirrors observer lapse and gravitational time dilation from the Telemetry & Invariants window.");
		ImGui::BulletText("Spectrograph Quick Readout mirrors the observer's live circular-orbit Doppler factor and exposure from the Radiative Transfer & Spectrograph Monitor.");
		ImGui::BulletText("Diagnostics Quick Readout mirrors the active metric and integrator from the Curvature Diagnostics window.");
		ImGui::BulletText("Profiler: GPU/CPU Split Readout mirrors the render path distribution from the Performance Analysis & Profiling window.");

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.6f, 0.9f, 1.0f, 1.0f), "Keybind Summary Panel Contents:");
		ImGui::TextDisabled("Choose which bound actions appear in the floating Keybind Summary panel on the viewport. Actions with no assigned key are hidden automatically and cannot be enabled here.");

		const auto& keybinds_ref = camera_controller_.config().keybinds;
		for (uint32_t cat_idx = 0; cat_idx < static_cast<uint32_t>(InputActionCategory::InterfaceWindows) + 1; ++cat_idx) {
			const auto category = static_cast<InputActionCategory>(cat_idx);
			bool any_in_category = false;
			for (size_t i = 0; i < static_cast<size_t>(InputAction::Count); ++i) {
				const auto action = static_cast<InputAction>(i);
				if (input_action_category(action) != category) continue;
				const auto& b = keybinds_ref.get(action);
				if (b.primary_key == GLFW_KEY_UNKNOWN && b.secondary_key == GLFW_KEY_UNKNOWN) continue;
				any_in_category = true;
				break;
			}
			if (!any_in_category) continue;

			ImGui::PushID(static_cast<int>(cat_idx) + 9000);
			if (ImGui::CollapsingHeader(std::string(input_action_category_name(category)).c_str())) {
				for (size_t i = 0; i < static_cast<size_t>(InputAction::Count); ++i) {
					const auto action = static_cast<InputAction>(i);
					if (input_action_category(action) != category) continue;
					const auto& b = keybinds_ref.get(action);
					if (b.primary_key == GLFW_KEY_UNKNOWN && b.secondary_key == GLFW_KEY_UNKNOWN) continue;

					const std::string label = std::string(input_action_name(action)) + " (" + format_key_binding(b) + ")";
					ImGui::Checkbox(label.c_str(), &hud_layout_.keybind_summary_visible[i]);
				}
			}
			ImGui::PopID();
		}
	}
};

}

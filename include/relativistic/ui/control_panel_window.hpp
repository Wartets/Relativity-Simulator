#pragma once

#include <imgui.h>
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/orchestrator/command.hpp"
#include "relativistic/render/gpu_types.hpp"
#include "relativistic/ui/interactive_camera_controller.hpp"
#include "relativistic/ui/hud_layout_config.hpp"
#include "relativistic/ui/schematic_view_config.hpp"
#include "relativistic/ui/tooltip_utils.hpp"
#include "relativistic/ui/numeric_slider_utils.hpp"
#include "relativistic/ui/compatibility_notes.hpp"
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
	uint64_t last_synced_version_{0};

public:
	explicit ControlPanelWindow(Orchestrator::SimulationOrchestrator<1024>& orchestrator, InteractiveCameraController& camera_controller, HudLayoutConfig& hud_layout, SchematicViewConfig& schematic_cfg, bool& hud_manager_open, bool& keybind_settings_open)
		: orchestrator_(orchestrator), camera_controller_(camera_controller), hud_layout_(hud_layout), schematic_cfg_(schematic_cfg), hud_manager_open_(hud_manager_open), keybind_settings_open_(keybind_settings_open) {
		sync_from_orchestrator();
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
			if (slider_float_with_input("Central Mass (M)", &mass_, 0.01f, 100.0f, "%.3f", &mass_log_mode_)) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::Mass, static_cast<double>(mass_))));
			}
			render_setting_tooltip("Central gravitating mass in geometrized units (M). Governs Schwarzschild radius rs = 2M and spacetime curvature strength. Enable Log for finer control across small or very large magnitudes.");
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
			if (ImGui::SliderFloat("Electric Charge (Q)", &charge_, -1.0f, 1.0f, "%.3f")) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::Charge, static_cast<double>(charge_))));
			}
			render_setting_tooltip("Net electrostatic charge in Coulomb geometrized units. Creates an inner Cauchy horizon and counteracts gravitational attraction.");
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

		if (slider_float_with_input("Field of View (FOV)", &camera_fov_, 10.0f, 160.0f, "%.1f")) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_camera_set_fov(static_cast<double>(camera_fov_))));
		}
		render_setting_tooltip("Horizontal angular aperture in degrees. Can also be dynamically zoomed using mouse wheel scroll.");

		if (ImGui::SliderFloat("Navigation Speed", &camera_speed_, 0.1f, 100.0f, "%.1f m/s")) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_camera_set_speed(static_cast<double>(camera_speed_))));
			camera_controller_.set_uniform_speed(static_cast<double>(camera_speed_));
		}
		render_setting_tooltip("Translational observer traversal speed in coordinate units per second. Hold Shift to sprint, Ctrl to crawl.");

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
		float ff_fwd = static_cast<float>(cfg.free_fly.forward_speed);
		if (ImGui::SliderFloat("Forward/Back Speed", &ff_fwd, 0.1f, 200.0f, "%.1f")) cfg.free_fly.forward_speed = static_cast<double>(ff_fwd);
		float ff_lat = static_cast<float>(cfg.free_fly.lateral_speed);
		if (ImGui::SliderFloat("Left/Right Speed", &ff_lat, 0.1f, 200.0f, "%.1f")) cfg.free_fly.lateral_speed = static_cast<double>(ff_lat);
		float ff_vert = static_cast<float>(cfg.free_fly.vertical_speed);
		if (ImGui::SliderFloat("Up/Down Speed", &ff_vert, 0.1f, 200.0f, "%.1f")) cfg.free_fly.vertical_speed = static_cast<double>(ff_vert);
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
		float orb_dist = static_cast<float>(cfg.orbit.orbit_distance_speed);
		if (ImGui::SliderFloat("Zoom Speed", &orb_dist, 0.1f, 200.0f, "%.1f")) cfg.orbit.orbit_distance_speed = static_cast<double>(orb_dist);
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
			auto& c = orchestrator_.camera();
			c.position = {0.0, 50.0, 0.0};
			c.pitch = 0.0;
			c.yaw = 180.0;
			c.roll = 0.0;
		}
		ImGui::SameLine();
		if (ImGui::Button("Top Polar View (z=50)")) {
			auto& c = orchestrator_.camera();
			c.position = {0.0, 0.001, 50.0};
			c.pitch = -89.0;
			c.yaw = 0.0;
			c.roll = 0.0;
		}
		ImGui::SameLine();
		if (ImGui::Button("Close-up ISCO (r=8)")) {
			auto& c = orchestrator_.camera();
			c.position = {0.0, 8.0, 0.0};
			c.pitch = 0.0;
			c.yaw = 180.0;
			c.roll = 0.0;
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
		}
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
				const char* sphere_styles[] = {"Opaque", "Translucent", "Wireframe Cage"};
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

			const char* color_modes[] = {"Body Color", "By Mass", "By Speed", "By Spin Magnitude", "By Distance From Center", "By Kinetic Energy", "Physical: Temperature", "Physical: Charge", "Physical: Density"};
			int color_idx = static_cast<int>(style.color_mode);
			if (ImGui::Combo("Color Coding", &color_idx, color_modes, IM_ARRAYSIZE(color_modes))) {
				style.color_mode = static_cast<SchematicColorCodingMode>(color_idx);
			}
			ImGui::ColorEdit4("Base / Fallback Color", style.uniform_color.data());

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

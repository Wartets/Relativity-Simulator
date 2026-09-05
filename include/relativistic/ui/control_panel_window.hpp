#pragma once

#include <imgui.h>
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/orchestrator/command.hpp"
#include "relativistic/render/gpu_types.hpp"
#include "relativistic/ui/interactive_camera_controller.hpp"
#include "relativistic/ui/hud_preferences.hpp"
#include "relativistic/ui/schematic_view_config.hpp"
#include "relativistic/ui/tooltip_utils.hpp"
#include <string>
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
	HudPreferences& hud_prefs_;
	SchematicViewConfig& schematic_cfg_;

	float mass_{1.0f};
	float spin_{0.0f};
	float charge_{0.0f};
	float lambda_{0.0f};
	float throat_{1.0f};
	float warp_vel_{1.0f};

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
	explicit ControlPanelWindow(Orchestrator::SimulationOrchestrator<1024>& orchestrator, InteractiveCameraController& camera_controller, HudPreferences& hud_prefs, SchematicViewConfig& schematic_cfg)
		: orchestrator_(orchestrator), camera_controller_(camera_controller), hud_prefs_(hud_prefs), schematic_cfg_(schematic_cfg) {
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
		render_setting_tooltip("Select the background Riemannian manifold or exact vacuum/electrovacuum spacetime solution to simulate.");

		ImGui::Separator();

		const bool needs_mass = (metric_selection_ == 1 || metric_selection_ == 2 || metric_selection_ == 3 || metric_selection_ == 4 || metric_selection_ == 5);
		const bool needs_spin = (metric_selection_ == 2 || metric_selection_ == 4);
		const bool needs_charge = (metric_selection_ == 3 || metric_selection_ == 4);
		const bool needs_lambda = (metric_selection_ == 5);
		const bool needs_throat = (metric_selection_ == 7);
		const bool needs_warp_velocity = (metric_selection_ == 8);
		const bool has_any_param = needs_mass || needs_spin || needs_charge || needs_lambda || needs_throat || needs_warp_velocity;

		if (needs_mass) {
			if (ImGui::SliderFloat("Central Mass (M)", &mass_, 0.01f, 100.0f, "%.3f")) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::Mass, static_cast<double>(mass_))));
			}
			render_setting_tooltip("Central gravitating mass in geometrized units (M). Governs Schwarzschild radius rs = 2M and spacetime curvature strength.");
		}

		if (needs_spin) {
			if (ImGui::SliderFloat("Spin Parameter (a)", &spin_, -0.999f, 0.999f, "%.4f")) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::Spin, static_cast<double>(spin_))));
			}
			render_setting_tooltip("Specific angular momentum a = J / M. Deforms the event horizon into an oblate spheroid and induces Lense-Thirring frame-dragging.");
		}

		if (needs_charge) {
			if (ImGui::SliderFloat("Electric Charge (Q)", &charge_, -1.0f, 1.0f, "%.3f")) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::Charge, static_cast<double>(charge_))));
			}
			render_setting_tooltip("Net electrostatic charge in Coulomb geometrized units. Creates an inner Cauchy horizon and counteracts gravitational attraction.");
		}

		if (needs_lambda) {
			if (ImGui::InputFloat("Cosmological Lambda", &lambda_, 1e-6f, 1e-4f, "%.6e")) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::CosmologicalLambda, static_cast<double>(lambda_))));
			}
			render_setting_tooltip("Cosmological constant responsible for large-scale cosmic acceleration and cosmological horizon creation.");
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

		ImGui::Separator();

		const char* cam_modes[] = {"Free Fly 6-DOF", "Orbit Center Target", "Spherical (Boyer-Lindquist)", "Rocket 6-DOF Thrust"};
		int mode = static_cast<int>(orchestrator_.parameters().camera_mode);
		if (ImGui::Combo("Camera Mode", &mode, cam_modes, IM_ARRAYSIZE(cam_modes))) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_camera_mode(static_cast<uint32_t>(mode))));
		}
		render_setting_tooltip("Observer navigation paradigm (6-DOF Free Fly, Spherical Boyer-Lindquist Orbit, Cockpit Flight).");

		if (ImGui::SliderFloat("Field of View (FOV)", &camera_fov_, 10.0f, 160.0f, "%.1f deg")) {
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
		bool schematic_mode = orchestrator_.parameters().schematic_mode_enabled;
		if (ImGui::Checkbox("Schematic Orbital View (No Lensing)", &schematic_mode)) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SchematicModeEnabled, schematic_mode ? 1.0 : 0.0)));
		}
		render_setting_tooltip("Replaces gravitational ray tracing with a simplified projection showing every body as a plain sphere against a coordinate grid backdrop, with orientation arrows for spin axes. Simulation time is frozen while this view is active.");

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
		ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.6f, 1.0f), "Keybind Reference:");
		auto show_bind = [&](const char* label, CameraAction action) noexcept {
			const auto& b = cfg.keybinds.get(action);
			ImGui::Text("%s: %s / %s", label, CameraKeybindMap::key_name(b.primary_key), CameraKeybindMap::key_name(b.secondary_key));
		};
		show_bind("Forward", CameraAction::MoveForward);
		show_bind("Backward", CameraAction::MoveBackward);
		show_bind("Left", CameraAction::MoveLeft);
		show_bind("Right", CameraAction::MoveRight);
		show_bind("Up", CameraAction::MoveUp);
		show_bind("Down", CameraAction::MoveDown);
		show_bind("Roll Left", CameraAction::RollLeft);
		show_bind("Roll Right", CameraAction::RollRight);
		show_bind("Sprint", CameraAction::Sprint);
		show_bind("Crawl", CameraAction::Crawl);
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
		render_setting_tooltip("Numerical differential solver scheme: adaptive Runge-Kutta Dormand-Prince, high-order Vernier 9(8), or symplectic Gauss-Legendre.");

		ImGui::Separator();

		float rtol = static_cast<float>(orchestrator_.parameters().integration_rtol);
		if (ImGui::InputFloat("Relative Tolerance (rtol)", &rtol, 1e-12f, 1e-8f, "%.2e")) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::IntegrationRtol, static_cast<double>(rtol))));
		}
		render_setting_tooltip("Local relative error tolerance threshold controlling adaptive step-size regulation.");

		float atol = static_cast<float>(orchestrator_.parameters().integration_atol);
		if (ImGui::InputFloat("Absolute Tolerance (atol)", &atol, 1e-16f, 1e-12f, "%.2e")) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::IntegrationAtol, static_cast<double>(atol))));
		}
		render_setting_tooltip("Absolute error tolerance floor preventing step-size collapse near null-coordinate vanishing states.");
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

		ImGui::Text("Simulation Cycle: #%llu", static_cast<unsigned long long>(snap.tick_index));
		ImGui::Text("Logical Time:     %.4f s", snap.logical_time);

		float warp = static_cast<float>(snap.warp_factor);
		if (ImGui::SliderFloat("Warp Factor", &warp, 0.1f, 100.0f, "%.2fx")) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_warp(static_cast<double>(warp))));
		}
		render_setting_tooltip("Temporal acceleration multiplier applied to the logical simulation clock.");

		float rate = static_cast<float>(snap.tick_rate_hz);
		if (ImGui::SliderFloat("Scheduler Rate", &rate, 10.0f, 240.0f, "%.0f Hz")) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_tickrate(static_cast<double>(rate))));
		}
		render_setting_tooltip("Fixed logical simulation clock frequency decoupled from display frame rates (10 Hz to 1000 Hz).");

		ImGui::Separator();

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

			const char* color_modes[] = {"Uniform Color", "By Mass", "By Speed", "By Spin Magnitude", "By Distance From Center", "By Kinetic Energy"};
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

		ImGui::Checkbox("Respect Active Camera Projection Mode", &schematic_cfg_.respect_active_projection_mode);
		render_setting_tooltip("When enabled, the schematic view reprojects geometry using the same projection (Pinhole, Fisheye, Equirectangular, etc.) selected in Optics & Camera. When disabled, a standard pinhole projection is always used.");

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
			if (ImGui::SliderInt("Ellipse Segments", &segs, 16, 400)) schematic_cfg_.orbit_prediction_segments = segs;
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
	}

	void render_hud_tab() noexcept {
		ImGui::TextColored(ImVec4(0.3f, 0.9f, 1.0f, 1.0f), "Heads-Up Display Manager");
		ImGui::TextDisabled("Choose which readouts and controls appear over the viewport.");
		ImGui::Separator();

		ImGui::Checkbox("Master HUD Visibility", &hud_prefs_.show_hud);
		render_setting_tooltip("Master switch for all overlay elements drawn on top of the viewport.");

		ImGui::BeginDisabled(!hud_prefs_.show_hud);
		ImGui::Checkbox("Viewport Toolbar (Play/Pause/Look-At)", &hud_prefs_.show_viewport_toolbar);
		ImGui::Checkbox("Rendering Progress Indicator", &hud_prefs_.show_loading_indicator);

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Telemetry Readouts:");
		ImGui::Checkbox("Frame Time", &hud_prefs_.show_frame_time);
		ImGui::Checkbox("Rolling Average FPS", &hud_prefs_.show_rolling_average_fps);
		ImGui::Checkbox("Camera Distance", &hud_prefs_.show_camera_distance);
		ImGui::Checkbox("Camera Angles (theta, phi)", &hud_prefs_.show_camera_angles);
		ImGui::Checkbox("Camera Orientation (pitch, yaw, roll)", &hud_prefs_.show_camera_orientation);
		ImGui::Checkbox("Active Metric Summary", &hud_prefs_.show_metric_summary);
		ImGui::Checkbox("Ray Statistics", &hud_prefs_.show_ray_statistics);
		ImGui::Checkbox("Navigation Keybind Panel", &hud_prefs_.show_navigation_controls);
		ImGui::EndDisabled();

		ImGui::Spacing();
		ImGui::Separator();
		if (ImGui::Button("Restore All Defaults", ImVec2(200.0f, 26.0f))) {
			hud_prefs_ = HudPreferences{};
		}
	}
};

}

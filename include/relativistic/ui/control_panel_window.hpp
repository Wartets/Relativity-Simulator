#pragma once

#include <imgui.h>
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/orchestrator/command.hpp"
#include <string>
#include <array>

namespace Relativistic::UI {

class ControlPanelWindow {
private:
	bool is_open_{true};
	Orchestrator::SimulationOrchestrator<1024>& orchestrator_;

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

	int metric_selection_{1};
	int integrator_selection_{0};

	float rocket_thrust_x_{0.0f};
	float rocket_thrust_y_{0.0f};
	float rocket_thrust_z_{0.0f};
	float rocket_throttle_{0.0f};
	int timeflow_mode_{0};

public:
	explicit ControlPanelWindow(Orchestrator::SimulationOrchestrator<1024>& orchestrator)
		: orchestrator_(orchestrator) {
		sync_from_orchestrator();
	}

	void sync_from_orchestrator() noexcept {
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
	}

	void render() {
		if (!is_open_) return;

		ImGui::SetNextWindowPos(ImVec2(1480.0f, 35.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(425.0f, 705.0f), ImGuiCond_FirstUseEver);

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

		ImGui::Separator();

		if (ImGui::SliderFloat("Central Mass (M)", &mass_, 0.01f, 100.0f, "%.3f")) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::Mass, static_cast<double>(mass_))));
		}

		if (ImGui::SliderFloat("Spin Parameter (a)", &spin_, -0.999f, 0.999f, "%.4f")) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::Spin, static_cast<double>(spin_))));
		}

		if (ImGui::SliderFloat("Electric Charge (Q)", &charge_, -1.0f, 1.0f, "%.3f")) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::Charge, static_cast<double>(charge_))));
		}

		if (ImGui::InputFloat("Cosmological Lambda", &lambda_, 1e-6f, 1e-4f, "%.6e")) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::CosmologicalLambda, static_cast<double>(lambda_))));
		}

		if (ImGui::SliderFloat("Wormhole Throat (b0)", &throat_, 0.1f, 20.0f, "%.2f")) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::WormholeThroat, static_cast<double>(throat_))));
		}

		if (ImGui::SliderFloat("Warp Bubble Velocity (vs)", &warp_vel_, 0.0f, 10.0f, "%.2f c")) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::WarpVelocity, static_cast<double>(warp_vel_))));
		}
	}

	void render_camera_tab() noexcept {
		const char* projections[] = {
			"Pinhole Perspective",
			"Auto-Zoom (Aberration Comp.)",
			"Fisheye Stereographic",
			"Equirectangular 360 Panorama"
		};

		if (ImGui::Combo("Projection Mode", &projection_mode_, projections, IM_ARRAYSIZE(projections))) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::ProjectionMode, static_cast<double>(projection_mode_))));
		}

		const char* tonemappers[] = {
			"Linear Unclamped",
			"ACES Filmic Curve",
			"Logarithmic Extended HDR",
			"Reinhard Modified"
		};

		if (ImGui::Combo("HDR Tonemapper", &tonemapping_mode_, tonemappers, IM_ARRAYSIZE(tonemappers))) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::TonemappingMode, static_cast<double>(tonemapping_mode_))));
		}

		ImGui::Separator();

		const char* cam_modes[] = {"Free Fly 6-DOF", "Orbit Center Target", "Cockpit View"};
		int mode = static_cast<int>(orchestrator_.parameters().camera_mode);
		if (ImGui::Combo("Camera Mode", &mode, cam_modes, IM_ARRAYSIZE(cam_modes))) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_camera_mode(static_cast<uint32_t>(mode))));
		}

		if (ImGui::SliderFloat("Field of View (FOV)", &camera_fov_, 10.0f, 160.0f, "%.1f deg")) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_camera_set_fov(static_cast<double>(camera_fov_))));
		}

		if (ImGui::SliderFloat("Navigation Speed", &camera_speed_, 0.1f, 100.0f, "%.1f m/s")) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_camera_set_speed(static_cast<double>(camera_speed_))));
		}

		if (ImGui::SliderFloat("Exposure Compensation (EV)", &camera_exposure_, -6.0f, 6.0f, "%.2f EV")) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::CameraExposure, static_cast<double>(camera_exposure_))));
		}

		ImGui::Separator();
		ImGui::Text("Camera Quick Viewpoints:");
		if (ImGui::Button("Equatorial View (r=50)")) {
			auto& c = orchestrator_.camera();
			c.position = {0.0, 50.0, 0.0};
			c.pitch = 0.0;
			c.yaw = 0.0;
		}
		ImGui::SameLine();
		if (ImGui::Button("Top Polar View (z=50)")) {
			auto& c = orchestrator_.camera();
			c.position = {0.0, 0.0, 50.0};
			c.pitch = -89.0;
			c.yaw = 0.0;
		}
		ImGui::SameLine();
		if (ImGui::Button("Close-up ISCO (r=8)")) {
			auto& c = orchestrator_.camera();
			c.position = {0.0, 8.0, 0.0};
			c.pitch = 0.0;
			c.yaw = 0.0;
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

		ImGui::Separator();

		float rtol = static_cast<float>(orchestrator_.parameters().integration_rtol);
		if (ImGui::InputFloat("Relative Tolerance (rtol)", &rtol, 1e-12f, 1e-8f, "%.2e")) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::IntegrationRtol, static_cast<double>(rtol))));
		}

		float atol = static_cast<float>(orchestrator_.parameters().integration_atol);
		if (ImGui::InputFloat("Absolute Tolerance (atol)", &atol, 1e-16f, 1e-12f, "%.2e")) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::IntegrationAtol, static_cast<double>(atol))));
		}
	}

	void render_rocket_tab() noexcept {
		const char* flows[] = {"Proper Time Comobile (tau)", "Coordinate Time (t)"};
		if (ImGui::Combo("Clock Flow", &timeflow_mode_, flows, IM_ARRAYSIZE(flows))) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::TimeFlowMode, static_cast<double>(timeflow_mode_))));
		}

		ImGui::Separator();
		ImGui::SliderFloat("Main Throttle", &rocket_throttle_, 0.0f, 1.0f, "%.2f");
		ImGui::SliderFloat("Thrust X (Longitudinal)", &rocket_thrust_x_, -100.0f, 100.0f, "%.1f m/s^2");
		ImGui::SliderFloat("Thrust Y (Lateral)", &rocket_thrust_y_, -50.0f, 50.0f, "%.1f m/s^2");
		ImGui::SliderFloat("Thrust Z (Normal)", &rocket_thrust_z_, -50.0f, 50.0f, "%.1f m/s^2");
	}

	void render_execution_tab() noexcept {
		const auto snap = orchestrator_.scheduler().snapshot();

		ImGui::Text("Simulation Cycle: #%llu", static_cast<unsigned long long>(snap.tick_index));
		ImGui::Text("Logical Time:     %.4f s", snap.logical_time);

		float warp = static_cast<float>(snap.warp_factor);
		if (ImGui::SliderFloat("Warp Factor", &warp, 0.1f, 100.0f, "%.2fx")) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_warp(static_cast<double>(warp))));
		}

		float rate = static_cast<float>(snap.tick_rate_hz);
		if (ImGui::SliderFloat("Scheduler Rate", &rate, 10.0f, 240.0f, "%.0f Hz")) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_tickrate(static_cast<double>(rate))));
		}

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
};

}

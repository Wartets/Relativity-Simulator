#pragma once

#include <imgui.h>
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/core/riemann.hpp"
#include "relativistic/metrics/kerr.hpp"
#include "relativistic/metrics/kerr_invariants.hpp"
#include "relativistic/metrics/bardeen_shadow.hpp"
#include "relativistic/ui/tooltip_utils.hpp"
#include <cmath>
#include <numbers>
#include <algorithm>

namespace Relativistic::UI {

class TelemetryWindow {
private:
	bool is_open_{true};
	double cached_r_scalar_{0.0};
	double cached_k1_{0.0};
	double last_eval_time_{-1.0};
	double last_mass_{-1.0};
	double last_spin_{-1.0};
	double last_r_{-1.0};

	[[nodiscard]] static double kerr_isco_radius(double m, double a_spin) noexcept {
		if (m <= 1e-12) return 0.0;
		const double a_star = std::clamp(a_spin / m, -0.9999999, 0.9999999);
		const double abs_a = std::abs(a_star);
		const double orbit_sign = (a_star >= 0.0) ? -1.0 : 1.0;
		const double cbrt_term = std::cbrt(std::max(1.0 - abs_a * abs_a, 0.0));
		const double z1 = 1.0 + cbrt_term * (std::cbrt(1.0 + abs_a) + std::cbrt(1.0 - abs_a));
		const double z2 = std::sqrt(3.0 * abs_a * abs_a + z1 * z1);
		const double r_isco_over_m = 3.0 + z2 + orbit_sign * std::sqrt(std::max((3.0 - z1) * (3.0 + z1 + 2.0 * z2), 0.0));
		return r_isco_over_m * m;
	}

public:
	[[nodiscard]] bool& open_state() noexcept {
		return is_open_;
	}

	void render(const Orchestrator::SimulationOrchestrator<1024>& orchestrator) {
		if (!is_open_) return;

		ImGui::SetNextWindowPos(ImVec2(15.0f, 705.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(340.0f, 560.0f), ImGuiCond_FirstUseEver);

		if (!ImGui::Begin("Telemetry & Invariants", &is_open_)) {
			ImGui::End();
			return;
		}

		{
			const auto& params = orchestrator.parameters();
			const auto& cam = orchestrator.camera();
			const double cur_time = ImGui::GetTime();

			if (cur_time - last_eval_time_ > 0.15 || params.mass != last_mass_ || params.spin != last_spin_ || std::abs(cam.radius - last_r_) > 0.1) {
				last_eval_time_ = cur_time;
				last_mass_ = params.mass;
				last_spin_ = params.spin;
				last_r_ = cam.radius;

				const double r_safe = std::max(cam.radius, 2.05 * params.mass);
				Metrics::KerrMetric<double> metric(params.mass, params.spin, 1.0, 1.0);
				Core::FourVector<double> obs_pos(0.0, r_safe, cam.theta, cam.phi);

				const double r6 = std::pow(r_safe, 6.0);
				if (std::abs(params.spin) < 1e-12) {
					cached_k1_ = 48.0 * params.mass * params.mass / std::max(r6, 1e-12);
					cached_r_scalar_ = 0.0;
				} else {
					auto R_tensor = Core::RiemannComputer<Core::DerivativeOrder::FourthOrder, Metrics::KerrMetric<double>, double>::compute_riemann(metric, obs_pos);
					auto g = metric.metric_tensor(obs_pos);
					auto inv_g = metric.inverse_metric(obs_pos);
					auto ricci = Core::RiemannComputer<Core::DerivativeOrder::FourthOrder, Metrics::KerrMetric<double>, double>::compute_ricci_tensor(R_tensor);
					cached_r_scalar_ = Core::RiemannComputer<Core::DerivativeOrder::FourthOrder, Metrics::KerrMetric<double>, double>::compute_ricci_scalar(ricci, inv_g);
					cached_k1_ = Core::RiemannComputer<Core::DerivativeOrder::FourthOrder, Metrics::KerrMetric<double>, double>::compute_kretschmann_invariant(R_tensor, g, inv_g);
				}
			}

			ImGui::Text("Observer Position: r = %.4f, theta = %.4f", cam.radius, cam.theta);
			ImGui::Separator();
			ImGui::Text("Ricci Scalar Curvature (R): %.6e", cached_r_scalar_);
			ImGui::Text("Kretschmann Invariant (K1): %.6e", cached_k1_);
			render_setting_tooltip("Coordinate-free curvature scalars at the observer's location. See the Curvature Diagnostics window for a fuller radial breakdown.");

			ImGui::Separator();
			ImGui::Text("Hamiltonian Constraint Residual: %.6e", std::abs(cached_r_scalar_));
			render_setting_tooltip("Deviation from the vacuum condition R = 0, reported as a numerical sanity check on the current evaluation.");

			ImGui::Separator();
			ImGui::TextColored(ImVec4(0.6f, 0.9f, 1.0f, 1.0f), "Observer Kinematics");

			const double r_safe = std::max(cam.radius, 2.05 * params.mass);
			Metrics::KerrMetric<double> metric(params.mass, params.spin, 1.0, 1.0);
			Core::FourVector<double> obs_pos(0.0, r_safe, cam.theta, cam.phi);
			const auto g = metric.metric_tensor(obs_pos);
			const double lapse_sq = -g(0, 0);
			const double lapse = std::sqrt(std::max(lapse_sq, 1e-30));
			const double gravitational_time_dilation = 1.0 / std::max(lapse, 1e-12);

			ImGui::Text("Lapse Function (alpha): %.6f", lapse);
			render_setting_tooltip("Ratio between proper time of a locally static observer and coordinate time. Approaches zero at the horizon.");
			ImGui::Text("Time Dilation vs Infinity: %.4fx", gravitational_time_dilation);
			render_setting_tooltip("How much slower a clock at this radius ticks compared to a distant observer at rest, for a static observer at this location.");

			if (std::abs(params.spin) > 1e-9) {
				const double omega_zamo = Metrics::compute_zamo_angular_velocity(metric, obs_pos);
				ImGui::Text("ZAMO Frame-Dragging Rate: %.6f rad/M", omega_zamo);
				render_setting_tooltip("Angular velocity a zero-angular-momentum observer is forced to co-rotate at due to Lense-Thirring frame dragging at this radius and latitude.");
			}

			const double keplerian_omega = std::sqrt(params.mass / std::pow(r_safe, 3.0));
			const double orbital_period = (keplerian_omega > 1e-12) ? (2.0 * std::numbers::pi_v<double> / keplerian_omega) : 0.0;
			const double escape_velocity_fraction_c = std::sqrt(std::min(2.0 * params.mass / r_safe, 1.0));

			ImGui::Text("Local Circular Orbital Period: %.4f M", orbital_period);
			render_setting_tooltip("Coordinate-time period of a Keplerian circular orbit at the current radius, using the weak-field approximation Omega = sqrt(M / r^3).");
			ImGui::Text("Newtonian Escape Velocity: %.4f c", escape_velocity_fraction_c);
			render_setting_tooltip("Escape speed estimate sqrt(2M / r) in units of the speed of light. Reaches c exactly at the Schwarzschild radius.");

			ImGui::Separator();
			ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.4f, 1.0f), "Proximity to Critical Radii");

			const double r_h = metric.outer_horizon_radius();
			const double r_isco = kerr_isco_radius(params.mass, params.spin);
			const auto photon_radii = Metrics::BardeenKerrShadow(params.mass, params.spin, cam.theta).photon_orbit_radii();

			auto proximity_row = [](const char* label, double observer_radius, double reference, ImVec4 warn_color) {
				const bool inside = observer_radius <= reference;
				ImGui::TextColored(inside ? warn_color : ImVec4(0.5f, 0.9f, 0.55f, 1.0f), "%s: %.4f M (observer %s)", label, reference, inside ? "inside" : "outside");
			};

			proximity_row("Event Horizon", cam.radius, r_h, ImVec4(1.0f, 0.25f, 0.25f, 1.0f));
			proximity_row("ISCO", cam.radius, r_isco, ImVec4(1.0f, 0.65f, 0.2f, 1.0f));
			proximity_row("Photon Orbit Band (outer)", cam.radius, photon_radii.second, ImVec4(1.0f, 0.85f, 0.3f, 1.0f));
			render_setting_tooltip("Marks whether the observer currently sits inside the horizon, the innermost stable circular orbit, or the unstable photon orbit band. Stable free-fall orbits require the observer to remain outside the ISCO.");

			ImGui::Separator();
			ImGui::TextColored(ImVec4(0.7f, 0.9f, 0.5f, 1.0f), "Simulation State");
			const auto snap = orchestrator.scheduler().snapshot();
			ImGui::Text("Logical Time: %.4f s", snap.logical_time);
			ImGui::Text("Tick Rate: %.2f Hz", snap.tick_rate_hz);
			ImGui::Text("Warp Factor: %.2fx", snap.warp_factor);
			ImGui::Text("Active Metric: %s", orchestrator.active_metric_name().c_str());
			ImGui::Text("Active Integrator: %s", orchestrator.active_integrator_name().c_str());
		}
		ImGui::End();
	}
};

}

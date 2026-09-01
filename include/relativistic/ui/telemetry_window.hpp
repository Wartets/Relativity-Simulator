#pragma once

#include <imgui.h>
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/core/riemann.hpp"
#include "relativistic/metrics/kerr.hpp"

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

public:
	void render(const Orchestrator::SimulationOrchestrator<1024>& orchestrator) {
		if (!is_open_) return;

		ImGui::SetNextWindowPos(ImVec2(15.0f, 705.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(315.0f, 335.0f), ImGuiCond_FirstUseEver);

		if (ImGui::Begin("Telemetry & Invariants", &is_open_)) {
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

			ImGui::Separator();
			ImGui::Text("Hamiltonian Constraint Residual: %.6e", std::abs(cached_r_scalar_));

			ImGui::Separator();
			const auto snap = orchestrator.scheduler().snapshot();
			ImGui::Text("Logical Time: %.4f s", snap.logical_time);
			ImGui::Text("Tick Rate: %.2f Hz", snap.tick_rate_hz);
		}
		ImGui::End();
	}
};

}

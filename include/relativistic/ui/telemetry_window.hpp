#pragma once

#include <imgui.h>
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/core/riemann.hpp"
#include "relativistic/metrics/kerr.hpp"

namespace Relativistic::UI {

class TelemetryWindow {
private:
	bool is_open_{true};

public:
	void render(const Orchestrator::SimulationOrchestrator<1024>& orchestrator) {
		if (!is_open_) return;

		if (ImGui::Begin("Telemetry & Invariants", &is_open_)) {
			const auto& params = orchestrator.parameters();
			
			Metrics::KerrMetric<double> metric(params.mass, params.spin, 1.0, 1.0);
			Core::FourVector<double> obs_pos(0.0, 10.0, 1.57079632679, 0.0);
			
			auto R_tensor = Core::RiemannComputer<Core::DerivativeOrder::FourthOrder, Metrics::KerrMetric<double>, double>::compute_riemann(metric, obs_pos);
			auto g = metric.metric_tensor(obs_pos);
			auto inv_g = metric.inverse_metric(obs_pos);
			
			auto ricci = Core::RiemannComputer<Core::DerivativeOrder::FourthOrder, Metrics::KerrMetric<double>, double>::compute_ricci_tensor(R_tensor);
			double R_scalar = Core::RiemannComputer<Core::DerivativeOrder::FourthOrder, Metrics::KerrMetric<double>, double>::compute_ricci_scalar(ricci, inv_g);
			double K1 = Core::RiemannComputer<Core::DerivativeOrder::FourthOrder, Metrics::KerrMetric<double>, double>::compute_kretschmann_invariant(R_tensor, g, inv_g);

			ImGui::Text("Observer Position: r = %.4f, theta = %.4f", obs_pos(1), obs_pos(2));
			ImGui::Separator();
			ImGui::Text("Ricci Scalar Curvature (R): %.6e", R_scalar);
			ImGui::Text("Kretschmann Invariant (K1): %.6e", K1);
			
			ImGui::Separator();
			ImGui::Text("Hamiltonian Constraint Residual: %.6e", std::abs(R_scalar));
			
			ImGui::Separator();
			const auto snap = orchestrator.scheduler().snapshot();
			ImGui::Text("Logical Time: %.4f s", snap.logical_time);
			ImGui::Text("Tick Rate: %.2f Hz", snap.tick_rate_hz);
		}
		ImGui::End();
	}
};

}

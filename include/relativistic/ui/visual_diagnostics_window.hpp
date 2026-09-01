#pragma once

#include <imgui.h>
#include <implot.h>
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/core/riemann.hpp"
#include "relativistic/metrics/kerr.hpp"
#include <vector>
#include <array>
#include <cmath>

namespace Relativistic::UI {

class VisualDiagnosticsWindow {
private:
	bool is_open_{true};
	Orchestrator::SimulationOrchestrator<1024>& orchestrator_;
	std::vector<double> r_history_{};
	std::vector<double> kretschmann_history_{};
	std::vector<double> ricci_history_{};

public:
	explicit VisualDiagnosticsWindow(Orchestrator::SimulationOrchestrator<1024>& orchestrator)
		: orchestrator_(orchestrator) {}

	void render() {
		if (!is_open_) return;

		ImGui::SetNextWindowPos(ImVec2(1480.0f, 750.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(425.0f, 290.0f), ImGuiCond_FirstUseEver);

		if (ImGui::Begin("Curvature Diagnostics & Tensor Inspector", &is_open_)) {
			const auto& cam = orchestrator_.camera();
			const auto& params = orchestrator_.parameters();

			Metrics::KerrMetric<double> metric(params.mass, params.spin, 1.0, 1.0);
			Core::FourVector<double> pos(0.0, cam.radius, cam.theta, cam.phi);

			const auto g = metric.metric_tensor(pos);
			const auto inv_g = metric.inverse_metric(pos);
			const auto R_tensor = Core::RiemannComputer<Core::DerivativeOrder::FourthOrder, Metrics::KerrMetric<double>, double>::compute_riemann(metric, pos);
			const auto ricci = Core::RiemannComputer<Core::DerivativeOrder::FourthOrder, Metrics::KerrMetric<double>, double>::compute_ricci_tensor(R_tensor);
			const double R_scalar = Core::RiemannComputer<Core::DerivativeOrder::FourthOrder, Metrics::KerrMetric<double>, double>::compute_ricci_scalar(ricci, inv_g);
			const double K1 = Core::RiemannComputer<Core::DerivativeOrder::FourthOrder, Metrics::KerrMetric<double>, double>::compute_kretschmann_invariant(R_tensor, g, inv_g);

			if (r_history_.size() >= 120) {
				r_history_.erase(r_history_.begin());
				kretschmann_history_.erase(kretschmann_history_.begin());
				ricci_history_.erase(ricci_history_.begin());
			}

			r_history_.push_back(cam.radius);
			kretschmann_history_.push_back(std::abs(K1) + 1e-30);
			ricci_history_.push_back(std::abs(R_scalar));

			ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "Local Metric Tensor g_uv:");
			if (ImGui::BeginTable("MetricTensorTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
				for (int row = 0; row < 4; ++row) {
					ImGui::TableNextRow();
					for (int col = 0; col < 4; ++col) {
						ImGui::TableSetColumnIndex(col);
						ImGui::Text("%.4f", g(row, col));
					}
				}
				ImGui::EndTable();
			}

			ImGui::Spacing();
			ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Invariants & Curvature Quantities:");
			ImGui::Text("Kretschmann Invariant (R^abcd R_abcd): %.6e", K1);
			ImGui::Text("Ricci Curvature Scalar (R):            %.6e", R_scalar);
			ImGui::Text("Outer Horizon Radius (r+):             %.4f M", metric.outer_horizon_radius());
			ImGui::Text("Inner Horizon Radius (r-):             %.4f M", metric.inner_horizon_radius());
			ImGui::Text("Outer Ergosphere Radius (r_ergo):      %.4f M", metric.outer_ergosphere_radius(cam.theta));

			if (ImPlot::BeginPlot("Curvature Profile Along Path", ImVec2(-1, 200))) {
				ImPlot::SetupAxes("Sample Step", "Kretschmann Value (log10)");
				std::vector<double> x_indices(kretschmann_history_.size());
				std::vector<double> log_k(kretschmann_history_.size());
				for (size_t i = 0; i < kretschmann_history_.size(); ++i) {
					x_indices[i] = static_cast<double>(i);
					log_k[i] = std::log10(kretschmann_history_[i]);
				}
				ImPlot::PlotLine("log10(K1)", x_indices.data(), log_k.data(), static_cast<int>(x_indices.size()));
				ImPlot::EndPlot();
			}
		}
		ImGui::End();
	}
};

}

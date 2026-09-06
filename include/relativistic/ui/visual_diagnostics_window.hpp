#pragma once

#include <imgui.h>
#include <implot.h>
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/core/riemann.hpp"
#include "relativistic/core/tensor_ops.hpp"
#include "relativistic/metrics/kerr.hpp"
#include "relativistic/metrics/bardeen_shadow.hpp"
#include "relativistic/ui/tooltip_utils.hpp"
#include <array>
#include <cmath>
#include <numbers>
#include <algorithm>

namespace Relativistic::UI {

class VisualDiagnosticsWindow {
private:
	bool is_open_{true};
	Orchestrator::SimulationOrchestrator<1024>& orchestrator_;

	double last_diag_eval_{-1.0};
	double last_mass_{-1.0};
	double last_spin_{-1.0};

	Core::MetricTensor<double> cached_metric_{};
	double cached_kretschmann_{0.0};
	double cached_ricci_scalar_{0.0};
	double cached_metric_determinant_{0.0};

	static constexpr size_t kProfileSamples = 48;
	std::array<double, kProfileSamples> profile_radius_{};
	std::array<double, kProfileSamples> profile_log_kretschmann_{};
	std::array<double, kProfileSamples> profile_log_ricci_{};
	bool profile_valid_{false};
	double profile_mass_{-1.0};
	double profile_spin_{-1.0};

	void refresh_radial_profile(double mass, double spin) {
		Metrics::KerrMetric<double> metric(mass, spin, 1.0, 1.0);
		const double r_h = metric.outer_horizon_radius();
		const double r_min = std::max(r_h * 1.05, mass * 0.05);
		const double r_max = std::max(r_min * 2.0, 60.0 * mass);
		const bool near_schwarzschild = std::abs(spin) < 1e-9 * std::max(mass, 1e-6);

		for (size_t i = 0; i < kProfileSamples; ++i) {
			const double t = static_cast<double>(i) / static_cast<double>(kProfileSamples - 1);
			const double r = r_min + t * (r_max - r_min);
			profile_radius_[i] = r;

			if (near_schwarzschild) {
				const double k1 = 48.0 * mass * mass / std::pow(r, 6.0);
				profile_log_kretschmann_[i] = std::log10(std::max(k1, 1e-30));
				profile_log_ricci_[i] = std::log10(1e-30);
			} else {
				Core::FourVector<double> pos(0.0, r, std::numbers::pi_v<double> * 0.5, 0.0);
				const auto R_tensor = Core::RiemannComputer<Core::DerivativeOrder::FourthOrder, Metrics::KerrMetric<double>, double>::compute_riemann(metric, pos);
				const auto g = metric.metric_tensor(pos);
				const auto inv_g = metric.inverse_metric(pos);
				const auto ricci = Core::RiemannComputer<Core::DerivativeOrder::FourthOrder, Metrics::KerrMetric<double>, double>::compute_ricci_tensor(R_tensor);
				const double r_scalar = Core::RiemannComputer<Core::DerivativeOrder::FourthOrder, Metrics::KerrMetric<double>, double>::compute_ricci_scalar(ricci, inv_g);
				const double k1 = Core::RiemannComputer<Core::DerivativeOrder::FourthOrder, Metrics::KerrMetric<double>, double>::compute_kretschmann_invariant(R_tensor, g, inv_g);
				profile_log_kretschmann_[i] = std::log10(std::max(std::abs(k1), 1e-30));
				profile_log_ricci_[i] = std::log10(std::max(std::abs(r_scalar), 1e-30));
			}
		}

		profile_valid_ = true;
		profile_mass_ = mass;
		profile_spin_ = spin;
	}

public:
	explicit VisualDiagnosticsWindow(Orchestrator::SimulationOrchestrator<1024>& orchestrator)
		: orchestrator_(orchestrator) {}

	[[nodiscard]] bool& open_state() noexcept {
		return is_open_;
	}

	void render() {
		if (!is_open_) return;

		ImGui::SetNextWindowPos(ImVec2(1480.0f, 750.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(460.0f, 620.0f), ImGuiCond_FirstUseEver);

		if (!ImGui::Begin("Curvature Diagnostics & Tensor Inspector", &is_open_)) {
			ImGui::End();
			return;
		}

		const auto& cam = orchestrator_.camera();
		const auto& params = orchestrator_.parameters();
		const double cur_time = ImGui::GetTime();

		const double r_safe = std::max(cam.radius, 2.05 * params.mass);
		Metrics::KerrMetric<double> metric(params.mass, params.spin, 1.0, 1.0);
		Core::FourVector<double> pos(0.0, r_safe, cam.theta, cam.phi);

		if (cur_time - last_diag_eval_ > 0.15 || params.mass != last_mass_ || params.spin != last_spin_) {
			last_diag_eval_ = cur_time;
			last_mass_ = params.mass;
			last_spin_ = params.spin;

			cached_metric_ = metric.metric_tensor(pos);
			cached_metric_determinant_ = Core::determinant_4x4(cached_metric_);

			if (std::abs(params.spin) < 1e-12) {
				cached_kretschmann_ = 48.0 * params.mass * params.mass / std::pow(r_safe, 6.0);
				cached_ricci_scalar_ = 0.0;
			} else {
				const auto R_tensor = Core::RiemannComputer<Core::DerivativeOrder::FourthOrder, Metrics::KerrMetric<double>, double>::compute_riemann(metric, pos);
				const auto inv_g = metric.inverse_metric(pos);
				const auto ricci = Core::RiemannComputer<Core::DerivativeOrder::FourthOrder, Metrics::KerrMetric<double>, double>::compute_ricci_tensor(R_tensor);
				cached_ricci_scalar_ = Core::RiemannComputer<Core::DerivativeOrder::FourthOrder, Metrics::KerrMetric<double>, double>::compute_ricci_scalar(ricci, inv_g);
				cached_kretschmann_ = Core::RiemannComputer<Core::DerivativeOrder::FourthOrder, Metrics::KerrMetric<double>, double>::compute_kretschmann_invariant(R_tensor, cached_metric_, inv_g);
			}
		}

		if (!profile_valid_ || profile_mass_ != params.mass || profile_spin_ != params.spin) {
			refresh_radial_profile(params.mass, params.spin);
		}

		ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Active Model: %s", orchestrator_.active_metric_name().c_str());
		ImGui::Text("Observer Position: r = %.4f M, theta = %.4f, phi = %.4f", cam.radius, cam.theta, cam.phi);
		render_setting_tooltip("Current camera position in geometrized Boyer-Lindquist-style coordinates, in units of central mass M. Curvature quantities below are evaluated at this point.");

		ImGui::Separator();
		ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Local Curvature Invariants");
		ImGui::Text("Kretschmann Invariant (K = R_abcd R^abcd): %.6e", cached_kretschmann_);
		render_setting_tooltip("Quadratic curvature scalar independent of coordinate choice. Diverges as r approaches 0, giving a coordinate-free measure of tidal stress at the observer's location.");
		ImGui::Text("Ricci Scalar Curvature (R): %.6e", cached_ricci_scalar_);
		render_setting_tooltip("Trace of the Ricci tensor. Vanishes identically in vacuum solutions such as Schwarzschild and Kerr; a nonzero value here reflects residual numerical error from the finite-difference Christoffel evaluation.");
		ImGui::Text("Metric Determinant (det g): %.6e", cached_metric_determinant_);
		render_setting_tooltip("Determinant of the local metric tensor. Its square root enters the invariant spacetime volume element used in conserved-flux integrals.");
		ImGui::Text("Hamiltonian Constraint Residual: %.6e", std::abs(cached_ricci_scalar_));
		render_setting_tooltip("Vacuum Einstein equations require R = 0 everywhere outside matter. This residual reports how far the numerical evaluation deviates from that constraint at the current step.");

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.6f, 1.0f, 0.6f, 1.0f), "Horizon and Orbit Structure");

		const double r_plus = metric.outer_horizon_radius();
		const double r_minus = metric.inner_horizon_radius();
		const double r_ergo = metric.outer_ergosphere_radius(cam.theta);
		const double a_star = (params.mass > 1e-12) ? (params.spin / params.mass) : 0.0;

		ImGui::Text("Outer Horizon Radius (r+): %.4f M", r_plus);
		render_setting_tooltip("Boyer-Lindquist radius of the event horizon. Nothing, including light, escapes from inside this surface.");
		ImGui::Text("Inner Horizon Radius (r-): %.4f M", r_minus);
		render_setting_tooltip("Cauchy horizon of the Kerr solution. Only relevant inside r+; classically unstable and not physically traversable in realistic collapse.");
		ImGui::Text("Outer Ergosphere Radius: %.4f M", r_ergo);
		render_setting_tooltip("Boundary of the ergoregion at the observer's polar angle. Inside it, no observer can remain static relative to infinity due to frame dragging, though escape is still possible.");
		ImGui::Text("Spin Parameter (a* = a/M): %.4f", a_star);
		if (metric.is_extremal()) {
			ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "Extremal Kerr (|a| = M)");
		} else if (metric.is_hyperextremal()) {
			ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Hyperextremal: naked singularity (|a| > M)");
		} else {
			ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.5f, 1.0f), "Subextremal black hole (|a| < M)");
		}
		render_setting_tooltip("Dimensionless spin. Physical black holes must satisfy |a*| <= 1; values above this bound describe a naked singularity with no horizon.");

		if (!metric.is_hyperextremal()) {
			const double horizon_area = 4.0 * std::numbers::pi_v<double> * (r_plus * r_plus + params.spin * params.spin);
			const double surface_gravity = (r_minus < r_plus) ? ((r_plus - r_minus) / (2.0 * (r_plus * r_plus + params.spin * params.spin))) : 0.0;
			const double hawking_temperature = surface_gravity / (2.0 * std::numbers::pi_v<double>);
			const double bekenstein_entropy = horizon_area / 4.0;
			const double omega_horizon = (r_plus * r_plus + params.spin * params.spin > 1e-12) ? (params.spin / (r_plus * r_plus + params.spin * params.spin)) : 0.0;
			const double irreducible_mass = std::sqrt(horizon_area / (16.0 * std::numbers::pi_v<double>));

			ImGui::Text("Horizon Area: %.4f M^2", horizon_area);
			render_setting_tooltip("Two-surface area of the outer horizon in geometrized units. By Hawking's area theorem, this cannot decrease classically.");
			ImGui::Text("Surface Gravity (kappa): %.6f 1/M", surface_gravity);
			render_setting_tooltip("Redshifted acceleration a static observer would need to hover just outside the horizon. Constant over the horizon by the zeroth law of black hole mechanics.");
			ImGui::Text("Hawking Temperature: %.6e 1/M", hawking_temperature);
			render_setting_tooltip("Semiclassical black hole temperature T_H = kappa / 2*pi in units where hbar = c = k_B = 1. Astrophysical black holes have temperatures far below the cosmic microwave background.");
			ImGui::Text("Bekenstein-Hawking Entropy: %.4f M^2", bekenstein_entropy);
			render_setting_tooltip("Horizon entropy S = A / 4 in Planck units. Encodes the holographic information content associated with the horizon area.");
			ImGui::Text("Horizon Angular Velocity (Omega_H): %.6f 1/M", omega_horizon);
			render_setting_tooltip("Angular velocity at which the horizon itself rotates. Frame dragging forces anything inside the ergosphere toward this rotation rate.");
			ImGui::Text("Irreducible Mass: %.4f M", irreducible_mass);
			render_setting_tooltip("Portion of the mass that cannot be extracted by any classical process, such as the Penrose process. Spin-down radiates energy down toward this floor.");
		}

		if (std::abs(a_star) < 0.999999) {
			const auto shadow_radii = Metrics::BardeenKerrShadow(params.mass, params.spin, cam.theta).photon_orbit_radii();
			ImGui::Text("Photon Orbit Radii: %.4f M to %.4f M", shadow_radii.first, shadow_radii.second);
			render_setting_tooltip("Radial range spanned by unstable circular photon orbits, from retrograde to prograde. Light launched tangentially near these radii can circle the black hole multiple times before escaping or falling in.");
		}

		ImGui::Separator();
		ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.9f, 1.0f), "Simulation Clock");
		const auto snap = orchestrator_.scheduler().snapshot();
		ImGui::Text("Logical Time: %.4f s | Tick Rate: %.2f Hz", snap.logical_time, snap.tick_rate_hz);
		ImGui::Text("Integrator: %s", orchestrator_.active_integrator_name().c_str());

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.3f, 1.0f), "Radial Curvature Profile (equatorial plane)");
		render_setting_tooltip("Curvature invariants sampled along the equatorial plane from just outside the horizon out to 60 M, independent of the current camera position. This curve reflects the mass and spin parameters, not simulation time, so it updates only when those parameters change.");

		if (ImPlot::BeginPlot("Curvature vs Radius", ImVec2(-1, 220))) {
			ImPlot::SetupAxes("Radius r (M)", "log10(Invariant)");
			ImPlot::PlotLine("log10(Kretschmann)", profile_radius_.data(), profile_log_kretschmann_.data(), static_cast<int>(kProfileSamples));
			if (std::abs(params.spin) >= 1e-12) {
				ImPlot::PlotLine("log10(|Ricci|)", profile_radius_.data(), profile_log_ricci_.data(), static_cast<int>(kProfileSamples));
			}
			const double marker_x[2] = {r_plus, r_plus};
			const double marker_y[2] = {
				*std::min_element(profile_log_kretschmann_.begin(), profile_log_kretschmann_.end()),
				*std::max_element(profile_log_kretschmann_.begin(), profile_log_kretschmann_.end())
			};
			ImPlot::PlotLine("Horizon", marker_x, marker_y, 2);
			ImPlot::EndPlot();
		}

		ImGui::Spacing();
		ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "Local Metric Tensor g_uv:");
		render_setting_tooltip("Covariant metric tensor components evaluated at the observer's current spacetime position, in Boyer-Lindquist-style coordinates (t, r, theta, phi).");
		if (ImGui::BeginTable("MetricTensorTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
			for (int row = 0; row < 4; ++row) {
				ImGui::TableNextRow();
				for (int col = 0; col < 4; ++col) {
					ImGui::TableSetColumnIndex(col);
					ImGui::Text("%.4f", cached_metric_(row, col));
				}
			}
			ImGui::EndTable();
		}

		ImGui::End();
	}
};

}

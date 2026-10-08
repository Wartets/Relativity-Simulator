#pragma once

#include <imgui.h>
#include <implot.h>
#include "relativistic/optics/geodesic_ray_probe.hpp"
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/ui/tooltip_utils.hpp"
#include "relativistic/units/unit_system.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <numbers>
#include <string>
#include <vector>

namespace Relativistic::UI {

class RayProbePanel {
public:
	static constexpr size_t kHistoryCapacity = 256;

	[[nodiscard]] int source() const noexcept {
		return source_;
	}

	[[nodiscard]] bool frozen() const noexcept {
		return frozen_;
	}

	void render(const Orchestrator::SimulationOrchestrator<1024>& orchestrator, const Optics::RayProbeResult& probe) {
		const char* sources[] = {"Cursor Over Viewport (Sticky)", "Screen Center"};
		ImGui::SetNextItemWidth(260.0f);
		ImGui::Combo("Probe Source", &source_, sources, IM_ARRAYSIZE(sources));
		render_setting_tooltip("Chooses which pixel of the primary viewport is traced. The cursor mode keeps the last hovered pixel when the mouse leaves the viewport so the values can be read while working in this window.");
		ImGui::SameLine();
		ImGui::Checkbox("Freeze Probe", &frozen_);
		render_setting_tooltip("Stops updating the probed pixel and keeps the displayed geodesic until unfrozen.");

		ImGui::TextDisabled("Tip: hover the viewport, then freeze the probe to inspect a single geodesic. The probe marker is drawn on the image.");

		if (!probe.valid) {
			const char* reason = (probe.termination == Optics::RayTermination::UnsupportedMetric)
				? "The active spacetime metric is not supported by the exact ray probe (supported: Minkowski, Schwarzschild, Kerr, Reissner-Nordstrom, Kerr-Newman, Schwarzschild-de Sitter)."
				: "No probe data yet. Move the cursor over the viewport and keep the simulation running.";
			ImGui::TextColored(ImVec4(1.0f, 0.65f, 0.3f, 1.0f), "%s", reason);
			return;
		}

		record(probe);

		const auto& params = orchestrator.parameters();
		const auto& prefs = orchestrator.unit_preferences();
		const double length_scale = orchestrator.constants_engine().length_scale();
		const double mass = std::max(params.mass, 1e-9);
		constexpr double critical_impact = 5.196152422706632;

		ImVec4 status_color(0.5f, 0.9f, 1.0f, 1.0f);
		if (probe.disk_hit) status_color = ImVec4(1.0f, 0.7f, 0.25f, 1.0f);
		else if (probe.termination == Optics::RayTermination::HorizonAbsorbed) status_color = ImVec4(1.0f, 0.35f, 0.3f, 1.0f);
		else if (probe.termination == Optics::RayTermination::StepBudgetExhausted) status_color = ImVec4(1.0f, 0.85f, 0.3f, 1.0f);

		ImGui::TextColored(status_color, "Termination: %s%s", probe.disk_hit ? "Accretion Disk (primary image), then " : "", Optics::ray_termination_name(probe.termination));
		render_setting_tooltip("Where the traced ray ends. When the ray crosses the equatorial disk plane inside the disk annulus, the first crossing defines the primary emission point.");

		if (ImGui::BeginTable("##RayProbeTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
			ImGui::TableSetupColumn("Quantity", ImGuiTableColumnFlags_WidthFixed, 230.0f);
			ImGui::TableSetupColumn("Value");
			ImGui::TableHeadersRow();

			row("Probed Pixel", number("(%.0f", probe.pixel_x) + ", " + number("%.0f)", probe.pixel_y), "Pixel of the render target selected by the probe.");
			row("Spectral Shift g", number("%.6f", probe.spectral_shift_g), "Ratio of observed to emitted frequency, g = nu_obs / nu_emit. Disk hits use the orbiting emitter, sky rays use the static emitter at infinity, horizon rays are infinitely redshifted.");
			row("Static Redshift 1/E", number("%.6f", probe.static_redshift_factor), "Frequency ratio between the observer frame and a static emitter at infinity along this ray.");
			row("Impact Parameter b = Lz / E", number("%.6f M", probe.impact_parameter / mass), "Axial impact parameter in units of the central mass.");
			row("Total Impact Parameter", number("%.6f M", probe.total_impact_parameter / mass) + "  (" + number("%.3f", probe.total_impact_parameter / mass / critical_impact) + " x Schwarzschild critical)", "sqrt(eta + xi^2) in units of M, compared with the Schwarzschild critical impact parameter 3 sqrt(3) M.");
			row("Carter Constant Q", number("%.6e", probe.carter_constant), "Separation constant of the latitudinal motion, Q = p_theta^2 + cos^2(theta) [ -a^2 E^2 + Lz^2 / sin^2(theta) ].");
			row("Reduced Carter eta = Q / E^2", number("%.6f", probe.carter_eta), "Energy-normalized Carter constant.");
			row("Energy E (Local Unit Frame)", number("%.6f", probe.energy), "Conserved photon energy -p_t, normalized so a zero-angular-momentum observer measures unit energy.");
			row("Angular Momentum Lz", number("%.6f", probe.angular_momentum_z), "Conserved axial angular momentum p_phi.");
			row("Integration Iterations", std::to_string(probe.iterations), "Number of integrator steps consumed before termination.");
			row("Affine Path Length", number("%.4f", probe.affine_length), "Integrated affine parameter along the traced ray.");
			row("Closest Approach", number("%.4f M", probe.minimum_radius / mass), "Minimum coordinate radius reached by the ray.");
			row("Disk Plane Crossings", std::to_string(probe.disk_crossings), "Number of equatorial disk crossings. Values above one indicate higher-order lensed images.");
			row("Emission r", Units::format_distance(probe.emission_r * length_scale, prefs.distance) + "  (" + number("%.4f M)", probe.emission_r / mass), "Radial coordinate of the emission point (disk crossing) or of the ray termination point.");
			row("Emission theta", Units::format_angle(probe.emission_theta, prefs.angle), "Colatitude of the emission point.");
			row("Emission phi", Units::format_angle(probe.emission_phi, prefs.angle), "Azimuth of the emission point.");
			if (probe.disk_hit) {
				row("Disk Emitted Temperature", Units::format_temperature(probe.disk_temperature_k, prefs.temperature), "Effective disk temperature at the crossing radius before spectral shifting.");
				row("Disk Observed Temperature", Units::format_temperature(probe.disk_temperature_k * probe.spectral_shift_g, prefs.temperature), "Temperature after multiplying by the spectral shift g.");
			}
			ImGui::EndTable();
		}

		render_path_plot(probe, mass);
		render_history_plot();
	}

private:
	static void row(const char* label, const std::string& value, const char* tip) {
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::TextUnformatted(label);
		render_setting_tooltip(tip);
		ImGui::TableSetColumnIndex(1);
		ImGui::TextUnformatted(value.c_str());
	}

	[[nodiscard]] static std::string number(const char* format, double value) {
		char buffer[96];
		std::snprintf(buffer, sizeof(buffer), format, value);
		return buffer;
	}

	void record(const Optics::RayProbeResult& probe) {
		const uint64_t signature = (static_cast<uint64_t>(probe.pixel_x) * 73856093ULL) ^ (static_cast<uint64_t>(probe.pixel_y) * 19349663ULL)
			^ (static_cast<uint64_t>(probe.iterations) * 83492791ULL) ^ static_cast<uint64_t>(std::abs(probe.spectral_shift_g) * 1.0e6);
		if (signature == last_signature_) return;
		last_signature_ = signature;
		history_index_.push_back(static_cast<double>(sample_counter_++));
		history_g_.push_back(probe.spectral_shift_g);
		history_b_.push_back(probe.total_impact_parameter);
		if (history_index_.size() > kHistoryCapacity) {
			history_index_.erase(history_index_.begin());
			history_g_.erase(history_g_.begin());
			history_b_.erase(history_b_.begin());
		}
	}

	static void ring(std::vector<double>& xs, std::vector<double>& ys, double radius) {
		constexpr size_t points = 129;
		xs.resize(points);
		ys.resize(points);
		for (size_t i = 0; i < points; ++i) {
			const double angle = 2.0 * std::numbers::pi_v<double> * static_cast<double>(i) / static_cast<double>(points - 1);
			xs[i] = radius * std::cos(angle);
			ys[i] = radius * std::sin(angle);
		}
	}

	void render_path_plot(const Optics::RayProbeResult& probe, double mass) const {
		if (probe.path_x.size() < 2) return;
		if (ImPlot::BeginPlot("Probed Ray Path (x-y Projection)", ImVec2(-1, 240), ImPlotFlags_Equal)) {
			ImPlot::SetupAxes("x (M)", "y (M)");
			std::vector<double> px(probe.path_x.size());
			std::vector<double> py(probe.path_y.size());
			for (size_t i = 0; i < px.size(); ++i) {
				px[i] = probe.path_x[i] / mass;
				py[i] = probe.path_y[i] / mass;
			}
			std::vector<double> rx;
			std::vector<double> ry;
			if (probe.horizon_radius > 0.0) {
				ring(rx, ry, probe.horizon_radius / mass);
				ImPlot::PlotLine("Horizon", rx.data(), ry.data(), static_cast<int>(rx.size()));
			}
			if (probe.disk_outer_radius > 0.0) {
				ring(rx, ry, probe.disk_inner_radius / mass);
				ImPlot::PlotLine("Disk Inner Edge", rx.data(), ry.data(), static_cast<int>(rx.size()));
				ring(rx, ry, probe.disk_outer_radius / mass);
				ImPlot::PlotLine("Disk Outer Edge", rx.data(), ry.data(), static_cast<int>(rx.size()));
			}
			ImPlot::PlotLine("Ray", px.data(), py.data(), static_cast<int>(px.size()));
			ImPlot::EndPlot();
		}
	}

	void render_history_plot() const {
		if (history_index_.size() < 2) return;
		if (ImPlot::BeginPlot("Probe History", ImVec2(-1, 160))) {
			ImPlot::SetupAxes("Probe Sample", "Value");
			ImPlot::SetupAxisLimits(ImAxis_X1, history_index_.front(), history_index_.back(), ImPlotCond_Always);
			ImPlot::PlotLine("g", history_index_.data(), history_g_.data(), static_cast<int>(history_index_.size()));
			ImPlot::PlotLine("Total b (M units x mass)", history_index_.data(), history_b_.data(), static_cast<int>(history_index_.size()));
			ImPlot::EndPlot();
		}
	}

	int source_{0};
	bool frozen_{false};
	std::vector<double> history_index_{};
	std::vector<double> history_g_{};
	std::vector<double> history_b_{};
	uint64_t last_signature_{0};
	uint64_t sample_counter_{0};
};

}

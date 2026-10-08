#pragma once

#include <imgui.h>
#include <implot.h>
#include "relativistic/core/constants.hpp"
#include "relativistic/magnetosphere/blandford_znajek.hpp"
#include "relativistic/optics/polarization_spectrum.hpp"
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/ui/hud/hud_linked_readouts.hpp"
#include "relativistic/ui/tooltip_utils.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <numbers>
#include <string>
#include <vector>

namespace Relativistic::UI {

class PolarizationPanel {
public:
	PolarizationPanel() {
		std::snprintf(export_path_.data(), export_path_.size(), "%s", "output/polarization/polarized_spectrum.csv");
	}

	void update(const Orchestrator::SimulationOrchestrator<1024>& orchestrator, const Optics::PolarizedPlasmaState<double>& spectrograph_plasma, double spectrograph_g) {
		const Optics::PolarizedSpectrumSettings desired = resolve_settings(orchestrator, spectrograph_plasma, spectrograph_g);
		if (!has_spectrum_ || !same_settings(desired, active_settings_)) {
			active_settings_ = desired;
			spectrum_ = Optics::PolarizedSpectrumSynthesizer::synthesize(active_settings_);
			has_spectrum_ = true;
			rebuild_plot_cache();
		}
		if (!spectrum_.empty()) {
			selected_wavelength_nm_ = std::clamp(selected_wavelength_nm_, spectrum_.wavelength_nm.front(), spectrum_.wavelength_nm.back());
		}
	}

	[[nodiscard]] HudPolarizationReadout hud_summary() const {
		HudPolarizationReadout readout;
		if (!has_spectrum_ || spectrum_.empty()) {
			return readout;
		}
		const auto summary = Optics::PolarizedSpectrumSynthesizer::summarize(spectrum_, selected_wavelength_nm_);
		readout.valid = summary.valid;
		readout.wavelength_nm = summary.wavelength_nm;
		readout.intensity = summary.intensity;
		readout.dolp = summary.dolp;
		readout.docp = summary.docp;
		readout.evpa_deg = summary.evpa_deg;
		return readout;
	}

	void render(const Orchestrator::SimulationOrchestrator<1024>& orchestrator) {
		render_source_controls(orchestrator);
		render_observation_controls();
		if (spectrum_.empty()) {
			ImGui::TextColored(ImVec4(1.0f, 0.65f, 0.3f, 1.0f), "No polarized spectrum is available for the current plasma state.");
			return;
		}
		render_statistics();
		render_plots();
		render_export();
	}

private:
	void rebuild_plot_cache() {
		const size_t count = spectrum_.wavelength_nm.size();
		double peak = 0.0;
		for (const double value : spectrum_.intensity) {
			peak = std::max(peak, value);
		}
		const double floor_value = (peak > 0.0) ? (peak * 1.0e-14) : 1.0e-300;
		intensity_plot_.resize(count);
		linear_flux_.resize(count);
		circular_flux_.resize(count);
		q_norm_.resize(count);
		u_norm_.resize(count);
		v_norm_.resize(count);
		for (size_t i = 0; i < count; ++i) {
			const double intensity = spectrum_.intensity[i];
			intensity_plot_[i] = std::max(intensity, floor_value);
			linear_flux_[i] = std::max(std::hypot(spectrum_.stokes_q[i], spectrum_.stokes_u[i]), floor_value);
			circular_flux_[i] = std::max(std::abs(spectrum_.stokes_v[i]), floor_value);
			const double inverse = (intensity > 1.0e-300) ? (1.0 / intensity) : 0.0;
			q_norm_[i] = spectrum_.stokes_q[i] * inverse;
			u_norm_[i] = spectrum_.stokes_u[i] * inverse;
			v_norm_[i] = spectrum_.stokes_v[i] * inverse;
		}
		plot_refit_ = true;
	}

	[[nodiscard]] static bool same_settings(const Optics::PolarizedSpectrumSettings& a, const Optics::PolarizedSpectrumSettings& b) noexcept {
		return a.model == b.model
			&& a.path_length_m == b.path_length_m
			&& a.doppler_factor == b.doppler_factor
			&& a.field_position_angle_rad == b.field_position_angle_rad
			&& a.wavelength_min_nm == b.wavelength_min_nm
			&& a.wavelength_max_nm == b.wavelength_max_nm
			&& a.sample_count == b.sample_count
			&& a.plasma.electron_density == b.plasma.electron_density
			&& a.plasma.ion_density == b.plasma.ion_density
			&& a.plasma.electron_temperature_k == b.plasma.electron_temperature_k
			&& a.plasma.magnetic_field_tesla == b.plasma.magnetic_field_tesla
			&& a.plasma.pitch_angle_rad == b.plasma.pitch_angle_rad
			&& a.plasma.power_law_index == b.plasma.power_law_index
			&& a.plasma.non_thermal_fraction == b.plasma.non_thermal_fraction
			&& a.plasma.gamma_min == b.plasma.gamma_min
			&& a.plasma.gamma_max == b.plasma.gamma_max;
	}

	[[nodiscard]] Optics::PolarizedSpectrumSettings resolve_settings(const Orchestrator::SimulationOrchestrator<1024>& orchestrator, const Optics::PolarizedPlasmaState<double>& spectrograph_plasma, double spectrograph_g) {
		constexpr double degrees = std::numbers::pi_v<double> / 180.0;
		Optics::PolarizedSpectrumSettings settings;
		settings.model = static_cast<Optics::PolarizationEmissionModel>(std::clamp(model_index_, 0, 2));
		settings.path_length_m = static_cast<double>(path_length_m_);
		settings.field_position_angle_rad = static_cast<double>(field_angle_deg_) * degrees;
		settings.wavelength_min_nm = static_cast<double>(wavelength_min_nm_);
		settings.wavelength_max_nm = std::max(static_cast<double>(wavelength_max_nm_), settings.wavelength_min_nm * 1.05);
		settings.sample_count = static_cast<size_t>(std::clamp(sample_count_, 16, 4096));

		if (link_mode_ == 1) {
			settings.plasma = spectrograph_plasma;
			settings.doppler_factor = std::max(spectrograph_g, 1.0e-3);
			status_message_ = "Plasma state and Doppler factor are driven by the Spectrum tab.";
			return settings;
		}

		if (link_mode_ == 2) {
			const auto& params = orchestrator.parameters();
			const Render::GpuJetProfile profile = params.jet.to_gpu_profile(params.mass, params.spin, orchestrator.constants_engine().length_scale());
			const Magnetosphere::BlandfordZnajekModel model(profile);
			const double inner = static_cast<double>(profile.inner_radius);
			const double outer = std::max(static_cast<double>(profile.outer_radius), inner * 1.01);
			const double radius = std::clamp(inner + static_cast<double>(jet_radius_fraction_) * (outer - inner), inner, outer);
			const double view = static_cast<double>(jet_view_angle_deg_) * degrees;
			const Magnetosphere::Vec3 photon{std::cos(view), std::sin(view), 0.0};
			const auto sample = model.sample_local(radius, static_cast<double>(jet_polar_angle_deg_) * degrees, photon);
			if (sample.valid) {
				jet_plasma_.electron_density = sample.electron_density * sample.weight;
				jet_plasma_.ion_density = jet_plasma_.electron_density;
				jet_plasma_.electron_temperature_k = std::max(sample.temperature_k, 1.0);
				jet_plasma_.magnetic_field_tesla = std::max(sample.field_tesla, 1.0e-12);
				jet_plasma_.pitch_angle_rad = sample.pitch_angle;
				jet_plasma_.power_law_index = static_cast<double>(profile.power_law_index);
				jet_plasma_.non_thermal_fraction = static_cast<double>(profile.non_thermal_fraction);
				jet_plasma_.gamma_min = static_cast<double>(profile.gamma_min);
				jet_doppler_ = sample.doppler;
				jet_radius_resolved_ = radius;
				status_message_ = params.jet.enabled
					? "Plasma sampled from the Blandford-Znajek magnetosphere at the selected point."
					: "Jet rendering is disabled in the Master Controls; the magnetosphere model is still sampled here.";
			} else {
				status_message_ = "The selected point lies outside the emitting jet volume; the previous sample is kept.";
			}
			settings.plasma = jet_plasma_;
			settings.doppler_factor = jet_doppler_;
			settings.model = static_cast<Optics::PolarizationEmissionModel>(std::clamp(static_cast<int>(profile.emission_model + 0.5f), 0, 2));
			return settings;
		}

		settings.plasma.electron_density = static_cast<double>(electron_density_);
		settings.plasma.ion_density = settings.plasma.electron_density;
		settings.plasma.electron_temperature_k = static_cast<double>(electron_temperature_k_);
		settings.plasma.magnetic_field_tesla = static_cast<double>(magnetic_field_tesla_);
		settings.plasma.pitch_angle_rad = static_cast<double>(pitch_angle_deg_) * degrees;
		settings.plasma.power_law_index = static_cast<double>(power_law_index_);
		settings.plasma.non_thermal_fraction = static_cast<double>(non_thermal_fraction_);
		settings.plasma.gamma_min = static_cast<double>(gamma_min_);
		settings.plasma.gamma_max = static_cast<double>(gamma_max_);
		settings.doppler_factor = static_cast<double>(doppler_factor_);
		status_message_ = "Plasma state is edited manually.";
		return settings;
	}

	void render_source_controls(const Orchestrator::SimulationOrchestrator<1024>& orchestrator) {
		ImGui::TextColored(ImVec4(0.5f, 0.85f, 1.0f, 1.0f), "Plasma Source");
		const char* links[] = {"Manual Plasma", "Spectrum Tab Plasma (Linked)", "Jet Magnetosphere Sample (Linked)"};
		ImGui::SetNextItemWidth(300.0f);
		ImGui::Combo("Plasma Source", &link_mode_, links, IM_ARRAYSIZE(links));
		render_setting_tooltip("Manual edits every plasma parameter here. The linked modes take the plasma state from the Spectrum tab or sample the Blandford-Znajek jet model at a chosen point, so the polarization always follows the other widgets.");

		const bool model_locked = (link_mode_ == 2);
		if (model_locked) {
			ImGui::BeginDisabled(true);
		}
		const char* models[] = {"Non-Thermal Power Law", "Thermal Maxwell-Juttner", "Hybrid (Thermal + Non-Thermal)"};
		ImGui::SetNextItemWidth(300.0f);
		ImGui::Combo("Emission Model", &model_index_, models, IM_ARRAYSIZE(models));
		if (model_locked) {
			ImGui::EndDisabled();
		}
		render_setting_tooltip("Selects which electron population produces the polarized synchrotron emission. In jet-linked mode the model follows the jet emission model.");

		if (link_mode_ == 0) {
			ImGui::SliderFloat("Electron Density (m^-3)", &electron_density_, 1.0e8f, 1.0e24f, "%.3e", ImGuiSliderFlags_Logarithmic);
			ImGui::SliderFloat("Electron Temperature (K)", &electron_temperature_k_, 1.0e6f, 1.0e13f, "%.3e", ImGuiSliderFlags_Logarithmic);
			ImGui::SliderFloat("Magnetic Field (T)", &magnetic_field_tesla_, 1.0e-6f, 1.0e5f, "%.3e", ImGuiSliderFlags_Logarithmic);
			ImGui::SliderFloat("Pitch Angle (deg)", &pitch_angle_deg_, 1.0f, 179.0f, "%.1f");
			ImGui::SliderFloat("Power Law Index (p)", &power_law_index_, 1.5f, 5.0f, "%.2f");
			ImGui::SliderFloat("Non-Thermal Fraction", &non_thermal_fraction_, 0.0f, 1.0f, "%.3f");
			ImGui::SliderFloat("Minimum Lorentz Factor", &gamma_min_, 1.0f, 1.0e4f, "%.1f", ImGuiSliderFlags_Logarithmic);
			ImGui::SliderFloat("Maximum Lorentz Factor", &gamma_max_, 100.0f, 1.0e8f, "%.3e", ImGuiSliderFlags_Logarithmic);
			ImGui::SliderFloat("Doppler Factor (g)", &doppler_factor_, 0.05f, 10.0f, "%.3f");
			render_setting_tooltip("Spectral shift g = nu_obs / nu_emit applied to the emitted plasma. The emissivity and absorptivity are evaluated at nu_obs / g and boosted by g^2 and g^-1.");
		} else if (link_mode_ == 2) {
			ImGui::SliderFloat("Jet Radial Position", &jet_radius_fraction_, 0.0f, 1.0f, "%.3f");
			render_setting_tooltip("Position between the inner and outer radius of the emitting jet volume.");
			ImGui::SliderFloat("Jet Polar Angle (deg)", &jet_polar_angle_deg_, 0.5f, 179.5f, "%.1f");
			ImGui::SliderFloat("Line-Of-Sight Angle (deg)", &jet_view_angle_deg_, 0.0f, 180.0f, "%.1f");
			render_setting_tooltip("Angle between the local radial direction and the photon direction toward the observer, used for the pitch angle and the Doppler factor.");
			ImGui::TextDisabled("Resolved radius: %.3f | Doppler g: %.4f | Pitch: %.1f deg", jet_radius_resolved_, jet_doppler_, jet_plasma_.pitch_angle_rad * 180.0 / std::numbers::pi_v<double>);
		} else {
			ImGui::TextDisabled("n_e: %.3e m^-3 | T_e: %.3e K | B: %.3e T | g: %.4f", active_settings_.plasma.electron_density, active_settings_.plasma.electron_temperature_k, active_settings_.plasma.magnetic_field_tesla, active_settings_.doppler_factor);
		}
		ImGui::TextDisabled("%s", status_message_.c_str());
		static_cast<void>(orchestrator);
	}

	void render_observation_controls() {
		ImGui::Separator();
		ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.35f, 1.0f), "Radiative Transfer Window");
		ImGui::SliderFloat("Path Length (m)", &path_length_m_, 1.0f, 1.0e15f, "%.3e", ImGuiSliderFlags_Logarithmic);
		render_setting_tooltip("Geometric depth of the emitting slab. Larger depths push the spectrum toward the optically thick regime where Faraday rotation and conversion dominate.");
		ImGui::SliderFloat("Field Position Angle (deg)", &field_angle_deg_, -90.0f, 90.0f, "%.1f");
		render_setting_tooltip("Rotates the Stokes reference frame, i.e. the projected magnetic field direction on the sky.");
		ImGui::SliderFloat("Minimum Wavelength (nm)", &wavelength_min_nm_, 1.0f, 1.0e7f, "%.1f", ImGuiSliderFlags_Logarithmic);
		ImGui::SliderFloat("Maximum Wavelength (nm)", &wavelength_max_nm_, 2.0f, 1.0e9f, "%.1f", ImGuiSliderFlags_Logarithmic);
		wavelength_max_nm_ = std::max(wavelength_max_nm_, wavelength_min_nm_ * 1.05f);
		ImGui::SliderInt("Spectral Samples", &sample_count_, 64, 2048);
		if (ImGui::Button("Optical (380-780 nm)")) {
			wavelength_min_nm_ = 380.0f;
			wavelength_max_nm_ = 780.0f;
		}
		ImGui::SameLine();
		if (ImGui::Button("Near IR (0.78-2.5 um)")) {
			wavelength_min_nm_ = 780.0f;
			wavelength_max_nm_ = 2500.0f;
		}
		ImGui::SameLine();
		if (ImGui::Button("Millimeter (0.3-3 mm)")) {
			wavelength_min_nm_ = 3.0e5f;
			wavelength_max_nm_ = 3.0e6f;
		}
		ImGui::SameLine();
		if (ImGui::Button("Radio (1-100 cm)")) {
			wavelength_min_nm_ = 1.0e7f;
			wavelength_max_nm_ = 1.0e9f;
		}
		render_setting_tooltip("Quick wavelength windows. Faraday rotation and the EVPA slope are most visible in the millimeter and radio windows.");
	}

	void render_statistics() {
		const auto summary = Optics::PolarizedSpectrumSynthesizer::summarize(spectrum_, selected_wavelength_nm_);
		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.4f, 0.95f, 0.5f, 1.0f), "Polarization Statistics");
		if (ImGui::BeginTable("##PolarizationStatistics", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
			ImGui::TableSetupColumn("Quantity", ImGuiTableColumnFlags_WidthFixed, 260.0f);
			ImGui::TableSetupColumn("Value");
			ImGui::TableHeadersRow();
			table_row("Selected Wavelength", number("%.4g nm", summary.wavelength_nm), "Wavelength marked by the draggable line in the plots.");
			table_row("Stokes I, Q, U, V", number("%.3e", summary.intensity) + ", " + number("%.3e", summary.stokes_q) + ", " + number("%.3e", summary.stokes_u) + ", " + number("%.3e", summary.stokes_v), "Spectral Stokes parameters per unit wavelength at the selected wavelength.");
			table_row("Linear Polarization (DoLP)", number("%.3f %%", 100.0 * summary.dolp), "sqrt(Q^2 + U^2) / I.");
			table_row("Circular Polarization (DoCP)", number("%.3f %%", 100.0 * summary.docp), "V / I. The sign gives the handedness.");
			table_row("Total Polarization (DoP)", number("%.3f %%", 100.0 * summary.dop), "sqrt(Q^2 + U^2 + V^2) / I.");
			table_row("EVPA chi", number("%.3f deg", summary.evpa_deg), "Electric vector position angle chi = 0.5 atan2(U, Q).");
			const double axial_ratio = (summary.ellipse_semi_major > 1.0e-12) ? (summary.ellipse_semi_minor / summary.ellipse_semi_major) : 0.0;
			table_row("Ellipse Semi-Axes (a, b)", number("%.4f", summary.ellipse_semi_major) + ", " + number("%.4f", summary.ellipse_semi_minor), "Normalized semi-axes of the polarization ellipse.");
			table_row("Axial Ratio b / a", number("%.4f", axial_ratio), "Zero for purely linear polarization, one for purely circular polarization.");
			table_row("Handedness", (summary.stokes_v > 0.0) ? std::string("Positive V") : ((summary.stokes_v < 0.0) ? std::string("Negative V") : std::string("Linear")), "Sign of Stokes V.");
			table_row("Peak DoLP", number("%.3f %%", 100.0 * spectrum_.peak_dolp) + " at " + number("%.4g nm", spectrum_.peak_dolp_wavelength_nm), "Highest linear polarization over the window.");
			table_row("Mean DoLP / DoCP", number("%.3f %%", 100.0 * spectrum_.mean_dolp) + " / " + number("%.3f %%", 100.0 * spectrum_.mean_docp), "Averages over the sampled window.");
			table_row("Rotation Measure", number("%.4e rad/m^2", spectrum_.rotation_measure_rad_m2), "Linear fit of the unwrapped EVPA versus lambda^2.");
			table_row("Spectral Index (nu^alpha)", number("%.4f", spectrum_.spectral_index), "Power-law slope of the specific intensity between the window edges.");
			ImGui::EndTable();
		}
	}

	void render_plots() {
		const double lower = spectrum_.wavelength_nm.front();
		const double upper = spectrum_.wavelength_nm.back();
		const ImPlotCond limit_condition = plot_refit_ ? ImPlotCond_Always : ImPlotCond_Once;
		const int count = static_cast<int>(spectrum_.wavelength_nm.size());

		if (ImPlot::BeginSubplots("##PolarizationSubplots", 3, 1, ImVec2(-1.0f, 640.0f), ImPlotSubplotFlags_LinkAllX)) {
			if (ImPlot::BeginPlot("Polarized Intensity")) {
				ImPlot::SetupAxes("Wavelength (nm)", "Spectral Flux (W/m^2/sr/m)");
				ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Log10);
				ImPlot::SetupAxisScale(ImAxis_Y1, ImPlotScale_Log10);
				ImPlot::SetupAxisLimits(ImAxis_X1, lower, upper, limit_condition);
				ImPlot::PlotLine("I", spectrum_.wavelength_nm.data(), intensity_plot_.data(), count);
				ImPlot::PlotLine("Linear Flux sqrt(Q^2+U^2)", spectrum_.wavelength_nm.data(), linear_flux_.data(), count);
				ImPlot::PlotLine("Circular Flux |V|", spectrum_.wavelength_nm.data(), circular_flux_.data(), count);
				ImPlot::DragLineX(0, &selected_wavelength_nm_, ImVec4(1.0f, 0.85f, 0.2f, 1.0f));
				ImPlot::EndPlot();
			}
			if (ImPlot::BeginPlot("Degrees Of Polarization")) {
				ImPlot::SetupAxes("Wavelength (nm)", "Fraction");
				ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Log10);
				ImPlot::SetupAxisLimits(ImAxis_X1, lower, upper, limit_condition);
				ImPlot::SetupAxisLimits(ImAxis_Y1, -1.05, 1.05, ImPlotCond_Once);
				ImPlot::PlotLine("DoLP", spectrum_.wavelength_nm.data(), spectrum_.dolp.data(), count);
				ImPlot::PlotLine("DoCP", spectrum_.wavelength_nm.data(), spectrum_.docp.data(), count);
				ImPlot::PlotLine("DoP", spectrum_.wavelength_nm.data(), spectrum_.dop.data(), count);
				ImPlot::DragLineX(1, &selected_wavelength_nm_, ImVec4(1.0f, 0.85f, 0.2f, 1.0f));
				ImPlot::EndPlot();
			}
			if (ImPlot::BeginPlot("Electric Vector Position Angle")) {
				ImPlot::SetupAxes("Wavelength (nm)", "EVPA chi (deg)");
				ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Log10);
				ImPlot::SetupAxisLimits(ImAxis_X1, lower, upper, limit_condition);
				ImPlot::SetupAxisLimits(ImAxis_Y1, -95.0, 95.0, ImPlotCond_Once);
				ImPlot::PlotLine("chi", spectrum_.wavelength_nm.data(), spectrum_.evpa_deg.data(), count);
				ImPlot::DragLineX(2, &selected_wavelength_nm_, ImVec4(1.0f, 0.85f, 0.2f, 1.0f));
				ImPlot::EndPlot();
			}
			ImPlot::EndSubplots();
		}
		plot_refit_ = false;
		selected_wavelength_nm_ = std::clamp(selected_wavelength_nm_, lower, upper);

		const auto summary = Optics::PolarizedSpectrumSynthesizer::summarize(spectrum_, selected_wavelength_nm_);
		ImGui::SliderFloat("##SelectedWavelengthSlider", &selected_slider_value_, 0.0f, 1.0f, "Selected Wavelength Position");
		selected_wavelength_nm_ = lower * std::pow(upper / lower, static_cast<double>(selected_slider_value_)) * ((std::abs(selected_slider_value_ - last_slider_value_) > 1.0e-6f) ? 1.0 : 0.0)
			+ ((std::abs(selected_slider_value_ - last_slider_value_) > 1.0e-6f) ? 0.0 : selected_wavelength_nm_);
		last_slider_value_ = selected_slider_value_;
		selected_slider_value_ = static_cast<float>(std::clamp(std::log(selected_wavelength_nm_ / lower) / std::max(std::log(upper / lower), 1.0e-12), 0.0, 1.0));
		last_slider_value_ = selected_slider_value_;
		render_setting_tooltip("Moves the analysis wavelength logarithmically. The draggable vertical line in the plots does the same.");

		if (ImPlot::BeginSubplots("##PolarizationEllipseSubplots", 1, 2, ImVec2(-1.0f, 320.0f))) {
			if (ImPlot::BeginPlot("Polarization Ellipse", ImVec2(-1.0f, -1.0f), ImPlotFlags_Equal)) {
				ImPlot::SetupAxes("Q direction", "U direction");
				ImPlot::SetupAxisLimits(ImAxis_X1, -1.15, 1.15, ImPlotCond_Always);
				ImPlot::SetupAxisLimits(ImAxis_Y1, -1.15, 1.15, ImPlotCond_Always);
				constexpr size_t points = 129;
				std::array<double, points> circle_x{};
				std::array<double, points> circle_y{};
				std::array<double, points> ellipse_x{};
				std::array<double, points> ellipse_y{};
				const double orientation = summary.ellipse_orientation_rad;
				const double handedness = (summary.stokes_v >= 0.0) ? 1.0 : -1.0;
				const double cos_psi = std::cos(orientation);
				const double sin_psi = std::sin(orientation);
				for (size_t i = 0; i < points; ++i) {
					const double angle = 2.0 * std::numbers::pi_v<double> * static_cast<double>(i) / static_cast<double>(points - 1);
					circle_x[i] = std::cos(angle);
					circle_y[i] = std::sin(angle);
					const double ex = summary.ellipse_semi_major * std::cos(angle);
					const double ey = handedness * summary.ellipse_semi_minor * std::sin(angle);
					ellipse_x[i] = ex * cos_psi - ey * sin_psi;
					ellipse_y[i] = ex * sin_psi + ey * cos_psi;
				}
				ImPlot::PlotLine("Unit Amplitude", circle_x.data(), circle_y.data(), static_cast<int>(points));
				ImPlot::PlotLine("Ellipse", ellipse_x.data(), ellipse_y.data(), static_cast<int>(points));
				const double axis_x[2] = {-summary.ellipse_semi_major * cos_psi * 1.1, summary.ellipse_semi_major * cos_psi * 1.1};
				const double axis_y[2] = {-summary.ellipse_semi_major * sin_psi * 1.1, summary.ellipse_semi_major * sin_psi * 1.1};
				ImPlot::PlotLine("EVPA Axis", axis_x, axis_y, 2);
				ImPlot::EndPlot();
			}
			if (ImPlot::BeginPlot("Normalized Stokes Q/I - U/I Trajectory", ImVec2(-1.0f, -1.0f), ImPlotFlags_Equal)) {
				ImPlot::SetupAxes("Q / I", "U / I");
				ImPlot::SetupAxisLimits(ImAxis_X1, -1.1, 1.1, ImPlotCond_Once);
				ImPlot::SetupAxisLimits(ImAxis_Y1, -1.1, 1.1, ImPlotCond_Once);
				constexpr size_t points = 129;
				std::array<double, points> circle_x{};
				std::array<double, points> circle_y{};
				for (size_t i = 0; i < points; ++i) {
					const double angle = 2.0 * std::numbers::pi_v<double> * static_cast<double>(i) / static_cast<double>(points - 1);
					circle_x[i] = std::cos(angle);
					circle_y[i] = std::sin(angle);
				}
				ImPlot::PlotLine("Full Polarization", circle_x.data(), circle_y.data(), static_cast<int>(points));
				ImPlot::PlotLine("Spectrum Trace", q_norm_.data(), u_norm_.data(), count);
				const double marker_x = (summary.intensity > 1.0e-300) ? summary.stokes_q / summary.intensity : 0.0;
				const double marker_y = (summary.intensity > 1.0e-300) ? summary.stokes_u / summary.intensity : 0.0;
				ImPlot::PlotScatter("Selected Wavelength", &marker_x, &marker_y, 1);
				ImPlot::EndPlot();
			}
			ImPlot::EndSubplots();
		}
		ImGui::TextDisabled("The ellipse is drawn in the (Q, U) frame with its major axis along the EVPA. Positive V is drawn counterclockwise. A circular trace in the Q/U plane indicates Faraday rotation.");
	}

	void render_export() {
		ImGui::Separator();
		ImGui::SetNextItemWidth(360.0f);
		ImGui::InputText("Export Path (.csv)", export_path_.data(), export_path_.size());
		ImGui::SameLine();
		if (ImGui::Button("Export Polarized Spectrum")) {
			std::error_code error_code;
			const std::filesystem::path path(export_path_.data());
			if (path.has_parent_path()) {
				std::filesystem::create_directories(path.parent_path(), error_code);
			}
			std::ofstream out(path, std::ios::trunc);
			if (!out.is_open()) {
				export_status_ = "Export failed: unable to open " + path.string();
			} else {
				out << "wavelength_nm,intensity,stokes_q,stokes_u,stokes_v,dolp,docp,dop,evpa_deg\n";
				out << std::setprecision(12);
				for (size_t i = 0; i < spectrum_.wavelength_nm.size(); ++i) {
					out << spectrum_.wavelength_nm[i] << ',' << spectrum_.intensity[i] << ',' << spectrum_.stokes_q[i] << ',' << spectrum_.stokes_u[i] << ',' << spectrum_.stokes_v[i] << ',' << spectrum_.dolp[i] << ',' << spectrum_.docp[i] << ',' << spectrum_.dop[i] << ',' << spectrum_.evpa_deg[i] << '\n';
				}
				export_status_ = out.good() ? ("Exported " + std::to_string(spectrum_.wavelength_nm.size()) + " samples to " + path.string()) : "Export failed: write error";
			}
		}
		render_setting_tooltip("Writes the full Stokes spectrum, polarization degrees and EVPA as a CSV table.");
		if (!export_status_.empty()) {
			const bool failed = export_status_.find("failed") != std::string::npos;
			ImGui::TextColored(failed ? ImVec4(1.0f, 0.4f, 0.3f, 1.0f) : ImVec4(0.4f, 0.95f, 0.5f, 1.0f), "%s", export_status_.c_str());
		}
	}

	static void table_row(const char* label, const std::string& value, const char* tip) {
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::TextUnformatted(label);
		render_setting_tooltip(tip);
		ImGui::TableSetColumnIndex(1);
		ImGui::TextUnformatted(value.c_str());
	}

	#if defined(__GNUC__) || defined(__clang__)
	[[gnu::format(printf, 1, 2)]]
#endif
	[[nodiscard]] static std::string number(const char* format, ...) {
		char buffer[96];
		va_list args;
		va_start(args, format);
		std::vsnprintf(buffer, sizeof(buffer), format, args);
		va_end(args);
		return buffer;
	}

	int link_mode_{1};
	int model_index_{0};
	float electron_density_{1.0e18f};
	float electron_temperature_k_{1.0e9f};
	float magnetic_field_tesla_{0.1f};
	float pitch_angle_deg_{60.0f};
	float power_law_index_{3.0f};
	float non_thermal_fraction_{0.01f};
	float gamma_min_{10.0f};
	float gamma_max_{1.0e5f};
	float doppler_factor_{1.0f};
	float path_length_m_{1.0e3f};
	float field_angle_deg_{0.0f};
	float wavelength_min_nm_{380.0f};
	float wavelength_max_nm_{780.0f};
	int sample_count_{400};
	float jet_radius_fraction_{0.15f};
	float jet_polar_angle_deg_{20.0f};
	float jet_view_angle_deg_{60.0f};
	double jet_radius_resolved_{0.0};
	double jet_doppler_{1.0};
	Optics::PolarizedPlasmaState<double> jet_plasma_{};

	double selected_wavelength_nm_{550.0};
	float selected_slider_value_{0.0f};
	float last_slider_value_{0.0f};
	bool plot_refit_{true};
	bool has_spectrum_{false};
	std::string status_message_{};

	Optics::PolarizedSpectrumSettings active_settings_{};
	Optics::PolarizedSpectrum spectrum_{};
	std::vector<double> intensity_plot_{};
	std::vector<double> linear_flux_{};
	std::vector<double> circular_flux_{};
	std::vector<double> q_norm_{};
	std::vector<double> u_norm_{};
	std::vector<double> v_norm_{};

	std::array<char, 260> export_path_{};
	std::string export_status_{};
};

}

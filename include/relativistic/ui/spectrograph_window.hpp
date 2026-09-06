#pragma once

#include <imgui.h>
#include <implot.h>
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/optics/spectrum.hpp"
#include "relativistic/optics/cie_observer.hpp"
#include "relativistic/optics/radiative_processes.hpp"
#include "relativistic/ui/tooltip_utils.hpp"
#include <vector>
#include <cmath>
#include <algorithm>

namespace Relativistic::UI {

class SpectrographWindow {
private:
	bool is_open_{true};
	int spectrum_type_{0};
	float temperature_k_{5778.0f};
	float doppler_shift_factor_{1.0f};
	float magnetic_field_tesla_{0.1f};
	float electron_density_{1e18f};
	bool link_temperature_to_disk_{false};
	float disk_sample_radius_isco_multiple_{1.5f};

	std::vector<double> wavelengths_nm_{};
	std::vector<double> intensities_{};
	std::vector<double> upper_band_{};
	std::vector<double> lower_band_{};
	Optics::ColorRGB perceived_color_{};

	static constexpr size_t kDiskProfileSamples = 60;
	std::vector<double> disk_radius_{};
	std::vector<double> disk_temperature_k_{};

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

	void recompute_disk_profile(double mass, double spin) {
		disk_radius_.assign(kDiskProfileSamples, 0.0);
		disk_temperature_k_.assign(kDiskProfileSamples, 0.0);

		const double r_isco = std::max(kerr_isco_radius(mass, spin), 1e-6);
		const double r_outer = 24.0 * std::max(mass, 1e-6);

		for (size_t i = 0; i < kDiskProfileSamples; ++i) {
			const double t = static_cast<double>(i) / static_cast<double>(kDiskProfileSamples - 1);
			const double r = r_isco + t * (r_outer - r_isco);
			const double ratio = r_isco / r;
			const double t_norm = std::pow(ratio, 0.75) * std::pow(std::max(1.0 - std::sqrt(ratio), 0.0), 0.25);
			disk_radius_[i] = r;
			disk_temperature_k_[i] = 18000.0 * t_norm + 1200.0;
		}
	}

	void recompute_spectrum() {
		wavelengths_nm_.resize(400);
		intensities_.resize(400);
		upper_band_.resize(400);
		lower_band_.resize(400);

		Optics::ContinuousSpectrum<double> base_spec;
		if (spectrum_type_ == 0) {
			base_spec = Optics::ContinuousSpectrum<double>::make_blackbody(static_cast<double>(temperature_k_));
		} else if (spectrum_type_ == 1) {
			base_spec = Optics::ContinuousSpectrum<double>::make_synchrotron(-0.7, 1e-12);
		} else {
			base_spec = Optics::ContinuousSpectrum<double>::make_monochromatic(550e-9, 1.0);
		}

		const auto shifted_spec = base_spec.transform_doppler(static_cast<double>(doppler_shift_factor_));
		perceived_color_ = Optics::CIE1931Observer::spectrum_to_srgb(shifted_spec);

		for (size_t i = 0; i < 400; ++i) {
			const double wl_nm = 380.0 + static_cast<double>(i);
			const double wl_m = wl_nm * 1e-9;
			const double val = shifted_spec.sample_radiance(wl_m);
			const double unc = 0.08 * val;

			wavelengths_nm_[i] = wl_nm;
			intensities_[i] = val;
			upper_band_[i] = val + unc;
			lower_band_[i] = std::max(0.0, val - unc);
		}
	}

	[[nodiscard]] static double peak_wavelength_nm(double temperature_k) noexcept {
		if (temperature_k <= 0.0) return 0.0;
		constexpr double wien_constant_nm_k = 2.897771955e6;
		return wien_constant_nm_k / temperature_k;
	}

public:
	SpectrographWindow() {
		recompute_spectrum();
	}

	[[nodiscard]] bool& open_state() noexcept {
		return is_open_;
	}

	void render(const Orchestrator::SimulationOrchestrator<1024>& orchestrator) {
		if (!is_open_) return;

		ImGui::SetNextWindowPos(ImVec2(340.0f, 750.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(600.0f, 460.0f), ImGuiCond_FirstUseEver);

		if (ImGui::Begin("Radiative Transfer & Spectrograph Monitor", &is_open_)) {
			ImGui::TextWrapped("Standalone spectral synthesis laboratory. It previews emission physics (blackbody, synchrotron, disk thermal profile) independently of the live render pipeline for performance reasons; adjusting it here does not alter the viewport, but the disk profile below reads live mass and spin from the active simulation.");
			ImGui::Separator();

			const char* types[] = {"Thermal Blackbody Emission", "Relativistic Synchrotron Power-Law", "Monochromatic Calibration Line"};
			if (ImGui::Combo("Emission Process", &spectrum_type_, types, IM_ARRAYSIZE(types))) {
				recompute_spectrum();
			}
			render_setting_tooltip("Selects the underlying radiative process used to synthesize the spectrum below. Blackbody suits stellar or thermal disk emission; synchrotron suits relativistic jets and hot coronae.");

			if (spectrum_type_ == 0) {
				ImGui::Checkbox("Link Temperature To Accretion Disk Profile", &link_temperature_to_disk_);
				render_setting_tooltip("When enabled, the blackbody temperature below is sampled from the live disk temperature profile at the chosen radius instead of being set manually.");
				if (link_temperature_to_disk_) {
					ImGui::SliderFloat("Sample Radius (ISCO multiples)", &disk_sample_radius_isco_multiple_, 1.0f, 16.0f, "%.2fx");
				} else {
					if (ImGui::SliderFloat("Temperature (K)", &temperature_k_, 500.0f, 50000.0f, "%.0f K")) {
						recompute_spectrum();
					}
				}
			}

			if (ImGui::SliderFloat("Doppler Factor (g)", &doppler_shift_factor_, 0.05f, 10.0f, "%.3f")) {
				recompute_spectrum();
			}
			render_setting_tooltip("Combined gravitational and kinematic redshift/blueshift factor g = nu_obs / nu_emit applied to the whole spectrum, matching the factor used by the accretion disk renderer.");

			const auto& params = orchestrator.parameters();
			if (link_temperature_to_disk_ && spectrum_type_ == 0) {
				const double r_isco = std::max(kerr_isco_radius(params.mass, params.spin), 1e-6);
				const double sample_r = r_isco * static_cast<double>(disk_sample_radius_isco_multiple_);
				const double ratio = r_isco / std::max(sample_r, r_isco);
				const double t_norm = std::pow(ratio, 0.75) * std::pow(std::max(1.0 - std::sqrt(ratio), 0.0), 0.25);
				const double sampled_temperature = 18000.0 * t_norm + 1200.0;
				if (std::abs(sampled_temperature - static_cast<double>(temperature_k_)) > 1.0) {
					temperature_k_ = static_cast<float>(sampled_temperature);
					recompute_spectrum();
				}
			}

			ImGui::Separator();
			ImGui::Text("Perceived CIE sRGB Color: ");
			ImGui::SameLine();
			ImGui::ColorButton("CIE Color Swatch", ImVec4(static_cast<float>(perceived_color_.r), static_cast<float>(perceived_color_.g), static_cast<float>(perceived_color_.b), 1.0f), 0, ImVec2(40.0f, 20.0f));
			render_setting_tooltip("Human-perceived color of the synthesized, Doppler-shifted spectrum after CIE 1931 color matching and sRGB gamma encoding.");

			ImGui::SameLine();
			const double peak_nm = peak_wavelength_nm(static_cast<double>(temperature_k_) / std::max(static_cast<double>(doppler_shift_factor_), 1e-6));
			ImGui::Text("| Peak Wavelength: %.1f nm", peak_nm);
			render_setting_tooltip("Wien's law estimate of the wavelength carrying the most radiant power, computed from the observed (Doppler-shifted) effective temperature.");

			double bolometric = 0.0;
			for (size_t i = 0; i < intensities_.size(); ++i) {
				bolometric += intensities_[i];
			}
			ImGui::Text("Approx. Bolometric Radiance: %.4e W/m^2/sr", bolometric);
			render_setting_tooltip("Trapezoidal-style integral of the plotted radiance curve across the sampled wavelength window, used as a rough total-brightness indicator.");

			ImGui::Separator();
			ImGui::TextColored(ImVec4(0.6f, 0.9f, 1.0f, 1.0f), "Live Accretion Disk Temperature Profile");
			render_setting_tooltip("Local disk temperature versus radius using the same Novikov-Thorne-inspired profile the viewport renderer applies, evaluated from the simulation's current mass and spin.");
			recompute_disk_profile(params.mass, params.spin);

			if (ImPlot::BeginPlot("Disk Temperature Profile", ImVec2(-1, 160))) {
				ImPlot::SetupAxes("Radius (M)", "Temperature (K)");
				ImPlot::PlotLine("T(r)", disk_radius_.data(), disk_temperature_k_.data(), static_cast<int>(kDiskProfileSamples));
				ImPlot::EndPlot();
			}

			ImGui::Separator();
			if (ImPlot::BeginPlot("Spectral Radiance I(lambda)", ImVec2(-1, -1))) {
				ImPlot::SetupAxes("Observed Wavelength (nm)", "Radiance (W/m^2/sr/m)");
				ImPlot::SetupAxesLimits(380.0, 780.0, 0.0, 1.2 * (*std::max_element(upper_band_.begin(), upper_band_.end()) + 1e-30), ImPlotCond_Always);

				ImPlot::PlotShaded("1-Sigma Confidence", wavelengths_nm_.data(), lower_band_.data(), upper_band_.data(), static_cast<int>(wavelengths_nm_.size()));
				ImPlot::PlotLine("Spectral Radiance", wavelengths_nm_.data(), intensities_.data(), static_cast<int>(wavelengths_nm_.size()));

				ImPlot::EndPlot();
			}
		}
		ImGui::End();
	}
};

}

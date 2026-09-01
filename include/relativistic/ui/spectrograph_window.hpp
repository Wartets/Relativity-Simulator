#pragma once

#include <imgui.h>
#include <implot.h>
#include "relativistic/optics/spectrum.hpp"
#include "relativistic/optics/cie_observer.hpp"
#include "relativistic/optics/radiative_processes.hpp"
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

	std::vector<double> wavelengths_nm_{};
	std::vector<double> intensities_{};
	std::vector<double> upper_band_{};
	std::vector<double> lower_band_{};
	Optics::ColorRGB perceived_color_{};

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

public:
	SpectrographWindow() {
		recompute_spectrum();
	}

	void render() {
		if (!is_open_) return;

		if (ImGui::Begin("Radiative Transfer & Spectrograph Monitor", &is_open_)) {
			const char* types[] = {"Thermal Blackbody Emission", "Relativistic Synchrotron Power-Law", "Monochromatic Calibration Line"};
			if (ImGui::Combo("Emission Process", &spectrum_type_, types, IM_ARRAYSIZE(types))) {
				recompute_spectrum();
			}

			if (spectrum_type_ == 0) {
				if (ImGui::SliderFloat("Temperature (K)", &temperature_k_, 500.0f, 50000.0f, "%.0f K")) {
					recompute_spectrum();
				}
			}

			if (ImGui::SliderFloat("Doppler Factor (g)", &doppler_shift_factor_, 0.1f, 5.0f, "%.3f")) {
				recompute_spectrum();
			}

			ImGui::Separator();
			ImGui::Text("Perceived CIE sRGB Color: ");
			ImGui::SameLine();
			ImGui::ColorButton("CIE Color Swatch", ImVec4(static_cast<float>(perceived_color_.r), static_cast<float>(perceived_color_.g), static_cast<float>(perceived_color_.b), 1.0f), 0, ImVec2(40.0f, 20.0f));

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

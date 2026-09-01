#pragma once

#include <imgui.h>
#include <implot.h>
#include <vector>
#include <cmath>
#include <algorithm>

namespace Relativistic::UI {

class SpectrographWindow {
private:
	bool is_open_{true};
	std::vector<double> wavelengths_;
	std::vector<double> intensities_;
	std::vector<double> upper_band_;
	std::vector<double> lower_band_;

	void generate_dummy_data() {
		wavelengths_.clear();
		intensities_.clear();
		upper_band_.clear();
		lower_band_.clear();
		
		for (int i = 0; i < 400; ++i) {
			double wl = 380.0 + static_cast<double>(i);
			double intensity = std::exp(-0.5 * std::pow((wl - 550.0) / 50.0, 2.0));
			double noise = 0.1 * intensity;
			
			wavelengths_.push_back(wl);
			intensities_.push_back(intensity);
			upper_band_.push_back(intensity + noise);
			lower_band_.push_back(std::max(0.0, intensity - noise));
		}
	}

public:
	SpectrographWindow() {
		generate_dummy_data();
	}

	void render() {
		if (!is_open_) return;

		if (ImGui::Begin("Spectrograph Monitor", &is_open_)) {
			if (ImPlot::BeginPlot("Spectral Intensity", ImVec2(-1, -1))) {
				ImPlot::SetupAxes("Wavelength (nm)", "Intensity (W/m^2/sr/m)");
				ImPlot::SetupAxesLimits(380.0, 780.0, 0.0, 1.2);
				
				ImPlot::PlotShaded("1-Sigma Uncertainty", wavelengths_.data(), lower_band_.data(), upper_band_.data(), static_cast<int>(wavelengths_.size()));
				
				ImPlot::PlotLine("Intensity", wavelengths_.data(), intensities_.data(), static_cast<int>(wavelengths_.size()));
				
				ImPlot::EndPlot();
			}
		}
		ImGui::End();
	}
};

}

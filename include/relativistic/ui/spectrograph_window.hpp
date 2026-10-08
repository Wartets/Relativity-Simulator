#pragma once

#include <imgui.h>
#include <implot.h>
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/optics/spectrum.hpp"
#include "relativistic/optics/cie_observer.hpp"
#include "relativistic/optics/radiative_processes.hpp"
#include "relativistic/optics/polarized_radiative_transfer.hpp"
#include "relativistic/optics/disk_thermal_profile.hpp"
#include "relativistic/optics/geodesic_ray_probe.hpp"
#include "relativistic/core/constants.hpp"
#include "relativistic/ui/tooltip_utils.hpp"
#include "relativistic/ui/hud/hud_linked_readouts.hpp"
#include "relativistic/ui/ray_probe_panel.hpp"
#include "relativistic/ui/polarization_panel.hpp"
#include "relativistic/ui/interferometry_panel.hpp"
#include "relativistic/units/unit_system.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <numbers>
#include <string>
#include <vector>

namespace Relativistic::UI {

class SpectrographWindow {
private:
	static constexpr size_t kPlotSamples = 900;
	static constexpr size_t kDiskProfileSamples = 60;
	static constexpr size_t kMaxReportedPeaks = 8;

	struct SpectralPeak {
		double wavelength_nm{0.0};
		double intensity{0.0};
		double prominence{0.0};
		double fwhm_nm{0.0};
		bool at_window_edge{false};
	};

	struct SpectralBandSummary {
		const char* name;
		double lower_nm;
		double upper_nm;
		double integral{0.0};
		double fraction{0.0};
	};

	struct SpectralStatistics {
		double peak_wavelength_nm{0.0};
		double peak_intensity{0.0};
		double centroid_nm{0.0};
		double fwhm_nm{0.0};
		double window_integral{0.0};
		double bolometric{0.0};
		double chromaticity_x{0.0};
		double chromaticity_y{0.0};
		double luminance_y{0.0};
		double correlated_color_temperature_k{0.0};
		double wien_temperature_k{0.0};
	};

	bool is_open_{true};
	int spectrum_type_{0};
	float temperature_k_{5778.0f};
	float doppler_shift_factor_{1.0f};
	float synchrotron_index_{-0.7f};
	float line_wavelength_nm_{550.0f};
	float magnetic_field_tesla_{0.1f};
	float electron_density_{1e18f};
	float plasma_temperature_k_{1.0e9f};
	float path_length_m_{1.0e3f};
	float power_law_index_{3.0f};
	float non_thermal_fraction_{0.01f};
	float gamma_min_{10.0f};
	float pitch_angle_deg_{60.0f};
	bool link_temperature_to_disk_{false};
	float disk_sample_radius_isco_multiple_{1.5f};
	bool link_doppler_to_camera_{false};
	bool link_to_ray_probe_{false};
	double camera_linked_g_{1.0};
	bool camera_within_disk_band_{false};
	double camera_linked_radius_{0.0};

	float wavelength_min_nm_{380.0f};
	float wavelength_max_nm_{780.0f};
	bool log_wavelength_axis_{false};
	bool log_intensity_axis_{false};
	bool show_emitted_reference_{false};
	float uncertainty_fraction_{0.08f};
	float detection_sensitivity_{0.05f};
	bool plot_refit_{true};

	Optics::ContinuousSpectrum<double> base_spectrum_{};
	Optics::ContinuousSpectrum<double> shifted_spectrum_{};
	Optics::ColorRGB perceived_color_{};
	Optics::ColorXYZ perceived_xyz_{};
	SpectralStatistics statistics_{};
	std::vector<SpectralBandSummary> bands_{};
	std::vector<SpectralPeak> peaks_{};

	std::vector<double> wavelengths_nm_{};
	std::vector<double> intensities_{};
	std::vector<double> emitted_intensities_{};
	std::vector<double> upper_band_{};
	std::vector<double> lower_band_{};

	std::vector<double> disk_radius_{};
	std::vector<double> disk_temperature_k_{};

	const Optics::RayProbeResult* probe_result_{nullptr};
	Optics::RayProbeResult empty_probe_{};
	RayProbePanel ray_probe_panel_{};
	PolarizationPanel polarization_panel_{};
	InterferometryPanel interferometry_panel_{};
	HudLinkedReadouts linked_readouts_{};

	[[nodiscard]] Optics::PolarizedPlasmaState<double> build_plasma() const noexcept {
		Optics::PolarizedPlasmaState<double> plasma;
		plasma.electron_density = static_cast<double>(electron_density_);
		plasma.ion_density = plasma.electron_density;
		plasma.electron_temperature_k = static_cast<double>(plasma_temperature_k_);
		plasma.magnetic_field_tesla = static_cast<double>(magnetic_field_tesla_);
		plasma.pitch_angle_rad = static_cast<double>(pitch_angle_deg_) * std::numbers::pi_v<double> / 180.0;
		plasma.power_law_index = static_cast<double>(power_law_index_);
		plasma.non_thermal_fraction = static_cast<double>(non_thermal_fraction_);
		plasma.gamma_min = static_cast<double>(gamma_min_);
		plasma.gamma_max = 1.0e5;
		return plasma;
	}

	[[nodiscard]] bool plasma_spectrum_selected() const noexcept {
		return spectrum_type_ >= 3;
	}

	[[nodiscard]] Optics::ContinuousSpectrum<double> build_base_spectrum() const {
		using Spectrum = Optics::ContinuousSpectrum<double>;
		constexpr double c = Core::PhysicalConstants<double>::SPEED_OF_LIGHT;
		const Optics::PolarizedPlasmaState<double> plasma = build_plasma();
		const double path = static_cast<double>(path_length_m_);
		switch (spectrum_type_) {
			case 1:
				return Spectrum::make_synchrotron(static_cast<double>(synchrotron_index_), 1e-12);
			case 2:
				return Spectrum::make_monochromatic(static_cast<double>(line_wavelength_nm_) * 1e-9, 1.0);
			case 3:
			case 4:
			{
				const bool thermal = (spectrum_type_ == 4);
				return Spectrum::from_wavelength_function([&](double lambda) {
					const double nu = c / lambda;
					const auto stokes = Optics::PolarizedRadiativeTransfer<double>::integrate_ray_segment(Optics::StokesVector<double>(0.0), nu, 1.0, plasma, path, thermal);
					return stokes.i * c / (lambda * lambda);
				});
			}
			case 5:
				return Spectrum::from_wavelength_function([&](double lambda) {
					const double nu = c / lambda;
					const double emissivity = Optics::RadiativeProcessEngine<double>::thermal_bremsstrahlung_emissivity(nu, plasma.electron_density, plasma.ion_density, plasma.electron_temperature_k);
					return emissivity * path * c / (lambda * lambda);
				});
			case 0:
			default:
				return Spectrum::make_blackbody(static_cast<double>(temperature_k_));
		}
	}

	void recompute_disk_profile(double mass, double spin) {
		disk_radius_.assign(kDiskProfileSamples, 0.0);
		disk_temperature_k_.assign(kDiskProfileSamples, 0.0);

		const double r_isco = std::max(Optics::DiskThermalProfile::kerr_isco_radius(mass, spin), 1e-6);
		const double r_outer = Optics::DiskThermalProfile::disk_outer_radius(std::max(mass, 1e-6));

		for (size_t i = 0; i < kDiskProfileSamples; ++i) {
			const double t = static_cast<double>(i) / static_cast<double>(kDiskProfileSamples - 1);
			const double r = r_isco + t * (r_outer - r_isco);
			disk_radius_[i] = r;
			disk_temperature_k_[i] = Optics::DiskThermalProfile::effective_temperature_kelvin(r_isco, r);
		}
	}

	void recompute_spectrum() {
		base_spectrum_ = build_base_spectrum();
		shifted_spectrum_ = base_spectrum_.transform_doppler(static_cast<double>(doppler_shift_factor_));
		perceived_color_ = Optics::CIE1931Observer::spectrum_to_srgb(shifted_spectrum_);
		perceived_xyz_ = Optics::CIE1931Observer::integrate_spectrum(shifted_spectrum_);
		resample_plot();
		compute_statistics();
		detect_peaks();
		plot_refit_ = true;
	}

	void resample_plot() {
		const double lower = std::max(static_cast<double>(wavelength_min_nm_), 1.0e-3);
		const double upper = std::max(static_cast<double>(wavelength_max_nm_), lower * 1.01);
		wavelengths_nm_.resize(kPlotSamples);
		intensities_.resize(kPlotSamples);
		emitted_intensities_.resize(kPlotSamples);
		upper_band_.resize(kPlotSamples);
		lower_band_.resize(kPlotSamples);
		const double log_lower = std::log(lower);
		const double log_span = std::log(upper) - log_lower;
		const double uncertainty = static_cast<double>(uncertainty_fraction_);
		for (size_t i = 0; i < kPlotSamples; ++i) {
			const double fraction = static_cast<double>(i) / static_cast<double>(kPlotSamples - 1);
			const double wavelength = log_wavelength_axis_ ? std::exp(log_lower + fraction * log_span) : (lower + fraction * (upper - lower));
			const double value = shifted_spectrum_.sample_radiance(wavelength * 1e-9);
			wavelengths_nm_[i] = wavelength;
			intensities_[i] = value;
			emitted_intensities_[i] = base_spectrum_.sample_radiance(wavelength * 1e-9);
			upper_band_[i] = value * (1.0 + uncertainty);
			lower_band_[i] = std::max(value * (1.0 - uncertainty), 0.0);
		}
	}

	void compute_statistics() {
		statistics_ = SpectralStatistics{};
		const size_t n = intensities_.size();
		if (n < 2) {
			return;
		}
		size_t peak_index = 0;
		for (size_t i = 1; i < n; ++i) {
			if (intensities_[i] > intensities_[peak_index]) {
				peak_index = i;
			}
		}
		statistics_.peak_wavelength_nm = wavelengths_nm_[peak_index];
		statistics_.peak_intensity = intensities_[peak_index];

		double weighted = 0.0;
		double total = 0.0;
		for (size_t i = 0; i + 1 < n; ++i) {
			const double d_lambda = wavelengths_nm_[i + 1] - wavelengths_nm_[i];
			const double mean_i = 0.5 * (intensities_[i] + intensities_[i + 1]);
			const double mean_lambda_i = 0.5 * (wavelengths_nm_[i] * intensities_[i] + wavelengths_nm_[i + 1] * intensities_[i + 1]);
			total += mean_i * d_lambda;
			weighted += mean_lambda_i * d_lambda;
		}
		statistics_.window_integral = total * 1e-9;
		statistics_.centroid_nm = (total > 0.0) ? (weighted / total) : 0.0;

		const double half = 0.5 * statistics_.peak_intensity;
		size_t left = peak_index;
		while (left > 0 && intensities_[left] > half) {
			--left;
		}
		size_t right = peak_index;
		while (right + 1 < n && intensities_[right] > half) {
			++right;
		}
		statistics_.fwhm_nm = wavelengths_nm_[right] - wavelengths_nm_[left];

		statistics_.bolometric = shifted_spectrum_.bolometric_radiance();
		const double xyz_sum = perceived_xyz_.x + perceived_xyz_.y + perceived_xyz_.z;
		if (xyz_sum > 1e-300) {
			statistics_.chromaticity_x = perceived_xyz_.x / xyz_sum;
			statistics_.chromaticity_y = perceived_xyz_.y / xyz_sum;
			const double denominator = 0.1858 - statistics_.chromaticity_y;
			if (std::abs(denominator) > 1e-9) {
				const double n_mccamy = (statistics_.chromaticity_x - 0.3320) / denominator;
				const double cct = 449.0 * n_mccamy * n_mccamy * n_mccamy + 3525.0 * n_mccamy * n_mccamy + 6823.3 * n_mccamy + 5520.33;
				statistics_.correlated_color_temperature_k = (cct >= 1000.0 && cct <= 40000.0) ? cct : 0.0;
			}
		}
		statistics_.luminance_y = perceived_xyz_.y;
		const bool interior_peak = (peak_index > 0 && peak_index + 1 < n);
		statistics_.wien_temperature_k = interior_peak ? (2.897771955e6 / statistics_.peak_wavelength_nm) : 0.0;

		bands_.clear();
		bands_.push_back(SpectralBandSummary{"Ultraviolet", 10.0, 380.0});
		bands_.push_back(SpectralBandSummary{"Visible", 380.0, 780.0});
		bands_.push_back(SpectralBandSummary{"Near Infrared", 780.0, 2500.0});
		bands_.push_back(SpectralBandSummary{"Mid Infrared", 2500.0, 25000.0});
		bands_.push_back(SpectralBandSummary{"Far Infrared", 25000.0, 1.0e6});
		double band_total = 0.0;
		for (auto& band : bands_) {
			band.integral = shifted_spectrum_.integrate_band_logarithmic(band.lower_nm * 1e-9, band.upper_nm * 1e-9, 600);
			band_total += band.integral;
		}
		for (auto& band : bands_) {
			band.fraction = (band_total > 0.0) ? (band.integral / band_total) : 0.0;
		}
	}

	void detect_peaks() {
		peaks_.clear();
		const size_t n = intensities_.size();
		if (n < 5) {
			return;
		}
		std::vector<double> smooth(n);
		smooth.front() = intensities_.front();
		smooth.back() = intensities_.back();
		for (size_t i = 1; i + 1 < n; ++i) {
			smooth[i] = 0.25 * intensities_[i - 1] + 0.5 * intensities_[i] + 0.25 * intensities_[i + 1];
		}
		const double max_value = *std::max_element(smooth.begin(), smooth.end());
		if (max_value <= 0.0) {
			return;
		}
		const double threshold = static_cast<double>(detection_sensitivity_) * max_value;

		for (size_t i = 1; i + 1 < n; ++i) {
			if (!(smooth[i] > smooth[i - 1] && smooth[i] >= smooth[i + 1])) {
				continue;
			}
			double left_min = smooth[i];
			for (size_t j = i; j-- > 0;) {
				if (smooth[j] > smooth[i]) break;
				left_min = std::min(left_min, smooth[j]);
			}
			double right_min = smooth[i];
			for (size_t j = i + 1; j < n; ++j) {
				if (smooth[j] > smooth[i]) break;
				right_min = std::min(right_min, smooth[j]);
			}
			const double prominence = smooth[i] - std::max(left_min, right_min);
			if (prominence < threshold) {
				continue;
			}
			peaks_.push_back(make_peak(smooth, i, prominence, false));
		}

		const size_t global_index = static_cast<size_t>(std::max_element(smooth.begin(), smooth.end()) - smooth.begin());
		if ((global_index == 0 || global_index == n - 1) && smooth[global_index] > threshold) {
			peaks_.push_back(make_peak(smooth, global_index, smooth[global_index], true));
		}

		std::sort(peaks_.begin(), peaks_.end(), [](const SpectralPeak& a, const SpectralPeak& b) noexcept {
			return a.prominence > b.prominence;
		});
		if (peaks_.size() > kMaxReportedPeaks) {
			peaks_.resize(kMaxReportedPeaks);
		}
	}

	[[nodiscard]] SpectralPeak make_peak(const std::vector<double>& smooth, size_t index, double prominence, bool edge) const {
		const size_t n = smooth.size();
		const double half_level = smooth[index] - 0.5 * prominence;
		size_t left = index;
		while (left > 0 && smooth[left] > half_level) {
			--left;
		}
		size_t right = index;
		while (right + 1 < n && smooth[right] > half_level) {
			++right;
		}
		double left_wavelength = wavelengths_nm_[left];
		if (left + 1 <= index && smooth[left + 1] != smooth[left] && smooth[left] <= half_level) {
			const double t = (half_level - smooth[left]) / (smooth[left + 1] - smooth[left]);
			left_wavelength = wavelengths_nm_[left] + t * (wavelengths_nm_[left + 1] - wavelengths_nm_[left]);
		}
		double right_wavelength = wavelengths_nm_[right];
		if (right >= 1 && right - 1 >= index && smooth[right - 1] != smooth[right] && smooth[right] <= half_level) {
			const double t = (smooth[right - 1] - half_level) / (smooth[right - 1] - smooth[right]);
			right_wavelength = wavelengths_nm_[right - 1] + t * (wavelengths_nm_[right] - wavelengths_nm_[right - 1]);
		}
		SpectralPeak peak;
		peak.wavelength_nm = wavelengths_nm_[index];
		peak.intensity = intensities_[index];
		peak.prominence = prominence;
		peak.fwhm_nm = std::max(right_wavelength - left_wavelength, 0.0);
		peak.at_window_edge = edge;
		return peak;
	}

	[[nodiscard]] static const char* describe_color(const Optics::ColorRGB& color) noexcept {
		const double max_c = std::max({color.r, color.g, color.b});
		const double min_c = std::min({color.r, color.g, color.b});
		const double delta = max_c - min_c;
		if (max_c < 0.04) return "Black";
		if (delta < 0.08 * max_c) return (max_c > 0.8) ? "White" : "Gray";
		double hue = 0.0;
		if (max_c == color.r) hue = std::fmod((color.g - color.b) / delta, 6.0);
		else if (max_c == color.g) hue = (color.b - color.r) / delta + 2.0;
		else hue = (color.r - color.g) / delta + 4.0;
		hue *= 60.0;
		if (hue < 0.0) hue += 360.0;
		if (hue < 15.0) return "Red";
		if (hue < 45.0) return "Orange";
		if (hue < 70.0) return "Yellow";
		if (hue < 95.0) return "Yellow-Green";
		if (hue < 165.0) return "Green";
		if (hue < 195.0) return "Cyan";
		if (hue < 255.0) return "Blue";
		if (hue < 290.0) return "Violet";
		if (hue < 335.0) return "Magenta";
		return "Red";
	}

	[[nodiscard]] double disk_link_temperature(const Orchestrator::PhysicalParameters& params) const noexcept {
		const double r_isco = std::max(Optics::DiskThermalProfile::kerr_isco_radius(params.mass, params.spin), 1e-6);
		const double sample_r = r_isco * static_cast<double>(disk_sample_radius_isco_multiple_);
		return Optics::DiskThermalProfile::effective_temperature_kelvin(r_isco, sample_r);
	}

	void update_links(const Orchestrator::SimulationOrchestrator<1024>& orchestrator) {
		const auto& params = orchestrator.parameters();
		const auto& cam = orchestrator.camera();
		camera_linked_radius_ = cam.radius;
		camera_within_disk_band_ = Optics::DiskThermalProfile::radius_within_disk(params.mass, params.spin, cam.radius);
		camera_linked_g_ = camera_within_disk_band_ ? Optics::DiskThermalProfile::circular_orbit_redshift_factor(params.mass, cam.radius) : 1.0;

		bool changed = false;
		const Optics::RayProbeResult& probe = (probe_result_ != nullptr) ? *probe_result_ : empty_probe_;

		if (link_to_ray_probe_ && probe.valid) {
			const float target_g = static_cast<float>(std::clamp(probe.spectral_shift_g, 0.05, 10.0));
			if (std::abs(target_g - doppler_shift_factor_) > 1e-6f) {
				doppler_shift_factor_ = target_g;
				changed = true;
			}
		} else if (link_doppler_to_camera_) {
			const float target_g = static_cast<float>(std::clamp(camera_linked_g_, 0.05, 10.0));
			if (std::abs(target_g - doppler_shift_factor_) > 1e-6f) {
				doppler_shift_factor_ = target_g;
				changed = true;
			}
		}

		if (spectrum_type_ == 0) {
			double target_temperature = static_cast<double>(temperature_k_);
			bool temperature_linked = false;
			if (link_to_ray_probe_ && probe.valid && probe.disk_hit) {
				target_temperature = probe.disk_temperature_k;
				temperature_linked = true;
			} else if (link_temperature_to_disk_) {
				target_temperature = disk_link_temperature(params);
				temperature_linked = true;
			}
			if (temperature_linked) {
				const float clamped = static_cast<float>(std::clamp(target_temperature, 500.0, 50000.0));
				if (std::abs(clamped - temperature_k_) > 1.0f) {
					temperature_k_ = clamped;
					changed = true;
				}
			}
		}

		if (changed) {
			recompute_spectrum();
		}
	}

	void refresh_linked_readouts() {
		linked_readouts_.polarization = polarization_panel_.hud_summary();
		linked_readouts_.interferometry = interferometry_panel_.hud_summary();
	}

	void render_header() {
		ImGui::ColorButton("CIE Color Swatch", ImVec4(static_cast<float>(perceived_color_.r), static_cast<float>(perceived_color_.g), static_cast<float>(perceived_color_.b), 1.0f), 0, ImVec2(40.0f, 20.0f));
		render_setting_tooltip("Human-perceived color of the synthesized, Doppler-shifted spectrum after CIE 1931 color matching and sRGB gamma encoding.");
		ImGui::SameLine();
		ImGui::Text("%s | g = %.4f | Peak %.1f nm", describe_color(perceived_color_), static_cast<double>(doppler_shift_factor_), statistics_.peak_wavelength_nm);
		if (link_to_ray_probe_ && probe_result_ != nullptr && !probe_result_->valid) {
			ImGui::TextColored(ImVec4(1.0f, 0.65f, 0.3f, 1.0f), "Ray probe link is enabled but no probe data is available yet; hover the viewport.");
		}
	}

	void render_spectrum_tab(const Orchestrator::SimulationOrchestrator<1024>& orchestrator) {
		const auto& params = orchestrator.parameters();
		const Optics::RayProbeResult& probe = (probe_result_ != nullptr) ? *probe_result_ : empty_probe_;

		const char* types[] = {
			"Thermal Blackbody Emission",
			"Parametric Synchrotron Power-Law",
			"Monochromatic Calibration Line",
			"Physical Non-Thermal Synchrotron (Plasma)",
			"Physical Thermal Synchrotron (Plasma)",
			"Thermal Bremsstrahlung (Plasma)"
		};
		if (ImGui::Combo("Emission Process", &spectrum_type_, types, IM_ARRAYSIZE(types))) {
			recompute_spectrum();
		}
		render_setting_tooltip("Selects the radiative process used to synthesize the spectrum. The plasma-based processes evaluate the same emissivity and absorptivity engine as the polarized radiative transfer, using the plasma parameters below.");

		const bool temperature_linked_to_probe = link_to_ray_probe_ && probe_result_ != nullptr && probe_result_->valid && probe_result_->disk_hit;
		if (spectrum_type_ == 0) {
			if (ImGui::Checkbox("Link Temperature To Accretion Disk Profile", &link_temperature_to_disk_)) {
				recompute_spectrum();
			}
			render_setting_tooltip("When enabled, the blackbody temperature is sampled from the live disk temperature profile at the chosen radius.");
			if (link_temperature_to_disk_) {
				if (ImGui::SliderFloat("Sample Radius (ISCO multiples)", &disk_sample_radius_isco_multiple_, 1.0f, 16.0f, "%.2fx")) {
					recompute_spectrum();
				}
			}
			const bool locked = link_temperature_to_disk_ || temperature_linked_to_probe;
			if (locked) {
				ImGui::BeginDisabled(true);
			}
			if (ImGui::SliderFloat("Temperature (K)", &temperature_k_, 500.0f, 50000.0f, "%.0f K")) {
				recompute_spectrum();
			}
			if (locked) {
				ImGui::EndDisabled();
			}
			ImGui::TextDisabled("%s", Units::format_temperature(static_cast<double>(temperature_k_), orchestrator.unit_preferences().temperature).c_str());
		} else if (spectrum_type_ == 1) {
			if (ImGui::SliderFloat("Spectral Index", &synchrotron_index_, -2.5f, 1.5f, "%.2f")) {
				recompute_spectrum();
			}
			render_setting_tooltip("Power-law exponent of the parametric synchrotron continuum.");
		} else if (spectrum_type_ == 2) {
			if (ImGui::SliderFloat("Line Wavelength (nm)", &line_wavelength_nm_, 100.0f, 2000.0f, "%.1f")) {
				recompute_spectrum();
			}
			render_setting_tooltip("Rest wavelength of the calibration line. The Doppler factor moves it to the observed wavelength.");
		}

		if (ImGui::Checkbox("Link Doppler Factor To Camera Line-Of-Sight", &link_doppler_to_camera_)) {
			if (link_doppler_to_camera_) {
				link_to_ray_probe_ = false;
			}
			recompute_spectrum();
		}
		render_setting_tooltip("Computes g from the observer's current radius with the circular-orbit redshift approximation used by the disk renderer. Outside the disk band the factor falls back to 1.");
		if (ImGui::Checkbox("Link To Ray Probe (g And Disk Temperature)", &link_to_ray_probe_)) {
			if (link_to_ray_probe_) {
				link_doppler_to_camera_ = false;
			}
			recompute_spectrum();
		}
		render_setting_tooltip("Drives the spectral shift with the exact value traced for the probed pixel and, for blackbody spectra, the emitted disk temperature at the probed emission point. Hover the viewport or freeze the probe to hold a pixel.");

		const bool g_locked = link_doppler_to_camera_ || (link_to_ray_probe_ && probe_result_ != nullptr && probe_result_->valid);
		if (g_locked) {
			ImGui::BeginDisabled(true);
		}
		if (ImGui::SliderFloat("Doppler Factor (g)", &doppler_shift_factor_, 0.05f, 10.0f, "%.3f")) {
			recompute_spectrum();
		}
		if (g_locked) {
			ImGui::EndDisabled();
		}
		render_setting_tooltip("Combined gravitational and kinematic factor g = nu_obs / nu_emit applied to the spectrum with the exact g^5 wavelength-intensity beaming law.");
		if (link_doppler_to_camera_) {
			if (!camera_within_disk_band_) {
				ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.3f, 1.0f), "Observer at r=%.2f M is outside the disk band [%.2f M, %.2f M]; using g=1.0.", camera_linked_radius_, Optics::DiskThermalProfile::kerr_isco_radius(params.mass, params.spin), Optics::DiskThermalProfile::disk_outer_radius(params.mass));
			} else {
				ImGui::TextColored(ImVec4(0.4f, 0.95f, 0.5f, 1.0f), "Linked to observer radius r=%.2f M -> g=%.4f.", camera_linked_radius_, camera_linked_g_);
			}
		}
		if (link_to_ray_probe_ && probe.valid) {
			ImGui::TextColored(ImVec4(0.4f, 0.95f, 0.5f, 1.0f), "Linked to probe pixel (%u, %u): g=%.5f%s", probe.pixel_x, probe.pixel_y, probe.spectral_shift_g, probe.termination == Optics::RayTermination::HorizonAbsorbed ? " (horizon: clamped)" : "");
		}

		if (plasma_spectrum_selected()) {
			render_plasma_controls();
		}

		render_window_controls();
		render_statistics_section();
		render_plots(orchestrator, probe);
	}

	void render_plasma_controls() {
		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.75f, 0.6f, 1.0f, 1.0f), "Plasma Parameters");
		ImGui::TextDisabled("Feeds the same emissivity and absorptivity models used by the polarized radiative transfer engine.");
		bool changed = false;

		float mag_log = std::log10(std::max(magnetic_field_tesla_, 1e-6f));
		if (ImGui::SliderFloat("Magnetic Field (log10 Tesla)", &mag_log, -6.0f, 5.0f, "10^%.2f T")) {
			magnetic_field_tesla_ = std::pow(10.0f, mag_log);
			changed = true;
		}
		render_setting_tooltip("Local magnetic field strength threading the plasma. Accretion flows near stellar-mass black holes reach roughly 10 to 10000 Tesla close to the horizon.");

		float density_log = std::log10(std::max(electron_density_, 1.0f));
		if (ImGui::SliderFloat("Electron Density (log10 per m^3)", &density_log, 8.0f, 24.0f, "10^%.2f m^-3")) {
			electron_density_ = std::pow(10.0f, density_log);
			changed = true;
		}
		render_setting_tooltip("Number density of the radiating electrons. Higher densities raise both emissivity and self-absorption.");

		float temperature_log = std::log10(std::max(plasma_temperature_k_, 1.0e3f));
		if (ImGui::SliderFloat("Electron Temperature (log10 K)", &temperature_log, 6.0f, 13.0f, "10^%.2f K")) {
			plasma_temperature_k_ = std::pow(10.0f, temperature_log);
			changed = true;
		}
		float path_log = std::log10(std::max(path_length_m_, 1.0f));
		if (ImGui::SliderFloat("Path Length (log10 m)", &path_log, 0.0f, 15.0f, "10^%.2f m")) {
			path_length_m_ = std::pow(10.0f, path_log);
			changed = true;
		}
		changed = ImGui::SliderFloat("Pitch Angle (deg)", &pitch_angle_deg_, 1.0f, 179.0f, "%.1f") || changed;
		if (spectrum_type_ == 3) {
			changed = ImGui::SliderFloat("Power Law Index (p)", &power_law_index_, 1.5f, 5.0f, "%.2f") || changed;
			changed = ImGui::SliderFloat("Non-Thermal Fraction", &non_thermal_fraction_, 0.0f, 1.0f, "%.3f") || changed;
			changed = ImGui::SliderFloat("Minimum Lorentz Factor", &gamma_min_, 1.0f, 1.0e4f, "%.1f", ImGuiSliderFlags_Logarithmic) || changed;
		}
		if (changed) {
			recompute_spectrum();
		}

		const Optics::PolarizedPlasmaState<double> plasma = build_plasma();
		constexpr double e_charge = Core::PhysicalConstants<double>::ELEMENTARY_CHARGE;
		constexpr double m_e = Core::PhysicalConstants<double>::ELECTRON_MASS;
		constexpr double m_p = Core::PhysicalConstants<double>::PROTON_MASS;
		constexpr double c = Core::PhysicalConstants<double>::SPEED_OF_LIGHT;
		constexpr double eps0 = Core::PhysicalConstants<double>::VACUUM_PERMITTIVITY;
		constexpr double mu0 = 1.0 / (eps0 * c * c);
		constexpr double two_pi = 2.0 * std::numbers::pi_v<double>;
		const double nu_p = std::sqrt(plasma.electron_density * e_charge * e_charge / (eps0 * m_e)) / two_pi;
		const double nu_b = Optics::RadiativeProcessEngine<double>::synchrotron_cyclotron_frequency(plasma.magnetic_field_tesla);
		const double nu_crit = 1.5 * plasma.gamma_min * plasma.gamma_min * nu_b * std::sin(plasma.pitch_angle_rad);
		const double sigma_magnetization = plasma.magnetic_field_tesla * plasma.magnetic_field_tesla / (mu0 * std::max(plasma.electron_density, 1.0) * m_p * c * c);
		const double theta_e = Core::PhysicalConstants<double>::BOLTZMANN_CONSTANT * plasma.electron_temperature_k / (m_e * c * c);
		const double nu_ref = c / 550e-9;
		const auto emissivity = Optics::RadiativeProcessEngine<double>::non_thermal_synchrotron_emissivity(nu_ref, plasma);
		const auto absorptivity = Optics::RadiativeProcessEngine<double>::non_thermal_synchrotron_absorptivity(nu_ref, plasma);
		const double sigma_thomson = Core::PhysicalConstants<double>::THOMSON_CROSS_SECTION;
		const double cooling_time_s = (plasma.magnetic_field_tesla > 1e-9)
			? (6.0 * std::numbers::pi_v<double> * m_e * c) / (sigma_thomson * plasma.magnetic_field_tesla * plasma.magnetic_field_tesla * std::max(plasma.gamma_min, 1.0))
			: 0.0;

		ImGui::Text("Plasma Frequency: %.4e Hz | Electron Cyclotron Frequency: %.4e Hz", nu_p, nu_b);
		render_setting_tooltip("nu_p = sqrt(n e^2 / (eps0 m_e)) / 2pi and nu_B = eB / (2 pi m_e). Emission below the plasma frequency cannot propagate.");
		ImGui::Text("Critical Synchrotron Frequency (gamma_min): %.4e Hz", nu_crit);
		render_setting_tooltip("nu_c = 1.5 gamma^2 nu_B sin(alpha): the frequency near which an electron of Lorentz factor gamma radiates most of its power.");
		ImGui::Text("Magnetization sigma: %.4e | Dimensionless Temperature: %.4e", sigma_magnetization, theta_e);
		render_setting_tooltip("sigma = B^2 / (mu0 n m_p c^2) and theta_e = kT / (m_e c^2). Values of theta_e above one imply relativistic Maxwell-Juttner electrons.");
		ImGui::Text("Emissivity At 550nm (j_i, j_q): %.3e, %.3e W/m^3/sr/Hz", emissivity.j_i, emissivity.j_q);
		ImGui::Text("Absorptivity At 550nm (alpha_i): %.3e 1/m | Optical Depth: %.3e", absorptivity.alpha_i, absorptivity.alpha_i * static_cast<double>(path_length_m_));
		render_setting_tooltip("Stokes-I emission and absorption coefficients of the non-thermal population at a reference visible wavelength, and the resulting optical depth across the selected path length.");
		ImGui::Text("Approx. Synchrotron Cooling Time (gamma=%.0f): %.3e s", plasma.gamma_min, cooling_time_s);
		render_setting_tooltip("Timescale for an electron at the minimum Lorentz factor to radiate an order-unity fraction of its energy.");
	}

	void render_window_controls() {
		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.6f, 0.9f, 1.0f, 1.0f), "Spectral Window And Detection");
		bool changed = false;
		if (ImGui::Button("Visible")) {
			wavelength_min_nm_ = 380.0f;
			wavelength_max_nm_ = 780.0f;
			log_wavelength_axis_ = false;
			changed = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("UV To Near IR")) {
			wavelength_min_nm_ = 100.0f;
			wavelength_max_nm_ = 2500.0f;
			log_wavelength_axis_ = true;
			changed = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Infrared")) {
			wavelength_min_nm_ = 700.0f;
			wavelength_max_nm_ = 1.0e5f;
			log_wavelength_axis_ = true;
			changed = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("X-Ray To Radio")) {
			wavelength_min_nm_ = 0.01f;
			wavelength_max_nm_ = 1.0e9f;
			log_wavelength_axis_ = true;
			log_intensity_axis_ = true;
			changed = true;
		}
		render_setting_tooltip("Quick spectral windows. Wide windows use logarithmic wavelength sampling so that narrow features stay resolved.");
		changed = ImGui::SliderFloat("Window Minimum (nm)", &wavelength_min_nm_, 0.001f, 1.0e8f, "%.3f", ImGuiSliderFlags_Logarithmic) || changed;
		changed = ImGui::SliderFloat("Window Maximum (nm)", &wavelength_max_nm_, 0.01f, 1.0e9f, "%.3f", ImGuiSliderFlags_Logarithmic) || changed;
		wavelength_max_nm_ = std::max(wavelength_max_nm_, wavelength_min_nm_ * 1.05f);
		changed = ImGui::Checkbox("Logarithmic Wavelength Axis", &log_wavelength_axis_) || changed;
		ImGui::SameLine();
		changed = ImGui::Checkbox("Logarithmic Intensity Axis", &log_intensity_axis_) || changed;
		ImGui::SameLine();
		ImGui::Checkbox("Show Emitted Reference", &show_emitted_reference_);
		render_setting_tooltip("Overlays the rest-frame spectrum before the Doppler shift for direct comparison with the observed one.");
		changed = ImGui::SliderFloat("Display Uncertainty (%)", &uncertainty_fraction_, 0.0f, 0.5f, "%.2f") || changed;
		render_setting_tooltip("Width of the shaded confidence band drawn around the spectrum, as a fraction of the local radiance.");
		if (ImGui::SliderFloat("Detection Sensitivity", &detection_sensitivity_, 0.005f, 0.5f, "%.3f", ImGuiSliderFlags_Logarithmic)) {
			detect_peaks();
		}
		render_setting_tooltip("Minimum prominence of a spectral feature, as a fraction of the strongest peak, for it to be reported. Lower values reveal weaker lines and bumps.");
		if (changed) {
			recompute_spectrum();
		}
	}

	void render_statistics_section() {
		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.4f, 0.95f, 0.5f, 1.0f), "Spectral Statistics");
		if (ImGui::BeginTable("##SpectralStatistics", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
			ImGui::TableSetupColumn("Quantity", ImGuiTableColumnFlags_WidthFixed, 260.0f);
			ImGui::TableSetupColumn("Value");
			ImGui::TableHeadersRow();
			stat_row("Perceived Color", describe_color(perceived_color_), "Name of the dominant hue of the perceived sRGB color.");
			stat_row("CIE Chromaticity (x, y)", number("%.4f, ", statistics_.chromaticity_x) + number("%.4f", statistics_.chromaticity_y), "Position of the spectrum on the CIE 1931 chromaticity diagram.");
			stat_row("Correlated Color Temperature", (statistics_.correlated_color_temperature_k > 0.0) ? number("%.0f K", statistics_.correlated_color_temperature_k) : std::string("Outside valid range"), "McCamy approximation of the blackbody temperature with the closest chromaticity, valid between 1000 K and 40000 K.");
			stat_row("Luminance Y (CIE)", number("%.4e", statistics_.luminance_y), "Photopic weighted integral of the spectrum.");
			stat_row("Peak Wavelength", number("%.3f nm", statistics_.peak_wavelength_nm), "Wavelength of maximum radiance inside the window.");
			stat_row("Centroid Wavelength", number("%.3f nm", statistics_.centroid_nm), "Radiance-weighted mean wavelength inside the window.");
			stat_row("Full Width At Half Maximum", number("%.3f nm", statistics_.fwhm_nm), "Width of the main peak at half of its maximum radiance.");
			stat_row("Window-Integrated Radiance", number("%.4e W/m^2/sr", statistics_.window_integral), "Integral of the radiance over the displayed wavelength window.");
			stat_row("Bolometric Radiance", number("%.4e W/m^2/sr", statistics_.bolometric), "Integral over the full spectral grid from gamma rays to radio, using logarithmic quadrature.");
			stat_row("Wien-Equivalent Temperature", (statistics_.wien_temperature_k > 0.0) ? number("%.0f K", statistics_.wien_temperature_k) : std::string("Peak outside window"), "Blackbody temperature whose spectral peak matches the observed peak.");
			if (spectrum_type_ == 0) {
				stat_row("Observed Temperature (g T)", number("%.0f K", static_cast<double>(temperature_k_) * static_cast<double>(doppler_shift_factor_)), "Color temperature after the spectral shift, T_obs = g T_emit.");
			}
			ImGui::EndTable();
		}

		if (ImGui::BeginTable("##SpectralBands", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
			ImGui::TableSetupColumn("Band");
			ImGui::TableSetupColumn("Range (nm)");
			ImGui::TableSetupColumn("Radiance (W/m^2/sr)");
			ImGui::TableSetupColumn("Share");
			ImGui::TableHeadersRow();
			for (const auto& band : bands_) {
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::TextUnformatted(band.name);
				ImGui::TableSetColumnIndex(1);
				ImGui::Text("%.3g - %.3g", band.lower_nm, band.upper_nm);
				ImGui::TableSetColumnIndex(2);
				ImGui::Text("%.4e", band.integral);
				ImGui::TableSetColumnIndex(3);
				ImGui::ProgressBar(static_cast<float>(band.fraction), ImVec2(-1.0f, 0.0f));
			}
			ImGui::EndTable();
		}
		render_setting_tooltip("Radiance integrated in each spectral band with logarithmic quadrature and the share of the sum of the bands.");

		ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.4f, 1.0f), "Detected Spectral Features (%zu)", peaks_.size());
		if (peaks_.empty()) {
			ImGui::TextDisabled("No feature exceeds the current detection sensitivity.");
		} else if (ImGui::BeginTable("##SpectralPeaks", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
			ImGui::TableSetupColumn("Rank");
			ImGui::TableSetupColumn("Wavelength (nm)");
			ImGui::TableSetupColumn("Radiance");
			ImGui::TableSetupColumn("Prominence");
			ImGui::TableSetupColumn("FWHM (nm)");
			ImGui::TableHeadersRow();
			for (size_t i = 0; i < peaks_.size(); ++i) {
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::Text("%zu%s", i + 1, peaks_[i].at_window_edge ? " (edge)" : "");
				ImGui::TableSetColumnIndex(1);
				ImGui::Text("%.4g", peaks_[i].wavelength_nm);
				ImGui::TableSetColumnIndex(2);
				ImGui::Text("%.3e", peaks_[i].intensity);
				ImGui::TableSetColumnIndex(3);
				ImGui::Text("%.3e", peaks_[i].prominence);
				ImGui::TableSetColumnIndex(4);
				ImGui::Text("%.4g", peaks_[i].fwhm_nm);
			}
			ImGui::EndTable();
		}
		render_setting_tooltip("Local maxima found on the smoothed spectrum, ranked by prominence. Features flagged edge continue beyond the displayed window.");
	}

	void render_plots(const Orchestrator::SimulationOrchestrator<1024>& orchestrator, const Optics::RayProbeResult& probe) {
		const auto& params = orchestrator.parameters();
		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.6f, 0.9f, 1.0f, 1.0f), "Live Accretion Disk Temperature Profile");
		render_setting_tooltip("Local disk temperature versus radius using the profile of the viewport renderer, evaluated from the current mass and spin. The vertical lines mark the observer and the probed emission point.");
		recompute_disk_profile(params.mass, params.spin);

		if (ImPlot::BeginPlot("Disk Temperature Profile", ImVec2(-1, 190))) {
			ImPlot::SetupAxes("Radius (M)", "Temperature (K)");
			ImPlot::PlotLine("T(r)", disk_radius_.data(), disk_temperature_k_.data(), static_cast<int>(kDiskProfileSamples));
			const double observer_radius = camera_linked_radius_;
			ImPlot::PlotInfLines("Observer Radius", &observer_radius, 1);
			if (probe.valid && probe.disk_hit) {
				const double emission_radius = probe.disk_radius;
				ImPlot::PlotInfLines("Probe Emission Radius", &emission_radius, 1);
			}
			ImPlot::EndPlot();
		}

		ImGui::Separator();
		const ImPlotCond limit_condition = plot_refit_ ? ImPlotCond_Always : ImPlotCond_Once;
		if (ImPlot::BeginPlot("Spectral Radiance I(lambda)", ImVec2(-1, 360))) {
			ImPlot::SetupAxes("Observed Wavelength (nm)", "Radiance (W/m^2/sr/m)");
			if (log_wavelength_axis_) {
				ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Log10);
			}
			if (log_intensity_axis_) {
				ImPlot::SetupAxisScale(ImAxis_Y1, ImPlotScale_Log10);
			}
			const double peak = std::max(statistics_.peak_intensity, 1e-300);
			ImPlot::SetupAxisLimits(ImAxis_X1, wavelengths_nm_.front(), wavelengths_nm_.back(), limit_condition);
			ImPlot::SetupAxisLimits(ImAxis_Y1, log_intensity_axis_ ? peak * 1e-8 : 0.0, peak * 1.25, limit_condition);

			std::vector<double> lower_plot = lower_band_;
			if (log_intensity_axis_) {
				for (double& value : lower_plot) {
					value = std::max(value, peak * 1e-12);
				}
			}
			const int count = static_cast<int>(wavelengths_nm_.size());
			if (uncertainty_fraction_ > 0.0f) {
				ImPlot::PlotShaded("Uncertainty Band", wavelengths_nm_.data(), lower_plot.data(), upper_band_.data(), count);
			}
			if (show_emitted_reference_) {
				ImPlot::PlotLine("Emitted (Rest Frame)", wavelengths_nm_.data(), emitted_intensities_.data(), count);
			}
			ImPlot::PlotLine("Observed Spectral Radiance", wavelengths_nm_.data(), intensities_.data(), count);

			const double visible_edges[2] = {380.0, 780.0};
			ImPlot::PlotInfLines("Visible Limits", visible_edges, 2);

			if (!peaks_.empty()) {
				std::vector<double> peak_x;
				std::vector<double> peak_y;
				for (const auto& feature : peaks_) {
					peak_x.push_back(feature.wavelength_nm);
					peak_y.push_back(feature.intensity);
				}
				ImPlot::PlotScatter("Detected Features", peak_x.data(), peak_y.data(), static_cast<int>(peak_x.size()));
				const size_t labelled = std::min<size_t>(peaks_.size(), 4);
				for (size_t i = 0; i < labelled; ++i) {
					char label[48];
					std::snprintf(label, sizeof(label), "%.4g nm", peaks_[i].wavelength_nm);
					ImPlot::PlotText(label, peaks_[i].wavelength_nm, peaks_[i].intensity, ImVec2(0.0f, -12.0f));
				}
			}
			ImPlot::EndPlot();
		}
		plot_refit_ = false;
	}

	static void stat_row(const char* label, const std::string& value, const char* tip) {
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

public:
	SpectrographWindow() {
		recompute_spectrum();
	}

	[[nodiscard]] bool& open_state() noexcept {
		return is_open_;
	}

	void attach_ray_probe(const Optics::RayProbeResult* result) noexcept {
		probe_result_ = result;
	}

	void set_intensity_provider(InterferometryPanel::ImageProvider provider) {
		interferometry_panel_.set_image_provider(std::move(provider));
	}

	[[nodiscard]] int ray_probe_source() const noexcept {
		return ray_probe_panel_.source();
	}

	[[nodiscard]] bool ray_probe_frozen() const noexcept {
		return ray_probe_panel_.frozen();
	}

	void toggle_ray_probe_freeze() noexcept {
		ray_probe_panel_.toggle_freeze();
	}

	[[nodiscard]] const HudLinkedReadouts& linked_readouts() const noexcept {
		return linked_readouts_;
	}

	void render(const Orchestrator::SimulationOrchestrator<1024>& orchestrator) {
		if (!is_open_) return;

		update_links(orchestrator);
		const Optics::PolarizedPlasmaState<double> plasma = build_plasma();
		polarization_panel_.update(orchestrator, plasma, static_cast<double>(doppler_shift_factor_));
		interferometry_panel_.update();

		ImGui::SetNextWindowPos(ImVec2(340.0f, 750.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(760.0f, 620.0f), ImGuiCond_FirstUseEver);

		if (ImGui::Begin("Radiative Transfer & Spectrograph Monitor", &is_open_)) {
			ImGui::TextWrapped("Spectral synthesis laboratory for the emissivity, absorptivity, polarization and Doppler models used by the renderer. It reads the live simulation, the ray probe of the viewport and the jet magnetosphere, never writes back into the render pipeline, and feeds the HUD readouts.");
			ImGui::Separator();
			render_header();

			if (ImGui::BeginTabBar("##SpectrographTabs")) {
				if (ImGui::BeginTabItem("Spectrum")) {
					render_spectrum_tab(orchestrator);
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Polarization")) {
					polarization_panel_.render(orchestrator);
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Ray Probe")) {
					ray_probe_panel_.render(orchestrator, (probe_result_ != nullptr) ? *probe_result_ : empty_probe_);
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Interferometry")) {
					interferometry_panel_.render(orchestrator);
					ImGui::EndTabItem();
				}
				ImGui::EndTabBar();
			}
		}
		ImGui::End();
		refresh_linked_readouts();
	}
};

}

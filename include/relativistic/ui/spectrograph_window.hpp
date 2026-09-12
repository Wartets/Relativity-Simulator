#pragma once

#include <imgui.h>
#include <implot.h>
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/optics/spectrum.hpp"
#include "relativistic/optics/cie_observer.hpp"
#include "relativistic/optics/radiative_processes.hpp"
#include "relativistic/optics/disk_thermal_profile.hpp"
#include "relativistic/core/constants.hpp"
#include "relativistic/ui/tooltip_utils.hpp"
#include "relativistic/units/unit_system.hpp"
#include <vector>
#include <cmath>
#include <algorithm>
#include <numbers>

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
	bool link_doppler_to_camera_{false};
	double camera_linked_g_{1.0};
	bool camera_within_disk_band_{false};
	double camera_linked_radius_{0.0};

	std::vector<double> wavelengths_nm_{};
	std::vector<double> intensities_{};
	std::vector<double> upper_band_{};
	std::vector<double> lower_band_{};
	Optics::ColorRGB perceived_color_{};

	static constexpr size_t kDiskProfileSamples = 60;
	std::vector<double> disk_radius_{};
	std::vector<double> disk_temperature_k_{};

	[[nodiscard]] static double kerr_isco_radius(double m, double a_spin) noexcept {
		return Optics::DiskThermalProfile::kerr_isco_radius(m, a_spin);
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
			ImGui::TextWrapped("Standalone spectral synthesis laboratory for inspecting the same emissivity, absorptivity, and Doppler models the accretion disk renderer uses internally, without re-rendering the viewport. It never writes back into the render pipeline, but every quantity below can read live mass, spin, and observer position from the active simulation when the relevant link option is enabled.");
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
					ImGui::TextDisabled("%s", Units::format_temperature(static_cast<double>(temperature_k_), orchestrator.unit_preferences().temperature).c_str());
				}
			}

			const auto& params = orchestrator.parameters();
			const auto& cam = orchestrator.camera();
			camera_linked_radius_ = cam.radius;
			camera_within_disk_band_ = Optics::DiskThermalProfile::radius_within_disk(params.mass, params.spin, cam.radius);
			camera_linked_g_ = camera_within_disk_band_
				? Optics::DiskThermalProfile::circular_orbit_redshift_factor(params.mass, cam.radius)
				: 1.0;

			if (ImGui::Checkbox("Link Doppler Factor To Camera Line-Of-Sight", &link_doppler_to_camera_)) {
				if (link_doppler_to_camera_) {
					doppler_shift_factor_ = static_cast<float>(std::clamp(camera_linked_g_, 0.05, 10.0));
					recompute_spectrum();
				}
			}
			render_setting_tooltip("When enabled, the Doppler factor below is computed from the observer's actual current radius using the same circular-orbit redshift approximation the accretion disk renderer applies, instead of being set manually. Only meaningful while the observer sits within the disk's ISCO-to-outer-edge band; outside that band the factor falls back to 1.0.");

			if (link_doppler_to_camera_) {
				ImGui::BeginDisabled(true);
			}
			if (ImGui::SliderFloat("Doppler Factor (g)", &doppler_shift_factor_, 0.05f, 10.0f, "%.3f")) {
				recompute_spectrum();
			}
			if (link_doppler_to_camera_) {
				ImGui::EndDisabled();
				if (!camera_within_disk_band_) {
					ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.3f, 1.0f), "Observer at r=%.2f M is outside the disk band [%.2f M, %.2f M]; using g=1.0.", camera_linked_radius_, Optics::DiskThermalProfile::kerr_isco_radius(params.mass, params.spin), Optics::DiskThermalProfile::disk_outer_radius(params.mass));
				} else {
					ImGui::TextColored(ImVec4(0.4f, 0.95f, 0.5f, 1.0f), "Linked to observer radius r=%.2f M -> g=%.4f.", camera_linked_radius_, camera_linked_g_);
				}
				if (std::abs(static_cast<double>(doppler_shift_factor_) - camera_linked_g_) > 1e-6) {
					doppler_shift_factor_ = static_cast<float>(std::clamp(camera_linked_g_, 0.05, 10.0));
					recompute_spectrum();
				}
			}
			render_setting_tooltip("Combined gravitational and kinematic redshift/blueshift factor g = nu_obs / nu_emit applied to the whole spectrum, matching the factor used by the accretion disk renderer.");
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

			if (spectrum_type_ == 1) {
				ImGui::Separator();
				ImGui::TextColored(ImVec4(0.75f, 0.6f, 1.0f, 1.0f), "Synchrotron Plasma Parameters");
				ImGui::TextDisabled("Feeds the same emissivity and absorptivity models used by the polarized radiative transfer engine, evaluated here in isolation for inspection.");

				float mag_log = std::log10(std::max(magnetic_field_tesla_, 1e-6f));
				if (ImGui::SliderFloat("Magnetic Field (log10 Tesla)", &mag_log, -4.0f, 3.0f, "10^%.2f T")) {
					magnetic_field_tesla_ = std::pow(10.0f, mag_log);
				}
				render_setting_tooltip("Local magnetic field strength threading the emitting plasma. Astrophysical accretion flows near stellar-mass black holes typically range from roughly 10 to 10000 Tesla close to the horizon.");

				float density_log = std::log10(std::max(electron_density_, 1.0f));
				if (ImGui::SliderFloat("Electron Density (log10 per m^3)", &density_log, 10.0f, 24.0f, "10^%.2f m^-3")) {
					electron_density_ = std::pow(10.0f, density_log);
				}
				render_setting_tooltip("Number density of relativistic electrons contributing to synchrotron emission. Higher densities raise both emissivity and self-absorption.");

				Optics::PolarizedPlasmaState<double> plasma;
				plasma.electron_density = static_cast<double>(electron_density_);
				plasma.magnetic_field_tesla = static_cast<double>(magnetic_field_tesla_);
				plasma.non_thermal_fraction = 0.01;
				plasma.gamma_min = 10.0;
				plasma.power_law_index = 3.0;

				const double nu_ref = 299792458.0 / 550e-9;
				const double cyclotron_hz = Optics::RadiativeProcessEngine<double>::synchrotron_cyclotron_frequency(plasma.magnetic_field_tesla);
				const auto emissivity = Optics::RadiativeProcessEngine<double>::non_thermal_synchrotron_emissivity(nu_ref, plasma);
				const auto absorptivity = Optics::RadiativeProcessEngine<double>::non_thermal_synchrotron_absorptivity(nu_ref, plasma);
				constexpr double sigma_thomson = 6.6524587321e-29;
				const double electron_mass = Core::PhysicalConstants<double>::ELECTRON_MASS;
				const double speed_of_light = Core::PhysicalConstants<double>::SPEED_OF_LIGHT;
				const double cooling_time_s = (plasma.magnetic_field_tesla > 1e-9)
					? (6.0 * std::numbers::pi_v<double> * electron_mass * speed_of_light) / (sigma_thomson * plasma.magnetic_field_tesla * plasma.magnetic_field_tesla * std::max(plasma.gamma_min, 1.0))
					: 0.0;

				ImGui::Text("Electron Cyclotron Frequency: %.4e Hz", cyclotron_hz);
				render_setting_tooltip("Non-relativistic gyration frequency of electrons around the field line, nu_c = eB / (2*pi*m_e). The synchrotron spectrum peaks near gamma^2 times this frequency for relativistic electrons.");
				ImGui::Text("Emissivity At 550nm (j_i, j_q): %.3e, %.3e W/m^3/sr/Hz", emissivity.j_i, emissivity.j_q);
				ImGui::Text("Absorptivity At 550nm (alpha_i): %.3e 1/m", absorptivity.alpha_i);
				render_setting_tooltip("Stokes-I emission and absorption coefficients from the non-thermal power-law electron population at a reference visible wavelength, using the current field strength and density.");
				ImGui::Text("Approx. Synchrotron Cooling Time (gamma=%.0f): %.3e s", plasma.gamma_min, cooling_time_s);
				render_setting_tooltip("Characteristic timescale for a relativistic electron at the minimum Lorentz factor to radiate away an order-unity fraction of its energy via synchrotron emission. Shorter times indicate a plasma that cannot sustain this emission for long without continuous re-acceleration.");
			}

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

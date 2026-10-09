#pragma once

#include <imgui.h>
#include <implot.h>
#include "relativistic/core/constants.hpp"
#include "relativistic/interferometry/oifits_writer.hpp"
#include "relativistic/interferometry/visibility_synthesis.hpp"
#include "relativistic/interferometry/vlbi_array.hpp"
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/ui/hud/hud_linked_readouts.hpp"
#include "relativistic/ui/numeric_slider_utils.hpp"
#include "relativistic/ui/tooltip_utils.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <functional>
#include <future>
#include <memory>
#include <numbers>
#include <string>
#include <utility>
#include <vector>

namespace Relativistic::UI {

class InterferometryPanel {
public:
	using ImageProvider = std::function<bool(Interferometry::IntensityImage&, uint32_t)>;

	InterferometryPanel() {
		array_ = Interferometry::VlbiArrayCatalog::preset(0);
		apply_source_preset(0);
		std::snprintf(output_path_.data(), output_path_.size(), "%s", "output/oifits/synthetic_eht.oifits");
	}

	~InterferometryPanel() {
		if (control_) {
			control_->cancel.store(true);
		}
		if (task_.valid()) {
			task_.wait();
		}
	}

	InterferometryPanel(const InterferometryPanel&) = delete;
	InterferometryPanel& operator=(const InterferometryPanel&) = delete;

	void set_image_provider(ImageProvider provider) {
		provider_ = std::move(provider);
	}

	void update() {
		poll_task();
	}

	[[nodiscard]] HudInterferometryReadout hud_summary() const noexcept {
		HudInterferometryReadout readout;
		if (!dataset_.valid) {
			return readout;
		}
		readout.valid = true;
		readout.visibility_count = cache_.amplitude.size();
		readout.closure_count = dataset_.closures.size();
		readout.maximum_baseline_glambda = dataset_.maximum_baseline_lambda * 1.0e-9;
		readout.resolution_uas = (dataset_.maximum_baseline_lambda > 0.0) ? ((1.0 / dataset_.maximum_baseline_lambda) * 206264.806247096355 * 1.0e6) : 0.0;
		readout.mean_snr = dataset_.mean_snr;
		return readout;
	}

	void render(const Orchestrator::SimulationOrchestrator<1024>& orchestrator) {
		poll_task();
		render_source_controls(orchestrator);
		render_array_controls();
		render_observation_controls();
		render_actions(orchestrator);
		render_results();
	}

private:
	struct PlotCache {
		std::vector<double> sky_u_glambda{};
		std::vector<double> sky_v_glambda{};
		std::vector<double> sky_u_conj_glambda{};
		std::vector<double> sky_v_conj_glambda{};
		std::vector<double> image_u_glambda{};
		std::vector<double> image_v_glambda{};
		std::vector<double> image_u_conj_glambda{};
		std::vector<double> image_v_conj_glambda{};
		std::vector<double> baseline_glambda{};
		std::vector<double> amplitude{};
		std::vector<double> amplitude_error{};
		std::vector<double> phase_deg{};
		std::vector<double> phase_error_deg{};
		std::vector<double> closure_time_hours{};
		std::vector<double> closure_phase_deg{};
		std::vector<double> closure_phase_err_deg{};
		std::vector<double> profile_baseline_glambda{};
		std::vector<double> profile_amplitude_jy{};
	};

	[[nodiscard]] double theta_g_rad() const noexcept {
		constexpr double g = Core::PhysicalConstants<double>::GRAVITATIONAL_CONSTANT;
		constexpr double c = Core::PhysicalConstants<double>::SPEED_OF_LIGHT;
		constexpr double solar = Core::PhysicalConstants<double>::SOLAR_MASS;
		constexpr double parsec = Core::PhysicalConstants<double>::PARSEC;
		return g * mass_msun_ * solar / (c * c * std::max(distance_pc_, 1e-6) * parsec);
	}

	[[nodiscard]] double theta_g_uas() const noexcept {
		constexpr double rad_to_uas = 206264.806247096355 * 1.0e6;
		return theta_g_rad() * rad_to_uas;
	}

	void apply_source_preset(int preset) {
		switch (preset) {
			case 0:
				mass_msun_ = 6.5e9;
				distance_pc_ = 16.8e6;
				right_ascension_deg_ = 187.7059;
				declination_deg_ = 12.3911;
				flux_jy_ = 0.5;
				position_angle_deg_ = 288.0;
				frequency_ghz_ = 230.0;
				std::snprintf(target_name_.data(), target_name_.size(), "%s", "M87");
				break;
			case 1:
				mass_msun_ = 4.15e6;
				distance_pc_ = 8178.0;
				right_ascension_deg_ = 266.4168;
				declination_deg_ = -29.0078;
				flux_jy_ = 2.4;
				position_angle_deg_ = 0.0;
				frequency_ghz_ = 230.0;
				std::snprintf(target_name_.data(), target_name_.size(), "%s", "SGRA");
				break;
			default:
				break;
		}
	}

	void render_source_controls(const Orchestrator::SimulationOrchestrator<1024>& orchestrator) {
		ImGui::TextColored(ImVec4(0.5f, 0.85f, 1.0f, 1.0f), "Source Calibration");
		const char* presets[] = {"M87* (EHT 2017)", "Sgr A* (EHT 2017)", "Custom"};
		if (ImGui::Combo("Source Preset", &source_preset_, presets, IM_ARRAYSIZE(presets))) {
			apply_source_preset(source_preset_);
		}
		render_setting_tooltip("Loads the black hole mass, distance, sky position, total flux and jet position angle of a reference source.");

		slider_double_with_input("Black Hole Mass (M_sun)", &mass_msun_, 1e3, 1e12, "%.4e", &log_mass_, 1e3f, 1e12f);
		slider_double_with_input("Distance (pc)", &distance_pc_, 1.0, 1e10, "%.4e", &log_distance_, 1.0f, 1e10f);
		ImGui::SameLine();
		if (ImGui::Button("Use Simulation Mass")) {
			const double mass_kg = orchestrator.parameters().mass * orchestrator.constants_engine().mass_scale();
			mass_msun_ = std::clamp(mass_kg / Core::PhysicalConstants<double>::SOLAR_MASS, 1e3, 1e12);
			source_preset_ = 2;
		}
		render_setting_tooltip("Replaces the mass above with the central mass of the running simulation converted through the active constants.");

		ImGui::InputDouble("Right Ascension (deg)", &right_ascension_deg_, 0.0, 0.0, "%.5f");
		ImGui::InputDouble("Declination (deg)", &declination_deg_, 0.0, 0.0, "%.5f");
		ImGui::InputDouble("Total Flux Density (Jy)", &flux_jy_, 0.0, 0.0, "%.4f");
		ImGui::InputDouble("Image Position Angle (deg E of N)", &position_angle_deg_, 0.0, 0.0, "%.2f");
		render_setting_tooltip("Position angle on the sky of the image vertical axis, measured from north through east. East is displayed to the left.");

		ImGui::InputText("Target Name", target_name_.data(), target_name_.size());
		ImGui::TextDisabled("Gravitational angular scale theta_g = GM/(c^2 D): %.4f microarcseconds per M", theta_g_uas());
		render_setting_tooltip("Angular size subtended by one gravitational radius at the source distance. It converts simulation lengths to sky angles.");
	}

	void render_array_controls() {
		ImGui::Separator();
		ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.35f, 1.0f), "VLBI Array");
		if (ImGui::BeginCombo("Array Preset", Interferometry::VlbiArrayCatalog::preset_name(static_cast<size_t>(array_preset_)))) {
			for (size_t i = 0; i < Interferometry::VlbiArrayCatalog::kPresetCount; ++i) {
				if (ImGui::Selectable(Interferometry::VlbiArrayCatalog::preset_name(i), static_cast<int>(i) == array_preset_)) {
					array_preset_ = static_cast<int>(i);
					array_ = Interferometry::VlbiArrayCatalog::preset(i);
				}
			}
			ImGui::EndCombo();
		}
		render_setting_tooltip("Selects a ground array. Space-orbiter presets add an orbiting antenna with a Keplerian circular orbit.");

		if (ImGui::BeginTable("##VlbiStations", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
			ImGui::TableSetupColumn("Use", ImGuiTableColumnFlags_WidthFixed, 40.0f);
			ImGui::TableSetupColumn("Station");
			ImGui::TableSetupColumn("Diameter (m)", ImGuiTableColumnFlags_WidthFixed, 90.0f);
			ImGui::TableSetupColumn("SEFD (Jy)", ImGuiTableColumnFlags_WidthFixed, 100.0f);
			ImGui::TableSetupColumn("Visible Epochs", ImGuiTableColumnFlags_WidthFixed, 110.0f);
			ImGui::TableHeadersRow();

			for (size_t i = 0; i < array_.stations.size(); ++i) {
				auto& station = array_.stations[i];
				ImGui::PushID(static_cast<int>(i));
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::Checkbox("##use", &station.enabled);
				ImGui::TableSetColumnIndex(1);
				ImGui::Text("%s (%s)%s", station.telescope.c_str(), station.code.c_str(), station.kind == Interferometry::StationKind::SpaceOrbit ? " [space]" : "");
				ImGui::TableSetColumnIndex(2);
				ImGui::Text("%.1f", station.diameter_m);
				ImGui::TableSetColumnIndex(3);
				ImGui::SetNextItemWidth(90.0f);
				ImGui::InputDouble("##sefd", &station.sefd_jy, 0.0, 0.0, "%.0f");
				ImGui::TableSetColumnIndex(4);
				if (dataset_.valid && i < dataset_.station_visible_epochs.size()) {
					ImGui::Text("%u / %zu", dataset_.station_visible_epochs[i], dataset_.epoch_count);
				} else {
					ImGui::TextDisabled("-");
				}
				ImGui::PopID();
			}
			ImGui::EndTable();
		}

		for (size_t i = 0; i < array_.stations.size(); ++i) {
			auto& station = array_.stations[i];
			if (station.kind != Interferometry::StationKind::SpaceOrbit) continue;
			ImGui::PushID(static_cast<int>(i) + 5000);
			ImGui::TextDisabled("Orbit of %s", station.telescope.c_str());
			slider_double_with_input("Orbit Altitude (km)", &station.orbit_altitude_km, 300.0, 400000.0, "%.0f", &log_altitude_, 300.0f, 400000.0f);
			slider_double_with_input("Inclination (deg)", &station.orbit_inclination_deg, 0.0, 180.0, "%.1f");
			slider_double_with_input("RAAN (deg)", &station.orbit_raan_deg, 0.0, 360.0, "%.1f");
			slider_double_with_input("Initial Phase (deg)", &station.orbit_phase_deg, 0.0, 360.0, "%.1f");
			ImGui::PopID();
		}
	}

	void render_observation_controls() {
		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.6f, 0.9f, 0.6f, 1.0f), "Observation Schedule And Noise");

		slider_double_with_input("Observing Frequency (GHz)", &frequency_ghz_, 1.0, 1000.0, "%.2f", &log_frequency_, 1.0f, 1000.0f);
		slider_double_with_input("Bandwidth (GHz)", &bandwidth_ghz_, 0.01, 32.0, "%.3f", &log_bandwidth_, 0.01f, 32.0f);
		slider_double_with_input("Integration Time (s)", &integration_time_s_, 0.5, 300.0, "%.1f");
		render_setting_tooltip("Accumulation time per raw visibility record. Standard EHT continuum scans use 10 seconds.");

		slider_double_with_input("Track Cadence (s)", &cadence_s_, 30.0, 3600.0, "%.0f");
		slider_double_with_input("Observation Duration (hours)", &duration_hours_, 1.0, 72.0, "%.1f");
		slider_double_with_input("Start UT (hours)", &start_hour_ut_, 0.0, 24.0, "%.2f");
		ImGui::InputDouble("Start MJD", &start_mjd_, 0.0, 0.0, "%.1f");
		render_setting_tooltip("Modified Julian Date of the observing epoch (e.g. 57854 for April 11, 2017).");

		slider_double_with_input("Min Station Elevation (deg)", &minimum_elevation_deg_, 0.0, 45.0, "%.1f");
		slider_double_with_input("Quantization Efficiency", &quantization_efficiency_, 0.5, 1.0, "%.2f");
		render_setting_tooltip("Digital 2-bit correlation loss factor (typically 0.88 for standard 2-bit 4-level sampling).");

		ImGui::Checkbox("Add Thermal Gaussian Noise", &thermal_noise_);
		render_setting_tooltip("Synthesizes baseline thermal noise from the station SEFD values, bandwidth and integration time.");
		if (thermal_noise_) {
			ImGui::SameLine();
			ImGui::SetNextItemWidth(140.0f);
			ImGui::InputScalar("Noise Seed", ImGuiDataType_U64, &noise_seed_);
		}

		const char* resolutions[] = {"64 x 64", "128 x 128", "256 x 256", "512 x 512"};
		int res_index = 1;
		if (image_resolution_ == 64) res_index = 0;
		else if (image_resolution_ == 128) res_index = 1;
		else if (image_resolution_ == 256) res_index = 2;
		else if (image_resolution_ == 512) res_index = 3;
		if (ImGui::Combo("Synthesis Grid Resolution", &res_index, resolutions, IM_ARRAYSIZE(resolutions))) {
			const uint32_t values[] = {64, 128, 256, 512};
			image_resolution_ = values[res_index];
		}
		render_setting_tooltip("Resolution of the square intensity grid downsampled from the viewport for DFT and FFT processing.");

		ImGui::Checkbox("Auto-Calibrate Pixel Scale From Simulation Viewport", &auto_pixel_scale_);
		render_setting_tooltip("When checked, one mass unit M in the viewport corresponds to theta_g on the sky. When unchecked, the manual pixel scale below is used.");
		if (!auto_pixel_scale_) {
			slider_double_with_input("Manual Pixel Scale (microarcseconds)", &manual_pixel_scale_uas_, 0.01, 100.0, "%.3f", &log_scale_, 0.01f, 100.0f);
		}
	}

	void render_actions(const Orchestrator::SimulationOrchestrator<1024>& orchestrator) {
		ImGui::Separator();
		if (synthesizing_) {
			const float progress = control_ ? control_->progress.load(std::memory_order_relaxed) : 0.0f;
			ImGui::ProgressBar(progress, ImVec2(320.0f, 0.0f), "Synthesizing Visibilities...");
			ImGui::SameLine();
			if (ImGui::Button("Cancel")) {
				if (control_) {
					control_->cancel.store(true, std::memory_order_relaxed);
				}
			}
		} else {
			if (ImGui::Button("Synthesize Visibilities From Viewport", ImVec2(320.0f, 32.0f))) {
				start_synthesis(orchestrator);
			}
			render_setting_tooltip("Grabs the current viewport luminance frame, computes baseline tracks across the observing window, evaluates exact DFT visibilities and generates closure phases.");
		}

		if (!status_message_.empty()) {
			ImVec4 msg_color(0.5f, 0.9f, 1.0f, 1.0f);
			if (status_message_.find("Error") != std::string::npos || status_message_.find("invalid") != std::string::npos || status_message_.find("missing") != std::string::npos) {
				msg_color = ImVec4(1.0f, 0.4f, 0.3f, 1.0f);
			}
			ImGui::TextColored(msg_color, "%s", status_message_.c_str());
		}
	}

	void start_synthesis(const Orchestrator::SimulationOrchestrator<1024>& orchestrator) {
		if (!provider_) {
			status_message_ = "Error: no image provider is hooked to the interferometry panel.";
			return;
		}

		Interferometry::IntensityImage image;
		if (!provider_(image, image_resolution_)) {
			status_message_ = "Error: unable to capture the viewport intensity frame.";
			return;
		}

		double pixel_scale_rad = 0.0;
		if (auto_pixel_scale_) {
			if (image.simulation_pixel_scale_rad > 0.0) {
				pixel_scale_rad = image.simulation_pixel_scale_rad * theta_g_rad();
			} else {
				const double r_obs = std::max(image.observer_radius > 0.0 ? image.observer_radius : orchestrator.camera().radius, 20.0);
				const double mass = std::max(orchestrator.parameters().mass, 1e-9);
				const double fov_rad = orchestrator.camera().fov_deg * std::numbers::pi_v<double> / 180.0;
				const double screen_span = 2.0 * r_obs * std::tan(0.5 * fov_rad);
				pixel_scale_rad = (screen_span / mass / static_cast<double>(image.size)) * theta_g_rad();
			}
		} else {
			constexpr double uas_to_rad = (1.0 / 206264.806247096355) * 1.0e-6;
			pixel_scale_rad = manual_pixel_scale_uas_ * uas_to_rad;
		}

		Interferometry::ObservationSettings settings;
		settings.frequency_hz = frequency_ghz_ * 1.0e9;
		settings.bandwidth_hz = bandwidth_ghz_ * 1.0e9;
		settings.integration_time_s = integration_time_s_;
		settings.source_right_ascension_deg = right_ascension_deg_;
		settings.source_declination_deg = declination_deg_;
		settings.image_position_angle_deg = position_angle_deg_;
		settings.total_flux_jy = flux_jy_;
		settings.start_mjd = start_mjd_;
		settings.start_hour_ut = start_hour_ut_;
		settings.duration_hours = duration_hours_;
		settings.cadence_s = cadence_s_;
		settings.minimum_elevation_deg = minimum_elevation_deg_;
		settings.quantization_efficiency = quantization_efficiency_;
		settings.thermal_noise = thermal_noise_;
		settings.noise_seed = noise_seed_;

		control_ = std::make_shared<Interferometry::SynthesisControl>();
		synthesizing_ = true;
		status_message_ = "Synthesis started in background threads...";

		auto array_snapshot = array_;
		task_ = std::async(std::launch::async, [img = std::move(image), arr = std::move(array_snapshot), set = settings, scale = pixel_scale_rad, ctrl = control_]() {
			return Interferometry::VisibilitySynthesizer::synthesize(img, arr, set, scale, ctrl.get());
		});
	}

	void poll_task() {
		if (!synthesizing_ || !task_.valid()) return;
		if (task_.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
			Interferometry::VlbiDataset result = task_.get();
			synthesizing_ = false;
			status_message_ = result.message;
			if (result.valid || !dataset_.valid) {
				dataset_ = std::move(result);
				if (dataset_.valid) {
					update_cache();
				}
			}
		}
	}

	void update_cache() {
		cache_ = PlotCache{};
		constexpr double rad_to_deg = 180.0 / std::numbers::pi_v<double>;
		const double t0 = dataset_.samples.empty() ? 0.0 : dataset_.samples.front().time_seconds;

		for (const auto& sample : dataset_.samples) {
			if (sample.flagged) continue;
			const double u_gl = sample.u_lambda * 1.0e-9;
			const double v_gl = sample.v_lambda * 1.0e-9;
			const double bl = std::hypot(u_gl, v_gl);
			const double amp = std::abs(sample.value);
			const double phi = std::arg(sample.value) * rad_to_deg;
			const double phi_err = (amp > 1e-30) ? std::min((sample.sigma_jy / amp) * rad_to_deg, 180.0) : 180.0;

			cache_.sky_u_glambda.push_back(u_gl);
			cache_.sky_v_glambda.push_back(v_gl);
			cache_.sky_u_conj_glambda.push_back(-u_gl);
			cache_.sky_v_conj_glambda.push_back(-v_gl);
			cache_.image_u_glambda.push_back(sample.image_u_lambda * 1.0e-9);
			cache_.image_v_glambda.push_back(sample.image_v_lambda * 1.0e-9);
			cache_.image_u_conj_glambda.push_back(-sample.image_u_lambda * 1.0e-9);
			cache_.image_v_conj_glambda.push_back(-sample.image_v_lambda * 1.0e-9);

			cache_.baseline_glambda.push_back(bl);
			cache_.amplitude.push_back(amp);
			cache_.amplitude_error.push_back(sample.sigma_jy);
			cache_.phase_deg.push_back(phi);
			cache_.phase_error_deg.push_back(phi_err);
		}

		for (const auto& closure : dataset_.closures) {
			cache_.closure_time_hours.push_back((closure.time_seconds - t0) / 3600.0);
			cache_.closure_phase_deg.push_back(closure.phase_rad * rad_to_deg);
			cache_.closure_phase_err_deg.push_back(closure.phase_error_rad * rad_to_deg);
		}

		cache_.profile_baseline_glambda = dataset_.profile_baseline_glambda;
		cache_.profile_amplitude_jy.resize(dataset_.profile_amplitude.size());
		for (size_t i = 0; i < dataset_.profile_amplitude.size(); ++i) {
			cache_.profile_amplitude_jy[i] = dataset_.profile_amplitude[i] * dataset_.settings.total_flux_jy;
		}
	}

	void render_results() {
		if (!dataset_.valid) return;

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.4f, 0.95f, 0.5f, 1.0f), "Synthesized Dataset Summary");

		const double max_bl_gl = dataset_.maximum_baseline_lambda * 1.0e-9;
		const double res_uas = (dataset_.maximum_baseline_lambda > 0.0)
			? ((1.0 / dataset_.maximum_baseline_lambda) * 206264.806247096355 * 1.0e6)
			: 0.0;
		const double max_bl_km = dataset_.maximum_baseline_lambda * dataset_.wavelength_m * 1.0e-3;

		ImGui::Text("Valid Visibilities: %zu | Flagged: %zu | Closure Triangles: %zu", cache_.amplitude.size(), dataset_.flagged_count, dataset_.closures.size());
		ImGui::Text("Maximum Baseline: %.2f Glambda (%.0f km) | Nominal Resolution: %.2f uas", max_bl_gl, max_bl_km, res_uas);
		render_setting_tooltip("Nominal fringe spacing lambda / B_max. Ground-based 230 GHz EHT reaches ~25 uas; space orbiters extend this to sub-10 uas.");
		ImGui::Text("Mean Baseline SNR: %.2f | Observing Wavelength: %.3f mm", dataset_.mean_snr, dataset_.wavelength_m * 1000.0);

		if (ImGui::BeginTabBar("##InterferometryTabs")) {
			if (ImGui::BeginTabItem("UV Coverage")) {
				render_uv_tab();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Visibility Amplitude")) {
				render_amplitude_tab();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Closure Phases")) {
				render_closure_tab();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Synthesized UV Plane Heatmap")) {
				render_plane_tab();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Visibility Table")) {
				render_table_tab();
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}

		render_export();
	}

	void render_uv_tab() {
		ImGui::Checkbox("Show Hermitian Conjugate Tracks (-u, -v)", &show_conjugate_uv_);
		render_setting_tooltip("Radio astronomy convention: visibilities are Hermitian V(-u, -v) = V*(u, v). Displaying conjugates completes the synthesized aperture.");

		if (ImPlot::BeginPlot("Synthesized UV Coverage", ImVec2(-1, 380), ImPlotFlags_Equal)) {
			ImPlot::SetupAxes("u (Glambda) [East -> Left]", "v (Glambda) [North]", ImPlotAxisFlags_Invert, ImPlotAxisFlags_None);
			if (!cache_.sky_u_glambda.empty()) {
				ImPlot::PlotScatter("Visibilities", cache_.sky_u_glambda.data(), cache_.sky_v_glambda.data(), static_cast<int>(cache_.sky_u_glambda.size()));
				if (show_conjugate_uv_) {
					ImPlot::PlotScatter("Conjugates", cache_.sky_u_conj_glambda.data(), cache_.sky_v_conj_glambda.data(), static_cast<int>(cache_.sky_u_conj_glambda.size()));
				}
			}
			ImPlot::EndPlot();
		}
	}

	void render_amplitude_tab() {
		if (ImPlot::BeginPlot("Visibility Amplitude vs Baseline Length", ImVec2(-1, 380))) {
			ImPlot::SetupAxes("Baseline (Glambda)", "Correlated Flux Density (Jy)", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);

			if (!cache_.profile_baseline_glambda.empty()) {
				ImPlot::PlotLine("Radial Profile (FFT)", cache_.profile_baseline_glambda.data(), cache_.profile_amplitude_jy.data(), static_cast<int>(cache_.profile_baseline_glambda.size()));
				if (dataset_.first_null_baseline_lambda > 0.0) {
					const double first_null = dataset_.first_null_baseline_lambda * 1.0e-9;
					ImPlot::PlotInfLines("First Null", &first_null, 1);
				}
			}

			if (!cache_.baseline_glambda.empty()) {
				ImPlot::PlotScatter("Measured Visibilities", cache_.baseline_glambda.data(), cache_.amplitude.data(), static_cast<int>(cache_.baseline_glambda.size()));
				if (dataset_.settings.thermal_noise) {
					ImPlot::PlotErrorBars("1-sigma Noise", cache_.baseline_glambda.data(), cache_.amplitude.data(), cache_.amplitude_error.data(), static_cast<int>(cache_.baseline_glambda.size()));
				}
			}
			ImPlot::EndPlot();
		}
		if (dataset_.first_null_baseline_lambda > 0.0) {
			constexpr double rad_to_uas = 206264.806247096355 * 1.0e6;
			ImGui::Text("First visibility null: %.3f Glambda | Equivalent thin-ring diameter: %.2f uas | Image Nyquist limit: %.2f Glambda", dataset_.first_null_baseline_lambda * 1.0e-9, dataset_.estimated_ring_diameter_rad * rad_to_uas, dataset_.nyquist_baseline_lambda * 1.0e-9);
			render_setting_tooltip("The first zero of the circularly averaged visibility amplitude of a thin ring of diameter d lies at u = 0.7655 / d, which yields the equivalent ring diameter of the synthesized image.");
		} else {
			ImGui::TextDisabled("No visibility null is resolved by the radial profile. The image field of view is too small or the source has no ring-like structure. Image Nyquist limit: %.2f Glambda.", dataset_.nyquist_baseline_lambda * 1.0e-9);
		}
	}

	void render_closure_tab() {
		if (cache_.closure_time_hours.empty()) {
			ImGui::TextDisabled("No closed station triangles were observed in this run.");
			return;
		}

		if (ImPlot::BeginPlot("Closure Phase vs Observing Time", ImVec2(-1, 380))) {
			ImPlot::SetupAxes("Elapsed Time (hours)", "Closure Phase (deg)");
			ImPlot::SetupAxisLimits(ImAxis_Y1, -190.0, 190.0, ImPlotCond_Always);

			ImPlot::PlotScatter("Closure Phase", cache_.closure_time_hours.data(), cache_.closure_phase_deg.data(), static_cast<int>(cache_.closure_time_hours.size()));
			if (dataset_.settings.thermal_noise) {
				ImPlot::PlotErrorBars("Phase Uncertainty", cache_.closure_time_hours.data(), cache_.closure_phase_deg.data(), cache_.closure_phase_err_deg.data(), static_cast<int>(cache_.closure_time_hours.size()));
			}
			ImPlot::EndPlot();
		}
		ImGui::TextDisabled("Closure phase is invariant to station-based atmospheric phase errors. Deviations from 0 deg and 180 deg indicate source structural asymmetry.");
	}

	void render_plane_tab() {
		if (dataset_.plane_log_amplitude.empty() || dataset_.plane_size == 0) return;

		const double half_span = 0.5 * static_cast<double>(dataset_.plane_size) * dataset_.plane_cell_lambda * 1.0e-9;
		if (ImPlot::BeginPlot("Synthesized UV Fourier Plane Heatmap", ImVec2(-1, 380), ImPlotFlags_Equal)) {
			ImPlot::SetupAxes("u (Glambda)", "v (Glambda)");
			ImPlot::SetupAxisLimits(ImAxis_X1, -half_span, half_span);
			ImPlot::SetupAxisLimits(ImAxis_Y1, -half_span, half_span);

			const ImPlotPoint bmin(-half_span, -half_span);
			const ImPlotPoint bmax(half_span, half_span);
			ImPlot::PlotHeatmap("log10 Correlated Amplitude", dataset_.plane_log_amplitude.data(), static_cast<int>(dataset_.plane_size), static_cast<int>(dataset_.plane_size), -4.0, 0.0, nullptr, bmin, bmax);

			if (!cache_.image_u_glambda.empty()) {
				ImPlot::PlotScatter("Coverage Points", cache_.image_u_glambda.data(), cache_.image_v_glambda.data(), static_cast<int>(cache_.image_u_glambda.size()));
				if (show_conjugate_uv_) {
					ImPlot::PlotScatter("Hermitian Conjugates", cache_.image_u_conj_glambda.data(), cache_.image_v_conj_glambda.data(), static_cast<int>(cache_.image_u_conj_glambda.size()));
				}
			}
			ImPlot::EndPlot();
		}
	}

	void render_table_tab() {
		if (ImGui::BeginTable("##VisibilitySamplesTable", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchProp, ImVec2(0, 340.0f))) {
			ImGui::TableSetupColumn("Time (s)", ImGuiTableColumnFlags_WidthFixed, 80.0f);
			ImGui::TableSetupColumn("Baseline", ImGuiTableColumnFlags_WidthFixed, 90.0f);
			ImGui::TableSetupColumn("u (Glambda)");
			ImGui::TableSetupColumn("v (Glambda)");
			ImGui::TableSetupColumn("Amp (Jy)");
			ImGui::TableSetupColumn("Phase (deg)");
			ImGui::TableSetupColumn("SNR");
			ImGui::TableHeadersRow();

			const size_t display_count = std::min<size_t>(dataset_.samples.size(), 300);
			for (size_t i = 0; i < display_count; ++i) {
				const auto& sample = dataset_.samples[i];
				if (sample.flagged) continue;
				const std::string baseline = dataset_.array.stations[sample.station_a].code + "-" + dataset_.array.stations[sample.station_b].code;
				const double amp = std::abs(sample.value);
				const double snr = (sample.sigma_jy > 0.0) ? (amp / sample.sigma_jy) : 999.0;

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::Text("%.1f", sample.time_seconds);
				ImGui::TableSetColumnIndex(1);
				ImGui::TextUnformatted(baseline.c_str());
				ImGui::TableSetColumnIndex(2);
				ImGui::Text("%.4f", sample.u_lambda * 1.0e-9);
				ImGui::TableSetColumnIndex(3);
				ImGui::Text("%.4f", sample.v_lambda * 1.0e-9);
				ImGui::TableSetColumnIndex(4);
				ImGui::Text("%.4f", amp);
				ImGui::TableSetColumnIndex(5);
				ImGui::Text("%.2f", std::arg(sample.value) * 180.0 / std::numbers::pi_v<double>);
				ImGui::TableSetColumnIndex(6);
				ImGui::Text("%.1f", snr);
			}
			ImGui::EndTable();
		}
		if (dataset_.samples.size() > 300) {
			ImGui::TextDisabled("Showing first 300 of %zu visibilities. Use OIFITS export for the complete dataset.", dataset_.samples.size());
		}
	}

	void render_export() {
		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.4f, 1.0f), "OIFITS Standard Exporter");
		ImGui::SetNextItemWidth(360.0f);
		ImGui::InputText("Export Destination (.oifits / .fits)", output_path_.data(), output_path_.size());
		ImGui::SameLine();

		if (ImGui::Button("Export OIFITS File", ImVec2(180.0f, 0.0f))) {
			Interferometry::OifitsTarget target;
			target.name = target_name_.data();
			target.instrument = "EHT_VLBI_SYNTH";
			target.observer = "Relativistic Engine";
			target.right_ascension_deg = right_ascension_deg_;
			target.declination_deg = declination_deg_;

			std::string error;
			const bool ok = Interferometry::OifitsWriter::write(dataset_, target, output_path_.data(), error);
			if (ok) {
				export_status_ = "Successfully exported OIFITS to: " + std::string(output_path_.data());
			} else {
				export_status_ = "Export failed: " + error;
			}
		}
		render_setting_tooltip("Exports standard OI_TARGET, OI_ARRAY, OI_WAVELENGTH, OI_VIS, OI_VIS2 and OI_T3 binary tables, ready for eht-imaging, DIFMAP or SMILI.");

		if (!export_status_.empty()) {
			const bool is_err = export_status_.find("failed") != std::string::npos;
			ImGui::TextColored(is_err ? ImVec4(1.0f, 0.4f, 0.3f, 1.0f) : ImVec4(0.4f, 0.95f, 0.5f, 1.0f), "%s", export_status_.c_str());
		}
	}

	int source_preset_{0};
	int array_preset_{0};
	double mass_msun_{6.5e9};
	double distance_pc_{16.8e6};
	double right_ascension_deg_{187.7059};
	double declination_deg_{12.3911};
	double flux_jy_{0.5};
	double position_angle_deg_{288.0};
	std::array<char, 64> target_name_{"M87"};

	double frequency_ghz_{230.0};
	double bandwidth_ghz_{2.0};
	double integration_time_s_{10.0};
	double cadence_s_{600.0};
	double duration_hours_{24.0};
	double start_hour_ut_{0.0};
	double start_mjd_{57854.0};
	double minimum_elevation_deg_{10.0};
	double quantization_efficiency_{0.88};
	bool thermal_noise_{true};
	uint64_t noise_seed_{20170411ULL};

	uint32_t image_resolution_{128};
	bool auto_pixel_scale_{true};
	double manual_pixel_scale_uas_{1.0};
	bool show_conjugate_uv_{true};

	bool log_mass_{true};
	bool log_distance_{true};
	bool log_frequency_{true};
	bool log_bandwidth_{true};
	bool log_altitude_{true};
	bool log_scale_{true};

	Interferometry::VlbiArray array_{};
	Interferometry::VlbiDataset dataset_{};
	PlotCache cache_{};

	std::shared_ptr<Interferometry::SynthesisControl> control_{nullptr};
	std::future<Interferometry::VlbiDataset> task_{};
	bool synthesizing_{false};
	std::string status_message_{};

	std::array<char, 260> output_path_{};
	std::string export_status_{};
	ImageProvider provider_{nullptr};
};

}

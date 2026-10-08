#pragma once

#include "relativistic/core/constants.hpp"
#include "relativistic/optics/polarized_radiative_transfer.hpp"
#include "relativistic/optics/radiative_processes.hpp"
#include "relativistic/optics/stokes_vector.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numbers>
#include <vector>

namespace Relativistic::Optics {

enum class PolarizationEmissionModel : uint32_t {
	NonThermalPowerLaw = 0,
	ThermalMaxwellJuttner = 1,
	Hybrid = 2
};

struct PolarizedSpectrumSettings {
	PolarizationEmissionModel model{PolarizationEmissionModel::NonThermalPowerLaw};
	PolarizedPlasmaState<double> plasma{};
	double path_length_m{1.0e3};
	double doppler_factor{1.0};
	double field_position_angle_rad{0.0};
	double wavelength_min_nm{380.0};
	double wavelength_max_nm{780.0};
	size_t sample_count{400};
};

struct PolarizationSummary {
	bool valid{false};
	size_t index{0};
	double wavelength_nm{0.0};
	double intensity{0.0};
	double stokes_q{0.0};
	double stokes_u{0.0};
	double stokes_v{0.0};
	double dolp{0.0};
	double docp{0.0};
	double dop{0.0};
	double evpa_deg{0.0};
	double ellipse_semi_major{0.0};
	double ellipse_semi_minor{0.0};
	double ellipse_orientation_rad{0.0};
};

struct PolarizedSpectrum {
	std::vector<double> wavelength_nm{};
	std::vector<double> intensity{};
	std::vector<double> stokes_q{};
	std::vector<double> stokes_u{};
	std::vector<double> stokes_v{};
	std::vector<double> dolp{};
	std::vector<double> docp{};
	std::vector<double> dop{};
	std::vector<double> evpa_deg{};
	double peak_dolp{0.0};
	double peak_dolp_wavelength_nm{0.0};
	double mean_dolp{0.0};
	double mean_docp{0.0};
	double rotation_measure_rad_m2{0.0};
	double spectral_index{0.0};

	[[nodiscard]] bool empty() const noexcept {
		return wavelength_nm.empty();
	}

	[[nodiscard]] size_t nearest_index(double wavelength) const noexcept {
		if (wavelength_nm.empty()) return 0;
		const auto upper = std::lower_bound(wavelength_nm.begin(), wavelength_nm.end(), wavelength);
		if (upper == wavelength_nm.begin()) return 0;
		if (upper == wavelength_nm.end()) return wavelength_nm.size() - 1;
		const size_t high = static_cast<size_t>(upper - wavelength_nm.begin());
		const size_t low = high - 1;
		return (wavelength - wavelength_nm[low] <= wavelength_nm[high] - wavelength) ? low : high;
	}
};

class PolarizedSpectrumSynthesizer {
public:
	[[nodiscard]] static PolarizedSpectrum synthesize(const PolarizedSpectrumSettings& settings) {
		PolarizedSpectrum out;
		constexpr double c = Core::PhysicalConstants<double>::SPEED_OF_LIGHT;
		constexpr double pi = std::numbers::pi_v<double>;
		const size_t count = std::max<size_t>(settings.sample_count, 8);
		const double wl_min = std::max(settings.wavelength_min_nm, 1.0);
		const double wl_max = std::max(settings.wavelength_max_nm, wl_min * 1.01);
		const double log_ratio = std::log(wl_max / wl_min);
		const double g = std::max(settings.doppler_factor, 1e-6);

		out.wavelength_nm.reserve(count);
		out.intensity.reserve(count);
		out.stokes_q.reserve(count);
		out.stokes_u.reserve(count);
		out.stokes_v.reserve(count);
		out.dolp.reserve(count);
		out.docp.reserve(count);
		out.dop.reserve(count);
		out.evpa_deg.reserve(count);

		for (size_t i = 0; i < count; ++i) {
			const double fraction = static_cast<double>(i) / static_cast<double>(count - 1);
			const double wavelength_nm = wl_min * std::exp(log_ratio * fraction);
			const double wavelength_m = wavelength_nm * 1e-9;
			const double nu = c / wavelength_m;

			const auto transfer = [&](bool thermal) noexcept -> StokesVector<double> {
				return PolarizedRadiativeTransfer<double>::integrate_ray_segment(StokesVector<double>(0.0), nu, g, settings.plasma, settings.path_length_m, thermal);
			};

			StokesVector<double> stokes{};
			switch (settings.model) {
				case PolarizationEmissionModel::ThermalMaxwellJuttner:
					stokes = transfer(true);
					break;
				case PolarizationEmissionModel::Hybrid:
					stokes = transfer(false) + transfer(true);
					break;
				case PolarizationEmissionModel::NonThermalPowerLaw:
				default:
					stokes = transfer(false);
					break;
			}
			if (settings.field_position_angle_rad != 0.0) {
				stokes = stokes.rotate_reference_frame(settings.field_position_angle_rad);
			}

			const double to_wavelength = c / (wavelength_m * wavelength_m);
			double si = stokes.i * to_wavelength;
			double sq = stokes.q * to_wavelength;
			double su = stokes.u * to_wavelength;
			double sv = stokes.v * to_wavelength;
			if (!std::isfinite(si) || !std::isfinite(sq) || !std::isfinite(su) || !std::isfinite(sv)) {
				si = sq = su = sv = 0.0;
			}

			const double linear = std::sqrt(sq * sq + su * su);
			const double total = std::sqrt(sq * sq + su * su + sv * sv);
			const bool has_intensity = si > 1e-300;
			out.wavelength_nm.push_back(wavelength_nm);
			out.intensity.push_back(si);
			out.stokes_q.push_back(sq);
			out.stokes_u.push_back(su);
			out.stokes_v.push_back(sv);
			out.dolp.push_back(has_intensity ? std::clamp(linear / si, 0.0, 1.0) : 0.0);
			out.docp.push_back(has_intensity ? std::clamp(sv / si, -1.0, 1.0) : 0.0);
			out.dop.push_back(has_intensity ? std::clamp(total / si, 0.0, 1.0) : 0.0);
			out.evpa_deg.push_back(0.5 * std::atan2(su, sq) * 180.0 / pi);
		}

		double dolp_sum = 0.0;
		double docp_sum = 0.0;
		for (size_t i = 0; i < count; ++i) {
			dolp_sum += out.dolp[i];
			docp_sum += out.docp[i];
			if (out.dolp[i] > out.peak_dolp) {
				out.peak_dolp = out.dolp[i];
				out.peak_dolp_wavelength_nm = out.wavelength_nm[i];
			}
		}
		out.mean_dolp = dolp_sum / static_cast<double>(count);
		out.mean_docp = docp_sum / static_cast<double>(count);
		out.rotation_measure_rad_m2 = fit_rotation_measure(out);
		out.spectral_index = fit_spectral_index(out);
		return out;
	}

	[[nodiscard]] static PolarizationSummary summarize(const PolarizedSpectrum& spectrum, double wavelength_nm) {
		PolarizationSummary summary;
		if (spectrum.empty()) {
			return summary;
		}
		const size_t index = spectrum.nearest_index(wavelength_nm);
		const double i = spectrum.intensity[index];
		const double q = spectrum.stokes_q[index];
		const double u = spectrum.stokes_u[index];
		const double v = spectrum.stokes_v[index];
		const double linear = std::sqrt(q * q + u * u);
		const double total = std::sqrt(q * q + u * u + v * v);
		summary.valid = true;
		summary.index = index;
		summary.wavelength_nm = spectrum.wavelength_nm[index];
		summary.intensity = i;
		summary.stokes_q = q;
		summary.stokes_u = u;
		summary.stokes_v = v;
		summary.dolp = spectrum.dolp[index];
		summary.docp = spectrum.docp[index];
		summary.dop = spectrum.dop[index];
		summary.evpa_deg = spectrum.evpa_deg[index];
		summary.ellipse_orientation_rad = 0.5 * std::atan2(u, q);
		if (i > 1e-300) {
			summary.ellipse_semi_major = std::sqrt(std::max(0.5 * (total + linear), 0.0) / i);
			summary.ellipse_semi_minor = std::sqrt(std::max(0.5 * (total - linear), 0.0) / i);
		}
		return summary;
	}

private:
	[[nodiscard]] static double fit_rotation_measure(const PolarizedSpectrum& spectrum) noexcept {
		constexpr double pi = std::numbers::pi_v<double>;
		std::vector<double> x;
		std::vector<double> y;
		x.reserve(spectrum.wavelength_nm.size());
		y.reserve(spectrum.wavelength_nm.size());
		double previous = 0.0;
		bool has_previous = false;
		for (size_t i = 0; i < spectrum.wavelength_nm.size(); ++i) {
			const double linear = std::sqrt(spectrum.stokes_q[i] * spectrum.stokes_q[i] + spectrum.stokes_u[i] * spectrum.stokes_u[i]);
			if (spectrum.intensity[i] <= 0.0 || linear <= 1e-9 * spectrum.intensity[i]) continue;
			double chi = 0.5 * std::atan2(spectrum.stokes_u[i], spectrum.stokes_q[i]);
			if (has_previous) {
				while (chi - previous > 0.5 * pi) chi -= pi;
				while (chi - previous < -0.5 * pi) chi += pi;
			}
			previous = chi;
			has_previous = true;
			const double lambda_m = spectrum.wavelength_nm[i] * 1e-9;
			x.push_back(lambda_m * lambda_m);
			y.push_back(chi);
		}
		if (x.size() < 3) return 0.0;
		double mean_x = 0.0;
		double mean_y = 0.0;
		for (size_t i = 0; i < x.size(); ++i) {
			mean_x += x[i];
			mean_y += y[i];
		}
		mean_x /= static_cast<double>(x.size());
		mean_y /= static_cast<double>(y.size());
		double sxy = 0.0;
		double sxx = 0.0;
		for (size_t i = 0; i < x.size(); ++i) {
			sxy += (x[i] - mean_x) * (y[i] - mean_y);
			sxx += (x[i] - mean_x) * (x[i] - mean_x);
		}
		return (sxx > 0.0) ? (sxy / sxx) : 0.0;
	}

	[[nodiscard]] static double fit_spectral_index(const PolarizedSpectrum& spectrum) noexcept {
		if (spectrum.wavelength_nm.size() < 2) return 0.0;
		constexpr double c = Core::PhysicalConstants<double>::SPEED_OF_LIGHT;
		const size_t last = spectrum.wavelength_nm.size() - 1;
		const double i_first = spectrum.intensity.front();
		const double i_last = spectrum.intensity[last];
		if (i_first <= 0.0 || i_last <= 0.0) return 0.0;
		const double l_first = spectrum.wavelength_nm.front() * 1e-9;
		const double l_last = spectrum.wavelength_nm[last] * 1e-9;
		const double nu_first = i_first * l_first * l_first / c;
		const double nu_last = i_last * l_last * l_last / c;
		const double frequency_ratio = l_first / l_last;
		if (nu_first <= 0.0 || nu_last <= 0.0 || frequency_ratio <= 0.0 || std::abs(std::log(frequency_ratio)) < 1e-12) return 0.0;
		return std::log(nu_last / nu_first) / std::log(1.0 / frequency_ratio);
	}
};

}

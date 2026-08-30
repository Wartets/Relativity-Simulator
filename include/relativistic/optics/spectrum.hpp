#pragma once

#include "relativistic/core/constants.hpp"
#include <array>
#include <cmath>
#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>

namespace Relativistic::Optics {

enum class SpectralBand : uint32_t {
	GammaRay = 0,
	XRay = 1,
	Ultraviolet = 2,
	Visible = 3,
	NearInfrared = 4,
	MidInfrared = 5,
	FarInfrared = 6,
	Radio = 7
};

template <typename Scalar = double>
class ContinuousSpectrum {
public:
	static constexpr size_t NUM_SAMPLES = 64;

private:
	Scalar min_wavelength_m_{static_cast<Scalar>(1e-14)};
	Scalar max_wavelength_m_{static_cast<Scalar>(1e3)};
	std::array<Scalar, NUM_SAMPLES> log_wavelengths_{};
	std::array<Scalar, NUM_SAMPLES> radiance_values_{};

	void init_wavelength_grid() noexcept {
		const Scalar log_min = std::log(min_wavelength_m_);
		const Scalar log_max = std::log(max_wavelength_m_);
		const Scalar step = (log_max - log_min) / static_cast<Scalar>(NUM_SAMPLES - 1);
		for (size_t i = 0; i < NUM_SAMPLES; ++i) {
			log_wavelengths_[i] = log_min + static_cast<Scalar>(i) * step;
		}
	}

public:
	constexpr ContinuousSpectrum() noexcept {
		init_wavelength_grid();
		radiance_values_.fill(static_cast<Scalar>(0));
	}

	explicit ContinuousSpectrum(Scalar uniform_radiance) noexcept {
		init_wavelength_grid();
		radiance_values_.fill(uniform_radiance);
	}

	[[nodiscard]] static ContinuousSpectrum make_blackbody(Scalar temperature_kelvin) noexcept {
		ContinuousSpectrum spec;
		constexpr Scalar h = static_cast<Scalar>(Core::PhysicalConstants<Scalar>::PLANCK_CONSTANT);
		constexpr Scalar c = static_cast<Scalar>(Core::PhysicalConstants<Scalar>::SPEED_OF_LIGHT);
		constexpr Scalar k_b = static_cast<Scalar>(Core::PhysicalConstants<Scalar>::BOLTZMANN_CONSTANT);

		const Scalar c1 = static_cast<Scalar>(2) * h * c * c;
		const Scalar c2 = h * c / (k_b * temperature_kelvin);

		for (size_t i = 0; i < NUM_SAMPLES; ++i) {
			const Scalar lambda = std::exp(spec.log_wavelengths_[i]);
			const Scalar lambda5 = lambda * lambda * lambda * lambda * lambda;
			const Scalar x = c2 / lambda;
			Scalar planck_val = static_cast<Scalar>(0);
			if (x < static_cast<Scalar>(700.0)) {
				planck_val = c1 / (lambda5 * (std::exp(x) - static_cast<Scalar>(1)));
			}
			spec.radiance_values_[i] = planck_val;
		}
		return spec;
	}

	[[nodiscard]] static ContinuousSpectrum make_synchrotron(Scalar spectral_index, Scalar reference_flux = static_cast<Scalar>(1.0)) noexcept {
		ContinuousSpectrum spec;
		constexpr Scalar lambda_0 = static_cast<Scalar>(1e-2);
		for (size_t i = 0; i < NUM_SAMPLES; ++i) {
			const Scalar lambda = std::exp(spec.log_wavelengths_[i]);
			const Scalar ratio = lambda / lambda_0;
			spec.radiance_values_[i] = reference_flux * std::pow(ratio, spectral_index - static_cast<Scalar>(1));
		}
		return spec;
	}

	[[nodiscard]] static ContinuousSpectrum make_monochromatic(Scalar wavelength_m, Scalar intensity = static_cast<Scalar>(1.0)) noexcept {
		ContinuousSpectrum spec;
		const Scalar log_target = std::log(wavelength_m);
		size_t closest_idx = 0;
		Scalar min_dist = std::abs(spec.log_wavelengths_[0] - log_target);
		for (size_t i = 1; i < NUM_SAMPLES; ++i) {
			const Scalar dist = std::abs(spec.log_wavelengths_[i] - log_target);
			if (dist < min_dist) {
				min_dist = dist;
				closest_idx = i;
			}
		}
		spec.radiance_values_[closest_idx] = intensity;
		return spec;
	}

	[[nodiscard]] Scalar sample_radiance(Scalar wavelength_m) const noexcept {
		if (wavelength_m <= min_wavelength_m_) return radiance_values_.front();
		if (wavelength_m >= max_wavelength_m_) return radiance_values_.back();

		const Scalar log_lambda = std::log(wavelength_m);
		const Scalar log_min = log_wavelengths_.front();
		const Scalar log_max = log_wavelengths_.back();
		const Scalar norm_idx = (log_lambda - log_min) / (log_max - log_min) * static_cast<Scalar>(NUM_SAMPLES - 1);
		const size_t idx0 = static_cast<size_t>(std::floor(norm_idx));
		const size_t idx1 = std::min(idx0 + 1, NUM_SAMPLES - 1);
		const Scalar frac = norm_idx - static_cast<Scalar>(idx0);

		return (static_cast<Scalar>(1) - frac) * radiance_values_[idx0] + frac * radiance_values_[idx1];
	}

	[[nodiscard]] ContinuousSpectrum transform_doppler(Scalar g) const noexcept {
		ContinuousSpectrum result;
		const Scalar g3 = g * g * g;
		for (size_t i = 0; i < NUM_SAMPLES; ++i) {
			const Scalar lambda_obs = std::exp(result.log_wavelengths_[i]);
			const Scalar lambda_emit = lambda_obs * g;
			const Scalar i_emit = sample_radiance(lambda_emit);
			result.radiance_values_[i] = g3 * i_emit;
		}
		return result;
	}

	[[nodiscard]] Scalar integrate_band(Scalar lambda_start_m, Scalar lambda_end_m, size_t steps = 100) const noexcept {
		const Scalar lambda_a = std::max(lambda_start_m, min_wavelength_m_);
		const Scalar lambda_b = std::min(lambda_end_m, max_wavelength_m_);
		if (lambda_a >= lambda_b) {
			return static_cast<Scalar>(0);
		}

		const Scalar d_lambda = (lambda_b - lambda_a) / static_cast<Scalar>(steps);
		Scalar sum = static_cast<Scalar>(0.5) * (sample_radiance(lambda_a) + sample_radiance(lambda_b));
		for (size_t i = 1; i < steps; ++i) {
			const Scalar l = lambda_a + static_cast<Scalar>(i) * d_lambda;
			sum += sample_radiance(l);
		}
		return sum * d_lambda;
	}

	[[nodiscard]] Scalar bolometric_radiance(size_t steps = 500) const noexcept {
		return integrate_band(min_wavelength_m_, max_wavelength_m_, steps);
	}
};

}

#pragma once

#include "relativistic/optics/spectrum.hpp"
#include <array>
#include <cmath>
#include <algorithm>
#include <concepts>

namespace Relativistic::Optics {

struct alignas(16) ColorXYZ {
	double x{0.0};
	double y{0.0};
	double z{0.0};
};

struct alignas(16) ColorRGB {
	double r{0.0};
	double g{0.0};
	double b{0.0};
};

class CIE1931Observer {
private:
	static constexpr double gaussian(double x, double alpha, double mu, double sigma1, double sigma2) noexcept {
		const double sigma = (x < mu) ? sigma1 : sigma2;
		const double t = (x - mu) / sigma;
		return alpha * std::exp(-0.5 * t * t);
	}

public:
	[[nodiscard]] static constexpr double x_bar(double lambda_nm) noexcept {
		return gaussian(lambda_nm, 1.056, 599.8, 37.9, 31.0)
		     + gaussian(lambda_nm, 0.362, 442.0, 16.0, 26.7)
		     - gaussian(lambda_nm, 0.065, 501.1, 20.4, 26.2);
	}

	[[nodiscard]] static constexpr double y_bar(double lambda_nm) noexcept {
		return gaussian(lambda_nm, 0.821, 568.8, 46.9, 40.5)
		     + gaussian(lambda_nm, 0.286, 530.9, 16.3, 31.1);
	}

	[[nodiscard]] static constexpr double z_bar(double lambda_nm) noexcept {
		return gaussian(lambda_nm, 1.217, 437.0, 11.8, 36.0)
		     + gaussian(lambda_nm, 0.681, 459.0, 26.0, 13.8);
	}

	template <typename Scalar = double>
	[[nodiscard]] static ColorXYZ integrate_spectrum(
		const ContinuousSpectrum<Scalar>& spectrum,
		double lambda_start_nm = 380.0,
		double lambda_end_nm = 780.0,
		size_t samples = 400
	) noexcept {
		const double d_lambda = (lambda_end_nm - lambda_start_nm) / static_cast<double>(samples);
		double int_x = 0.0;
		double int_y = 0.0;
		double int_z = 0.0;

		for (size_t i = 0; i <= samples; ++i) {
			const double lambda_nm = lambda_start_nm + static_cast<double>(i) * d_lambda;
			const double lambda_m = lambda_nm * 1e-9;
			const double radiance = static_cast<double>(spectrum.sample_radiance(static_cast<Scalar>(lambda_m)));
			const double weight = (i == 0 || i == samples) ? 0.5 : 1.0;

			int_x += weight * radiance * x_bar(lambda_nm);
			int_y += weight * radiance * y_bar(lambda_nm);
			int_z += weight * radiance * z_bar(lambda_nm);
		}

		return ColorXYZ{
			.x = int_x * d_lambda,
			.y = int_y * d_lambda,
			.z = int_z * d_lambda
		};
	}

	[[nodiscard]] static constexpr ColorRGB xyz_to_linear_srgb(const ColorXYZ& xyz) noexcept {
		const double r =  3.2404542 * xyz.x - 1.5371385 * xyz.y - 0.4985314 * xyz.z;
		const double g = -0.9692660 * xyz.x + 1.8760108 * xyz.y + 0.0415560 * xyz.z;
		const double b =  0.0556434 * xyz.x - 0.2040259 * xyz.y + 1.0572252 * xyz.z;
		return ColorRGB{
			.r = std::max(r, 0.0),
			.g = std::max(g, 0.0),
			.b = std::max(b, 0.0)
		};
	}

	[[nodiscard]] static constexpr double linear_to_srgb_component(double val) noexcept {
		if (val <= 0.0031308) {
			return 12.92 * val;
		}
		return 1.055 * std::pow(val, 1.0 / 2.4) - 0.055;
	}

	[[nodiscard]] static constexpr double srgb_to_linear_component(double val) noexcept {
		if (val <= 0.04045) {
			return val / 12.92;
		}
		return std::pow((val + 0.055) / 1.055, 2.4);
	}

	[[nodiscard]] static constexpr ColorRGB linear_to_srgb(const ColorRGB& linear) noexcept {
		return ColorRGB{
			.r = std::clamp(linear_to_srgb_component(linear.r), 0.0, 1.0),
			.g = std::clamp(linear_to_srgb_component(linear.g), 0.0, 1.0),
			.b = std::clamp(linear_to_srgb_component(linear.b), 0.0, 1.0)
		};
	}

	template <typename Scalar = double>
	[[nodiscard]] static ColorRGB spectrum_to_srgb(const ContinuousSpectrum<Scalar>& spectrum) noexcept {
		const auto xyz = integrate_spectrum(spectrum);
		const auto linear = xyz_to_linear_srgb(xyz);
		return linear_to_srgb(linear);
	}
};

}

#pragma once

#include "relativistic/optics/cie_observer.hpp"
#include <cmath>
#include <algorithm>

namespace Relativistic::Optics {

class Tonemapper {
public:
	[[nodiscard]] static constexpr double aces_film_curve(double x) noexcept {
		const double a = 2.51;
		const double b = 0.03;
		const double c = 2.43;
		const double d = 0.59;
		const double e = 0.14;
		return std::clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
	}

	[[nodiscard]] static constexpr ColorRGB aces_tonemap(const ColorRGB& linear_color) noexcept {
		return ColorRGB{
			.r = aces_film_curve(linear_color.r),
			.g = aces_film_curve(linear_color.g),
			.b = aces_film_curve(linear_color.b)
		};
	}

	[[nodiscard]] static ColorRGB logarithmic_hdr_tonemap(
		const ColorRGB& linear_radiance,
		double min_radiance = 1e-12,
		double max_radiance = 1e20
	) noexcept {
		const double lum = 0.2126 * linear_radiance.r + 0.7152 * linear_radiance.g + 0.0722 * linear_radiance.b;
		if (lum <= 0.0) {
			return ColorRGB{0.0, 0.0, 0.0};
		}

		const double log_min = std::log10(1.0);
		const double log_max = std::log10(1.0 + max_radiance / min_radiance);
		const double log_val = std::log10(1.0 + lum / min_radiance);
		const double mapped_lum = std::clamp(log_val / (log_max - log_min), 0.0, 1.0);
		const double scale = mapped_lum / lum;

		return ColorRGB{
			.r = std::clamp(linear_radiance.r * scale, 0.0, 1.0),
			.g = std::clamp(linear_radiance.g * scale, 0.0, 1.0),
			.b = std::clamp(linear_radiance.b * scale, 0.0, 1.0)
		};
	}

	[[nodiscard]] static constexpr ColorRGB reinhard_extended(const ColorRGB& linear_color, double max_white = 10.0) noexcept {
		const double max_white2 = max_white * max_white;
		auto map_c = [max_white2](double c) noexcept -> double {
			return (c * (1.0 + c / max_white2)) / (1.0 + c);
		};
		return ColorRGB{
			.r = std::clamp(map_c(linear_color.r), 0.0, 1.0),
			.g = std::clamp(map_c(linear_color.g), 0.0, 1.0),
			.b = std::clamp(map_c(linear_color.b), 0.0, 1.0)
		};
	}

	[[nodiscard]] static constexpr ColorRGB apply_exposure(const ColorRGB& color, double exposure_ev) noexcept {
		const double factor = std::exp2(exposure_ev);
		return ColorRGB{
			.r = color.r * factor,
			.g = color.g * factor,
			.b = color.b * factor
		};
	}
};

}

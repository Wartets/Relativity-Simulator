#pragma once

#include <cmath>
#include <algorithm>

namespace Relativistic::Optics {

class DiskThermalProfile {
public:
	[[nodiscard]] static double kerr_isco_radius(double mass, double spin) noexcept {
		if (mass <= 1e-12) return 0.0;
		const double a_star = std::clamp(spin / mass, -0.9999999, 0.9999999);
		const double abs_a = std::abs(a_star);
		const double orbit_sign = (a_star >= 0.0) ? -1.0 : 1.0;
		const double cbrt_term = std::cbrt(std::max(1.0 - abs_a * abs_a, 0.0));
		const double z1 = 1.0 + cbrt_term * (std::cbrt(1.0 + abs_a) + std::cbrt(1.0 - abs_a));
		const double z2 = std::sqrt(3.0 * abs_a * abs_a + z1 * z1);
		const double r_isco_over_m = 3.0 + z2 + orbit_sign * std::sqrt(std::max((3.0 - z1) * (3.0 + z1 + 2.0 * z2), 0.0));
		return r_isco_over_m * mass;
	}

	[[nodiscard]] static constexpr double disk_outer_radius(double mass) noexcept {
		return 24.0 * mass;
	}

	[[nodiscard]] static double normalized_temperature(double isco_radius, double r) noexcept {
		if (r <= isco_radius) return 0.0;
		const double ratio = isco_radius / r;
		return std::pow(ratio, 0.75) * std::pow(std::max(1.0 - std::sqrt(ratio), 0.0), 0.25);
	}

	[[nodiscard]] static double effective_temperature_kelvin(double isco_radius, double r) noexcept {
		return 18000.0 * normalized_temperature(isco_radius, r) + 1200.0;
	}

	[[nodiscard]] static double circular_orbit_redshift_factor(double mass, double r) noexcept {
		const double rs = 2.0 * mass;
		if (r <= 1.5 * rs) return 0.0;
		return std::sqrt(std::max(1.0 - 3.0 * mass / r, 1e-6));
	}

	[[nodiscard]] static bool radius_within_disk(double mass, double spin, double r) noexcept {
		const double isco = kerr_isco_radius(mass, spin);
		return (r >= isco) && (r <= disk_outer_radius(mass));
	}
};

}

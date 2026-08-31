#pragma once

#include "relativistic/gravimetry/spherical_harmonics.hpp"
#include <cmath>
#include <numbers>
#include <array>

namespace Relativistic::Gravimetry {

struct OsculatingElements {
	double semi_major_axis{0.0};
	double eccentricity{0.0};
	double inclination{0.0};
	double raan{0.0};
	double argument_of_periapsis{0.0};
	double true_anomaly{0.0};
	double mean_motion{0.0};
	double orbital_period{0.0};
};

class OrbitalPrecessionAnalytic {
public:
	[[nodiscard]] static double nodal_precession_rate_j2_rad_s(
		double gm,
		double r_ref,
		double j2,
		double semi_major_axis,
		double eccentricity,
		double inclination_rad
	) noexcept {
		const double a = semi_major_axis;
		const double e = eccentricity;
		const double i = inclination_rad;
		const double n = std::sqrt(gm / (a * a * a));
		const double p = a * (1.0 - e * e);
		const double r_over_p = r_ref / p;
		return -1.5 * n * j2 * r_over_p * r_over_p * std::cos(i);
	}

	[[nodiscard]] static double apsidal_precession_rate_j2_rad_s(
		double gm,
		double r_ref,
		double j2,
		double semi_major_axis,
		double eccentricity,
		double inclination_rad
	) noexcept {
		const double a = semi_major_axis;
		const double e = eccentricity;
		const double i = inclination_rad;
		const double n = std::sqrt(gm / (a * a * a));
		const double p = a * (1.0 - e * e);
		const double r_over_p = r_ref / p;
		const double cos_i = std::cos(i);
		return 0.75 * n * j2 * r_over_p * r_over_p * (5.0 * cos_i * cos_i - 1.0);
	}

	[[nodiscard]] static double nodal_precession_rate_j2_j4_rad_s(
		double gm,
		double r_ref,
		double j2,
		double j4,
		double semi_major_axis,
		double eccentricity,
		double inclination_rad
	) noexcept {
		const double a = semi_major_axis;
		const double e = eccentricity;
		const double i = inclination_rad;
		const double n = std::sqrt(gm / (a * a * a));
		const double p = a * (1.0 - e * e);
		const double r_p = r_ref / p;
		const double r_p2 = r_p * r_p;
		const double r_p4 = r_p2 * r_p2;
		const double cos_i = std::cos(i);
		const double cos2_i = cos_i * cos_i;
		const double sin2_i = 1.0 - cos2_i;

		const double dot_omega_j2 = -1.5 * n * j2 * r_p2 * cos_i;
		const double dot_omega_j2_sq = (9.0 / 8.0) * n * j2 * j2 * r_p4 * cos_i * (1.5 - 2.5 * sin2_i + e * e * (0.5 - (5.0 / 6.0) * sin2_i));
		const double dot_omega_j4 = (15.0 / 16.0) * n * j4 * r_p4 * cos_i * (1.0 + 1.5 * e * e) * (3.0 - 7.0 * cos2_i);

		return dot_omega_j2 + dot_omega_j2_sq + dot_omega_j4;
	}

	[[nodiscard]] static double nodal_precession_rate_rad_day(
		double gm,
		double r_ref,
		double j2,
		double semi_major_axis,
		double eccentricity,
		double inclination_rad
	) noexcept {
		return nodal_precession_rate_j2_rad_s(gm, r_ref, j2, semi_major_axis, eccentricity, inclination_rad) * 86400.0;
	}

	[[nodiscard]] static double sun_synchronous_inclination_rad(
		double gm,
		double r_ref,
		double j2,
		double semi_major_axis,
		double eccentricity
	) noexcept {
		const double target_rate_rad_s = 2.0 * std::numbers::pi_v<double> / (365.2422 * 86400.0);
		const double a = semi_major_axis;
		const double e = eccentricity;
		const double n = std::sqrt(gm / (a * a * a));
		const double p = a * (1.0 - e * e);
		const double r_over_p = r_ref / p;
		const double cos_i = -target_rate_rad_s / (1.5 * n * j2 * r_over_p * r_over_p);
		return std::acos(std::clamp(cos_i, -1.0, 1.0));
	}

	[[nodiscard]] static OsculatingElements cartesian_to_osculating(
		const std::array<double, 3>& r_vec,
		const std::array<double, 3>& v_vec,
		double gm
	) noexcept {
		OsculatingElements elem{};

		const double rx = r_vec[0], ry = r_vec[1], rz = r_vec[2];
		const double vx = v_vec[0], vy = v_vec[1], vz = v_vec[2];

		const double r = std::sqrt(rx * rx + ry * ry + rz * rz);
		const double v2 = vx * vx + vy * vy + vz * vz;

		if (r <= 0.0 || gm <= 0.0) return elem;

		const double hx = ry * vz - rz * vy;
		const double hy = rz * vx - rx * vz;
		const double hz = rx * vy - ry * vx;
		const double h = std::sqrt(hx * hx + hy * hy + hz * hz);

		const double nx = -hy;
		const double ny = hx;
		const double n = std::sqrt(nx * nx + ny * ny);

		const double r_dot_v = rx * vx + ry * vy + rz * vz;
		const double factor1 = (v2 - gm / r);

		const double ex = (factor1 * rx - r_dot_v * vx) / gm;
		const double ey = (factor1 * ry - r_dot_v * vy) / gm;
		const double ez = (factor1 * rz - r_dot_v * vz) / gm;
		const double e = std::sqrt(ex * ex + ey * ey + ez * ez);

		const double energy = 0.5 * v2 - gm / r;
		const double a = (std::abs(energy) > 1e-15) ? (-gm / (2.0 * energy)) : 0.0;

		const double inc = (h > 0.0) ? std::acos(std::clamp(hz / h, -1.0, 1.0)) : 0.0;

		double raan = 0.0;
		if (n > 1e-14) {
			raan = std::atan2(ny, nx);
			if (raan < 0.0) raan += 2.0 * std::numbers::pi_v<double>;
		}

		double arg_p = 0.0;
		if (n > 1e-14 && e > 1e-14) {
			const double n_dot_e = nx * ex + ny * ey;
			const double sin_w = ez * (h / (n * e));
			const double cos_w = n_dot_e / (n * e);
			arg_p = std::atan2(sin_w, cos_w);
			if (arg_p < 0.0) arg_p += 2.0 * std::numbers::pi_v<double>;
		}

		double nu = 0.0;
		if (e > 1e-14) {
			const double e_dot_r = ex * rx + ey * ry + ez * rz;
			const double sin_nu = r_dot_v * (h / (e * gm));
			const double cos_nu = e_dot_r / (e * r);
			nu = std::atan2(sin_nu, cos_nu);
			if (nu < 0.0) nu += 2.0 * std::numbers::pi_v<double>;
		}

		elem.semi_major_axis = a;
		elem.eccentricity = e;
		elem.inclination = inc;
		elem.raan = raan;
		elem.argument_of_periapsis = arg_p;
		elem.true_anomaly = nu;

		if (a > 0.0) {
			elem.mean_motion = std::sqrt(gm / (a * a * a));
			elem.orbital_period = 2.0 * std::numbers::pi_v<double> / elem.mean_motion;
		}

		return elem;
	}
};

}

#pragma once

#include "relativistic/core/tensor.hpp"
#include <cstdint>
#include <cmath>
#include <numbers>
#include <string_view>
#include <array>
#include <algorithm>

namespace Relativistic::IO {

struct Epoch {
	static constexpr double J2000_JD = 2451545.0;
	static constexpr double SECONDS_PER_DAY = 86400.0;

	double jd{J2000_JD};

	constexpr Epoch() noexcept = default;

	explicit constexpr Epoch(double julian_date) noexcept : jd(julian_date) {}

	[[nodiscard]] static constexpr Epoch from_j2000_seconds(double seconds) noexcept {
		return Epoch(J2000_JD + seconds / SECONDS_PER_DAY);
	}

	[[nodiscard]] static constexpr Epoch from_calendar(int year, int month, double day) noexcept {
		int y = year;
		int m = month;
		if (m <= 2) {
			y -= 1;
			m += 12;
		}
		const int a = y / 100;
		const int b = 2 - a + (a / 4);
		const double jd_val = static_cast<double>(static_cast<int64_t>(365.25 * static_cast<double>(y + 4716)))
			+ static_cast<double>(static_cast<int64_t>(30.6001 * static_cast<double>(m + 1)))
			+ day + static_cast<double>(b) - 1524.5;
		return Epoch(jd_val);
	}

	[[nodiscard]] constexpr double to_j2000_seconds() const noexcept {
		return (jd - J2000_JD) * SECONDS_PER_DAY;
	}

	[[nodiscard]] constexpr double to_j2000_centuries() const noexcept {
		return (jd - J2000_JD) / 36525.0;
	}
};

struct EphemerisStateVector {
	Epoch epoch{};
	Core::FourVector<double> position{};
	Core::FourVector<double> velocity{};
	uint32_t body_id{0};
	uint32_t center_id{0};

	constexpr EphemerisStateVector() noexcept = default;

	constexpr EphemerisStateVector(
		Epoch ep,
		double x, double y, double z,
		double vx, double vy, double vz,
		uint32_t target = 0,
		uint32_t center = 0,
		double c_light = 299792458.0
	) noexcept
		: epoch(ep),
		  position(ep.to_j2000_seconds() * c_light, x, y, z),
		  velocity(c_light, vx, vy, vz),
		  body_id(target),
		  center_id(center) {}
};

struct KeplerianElements {
	double semi_major_axis{0.0};
	double eccentricity{0.0};
	double inclination{0.0};
	double longitude_ascending_node{0.0};
	double argument_of_periapsis{0.0};
	double mean_anomaly{0.0};
	double gravitational_parameter{1.32712440018e20};

	[[nodiscard]] double eccentric_anomaly(double tol = 1e-14, size_t max_iter = 50) const noexcept {
		const double pi = std::numbers::pi_v<double>;
		const double two_pi = 2.0 * pi;
		double m = std::fmod(mean_anomaly, two_pi);
		if (m < 0.0) {
			m += two_pi;
		}

		double e_anom = (eccentricity < 0.8) ? m : pi;
		for (size_t i = 0; i < max_iter; ++i) {
			const double sin_e = std::sin(e_anom);
			const double cos_e = std::cos(e_anom);
			const double f = e_anom - eccentricity * sin_e - m;
			const double f_prime = 1.0 - eccentricity * cos_e;
			const double f_prime2 = eccentricity * sin_e;

			const double delta = f / f_prime;
			const double halley_step = delta / (1.0 - 0.5 * delta * (f_prime2 / f_prime));
			e_anom -= halley_step;

			if (std::abs(halley_step) < tol) {
				break;
			}
		}
		return e_anom;
	}

	[[nodiscard]] double true_anomaly() const noexcept {
		const double e_anom = eccentric_anomaly();
		const double sin_e = std::sin(e_anom);
		const double cos_e = std::cos(e_anom);
		const double y = std::sqrt(std::max(1.0 - eccentricity * eccentricity, 0.0)) * sin_e;
		const double x = cos_e - eccentricity;
		double nu = std::atan2(y, x);
		if (nu < 0.0) {
			nu += 2.0 * std::numbers::pi_v<double>;
		}
		return nu;
	}

	[[nodiscard]] EphemerisStateVector to_state_vector(Epoch epoch, uint32_t target_id = 0, uint32_t center_id = 0) const noexcept {
		const double e_anom = eccentric_anomaly();
		const double cos_e = std::cos(e_anom);
		const double sin_e = std::sin(e_anom);

		const double a = semi_major_axis;
		const double e = eccentricity;
		const double mu = gravitational_parameter;

		const double r = a * (1.0 - e * cos_e);
		const double factor_v = std::sqrt(mu * a) / r;

		const double x_orb = a * (cos_e - e);
		const double y_orb = a * std::sqrt(std::max(1.0 - e * e, 0.0)) * sin_e;

		const double vx_orb = -factor_v * sin_e;
		const double vy_orb = factor_v * std::sqrt(std::max(1.0 - e * e, 0.0)) * cos_e;

		const double cos_o = std::cos(longitude_ascending_node);
		const double sin_o = std::sin(longitude_ascending_node);
		const double cos_w = std::cos(argument_of_periapsis);
		const double sin_w = std::sin(argument_of_periapsis);
		const double cos_i = std::cos(inclination);
		const double sin_i = std::sin(inclination);

		const double p_x = cos_o * cos_w - sin_o * sin_w * cos_i;
		const double p_y = sin_o * cos_w + cos_o * sin_w * cos_i;
		const double p_z = sin_w * sin_i;

		const double q_x = -cos_o * sin_w - sin_o * cos_w * cos_i;
		const double q_y = -sin_o * sin_w + cos_o * cos_w * cos_i;
		const double q_z = cos_w * sin_i;

		const double x = p_x * x_orb + q_x * y_orb;
		const double y = p_y * x_orb + q_y * y_orb;
		const double z = p_z * x_orb + q_z * y_orb;

		const double vx = p_x * vx_orb + q_x * vy_orb;
		const double vy = p_y * vx_orb + q_y * vy_orb;
		const double vz = p_z * vx_orb + q_z * vy_orb;

		return EphemerisStateVector(epoch, x, y, z, vx, vy, vz, target_id, center_id);
	}
};

namespace NaifBodyId {
	static constexpr uint32_t SOLAR_SYSTEM_BARYCENTER = 0;
	static constexpr uint32_t SUN = 10;
	static constexpr uint32_t MERCURY_BARYCENTER = 1;
	static constexpr uint32_t MERCURY = 199;
	static constexpr uint32_t VENUS_BARYCENTER = 2;
	static constexpr uint32_t VENUS = 299;
	static constexpr uint32_t EARTH_MOON_BARYCENTER = 3;
	static constexpr uint32_t EARTH = 399;
	static constexpr uint32_t MOON = 301;
	static constexpr uint32_t MARS_BARYCENTER = 4;
	static constexpr uint32_t MARS = 499;
	static constexpr uint32_t JUPITER_BARYCENTER = 5;
	static constexpr uint32_t SATURN_BARYCENTER = 6;
	static constexpr uint32_t URANUS_BARYCENTER = 7;
	static constexpr uint32_t NEPTUNE_BARYCENTER = 8;
	static constexpr uint32_t PLUTO_BARYCENTER = 9;
}

}

#pragma once

#include "relativistic/dynamics/pn_body.hpp"
#include "relativistic/dynamics/pn_orders.hpp"
#include "relativistic/dynamics/pn_nbody_system.hpp"
#include "relativistic/core/constants.hpp"
#include <cmath>
#include <numbers>
#include <algorithm>

namespace Relativistic::Dynamics {

class HulseTaylorPulsar {
public:
	static constexpr double M_PULSAR_SOLAR = 1.4398;
	static constexpr double M_COMPANION_SOLAR = 1.3886;
	static constexpr double ECCENTRICITY = 0.6171334;
	static constexpr double ORBITAL_PERIOD_SECONDS = 27906.980894;
	static constexpr double OBSERVATIONAL_P_DOT = -2.423e-12;

	[[nodiscard]] static double pulsar_mass_kg() noexcept {
		return M_PULSAR_SOLAR * Core::PhysicalConstants<double>::SOLAR_MASS;
	}

	[[nodiscard]] static double companion_mass_kg() noexcept {
		return M_COMPANION_SOLAR * Core::PhysicalConstants<double>::SOLAR_MASS;
	}

	[[nodiscard]] static double total_mass_kg() noexcept {
		return pulsar_mass_kg() + companion_mass_kg();
	}

	[[nodiscard]] static double reduced_mass_kg() noexcept {
		return (pulsar_mass_kg() * companion_mass_kg()) / total_mass_kg();
	}

	[[nodiscard]] static double semi_major_axis_meters(
		double g = Core::PhysicalConstants<double>::GRAVITATIONAL_CONSTANT
	) noexcept {
		const double gm = g * total_mass_kg();
		const double pb = ORBITAL_PERIOD_SECONDS;
		const double two_pi = 2.0 * std::numbers::pi_v<double>;
		const double mean_motion = two_pi / pb;
		return std::cbrt(gm / (mean_motion * mean_motion));
	}

	[[nodiscard]] static double peters_mathews_p_dot(
		double c = Core::PhysicalConstants<double>::SPEED_OF_LIGHT,
		double g = Core::PhysicalConstants<double>::GRAVITATIONAL_CONSTANT
	) noexcept {
		const double m1 = pulsar_mass_kg();
		const double m2 = companion_mass_kg();
		const double m_tot = m1 + m2;
		const double pb = ORBITAL_PERIOD_SECONDS;
		const double e = ECCENTRICITY;

		const double e2 = e * e;
		const double e4 = e2 * e2;
		const double fe = (1.0 + (73.0 / 24.0) * e2 + (37.0 / 96.0) * e4) / std::pow(1.0 - e2, 3.5);

		const double c5 = c * c * c * c * c;
		const double two_pi = 2.0 * std::numbers::pi_v<double>;

		const double term1 = (-192.0 * std::numbers::pi_v<double> / 5.0);
		const double term2 = std::pow(g, 5.0 / 3.0) / c5;
		const double term3 = std::pow(pb / two_pi, -5.0 / 3.0);
		const double term4 = (m1 * m2) / std::cbrt(m_tot);

		return term1 * term2 * term3 * term4 * fe;
	}

	[[nodiscard]] static double periastron_advance_rate_rad_s(
		double c = Core::PhysicalConstants<double>::SPEED_OF_LIGHT,
		double g = Core::PhysicalConstants<double>::GRAVITATIONAL_CONSTANT
	) noexcept {
		const double m_tot = total_mass_kg();
		const double a = semi_major_axis_meters(g);
		const double e = ECCENTRICITY;
		const double pb = ORBITAL_PERIOD_SECONDS;
		const double c2 = c * c;
		const double delta_phi = (6.0 * std::numbers::pi_v<double> * g * m_tot) / (c2 * a * (1.0 - e * e));
		return delta_phi / pb;
	}

	[[nodiscard]] static PostNewtonianSystem create_initial_system(
		const PNOrderConfig& config
	) noexcept {
		PostNewtonianSystem sys(config);

		const double m1 = pulsar_mass_kg();
		const double m2 = companion_mass_kg();
		const double m_tot = m1 + m2;
		const double a = semi_major_axis_meters(config.gravitational_constant);
		const double e = ECCENTRICITY;
		const double g = config.gravitational_constant;

		const double r_peri = a * (1.0 - e);
		const double v_peri = std::sqrt((g * m_tot * (1.0 + e)) / (a * (1.0 - e)));

		const double r1_x = (m2 / m_tot) * r_peri;
		const double r2_x = -(m1 / m_tot) * r_peri;

		const double v1_y = (m2 / m_tot) * v_peri;
		const double v2_y = -(m1 / m_tot) * v_peri;

		const PostNewtonianBody b1(1, m1, 10000.0, {r1_x, 0.0, 0.0}, {0.0, v1_y, 0.0});
		const PostNewtonianBody b2(2, m2, 10000.0, {r2_x, 0.0, 0.0}, {0.0, v2_y, 0.0});

		sys.add_body(b1);
		sys.add_body(b2);
		sys.update_accelerations();

		return sys;
	}
};

}

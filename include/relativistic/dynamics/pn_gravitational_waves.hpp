#pragma once

#include "relativistic/dynamics/pn_body.hpp"
#include "relativistic/dynamics/pn_orders.hpp"
#include <array>
#include <vector>
#include <span>
#include <cmath>
#include <numbers>
#include <algorithm>

namespace Relativistic::Dynamics {

struct GravitationalWavePolarizations {
	double h_plus{0.0};
	double h_cross{0.0};
};

struct GravitationalWaveEmission {
	std::array<std::array<double, 3>, 3> quadrupole_moment{};
	std::array<std::array<double, 3>, 3> quadrupole_derivative1{};
	std::array<std::array<double, 3>, 3> quadrupole_derivative2{};
	std::array<std::array<double, 3>, 3> quadrupole_derivative3{};
	double radiated_power{0.0};
	std::array<double, 3> angular_momentum_loss{0.0, 0.0, 0.0};
};

class GravitationalWaveCalculator {
public:
	[[nodiscard]] static GravitationalWaveEmission compute_nbody_quadrupole(
		std::span<const PostNewtonianBody> bodies,
		double speed_of_light = Core::PhysicalConstants<double>::SPEED_OF_LIGHT,
		double gravitational_constant = Core::PhysicalConstants<double>::GRAVITATIONAL_CONSTANT
	) noexcept {
		GravitationalWaveEmission em{};
		const size_t n = bodies.size();

		for (size_t a = 0; a < n; ++a) {
			const auto& ba = bodies[a];
			const double m = ba.mass;
			const double r2 = ba.position[0] * ba.position[0] + ba.position[1] * ba.position[1] + ba.position[2] * ba.position[2];
			const double r_dot_v = ba.position[0] * ba.velocity[0] + ba.position[1] * ba.velocity[1] + ba.position[2] * ba.velocity[2];
			const double v2 = ba.speed_squared();
			const double r_dot_a = ba.position[0] * ba.acceleration[0] + ba.position[1] * ba.acceleration[1] + ba.position[2] * ba.acceleration[2];

			for (size_t i = 0; i < 3; ++i) {
				for (size_t j = 0; j < 3; ++j) {
					const double delta_ij = (i == j) ? 1.0 : 0.0;

					em.quadrupole_moment[i][j] += m * (ba.position[i] * ba.position[j] - (1.0 / 3.0) * delta_ij * r2);

					em.quadrupole_derivative1[i][j] += m * (ba.velocity[i] * ba.position[j] + ba.position[i] * ba.velocity[j] - (2.0 / 3.0) * delta_ij * r_dot_v);

					em.quadrupole_derivative2[i][j] += 2.0 * m * (ba.velocity[i] * ba.velocity[j] + ba.position[i] * ba.acceleration[j] - (1.0 / 3.0) * delta_ij * (v2 + r_dot_a));
				}
			}
		}

		for (size_t a = 0; a < n; ++a) {
			const auto& ba = bodies[a];
			const double m = ba.mass;
			for (size_t i = 0; i < 3; ++i) {
				for (size_t j = 0; j < 3; ++j) {
					const double delta_ij = (i == j) ? 1.0 : 0.0;
					const double v_dot_a = ba.velocity[0] * ba.acceleration[0] + ba.velocity[1] * ba.acceleration[1] + ba.velocity[2] * ba.acceleration[2];
					em.quadrupole_derivative3[i][j] += 2.0 * m * (3.0 * ba.velocity[i] * ba.acceleration[j] - delta_ij * v_dot_a);
				}
			}
		}

		double ddd_i2 = 0.0;
		for (size_t i = 0; i < 3; ++i) {
			for (size_t j = 0; j < 3; ++j) {
				ddd_i2 += em.quadrupole_derivative3[i][j] * em.quadrupole_derivative3[i][j];
			}
		}

		const double c5 = speed_of_light * speed_of_light * speed_of_light * speed_of_light * speed_of_light;
		em.radiated_power = (gravitational_constant / (5.0 * c5)) * ddd_i2;

		const double factor_j = 2.0 * gravitational_constant / (5.0 * c5);
		em.angular_momentum_loss[0] = factor_j * (em.quadrupole_derivative2[1][0] * em.quadrupole_derivative3[2][0] + em.quadrupole_derivative2[1][1] * em.quadrupole_derivative3[2][1] + em.quadrupole_derivative2[1][2] * em.quadrupole_derivative3[2][2] - em.quadrupole_derivative2[2][0] * em.quadrupole_derivative3[1][0] - em.quadrupole_derivative2[2][1] * em.quadrupole_derivative3[1][1] - em.quadrupole_derivative2[2][2] * em.quadrupole_derivative3[1][2]);
		em.angular_momentum_loss[1] = factor_j * (em.quadrupole_derivative2[2][0] * em.quadrupole_derivative3[0][0] + em.quadrupole_derivative2[2][1] * em.quadrupole_derivative3[0][1] + em.quadrupole_derivative2[2][2] * em.quadrupole_derivative3[0][2] - em.quadrupole_derivative2[0][0] * em.quadrupole_derivative3[2][0] - em.quadrupole_derivative2[0][1] * em.quadrupole_derivative3[2][1] - em.quadrupole_derivative2[0][2] * em.quadrupole_derivative3[2][2]);
		em.angular_momentum_loss[2] = factor_j * (em.quadrupole_derivative2[0][0] * em.quadrupole_derivative3[1][0] + em.quadrupole_derivative2[0][1] * em.quadrupole_derivative3[1][1] + em.quadrupole_derivative2[0][2] * em.quadrupole_derivative3[1][2] - em.quadrupole_derivative2[1][0] * em.quadrupole_derivative3[0][0] - em.quadrupole_derivative2[1][1] * em.quadrupole_derivative3[0][1] - em.quadrupole_derivative2[1][2] * em.quadrupole_derivative3[0][2]);

		return em;
	}

	[[nodiscard]] static GravitationalWavePolarizations compute_strain_polarizations(
		const GravitationalWaveEmission& em,
		double distance_meters,
		double inclination_rad,
		double polarization_angle_rad = 0.0,
		double speed_of_light = Core::PhysicalConstants<double>::SPEED_OF_LIGHT,
		double gravitational_constant = Core::PhysicalConstants<double>::GRAVITATIONAL_CONSTANT
	) noexcept {
		if (distance_meters <= 0.0) {
			return GravitationalWavePolarizations{};
		}

		const double cos_i = std::cos(inclination_rad);
		const double sin_i = std::sin(inclination_rad);
		const double cos_p = std::cos(polarization_angle_rad);
		const double sin_p = std::sin(polarization_angle_rad);

		const std::array<double, 3> p = {-sin_p, cos_p, 0.0};
		const std::array<double, 3> q = {-cos_i * cos_p, -cos_i * sin_p, sin_i};

		double p_ddot_p = 0.0;
		double q_ddot_q = 0.0;
		double p_ddot_q = 0.0;

		for (size_t i = 0; i < 3; ++i) {
			for (size_t j = 0; j < 3; ++j) {
				const double dij = em.quadrupole_derivative2[i][j];
				p_ddot_p += p[i] * dij * p[j];
				q_ddot_q += q[i] * dij * q[j];
				p_ddot_q += p[i] * dij * q[j];
			}
		}

		const double c4 = speed_of_light * speed_of_light * speed_of_light * speed_of_light;
		const double factor = gravitational_constant / (c4 * distance_meters);

		return GravitationalWavePolarizations{
			.h_plus = factor * (p_ddot_p - q_ddot_q),
			.h_cross = factor * (2.0 * p_ddot_q)
		};
	}

	[[nodiscard]] static GravitationalWavePolarizations compute_binary_quadrupole_strain(
		double m1,
		double m2,
		double separation,
		double orbital_phase,
		double distance_meters,
		double inclination_rad,
		double speed_of_light = Core::PhysicalConstants<double>::SPEED_OF_LIGHT,
		double gravitational_constant = Core::PhysicalConstants<double>::GRAVITATIONAL_CONSTANT
	) noexcept {
		if (separation <= 0.0 || distance_meters <= 0.0) {
			return GravitationalWavePolarizations{};
		}

		const double mu = (m1 * m2) / (m1 + m2);
		const double m_tot = m1 + m2;
		const double c4 = speed_of_light * speed_of_light * speed_of_light * speed_of_light;
		const double omega = std::sqrt(gravitational_constant * m_tot / (separation * separation * separation));
		const double h0 = (4.0 * gravitational_constant * mu * separation * separation * omega * omega) / (c4 * distance_meters);

		const double cos_i = std::cos(inclination_rad);
		const double hp = h0 * 0.5 * (1.0 + cos_i * cos_i) * std::cos(2.0 * orbital_phase);
		const double hc = h0 * cos_i * std::sin(2.0 * orbital_phase);

		return GravitationalWavePolarizations{
			.h_plus = hp,
			.h_cross = hc
		};
	}
};

}

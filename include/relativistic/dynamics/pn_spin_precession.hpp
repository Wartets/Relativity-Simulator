#pragma once

#include "relativistic/dynamics/pn_body.hpp"
#include "relativistic/dynamics/pn_orders.hpp"
#include <array>
#include <cmath>
#include <algorithm>

namespace Relativistic::Dynamics {

struct BinarySpinPrecessionRates {
	std::array<double, 3> omega_1{0.0, 0.0, 0.0};
	std::array<double, 3> omega_2{0.0, 0.0, 0.0};
	std::array<double, 3> d_spin_1{0.0, 0.0, 0.0};
	std::array<double, 3> d_spin_2{0.0, 0.0, 0.0};
};

class PostNewtonianSpinSolver {
private:
	static constexpr double dot3(const std::array<double, 3>& a, const std::array<double, 3>& b) noexcept {
		return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
	}

	static constexpr std::array<double, 3> cross3(const std::array<double, 3>& a, const std::array<double, 3>& b) noexcept {
		return {
			a[1] * b[2] - a[2] * b[1],
			a[2] * b[0] - a[0] * b[2],
			a[0] * b[1] - a[1] * b[0]
		};
	}

	static constexpr std::array<double, 3> add3(const std::array<double, 3>& a, const std::array<double, 3>& b) noexcept {
		return {a[0] + b[0], a[1] + b[1], a[2] + b[2]};
	}

	static constexpr std::array<double, 3> mul3(const std::array<double, 3>& a, double s) noexcept {
		return {a[0] * s, a[1] * s, a[2] * s};
	}

public:
	[[nodiscard]] static BinarySpinPrecessionRates compute_spin_precession_binary(
		const std::array<double, 3>& r_vec,
		const std::array<double, 3>& v_vec,
		double m1,
		double m2,
		const std::array<double, 3>& s1,
		const std::array<double, 3>& s2,
		const PNOrderConfig& config
	) noexcept {
		BinarySpinPrecessionRates res{};

		const double r2 = dot3(r_vec, r_vec);
		if (r2 <= 0.0) {
			return res;
		}

		const double r = std::sqrt(r2);
		const double r3 = r2 * r;
		const std::array<double, 3> n = mul3(r_vec, 1.0 / r);

		const double c = config.speed_of_light;
		const double g = config.gravitational_constant;
		const double c2 = c * c;
		const double m_total = m1 + m2;
		const double mu = (m1 * m2) / m_total;

		const std::array<double, 3> l_orb = mul3(cross3(r_vec, v_vec), mu);

		if (config.enable_spin_orbit) {
			const double factor_so1 = g / (c2 * r3) * (2.0 + (1.5 * m2 / m1));
			const double factor_so2 = g / (c2 * r3) * (2.0 + (1.5 * m1 / m2));

			res.omega_1 = add3(res.omega_1, mul3(l_orb, factor_so1));
			res.omega_2 = add3(res.omega_2, mul3(l_orb, factor_so2));
		}

		if (config.enable_spin_spin) {
			const double factor_ss = g / (c2 * r3);
			const double n_dot_s2 = dot3(n, s2);
			const std::array<double, 3> ss_vec1 = add3(s2, mul3(n, -3.0 * n_dot_s2));
			res.omega_1 = add3(res.omega_1, mul3(ss_vec1, factor_ss));

			const double n_dot_s1 = dot3(n, s1);
			const std::array<double, 3> ss_vec2 = add3(s1, mul3(n, -3.0 * n_dot_s1));
			res.omega_2 = add3(res.omega_2, mul3(ss_vec2, factor_ss));

			if (config.enable_spin_self) {
				const double factor_s1s1 = 0.5 * g * (m2 / m1) / (c2 * r3);
				const std::array<double, 3> self_vec1 = add3(s1, mul3(n, -3.0 * n_dot_s1));
				res.omega_1 = add3(res.omega_1, mul3(self_vec1, factor_s1s1));

				const double factor_s2s2 = 0.5 * g * (m1 / m2) / (c2 * r3);
				const std::array<double, 3> self_vec2 = add3(s2, mul3(n, -3.0 * n_dot_s2));
				res.omega_2 = add3(res.omega_2, mul3(self_vec2, factor_s2s2));
			}
		}

		res.d_spin_1 = cross3(res.omega_1, s1);
		res.d_spin_2 = cross3(res.omega_2, s2);

		return res;
	}
};

}

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

struct PostNewtonianAccelerations {
	std::array<double, 3> a_newton{0.0, 0.0, 0.0};
	std::array<double, 3> a_1pn{0.0, 0.0, 0.0};
	std::array<double, 3> a_2pn{0.0, 0.0, 0.0};
	std::array<double, 3> a_2_5pn{0.0, 0.0, 0.0};
	std::array<double, 3> a_3pn{0.0, 0.0, 0.0};
	std::array<double, 3> a_3_5pn{0.0, 0.0, 0.0};
	std::array<double, 3> a_spin_orbit{0.0, 0.0, 0.0};
	std::array<double, 3> a_spin_spin{0.0, 0.0, 0.0};
	std::array<double, 3> a_harmonics{0.0, 0.0, 0.0};
	std::array<double, 3> a_total{0.0, 0.0, 0.0};
};

class PostNewtonianSolver {
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

	static constexpr std::array<double, 3> sub3(const std::array<double, 3>& a, const std::array<double, 3>& b) noexcept {
		return {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
	}

	static constexpr std::array<double, 3> mul3(const std::array<double, 3>& a, double s) noexcept {
		return {a[0] * s, a[1] * s, a[2] * s};
	}

public:
	[[nodiscard]] static PostNewtonianAccelerations compute_binary_relative_acceleration(
		const std::array<double, 3>& r_vec,
		const std::array<double, 3>& v_vec,
		double m1,
		double m2,
		const std::array<double, 3>& s1,
		const std::array<double, 3>& s2,
		const PNOrderConfig& config
	) noexcept {
		PostNewtonianAccelerations res{};

		const double r2 = dot3(r_vec, r_vec);
		if (r2 <= 0.0) {
			return res;
		}

		const double r = std::sqrt(r2);
		const double inv_r = 1.0 / r;
		const std::array<double, 3> n = mul3(r_vec, inv_r);

		const double c = config.speed_of_light;
		const double g = config.gravitational_constant;
		const double c2 = c * c;
		const double c4 = c2 * c2;
		const double c5 = c4 * c;
		const double c6 = c4 * c2;
		const double c7 = c6 * c;

		const double m_total = m1 + m2;
		const double mu = (m1 * m2) / m_total;
		const double eta = mu / m_total;
		const double gm = g * m_total;
		const double gamma = gm / (r * c2);

		const double v2 = dot3(v_vec, v_vec) / c2;
		const double r_dot = dot3(n, v_vec) / c;
		const double r_dot2 = r_dot * r_dot;

		if (config.enable_newtonian) {
			const double a_n_mag = -gm / r2;
			res.a_newton = mul3(n, a_n_mag);
		}

		if (config.enable_1pn) {
			const double a_1pn_n = (1.0 + 3.0 * eta) * v2 - 1.5 * eta * r_dot2 - 2.0 * (2.0 + eta) * gamma;
			const double a_1pn_v = 2.0 * (2.0 - eta) * r_dot;
			const double scale = gm / r2;
			res.a_1pn = add3(mul3(n, scale * a_1pn_n), mul3(v_vec, (scale / c) * a_1pn_v));
		}

		if (config.enable_2pn) {
			const double v4 = v2 * v2;
			const double r_dot4 = r_dot2 * r_dot2;
			const double a_2pn_n = 0.75 * (12.0 + 29.0 * eta) * (gamma * gamma)
				- eta * (3.0 - 4.0 * eta) * v4
				- 1.875 * eta * (1.0 - 3.0 * eta) * r_dot4
				+ 1.5 * eta * (3.0 - 4.0 * eta) * v2 * r_dot2
				- 0.5 * eta * (13.0 - 4.0 * eta) * gamma * v2
				- (20.0 + 29.0 * eta) * gamma * r_dot2;

			const double a_2pn_v = 0.5 * (15.0 + 4.0 * eta) * v2
				- 1.5 * (3.0 + 2.0 * eta) * r_dot2
				- 0.5 * (55.0 + 12.0 * eta) * gamma;

			const double scale = gm / r2;
			res.a_2pn = add3(mul3(n, scale * a_2pn_n), mul3(v_vec, (scale / c) * (a_2pn_v * r_dot)));
		}

		if (config.enable_2_5pn) {
			const double factor_rad = (8.0 / 5.0) * (g * g * m_total * m_total * eta) / (c5 * r * r * r);
			const double term_n = (3.0 * v2 * c2 + (17.0 / 3.0) * (gm / r)) * (r_dot * c);
			const double term_v = -(v2 * c2 + 3.0 * (gm / r));
			res.a_2_5pn = add3(mul3(n, factor_rad * term_n), mul3(v_vec, factor_rad * term_v));
		}

		if (config.enable_3pn) {
			const double v4 = v2 * v2;
			const double v6 = v4 * v2;
			const double r_dot4 = r_dot2 * r_dot2;
			const double r_dot6 = r_dot4 * r_dot2;
			const double pi2 = std::numbers::pi_v<double> * std::numbers::pi_v<double>;
			const double ln_r = std::log(r / config.r0_scale);

			const double a_3pn_n = -eta * (1.0 - 5.0 * eta + 5.0 * eta * eta) * v6
				+ 2.1875 * eta * (1.0 - 5.0 * eta + 5.0 * eta * eta) * r_dot6
				- 1.875 * eta * (3.0 - 10.0 * eta + 5.0 * eta * eta) * v2 * r_dot4
				+ 0.375 * eta * (15.0 - 25.0 * eta + 2.0 * eta * eta) * v4 * r_dot2
				+ gamma * (0.5 * eta * (208.0 - 85.0 * eta - 4.0 * eta * eta) * v4
					+ 0.125 * eta * (177.0 - 273.0 * eta - 8.0 * eta * eta) * r_dot4
					- 1.5 * eta * (83.0 - 56.0 * eta - 2.0 * eta * eta) * v2 * r_dot2)
				+ (gamma * gamma) * (((3167.0 / 48.0) - (16.0 / 3.0) * ln_r + ((227.0 / 6.0) + (41.0 / 64.0) * pi2) * eta - 10.0 * eta * eta) * v2
					+ ((-1579.0 / 24.0) + (32.0 / 3.0) * ln_r - ((827.0 / 12.0) + (41.0 / 64.0) * pi2) * eta + 12.0 * eta * eta) * r_dot2)
				+ (gamma * gamma * gamma) * (-3.2 - ((175.0 / 12.0) - (41.0 / 64.0) * pi2) * eta + (227.0 / 6.0) * eta * eta);

			const double a_3pn_v = 0.125 * (15.0 - 55.0 * eta + 44.0 * eta * eta) * v4
				+ 0.625 * (5.0 - 5.0 * eta + 4.0 * eta * eta) * r_dot4
				- 0.5 * (15.0 - 25.0 * eta + 12.0 * eta * eta) * v2 * r_dot2
				- gamma * (0.5 * (125.0 - 98.0 * eta + 12.0 * eta * eta) * v2 - 1.5 * (25.0 - 4.0 * eta + 4.0 * eta * eta) * r_dot2)
				- (gamma * gamma) * ((1537.0 / 24.0) - (16.0 / 3.0) * ln_r + ((151.0 / 12.0) + (41.0 / 64.0) * pi2) * eta + 12.0 * eta * eta);

			const double scale = gm / r2;
			res.a_3pn = add3(mul3(n, scale * a_3pn_n), mul3(v_vec, (scale / c) * (a_3pn_v * r_dot)));
		}

		if (config.enable_3_5pn) {
			const double v4 = v2 * v2;
			const double r_dot4 = r_dot2 * r_dot2;
			const double factor_rad35 = -(8.0 / 5.0) * (g * g * m_total * m_total * eta) / (c7 * r * r * r);

			const double term_n_35 = (r_dot * c) * (((53.0 / 8.0) - (55.0 / 8.0) * eta) * v4 * c4
				- ((25.0 / 8.0) - (45.0 / 8.0) * eta) * v2 * c2 * r_dot2 * c2
				+ (35.0 / 8.0) * (1.0 - eta) * r_dot4 * c4
				+ (gm / r) * (((413.0 / 12.0) + 3.25 * eta) * v2 * c2 + (34.25 - 0.75 * eta) * r_dot2 * c2)
				+ (gm * gm / (r * r)) * ((116.0 / 3.0) + (22.0 / 3.0) * eta));

			const double term_v_35 = -(((13.0 / 8.0) - (15.0 / 8.0) * eta) * v4 * c4
				- ((15.0 / 8.0) - (25.0 / 8.0) * eta) * v2 * c2 * r_dot2 * c2
				+ (15.0 / 8.0) * (1.0 - eta) * r_dot4 * c4
				+ (gm / r) * (((389.0 / 12.0) + 2.25 * eta) * v2 * c2 + (22.25 - 1.25 * eta) * r_dot2 * c2)
				+ (gm * gm / (r * r)) * ((287.0 / 6.0) + (31.0 / 6.0) * eta));

			res.a_3_5pn = add3(mul3(n, factor_rad35 * term_n_35), mul3(v_vec, factor_rad35 * term_v_35));
		}

		if (config.enable_spin_orbit) {
			const std::array<double, 3> s_total = add3(s1, s2);
			const std::array<double, 3> sigma = add3(mul3(s1, m2 / m1), mul3(s2, m1 / m2));
			const std::array<double, 3> s_eff = add3(mul3(s_total, 2.0), sigma);

			const std::array<double, 3> n_cross_seff = cross3(n, s_eff);
			const std::array<double, 3> s_eff_prime = add3(mul3(s_total, 2.0), mul3(sigma, 1.5));
			const std::array<double, 3> v_cross_seffp = cross3(v_vec, s_eff_prime);

			const double v_dot_ncross_seff = dot3(v_vec, n_cross_seff);
			const double r3 = r * r * r;
			const double factor_so = g / (c2 * r3);

			const std::array<double, 3> term1 = mul3(n, 1.5 * v_dot_ncross_seff);
			const std::array<double, 3> term2 = v_cross_seffp;
			const std::array<double, 3> term3 = mul3(n_cross_seff, -1.5 * (r_dot * c));

			res.a_spin_orbit = mul3(add3(term1, add3(term2, term3)), factor_so);
		}

		if (config.enable_spin_spin) {
			const double r4 = r2 * r2;
			const double factor_ss = -3.0 * g / (mu * c2 * r4);

			const double s1_dot_s2 = dot3(s1, s2);
			const double n_dot_s1 = dot3(n, s1);
			const double n_dot_s2 = dot3(n, s2);

			const std::array<double, 3> ss_term = add3(
				mul3(n, s1_dot_s2 - 5.0 * n_dot_s1 * n_dot_s2),
				add3(mul3(s1, n_dot_s2), mul3(s2, n_dot_s1))
			);

			std::array<double, 3> total_ss = mul3(ss_term, factor_ss);

			if (config.enable_spin_self) {
				const double s1_sq = dot3(s1, s1);
				const double s2_sq = dot3(s2, s2);

				const double factor_s1s1 = -1.5 * g * (m2 / m1) / (mu * c2 * r4);
				const std::array<double, 3> s1s1_term = add3(
					mul3(n, s1_sq - 5.0 * n_dot_s1 * n_dot_s1),
					mul3(s1, 2.0 * n_dot_s1)
				);

				const double factor_s2s2 = -1.5 * g * (m1 / m2) / (mu * c2 * r4);
				const std::array<double, 3> s2s2_term = add3(
					mul3(n, s2_sq - 5.0 * n_dot_s2 * n_dot_s2),
					mul3(s2, 2.0 * n_dot_s2)
				);

				total_ss = add3(total_ss, add3(mul3(s1s1_term, factor_s1s1), mul3(s2s2_term, factor_s2s2)));
			}

			res.a_spin_spin = total_ss;
		}

		res.a_total = add3(res.a_newton,
			add3(res.a_1pn,
				add3(res.a_2pn,
					add3(res.a_2_5pn,
						add3(res.a_3pn,
							add3(res.a_3_5pn,
								add3(res.a_spin_orbit, res.a_spin_spin)))))));

		return res;
	}

	static void compute_nbody_accelerations(
		std::span<const PostNewtonianBody> bodies,
		const PNOrderConfig& config,
		std::span<PostNewtonianAccelerations> out_accelerations
	) noexcept {
		const size_t n = bodies.size();
		if (out_accelerations.size() < n) {
			return;
		}

		for (size_t i = 0; i < n; ++i) {
			out_accelerations[i] = PostNewtonianAccelerations{};
		}

		const double c = config.speed_of_light;
		const double g = config.gravitational_constant;
		const double c2 = c * c;

		for (size_t i = 0; i < n; ++i) {
			const auto& bi = bodies[i];
			const double vi2 = bi.speed_squared();

			for (size_t j = 0; j < n; ++j) {
				if (i == j) continue;
				const auto& bj = bodies[j];

				const std::array<double, 3> r_ij = sub3(bi.position, bj.position);
				const double r2 = dot3(r_ij, r_ij);
				if (r2 <= 0.0) continue;

				const double r = std::sqrt(r2);
				const double inv_r = 1.0 / r;
				const std::array<double, 3> n_ij = mul3(r_ij, inv_r);
				const std::array<double, 3> n_ji = mul3(n_ij, -1.0);
				const std::array<double, 3> v_ij = sub3(bi.velocity, bj.velocity);

				const double vj2 = bj.speed_squared();
				const double vi_dot_vj = dot3(bi.velocity, bj.velocity);
				const double n_dot_vj = dot3(n_ij, bj.velocity);

				if (config.enable_newtonian) {
					const double an_mag = g * bj.mass / r2;
					out_accelerations[i].a_newton = add3(out_accelerations[i].a_newton, mul3(n_ji, an_mag));
				}

				if (config.enable_spherical_harmonics && (bj.j2 != 0.0 || bj.j3 != 0.0 || bj.j4 != 0.0)) {
					const double r_ref = bj.reference_radius;
					const double r_ref_over_r = r_ref / r;
					const double r_ref_over_r2 = r_ref_over_r * r_ref_over_r;
					const double gm_j = g * bj.mass;

					const double z_rel = bi.position[2] - bj.position[2];
					const double sin_phi = z_rel / r;
					const double sin_phi2 = sin_phi * sin_phi;

					std::array<double, 3> a_h_pair{0.0, 0.0, 0.0};

					if (bj.j2 != 0.0) {
						const double factor_j2 = 1.5 * gm_j * bj.j2 * (r_ref * r_ref) / (r2 * r2);
						const double term_radial = 1.0 - 5.0 * sin_phi2;
						a_h_pair[0] += factor_j2 * (bi.position[0] - bj.position[0]) * term_radial / r;
						a_h_pair[1] += factor_j2 * (bi.position[1] - bj.position[1]) * term_radial / r;
						a_h_pair[2] += factor_j2 * ((bi.position[2] - bj.position[2]) * term_radial / r + 2.0 * sin_phi);
					}

					if (bj.j3 != 0.0) {
						const double factor_j3 = 0.5 * gm_j * bj.j3 * (r_ref * r_ref * r_ref) / (r2 * r2 * r);
						const double term_radial_j3 = 5.0 * sin_phi * (7.0 * sin_phi2 - 3.0);
						a_h_pair[0] += factor_j3 * (bi.position[0] - bj.position[0]) * (5.0 * sin_phi - 35.0 / 3.0 * sin_phi2 * sin_phi) / r;
						a_h_pair[1] += factor_j3 * (bi.position[1] - bj.position[1]) * (5.0 * sin_phi - 35.0 / 3.0 * sin_phi2 * sin_phi) / r;
						a_h_pair[2] += factor_j3 * (term_radial_j3 * sin_phi + 3.0 - 15.0 * sin_phi2);
					}

					if (bj.j4 != 0.0) {
						const double factor_j4 = (5.0 / 8.0) * gm_j * bj.j4 * (r_ref_over_r2 * r_ref_over_r2) * (gm_j / r2);
						const double term_radial_j4 = 3.0 - 30.0 * sin_phi2 + 35.0 * sin_phi2 * sin_phi2;
						a_h_pair[0] += factor_j4 * (bi.position[0] - bj.position[0]) * term_radial_j4 / r;
						a_h_pair[1] += factor_j4 * (bi.position[1] - bj.position[1]) * term_radial_j4 / r;
						a_h_pair[2] += factor_j4 * ((bi.position[2] - bj.position[2]) * term_radial_j4 / r + 4.0 * sin_phi * (3.0 - 7.0 * sin_phi2));
					}

					out_accelerations[i].a_harmonics = add3(out_accelerations[i].a_harmonics, a_h_pair);
				}

				if (config.enable_1pn) {
					double sum_u_i = 0.0;
					for (size_t k = 0; k < n; ++k) {
						if (k == i) continue;
						const double r_ik = std::sqrt(std::max(dot3(sub3(bi.position, bodies[k].position), sub3(bi.position, bodies[k].position)), 1e-30));
						sum_u_i += g * bodies[k].mass / r_ik;
					}

					double sum_u_j = 0.0;
					for (size_t k = 0; k < n; ++k) {
						if (k == j) continue;
						const double r_jk = std::sqrt(std::max(dot3(sub3(bj.position, bodies[k].position), sub3(bj.position, bodies[k].position)), 1e-30));
						sum_u_j += g * bodies[k].mass / r_jk;
					}

					const double term_eih_n = vi2 + 2.0 * vj2 - 4.0 * vi_dot_vj - 1.5 * n_dot_vj * n_dot_vj - 4.0 * sum_u_i - sum_u_j;
					const double term_eih_v = dot3(n_ji, sub3(mul3(bi.velocity, 4.0), mul3(bj.velocity, 3.0)));

					const double scale_1pn = (g * bj.mass) / (c2 * r2);
					const std::array<double, 3> a_pair_1pn = add3(
						mul3(n_ji, scale_1pn * term_eih_n),
						mul3(v_ij, scale_1pn * term_eih_v)
					);
					out_accelerations[i].a_1pn = add3(out_accelerations[i].a_1pn, a_pair_1pn);

					for (size_t k = 0; k < n; ++k) {
						if (k == j) continue;
						const auto r_jk_vec = sub3(bj.position, bodies[k].position);
						const double r_jk2 = dot3(r_jk_vec, r_jk_vec);
						if (r_jk2 <= 0.0) continue;
						const double r_jk = std::sqrt(r_jk2);
						const std::array<double, 3> n_kj = mul3(r_jk_vec, -1.0 / r_jk);
						const double mag = (7.0 * g * g * bj.mass * bodies[k].mass) / (2.0 * c2 * r * r_jk2);
						out_accelerations[i].a_1pn = add3(out_accelerations[i].a_1pn, mul3(n_kj, mag));
					}
				}
			}
		}

		for (size_t i = 0; i < n; ++i) {
			out_accelerations[i].a_total = add3(
				out_accelerations[i].a_newton,
				add3(out_accelerations[i].a_1pn,
					add3(out_accelerations[i].a_2pn,
						add3(out_accelerations[i].a_2_5pn,
							add3(out_accelerations[i].a_3pn,
								add3(out_accelerations[i].a_3_5pn,
									add3(out_accelerations[i].a_spin_orbit,
										add3(out_accelerations[i].a_spin_spin, out_accelerations[i].a_harmonics)))))))
			);
		}
	}
};

}

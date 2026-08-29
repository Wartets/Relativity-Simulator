#include "relativistic/metrics/kerr.hpp"
#include "relativistic/metrics/kerr_schild.hpp"
#include "relativistic/metrics/kerr_invariants.hpp"
#include "relativistic/core/christoffel.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
#include <numbers>

using namespace Relativistic::Core;
using namespace Relativistic::Metrics;

void test_kerr_boyer_lindquist_properties() {
	const double m = 1.0;
	const double a = 0.8;
	const double c = 1.0;
	const double g = 1.0;

	const KerrMetric<double> kerr(m, a, c, g);

	assert(kerr.is_subextremal());
	assert(!kerr.is_extremal());
	assert(!kerr.is_hyperextremal());

	const double r_plus_expected = 1.0 + std::sqrt(1.0 - 0.8 * 0.8);
	const double r_minus_expected = 1.0 - std::sqrt(1.0 - 0.8 * 0.8);
	assert(std::abs(kerr.outer_horizon_radius() - r_plus_expected) < 1e-14);
	assert(std::abs(kerr.inner_horizon_radius() - r_minus_expected) < 1e-14);

	const double r_e_pole = kerr.outer_ergosphere_radius(0.0);
	assert(std::abs(r_e_pole - r_plus_expected) < 1e-14);

	const double r_e_eq = kerr.outer_ergosphere_radius(std::numbers::pi / 2.0);
	assert(std::abs(r_e_eq - 2.0) < 1e-14);

	const FourVector<double> x(0.0, 5.0, std::numbers::pi / 3.0, 0.2);
	const auto g_cov = kerr.metric_tensor(x);
	const auto g_con = kerr.inverse_metric(x);

	assert(g_cov(0, 3) != 0.0);
	assert(g_cov(3, 0) == g_cov(0, 3));

	for (size_t i = 0; i < 4; ++i) {
		for (size_t j = 0; j < 4; ++j) {
			double delta_ij = 0.0;
			for (size_t k = 0; k < 4; ++k) {
				delta_ij += g_cov(i, k) * g_con(k, j);
			}
			const double target = (i == j) ? 1.0 : 0.0;
			assert(std::abs(delta_ij - target) < 1e-13);
		}
	}

	const auto gamma_analytic = kerr.christoffel_symbols(x);
	const auto gamma_numeric = compute_christoffel_numerical<DerivativeOrder::EighthOrder>(kerr, x);

	for (size_t s = 0; s < 4; ++s) {
		for (size_t u = 0; u < 4; ++u) {
			for (size_t v = 0; v < 4; ++v) {
				const double diff = std::abs(gamma_analytic(s, u, v) - gamma_numeric(s, u, v));
				assert(diff < 1e-10);
			}
		}
	}

	const double omega_zamo = compute_zamo_angular_velocity(kerr, x);
	assert(omega_zamo > 0.0);
}

void test_kerr_schild_cartesian_properties() {
	const double m = 1.0;
	const double a = 0.85;
	const double c = 1.0;
	const double g = 1.0;

	const KerrSchildMetric<double> ks(m, a, c, g);

	const FourVector<double> x(0.0, 3.0, 2.0, 1.5);
	const auto g_cov = ks.metric_tensor(x);
	const auto g_con = ks.inverse_metric(x);

	for (size_t i = 0; i < 4; ++i) {
		for (size_t j = 0; j < 4; ++j) {
			double delta_ij = 0.0;
			for (size_t k = 0; k < 4; ++k) {
				delta_ij += g_cov(i, k) * g_con(k, j);
			}
			const double target = (i == j) ? 1.0 : 0.0;
			assert(std::abs(delta_ij - target) < 1e-13);
		}
	}

	const double det = determinant_4x4(g_cov);
	assert(std::abs(det - (-1.0)) < 1e-12);

	const auto gamma_analytic = ks.christoffel_symbols(x);
	const auto gamma_numeric = compute_christoffel_numerical<DerivativeOrder::EighthOrder>(ks, x);

	for (size_t s = 0; s < 4; ++s) {
		for (size_t u = 0; u < 4; ++u) {
			for (size_t v = 0; v < 4; ++v) {
				const double diff = std::abs(gamma_analytic(s, u, v) - gamma_numeric(s, u, v));
				assert(diff < 1e-10);
			}
		}
	}
}

int main() {
	test_kerr_boyer_lindquist_properties();
	test_kerr_schild_cartesian_properties();
	return 0;
}

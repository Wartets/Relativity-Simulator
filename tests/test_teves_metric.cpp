#include "relativistic/modified_gravity/teves.hpp"
#include "relativistic/core/christoffel.hpp"
#include <cassert>
#include <cmath>

int main() {
	using namespace Relativistic::ModifiedGravity;
	using namespace Relativistic::Core;

	const double m_geom = 10.0;
	const double a0_geom = 1e-3;
	const double k_geom = 0.01;
	const double c_geom = 1.0;
	const double g_geom = 1.0;

	TeVeSSpacetimeMetric<double> metric_geom(m_geom, a0_geom, k_geom, c_geom, g_geom);
	static_assert(SpacetimeMetric<TeVeSSpacetimeMetric<double>>);

	const FourVector<double> x_geom(0.0, 50.0, std::numbers::pi_v<double> / 3.0, 0.0);

	const auto g_g = metric_geom.metric_tensor(x_geom);
	const auto inv_g_g = metric_geom.inverse_metric(x_geom);
	const auto id_g = contract_metric_inverse(inv_g_g, g_g);

	for (size_t i = 0; i < 4; ++i) {
		for (size_t j = 0; j < 4; ++j) {
			const double target = (i == j) ? 1.0 : 0.0;
			assert(std::abs(id_g(i, j) - target) < 1e-12);
		}
	}

	const auto gamma_ana_g = metric_geom.christoffel_symbols(x_geom);
	const auto gamma_num_g = compute_christoffel_numerical<DerivativeOrder::EighthOrder>(metric_geom, x_geom);

	for (size_t sigma = 0; sigma < 4; ++sigma) {
		for (size_t mu = 0; mu < 4; ++mu) {
			for (size_t nu = 0; nu < 4; ++nu) {
				const double diff = std::abs(gamma_ana_g(sigma, mu, nu) - gamma_num_g(sigma, mu, nu));
				const double scale = std::abs(gamma_ana_g(sigma, mu, nu)) + 1.0;
				assert(diff / scale < 1e-5);
			}
		}
	}

	const double m_si = 1e11 * PhysicalConstants<double>::SOLAR_MASS;
	const double a0_si = 1.2e-10;
	const double k_si = 0.01;

	TeVeSSpacetimeMetric<double> metric_si(m_si, a0_si, k_si);
	const double r_kpc = 50.0 * 1000.0 * 3.085677581491367e16;
	const FourVector<double> x_si(0.0, r_kpc, std::numbers::pi_v<double> / 2.0, 0.0);

	const auto g_si = metric_si.metric_tensor(x_si);
	const auto inv_g_si = metric_si.inverse_metric(x_si);
	const auto id_si = contract_metric_inverse(inv_g_si, g_si);

	for (size_t i = 0; i < 4; ++i) {
		for (size_t j = 0; j < 4; ++j) {
			const double target = (i == j) ? 1.0 : 0.0;
			assert(std::abs(id_si(i, j) - target) < 1e-12);
		}
	}

	const auto gamma_si = metric_si.christoffel_symbols(x_si);
	assert(gamma_si(1, 0, 0) > 0.0);
	assert(gamma_si(0, 0, 1) > 0.0);

	const double g_newton = PhysicalConstants<double>::GRAVITATIONAL_CONSTANT * m_si / (r_kpc * r_kpc);
	const double a_mond_expected = 0.5 * (g_newton + std::sqrt(g_newton * g_newton + 4.0 * g_newton * a0_si));
	const double v_circ_expected = std::sqrt(r_kpc * a_mond_expected);
	const double v_asymp_tully_fisher = std::sqrt(std::sqrt(PhysicalConstants<double>::GRAVITATIONAL_CONSTANT * m_si * a0_si));

	const double omega_sq = -gamma_si(1, 0, 0) / gamma_si(1, 3, 3);
	const double v_circ_metric = r_kpc * std::sqrt(omega_sq);
	assert(std::abs(v_circ_metric - v_circ_expected) / v_circ_expected < 1e-4);

	const double r_asymp = 250.0 * 1000.0 * 3.085677581491367e16;
	const FourVector<double> x_asymp(0.0, r_asymp, std::numbers::pi_v<double> / 2.0, 0.0);
	const auto gamma_asymp = metric_si.christoffel_symbols(x_asymp);
	const double omega_sq_asymp = -gamma_asymp(1, 0, 0) / gamma_asymp(1, 3, 3);
	const double v_circ_asymp = r_asymp * std::sqrt(omega_sq_asymp);

	assert(std::abs(v_circ_asymp - v_asymp_tully_fisher) / v_asymp_tully_fisher < 0.02);

	return 0;
}

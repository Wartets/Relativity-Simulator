#include "relativistic/metrics/reissner_nordstrom.hpp"
#include "relativistic/metrics/kerr_newman.hpp"
#include "relativistic/metrics/schwarzschild_de_sitter.hpp"
#include "relativistic/metrics/kerr_de_sitter.hpp"
#include "relativistic/metrics/flrw.hpp"
#include "relativistic/core/christoffel.hpp"
#include "relativistic/integrators/rk45_adaptive.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

using namespace Relativistic;
using namespace Relativistic::Core;
using namespace Relativistic::Metrics;
using namespace Relativistic::Integrators;

void test_reissner_nordstrom_horizons() {
	ReissnerNordstromMetric<double> rn_sub(1.0, 0.6);
	assert(rn_sub.is_subextremal());
	assert(!rn_sub.is_extremal());
	assert(!rn_sub.is_hyperextremal());
	assert(std::abs(rn_sub.outer_horizon_radius() - 1.8) < 1e-12);
	assert(std::abs(rn_sub.inner_horizon_radius() - 0.2) < 1e-12);

	ReissnerNordstromMetric<double> rn_ext(1.0, 1.0);
	assert(rn_ext.is_extremal());
	assert(std::abs(rn_ext.outer_horizon_radius() - 1.0) < 1e-12);
	assert(std::abs(rn_ext.inner_horizon_radius() - 1.0) < 1e-12);

	ReissnerNordstromMetric<double> rn_naked(1.0, 1.2);
	assert(rn_naked.is_hyperextremal());

	ReissnerNordstromMetric<double> rn_sch(1.0, 0.0);
	assert(std::abs(rn_sch.outer_photon_sphere_radius() - 3.0) < 1e-12);
}

void test_reissner_nordstrom_christoffel() {
	ReissnerNordstromMetric<double> metric(1.0, 0.5);
	FourVector<double> x(0.0, 5.0, 1.2, 0.4);

	const auto gamma_ana = metric.christoffel_symbols(x);
	const auto gamma_num = compute_christoffel_numerical<DerivativeOrder::EighthOrder>(metric, x);

	for (size_t sigma = 0; sigma < 4; ++sigma) {
		for (size_t mu = 0; mu < 4; ++mu) {
			for (size_t nu = 0; nu < 4; ++nu) {
				const double diff = std::abs(gamma_ana(sigma, mu, nu) - gamma_num(sigma, mu, nu));
				assert(diff < 1e-8);
			}
		}
	}
}

void test_kerr_newman_horizons_and_reduction() {
	KerrNewmanMetric<double> kn(1.0, 0.6, 0.4);
	assert(kn.is_subextremal());
	const double expected_r_plus = 1.0 + std::sqrt(1.0 - 0.36 - 0.16);
	assert(std::abs(kn.outer_horizon_radius() - expected_r_plus) < 1e-12);

	KerrNewmanMetric<double> kn_no_charge(1.0, 0.5, 0.0);
	FourVector<double> x(0.0, 4.0, 1.0, 0.0);
	const auto g_kn = kn_no_charge.metric_tensor(x);
	const auto inv_g_kn = kn_no_charge.inverse_metric(x);

	for (size_t i = 0; i < 4; ++i) {
		for (size_t j = 0; j < 4; ++j) {
			double delta_ij = 0.0;
			for (size_t k = 0; k < 4; ++k) {
				delta_ij += g_kn(i, k) * inv_g_kn(k, j);
			}
			const double target = (i == j) ? 1.0 : 0.0;
			assert(std::abs(delta_ij - target) < 1e-12);
		}
	}
}

void test_kerr_newman_christoffel() {
	KerrNewmanMetric<double> metric(1.0, 0.5, 0.3);
	FourVector<double> x(0.0, 3.5, 1.1, 0.7);

	const auto gamma_ana = metric.christoffel_symbols(x);
	const auto gamma_num = compute_christoffel_numerical<DerivativeOrder::EighthOrder>(metric, x);

	for (size_t sigma = 0; sigma < 4; ++sigma) {
		for (size_t mu = 0; mu < 4; ++mu) {
			for (size_t nu = 0; nu < 4; ++nu) {
				const double diff = std::abs(gamma_ana(sigma, mu, nu) - gamma_num(sigma, mu, nu));
				assert(diff < 1e-7);
			}
		}
	}
}

void test_schwarzschild_de_sitter() {
	const double lambda = 1e-5;
	SchwarzschildDeSitterMetric<double> sds(1.0, lambda);
	FourVector<double> x(0.0, 10.0, 1.3, 0.5);

	const auto g = sds.metric_tensor(x);
	const auto inv_g = sds.inverse_metric(x);

	for (size_t i = 0; i < 4; ++i) {
		for (size_t j = 0; j < 4; ++j) {
			double delta_ij = 0.0;
			for (size_t k = 0; k < 4; ++k) {
				delta_ij += g(i, k) * inv_g(k, j);
			}
			const double target = (i == j) ? 1.0 : 0.0;
			assert(std::abs(delta_ij - target) < 1e-12);
		}
	}

	const auto gamma_ana = sds.christoffel_symbols(x);
	const auto gamma_num = compute_christoffel_numerical<DerivativeOrder::EighthOrder>(sds, x);

	for (size_t sigma = 0; sigma < 4; ++sigma) {
		for (size_t mu = 0; mu < 4; ++mu) {
			for (size_t nu = 0; nu < 4; ++nu) {
				const double diff = std::abs(gamma_ana(sigma, mu, nu) - gamma_num(sigma, mu, nu));
				assert(diff < 1e-8);
			}
		}
	}
}

void test_kerr_de_sitter() {
	const double lambda = 1e-4;
	KerrDeSitterMetric<double> kds(1.0, 0.4, lambda);
	FourVector<double> x(0.0, 6.0, 1.2, 0.3);

	const auto g = kds.metric_tensor(x);
	const auto inv_g = kds.inverse_metric(x);

	for (size_t i = 0; i < 4; ++i) {
		for (size_t j = 0; j < 4; ++j) {
			double delta_ij = 0.0;
			for (size_t k = 0; k < 4; ++k) {
				delta_ij += g(i, k) * inv_g(k, j);
			}
			const double target = (i == j) ? 1.0 : 0.0;
			assert(std::abs(delta_ij - target) < 1e-11);
		}
	}

	const auto gamma_ana = kds.christoffel_symbols(x);
	const auto gamma_num = compute_christoffel_numerical<DerivativeOrder::EighthOrder>(kds, x);

	for (size_t sigma = 0; sigma < 4; ++sigma) {
		for (size_t mu = 0; mu < 4; ++mu) {
			for (size_t nu = 0; nu < 4; ++nu) {
				const double diff = std::abs(gamma_ana(sigma, mu, nu) - gamma_num(sigma, mu, nu));
				assert(diff < 1e-7);
			}
		}
	}
}

void test_flrw_expansion_and_redshift() {
	FLRWCosmologyConfig<double> cfg{
		.model_type = CosmologicalModelType::MatterDominated,
		.t0 = 1.0
	};
	FLRWMetric<double> flrw(cfg);

	const double a_1 = flrw.scale_factor(1.0);
	const double a_8 = flrw.scale_factor(8.0);
	assert(std::abs(a_1 - 1.0) < 1e-12);
	assert(std::abs(a_8 - 4.0) < 1e-12);

	const double z = flrw.cosmological_redshift(1.0, 8.0);
	assert(std::abs(z - 3.0) < 1e-12);

	FourVector<double> x(2.0, 0.5, 1.0, 0.2);
	const auto g = flrw.metric_tensor(x);
	const auto inv_g = flrw.inverse_metric(x);

	for (size_t i = 0; i < 4; ++i) {
		for (size_t j = 0; j < 4; ++j) {
			double delta_ij = 0.0;
			for (size_t k = 0; k < 4; ++k) {
				delta_ij += g(i, k) * inv_g(k, j);
			}
			const double target = (i == j) ? 1.0 : 0.0;
			assert(std::abs(delta_ij - target) < 1e-12);
		}
	}
}

int main() {
	test_reissner_nordstrom_horizons();
	test_reissner_nordstrom_christoffel();
	test_kerr_newman_horizons_and_reduction();
	test_kerr_newman_christoffel();
	test_schwarzschild_de_sitter();
	test_kerr_de_sitter();
	test_flrw_expansion_and_redshift();
	return 0;
}

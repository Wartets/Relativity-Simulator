#include "relativistic/metrics/morris_thorne.hpp"
#include "relativistic/metrics/alcubierre.hpp"
#include "relativistic/core/christoffel.hpp"
#include "relativistic/integrators/rk45_adaptive.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

using namespace Relativistic;
using namespace Relativistic::Core;
using namespace Relativistic::Metrics;
using namespace Relativistic::Integrators;

void test_morris_thorne_throat_crossing() {
	const double b0 = 2.0;
	MorrisThorneWormholeMetric<double> wh(b0, 0.0);

	FourVector<double> x_throat(0.0, 0.0, std::numbers::pi / 2.0, 0.0);
	const auto g_throat = wh.metric_tensor(x_throat);
	assert(std::abs(g_throat(0, 0) - (-1.0)) < 1e-12);
	assert(std::abs(g_throat(1, 1) - 1.0) < 1e-12);
	assert(std::abs(g_throat(2, 2) - 4.0) < 1e-12);
	assert(std::abs(g_throat(3, 3) - 4.0) < 1e-12);

	FourVector<double> x_init(0.0, -10.0, std::numbers::pi / 2.0, 0.0);
	FourVector<double> u_init(1.0, 1.0, 0.0, 0.0);

	GeodesicState<double> state{
		.x = x_init,
		.u = u_init
	};

	RK45Config<double> config{
		.initial_step = 0.05,
		.min_step = 1e-6,
		.max_step = 0.2,
		.rtol = 1e-11,
		.atol = 1e-13,
		.invariant_tolerance = 1e-10
	};

	RK45AdaptiveIntegrator<MorrisThorneWormholeMetric<double>, double> integrator(wh, GeodesicType::Null, config);

	double dt = 0.05;
	bool crossed_throat = false;
	double min_abs_l = 100.0;

	for (size_t step_idx = 0; step_idx < 500; ++step_idx) {
		const auto res = integrator.step(state, dt);
		assert(res.has_value());

		const double l_cur = state.x(1);
		if (std::abs(l_cur) < min_abs_l) {
			min_abs_l = std::abs(l_cur);
		}
		if (l_cur > 0.0) {
			crossed_throat = true;
		}
		if (l_cur > 10.0) {
			break;
		}
	}

	assert(crossed_throat);
	assert(min_abs_l < 0.2);
	assert(state.x(1) >= 10.0);
}

void test_morris_thorne_christoffel() {
	MorrisThorneWormholeMetric<double> wh(2.5, 0.5);
	FourVector<double> x(0.0, 1.5, 1.2, 0.8);

	const auto gamma_ana = wh.christoffel_symbols(x);
	const auto gamma_num = compute_christoffel_numerical<DerivativeOrder::EighthOrder>(wh, x);

	for (size_t sigma = 0; sigma < 4; ++sigma) {
		for (size_t mu = 0; mu < 4; ++mu) {
			for (size_t nu = 0; nu < 4; ++nu) {
				const double diff = std::abs(gamma_ana(sigma, mu, nu) - gamma_num(sigma, mu, nu));
				assert(diff < 1e-8);
			}
		}
	}
}

void test_alcubierre_metric_properties() {
	const double vs = 2.0;
	const double R = 50.0;
	const double sigma = 0.2;
	AlcubierreWarpMetric<double> warp(vs, R, sigma);

	FourVector<double> x_center(0.0, 0.0, 0.0, 0.0);
	const double f_center = warp.shaping_function(warp.radial_distance_from_center(x_center));
	assert(std::abs(f_center - 1.0) < 1e-6);

	FourVector<double> x_distant(0.0, 500.0, 0.0, 0.0);
	const double f_distant = warp.shaping_function(warp.radial_distance_from_center(x_distant));
	assert(f_distant < 1e-6);

	const auto g_center = warp.metric_tensor(x_center);
	const auto inv_g_center = warp.inverse_metric(x_center);

	for (size_t i = 0; i < 4; ++i) {
		for (size_t j = 0; j < 4; ++j) {
			double delta_ij = 0.0;
			for (size_t k = 0; k < 4; ++k) {
				delta_ij += g_center(i, k) * inv_g_center(k, j);
			}
			const double target = (i == j) ? 1.0 : 0.0;
			assert(std::abs(delta_ij - target) < 1e-12);
		}
	}

	FourVector<double> x_wall(0.0, 50.0, 0.0, 0.0);
	const auto gamma_ana = warp.christoffel_symbols(x_wall);
	const auto gamma_num = compute_christoffel_numerical<DerivativeOrder::EighthOrder>(warp, x_wall);

	for (size_t s = 0; s < 4; ++s) {
		for (size_t mu = 0; mu < 4; ++mu) {
			for (size_t nu = 0; nu < 4; ++nu) {
				const double diff = std::abs(gamma_ana(s, mu, nu) - gamma_num(s, mu, nu));
				assert(diff < 1e-7);
			}
		}
	}
}

int main() {
	test_morris_thorne_throat_crossing();
	test_morris_thorne_christoffel();
	test_alcubierre_metric_properties();
	return 0;
}

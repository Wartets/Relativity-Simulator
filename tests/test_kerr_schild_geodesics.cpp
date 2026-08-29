#include "relativistic/metrics/kerr_schild.hpp"
#include "relativistic/integrators/rk45_adaptive.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

using namespace Relativistic::Core;
using namespace Relativistic::Metrics;
using namespace Relativistic::Integrators;

void test_polar_axis_crossing() {
	const double m = 1.0;
	const double a = 0.8;
	const double c = 1.0;
	const double g_const = 1.0;

	const KerrSchildMetric<double> ks(m, a, c, g_const);

	RK45Config<double> cfg;
	cfg.initial_step = 0.01;
	cfg.min_step = 1e-10;
	cfg.max_step = 0.05;
	cfg.rtol = 1e-11;
	cfg.atol = 1e-13;
	cfg.crossing_mode = HorizonCrossingMode::Continuity;

	RK45AdaptiveIntegrator<KerrSchildMetric<double>, double> integrator(ks, GeodesicType::Timelike, cfg);

	GeodesicState<double> state;
	state.x = FourVector<double>(0.0, -3.0, 0.0, 6.0);

	const auto g_cov = ks.metric_tensor(state.x);
	const double ux = 0.5;
	const double uy = 0.0;
	const double uz = -0.1;

	const double A = g_cov(0, 0);
	const double B = 2.0 * (g_cov(0, 1) * ux + g_cov(0, 2) * uy + g_cov(0, 3) * uz);
	const double C = 1.0 + g_cov(1, 1) * ux * ux + g_cov(2, 2) * uy * uy + g_cov(3, 3) * uz * uz + 2.0 * (g_cov(1, 2) * ux * uy + g_cov(1, 3) * ux * uz + g_cov(2, 3) * uy * uz);
	const double discr = B * B - 4.0 * A * C;
	assert(discr >= 0.0);
	const double ut = (-B - std::sqrt(discr)) / (2.0 * A);

	state.u = FourVector<double>(ut, ux, uy, uz);

	double dt = cfg.initial_step;
	for (size_t i = 0; i < 2000; ++i) {
		const auto res = integrator.step(state, dt);
		assert(res.has_value());
		assert(!std::isnan(state.x(1)));
		assert(!std::isnan(state.x(2)));
		assert(!std::isnan(state.x(3)));
	}
}

void test_horizon_infall_kerr_schild() {
	const double m = 1.0;
	const double a = 0.7;
	const double c = 1.0;
	const double g_const = 1.0;

	const KerrSchildMetric<double> ks(m, a, c, g_const);

	RK45Config<double> cfg;
	cfg.initial_step = 0.005;
	cfg.min_step = 1e-11;
	cfg.max_step = 0.02;
	cfg.rtol = 1e-11;
	cfg.atol = 1e-13;
	cfg.crossing_mode = HorizonCrossingMode::Continuity;

	RK45AdaptiveIntegrator<KerrSchildMetric<double>, double> integrator(ks, GeodesicType::Timelike, cfg);

	GeodesicState<double> state;
	state.x = FourVector<double>(0.0, 4.0, 0.0, 0.0);

	const auto g_cov = ks.metric_tensor(state.x);
	const double ux = -0.4;
	const double uy = 0.0;
	const double uz = 0.0;

	const double A = g_cov(0, 0);
	const double B = 2.0 * (g_cov(0, 1) * ux + g_cov(0, 2) * uy + g_cov(0, 3) * uz);
	const double C = 1.0 + g_cov(1, 1) * ux * ux;
	const double discr = B * B - 4.0 * A * C;
	assert(discr >= 0.0);
	const double ut = (-B - std::sqrt(discr)) / (2.0 * A);

	state.u = FourVector<double>(ut, ux, uy, uz);

	double dt = cfg.initial_step;
	const double r_plus = ks.outer_horizon_radius();
	bool crossed_r_plus = false;

	for (size_t i = 0; i < 5000; ++i) {
		const auto res = integrator.step(state, dt);
		assert(res.has_value());
		const double current_r = ks.boyer_lindquist_r(state.x);
		if (current_r < r_plus) {
			crossed_r_plus = true;
			break;
		}
	}

	assert(crossed_r_plus);
}

int main() {
	test_polar_axis_crossing();
	test_horizon_infall_kerr_schild();
	return 0;
}

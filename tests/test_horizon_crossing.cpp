#include "relativistic/metrics/painleve_gullstrand.hpp"
#include "relativistic/metrics/eddington_finkelstein.hpp"
#include "relativistic/core/christoffel.hpp"
#include "relativistic/integrators/rk45_adaptive.hpp"
#include "relativistic/core/constants.hpp"
#include <iostream>
#include <cassert>
#include <cmath>
#include <vector>

void test_painleve_gullstrand_regularity() {
	using namespace Relativistic::Metrics;
	using namespace Relativistic::Core;

	constexpr double mass = 1.0;
	constexpr double c = 1.0;
	constexpr double G = 1.0;
	const PainleveGullstrandMetric<double> metric(mass, c, G);
	const double r_s = metric.schwarzschild_radius();
	assert(std::abs(r_s - 2.0) < 1e-15);

	const std::vector<double> test_radii = {0.1 * r_s, 0.5 * r_s, 0.99 * r_s, 1.0 * r_s, 1.01 * r_s, 2.0 * r_s, 5.0 * r_s, 10.0 * r_s};

	for (const double r : test_radii) {
		FourVector<double> x{0.0, r, std::numbers::pi / 3.0, 0.5};

		const auto g = metric.metric_tensor(x);
		const auto inv_g = metric.inverse_metric(x);
		const auto identity = matrix_multiply(inv_g, g);

		for (size_t i = 0; i < 4; ++i) {
			for (size_t j = 0; j < 4; ++j) {
				const double expected = (i == j) ? 1.0 : 0.0;
				const double diff = std::abs(identity(i, j) - expected);
				assert(diff < 1e-12);
			}
		}

		const auto gamma_analytic = metric.christoffel_symbols(x);
		const auto gamma_numerical = compute_christoffel_numerical<DerivativeOrder::EighthOrder>(metric, x);

		for (size_t sigma = 0; sigma < 4; ++sigma) {
			for (size_t mu = 0; mu < 4; ++mu) {
				for (size_t nu = 0; nu < 4; ++nu) {
					const double val_a = gamma_analytic(sigma, mu, nu);
					const double val_n = gamma_numerical(sigma, mu, nu);
					assert(!std::isnan(val_a) && !std::isinf(val_a));
					assert(!std::isnan(val_n) && !std::isinf(val_n));
					const double diff = std::abs(val_a - val_n);
					const double scale = std::max({1.0, std::abs(val_a), std::abs(val_n)});
					assert((diff / scale) < 1e-6);
				}
			}
		}
	}
}

void test_eddington_finkelstein_regularity() {
	using namespace Relativistic::Metrics;
	using namespace Relativistic::Core;

	constexpr double mass = 1.0;
	constexpr double c = 1.0;
	constexpr double G = 1.0;
	const EddingtonFinkelsteinMetric<double> metric(mass, c, G);
	const double r_s = metric.schwarzschild_radius();
	assert(std::abs(r_s - 2.0) < 1e-15);

	const std::vector<double> test_radii = {0.1 * r_s, 0.5 * r_s, 0.99 * r_s, 1.0 * r_s, 1.01 * r_s, 2.0 * r_s, 5.0 * r_s, 10.0 * r_s};

	for (const double r : test_radii) {
		FourVector<double> x{0.0, r, std::numbers::pi / 4.0, 1.2};

		const auto g = metric.metric_tensor(x);
		const auto inv_g = metric.inverse_metric(x);
		const auto identity = matrix_multiply(inv_g, g);

		for (size_t i = 0; i < 4; ++i) {
			for (size_t j = 0; j < 4; ++j) {
				const double expected = (i == j) ? 1.0 : 0.0;
				const double diff = std::abs(identity(i, j) - expected);
				assert(diff < 1e-12);
			}
		}

		const auto gamma_analytic = metric.christoffel_symbols(x);
		const auto gamma_numerical = compute_christoffel_numerical<DerivativeOrder::EighthOrder>(metric, x);

		for (size_t sigma = 0; sigma < 4; ++sigma) {
			for (size_t mu = 0; mu < 4; ++mu) {
				for (size_t nu = 0; nu < 4; ++nu) {
					const double val_a = gamma_analytic(sigma, mu, nu);
					const double val_n = gamma_numerical(sigma, mu, nu);
					assert(!std::isnan(val_a) && !std::isinf(val_a));
					assert(!std::isnan(val_n) && !std::isinf(val_n));
					const double diff = std::abs(val_a - val_n);
					const double scale = std::max({1.0, std::abs(val_a), std::abs(val_n)});
					assert((diff / scale) < 1e-6);
				}
			}
		}
	}
}

void test_painleve_gullstrand_freefall_timelike() {
	using namespace Relativistic::Metrics;
	using namespace Relativistic::Integrators;
	using namespace Relativistic::Core;

	constexpr double mass = 1.0;
	constexpr double c = 1.0;
	constexpr double G = 1.0;
	const PainleveGullstrandMetric<double> metric(mass, c, G);
	const double r_s = metric.schwarzschild_radius();

	RK45Config<double> config;
	config.initial_step = 0.05;
	config.min_step = 1e-9;
	config.max_step = 0.5;
	config.rtol = 1e-11;
	config.atol = 1e-14;
	config.crossing_mode = HorizonCrossingMode::Continuity;
	config.singularity_threshold = 0.01;

	RK45AdaptiveIntegrator<PainleveGullstrandMetric<double>, double> integrator(metric, GeodesicType::Timelike, config);

	const double r0 = 4.0 * r_s;
	const double u_r0 = -c * std::sqrt(r_s / r0);

	GeodesicState<double> state{
		.x = FourVector<double>{0.0, r0, std::numbers::pi / 2.0, 0.0},
		.u = FourVector<double>{1.0, u_r0, 0.0, 0.0}
	};

	double dt = config.initial_step;
	bool crossed_r_s = false;
	size_t step_count = 0;

	while (state.x(1) > 0.05 * r_s && step_count < 20000) {
		const double r_before = state.x(1);
		const auto step_res = integrator.step(state, dt);
		assert(step_res.has_value());
		++step_count;

		const double r_after = state.x(1);
		if (r_before > r_s && r_after <= r_s) {
			crossed_r_s = true;
		}

		assert(!std::isnan(state.x(0)) && !std::isnan(state.x(1)));
		assert(!std::isnan(state.u(0)) && !std::isnan(state.u(1)));

		assert(std::abs(state.u(0) - 1.0) < 1e-6);

		const auto g = metric.metric_tensor(state.x);
		const double norm_sq = g(0, 0) * state.u(0) * state.u(0) +
		                       2.0 * g(0, 1) * state.u(0) * state.u(1) +
		                       g(1, 1) * state.u(1) * state.u(1);
		const double invariant_error = std::abs(norm_sq - (-c * c));
		assert(invariant_error < 1e-8);
	}

	assert(crossed_r_s);
	assert(integrator.statistics().crossed_horizon);
	assert(state.x(1) < 0.1 * r_s);
}

void test_eddington_finkelstein_infalling_photon() {
	using namespace Relativistic::Metrics;
	using namespace Relativistic::Integrators;
	using namespace Relativistic::Core;

	constexpr double mass = 1.0;
	constexpr double c = 1.0;
	constexpr double G = 1.0;
	const EddingtonFinkelsteinMetric<double> metric(mass, c, G);
	const double r_s = metric.schwarzschild_radius();

	RK45Config<double> config;
	config.initial_step = 0.1;
	config.min_step = 1e-9;
	config.max_step = 0.5;
	config.rtol = 1e-11;
	config.atol = 1e-14;
	config.crossing_mode = HorizonCrossingMode::Continuity;
	config.singularity_threshold = 0.01;

	RK45AdaptiveIntegrator<EddingtonFinkelsteinMetric<double>, double> integrator(metric, GeodesicType::Null, config);

	const double r0 = 5.0 * r_s;
	const double v0 = 10.0;

	GeodesicState<double> state{
		.x = FourVector<double>{v0, r0, std::numbers::pi / 2.0, 0.0},
		.u = FourVector<double>{0.0, -1.0, 0.0, 0.0}
	};

	double dt = config.initial_step;
	bool crossed_r_s = false;
	size_t step_count = 0;

	while (state.x(1) > 0.05 * r_s && step_count < 20000) {
		const double r_before = state.x(1);
		const auto step_res = integrator.step(state, dt);
		assert(step_res.has_value());
		++step_count;

		const double r_after = state.x(1);
		if (r_before > r_s && r_after <= r_s) {
			crossed_r_s = true;
		}

		assert(std::abs(state.x(0) - v0) < 1e-8);
		assert(std::abs(state.u(0)) < 1e-8);
		assert(std::abs(state.u(1) - (-1.0)) < 1e-8);

		const auto g = metric.metric_tensor(state.x);
		const double norm_sq = g(0, 0) * state.u(0) * state.u(0) +
		                       2.0 * g(0, 1) * state.u(0) * state.u(1) +
		                       g(1, 1) * state.u(1) * state.u(1);
		assert(std::abs(norm_sq) < 1e-12);
	}

	assert(crossed_r_s);
	assert(integrator.statistics().crossed_horizon);
	assert(state.x(1) < 0.1 * r_s);
}

void test_horizon_absorption_mode() {
	using namespace Relativistic::Metrics;
	using namespace Relativistic::Integrators;
	using namespace Relativistic::Core;

	constexpr double mass = 1.0;
	constexpr double c = 1.0;
	constexpr double G = 1.0;
	const PainleveGullstrandMetric<double> metric(mass, c, G);
	const double r_s = metric.schwarzschild_radius();

	RK45Config<double> config;
	config.initial_step = 0.05;
	config.min_step = 1e-9;
	config.max_step = 0.2;
	config.crossing_mode = HorizonCrossingMode::Absorption;

	RK45AdaptiveIntegrator<PainleveGullstrandMetric<double>, double> integrator(metric, GeodesicType::Timelike, config);

	const double r0 = 2.0 * r_s;
	const double u_r0 = -c * std::sqrt(r_s / r0);

	GeodesicState<double> state{
		.x = FourVector<double>{0.0, r0, std::numbers::pi / 2.0, 0.0},
		.u = FourVector<double>{1.0, u_r0, 0.0, 0.0}
	};

	double dt = config.initial_step;
	size_t step_count = 0;

	for (;;) {
		const auto res = integrator.step(state, dt);
		if (!res.has_value()) {
			break;
		}
		++step_count;
		assert(step_count < 10000);
	}

	assert(integrator.statistics().crossed_horizon);
	assert(integrator.statistics().absorbed_at_horizon);
	assert(!integrator.statistics().reached_singularity);
	assert(state.x(1) <= r_s);
}

void test_horizon_continuity_to_singularity() {
	using namespace Relativistic::Metrics;
	using namespace Relativistic::Integrators;
	using namespace Relativistic::Core;

	constexpr double mass = 1.0;
	constexpr double c = 1.0;
	constexpr double G = 1.0;
	const PainleveGullstrandMetric<double> metric(mass, c, G);
	const double r_s = metric.schwarzschild_radius();

	RK45Config<double> config;
	config.initial_step = 0.02;
	config.min_step = 1e-9;
	config.max_step = 0.1;
	config.crossing_mode = HorizonCrossingMode::Continuity;
	config.singularity_threshold = 0.005;

	RK45AdaptiveIntegrator<PainleveGullstrandMetric<double>, double> integrator(metric, GeodesicType::Timelike, config);

	const double r0 = 1.5 * r_s;
	const double u_r0 = -c * std::sqrt(r_s / r0);

	GeodesicState<double> state{
		.x = FourVector<double>{0.0, r0, std::numbers::pi / 2.0, 0.0},
		.u = FourVector<double>{1.0, u_r0, 0.0, 0.0}
	};

	double dt = config.initial_step;
	size_t step_count = 0;

	for (;;) {
		const auto res = integrator.step(state, dt);
		if (!res.has_value()) {
			break;
		}
		++step_count;
		assert(step_count < 20000);
	}

	assert(integrator.statistics().crossed_horizon);
	assert(!integrator.statistics().absorbed_at_horizon);
	assert(integrator.statistics().reached_singularity);
	assert(state.x(1) <= config.singularity_threshold * r_s);
}

void test_interior_trapped_surface() {
	using namespace Relativistic::Metrics;
	using namespace Relativistic::Integrators;
	using namespace Relativistic::Core;

	constexpr double mass = 1.0;
	constexpr double c = 1.0;
	constexpr double G = 1.0;
	const EddingtonFinkelsteinMetric<double> metric(mass, c, G);
	const double r_s = metric.schwarzschild_radius();

	RK45Config<double> config;
	config.initial_step = 0.01;
	config.min_step = 1e-9;
	config.max_step = 0.05;
	config.crossing_mode = HorizonCrossingMode::Continuity;
	config.singularity_threshold = 0.005;

	RK45AdaptiveIntegrator<EddingtonFinkelsteinMetric<double>, double> integrator(metric, GeodesicType::Null, config);

	const double r0 = 0.5 * r_s;
	const double v0 = 0.0;
	const double u_v = 1.0;
	const double factor = 1.0 - r_s / r0;
	const double u_r = 0.5 * c * factor * u_v;

	GeodesicState<double> state{
		.x = FourVector<double>{v0, r0, std::numbers::pi / 2.0, 0.0},
		.u = FourVector<double>{u_v, u_r, 0.0, 0.0}
	};

	assert(u_r < 0.0);

	double dt = config.initial_step;
	size_t step_count = 0;

	while (state.x(1) > 0.01 * r_s && step_count < 10000) {
		const auto res = integrator.step(state, dt);
		assert(res.has_value());
		++step_count;
		assert(state.x(1) < r_s);
	}

	assert(state.x(1) <= 0.01 * r_s);
}

int main() {
	test_painleve_gullstrand_regularity();
	test_eddington_finkelstein_regularity();
	test_painleve_gullstrand_freefall_timelike();
	test_eddington_finkelstein_infalling_photon();
	test_horizon_absorption_mode();
	test_horizon_continuity_to_singularity();
	test_interior_trapped_surface();
	return 0;
}

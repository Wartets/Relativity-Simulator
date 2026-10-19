#include "support/test_harness.hpp"
#include "support/metric_checks.hpp"
#include "relativistic/metrics/painleve_gullstrand.hpp"
#include "relativistic/metrics/eddington_finkelstein.hpp"
#include "relativistic/core/math/christoffel.hpp"
#include "relativistic/integrators/rk45_adaptive.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <string>

using namespace Relativistic::Metrics;
using namespace Relativistic::Integrators;
using namespace Relativistic::Core;
using Relativistic::Testing::TestSuite;
using Relativistic::Testing::connection_deviation;
using Relativistic::Testing::inversion_residual;

namespace {

constexpr double c = 1.0;
constexpr double g_constant = 1.0;
constexpr double mass = 1.0;

template <typename MetricType>
void check_regularity(TestSuite& suite, const std::string& name, const MetricType& metric, double theta, double phi) {
	suite.section(name + " regularity across the horizon");
	const double r_s = metric.schwarzschild_radius();
	suite.check_near(r_s, 2.0, 1e-15, name + ": the Schwarzschild radius equals 2 M in geometric units");
	constexpr std::array<double, 8> factors{0.1, 0.5, 0.99, 1.0, 1.01, 2.0, 5.0, 10.0};
	double worst_inversion = 0.0;
	double worst_connection = 0.0;
	bool finite = true;
	for (const double factor : factors) {
		const FourVector<double> x{0.0, factor * r_s, theta, phi};
		worst_inversion = std::max(worst_inversion, inversion_residual(metric.metric_tensor(x), metric.inverse_metric(x)));
		const auto analytic = metric.christoffel_symbols(x);
		const auto numerical = compute_christoffel_numerical<DerivativeOrder::EighthOrder>(metric, x);
		for (size_t sigma = 0; sigma < 4; ++sigma) {
			for (size_t mu = 0; mu < 4; ++mu) {
				for (size_t nu = 0; nu < 4; ++nu) {
					finite = finite && std::isfinite(analytic(sigma, mu, nu)) && std::isfinite(numerical(sigma, mu, nu));
				}
			}
		}
		worst_connection = std::max(worst_connection, connection_deviation(analytic, numerical).relative);
	}
	suite.check_at_most(worst_inversion, 1e-12, name + ": the inverse metric reproduces the identity at every sampled radius");
	suite.check(finite, name + ": the analytic and numerical connections are finite at every sampled radius");
	suite.check_at_most(worst_connection, 1e-6, name + ": the analytic connection matches eighth-order finite differences relative to the largest symbol of its row");
}

RK45Config<double> continuity_config(double initial, double maximum, double threshold) {
	RK45Config<double> config;
	config.initial_step = initial;
	config.min_step = 1e-9;
	config.max_step = maximum;
	config.rtol = 1e-11;
	config.atol = 1e-14;
	config.crossing_mode = HorizonCrossingMode::Continuity;
	config.singularity_threshold = threshold;
	return config;
}

void check_painleve_freefall(TestSuite& suite) {
	suite.section("Painleve-Gullstrand timelike free fall");
	const PainleveGullstrandMetric<double> metric(mass, c, g_constant);
	const double r_s = metric.schwarzschild_radius();
	RK45AdaptiveIntegrator<PainleveGullstrandMetric<double>, double> integrator(metric, GeodesicType::Timelike, continuity_config(0.05, 0.5, 0.01));
	const double r0 = 4.0 * r_s;
	GeodesicState<double> state{
		.x = FourVector<double>{0.0, r0, std::numbers::pi / 2.0, 0.0},
		.u = FourVector<double>{1.0, -c * std::sqrt(r_s / r0), 0.0, 0.0}
	};
	double dt = 0.05;
	bool steps_succeeded = true;
	bool finite = true;
	bool crossed = false;
	double worst_time_velocity = 0.0;
	double worst_invariant = 0.0;
	size_t step_count = 0;
	while (state.x(1) > 0.05 * r_s && step_count < 20000) {
		const double before = state.x(1);
		if (!integrator.step(state, dt).has_value()) {
			steps_succeeded = false;
			break;
		}
		++step_count;
		if (before > r_s && state.x(1) <= r_s) {
			crossed = true;
		}
		finite = finite && std::isfinite(state.x(0)) && std::isfinite(state.x(1)) && std::isfinite(state.u(0)) && std::isfinite(state.u(1));
		worst_time_velocity = std::max(worst_time_velocity, std::abs(state.u(0) - 1.0));
		const auto g = metric.metric_tensor(state.x);
		const double norm = g(0, 0) * state.u(0) * state.u(0) + 2.0 * g(0, 1) * state.u(0) * state.u(1) + g(1, 1) * state.u(1) * state.u(1);
		worst_invariant = std::max(worst_invariant, std::abs(norm + c * c));
	}
	suite.check(steps_succeeded, "every adaptive step of the free fall succeeds");
	suite.check(finite, "the state stays finite throughout the fall");
	suite.check(crossed, "the particle crosses the Schwarzschild radius");
	suite.check(integrator.statistics().crossed_horizon, "the integrator statistics report the horizon crossing");
	suite.check(state.x(1) < 0.1 * r_s, "the particle reaches the deep interior");
	suite.check_at_most(worst_time_velocity, 1e-6, "the temporal velocity of a free fall from rest stays equal to 1");
	suite.check_at_most(worst_invariant, 1e-8, "the four-velocity normalization stays equal to -c^2");
}

void check_eddington_photon(TestSuite& suite) {
	suite.section("Eddington-Finkelstein infalling photon");
	const EddingtonFinkelsteinMetric<double> metric(mass, c, g_constant);
	const double r_s = metric.schwarzschild_radius();
	RK45AdaptiveIntegrator<EddingtonFinkelsteinMetric<double>, double> integrator(metric, GeodesicType::Null, continuity_config(0.1, 0.5, 0.01));
	const double advanced_time = 10.0;
	GeodesicState<double> state{
		.x = FourVector<double>{advanced_time, 5.0 * r_s, std::numbers::pi / 2.0, 0.0},
		.u = FourVector<double>{0.0, -1.0, 0.0, 0.0}
	};
	double dt = 0.1;
	bool steps_succeeded = true;
	bool crossed = false;
	double worst_time = 0.0;
	double worst_time_velocity = 0.0;
	double worst_radial_velocity = 0.0;
	double worst_null = 0.0;
	size_t step_count = 0;
	while (state.x(1) > 0.05 * r_s && step_count < 20000) {
		const double before = state.x(1);
		if (!integrator.step(state, dt).has_value()) {
			steps_succeeded = false;
			break;
		}
		++step_count;
		if (before > r_s && state.x(1) <= r_s) {
			crossed = true;
		}
		worst_time = std::max(worst_time, std::abs(state.x(0) - advanced_time));
		worst_time_velocity = std::max(worst_time_velocity, std::abs(state.u(0)));
		worst_radial_velocity = std::max(worst_radial_velocity, std::abs(state.u(1) + 1.0));
		const auto g = metric.metric_tensor(state.x);
		worst_null = std::max(worst_null, std::abs(g(0, 0) * state.u(0) * state.u(0) + 2.0 * g(0, 1) * state.u(0) * state.u(1) + g(1, 1) * state.u(1) * state.u(1)));
	}
	suite.check(steps_succeeded, "every adaptive step of the photon succeeds");
	suite.check(crossed, "the photon crosses the Schwarzschild radius");
	suite.check(integrator.statistics().crossed_horizon, "the integrator statistics report the horizon crossing");
	suite.check(state.x(1) < 0.1 * r_s, "the photon reaches the deep interior");
	suite.check_at_most(worst_time, 1e-8, "the advanced time of an ingoing null ray stays constant");
	suite.check_at_most(worst_time_velocity, 1e-8, "the advanced-time velocity of an ingoing null ray stays zero");
	suite.check_at_most(worst_radial_velocity, 1e-8, "the radial velocity of an ingoing null ray stays equal to -1");
	suite.check_at_most(worst_null, 1e-12, "the null constraint holds along the ray");
}

template <typename MetricType>
size_t run_until_stop(RK45AdaptiveIntegrator<MetricType, double>& integrator, GeodesicState<double>& state, double step, size_t limit, bool& within_limit) {
	size_t count = 0;
	within_limit = true;
	double dt = step;
	for (;;) {
		if (!integrator.step(state, dt).has_value()) {
			break;
		}
		++count;
		if (count >= limit) {
			within_limit = false;
			break;
		}
	}
	return count;
}

void check_boundary_modes(TestSuite& suite) {
	suite.section("Horizon boundary modes");
	const PainleveGullstrandMetric<double> metric(mass, c, g_constant);
	const double r_s = metric.schwarzschild_radius();

	RK45Config<double> absorption;
	absorption.initial_step = 0.05;
	absorption.min_step = 1e-9;
	absorption.max_step = 0.2;
	absorption.crossing_mode = HorizonCrossingMode::Absorption;
	RK45AdaptiveIntegrator<PainleveGullstrandMetric<double>, double> absorbing(metric, GeodesicType::Timelike, absorption);
	const double r_absorb = 2.0 * r_s;
	GeodesicState<double> absorbed{.x = FourVector<double>{0.0, r_absorb, std::numbers::pi / 2.0, 0.0}, .u = FourVector<double>{1.0, -c * std::sqrt(r_s / r_absorb), 0.0, 0.0}};
	bool within = true;
	run_until_stop(absorbing, absorbed, absorption.initial_step, 10000, within);
	suite.check(within, "the absorbing integration terminates within the step budget");
	suite.check(absorbing.statistics().crossed_horizon && absorbing.statistics().absorbed_at_horizon, "absorption mode reports the crossing and the absorption");
	suite.check(!absorbing.statistics().reached_singularity, "absorption mode does not reach the singularity");
	suite.check(absorbed.x(1) <= r_s, "the absorbed particle ends at or inside the horizon");

	const auto continuity = continuity_config(0.02, 0.1, 0.005);
	RK45AdaptiveIntegrator<PainleveGullstrandMetric<double>, double> continuing(metric, GeodesicType::Timelike, continuity);
	const double r_continue = 1.5 * r_s;
	GeodesicState<double> continued{.x = FourVector<double>{0.0, r_continue, std::numbers::pi / 2.0, 0.0}, .u = FourVector<double>{1.0, -c * std::sqrt(r_s / r_continue), 0.0, 0.0}};
	run_until_stop(continuing, continued, continuity.initial_step, 20000, within);
	suite.check(within, "the continuing integration terminates within the step budget");
	suite.check(continuing.statistics().crossed_horizon && !continuing.statistics().absorbed_at_horizon, "continuity mode crosses the horizon without absorption");
	suite.check(continuing.statistics().reached_singularity, "continuity mode reaches the singularity threshold");
	suite.check(continued.x(1) <= continuity.singularity_threshold * r_s, "the particle ends inside the singularity threshold");
}

void check_trapped_surface(TestSuite& suite) {
	suite.section("Interior trapped region");
	const EddingtonFinkelsteinMetric<double> metric(mass, c, g_constant);
	const double r_s = metric.schwarzschild_radius();
	auto config = continuity_config(0.01, 0.05, 0.005);
	RK45AdaptiveIntegrator<EddingtonFinkelsteinMetric<double>, double> integrator(metric, GeodesicType::Null, config);
	const double r0 = 0.5 * r_s;
	const double u_v = 1.0;
	const double u_r = 0.5 * c * (1.0 - r_s / r0) * u_v;
	suite.check(u_r < 0.0, "the initial radial velocity inside the horizon points inward");
	GeodesicState<double> state{.x = FourVector<double>{0.0, r0, std::numbers::pi / 2.0, 0.0}, .u = FourVector<double>{u_v, u_r, 0.0, 0.0}};
	double dt = config.initial_step;
	bool steps_succeeded = true;
	bool always_inside = true;
	size_t step_count = 0;
	while (state.x(1) > 0.01 * r_s && step_count < 10000) {
		if (!integrator.step(state, dt).has_value()) {
			steps_succeeded = false;
			break;
		}
		++step_count;
		always_inside = always_inside && state.x(1) < r_s;
	}
	suite.check(steps_succeeded, "every adaptive step inside the horizon succeeds");
	suite.check(always_inside, "the radius never exceeds the horizon while inside the trapped region");
	suite.check(state.x(1) <= 0.01 * r_s, "the ray reaches the singularity threshold");
}

}

int main() {
	TestSuite suite(
		"horizon_crossing",
		"Verifies horizon-regular spacetimes and horizon handling: Painleve-Gullstrand and Eddington-Finkelstein metrics and connections stay regular from 0.1 to 10 Schwarzschild radii, a free-falling particle and an ingoing photon cross the horizon with preserved invariants, absorption and continuity boundary modes behave as configured, and rays inside the horizon cannot escape."
	);
	check_regularity(suite, "Painleve-Gullstrand", PainleveGullstrandMetric<double>(mass, c, g_constant), std::numbers::pi / 3.0, 0.5);
	check_regularity(suite, "Eddington-Finkelstein", EddingtonFinkelsteinMetric<double>(mass, c, g_constant), std::numbers::pi / 4.0, 1.2);
	check_painleve_freefall(suite);
	check_eddington_photon(suite);
	check_boundary_modes(suite);
	check_trapped_surface(suite);
	return suite.finish();
}

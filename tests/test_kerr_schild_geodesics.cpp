#include "support/test_harness.hpp"
#include "relativistic/metrics/kerr_schild.hpp"
#include "relativistic/integrators/rk45_adaptive.hpp"
#include <cmath>

using namespace Relativistic::Core;
using namespace Relativistic::Metrics;
using namespace Relativistic::Integrators;
using Relativistic::Testing::TestSuite;

namespace {

void check_polar_axis_crossing(TestSuite& suite) {
	suite.section("Polar axis crossing");
	const KerrSchildMetric<double> metric(1.0, 0.8, 1.0, 1.0);
	RK45Config<double> cfg;
	cfg.initial_step = 0.01;
	cfg.min_step = 1e-10;
	cfg.max_step = 0.05;
	cfg.rtol = 1e-11;
	cfg.atol = 1e-13;
	cfg.crossing_mode = HorizonCrossingMode::Continuity;
	RK45AdaptiveIntegrator<KerrSchildMetric<double>, double> integrator(metric, GeodesicType::Timelike, cfg);

	GeodesicState<double> state;
	state.x = FourVector<double>(0.0, -3.0, 0.0, 6.0);
	const auto g = metric.metric_tensor(state.x);
	const double ux = 0.5;
	const double uy = 0.0;
	const double uz = -0.1;
	const double quad_a = g(0, 0);
	const double quad_b = 2.0 * (g(0, 1) * ux + g(0, 2) * uy + g(0, 3) * uz);
	const double quad_c = 1.0 + g(1, 1) * ux * ux + g(2, 2) * uy * uy + g(3, 3) * uz * uz + 2.0 * (g(1, 2) * ux * uy + g(1, 3) * ux * uz + g(2, 3) * uy * uz);
	const double discriminant = quad_b * quad_b - 4.0 * quad_a * quad_c;
	suite.check(discriminant >= 0.0, "the timelike normalization admits a real temporal velocity near the axis");
	if (discriminant < 0.0) {
		return;
	}
	state.u = FourVector<double>((-quad_b - std::sqrt(discriminant)) / (2.0 * quad_a), ux, uy, uz);

	bool steps_succeeded = true;
	bool finite = true;
	double dt = cfg.initial_step;
	for (size_t index = 0; index < 2000; ++index) {
		if (!integrator.step(state, dt).has_value()) {
			steps_succeeded = false;
			break;
		}
		finite = finite && std::isfinite(state.x(1)) && std::isfinite(state.x(2)) && std::isfinite(state.x(3));
	}
	suite.check(steps_succeeded, "every adaptive step of the polar trajectory succeeds");
	suite.check(finite, "the Cartesian coordinates stay finite while the orbit crosses the polar axis");
}

void check_horizon_infall(TestSuite& suite) {
	suite.section("Horizon infall");
	const KerrSchildMetric<double> metric(1.0, 0.7, 1.0, 1.0);
	RK45Config<double> cfg;
	cfg.initial_step = 0.005;
	cfg.min_step = 1e-11;
	cfg.max_step = 0.02;
	cfg.rtol = 1e-11;
	cfg.atol = 1e-13;
	cfg.crossing_mode = HorizonCrossingMode::Continuity;
	RK45AdaptiveIntegrator<KerrSchildMetric<double>, double> integrator(metric, GeodesicType::Timelike, cfg);

	GeodesicState<double> state;
	state.x = FourVector<double>(0.0, 4.0, 0.0, 0.0);
	const auto g = metric.metric_tensor(state.x);
	const double ux = -0.4;
	const double quad_a = g(0, 0);
	const double quad_b = 2.0 * g(0, 1) * ux;
	const double quad_c = 1.0 + g(1, 1) * ux * ux;
	const double discriminant = quad_b * quad_b - 4.0 * quad_a * quad_c;
	suite.check(discriminant >= 0.0, "the timelike normalization admits a real temporal velocity for the radial infall");
	if (discriminant < 0.0) {
		return;
	}
	state.u = FourVector<double>((-quad_b - std::sqrt(discriminant)) / (2.0 * quad_a), ux, 0.0, 0.0);

	const double outer_horizon = metric.outer_horizon_radius();
	bool steps_succeeded = true;
	bool crossed = false;
	double dt = cfg.initial_step;
	for (size_t index = 0; index < 5000; ++index) {
		if (!integrator.step(state, dt).has_value()) {
			steps_succeeded = false;
			break;
		}
		if (metric.boyer_lindquist_r(state.x) < outer_horizon) {
			crossed = true;
			break;
		}
	}
	suite.check(steps_succeeded, "every adaptive step of the infall succeeds");
	suite.check(crossed, "the infalling particle crosses the outer horizon in Kerr-Schild coordinates");
}

}

int main() {
	TestSuite suite(
		"kerr_schild_geodesics",
		"Verifies timelike geodesics in horizon-regular Kerr-Schild coordinates: an orbit crossing the polar axis stays finite for two thousand adaptive steps, and a radial infall crosses the outer horizon without the integration failing."
	);
	check_polar_axis_crossing(suite);
	check_horizon_infall(suite);
	return suite.finish();
}

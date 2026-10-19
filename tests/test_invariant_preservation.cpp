#include "support/test_harness.hpp"
#include "relativistic/metrics/schwarzschild.hpp"
#include "relativistic/metrics/painleve_gullstrand.hpp"
#include "relativistic/integrators/rk45_adaptive.hpp"
#include <cmath>
#include <numbers>

using namespace Relativistic::Metrics;
using namespace Relativistic::Integrators;
using namespace Relativistic::Core;
using Relativistic::Testing::TestSuite;

int main() {
	TestSuite suite(
		"invariant_preservation",
		"Verifies that the adaptive integrator preserves the four-velocity normalization: a circular Schwarzschild orbit over fifty thousand steps keeps the invariant residual below 1e-11, and a radial free fall through the horizon in Painleve-Gullstrand coordinates stays below 1e-10."
	);

	constexpr double c = 1.0;
	constexpr double g_constant = 1.0;
	constexpr double mass = 1.0;

	RK45Config<double> config;
	config.initial_step = 0.1;
	config.min_step = 1e-8;
	config.max_step = 10.0;
	config.rtol = 1e-10;
	config.atol = 1e-14;
	config.invariant_tolerance = 1e-12;

	suite.section("Circular timelike orbit");
	SchwarzschildMetric<double> schwarzschild(mass, c, g_constant);
	const double r_s = schwarzschild.schwarzschild_radius();
	const double orbit_radius = 10.0 * r_s;
	const double circular_speed = std::sqrt(g_constant * mass / orbit_radius);
	const double gamma = 1.0 / std::sqrt(1.0 - 3.0 * g_constant * mass / orbit_radius);

	GeodesicState<double> particle;
	particle.x = FourVector<double>(0.0, orbit_radius, std::numbers::pi_v<double> / 2.0, 0.0);
	particle.u = FourVector<double>(gamma, 0.0, 0.0, gamma * circular_speed / orbit_radius);
	RK45AdaptiveIntegrator<SchwarzschildMetric<double>, double> orbit_integrator(schwarzschild, GeodesicType::Timelike, config);
	bool orbit_succeeded = true;
	double dt = config.initial_step;
	for (size_t step = 0; step < 50000; ++step) {
		if (!orbit_integrator.step(particle, dt).has_value()) {
			orbit_succeeded = false;
			break;
		}
	}
	suite.check(orbit_succeeded, "every adaptive step of the circular orbit succeeds");
	const double orbit_residual = orbit_integrator.statistics().max_invariant_residual;
	suite.measure("orbit_invariant_residual", orbit_residual, "dimensionless");
	suite.check_at_most(orbit_residual, 1e-11, "the normalization residual of the circular orbit stays below 1e-11");

	suite.section("Horizon infall in Painleve-Gullstrand coordinates");
	PainleveGullstrandMetric<double> painleve(mass, c, g_constant);
	GeodesicState<double> infalling;
	infalling.x = FourVector<double>(0.0, 4.0 * r_s, std::numbers::pi_v<double> / 2.0, 0.0);
	infalling.u = FourVector<double>(1.0, -std::sqrt(r_s / (4.0 * r_s)), 0.0, 0.0);
	RK45AdaptiveIntegrator<PainleveGullstrandMetric<double>, double> infall_integrator(painleve, GeodesicType::Timelike, config);
	bool infall_succeeded = true;
	bool crossed = false;
	dt = 0.05;
	for (size_t step = 0; step < 20000; ++step) {
		if (!infall_integrator.step(infalling, dt).has_value()) {
			infall_succeeded = false;
			break;
		}
		if (infalling.x(1) < r_s) {
			crossed = true;
		}
		if (infalling.x(1) < 0.2 * r_s) {
			break;
		}
	}
	suite.check(infall_succeeded, "every adaptive step of the infall succeeds");
	suite.check(crossed, "the infalling particle crosses the horizon");
	const double infall_residual = infall_integrator.statistics().max_invariant_residual;
	suite.measure("infall_invariant_residual", infall_residual, "dimensionless");
	suite.check_at_most(infall_residual, 1e-10, "the normalization residual of the infall stays below 1e-10");
	return suite.finish();
}

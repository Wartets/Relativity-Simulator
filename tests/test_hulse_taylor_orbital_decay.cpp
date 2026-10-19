#include "support/test_harness.hpp"
#include "relativistic/dynamics/hulse_taylor_pulsar.hpp"
#include "relativistic/dynamics/pn/pn_orders.hpp"
#include "relativistic/dynamics/pn/pn_integrator.hpp"
#include <cmath>
#include <cstddef>

using namespace Relativistic::Dynamics;
using Relativistic::Testing::TestSuite;

int main() {
	TestSuite suite(
		"hulse_taylor_orbital_decay",
		"Verifies the Hulse-Taylor binary pulsar: the Peters-Mathews orbital period derivative agrees with the observed value, the post-Newtonian system of the pulsar is built with two bodies and a positive gravitational wave luminosity, and the adaptive integrator advances it."
	);

	suite.section("Peters-Mathews period derivative");
	const double analytical = HulseTaylorPulsar::peters_mathews_p_dot();
	const double observed = HulseTaylorPulsar::OBSERVATIONAL_P_DOT;
	const double agreement = std::abs((analytical - observed) / observed);
	suite.measure("peters_mathews_p_dot", analytical, "s/s");
	suite.measure("observed_p_dot", observed, "s/s");
	suite.measure("relative_difference", agreement, "dimensionless");
	suite.check_at_most(agreement, 0.01, "the analytical period derivative agrees with the observed value within 1 percent");

	suite.section("Post-Newtonian orbital evolution");
	const auto config = PNOrderConfig::make_all_enabled();
	auto system = HulseTaylorPulsar::create_initial_system(config);
	suite.check(system.body_count() == 2, "the pulsar system contains exactly two bodies");
	suite.check(system.latest_gw_emission().radiated_power > 0.0, "the initial gravitational wave luminosity is positive");

	double dt = 10.0;
	for (size_t step = 0; step < 500; ++step) {
		AdaptiveRungeKuttaPNIntegrator::step(system, dt);
	}
	suite.check(system.step_count() > 0, "the integrator recorded at least one step");
	suite.check(system.time() > 0.0, "the system time advanced");
	suite.measure("evolved_time", system.time(), "simulation time");
	return suite.finish();
}

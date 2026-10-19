#include "support/test_harness.hpp"
#include "relativistic/dynamics/pn/pn_body.hpp"
#include "relativistic/dynamics/pn/pn_orders.hpp"
#include "relativistic/dynamics/pn/pn_acceleration.hpp"
#include "relativistic/dynamics/pn/pn_nbody_system.hpp"
#include "relativistic/dynamics/pn/pn_integrator.hpp"
#include <cstddef>

using namespace Relativistic::Dynamics;
using Relativistic::Testing::TestSuite;

int main() {
	TestSuite suite(
		"pn_radiation_reaction",
		"Verifies the 2.5PN radiation reaction of an equal-mass binary: the total energy decreases monotonically in aggregate and the quadrupole radiated power is positive."
	);

	suite.section("Radiative energy loss");
	constexpr double separation = 15.0;
	constexpr double speed = 0.2;
	constexpr double step = 0.01;
	constexpr size_t step_count = 2000;

	PostNewtonianSystem system(PNOrderConfig::make_2_5pn_radiation_only(1.0, 1.0));
	const PostNewtonianBody first(1, 1.0, 1.0, {-separation * 0.5, 0.0, 0.0}, {0.0, -speed, 0.0});
	const PostNewtonianBody second(2, 1.0, 1.0, {separation * 0.5, 0.0, 0.0}, {0.0, speed, 0.0});
	system.add_body(first);
	system.add_body(second);
	system.update_accelerations();

	const double initial_energy = system.compute_total_energy();
	for (size_t index = 0; index < step_count; ++index) {
		RungeKutta4PNIntegrator::step(system, step);
	}
	const double final_energy = system.compute_total_energy();
	const double radiated_power = system.latest_gw_emission().radiated_power;

	suite.measure("initial_energy", initial_energy, "simulation energy");
	suite.measure("final_energy", final_energy, "simulation energy");
	suite.measure("radiated_power", radiated_power, "simulation power");
	suite.check_finite(final_energy, "the final total energy is finite");
	suite.check(final_energy < initial_energy, "the total energy after 2000 steps is lower than the initial energy", "initial=" + Relativistic::Testing::format_number(initial_energy, 12) + " final=" + Relativistic::Testing::format_number(final_energy, 12));
	suite.check(radiated_power > 0.0, "the quadrupole radiated power is positive");
	return suite.finish();
}

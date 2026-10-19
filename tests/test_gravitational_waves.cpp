#include "support/test_harness.hpp"
#include "relativistic/dynamics/pn/pn_body.hpp"
#include "relativistic/dynamics/pn/pn_gravitational_waves.hpp"
#include <cmath>
#include <numbers>

using namespace Relativistic::Dynamics;
using Relativistic::Testing::TestSuite;

int main() {
	TestSuite suite(
		"gravitational_waves",
		"Verifies the quadrupole gravitational wave strain of a binary neutron star system at face-on inclination: the plus polarization dominates at orbital phase zero, the cross polarization dominates at a quarter-turn phase, and each suppressed polarization vanishes relative to the dominant one."
	);

	constexpr double solar_mass = 1.98847e30;
	const double mass = 1.4 * solar_mass;
	constexpr double separation = 1e8;
	constexpr double distance = 1e20;
	constexpr double inclination = 0.0;

	suite.section("Orbital phase zero");
	const auto strain_zero = GravitationalWaveCalculator::compute_binary_quadrupole_strain(mass, mass, separation, 0.0, distance, inclination);
	suite.check(strain_zero.h_plus != 0.0, "the plus polarization is non-zero at phase zero");
	suite.check_at_most(std::abs(strain_zero.h_cross), 1e-9 * std::abs(strain_zero.h_plus), "the cross polarization vanishes relative to the plus polarization at phase zero");
	suite.measure("h_plus_phase_zero", strain_zero.h_plus, "dimensionless strain");

	suite.section("Orbital phase of a quarter of a half turn");
	const auto strain_quarter = GravitationalWaveCalculator::compute_binary_quadrupole_strain(mass, mass, separation, std::numbers::pi / 4.0, distance, inclination);
	suite.check(strain_quarter.h_cross != 0.0, "the cross polarization is non-zero at phase pi/4");
	suite.check_at_most(std::abs(strain_quarter.h_plus), 1e-9 * std::abs(strain_quarter.h_cross), "the plus polarization vanishes relative to the cross polarization at phase pi/4");
	suite.measure("h_cross_phase_quarter", strain_quarter.h_cross, "dimensionless strain");
	return suite.finish();
}

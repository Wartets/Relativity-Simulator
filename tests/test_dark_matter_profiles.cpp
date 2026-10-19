#include "support/test_harness.hpp"
#include "relativistic/dark_matter/dark_matter_profiles.hpp"
#include <cmath>
#include <numbers>

using namespace Relativistic::DarkMatter;
using Relativistic::Testing::TestSuite;

int main() {
	TestSuite suite(
		"dark_matter_profiles",
		"Verifies the analytic dark matter halo profiles: the NFW density and enclosed mass at the scale radius, the location of the peak circular velocity, the sign and direction of the acceleration, the Einasto and Burkert densities at the scale radius, and the Hernquist enclosed mass and potential."
	);

	constexpr double characteristic_density = 1e7;
	constexpr double scale_radius = 20.0;
	constexpr double gravitational_constant = 1.0;
	const NFWProfile<double> nfw(characteristic_density, scale_radius, gravitational_constant);

	suite.section("Navarro-Frenk-White");
	suite.check_relative(nfw.density(scale_radius), characteristic_density / 4.0, 1e-12, "the density at the scale radius equals a quarter of the characteristic density");
	const double expected_mass = 4.0 * std::numbers::pi_v<double> * characteristic_density * scale_radius * scale_radius * scale_radius * (std::log(2.0) - 0.5);
	suite.check_relative(nfw.enclosed_mass(scale_radius), expected_mass, 1e-12, "the enclosed mass at the scale radius equals 4 pi rho0 rs^3 (ln 2 - 1/2)");
	const double peak_radius = nfw.radius_of_max_velocity();
	const double peak_velocity = nfw.max_circular_velocity();
	suite.check(peak_velocity >= nfw.circular_velocity(peak_radius * 0.9) && peak_velocity >= nfw.circular_velocity(peak_radius * 1.1), "the circular velocity at the reported radius is a local maximum");
	const auto acceleration = nfw.acceleration_3d({scale_radius, 0.0, 0.0});
	suite.check(acceleration[0] < 0.0, "the acceleration points toward the halo centre");
	suite.check_at_most(std::abs(acceleration[1]) + std::abs(acceleration[2]), 1e-12 * std::abs(acceleration[0]), "the acceleration has no transverse component on the x axis");

	suite.section("Einasto");
	const EinastoProfile<double> einasto(characteristic_density, scale_radius, 0.16, gravitational_constant);
	suite.check_relative(einasto.density(scale_radius), characteristic_density, 1e-12, "the density at the scale radius equals the characteristic density");
	suite.check(einasto.enclosed_mass(scale_radius) > 0.0, "the enclosed mass is positive");
	suite.check(einasto.circular_velocity(scale_radius) > 0.0, "the circular velocity is positive");

	suite.section("Burkert");
	const BurkertProfile<double> burkert(characteristic_density, scale_radius, gravitational_constant);
	suite.check_relative(burkert.density(scale_radius), characteristic_density / 4.0, 1e-12, "the density at the scale radius equals a quarter of the characteristic density");
	suite.check(burkert.enclosed_mass(scale_radius) > 0.0, "the enclosed mass is positive");

	suite.section("Hernquist");
	constexpr double hernquist_mass = 1e11;
	const HernquistProfile<double> hernquist(hernquist_mass, scale_radius, gravitational_constant);
	suite.check_relative(hernquist.enclosed_mass(scale_radius), hernquist_mass * 0.25, 1e-14, "a quarter of the mass is enclosed within the scale radius");
	suite.check_relative(hernquist.gravitational_potential(scale_radius), -(gravitational_constant * hernquist_mass) / (2.0 * scale_radius), 1e-14, "the potential at the scale radius equals -G M / (2 a)");
	return suite.finish();
}

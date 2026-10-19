#include "support/test_harness.hpp"
#include "relativistic/modified_gravity/f_r_gravity.hpp"
#include <cmath>

using namespace Relativistic::ModifiedGravity;
using Relativistic::Testing::TestSuite;

int main() {
	TestSuite suite(
		"f_r_chameleon",
		"Verifies the Hu-Sawicki chameleon model: the scalaron mass grows with the ambient density, the Earth is thin-shell screened so that its effective coupling equals Newtonian gravity, and an unscreened low-density cloud feels the enhanced coupling of 4/3."
	);

	HuSawickiParams<double> parameters;
	parameters.f_r0 = 1e-11;
	parameters.n_index = 1.0;
	parameters.r_background0 = 1e-52;
	FRChameleonModel<double> model(parameters);

	constexpr double dense = 5500.0;
	constexpr double cosmic = 1e-26;

	suite.section("Scalaron mass");
	const double mass_dense = model.scalaron_effective_mass(dense);
	const double mass_cosmic = model.scalaron_effective_mass(cosmic);
	suite.measure("scalaron_mass_ratio", mass_dense / mass_cosmic, "ratio");
	suite.check(mass_dense > mass_cosmic * 1e6, "the scalaron is more than a million times heavier in the dense environment");

	suite.section("Screened Earth");
	constexpr double earth_mass = 5.972e24;
	constexpr double earth_radius = 6.371e6;
	const double thin_shell = model.thin_shell_factor(earth_mass, earth_radius, cosmic, dense);
	const double coupling = model.effective_gravitational_coupling(earth_mass, earth_radius, cosmic, dense);
	const double newton = model.gravitational_constant();
	suite.check_at_most(thin_shell, 0.1, "the thin shell factor of the Earth is below 0.1");
	suite.check_relative(coupling, newton, 0.05, "the effective coupling of the Earth equals Newtonian gravity within 5 percent");

	suite.section("Unscreened cloud");
	constexpr double cloud_mass = 1e15;
	constexpr double cloud_radius = 1e9;
	const double cloud_shell = model.thin_shell_factor(cloud_mass, cloud_radius, cosmic, 1e-20);
	const double cloud_coupling = model.effective_gravitational_coupling(cloud_mass, cloud_radius, cosmic, 1e-20);
	suite.check_near(cloud_shell, 1.0, 1e-12, "the thin shell factor of the cloud saturates at one");
	suite.check_relative(cloud_coupling, (4.0 / 3.0) * newton, 1e-6, "the effective coupling of the cloud equals four thirds of Newtonian gravity");
	return suite.finish();
}

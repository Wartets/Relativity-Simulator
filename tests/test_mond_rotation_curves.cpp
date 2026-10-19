#include "support/test_harness.hpp"
#include "relativistic/modified_gravity/mond.hpp"
#include "relativistic/core/constants.hpp"
#include <cmath>

using namespace Relativistic::ModifiedGravity;
using namespace Relativistic::Core;
using Relativistic::Testing::TestSuite;

int main() {
	TestSuite suite(
		"mond_rotation_curves",
		"Verifies the MOND framework: the Newtonian limit at high acceleration, the deep-MOND limit at low acceleration, the asymptotically flat rotation velocity of a point mass, the baryonic Tully-Fisher relation v^4 = G M a0, and the simple interpolation function."
	);

	constexpr double a0 = 1.2e-10;
	constexpr double parsec = 3.085677581491367e16;
	const double g = PhysicalConstants<double>::GRAVITATIONAL_CONSTANT;
	const double baryonic_mass = 1e11 * PhysicalConstants<double>::SOLAR_MASS;
	MondFramework<double> mond(a0, MondInterpolationFunction::Standard, g);

	suite.section("Acceleration limits");
	constexpr double high = 1e-7;
	suite.check_relative(mond.compute_mond_acceleration(high), high, 0.01, "at high acceleration the MOND acceleration equals the Newtonian value within 1 percent");
	constexpr double low = 1e-13;
	suite.check_relative(mond.compute_mond_acceleration(low), std::sqrt(low * a0), 0.05, "at low acceleration the MOND acceleration follows sqrt(a_N a0) within 5 percent");

	suite.section("Rotation curve of a point mass");
	const double flat = mond.asymptotic_flat_velocity(baryonic_mass);
	suite.measure("asymptotic_flat_velocity", flat, "m/s");
	suite.check_relative(mond.point_mass_circular_velocity(baryonic_mass, 30.0 * 1000.0 * parsec), flat, 0.05, "the circular velocity at 30 kpc is within 5 percent of the flat value");
	suite.check_relative(mond.point_mass_circular_velocity(baryonic_mass, 50.0 * 1000.0 * parsec), flat, 0.03, "the circular velocity at 50 kpc is within 3 percent of the flat value");
	const double velocity_100 = mond.point_mass_circular_velocity(baryonic_mass, 100.0 * 1000.0 * parsec);
	suite.check_relative(velocity_100, flat, 0.02, "the circular velocity at 100 kpc is within 2 percent of the flat value");

	suite.section("Baryonic Tully-Fisher relation");
	suite.check_relative(velocity_100 * velocity_100 * velocity_100 * velocity_100, g * baryonic_mass * a0, 0.05, "v^4 equals G M a0 within 5 percent at 100 kpc");

	suite.section("Simple interpolation function");
	mond.set_function_type(MondInterpolationFunction::Simple);
	suite.check_relative(mond.point_mass_circular_velocity(baryonic_mass, 100.0 * 1000.0 * parsec), flat, 0.05, "the simple interpolation reaches the flat velocity within 5 percent at 100 kpc");
	return suite.finish();
}

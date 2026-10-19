#include "support/test_harness.hpp"
#include "relativistic/gravimetry/tidal_perturbations.hpp"
#include "relativistic/gravimetry/spherical_harmonics.hpp"
#include <array>
#include <cmath>

using namespace Relativistic::Gravimetry;
using Relativistic::Testing::TestSuite;

int main() {
	TestSuite suite(
		"tidal_rotational_gravity",
		"Verifies the tidal and rotational perturbation models: the lunar tide changes the Earth C20 coefficient by a small non-zero amount, the direct tidal acceleration of a satellite on the Earth-Moon axis is purely radial, and the rotational flattening reproduces the Earth J2."
	);

	suite.section("Third-body tide");
	SphericalHarmonicsGravityModel<4> earth(3.986004418e14, 6378137.0, 2);
	earth.set_zonal_coefficient(2, 1.08263e-3);
	const double before = earth.normalized_c(2, 0);
	TidalPerturbationModel tidal;
	const std::array<double, 3> moon = {384400000.0, 0.0, 0.0};
	const double moon_gm = 4.9028e12;
	tidal.apply_third_body_tide(earth, moon, moon_gm);
	const double after = earth.normalized_c(2, 0);
	suite.check(after != before, "the lunar tide modifies the normalized C20 coefficient");
	suite.check_at_most(std::abs(after - before), 1e-7, "the C20 change stays below 1e-7");

	suite.section("Direct tidal acceleration");
	const auto acceleration = TidalPerturbationModel::direct_tidal_acceleration({7000000.0, 0.0, 0.0}, moon, moon_gm);
	suite.check(std::abs(acceleration[0]) > 0.0, "the tidal acceleration has a non-zero component along the Earth-Moon axis");
	suite.check_at_most(std::abs(acceleration[1]), 1e-20, "the transverse y component vanishes on the axis");
	suite.check_at_most(std::abs(acceleration[2]), 1e-20, "the transverse z component vanishes on the axis");

	suite.section("Rotational flattening");
	const double flattening = TidalPerturbationModel::rotational_flattening_j2(7.292115e-5, 6378137.0, 3.986004418e14, 0.938);
	suite.measure("rotational_j2", flattening, "dimensionless");
	suite.check_near(flattening, 1.08e-3, 1e-4, "the rotational contribution to J2 is within 1e-4 of 1.08e-3");
	return suite.finish();
}

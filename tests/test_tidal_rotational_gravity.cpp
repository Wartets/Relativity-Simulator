#include "relativistic/gravimetry/tidal_perturbations.hpp"
#include "relativistic/gravimetry/spherical_harmonics.hpp"
#include <iostream>
#include <cassert>
#include <cmath>

int main() {
	using namespace Relativistic::Gravimetry;

	SphericalHarmonicsGravityModel<4> earth(3.986004418e14, 6378137.0, 2);
	earth.set_zonal_coefficient(2, 1.08263e-3);

	const double c20_before = earth.normalized_c(2, 0);

	TidalPerturbationModel tidal;
	const std::array<double, 3> moon_pos = {384400000.0, 0.0, 0.0};
	const double moon_gm = 4.9028e12;

	tidal.apply_third_body_tide(earth, moon_pos, moon_gm);
	const double c20_after = earth.normalized_c(2, 0);

	assert(c20_after != c20_before);
	assert(std::abs(c20_after - c20_before) < 1e-7);

	const std::array<double, 3> sat_pos = {7000000.0, 0.0, 0.0};
	const auto a_tide = TidalPerturbationModel::direct_tidal_acceleration(sat_pos, moon_pos, moon_gm);

	assert(std::abs(a_tide[0]) > 0.0);
	assert(std::abs(a_tide[1]) < 1e-20);
	assert(std::abs(a_tide[2]) < 1e-20);

	const double omega_earth = 7.292115e-5;
	const double j2_rot = TidalPerturbationModel::rotational_flattening_j2(
		omega_earth, 6378137.0, 3.986004418e14, 0.938
	);
	assert(std::abs(j2_rot - 1.08e-3) < 1e-4);

	std::cout << "Tidal and rotational gravimetry tests passed.\n";
	return 0;
}

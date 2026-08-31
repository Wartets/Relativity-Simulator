#include "relativistic/dynamics/pn_body.hpp"
#include "relativistic/dynamics/pn_gravitational_waves.hpp"
#include <iostream>
#include <cassert>
#include <cmath>

int main() {
	using namespace Relativistic::Dynamics;

	const double m1 = 1.4 * 1.98847e30;
	const double m2 = 1.4 * 1.98847e30;
	const double sep = 1e8;
	const double dist = 1e20;
	const double inc = 0.0;

	const auto strain = GravitationalWaveCalculator::compute_binary_quadrupole_strain(
		m1, m2, sep, 0.0, dist, inc
	);

	assert(strain.h_plus != 0.0);
	assert(std::abs(strain.h_cross) < 1e-30);

	const auto strain_45 = GravitationalWaveCalculator::compute_binary_quadrupole_strain(
		m1, m2, sep, std::numbers::pi / 4.0, dist, inc
	);

	assert(std::abs(strain_45.h_plus) < 1e-30);
	assert(strain_45.h_cross != 0.0);

	std::cout << "test_gravitational_waves passed.\n";
	return 0;
}

#include "relativistic/dynamics/pn_body.hpp"
#include "relativistic/dynamics/pn_orders.hpp"
#include "relativistic/dynamics/pn_spin_precession.hpp"
#include <iostream>
#include <cassert>
#include <cmath>

int main() {
	using namespace Relativistic::Dynamics;

	const auto cfg = PNOrderConfig::make_all_enabled(1.0, 1.0);

	const double m1 = 1.0;
	const double m2 = 1.0;
	const std::array<double, 3> r = {10.0, 0.0, 0.0};
	const std::array<double, 3> v = {0.0, 0.2, 0.0};
	const std::array<double, 3> s1 = {0.0, 0.0, 0.5};
	const std::array<double, 3> s2 = {0.1, 0.0, 0.4};

	const auto rates = PostNewtonianSpinSolver::compute_spin_precession_binary(
		r, v, m1, m2, s1, s2, cfg
	);

	assert(rates.omega_1[2] > 0.0);
	assert(rates.omega_2[2] > 0.0);

	const double s1_dot_ds1 = s1[0] * rates.d_spin_1[0] + s1[1] * rates.d_spin_1[1] + s1[2] * rates.d_spin_1[2];
	const double s2_dot_ds2 = s2[0] * rates.d_spin_2[0] + s2[1] * rates.d_spin_2[1] + s2[2] * rates.d_spin_2[2];

	assert(std::abs(s1_dot_ds1) < 1e-14);
	assert(std::abs(s2_dot_ds2) < 1e-14);

	std::cout << "test_pn_spin_precession passed.\n";
	return 0;
}

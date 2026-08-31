#include "relativistic/gravimetry/legendre_table.hpp"
#include "relativistic/gravimetry/spherical_harmonics.hpp"
#include <iostream>
#include <cassert>
#include <cmath>
#include <numbers>

void test_associated_legendre_polynomials() {
	using namespace Relativistic::Gravimetry;
	AssociatedLegendreTable<16> table;

	const double sin_phi = 0.5;
	const double cos_phi = std::sqrt(1.0 - 0.25);
	table.compute(sin_phi, cos_phi, 8);

	const double p00 = table.p_unnormalized(0, 0);
	assert(std::abs(p00 - 1.0) < 1e-14);

	const double p10 = table.p_unnormalized(1, 0);
	assert(std::abs(p10 - sin_phi) < 1e-14);

	const double p11 = table.p_unnormalized(1, 1);
	assert(std::abs(p11 - cos_phi) < 1e-14);

	const double p20 = table.p_unnormalized(2, 0);
	const double p20_exact = 0.5 * (3.0 * sin_phi * sin_phi - 1.0);
	assert(std::abs(p20 - p20_exact) < 1e-14);

	const double p21 = table.p_unnormalized(2, 1);
	const double p21_exact = 3.0 * sin_phi * cos_phi;
	assert(std::abs(p21 - p21_exact) < 1e-14);

	const double p22 = table.p_unnormalized(2, 2);
	const double p22_exact = 3.0 * cos_phi * cos_phi;
	assert(std::abs(p22 - p22_exact) < 1e-14);

	const double dp20 = table.dp_unnormalized_dphi(2, 0);
	const double dp20_exact = 3.0 * sin_phi * cos_phi;
	assert(std::abs(dp20 - dp20_exact) < 1e-13);
}

void test_gravity_gradient_consistency() {
	using namespace Relativistic::Gravimetry;
	auto earth = SphericalHarmonicsGravityModel<20>::make_earth_egm96(10);

	const std::array<double, 3> pos = {4500000.0, 2500000.0, 4800000.0};
	const auto a_ana = earth.evaluate_acceleration(pos);

	const double h = 1.0;
	const double v_x_plus = earth.evaluate_potential({pos[0] + h, pos[1], pos[2]});
	const double v_x_minus = earth.evaluate_potential({pos[0] - h, pos[1], pos[2]});
	const double a_x_num = -(v_x_plus - v_x_minus) / (2.0 * h);

	const double v_y_plus = earth.evaluate_potential({pos[0], pos[1] + h, pos[2]});
	const double v_y_minus = earth.evaluate_potential({pos[0], pos[1] - h, pos[2]});
	const double a_y_num = -(v_y_plus - v_y_minus) / (2.0 * h);

	const double v_z_plus = earth.evaluate_potential({pos[0], pos[1], pos[2] + h});
	const double v_z_minus = earth.evaluate_potential({pos[0], pos[1], pos[2] - h});
	const double a_z_num = -(v_z_plus - v_z_minus) / (2.0 * h);

	const double err_x = std::abs(a_ana[0] - a_x_num) / std::abs(a_ana[0]);
	const double err_y = std::abs(a_ana[1] - a_y_num) / std::abs(a_ana[1]);
	const double err_z = std::abs(a_ana[2] - a_z_num) / std::abs(a_ana[2]);

	assert(err_x < 1e-8);
	assert(err_y < 1e-8);
	assert(err_z < 1e-8);
}

void test_planetary_models_loading() {
	using namespace Relativistic::Gravimetry;
	const auto earth = SphericalHarmonicsGravityModel<20>::make_earth_egm96(20);
	assert(std::abs(earth.zonal_j(2) - 1.08262668355e-3) < 1e-12);
	assert(std::abs(earth.zonal_j(3) - (-2.53265648533e-6)) < 1e-12);
	assert(std::abs(earth.zonal_j(4) - (-1.61962159137e-6)) < 1e-12);

	const auto moon = SphericalHarmonicsGravityModel<16>::make_moon_lp165(16);
	assert(std::abs(moon.zonal_j(2) - 2.0335e-4) < 1e-12);

	const auto jupiter = SphericalHarmonicsGravityModel<10>::make_jupiter(10);
	assert(std::abs(jupiter.zonal_j(2) - 1.4696572e-2) < 1e-12);
}

int main() {
	test_associated_legendre_polynomials();
	test_gravity_gradient_consistency();
	test_planetary_models_loading();
	std::cout << "All spherical harmonics unit tests passed.\n";
	return 0;
}

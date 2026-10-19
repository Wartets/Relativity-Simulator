#include "support/test_harness.hpp"
#include "relativistic/gravimetry/legendre_table.hpp"
#include "relativistic/gravimetry/spherical_harmonics.hpp"
#include <array>
#include <cmath>

using namespace Relativistic::Gravimetry;
using Relativistic::Testing::TestSuite;

int main() {
	TestSuite suite(
		"spherical_harmonics",
		"Verifies the spherical harmonic gravity model: unnormalized associated Legendre functions and their derivative against closed forms, the consistency of the analytic acceleration with the numerical gradient of the potential, and the zonal coefficients of the Earth, Moon and Jupiter models."
	);

	suite.section("Associated Legendre functions");
	AssociatedLegendreTable<16> table;
	const double sine = 0.5;
	const double cosine = std::sqrt(0.75);
	table.compute(sine, cosine, 8);
	suite.check_near(table.p_unnormalized(0, 0), 1.0, 1e-14, "P00 equals 1");
	suite.check_near(table.p_unnormalized(1, 0), sine, 1e-14, "P10 equals sin(phi)");
	suite.check_near(table.p_unnormalized(1, 1), cosine, 1e-14, "P11 equals cos(phi)");
	suite.check_near(table.p_unnormalized(2, 0), 0.5 * (3.0 * sine * sine - 1.0), 1e-14, "P20 equals (3 sin^2 - 1) / 2");
	suite.check_near(table.p_unnormalized(2, 1), 3.0 * sine * cosine, 1e-14, "P21 equals 3 sin cos");
	suite.check_near(table.p_unnormalized(2, 2), 3.0 * cosine * cosine, 1e-14, "P22 equals 3 cos^2");
	suite.check_near(table.dp_unnormalized_dphi(2, 0), 3.0 * sine * cosine, 1e-13, "the derivative of P20 equals 3 sin cos");

	suite.section("Acceleration against potential gradient");
	auto earth = SphericalHarmonicsGravityModel<20>::make_earth_egm96(10);
	const std::array<double, 3> position = {4500000.0, 2500000.0, 4800000.0};
	const auto analytic = earth.evaluate_acceleration(position);
	constexpr double step = 1.0;
	for (size_t axis = 0; axis < 3; ++axis) {
		std::array<double, 3> forward = position;
		std::array<double, 3> backward = position;
		forward[axis] += step;
		backward[axis] -= step;
		const double numeric = -(earth.evaluate_potential(forward) - earth.evaluate_potential(backward)) / (2.0 * step);
		suite.check_relative(numeric, analytic[axis], 1e-8, "the central-difference gradient of the potential matches the analytic acceleration on axis " + std::to_string(axis));
	}

	suite.section("Planetary models");
	const auto earth_full = SphericalHarmonicsGravityModel<20>::make_earth_egm96(20);
	suite.check_near(earth_full.zonal_j(2), 1.08262668355e-3, 1e-12, "the EGM96 J2 coefficient is loaded");
	suite.check_near(earth_full.zonal_j(3), -2.53265648533e-6, 1e-12, "the EGM96 J3 coefficient is loaded");
	suite.check_near(earth_full.zonal_j(4), -1.61962159137e-6, 1e-12, "the EGM96 J4 coefficient is loaded");
	suite.check_near(SphericalHarmonicsGravityModel<16>::make_moon_lp165(16).zonal_j(2), 2.0335e-4, 1e-12, "the LP165 lunar J2 coefficient is loaded");
	suite.check_near(SphericalHarmonicsGravityModel<10>::make_jupiter(10).zonal_j(2), 1.4696572e-2, 1e-12, "the Jupiter J2 coefficient is loaded");
	return suite.finish();
}

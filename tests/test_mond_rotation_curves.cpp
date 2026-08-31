#include "relativistic/modified_gravity/mond.hpp"
#include "relativistic/core/constants.hpp"
#include <cassert>
#include <cmath>

int main() {
	using namespace Relativistic::ModifiedGravity;
	using namespace Relativistic::Core;

	const double a0 = 1.2e-10;
	const double g = PhysicalConstants<double>::GRAVITATIONAL_CONSTANT;
	const double m_baryon = 1e11 * PhysicalConstants<double>::SOLAR_MASS;

	MondFramework<double> mond(a0, MondInterpolationFunction::Standard, g);

	const double a_newt_high = 1e-7;
	const double a_mond_high = mond.compute_mond_acceleration(a_newt_high);
	assert(std::abs(a_mond_high - a_newt_high) / a_newt_high < 0.01);

	const double a_newt_low = 1e-13;
	const double a_mond_low = mond.compute_mond_acceleration(a_newt_low);
	const double a_mond_asymp = std::sqrt(a_newt_low * a0);
	assert(std::abs(a_mond_low - a_mond_asymp) / a_mond_asymp < 0.05);

	const double v_flat_theory = mond.asymptotic_flat_velocity(m_baryon);

	const double r_kpc30 = 30.0 * 1000.0 * 3.085677581491367e16;
	const double r_kpc50 = 50.0 * 1000.0 * 3.085677581491367e16;
	const double r_kpc100 = 100.0 * 1000.0 * 3.085677581491367e16;

	const double v_30 = mond.point_mass_circular_velocity(m_baryon, r_kpc30);
	const double v_50 = mond.point_mass_circular_velocity(m_baryon, r_kpc50);
	const double v_100 = mond.point_mass_circular_velocity(m_baryon, r_kpc100);

	assert(std::abs(v_30 - v_flat_theory) / v_flat_theory < 0.05);
	assert(std::abs(v_50 - v_flat_theory) / v_flat_theory < 0.03);
	assert(std::abs(v_100 - v_flat_theory) / v_flat_theory < 0.02);

	const double v4_relation = (v_100 * v_100 * v_100 * v_100);
	const double gm_a0 = g * m_baryon * a0;
	assert(std::abs(v4_relation - gm_a0) / gm_a0 < 0.05);

	mond.set_function_type(MondInterpolationFunction::Simple);
	const double v_simple_100 = mond.point_mass_circular_velocity(m_baryon, r_kpc100);
	assert(std::abs(v_simple_100 - v_flat_theory) / v_flat_theory < 0.05);

	return 0;
}

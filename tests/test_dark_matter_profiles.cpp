#include "relativistic/dark_matter/dark_matter_profiles.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

int main() {
	using namespace Relativistic::DarkMatter;

	const double rho_0 = 1e7;
	const double r_s = 20.0;
	const double g = 1.0;

	const NFWProfile<double> nfw(rho_0, r_s, g);

	const double d_rs = nfw.density(r_s);
	const double d_expected = rho_0 / 4.0;
	assert(std::abs(d_rs - d_expected) < 1e-10);

	const double m_rs = nfw.enclosed_mass(r_s);
	const double m_rs_expected = 4.0 * std::numbers::pi_v<double> * rho_0 * (r_s * r_s * r_s) * (std::log(2.0) - 0.5);
	assert(std::abs(m_rs - m_rs_expected) < 1e-8);

	const double r_peak = nfw.radius_of_max_velocity();
	const double v_peak = nfw.max_circular_velocity();
	const double v_before = nfw.circular_velocity(r_peak * 0.9);
	const double v_after = nfw.circular_velocity(r_peak * 1.1);
	assert(v_peak >= v_before && v_peak >= v_after);

	const auto a3d = nfw.acceleration_3d({r_s, 0.0, 0.0});
	assert(a3d[0] < 0.0);
	assert(std::abs(a3d[1]) < 1e-12 && std::abs(a3d[2]) < 1e-12);

	const EinastoProfile<double> einasto(rho_0, r_s, 0.16, g);
	const double d_ein_rs = einasto.density(r_s);
	assert(std::abs(d_ein_rs - rho_0) < 1e-10);
	assert(einasto.enclosed_mass(r_s) > 0.0);
	assert(einasto.circular_velocity(r_s) > 0.0);

	const BurkertProfile<double> burkert(rho_0, r_s, g);
	assert(std::abs(burkert.density(r_s) - rho_0 / 4.0) < 1e-10);
	assert(burkert.enclosed_mass(r_s) > 0.0);

	const HernquistProfile<double> hernquist(1e11, r_s, g);
	assert(hernquist.enclosed_mass(r_s) == 1e11 * 0.25);
	assert(hernquist.gravitational_potential(r_s) == -(g * 1e11) / (2.0 * r_s));

	return 0;
}

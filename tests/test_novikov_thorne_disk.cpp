#include "support/test_harness.hpp"
#include "relativistic/hydro/novikov_thorne.hpp"
#include "relativistic/core/constants.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <vector>

using namespace Relativistic::Hydro;
using namespace Relativistic::Core;
using Relativistic::Testing::TestSuite;

namespace {

[[nodiscard]] NovikovThorneConfig<double> unit_config(double spin) {
	NovikovThorneConfig<double> config;
	config.mass = 1.0;
	config.speed_of_light = 1.0;
	config.gravitational_constant = 1.0;
	config.spin_parameter = spin;
	return config;
}

}

int main() {
	TestSuite suite(
		"novikov_thorne_disk",
		"Verifies the Novikov-Thorne thin disk: the ISCO radius for Schwarzschild, prograde and retrograde Kerr holes, the closed-form Page-Thorne function against numerical quadrature, the radiative efficiency, the normalization of the circular four-velocity, and the flux and temperature profile."
	);

	suite.section("Innermost stable circular orbit");
	NovikovThorneConfig<double> physical;
	physical.mass = PhysicalConstants<double>::SOLAR_MASS;
	const double c = PhysicalConstants<double>::SPEED_OF_LIGHT;
	const double r_g = PhysicalConstants<double>::GRAVITATIONAL_CONSTANT * physical.mass / (c * c);
	physical.spin_parameter = 0.0;
	suite.check_relative(NovikovThorneDisk<double>(physical).isco_radius(), 6.0 * r_g, 1e-12, "the Schwarzschild ISCO equals 6 r_g");
	physical.spin_parameter = 0.998 * r_g;
	const double prograde = NovikovThorneDisk<double>(physical).isco_radius();
	suite.check(prograde > 1.0 * r_g && prograde < 1.3 * r_g, "the prograde ISCO of a = 0.998 lies between 1 and 1.3 r_g", "isco=" + Relativistic::Testing::format_number(prograde / r_g, 6));
	physical.spin_parameter = -0.9 * r_g;
	const double retrograde = NovikovThorneDisk<double>(physical).isco_radius();
	suite.check(retrograde > 8.0 * r_g && retrograde < 9.5 * r_g, "the retrograde ISCO of a = -0.9 lies between 8 and 9.5 r_g", "isco=" + Relativistic::Testing::format_number(retrograde / r_g, 6));

	suite.section("Page-Thorne function");
	double worst = 0.0;
	bool zero_at_isco = true;
	for (const double spin : {0.0, 0.2, 0.5, 0.8, 0.95, -0.3, -0.7}) {
		NovikovThorneDisk<double> disk(unit_config(spin));
		const double isco = disk.isco_radius();
		zero_at_isco = zero_at_isco && disk.page_thorne_f(std::sqrt(isco)) == 0.0;
		for (double multiple = 1.05; multiple <= 50.0; multiple *= 1.5) {
			const double x = std::sqrt(isco * multiple);
			const double numeric = disk.numerical_integral_f(x, 10000);
			worst = std::max(worst, std::abs(disk.page_thorne_f(x) - numeric) / std::max(numeric, 1e-15));
		}
	}
	suite.check(zero_at_isco, "the Page-Thorne function vanishes at the ISCO for every sampled spin");
	suite.measure("worst_closed_form_deviation", worst, "dimensionless");
	suite.check_at_most(worst, 1e-7, "the closed form agrees with numerical quadrature within a relative 1e-7");

	suite.section("Radiative efficiency");
	suite.check_near(NovikovThorneDisk<double>(unit_config(0.0)).radiative_efficiency(), 1.0 - std::sqrt(8.0 / 9.0), 1e-10, "the Schwarzschild efficiency equals 1 - sqrt(8/9)");
	const double kerr_efficiency = NovikovThorneDisk<double>(unit_config(0.998)).radiative_efficiency();
	suite.check(kerr_efficiency > 0.30 && kerr_efficiency < 0.35, "the efficiency of a = 0.998 lies between 0.30 and 0.35", "eta=" + Relativistic::Testing::format_number(kerr_efficiency, 6));

	suite.section("Circular four-velocity");
	NovikovThorneConfig<double> rotating = unit_config(5.0);
	rotating.mass = 10.0;
	NovikovThorneDisk<double> rotating_disk(rotating);
	const double isco = rotating_disk.isco_radius();
	double norm_deviation = 0.0;
	for (double r = isco * 1.01; r <= isco * 20.0; r += isco) {
		const auto u = rotating_disk.four_velocity(r);
		const auto g = rotating_disk.metric().metric_tensor(FourVector<double>(0.0, r, std::numbers::pi_v<double> * 0.5, 0.0));
		double norm = 0.0;
		for (size_t mu = 0; mu < 4; ++mu) {
			for (size_t nu = 0; nu < 4; ++nu) {
				norm += g(mu, nu) * u(mu) * u(nu);
			}
		}
		norm_deviation = std::max(norm_deviation, std::abs(norm + 1.0));
	}
	suite.check_at_most(norm_deviation, 1e-10, "the circular four-velocity satisfies u.u = -1 across the disk");

	suite.section("Flux and temperature profile");
	NovikovThorneConfig<double> stellar;
	stellar.mass = 10.0 * PhysicalConstants<double>::SOLAR_MASS;
	stellar.spin_parameter = 0.5 * (PhysicalConstants<double>::GRAVITATIONAL_CONSTANT * stellar.mass / (c * c));
	stellar.accretion_rate = 1e15;
	NovikovThorneDisk<double> stellar_disk(stellar);
	const double stellar_isco = stellar_disk.isco_radius();
	suite.check(stellar_disk.radiative_flux(0.9 * stellar_isco) == 0.0 && stellar_disk.effective_temperature(0.9 * stellar_isco) == 0.0, "the flux and temperature vanish inside the ISCO");
	bool positive = true;
	double peak = 0.0;
	double peak_radius = 0.0;
	for (double r = stellar_isco * 1.0 + 0.05 * stellar_isco; r <= 30.0 * stellar_isco; r += 0.05 * stellar_isco) {
		const double flux = stellar_disk.radiative_flux(r);
		positive = positive && flux > 0.0 && stellar_disk.effective_temperature(r) > 0.0;
		if (flux > peak) {
			peak = flux;
			peak_radius = r;
		}
	}
	suite.check(positive, "the flux and temperature are positive outside the ISCO");
	suite.check(peak_radius > stellar_isco && peak_radius < 3.0 * stellar_isco, "the flux peaks between one and three ISCO radii", "peak_over_isco=" + Relativistic::Testing::format_number(peak_radius / stellar_isco, 6));
	return suite.finish();
}

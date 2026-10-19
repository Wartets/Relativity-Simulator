#include "support/test_harness.hpp"
#include "relativistic/hydro/fishbone_moncrief.hpp"
#include "relativistic/core/constants.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>

using namespace Relativistic::Hydro;
using namespace Relativistic::Core;
using Relativistic::Testing::TestSuite;

namespace {

[[nodiscard]] FishboneMoncriefConfig<double> make_config(double spin, double inner, double center, double gamma, double density) {
	FishboneMoncriefConfig<double> config;
	config.mass = 1.0;
	config.speed_of_light = 1.0;
	config.gravitational_constant = 1.0;
	config.spin_parameter = spin;
	config.r_in = inner;
	config.r_center = center;
	config.gamma = gamma;
	config.max_density = density;
	return config;
}

}

int main() {
	TestSuite suite(
		"fishbone_moncrief_torus",
		"Verifies the Fishbone-Moncrief torus: the enthalpy and density at the pressure maximum and at the cusp, the exclusion of the region inside the inner edge, the normalization of the fluid four-velocity, the polytropic and enthalpy relations and the Cartesian evaluation."
	);
	const double equator = std::numbers::pi_v<double> * 0.5;

	suite.section("Equilibrium and cusp");
	const auto config = make_config(0.9, 4.5, 8.5, 4.0 / 3.0, 1000.0);
	FishboneMoncriefTorus<double> torus(config);
	suite.check(torus.specific_angular_momentum() > 0.0, "the specific angular momentum is positive");
	suite.check(torus.w_in() > torus.w_center(), "the potential at the inner edge exceeds that at the centre");
	suite.check(torus.max_enthalpy() > 1.0, "the maximum enthalpy exceeds 1");
	suite.check_near(torus.specific_enthalpy(config.r_center, equator), torus.max_enthalpy(), 1e-12, "the enthalpy at the pressure maximum equals the maximum enthalpy");
	suite.check_relative(torus.density(config.r_center, equator), config.max_density, 1e-10, "the density at the pressure maximum equals the configured maximum");
	suite.check_near(torus.specific_enthalpy(config.r_in, equator), 1.0, 1e-12, "the enthalpy at the cusp equals 1");
	suite.check(!torus.is_inside_torus(0.9 * config.r_in, equator) && torus.density(0.9 * config.r_in, equator) == 0.0 && torus.pressure(0.9 * config.r_in, equator) == 0.0, "the region inside the inner edge is empty");
	suite.check(torus.is_inside_torus(config.r_center, equator) && torus.density(config.r_center, equator) > 0.0 && torus.pressure(config.r_center, equator) > 0.0, "the pressure maximum lies inside the torus with positive density and pressure");

	suite.section("Four-velocity normalization");
	FishboneMoncriefTorus<double> rotating(make_config(0.5, 5.0, 10.0, 5.0 / 3.0, 1.0));
	double worst = 0.0;
	for (double r = 5.1; r <= 15.0; r += 0.5) {
		if (!rotating.is_inside_torus(r, equator)) {
			continue;
		}
		const auto u = rotating.four_velocity(r, equator);
		const auto g = rotating.metric().metric_tensor(FourVector<double>(0.0, r, equator, 0.0));
		double norm = 0.0;
		for (size_t mu = 0; mu < 4; ++mu) {
			for (size_t nu = 0; nu < 4; ++nu) {
				norm += g(mu, nu) * u(mu) * u(nu);
			}
		}
		worst = std::max(worst, std::abs(norm + 1.0));
	}
	suite.check_at_most(worst, 1e-10, "the fluid four-velocity satisfies u.u = -1 inside the torus");

	suite.section("Polytropic relations");
	const auto polytropic = make_config(0.8, 4.8, 9.0, 4.0 / 3.0, 500.0);
	FishboneMoncriefTorus<double> polytrope(polytropic);
	const double constant = polytrope.polytropic_constant();
	double pressure_deviation = 0.0;
	double enthalpy_deviation = 0.0;
	for (double r = 5.0; r < 16.2; r += 0.3) {
		if (!polytrope.is_inside_torus(r, equator)) {
			continue;
		}
		const double density = polytrope.density(r, equator);
		const double pressure = polytrope.pressure(r, equator);
		const double enthalpy = 1.0 + (polytropic.gamma / (polytropic.gamma - 1.0)) * pressure / density;
		pressure_deviation = std::max(pressure_deviation, std::abs(pressure - constant * std::pow(density, polytropic.gamma)) / (pressure + 1e-30));
		enthalpy_deviation = std::max(enthalpy_deviation, std::abs(polytrope.specific_enthalpy(r, equator) - enthalpy) / enthalpy);
	}
	suite.check_at_most(pressure_deviation, 1e-10, "the pressure follows P = K rho^Gamma");
	suite.check_at_most(enthalpy_deviation, 1e-10, "the enthalpy follows h = 1 + Gamma / (Gamma - 1) P / rho");

	suite.section("Cartesian evaluation");
	FishboneMoncriefTorus<double> schwarzschild(make_config(0.0, 6.0, 12.0, 5.0 / 3.0, 100.0));
	const auto center = schwarzschild.evaluate_cartesian(12.0, 0.0, 0.0);
	suite.check(center.rho > 99.0 && center.p > 0.0, "the Cartesian evaluation at the pressure maximum returns the peak density and a positive pressure");
	suite.check(schwarzschild.evaluate_cartesian(1.0, 0.0, 0.0).rho < 1e-10, "the Cartesian evaluation inside the cusp returns a negligible density");
	return suite.finish();
}

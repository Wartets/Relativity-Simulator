#include "relativistic/hydro/fishbone_moncrief.hpp"
#include "relativistic/core/constants.hpp"
#include <iostream>
#include <cassert>
#include <cmath>
#include <numbers>

void test_torus_equilibrium_and_cusp() {
	using namespace Relativistic::Hydro;
	using namespace Relativistic::Core;

	FishboneMoncriefConfig<double> cfg;
	cfg.mass = 1.0;
	cfg.speed_of_light = 1.0;
	cfg.gravitational_constant = 1.0;
	cfg.spin_parameter = 0.9;
	cfg.r_in = 4.5;
	cfg.r_center = 8.5;
	cfg.gamma = 4.0 / 3.0;
	cfg.max_density = 1000.0;

	FishboneMoncriefTorus<double> torus(cfg);

	const double theta_eq = std::numbers::pi_v<double> * 0.5;

	assert(torus.specific_angular_momentum() > 0.0);
	assert(torus.w_in() > torus.w_center());
	assert(torus.max_enthalpy() > 1.0);

	const double h_center = torus.specific_enthalpy(cfg.r_center, theta_eq);
	assert(std::abs(h_center - torus.max_enthalpy()) < 1e-12);

	const double rho_center = torus.density(cfg.r_center, theta_eq);
	assert(std::abs(rho_center - cfg.max_density) / cfg.max_density < 1e-10);

	const double h_in = torus.specific_enthalpy(cfg.r_in, theta_eq);
	assert(std::abs(h_in - 1.0) < 1e-12);

	assert(!torus.is_inside_torus(cfg.r_in * 0.9, theta_eq));
	assert(torus.density(cfg.r_in * 0.9, theta_eq) == 0.0);
	assert(torus.pressure(cfg.r_in * 0.9, theta_eq) == 0.0);

	assert(torus.is_inside_torus(cfg.r_center, theta_eq));
	assert(torus.density(cfg.r_center, theta_eq) > 0.0);
	assert(torus.pressure(cfg.r_center, theta_eq) > 0.0);
}

void test_torus_four_velocity_normalization() {
	using namespace Relativistic::Hydro;
	using namespace Relativistic::Core;

	FishboneMoncriefConfig<double> cfg;
	cfg.mass = 1.0;
	cfg.speed_of_light = 1.0;
	cfg.gravitational_constant = 1.0;
	cfg.spin_parameter = 0.5;
	cfg.r_in = 5.0;
	cfg.r_center = 10.0;
	cfg.gamma = 5.0 / 3.0;

	FishboneMoncriefTorus<double> torus(cfg);

	const double theta_eq = std::numbers::pi_v<double> * 0.5;

	for (double r = cfg.r_in + 0.1; r <= cfg.r_center * 1.5; r += 0.5) {
		if (torus.is_inside_torus(r, theta_eq)) {
			const auto u = torus.four_velocity(r, theta_eq);
			const auto g = torus.metric().metric_tensor(FourVector<double>(0.0, r, theta_eq, 0.0));

			double norm_sq = 0.0;
			for (size_t mu = 0; mu < 4; ++mu) {
				for (size_t nu = 0; nu < 4; ++nu) {
					norm_sq += g(mu, nu) * u(mu) * u(nu);
				}
			}
			assert(std::abs(norm_sq - (-1.0)) < 1e-10);
		}
	}
}

void test_polytropic_relation_and_enthalpy() {
	using namespace Relativistic::Hydro;
	using namespace Relativistic::Core;

	FishboneMoncriefConfig<double> cfg;
	cfg.mass = 1.0;
	cfg.speed_of_light = 1.0;
	cfg.gravitational_constant = 1.0;
	cfg.spin_parameter = 0.8;
	cfg.r_in = 4.8;
	cfg.r_center = 9.0;
	cfg.gamma = 4.0 / 3.0;
	cfg.max_density = 500.0;

	FishboneMoncriefTorus<double> torus(cfg);

	const double theta_eq = std::numbers::pi_v<double> * 0.5;
	const double k = torus.polytropic_constant();
	const double gamma = cfg.gamma;

	for (double r = cfg.r_in + 0.2; r < cfg.r_center * 1.8; r += 0.3) {
		if (torus.is_inside_torus(r, theta_eq)) {
			const double rho = torus.density(r, theta_eq);
			const double p = torus.pressure(r, theta_eq);
			const double expected_p = k * std::pow(rho, gamma);
			assert(std::abs(p - expected_p) / (p + 1e-30) < 1e-10);

			const double h = torus.specific_enthalpy(r, theta_eq);
			const double expected_h = 1.0 + (gamma / (gamma - 1.0)) * (p / rho);
			assert(std::abs(h - expected_h) / expected_h < 1e-10);
		}
	}
}

void test_cartesian_evaluation() {
	using namespace Relativistic::Hydro;
	using namespace Relativistic::Core;

	FishboneMoncriefConfig<double> cfg;
	cfg.mass = 1.0;
	cfg.speed_of_light = 1.0;
	cfg.gravitational_constant = 1.0;
	cfg.spin_parameter = 0.0;
	cfg.r_in = 6.0;
	cfg.r_center = 12.0;
	cfg.gamma = 5.0 / 3.0;
	cfg.max_density = 100.0;

	FishboneMoncriefTorus<double> torus(cfg);

	const auto prim_center = torus.evaluate_cartesian(12.0, 0.0, 0.0);
	assert(prim_center.rho > 99.0);
	assert(prim_center.p > 0.0);

	const auto prim_outside = torus.evaluate_cartesian(1.0, 0.0, 0.0);
	assert(prim_outside.rho < 1e-10);
}

int main() {
	test_torus_equilibrium_and_cusp();
	test_torus_four_velocity_normalization();
	test_polytropic_relation_and_enthalpy();
	test_cartesian_evaluation();
	return 0;
}

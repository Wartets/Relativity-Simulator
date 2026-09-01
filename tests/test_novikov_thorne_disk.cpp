#include "relativistic/hydro/novikov_thorne.hpp"
#include "relativistic/core/constants.hpp"
#include <iostream>
#include <cassert>
#include <cmath>
#include <vector>

void test_isco_radius() {
	using namespace Relativistic::Hydro;
	using namespace Relativistic::Core;

	NovikovThorneConfig<double> cfg_schwarzschild;
	cfg_schwarzschild.mass = PhysicalConstants<double>::SOLAR_MASS;
	cfg_schwarzschild.spin_parameter = 0.0;
	NovikovThorneDisk<double> disk_schw(cfg_schwarzschild);

	const double r_g = PhysicalConstants<double>::GRAVITATIONAL_CONSTANT * cfg_schwarzschild.mass / (PhysicalConstants<double>::SPEED_OF_LIGHT * PhysicalConstants<double>::SPEED_OF_LIGHT);
	const double isco_schw = disk_schw.isco_radius();
	assert(std::abs(isco_schw - 6.0 * r_g) / (6.0 * r_g) < 1e-12);

	NovikovThorneConfig<double> cfg_kerr_prograde;
	cfg_kerr_prograde.mass = PhysicalConstants<double>::SOLAR_MASS;
	cfg_kerr_prograde.spin_parameter = 0.998 * r_g;
	NovikovThorneDisk<double> disk_pro(cfg_kerr_prograde);

	const double isco_pro = disk_pro.isco_radius();
	assert(isco_pro < 1.3 * r_g && isco_pro > 1.0 * r_g);

	NovikovThorneConfig<double> cfg_kerr_retrograde;
	cfg_kerr_retrograde.mass = PhysicalConstants<double>::SOLAR_MASS;
	cfg_kerr_retrograde.spin_parameter = -0.9 * r_g;
	NovikovThorneDisk<double> disk_retro(cfg_kerr_retrograde);

	const double isco_retro = disk_retro.isco_radius();
	assert(isco_retro > 8.0 * r_g && isco_retro < 9.5 * r_g);
}

void test_page_thorne_analytical_vs_numerical() {
	using namespace Relativistic::Hydro;
	using namespace Relativistic::Core;

	const double r_g = 1.0;
	const std::vector<double> spins = {0.0, 0.2, 0.5, 0.8, 0.95, -0.3, -0.7};

	for (double a_star : spins) {
		NovikovThorneConfig<double> cfg;
		cfg.mass = 1.0;
		cfg.speed_of_light = 1.0;
		cfg.gravitational_constant = 1.0;
		cfg.spin_parameter = a_star * r_g;
		NovikovThorneDisk<double> disk(cfg);

		const double r_isco = disk.isco_radius();
		const double x0 = std::sqrt(r_isco);

		assert(disk.page_thorne_f(x0) == 0.0);

		for (double r_mult = 1.05; r_mult <= 50.0; r_mult *= 1.5) {
			const double r = r_isco * r_mult;
			const double x = std::sqrt(r);

			const double f_ana = disk.page_thorne_f(x);
			const double f_num = disk.numerical_integral_f(x, 10000);

			const double rel_diff = std::abs(f_ana - f_num) / std::max(f_num, 1e-15);
			assert(rel_diff < 1e-7);
		}
	}
}

void test_radiative_efficiency() {
	using namespace Relativistic::Hydro;
	using namespace Relativistic::Core;

	NovikovThorneConfig<double> cfg_schw;
	cfg_schw.mass = 1.0;
	cfg_schw.speed_of_light = 1.0;
	cfg_schw.gravitational_constant = 1.0;
	cfg_schw.spin_parameter = 0.0;
	NovikovThorneDisk<double> disk_schw(cfg_schw);

	const double eta_schw = disk_schw.radiative_efficiency();
	const double expected_schw = 1.0 - std::sqrt(8.0 / 9.0);
	assert(std::abs(eta_schw - expected_schw) < 1e-10);

	NovikovThorneConfig<double> cfg_kerr;
	cfg_kerr.mass = 1.0;
	cfg_kerr.speed_of_light = 1.0;
	cfg_kerr.gravitational_constant = 1.0;
	cfg_kerr.spin_parameter = 0.998;
	NovikovThorneDisk<double> disk_kerr(cfg_kerr);

	const double eta_kerr = disk_kerr.radiative_efficiency();
	assert(eta_kerr > 0.30 && eta_kerr < 0.35);
}

void test_four_velocity_normalization() {
	using namespace Relativistic::Hydro;
	using namespace Relativistic::Core;

	NovikovThorneConfig<double> cfg;
	cfg.mass = 10.0;
	cfg.spin_parameter = 5.0;
	cfg.speed_of_light = 1.0;
	cfg.gravitational_constant = 1.0;
	NovikovThorneDisk<double> disk(cfg);

	const double r_isco = disk.isco_radius();
	for (double r = r_isco * 1.01; r <= r_isco * 20.0; r += r_isco) {
		const auto u = disk.four_velocity(r);
		const auto g = disk.metric().metric_tensor(FourVector<double>(0.0, r, std::numbers::pi_v<double> * 0.5, 0.0));

		double norm_sq = 0.0;
		for (size_t mu = 0; mu < 4; ++mu) {
			for (size_t nu = 0; nu < 4; ++nu) {
				norm_sq += g(mu, nu) * u(mu) * u(nu);
			}
		}
		assert(std::abs(norm_sq - (-1.0)) < 1e-10);
	}
}

void test_flux_and_temperature_profile() {
	using namespace Relativistic::Hydro;
	using namespace Relativistic::Core;

	NovikovThorneConfig<double> cfg;
	cfg.mass = 10.0 * PhysicalConstants<double>::SOLAR_MASS;
	cfg.spin_parameter = 0.5 * (PhysicalConstants<double>::GRAVITATIONAL_CONSTANT * cfg.mass / (PhysicalConstants<double>::SPEED_OF_LIGHT * PhysicalConstants<double>::SPEED_OF_LIGHT));
	cfg.accretion_rate = 1e15;
	NovikovThorneDisk<double> disk(cfg);

	const double r_isco = disk.isco_radius();

	assert(disk.radiative_flux(r_isco * 0.9) == 0.0);
	assert(disk.effective_temperature(r_isco * 0.9) == 0.0);

	double max_flux = 0.0;
	double r_at_max = 0.0;

	for (double r = r_isco; r <= 30.0 * r_isco; r += 0.05 * r_isco) {
		const double f = disk.radiative_flux(r);
		const double t = disk.effective_temperature(r);
		if (r > r_isco) {
			assert(f > 0.0);
			assert(t > 0.0);
		}
		if (f > max_flux) {
			max_flux = f;
			r_at_max = r;
		}
	}

	assert(r_at_max > r_isco && r_at_max < 3.0 * r_isco);
}

int main() {
	test_isco_radius();
	test_page_thorne_analytical_vs_numerical();
	test_radiative_efficiency();
	test_four_velocity_normalization();
	test_flux_and_temperature_profile();
	return 0;
}

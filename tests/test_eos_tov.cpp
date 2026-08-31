#include "relativistic/hydro/eos.hpp"
#include "relativistic/hydro/tov_solver.hpp"
#include "relativistic/core/constants.hpp"
#include <iostream>
#include <cassert>
#include <cmath>
#include <vector>

void test_ideal_gas_eos() {
	using namespace Relativistic::Hydro;
	IdealGasEOS<double> eos(5.0 / 3.0);

	const double rho = 1.0e3;
	const double eps = 0.05;
	const double p = eos.pressure(rho, eps);
	const double expected_p = (5.0 / 3.0 - 1.0) * rho * eps;
	assert(std::abs(p - expected_p) < 1e-12);

	const double eps_rec = eos.specific_internal_energy(rho, p);
	assert(std::abs(eps_rec - eps) < 1e-12);

	const double h = eos.specific_enthalpy(rho, p);
	const double expected_h = 1.0 + eps + p / rho;
	assert(std::abs(h - expected_h) < 1e-12);

	const double cs2 = eos.sound_speed_squared(rho, p);
	const double expected_cs2 = (5.0 / 3.0 * p) / (rho * h);
	assert(std::abs(cs2 - expected_cs2) < 1e-12);
	assert(cs2 > 0.0 && cs2 < 5.0 / 3.0 - 1.0);
}

void test_synge_mathews_eos() {
	using namespace Relativistic::Hydro;
	SyngeEOS<double> synge;
	MathewsEOS<double> mathews;

	const double rho = 1.0;
	const double p_cold = 1e-5;
	const double h_synge_cold = synge.specific_enthalpy(rho, p_cold);
	const double expected_h_nr = 1.0 + 2.5 * (p_cold / rho);
	assert(std::abs(h_synge_cold - expected_h_nr) < 1e-4);

	const double p_hot = 100.0;
	const double h_synge_hot = synge.specific_enthalpy(rho, p_hot);
	const double theta_hot = p_hot / rho;
	const double expected_h_ur = 4.0 * theta_hot;
	assert(std::abs(h_synge_hot - expected_h_ur) / expected_h_ur < 0.05);

	const double cs2_cold = mathews.sound_speed_squared(rho, p_cold);
	assert(cs2_cold > 0.0 && cs2_cold < 1.0 / 3.0);

	const double cs2_hot = mathews.sound_speed_squared(rho, 1e6);
	assert(std::abs(cs2_hot - 1.0 / 3.0) < 1e-4);
}

void test_relativistic_fermi_gas() {
	using namespace Relativistic::Hydro;

	RelativisticFermiGasEOS<double> neutron_gas(FermiParticleType::Neutron);

	const double x = 1.5;
	const double rho = neutron_gas.rho0_scale() * x * x * x;
	const double p = neutron_gas.pressure(rho);
	const double eps = neutron_gas.energy_density(rho);
	assert(eps > p);
	const double cs2 = neutron_gas.sound_speed_squared(rho, p);
	const double h = neutron_gas.specific_enthalpy(rho, p);

	const double expected_h = std::sqrt(1.0 + x * x);
	assert(std::abs(h - expected_h) < 1e-12);

	const double c = Relativistic::Core::PhysicalConstants<double>::SPEED_OF_LIGHT;
	const double expected_cs2 = (c * c * x * x) / (3.0 * (1.0 + x * x));
	assert(std::abs(cs2 - expected_cs2) < 1e-6);

	const double rho_rec = neutron_gas.density_from_pressure(p);
	assert(std::abs(rho_rec - rho) / rho < 1e-8);

	const double p_h = neutron_gas.pressure_from_enthalpy(rho, h);
	assert(std::abs(p_h - p) / p < 1e-8);
}

void test_tabulated_nuclear_eos_serialization() {
	using namespace Relativistic::Hydro;

	const auto sfho = TabulatedNuclearEOS<double>::create_preset(NuclearPreset::SFHo, 200, 10, 10);
	assert(sfho.model_name() == "SFHo");

	const double test_rho = 3.0e17;
	const double p_orig = sfho.pressure(test_rho);
	const double cs2_orig = sfho.sound_speed_squared(test_rho);
	const double c = Relativistic::Core::PhysicalConstants<double>::SPEED_OF_LIGHT;
	assert(p_orig > 0.0);
	assert(cs2_orig > 0.0 && cs2_orig <= c * c);

	const auto container = sfho.to_hdf5_container();
	const auto opt_reloaded = TabulatedNuclearEOS<double>::from_hdf5_container(container);
	assert(opt_reloaded.has_value());

	const auto& reloaded = *opt_reloaded;
	const double p_reloaded = reloaded.pressure(test_rho);
	const double cs2_reloaded = reloaded.sound_speed_squared(test_rho);

	assert(std::abs(p_reloaded - p_orig) / p_orig < 1e-10);
	assert(std::abs(cs2_reloaded - cs2_orig) < 1e-10);
}

void test_tov_white_dwarf_fermi_gas() {
	using namespace Relativistic::Hydro;

	RelativisticFermiGasEOS<double> electron_gas(FermiParticleType::Electron, 2.0);
	TOVConfig<double> cfg;
	cfg.initial_step_meters = 10.0;
	cfg.max_step_meters = 20000.0;
	cfg.max_radius_meters = 5.0e7;

	TOVSolver<RelativisticFermiGasEOS<double>, double> solver(electron_gas, cfg);

	const double central_rho = 1.0e12;
	const auto profile = solver.solve_star_from_central_density(central_rho);

	assert(profile.converged);
	assert(profile.gravitational_mass_solar > 0.5 && profile.gravitational_mass_solar < 1.44);
	assert(profile.surface_radius_km > 1000.0 && profile.surface_radius_km < 20000.0);
	assert(profile.compactness < 1e-3);
}

void test_tov_free_neutron_gas() {
	using namespace Relativistic::Hydro;

	RelativisticFermiGasEOS<double> neutron_gas(FermiParticleType::Neutron);
	TOVConfig<double> cfg;
	cfg.initial_step_meters = 1.0;
	cfg.max_step_meters = 20.0;
	cfg.max_radius_meters = 50000.0;

	TOVSolver<RelativisticFermiGasEOS<double>, double> solver(neutron_gas, cfg);

	const auto curve = solver.compute_mass_radius_curve(1.0e17, 1.0e19, 40);

	assert(curve.maximum_mass_solar >= 0.65 && curve.maximum_mass_solar <= 0.80);
	assert(curve.radius_at_maximum_mass_km >= 8.0 && curve.radius_at_maximum_mass_km <= 12.0);
}

void test_tov_sfho_maximum_mass() {
	using namespace Relativistic::Hydro;

	const auto sfho = TabulatedNuclearEOS<double>::create_preset(NuclearPreset::SFHo, 1600, 5, 5);
	TOVConfig<double> cfg;
	cfg.initial_step_meters = 2.0;
	cfg.max_step_meters = 30.0;
	cfg.max_radius_meters = 100000.0;

	TOVSolver<TabulatedNuclearEOS<double>, double> solver(sfho, cfg);

	const auto curve = solver.compute_mass_radius_curve(4.0e17, 4.5e18, 70);

	assert(curve.maximum_mass_solar >= 1.95 && curve.maximum_mass_solar <= 2.15);
	assert(curve.radius_at_maximum_mass_km >= 9.0 && curve.radius_at_maximum_mass_km <= 13.0);
	assert(curve.radius_at_1_4_solar_km >= 10.5 && curve.radius_at_1_4_solar_km <= 14.0);
}

void test_tov_shen_ls220_presets() {
	using namespace Relativistic::Hydro;

	const auto shen = TabulatedNuclearEOS<double>::create_preset(NuclearPreset::Shen, 1600, 5, 5);
	const auto ls220 = TabulatedNuclearEOS<double>::create_preset(NuclearPreset::LS220, 1600, 5, 5);

	TOVConfig<double> cfg;
	cfg.initial_step_meters = 2.0;
	cfg.max_step_meters = 30.0;
	cfg.max_radius_meters = 100000.0;

	TOVSolver<TabulatedNuclearEOS<double>, double> solver_shen(shen, cfg);
	TOVSolver<TabulatedNuclearEOS<double>, double> solver_ls220(ls220, cfg);

	const auto curve_shen = solver_shen.compute_mass_radius_curve(4.0e17, 4.0e18, 70);
	const auto curve_ls220 = solver_ls220.compute_mass_radius_curve(4.0e17, 4.5e18, 70);

	assert(curve_shen.maximum_mass_solar >= 2.05 && curve_shen.maximum_mass_solar <= 2.30);
	assert(curve_ls220.maximum_mass_solar >= 1.95 && curve_ls220.maximum_mass_solar <= 2.15);
	assert(curve_shen.maximum_mass_solar > curve_ls220.maximum_mass_solar);
}

int main() {
	test_ideal_gas_eos();
	test_synge_mathews_eos();
	test_relativistic_fermi_gas();
	test_tabulated_nuclear_eos_serialization();
	test_tov_white_dwarf_fermi_gas();
	test_tov_free_neutron_gas();
	test_tov_sfho_maximum_mass();
	test_tov_shen_ls220_presets();

	std::cout << "All EoS and TOV tests passed successfully." << std::endl;
	return 0;
}

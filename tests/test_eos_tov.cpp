#include "support/test_harness.hpp"
#include "relativistic/hydro/eos.hpp"
#include "relativistic/hydro/solvers/tov_solver.hpp"
#include "relativistic/core/constants.hpp"
#include <cmath>

using namespace Relativistic::Hydro;
using Relativistic::Testing::TestSuite;
using Relativistic::Core::PhysicalConstants;

namespace {

void check_ideal_gas(TestSuite& suite) {
	suite.section("Ideal gas");
	IdealGasEOS<double> eos(5.0 / 3.0);
	const double rho = 1.0e3;
	const double eps = 0.05;
	const double p = eos.pressure(rho, eps);
	suite.check_near(p, (5.0 / 3.0 - 1.0) * rho * eps, 1e-12, "the pressure equals (Gamma - 1) rho eps");
	suite.check_near(eos.specific_internal_energy(rho, p), eps, 1e-12, "the specific internal energy is recovered");
	const double h = eos.specific_enthalpy(rho, p);
	suite.check_near(h, 1.0 + eps + p / rho, 1e-12, "the specific enthalpy equals 1 + eps + p / rho");
	const double cs2 = eos.sound_speed_squared(rho, p);
	suite.check_near(cs2, (5.0 / 3.0 * p) / (rho * h), 1e-12, "the sound speed squared equals Gamma p / (rho h)");
	suite.check(cs2 > 0.0 && cs2 < 5.0 / 3.0 - 1.0, "the sound speed squared is positive and below Gamma - 1");
}

void check_synge_mathews(TestSuite& suite) {
	suite.section("Synge and Mathews gases");
	SyngeEOS<double> synge;
	MathewsEOS<double> mathews;
	const double rho = 1.0;
	suite.check_near(synge.specific_enthalpy(rho, 1e-5), 1.0 + 2.5 * 1e-5, 1e-4, "the cold Synge enthalpy approaches the non-relativistic limit 1 + 5/2 theta");
	const double hot_expected = 4.0 * 100.0;
	suite.check_relative(synge.specific_enthalpy(rho, 100.0), hot_expected, 0.05, "the hot Synge enthalpy approaches the ultra-relativistic limit 4 theta within 5 percent");
	const double cold_speed = mathews.sound_speed_squared(rho, 1e-5);
	suite.check(cold_speed > 0.0 && cold_speed < 1.0 / 3.0, "the cold Mathews sound speed squared lies in (0, 1/3)");
	suite.check_near(mathews.sound_speed_squared(rho, 1e6), 1.0 / 3.0, 1e-4, "the hot Mathews sound speed squared approaches 1/3");
}

void check_fermi_gas(TestSuite& suite) {
	suite.section("Relativistic degenerate Fermi gas");
	RelativisticFermiGasEOS<double> neutron_gas(FermiParticleType::Neutron);
	const double x = 1.5;
	const double rho = neutron_gas.rho0_scale() * x * x * x;
	const double p = neutron_gas.pressure(rho);
	suite.check(neutron_gas.energy_density(rho) > p, "the energy density exceeds the pressure");
	const double h = neutron_gas.specific_enthalpy(rho, p);
	suite.check_near(h, std::sqrt(1.0 + x * x), 1e-12, "the specific enthalpy equals sqrt(1 + x^2)");
	const double c = PhysicalConstants<double>::SPEED_OF_LIGHT;
	suite.check_near(neutron_gas.sound_speed_squared(rho, p), (c * c * x * x) / (3.0 * (1.0 + x * x)), 1e-6, "the sound speed squared equals c^2 x^2 / (3 (1 + x^2))");
	suite.check_relative(neutron_gas.density_from_pressure(p), rho, 1e-8, "the density is recovered from the pressure within a relative 1e-8");
	suite.check_relative(neutron_gas.pressure_from_enthalpy(rho, h), p, 1e-8, "the pressure is recovered from the enthalpy within a relative 1e-8");
}

void check_tabulated(TestSuite& suite) {
	suite.section("Tabulated nuclear equation of state");
	const auto sfho = TabulatedNuclearEOS<double>::create_preset(NuclearPreset::SFHo, 200, 10, 10);
	suite.check(sfho.model_name() == "SFHo", "the preset carries the SFHo name");
	const double density = 3.0e17;
	const double pressure = sfho.pressure(density);
	const double speed = sfho.sound_speed_squared(density);
	const double c = PhysicalConstants<double>::SPEED_OF_LIGHT;
	suite.check(pressure > 0.0, "the pressure at 3e17 kg/m^3 is positive");
	suite.check(speed > 0.0 && speed <= c * c, "the sound speed squared is positive and causal");
	const auto reloaded = TabulatedNuclearEOS<double>::from_hdf5_container(sfho.to_hdf5_container());
	suite.check(reloaded.has_value(), "the table survives the container round trip");
	if (reloaded.has_value()) {
		suite.check_relative(reloaded->pressure(density), pressure, 1e-10, "the pressure survives the round trip within a relative 1e-10");
		suite.check_near(reloaded->sound_speed_squared(density), speed, 1e-10, "the sound speed survives the round trip within 1e-10");
	}
}

void check_stellar_models(TestSuite& suite) {
	suite.section("White dwarf");
	{
		RelativisticFermiGasEOS<double> electron_gas(FermiParticleType::Electron, 2.0);
		TOVConfig<double> config;
		config.initial_step_meters = 10.0;
		config.max_step_meters = 20000.0;
		config.max_radius_meters = 5.0e7;
		TOVSolver<RelativisticFermiGasEOS<double>, double> solver(electron_gas, config);
		const auto profile = solver.solve_star_from_central_density(1.0e12);
		suite.check(profile.converged, "the white dwarf integration converges");
		suite.check(profile.gravitational_mass_solar > 0.5 && profile.gravitational_mass_solar < 1.44, "the white dwarf mass lies between 0.5 and the Chandrasekhar limit 1.44 solar masses", "mass=" + std::to_string(profile.gravitational_mass_solar));
		suite.check(profile.surface_radius_km > 1000.0 && profile.surface_radius_km < 20000.0, "the white dwarf radius lies between 1000 and 20000 km", "radius=" + std::to_string(profile.surface_radius_km));
		suite.check(profile.compactness < 1e-3, "the white dwarf compactness is below 1e-3");
	}
	suite.section("Free neutron gas");
	{
		RelativisticFermiGasEOS<double> neutron_gas(FermiParticleType::Neutron);
		TOVConfig<double> config;
		config.initial_step_meters = 1.0;
		config.max_step_meters = 20.0;
		config.max_radius_meters = 50000.0;
		TOVSolver<RelativisticFermiGasEOS<double>, double> solver(neutron_gas, config);
		const auto curve = solver.compute_mass_radius_curve(1.0e17, 1.0e19, 40);
		suite.measure("free_neutron_maximum_mass", curve.maximum_mass_solar, "solar masses");
		suite.check(curve.maximum_mass_solar >= 0.65 && curve.maximum_mass_solar <= 0.80, "the maximum mass of a free neutron gas lies near the Oppenheimer-Volkoff value of 0.7 solar masses");
		suite.check(curve.radius_at_maximum_mass_km >= 8.0 && curve.radius_at_maximum_mass_km <= 12.0, "the radius at the maximum mass lies between 8 and 12 km");
	}
	suite.section("Nuclear equations of state");
	{
		TOVConfig<double> config;
		config.initial_step_meters = 2.0;
		config.max_step_meters = 30.0;
		config.max_radius_meters = 100000.0;
		const auto sfho = TabulatedNuclearEOS<double>::create_preset(NuclearPreset::SFHo, 1600, 5, 5);
		TOVSolver<TabulatedNuclearEOS<double>, double> sfho_solver(sfho, config);
		const auto sfho_curve = sfho_solver.compute_mass_radius_curve(4.0e17, 4.5e18, 70);
		suite.measure("sfho_maximum_mass", sfho_curve.maximum_mass_solar, "solar masses");
		suite.check(sfho_curve.maximum_mass_solar >= 1.95 && sfho_curve.maximum_mass_solar <= 2.15, "the SFHo maximum mass lies between 1.95 and 2.15 solar masses");
		suite.check(sfho_curve.radius_at_maximum_mass_km >= 9.0 && sfho_curve.radius_at_maximum_mass_km <= 13.0, "the SFHo radius at the maximum mass lies between 9 and 13 km");
		suite.check(sfho_curve.radius_at_1_4_solar_km >= 10.5 && sfho_curve.radius_at_1_4_solar_km <= 14.0, "the SFHo radius of a 1.4 solar mass star lies between 10.5 and 14 km");

		const auto shen = TabulatedNuclearEOS<double>::create_preset(NuclearPreset::Shen, 1600, 5, 5);
		const auto ls220 = TabulatedNuclearEOS<double>::create_preset(NuclearPreset::LS220, 1600, 5, 5);
		TOVSolver<TabulatedNuclearEOS<double>, double> shen_solver(shen, config);
		TOVSolver<TabulatedNuclearEOS<double>, double> ls220_solver(ls220, config);
		const auto shen_curve = shen_solver.compute_mass_radius_curve(4.0e17, 4.0e18, 70);
		const auto ls220_curve = ls220_solver.compute_mass_radius_curve(4.0e17, 4.5e18, 70);
		suite.measure("shen_maximum_mass", shen_curve.maximum_mass_solar, "solar masses");
		suite.measure("ls220_maximum_mass", ls220_curve.maximum_mass_solar, "solar masses");
		suite.check(shen_curve.maximum_mass_solar >= 2.05 && shen_curve.maximum_mass_solar <= 2.30, "the Shen maximum mass lies between 2.05 and 2.30 solar masses");
		suite.check(ls220_curve.maximum_mass_solar >= 1.95 && ls220_curve.maximum_mass_solar <= 2.15, "the LS220 maximum mass lies between 1.95 and 2.15 solar masses");
		suite.check(shen_curve.maximum_mass_solar > ls220_curve.maximum_mass_solar, "the stiffer Shen equation of state supports a larger maximum mass than LS220");
	}
}

}

int main() {
	TestSuite suite(
		"eos_tov",
		"Verifies the equations of state and the stellar structure solver: ideal gas, Synge and Mathews relations and limits, the degenerate Fermi gas identities, the tabulated nuclear table round trip, and Tolman-Oppenheimer-Volkoff stars for a white dwarf, a free neutron gas and the SFHo, Shen and LS220 tables."
	);
	check_ideal_gas(suite);
	check_synge_mathews(suite);
	check_fermi_gas(suite);
	check_tabulated(suite);
	check_stellar_models(suite);
	return suite.finish();
}

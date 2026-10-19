#include "support/test_harness.hpp"
#include "relativistic/optics/stokes_vector.hpp"
#include "relativistic/optics/maxwell_juttner.hpp"
#include "relativistic/optics/radiative_processes.hpp"
#include "relativistic/optics/polarized_radiative_transfer.hpp"
#include "relativistic/optics/inverse_compton.hpp"
#include "relativistic/core/pcg64.hpp"
#include <cmath>
#include <numbers>
#include <vector>

using namespace Relativistic::Optics;
using Relativistic::Testing::TestSuite;

namespace {

void check_stokes(TestSuite& suite) {
	suite.section("Stokes vector");
	StokesVector<double> s(10.0, 6.0, 8.0, 0.0);
	suite.check(s.is_physically_valid(), "a fully linearly polarized vector is physically valid");
	suite.check_near(s.intensity(), 10.0, 1e-12, "the intensity is the first component");
	suite.check_near(s.linear_polarization_intensity(), 10.0, 1e-12, "the linearly polarized intensity equals sqrt(Q^2 + U^2)");
	suite.check_near(s.fractional_linear_polarization(), 1.0, 1e-12, "the fractional linear polarization equals 1");
	suite.check_near(s.electric_vector_position_angle(), 0.5 * std::atan2(8.0, 6.0), 1e-12, "the electric vector position angle equals half of atan2(U, Q)");
	const auto rotated = s.rotate_reference_frame(std::numbers::pi_v<double> / 4.0);
	suite.check_near(rotated.intensity(), 10.0, 1e-12, "the intensity is invariant under a frame rotation");
	suite.check_near(rotated.q, 8.0, 1e-12, "a quarter-turn rotation of the position angle exchanges Q and U");
	suite.check_near(rotated.u, -6.0, 1e-12, "a quarter-turn rotation of the position angle changes the sign of the old Q");
}

void check_thermal_distribution(TestSuite& suite) {
	suite.section("Bessel functions and Maxwell-Juttner distribution");
	const double k0 = RelativisticBessel::k0(1.0);
	const double k1 = RelativisticBessel::k1(1.0);
	const double k2 = RelativisticBessel::k2(1.0);
	suite.check(k0 > 0.4 && k0 < 0.5, "K0(1) lies between 0.4 and 0.5");
	suite.check(k1 > 0.5 && k1 < 0.7, "K1(1) lies between 0.5 and 0.7");
	suite.check(k2 > 1.5 && k2 < 1.8, "K2(1) lies between 1.5 and 1.8");
	suite.check_near(k2, 2.0 * k1 + k0, 1e-10, "the recurrence K2(x) = 2 K1(x) / x + K0(x) holds at x = 1");

	MaxwellJuttnerDistribution distribution(1.0);
	const double mean = distribution.mean_lorentz_factor();
	suite.check(mean > 3.0 && mean < 4.0, "the mean Lorentz factor at theta = 1 lies between 3 and 4");
	Relativistic::Core::PCG64Engine rng(42, 1);
	constexpr size_t samples = 20000;
	double sum = 0.0;
	bool valid = true;
	for (size_t i = 0; i < samples; ++i) {
		const double gamma = distribution.sample_lorentz_factor(rng);
		valid = valid && gamma >= 1.0;
		sum += gamma;
	}
	suite.check(valid, "every sampled Lorentz factor is at least 1");
	suite.check_near(sum / static_cast<double>(samples), mean, 0.1, "the sample mean agrees with the analytic mean within 0.1");
}

void check_synchrotron(TestSuite& suite) {
	suite.section("Synchrotron emission");
	PolarizedPlasmaState<double> plasma;
	plasma.electron_density = 1e18;
	plasma.ion_density = 1e18;
	plasma.electron_temperature_k = 1e8;
	plasma.magnetic_field_tesla = 0.1;
	plasma.pitch_angle_rad = std::numbers::pi_v<double> / 3.0;
	plasma.non_thermal_fraction = 1.0;
	plasma.power_law_index = 3.0;
	const auto emissivity_3 = RadiativeProcessEngine<double>::non_thermal_synchrotron_emissivity(1e10, plasma);
	const double polarization_3 = emissivity_3.j_q / emissivity_3.j_i;
	suite.check_near(polarization_3, (3.0 + 1.0) / (3.0 + 7.0 / 3.0), 1e-12, "the linear polarization of a power law with p = 3 equals (p + 1) / (p + 7/3)");
	suite.check_near(polarization_3, 0.75, 1e-12, "the linear polarization for p = 3 equals 0.75");
	plasma.power_law_index = 2.0;
	const auto emissivity_2 = RadiativeProcessEngine<double>::non_thermal_synchrotron_emissivity(1e10, plasma);
	const double polarization_2 = emissivity_2.j_q / emissivity_2.j_i;
	suite.check_near(polarization_2, 9.0 / 13.0, 1e-12, "the linear polarization for p = 2 equals 9/13");
	const auto absorptivity = RadiativeProcessEngine<double>::non_thermal_synchrotron_absorptivity(1e10, plasma);
	suite.check(absorptivity.alpha_i > 0.0 && absorptivity.alpha_q > 0.0, "the absorption coefficients are positive");
	suite.check_near(absorptivity.alpha_q / absorptivity.alpha_i, polarization_2, 1e-12, "the absorption polarization equals the emission polarization for a power law");
}

void check_bremsstrahlung(TestSuite& suite) {
	suite.section("Thermal bremsstrahlung");
	suite.check(RadiativeProcessEngine<double>::thermal_bremsstrahlung_emissivity(1e14, 1e20, 1e20, 1e7, 1.0) > 0.0, "the bremsstrahlung emissivity is positive");
	const double gaunt = RadiativeProcessEngine<double>::thermal_gaunt_factor(1e14, 1e7);
	suite.check(gaunt > 0.5 && gaunt < 20.0, "the thermal Gaunt factor lies between 0.5 and 20", "gaunt=" + std::to_string(gaunt));
}

void check_transfer(TestSuite& suite) {
	suite.section("Polarized transfer");
	StokesVector<double> initial(0.0, 0.0, 0.0, 0.0);
	StokesEmissivity<double> emissivity(1.0, 0.7, 0.0, 0.0);
	StokesTransferMatrix<double> matrix(0.1, 0.07, 0.0, 0.0, 0.0, 0.0, 0.5);
	const auto analytic = PolarizedRadiativeTransfer<double>::step_delano_analytical(initial, emissivity, matrix, 10.0);
	suite.check(analytic.is_physically_valid(), "the analytic Delano step yields a physically valid Stokes vector");
	suite.check(analytic.i > 0.0, "the analytic step builds up positive intensity");
	suite.check(analytic.fractional_linear_polarization() > 0.0, "the analytic step builds up linear polarization");
	suite.check(PolarizedRadiativeTransfer<double>::step_rk4(initial, emissivity, matrix, 0.1).i > 0.0, "the Runge-Kutta step builds up positive intensity");
}

void check_inverse_compton(TestSuite& suite) {
	suite.section("Inverse Compton scattering");
	using Relativistic::Core::PhysicalConstants;
	const double electron_energy = PhysicalConstants<double>::ELECTRON_MASS * PhysicalConstants<double>::SPEED_OF_LIGHT * PhysicalConstants<double>::SPEED_OF_LIGHT;
	const double low_energy_cross_section = InverseComptonEngine::klein_nishina_total_cross_section(1e-5 * electron_energy);
	suite.check_relative(low_energy_cross_section, 6.6524587321e-29, 1e-3, "the Klein-Nishina cross section tends to the Thomson value at low energy within a relative 1e-3");
	suite.check(InverseComptonEngine::compton_y_parameter(1.0, 0.1) > 0.0, "the Compton y parameter is positive");

	std::vector<PhotonPacket> seed;
	for (size_t i = 0; i < 500; ++i) {
		PhotonPacket packet;
		packet.energy_joules = 1e-20;
		seed.push_back(packet);
	}
	Relativistic::Core::PCG64Engine rng(1234, 5678);
	const auto scattered = InverseComptonEngine::simulate_los_compton_scattering(seed, 2.0, 0.5, rng);
	suite.check(scattered.size() == seed.size(), "scattering preserves the number of photon packets");
	double energy_in = 0.0;
	double energy_out = 0.0;
	for (size_t i = 0; i < seed.size(); ++i) {
		energy_in += seed[i].energy_joules;
		energy_out += scattered[i].energy_joules;
	}
	suite.check(energy_out > energy_in, "scattering off hot electrons raises the mean photon energy");
}

}

int main() {
	TestSuite suite(
		"polarized_radiative_transfer",
		"Verifies polarized radiative transfer: Stokes vector invariants and frame rotation, Bessel functions and the Maxwell-Juttner distribution, the polarization degree of power-law synchrotron emission and absorption, thermal bremsstrahlung and its Gaunt factor, the analytic and Runge-Kutta transfer steps, and inverse Compton scattering against the Thomson limit."
	);
	check_stokes(suite);
	check_thermal_distribution(suite);
	check_synchrotron(suite);
	check_bremsstrahlung(suite);
	check_transfer(suite);
	check_inverse_compton(suite);
	return suite.finish();
}

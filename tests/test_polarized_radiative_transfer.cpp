#include "relativistic/optics/stokes_vector.hpp"
#include "relativistic/optics/maxwell_juttner.hpp"
#include "relativistic/optics/radiative_processes.hpp"
#include "relativistic/optics/polarized_radiative_transfer.hpp"
#include "relativistic/optics/inverse_compton.hpp"
#include "relativistic/core/pcg64.hpp"
#include <cassert>
#include <iostream>
#include <cmath>
#include <numbers>

int main() {
	using namespace Relativistic::Optics;

	{
		StokesVector<double> s(10.0, 6.0, 8.0, 0.0);
		assert(s.is_physically_valid());
		assert(std::abs(s.intensity() - 10.0) < 1e-12);
		assert(std::abs(s.linear_polarization_intensity() - 10.0) < 1e-12);
		assert(std::abs(s.fractional_linear_polarization() - 1.0) < 1e-12);

		const double evpa = s.electric_vector_position_angle();
		const double expected_evpa = 0.5 * std::atan2(8.0, 6.0);
		assert(std::abs(evpa - expected_evpa) < 1e-12);

		const auto s_rot = s.rotate_reference_frame(std::numbers::pi / 4.0);
		assert(std::abs(s_rot.intensity() - 10.0) < 1e-12);
		assert(std::abs(s_rot.q - 8.0) < 1e-12);
		assert(std::abs(s_rot.u - (-6.0)) < 1e-12);
	}

	{
		const double k0_val = RelativisticBessel::k0(1.0);
		const double k1_val = RelativisticBessel::k1(1.0);
		const double k2_val = RelativisticBessel::k2(1.0);
		assert(k0_val > 0.4 && k0_val < 0.5);
		assert(k1_val > 0.5 && k1_val < 0.7);
		assert(k2_val > 1.5 && k2_val < 1.8);
		assert(std::abs(k2_val - (2.0 * k1_val + k0_val)) < 1e-10);

		MaxwellJuttnerDistribution mj(1.0);
		const double mean_g = mj.mean_lorentz_factor();
		assert(mean_g > 3.0 && mean_g < 4.0);

		Relativistic::Core::PCG64Engine rng(42, 1);
		double sum_gamma = 0.0;
		constexpr size_t samples = 20000;
		for (size_t i = 0; i < samples; ++i) {
			const double g = mj.sample_lorentz_factor(rng);
			assert(g >= 1.0);
			sum_gamma += g;
		}
		const double sample_mean = sum_gamma / static_cast<double>(samples);
		assert(std::abs(sample_mean - mean_g) < 0.1);
	}

	{
		PolarizedPlasmaState<double> plasma;
		plasma.electron_density = 1e18;
		plasma.ion_density = 1e18;
		plasma.electron_temperature_k = 1e8;
		plasma.magnetic_field_tesla = 0.1;
		plasma.pitch_angle_rad = std::numbers::pi / 3.0;
		plasma.non_thermal_fraction = 1.0;
		plasma.power_law_index = 3.0;

		const auto emis_p3 = RadiativeProcessEngine<double>::non_thermal_synchrotron_emissivity(1e10, plasma);
		const double pi_l_3 = emis_p3.j_q / emis_p3.j_i;
		const double expected_pi_l_3 = (3.0 + 1.0) / (3.0 + 7.0 / 3.0);
		assert(std::abs(pi_l_3 - expected_pi_l_3) < 1e-12);
		assert(std::abs(pi_l_3 - 0.75) < 1e-12);

		plasma.power_law_index = 2.0;
		const auto emis_p2 = RadiativeProcessEngine<double>::non_thermal_synchrotron_emissivity(1e10, plasma);
		const double pi_l_2 = emis_p2.j_q / emis_p2.j_i;
		const double expected_pi_l_2 = (2.0 + 1.0) / (2.0 + 7.0 / 3.0);
		assert(std::abs(pi_l_2 - expected_pi_l_2) < 1e-12);
		assert(std::abs(expected_pi_l_2 - (9.0 / 13.0)) < 1e-12);

		const auto abs_p2 = RadiativeProcessEngine<double>::non_thermal_synchrotron_absorptivity(1e10, plasma);
		assert(abs_p2.alpha_i > 0.0);
		assert(abs_p2.alpha_q > 0.0);
		assert(std::abs(abs_p2.alpha_q / abs_p2.alpha_i - expected_pi_l_2) < 1e-12);
	}

	{
		const double j_brems = RadiativeProcessEngine<double>::thermal_bremsstrahlung_emissivity(1e14, 1e20, 1e20, 1e7, 1.0);
		assert(j_brems > 0.0);
		const double gaunt = RadiativeProcessEngine<double>::thermal_gaunt_factor(1e14, 1e7);
		assert(gaunt > 0.5 && gaunt < 20.0);
	}

	{
		StokesVector<double> s_init(0.0, 0.0, 0.0, 0.0);
		StokesEmissivity<double> j(1.0, 0.7, 0.0, 0.0);
		StokesTransferMatrix<double> k(0.1, 0.07, 0.0, 0.0, 0.0, 0.0, 0.5);

		const auto s_out = PolarizedRadiativeTransfer<double>::step_delano_analytical(s_init, j, k, 10.0);
		assert(s_out.is_physically_valid());
		assert(s_out.i > 0.0);
		assert(s_out.fractional_linear_polarization() > 0.0);

		const auto s_rk4 = PolarizedRadiativeTransfer<double>::step_rk4(s_init, j, k, 0.1);
		assert(s_rk4.i > 0.0);
	}

	{
		const double x_low = 1e-5;
		const double sig_low = InverseComptonEngine::klein_nishina_total_cross_section(x_low * Relativistic::Core::PhysicalConstants<double>::ELECTRON_MASS * Relativistic::Core::PhysicalConstants<double>::SPEED_OF_LIGHT * Relativistic::Core::PhysicalConstants<double>::SPEED_OF_LIGHT);
		assert(std::abs(sig_low - 6.6524587321e-29) / 6.6524587321e-29 < 1e-3);

		const double y_param = InverseComptonEngine::compton_y_parameter(1.0, 0.1);
		assert(y_param > 0.0);

		std::vector<PhotonPacket> seed;
		for (size_t i = 0; i < 500; ++i) {
			PhotonPacket p;
			p.energy_joules = 1e-20;
			seed.push_back(p);
		}

		Relativistic::Core::PCG64Engine rng(1234, 5678);
		const auto scattered = InverseComptonEngine::simulate_los_compton_scattering(seed, 2.0, 0.5, rng);
		assert(scattered.size() == seed.size());

		double avg_energy_in = 0.0;
		double avg_energy_out = 0.0;
		for (size_t i = 0; i < seed.size(); ++i) {
			avg_energy_in += seed[i].energy_joules;
			avg_energy_out += scattered[i].energy_joules;
		}
		avg_energy_in /= static_cast<double>(seed.size());
		avg_energy_out /= static_cast<double>(seed.size());

		assert(avg_energy_out > avg_energy_in);
	}

	std::cout << "All polarized radiative transfer tests passed successfully.\n";
	return 0;
}

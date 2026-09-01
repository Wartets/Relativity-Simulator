#pragma once

#include "relativistic/core/constants.hpp"
#include "relativistic/core/tensor.hpp"
#include "relativistic/optics/stokes_vector.hpp"
#include "relativistic/optics/maxwell_juttner.hpp"
#include <cmath>
#include <numbers>
#include <algorithm>
#include <concepts>
#include <array>

namespace Relativistic::Optics {

template <typename Scalar = double>
struct PolarizedPlasmaState {
	Scalar electron_density{static_cast<Scalar>(1e18)};
	Scalar ion_density{static_cast<Scalar>(1e18)};
	Scalar electron_temperature_k{static_cast<Scalar>(1e9)};
	Scalar magnetic_field_tesla{static_cast<Scalar>(0.1)};
	Scalar pitch_angle_rad{static_cast<Scalar>(std::numbers::pi / 3.0)};
	Scalar power_law_index{static_cast<Scalar>(3.0)};
	Scalar non_thermal_fraction{static_cast<Scalar>(0.0)};
	Scalar gamma_min{static_cast<Scalar>(10.0)};
	Scalar gamma_max{static_cast<Scalar>(1e5)};
};

template <typename Scalar = double>
class RadiativeProcessEngine {
private:
	static constexpr double E_CHARGE = 1.602176634e-19;
	static constexpr double M_ELECTRON = Core::PhysicalConstants<double>::ELECTRON_MASS;
	static constexpr double C_LIGHT = Core::PhysicalConstants<double>::SPEED_OF_LIGHT;
	static constexpr double H_PLANCK = Core::PhysicalConstants<double>::PLANCK_CONSTANT;
	static constexpr double K_BOLTZ = Core::PhysicalConstants<double>::BOLTZMANN_CONSTANT;

public:
	[[nodiscard]] static constexpr Scalar synchrotron_cyclotron_frequency(Scalar b_tesla) noexcept {
		return static_cast<Scalar>((E_CHARGE * static_cast<double>(b_tesla)) / (2.0 * std::numbers::pi * M_ELECTRON));
	}

	[[nodiscard]] static Scalar thermal_gaunt_factor(Scalar nu, Scalar temp_k) noexcept {
		const double u = (H_PLANCK * static_cast<double>(nu)) / (K_BOLTZ * static_cast<double>(temp_k));
		const double gamma_e = 0.5772156649;
		if (u < 1e-4) {
			return static_cast<Scalar>(std::max(1.0, (std::sqrt(3.0) / std::numbers::pi) * std::log(2.25 / (gamma_e * u))));
		}
		return static_cast<Scalar>((std::sqrt(3.0) / std::numbers::pi) * std::exp(0.5 * u) * RelativisticBessel::k0(0.5 * u));
	}

	[[nodiscard]] static Scalar thermal_bremsstrahlung_emissivity(
		Scalar nu,
		Scalar n_e,
		Scalar n_i,
		Scalar temp_k,
		Scalar z_ion = static_cast<Scalar>(1.0)
	) noexcept {
		const double t = static_cast<double>(temp_k);
		const double z = static_cast<double>(z_ion);
		const double ne = static_cast<double>(n_e);
		const double ni = static_cast<double>(n_i);
		const double freq = static_cast<double>(nu);

		const double prefactor = (32.0 * std::numbers::pi * std::pow(E_CHARGE, 6.0)) / (3.0 * std::sqrt(3.0) * M_ELECTRON * M_ELECTRON * M_ELECTRON * std::pow(C_LIGHT, 3.0));
		const double thermal_term = std::sqrt((2.0 * std::numbers::pi * M_ELECTRON) / (3.0 * K_BOLTZ * t));
		const double exp_term = std::exp(-(H_PLANCK * freq) / (K_BOLTZ * t));
		const double gaunt = static_cast<double>(thermal_gaunt_factor(nu, temp_k));

		const double relativistic_correction = 1.0 + 4.4e-10 * t;
		const double j_nu = (prefactor * thermal_term * z * z * ne * ni * exp_term * gaunt * relativistic_correction) / (4.0 * std::numbers::pi);
		return static_cast<Scalar>(std::max(0.0, j_nu));
	}

	[[nodiscard]] static StokesEmissivity<Scalar> non_thermal_synchrotron_emissivity(
		Scalar nu,
		const PolarizedPlasmaState<Scalar>& plasma
	) noexcept {
		const Scalar p = plasma.power_law_index;
		const Scalar b = std::max(plasma.magnetic_field_tesla, static_cast<Scalar>(1e-12));
		const Scalar sin_th = std::max(std::sin(plasma.pitch_angle_rad), static_cast<Scalar>(1e-4));

		const Scalar pi_l = (p + static_cast<Scalar>(1.0)) / (p + static_cast<Scalar>(7.0 / 3.0));

		const double p_d = static_cast<double>(p);
		const double gamma1 = std::tgamma((3.0 * p_d - 1.0) / 12.0);
		const double gamma2 = std::tgamma((3.0 * p_d + 19.0) / 12.0);

		const double c_e = static_cast<double>(plasma.electron_density * plasma.non_thermal_fraction) * (p_d - 1.0) * std::pow(static_cast<double>(plasma.gamma_min), p_d - 1.0);
		const double prefactor = (std::sqrt(3.0) * std::pow(E_CHARGE, 3.0) * static_cast<double>(b * sin_th)) / (4.0 * std::numbers::pi * M_ELECTRON * C_LIGHT * C_LIGHT * (p_d + 1.0));
		const double nu_scale = (2.0 * std::numbers::pi * M_ELECTRON * C_LIGHT * static_cast<double>(nu)) / (3.0 * E_CHARGE * static_cast<double>(b * sin_th));

		const double j_tot = prefactor * c_e * gamma1 * gamma2 * std::pow(std::max(nu_scale, 1e-30), -(p_d - 1.0) * 0.5) / (4.0 * std::numbers::pi);

		const Scalar j_i = static_cast<Scalar>(std::max(0.0, j_tot));
		const Scalar j_q = pi_l * j_i;

		return StokesEmissivity<Scalar>(j_i, j_q, static_cast<Scalar>(0.0), static_cast<Scalar>(0.0));
	}

	[[nodiscard]] static StokesTransferMatrix<Scalar> non_thermal_synchrotron_absorptivity(
		Scalar nu,
		const PolarizedPlasmaState<Scalar>& plasma
	) noexcept {
		const Scalar p = plasma.power_law_index;
		const Scalar b = std::max(plasma.magnetic_field_tesla, static_cast<Scalar>(1e-12));
		const Scalar sin_th = std::max(std::sin(plasma.pitch_angle_rad), static_cast<Scalar>(1e-4));
		const Scalar cos_th = std::cos(plasma.pitch_angle_rad);

		const double p_d = static_cast<double>(p);
		const double c_e = static_cast<double>(plasma.electron_density * plasma.non_thermal_fraction) * (p_d - 1.0) * std::pow(static_cast<double>(plasma.gamma_min), p_d - 1.0);

		const double gamma1 = std::tgamma((3.0 * p_d + 2.0) / 12.0);
		const double gamma2 = std::tgamma((3.0 * p_d + 22.0) / 12.0);

		const double prefactor = (std::sqrt(3.0) * std::pow(E_CHARGE, 3.0)) / (8.0 * std::numbers::pi * M_ELECTRON * M_ELECTRON * C_LIGHT * C_LIGHT * static_cast<double>(nu));
		const double nu_scale = (2.0 * std::numbers::pi * M_ELECTRON * C_LIGHT * static_cast<double>(nu)) / (3.0 * E_CHARGE * static_cast<double>(b * sin_th));

		const double alpha_tot = prefactor * c_e * (static_cast<double>(b * sin_th)) * gamma1 * gamma2 * std::pow(std::max(nu_scale, 1e-30), -(p_d + 2.0) * 0.5);

		const Scalar pi_l = (p + static_cast<Scalar>(1.0)) / (p + static_cast<Scalar>(7.0 / 3.0));
		const Scalar a_i = static_cast<Scalar>(std::max(0.0, alpha_tot));
		const Scalar a_q = pi_l * a_i;

		const Scalar nu_p2 = static_cast<Scalar>((E_CHARGE * E_CHARGE * static_cast<double>(plasma.electron_density)) / (M_ELECTRON * 8.8541878128e-12));
		const Scalar nu_b = synchrotron_cyclotron_frequency(b);
		const Scalar rho_v = (nu_p2 * nu_b * cos_th) / (static_cast<Scalar>(C_LIGHT) * nu * nu);

		return StokesTransferMatrix<Scalar>(a_i, a_q, static_cast<Scalar>(0.0), static_cast<Scalar>(0.0), static_cast<Scalar>(0.0), static_cast<Scalar>(0.0), rho_v);
	}

	[[nodiscard]] static StokesEmissivity<Scalar> thermal_synchrotron_emissivity(
		Scalar nu,
		const PolarizedPlasmaState<Scalar>& plasma
	) noexcept {
		const Scalar theta_e = static_cast<Scalar>((K_BOLTZ * static_cast<double>(plasma.electron_temperature_k)) / (M_ELECTRON * C_LIGHT * C_LIGHT));
		const Scalar b = std::max(plasma.magnetic_field_tesla, static_cast<Scalar>(1e-12));
		const Scalar sin_th = std::max(std::sin(plasma.pitch_angle_rad), static_cast<Scalar>(1e-4));
		const Scalar nu_c = synchrotron_cyclotron_frequency(b);
		const Scalar nu_s = (static_cast<Scalar>(2.0 / 9.0)) * nu_c * theta_e * theta_e * sin_th;

		const Scalar x = nu / std::max(nu_s, static_cast<Scalar>(1e-30));
		const Scalar sqrt_x = std::sqrt(x);
		const Scalar cbrt_x = std::cbrt(x);

		const Scalar i_x = static_cast<Scalar>(4.0505) / cbrt_x * (static_cast<Scalar>(1.0) + static_cast<Scalar>(0.40) / sqrt_x + static_cast<Scalar>(0.5316) / cbrt_x) * std::exp(-cbrt_x);

		const Scalar prefactor = (static_cast<Scalar>(std::numbers::sqrt2_v<double>) * std::numbers::pi_v<Scalar> * static_cast<Scalar>(E_CHARGE * E_CHARGE) * plasma.electron_density * nu_s) / (static_cast<Scalar>(3.0 * C_LIGHT) * RelativisticBessel::k2(1.0 / static_cast<double>(theta_e)));
		const Scalar j_i = prefactor * i_x / static_cast<Scalar>(4.0 * std::numbers::pi);

		const Scalar pol_deg = std::clamp((static_cast<Scalar>(1.0) + static_cast<Scalar>(0.5) / theta_e) / (static_cast<Scalar>(1.0) + static_cast<Scalar>(1.5) / theta_e), static_cast<Scalar>(0.5), static_cast<Scalar>(0.85));
		const Scalar j_q = pol_deg * j_i;

		return StokesEmissivity<Scalar>(j_i, j_q, static_cast<Scalar>(0.0), static_cast<Scalar>(0.0));
	}

	[[nodiscard]] static StokesTransferMatrix<Scalar> thermal_synchrotron_absorptivity(
		Scalar nu,
		const PolarizedPlasmaState<Scalar>& plasma
	) noexcept {
		const auto emis = thermal_synchrotron_emissivity(nu, plasma);
		const double freq = static_cast<double>(nu);
		const double t = static_cast<double>(plasma.electron_temperature_k);
		const double b_nu = (2.0 * H_PLANCK * freq * freq * freq) / (C_LIGHT * C_LIGHT * std::max(std::exp((H_PLANCK * freq) / (K_BOLTZ * t)) - 1.0, 1e-30));

		const Scalar a_i = static_cast<Scalar>(emis.j_i / std::max(b_nu, 1e-30));
		const Scalar a_q = static_cast<Scalar>(emis.j_q / std::max(b_nu, 1e-30));

		const Scalar cos_th = std::cos(plasma.pitch_angle_rad);
		const Scalar sin_th = std::sin(plasma.pitch_angle_rad);
		const Scalar theta_e = static_cast<Scalar>((K_BOLTZ * t) / (M_ELECTRON * C_LIGHT * C_LIGHT));
		const Scalar nu_p2 = static_cast<Scalar>((E_CHARGE * E_CHARGE * static_cast<double>(plasma.electron_density)) / (M_ELECTRON * 8.8541878128e-12));
		const Scalar nu_b = synchrotron_cyclotron_frequency(plasma.magnetic_field_tesla);

		const Scalar rho_v = (nu_p2 * nu_b * cos_th) / (static_cast<Scalar>(C_LIGHT) * nu * nu) * (RelativisticBessel::k0(1.0 / static_cast<double>(theta_e)) / RelativisticBessel::k2(1.0 / static_cast<double>(theta_e)));
		const Scalar rho_q = (nu_p2 * nu_b * nu_b * sin_th * sin_th) / (static_cast<Scalar>(2.0 * C_LIGHT) * nu * nu * nu) * (RelativisticBessel::k1(1.0 / static_cast<double>(theta_e)) / RelativisticBessel::k2(1.0 / static_cast<double>(theta_e)));

		return StokesTransferMatrix<Scalar>(a_i, a_q, static_cast<Scalar>(0.0), static_cast<Scalar>(0.0), rho_q, static_cast<Scalar>(0.0), rho_v);
	}
};

}

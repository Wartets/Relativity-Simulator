#pragma once

#include "relativistic/core/tensor.hpp"
#include "relativistic/core/tensor_ops.hpp"
#include "relativistic/core/four_vector_bundle.hpp"
#include <cmath>
#include <algorithm>
#include <concepts>

namespace Relativistic::Optics {

template <typename Scalar = double>
[[nodiscard]] constexpr Scalar compute_spectral_shift(
	const Core::FourVector<Scalar>& p_obs,
	const Core::FourVector<Scalar>& u_obs,
	const Core::MetricTensor<Scalar>& g_obs,
	const Core::FourVector<Scalar>& p_emit,
	const Core::FourVector<Scalar>& u_emit,
	const Core::MetricTensor<Scalar>& g_emit
) noexcept {
	Scalar num = static_cast<Scalar>(0);
	Scalar den = static_cast<Scalar>(0);
	for (size_t mu = 0; mu < 4; ++mu) {
		for (size_t nu = 0; nu < 4; ++nu) {
			num += g_obs(mu, nu) * p_obs(mu) * u_obs(nu);
			den += g_emit(mu, nu) * p_emit(mu) * u_emit(nu);
		}
	}
	if (std::abs(den) < static_cast<Scalar>(1e-30)) [[unlikely]] {
		return static_cast<Scalar>(1);
	}
	return num / den;
}

template <typename Scalar = double>
[[nodiscard]] constexpr Scalar compute_spectral_shift_flat(
	const Core::FourVector<Scalar>& p_obs,
	const Core::FourVector<Scalar>& u_obs,
	const Core::FourVector<Scalar>& p_emit,
	const Core::FourVector<Scalar>& u_emit,
	Scalar speed_of_light = static_cast<Scalar>(1)
) noexcept {
	const Scalar c2 = speed_of_light * speed_of_light;
	const Scalar num = -(c2 * p_obs(0) * u_obs(0)) + (p_obs(1) * u_obs(1) + p_obs(2) * u_obs(2) + p_obs(3) * u_obs(3));
	const Scalar den = -(c2 * p_emit(0) * u_emit(0)) + (p_emit(1) * u_emit(1) + p_emit(2) * u_emit(2) + p_emit(3) * u_emit(3));
	if (std::abs(den) < static_cast<Scalar>(1e-30)) [[unlikely]] {
		return static_cast<Scalar>(1);
	}
	return num / den;
}

template <typename Scalar = double>
[[nodiscard]] constexpr Scalar compute_kinematic_doppler(
	Scalar beta_x,
	Scalar beta_y,
	Scalar beta_z,
	Scalar n_x,
	Scalar n_y,
	Scalar n_z
) noexcept {
	const Scalar beta_sq = beta_x * beta_x + beta_y * beta_y + beta_z * beta_z;
	const Scalar beta_dot_n = beta_x * n_x + beta_y * n_y + beta_z * n_z;
	const Scalar gamma_inv = std::sqrt(std::max(static_cast<Scalar>(1) - beta_sq, static_cast<Scalar>(1e-30)));
	const Scalar denom = static_cast<Scalar>(1) - beta_dot_n;
	if (std::abs(denom) < static_cast<Scalar>(1e-30)) [[unlikely]] {
		return static_cast<Scalar>(1);
	}
	return gamma_inv / denom;
}

template <typename Scalar = double>
[[nodiscard]] constexpr Scalar apply_specific_intensity_beaming(Scalar i_emit, Scalar g) noexcept {
	const Scalar g3 = g * g * g;
	return g3 * i_emit;
}

template <typename Scalar = double>
[[nodiscard]] constexpr Scalar apply_bolometric_flux_beaming(Scalar f_emit, Scalar g) noexcept {
	const Scalar g2 = g * g;
	const Scalar g4 = g2 * g2;
	return g4 * f_emit;
}

template <typename Scalar = double>
[[nodiscard]] constexpr Scalar apply_wavelength_intensity_beaming(Scalar i_lambda_emit, Scalar g) noexcept {
	const Scalar g2 = g * g;
	const Scalar g5 = g2 * g2 * g;
	return g5 * i_lambda_emit;
}

template <typename Scalar = double>
[[nodiscard]] constexpr Scalar observed_wavelength(Scalar lambda_emit, Scalar g) noexcept {
	if (std::abs(g) < static_cast<Scalar>(1e-30)) [[unlikely]] {
		return lambda_emit;
	}
	return lambda_emit / g;
}

template <typename Scalar = double>
[[nodiscard]] constexpr Scalar emitted_wavelength(Scalar lambda_obs, Scalar g) noexcept {
	return lambda_obs * g;
}

}

#pragma once

#include "relativistic/core/math/tensor.hpp"
#include "relativistic/optics/stokes_vector.hpp"
#include "relativistic/optics/radiative_processes.hpp"
#include <cmath>
#include <algorithm>
#include <array>
#include <concepts>

namespace Relativistic::Optics {

template <typename Scalar = double>
class PolarizedRadiativeTransfer {
public:
	[[nodiscard]] static StokesVector<Scalar> step_delano_analytical(
		const StokesVector<Scalar>& s_in,
		const StokesEmissivity<Scalar>& j,
		const StokesTransferMatrix<Scalar>& k,
		Scalar delta_lambda
	) noexcept {
		if (delta_lambda <= static_cast<Scalar>(0.0)) {
			return s_in;
		}

		const auto mat = k.to_matrix_array();
		Scalar norm_k = static_cast<Scalar>(0.0);
		for (size_t row = 0; row < 4; ++row) {
			Scalar row_sum = static_cast<Scalar>(0.0);
			for (size_t col = 0; col < 4; ++col) {
				row_sum += std::abs(mat[row][col]);
			}
			if (row_sum > norm_k) {
				norm_k = row_sum;
			}
		}

		const Scalar mu_k = norm_k * delta_lambda;
		int m_squarings = 0;
		Scalar scale = static_cast<Scalar>(1.0);
		if (mu_k > static_cast<Scalar>(0.5)) {
			m_squarings = static_cast<int>(std::ceil(std::log2(static_cast<double>(mu_k * static_cast<Scalar>(2.0)))));
			scale = static_cast<Scalar>(1.0) / static_cast<Scalar>(1ULL << m_squarings);
		}

		const Scalar h = delta_lambda * scale;
		std::array<std::array<Scalar, 4>, 4> a{};
		for (size_t r = 0; r < 4; ++r) {
			for (size_t c = 0; c < 4; ++c) {
				a[r][c] = -mat[r][c] * h;
			}
		}

		std::array<std::array<Scalar, 4>, 4> e_mat{};
		std::array<std::array<Scalar, 4>, 4> psi_mat{};
		std::array<std::array<Scalar, 4>, 4> a_pow{};

		for (size_t r = 0; r < 4; ++r) {
			for (size_t c = 0; c < 4; ++c) {
				const Scalar id = (r == c) ? static_cast<Scalar>(1.0) : static_cast<Scalar>(0.0);
				e_mat[r][c] = id;
				psi_mat[r][c] = id * h;
				a_pow[r][c] = id;
			}
		}

		Scalar fact = static_cast<Scalar>(1.0);
		for (size_t n = 1; n <= 12; ++n) {
			fact *= static_cast<Scalar>(n);
			std::array<std::array<Scalar, 4>, 4> next_pow{};
			for (size_t r = 0; r < 4; ++r) {
				for (size_t c = 0; c < 4; ++c) {
					for (size_t p = 0; p < 4; ++p) {
						next_pow[r][c] += a_pow[r][p] * a[p][c];
					}
				}
			}
			a_pow = next_pow;
			const Scalar inv_fact = static_cast<Scalar>(1.0) / fact;
			const Scalar inv_fact_psi = h / (fact * static_cast<Scalar>(n + 1));
			for (size_t r = 0; r < 4; ++r) {
				for (size_t c = 0; c < 4; ++c) {
					e_mat[r][c] += a_pow[r][c] * inv_fact;
					psi_mat[r][c] += a_pow[r][c] * inv_fact_psi;
				}
			}
		}

		for (int sq = 0; sq < m_squarings; ++sq) {
			std::array<std::array<Scalar, 4>, 4> next_psi{};
			std::array<std::array<Scalar, 4>, 4> next_e{};
			for (size_t r = 0; r < 4; ++r) {
				for (size_t c = 0; c < 4; ++c) {
					Scalar term_psi = static_cast<Scalar>(0.0);
					Scalar term_e = static_cast<Scalar>(0.0);
					for (size_t p = 0; p < 4; ++p) {
						const Scalar id_p = (p == c) ? static_cast<Scalar>(1.0) : static_cast<Scalar>(0.0);
						term_psi += psi_mat[r][p] * (id_p + e_mat[p][c]);
						term_e += e_mat[r][p] * e_mat[p][c];
					}
					next_psi[r][c] = term_psi;
					next_e[r][c] = term_e;
				}
			}
			psi_mat = next_psi;
			e_mat = next_e;
		}

		const std::array<Scalar, 4> s_in_vec = {s_in.i, s_in.q, s_in.u, s_in.v};
		const std::array<Scalar, 4> j_vec = {j.j_i, j.j_q, j.j_u, j.j_v};
		std::array<Scalar, 4> s_out_vec{};

		for (size_t r = 0; r < 4; ++r) {
			Scalar sum = static_cast<Scalar>(0.0);
			for (size_t c = 0; c < 4; ++c) {
				sum += e_mat[r][c] * s_in_vec[c] + psi_mat[r][c] * j_vec[c];
			}
			s_out_vec[r] = sum;
		}

		return StokesVector<Scalar>(
			std::max(static_cast<Scalar>(0.0), s_out_vec[0]),
			s_out_vec[1],
			s_out_vec[2],
			s_out_vec[3]
		);
	}

	[[nodiscard]] static StokesVector<Scalar> step_rk4(
		const StokesVector<Scalar>& s_in,
		const StokesEmissivity<Scalar>& j,
		const StokesTransferMatrix<Scalar>& k,
		Scalar delta_lambda
	) noexcept {
		auto derivatives = [&](const StokesVector<Scalar>& s) noexcept -> StokesVector<Scalar> {
			return j.to_stokes_vector() - k.multiply(s);
		};

		const auto k1 = derivatives(s_in);
		const auto s2 = s_in + k1 * (delta_lambda * static_cast<Scalar>(0.5));
		const auto k2 = derivatives(s2);
		const auto s3 = s_in + k2 * (delta_lambda * static_cast<Scalar>(0.5));
		const auto k3 = derivatives(s3);
		const auto s4 = s_in + k3 * delta_lambda;
		const auto k4 = derivatives(s4);

		const auto s_out = s_in + (k1 + static_cast<Scalar>(2.0) * k2 + static_cast<Scalar>(2.0) * k3 + k4) * (delta_lambda / static_cast<Scalar>(6.0));
		return StokesVector<Scalar>(
			std::max(static_cast<Scalar>(0.0), s_out.i),
			s_out.q,
			s_out.u,
			s_out.v
		);
	}

	[[nodiscard]] static StokesVector<Scalar> integrate_ray_segment(
		const StokesVector<Scalar>& initial_stokes,
		Scalar nu_obs,
		Scalar g_doppler,
		const PolarizedPlasmaState<Scalar>& plasma,
		Scalar path_length_m,
		bool is_thermal = false
	) noexcept {
		const Scalar nu_emit = nu_obs / g_doppler;

		const auto j_emit = is_thermal
			? RadiativeProcessEngine<Scalar>::thermal_synchrotron_emissivity(nu_emit, plasma)
			: RadiativeProcessEngine<Scalar>::non_thermal_synchrotron_emissivity(nu_emit, plasma);

		const auto k_emit = is_thermal
			? RadiativeProcessEngine<Scalar>::thermal_synchrotron_absorptivity(nu_emit, plasma)
			: RadiativeProcessEngine<Scalar>::non_thermal_synchrotron_absorptivity(nu_emit, plasma);

		const Scalar g2 = g_doppler * g_doppler;

		const StokesEmissivity<Scalar> j_obs(
			j_emit.j_i * g2,
			j_emit.j_q * g2,
			j_emit.j_u * g2,
			j_emit.j_v * g2
		);

		const StokesTransferMatrix<Scalar> k_obs(
			k_emit.alpha_i / g_doppler,
			k_emit.alpha_q / g_doppler,
			k_emit.alpha_u / g_doppler,
			k_emit.alpha_v / g_doppler,
			k_emit.rho_q / g_doppler,
			k_emit.rho_u / g_doppler,
			k_emit.rho_v / g_doppler
		);

		return step_delano_analytical(initial_stokes, j_obs, k_obs, path_length_m);
	}

	[[nodiscard]] static StokesVector<Scalar> integrate_hybrid_segment(
		const StokesVector<Scalar>& initial_stokes,
		Scalar nu_obs,
		Scalar g_doppler,
		const PolarizedPlasmaState<Scalar>& plasma,
		Scalar path_length_m
	) noexcept {
		using Engine = RadiativeProcessEngine<Scalar>;
		const Scalar nu_emit = nu_obs / g_doppler;
		const Scalar fraction = std::clamp(plasma.non_thermal_fraction, static_cast<Scalar>(0.0), static_cast<Scalar>(1.0));

		PolarizedPlasmaState<Scalar> thermal = plasma;
		thermal.electron_density = plasma.electron_density * (static_cast<Scalar>(1.0) - fraction);
		thermal.ion_density = thermal.electron_density;

		const auto j_nt = Engine::non_thermal_synchrotron_emissivity(nu_emit, plasma);
		const auto k_nt = Engine::non_thermal_synchrotron_absorptivity(nu_emit, plasma);
		const auto j_th = Engine::thermal_synchrotron_emissivity(nu_emit, thermal);
		const auto k_th = Engine::thermal_synchrotron_absorptivity(nu_emit, thermal);

		const Scalar g2 = g_doppler * g_doppler;
		const StokesEmissivity<Scalar> j_obs(
			(j_nt.j_i + j_th.j_i) * g2,
			(j_nt.j_q + j_th.j_q) * g2,
			(j_nt.j_u + j_th.j_u) * g2,
			(j_nt.j_v + j_th.j_v) * g2
		);
		const StokesTransferMatrix<Scalar> k_obs(
			(k_nt.alpha_i + k_th.alpha_i) / g_doppler,
			(k_nt.alpha_q + k_th.alpha_q) / g_doppler,
			(k_nt.alpha_u + k_th.alpha_u) / g_doppler,
			(k_nt.alpha_v + k_th.alpha_v) / g_doppler,
			(k_nt.rho_q + k_th.rho_q) / g_doppler,
			(k_nt.rho_u + k_th.rho_u) / g_doppler,
			(k_nt.rho_v + k_th.rho_v) / g_doppler
		);

		return step_delano_analytical(initial_stokes, j_obs, k_obs, path_length_m);
	}
};

}

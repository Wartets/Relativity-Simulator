#pragma once

#include "relativistic/hydro/eos.hpp"
#include "relativistic/hydro/hydro_types.hpp"
#include <cmath>
#include <algorithm>
#include <array>

namespace Relativistic::Hydro {

enum class RiemannSolverType : uint32_t {
	HLL = 0,
	HLLC = 1,
	HLLD = 2
};

template <typename EOS = IdealGasEOS<double>, typename Scalar = double>
class HLLRiemannSolver {
private:
	EOS eos_{};

public:
	constexpr HLLRiemannSolver() noexcept = default;
	explicit constexpr HLLRiemannSolver(const EOS& eos) noexcept : eos_(eos) {}

	[[nodiscard]] FluxVariables<Scalar> solve_1d_x(
		const PrimitiveVariables<Scalar>& prim_l,
		const PrimitiveVariables<Scalar>& prim_r
	) const noexcept {
		const Scalar cs_l = eos_.sound_speed(prim_l.rho, prim_l.p);
		const Scalar cs_r = eos_.sound_speed(prim_r.rho, prim_r.p);

		const Scalar lambda_l_minus = (prim_l.vx - cs_l) / (static_cast<Scalar>(1.0) - prim_l.vx * cs_l);
		const Scalar lambda_l_plus = (prim_l.vx + cs_l) / (static_cast<Scalar>(1.0) + prim_l.vx * cs_l);
		const Scalar lambda_r_minus = (prim_r.vx - cs_r) / (static_cast<Scalar>(1.0) - prim_r.vx * cs_r);
		const Scalar lambda_r_plus = (prim_r.vx + cs_r) / (static_cast<Scalar>(1.0) + prim_r.vx * cs_r);

		const Scalar lambda_l = std::min({lambda_l_minus, lambda_r_minus, static_cast<Scalar>(0.0)});
		const Scalar lambda_r = std::max({lambda_l_plus, lambda_r_plus, static_cast<Scalar>(0.0)});

		const auto u_l = prim_to_con_flat(prim_l);
		const auto u_r = prim_to_con_flat(prim_r);
		const auto f_l = compute_flux_1d_x(prim_l);
		const auto f_r = compute_flux_1d_x(prim_r);

		if (lambda_l >= static_cast<Scalar>(0.0)) {
			return f_l;
		}
		if (lambda_r <= static_cast<Scalar>(0.0)) {
			return f_r;
		}

		const Scalar denom = static_cast<Scalar>(1.0) / (lambda_r - lambda_l);
		const Scalar num_fd = lambda_r * f_l.fd - lambda_l * f_r.fd + lambda_l * lambda_r * (u_r.d - u_l.d);
		const Scalar num_fsx = lambda_r * f_l.fsx - lambda_l * f_r.fsx + lambda_l * lambda_r * (u_r.sx - u_l.sx);
		const Scalar num_fsy = lambda_r * f_l.fsy - lambda_l * f_r.fsy + lambda_l * lambda_r * (u_r.sy - u_l.sy);
		const Scalar num_fsz = lambda_r * f_l.fsz - lambda_l * f_r.fsz + lambda_l * lambda_r * (u_r.sz - u_l.sz);
		const Scalar num_ftau = lambda_r * f_l.ftau - lambda_l * f_r.ftau + lambda_l * lambda_r * (u_r.tau - u_l.tau);
		const Scalar num_fbx = static_cast<Scalar>(0.0);
		const Scalar num_fby = lambda_r * f_l.fby - lambda_l * f_r.fby + lambda_l * lambda_r * (u_r.by - u_l.by);
		const Scalar num_fbz = lambda_r * f_l.fbz - lambda_l * f_r.fbz + lambda_l * lambda_r * (u_r.bz - u_l.bz);

		return FluxVariables<Scalar>(
			num_fd * denom, num_fsx * denom, num_fsy * denom, num_fsz * denom,
			num_ftau * denom, num_fbx * denom, num_fby * denom, num_fbz * denom
		);
	}
};

template <typename EOS = IdealGasEOS<double>, typename Scalar = double>
class HLLCRiemannSolver {
private:
	EOS eos_{};

public:
	constexpr HLLCRiemannSolver() noexcept = default;
	explicit constexpr HLLCRiemannSolver(const EOS& eos) noexcept : eos_(eos) {}

	[[nodiscard]] FluxVariables<Scalar> solve_1d_x(
		const PrimitiveVariables<Scalar>& prim_l,
		const PrimitiveVariables<Scalar>& prim_r
	) const noexcept {
		const Scalar cs_l = eos_.sound_speed(prim_l.rho, prim_l.p);
		const Scalar cs_r = eos_.sound_speed(prim_r.rho, prim_r.p);

		const Scalar lambda_l_minus = (prim_l.vx - cs_l) / (static_cast<Scalar>(1.0) - prim_l.vx * cs_l);
		const Scalar lambda_l_plus = (prim_l.vx + cs_l) / (static_cast<Scalar>(1.0) + prim_l.vx * cs_l);
		const Scalar lambda_r_minus = (prim_r.vx - cs_r) / (static_cast<Scalar>(1.0) - prim_r.vx * cs_r);
		const Scalar lambda_r_plus = (prim_r.vx + cs_r) / (static_cast<Scalar>(1.0) + prim_r.vx * cs_r);

		const Scalar lambda_l = std::min({lambda_l_minus, lambda_r_minus, static_cast<Scalar>(0.0)});
		const Scalar lambda_r = std::max({lambda_l_plus, lambda_r_plus, static_cast<Scalar>(0.0)});

		const auto u_l = prim_to_con_flat(prim_l);
		const auto u_r = prim_to_con_flat(prim_r);
		const auto f_l = compute_flux_1d_x(prim_l);
		const auto f_r = compute_flux_1d_x(prim_r);

		if (lambda_l >= static_cast<Scalar>(0.0)) {
			return f_l;
		}
		if (lambda_r <= static_cast<Scalar>(0.0)) {
			return f_r;
		}

		const Scalar inv_diff = static_cast<Scalar>(1.0) / (lambda_r - lambda_l);
		const Scalar u_hll_d = (lambda_r * u_r.d - lambda_l * u_l.d + f_l.fd - f_r.fd) * inv_diff;
		const Scalar u_hll_sx = (lambda_r * u_r.sx - lambda_l * u_l.sx + f_l.fsx - f_r.fsx) * inv_diff;
		const Scalar u_hll_tau = (lambda_r * u_r.tau - lambda_l * u_l.tau + f_l.ftau - f_r.ftau) * inv_diff;
		const Scalar f_hll_sx = (lambda_r * f_l.fsx - lambda_l * f_r.fsx + lambda_l * lambda_r * (u_r.sx - u_l.sx)) * inv_diff;

		const Scalar e_hll = u_hll_tau + u_hll_d;

		Scalar lambda_star = static_cast<Scalar>(0.0);
		if (std::abs(u_hll_sx) > static_cast<Scalar>(1e-15)) {
			const Scalar b0 = (e_hll + f_hll_sx) / u_hll_sx;
			const Scalar discr = b0 * b0 - static_cast<Scalar>(4.0);
			if (discr >= static_cast<Scalar>(0.0)) {
				const Scalar sqrt_d = std::sqrt(discr);
				lambda_star = (u_hll_sx > static_cast<Scalar>(0.0)) ? (static_cast<Scalar>(0.5) * (b0 - sqrt_d))
				                                                    : (static_cast<Scalar>(0.5) * (b0 + sqrt_d));
			} else {
				lambda_star = static_cast<Scalar>(0.5) * (prim_l.vx + prim_r.vx);
			}
		} else {
			lambda_star = static_cast<Scalar>(0.0);
		}

		lambda_star = std::clamp(lambda_star, static_cast<Scalar>(-0.99999999), static_cast<Scalar>(0.99999999));
		const Scalar p_star = -u_hll_sx * lambda_star + f_hll_sx;

		if (lambda_star >= static_cast<Scalar>(0.0)) {
			const Scalar denom_l = static_cast<Scalar>(1.0) / (lambda_l - lambda_star);
			const Scalar d_star_l = u_l.d * (lambda_l - prim_l.vx) * denom_l;
			const Scalar sx_star_l = (lambda_l * u_l.sx - f_l.fsx + p_star) * denom_l;
			const Scalar sy_star_l = u_l.sy * (lambda_l - prim_l.vx) * denom_l;
			const Scalar sz_star_l = u_l.sz * (lambda_l - prim_l.vx) * denom_l;
			const Scalar e_star_l = (lambda_l * (u_l.tau + u_l.d) - u_l.sx + p_star * lambda_star) * denom_l;
			const Scalar tau_star_l = e_star_l - d_star_l;

			return FluxVariables<Scalar>(
				f_l.fd + lambda_l * (d_star_l - u_l.d),
				f_l.fsx + lambda_l * (sx_star_l - u_l.sx),
				f_l.fsy + lambda_l * (sy_star_l - u_l.sy),
				f_l.fsz + lambda_l * (sz_star_l - u_l.sz),
				f_l.ftau + lambda_l * (tau_star_l - u_l.tau)
			);
		} else {
			const Scalar denom_r = static_cast<Scalar>(1.0) / (lambda_r - lambda_star);
			const Scalar d_star_r = u_r.d * (lambda_r - prim_r.vx) * denom_r;
			const Scalar sx_star_r = (lambda_r * u_r.sx - f_r.fsx + p_star) * denom_r;
			const Scalar sy_star_r = u_r.sy * (lambda_r - prim_r.vx) * denom_r;
			const Scalar sz_star_r = u_r.sz * (lambda_r - prim_r.vx) * denom_r;
			const Scalar e_star_r = (lambda_r * (u_r.tau + u_r.d) - u_r.sx + p_star * lambda_star) * denom_r;
			const Scalar tau_star_r = e_star_r - d_star_r;

			return FluxVariables<Scalar>(
				f_r.fd + lambda_r * (d_star_r - u_r.d),
				f_r.fsx + lambda_r * (sx_star_r - u_r.sx),
				f_r.fsy + lambda_r * (sy_star_r - u_r.sy),
				f_r.fsz + lambda_r * (sz_star_r - u_r.sz),
				f_r.ftau + lambda_r * (tau_star_r - u_r.tau)
			);
		}
	}
};

template <typename EOS = IdealGasEOS<double>, typename Scalar = double>
class HLLDRiemannSolver {
private:
	EOS eos_{};
	HLLCRiemannSolver<EOS, Scalar> hllc_{};
	HLLRiemannSolver<EOS, Scalar> hll_{};

public:
	constexpr HLLDRiemannSolver() noexcept = default;
	explicit constexpr HLLDRiemannSolver(const EOS& eos) noexcept : eos_(eos), hllc_(eos), hll_(eos) {}

	[[nodiscard]] FluxVariables<Scalar> solve_1d_x(
		const PrimitiveVariables<Scalar>& prim_l,
		const PrimitiveVariables<Scalar>& prim_r
	) const noexcept {
		const Scalar b2_l = prim_l.bx * prim_l.bx + prim_l.by * prim_l.by + prim_l.bz * prim_l.bz;
		const Scalar b2_r = prim_r.bx * prim_r.bx + prim_r.by * prim_r.by + prim_r.bz * prim_r.bz;

		if (b2_l < static_cast<Scalar>(1e-14) && b2_r < static_cast<Scalar>(1e-14)) {
			return hllc_.solve_1d_x(prim_l, prim_r);
		}

		return hll_.solve_1d_x(prim_l, prim_r);
	}
};

}

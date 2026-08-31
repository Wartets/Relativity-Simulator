#pragma once

#include "relativistic/hydro/eos.hpp"
#include "relativistic/hydro/hydro_types.hpp"
#include <cmath>
#include <algorithm>
#include <limits>

namespace Relativistic::Hydro {

template <typename EOS = IdealGasEOS<double>, typename Scalar = double>
class Con2PrimSolver {
private:
	EOS eos_{};
	Scalar tol_{static_cast<Scalar>(1e-12)};
	size_t max_iterations_{60};
	Scalar p_floor_{static_cast<Scalar>(1e-15)};
	Scalar rho_floor_{static_cast<Scalar>(1e-15)};

public:
	constexpr Con2PrimSolver() noexcept = default;

	explicit constexpr Con2PrimSolver(const EOS& eos, Scalar tolerance = static_cast<Scalar>(1e-12)) noexcept
		: eos_(eos), tol_(tolerance) {}

	[[nodiscard]] PrimitiveVariables<Scalar> solve_hydro(const ConservedVariables<Scalar>& con) const noexcept {
		const Scalar d = std::max(con.d, rho_floor_);
		const Scalar s2 = con.sx * con.sx + con.sy * con.sy + con.sz * con.sz;
		const Scalar tau = std::max(con.tau, static_cast<Scalar>(0.0));
		const Scalar gamma = eos_.gamma();

		Scalar p = std::max(p_floor_, eos_.pressure(d, tau / d));
		Scalar p_min = p_floor_;
		Scalar p_max = std::max(tau + d, static_cast<Scalar>(100.0) * p);

		for (size_t iter = 0; iter < max_iterations_; ++iter) {
			const Scalar z = tau + p + d;
			const Scalar v2 = (z > static_cast<Scalar>(1e-15)) ? (s2 / (z * z)) : static_cast<Scalar>(0.0);
			const Scalar safe_v2 = (v2 < static_cast<Scalar>(1.0 - 1e-15)) ? v2 : static_cast<Scalar>(1.0 - 1e-15);
			const Scalar w = static_cast<Scalar>(1.0) / std::sqrt(static_cast<Scalar>(1.0) - safe_v2);
			const Scalar h = (d * w > static_cast<Scalar>(0.0)) ? (z / (d * w)) : static_cast<Scalar>(1.0);

			const Scalar f = gamma * p - (gamma - static_cast<Scalar>(1.0)) * (z / (w * w) - d / w);

			if (std::abs(f) < tol_ * std::max(p, static_cast<Scalar>(1.0))) {
				break;
			}

			const Scalar df_dp = gamma - (gamma - static_cast<Scalar>(1.0)) * safe_v2 * (static_cast<Scalar>(1.0) - static_cast<Scalar>(1.0) / std::max(h, static_cast<Scalar>(1e-15)));
			Scalar p_next = p - f / ((std::abs(df_dp) > static_cast<Scalar>(1e-8)) ? df_dp : static_cast<Scalar>(1.0));

			if (p_next < p_min || p_next > p_max || std::isnan(p_next)) {
				p_next = static_cast<Scalar>(0.5) * (p_min + p_max);
			}

			if (f > static_cast<Scalar>(0.0)) {
				p_max = p;
			} else {
				p_min = p;
			}

			p = p_next;
		}

		p = std::max(p_floor_, p);
		const Scalar z = tau + p + d;
		const Scalar v2 = (z > static_cast<Scalar>(1e-15)) ? (s2 / (z * z)) : static_cast<Scalar>(0.0);
		const Scalar safe_v2 = (v2 < static_cast<Scalar>(1.0 - 1e-15)) ? v2 : static_cast<Scalar>(1.0 - 1e-15);
		const Scalar w = static_cast<Scalar>(1.0) / std::sqrt(static_cast<Scalar>(1.0) - safe_v2);
		const Scalar rho = std::max(rho_floor_, d / w);
		const Scalar inv_z = (z > static_cast<Scalar>(1e-15)) ? (static_cast<Scalar>(1.0) / z) : static_cast<Scalar>(0.0);

		PrimitiveVariables<Scalar> prim;
		prim.rho = rho;
		prim.p = p;
		prim.vx = con.sx * inv_z;
		prim.vy = con.sy * inv_z;
		prim.vz = con.sz * inv_z;
		prim.bx = static_cast<Scalar>(0.0);
		prim.by = static_cast<Scalar>(0.0);
		prim.bz = static_cast<Scalar>(0.0);
		prim.w = w;
		prim.h = eos_.specific_enthalpy(rho, p);
		prim.eps = eos_.specific_internal_energy(rho, p);
		prim.b2 = static_cast<Scalar>(0.0);

		return prim;
	}

	[[nodiscard]] PrimitiveVariables<Scalar> solve_mhd(const ConservedVariables<Scalar>& con) const noexcept {
		const Scalar d = std::max(con.d, rho_floor_);
		const Scalar s2 = con.sx * con.sx + con.sy * con.sy + con.sz * con.sz;
		const Scalar b2_raw = con.bx * con.bx + con.by * con.by + con.bz * con.bz;
		const Scalar s_dot_b = con.sx * con.bx + con.sy * con.by + con.sz * con.bz;
		const Scalar s_dot_b_sq = s_dot_b * s_dot_b;
		const Scalar tau = std::max(con.tau, static_cast<Scalar>(0.0));
		const Scalar gamma = eos_.gamma();

		if (b2_raw < static_cast<Scalar>(1e-15)) {
			return solve_hydro(con);
		}

		Scalar z = std::max(std::sqrt(s2), d + tau);
		Scalar z_min = std::max(std::sqrt(s2) * static_cast<Scalar>(0.5), static_cast<Scalar>(1e-12));
		Scalar z_max = static_cast<Scalar>(100.0) * (d + tau + b2_raw);

		for (size_t iter = 0; iter < max_iterations_; ++iter) {
			const Scalar z_plus_b2 = z + b2_raw;
			const Scalar z2 = z * z;
			const Scalar num_v2 = s2 * z2 + s_dot_b_sq * (static_cast<Scalar>(2.0) * z + b2_raw);
			const Scalar den_v2 = z2 * z_plus_b2 * z_plus_b2;
			const Scalar v2 = (den_v2 > static_cast<Scalar>(1e-30)) ? (num_v2 / den_v2) : static_cast<Scalar>(0.0);
			const Scalar safe_v2 = (v2 < static_cast<Scalar>(1.0 - 1e-15)) ? v2 : static_cast<Scalar>(1.0 - 1e-15);
			const Scalar w = static_cast<Scalar>(1.0) / std::sqrt(static_cast<Scalar>(1.0) - safe_v2);

			const Scalar b_dot_v = (z > static_cast<Scalar>(1e-15)) ? (s_dot_b / z) : static_cast<Scalar>(0.0);
			const Scalar b2_comov = b2_raw / (w * w) + b_dot_v * b_dot_v;
			const Scalar p = z + b2_raw - static_cast<Scalar>(0.5) * b2_comov - tau - d;
			const Scalar rho = std::max(rho_floor_, d / w);
			const Scalar h = (d * w > static_cast<Scalar>(1e-15)) ? (z / (d * w)) : static_cast<Scalar>(1.0);
			const Scalar p_eos = eos_.pressure_from_enthalpy(rho, h);

			const Scalar f = p - p_eos;

			if (std::abs(f) < tol_ * std::max(z, static_cast<Scalar>(1.0))) {
				break;
			}

			const Scalar df_dz = static_cast<Scalar>(1.0) + static_cast<Scalar>(0.5) * (gamma - static_cast<Scalar>(1.0));
			Scalar z_next = z - f / df_dz;

			if (z_next < z_min || z_next > z_max || std::isnan(z_next)) {
				z_next = static_cast<Scalar>(0.5) * (z_min + z_max);
			}

			if (f > static_cast<Scalar>(0.0)) {
				z_max = z;
			} else {
				z_min = z;
			}

			z = z_next;
		}

		const Scalar z_plus_b2 = z + b2_raw;
		const Scalar z2 = z * z;
		const Scalar num_v2 = s2 * z2 + s_dot_b_sq * (static_cast<Scalar>(2.0) * z + b2_raw);
		const Scalar den_v2 = z2 * z_plus_b2 * z_plus_b2;
		const Scalar v2 = (den_v2 > static_cast<Scalar>(1e-30)) ? (num_v2 / den_v2) : static_cast<Scalar>(0.0);
		const Scalar safe_v2 = (v2 < static_cast<Scalar>(1.0 - 1e-15)) ? v2 : static_cast<Scalar>(1.0 - 1e-15);
		const Scalar w = static_cast<Scalar>(1.0) / std::sqrt(static_cast<Scalar>(1.0) - safe_v2);

		const Scalar b_dot_v = (z > static_cast<Scalar>(1e-15)) ? (s_dot_b / z) : static_cast<Scalar>(0.0);
		const Scalar b2_comov = b2_raw / (w * w) + b_dot_v * b_dot_v;
		const Scalar p = std::max(p_floor_, z + b2_raw - static_cast<Scalar>(0.5) * b2_comov - tau - d);
		const Scalar rho = std::max(rho_floor_, d / w);

		const Scalar factor_s = static_cast<Scalar>(1.0) / z_plus_b2;
		const Scalar factor_b = (z > static_cast<Scalar>(1e-15)) ? (s_dot_b / (z * z_plus_b2)) : static_cast<Scalar>(0.0);

		PrimitiveVariables<Scalar> prim;
		prim.rho = rho;
		prim.p = p;
		prim.vx = factor_s * con.sx + factor_b * con.bx;
		prim.vy = factor_s * con.sy + factor_b * con.by;
		prim.vz = factor_s * con.sz + factor_b * con.bz;
		prim.bx = con.bx;
		prim.by = con.by;
		prim.bz = con.bz;
		prim.w = w;
		prim.h = eos_.specific_enthalpy(rho, p);
		prim.eps = eos_.specific_internal_energy(rho, p);
		prim.b2 = b2_comov;

		return prim;
	}

	[[nodiscard]] PrimitiveVariables<Scalar> solve(const ConservedVariables<Scalar>& con) const noexcept {
		const Scalar b2_raw = con.bx * con.bx + con.by * con.by + con.bz * con.bz;
		if (b2_raw > static_cast<Scalar>(1e-15)) {
			return solve_mhd(con);
		}
		return solve_hydro(con);
	}
};

}

#pragma once

#include "relativistic/hydro/hydro_types.hpp"
#include <span>
#include <cmath>
#include <algorithm>
#include <array>

namespace Relativistic::Hydro {

enum class ReconstructionMethod : uint32_t {
	PiecewiseConstant = 0,
	MinMod = 1,
	MonotonizedCentral = 2,
	WENO5_JS = 3,
	WENO5_Z = 4,
	MP5 = 5
};

template <typename Scalar = double>
class WENO5Reconstructor {
private:
	static constexpr Scalar EPS = static_cast<Scalar>(1e-12);

public:
	[[nodiscard]] static constexpr Scalar reconstruct_left_face(
		Scalar q_m2,
		Scalar q_m1,
		Scalar q_0,
		Scalar q_p1,
		Scalar q_p2,
		bool use_weno_z = true
	) noexcept {
		const Scalar p0 = (static_cast<Scalar>(2.0) * q_m2 - static_cast<Scalar>(7.0) * q_m1 + static_cast<Scalar>(11.0) * q_0) / static_cast<Scalar>(6.0);
		const Scalar p1 = (-q_m1 + static_cast<Scalar>(5.0) * q_0 + static_cast<Scalar>(2.0) * q_p1) / static_cast<Scalar>(6.0);
		const Scalar p2 = (static_cast<Scalar>(2.0) * q_0 + static_cast<Scalar>(5.0) * q_p1 - q_p2) / static_cast<Scalar>(6.0);

		const Scalar d01 = q_m2 - static_cast<Scalar>(2.0) * q_m1 + q_0;
		const Scalar d02 = q_m2 - static_cast<Scalar>(4.0) * q_m1 + static_cast<Scalar>(3.0) * q_0;
		const Scalar beta0 = (static_cast<Scalar>(13.0) / static_cast<Scalar>(12.0)) * d01 * d01 + static_cast<Scalar>(0.25) * d02 * d02;

		const Scalar d11 = q_m1 - static_cast<Scalar>(2.0) * q_0 + q_p1;
		const Scalar d12 = q_m1 - q_p1;
		const Scalar beta1 = (static_cast<Scalar>(13.0) / static_cast<Scalar>(12.0)) * d11 * d11 + static_cast<Scalar>(0.25) * d12 * d12;

		const Scalar d21 = q_0 - static_cast<Scalar>(2.0) * q_p1 + q_p2;
		const Scalar d22 = static_cast<Scalar>(3.0) * q_0 - static_cast<Scalar>(4.0) * q_p1 + q_p2;
		const Scalar beta2 = (static_cast<Scalar>(13.0) / static_cast<Scalar>(12.0)) * d21 * d21 + static_cast<Scalar>(0.25) * d22 * d22;

		constexpr Scalar d0 = static_cast<Scalar>(0.1);
		constexpr Scalar d1 = static_cast<Scalar>(0.6);
		constexpr Scalar d2 = static_cast<Scalar>(0.3);

		Scalar w0 = static_cast<Scalar>(0.0);
		Scalar w1 = static_cast<Scalar>(0.0);
		Scalar w2 = static_cast<Scalar>(0.0);

		if (use_weno_z) {
			const Scalar tau5 = std::abs(beta0 - beta2);
			const Scalar a0 = d0 * (static_cast<Scalar>(1.0) + (tau5 / (beta0 + EPS)) * (tau5 / (beta0 + EPS)));
			const Scalar a1 = d1 * (static_cast<Scalar>(1.0) + (tau5 / (beta1 + EPS)) * (tau5 / (beta1 + EPS)));
			const Scalar a2 = d2 * (static_cast<Scalar>(1.0) + (tau5 / (beta2 + EPS)) * (tau5 / (beta2 + EPS)));
			const Scalar inv_sum = static_cast<Scalar>(1.0) / (a0 + a1 + a2);
			w0 = a0 * inv_sum;
			w1 = a1 * inv_sum;
			w2 = a2 * inv_sum;
		} else {
			const Scalar a0 = d0 / ((EPS + beta0) * (EPS + beta0));
			const Scalar a1 = d1 / ((EPS + beta1) * (EPS + beta1));
			const Scalar a2 = d2 / ((EPS + beta2) * (EPS + beta2));
			const Scalar inv_sum = static_cast<Scalar>(1.0) / (a0 + a1 + a2);
			w0 = a0 * inv_sum;
			w1 = a1 * inv_sum;
			w2 = a2 * inv_sum;
		}

		return w0 * p0 + w1 * p1 + w2 * p2;
	}

	[[nodiscard]] static constexpr Scalar reconstruct_right_face(
		Scalar q_m1,
		Scalar q_0,
		Scalar q_p1,
		Scalar q_p2,
		Scalar q_p3,
		bool use_weno_z = true
	) noexcept {
		return reconstruct_left_face(q_p3, q_p2, q_p1, q_0, q_m1, use_weno_z);
	}
};

template <typename Scalar = double>
class MP5Reconstructor {
private:
	static constexpr Scalar ALPHA = static_cast<Scalar>(4.0);
	static constexpr Scalar EPS = static_cast<Scalar>(1e-12);

	[[nodiscard]] static constexpr Scalar minmod(Scalar a, Scalar b) noexcept {
		if (a * b <= static_cast<Scalar>(0.0)) return static_cast<Scalar>(0.0);
		return (a > static_cast<Scalar>(0.0)) ? std::min(a, b) : std::max(a, b);
	}

	[[nodiscard]] static constexpr Scalar minmod4(Scalar a, Scalar b, Scalar c, Scalar d) noexcept {
		return minmod(minmod(a, b), minmod(c, d));
	}

	[[nodiscard]] static constexpr Scalar median(Scalar a, Scalar b, Scalar c) noexcept {
		return std::max(std::min(a, b), std::min(std::max(a, b), c));
	}

public:
	[[nodiscard]] static constexpr Scalar reconstruct_left_face(
		Scalar q_m2,
		Scalar q_m1,
		Scalar q_0,
		Scalar q_p1,
		Scalar q_p2
	) noexcept {
		const Scalar q_orig = (static_cast<Scalar>(2.0) * q_m2 - static_cast<Scalar>(13.0) * q_m1 + static_cast<Scalar>(47.0) * q_0 + static_cast<Scalar>(27.0) * q_p1 - static_cast<Scalar>(3.0) * q_p2) / static_cast<Scalar>(60.0);

		const Scalar q_mp = q_0 + minmod(q_p1 - q_0, ALPHA * (q_0 - q_m1));
		if ((q_orig - q_0) * (q_orig - q_mp) <= EPS) {
			return q_orig;
		}

		const Scalar d_m1 = q_0 - static_cast<Scalar>(2.0) * q_m1 + q_m2;
		const Scalar d_0 = q_p1 - static_cast<Scalar>(2.0) * q_0 + q_m1;
		const Scalar d_p1 = q_p2 - static_cast<Scalar>(2.0) * q_p1 + q_0;

		const Scalar dm4_0 = minmod4(ALPHA * d_0 - d_p1, ALPHA * d_p1 - d_0, d_0, d_p1);
		const Scalar dm4_m1 = minmod4(ALPHA * d_m1 - d_0, ALPHA * d_0 - d_m1, d_m1, d_0);

		const Scalar q_ul = q_0 + ALPHA * (q_0 - q_m1);
		const Scalar q_md = static_cast<Scalar>(0.5) * (q_0 + q_p1) - static_cast<Scalar>(0.5) * dm4_0;
		const Scalar q_lc = q_0 + static_cast<Scalar>(0.5) * (q_0 - q_m1) + (static_cast<Scalar>(4.0) / static_cast<Scalar>(3.0)) * dm4_m1;

		const Scalar q_min = std::max(std::min({q_0, q_p1, q_md}), std::min({q_0, q_ul, q_lc}));
		const Scalar q_max = std::min(std::max({q_0, q_p1, q_md}), std::max({q_0, q_ul, q_lc}));

		return median(q_orig, q_min, q_max);
	}

	[[nodiscard]] static constexpr Scalar reconstruct_right_face(
		Scalar q_m1,
		Scalar q_0,
		Scalar q_p1,
		Scalar q_p2,
		Scalar q_p3
	) noexcept {
		return reconstruct_left_face(q_p3, q_p2, q_p1, q_0, q_m1);
	}
};

template <typename Scalar = double>
class TVDReconstructor {
private:
	[[nodiscard]] static constexpr Scalar minmod(Scalar a, Scalar b) noexcept {
		if (a * b <= static_cast<Scalar>(0.0)) return static_cast<Scalar>(0.0);
		return (a > static_cast<Scalar>(0.0)) ? std::min(a, b) : std::max(a, b);
	}

	[[nodiscard]] static constexpr Scalar mc_limiter(Scalar a, Scalar b) noexcept {
		if (a * b <= static_cast<Scalar>(0.0)) return static_cast<Scalar>(0.0);
		const Scalar c = static_cast<Scalar>(0.5) * (a + b);
		const Scalar two_a = static_cast<Scalar>(2.0) * a;
		const Scalar two_b = static_cast<Scalar>(2.0) * b;
		if (a > static_cast<Scalar>(0.0)) return std::min({two_a, two_b, c});
		return std::max({two_a, two_b, c});
	}

public:
	[[nodiscard]] static constexpr std::pair<Scalar, Scalar> reconstruct_cell_faces(
		Scalar q_m1,
		Scalar q_0,
		Scalar q_p1,
		bool use_mc = true
	) noexcept {
		const Scalar delta_l = q_0 - q_m1;
		const Scalar delta_r = q_p1 - q_0;
		const Scalar slope = use_mc ? mc_limiter(delta_l, delta_r) : minmod(delta_l, delta_r);

		const Scalar q_left = q_0 - static_cast<Scalar>(0.5) * slope;
		const Scalar q_right = q_0 + static_cast<Scalar>(0.5) * slope;
		return {q_left, q_right};
	}
};

template <typename Scalar = double>
[[nodiscard]] inline PrimitiveVariables<Scalar> reconstruct_primitive_variable_component(
	const PrimitiveVariables<Scalar>& q_m2,
	const PrimitiveVariables<Scalar>& q_m1,
	const PrimitiveVariables<Scalar>& q_0,
	const PrimitiveVariables<Scalar>& q_p1,
	const PrimitiveVariables<Scalar>& q_p2,
	ReconstructionMethod method,
	bool is_left_face = true
) noexcept {
	static_cast<void>(is_left_face);
	auto reconstruct_scalar = [&](Scalar m2, Scalar m1, Scalar c0, Scalar p1, Scalar p2) noexcept -> Scalar {
		switch (method) {
			case ReconstructionMethod::WENO5_JS:
				return WENO5Reconstructor<Scalar>::reconstruct_left_face(m2, m1, c0, p1, p2, false);
			case ReconstructionMethod::MP5:
				return MP5Reconstructor<Scalar>::reconstruct_left_face(m2, m1, c0, p1, p2);
			case ReconstructionMethod::MinMod: {
				const auto [ql, qr] = TVDReconstructor<Scalar>::reconstruct_cell_faces(m1, c0, p1, false);
				return qr;
			}
			case ReconstructionMethod::MonotonizedCentral: {
				const auto [ql, qr] = TVDReconstructor<Scalar>::reconstruct_cell_faces(m1, c0, p1, true);
				return qr;
			}
			case ReconstructionMethod::WENO5_Z:
			default:
				return WENO5Reconstructor<Scalar>::reconstruct_left_face(m2, m1, c0, p1, p2, true);
		}
	};

	const Scalar rho = std::max(static_cast<Scalar>(1e-15), reconstruct_scalar(q_m2.rho, q_m1.rho, q_0.rho, q_p1.rho, q_p2.rho));
	const Scalar p = std::max(static_cast<Scalar>(1e-15), reconstruct_scalar(q_m2.p, q_m1.p, q_0.p, q_p1.p, q_p2.p));
	Scalar vx = reconstruct_scalar(q_m2.vx, q_m1.vx, q_0.vx, q_p1.vx, q_p2.vx);
	Scalar vy = reconstruct_scalar(q_m2.vy, q_m1.vy, q_0.vy, q_p1.vy, q_p2.vy);
	Scalar vz = reconstruct_scalar(q_m2.vz, q_m1.vz, q_0.vz, q_p1.vz, q_p2.vz);

	const Scalar v2 = vx * vx + vy * vy + vz * vz;
	if (v2 >= static_cast<Scalar>(1.0 - 1e-14)) {
		const Scalar factor = std::sqrt(static_cast<Scalar>(1.0 - 1e-14) / v2);
		vx *= factor;
		vy *= factor;
		vz *= factor;
	}

	const Scalar bx = reconstruct_scalar(q_m2.bx, q_m1.bx, q_0.bx, q_p1.bx, q_p2.bx);
	const Scalar by = reconstruct_scalar(q_m2.by, q_m1.by, q_0.by, q_p1.by, q_p2.by);
	const Scalar bz = reconstruct_scalar(q_m2.bz, q_m1.bz, q_0.bz, q_p1.bz, q_p2.bz);

	PrimitiveVariables<Scalar> res(rho, p, vx, vy, vz, bx, by, bz);
	return res;
}

}

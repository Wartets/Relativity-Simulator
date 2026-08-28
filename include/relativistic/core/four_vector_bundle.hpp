#pragma once

#include "simd.hpp"
#include "simd_math.hpp"

namespace Relativistic::Core {

template <typename T, size_t Width>
[[nodiscard]] constexpr SimdVec<T, Width> minkowski_dot(
	const SimdVec<T, Width>& u0,
	const SimdVec<T, Width>& u1,
	const SimdVec<T, Width>& u2,
	const SimdVec<T, Width>& u3,
	const SimdVec<T, Width>& v0,
	const SimdVec<T, Width>& v1,
	const SimdVec<T, Width>& v2,
	const SimdVec<T, Width>& v3,
	T speed_of_light = static_cast<T>(1)
) noexcept {
	const T c2 = speed_of_light * speed_of_light;
	const auto spatial = fma(u3, v3, fma(u2, v2, u1 * v1));
	return spatial - SimdVec<T, Width>(c2) * (u0 * v0);
}

template <typename T, size_t Width>
[[nodiscard]] constexpr SimdVec<T, Width> minkowski_norm_squared(
	const SimdVec<T, Width>& u0,
	const SimdVec<T, Width>& u1,
	const SimdVec<T, Width>& u2,
	const SimdVec<T, Width>& u3,
	T speed_of_light = static_cast<T>(1)
) noexcept {
	return minkowski_dot(u0, u1, u2, u3, u0, u1, u2, u3, speed_of_light);
}

template <typename T, size_t Width>
[[nodiscard]] constexpr SimdVec<T, Width> euclidean_dot3(
	const SimdVec<T, Width>& u1,
	const SimdVec<T, Width>& u2,
	const SimdVec<T, Width>& u3,
	const SimdVec<T, Width>& v1,
	const SimdVec<T, Width>& v2,
	const SimdVec<T, Width>& v3
) noexcept {
	return fma(u3, v3, fma(u2, v2, u1 * v1));
}

template <typename T, size_t Width>
[[nodiscard]] constexpr SimdVec<T, Width> euclidean_norm3(
	const SimdVec<T, Width>& u1,
	const SimdVec<T, Width>& u2,
	const SimdVec<T, Width>& u3
) noexcept {
	return sqrt(euclidean_dot3(u1, u2, u3, u1, u2, u3));
}

template <typename T, size_t Width>
[[nodiscard]] constexpr SimdVec<T, Width> lorentz_factor_from_beta_sq(
	const SimdVec<T, Width>& beta_sq
) noexcept {
	const SimdVec<T, Width> one(static_cast<T>(1));
	constexpr T eps_val = std::is_same_v<T, float> ? static_cast<T>(1e-7f) : static_cast<T>(1e-15);
	const SimdVec<T, Width> eps(eps_val);
	const SimdVec<T, Width> denom = max(one - beta_sq, eps);
	return simd_inv_sqrt(denom);
}

template <typename T, size_t Width>
[[nodiscard]] constexpr SimdVec<T, Width> lorentz_factor(
	const SimdVec<T, Width>& v1,
	const SimdVec<T, Width>& v2,
	const SimdVec<T, Width>& v3,
	T speed_of_light = static_cast<T>(1)
) noexcept {
	const T inv_c2 = static_cast<T>(1) / (speed_of_light * speed_of_light);
	const auto v_sq = euclidean_dot3(v1, v2, v3, v1, v2, v3);
	const auto beta_sq = v_sq * SimdVec<T, Width>(inv_c2);
	return lorentz_factor_from_beta_sq(beta_sq);
}

template <typename T, size_t Width>
struct SimdVelocity3D {
	SimdVec<T, Width> v1;
	SimdVec<T, Width> v2;
	SimdVec<T, Width> v3;
};

template <typename T, size_t Width>
[[nodiscard]] constexpr SimdVelocity3D<T, Width> relativistic_velocity_add(
	const SimdVec<T, Width>& u1,
	const SimdVec<T, Width>& u2,
	const SimdVec<T, Width>& u3,
	const SimdVec<T, Width>& v1,
	const SimdVec<T, Width>& v2,
	const SimdVec<T, Width>& v3,
	T speed_of_light = static_cast<T>(1)
) noexcept {
	const T inv_c2 = static_cast<T>(1) / (speed_of_light * speed_of_light);
	const auto u_dot_v = euclidean_dot3(u1, u2, u3, v1, v2, v3);
	const auto u_sq = euclidean_dot3(u1, u2, u3, u1, u2, u3);
	const auto beta_u_sq = u_sq * SimdVec<T, Width>(inv_c2);
	const auto gamma_u = lorentz_factor_from_beta_sq(beta_u_sq);

	const SimdVec<T, Width> one(static_cast<T>(1));
	const auto denom = one + u_dot_v * SimdVec<T, Width>(inv_c2);
	const auto inv_denom = one / denom;

	const auto inv_gamma_u = one / gamma_u;
	const auto factor_parallel = (gamma_u / (gamma_u + one)) * (u_dot_v * SimdVec<T, Width>(inv_c2));

	SimdVelocity3D<T, Width> result;
	result.v1 = (u1 + v1 * inv_gamma_u + u1 * factor_parallel) * inv_denom;
	result.v2 = (u2 + v2 * inv_gamma_u + u2 * factor_parallel) * inv_denom;
	result.v3 = (u3 + v3 * inv_gamma_u + u3 * factor_parallel) * inv_denom;
	return result;
}

template <typename T, size_t Width>
[[nodiscard]] constexpr SimdVec<T, Width> doppler_shift_factor(
	const SimdVec<T, Width>& p0_obs,
	const SimdVec<T, Width>& p1_obs,
	const SimdVec<T, Width>& p2_obs,
	const SimdVec<T, Width>& p3_obs,
	const SimdVec<T, Width>& u0_obs,
	const SimdVec<T, Width>& u1_obs,
	const SimdVec<T, Width>& u2_obs,
	const SimdVec<T, Width>& u3_obs,
	const SimdVec<T, Width>& p0_emit,
	const SimdVec<T, Width>& p1_emit,
	const SimdVec<T, Width>& p2_emit,
	const SimdVec<T, Width>& p3_emit,
	const SimdVec<T, Width>& u0_emit,
	const SimdVec<T, Width>& u1_emit,
	const SimdVec<T, Width>& u2_emit,
	const SimdVec<T, Width>& u3_emit,
	T speed_of_light = static_cast<T>(1)
) noexcept {
	const auto num = minkowski_dot(p0_obs, p1_obs, p2_obs, p3_obs, u0_obs, u1_obs, u2_obs, u3_obs, speed_of_light);
	const auto den = minkowski_dot(p0_emit, p1_emit, p2_emit, p3_emit, u0_emit, u1_emit, u2_emit, u3_emit, speed_of_light);
	return num / den;
}

}

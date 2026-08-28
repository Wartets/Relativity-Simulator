#pragma once

#include "simd.hpp"
#include <cmath>
#include <numbers>

namespace Relativistic::Core {

template <typename T, size_t Width>
[[nodiscard]] constexpr SimdVec<T, Width> simd_sin(const SimdVec<T, Width>& x) noexcept {
	SimdVec<T, Width> result;
	for (size_t i = 0; i < Width; ++i) {
		result[i] = std::sin(x[i]);
	}
	return result;
}

template <typename T, size_t Width>
[[nodiscard]] constexpr SimdVec<T, Width> simd_cos(const SimdVec<T, Width>& x) noexcept {
	SimdVec<T, Width> result;
	for (size_t i = 0; i < Width; ++i) {
		result[i] = std::cos(x[i]);
	}
	return result;
}

template <typename T, size_t Width>
struct SimdSinCosResult {
	SimdVec<T, Width> sin_val;
	SimdVec<T, Width> cos_val;
};

template <typename T, size_t Width>
[[nodiscard]] constexpr SimdSinCosResult<T, Width> simd_sincos(const SimdVec<T, Width>& x) noexcept {
	SimdSinCosResult<T, Width> res;
	for (size_t i = 0; i < Width; ++i) {
		res.sin_val[i] = std::sin(x[i]);
		res.cos_val[i] = std::cos(x[i]);
	}
	return res;
}

template <typename T, size_t Width>
[[nodiscard]] constexpr SimdVec<T, Width> simd_exp(const SimdVec<T, Width>& x) noexcept {
	SimdVec<T, Width> result;
	for (size_t i = 0; i < Width; ++i) {
		result[i] = std::exp(x[i]);
	}
	return result;
}

template <typename T, size_t Width>
[[nodiscard]] constexpr SimdVec<T, Width> simd_log(const SimdVec<T, Width>& x) noexcept {
	SimdVec<T, Width> result;
	for (size_t i = 0; i < Width; ++i) {
		result[i] = std::log(x[i]);
	}
	return result;
}

template <typename T, size_t Width>
[[nodiscard]] constexpr SimdVec<T, Width> simd_pow(
	const SimdVec<T, Width>& base,
	const SimdVec<T, Width>& exponent
) noexcept {
	SimdVec<T, Width> result;
	for (size_t i = 0; i < Width; ++i) {
		result[i] = std::pow(base[i], exponent[i]);
	}
	return result;
}

template <typename T, size_t Width>
[[nodiscard]] constexpr SimdVec<T, Width> simd_inv_sqrt(const SimdVec<T, Width>& x) noexcept {
	return SimdVec<T, Width>(static_cast<T>(1)) / sqrt(x);
}

template <typename T, size_t Width>
[[nodiscard]] constexpr SimdVec<T, Width> simd_atan2(
	const SimdVec<T, Width>& y,
	const SimdVec<T, Width>& x
) noexcept {
	SimdVec<T, Width> result;
	for (size_t i = 0; i < Width; ++i) {
		result[i] = std::atan2(y[i], x[i]);
	}
	return result;
}

template <typename T, size_t Width>
[[nodiscard]] constexpr SimdVec<T, Width> simd_hypot(
	const SimdVec<T, Width>& x,
	const SimdVec<T, Width>& y
) noexcept {
	return sqrt(fma(x, x, y * y));
}

}

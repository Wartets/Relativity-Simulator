#pragma once

#include "tensor.hpp"
#include <array>
#include <cmath>
#include <concepts>
#include <algorithm>

namespace Relativistic::Core {

template <typename T, size_t Dim>
[[nodiscard]] constexpr Tensor<T, 2, Dim> identity_matrix() noexcept {
	Tensor<T, 2, Dim> result;
	for (size_t i = 0; i < Dim; ++i) {
		result(i, i) = static_cast<T>(1);
	}
	return result;
}

template <typename T, size_t Dim>
[[nodiscard]] constexpr Tensor<T, 2, Dim> kronecker_delta() noexcept {
	return identity_matrix<T, Dim>();
}

template <typename T, size_t R1, size_t R2, size_t Dim>
[[nodiscard]] constexpr Tensor<T, R1 + R2, Dim> tensor_product(
	const Tensor<T, R1, Dim>& a,
	const Tensor<T, R2, Dim>& b
) noexcept {
	Tensor<T, R1 + R2, Dim> result;
	constexpr size_t size_a = Tensor<T, R1, Dim>::TOTAL_ELEMENTS;
	constexpr size_t size_b = Tensor<T, R2, Dim>::TOTAL_ELEMENTS;

	for (size_t i = 0; i < size_a; ++i) {
		const T val_a = a[i];
		const size_t offset = i * size_b;
		for (size_t j = 0; j < size_b; ++j) {
			result[offset + j] = val_a * b[j];
		}
	}
	return result;
}

template <typename T, size_t Dim>
[[nodiscard]] constexpr Tensor<T, 2, Dim> outer_product(
	const Tensor<T, 1, Dim>& u,
	const Tensor<T, 1, Dim>& v
) noexcept {
	return tensor_product(u, v);
}

template <size_t IndexA, size_t IndexB, typename T, size_t Rank, size_t Dim>
	requires (Rank >= 2 && IndexA < Rank && IndexB < Rank && IndexA != IndexB)
[[nodiscard]] constexpr Tensor<T, Rank - 2, Dim> contract(const Tensor<T, Rank, Dim>& tensor) noexcept {
	constexpr size_t ResultRank = Rank - 2;
	Tensor<T, ResultRank, Dim> result;

	constexpr size_t TotalResultElements = Tensor<T, ResultRank, Dim>::TOTAL_ELEMENTS;

	for (size_t res_flat = 0; res_flat < TotalResultElements; ++res_flat) {
		const auto res_coords = Tensor<T, ResultRank, Dim>::flat_to_coordinates(res_flat);

		T sum = static_cast<T>(0);
		for (size_t k = 0; k < Dim; ++k) {
			std::array<size_t, Rank> orig_coords{};
			size_t res_idx = 0;
			for (size_t orig_idx = 0; orig_idx < Rank; ++orig_idx) {
				if (orig_idx == IndexA || orig_idx == IndexB) {
					orig_coords[orig_idx] = k;
				} else {
					orig_coords[orig_idx] = res_coords[res_idx++];
				}
			}
			sum += tensor.at_coordinates(orig_coords);
		}
		result[res_flat] = sum;
	}
	return result;
}

template <size_t IndexA_in_A, size_t IndexB_in_B, typename T, size_t R1, size_t R2, size_t Dim>
	requires (R1 >= 1 && R2 >= 1 && IndexA_in_A < R1 && IndexB_in_B < R2)
[[nodiscard]] constexpr Tensor<T, R1 + R2 - 2, Dim> contract_tensors(
	const Tensor<T, R1, Dim>& a,
	const Tensor<T, R2, Dim>& b
) noexcept {
	const auto prod = tensor_product(a, b);
	constexpr size_t ActualIndexB = R1 + IndexB_in_B;
	return contract<IndexA_in_A, ActualIndexB>(prod);
}

template <typename T, size_t Dim>
[[nodiscard]] constexpr T trace(const Tensor<T, 2, Dim>& matrix) noexcept {
	T tr = static_cast<T>(0);
	for (size_t i = 0; i < Dim; ++i) {
		tr += matrix(i, i);
	}
	return tr;
}

template <typename T, size_t Dim>
[[nodiscard]] constexpr Tensor<T, 2, Dim> symmetrize(const Tensor<T, 2, Dim>& matrix) noexcept {
	Tensor<T, 2, Dim> result;
	const T half = static_cast<T>(0.5);
	for (size_t i = 0; i < Dim; ++i) {
		for (size_t j = 0; j < Dim; ++j) {
			result(i, j) = half * (matrix(i, j) + matrix(j, i));
		}
	}
	return result;
}

template <typename T, size_t Dim>
[[nodiscard]] constexpr Tensor<T, 2, Dim> antisymmetrize(const Tensor<T, 2, Dim>& matrix) noexcept {
	Tensor<T, 2, Dim> result;
	const T half = static_cast<T>(0.5);
	for (size_t i = 0; i < Dim; ++i) {
		for (size_t j = 0; j < Dim; ++j) {
			result(i, j) = half * (matrix(i, j) - matrix(j, i));
		}
	}
	return result;
}

template <typename T, size_t Dim>
[[nodiscard]] constexpr Tensor<T, 2, Dim> matrix_multiply(
	const Tensor<T, 2, Dim>& a,
	const Tensor<T, 2, Dim>& b
) noexcept {
	Tensor<T, 2, Dim> result;
	for (size_t i = 0; i < Dim; ++i) {
		for (size_t k = 0; k < Dim; ++k) {
			const T a_ik = a(i, k);
			for (size_t j = 0; j < Dim; ++j) {
				result(i, j) += a_ik * b(k, j);
			}
		}
	}
	return result;
}

template <typename T>
[[nodiscard]] constexpr T determinant_4x4(const Tensor<T, 2, 4>& m) noexcept {
	const T a00 = m(0, 0), a01 = m(0, 1), a02 = m(0, 2), a03 = m(0, 3);
	const T a10 = m(1, 0), a11 = m(1, 1), a12 = m(1, 2), a13 = m(1, 3);
	const T a20 = m(2, 0), a21 = m(2, 1), a22 = m(2, 2), a23 = m(2, 3);
	const T a30 = m(3, 0), a31 = m(3, 1), a32 = m(3, 2), a33 = m(3, 3);

	const T s0 = a00 * a11 - a01 * a10;
	const T s1 = a00 * a12 - a02 * a10;
	const T s2 = a00 * a13 - a03 * a10;
	const T s3 = a01 * a12 - a02 * a11;
	const T s4 = a01 * a13 - a03 * a11;
	const T s5 = a02 * a13 - a03 * a12;

	const T c5 = a22 * a33 - a23 * a32;
	const T c4 = a21 * a33 - a23 * a31;
	const T c3 = a21 * a32 - a22 * a31;
	const T c2 = a20 * a33 - a23 * a30;
	const T c1 = a20 * a32 - a22 * a30;
	const T c0 = a20 * a31 - a21 * a30;

	return s0 * c5 - s1 * c4 + s2 * c3 + s3 * c2 - s4 * c1 + s5 * c0;
}

template <typename T>
[[nodiscard]] constexpr Tensor<T, 2, 4> inverse_metric_4x4(const Tensor<T, 2, 4>& m) noexcept {
	const T a00 = m(0, 0), a01 = m(0, 1), a02 = m(0, 2), a03 = m(0, 3);
	const T a10 = m(1, 0), a11 = m(1, 1), a12 = m(1, 2), a13 = m(1, 3);
	const T a20 = m(2, 0), a21 = m(2, 1), a22 = m(2, 2), a23 = m(2, 3);
	const T a30 = m(3, 0), a31 = m(3, 1), a32 = m(3, 2), a33 = m(3, 3);

	const T s0 = a00 * a11 - a01 * a10;
	const T s1 = a00 * a12 - a02 * a10;
	const T s2 = a00 * a13 - a03 * a10;
	const T s3 = a01 * a12 - a02 * a11;
	const T s4 = a01 * a13 - a03 * a11;
	const T s5 = a02 * a13 - a03 * a12;

	const T c5 = a22 * a33 - a23 * a32;
	const T c4 = a21 * a33 - a23 * a31;
	const T c3 = a21 * a32 - a22 * a31;
	const T c2 = a20 * a33 - a23 * a30;
	const T c1 = a20 * a32 - a22 * a30;
	const T c0 = a20 * a31 - a21 * a30;

	const T det = s0 * c5 - s1 * c4 + s2 * c3 + s3 * c2 - s4 * c1 + s5 * c0;
	if (std::abs(det) < static_cast<T>(1e-30)) [[unlikely]] {
		return Tensor<T, 2, 4>{};
	}

	const T inv_det = static_cast<T>(1) / det;

	Tensor<T, 2, 4> inv;

	inv(0, 0) = ( a11 * c5 - a12 * c4 + a13 * c3) * inv_det;
	inv(0, 1) = (-a01 * c5 + a02 * c4 - a03 * c3) * inv_det;
	inv(0, 2) = ( a31 * s5 - a32 * s4 + a33 * s3) * inv_det;
	inv(0, 3) = (-a21 * s5 + a22 * s4 - a23 * s3) * inv_det;

	inv(1, 0) = (-a10 * c5 + a12 * c2 - a13 * c1) * inv_det;
	inv(1, 1) = ( a00 * c5 - a02 * c2 + a03 * c1) * inv_det;
	inv(1, 2) = (-a30 * s5 + a32 * s2 - a33 * s1) * inv_det;
	inv(1, 3) = ( a20 * s5 - a22 * s2 + a23 * s1) * inv_det;

	inv(2, 0) = ( a10 * c4 - a11 * c2 + a13 * c0) * inv_det;
	inv(2, 1) = (-a00 * c4 + a01 * c2 - a03 * c0) * inv_det;
	inv(2, 2) = ( a30 * s4 - a31 * s2 + a33 * s0) * inv_det;
	inv(2, 3) = (-a20 * s4 + a21 * s2 - a23 * s0) * inv_det;

	inv(3, 0) = (-a10 * c3 + a11 * c1 - a12 * c0) * inv_det;
	inv(3, 1) = ( a00 * c3 - a01 * c1 + a02 * c0) * inv_det;
	inv(3, 2) = (-a30 * s3 + a31 * s1 - a32 * s0) * inv_det;
	inv(3, 3) = ( a20 * s3 - a21 * s1 + a22 * s0) * inv_det;

	return inv;
}

template <typename T>
[[nodiscard]] constexpr Tensor<T, 2, 4> contract_metric_inverse(
	const Tensor<T, 2, 4>& inv_g,
	const Tensor<T, 2, 4>& g
) noexcept {
	return matrix_multiply(inv_g, g);
}

}

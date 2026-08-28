#include "relativistic/core/simd.hpp"
#include "relativistic/core/simd_math.hpp"
#include <cassert>
#include <cmath>
#include <array>
#include <numeric>

template <typename T, size_t Width>
void test_simd_arithmetic() {
	using namespace Relativistic::Core;

	std::array<T, Width> a_arr{};
	std::array<T, Width> b_arr{};
	for (size_t i = 0; i < Width; ++i) {
		a_arr[i] = static_cast<T>(i + 1);
		b_arr[i] = static_cast<T>((i + 1) * 2);
	}

	const SimdVec<T, Width> va(a_arr);
	const SimdVec<T, Width> vb(b_arr);

	const auto v_sum = va + vb;
	const auto v_sub = vb - va;
	const auto v_mul = va * vb;
	const auto v_div = vb / va;

	for (size_t i = 0; i < Width; ++i) {
		assert(std::abs(v_sum[i] - (a_arr[i] + b_arr[i])) < static_cast<T>(1e-6));
		assert(std::abs(v_sub[i] - (b_arr[i] - a_arr[i])) < static_cast<T>(1e-6));
		assert(std::abs(v_mul[i] - (a_arr[i] * b_arr[i])) < static_cast<T>(1e-6));
		assert(std::abs(v_div[i] - static_cast<T>(2)) < static_cast<T>(1e-6));
	}

	const auto v_fma = fma(va, vb, va);
	for (size_t i = 0; i < Width; ++i) {
		const T expected = a_arr[i] * b_arr[i] + a_arr[i];
		assert(std::abs(v_fma[i] - expected) < static_cast<T>(1e-6));
	}

	const T sum = va.reduce_add();
	T expected_sum = static_cast<T>(0);
	for (size_t i = 0; i < Width; ++i) expected_sum += a_arr[i];
	assert(std::abs(sum - expected_sum) < static_cast<T>(1e-6));

	assert(va.reduce_min() == a_arr[0]);
	assert(va.reduce_max() == a_arr[Width - 1]);
}

template <typename T, size_t Width>
void test_simd_masking_and_select() {
	using namespace Relativistic::Core;

	SimdVec<T, Width> va;
	SimdVec<T, Width> vb;
	for (size_t i = 0; i < Width; ++i) {
		va[i] = static_cast<T>(i);
		vb[i] = static_cast<T>(Width - i);
	}

	const auto mask_lt = (va < vb);
	for (size_t i = 0; i < Width; ++i) {
		assert(mask_lt[i] == (va[i] < vb[i]));
	}

	const auto selected = select(mask_lt, va, vb);
	for (size_t i = 0; i < Width; ++i) {
		const T expected = (va[i] < vb[i]) ? va[i] : vb[i];
		assert(selected[i] == expected);
	}

	const auto mask_all_true = SimdMask<T, Width>(true);
	const auto mask_all_false = SimdMask<T, Width>(false);

	assert(mask_all_true.all());
	assert(!mask_all_true.none());
	assert(mask_all_true.count() == Width);

	assert(!mask_all_false.any());
	assert(mask_all_false.none());
	assert(mask_all_false.count() == 0);

	const auto mask_not = !mask_all_true;
	assert(mask_not == mask_all_false);
}

template <typename T, size_t Width>
void test_simd_math() {
	using namespace Relativistic::Core;

	SimdVec<T, Width> v;
	for (size_t i = 0; i < Width; ++i) {
		v[i] = static_cast<T>(i) * static_cast<T>(0.25);
	}

	const auto v_sin = simd_sin(v);
	const auto v_cos = simd_cos(v);
	const auto sincos_res = simd_sincos(v);

	for (size_t i = 0; i < Width; ++i) {
		assert(std::abs(v_sin[i] - std::sin(v[i])) < static_cast<T>(1e-6));
		assert(std::abs(v_cos[i] - std::cos(v[i])) < static_cast<T>(1e-6));
		assert(std::abs(sincos_res.sin_val[i] - std::sin(v[i])) < static_cast<T>(1e-6));
		assert(std::abs(sincos_res.cos_val[i] - std::cos(v[i])) < static_cast<T>(1e-6));
	}

	const auto identity = fma(v_sin, v_sin, v_cos * v_cos);
	for (size_t i = 0; i < Width; ++i) {
		assert(std::abs(identity[i] - static_cast<T>(1)) < static_cast<T>(1e-6));
	}
}

int main() {
	test_simd_arithmetic<float, 4>();
	test_simd_arithmetic<float, 8>();
	test_simd_arithmetic<float, 16>();
	test_simd_arithmetic<double, 2>();
	test_simd_arithmetic<double, 4>();
	test_simd_arithmetic<double, 8>();

	test_simd_masking_and_select<float, 4>();
	test_simd_masking_and_select<float, 8>();
	test_simd_masking_and_select<float, 16>();
	test_simd_masking_and_select<double, 4>();
	test_simd_masking_and_select<double, 8>();

	test_simd_math<float, 4>();
	test_simd_math<float, 8>();
	test_simd_math<double, 4>();
	test_simd_math<double, 8>();

	return 0;
}

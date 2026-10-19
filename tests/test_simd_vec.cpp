#include "support/test_harness.hpp"
#include "relativistic/core/simd.hpp"
#include "relativistic/core/simd_math.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <type_traits>

using namespace Relativistic::Core;
using Relativistic::Testing::TestSuite;

namespace {

template <typename T, size_t Width>
[[nodiscard]] std::string tag() {
	return std::string(std::is_same_v<T, float> ? "float" : "double") + " x" + std::to_string(Width);
}

[[nodiscard]] double deviation(double a, double b) noexcept {
	return std::abs(a - b);
}

template <typename T, size_t Width>
void check_arithmetic(TestSuite& suite) {
	std::array<T, Width> a{};
	std::array<T, Width> b{};
	for (size_t i = 0; i < Width; ++i) {
		a[i] = static_cast<T>(i + 1);
		b[i] = static_cast<T>((i + 1) * 2);
	}
	const SimdVec<T, Width> va(a);
	const SimdVec<T, Width> vb(b);
	const auto sum = va + vb;
	const auto difference = vb - va;
	const auto product = va * vb;
	const auto quotient = vb / va;
	const auto fused = fma(va, vb, va);
	double worst = 0.0;
	double sum_expected = 0.0;
	for (size_t i = 0; i < Width; ++i) {
		const double x = static_cast<double>(a[i]);
		const double y = static_cast<double>(b[i]);
		worst = std::max({worst, deviation(sum[i], x + y), deviation(difference[i], y - x), deviation(product[i], x * y), deviation(quotient[i], 2.0), deviation(fused[i], x * y + x)});
		sum_expected += x;
	}
	suite.check_at_most(worst, 1e-6, tag<T, Width>() + ": addition, subtraction, multiplication, division and fused multiply-add match scalar arithmetic");
	suite.check(deviation(va.reduce_add(), sum_expected) < 1e-6 && va.reduce_min() == a[0] && va.reduce_max() == a[Width - 1], tag<T, Width>() + ": the horizontal sum, minimum and maximum are exact");
}

template <typename T, size_t Width>
void check_masks(TestSuite& suite) {
	SimdVec<T, Width> va;
	SimdVec<T, Width> vb;
	for (size_t i = 0; i < Width; ++i) {
		va[i] = static_cast<T>(i);
		vb[i] = static_cast<T>(Width - i);
	}
	const auto less = (va < vb);
	const auto selected = select(less, va, vb);
	bool consistent = true;
	for (size_t i = 0; i < Width; ++i) {
		consistent = consistent && (less[i] == (va[i] < vb[i])) && (selected[i] == ((va[i] < vb[i]) ? va[i] : vb[i]));
	}
	suite.check(consistent, tag<T, Width>() + ": comparison masks and blended selection follow the scalar semantics");
	const SimdMask<T, Width> all_true(true);
	const SimdMask<T, Width> all_false(false);
	suite.check(all_true.all() && !all_true.none() && all_true.count() == Width && !all_false.any() && all_false.none() && all_false.count() == 0 && (!all_true) == all_false, tag<T, Width>() + ": uniform masks report their population and negate correctly");
}

template <typename T, size_t Width>
void check_math(TestSuite& suite) {
	SimdVec<T, Width> v;
	for (size_t i = 0; i < Width; ++i) {
		v[i] = static_cast<T>(i) * static_cast<T>(0.25);
	}
	const auto sine = simd_sin(v);
	const auto cosine = simd_cos(v);
	const auto pair = simd_sincos(v);
	const auto identity = fma(sine, sine, cosine * cosine);
	double worst = 0.0;
	for (size_t i = 0; i < Width; ++i) {
		const double x = static_cast<double>(v[i]);
		worst = std::max({worst, deviation(sine[i], std::sin(x)), deviation(cosine[i], std::cos(x)), deviation(pair.sin_val[i], std::sin(x)), deviation(pair.cos_val[i], std::cos(x)), deviation(identity[i], 1.0)});
	}
	suite.check_at_most(worst, 1e-6, tag<T, Width>() + ": sine, cosine and the Pythagorean identity match the scalar library");
}

}

int main() {
	TestSuite suite(
		"simd_vec",
		"Verifies the portable SIMD vector and mask types across element types and widths: elementwise arithmetic, fused multiply-add, horizontal reductions, comparison masks, blended selection, mask population counts and the trigonometric helpers."
	);

	suite.section("Arithmetic and reductions");
	check_arithmetic<float, 4>(suite);
	check_arithmetic<float, 8>(suite);
	check_arithmetic<float, 16>(suite);
	check_arithmetic<double, 2>(suite);
	check_arithmetic<double, 4>(suite);
	check_arithmetic<double, 8>(suite);

	suite.section("Masks and selection");
	check_masks<float, 4>(suite);
	check_masks<float, 8>(suite);
	check_masks<float, 16>(suite);
	check_masks<double, 4>(suite);
	check_masks<double, 8>(suite);

	suite.section("Trigonometric helpers");
	check_math<float, 4>(suite);
	check_math<float, 8>(suite);
	check_math<double, 4>(suite);
	check_math<double, 8>(suite);
	return suite.finish();
}

#pragma once

#include <cstddef>
#include <cstdint>
#include <concepts>
#include <type_traits>
#include <array>
#include <cmath>
#include <algorithm>
#include <bit>

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#include <immintrin.h>
#define RELATIVISTIC_ARCH_X86 1
#elif defined(__ARM_NEON) || defined(__aarch64__) || defined(_M_ARM64)
#include <arm_neon.h>
#define RELATIVISTIC_ARCH_ARM 1
#endif

namespace Relativistic::Core {

template <typename T, size_t Width>
class alignas(sizeof(T) * Width >= 64 ? 64 : (sizeof(T) * Width >= 32 ? 32 : (sizeof(T) * Width >= 16 ? 16 : alignof(T)))) SimdMask {
	static_assert(std::is_arithmetic_v<T>, "SimdMask requires an arithmetic scalar type");
	static_assert((Width & (Width - 1)) == 0 && Width >= 1, "Width must be a power of two");

public:
	static constexpr size_t WIDTH = Width;
	using value_type = bool;

private:
	std::array<bool, Width> mask_;

public:
	constexpr SimdMask() noexcept : mask_{} {}

	explicit constexpr SimdMask(bool initial_value) noexcept {
		mask_.fill(initial_value);
	}

	constexpr SimdMask(const std::array<bool, Width>& raw_mask) noexcept : mask_(raw_mask) {}

	template <typename... Bools>
		requires (sizeof...(Bools) == Width && (std::convertible_to<Bools, bool> && ...))
	explicit constexpr SimdMask(Bools... values) noexcept : mask_{static_cast<bool>(values)...} {}

	[[nodiscard]] static constexpr size_t size() noexcept { return Width; }

	[[nodiscard]] constexpr bool operator[](size_t index) const noexcept {
		return mask_[index];
	}

	[[nodiscard]] constexpr bool& operator[](size_t index) noexcept {
		return mask_[index];
	}

	[[nodiscard]] constexpr bool all() const noexcept {
		for (size_t i = 0; i < Width; ++i) {
			if (!mask_[i]) return false;
		}
		return true;
	}

	[[nodiscard]] constexpr bool any() const noexcept {
		for (size_t i = 0; i < Width; ++i) {
			if (mask_[i]) return true;
		}
		return false;
	}

	[[nodiscard]] constexpr bool none() const noexcept {
		return !any();
	}

	[[nodiscard]] constexpr size_t count() const noexcept {
		size_t active = 0;
		for (size_t i = 0; i < Width; ++i) {
			if (mask_[i]) ++active;
		}
		return active;
	}

	[[nodiscard]] constexpr SimdMask operator!() const noexcept {
		SimdMask result;
		for (size_t i = 0; i < Width; ++i) {
			result.mask_[i] = !mask_[i];
		}
		return result;
	}

	[[nodiscard]] constexpr SimdMask operator&&(const SimdMask& other) const noexcept {
		SimdMask result;
		for (size_t i = 0; i < Width; ++i) {
			result.mask_[i] = mask_[i] && other.mask_[i];
		}
		return result;
	}

	[[nodiscard]] constexpr SimdMask operator||(const SimdMask& other) const noexcept {
		SimdMask result;
		for (size_t i = 0; i < Width; ++i) {
			result.mask_[i] = mask_[i] || other.mask_[i];
		}
		return result;
	}

	[[nodiscard]] constexpr SimdMask operator^(const SimdMask& other) const noexcept {
		SimdMask result;
		for (size_t i = 0; i < Width; ++i) {
			result.mask_[i] = mask_[i] ^ other.mask_[i];
		}
		return result;
	}

	constexpr SimdMask& operator&=(const SimdMask& other) noexcept {
		for (size_t i = 0; i < Width; ++i) {
			mask_[i] = mask_[i] && other.mask_[i];
		}
		return *this;
	}

	constexpr SimdMask& operator|=(const SimdMask& other) noexcept {
		for (size_t i = 0; i < Width; ++i) {
			mask_[i] = mask_[i] || other.mask_[i];
		}
		return *this;
	}

	constexpr SimdMask& operator^=(const SimdMask& other) noexcept {
		for (size_t i = 0; i < Width; ++i) {
			mask_[i] = mask_[i] ^ other.mask_[i];
		}
		return *this;
	}

	[[nodiscard]] constexpr bool operator==(const SimdMask& other) const noexcept {
		for (size_t i = 0; i < Width; ++i) {
			if (mask_[i] != other.mask_[i]) return false;
		}
		return true;
	}

	[[nodiscard]] constexpr bool operator!=(const SimdMask& other) const noexcept {
		return !(*this == other);
	}
};

template <typename T, size_t Width>
class alignas(sizeof(T) * Width >= 64 ? 64 : (sizeof(T) * Width >= 32 ? 32 : (sizeof(T) * Width >= 16 ? 16 : alignof(T)))) SimdVec {
	static_assert(std::is_arithmetic_v<T>, "SimdVec requires an arithmetic scalar type");
	static_assert((Width & (Width - 1)) == 0 && Width >= 1, "Width must be a power of two");

public:
	static constexpr size_t WIDTH = Width;
	using value_type = T;
	using mask_type = SimdMask<T, Width>;
	using size_type = size_t;

private:
	std::array<T, Width> data_;

public:
	constexpr SimdVec() noexcept : data_{} {}

	explicit constexpr SimdVec(T scalar) noexcept {
		data_.fill(scalar);
	}

	constexpr SimdVec(const std::array<T, Width>& raw_data) noexcept : data_(raw_data) {}

	template <typename... Elements>
		requires (sizeof...(Elements) == Width && (std::convertible_to<Elements, T> && ...))
	explicit constexpr SimdVec(Elements... elements) noexcept : data_{static_cast<T>(elements)...} {}

	[[nodiscard]] static constexpr size_t size() noexcept { return Width; }

	[[nodiscard]] static SimdVec load_aligned(const T* ptr) noexcept {
		SimdVec result;
		for (size_t i = 0; i < Width; ++i) {
			result.data_[i] = ptr[i];
		}
		return result;
	}

	[[nodiscard]] static SimdVec load_unaligned(const T* ptr) noexcept {
		SimdVec result;
		for (size_t i = 0; i < Width; ++i) {
			result.data_[i] = ptr[i];
		}
		return result;
	}

	void store_aligned(T* ptr) const noexcept {
		for (size_t i = 0; i < Width; ++i) {
			ptr[i] = data_[i];
		}
	}

	void store_unaligned(T* ptr) const noexcept {
		for (size_t i = 0; i < Width; ++i) {
			ptr[i] = data_[i];
		}
	}

	[[nodiscard]] constexpr T operator[](size_t index) const noexcept {
		return data_[index];
	}

	[[nodiscard]] constexpr T& operator[](size_t index) noexcept {
		return data_[index];
	}

	[[nodiscard]] constexpr const T* data() const noexcept {
		return data_.data();
	}

	[[nodiscard]] constexpr T* data() noexcept {
		return data_.data();
	}

	[[nodiscard]] constexpr SimdVec operator+() const noexcept {
		return *this;
	}

	[[nodiscard]] constexpr SimdVec operator-() const noexcept {
		SimdVec result;
		for (size_t i = 0; i < Width; ++i) {
			result.data_[i] = -data_[i];
		}
		return result;
	}

	[[nodiscard]] constexpr SimdVec operator~() const noexcept requires std::is_integral_v<T> {
		SimdVec result;
		for (size_t i = 0; i < Width; ++i) {
			result.data_[i] = ~data_[i];
		}
		return result;
	}

	[[nodiscard]] constexpr SimdVec operator+(const SimdVec& other) const noexcept {
		SimdVec result;
		for (size_t i = 0; i < Width; ++i) {
			result.data_[i] = data_[i] + other.data_[i];
		}
		return result;
	}

	[[nodiscard]] constexpr SimdVec operator-(const SimdVec& other) const noexcept {
		SimdVec result;
		for (size_t i = 0; i < Width; ++i) {
			result.data_[i] = data_[i] - other.data_[i];
		}
		return result;
	}

	[[nodiscard]] constexpr SimdVec operator*(const SimdVec& other) const noexcept {
		SimdVec result;
		for (size_t i = 0; i < Width; ++i) {
			result.data_[i] = data_[i] * other.data_[i];
		}
		return result;
	}

	[[nodiscard]] constexpr SimdVec operator/(const SimdVec& other) const noexcept {
		SimdVec result;
		for (size_t i = 0; i < Width; ++i) {
			result.data_[i] = data_[i] / other.data_[i];
		}
		return result;
	}

	[[nodiscard]] constexpr SimdVec operator+(T scalar) const noexcept {
		return *this + SimdVec(scalar);
	}

	[[nodiscard]] constexpr SimdVec operator-(T scalar) const noexcept {
		return *this - SimdVec(scalar);
	}

	[[nodiscard]] constexpr SimdVec operator*(T scalar) const noexcept {
		return *this * SimdVec(scalar);
	}

	[[nodiscard]] constexpr SimdVec operator/(T scalar) const noexcept {
		return *this / SimdVec(scalar);
	}

	constexpr SimdVec& operator+=(const SimdVec& other) noexcept {
		for (size_t i = 0; i < Width; ++i) {
			data_[i] += other.data_[i];
		}
		return *this;
	}

	constexpr SimdVec& operator-=(const SimdVec& other) noexcept {
		for (size_t i = 0; i < Width; ++i) {
			data_[i] -= other.data_[i];
		}
		return *this;
	}

	constexpr SimdVec& operator*=(const SimdVec& other) noexcept {
		for (size_t i = 0; i < Width; ++i) {
			data_[i] *= other.data_[i];
		}
		return *this;
	}

	constexpr SimdVec& operator/=(const SimdVec& other) noexcept {
		for (size_t i = 0; i < Width; ++i) {
			data_[i] /= other.data_[i];
		}
		return *this;
	}

	constexpr SimdVec& operator+=(T scalar) noexcept {
		return *this += SimdVec(scalar);
	}

	constexpr SimdVec& operator-=(T scalar) noexcept {
		return *this -= SimdVec(scalar);
	}

	constexpr SimdVec& operator*=(T scalar) noexcept {
		return *this *= SimdVec(scalar);
	}

	constexpr SimdVec& operator/=(T scalar) noexcept {
		return *this /= SimdVec(scalar);
	}

	[[nodiscard]] constexpr SimdVec operator&(const SimdVec& other) const noexcept requires std::is_integral_v<T> {
		SimdVec result;
		for (size_t i = 0; i < Width; ++i) {
			result.data_[i] = data_[i] & other.data_[i];
		}
		return result;
	}

	[[nodiscard]] constexpr SimdVec operator|(const SimdVec& other) const noexcept requires std::is_integral_v<T> {
		SimdVec result;
		for (size_t i = 0; i < Width; ++i) {
			result.data_[i] = data_[i] | other.data_[i];
		}
		return result;
	}

	[[nodiscard]] constexpr SimdVec operator^(const SimdVec& other) const noexcept requires std::is_integral_v<T> {
		SimdVec result;
		for (size_t i = 0; i < Width; ++i) {
			result.data_[i] = data_[i] ^ other.data_[i];
		}
		return result;
	}

	[[nodiscard]] constexpr SimdMask<T, Width> operator==(const SimdVec& other) const noexcept {
		SimdMask<T, Width> result;
		for (size_t i = 0; i < Width; ++i) {
			result[i] = (data_[i] == other.data_[i]);
		}
		return result;
	}

	[[nodiscard]] constexpr SimdMask<T, Width> operator!=(const SimdVec& other) const noexcept {
		SimdMask<T, Width> result;
		for (size_t i = 0; i < Width; ++i) {
			result[i] = (data_[i] != other.data_[i]);
		}
		return result;
	}

	[[nodiscard]] constexpr SimdMask<T, Width> operator<(const SimdVec& other) const noexcept {
		SimdMask<T, Width> result;
		for (size_t i = 0; i < Width; ++i) {
			result[i] = (data_[i] < other.data_[i]);
		}
		return result;
	}

	[[nodiscard]] constexpr SimdMask<T, Width> operator<=(const SimdVec& other) const noexcept {
		SimdMask<T, Width> result;
		for (size_t i = 0; i < Width; ++i) {
			result[i] = (data_[i] <= other.data_[i]);
		}
		return result;
	}

	[[nodiscard]] constexpr SimdMask<T, Width> operator>(const SimdVec& other) const noexcept {
		SimdMask<T, Width> result;
		for (size_t i = 0; i < Width; ++i) {
			result[i] = (data_[i] > other.data_[i]);
		}
		return result;
	}

	[[nodiscard]] constexpr SimdMask<T, Width> operator>=(const SimdVec& other) const noexcept {
		SimdMask<T, Width> result;
		for (size_t i = 0; i < Width; ++i) {
			result[i] = (data_[i] >= other.data_[i]);
		}
		return result;
	}

	[[nodiscard]] constexpr T reduce_add() const noexcept {
		T sum = static_cast<T>(0);
		for (size_t i = 0; i < Width; ++i) {
			sum += data_[i];
		}
		return sum;
	}

	[[nodiscard]] constexpr T reduce_min() const noexcept {
		T val = data_[0];
		for (size_t i = 1; i < Width; ++i) {
			if (data_[i] < val) val = data_[i];
		}
		return val;
	}

	[[nodiscard]] constexpr T reduce_max() const noexcept {
		T val = data_[0];
		for (size_t i = 1; i < Width; ++i) {
			if (data_[i] > val) val = data_[i];
		}
		return val;
	}

	[[nodiscard]] constexpr T dot(const SimdVec& other) const noexcept {
		return (*this * other).reduce_add();
	}
};

template <typename T, size_t Width>
[[nodiscard]] constexpr SimdVec<T, Width> operator+(T scalar, const SimdVec<T, Width>& vec) noexcept {
	return SimdVec<T, Width>(scalar) + vec;
}

template <typename T, size_t Width>
[[nodiscard]] constexpr SimdVec<T, Width> operator-(T scalar, const SimdVec<T, Width>& vec) noexcept {
	return SimdVec<T, Width>(scalar) - vec;
}

template <typename T, size_t Width>
[[nodiscard]] constexpr SimdVec<T, Width> operator*(T scalar, const SimdVec<T, Width>& vec) noexcept {
	return SimdVec<T, Width>(scalar) * vec;
}

template <typename T, size_t Width>
[[nodiscard]] constexpr SimdVec<T, Width> operator/(T scalar, const SimdVec<T, Width>& vec) noexcept {
	return SimdVec<T, Width>(scalar) / vec;
}

template <typename T, size_t Width>
[[nodiscard]] constexpr SimdVec<T, Width> select(
	const SimdMask<T, Width>& mask,
	const SimdVec<T, Width>& if_true,
	const SimdVec<T, Width>& if_false
) noexcept {
	SimdVec<T, Width> result;
	for (size_t i = 0; i < Width; ++i) {
		result[i] = mask[i] ? if_true[i] : if_false[i];
	}
	return result;
}

template <typename T, size_t Width>
[[nodiscard]] constexpr SimdVec<T, Width> blend(
	const SimdMask<T, Width>& mask,
	const SimdVec<T, Width>& if_true,
	const SimdVec<T, Width>& if_false
) noexcept {
	return select(mask, if_true, if_false);
}

template <typename T, size_t Width>
constexpr void masked_assign(
	const SimdMask<T, Width>& mask,
	SimdVec<T, Width>& target,
	const SimdVec<T, Width>& source
) noexcept {
	target = select(mask, source, target);
}

template <typename T, size_t Width>
[[nodiscard]] constexpr SimdVec<T, Width> fma(
	const SimdVec<T, Width>& a,
	const SimdVec<T, Width>& b,
	const SimdVec<T, Width>& c
) noexcept {
	SimdVec<T, Width> result;
	for (size_t i = 0; i < Width; ++i) {
		if constexpr (std::is_floating_point_v<T>) {
			result[i] = std::fma(a[i], b[i], c[i]);
		} else {
			result[i] = a[i] * b[i] + c[i];
		}
	}
	return result;
}

template <typename T, size_t Width>
[[nodiscard]] constexpr SimdVec<T, Width> sqrt(const SimdVec<T, Width>& vec) noexcept {
	SimdVec<T, Width> result;
	for (size_t i = 0; i < Width; ++i) {
		result[i] = std::sqrt(vec[i]);
	}
	return result;
}

template <typename T, size_t Width>
[[nodiscard]] constexpr SimdVec<T, Width> abs(const SimdVec<T, Width>& vec) noexcept {
	SimdVec<T, Width> result;
	for (size_t i = 0; i < Width; ++i) {
		result[i] = std::abs(vec[i]);
	}
	return result;
}

template <typename T, size_t Width>
[[nodiscard]] constexpr SimdVec<T, Width> min(
	const SimdVec<T, Width>& a,
	const SimdVec<T, Width>& b
) noexcept {
	SimdVec<T, Width> result;
	for (size_t i = 0; i < Width; ++i) {
		result[i] = std::min(a[i], b[i]);
	}
	return result;
}

template <typename T, size_t Width>
[[nodiscard]] constexpr SimdVec<T, Width> max(
	const SimdVec<T, Width>& a,
	const SimdVec<T, Width>& b
) noexcept {
	SimdVec<T, Width> result;
	for (size_t i = 0; i < Width; ++i) {
		result[i] = std::max(a[i], b[i]);
	}
	return result;
}

template <typename T, size_t Width>
[[nodiscard]] constexpr SimdVec<T, Width> clamp(
	const SimdVec<T, Width>& val,
	const SimdVec<T, Width>& min_val,
	const SimdVec<T, Width>& max_val
) noexcept {
	return min(max(val, min_val), max_val);
}

template <typename T, size_t Width>
[[nodiscard]] constexpr SimdVec<T, Width> floor(const SimdVec<T, Width>& vec) noexcept {
	SimdVec<T, Width> result;
	for (size_t i = 0; i < Width; ++i) {
		result[i] = std::floor(vec[i]);
	}
	return result;
}

template <typename T, size_t Width>
[[nodiscard]] constexpr SimdVec<T, Width> ceil(const SimdVec<T, Width>& vec) noexcept {
	SimdVec<T, Width> result;
	for (size_t i = 0; i < Width; ++i) {
		result[i] = std::ceil(vec[i]);
	}
	return result;
}

template <size_t Width> using FloatVec = SimdVec<float, Width>;
template <size_t Width> using DoubleVec = SimdVec<double, Width>;
template <size_t Width> using Int32Vec = SimdVec<int32_t, Width>;
template <size_t Width> using Int64Vec = SimdVec<int64_t, Width>;

using Float4 = SimdVec<float, 4>;
using Float8 = SimdVec<float, 8>;
using Float16 = SimdVec<float, 16>;

using Double2 = SimdVec<double, 2>;
using Double4 = SimdVec<double, 4>;
using Double8 = SimdVec<double, 8>;

using Mask4f = SimdMask<float, 4>;
using Mask8f = SimdMask<float, 8>;
using Mask16f = SimdMask<float, 16>;

using Mask2d = SimdMask<double, 2>;
using Mask4d = SimdMask<double, 4>;
using Mask8d = SimdMask<double, 8>;

}

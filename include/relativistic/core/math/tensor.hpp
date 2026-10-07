#pragma once

#include <cstddef>
#include <array>
#include <concepts>
#include <type_traits>
#include <algorithm>
#include <cmath>

namespace Relativistic::Core {

template <size_t Base, size_t Exp>
struct StaticPower {
	static constexpr size_t value = Base * StaticPower<Base, Exp - 1>::value;
};

template <size_t Base>
struct StaticPower<Base, 0> {
	static constexpr size_t value = 1;
};

template <typename T, size_t Rank, size_t Dim>
class alignas(64) Tensor {
	static_assert(std::is_floating_point_v<T> || std::is_integral_v<T>, "Tensor scalar type must be arithmetic");
	static_assert(Dim > 0, "Tensor dimension must be strictly positive");

public:
	static constexpr size_t RANK = Rank;
	static constexpr size_t DIMENSION = Dim;
	static constexpr size_t TOTAL_ELEMENTS = StaticPower<Dim, Rank>::value;

	using value_type = T;
	using size_type = size_t;
	using reference = T&;
	using const_reference = const T&;
	using pointer = T*;
	using const_pointer = const T*;
	using iterator = typename std::array<T, TOTAL_ELEMENTS>::iterator;
	using const_iterator = typename std::array<T, TOTAL_ELEMENTS>::const_iterator;

private:
	std::array<T, TOTAL_ELEMENTS> data_;

	template <typename... Indices>
	static constexpr size_t compute_flat_index(Indices... indices) noexcept {
		const std::array<size_t, sizeof...(Indices)> idx_array{static_cast<size_t>(indices)...};
		size_t flat = 0;
		for (size_t i = 0; i < sizeof...(Indices); ++i) {
			flat = flat * Dim + idx_array[i];
		}
		return flat;
	}

public:
	constexpr Tensor() noexcept : data_{} {}

	explicit constexpr Tensor(T initial_value) noexcept {
		data_.fill(initial_value);
	}

	constexpr Tensor(const std::array<T, TOTAL_ELEMENTS>& raw_data) noexcept : data_(raw_data) {}

	template <typename... Elements>
		requires (sizeof...(Elements) == TOTAL_ELEMENTS && (std::convertible_to<Elements, T> && ...))
	explicit constexpr Tensor(Elements... elements) noexcept : data_{static_cast<T>(elements)...} {}

	[[nodiscard]] static constexpr size_t rank() noexcept { return Rank; }
	[[nodiscard]] static constexpr size_t dimension() noexcept { return Dim; }
	[[nodiscard]] static constexpr size_t size() noexcept { return TOTAL_ELEMENTS; }

	[[nodiscard]] constexpr pointer data() noexcept { return data_.data(); }
	[[nodiscard]] constexpr const_pointer data() const noexcept { return data_.data(); }

	[[nodiscard]] constexpr iterator begin() noexcept { return data_.begin(); }
	[[nodiscard]] constexpr const_iterator begin() const noexcept { return data_.begin(); }
	[[nodiscard]] constexpr iterator end() noexcept { return data_.end(); }
	[[nodiscard]] constexpr const_iterator end() const noexcept { return data_.end(); }

	template <typename... Indices>
		requires (sizeof...(Indices) == Rank && (std::convertible_to<Indices, size_t> && ...))
	[[nodiscard]] constexpr reference operator()(Indices... indices) noexcept {
		return data_[compute_flat_index(indices...)];
	}

	template <typename... Indices>
		requires (sizeof...(Indices) == Rank && (std::convertible_to<Indices, size_t> && ...))
	[[nodiscard]] constexpr const_reference operator()(Indices... indices) const noexcept {
		return data_[compute_flat_index(indices...)];
	}

	[[nodiscard]] constexpr reference operator[](size_t flat_index) noexcept {
		return data_[flat_index];
	}

	[[nodiscard]] constexpr const_reference operator[](size_t flat_index) const noexcept {
		return data_[flat_index];
	}

	[[nodiscard]] constexpr reference at_coordinates(const std::array<size_t, Rank>& coords) noexcept {
		size_t flat = 0;
		for (size_t i = 0; i < Rank; ++i) {
			flat = flat * Dim + coords[i];
		}
		return data_[flat];
	}

	[[nodiscard]] constexpr const_reference at_coordinates(const std::array<size_t, Rank>& coords) const noexcept {
		size_t flat = 0;
		for (size_t i = 0; i < Rank; ++i) {
			flat = flat * Dim + coords[i];
		}
		return data_[flat];
	}

	[[nodiscard]] static constexpr std::array<size_t, Rank> flat_to_coordinates(size_t flat_index) noexcept {
		std::array<size_t, Rank> coords{};
		size_t rem = flat_index;
		for (size_t i = Rank; i > 0; --i) {
			coords[i - 1] = rem % Dim;
			rem /= Dim;
		}
		return coords;
	}

	constexpr void fill(T value) noexcept {
		data_.fill(value);
	}

	constexpr void zero() noexcept {
		data_.fill(static_cast<T>(0));
	}

	[[nodiscard]] constexpr Tensor operator+() const noexcept {
		return *this;
	}

	[[nodiscard]] constexpr Tensor operator-() const noexcept {
		Tensor result;
		for (size_t i = 0; i < TOTAL_ELEMENTS; ++i) {
			result.data_[i] = -data_[i];
		}
		return result;
	}

	[[nodiscard]] constexpr Tensor operator+(const Tensor& other) const noexcept {
		Tensor result;
		for (size_t i = 0; i < TOTAL_ELEMENTS; ++i) {
			result.data_[i] = data_[i] + other.data_[i];
		}
		return result;
	}

	[[nodiscard]] constexpr Tensor operator-(const Tensor& other) const noexcept {
		Tensor result;
		for (size_t i = 0; i < TOTAL_ELEMENTS; ++i) {
			result.data_[i] = data_[i] - other.data_[i];
		}
		return result;
	}

	[[nodiscard]] constexpr Tensor operator*(T scalar) const noexcept {
		Tensor result;
		for (size_t i = 0; i < TOTAL_ELEMENTS; ++i) {
			result.data_[i] = data_[i] * scalar;
		}
		return result;
	}

	[[nodiscard]] constexpr Tensor operator/(T scalar) const noexcept {
		Tensor result;
		const T inv_scalar = static_cast<T>(1) / scalar;
		for (size_t i = 0; i < TOTAL_ELEMENTS; ++i) {
			result.data_[i] = data_[i] * inv_scalar;
		}
		return result;
	}

	constexpr Tensor& operator+=(const Tensor& other) noexcept {
		for (size_t i = 0; i < TOTAL_ELEMENTS; ++i) {
			data_[i] += other.data_[i];
		}
		return *this;
	}

	constexpr Tensor& operator-=(const Tensor& other) noexcept {
		for (size_t i = 0; i < TOTAL_ELEMENTS; ++i) {
			data_[i] -= other.data_[i];
		}
		return *this;
	}

	constexpr Tensor& operator*=(T scalar) noexcept {
		for (size_t i = 0; i < TOTAL_ELEMENTS; ++i) {
			data_[i] *= scalar;
		}
		return *this;
	}

	constexpr Tensor& operator/=(T scalar) noexcept {
		const T inv_scalar = static_cast<T>(1) / scalar;
		for (size_t i = 0; i < TOTAL_ELEMENTS; ++i) {
			data_[i] *= inv_scalar;
		}
		return *this;
	}

	[[nodiscard]] constexpr bool operator==(const Tensor& other) const noexcept {
		for (size_t i = 0; i < TOTAL_ELEMENTS; ++i) {
			if (data_[i] != other.data_[i]) {
				return false;
			}
		}
		return true;
	}

	[[nodiscard]] constexpr bool operator!=(const Tensor& other) const noexcept {
		return !(*this == other);
	}
};

template <typename T, size_t Rank, size_t Dim>
[[nodiscard]] constexpr Tensor<T, Rank, Dim> operator*(T scalar, const Tensor<T, Rank, Dim>& tensor) noexcept {
	return tensor * scalar;
}

template <typename T, size_t Dim>
using Vector = Tensor<T, 1, Dim>;

template <typename T, size_t Dim>
using Matrix = Tensor<T, 2, Dim>;

template <typename T>
using FourVector = Tensor<T, 1, 4>;

template <typename T>
using MetricTensor = Tensor<T, 2, 4>;

template <typename T>
using ChristoffelSymbols = Tensor<T, 3, 4>;

template <typename T>
using RiemannTensor = Tensor<T, 4, 4>;

static_assert(sizeof(MetricTensor<double>) == 16 * sizeof(double));
static_assert(std::is_trivially_copyable_v<MetricTensor<double>>);

}

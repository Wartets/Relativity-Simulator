#pragma once

#include <cstddef>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <type_traits>
#include <concepts>

namespace Relativistic::Render {

struct alignas(8) DoubleSingle {
	float hi{0.0f};
	float lo{0.0f};

	constexpr DoubleSingle() noexcept = default;

	constexpr DoubleSingle(float high, float low) noexcept : hi(high), lo(low) {}

	explicit constexpr DoubleSingle(float val) noexcept : hi(val), lo(0.0f) {}

	explicit constexpr DoubleSingle(double val) noexcept {
		hi = static_cast<float>(val);
		lo = static_cast<float>(val - static_cast<double>(hi));
	}

	[[nodiscard]] constexpr explicit operator double() const noexcept {
		return static_cast<double>(hi) + static_cast<double>(lo);
	}

	[[nodiscard]] constexpr explicit operator float() const noexcept {
		return hi;
	}

	[[nodiscard]] static constexpr DoubleSingle quick_two_sum(float a, float b) noexcept {
		const float s = a + b;
		const float e = b - (s - a);
		return DoubleSingle(s, e);
	}

	[[nodiscard]] static constexpr DoubleSingle two_sum(float a, float b) noexcept {
		const float s = a + b;
		const float v = s - a;
		const float e = (a - (s - v)) + (b - v);
		return DoubleSingle(s, e);
	}

	[[nodiscard]] static constexpr DoubleSingle two_diff(float a, float b) noexcept {
		const float s = a - b;
		const float v = s - a;
		const float e = (a - (s - v)) - (b + v);
		return DoubleSingle(s, e);
	}

	[[nodiscard]] static constexpr DoubleSingle two_prod(float a, float b) noexcept {
		const float p = a * b;
		const float e = std::fma(a, b, -p);
		return DoubleSingle(p, e);
	}

	[[nodiscard]] constexpr DoubleSingle operator+() const noexcept {
		return *this;
	}

	[[nodiscard]] constexpr DoubleSingle operator-() const noexcept {
		return DoubleSingle(-hi, -lo);
	}

	[[nodiscard]] constexpr DoubleSingle operator+(const DoubleSingle& rhs) const noexcept {
		const DoubleSingle st = two_sum(hi, rhs.hi);
		const DoubleSingle th = two_sum(lo, rhs.lo);
		const float c = st.lo + th.hi;
		const DoubleSingle v = two_sum(st.hi, c);
		const float w = v.lo + th.lo;
		return two_sum(v.hi, w);
	}

	[[nodiscard]] constexpr DoubleSingle operator-(const DoubleSingle& rhs) const noexcept {
		return *this + (-rhs);
	}

	[[nodiscard]] constexpr DoubleSingle operator*(const DoubleSingle& rhs) const noexcept {
		const DoubleSingle p = two_prod(hi, rhs.hi);
		const float c = std::fma(hi, rhs.lo, std::fma(lo, rhs.hi, p.lo));
		return two_sum(p.hi, c);
	}

	[[nodiscard]] constexpr DoubleSingle operator/(const DoubleSingle& rhs) const noexcept {
		const float q1 = hi / rhs.hi;
		const DoubleSingle r = *this - rhs * DoubleSingle(q1);
		const float q2 = r.hi / rhs.hi;
		const DoubleSingle r2 = r - rhs * DoubleSingle(q2);
		const float q3 = r2.hi / rhs.hi;
		const DoubleSingle q_sum = quick_two_sum(q1, q2);
		return q_sum + DoubleSingle(q3);
	}

	[[nodiscard]] constexpr DoubleSingle operator+(float scalar) const noexcept {
		return *this + DoubleSingle(scalar);
	}

	[[nodiscard]] constexpr DoubleSingle operator-(float scalar) const noexcept {
		return *this - DoubleSingle(scalar);
	}

	[[nodiscard]] constexpr DoubleSingle operator*(float scalar) const noexcept {
		return *this * DoubleSingle(scalar);
	}

	[[nodiscard]] constexpr DoubleSingle operator/(float scalar) const noexcept {
		return *this / DoubleSingle(scalar);
	}

	constexpr DoubleSingle& operator+=(const DoubleSingle& rhs) noexcept {
		*this = *this + rhs;
		return *this;
	}

	constexpr DoubleSingle& operator-=(const DoubleSingle& rhs) noexcept {
		*this = *this - rhs;
		return *this;
	}

	constexpr DoubleSingle& operator*=(const DoubleSingle& rhs) noexcept {
		*this = *this * rhs;
		return *this;
	}

	constexpr DoubleSingle& operator/=(const DoubleSingle& rhs) noexcept {
		*this = *this / rhs;
		return *this;
	}

	[[nodiscard]] constexpr bool operator==(const DoubleSingle& rhs) const noexcept {
		return (hi == rhs.hi) && (lo == rhs.lo);
	}

	[[nodiscard]] constexpr bool operator!=(const DoubleSingle& rhs) const noexcept {
		return !(*this == rhs);
	}

	[[nodiscard]] constexpr bool operator<(const DoubleSingle& rhs) const noexcept {
		if (hi < rhs.hi) return true;
		if (hi > rhs.hi) return false;
		return lo < rhs.lo;
	}

	[[nodiscard]] constexpr bool operator<=(const DoubleSingle& rhs) const noexcept {
		return (*this < rhs) || (*this == rhs);
	}

	[[nodiscard]] constexpr bool operator>(const DoubleSingle& rhs) const noexcept {
		return !(*this <= rhs);
	}

	[[nodiscard]] constexpr bool operator>=(const DoubleSingle& rhs) const noexcept {
		return !(*this < rhs);
	}
};

[[nodiscard]] inline constexpr DoubleSingle operator+(float lhs, const DoubleSingle& rhs) noexcept {
	return DoubleSingle(lhs) + rhs;
}

[[nodiscard]] inline constexpr DoubleSingle operator-(float lhs, const DoubleSingle& rhs) noexcept {
	return DoubleSingle(lhs) - rhs;
}

[[nodiscard]] inline constexpr DoubleSingle operator*(float lhs, const DoubleSingle& rhs) noexcept {
	return DoubleSingle(lhs) * rhs;
}

[[nodiscard]] inline constexpr DoubleSingle operator/(float lhs, const DoubleSingle& rhs) noexcept {
	return DoubleSingle(lhs) / rhs;
}

[[nodiscard]] inline constexpr DoubleSingle ds_abs(const DoubleSingle& val) noexcept {
	if (val.hi < 0.0f) {
		return -val;
	}
	if (val.hi == 0.0f && val.lo < 0.0f) {
		return -val;
	}
	return val;
}

[[nodiscard]] inline constexpr DoubleSingle ds_max(const DoubleSingle& a, const DoubleSingle& b) noexcept {
	return (a > b) ? a : b;
}

[[nodiscard]] inline constexpr DoubleSingle ds_min(const DoubleSingle& a, const DoubleSingle& b) noexcept {
	return (a < b) ? a : b;
}

[[nodiscard]] inline constexpr DoubleSingle ds_clamp(const DoubleSingle& val, const DoubleSingle& min_val, const DoubleSingle& max_val) noexcept {
	return ds_min(ds_max(val, min_val), max_val);
}

[[nodiscard]] inline DoubleSingle ds_sqrt(const DoubleSingle& val) noexcept {
	if (val.hi <= 0.0f) {
		return DoubleSingle(0.0f, 0.0f);
	}
	const float s = std::sqrt(val.hi);
	const DoubleSingle s_ds(s);
	const DoubleSingle diff = val - s_ds * s_ds;
	const float diff_f = diff.hi / (2.0f * s);
	return DoubleSingle::quick_two_sum(s, diff_f);
}

[[nodiscard]] inline DoubleSingle ds_sin(const DoubleSingle& x) noexcept {
	const double x_d = static_cast<double>(x);
	const double sin_d = std::sin(x_d);
	return DoubleSingle(sin_d);
}

[[nodiscard]] inline DoubleSingle ds_cos(const DoubleSingle& x) noexcept {
	const double x_d = static_cast<double>(x);
	const double cos_d = std::cos(x_d);
	return DoubleSingle(cos_d);
}

[[nodiscard]] inline DoubleSingle ds_atan2(const DoubleSingle& y, const DoubleSingle& x) noexcept {
	const double y_d = static_cast<double>(y);
	const double x_d = static_cast<double>(x);
	const double angle = std::atan2(y_d, x_d);
	return DoubleSingle(angle);
}

}

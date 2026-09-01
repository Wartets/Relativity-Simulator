#pragma once

#include <cmath>
#include <algorithm>
#include <concepts>
#include <numbers>
#include <limits>
#include <type_traits>
#include <cstdint>

namespace Relativistic::Uncertainty {

template <typename Scalar = double>
class alignas(sizeof(Scalar) * 2) Interval {
	static_assert(std::is_floating_point_v<Scalar>, "Interval requires floating point type");

private:
	Scalar lower_{static_cast<Scalar>(0.0)};
	Scalar upper_{static_cast<Scalar>(0.0)};

public:
	constexpr Interval() noexcept = default;

	constexpr Interval(Scalar inf, Scalar sup) noexcept
		: lower_(std::min(inf, sup)), upper_(std::max(inf, sup)) {}

	explicit constexpr Interval(Scalar value) noexcept
		: lower_(value), upper_(value) {}

	[[nodiscard]] static constexpr Interval empty() noexcept {
		Interval intv;
		intv.lower_ = std::numeric_limits<Scalar>::quiet_NaN();
		intv.upper_ = std::numeric_limits<Scalar>::quiet_NaN();
		return intv;
	}

	[[nodiscard]] static constexpr Interval entire() noexcept {
		return Interval(-std::numeric_limits<Scalar>::infinity(), std::numeric_limits<Scalar>::infinity());
	}

	[[nodiscard]] static constexpr Interval symmetric(Scalar radius) noexcept {
		const Scalar r = std::abs(radius);
		return Interval(-r, r);
	}

	[[nodiscard]] static constexpr Interval with_uncertainty(Scalar center, Scalar absolute_uncertainty) noexcept {
		const Scalar u = std::abs(absolute_uncertainty);
		return Interval(center - u, center + u);
	}

	[[nodiscard]] static constexpr Interval with_relative_uncertainty(Scalar center, Scalar relative_fraction) noexcept {
		const Scalar u = std::abs(center * relative_fraction);
		return Interval(center - u, center + u);
	}

	[[nodiscard]] constexpr Scalar lower() const noexcept { return lower_; }
	[[nodiscard]] constexpr Scalar upper() const noexcept { return upper_; }
	[[nodiscard]] constexpr Scalar inf() const noexcept { return lower_; }
	[[nodiscard]] constexpr Scalar sup() const noexcept { return upper_; }

	[[nodiscard]] constexpr Scalar midpoint() const noexcept {
		return static_cast<Scalar>(0.5) * (lower_ + upper_);
	}

	[[nodiscard]] constexpr Scalar width() const noexcept {
		return upper_ - lower_;
	}

	[[nodiscard]] constexpr Scalar radius() const noexcept {
		return static_cast<Scalar>(0.5) * (upper_ - lower_);
	}

	[[nodiscard]] constexpr Scalar absolute_value() const noexcept {
		return std::max(std::abs(lower_), std::abs(upper_));
	}

	[[nodiscard]] constexpr bool is_empty() const noexcept {
		return std::isnan(lower_) || std::isnan(upper_) || (lower_ > upper_);
	}

	[[nodiscard]] constexpr bool is_singleton() const noexcept {
		return lower_ == upper_;
	}

	[[nodiscard]] constexpr bool is_bounded() const noexcept {
		return std::isfinite(lower_) && std::isfinite(upper_);
	}

	[[nodiscard]] constexpr bool contains(Scalar point) const noexcept {
		return point >= lower_ && point <= upper_;
	}

	[[nodiscard]] constexpr bool contains(const Interval& other) const noexcept {
		return other.lower_ >= lower_ && other.upper_ <= upper_;
	}

	[[nodiscard]] constexpr bool strictly_contains(const Interval& other) const noexcept {
		return other.lower_ > lower_ && other.upper_ < upper_;
	}

	[[nodiscard]] constexpr bool overlaps(const Interval& other) const noexcept {
		return std::max(lower_, other.lower_) <= std::min(upper_, other.upper_);
	}

	[[nodiscard]] constexpr Interval hull(const Interval& other) const noexcept {
		if (is_empty()) return other;
		if (other.is_empty()) return *this;
		return Interval(std::min(lower_, other.lower_), std::max(upper_, other.upper_));
	}

	[[nodiscard]] constexpr Interval intersect(const Interval& other) const noexcept {
		const Scalar new_low = std::max(lower_, other.lower_);
		const Scalar new_high = std::min(upper_, other.upper_);
		if (new_low <= new_high) {
			return Interval(new_low, new_high);
		}
		return empty();
	}

	[[nodiscard]] constexpr Interval operator+() const noexcept {
		return *this;
	}

	[[nodiscard]] constexpr Interval operator-() const noexcept {
		return Interval(-upper_, -lower_);
	}

	[[nodiscard]] constexpr Interval operator+(const Interval& rhs) const noexcept {
		return Interval(lower_ + rhs.lower_, upper_ + rhs.upper_);
	}

	[[nodiscard]] constexpr Interval operator-(const Interval& rhs) const noexcept {
		return Interval(lower_ - rhs.upper_, upper_ - rhs.lower_);
	}

	[[nodiscard]] constexpr Interval operator*(const Interval& rhs) const noexcept {
		const Scalar p1 = lower_ * rhs.lower_;
		const Scalar p2 = lower_ * rhs.upper_;
		const Scalar p3 = upper_ * rhs.lower_;
		const Scalar p4 = upper_ * rhs.upper_;
		return Interval(std::min({p1, p2, p3, p4}), std::max({p1, p2, p3, p4}));
	}

	[[nodiscard]] constexpr Interval operator/(const Interval& rhs) const noexcept {
		if (rhs.contains(static_cast<Scalar>(0.0))) {
			if (rhs.lower_ == static_cast<Scalar>(0.0) && rhs.upper_ == static_cast<Scalar>(0.0)) {
				return empty();
			}
			if (rhs.lower_ == static_cast<Scalar>(0.0)) {
				return *this * Interval(static_cast<Scalar>(1.0) / rhs.upper_, std::numeric_limits<Scalar>::infinity());
			}
			if (rhs.upper_ == static_cast<Scalar>(0.0)) {
				return *this * Interval(-std::numeric_limits<Scalar>::infinity(), static_cast<Scalar>(1.0) / rhs.lower_);
			}
			return entire();
		}
		const Scalar inv_low = static_cast<Scalar>(1.0) / rhs.upper_;
		const Scalar inv_high = static_cast<Scalar>(1.0) / rhs.lower_;
		return *this * Interval(inv_low, inv_high);
	}

	[[nodiscard]] constexpr Interval operator+(Scalar rhs) const noexcept {
		return Interval(lower_ + rhs, upper_ + rhs);
	}

	[[nodiscard]] constexpr Interval operator-(Scalar rhs) const noexcept {
		return Interval(lower_ - rhs, upper_ - rhs);
	}

	[[nodiscard]] constexpr Interval operator*(Scalar rhs) const noexcept {
		const Scalar p1 = lower_ * rhs;
		const Scalar p2 = upper_ * rhs;
		return Interval(std::min(p1, p2), std::max(p1, p2));
	}

	[[nodiscard]] constexpr Interval operator/(Scalar rhs) const noexcept {
		const Scalar inv = static_cast<Scalar>(1.0) / rhs;
		const Scalar p1 = lower_ * inv;
		const Scalar p2 = upper_ * inv;
		return Interval(std::min(p1, p2), std::max(p1, p2));
	}

	constexpr Interval& operator+=(const Interval& rhs) noexcept {
		*this = *this + rhs;
		return *this;
	}

	constexpr Interval& operator-=(const Interval& rhs) noexcept {
		*this = *this - rhs;
		return *this;
	}

	constexpr Interval& operator*=(const Interval& rhs) noexcept {
		*this = *this * rhs;
		return *this;
	}

	constexpr Interval& operator/=(const Interval& rhs) noexcept {
		*this = *this / rhs;
		return *this;
	}

	constexpr Interval& operator+=(Scalar rhs) noexcept {
		lower_ += rhs;
		upper_ += rhs;
		return *this;
	}

	constexpr Interval& operator-=(Scalar rhs) noexcept {
		lower_ -= rhs;
		upper_ -= rhs;
		return *this;
	}

	constexpr Interval& operator*=(Scalar rhs) noexcept {
		*this = *this * rhs;
		return *this;
	}

	constexpr Interval& operator/=(Scalar rhs) noexcept {
		*this = *this / rhs;
		return *this;
	}

	[[nodiscard]] constexpr bool operator==(const Interval& rhs) const noexcept {
		return lower_ == rhs.lower_ && upper_ == rhs.upper_;
	}

	[[nodiscard]] constexpr bool operator!=(const Interval& rhs) const noexcept {
		return !(*this == rhs);
	}

	[[nodiscard]] constexpr bool operator<(const Interval& rhs) const noexcept {
		return upper_ < rhs.lower_;
	}

	[[nodiscard]] constexpr bool operator<=(const Interval& rhs) const noexcept {
		return upper_ <= rhs.upper_;
	}

	[[nodiscard]] constexpr bool operator>(const Interval& rhs) const noexcept {
		return lower_ > rhs.upper_;
	}

	[[nodiscard]] constexpr bool operator>=(const Interval& rhs) const noexcept {
		return lower_ >= rhs.lower_;
	}

	[[nodiscard]] Interval sqr() const noexcept {
		if (contains(static_cast<Scalar>(0.0))) {
			const Scalar max_val = std::max(lower_ * lower_, upper_ * upper_);
			return Interval(static_cast<Scalar>(0.0), max_val);
		}
		const Scalar p1 = lower_ * lower_;
		const Scalar p2 = upper_ * upper_;
		return Interval(std::min(p1, p2), std::max(p1, p2));
	}

	[[nodiscard]] Interval sqrt() const noexcept {
		const Scalar low = std::max(lower_, static_cast<Scalar>(0.0));
		const Scalar high = std::max(upper_, static_cast<Scalar>(0.0));
		return Interval(std::sqrt(low), std::sqrt(high));
	}

	[[nodiscard]] Interval exp() const noexcept {
		return Interval(std::exp(lower_), std::exp(upper_));
	}

	[[nodiscard]] Interval log() const noexcept {
		if (upper_ <= static_cast<Scalar>(0.0)) {
			return empty();
		}
		const Scalar low = (lower_ > static_cast<Scalar>(0.0)) ? std::log(lower_) : -std::numeric_limits<Scalar>::infinity();
		return Interval(low, std::log(upper_));
	}

	[[nodiscard]] Interval pow(int exponent) const noexcept {
		if (exponent == 0) return Interval(static_cast<Scalar>(1.0));
		if (exponent == 1) return *this;
		if (exponent == 2) return sqr();
		if (exponent < 0) return Interval(static_cast<Scalar>(1.0)) / pow(-exponent);

		if (exponent % 2 == 1) {
			return Interval(std::pow(lower_, static_cast<Scalar>(exponent)), std::pow(upper_, static_cast<Scalar>(exponent)));
		}
		if (contains(static_cast<Scalar>(0.0))) {
			const Scalar max_val = std::max(std::pow(lower_, static_cast<Scalar>(exponent)), std::pow(upper_, static_cast<Scalar>(exponent)));
			return Interval(static_cast<Scalar>(0.0), max_val);
		}
		const Scalar p1 = std::pow(lower_, static_cast<Scalar>(exponent));
		const Scalar p2 = std::pow(upper_, static_cast<Scalar>(exponent));
		return Interval(std::min(p1, p2), std::max(p1, p2));
	}

	[[nodiscard]] Interval sin() const noexcept {
		if (width() >= static_cast<Scalar>(2.0) * std::numbers::pi_v<Scalar>) {
			return Interval(static_cast<Scalar>(-1.0), static_cast<Scalar>(1.0));
		}
		const Scalar pi2 = static_cast<Scalar>(2.0) * std::numbers::pi_v<Scalar>;
		const Scalar half_pi = static_cast<Scalar>(0.5) * std::numbers::pi_v<Scalar>;

		Scalar min_val = std::min(std::sin(lower_), std::sin(upper_));
		Scalar max_val = std::max(std::sin(lower_), std::sin(upper_));

		const auto k_min = static_cast<int64_t>(std::floor((lower_ - half_pi) / pi2));
		const auto k_max = static_cast<int64_t>(std::ceil((upper_ - half_pi) / pi2));
		for (int64_t k = k_min; k <= k_max; ++k) {
			const Scalar crit_max = half_pi + static_cast<Scalar>(k) * pi2;
			if (crit_max >= lower_ && crit_max <= upper_) {
				max_val = static_cast<Scalar>(1.0);
			}
			const Scalar crit_min = -half_pi + static_cast<Scalar>(k) * pi2;
			if (crit_min >= lower_ && crit_min <= upper_) {
				min_val = static_cast<Scalar>(-1.0);
			}
		}
		return Interval(min_val, max_val);
	}

	[[nodiscard]] Interval cos() const noexcept {
		const Scalar half_pi = static_cast<Scalar>(0.5) * std::numbers::pi_v<Scalar>;
		return (*this + half_pi).sin();
	}
};

template <typename Scalar>
[[nodiscard]] inline constexpr Interval<Scalar> operator+(Scalar lhs, const Interval<Scalar>& rhs) noexcept {
	return rhs + lhs;
}

template <typename Scalar>
[[nodiscard]] inline constexpr Interval<Scalar> operator-(Scalar lhs, const Interval<Scalar>& rhs) noexcept {
	return Interval<Scalar>(lhs - rhs.upper(), lhs - rhs.lower());
}

template <typename Scalar>
[[nodiscard]] inline constexpr Interval<Scalar> operator*(Scalar lhs, const Interval<Scalar>& rhs) noexcept {
	return rhs * lhs;
}

template <typename Scalar>
[[nodiscard]] inline constexpr Interval<Scalar> operator/(Scalar lhs, const Interval<Scalar>& rhs) noexcept {
	return Interval<Scalar>(lhs) / rhs;
}

template <typename Scalar>
[[nodiscard]] inline Interval<Scalar> abs(const Interval<Scalar>& intv) noexcept {
	if (intv.contains(static_cast<Scalar>(0.0))) {
		return Interval<Scalar>(static_cast<Scalar>(0.0), std::max(std::abs(intv.lower()), std::abs(intv.upper())));
	}
	const Scalar a = std::abs(intv.lower());
	const Scalar b = std::abs(intv.upper());
	return Interval<Scalar>(std::min(a, b), std::max(a, b));
}

template <typename Scalar>
[[nodiscard]] inline Interval<Scalar> sqrt(const Interval<Scalar>& intv) noexcept {
	return intv.sqrt();
}

template <typename Scalar>
[[nodiscard]] inline Interval<Scalar> exp(const Interval<Scalar>& intv) noexcept {
	return intv.exp();
}

template <typename Scalar>
[[nodiscard]] inline Interval<Scalar> log(const Interval<Scalar>& intv) noexcept {
	return intv.log();
}

template <typename Scalar>
[[nodiscard]] inline Interval<Scalar> sin(const Interval<Scalar>& intv) noexcept {
	return intv.sin();
}

template <typename Scalar>
[[nodiscard]] inline Interval<Scalar> cos(const Interval<Scalar>& intv) noexcept {
	return intv.cos();
}

template <typename Scalar>
[[nodiscard]] inline Interval<Scalar> pow(const Interval<Scalar>& intv, int exp_val) noexcept {
	return intv.pow(exp_val);
}

}

#pragma once

#include "relativistic/uncertainty/uncertainty_types.hpp"
#include "relativistic/uncertainty/interval.hpp"
#include "relativistic/uncertainty/zonotope.hpp"
#include "relativistic/uncertainty/covariance.hpp"
#include "relativistic/uncertainty/polynomial_chaos.hpp"
#include "relativistic/core/tensor.hpp"
#include <concepts>
#include <utility>

namespace Relativistic::Uncertainty {

template <typename T, UncertaintyMethod Method = UncertaintyMethod::Interval>
class UncertainQuantity;

template <typename Scalar>
class UncertainQuantity<Scalar, UncertaintyMethod::Interval> {
public:
	using ValueType = Scalar;
	static constexpr UncertaintyMethod METHOD = UncertaintyMethod::Interval;

private:
	Interval<Scalar> interval_{};

public:
	constexpr UncertainQuantity() noexcept = default;

	explicit constexpr UncertainQuantity(Scalar nominal) noexcept
		: interval_(nominal) {}

	constexpr UncertainQuantity(Scalar nominal, Scalar uncertainty) noexcept
		: interval_(nominal - std::abs(uncertainty), nominal + std::abs(uncertainty)) {}

	constexpr UncertainQuantity(const Interval<Scalar>& intv) noexcept
		: interval_(intv) {}

	[[nodiscard]] constexpr Scalar nominal() const noexcept { return interval_.midpoint(); }
	[[nodiscard]] constexpr Scalar uncertainty() const noexcept { return interval_.radius(); }
	[[nodiscard]] constexpr const Interval<Scalar>& interval() const noexcept { return interval_; }
	[[nodiscard]] constexpr Interval<Scalar>& interval() noexcept { return interval_; }

	[[nodiscard]] constexpr UncertainQuantity operator+() const noexcept { return *this; }
	[[nodiscard]] constexpr UncertainQuantity operator-() const noexcept { return UncertainQuantity(-interval_); }

	[[nodiscard]] constexpr UncertainQuantity operator+(const UncertainQuantity& rhs) const noexcept {
		return UncertainQuantity(interval_ + rhs.interval_);
	}

	[[nodiscard]] constexpr UncertainQuantity operator-(const UncertainQuantity& rhs) const noexcept {
		return UncertainQuantity(interval_ - rhs.interval_);
	}

	[[nodiscard]] constexpr UncertainQuantity operator*(const UncertainQuantity& rhs) const noexcept {
		return UncertainQuantity(interval_ * rhs.interval_);
	}

	[[nodiscard]] constexpr UncertainQuantity operator/(const UncertainQuantity& rhs) const noexcept {
		return UncertainQuantity(interval_ / rhs.interval_);
	}

	[[nodiscard]] constexpr UncertainQuantity operator+(Scalar rhs) const noexcept {
		return UncertainQuantity(interval_ + rhs);
	}

	[[nodiscard]] constexpr UncertainQuantity operator-(Scalar rhs) const noexcept {
		return UncertainQuantity(interval_ - rhs);
	}

	[[nodiscard]] constexpr UncertainQuantity operator*(Scalar rhs) const noexcept {
		return UncertainQuantity(interval_ * rhs);
	}

	[[nodiscard]] constexpr UncertainQuantity operator/(Scalar rhs) const noexcept {
		return UncertainQuantity(interval_ / rhs);
	}
};

template <typename Scalar, size_t Dim>
class UncertainQuantity<Zonotope<Scalar, Dim>, UncertaintyMethod::Zonotope> {
public:
	using ValueType = Zonotope<Scalar, Dim>;
	static constexpr UncertaintyMethod METHOD = UncertaintyMethod::Zonotope;

private:
	Zonotope<Scalar, Dim> zono_{};

public:
	constexpr UncertainQuantity() noexcept = default;

	explicit constexpr UncertainQuantity(const typename Zonotope<Scalar, Dim>::VectorType& nominal) noexcept
		: zono_(nominal) {}

	UncertainQuantity(const typename Zonotope<Scalar, Dim>::VectorType& nominal, std::span<const typename Zonotope<Scalar, Dim>::VectorType> generators)
		: zono_(nominal, generators) {}

	explicit UncertainQuantity(const Zonotope<Scalar, Dim>& z)
		: zono_(z) {}

	[[nodiscard]] const typename Zonotope<Scalar, Dim>::VectorType& nominal() const noexcept { return zono_.center(); }
	[[nodiscard]] const Zonotope<Scalar, Dim>& zonotope() const noexcept { return zono_; }
	[[nodiscard]] Zonotope<Scalar, Dim>& zonotope() noexcept { return zono_; }

	[[nodiscard]] UncertainQuantity operator+(const UncertainQuantity& rhs) const {
		return UncertainQuantity(zono_.minkowski_sum(rhs.zono_));
	}

	[[nodiscard]] UncertainQuantity operator*(Scalar factor) const {
		return UncertainQuantity(zono_.scale(factor));
	}
};

template <typename Scalar, size_t Dim>
class UncertainQuantity<CovarianceMatrix<Scalar, Dim>, UncertaintyMethod::Covariance> {
public:
	using ValueType = CovarianceMatrix<Scalar, Dim>;
	static constexpr UncertaintyMethod METHOD = UncertaintyMethod::Covariance;

private:
	typename CovarianceMatrix<Scalar, Dim>::VectorType state_{};
	CovarianceMatrix<Scalar, Dim> cov_{};

public:
	constexpr UncertainQuantity() noexcept = default;

	UncertainQuantity(const typename CovarianceMatrix<Scalar, Dim>::VectorType& mean, const CovarianceMatrix<Scalar, Dim>& covariance) noexcept
		: state_(mean), cov_(covariance) {}

	[[nodiscard]] const typename CovarianceMatrix<Scalar, Dim>::VectorType& nominal() const noexcept { return state_; }
	[[nodiscard]] typename CovarianceMatrix<Scalar, Dim>::VectorType& nominal() noexcept { return state_; }

	[[nodiscard]] const CovarianceMatrix<Scalar, Dim>& covariance() const noexcept { return cov_; }
	[[nodiscard]] CovarianceMatrix<Scalar, Dim>& covariance() noexcept { return cov_; }
};

template <size_t NumDims, size_t MaxDegree>
class UncertainQuantity<PolynomialChaosExpansion<NumDims, MaxDegree>, UncertaintyMethod::PolynomialChaos> {
public:
	using ValueType = PolynomialChaosExpansion<NumDims, MaxDegree>;
	static constexpr UncertaintyMethod METHOD = UncertaintyMethod::PolynomialChaos;

private:
	PolynomialChaosExpansion<NumDims, MaxDegree> pce_{};

public:
	constexpr UncertainQuantity() noexcept = default;

	explicit UncertainQuantity(const PolynomialChaosExpansion<NumDims, MaxDegree>& pce)
		: pce_(pce) {}

	[[nodiscard]] double nominal() const noexcept { return pce_.mean(); }
	[[nodiscard]] double uncertainty() const noexcept { return pce_.standard_deviation(); }
	[[nodiscard]] const PolynomialChaosExpansion<NumDims, MaxDegree>& pce() const noexcept { return pce_; }
	[[nodiscard]] PolynomialChaosExpansion<NumDims, MaxDegree>& pce() noexcept { return pce_; }
};

}

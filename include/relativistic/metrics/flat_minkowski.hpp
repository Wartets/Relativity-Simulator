#pragma once

#include "relativistic/core/tensor.hpp"
#include "spacetime_concept.hpp"

namespace Relativistic::Metrics {

template <typename Scalar = double>
class FlatMinkowskiMetric {
private:
	Scalar c_;

public:
	explicit constexpr FlatMinkowskiMetric(Scalar speed_of_light = static_cast<Scalar>(1)) noexcept
		: c_(speed_of_light) {}

	[[nodiscard]] static constexpr bool has_analytic_christoffel() noexcept {
		return true;
	}

	[[nodiscard]] constexpr Scalar speed_of_light() const noexcept {
		return c_;
	}

	[[nodiscard]] Core::MetricTensor<Scalar> metric_tensor(const Core::FourVector<Scalar>&) const noexcept {
		Core::MetricTensor<Scalar> g;
		g.zero();
		g(0, 0) = -(c_ * c_);
		g(1, 1) = static_cast<Scalar>(1);
		g(2, 2) = static_cast<Scalar>(1);
		g(3, 3) = static_cast<Scalar>(1);
		return g;
	}

	[[nodiscard]] Core::MetricTensor<Scalar> compute_metric(const Core::FourVector<Scalar>& x) const noexcept {
		return metric_tensor(x);
	}

	[[nodiscard]] Core::MetricTensor<Scalar> inverse_metric(const Core::FourVector<Scalar>&) const noexcept {
		Core::MetricTensor<Scalar> inv_g;
		inv_g.zero();
		inv_g(0, 0) = -static_cast<Scalar>(1) / (c_ * c_);
		inv_g(1, 1) = static_cast<Scalar>(1);
		inv_g(2, 2) = static_cast<Scalar>(1);
		inv_g(3, 3) = static_cast<Scalar>(1);
		return inv_g;
	}

	[[nodiscard]] Core::ChristoffelSymbols<Scalar> christoffel_symbols(const Core::FourVector<Scalar>&) const noexcept {
		Core::ChristoffelSymbols<Scalar> gamma;
		gamma.zero();
		return gamma;
	}

	[[nodiscard]] Core::ChristoffelSymbols<Scalar> compute_christoffel(const Core::FourVector<Scalar>& x) const noexcept {
		return christoffel_symbols(x);
	}
};

}

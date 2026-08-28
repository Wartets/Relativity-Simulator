#pragma once

#include "relativistic/core/tensor.hpp"
#include "spacetime_concept.hpp"
#include <cmath>
#include <numbers>

namespace Relativistic::Metrics {

template <typename Scalar = double>
class SchwarzschildMetric {
private:
	Scalar mass_;
	Scalar c_;
	Scalar g_const_;
	Scalar r_s_;

public:
	explicit constexpr SchwarzschildMetric(
		Scalar mass,
		Scalar speed_of_light = static_cast<Scalar>(1),
		Scalar gravitational_constant = static_cast<Scalar>(1)
	) noexcept
		: mass_(mass),
		  c_(speed_of_light),
		  g_const_(gravitational_constant),
		  r_s_(static_cast<Scalar>(2) * gravitational_constant * mass / (speed_of_light * speed_of_light)) {}

	[[nodiscard]] static constexpr bool has_analytic_christoffel() noexcept {
		return true;
	}

	[[nodiscard]] constexpr Scalar schwarzschild_radius() const noexcept {
		return r_s_;
	}

	[[nodiscard]] constexpr Scalar mass() const noexcept {
		return mass_;
	}

	[[nodiscard]] constexpr Scalar speed_of_light() const noexcept {
		return c_;
	}

	[[nodiscard]] constexpr Scalar gravitational_constant() const noexcept {
		return g_const_;
	}

	[[nodiscard]] Core::MetricTensor<Scalar> metric_tensor(const Core::FourVector<Scalar>& x) const noexcept {
		const Scalar r = x(1);
		const Scalar theta = x(2);
		const Scalar factor = static_cast<Scalar>(1) - r_s_ / r;
		const Scalar sin_t = std::sin(theta);

		Core::MetricTensor<Scalar> g;
		g.zero();
		g(0, 0) = -factor * (c_ * c_);
		g(1, 1) = static_cast<Scalar>(1) / factor;
		g(2, 2) = r * r;
		g(3, 3) = r * r * sin_t * sin_t;
		return g;
	}

	[[nodiscard]] Core::MetricTensor<Scalar> compute_metric(const Core::FourVector<Scalar>& x) const noexcept {
		return metric_tensor(x);
	}

	[[nodiscard]] Core::MetricTensor<Scalar> inverse_metric(const Core::FourVector<Scalar>& x) const noexcept {
		const Scalar r = x(1);
		const Scalar theta = x(2);
		const Scalar factor = static_cast<Scalar>(1) - r_s_ / r;
		const Scalar sin_t = std::sin(theta);

		Core::MetricTensor<Scalar> inv_g;
		inv_g.zero();
		inv_g(0, 0) = -static_cast<Scalar>(1) / (factor * c_ * c_);
		inv_g(1, 1) = factor;
		inv_g(2, 2) = static_cast<Scalar>(1) / (r * r);
		inv_g(3, 3) = static_cast<Scalar>(1) / (r * r * sin_t * sin_t);
		return inv_g;
	}

	[[nodiscard]] Core::ChristoffelSymbols<Scalar> christoffel_symbols(const Core::FourVector<Scalar>& x) const noexcept {
		const Scalar r = x(1);
		const Scalar theta = x(2);
		const Scalar sin_t = std::sin(theta);
		const Scalar cos_t = std::cos(theta);
		const Scalar r_diff = r - r_s_;
		const Scalar r2 = r * r;
		const Scalar r3 = r2 * r;

		Core::ChristoffelSymbols<Scalar> gamma;
		gamma.zero();

		const Scalar g001 = r_s_ / (static_cast<Scalar>(2) * r * r_diff);
		gamma(0, 0, 1) = g001;
		gamma(0, 1, 0) = g001;

		gamma(1, 0, 0) = (c_ * c_ * r_s_ * r_diff) / (static_cast<Scalar>(2) * r3);
		gamma(1, 1, 1) = -r_s_ / (static_cast<Scalar>(2) * r * r_diff);
		gamma(1, 2, 2) = -r_diff;
		gamma(1, 3, 3) = -r_diff * sin_t * sin_t;

		const Scalar g212 = static_cast<Scalar>(1) / r;
		gamma(2, 1, 2) = g212;
		gamma(2, 2, 1) = g212;
		gamma(2, 3, 3) = -sin_t * cos_t;

		const Scalar g313 = static_cast<Scalar>(1) / r;
		gamma(3, 1, 3) = g313;
		gamma(3, 3, 1) = g313;

		const Scalar g323 = cos_t / sin_t;
		gamma(3, 2, 3) = g323;
		gamma(3, 3, 2) = g323;

		return gamma;
	}

	[[nodiscard]] Core::ChristoffelSymbols<Scalar> compute_christoffel(const Core::FourVector<Scalar>& x) const noexcept {
		return christoffel_symbols(x);
	}
};

}

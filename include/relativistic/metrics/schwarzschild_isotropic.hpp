#pragma once

#include "relativistic/core/tensor.hpp"
#include "relativistic/metrics/spacetime_concept.hpp"
#include <cmath>
#include <algorithm>

namespace Relativistic::Metrics {

template <typename Scalar = double>
class SchwarzschildIsotropicMetric {
private:
	Scalar mass_;
	Scalar c_;
	Scalar g_const_;
	Scalar r_s_;

public:
	explicit constexpr SchwarzschildIsotropicMetric(
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
		const Scalar r = std::max(x(1), static_cast<Scalar>(r_s_ * 0.250000001));
		const Scalar theta = x(2);
		
		const Scalar rs_4r = r_s_ / (static_cast<Scalar>(4) * r);
		const Scalar num = static_cast<Scalar>(1) - rs_4r;
		const Scalar den = static_cast<Scalar>(1) + rs_4r;
		const Scalar factor_t = num / den;
		const Scalar den2 = den * den;
		const Scalar factor_s = den2 * den2;
		const Scalar sin_t = std::sin(theta);

		Core::MetricTensor<Scalar> g;
		g.zero();
		g(0, 0) = -factor_t * factor_t * (c_ * c_);
		g(1, 1) = factor_s;
		g(2, 2) = factor_s * r * r;
		g(3, 3) = factor_s * r * r * sin_t * sin_t;
		return g;
	}

	[[nodiscard]] Core::MetricTensor<Scalar> compute_metric(const Core::FourVector<Scalar>& x) const noexcept {
		return metric_tensor(x);
	}

	[[nodiscard]] Core::MetricTensor<Scalar> inverse_metric(const Core::FourVector<Scalar>& x) const noexcept {
		const Scalar r = std::max(x(1), static_cast<Scalar>(r_s_ * 0.250000001));
		const Scalar theta = x(2);
		
		const Scalar rs_4r = r_s_ / (static_cast<Scalar>(4) * r);
		const Scalar num = static_cast<Scalar>(1) - rs_4r;
		const Scalar den = static_cast<Scalar>(1) + rs_4r;
		const Scalar factor_t = num / den;
		const Scalar den2 = den * den;
		const Scalar factor_s = den2 * den2;
		const Scalar sin_t = std::sin(theta);
		const Scalar safe_sin2 = std::max(sin_t * sin_t, static_cast<Scalar>(1e-30));

		Core::MetricTensor<Scalar> inv_g;
		inv_g.zero();
		inv_g(0, 0) = -static_cast<Scalar>(1) / (factor_t * factor_t * c_ * c_);
		inv_g(1, 1) = static_cast<Scalar>(1) / factor_s;
		inv_g(2, 2) = static_cast<Scalar>(1) / (factor_s * r * r);
		inv_g(3, 3) = static_cast<Scalar>(1) / (factor_s * r * r * safe_sin2);
		return inv_g;
	}

	[[nodiscard]] Core::ChristoffelSymbols<Scalar> christoffel_symbols(const Core::FourVector<Scalar>& x) const noexcept {
		const Scalar r = std::max(x(1), static_cast<Scalar>(r_s_ * 0.250000001));
		const Scalar theta = x(2);
		const Scalar sin_t = std::sin(theta);
		const Scalar cos_t = std::cos(theta);

		const Scalar rs_4r = r_s_ / (static_cast<Scalar>(4) * r);
		const Scalar num = static_cast<Scalar>(1) - rs_4r;
		const Scalar den = static_cast<Scalar>(1) + rs_4r;
		const Scalar r2 = r * r;

		const Scalar d_ln_a = r_s_ / (static_cast<Scalar>(2) * r2 * num * den);
		const Scalar d_ln_b = -r_s_ / (static_cast<Scalar>(2) * r2 * den);

		const Scalar den2 = den * den;
		const Scalar den4 = den2 * den2;
		const Scalar a2_c2 = (num * num / (den * den)) * (c_ * c_);

		Core::ChristoffelSymbols<Scalar> gamma;
		gamma.zero();

		gamma(0, 0, 1) = d_ln_a;
		gamma(0, 1, 0) = d_ln_a;

		gamma(1, 0, 0) = (a2_c2 * d_ln_a) / den4;
		gamma(1, 1, 1) = d_ln_b;
		gamma(1, 2, 2) = -r * (static_cast<Scalar>(1) + r * d_ln_b);
		gamma(1, 3, 3) = gamma(1, 2, 2) * sin_t * sin_t;

		const Scalar g212 = (static_cast<Scalar>(1) / r) + d_ln_b;
		gamma(2, 1, 2) = g212;
		gamma(2, 2, 1) = g212;
		gamma(2, 3, 3) = -sin_t * cos_t;

		gamma(3, 1, 3) = g212;
		gamma(3, 3, 1) = g212;
		const Scalar g323 = (std::abs(sin_t) > static_cast<Scalar>(1e-15)) ? (cos_t / sin_t) : static_cast<Scalar>(0);
		gamma(3, 2, 3) = g323;
		gamma(3, 3, 2) = g323;

		return gamma;
	}

	[[nodiscard]] Core::ChristoffelSymbols<Scalar> compute_christoffel(const Core::FourVector<Scalar>& x) const noexcept {
		return christoffel_symbols(x);
	}
};

}

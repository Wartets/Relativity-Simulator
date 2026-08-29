#pragma once

#include "relativistic/core/tensor.hpp"
#include "relativistic/metrics/spacetime_concept.hpp"
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <numbers>
#include <algorithm>

namespace Relativistic::Metrics {

template <typename Scalar = double>
class SchwarzschildDeSitterMetric {
private:
	Scalar mass_;
	Scalar lambda_;
	Scalar c_;
	Scalar g_const_;
	Scalar r_s_;

public:
	explicit constexpr SchwarzschildDeSitterMetric(
		Scalar mass,
		Scalar cosmological_constant,
		Scalar speed_of_light = static_cast<Scalar>(1),
		Scalar gravitational_constant = static_cast<Scalar>(1)
	) noexcept
		: mass_(mass),
		  lambda_(cosmological_constant),
		  c_(speed_of_light),
		  g_const_(gravitational_constant),
		  r_s_(static_cast<Scalar>(2) * gravitational_constant * mass / (speed_of_light * speed_of_light)) {}

	[[nodiscard]] static constexpr bool has_analytic_christoffel() noexcept {
		return true;
	}

	[[nodiscard]] constexpr Scalar mass() const noexcept {
		return mass_;
	}

	[[nodiscard]] constexpr Scalar cosmological_constant() const noexcept {
		return lambda_;
	}

	[[nodiscard]] constexpr Scalar speed_of_light() const noexcept {
		return c_;
	}

	[[nodiscard]] constexpr Scalar gravitational_constant() const noexcept {
		return g_const_;
	}

	[[nodiscard]] constexpr Scalar schwarzschild_radius() const noexcept {
		return r_s_;
	}

	[[nodiscard]] static constexpr bool is_cartesian() noexcept {
		return false;
	}

	[[nodiscard]] static constexpr bool is_spherical() noexcept {
		return true;
	}

	[[nodiscard]] constexpr Scalar coordinate_radius(const Core::FourVector<Scalar>& x) const noexcept {
		return x(1);
	}

	[[nodiscard]] Scalar lapse_function(Scalar r) const noexcept {
		return static_cast<Scalar>(1) - r_s_ / r - (lambda_ * r * r) / static_cast<Scalar>(3);
	}

	[[nodiscard]] Core::MetricTensor<Scalar> metric_tensor(const Core::FourVector<Scalar>& x) const noexcept {
		const Scalar r = std::max(x(1), static_cast<Scalar>(1e-12));
		const Scalar theta = x(2);
		const Scalar sin_t = std::sin(theta);

		const Scalar factor = lapse_function(r);

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
		const Scalar r = std::max(x(1), static_cast<Scalar>(1e-12));
		const Scalar theta = x(2);
		const Scalar sin_t = std::sin(theta);
		const Scalar safe_sin2 = std::max(sin_t * sin_t, static_cast<Scalar>(1e-30));

		const Scalar factor = lapse_function(r);

		Core::MetricTensor<Scalar> inv_g;
		inv_g.zero();
		inv_g(0, 0) = -static_cast<Scalar>(1) / (factor * c_ * c_);
		inv_g(1, 1) = factor;
		inv_g(2, 2) = static_cast<Scalar>(1) / (r * r);
		inv_g(3, 3) = static_cast<Scalar>(1) / (r * r * safe_sin2);
		return inv_g;
	}

	[[nodiscard]] Core::ChristoffelSymbols<Scalar> christoffel_symbols(const Core::FourVector<Scalar>& x) const noexcept {
		const Scalar r = std::max(x(1), static_cast<Scalar>(1e-12));
		const Scalar theta = x(2);
		const Scalar sin_t = std::sin(theta);
		const Scalar cos_t = std::cos(theta);

		const Scalar factor = lapse_function(r);
		const Scalar df_dr = r_s_ / (r * r) - (static_cast<Scalar>(2) * lambda_ * r) / static_cast<Scalar>(3);

		Core::ChristoffelSymbols<Scalar> gamma;
		gamma.zero();

		const Scalar g001 = df_dr / (static_cast<Scalar>(2) * factor);
		gamma(0, 0, 1) = g001;
		gamma(0, 1, 0) = g001;

		gamma(1, 0, 0) = (c_ * c_ * factor * df_dr) / static_cast<Scalar>(2);
		gamma(1, 1, 1) = -df_dr / (static_cast<Scalar>(2) * factor);
		gamma(1, 2, 2) = -r * factor;
		gamma(1, 3, 3) = -r * factor * sin_t * sin_t;

		const Scalar g212 = static_cast<Scalar>(1) / r;
		gamma(2, 1, 2) = g212;
		gamma(2, 2, 1) = g212;
		gamma(2, 3, 3) = -sin_t * cos_t;

		const Scalar g313 = static_cast<Scalar>(1) / r;
		gamma(3, 1, 3) = g313;
		gamma(3, 3, 1) = g313;

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

#pragma once

#include "relativistic/core/tensor.hpp"
#include "relativistic/core/tensor_ops.hpp"
#include "relativistic/core/constants.hpp"
#include "relativistic/metrics/spacetime_concept.hpp"
#include <cmath>
#include <numbers>
#include <algorithm>
#include <concepts>

namespace Relativistic::ModifiedGravity {

template <typename Scalar = double>
class TeVeSSpacetimeMetric {
private:
	Scalar mass_;
	Scalar a0_{static_cast<Scalar>(1.2e-10)};
	Scalar k_scalar_{static_cast<Scalar>(0.01)};
	Scalar c_{static_cast<Scalar>(Core::PhysicalConstants<double>::SPEED_OF_LIGHT)};
	Scalar g_const_{static_cast<Scalar>(Core::PhysicalConstants<double>::GRAVITATIONAL_CONSTANT)};
	Scalar r_0_{static_cast<Scalar>(1e20)};

public:
	explicit constexpr TeVeSSpacetimeMetric(
		Scalar mass,
		Scalar a0 = static_cast<Scalar>(1.2e-10),
		Scalar k_scalar = static_cast<Scalar>(0.01),
		Scalar speed_of_light = static_cast<Scalar>(Core::PhysicalConstants<double>::SPEED_OF_LIGHT),
		Scalar gravitational_constant = static_cast<Scalar>(Core::PhysicalConstants<double>::GRAVITATIONAL_CONSTANT),
		Scalar reference_scale = static_cast<Scalar>(1e20)
	) noexcept
		: mass_(mass),
		  a0_(a0),
		  k_scalar_(k_scalar),
		  c_(speed_of_light),
		  g_const_(gravitational_constant),
		  r_0_(reference_scale) {}

	[[nodiscard]] static constexpr bool has_analytic_christoffel() noexcept { return true; }
	[[nodiscard]] constexpr Scalar mass() const noexcept { return mass_; }
	[[nodiscard]] constexpr Scalar critical_acceleration() const noexcept { return a0_; }
	[[nodiscard]] constexpr Scalar scalar_coupling() const noexcept { return k_scalar_; }
	[[nodiscard]] constexpr Scalar speed_of_light() const noexcept { return c_; }
	[[nodiscard]] constexpr Scalar gravitational_constant() const noexcept { return g_const_; }

	[[nodiscard]] static constexpr bool is_cartesian() noexcept { return false; }
	[[nodiscard]] static constexpr bool is_spherical() noexcept { return true; }
	[[nodiscard]] constexpr Scalar coordinate_radius(const Core::FourVector<Scalar>& x) const noexcept { return x(1); }

	[[nodiscard]] Scalar scalar_field_phi(Scalar r) const noexcept {
		const Scalar r_safe = std::max(r, static_cast<Scalar>(1e-12));
		const Scalar c2 = c_ * c_;
		const Scalar r_0 = std::sqrt(g_const_ * mass_ / a0_);
		const Scalar x = r_safe / r_0;
		const Scalar sqrt_term = std::sqrt(static_cast<Scalar>(1.0) + static_cast<Scalar>(4.0) * x * x);
		const Scalar bracket = (static_cast<Scalar>(1.0) - sqrt_term) / x + static_cast<Scalar>(2.0) * std::log(static_cast<Scalar>(2.0) * x + sqrt_term);
		return (std::sqrt(g_const_ * mass_ * a0_) / (static_cast<Scalar>(2.0) * c2)) * bracket;
	}

	[[nodiscard]] Scalar d_phi_dr(Scalar r) const noexcept {
		const Scalar r_safe = std::max(r, static_cast<Scalar>(1e-12));
		const Scalar c2 = c_ * c_;
		const Scalar r_0 = std::sqrt(g_const_ * mass_ / a0_);
		const Scalar x = r_safe / r_0;
		const Scalar sqrt_term = std::sqrt(static_cast<Scalar>(1.0) + static_cast<Scalar>(4.0) * x * x);
		const Scalar d_bracket_dx = (sqrt_term - static_cast<Scalar>(1.0)) / (x * x);
		return (std::sqrt(g_const_ * mass_ * a0_) / (static_cast<Scalar>(2.0) * c2 * r_0)) * d_bracket_dx;
	}

	[[nodiscard]] Core::MetricTensor<Scalar> metric_tensor(const Core::FourVector<Scalar>& x) const noexcept {
		const Scalar r = std::max(x(1), static_cast<Scalar>(1e-12));
		const Scalar theta = x(2);
		const Scalar sin_t = std::sin(theta);
		const Scalar c2 = c_ * c_;

		const Scalar phi = scalar_field_phi(r);
		const Scalar exp_2phi = std::exp(static_cast<Scalar>(2.0) * phi);
		const Scalar exp_minus_2phi = std::exp(-static_cast<Scalar>(2.0) * phi);

		const Scalar r_s = static_cast<Scalar>(2.0) * g_const_ * mass_ / c2;
		const Scalar factor_s = std::max(static_cast<Scalar>(1.0) - r_s / r, static_cast<Scalar>(1e-12));

		Core::MetricTensor<Scalar> g;
		g.zero();
		g(0, 0) = -c2 * exp_2phi * factor_s;
		g(1, 1) = exp_minus_2phi / factor_s;
		g(2, 2) = exp_minus_2phi * r * r;
		g(3, 3) = exp_minus_2phi * r * r * sin_t * sin_t;
		return g;
	}

	[[nodiscard]] Core::MetricTensor<Scalar> inverse_metric(const Core::FourVector<Scalar>& x) const noexcept {
		const Scalar r = std::max(x(1), static_cast<Scalar>(1e-12));
		const Scalar theta = x(2);
		const Scalar sin_t = std::sin(theta);
		const Scalar safe_sin2 = std::max(sin_t * sin_t, static_cast<Scalar>(1e-30));
		const Scalar c2 = c_ * c_;

		const Scalar phi = scalar_field_phi(r);
		const Scalar exp_2phi = std::exp(static_cast<Scalar>(2.0) * phi);

		const Scalar r_s = static_cast<Scalar>(2.0) * g_const_ * mass_ / c2;
		const Scalar factor_s = std::max(static_cast<Scalar>(1.0) - r_s / r, static_cast<Scalar>(1e-12));

		Core::MetricTensor<Scalar> inv_g;
		inv_g.zero();
		inv_g(0, 0) = -static_cast<Scalar>(1.0) / (c2 * exp_2phi * factor_s);
		inv_g(1, 1) = exp_2phi * factor_s;
		inv_g(2, 2) = exp_2phi / (r * r);
		inv_g(3, 3) = exp_2phi / (r * r * safe_sin2);
		return inv_g;
	}

	[[nodiscard]] Core::ChristoffelSymbols<Scalar> christoffel_symbols(const Core::FourVector<Scalar>& x) const noexcept {
		const Scalar r = std::max(x(1), static_cast<Scalar>(1e-12));
		const Scalar theta = x(2);
		const Scalar sin_t = std::sin(theta);
		const Scalar cos_t = std::cos(theta);
		const Scalar c2 = c_ * c_;

		const Scalar phi = scalar_field_phi(r);
		const Scalar dphi = d_phi_dr(r);
		const Scalar exp_4phi = std::exp(static_cast<Scalar>(4.0) * phi);

		const Scalar r_s = static_cast<Scalar>(2.0) * g_const_ * mass_ / c2;
		const Scalar factor_s = std::max(static_cast<Scalar>(1.0) - r_s / r, static_cast<Scalar>(1e-12));
		const Scalar d_factor_s = r_s / (r * r);

		Core::ChristoffelSymbols<Scalar> gamma;
		gamma.zero();

		const Scalar g001 = dphi + static_cast<Scalar>(0.5) * d_factor_s / factor_s;
		gamma(0, 0, 1) = g001;
		gamma(0, 1, 0) = g001;

		gamma(1, 0, 0) = c2 * exp_4phi * factor_s * factor_s * g001;
		gamma(1, 1, 1) = -dphi - static_cast<Scalar>(0.5) * d_factor_s / factor_s;
		gamma(1, 2, 2) = -factor_s * r * (static_cast<Scalar>(1.0) - r * dphi);
		gamma(1, 3, 3) = gamma(1, 2, 2) * sin_t * sin_t;

		const Scalar g212 = static_cast<Scalar>(1.0) / r - dphi;
		gamma(2, 1, 2) = g212;
		gamma(2, 2, 1) = g212;
		gamma(2, 3, 3) = -sin_t * cos_t;

		gamma(3, 1, 3) = g212;
		gamma(3, 3, 1) = g212;
		const Scalar g323 = (std::abs(sin_t) > static_cast<Scalar>(1e-15)) ? (cos_t / sin_t) : static_cast<Scalar>(0.0);
		gamma(3, 2, 3) = g323;
		gamma(3, 3, 2) = g323;

		return gamma;
	}
};

}

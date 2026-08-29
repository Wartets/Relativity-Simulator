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
class KerrNewmanMetric {
private:
	Scalar mass_;
	Scalar spin_;
	Scalar charge_;
	Scalar c_;
	Scalar g_const_;
	Scalar ke_const_;
	Scalar r_g_;
	Scalar r_q2_;

public:
	explicit constexpr KerrNewmanMetric(
		Scalar mass,
		Scalar spin_parameter,
		Scalar charge,
		Scalar speed_of_light = static_cast<Scalar>(1),
		Scalar gravitational_constant = static_cast<Scalar>(1),
		Scalar coulomb_constant = static_cast<Scalar>(1)
	) noexcept
		: mass_(mass),
		  spin_(spin_parameter),
		  charge_(charge),
		  c_(speed_of_light),
		  g_const_(gravitational_constant),
		  ke_const_(coulomb_constant),
		  r_g_(gravitational_constant * mass / (speed_of_light * speed_of_light)),
		  r_q2_((gravitational_constant * coulomb_constant * charge * charge) / (speed_of_light * speed_of_light * speed_of_light * speed_of_light)) {}

	[[nodiscard]] static constexpr bool has_analytic_christoffel() noexcept {
		return true;
	}

	[[nodiscard]] constexpr Scalar mass() const noexcept {
		return mass_;
	}

	[[nodiscard]] constexpr Scalar spin() const noexcept {
		return spin_;
	}

	[[nodiscard]] constexpr Scalar charge() const noexcept {
		return charge_;
	}

	[[nodiscard]] constexpr Scalar speed_of_light() const noexcept {
		return c_;
	}

	[[nodiscard]] constexpr Scalar gravitational_constant() const noexcept {
		return g_const_;
	}

	[[nodiscard]] constexpr Scalar coulomb_constant() const noexcept {
		return ke_const_;
	}

	[[nodiscard]] constexpr Scalar gravitational_radius() const noexcept {
		return r_g_;
	}

	[[nodiscard]] constexpr Scalar schwarzschild_radius() const noexcept {
		return static_cast<Scalar>(2) * r_g_;
	}

	[[nodiscard]] constexpr Scalar charge_radius_squared() const noexcept {
		return r_q2_;
	}

	[[nodiscard]] constexpr bool is_subextremal() const noexcept {
		return (spin_ * spin_ + r_q2_) < (r_g_ * r_g_);
	}

	[[nodiscard]] constexpr bool is_extremal() const noexcept {
		return (spin_ * spin_ + r_q2_) == (r_g_ * r_g_);
	}

	[[nodiscard]] constexpr bool is_hyperextremal() const noexcept {
		return (spin_ * spin_ + r_q2_) > (r_g_ * r_g_);
	}

	[[nodiscard]] Scalar outer_horizon_radius() const noexcept {
		const Scalar diff = r_g_ * r_g_ - spin_ * spin_ - r_q2_;
		if (diff < static_cast<Scalar>(0)) {
			return r_g_;
		}
		return r_g_ + std::sqrt(diff);
	}

	[[nodiscard]] Scalar inner_horizon_radius() const noexcept {
		const Scalar diff = r_g_ * r_g_ - spin_ * spin_ - r_q2_;
		if (diff < static_cast<Scalar>(0)) {
			return r_g_;
		}
		return r_g_ - std::sqrt(diff);
	}

	[[nodiscard]] Scalar outer_ergosphere_radius(Scalar theta) const noexcept {
		const Scalar cos_t = std::cos(theta);
		const Scalar diff = r_g_ * r_g_ - spin_ * spin_ * cos_t * cos_t - r_q2_;
		if (diff < static_cast<Scalar>(0)) {
			return r_g_;
		}
		return r_g_ + std::sqrt(diff);
	}

	[[nodiscard]] Scalar inner_ergosphere_radius(Scalar theta) const noexcept {
		const Scalar cos_t = std::cos(theta);
		const Scalar diff = r_g_ * r_g_ - spin_ * spin_ * cos_t * cos_t - r_q2_;
		if (diff < static_cast<Scalar>(0)) {
			return r_g_;
		}
		return r_g_ - std::sqrt(diff);
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

	[[nodiscard]] Core::MetricTensor<Scalar> metric_tensor(const Core::FourVector<Scalar>& x) const noexcept {
		const Scalar r = std::max(x(1), static_cast<Scalar>(1e-12));
		const Scalar theta = x(2);
		const Scalar sin_t = std::sin(theta);
		const Scalar cos_t = std::cos(theta);
		const Scalar sin2_t = sin_t * sin_t;
		const Scalar cos2_t = cos_t * cos_t;

		const Scalar a = spin_;
		const Scalar a2 = a * a;
		const Scalar r2 = r * r;
		const Scalar rho2 = r2 + a2 * cos2_t;
		const Scalar delta = r2 - static_cast<Scalar>(2) * r_g_ * r + a2 + r_q2_;
		const Scalar two_mr_minus_q2 = static_cast<Scalar>(2) * r_g_ * r - r_q2_;
		const Scalar sigma = (r2 + a2) * (r2 + a2) - a2 * delta * sin2_t;

		Core::MetricTensor<Scalar> g;
		g.zero();

		g(0, 0) = -(c_ * c_) * (static_cast<Scalar>(1) - two_mr_minus_q2 / rho2);
		g(0, 3) = -c_ * (two_mr_minus_q2 * a * sin2_t) / rho2;
		g(3, 0) = g(0, 3);
		g(1, 1) = rho2 / delta;
		g(2, 2) = rho2;
		g(3, 3) = (sigma * sin2_t) / rho2;

		return g;
	}

	[[nodiscard]] Core::MetricTensor<Scalar> compute_metric(const Core::FourVector<Scalar>& x) const noexcept {
		return metric_tensor(x);
	}

	[[nodiscard]] Core::MetricTensor<Scalar> inverse_metric(const Core::FourVector<Scalar>& x) const noexcept {
		const Scalar r = std::max(x(1), static_cast<Scalar>(1e-12));
		const Scalar theta = x(2);
		const Scalar sin_t = std::sin(theta);
		const Scalar cos_t = std::cos(theta);
		const Scalar sin2_t = std::max(sin_t * sin_t, static_cast<Scalar>(1e-30));
		const Scalar cos2_t = cos_t * cos_t;

		const Scalar a = spin_;
		const Scalar a2 = a * a;
		const Scalar r2 = r * r;
		const Scalar rho2 = r2 + a2 * cos2_t;
		const Scalar delta = r2 - static_cast<Scalar>(2) * r_g_ * r + a2 + r_q2_;
		const Scalar two_mr_minus_q2 = static_cast<Scalar>(2) * r_g_ * r - r_q2_;
		const Scalar sigma = (r2 + a2) * (r2 + a2) - a2 * delta * sin2_t;

		Core::MetricTensor<Scalar> inv_g;
		inv_g.zero();

		inv_g(0, 0) = -sigma / (c_ * c_ * rho2 * delta);
		inv_g(0, 3) = -(two_mr_minus_q2 * a) / (c_ * rho2 * delta);
		inv_g(3, 0) = inv_g(0, 3);
		inv_g(1, 1) = delta / rho2;
		inv_g(2, 2) = static_cast<Scalar>(1) / rho2;
		inv_g(3, 3) = (delta - a2 * sin2_t) / (rho2 * delta * sin2_t);

		return inv_g;
	}

	[[nodiscard]] Core::ChristoffelSymbols<Scalar> christoffel_symbols(const Core::FourVector<Scalar>& x) const noexcept {
		const Scalar r = std::max(x(1), static_cast<Scalar>(1e-12));
		const Scalar theta = x(2);
		const Scalar sin_t = std::sin(theta);
		const Scalar cos_t = std::cos(theta);
		const Scalar sin2_t = sin_t * sin_t;
		const Scalar cos2_t = cos_t * cos_t;

		const Scalar a = spin_;
		const Scalar a2 = a * a;
		const Scalar r2 = r * r;
		const Scalar rho2 = r2 + a2 * cos2_t;
		const Scalar rho4 = rho2 * rho2;
		const Scalar delta = r2 - static_cast<Scalar>(2) * r_g_ * r + a2 + r_q2_;
		const Scalar two_mr_minus_q2 = static_cast<Scalar>(2) * r_g_ * r - r_q2_;

		const Scalar d_two_mr_minus_q2 = static_cast<Scalar>(2) * r_g_;
		const Scalar d1_rho2 = static_cast<Scalar>(2) * r;
		const Scalar d1_ratio = (d_two_mr_minus_q2 * rho2 - two_mr_minus_q2 * d1_rho2) / rho4;

		Core::MetricTensor<Scalar> d1_g;
		d1_g.zero();
		d1_g(0, 0) = -(c_ * c_) * (-d1_ratio);
		d1_g(0, 3) = -c_ * a * sin2_t * d1_ratio;
		d1_g(3, 0) = d1_g(0, 3);

		const Scalar d1_delta = static_cast<Scalar>(2) * (r - r_g_);
		d1_g(1, 1) = (d1_rho2 * delta - rho2 * d1_delta) / (delta * delta);
		d1_g(2, 2) = d1_rho2;

		const Scalar d1_sigma = static_cast<Scalar>(4) * r * (r2 + a2) - a2 * d1_delta * sin2_t;
		d1_g(3, 3) = sin2_t * (d1_sigma * rho2 - ((r2 + a2) * (r2 + a2) - a2 * delta * sin2_t) * d1_rho2) / rho4;

		const Scalar sin_cos = sin_t * cos_t;
		const Scalar d2_rho2 = -static_cast<Scalar>(2) * a2 * sin_cos;
		const Scalar d2_ratio = -two_mr_minus_q2 * d2_rho2 / rho4;

		Core::MetricTensor<Scalar> d2_g;
		d2_g.zero();
		d2_g(0, 0) = -(c_ * c_) * (-d2_ratio);
		d2_g(0, 3) = -c_ * a * (static_cast<Scalar>(2) * sin_cos * two_mr_minus_q2 * rho2 - two_mr_minus_q2 * sin2_t * d2_rho2) / rho4;
		d2_g(3, 0) = d2_g(0, 3);
		d2_g(1, 1) = d2_rho2 / delta;
		d2_g(2, 2) = d2_rho2;

		const Scalar sigma = (r2 + a2) * (r2 + a2) - a2 * delta * sin2_t;
		const Scalar d2_sigma = -static_cast<Scalar>(2) * a2 * delta * sin_cos;
		const Scalar d2_term = static_cast<Scalar>(2) * sin_cos * sigma * rho2 + sin2_t * d2_sigma * rho2 - sigma * sin2_t * d2_rho2;
		d2_g(3, 3) = d2_term / rho4;

		const auto inv_g = inverse_metric(x);

		Core::ChristoffelSymbols<Scalar> gamma;
		gamma.zero();

		for (size_t sigma_idx = 0; sigma_idx < 4; ++sigma_idx) {
			for (size_t mu = 0; mu < 4; ++mu) {
				for (size_t nu = mu; nu < 4; ++nu) {
					Scalar sum = static_cast<Scalar>(0);
					for (size_t lambda = 0; lambda < 4; ++lambda) {
						const Scalar g_inv_elem = inv_g(sigma_idx, lambda);
						if (g_inv_elem == static_cast<Scalar>(0)) {
							continue;
						}
						Scalar term = static_cast<Scalar>(0);
						if (mu == 1) term += d1_g(nu, lambda);
						else if (mu == 2) term += d2_g(nu, lambda);

						if (nu == 1) term += d1_g(mu, lambda);
						else if (nu == 2) term += d2_g(mu, lambda);

						if (lambda == 1) term -= d1_g(mu, nu);
						else if (lambda == 2) term -= d2_g(mu, nu);

						sum += g_inv_elem * term;
					}
					const Scalar val = static_cast<Scalar>(0.5) * sum;
					gamma(sigma_idx, mu, nu) = val;
					gamma(sigma_idx, nu, mu) = val;
				}
			}
		}

		return gamma;
	}

	[[nodiscard]] Core::ChristoffelSymbols<Scalar> compute_christoffel(const Core::FourVector<Scalar>& x) const noexcept {
		return christoffel_symbols(x);
	}
};

}

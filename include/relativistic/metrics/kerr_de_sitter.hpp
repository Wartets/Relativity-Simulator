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
class KerrDeSitterMetric {
private:
	Scalar mass_;
	Scalar spin_;
	Scalar lambda_;
	Scalar c_;
	Scalar g_const_;
	Scalar r_g_;
	Scalar xi_;

public:
	explicit constexpr KerrDeSitterMetric(
		Scalar mass,
		Scalar spin_parameter,
		Scalar cosmological_constant,
		Scalar speed_of_light = static_cast<Scalar>(1),
		Scalar gravitational_constant = static_cast<Scalar>(1)
	) noexcept
		: mass_(mass),
		  spin_(spin_parameter),
		  lambda_(cosmological_constant),
		  c_(speed_of_light),
		  g_const_(gravitational_constant),
		  r_g_(gravitational_constant * mass / (speed_of_light * speed_of_light)),
		  xi_(static_cast<Scalar>(1) + (cosmological_constant * spin_parameter * spin_parameter) / static_cast<Scalar>(3)) {}

	[[nodiscard]] static constexpr bool has_analytic_christoffel() noexcept {
		return true;
	}

	[[nodiscard]] constexpr Scalar mass() const noexcept {
		return mass_;
	}

	[[nodiscard]] constexpr Scalar spin() const noexcept {
		return spin_;
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

	[[nodiscard]] constexpr Scalar gravitational_radius() const noexcept {
		return r_g_;
	}

	[[nodiscard]] constexpr Scalar schwarzschild_radius() const noexcept {
		return static_cast<Scalar>(2) * r_g_;
	}

	[[nodiscard]] constexpr Scalar xi_factor() const noexcept {
		return xi_;
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
		const Scalar xi2 = xi_ * xi_;

		const Scalar delta_r = (r2 + a2) * (static_cast<Scalar>(1) - (lambda_ * r2) / static_cast<Scalar>(3)) - static_cast<Scalar>(2) * r_g_ * r;
		const Scalar delta_theta = static_cast<Scalar>(1) + (lambda_ * a2 * cos2_t) / static_cast<Scalar>(3);

		Core::MetricTensor<Scalar> g;
		g.zero();

		const Scalar term_t = delta_r - a2 * delta_theta * sin2_t;
		g(0, 0) = -((c_ * c_) / (rho2 * xi2)) * term_t;

		const Scalar term_t_phi = a * sin2_t * (delta_r - (r2 + a2) * delta_theta);
		g(0, 3) = (c_ / (rho2 * xi2)) * term_t_phi;
		g(3, 0) = g(0, 3);

		g(1, 1) = rho2 / delta_r;
		g(2, 2) = rho2 / delta_theta;

		const Scalar term_phi = (r2 + a2) * (r2 + a2) * delta_theta - a2 * delta_r * sin2_t;
		g(3, 3) = (sin2_t / (rho2 * xi2)) * term_phi;

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
		const Scalar xi2 = xi_ * xi_;

		const Scalar delta_r = (r2 + a2) * (static_cast<Scalar>(1) - (lambda_ * r2) / static_cast<Scalar>(3)) - static_cast<Scalar>(2) * r_g_ * r;
		const Scalar delta_theta = static_cast<Scalar>(1) + (lambda_ * a2 * cos2_t) / static_cast<Scalar>(3);

		Core::MetricTensor<Scalar> inv_g;
		inv_g.zero();

		const Scalar denom_det = rho2 * delta_r * delta_theta;
		const Scalar term_phi = (r2 + a2) * (r2 + a2) * delta_theta - a2 * delta_r * sin2_t;
		inv_g(0, 0) = -(xi2 * term_phi) / (c_ * c_ * denom_det);

		const Scalar term_t_phi = a * (delta_r - (r2 + a2) * delta_theta);
		inv_g(0, 3) = (xi2 * term_t_phi) / (c_ * denom_det);
		inv_g(3, 0) = inv_g(0, 3);

		inv_g(1, 1) = delta_r / rho2;
		inv_g(2, 2) = delta_theta / rho2;

		const Scalar term_t = delta_r - a2 * delta_theta * sin2_t;
		inv_g(3, 3) = (xi2 * term_t) / (denom_det * sin2_t);

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
		const Scalar xi2 = xi_ * xi_;

		const Scalar delta_r = (r2 + a2) * (static_cast<Scalar>(1) - (lambda_ * r2) / static_cast<Scalar>(3)) - static_cast<Scalar>(2) * r_g_ * r;
		const Scalar delta_theta = static_cast<Scalar>(1) + (lambda_ * a2 * cos2_t) / static_cast<Scalar>(3);

		const Scalar d1_delta_r = static_cast<Scalar>(2) * r * (static_cast<Scalar>(1) - (lambda_ * r2) / static_cast<Scalar>(3))
		                        - (r2 + a2) * (static_cast<Scalar>(2) * lambda_ * r) / static_cast<Scalar>(3)
		                        - static_cast<Scalar>(2) * r_g_;
		const Scalar d1_rho2 = static_cast<Scalar>(2) * r;

		Core::MetricTensor<Scalar> d1_g;
		d1_g.zero();

		const Scalar term_t = delta_r - a2 * delta_theta * sin2_t;
		const Scalar d1_term_t = d1_delta_r;
		d1_g(0, 0) = -((c_ * c_) / xi2) * (d1_term_t * rho2 - term_t * d1_rho2) / rho4;

		const Scalar term_t_phi_base = delta_r - (r2 + a2) * delta_theta;
		const Scalar d1_term_t_phi_base = d1_delta_r - static_cast<Scalar>(2) * r * delta_theta;
		d1_g(0, 3) = ((c_ * a * sin2_t) / xi2) * (d1_term_t_phi_base * rho2 - term_t_phi_base * d1_rho2) / rho4;
		d1_g(3, 0) = d1_g(0, 3);

		d1_g(1, 1) = (d1_rho2 * delta_r - rho2 * d1_delta_r) / (delta_r * delta_r);
		d1_g(2, 2) = d1_rho2 / delta_theta;

		const Scalar term_phi = (r2 + a2) * (r2 + a2) * delta_theta - a2 * delta_r * sin2_t;
		const Scalar d1_term_phi = static_cast<Scalar>(4) * r * (r2 + a2) * delta_theta - a2 * d1_delta_r * sin2_t;
		d1_g(3, 3) = (sin2_t / xi2) * (d1_term_phi * rho2 - term_phi * d1_rho2) / rho4;

		const Scalar sin_cos = sin_t * cos_t;
		const Scalar d2_delta_theta = -static_cast<Scalar>(2) * lambda_ * a2 * sin_cos / static_cast<Scalar>(3);
		const Scalar d2_rho2 = -static_cast<Scalar>(2) * a2 * sin_cos;

		Core::MetricTensor<Scalar> d2_g;
		d2_g.zero();

		const Scalar d2_term_t = -a2 * (d2_delta_theta * sin2_t + delta_theta * static_cast<Scalar>(2) * sin_cos);
		d2_g(0, 0) = -((c_ * c_) / xi2) * (d2_term_t * rho2 - term_t * d2_rho2) / rho4;

		const Scalar d2_term_t_phi = a * (static_cast<Scalar>(2) * sin_cos * term_t_phi_base + sin2_t * (-(r2 + a2) * d2_delta_theta));
		d2_g(0, 3) = (c_ / xi2) * (d2_term_t_phi * rho2 - (a * sin2_t * term_t_phi_base) * d2_rho2) / rho4;
		d2_g(3, 0) = d2_g(0, 3);

		d2_g(1, 1) = d2_rho2 / delta_r;
		d2_g(2, 2) = (d2_rho2 * delta_theta - rho2 * d2_delta_theta) / (delta_theta * delta_theta);

		const Scalar d2_term_phi = (r2 + a2) * (r2 + a2) * d2_delta_theta - a2 * delta_r * static_cast<Scalar>(2) * sin_cos;
		const Scalar full_d2_phi = static_cast<Scalar>(2) * sin_cos * term_phi + sin2_t * d2_term_phi;
		d2_g(3, 3) = (static_cast<Scalar>(1) / xi2) * (full_d2_phi * rho2 - (sin2_t * term_phi) * d2_rho2) / rho4;

		const auto inv_g = inverse_metric(x);

		Core::ChristoffelSymbols<Scalar> gamma;
		gamma.zero();

		for (size_t sigma_idx = 0; sigma_idx < 4; ++sigma_idx) {
			for (size_t mu = 0; mu < 4; ++mu) {
				for (size_t nu = mu; nu < 4; ++nu) {
					Scalar sum = static_cast<Scalar>(0);
					for (size_t lambda_idx = 0; lambda_idx < 4; ++lambda_idx) {
						const Scalar g_inv_elem = inv_g(sigma_idx, lambda_idx);
						if (g_inv_elem == static_cast<Scalar>(0)) {
							continue;
						}
						Scalar term = static_cast<Scalar>(0);
						if (mu == 1) term += d1_g(nu, lambda_idx);
						else if (mu == 2) term += d2_g(nu, lambda_idx);

						if (nu == 1) term += d1_g(mu, lambda_idx);
						else if (nu == 2) term += d2_g(mu, lambda_idx);

						if (lambda_idx == 1) term -= d1_g(mu, nu);
						else if (lambda_idx == 2) term -= d2_g(mu, nu);

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

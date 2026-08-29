#pragma once

#include "relativistic/core/tensor.hpp"
#include "relativistic/metrics/spacetime_concept.hpp"
#include <cmath>
#include <algorithm>

namespace Relativistic::Metrics {

template <typename Scalar = double>
class KerrSchildMetric {
private:
	Scalar mass_;
	Scalar spin_;
	Scalar c_;
	Scalar g_const_;
	Scalar r_g_;

	struct AuxiliaryFields {
		Scalar r;
		Scalar r2;
		Scalar H;
		Core::FourVector<Scalar> k_cov;
		Core::FourVector<Scalar> k_con;
	};

	[[nodiscard]] AuxiliaryFields compute_auxiliary(const Core::FourVector<Scalar>& x) const noexcept {
		const Scalar x1 = x(1);
		const Scalar x2 = x(2);
		const Scalar x3 = x(3);
		const Scalar a = spin_;
		const Scalar a2 = a * a;

		const Scalar capital_r2 = x1 * x1 + x2 * x2 + x3 * x3;
		const Scalar diff = capital_r2 - a2;
		const Scalar discr = diff * diff + static_cast<Scalar>(4) * a2 * x3 * x3;
		const Scalar sqrt_discr = std::sqrt(std::max(discr, static_cast<Scalar>(0)));
		const Scalar r2 = std::max(static_cast<Scalar>(0.5) * (diff + sqrt_discr), static_cast<Scalar>(1e-24));
		const Scalar r = std::sqrt(r2);

		const Scalar denom_h = r2 * r2 + a2 * x3 * x3;
		const Scalar H = (r_g_ * r2 * r) / denom_h;

		const Scalar r2_plus_a2 = r2 + a2;
		const Scalar k0 = c_;
		const Scalar k1 = (r * x1 + a * x2) / r2_plus_a2;
		const Scalar k2 = (r * x2 - a * x1) / r2_plus_a2;
		const Scalar k3 = x3 / r;

		AuxiliaryFields aux;
		aux.r = r;
		aux.r2 = r2;
		aux.H = H;
		aux.k_cov = Core::FourVector<Scalar>(k0, k1, k2, k3);
		aux.k_con = Core::FourVector<Scalar>(-static_cast<Scalar>(1) / c_, k1, k2, k3);
		return aux;
	}

public:
	explicit constexpr KerrSchildMetric(
		Scalar mass,
		Scalar spin_parameter,
		Scalar speed_of_light = static_cast<Scalar>(1),
		Scalar gravitational_constant = static_cast<Scalar>(1)
	) noexcept
		: mass_(mass),
		  spin_(spin_parameter),
		  c_(speed_of_light),
		  g_const_(gravitational_constant),
		  r_g_(gravitational_constant * mass / (speed_of_light * speed_of_light)) {}

	[[nodiscard]] static constexpr bool has_analytic_christoffel() noexcept {
		return true;
	}

	[[nodiscard]] constexpr Scalar mass() const noexcept {
		return mass_;
	}

	[[nodiscard]] constexpr Scalar spin() const noexcept {
		return spin_;
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

	[[nodiscard]] Scalar outer_horizon_radius() const noexcept {
		const Scalar diff = r_g_ * r_g_ - spin_ * spin_;
		if (diff < static_cast<Scalar>(0)) {
			return r_g_;
		}
		return r_g_ + std::sqrt(diff);
	}

	[[nodiscard]] Scalar inner_horizon_radius() const noexcept {
		const Scalar diff = r_g_ * r_g_ - spin_ * spin_;
		if (diff < static_cast<Scalar>(0)) {
			return r_g_;
		}
		return r_g_ - std::sqrt(diff);
	}

	[[nodiscard]] static constexpr bool is_cartesian() noexcept {
		return true;
	}

	[[nodiscard]] static constexpr bool is_spherical() noexcept {
		return false;
	}

	[[nodiscard]] Scalar coordinate_radius(const Core::FourVector<Scalar>& x) const noexcept {
		return boyer_lindquist_r(x);
	}

	[[nodiscard]] Scalar boyer_lindquist_r(const Core::FourVector<Scalar>& x) const noexcept {
		const Scalar x1 = x(1);
		const Scalar x2 = x(2);
		const Scalar x3 = x(3);
		const Scalar a = spin_;
		const Scalar a2 = a * a;
		const Scalar capital_r2 = x1 * x1 + x2 * x2 + x3 * x3;
		const Scalar diff = capital_r2 - a2;
		const Scalar discr = diff * diff + static_cast<Scalar>(4) * a2 * x3 * x3;
		const Scalar sqrt_discr = std::sqrt(std::max(discr, static_cast<Scalar>(0)));
		return std::sqrt(std::max(static_cast<Scalar>(0.5) * (diff + sqrt_discr), static_cast<Scalar>(1e-24)));
	}

	[[nodiscard]] Core::MetricTensor<Scalar> metric_tensor(const Core::FourVector<Scalar>& x) const noexcept {
		const auto aux = compute_auxiliary(x);
		const Scalar two_H = static_cast<Scalar>(2) * aux.H;

		Core::MetricTensor<Scalar> g;
		g.zero();

		g(0, 0) = -(c_ * c_) + two_H * aux.k_cov(0) * aux.k_cov(0);
		for (size_t i = 1; i < 4; ++i) {
			g(i, i) = static_cast<Scalar>(1) + two_H * aux.k_cov(i) * aux.k_cov(i);
		}

		for (size_t mu = 0; mu < 4; ++mu) {
			for (size_t nu = mu + 1; nu < 4; ++nu) {
				const Scalar val = two_H * aux.k_cov(mu) * aux.k_cov(nu);
				g(mu, nu) = val;
				g(nu, mu) = val;
			}
		}

		return g;
	}

	[[nodiscard]] Core::MetricTensor<Scalar> compute_metric(const Core::FourVector<Scalar>& x) const noexcept {
		return metric_tensor(x);
	}

	[[nodiscard]] Core::MetricTensor<Scalar> inverse_metric(const Core::FourVector<Scalar>& x) const noexcept {
		const auto aux = compute_auxiliary(x);
		const Scalar two_H = static_cast<Scalar>(2) * aux.H;

		Core::MetricTensor<Scalar> inv_g;
		inv_g.zero();

		inv_g(0, 0) = -static_cast<Scalar>(1) / (c_ * c_) - two_H * aux.k_con(0) * aux.k_con(0);
		for (size_t i = 1; i < 4; ++i) {
			inv_g(i, i) = static_cast<Scalar>(1) - two_H * aux.k_con(i) * aux.k_con(i);
		}

		for (size_t mu = 0; mu < 4; ++mu) {
			for (size_t nu = mu + 1; nu < 4; ++nu) {
				const Scalar val = -two_H * aux.k_con(mu) * aux.k_con(nu);
				inv_g(mu, nu) = val;
				inv_g(nu, mu) = val;
			}
		}

		return inv_g;
	}

	[[nodiscard]] Core::ChristoffelSymbols<Scalar> christoffel_symbols(const Core::FourVector<Scalar>& x) const noexcept {
		const Scalar x1 = x(1);
		const Scalar x2 = x(2);
		const Scalar x3 = x(3);
		const Scalar a = spin_;
		const Scalar a2 = a * a;

		const auto aux = compute_auxiliary(x);
		const Scalar r = aux.r;
		const Scalar r2 = aux.r2;
		const Scalar r3 = r2 * r;
		const Scalar r4 = r2 * r2;
		const Scalar H = aux.H;

		const Scalar denom_r = r4 + a2 * x3 * x3;
		const Scalar dr_dx1 = (r3 * x1) / denom_r;
		const Scalar dr_dx2 = (r3 * x2) / denom_r;
		const Scalar dr_dx3 = (r * (r2 + a2) * x3) / denom_r;
		const std::array<Scalar, 3> dr_dx = {dr_dx1, dr_dx2, dr_dx3};

		const Scalar denom_h2 = denom_r * denom_r;
		const Scalar term_bracket = static_cast<Scalar>(3) * a2 * x3 * x3 - r4;
		std::array<Scalar, 3> dH_dx{};
		for (size_t i = 0; i < 3; ++i) {
			const Scalar delta_i3 = (i == 2) ? static_cast<Scalar>(1) : static_cast<Scalar>(0);
			dH_dx[i] = r_g_ * (dr_dx[i] * r2 * term_bracket - static_cast<Scalar>(2) * a2 * r3 * x3 * delta_i3) / denom_h2;
		}

		const Scalar r2_plus_a2 = r2 + a2;
		const Scalar denom_k = r2_plus_a2 * r2_plus_a2;

		std::array<Core::FourVector<Scalar>, 3> dk_dx{};
		for (size_t i = 0; i < 3; ++i) {
			const Scalar d_r = dr_dx[i];
			const Scalar d_x1 = (i == 0) ? static_cast<Scalar>(1) : static_cast<Scalar>(0);
			const Scalar d_x2 = (i == 1) ? static_cast<Scalar>(1) : static_cast<Scalar>(0);
			const Scalar d_x3 = (i == 2) ? static_cast<Scalar>(1) : static_cast<Scalar>(0);

			const Scalar dk1 = ((d_r * x1 + r * d_x1 + a * d_x2) * r2_plus_a2 - (r * x1 + a * x2) * (static_cast<Scalar>(2) * r * d_r)) / denom_k;
			const Scalar dk2 = ((d_r * x2 + r * d_x2 - a * d_x1) * r2_plus_a2 - (r * x2 - a * x1) * (static_cast<Scalar>(2) * r * d_r)) / denom_k;
			const Scalar dk3 = (d_x3 * r - x3 * d_r) / r2;

			dk_dx[i] = Core::FourVector<Scalar>(static_cast<Scalar>(0), dk1, dk2, dk3);
		}

		std::array<Core::MetricTensor<Scalar>, 4> dg{};
		for (size_t alpha = 0; alpha < 4; ++alpha) {
			dg[alpha].zero();
			if (alpha == 0) {
				continue;
			}
			const size_t sp_idx = alpha - 1;
			const Scalar dH = dH_dx[sp_idx];
			const auto& dk = dk_dx[sp_idx];

			for (size_t mu = 0; mu < 4; ++mu) {
				for (size_t nu = 0; nu < 4; ++nu) {
					dg[alpha](mu, nu) = static_cast<Scalar>(2) * (dH * aux.k_cov(mu) * aux.k_cov(nu) + H * (dk(mu) * aux.k_cov(nu) + aux.k_cov(mu) * dk(nu)));
				}
			}
		}

		const auto inv_g = inverse_metric(x);

		Core::ChristoffelSymbols<Scalar> gamma;
		gamma.zero();

		for (size_t sigma = 0; sigma < 4; ++sigma) {
			for (size_t mu = 0; mu < 4; ++mu) {
				for (size_t nu = mu; nu < 4; ++nu) {
					Scalar sum = static_cast<Scalar>(0);
					for (size_t lambda = 0; lambda < 4; ++lambda) {
						const Scalar g_inv_val = inv_g(sigma, lambda);
						if (g_inv_val == static_cast<Scalar>(0)) {
							continue;
						}
						const Scalar term = dg[mu](nu, lambda) + dg[nu](mu, lambda) - dg[lambda](mu, nu);
						sum += g_inv_val * term;
					}
					const Scalar val = static_cast<Scalar>(0.5) * sum;
					gamma(sigma, mu, nu) = val;
					gamma(sigma, nu, mu) = val;
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

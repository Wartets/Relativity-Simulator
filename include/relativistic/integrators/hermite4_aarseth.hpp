#pragma once

#include "relativistic/core/tensor.hpp"
#include "relativistic/core/tensor_ops.hpp"
#include "relativistic/core/christoffel.hpp"
#include "relativistic/metrics/spacetime_concept.hpp"
#include "relativistic/integrators/geodesic_state.hpp"
#include <array>
#include <cmath>
#include <algorithm>
#include <optional>
#include <cstdint>

namespace Relativistic::Integrators {

template <typename Scalar = double>
struct Hermite4Config {
	Scalar eta = static_cast<Scalar>(0.02);
	Scalar min_step = static_cast<Scalar>(1e-8);
	Scalar max_step = static_cast<Scalar>(1e12);
	Scalar epsilon_fd = static_cast<Scalar>(1e-6);
};

template <typename Scalar = double>
struct Hermite4Stats {
	uint64_t total_steps = 0;
	uint64_t evaluations = 0;
	Scalar last_step_size = static_cast<Scalar>(0);
};

template <typename MetricType, typename Scalar = double>
	requires Metrics::SpacetimeMetric<MetricType, Scalar>
class Hermite4AarsethIntegrator {
private:
	const MetricType& metric_;
	Hermite4Config<Scalar> config_;
	mutable Hermite4Stats<Scalar> stats_;

	[[nodiscard]] Core::FourVector<Scalar> compute_acceleration(
		const Core::FourVector<Scalar>& x,
		const Core::FourVector<Scalar>& u
	) const noexcept {
		const auto gamma = Core::compute_christoffel<Core::DerivativeOrder::EighthOrder, MetricType, Scalar>(metric_, x);
		Core::FourVector<Scalar> acc;
		acc.zero();

		for (size_t mu = 0; mu < 4; ++mu) {
			Scalar sum = static_cast<Scalar>(0);
			for (size_t alpha = 0; alpha < 4; ++alpha) {
				const Scalar u_alpha = u(alpha);
				if (u_alpha == static_cast<Scalar>(0)) {
					continue;
				}
				sum -= gamma(mu, alpha, alpha) * u_alpha * u_alpha;
				for (size_t beta = alpha + 1; beta < 4; ++beta) {
					const Scalar u_beta = u(beta);
					if (u_beta != static_cast<Scalar>(0)) {
						sum -= static_cast<Scalar>(2) * gamma(mu, alpha, beta) * u_alpha * u_beta;
					}
				}
			}
			acc(mu) = sum;
		}
		++stats_.evaluations;
		return acc;
	}

	[[nodiscard]] Core::FourVector<Scalar> compute_jerk(
		const Core::FourVector<Scalar>& x,
		const Core::FourVector<Scalar>& u,
		const Core::FourVector<Scalar>& a
	) const noexcept {
		const Scalar eps = config_.epsilon_fd;
		Core::FourVector<Scalar> x_plus2 = x, x_plus1 = x, x_minus1 = x, x_minus2 = x;
		Core::FourVector<Scalar> u_plus2 = u, u_plus1 = u, u_minus1 = u, u_minus2 = u;

		for (size_t i = 0; i < 4; ++i) {
			x_plus2(i) += static_cast<Scalar>(2) * eps * u(i);
			u_plus2(i) += static_cast<Scalar>(2) * eps * a(i);

			x_plus1(i) += eps * u(i);
			u_plus1(i) += eps * a(i);

			x_minus1(i) -= eps * u(i);
			u_minus1(i) -= eps * a(i);

			x_minus2(i) -= static_cast<Scalar>(2) * eps * u(i);
			u_minus2(i) -= static_cast<Scalar>(2) * eps * a(i);
		}

		const auto a_p2 = compute_acceleration(x_plus2, u_plus2);
		const auto a_p1 = compute_acceleration(x_plus1, u_plus1);
		const auto a_m1 = compute_acceleration(x_minus1, u_minus1);
		const auto a_m2 = compute_acceleration(x_minus2, u_minus2);

		Core::FourVector<Scalar> jerk;
		const Scalar inv_12eps = static_cast<Scalar>(1) / (static_cast<Scalar>(12) * eps);

		for (size_t i = 0; i < 4; ++i) {
			jerk(i) = (-a_p2(i) + static_cast<Scalar>(8) * a_p1(i) - static_cast<Scalar>(8) * a_m1(i) + a_m2(i)) * inv_12eps;
		}

		return jerk;
	}

	[[nodiscard]] static Scalar vector_euclidean_norm(const Core::FourVector<Scalar>& v) noexcept {
		Scalar sum = static_cast<Scalar>(0);
		for (size_t i = 0; i < 4; ++i) {
			sum += v(i) * v(i);
		}
		return std::sqrt(sum);
	}

public:
	explicit constexpr Hermite4AarsethIntegrator(
		const MetricType& metric,
		const Hermite4Config<Scalar>& config = {}
	) noexcept
		: metric_(metric), config_(config), stats_{} {}

	[[nodiscard]] constexpr const Hermite4Stats<Scalar>& statistics() const noexcept {
		return stats_;
	}

	constexpr void reset_statistics() noexcept {
		stats_ = Hermite4Stats<Scalar>{};
	}

	[[nodiscard]] Scalar compute_aarseth_step(
		const Core::FourVector<Scalar>& a,
		const Core::FourVector<Scalar>& j,
		const Core::FourVector<Scalar>& s,
		const Core::FourVector<Scalar>& c
	) const noexcept {
		const Scalar norm_a = vector_euclidean_norm(a);
		const Scalar norm_j = vector_euclidean_norm(j);
		const Scalar norm_s = vector_euclidean_norm(s);
		const Scalar norm_c = vector_euclidean_norm(c);

		const Scalar num = norm_a * norm_s + norm_j * norm_j;
		const Scalar den = norm_j * norm_c + norm_s * norm_s + static_cast<Scalar>(1e-30);

		const Scalar dt = config_.eta * std::sqrt(num / den);
		return std::clamp(dt, config_.min_step, config_.max_step);
	}

	[[nodiscard]] Scalar step(GeodesicState<Scalar>& state, Scalar current_dt) const noexcept {
		const auto a0 = compute_acceleration(state.x, state.u);
		const auto j0 = compute_jerk(state.x, state.u, a0);

		const Scalar dt = current_dt;
		const Scalar dt2 = dt * dt;
		const Scalar dt3 = dt2 * dt;
		const Scalar dt4 = dt3 * dt;
		const Scalar dt5 = dt4 * dt;

		Core::FourVector<Scalar> x_p;
		Core::FourVector<Scalar> u_p;

		for (size_t i = 0; i < 4; ++i) {
			x_p(i) = state.x(i) + state.u(i) * dt + static_cast<Scalar>(0.5) * a0(i) * dt2 + (static_cast<Scalar>(1.0 / 6.0)) * j0(i) * dt3;
			u_p(i) = state.u(i) + a0(i) * dt + static_cast<Scalar>(0.5) * j0(i) * dt2;
		}

		const auto a1 = compute_acceleration(x_p, u_p);
		const auto j1 = compute_jerk(x_p, u_p, a1);

		Core::FourVector<Scalar> s0;
		Core::FourVector<Scalar> c0;

		const Scalar inv_dt2 = static_cast<Scalar>(1) / dt2;
		const Scalar inv_dt3 = static_cast<Scalar>(1) / dt3;

		for (size_t i = 0; i < 4; ++i) {
			s0(i) = (-static_cast<Scalar>(6) * (a0(i) - a1(i)) - dt * (static_cast<Scalar>(4) * j0(i) + static_cast<Scalar>(2) * j1(i))) * inv_dt2;
			c0(i) = (static_cast<Scalar>(12) * (a0(i) - a1(i)) + static_cast<Scalar>(6) * dt * (j0(i) + j1(i))) * inv_dt3;
		}

		for (size_t i = 0; i < 4; ++i) {
			state.x(i) = x_p(i) + (static_cast<Scalar>(1.0 / 24.0)) * s0(i) * dt4 + (static_cast<Scalar>(1.0 / 120.0)) * c0(i) * dt5;
			state.u(i) = u_p(i) + (static_cast<Scalar>(1.0 / 6.0)) * s0(i) * dt3 + (static_cast<Scalar>(1.0 / 24.0)) * c0(i) * dt4;
		}

		Core::FourVector<Scalar> s1;
		for (size_t i = 0; i < 4; ++i) {
			s1(i) = s0(i) + c0(i) * dt;
		}

		const Scalar next_dt = compute_aarseth_step(a1, j1, s1, c0);
		stats_.last_step_size = dt;
		++stats_.total_steps;

		return next_dt;
	}
};

}

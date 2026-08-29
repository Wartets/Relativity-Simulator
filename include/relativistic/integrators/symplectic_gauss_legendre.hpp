#pragma once

#include "relativistic/core/tensor.hpp"
#include "relativistic/core/tensor_ops.hpp"
#include "relativistic/core/christoffel.hpp"
#include "relativistic/metrics/spacetime_concept.hpp"
#include "relativistic/integrators/geodesic_state.hpp"
#include <array>
#include <cmath>
#include <numbers>
#include <algorithm>
#include <cstdint>

namespace Relativistic::Integrators {

template <size_t Stages, typename Scalar = double>
struct GaussLegendreTableau;

template <typename Scalar>
struct GaussLegendreTableau<2, Scalar> {
	static constexpr size_t STAGES = 2;
	static constexpr size_t ORDER = 4;

	static constexpr Scalar SQRT3 = static_cast<Scalar>(1.732050807568877293527446341505872367);

	static constexpr std::array<Scalar, 2> C = {
		static_cast<Scalar>(0.5) - SQRT3 / static_cast<Scalar>(6),
		static_cast<Scalar>(0.5) + SQRT3 / static_cast<Scalar>(6)
	};

	static constexpr std::array<Scalar, 2> B = {
		static_cast<Scalar>(0.5),
		static_cast<Scalar>(0.5)
	};

	static constexpr std::array<std::array<Scalar, 2>, 2> A = {{
		{ static_cast<Scalar>(0.25), static_cast<Scalar>(0.25) - SQRT3 / static_cast<Scalar>(6) },
		{ static_cast<Scalar>(0.25) + SQRT3 / static_cast<Scalar>(6), static_cast<Scalar>(0.25) }
	}};
};

template <typename Scalar>
struct GaussLegendreTableau<3, Scalar> {
	static constexpr size_t STAGES = 3;
	static constexpr size_t ORDER = 6;

	static constexpr Scalar SQRT15 = static_cast<Scalar>(3.872983346207416885179265399782399611);

	static constexpr std::array<Scalar, 3> C = {
		static_cast<Scalar>(0.5) - SQRT15 / static_cast<Scalar>(10),
		static_cast<Scalar>(0.5),
		static_cast<Scalar>(0.5) + SQRT15 / static_cast<Scalar>(10)
	};

	static constexpr std::array<Scalar, 3> B = {
		static_cast<Scalar>(5.0 / 18.0),
		static_cast<Scalar>(4.0 / 9.0),
		static_cast<Scalar>(5.0 / 18.0)
	};

	static constexpr std::array<std::array<Scalar, 3>, 3> A = {{
		{ static_cast<Scalar>(5.0 / 36.0), static_cast<Scalar>(2.0 / 9.0) - SQRT15 / static_cast<Scalar>(15), static_cast<Scalar>(5.0 / 36.0) - SQRT15 / static_cast<Scalar>(30) },
		{ static_cast<Scalar>(5.0 / 36.0) + SQRT15 / static_cast<Scalar>(24), static_cast<Scalar>(2.0 / 9.0), static_cast<Scalar>(5.0 / 36.0) - SQRT15 / static_cast<Scalar>(24) },
		{ static_cast<Scalar>(5.0 / 36.0) + SQRT15 / static_cast<Scalar>(30), static_cast<Scalar>(2.0 / 9.0) + SQRT15 / static_cast<Scalar>(15), static_cast<Scalar>(5.0 / 36.0) }
	}};
};

template <typename Scalar = double>
struct GaussLegendreConfig {
	Scalar tolerance = static_cast<Scalar>(1e-14);
	size_t max_iterations = 100;
	Scalar invariant_tolerance = static_cast<Scalar>(1e-12);
};

template <typename Scalar = double>
struct GaussLegendreStats {
	uint64_t total_steps = 0;
	uint64_t total_iterations = 0;
	uint64_t failed_steps = 0;
	Scalar max_invariant_drift = static_cast<Scalar>(0);
};

template <typename MetricType, size_t Stages, typename Scalar = double>
	requires Metrics::SpacetimeMetric<MetricType, Scalar>
class GaussLegendreIntegrator {
public:
	static constexpr size_t STAGES = Stages;
	static constexpr size_t ORDER = GaussLegendreTableau<Stages, Scalar>::ORDER;

private:
	using Tableau = GaussLegendreTableau<Stages, Scalar>;

	const MetricType& metric_;
	GaussLegendreConfig<Scalar> config_;
	mutable GaussLegendreStats<Scalar> stats_;

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
		return acc;
	}

public:
	explicit constexpr GaussLegendreIntegrator(
		const MetricType& metric,
		const GaussLegendreConfig<Scalar>& config = {}
	) noexcept
		: metric_(metric), config_(config), stats_{} {}

	[[nodiscard]] constexpr const GaussLegendreStats<Scalar>& statistics() const noexcept {
		return stats_;
	}

	constexpr void reset_statistics() noexcept {
		stats_ = GaussLegendreStats<Scalar>{};
	}

	[[nodiscard]] bool step(GeodesicState<Scalar>& state, Scalar dt) const noexcept {
		std::array<Core::FourVector<Scalar>, STAGES> k_x;
		std::array<Core::FourVector<Scalar>, STAGES> k_u;

		const auto init_acc = compute_acceleration(state.x, state.u);
		for (size_t i = 0; i < STAGES; ++i) {
			k_x[i] = state.u;
			k_u[i] = init_acc;
		}

		bool converged = false;
		size_t iter = 0;

		while (iter < config_.max_iterations && !converged) {
			++iter;
			Scalar max_residual = static_cast<Scalar>(0);

			std::array<Core::FourVector<Scalar>, STAGES> next_k_x;
			std::array<Core::FourVector<Scalar>, STAGES> next_k_u;

			for (size_t i = 0; i < STAGES; ++i) {
				Core::FourVector<Scalar> stage_x = state.x;
				Core::FourVector<Scalar> stage_u = state.u;

				for (size_t j = 0; j < STAGES; ++j) {
					const Scalar a_ij_dt = Tableau::A[i][j] * dt;
					for (size_t c = 0; c < 4; ++c) {
						stage_x(c) += a_ij_dt * k_x[j](c);
						stage_u(c) += a_ij_dt * k_u[j](c);
					}
				}

				next_k_x[i] = stage_u;
				next_k_u[i] = compute_acceleration(stage_x, stage_u);

				for (size_t c = 0; c < 4; ++c) {
					const Scalar res_x = std::abs(next_k_x[i](c) - k_x[i](c));
					const Scalar res_u = std::abs(next_k_u[i](c) - k_u[i](c));
					if (res_x > max_residual) max_residual = res_x;
					if (res_u > max_residual) max_residual = res_u;
				}
			}

			k_x = next_k_x;
			k_u = next_k_u;

			if (max_residual < config_.tolerance) {
				converged = true;
			}
		}

		stats_.total_iterations += iter;

		if (!converged) {
			++stats_.failed_steps;
			return false;
		}

		for (size_t i = 0; i < STAGES; ++i) {
			const Scalar b_i_dt = Tableau::B[i] * dt;
			for (size_t c = 0; c < 4; ++c) {
				state.x(c) += b_i_dt * k_x[i](c);
				state.u(c) += b_i_dt * k_u[i](c);
			}
		}

		++stats_.total_steps;
		return true;
	}
};

template <typename MetricType, typename Scalar = double>
using GaussLegendre4 = GaussLegendreIntegrator<MetricType, 2, Scalar>;

template <typename MetricType, typename Scalar = double>
using GaussLegendre6 = GaussLegendreIntegrator<MetricType, 3, Scalar>;

}

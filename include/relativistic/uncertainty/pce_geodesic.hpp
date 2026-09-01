#pragma once

#include "relativistic/uncertainty/polynomial_chaos.hpp"
#include "relativistic/uncertainty/variational_geodesic.hpp"
#include "relativistic/integrators/rk45_adaptive.hpp"
#include "relativistic/metrics/spacetime_concept.hpp"
#include <array>
#include <vector>
#include <span>
#include <cmath>
#include <thread>
#include <algorithm>

namespace Relativistic::Uncertainty {

template <size_t NumDims, size_t MaxDegree, typename Scalar = double>
struct PcePhaseState8D {
	std::array<PolynomialChaosExpansion<NumDims, MaxDegree>, 8> components;

	explicit PcePhaseState8D(const std::array<DistributionType, NumDims>& dists) {
		for (auto& comp : components) {
			comp = PolynomialChaosExpansion<NumDims, MaxDegree>(dists);
		}
	}

	[[nodiscard]] PhaseState8D<Scalar> evaluate(const std::array<double, NumDims>& xi) const noexcept {
		PhaseState8D<Scalar> state;
		for (size_t i = 0; i < 8; ++i) {
			state[i] = static_cast<Scalar>(components[i].evaluate(xi));
		}
		return state;
	}
};

template <typename MetricType, size_t NumDims, size_t MaxDegree, typename Scalar = double>
	requires Metrics::SpacetimeMetric<MetricType, Scalar>
class PceGeodesicPropagator {
private:
	const MetricType& metric_;
	Integrators::RK45Config<Scalar> rk_config_{};
	std::array<DistributionType, NumDims> distributions_{};

	void build_tensor_grid(
		size_t current_dim,
		const std::array<GaussQuadrature::QuadratureRule, NumDims>& rules,
		std::array<double, NumDims>& current_node,
		double current_weight,
		std::vector<std::array<double, NumDims>>& out_nodes,
		std::vector<double>& out_weights
	) const {
		if (current_dim == NumDims) {
			out_nodes.push_back(current_node);
			out_weights.push_back(current_weight);
			return;
		}
		for (size_t i = 0; i < rules[current_dim].nodes.size(); ++i) {
			current_node[current_dim] = rules[current_dim].nodes[i];
			build_tensor_grid(current_dim + 1, rules, current_node, current_weight * rules[current_dim].weights[i], out_nodes, out_weights);
		}
	}

public:
	explicit PceGeodesicPropagator(
		const MetricType& metric,
		const std::array<DistributionType, NumDims>& dists,
		const Integrators::RK45Config<Scalar>& config = {}
	) noexcept
		: metric_(metric), rk_config_(config), distributions_(dists) {}

	[[nodiscard]] PcePhaseState8D<NumDims, MaxDegree, Scalar> propagate(
		const PcePhaseState8D<NumDims, MaxDegree, Scalar>& initial_state,
		Scalar target_lambda,
		size_t quad_points_per_dim = MaxDegree + 2,
		size_t num_threads = 0
	) const {
		std::array<GaussQuadrature::QuadratureRule, NumDims> rules;
		for (size_t d = 0; d < NumDims; ++d) {
			rules[d] = (distributions_[d] == DistributionType::Gaussian)
				? GaussQuadrature::gauss_hermite(quad_points_per_dim)
				: GaussQuadrature::gauss_legendre(quad_points_per_dim);
		}

		std::vector<std::array<double, NumDims>> nodes;
		std::vector<double> weights;
		std::array<double, NumDims> current_node{};
		build_tensor_grid(0, rules, current_node, 1.0, nodes, weights);

		const size_t num_points = nodes.size();
		std::vector<PhaseState8D<Scalar>> final_states(num_points);

		const size_t threads_to_use = (num_threads > 0) ? num_threads : std::max(size_t{1}, static_cast<size_t>(std::thread::hardware_concurrency()));
		const size_t chunk_size = (num_points + threads_to_use - 1) / threads_to_use;
		std::vector<std::jthread> workers;

		auto worker_func = [&](size_t start_idx, size_t end_idx) {
			Integrators::RK45AdaptiveIntegrator<MetricType, Scalar> integrator(metric_, Integrators::GeodesicType::Timelike, rk_config_);
			for (size_t i = start_idx; i < end_idx; ++i) {
				auto state = initial_state.evaluate(nodes[i]);
				Integrators::GeodesicState<Scalar> rk_state{state.x, state.p};
				Scalar lambda = static_cast<Scalar>(0.0);
				Scalar dt = rk_config_.initial_step;

				while (lambda < target_lambda) {
					if (lambda + dt > target_lambda) dt = target_lambda - lambda;
					const auto dt_actual = integrator.step(rk_state, dt);
					if (!dt_actual.has_value()) break;
					lambda += *dt_actual;
				}
				final_states[i] = PhaseState8D<Scalar>(rk_state.x, rk_state.u);
			}
		};

		for (size_t t = 0; t < threads_to_use; ++t) {
			const size_t start_idx = t * chunk_size;
			const size_t end_idx = std::min(start_idx + chunk_size, num_points);
			if (start_idx < end_idx) {
				workers.emplace_back(worker_func, start_idx, end_idx);
			}
		}

		workers.clear();

		PcePhaseState8D<NumDims, MaxDegree, Scalar> result(distributions_);

		for (size_t i = 0; i < 8; ++i) {
			auto& pce = result.components[i];
			const size_t basis_size = pce.basis_size();
			std::vector<double> inner_prods(basis_size, 0.0);

			for (size_t q = 0; q < num_points; ++q) {
				const double w = weights[q];
				const double val = static_cast<double>(final_states[q][i]);
				for (size_t k = 0; k < basis_size; ++k) {
					const double psi_k = pce.evaluate_basis_function(k, nodes[q]);
					inner_prods[k] += w * val * psi_k;
				}
			}

			PolynomialChaosExpansion<NumDims, MaxDegree> empty_pce(distributions_);
			for (size_t k = 0; k < basis_size; ++k) {
				empty_pce.coefficients()[k] = 1.0;
				const double norm_sq = empty_pce.variance() + empty_pce.mean() * empty_pce.mean();
				pce.set_coefficient(k, inner_prods[k] / norm_sq);
				empty_pce.coefficients()[k] = 0.0;
			}
		}

		return result;
	}
};

}

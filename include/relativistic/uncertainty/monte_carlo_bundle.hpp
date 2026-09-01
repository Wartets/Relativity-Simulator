#pragma once

#include "relativistic/uncertainty/variational_geodesic.hpp"
#include "relativistic/integrators/rk45_adaptive.hpp"
#include "relativistic/metrics/spacetime_concept.hpp"
#include "relativistic/core/pcg64.hpp"
#include "relativistic/uncertainty/covariance.hpp"
#include "relativistic/uncertainty/uncertainty_types.hpp"
#include <vector>
#include <span>
#include <cmath>
#include <thread>
#include <algorithm>

namespace Relativistic::Uncertainty {

template <typename MetricType, typename Scalar = double>
	requires Metrics::SpacetimeMetric<MetricType, Scalar>
class MonteCarloGeodesicSampler {
public:
	[[nodiscard]] static std::vector<PhaseState8D<Scalar>> run_ensemble(
		const MetricType& metric,
		const PhaseState8D<Scalar>& nominal_state,
		const CovarianceMatrix<Scalar, 8>& initial_cov,
		Scalar target_lambda,
		size_t num_samples,
		const Integrators::RK45Config<Scalar>& rk_config = {},
		size_t num_threads = 0,
		uint64_t seed = 0x853C49E6748FEA9BULL
	) {
		std::vector<PhaseState8D<Scalar>> ensemble = MonteCarloCovarianceValidator<MetricType, Scalar>::generate_ensemble(
			nominal_state, initial_cov, num_samples, seed
		);

		const size_t threads_to_use = (num_threads > 0) ? num_threads : std::max(size_t{1}, static_cast<size_t>(std::thread::hardware_concurrency()));
		const size_t chunk_size = (num_samples + threads_to_use - 1) / threads_to_use;
		std::vector<std::jthread> workers;

		auto worker_func = [&](size_t start_idx, size_t end_idx) {
			Integrators::RK45AdaptiveIntegrator<MetricType, Scalar> integrator(metric, Integrators::GeodesicType::Timelike, rk_config);
			for (size_t i = start_idx; i < end_idx; ++i) {
				auto& pstate = ensemble[i];
				Integrators::GeodesicState<Scalar> rk_state{pstate.x, pstate.p};
				Scalar lambda = static_cast<Scalar>(0.0);
				Scalar dt = rk_config.initial_step;

				while (lambda < target_lambda) {
					if (lambda + dt > target_lambda) dt = target_lambda - lambda;
					const auto dt_actual = integrator.step(rk_state, dt);
					if (!dt_actual.has_value()) break;
					lambda += *dt_actual;
				}
				pstate.x = rk_state.x;
				pstate.p = rk_state.u;
			}
		};

		for (size_t t = 0; t < threads_to_use; ++t) {
			const size_t start_idx = t * chunk_size;
			const size_t end_idx = std::min(start_idx + chunk_size, num_samples);
			if (start_idx < end_idx) {
				workers.emplace_back(worker_func, start_idx, end_idx);
			}
		}

		workers.clear();
		return ensemble;
	}

	[[nodiscard]] static StatisticalMoments compute_component_moments(
		std::span<const PhaseState8D<Scalar>> ensemble,
		size_t component_idx
	) noexcept {
		StatisticalMoments m{};
		const size_t n = ensemble.size();
		if (n == 0) return m;

		double sum = 0.0;
		for (const auto& state : ensemble) {
			sum += static_cast<double>(state[component_idx]);
		}
		m.mean = sum / static_cast<double>(n);

		double sum_sq = 0.0;
		double sum_cub = 0.0;
		double sum_quad = 0.0;

		for (const auto& state : ensemble) {
			const double dev = static_cast<double>(state[component_idx]) - m.mean;
			const double dev2 = dev * dev;
			sum_sq += dev2;
			sum_cub += dev2 * dev;
			sum_quad += dev2 * dev2;
		}

		m.variance = sum_sq / static_cast<double>(n - 1);
		m.standard_deviation = std::sqrt(std::max(m.variance, 0.0));

		if (m.standard_deviation > 1e-15) {
			const double sigma3 = m.standard_deviation * m.standard_deviation * m.standard_deviation;
			const double sigma4 = sigma3 * m.standard_deviation;
			m.skewness = (sum_cub / static_cast<double>(n)) / sigma3;
			m.kurtosis = (sum_quad / static_cast<double>(n)) / sigma4;
		} else {
			m.skewness = 0.0;
			m.kurtosis = 3.0;
		}

		return m;
	}
};

}

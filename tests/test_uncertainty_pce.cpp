#include "relativistic/uncertainty/pce_geodesic.hpp"
#include "relativistic/uncertainty/monte_carlo_bundle.hpp"
#include "relativistic/metrics/schwarzschild.hpp"
#include <iostream>
#include <chrono>
#include <cmath>

using namespace Relativistic;

int main() {
	Metrics::SchwarzschildMetric<double> metric(1.0);
	Integrators::RK45Config<double> rk_config;
	rk_config.initial_step = -0.05;
	rk_config.min_step = 1e-8;

	std::array<Uncertainty::DistributionType, 1> dists = {Uncertainty::DistributionType::Gaussian};
	Uncertainty::PcePhaseState8D<1, 4, double> initial_pce(dists);

	const double p_phi_nominal = 0.3;
	const double p_phi_std = 0.01;
	
	for (size_t i = 0; i < 8; ++i) {
		initial_pce.components[i].coefficients()[0] = 0.0;
	}
	initial_pce.components[0].coefficients()[0] = 0.0;
	initial_pce.components[1].coefficients()[0] = 10.0;
	initial_pce.components[2].coefficients()[0] = std::numbers::pi / 2.0;
	initial_pce.components[3].coefficients()[0] = 0.0;
	
	initial_pce.components[4].coefficients()[0] = 1.0;
	initial_pce.components[5].coefficients()[0] = 0.0;
	initial_pce.components[6].coefficients()[0] = 0.0;
	initial_pce.components[7].coefficients()[0] = p_phi_nominal;
	initial_pce.components[7].coefficients()[1] = p_phi_std; 

	Uncertainty::PceGeodesicPropagator<decltype(metric), 1, 4, double> propagator(metric, dists, rk_config);

	auto start_pce = std::chrono::high_resolution_clock::now();
	auto final_pce = propagator.propagate(initial_pce, -10.0, 6, 0);
	auto end_pce = std::chrono::high_resolution_clock::now();
	double time_pce_ms = std::chrono::duration<double, std::milli>(end_pce - start_pce).count();

	Uncertainty::PhaseState8D<double> nominal_state(0.0, 10.0, std::numbers::pi / 2.0, 0.0, 1.0, 0.0, 0.0, p_phi_nominal);
	Uncertainty::CovarianceMatrix<double, 8> initial_cov;
	initial_cov(7, 7) = p_phi_std * p_phi_std;

	const size_t mc_samples = 10000000;
	auto start_mc = std::chrono::high_resolution_clock::now();
	auto ensemble = Uncertainty::MonteCarloGeodesicSampler<decltype(metric), double>::run_ensemble(
		metric, nominal_state, initial_cov, -10.0, mc_samples, rk_config, 0
	);
	auto end_mc = std::chrono::high_resolution_clock::now();
	double time_mc_ms = std::chrono::duration<double, std::milli>(end_mc - start_mc).count();

	const double ratio = time_mc_ms / time_pce_ms;

	std::cout << "PCE (Order 4) Execution Time: " << time_pce_ms << " ms\n";
	std::cout << "Monte-Carlo (10^7) Execution Time: " << time_mc_ms << " ms\n";
	std::cout << "Speedup Ratio (MC/PCE): " << ratio << "x\n\n";

	std::cout << "--- Statistical Moments Comparison (Final Azimuthal Position) ---\n";
	const auto pce_moments = final_pce.components[3].compute_moments(5000);
	const auto mc_moments = Uncertainty::MonteCarloGeodesicSampler<decltype(metric), double>::compute_component_moments(ensemble, 3);

	std::cout << "PCE Mean: " << pce_moments.mean << "\n";
	std::cout << "MC  Mean: " << mc_moments.mean << "\n";
	std::cout << "PCE Var:  " << pce_moments.variance << "\n";
	std::cout << "MC  Var:  " << mc_moments.variance << "\n";
	std::cout << "PCE Skew: " << pce_moments.skewness << "\n";
	std::cout << "MC  Skew: " << mc_moments.skewness << "\n";

	if (ratio > 400.0) {
		std::cout << "\nSUCCESS: Objective of 400x speedup met!\n";
		return 0;
	} else {
		std::cout << "\nFAILED: Objective of 400x speedup NOT met.\n";
		return 1;
	}
}

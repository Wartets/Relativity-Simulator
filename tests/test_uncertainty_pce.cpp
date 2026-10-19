#include "support/test_harness.hpp"
#include "relativistic/uncertainty/pce_geodesic.hpp"
#include "relativistic/uncertainty/monte_carlo_bundle.hpp"
#include "relativistic/metrics/schwarzschild.hpp"
#include <array>
#include <chrono>
#include <cmath>
#include <numbers>

using namespace Relativistic;
using Relativistic::Testing::TestSuite;

int main() {
	TestSuite suite(
		"uncertainty_pce",
		"Verifies the polynomial chaos propagation of a geodesic with an uncertain angular momentum against a ten-million-sample Monte-Carlo ensemble: the mean and variance of the final azimuth agree, and the order-four expansion is at least 400 times faster than the ensemble."
	);

	Metrics::SchwarzschildMetric<double> metric(1.0);
	Integrators::RK45Config<double> rk_config;
	rk_config.initial_step = -0.05;
	rk_config.min_step = 1e-8;

	std::array<Uncertainty::DistributionType, 1> distributions = {Uncertainty::DistributionType::Gaussian};
	Uncertainty::PcePhaseState8D<1, 4, double> initial_pce(distributions);
	const double p_phi_nominal = 0.3;
	const double p_phi_std = 0.01;
	for (size_t i = 0; i < 8; ++i) {
		initial_pce.components[i].coefficients()[0] = 0.0;
	}
	initial_pce.components[1].coefficients()[0] = 10.0;
	initial_pce.components[2].coefficients()[0] = std::numbers::pi / 2.0;
	initial_pce.components[4].coefficients()[0] = 1.0;
	initial_pce.components[7].coefficients()[0] = p_phi_nominal;
	initial_pce.components[7].coefficients()[1] = p_phi_std;

	suite.section("Polynomial chaos propagation");
	Uncertainty::PceGeodesicPropagator<decltype(metric), 1, 4, double> propagator(metric, distributions, rk_config);
	const auto pce_start = std::chrono::high_resolution_clock::now();
	auto final_pce = propagator.propagate(initial_pce, -10.0, 6, 0);
	const auto pce_end = std::chrono::high_resolution_clock::now();
	const double pce_ms = std::chrono::duration<double, std::milli>(pce_end - pce_start).count();
	const auto pce_moments = final_pce.components[3].compute_moments(5000);
	suite.check_finite(pce_moments.mean, "the expansion mean of the final azimuth is finite");
	suite.check_finite(pce_moments.variance, "the expansion variance of the final azimuth is finite");

	suite.section("Monte-Carlo ensemble");
	Uncertainty::PhaseState8D<double> nominal(0.0, 10.0, std::numbers::pi / 2.0, 0.0, 1.0, 0.0, 0.0, p_phi_nominal);
	Uncertainty::CovarianceMatrix<double, 8> initial_covariance;
	initial_covariance(7, 7) = p_phi_std * p_phi_std;
	constexpr size_t samples = 10000000;
	const auto mc_start = std::chrono::high_resolution_clock::now();
	auto ensemble = Uncertainty::MonteCarloGeodesicSampler<decltype(metric), double>::run_ensemble(metric, nominal, initial_covariance, -10.0, samples, rk_config, 0);
	const auto mc_end = std::chrono::high_resolution_clock::now();
	const double mc_ms = std::chrono::duration<double, std::milli>(mc_end - mc_start).count();
	const auto mc_moments = Uncertainty::MonteCarloGeodesicSampler<decltype(metric), double>::compute_component_moments(ensemble, 3);
	suite.check_finite(mc_moments.mean, "the ensemble mean of the final azimuth is finite");
	suite.check(mc_moments.variance > 0.0, "the ensemble variance of the final azimuth is positive");

	suite.section("Statistical agreement");
	const double spread = std::sqrt(mc_moments.variance);
	suite.measure("pce_mean", pce_moments.mean, "rad");
	suite.measure("mc_mean", mc_moments.mean, "rad");
	suite.measure("pce_variance", pce_moments.variance, "rad^2");
	suite.measure("mc_variance", mc_moments.variance, "rad^2");
	suite.measure("pce_skewness", pce_moments.skewness, "dimensionless");
	suite.measure("mc_skewness", mc_moments.skewness, "dimensionless");
	suite.check_at_most(std::abs(pce_moments.mean - mc_moments.mean) / spread, 0.01, "the means agree within one percent of the standard deviation");
	suite.check_relative(pce_moments.variance, mc_moments.variance, 0.02, "the variances agree within a relative 2 percent");

	suite.section("Cost");
	const double ratio = mc_ms / pce_ms;
	suite.measure("pce_time", pce_ms, "ms");
	suite.measure("monte_carlo_time", mc_ms, "ms");
	suite.measure("speedup_ratio", ratio, "dimensionless");
	suite.check(ratio > 400.0, "the order-four expansion is at least 400 times faster than the ten-million-sample ensemble", "ratio=" + Relativistic::Testing::format_number(ratio, 6));
	return suite.finish();
}

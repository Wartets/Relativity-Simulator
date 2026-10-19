#include "support/test_harness.hpp"
#include "relativistic/uncertainty/covariance.hpp"
#include "relativistic/uncertainty/variational_geodesic.hpp"
#include "relativistic/metrics/flat_minkowski.hpp"
#include "relativistic/metrics/schwarzschild.hpp"
#include "relativistic/metrics/kerr.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <string>

using namespace Relativistic::Uncertainty;
using namespace Relativistic::Metrics;
using namespace Relativistic::Core;
using Relativistic::Testing::TestSuite;

namespace {

void check_algebra(TestSuite& suite) {
	suite.section("Covariance algebra");
	CovarianceMatrix<double, 2> covariance;
	covariance(0, 0) = 4.0;
	covariance(1, 1) = 9.0;
	covariance(0, 1) = 1.0;
	covariance(1, 0) = 1.0;
	suite.check_near(covariance.standard_deviation(0), 2.0, 1e-12, "the first standard deviation is the root of the first variance");
	suite.check_near(covariance.standard_deviation(1), 3.0, 1e-12, "the second standard deviation is the root of the second variance");
	const auto system = covariance.compute_eigensystem();
	suite.check(system.eigenvalues[0] > 0.0 && system.eigenvalues[1] > 0.0, "the eigenvalues of a covariance matrix are positive");
	suite.check_near(system.eigenvalues[0] + system.eigenvalues[1], covariance.trace(), 1e-12, "the eigenvalues sum to the trace");
	const auto axes = covariance.principal_semi_axes();
	suite.check(axes[0] >= axes[1], "the principal semi-axes are sorted in decreasing order");
	suite.check_near(axes[0] * axes[0] + axes[1] * axes[1], covariance.trace(), 1e-12, "the squared semi-axes sum to the trace");
	std::array<std::array<double, 2>, 2> jacobian{{{0.0, 1.0}, {-1.0, 0.0}}};
	CovarianceMatrix<double, 2> noise;
	noise(0, 0) = 0.01;
	noise(1, 1) = 0.01;
	covariance.step_rk4(jacobian, noise, 0.01);
	suite.check(covariance(0, 0) > 0.0 && covariance(1, 1) > 0.0, "the propagated variances stay positive");
	suite.check(covariance.confidence_hypervolume(1.0) > 0.0, "the confidence hypervolume is positive");
}

void check_submatrices(TestSuite& suite) {
	suite.section("Submatrix extraction and sampling");
	CovarianceMatrix<double, 8> covariance;
	for (size_t i = 0; i < 8; ++i) {
		covariance(i, i) = static_cast<double>((i + 1) * (i + 1));
	}
	covariance(0, 1) = 0.5;
	covariance(1, 0) = 0.5;
	covariance(4, 5) = 0.8;
	covariance(5, 4) = 0.8;
	const auto position = covariance.extract_submatrix<4>(0, 0);
	suite.check_at_most(std::max({std::abs(position(0, 0) - 1.0), std::abs(position(1, 1) - 4.0), std::abs(position(0, 1) - 0.5)}), 1e-12, "the position block carries the expected variances and correlation");
	const auto spatial = covariance.extract_submatrix<3>(1, 1);
	suite.check_at_most(std::max({std::abs(spatial(0, 0) - 4.0), std::abs(spatial(1, 1) - 9.0), std::abs(spatial(2, 2) - 16.0)}), 1e-12, "the spatial block carries the expected variances");
	const auto momentum = covariance.extract_submatrix<4>(4, 4);
	suite.check_at_most(std::max({std::abs(momentum(0, 0) - 25.0), std::abs(momentum(1, 1) - 36.0), std::abs(momentum(0, 1) - 0.8)}), 1e-12, "the momentum block carries the expected variances and correlation");
	Relativistic::Core::PCG64Engine rng(42ULL, 1ULL);
	const std::array<double, 8> mean{1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0};
	suite.check(covariance.sample_multivariate_gaussian(rng, mean).size() == 8, "a multivariate Gaussian sample has eight components");
}

void check_jacobian(TestSuite& suite) {
	suite.section("Phase-space Jacobian");
	FlatMinkowskiMetric<double> metric(1.0);
	FourVector<double> x(0.0, 10.0, 0.0, 0.0);
	FourVector<double> p(1.0, 0.5, 0.0, 0.0);
	const auto j = GeodesicJacobianComputer<FlatMinkowskiMetric<double>, double>::compute_jacobian(metric, x, p);
	double worst = 0.0;
	for (size_t i = 0; i < 4; ++i) {
		for (size_t k = 0; k < 4; ++k) {
			worst = std::max({worst, std::abs(j[i][k]), std::abs(j[i][4 + k] - (i == k ? 1.0 : 0.0)), std::abs(j[4 + i][k]), std::abs(j[4 + i][4 + k])});
		}
	}
	suite.check_at_most(worst, 1e-15, "the flat-space Jacobian has the identity in the position-momentum block and zeros elsewhere");
}

void check_analytic_propagation(TestSuite& suite) {
	suite.section("Exact flat-space covariance");
	FlatMinkowskiMetric<double> metric(1.0);
	VariationalGeodesicIntegrator<FlatMinkowskiMetric<double>, double> integrator(metric);
	PhaseState8D<double> nominal(0.0, 10.0, 20.0, 30.0, 1.0, 0.1, -0.2, 0.3);
	CovarianceMatrix<double, 8> initial;
	initial(0, 0) = 0.04;
	initial(1, 1) = 0.09;
	initial(2, 2) = 0.16;
	initial(3, 3) = 0.25;
	initial(4, 4) = 0.0001;
	initial(5, 5) = 0.0004;
	initial(6, 6) = 0.0009;
	initial(7, 7) = 0.0016;
	VariationalGeodesicState<double> state(nominal, initial, 0.0);
	const double target = 10.0;
	integrator.integrate(state, target, 0.1);

	const std::array<double, 4> start{0.0, 10.0, 20.0, 30.0};
	const std::array<double, 4> velocity{1.0, 0.1, -0.2, 0.3};
	double position_error = 0.0;
	double covariance_error = 0.0;
	for (size_t mu = 0; mu < 4; ++mu) {
		position_error = std::max(position_error, std::abs(state.phase.x(mu) - (start[mu] + target * velocity[mu])));
		const double sigma_xx = initial(mu, mu) + target * target * initial(4 + mu, 4 + mu);
		const double sigma_xp = target * initial(4 + mu, 4 + mu);
		const double sigma_pp = initial(4 + mu, 4 + mu);
		covariance_error = std::max({covariance_error, std::abs(state.covariance(mu, mu) - sigma_xx), std::abs(state.covariance(mu, 4 + mu) - sigma_xp), std::abs(state.covariance(4 + mu, mu) - sigma_xp), std::abs(state.covariance(4 + mu, 4 + mu) - sigma_pp)});
	}
	suite.check_at_most(position_error, 1e-12, "the nominal worldline follows the straight-line solution");
	suite.check_at_most(covariance_error, 1e-10, "the propagated covariance equals the exact Sigma_xx = S0 + lambda^2 Sp, Sigma_xp = lambda Sp, Sigma_pp = Sp");
	suite.check_near(matrix_determinant_gaussian<8, double>(state.transition_matrix), 1.0, 1e-10, "the flat-space transition matrix has unit determinant");
}

template <typename MetricType>
void validate_against_monte_carlo(TestSuite& suite, const std::string& name, const MetricType& metric, const PhaseState8D<double>& nominal, const CovarianceMatrix<double, 8>& initial, const CovarianceMatrix<double, 8>& propagated, double target, double step, uint64_t seed) {
	using Validator = MonteCarloCovarianceValidator<MetricType, double>;
	auto ensemble = Validator::generate_ensemble(nominal, initial, 1000000, seed);
	Validator::propagate_ensemble_multithreaded(ensemble, metric, target, step);
	const auto empirical = Validator::compute_sample_covariance(ensemble);
	const auto comparison = Validator::compare_semi_axes(propagated, empirical, 0.001);
	suite.measure(name + " max_semi_axis_error", comparison.max_relative_error, "relative");
	suite.check(comparison.passed_tolerance, name + ": the semi-axes of the propagated covariance agree with the million-particle ensemble within the tolerance");
	suite.check_at_most(comparison.max_relative_error, 0.001, name + ": the largest relative semi-axis error is below 0.1 percent");
}

void check_minkowski_ensemble(TestSuite& suite) {
	suite.section("Minkowski Monte-Carlo validation");
	FlatMinkowskiMetric<double> metric(1.0);
	VariationalGeodesicIntegrator<FlatMinkowskiMetric<double>, double> integrator(metric);
	PhaseState8D<double> nominal(0.0, 100.0, 200.0, 300.0, 1.0, 0.2, -0.3, 0.1);
	CovarianceMatrix<double, 8> initial;
	initial(0, 0) = 1.0e-4;
	initial(1, 1) = 4.0e-4;
	initial(2, 2) = 2.25e-4;
	initial(3, 3) = 1.44e-4;
	initial(4, 4) = 0.64e-4;
	initial(5, 5) = 0.49e-4;
	initial(6, 6) = 0.36e-4;
	initial(7, 7) = 0.25e-4;
	initial(1, 5) = 1.0e-5;
	initial(5, 1) = 1.0e-5;
	VariationalGeodesicState<double> state(nominal, initial, 0.0);
	integrator.integrate(state, 2.0, 0.5);
	validate_against_monte_carlo(suite, "Minkowski", metric, nominal, initial, state.covariance, 2.0, 0.5, 0x123456789ABCDEF0ULL);
}

void check_schwarzschild_ensemble(TestSuite& suite) {
	suite.section("Schwarzschild Monte-Carlo validation");
	SchwarzschildMetric<double> metric(1.0, 1.0, 1.0);
	VariationalGeodesicIntegrator<SchwarzschildMetric<double>, double> integrator(metric);
	const double r0 = 10.0;
	const double u_t = 1.0 / std::sqrt(1.0 - 3.0 / r0);
	const double u_phi = (1.0 / std::sqrt(r0 * r0 * r0)) * u_t;
	PhaseState8D<double> nominal(0.0, r0, std::numbers::pi / 2.0, 0.0, u_t, 0.0, 0.0, u_phi);
	CovarianceMatrix<double, 8> initial;
	initial(0, 0) = 1.0e-6;
	initial(1, 1) = 4.0e-6;
	initial(2, 2) = 2.25e-6;
	initial(3, 3) = 1.44e-6;
	initial(4, 4) = 0.64e-6;
	initial(5, 5) = 0.49e-6;
	initial(6, 6) = 0.36e-6;
	initial(7, 7) = 0.25e-6;
	initial(1, 5) = 1.0e-7;
	initial(5, 1) = 1.0e-7;
	VariationalGeodesicState<double> state(nominal, initial, 0.0);
	integrator.integrate(state, 0.5, 0.05);
	suite.check_near(matrix_determinant_gaussian<8, double>(state.transition_matrix), 1.0, 1e-4, "the transition matrix of a Hamiltonian flow has unit determinant");
	validate_against_monte_carlo(suite, "Schwarzschild", metric, nominal, initial, state.covariance, 0.5, 0.05, 0xABCDEF0123456789ULL);
}

void check_kerr_ensemble(TestSuite& suite) {
	suite.section("Kerr Monte-Carlo validation");
	KerrMetric<double> metric(1.0, 0.9, 1.0, 1.0);
	VariationalGeodesicIntegrator<KerrMetric<double>, double> integrator(metric);
	PhaseState8D<double> nominal(0.0, 8.0, std::numbers::pi / 3.0, 0.0, 1.1, 0.002, 0.001, 0.04);
	CovarianceMatrix<double, 8> initial;
	initial(0, 0) = 1.0e-6;
	initial(1, 1) = 4.0e-6;
	initial(2, 2) = 2.25e-6;
	initial(3, 3) = 1.44e-6;
	initial(4, 4) = 0.64e-6;
	initial(5, 5) = 0.49e-6;
	initial(6, 6) = 0.36e-6;
	initial(7, 7) = 0.25e-6;
	initial(2, 6) = 1.0e-7;
	initial(6, 2) = 1.0e-7;
	VariationalGeodesicState<double> state(nominal, initial, 0.0);
	integrator.integrate(state, 0.5, 0.05);
	validate_against_monte_carlo(suite, "Kerr", metric, nominal, initial, state.covariance, 0.5, 0.05, 0xFEEDBEEFCAFE0001ULL);
}

void check_process_noise(TestSuite& suite) {
	suite.section("Process noise");
	FlatMinkowskiMetric<double> metric(1.0);
	VariationalGeodesicIntegrator<FlatMinkowskiMetric<double>, double> integrator(metric);
	PhaseState8D<double> nominal(0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0);
	CovarianceMatrix<double, 8> initial;
	initial.zero();
	CovarianceMatrix<double, 8> noise;
	for (size_t i = 0; i < 8; ++i) {
		noise(i, i) = 0.01;
	}
	VariationalGeodesicState<double> state(nominal, initial, 0.0);
	integrator.integrate(state, 1.0, 0.01, noise);
	bool positive = true;
	for (size_t i = 0; i < 8; ++i) {
		positive = positive && state.covariance(i, i) > 0.0;
	}
	suite.check(positive, "diffusion noise creates a positive variance in every phase-space direction");
}

}

int main() {
	TestSuite suite(
		"covariance_propagation",
		"Verifies the covariance machinery and the variational geodesic integrator: covariance algebra and eigensystem, block extraction and Gaussian sampling, the flat-space Jacobian, the exact analytic covariance of a free particle, million-particle Monte-Carlo validation in Minkowski, Schwarzschild and Kerr spacetimes within 0.1 percent, and process noise injection."
	);
	check_algebra(suite);
	check_submatrices(suite);
	check_jacobian(suite);
	check_analytic_propagation(suite);
	check_minkowski_ensemble(suite);
	check_schwarzschild_ensemble(suite);
	check_kerr_ensemble(suite);
	check_process_noise(suite);
	return suite.finish();
}

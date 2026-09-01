#include "relativistic/uncertainty/covariance.hpp"
#include "relativistic/uncertainty/variational_geodesic.hpp"
#include "relativistic/metrics/flat_minkowski.hpp"
#include "relativistic/metrics/schwarzschild.hpp"
#include "relativistic/metrics/kerr.hpp"
#include <iostream>
#include <cassert>
#include <cmath>
#include <vector>
#include <array>
#include <iomanip>

void test_basic_covariance_algebra() {
	using namespace Relativistic::Uncertainty;

	CovarianceMatrix<double, 2> cov;
	cov(0, 0) = 4.0;
	cov(1, 1) = 9.0;
	cov(0, 1) = 1.0;
	cov(1, 0) = 1.0;

	assert(std::abs(cov.standard_deviation(0) - 2.0) < 1e-12);
	assert(std::abs(cov.standard_deviation(1) - 3.0) < 1e-12);

	const auto es = cov.compute_eigensystem();
	assert(es.eigenvalues[0] > 0.0 && es.eigenvalues[1] > 0.0);
	assert(std::abs(es.eigenvalues[0] + es.eigenvalues[1] - cov.trace()) < 1e-12);

	const auto semi_axes = cov.principal_semi_axes();
	assert(semi_axes[0] >= semi_axes[1]);
	assert(std::abs(semi_axes[0] * semi_axes[0] + semi_axes[1] * semi_axes[1] - cov.trace()) < 1e-12);

	std::array<std::array<double, 2>, 2> jacobian{{{0.0, 1.0}, {-1.0, 0.0}}};
	CovarianceMatrix<double, 2> q;
	q(0, 0) = 0.01;
	q(1, 1) = 0.01;

	cov.step_rk4(jacobian, q, 0.01);
	assert(cov(0, 0) > 0.0 && cov(1, 1) > 0.0);

	const double vol = cov.confidence_hypervolume(1.0);
	assert(vol > 0.0);
}

void test_submatrix_extraction_and_sampling() {
	using namespace Relativistic::Uncertainty;

	CovarianceMatrix<double, 8> cov8;
	for (size_t i = 0; i < 8; ++i) {
		cov8(i, i) = static_cast<double>((i + 1) * (i + 1));
	}
	cov8(0, 1) = 0.5;
	cov8(1, 0) = 0.5;
	cov8(4, 5) = 0.8;
	cov8(5, 4) = 0.8;

	const auto pos_cov = cov8.extract_submatrix<4>(0, 0);
	assert(std::abs(pos_cov(0, 0) - 1.0) < 1e-12);
	assert(std::abs(pos_cov(1, 1) - 4.0) < 1e-12);
	assert(std::abs(pos_cov(0, 1) - 0.5) < 1e-12);

	const auto spatial_cov = cov8.extract_submatrix<3>(1, 1);
	assert(std::abs(spatial_cov(0, 0) - 4.0) < 1e-12);
	assert(std::abs(spatial_cov(1, 1) - 9.0) < 1e-12);
	assert(std::abs(spatial_cov(2, 2) - 16.0) < 1e-12);

	const auto mom_cov = cov8.extract_submatrix<4>(4, 4);
	assert(std::abs(mom_cov(0, 0) - 25.0) < 1e-12);
	assert(std::abs(mom_cov(1, 1) - 36.0) < 1e-12);
	assert(std::abs(mom_cov(0, 1) - 0.8) < 1e-12);

	Relativistic::Core::PCG64Engine rng(42ULL, 1ULL);
	std::array<double, 8> mean{1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0};
	const auto sample = cov8.sample_multivariate_gaussian(rng, mean);
	assert(sample.size() == 8);
}

void test_jacobian_minkowski_structure() {
	using namespace Relativistic::Uncertainty;
	using namespace Relativistic::Metrics;
	using namespace Relativistic::Core;

	FlatMinkowskiMetric<double> metric(1.0);
	FourVector<double> x(0.0, 10.0, 0.0, 0.0);
	FourVector<double> p(1.0, 0.5, 0.0, 0.0);

	const auto j = GeodesicJacobianComputer<FlatMinkowskiMetric<double>, double>::compute_jacobian(metric, x, p);

	for (size_t i = 0; i < 4; ++i) {
		for (size_t k = 0; k < 4; ++k) {
			assert(std::abs(j[i][k]) < 1e-15);
			assert(std::abs(j[i][4 + k] - (i == k ? 1.0 : 0.0)) < 1e-15);
			assert(std::abs(j[4 + i][k]) < 1e-15);
			assert(std::abs(j[4 + i][4 + k]) < 1e-15);
		}
	}
}

void test_minkowski_continuous_analytical_covariance() {
	using namespace Relativistic::Uncertainty;
	using namespace Relativistic::Metrics;
	using namespace Relativistic::Core;

	FlatMinkowskiMetric<double> metric(1.0);
	VariationalGeodesicIntegrator<FlatMinkowskiMetric<double>, double> integrator(metric);

	PhaseState8D<double> nominal_phase(0.0, 10.0, 20.0, 30.0, 1.0, 0.1, -0.2, 0.3);

	CovarianceMatrix<double, 8> init_cov;
	init_cov(0, 0) = 0.04;
	init_cov(1, 1) = 0.09;
	init_cov(2, 2) = 0.16;
	init_cov(3, 3) = 0.25;
	init_cov(4, 4) = 0.0001;
	init_cov(5, 5) = 0.0004;
	init_cov(6, 6) = 0.0009;
	init_cov(7, 7) = 0.0016;

	VariationalGeodesicState<double> state(nominal_phase, init_cov, 0.0);

	const double target_lambda = 10.0;
	const double step_size = 0.1;
	integrator.integrate(state, target_lambda, step_size);

	assert(std::abs(state.phase.x(0) - (0.0 + target_lambda * 1.0)) < 1e-12);
	assert(std::abs(state.phase.x(1) - (10.0 + target_lambda * 0.1)) < 1e-12);
	assert(std::abs(state.phase.x(2) - (20.0 + target_lambda * -0.2)) < 1e-12);
	assert(std::abs(state.phase.x(3) - (30.0 + target_lambda * 0.3)) < 1e-12);

	for (size_t mu = 0; mu < 4; ++mu) {
		const double expected_sigma_xx = init_cov(mu, mu) + (target_lambda * target_lambda) * init_cov(4 + mu, 4 + mu);
		const double expected_sigma_xp = target_lambda * init_cov(4 + mu, 4 + mu);
		const double expected_sigma_pp = init_cov(4 + mu, 4 + mu);

		assert(std::abs(state.covariance(mu, mu) - expected_sigma_xx) < 1e-10);
		assert(std::abs(state.covariance(mu, 4 + mu) - expected_sigma_xp) < 1e-10);
		assert(std::abs(state.covariance(4 + mu, mu) - expected_sigma_xp) < 1e-10);
		assert(std::abs(state.covariance(4 + mu, 4 + mu) - expected_sigma_pp) < 1e-10);
	}

	const double det_phi = matrix_determinant_gaussian<8, double>(state.transition_matrix);
	assert(std::abs(det_phi - 1.0) < 1e-10);
}

void test_minkowski_monte_carlo_1m_particles() {
	using namespace Relativistic::Uncertainty;
	using namespace Relativistic::Metrics;
	using namespace Relativistic::Core;

	FlatMinkowskiMetric<double> metric(1.0);
	VariationalGeodesicIntegrator<FlatMinkowskiMetric<double>, double> integrator(metric);

	PhaseState8D<double> nominal_phase(0.0, 100.0, 200.0, 300.0, 1.0, 0.2, -0.3, 0.1);

	CovarianceMatrix<double, 8> init_cov;
	init_cov(0, 0) = 1.0e-4;
	init_cov(1, 1) = 4.0e-4;
	init_cov(2, 2) = 2.25e-4;
	init_cov(3, 3) = 1.44e-4;
	init_cov(4, 4) = 0.64e-4;
	init_cov(5, 5) = 0.49e-4;
	init_cov(6, 6) = 0.36e-4;
	init_cov(7, 7) = 0.25e-4;
	init_cov(1, 5) = 1.0e-5;
	init_cov(5, 1) = 1.0e-5;

	VariationalGeodesicState<double> state(nominal_phase, init_cov, 0.0);

	const double target_lambda = 2.0;
	const double step_size = 0.5;
	integrator.integrate(state, target_lambda, step_size);

	const size_t particle_count = 1000000;
	auto ensemble = MonteCarloCovarianceValidator<FlatMinkowskiMetric<double>, double>::generate_ensemble(
		nominal_phase, init_cov, particle_count, 0x123456789ABCDEF0ULL
	);

	MonteCarloCovarianceValidator<FlatMinkowskiMetric<double>, double>::propagate_ensemble_multithreaded(
		ensemble, metric, target_lambda, step_size
	);

	const auto empirical_cov = MonteCarloCovarianceValidator<FlatMinkowskiMetric<double>, double>::compute_sample_covariance(ensemble);
	const auto comparison = MonteCarloCovarianceValidator<FlatMinkowskiMetric<double>, double>::compare_semi_axes(
		state.covariance, empirical_cov, 0.001
	);

	assert(comparison.passed_tolerance);
	assert(comparison.max_relative_error < 0.001);
}

void test_schwarzschild_variational_and_monte_carlo_1m() {
	using namespace Relativistic::Uncertainty;
	using namespace Relativistic::Metrics;
	using namespace Relativistic::Core;

	SchwarzschildMetric<double> metric(1.0, 1.0, 1.0);
	VariationalGeodesicIntegrator<SchwarzschildMetric<double>, double> integrator(metric);

	const double r0 = 10.0;
	const double theta0 = std::numbers::pi / 2.0;
	const double u_t = 1.0 / std::sqrt(1.0 - 3.0 / r0);
	const double u_phi = (1.0 / std::sqrt(r0 * r0 * r0)) * u_t;

	PhaseState8D<double> nominal_phase(0.0, r0, theta0, 0.0, u_t, 0.0, 0.0, u_phi);

	CovarianceMatrix<double, 8> init_cov;
	init_cov(0, 0) = 1.0e-6;
	init_cov(1, 1) = 4.0e-6;
	init_cov(2, 2) = 2.25e-6;
	init_cov(3, 3) = 1.44e-6;
	init_cov(4, 4) = 0.64e-6;
	init_cov(5, 5) = 0.49e-6;
	init_cov(6, 6) = 0.36e-6;
	init_cov(7, 7) = 0.25e-6;
	init_cov(1, 5) = 1.0e-7;
	init_cov(5, 1) = 1.0e-7;

	VariationalGeodesicState<double> state(nominal_phase, init_cov, 0.0);

	const double target_lambda = 0.5;
	const double step_size = 0.05;
	integrator.integrate(state, target_lambda, step_size);

	const double det_phi = matrix_determinant_gaussian<8, double>(state.transition_matrix);
	assert(std::abs(det_phi - 1.0) < 1e-4);

	const size_t particle_count = 1000000;
	auto ensemble = MonteCarloCovarianceValidator<SchwarzschildMetric<double>, double>::generate_ensemble(
		nominal_phase, init_cov, particle_count, 0xABCDEF0123456789ULL
	);

	MonteCarloCovarianceValidator<SchwarzschildMetric<double>, double>::propagate_ensemble_multithreaded(
		ensemble, metric, target_lambda, step_size
	);

	const auto empirical_cov = MonteCarloCovarianceValidator<SchwarzschildMetric<double>, double>::compute_sample_covariance(ensemble);
	const auto comparison = MonteCarloCovarianceValidator<SchwarzschildMetric<double>, double>::compare_semi_axes(
		state.covariance, empirical_cov, 0.001
	);

	assert(comparison.passed_tolerance);
	assert(comparison.max_relative_error < 0.001);
}

void test_kerr_covariance_monte_carlo_1m() {
	using namespace Relativistic::Uncertainty;
	using namespace Relativistic::Metrics;
	using namespace Relativistic::Core;

	KerrMetric<double> metric(1.0, 0.9, 1.0, 1.0);
	VariationalGeodesicIntegrator<KerrMetric<double>, double> integrator(metric);

	const double r0 = 8.0;
	const double theta0 = std::numbers::pi / 3.0;
	PhaseState8D<double> nominal_phase(0.0, r0, theta0, 0.0, 1.1, 0.002, 0.001, 0.04);

	CovarianceMatrix<double, 8> init_cov;
	init_cov(0, 0) = 1.0e-6;
	init_cov(1, 1) = 4.0e-6;
	init_cov(2, 2) = 2.25e-6;
	init_cov(3, 3) = 1.44e-6;
	init_cov(4, 4) = 0.64e-6;
	init_cov(5, 5) = 0.49e-6;
	init_cov(6, 6) = 0.36e-6;
	init_cov(7, 7) = 0.25e-6;
	init_cov(2, 6) = 1.0e-7;
	init_cov(6, 2) = 1.0e-7;

	VariationalGeodesicState<double> state(nominal_phase, init_cov, 0.0);

	const double target_lambda = 0.5;
	const double step_size = 0.05;
	integrator.integrate(state, target_lambda, step_size);

	const size_t particle_count = 1000000;
	auto ensemble = MonteCarloCovarianceValidator<KerrMetric<double>, double>::generate_ensemble(
		nominal_phase, init_cov, particle_count, 0xFEEDBEEFCAFE0001ULL
	);

	MonteCarloCovarianceValidator<KerrMetric<double>, double>::propagate_ensemble_multithreaded(
		ensemble, metric, target_lambda, step_size
	);

	const auto empirical_cov = MonteCarloCovarianceValidator<KerrMetric<double>, double>::compute_sample_covariance(ensemble);
	const auto comparison = MonteCarloCovarianceValidator<KerrMetric<double>, double>::compare_semi_axes(
		state.covariance, empirical_cov, 0.001
	);

	assert(comparison.passed_tolerance);
	assert(comparison.max_relative_error < 0.001);
}

void test_process_noise_injection() {
	using namespace Relativistic::Uncertainty;
	using namespace Relativistic::Metrics;
	using namespace Relativistic::Core;

	FlatMinkowskiMetric<double> metric(1.0);
	VariationalGeodesicIntegrator<FlatMinkowskiMetric<double>, double> integrator(metric);

	PhaseState8D<double> nominal_phase(0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0);
	CovarianceMatrix<double, 8> init_cov;
	init_cov.zero();

	CovarianceMatrix<double, 8> q_noise;
	for (size_t i = 0; i < 8; ++i) {
		q_noise(i, i) = 0.01;
	}

	VariationalGeodesicState<double> state(nominal_phase, init_cov, 0.0);

	const double target_lambda = 1.0;
	const double step_size = 0.01;
	integrator.integrate(state, target_lambda, step_size, q_noise);

	for (size_t i = 0; i < 8; ++i) {
		assert(state.covariance(i, i) > 0.0);
	}
}

int main() {
	std::cout << std::setprecision(10);
	std::cout << "Running Covariance and Variational Geodesic tests...\n";

	test_basic_covariance_algebra();
	std::cout << "PASS: Basic covariance algebra & eigensystem\n";

	test_submatrix_extraction_and_sampling();
	std::cout << "PASS: Submatrix extraction & multivariate Gaussian sampling\n";

	test_jacobian_minkowski_structure();
	std::cout << "PASS: 8D Phase-space Jacobian structure\n";

	test_minkowski_continuous_analytical_covariance();
	std::cout << "PASS: Continuous Lyapunov integration vs exact analytical solution\n";

	test_minkowski_monte_carlo_1m_particles();
	std::cout << "PASS: Minkowski 1,000,000 Monte-Carlo particles verification (< 0.1% error)\n";

	test_schwarzschild_variational_and_monte_carlo_1m();
	std::cout << "PASS: Schwarzschild orbit 1,000,000 Monte-Carlo particles verification (< 0.1% error)\n";

	test_kerr_covariance_monte_carlo_1m();
	std::cout << "PASS: Kerr frame-dragging 1,000,000 Monte-Carlo particles verification (< 0.1% error)\n";

	test_process_noise_injection();
	std::cout << "PASS: Process noise diffusion tensor injection\n";

	std::cout << "All covariance propagation and variational geodesic tests passed successfully!\n";
	return 0;
}

#include "relativistic/optics/spectral_shift.hpp"
#include "relativistic/metrics/schwarzschild.hpp"
#include "relativistic/metrics/flat_minkowski.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

void test_kinematic_doppler() {
	using namespace Relativistic::Optics;

	const double beta = 0.6;
	const double gamma = 1.0 / std::sqrt(1.0 - beta * beta);

	const double g_head_on = compute_kinematic_doppler(beta, 0.0, 0.0, 1.0, 0.0, 0.0);
	const double expected_head_on = std::sqrt((1.0 + beta) / (1.0 - beta));
	assert(std::abs(g_head_on - expected_head_on) < 1e-12);

	const double g_tail_on = compute_kinematic_doppler(beta, 0.0, 0.0, -1.0, 0.0, 0.0);
	const double expected_tail_on = std::sqrt((1.0 - beta) / (1.0 + beta));
	assert(std::abs(g_tail_on - expected_tail_on) < 1e-12);

	const double g_transverse = compute_kinematic_doppler(beta, 0.0, 0.0, 0.0, 1.0, 0.0);
	const double expected_transverse = 1.0 / gamma;
	assert(std::abs(g_transverse - expected_transverse) < 1e-12);
}

void test_gravitational_redshift_schwarzschild() {
	using namespace Relativistic::Optics;
	using namespace Relativistic::Metrics;

	SchwarzschildMetric metric(1.0);
	const double r_emit = 6.0;
	const double r_obs = 100.0;

	Relativistic::Core::FourVector<double> x_emit(0.0, r_emit, 1.57079632679, 0.0);
	Relativistic::Core::FourVector<double> x_obs(0.0, r_obs, 1.57079632679, 0.0);

	const auto g_tensor_emit = metric.metric_tensor(x_emit);
	const auto g_tensor_obs = metric.metric_tensor(x_obs);

	const double u0_emit = 1.0 / std::sqrt(-g_tensor_emit(0, 0));
	const double u0_obs = 1.0 / std::sqrt(-g_tensor_obs(0, 0));

	Relativistic::Core::FourVector<double> u_emit(u0_emit, 0.0, 0.0, 0.0);
	Relativistic::Core::FourVector<double> u_obs(u0_obs, 0.0, 0.0, 0.0);

	const double p0_emit = -1.0 / g_tensor_emit(0, 0);
	const double p0_obs = -1.0 / g_tensor_obs(0, 0);

	Relativistic::Core::FourVector<double> p_emit(p0_emit, 0.0, 0.0, 0.0);
	Relativistic::Core::FourVector<double> p_obs(p0_obs, 0.0, 0.0, 0.0);

	const double g_factor = compute_spectral_shift(p_obs, u_obs, g_tensor_obs, p_emit, u_emit, g_tensor_emit);
	const double expected_g = std::sqrt((1.0 - 2.0 / r_emit) / (1.0 - 2.0 / r_obs));

	assert(std::abs(g_factor - expected_g) < 1e-10);
}

void test_beaming_scaling() {
	using namespace Relativistic::Optics;

	const double i_0 = 100.0;
	const double g = 2.0;

	const double i_boost = apply_specific_intensity_beaming(i_0, g);
	assert(std::abs(i_boost - 800.0) < 1e-12);

	const double f_0 = 50.0;
	const double f_boost = apply_bolometric_flux_beaming(f_0, g);
	assert(std::abs(f_boost - 800.0) < 1e-12);

	const double lambda_0 = 500e-9;
	const double lambda_obs = observed_wavelength(lambda_0, g);
	assert(std::abs(lambda_obs - 250e-9) < 1e-18);
}

int main() {
	test_kinematic_doppler();
	test_gravitational_redshift_schwarzschild();
	test_beaming_scaling();
	std::cout << "Spectral shift and beaming validation passed.\n";
	return 0;
}

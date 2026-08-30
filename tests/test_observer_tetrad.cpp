#include "relativistic/core/tensor.hpp"
#include "relativistic/core/constants.hpp"
#include "relativistic/metrics/flat_minkowski.hpp"
#include "relativistic/metrics/schwarzschild.hpp"
#include "relativistic/metrics/kerr.hpp"
#include "relativistic/observer/observer_tetrad.hpp"
#include "relativistic/observer/tetrad_transport.hpp"
#include <iostream>
#include <cmath>
#include <cassert>
#include <numbers>

using namespace Relativistic::Core;
using namespace Relativistic::Metrics;
using namespace Relativistic::Observer;

void test_tetrad_orthonormality_schwarzschild() {
	SchwarzschildMetric<double> metric(1.0, 1.0, 1.0);
	FourVector<double> pos(0.0, 6.0, std::numbers::pi_v<double> / 2.0, 0.0);

	auto tetrad_stat = ObserverTetrad<double>::make_stationary(metric, pos);
	assert(tetrad_stat.check_orthonormality(metric, 1e-12));

	auto tetrad_zamo = ObserverTetrad<double>::make_zamo(metric, pos);
	assert(tetrad_zamo.check_orthonormality(metric, 1e-12));
}

void test_tetrad_orthonormality_kerr() {
	KerrMetric<double> metric(1.0, 0.9, 1.0, 1.0);
	FourVector<double> pos(0.0, 5.0, std::numbers::pi_v<double> / 3.0, 0.5);

	auto tetrad_zamo = ObserverTetrad<double>::make_zamo(metric, pos);
	assert(tetrad_zamo.check_orthonormality(metric, 1e-10));
}

void test_thomas_precession() {
	FlatMinkowskiMetric<double> metric(1.0);
	FermiWalkerTransport<FlatMinkowskiMetric<double>, double> transport(metric);

	const double v = 0.6;
	const double r = 10.0;
	const double gamma = 1.0 / std::sqrt(1.0 - v * v);
	const double omega = v / r;
	const double t_period = 2.0 * std::numbers::pi_v<double> / omega;
	const double tau_period = t_period / gamma;

	FourVector<double> pos(0.0, r, 0.0, 0.0);
	FourVector<double> vel(gamma, 0.0, gamma * v, 0.0);

	FourVector<double> e0(gamma, 0.0, gamma * v, 0.0);
	FourVector<double> e1(0.0, 1.0, 0.0, 0.0);
	FourVector<double> e2(gamma * v, 0.0, gamma, 0.0);
	FourVector<double> e3(0.0, 0.0, 0.0, 1.0);

	WorldlineObserverState<double> state(pos, vel, {e0, e1, e2, e3});
	transport.orthonormalize(state);

	const size_t num_steps = 4000;
	const double dtau = tau_period / static_cast<double>(num_steps);

	auto circular_acc = [omega](const WorldlineObserverState<double>& s) noexcept -> FourVector<double> {
		return FourVector<double>(0.0, -omega * s.four_velocity(0) * s.four_velocity(2), omega * s.four_velocity(0) * s.four_velocity(1), 0.0);
	};

	for (size_t step = 0; step < num_steps; ++step) {
		transport.step_rk4(state, dtau, circular_acc);
	}

	const double expected_thomas_angle = -2.0 * std::numbers::pi_v<double> * (gamma - 1.0);
	const double measured_angle = std::atan2(state.tetrad[1](2), state.tetrad[1](1));
	const double relative_error = std::abs(measured_angle - expected_thomas_angle) / std::abs(expected_thomas_angle);

	assert(relative_error < 1e-4);
	assert(transport.statistics().max_orthonormality_residual < 1e-10);
}

void test_gravity_probe_b_geodetic_precession() {
	const double mass_earth = 5.97219e24;
	const double c_light = 299792458.0;
	const double g_newton = 6.67430e-11;
	const double r_orbit = 7020137.0;

	SchwarzschildMetric<double> metric(mass_earth, c_light, g_newton);
	TransportConfig<double> config;
	config.auto_orthonormalize = false;
	FermiWalkerTransport<SchwarzschildMetric<double>, double> transport(metric, config);

	auto state = FermiWalkerTransport<SchwarzschildMetric<double>, double>::initialize_circular_geodesic_equatorial(metric, r_orbit);
	transport.orthonormalize(state);

	const double gm = g_newton * mass_earth;
	const double omega_orb = std::sqrt(gm / (r_orbit * r_orbit * r_orbit));
	const double orbital_period_sec = 2.0 * std::numbers::pi_v<double> / omega_orb;

	const size_t orbits = 2;
	const size_t steps_per_orbit = 10000;
	const double dtau = (orbital_period_sec / static_cast<double>(steps_per_orbit)) * std::sqrt(1.0 - (3.0 * gm) / (c_light * c_light * r_orbit));

	const auto initial_state = state;

	for (size_t i = 0; i < orbits * steps_per_orbit; ++i) {
		transport.step_rk4(state, dtau);
	}

	const double theo_rate_rad_s = FermiWalkerTransport<SchwarzschildMetric<double>, double>::theoretical_geodetic_precession_rate_rad_s(mass_earth, r_orbit, c_light, g_newton);
	const double seconds_per_year = 365.25 * 86400.0;
	const double rad_to_arcsec = (180.0 * 3600.0) / std::numbers::pi_v<double>;
	const double theo_annual_arcsec = theo_rate_rad_s * seconds_per_year * rad_to_arcsec;

	const double measured_angle_rad = FermiWalkerTransport<SchwarzschildMetric<double>, double>::compute_geodetic_precession_angle(initial_state, state, metric);
	const double measured_annual_arcsec = (measured_angle_rad / (state.coordinate_time - initial_state.coordinate_time)) * seconds_per_year * rad_to_arcsec;

	const double diff_arcsec = std::abs(measured_annual_arcsec - theo_annual_arcsec);

	assert(theo_annual_arcsec > 6.55 && theo_annual_arcsec < 6.65);
	assert(diff_arcsec < 1e-4);
	assert(transport.check_and_record_residuals(state) < 1e-10);
}

void test_lense_thirring_frame_dragging_kerr() {
	const double m = 1.0;
	const double a = 0.95;
	const double c = 1.0;
	const double g_const = 1.0;
	const double r0 = 15.0;

	KerrMetric<double> metric(m, a, c, g_const);
	TransportConfig<double> config;
	config.auto_orthonormalize = false;
	FermiWalkerTransport<KerrMetric<double>, double> transport(metric, config);

	FourVector<double> pos(0.0, r0, std::numbers::pi_v<double> / 2.0, 0.0);
	const auto g = metric.metric_tensor(pos);
	const double neg_g00 = -g(0, 0);
	const double u_t = 1.0 / std::sqrt(neg_g00);
	FourVector<double> vel(u_t, 0.0, 0.0, 0.0);

	FourVector<double> e0 = vel;
	FourVector<double> e1(0.0, 1.0 / std::sqrt(g(1, 1)), 0.0, 0.0);
	FourVector<double> e2(0.0, 0.0, 1.0 / std::sqrt(g(2, 2)), 0.0);
	FourVector<double> e3(0.0, 0.0, 0.0, 1.0 / std::sqrt(g(3, 3)));

	WorldlineObserverState<double> state(pos, vel, {e0, e1, e2, e3});
	transport.orthonormalize(state);

	const auto gamma = compute_christoffel<DerivativeOrder::EighthOrder, KerrMetric<double>, double>(metric, pos);
	const double a_r_hold = gamma(1, 0, 0) * u_t * u_t;

	const double dtau = 0.05;
	const size_t total_steps = 2000;

	for (size_t step = 0; step < total_steps; ++step) {
		FourVector<double> a_hold(0.0, a_r_hold, 0.0, 0.0);
		transport.step_rk4(state, dtau, a_hold);
	}

	const double g33_tilde = g(3, 3) - (g(0, 3) * g(0, 3)) / g(0, 0);
	const double expected_omega_stat = gamma(3, 0, 1) * std::sqrt(g33_tilde / g(1, 1));
	const auto g_final = metric.metric_tensor(state.position);
	const double final_g33_tilde = g_final(3, 3) - (g_final(0, 3) * g_final(0, 3)) / g_final(0, 0);
	const double v_r = state.tetrad[1](1) * std::sqrt(g_final(1, 1));
	const double v_phi = state.tetrad[1](3) * std::sqrt(final_g33_tilde);
	const double measured_angle = std::atan2(-v_phi, v_r);
	const double measured_omega_lt = measured_angle / state.coordinate_time;

	const double lt_relative_diff = std::abs(measured_omega_lt - expected_omega_stat) / expected_omega_stat;
	assert(lt_relative_diff < 1e-4);
	assert(transport.check_and_record_residuals(state) < 1e-9);
}

void test_fermi_walker_vs_parallel_geodesic_identity() {
	SchwarzschildMetric<double> metric(1.0, 1.0, 1.0);
	FermiWalkerTransport<SchwarzschildMetric<double>, double> transport(metric);

	auto state = FermiWalkerTransport<SchwarzschildMetric<double>, double>::initialize_circular_geodesic_equatorial(metric, 10.0);

	const auto deriv_pure_geodesic = transport.compute_derivatives(state, FourVector<double>{});
	FourVector<double> zero_acc{};
	const auto deriv_explicit_zero = transport.compute_derivatives(state, zero_acc);

	for (size_t mu = 0; mu < 4; ++mu) {
		assert(std::abs(deriv_pure_geodesic.d_velocity(mu) - deriv_explicit_zero.d_velocity(mu)) < 1e-15);
		for (size_t s = 0; s < 4; ++s) {
			assert(std::abs(deriv_pure_geodesic.d_tetrad[s](mu) - deriv_explicit_zero.d_tetrad[s](mu)) < 1e-15);
		}
	}
}

int main() {
	test_tetrad_orthonormality_schwarzschild();
	test_tetrad_orthonormality_kerr();
	test_thomas_precession();
	test_gravity_probe_b_geodetic_precession();
	test_lense_thirring_frame_dragging_kerr();
	test_fermi_walker_vs_parallel_geodesic_identity();

	std::cout << "All Tetrad and Fermi-Walker transport tests passed successfully.\n";
	return 0;
}

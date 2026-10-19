#include "support/test_harness.hpp"
#include "relativistic/core/math/tensor.hpp"
#include "relativistic/core/constants.hpp"
#include "relativistic/metrics/flat_minkowski.hpp"
#include "relativistic/metrics/schwarzschild.hpp"
#include "relativistic/metrics/kerr.hpp"
#include "relativistic/observer/observer_tetrad.hpp"
#include "relativistic/observer/tetrad_transport.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>

using namespace Relativistic::Core;
using namespace Relativistic::Metrics;
using namespace Relativistic::Observer;
using Relativistic::Testing::TestSuite;

namespace {

void check_orthonormality(TestSuite& suite) {
	suite.section("Tetrad orthonormality");
	SchwarzschildMetric<double> schwarzschild(1.0, 1.0, 1.0);
	const FourVector<double> equatorial(0.0, 6.0, std::numbers::pi_v<double> / 2.0, 0.0);
	suite.check(ObserverTetrad<double>::make_stationary(schwarzschild, equatorial).check_orthonormality(schwarzschild, 1e-12), "the stationary Schwarzschild tetrad is orthonormal within 1e-12");
	suite.check(ObserverTetrad<double>::make_zamo(schwarzschild, equatorial).check_orthonormality(schwarzschild, 1e-12), "the ZAMO Schwarzschild tetrad is orthonormal within 1e-12");
	KerrMetric<double> kerr(1.0, 0.9, 1.0, 1.0);
	const FourVector<double> inclined(0.0, 5.0, std::numbers::pi_v<double> / 3.0, 0.5);
	suite.check(ObserverTetrad<double>::make_zamo(kerr, inclined).check_orthonormality(kerr, 1e-10), "the ZAMO Kerr tetrad is orthonormal within 1e-10");
}

void check_thomas_precession(TestSuite& suite) {
	suite.section("Thomas precession");
	FlatMinkowskiMetric<double> metric(1.0);
	FermiWalkerTransport<FlatMinkowskiMetric<double>, double> transport(metric);
	const double v = 0.6;
	const double r = 10.0;
	const double gamma = 1.0 / std::sqrt(1.0 - v * v);
	const double omega = v / r;
	const double tau_period = (2.0 * std::numbers::pi_v<double> / omega) / gamma;

	FourVector<double> position(0.0, r, 0.0, 0.0);
	FourVector<double> velocity(gamma, 0.0, gamma * v, 0.0);
	FourVector<double> e0(gamma, 0.0, gamma * v, 0.0);
	FourVector<double> e1(0.0, 1.0, 0.0, 0.0);
	FourVector<double> e2(gamma * v, 0.0, gamma, 0.0);
	FourVector<double> e3(0.0, 0.0, 0.0, 1.0);
	WorldlineObserverState<double> state(position, velocity, {e0, e1, e2, e3});
	transport.orthonormalize(state);

	constexpr size_t steps = 4000;
	const double dtau = tau_period / static_cast<double>(steps);
	auto acceleration = [omega](const WorldlineObserverState<double>& s) noexcept -> FourVector<double> {
		return FourVector<double>(0.0, -omega * s.four_velocity(0) * s.four_velocity(2), omega * s.four_velocity(0) * s.four_velocity(1), 0.0);
	};
	for (size_t step = 0; step < steps; ++step) {
		transport.step_rk4(state, dtau, acceleration);
	}
	const double expected = -2.0 * std::numbers::pi_v<double> * (gamma - 1.0);
	const double measured = std::atan2(state.tetrad[1](2), state.tetrad[1](1));
	suite.measure("thomas_angle", measured, "rad");
	suite.check_relative(measured, expected, 1e-4, "the gyroscope rotation after one orbit equals -2 pi (gamma - 1) within a relative 1e-4");
	suite.check_at_most(transport.statistics().max_orthonormality_residual, 1e-10, "the transported tetrad stays orthonormal within 1e-10");
}

void check_geodetic_precession(TestSuite& suite) {
	suite.section("Gravity Probe B geodetic precession");
	const double mass_earth = 5.97219e24;
	const double c_light = 299792458.0;
	const double g_newton = 6.67430e-11;
	const double r_orbit = 7020137.0;
	using Metric = SchwarzschildMetric<double>;
	using Transport = FermiWalkerTransport<Metric, double>;

	Metric metric(mass_earth, c_light, g_newton);
	TransportConfig<double> config;
	config.auto_orthonormalize = false;
	Transport transport(metric, config);
	auto state = Transport::initialize_circular_geodesic_equatorial(metric, r_orbit);
	transport.orthonormalize(state);

	const double gm = g_newton * mass_earth;
	const double omega_orbit = std::sqrt(gm / (r_orbit * r_orbit * r_orbit));
	const double period = 2.0 * std::numbers::pi_v<double> / omega_orbit;
	constexpr size_t orbits = 2;
	constexpr size_t steps_per_orbit = 10000;
	const double dtau = (period / static_cast<double>(steps_per_orbit)) * std::sqrt(1.0 - (3.0 * gm) / (c_light * c_light * r_orbit));
	const auto initial = state;
	for (size_t i = 0; i < orbits * steps_per_orbit; ++i) {
		transport.step_rk4(state, dtau);
	}

	const double seconds_per_year = 365.25 * 86400.0;
	const double radians_to_arcseconds = (180.0 * 3600.0) / std::numbers::pi_v<double>;
	const double theory = Transport::theoretical_geodetic_precession_rate_rad_s(mass_earth, r_orbit, c_light, g_newton) * seconds_per_year * radians_to_arcseconds;
	const double angle = Transport::compute_geodetic_precession_angle(initial, state, metric);
	const double measured = (angle / (state.coordinate_time - initial.coordinate_time)) * seconds_per_year * radians_to_arcseconds;
	suite.measure("theoretical_annual_precession", theory, "arcsec/year");
	suite.measure("measured_annual_precession", measured, "arcsec/year");
	suite.check(theory > 6.55 && theory < 6.65, "the theoretical geodetic rate lies between 6.55 and 6.65 arcsec per year");
	suite.check_near(measured, theory, 1e-4, "the transported gyroscope reproduces the geodetic rate within 1e-4 arcsec per year");
	suite.check_at_most(transport.check_and_record_residuals(state), 1e-10, "the transported tetrad residual stays below 1e-10");
}

void check_lense_thirring(TestSuite& suite) {
	suite.section("Lense-Thirring frame dragging");
	const double r0 = 15.0;
	KerrMetric<double> metric(1.0, 0.95, 1.0, 1.0);
	TransportConfig<double> config;
	config.auto_orthonormalize = false;
	FermiWalkerTransport<KerrMetric<double>, double> transport(metric, config);

	FourVector<double> position(0.0, r0, std::numbers::pi_v<double> / 2.0, 0.0);
	const auto g = metric.metric_tensor(position);
	const double u_t = 1.0 / std::sqrt(-g(0, 0));
	FourVector<double> velocity(u_t, 0.0, 0.0, 0.0);
	FourVector<double> e0 = velocity;
	FourVector<double> e1(0.0, 1.0 / std::sqrt(g(1, 1)), 0.0, 0.0);
	FourVector<double> e2(0.0, 0.0, 1.0 / std::sqrt(g(2, 2)), 0.0);
	FourVector<double> e3(0.0, 0.0, 0.0, 1.0 / std::sqrt(g(3, 3)));
	WorldlineObserverState<double> state(position, velocity, {e0, e1, e2, e3});
	transport.orthonormalize(state);

	const auto gamma = compute_christoffel<DerivativeOrder::EighthOrder, KerrMetric<double>, double>(metric, position);
	const double hold = gamma(1, 0, 0) * u_t * u_t;
	for (size_t step = 0; step < 2000; ++step) {
		FourVector<double> holding(0.0, hold, 0.0, 0.0);
		transport.step_rk4(state, 0.05, holding);
	}

	const double g33_tilde = g(3, 3) - (g(0, 3) * g(0, 3)) / g(0, 0);
	const double expected = gamma(3, 0, 1) * std::sqrt(g33_tilde / g(1, 1));
	const auto g_final = metric.metric_tensor(state.position);
	const double final_tilde = g_final(3, 3) - (g_final(0, 3) * g_final(0, 3)) / g_final(0, 0);
	const double v_r = state.tetrad[1](1) * std::sqrt(g_final(1, 1));
	const double v_phi = state.tetrad[1](3) * std::sqrt(final_tilde);
	const double measured = std::atan2(-v_phi, v_r) / state.coordinate_time;
	suite.measure("expected_gyroscope_rate", expected, "rad per unit time");
	suite.measure("measured_gyroscope_rate", measured, "rad per unit time");
	suite.check_relative(measured, expected, 1e-4, "the gyroscope rate of a static observer matches the frame-dragging prediction within a relative 1e-4");
	suite.check_at_most(transport.check_and_record_residuals(state), 1e-9, "the transported tetrad residual stays below 1e-9");
}

void check_identity(TestSuite& suite) {
	suite.section("Fermi-Walker against parallel transport");
	SchwarzschildMetric<double> metric(1.0, 1.0, 1.0);
	FermiWalkerTransport<SchwarzschildMetric<double>, double> transport(metric);
	auto state = FermiWalkerTransport<SchwarzschildMetric<double>, double>::initialize_circular_geodesic_equatorial(metric, 10.0);
	const auto pure = transport.compute_derivatives(state, FourVector<double>{});
	FourVector<double> zero{};
	const auto explicit_zero = transport.compute_derivatives(state, zero);
	double worst = 0.0;
	for (size_t mu = 0; mu < 4; ++mu) {
		worst = std::max(worst, std::abs(pure.d_velocity(mu) - explicit_zero.d_velocity(mu)));
		for (size_t leg = 0; leg < 4; ++leg) {
			worst = std::max(worst, std::abs(pure.d_tetrad[leg](mu) - explicit_zero.d_tetrad[leg](mu)));
		}
	}
	suite.check_at_most(worst, 1e-15, "a vanishing acceleration reduces Fermi-Walker transport to parallel transport");
}

}

int main() {
	TestSuite suite(
		"observer_tetrad",
		"Verifies observer tetrads and their transport: orthonormality of stationary and ZAMO tetrads, the Thomas precession of a circularly moving gyroscope, the Gravity Probe B geodetic precession, the Lense-Thirring precession of a static observer in the Kerr field, and the reduction of Fermi-Walker to parallel transport for geodesic observers."
	);
	check_orthonormality(suite);
	check_thomas_precession(suite);
	check_geodetic_precession(suite);
	check_lense_thirring(suite);
	check_identity(suite);
	return suite.finish();
}

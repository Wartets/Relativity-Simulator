#include "support/test_harness.hpp"
#include "relativistic/optics/spectral_shift.hpp"
#include "relativistic/metrics/schwarzschild.hpp"
#include "relativistic/metrics/flat_minkowski.hpp"
#include <cmath>

using namespace Relativistic::Optics;
using namespace Relativistic::Metrics;
using Relativistic::Testing::TestSuite;

int main() {
	TestSuite suite(
		"spectral_shift_beaming",
		"Verifies the spectral shift and beaming relations: the longitudinal and transverse kinematic Doppler factors, the Schwarzschild gravitational redshift between a static emitter and a static observer, the g^3 and g^4 intensity and flux scalings, and the wavelength transformation."
	);

	suite.section("Kinematic Doppler factor");
	constexpr double beta = 0.6;
	const double gamma = 1.0 / std::sqrt(1.0 - beta * beta);
	suite.check_near(compute_kinematic_doppler(beta, 0.0, 0.0, 1.0, 0.0, 0.0), std::sqrt((1.0 + beta) / (1.0 - beta)), 1e-12, "a head-on source is blueshifted by sqrt((1 + beta) / (1 - beta))");
	suite.check_near(compute_kinematic_doppler(beta, 0.0, 0.0, -1.0, 0.0, 0.0), std::sqrt((1.0 - beta) / (1.0 + beta)), 1e-12, "a receding source is redshifted by sqrt((1 - beta) / (1 + beta))");
	suite.check_near(compute_kinematic_doppler(beta, 0.0, 0.0, 0.0, 1.0, 0.0), 1.0 / gamma, 1e-12, "a transverse source shows pure time dilation 1 / gamma");

	suite.section("Gravitational redshift in the Schwarzschild metric");
	SchwarzschildMetric metric(1.0);
	constexpr double emitter_radius = 6.0;
	constexpr double observer_radius = 100.0;
	constexpr double equator = 1.57079632679;
	Relativistic::Core::FourVector<double> emitter_position(0.0, emitter_radius, equator, 0.0);
	Relativistic::Core::FourVector<double> observer_position(0.0, observer_radius, equator, 0.0);
	const auto emitter_metric = metric.metric_tensor(emitter_position);
	const auto observer_metric = metric.metric_tensor(observer_position);
	const double emitter_u0 = 1.0 / std::sqrt(-emitter_metric(0, 0));
	const double observer_u0 = 1.0 / std::sqrt(-observer_metric(0, 0));
	Relativistic::Core::FourVector<double> emitter_velocity(emitter_u0, 0.0, 0.0, 0.0);
	Relativistic::Core::FourVector<double> observer_velocity(observer_u0, 0.0, 0.0, 0.0);
	Relativistic::Core::FourVector<double> emitter_momentum(-1.0 / emitter_metric(0, 0), 0.0, 0.0, 0.0);
	Relativistic::Core::FourVector<double> observer_momentum(-1.0 / observer_metric(0, 0), 0.0, 0.0, 0.0);
	const double shift = compute_spectral_shift(observer_momentum, observer_velocity, observer_metric, emitter_momentum, emitter_velocity, emitter_metric);
	const double expected = std::sqrt((1.0 - 2.0 / emitter_radius) / (1.0 - 2.0 / observer_radius));
	suite.check_near(shift, expected, 1e-10, "the frequency ratio equals sqrt(f(r_emit) / f(r_obs))");

	suite.section("Beaming and wavelength scaling");
	suite.check_near(apply_specific_intensity_beaming(100.0, 2.0), 800.0, 1e-12, "the specific intensity scales as g^3");
	suite.check_near(apply_bolometric_flux_beaming(50.0, 2.0), 800.0, 1e-12, "the bolometric flux scales as g^4 and equals 800 for g = 2");
	suite.check_relative(observed_wavelength(500e-9, 2.0), 250e-9, 1e-12, "the observed wavelength equals the emitted wavelength divided by g");
	return suite.finish();
}

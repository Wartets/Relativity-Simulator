#include "support/test_harness.hpp"
#include "relativistic/observer/rocket_dynamics.hpp"
#include "relativistic/metrics/flat_minkowski.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <vector>

using namespace Relativistic;
using Testing::TestSuite;

namespace {

using Rocket = Observer::RelativisticRocket<Metrics::FlatMinkowskiMetric<double>, double>;

struct WorstErrors {
	double proper_time{0.0};
	double velocity{0.0};
	double position{0.0};
	double gamma{0.0};
};

}

int main() {
	TestSuite suite(
		"hyperbolic_motion",
		"Verifies the relativistic rocket against the closed-form hyperbolic motion of constant proper acceleration in flat spacetime: proper time, velocity, position and Lorentz factor over a grid of accelerations and coordinate times."
	);

	suite.section("Constant proper acceleration sweep");
	Metrics::FlatMinkowskiMetric<double> metric(1.0);
	const std::vector<double> accelerations = {0.1, 1.0, 9.81, 50.0, 100.0};
	const std::vector<double> end_times = {0.1, 1.0, 5.0, 20.0};
	constexpr double step = 0.0005;
	WorstErrors worst;

	for (const double acceleration : accelerations) {
		for (const double end_time : end_times) {
			Core::FourVector<double> position(0.0, 0.0, 0.0, 0.0);
			Core::FourVector<double> velocity(1.0, 0.0, 0.0, 0.0);
			std::array<Core::FourVector<double>, 4> tetrad = {
				Core::FourVector<double>(1.0, 0.0, 0.0, 0.0),
				Core::FourVector<double>(0.0, 1.0, 0.0, 0.0),
				Core::FourVector<double>(0.0, 0.0, 1.0, 0.0),
				Core::FourVector<double>(0.0, 0.0, 0.0, 1.0)
			};
			Observer::WorldlineObserverState<double> initial(position, velocity, tetrad, 0.0);
			Rocket rocket(metric, initial, Observer::TimeFlowMode::CoordinateTime);
			rocket.set_thrust(acceleration, 0.0, 0.0);
			const size_t steps = static_cast<size_t>(std::ceil(end_time / step));
			for (size_t index = 0; index < steps; ++index) {
				rocket.step(step);
			}
			const double t = rocket.state().coordinate_time;
			const double tau = rocket.state().proper_time;
			const double x = rocket.state().position(1);
			const double v = rocket.state().four_velocity(1) / rocket.state().four_velocity(0);
			const double gamma = rocket.state().four_velocity(0);
			const double exact_tau = Rocket::hyperbolic_proper_time(acceleration, t, 1.0);
			const double exact_v = Rocket::hyperbolic_velocity(acceleration, t, 1.0);
			const double exact_x = Rocket::hyperbolic_position(acceleration, t, 1.0);
			const double exact_gamma = Rocket::hyperbolic_gamma(acceleration, t, 1.0);
			worst.proper_time = std::max(worst.proper_time, std::abs(tau - exact_tau) / exact_tau);
			worst.velocity = std::max(worst.velocity, std::abs(v - exact_v) / exact_v);
			worst.position = std::max(worst.position, std::abs(x - exact_x) / exact_x);
			worst.gamma = std::max(worst.gamma, std::abs(gamma - exact_gamma) / exact_gamma);
		}
	}

	suite.measure("worst_relative_proper_time_error", worst.proper_time, "dimensionless");
	suite.measure("worst_relative_velocity_error", worst.velocity, "dimensionless");
	suite.measure("worst_relative_position_error", worst.position, "dimensionless");
	suite.measure("worst_relative_gamma_error", worst.gamma, "dimensionless");
	suite.check_at_most(worst.proper_time, 1e-7, "the proper time matches the hyperbolic solution within 1e-7 over the whole grid");
	suite.check_at_most(worst.velocity, 1e-7, "the velocity matches the hyperbolic solution within 1e-7 over the whole grid");
	suite.check_at_most(worst.position, 1e-6, "the position matches the hyperbolic solution within 1e-6 over the whole grid");
	suite.check_at_most(worst.gamma, 1e-7, "the Lorentz factor matches the hyperbolic solution within 1e-7 over the whole grid");
	return suite.finish();
}

#include "support/test_harness.hpp"
#include "relativistic/observer/rocket_dynamics.hpp"
#include "relativistic/metrics/flat_minkowski.hpp"
#include <array>
#include <cmath>
#include <numbers>

using namespace Relativistic;
using Testing::TestSuite;

namespace {

using Rocket = Observer::RelativisticRocket<Metrics::FlatMinkowskiMetric<double>, double>;

[[nodiscard]] Observer::WorldlineObserverState<double> rest_state() {
	const std::array<Core::FourVector<double>, 4> tetrad = {
		Core::FourVector<double>(1.0, 0.0, 0.0, 0.0),
		Core::FourVector<double>(0.0, 1.0, 0.0, 0.0),
		Core::FourVector<double>(0.0, 0.0, 1.0, 0.0),
		Core::FourVector<double>(0.0, 0.0, 0.0, 1.0)
	};
	return Observer::WorldlineObserverState<double>(Core::FourVector<double>(0.0, 0.0, 0.0, 0.0), Core::FourVector<double>(1.0, 0.0, 0.0, 0.0), tetrad, 0.0);
}

}

int main() {
	TestSuite suite(
		"rocket_dynamics",
		"Verifies the relativistic rocket in flat spacetime: constant proper acceleration against the closed-form hyperbolic solution in proper-time flow, the coordinate-time flow mode, and an exact quarter-turn rotation that preserves the orthonormality of the tetrad."
	);
	Metrics::FlatMinkowskiMetric<double> metric(1.0);

	suite.section("Proper-time flow");
	Rocket proper(metric, rest_state(), Observer::TimeFlowMode::ProperTime);
	proper.set_thrust(2.0, 0.0, 0.0);
	for (size_t step = 0; step < 10000; ++step) {
		proper.step(1e-4);
	}
	const auto& state = proper.state();
	const double t = state.coordinate_time;
	suite.check_near(t, std::sinh(2.0 * state.proper_time) / 2.0, 1e-7, "the coordinate time equals sinh(a tau) / a");
	suite.check_near(state.proper_time, Rocket::hyperbolic_proper_time(2.0, t, 1.0), 1e-9, "the proper time matches the hyperbolic solution");
	suite.check_near(state.four_velocity(1) / state.four_velocity(0), Rocket::hyperbolic_velocity(2.0, t, 1.0), 1e-9, "the velocity matches the hyperbolic solution");
	suite.check_near(state.position(1), Rocket::hyperbolic_position(2.0, t, 1.0), 1e-7, "the position matches the hyperbolic solution");
	suite.check_near(state.four_velocity(0), Rocket::hyperbolic_gamma(2.0, t, 1.0), 1e-8, "the Lorentz factor matches the hyperbolic solution");

	suite.section("Coordinate-time flow");
	Rocket coordinate(metric, rest_state(), Observer::TimeFlowMode::CoordinateTime);
	coordinate.set_thrust(1.5, 0.0, 0.0);
	for (size_t step = 0; step < 2000; ++step) {
		coordinate.step(0.001);
	}
	const double elapsed = coordinate.state().coordinate_time;
	suite.check_near(elapsed, 2.0, 1e-4, "two thousand steps of 0.001 advance the coordinate time by 2");
	suite.check_near(coordinate.state().four_velocity(1) / coordinate.state().four_velocity(0), Rocket::hyperbolic_velocity(1.5, elapsed, 1.0), 1e-6, "the velocity matches the hyperbolic solution in coordinate-time flow");

	suite.section("Spatial rotation");
	Rocket rotating(metric, rest_state(), Observer::TimeFlowMode::ProperTime);
	rotating.set_angular_rates(0.0, std::numbers::pi / 2.0, 0.0);
	rotating.step(1.0);
	suite.check(rotating.state().to_observer_tetrad().check_orthonormality(metric, 1e-12), "the rotated tetrad remains orthonormal");
	suite.check_near(rotating.state().tetrad[1](1), 0.0, 1e-12, "a quarter turn removes the x component of the first spatial leg");
	suite.check_near(std::abs(rotating.state().tetrad[1](3)), 1.0, 1e-12, "a quarter turn aligns the first spatial leg with the z axis");
	return suite.finish();
}

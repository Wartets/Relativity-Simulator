#include "relativistic/observer/rocket_dynamics.hpp"
#include "relativistic/metrics/flat_minkowski.hpp"
#include "relativistic/metrics/schwarzschild.hpp"
#include <cassert>
#include <iostream>
#include <cmath>

void test_hyperbolic_motion_precision() {
	using namespace Relativistic;
	Metrics::FlatMinkowskiMetric<double> metric(1.0);

	Core::FourVector<double> p0(0.0, 0.0, 0.0, 0.0);
	Core::FourVector<double> u0(1.0, 0.0, 0.0, 0.0);
	std::array<Core::FourVector<double>, 4> e0 = {
		Core::FourVector<double>(1.0, 0.0, 0.0, 0.0),
		Core::FourVector<double>(0.0, 1.0, 0.0, 0.0),
		Core::FourVector<double>(0.0, 0.0, 1.0, 0.0),
		Core::FourVector<double>(0.0, 0.0, 0.0, 1.0)
	};

	Observer::WorldlineObserverState<double> initial_state(p0, u0, e0, 0.0);
	Observer::RelativisticRocket<Metrics::FlatMinkowskiMetric<double>, double> rocket(
		metric, initial_state, Observer::TimeFlowMode::ProperTime
	);

	const double proper_accel = 2.0;
	rocket.set_thrust(proper_accel, 0.0, 0.0);

	const double dtau = 1e-4;
	const size_t total_steps = 10000;

	for (size_t step = 0; step < total_steps; ++step) {
		rocket.step(dtau);
	}

	const double num_tau = rocket.state().proper_time;
	const double num_t = rocket.state().coordinate_time;
	const double num_x = rocket.state().position(1);
	const double num_vx = rocket.state().four_velocity(1) / rocket.state().four_velocity(0);
	const double num_gamma = rocket.state().four_velocity(0);

	const double exact_t = std::sinh(proper_accel * num_tau) / proper_accel;
	const double exact_tau = Observer::RelativisticRocket<Metrics::FlatMinkowskiMetric<double>, double>::hyperbolic_proper_time(proper_accel, num_t, 1.0);
	const double exact_v = Observer::RelativisticRocket<Metrics::FlatMinkowskiMetric<double>, double>::hyperbolic_velocity(proper_accel, num_t, 1.0);
	const double exact_x = Observer::RelativisticRocket<Metrics::FlatMinkowskiMetric<double>, double>::hyperbolic_position(proper_accel, num_t, 1.0);
	const double exact_gamma = Observer::RelativisticRocket<Metrics::FlatMinkowskiMetric<double>, double>::hyperbolic_gamma(proper_accel, num_t, 1.0);

	const double err_t = std::abs(num_t - exact_t);
	const double err_tau = std::abs(num_tau - exact_tau);
	const double err_v = std::abs(num_vx - exact_v);
	const double err_x = std::abs(num_x - exact_x);
	const double err_gamma = std::abs(num_gamma - exact_gamma);

	assert(err_t < 1e-7);
	assert(err_tau < 1e-9);
	assert(err_v < 1e-9);
	assert(err_x < 1e-7);
	assert(err_gamma < 1e-8);
}

void test_coordinate_time_flow_mode() {
	using namespace Relativistic;
	Metrics::FlatMinkowskiMetric<double> metric(1.0);

	Core::FourVector<double> p0(0.0, 0.0, 0.0, 0.0);
	Core::FourVector<double> u0(1.0, 0.0, 0.0, 0.0);
	std::array<Core::FourVector<double>, 4> e0 = {
		Core::FourVector<double>(1.0, 0.0, 0.0, 0.0),
		Core::FourVector<double>(0.0, 1.0, 0.0, 0.0),
		Core::FourVector<double>(0.0, 0.0, 1.0, 0.0),
		Core::FourVector<double>(0.0, 0.0, 0.0, 1.0)
	};

	Observer::WorldlineObserverState<double> initial_state(p0, u0, e0, 0.0);
	Observer::RelativisticRocket<Metrics::FlatMinkowskiMetric<double>, double> rocket(
		metric, initial_state, Observer::TimeFlowMode::CoordinateTime
	);

	const double proper_accel = 1.5;
	rocket.set_thrust(proper_accel, 0.0, 0.0);

	const double dt = 0.001;
	const size_t total_steps = 2000;

	for (size_t step = 0; step < total_steps; ++step) {
		rocket.step(dt);
	}

	const double target_coord_time = dt * static_cast<double>(total_steps);
	const double actual_coord_time = rocket.state().coordinate_time;

	assert(std::abs(actual_coord_time - target_coord_time) < 1e-4);

	const double exact_v = Observer::RelativisticRocket<Metrics::FlatMinkowskiMetric<double>, double>::hyperbolic_velocity(proper_accel, actual_coord_time, 1.0);
	const double num_v = rocket.state().four_velocity(1) / rocket.state().four_velocity(0);

	assert(std::abs(num_v - exact_v) < 1e-6);
}

void test_spatial_rotation_control() {
	using namespace Relativistic;
	Metrics::FlatMinkowskiMetric<double> metric(1.0);

	Core::FourVector<double> p0(0.0, 0.0, 0.0, 0.0);
	Core::FourVector<double> u0(1.0, 0.0, 0.0, 0.0);
	std::array<Core::FourVector<double>, 4> e0 = {
		Core::FourVector<double>(1.0, 0.0, 0.0, 0.0),
		Core::FourVector<double>(0.0, 1.0, 0.0, 0.0),
		Core::FourVector<double>(0.0, 0.0, 1.0, 0.0),
		Core::FourVector<double>(0.0, 0.0, 0.0, 1.0)
	};

	Observer::WorldlineObserverState<double> initial_state(p0, u0, e0, 0.0);
	Observer::RelativisticRocket<Metrics::FlatMinkowskiMetric<double>, double> rocket(
		metric, initial_state, Observer::TimeFlowMode::ProperTime
	);

	const double omega_yaw = std::numbers::pi / 2.0;
	rocket.set_angular_rates(0.0, omega_yaw, 0.0);

	rocket.step(1.0);

	const auto tetrad = rocket.state().to_observer_tetrad();
	assert(tetrad.check_orthonormality(metric, 1e-12));

	const double e1_x = rocket.state().tetrad[1](1);
	const double e1_z = rocket.state().tetrad[1](3);

	assert(std::abs(e1_x) < 1e-12);
	assert(std::abs(std::abs(e1_z) - 1.0) < 1e-12);
}

int main() {
	test_hyperbolic_motion_precision();
	test_coordinate_time_flow_mode();
	test_spatial_rotation_control();
	std::cout << "All rocket dynamics tests passed successfully.\n";
	return 0;
}

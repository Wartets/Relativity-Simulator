#include "relativistic/observer/rocket_dynamics.hpp"
#include "relativistic/metrics/flat_minkowski.hpp"
#include <cassert>
#include <iostream>
#include <cmath>
#include <vector>

int main() {
	using namespace Relativistic;
	Metrics::FlatMinkowskiMetric<double> metric(1.0);

	const std::vector<double> accelerations = {0.1, 1.0, 9.81, 50.0, 100.0};
	const std::vector<double> target_times = {0.1, 1.0, 5.0, 20.0};

	for (double a : accelerations) {
		for (double t_end : target_times) {
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

			rocket.set_thrust(a, 0.0, 0.0);

			const double dt = 0.0005;
			const size_t steps = static_cast<size_t>(std::ceil(t_end / dt));

			for (size_t i = 0; i < steps; ++i) {
				rocket.step(dt);
			}

			const double num_t = rocket.state().coordinate_time;
			const double num_tau = rocket.state().proper_time;
			const double num_x = rocket.state().position(1);
			const double num_vx = rocket.state().four_velocity(1) / rocket.state().four_velocity(0);
			const double num_gamma = rocket.state().four_velocity(0);

			const double exact_tau = Observer::RelativisticRocket<Metrics::FlatMinkowskiMetric<double>, double>::hyperbolic_proper_time(a, num_t, 1.0);
			const double exact_vx = Observer::RelativisticRocket<Metrics::FlatMinkowskiMetric<double>, double>::hyperbolic_velocity(a, num_t, 1.0);
			const double exact_x = Observer::RelativisticRocket<Metrics::FlatMinkowskiMetric<double>, double>::hyperbolic_position(a, num_t, 1.0);
			const double exact_gamma = Observer::RelativisticRocket<Metrics::FlatMinkowskiMetric<double>, double>::hyperbolic_gamma(a, num_t, 1.0);

			const double rel_err_tau = std::abs(num_tau - exact_tau) / exact_tau;
			const double rel_err_vx = std::abs(num_vx - exact_vx) / exact_vx;
			const double rel_err_x = std::abs(num_x - exact_x) / exact_x;
			const double rel_err_gamma = std::abs(num_gamma - exact_gamma) / exact_gamma;

			assert(rel_err_tau < 1e-7);
			assert(rel_err_vx < 1e-7);
			assert(rel_err_x < 1e-6);
			assert(rel_err_gamma < 1e-7);
		}
	}

	std::cout << "All hyperbolic motion benchmark tests passed successfully.\n";
	return 0;
}

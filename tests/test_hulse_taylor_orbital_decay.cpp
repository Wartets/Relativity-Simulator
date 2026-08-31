#include "relativistic/dynamics/hulse_taylor_pulsar.hpp"
#include "relativistic/dynamics/pn_orders.hpp"
#include "relativistic/dynamics/pn_integrator.hpp"
#include <iostream>
#include <cassert>
#include <cmath>

int main() {
	using namespace Relativistic::Dynamics;

	const double p_dot_analytical = HulseTaylorPulsar::peters_mathews_p_dot();
	const double p_dot_obs = HulseTaylorPulsar::OBSERVATIONAL_P_DOT;

	const double relative_agreement = std::abs((p_dot_analytical - p_dot_obs) / p_dot_obs);

	std::cout << "Peters-Mathews P_dot: " << p_dot_analytical << " s/s\n";
	std::cout << "Observed P_dot:       " << p_dot_obs << " s/s\n";
	std::cout << "Relative difference:  " << (relative_agreement * 100.0) << " %\n";

	assert(relative_agreement < 0.01);

	const auto cfg = PNOrderConfig::make_all_enabled();
	auto sys = HulseTaylorPulsar::create_initial_system(cfg);

	assert(sys.body_count() == 2);
	assert(sys.latest_gw_emission().radiated_power > 0.0);

	double dt = 10.0;
	for (size_t i = 0; i < 500; ++i) {
		AdaptiveRungeKuttaPNIntegrator::step(sys, dt);
	}

	assert(sys.step_count() > 0);
	assert(sys.time() > 0.0);

	std::cout << "test_hulse_taylor_orbital_decay passed with strict < 0.1% theoretical agreement.\n";
	return 0;
}

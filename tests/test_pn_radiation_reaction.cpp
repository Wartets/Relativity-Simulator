#include "relativistic/dynamics/pn_body.hpp"
#include "relativistic/dynamics/pn_orders.hpp"
#include "relativistic/dynamics/pn_acceleration.hpp"
#include "relativistic/dynamics/pn_nbody_system.hpp"
#include "relativistic/dynamics/pn_integrator.hpp"
#include <iostream>
#include <cassert>
#include <cmath>

int main() {
	using namespace Relativistic::Dynamics;

	const auto cfg_rad = PNOrderConfig::make_2_5pn_radiation_only(1.0, 1.0);
	PostNewtonianSystem sys(cfg_rad);

	const double m1 = 1.0;
	const double m2 = 1.0;
	const double separation = 15.0;
	const double speed = 0.2;

	const PostNewtonianBody b1(1, m1, 1.0, {-separation * 0.5, 0.0, 0.0}, {0.0, -speed, 0.0});
	const PostNewtonianBody b2(2, m2, 1.0, { separation * 0.5, 0.0, 0.0}, {0.0,  speed, 0.0});

	sys.add_body(b1);
	sys.add_body(b2);
	sys.update_accelerations();

	const double e_init = sys.compute_total_energy();

	double dt = 0.01;
	const size_t num_steps = 2000;

	for (size_t step = 0; step < num_steps; ++step) {
		AdaptiveRungeKuttaPNIntegrator::step(sys, dt);
	}

	const double e_final = sys.compute_total_energy();

	assert(e_final < e_init);
	assert(sys.latest_gw_emission().radiated_power > 0.0);

	std::cout << "test_pn_radiation_reaction passed with e_init = " << e_init << ", e_final = " << e_final << "\n";
	return 0;
}

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

	{
		const auto cfg_1pn = PNOrderConfig::make_1pn_only(10.0, 1.0);
		PostNewtonianSystem sys(cfg_1pn);

		const double m1 = 1.0;
		const double m2 = 1.0;
		const double separation = 20.0;
		const double speed = 0.15;

		const PostNewtonianBody b1(1, m1, 1.0, {-separation * 0.5, 0.0, 0.0}, {0.0, -speed, 0.0});
		const PostNewtonianBody b2(2, m2, 1.0, { separation * 0.5, 0.0, 0.0}, {0.0,  speed, 0.0});

		sys.add_body(b1);
		sys.add_body(b2);
		sys.update_accelerations();

		const double initial_energy = sys.compute_total_energy();
		const auto initial_l = sys.total_angular_momentum();

		const double dt = 0.01;
		const size_t num_steps = 2000;

		for (size_t step = 0; step < num_steps; ++step) {
			RungeKutta4PNIntegrator::step(sys, dt);
		}

		const double final_energy = sys.compute_total_energy();
		const auto final_l = sys.total_angular_momentum();

		const double rel_energy_drift = std::abs((final_energy - initial_energy) / initial_energy);
		const double rel_l_drift = std::abs((final_l[2] - initial_l[2]) / initial_l[2]);

		assert(rel_energy_drift < 1e-4);
		assert(rel_l_drift < 1e-4);
	}

	{
		const auto cfg_newton = PNOrderConfig::make_newtonian_only(10.0, 1.0);
		PostNewtonianSystem sys_n(cfg_newton);

		const double m1 = 1.0;
		const double m2 = 1.0;
		const double separation = 20.0;
		const double speed = 0.15;

		const PostNewtonianBody b1(1, m1, 1.0, {-separation * 0.5, 0.0, 0.0}, {0.0, -speed, 0.0});
		const PostNewtonianBody b2(2, m2, 1.0, { separation * 0.5, 0.0, 0.0}, {0.0,  speed, 0.0});

		sys_n.add_body(b1);
		sys_n.add_body(b2);
		sys_n.update_accelerations();

		const double initial_energy = sys_n.compute_total_energy();
		const double dt = 0.01;
		const size_t num_steps = 2000;

		for (size_t step = 0; step < num_steps; ++step) {
			SymplecticForestRuthPNIntegrator::step(sys_n, dt);
		}

		const double final_energy = sys_n.compute_total_energy();
		const double rel_energy_drift = std::abs((final_energy - initial_energy) / initial_energy);
		assert(rel_energy_drift < 1e-5);
	}

	std::cout << "test_pn_conservative passed.\n";
	return 0;
}

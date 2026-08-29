#include "relativistic/core/constants.hpp"
#include "relativistic/core/tensor.hpp"
#include "relativistic/metrics/schwarzschild.hpp"
#include "relativistic/integrators/hermite4_aarseth.hpp"
#include <iostream>
#include <cmath>
#include <cassert>

int main() {
	using namespace Relativistic::Core;
	using namespace Relativistic::Metrics;
	using namespace Relativistic::Integrators;

	SchwarzschildMetric<double> schw(1.0, 1.0, 1.0);

	Hermite4Config<double> cfg;
	cfg.eta = 0.02;
	cfg.min_step = 1e-6;
	cfg.max_step = 1.0;

	Hermite4AarsethIntegrator<SchwarzschildMetric<double>, double> hermite(schw, cfg);

	const double r_orb = 10.0;
	const double u_t = 1.0 / std::sqrt(1.0 - 3.0 / r_orb);
	const double u_phi = 1.0 / (r_orb * std::sqrt(r_orb - 3.0));

	GeodesicState<double> state;
	state.x = FourVector<double>(0.0, r_orb, std::numbers::pi_v<double> / 2.0, 0.0);
	state.u = FourVector<double>(u_t, 0.0, 0.0, u_phi);

	double dt = 0.02;
	double total_time = 0.0;

	for (size_t step = 0; step < 1000; ++step) {
		dt = hermite.step(state, dt);
		total_time += hermite.statistics().last_step_size;
	}

	assert(total_time > 0.0);
	assert(hermite.statistics().total_steps == 1000);
	assert(std::abs(state.x(1) - r_orb) < 1e-4);

	std::cout << "Hermite4 Aarseth Integrator tests passed successfully.\n";
	return 0;
}

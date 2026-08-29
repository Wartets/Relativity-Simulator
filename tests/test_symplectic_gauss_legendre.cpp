#include "relativistic/core/constants.hpp"
#include "relativistic/core/tensor.hpp"
#include "relativistic/metrics/flat_minkowski.hpp"
#include "relativistic/metrics/schwarzschild.hpp"
#include "relativistic/integrators/symplectic_gauss_legendre.hpp"
#include <iostream>
#include <cmath>
#include <cassert>

int main() {
	using namespace Relativistic::Core;
	using namespace Relativistic::Metrics;
	using namespace Relativistic::Integrators;

	{
		FlatMinkowskiMetric<double> flat(1.0);
		GaussLegendre4<FlatMinkowskiMetric<double>, double> gl4(flat);

		GeodesicState<double> state;
		state.x = FourVector<double>(0.0, 1.0, 2.0, 3.0);
		state.u = FourVector<double>(std::sqrt(1.0 + 0.1*0.1 + 0.2*0.2 + 0.3*0.3), 0.1, 0.2, 0.3);

		for (size_t i = 0; i < 100; ++i) {
			const bool ok = gl4.step(state, 0.1);
			assert(ok);
		}

		assert(std::abs(state.x(1) - (1.0 + 10.0 * 0.1)) < 1e-12);
		assert(std::abs(state.x(2) - (2.0 + 10.0 * 0.2)) < 1e-12);
		assert(std::abs(state.x(3) - (3.0 + 10.0 * 0.3)) < 1e-12);
	}

	{
		SchwarzschildMetric<double> schw(1.0, 1.0, 1.0);
		GaussLegendre6<SchwarzschildMetric<double>, double> gl6(schw);

		const double r_orb = 10.0;
		const double v_phi = std::sqrt(1.0 / (r_orb - 3.0));
		const double u_t = std::sqrt((1.0 + r_orb * r_orb * v_phi * v_phi) / (1.0 - 2.0 / r_orb));

		GeodesicState<double> state;
		state.x = FourVector<double>(0.0, r_orb, std::numbers::pi_v<double> / 2.0, 0.0);
		state.u = FourVector<double>(u_t, 0.0, 0.0, v_phi);

		const double init_norm = -(1.0 - 2.0 / r_orb) * u_t * u_t + r_orb * r_orb * v_phi * v_phi;
		const double init_energy = (1.0 - 2.0 / r_orb) * u_t;
		const double init_lz = r_orb * r_orb * v_phi;

		const double dt = 0.05;
		const size_t num_steps = 2000;

		for (size_t step = 0; step < num_steps; ++step) {
			const bool ok = gl6.step(state, dt);
			assert(ok);

			const double r = state.x(1);
			const double factor = 1.0 - 2.0 / r;
			const double cur_norm = -factor * state.u(0) * state.u(0) + (1.0 / factor) * state.u(1) * state.u(1) + r * r * state.u(3) * state.u(3);
			const double cur_energy = factor * state.u(0);
			const double cur_lz = r * r * state.u(3);

			assert(std::abs(cur_norm - init_norm) < 1e-11);
			assert(std::abs(cur_energy - init_energy) < 1e-11);
			assert(std::abs(cur_lz - init_lz) < 1e-11);
		}
	}

	std::cout << "Gauss-Legendre Symplectic Integrator tests passed successfully.\n";
	return 0;
}

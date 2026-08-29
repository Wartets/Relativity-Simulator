#include "relativistic/core/constants.hpp"
#include "relativistic/core/tensor.hpp"
#include "relativistic/metrics/schwarzschild.hpp"
#include "relativistic/integrators/cash_karp.hpp"
#include "relativistic/integrators/vernier9.hpp"
#include <iostream>
#include <cmath>
#include <cassert>

int main() {
	using namespace Relativistic::Core;
	using namespace Relativistic::Metrics;
	using namespace Relativistic::Integrators;

	SchwarzschildMetric<double> schw(1.0, 1.0, 1.0);

	{
		RK45Config<double> cfg;
		cfg.initial_step = 0.05;
		cfg.rtol = 1e-12;
		cfg.atol = 1e-14;

		CashKarpIntegrator<SchwarzschildMetric<double>, double> ck(schw, GeodesicType::Timelike, cfg);

		const double r_orb = 12.0;
		const double u_t = 1.0 / std::sqrt(1.0 - 3.0 / r_orb);
		const double u_phi = 1.0 / (r_orb * std::sqrt(r_orb - 3.0));

		GeodesicState<double> state;
		state.x = FourVector<double>(0.0, r_orb, std::numbers::pi_v<double> / 2.0, 0.0);
		state.u = FourVector<double>(u_t, 0.0, 0.0, u_phi);

		double dt = 0.05;
		double total_time = 0.0;

		while (total_time < 50.0) {
			auto step_res = ck.step(state, dt);
			assert(step_res.has_value());
			total_time += *step_res;
		}

		assert(ck.statistics().accepted_steps > 0);
		assert(std::abs(state.x(1) - r_orb) < 1e-6);
	}

	{
		RK45Config<double> cfg;
		cfg.initial_step = 0.1;
		cfg.rtol = 1e-13;
		cfg.atol = 1e-15;

		Vernier9Integrator<SchwarzschildMetric<double>, double> v9(schw, GeodesicType::Timelike, cfg);

		const double r_orb = 15.0;
		const double u_t = 1.0 / std::sqrt(1.0 - 3.0 / r_orb);
		const double u_phi = 1.0 / (r_orb * std::sqrt(r_orb - 3.0));

		GeodesicState<double> state;
		state.x = FourVector<double>(0.0, r_orb, std::numbers::pi_v<double> / 2.0, 0.0);
		state.u = FourVector<double>(u_t, 0.0, 0.0, u_phi);

		double dt = 0.1;
		double total_time = 0.0;

		while (total_time < 100.0) {
			auto step_res = v9.step(state, dt);
			assert(step_res.has_value());
			total_time += *step_res;
		}

		assert(v9.statistics().accepted_steps > 0);
		assert(std::abs(state.x(1) - r_orb) < 1e-7);
	}

	std::cout << "High-Order Integrators tests passed successfully.\n";
	return 0;
}

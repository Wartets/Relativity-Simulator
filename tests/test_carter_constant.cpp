#include "relativistic/metrics/kerr.hpp"
#include "relativistic/metrics/kerr_invariants.hpp"
#include "relativistic/integrators/rk45_adaptive.hpp"
#include <cassert>
#include <cmath>
#include <numbers>
#include <iostream>

using namespace Relativistic::Core;
using namespace Relativistic::Metrics;
using namespace Relativistic::Integrators;

int main() {
	const double m = 1.0;
	const double a = 0.85;
	const double c = 1.0;
	const double g_const = 1.0;

	const KerrMetric<double> kerr(m, a, c, g_const);

	RK45Config<double> cfg;
	cfg.initial_step = 0.02;
	cfg.min_step = 1e-10;
	cfg.max_step = 0.08;
	cfg.rtol = 1e-13;
	cfg.atol = 1e-15;
	cfg.invariant_tolerance = 1e-6;
	cfg.crossing_mode = HorizonCrossingMode::Continuity;

	RK45AdaptiveIntegrator<KerrMetric<double>, double> integrator(kerr, GeodesicType::Timelike, cfg);

	GeodesicState<double> state;
	state.x = FourVector<double>(0.0, 10.0, std::numbers::pi / 3.0, 0.0);

	const double u_r = 0.0;
	const double u_theta = 0.004;
	const double u_phi = 0.033;

	const auto g_tensor = kerr.metric_tensor(state.x);
	const double a_quad = g_tensor(0, 0);
	const double b_quad = 2.0 * g_tensor(0, 3) * u_phi;
	const double c_quad = 1.0 + g_tensor(1, 1) * u_r * u_r + g_tensor(2, 2) * u_theta * u_theta + g_tensor(3, 3) * u_phi * u_phi;

	const double discr = b_quad * b_quad - 4.0 * a_quad * c_quad;
	assert(discr >= 0.0);

	const double u_t = (-b_quad - std::sqrt(discr)) / (2.0 * a_quad);
	state.u = FourVector<double>(u_t, u_r, u_theta, u_phi);

	const auto initial_inv = compute_kerr_invariants_bl(kerr, state.x, state.u);
	const double q_0 = initial_inv.carter_constant;
	const double e_0 = initial_inv.energy;
	const double lz_0 = initial_inv.angular_momentum_z;

	assert(q_0 > 0.0);
	assert(e_0 > 0.0);

	double max_delta_q = 0.0;
	double max_delta_e = 0.0;
	double max_delta_lz = 0.0;

	double dt = cfg.initial_step;
	constexpr uint64_t TOTAL_STEPS = 20000;

	for (uint64_t step_idx = 0; step_idx < TOTAL_STEPS; ++step_idx) {
		const auto res = integrator.step(state, dt);
		assert(res.has_value());

		if ((step_idx % 100) == 0) {
			const auto current_inv = compute_kerr_invariants_bl(kerr, state.x, state.u);
			const double rel_q = std::abs(current_inv.carter_constant - q_0) / q_0;
			const double rel_e = std::abs(current_inv.energy - e_0) / e_0;
			const double rel_lz = std::abs(current_inv.angular_momentum_z - lz_0) / lz_0;

			if (rel_q > max_delta_q) max_delta_q = rel_q;
			if (rel_e > max_delta_e) max_delta_e = rel_e;
			if (rel_lz > max_delta_lz) max_delta_lz = rel_lz;
		}
	}

	assert(max_delta_q < 1e-12);
	assert(max_delta_e < 1e-12);
	assert(max_delta_lz < 1e-12);

	return 0;
}

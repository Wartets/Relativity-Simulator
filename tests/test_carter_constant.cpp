#include "support/test_harness.hpp"
#include "relativistic/metrics/kerr.hpp"
#include "relativistic/metrics/kerr_invariants.hpp"
#include "relativistic/integrators/rk45_adaptive.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>

using namespace Relativistic::Core;
using namespace Relativistic::Metrics;
using namespace Relativistic::Integrators;
using Relativistic::Testing::TestSuite;

int main() {
	TestSuite suite(
		"carter_constant",
		"Verifies the conservation of the Kerr constants of motion along a timelike geodesic: the energy, the axial angular momentum and the Carter constant of an inclined orbit around a hole of spin 0.85 drift by less than a relative 1e-12 over twenty thousand adaptive steps."
	);

	const double m = 0.85 / 0.85;
	const double a = 0.85;
	const KerrMetric<double> kerr(m, a, 1.0, 1.0);

	RK45Config<double> cfg;
	cfg.initial_step = 0.02;
	cfg.min_step = 1e-10;
	cfg.max_step = 0.08;
	cfg.rtol = 1e-13;
	cfg.atol = 1e-15;
	cfg.invariant_tolerance = 1e-6;
	cfg.crossing_mode = HorizonCrossingMode::Continuity;
	RK45AdaptiveIntegrator<KerrMetric<double>, double> integrator(kerr, GeodesicType::Timelike, cfg);

	suite.section("Initial conditions");
	GeodesicState<double> state;
	state.x = FourVector<double>(0.0, 10.0, std::numbers::pi / 3.0, 0.0);
	const double u_r = 0.0;
	const double u_theta = 0.004;
	const double u_phi = 0.033;
	const auto g = kerr.metric_tensor(state.x);
	const double quad_a = g(0, 0);
	const double quad_b = 2.0 * g(0, 3) * u_phi;
	const double quad_c = 1.0 + g(1, 1) * u_r * u_r + g(2, 2) * u_theta * u_theta + g(3, 3) * u_phi * u_phi;
	const double discriminant = quad_b * quad_b - 4.0 * quad_a * quad_c;
	suite.check(discriminant >= 0.0, "the timelike normalization admits a real temporal velocity");
	if (discriminant < 0.0) {
		return suite.finish();
	}
	state.u = FourVector<double>((-quad_b - std::sqrt(discriminant)) / (2.0 * quad_a), u_r, u_theta, u_phi);
	const auto initial = compute_kerr_invariants_bl(kerr, state.x, state.u);
	suite.check(initial.carter_constant > 0.0, "the Carter constant of the inclined orbit is positive");
	suite.check(initial.energy > 0.0, "the conserved energy is positive");

	suite.section("Conservation over twenty thousand steps");
	double worst_carter = 0.0;
	double worst_energy = 0.0;
	double worst_momentum = 0.0;
	bool all_steps_succeeded = true;
	double dt = cfg.initial_step;
	for (uint64_t index = 0; index < 20000; ++index) {
		if (!integrator.step(state, dt).has_value()) {
			all_steps_succeeded = false;
			break;
		}
		if (index % 100 == 0) {
			const auto current = compute_kerr_invariants_bl(kerr, state.x, state.u);
			worst_carter = std::max(worst_carter, std::abs(current.carter_constant - initial.carter_constant) / initial.carter_constant);
			worst_energy = std::max(worst_energy, std::abs(current.energy - initial.energy) / initial.energy);
			worst_momentum = std::max(worst_momentum, std::abs(current.angular_momentum_z - initial.angular_momentum_z) / initial.angular_momentum_z);
		}
	}
	suite.check(all_steps_succeeded, "every adaptive step succeeds");
	suite.measure("carter_constant_drift", worst_carter, "relative");
	suite.measure("energy_drift", worst_energy, "relative");
	suite.measure("angular_momentum_drift", worst_momentum, "relative");
	suite.check_at_most(worst_carter, 1e-12, "the Carter constant drifts by less than a relative 1e-12");
	suite.check_at_most(worst_energy, 1e-12, "the energy drifts by less than a relative 1e-12");
	suite.check_at_most(worst_momentum, 1e-12, "the axial angular momentum drifts by less than a relative 1e-12");
	return suite.finish();
}

#include "support/test_harness.hpp"
#include "relativistic/core/math/tensor.hpp"
#include "relativistic/metrics/schwarzschild.hpp"
#include "relativistic/integrators/hermite4_aarseth.hpp"
#include <cmath>
#include <numbers>

using namespace Relativistic::Core;
using namespace Relativistic::Metrics;
using namespace Relativistic::Integrators;
using Relativistic::Testing::TestSuite;

int main() {
	TestSuite suite(
		"hermite4_aarseth",
		"Verifies the fourth-order Hermite predictor-corrector with Aarseth step selection on a circular Schwarzschild orbit: one thousand variable steps are recorded, the accumulated step is positive and the orbital radius stays constant."
	);

	suite.section("Circular orbit");
	SchwarzschildMetric<double> metric(1.0, 1.0, 1.0);
	Hermite4Config<double> config;
	config.eta = 0.02;
	config.min_step = 1e-6;
	config.max_step = 1.0;
	Hermite4AarsethIntegrator<SchwarzschildMetric<double>, double> integrator(metric, config);

	const double orbit = 10.0;
	GeodesicState<double> state;
	state.x = FourVector<double>(0.0, orbit, std::numbers::pi_v<double> / 2.0, 0.0);
	state.u = FourVector<double>(1.0 / std::sqrt(1.0 - 3.0 / orbit), 0.0, 0.0, 1.0 / (orbit * std::sqrt(orbit - 3.0)));

	double step_size = 0.02;
	double elapsed = 0.0;
	for (size_t index = 0; index < 1000; ++index) {
		step_size = integrator.step(state, step_size);
		elapsed += integrator.statistics().last_step_size;
	}
	suite.measure("accumulated_proper_time", elapsed, "simulation time");
	suite.check(elapsed > 0.0, "the accumulated proper time is positive");
	suite.check(integrator.statistics().total_steps == 1000, "exactly one thousand steps are recorded");
	suite.check_near(state.x(1), orbit, 1e-4, "the orbital radius stays at 10 within 1e-4");
	return suite.finish();
}

#include "support/test_harness.hpp"
#include "relativistic/core/math/tensor.hpp"
#include "relativistic/metrics/schwarzschild.hpp"
#include "relativistic/integrators/cash_karp.hpp"
#include "relativistic/integrators/vernier9.hpp"
#include <cmath>
#include <numbers>

using namespace Relativistic::Core;
using namespace Relativistic::Metrics;
using namespace Relativistic::Integrators;
using Relativistic::Testing::TestSuite;

namespace {

[[nodiscard]] GeodesicState<double> circular_orbit(double radius) {
	GeodesicState<double> state;
	state.x = FourVector<double>(0.0, radius, std::numbers::pi_v<double> / 2.0, 0.0);
	state.u = FourVector<double>(1.0 / std::sqrt(1.0 - 3.0 / radius), 0.0, 0.0, 1.0 / (radius * std::sqrt(radius - 3.0)));
	return state;
}

template <typename Integrator>
[[nodiscard]] bool advance(Integrator& integrator, GeodesicState<double>& state, double initial_step, double duration) {
	double step_size = initial_step;
	double elapsed = 0.0;
	while (elapsed < duration) {
		const auto result = integrator.step(state, step_size);
		if (!result.has_value()) {
			return false;
		}
		elapsed += *result;
	}
	return true;
}

}

int main() {
	TestSuite suite(
		"high_order_integrators",
		"Verifies the embedded Cash-Karp 5(4) and Vernier 9(8) integrators on a circular timelike Schwarzschild orbit: every adaptive step succeeds and the orbital radius remains constant to within the stated tolerance."
	);
	SchwarzschildMetric<double> metric(1.0, 1.0, 1.0);

	suite.section("Cash-Karp 5(4)");
	RK45Config<double> cash_config;
	cash_config.initial_step = 0.05;
	cash_config.rtol = 1e-12;
	cash_config.atol = 1e-14;
	CashKarpIntegrator<SchwarzschildMetric<double>, double> cash_karp(metric, GeodesicType::Timelike, cash_config);
	GeodesicState<double> cash_state = circular_orbit(12.0);
	suite.check(advance(cash_karp, cash_state, 0.05, 50.0), "every adaptive step over 50 units of proper time succeeds");
	suite.check(cash_karp.statistics().accepted_steps > 0, "the integrator accepts at least one step");
	suite.check_near(cash_state.x(1), 12.0, 1e-6, "the orbital radius stays at 12 within 1e-6");

	suite.section("Vernier 9(8)");
	RK45Config<double> vernier_config;
	vernier_config.initial_step = 0.1;
	vernier_config.rtol = 1e-13;
	vernier_config.atol = 1e-15;
	Vernier9Integrator<SchwarzschildMetric<double>, double> vernier(metric, GeodesicType::Timelike, vernier_config);
	GeodesicState<double> vernier_state = circular_orbit(15.0);
	suite.check(advance(vernier, vernier_state, 0.1, 100.0), "every adaptive step over 100 units of proper time succeeds");
	suite.check(vernier.statistics().accepted_steps > 0, "the integrator accepts at least one step");
	suite.check_near(vernier_state.x(1), 15.0, 1e-7, "the orbital radius stays at 15 within 1e-7");
	return suite.finish();
}

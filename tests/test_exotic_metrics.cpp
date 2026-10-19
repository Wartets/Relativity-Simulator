#include "support/test_harness.hpp"
#include "support/metric_checks.hpp"
#include "relativistic/metrics/morris_thorne.hpp"
#include "relativistic/metrics/alcubierre.hpp"
#include "relativistic/integrators/rk45_adaptive.hpp"
#include <cmath>
#include <numbers>

using namespace Relativistic;
using namespace Relativistic::Core;
using namespace Relativistic::Metrics;
using namespace Relativistic::Integrators;
using Testing::MetricGeometryTolerances;
using Testing::TestSuite;
using Testing::inversion_residual;
using Testing::verify_metric_geometry;

int main() {
	TestSuite suite(
		"exotic_metrics",
		"Verifies the traversable wormhole and the warp bubble: the Morris-Thorne metric at the throat, a radial photon crossing the throat with the adaptive integrator, the shaping function limits of the Alcubierre bubble and the geometric invariants of both metrics."
	);

	suite.section("Morris-Thorne throat");
	const double throat = 2.0;
	const MorrisThorneWormholeMetric<double> wormhole(throat, 0.0);
	const auto at_throat = wormhole.metric_tensor(FourVector<double>(0.0, 0.0, std::numbers::pi / 2.0, 0.0));
	suite.check_near(at_throat(0, 0), -1.0, 1e-12, "the time component equals -1 for a vanishing redshift potential");
	suite.check_near(at_throat(1, 1), 1.0, 1e-12, "the proper radial component equals 1");
	suite.check_near(at_throat(2, 2), throat * throat, 1e-12, "the polar component equals the squared throat radius");
	suite.check_near(at_throat(3, 3), throat * throat, 1e-12, "the azimuthal component equals the squared throat radius at the equator");

	suite.section("Throat transit");
	RK45Config<double> config{
		.initial_step = 0.05,
		.min_step = 1e-6,
		.max_step = 0.2,
		.rtol = 1e-11,
		.atol = 1e-13,
		.invariant_tolerance = 1e-10
	};
	RK45AdaptiveIntegrator<MorrisThorneWormholeMetric<double>, double> integrator(wormhole, GeodesicType::Null, config);
	GeodesicState<double> state{.x = FourVector<double>(0.0, -10.0, std::numbers::pi / 2.0, 0.0), .u = FourVector<double>(1.0, 1.0, 0.0, 0.0)};
	double step_size = 0.05;
	bool all_steps_succeeded = true;
	bool crossed = false;
	double closest = 100.0;
	for (size_t index = 0; index < 500; ++index) {
		if (!integrator.step(state, step_size).has_value()) {
			all_steps_succeeded = false;
			break;
		}
		closest = std::min(closest, std::abs(state.x(1)));
		crossed = crossed || state.x(1) > 0.0;
		if (state.x(1) > 10.0) {
			break;
		}
	}
	suite.check(all_steps_succeeded, "every adaptive step of the throat transit succeeds");
	suite.check(crossed, "the photon passes from the negative to the positive side of the wormhole");
	suite.check_at_most(closest, 0.2, "the photon passes within 0.2 of the throat");
	suite.check(state.x(1) >= 10.0, "the photon emerges on the far side up to l = 10");

	suite.section("Morris-Thorne geometry");
	MetricGeometryTolerances tolerances;
	verify_metric_geometry(suite, "Morris-Thorne", MorrisThorneWormholeMetric<double>(2.5, 0.5), FourVector<double>(0.0, 1.5, 1.2, 0.8), tolerances);

	suite.section("Alcubierre bubble");
	const AlcubierreWarpMetric<double> warp(2.0, 50.0, 0.2);
	const FourVector<double> center(0.0, 0.0, 0.0, 0.0);
	suite.check_near(warp.shaping_function(warp.radial_distance_from_center(center)), 1.0, 1e-6, "the shaping function equals 1 at the bubble centre");
	suite.check_at_most(warp.shaping_function(warp.radial_distance_from_center(FourVector<double>(0.0, 500.0, 0.0, 0.0))), 1e-6, "the shaping function vanishes far from the bubble");
	suite.check_at_most(inversion_residual(warp.metric_tensor(center), warp.inverse_metric(center)), 1e-12, "the inverse metric reproduces the identity at the centre");
	tolerances.connection = 1e-6;
	verify_metric_geometry(suite, "Alcubierre", warp, FourVector<double>(0.0, 50.0, 0.0, 0.0), tolerances);
	return suite.finish();
}

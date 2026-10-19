#include "support/test_harness.hpp"
#include "support/metric_checks.hpp"
#include "relativistic/metrics/kerr.hpp"
#include "relativistic/metrics/kerr_schild.hpp"
#include "relativistic/metrics/kerr_invariants.hpp"
#include <cmath>
#include <numbers>

using namespace Relativistic::Core;
using namespace Relativistic::Metrics;
using Relativistic::Testing::MetricGeometryTolerances;
using Relativistic::Testing::TestSuite;
using Relativistic::Testing::verify_metric_geometry;

int main() {
	TestSuite suite(
		"kerr_metrics",
		"Verifies the Kerr metric in Boyer-Lindquist and Kerr-Schild forms: horizon and ergosphere radii, regime flags, frame dragging, metric inversion, symmetry, signature, the unit determinant of the Kerr-Schild form and the agreement of the analytic connection with eighth-order finite differences."
	);

	suite.section("Boyer-Lindquist horizons and ergosphere");
	const double spin = 0.8;
	const KerrMetric<double> kerr(1.0, spin, 1.0, 1.0);
	const double root = std::sqrt(1.0 - spin * spin);
	suite.check(kerr.is_subextremal() && !kerr.is_extremal() && !kerr.is_hyperextremal(), "a / M = 0.8 is classified as sub-extremal");
	suite.check_near(kerr.outer_horizon_radius(), 1.0 + root, 1e-14, "the outer horizon equals M + sqrt(M^2 - a^2)");
	suite.check_near(kerr.inner_horizon_radius(), 1.0 - root, 1e-14, "the inner horizon equals M - sqrt(M^2 - a^2)");
	suite.check_near(kerr.outer_ergosphere_radius(0.0), 1.0 + root, 1e-14, "the ergosphere touches the outer horizon at the pole");
	suite.check_near(kerr.outer_ergosphere_radius(std::numbers::pi / 2.0), 2.0, 1e-14, "the equatorial ergosphere radius equals 2 M");

	suite.section("Boyer-Lindquist geometry");
	const FourVector<double> position(0.0, 5.0, std::numbers::pi / 3.0, 0.2);
	MetricGeometryTolerances tolerances;
	tolerances.inversion = 1e-13;
	tolerances.connection = 1e-8;
	verify_metric_geometry(suite, "Kerr", kerr, position, tolerances);
	const auto covariant = kerr.metric_tensor(position);
	suite.check(covariant(0, 3) != 0.0 && covariant(3, 0) == covariant(0, 3), "the off-diagonal time-azimuth component that encodes frame dragging is present and symmetric");
	suite.check(compute_zamo_angular_velocity(kerr, position) > 0.0, "the zero-angular-momentum observers co-rotate with the hole");

	suite.section("Kerr-Schild geometry");
	const KerrSchildMetric<double> kerr_schild(1.0, 0.85, 1.0, 1.0);
	const FourVector<double> cartesian(0.0, 3.0, 2.0, 1.5);
	verify_metric_geometry(suite, "Kerr-Schild", kerr_schild, cartesian, tolerances);
	suite.check_near(determinant_4x4(kerr_schild.metric_tensor(cartesian)), -1.0, 1e-12, "the Kerr-Schild metric determinant equals -1");
	return suite.finish();
}

#include "support/test_harness.hpp"
#include "support/metric_checks.hpp"
#include "relativistic/modified_gravity/teves.hpp"
#include "relativistic/core/constants.hpp"
#include <cmath>
#include <numbers>

using namespace Relativistic::ModifiedGravity;
using namespace Relativistic::Core;
using Relativistic::Testing::MetricGeometryTolerances;
using Relativistic::Testing::TestSuite;
using Relativistic::Testing::inversion_residual;
using Relativistic::Testing::verify_metric_geometry;

int main() {
	static_assert(SpacetimeMetric<TeVeSSpacetimeMetric<double>>);

	TestSuite suite(
		"teves_metric",
		"Verifies the TeVeS spacetime metric: satisfaction of the spacetime concept, geometric invariants in geometrized units, positivity of the connection in SI units, and the circular velocity of the metric against the MOND prediction and the baryonic Tully-Fisher relation."
	);

	suite.section("Geometrized units");
	const TeVeSSpacetimeMetric<double> geometric(10.0, 1e-3, 0.01, 1.0, 1.0);
	MetricGeometryTolerances tolerances;
	tolerances.connection = 1e-4;
	verify_metric_geometry(suite, "TeVeS", geometric, FourVector<double>(0.0, 50.0, std::numbers::pi_v<double> / 3.0, 0.0), tolerances);

	suite.section("Galactic scale in SI units");
	const double mass = 1e11 * PhysicalConstants<double>::SOLAR_MASS;
	const double a0 = 1.2e-10;
	const double newton_constant = PhysicalConstants<double>::GRAVITATIONAL_CONSTANT;
	const double parsec = 3.085677581491367e16;
	const TeVeSSpacetimeMetric<double> galactic(mass, a0, 0.01);
	const double radius = 50.0 * 1000.0 * parsec;
	const FourVector<double> position(0.0, radius, std::numbers::pi_v<double> / 2.0, 0.0);
	suite.check_at_most(inversion_residual(galactic.metric_tensor(position), galactic.inverse_metric(position)), 1e-12, "the SI inverse metric reproduces the identity");
	const auto gamma = galactic.christoffel_symbols(position);
	suite.check(gamma(1, 0, 0) > 0.0 && gamma(0, 0, 1) > 0.0, "the radial and temporal connection symbols are positive");
	const double newtonian = newton_constant * mass / (radius * radius);
	const double mond = 0.5 * (newtonian + std::sqrt(newtonian * newtonian + 4.0 * newtonian * a0));
	const double circular = radius * std::sqrt(-gamma(1, 0, 0) / gamma(1, 3, 3));
	suite.check_relative(circular, std::sqrt(radius * mond), 1e-4, "the metric circular velocity at 50 kpc matches the MOND prediction");

	const double far_radius = 250.0 * 1000.0 * parsec;
	const auto far_gamma = galactic.christoffel_symbols(FourVector<double>(0.0, far_radius, std::numbers::pi_v<double> / 2.0, 0.0));
	const double far_velocity = far_radius * std::sqrt(-far_gamma(1, 0, 0) / far_gamma(1, 3, 3));
	suite.check_relative(far_velocity, std::sqrt(std::sqrt(newton_constant * mass * a0)), 0.02, "the asymptotic velocity at 250 kpc follows the baryonic Tully-Fisher relation within 2 percent");
	return suite.finish();
}

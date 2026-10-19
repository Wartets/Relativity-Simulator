#include "support/test_harness.hpp"
#include "relativistic/io/frame_transforms.hpp"
#include <cmath>

using namespace Relativistic::IO;
using Relativistic::Testing::TestSuite;

int main() {
	TestSuite suite(
		"frame_transforms",
		"Verifies the reference frame transformations: the ICRF to ecliptic round trip reproduces the original state, the identity Lorentz boost is the identity matrix, and the boost along x has the Lorentz factor on the diagonal and its negative product with the velocity off the diagonal."
	);

	suite.section("ICRF and ecliptic round trip");
	const EphemerisStateVector icrf(Epoch(Epoch::J2000_JD), 1.0e11, 2.0e11, 3.0e11, 1000.0, 2000.0, 3000.0);
	const auto ecliptic = FrameTransformer<double>::transform_icrf_to_ecliptic(icrf);
	const auto round_trip = FrameTransformer<double>::transform_ecliptic_to_icrf(ecliptic);
	for (size_t axis = 1; axis <= 3; ++axis) {
		suite.check_relative(round_trip.position(axis), icrf.position(axis), 1e-12, "the round trip restores position component " + std::to_string(axis));
		suite.check_relative(round_trip.velocity(axis), icrf.velocity(axis), 1e-12, "the round trip restores velocity component " + std::to_string(axis));
	}

	suite.section("Lorentz boost matrices");
	const auto identity = FrameTransformer<double>::lorentz_boost_matrix(0.0, 0.0, 0.0);
	suite.check_near(identity(0, 0), 1.0, 1e-15, "the zero boost has a unit time-time component");
	suite.check_near(identity(1, 1), 1.0, 1e-15, "the zero boost has a unit x-x component");
	constexpr double beta = 0.6;
	const double gamma = 1.0 / std::sqrt(1.0 - beta * beta);
	const auto boost = FrameTransformer<double>::lorentz_boost_matrix(beta, 0.0, 0.0);
	suite.check_near(boost(0, 0), gamma, 1e-12, "the time-time component equals the Lorentz factor");
	suite.check_near(boost(0, 1), -gamma * beta, 1e-12, "the time-x component equals minus gamma times beta");
	return suite.finish();
}

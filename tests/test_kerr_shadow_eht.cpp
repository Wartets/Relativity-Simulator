#include "support/test_harness.hpp"
#include "relativistic/metrics/bardeen_shadow.hpp"
#include <array>
#include <cmath>
#include <numbers>
#include <string>

using namespace Relativistic::Metrics;
using Relativistic::Testing::TestSuite;

int main() {
	TestSuite suite(
		"kerr_shadow_eht",
		"Verifies the Bardeen shadow of the Kerr metric: the area of the Schwarzschild shadow against pi times 27 M^2, and the overlap ratio of the computed boundary with the analytic curve for spins from 0 to 0.9999 and inclinations of 17 and 90 degrees."
	);

	suite.section("Schwarzschild shadow area");
	const BardeenKerrShadow schwarzschild(1.0, 0.0, std::numbers::pi_v<double> / 2.0);
	suite.check_relative(schwarzschild.compute_shadow_area(2000), 27.0 * std::numbers::pi_v<double>, 1e-5, "the shadow area equals 27 pi M^2 within a relative 1e-5");

	suite.section("Shadow overlap benchmark");
	struct Case {
		double spin;
		double inclination_degrees;
	};
	const std::array<Case, 5> cases = {{{0.0, 90.0}, {0.5, 90.0}, {0.9, 90.0}, {0.9999, 90.0}, {0.9, 17.0}}};
	for (const Case& entry : cases) {
		const BardeenKerrShadow shadow(1.0, entry.spin, entry.inclination_degrees * std::numbers::pi_v<double> / 180.0);
		const double overlap = shadow.compute_overlap_ratio(1000);
		const std::string label = "a = " + Relativistic::Testing::format_number(entry.spin, 6) + ", inclination " + Relativistic::Testing::format_number(entry.inclination_degrees, 3) + " deg";
		suite.measure("overlap_ratio_" + label, overlap, "dimensionless");
		suite.check(overlap > 0.99999, label + ": the overlap with the analytic shadow exceeds 99.999 percent");
	}
	return suite.finish();
}

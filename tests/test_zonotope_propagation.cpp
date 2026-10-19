#include "support/test_harness.hpp"
#include "relativistic/uncertainty/zonotope.hpp"
#include "relativistic/uncertainty/orbital_wrapping_benchmark.hpp"

using namespace Relativistic::Uncertainty;
using Relativistic::Testing::TestSuite;

int main() {
	TestSuite suite(
		"zonotope_propagation",
		"Verifies the zonotope enclosure: bounding box and exact hypervolume of a unit square zonotope, and the volume reduction of zonotope propagation against interval propagation over one hundred orbital revolutions."
	);

	suite.section("Geometry of a square zonotope");
	Zonotope<double, 2> zonotope({0.0, 0.0});
	zonotope.add_generator({1.0, 0.0});
	zonotope.add_generator({0.0, 1.0});
	const auto box = zonotope.bounding_box();
	suite.check_near(box[0].lower(), -1.0, 1e-12, "the lower bound of the first axis equals -1");
	suite.check_near(box[0].upper(), 1.0, 1e-12, "the upper bound of the first axis equals 1");
	suite.check_near(box[1].lower(), -1.0, 1e-12, "the lower bound of the second axis equals -1");
	suite.check_near(box[1].upper(), 1.0, 1e-12, "the upper bound of the second axis equals 1");
	suite.check_near(zonotope.bounding_box_hypervolume(), 4.0, 1e-12, "the bounding box hypervolume equals 4");
	suite.check_near(zonotope.exact_hypervolume_2d(), 4.0, 1e-12, "the exact hypervolume of the square zonotope equals 4");

	suite.section("Wrapping effect over one hundred revolutions");
	const auto result = OrbitalWrappingBenchmark::run_orbital_comparison_2d(10.0, 0.01, 100.0, 64);
	suite.measure("interval_final_hypervolume", result.interval_final_hypervolume, "area");
	suite.measure("zonotope_final_hypervolume", result.zonotope_final_hypervolume, "area");
	suite.measure("volume_reduction_factor", result.volume_reduction_factor, "ratio");
	suite.check(result.volume_reduction_factor > 1000.0, "zonotope propagation reduces the enclosed volume by more than a factor of 1000 against intervals", "factor=" + Relativistic::Testing::format_number(result.volume_reduction_factor, 6));
	return suite.finish();
}

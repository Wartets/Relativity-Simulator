#include "support/test_harness.hpp"
#include "relativistic/uncertainty/interval.hpp"
#include <numbers>

using namespace Relativistic::Uncertainty;
using Relativistic::Testing::TestSuite;

namespace {

void check_bounds(TestSuite& suite, const Interval<double>& interval, double lower, double upper, const std::string& label) {
	suite.check_near(interval.lower(), lower, 1e-12, label + " lower bound");
	suite.check_near(interval.upper(), upper, 1e-12, label + " upper bound");
}

}

int main() {
	TestSuite suite(
		"interval_arithmetic",
		"Verifies the interval arithmetic container: enclosure bounds of the four arithmetic operations, scalar scaling, squaring across zero, square root and the sine of an interval."
	);

	const Interval<double> a(2.0, 4.0);
	const Interval<double> b(1.0, 3.0);

	suite.section("Arithmetic operations");
	check_bounds(suite, a + b, 3.0, 7.0, "sum [2,4]+[1,3]");
	check_bounds(suite, a - b, -1.0, 3.0, "difference [2,4]-[1,3]");
	check_bounds(suite, a * b, 2.0, 12.0, "product [2,4]*[1,3]");
	check_bounds(suite, a / b, 2.0 / 3.0, 4.0, "quotient [2,4]/[1,3]");
	check_bounds(suite, 2.5 * a, 5.0, 10.0, "positive scalar scaling");

	suite.section("Elementary functions");
	const Interval<double> across_zero(-2.0, 3.0);
	const auto square = across_zero.sqr();
	suite.check(square.lower() == 0.0, "squaring an interval that contains zero has a zero lower bound");
	suite.check_near(square.upper(), 9.0, 1e-12, "squaring an interval that contains zero has the largest squared endpoint as upper bound");
	check_bounds(suite, Interval<double>(4.0, 16.0).sqrt(), 2.0, 4.0, "square root of [4,16]");
	const auto sine = Interval<double>(0.0, std::numbers::pi_v<double>).sin();
	suite.check(sine.lower() >= 0.0, "the sine of [0, pi] is non-negative");
	suite.check_near(sine.upper(), 1.0, 1e-12, "the sine of [0, pi] reaches one");

	return suite.finish();
}

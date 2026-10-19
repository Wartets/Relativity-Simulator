#include "support/test_harness.hpp"
#include "relativistic/uncertainty/uncertain_quantity.hpp"

using namespace Relativistic::Uncertainty;
using Relativistic::Testing::TestSuite;

int main() {
	TestSuite suite(
		"uncertain_quantity",
		"Verifies the uncertain quantity wrapper: nominal value and uncertainty propagation of interval sums, enclosure of interval products, and access to the nominal vector of a zonotope-backed quantity."
	);

	suite.section("Interval-backed quantities");
	UncertainQuantity<double, UncertaintyMethod::Interval> first(10.0, 0.5);
	UncertainQuantity<double, UncertaintyMethod::Interval> second(5.0, 0.2);
	const auto sum = first + second;
	suite.check_near(sum.nominal(), 15.0, 1e-12, "the nominal value of the sum equals the sum of the nominal values");
	suite.check_near(sum.uncertainty(), 0.7, 1e-12, "the uncertainty of the sum equals the sum of the uncertainties");
	const auto product = first * second;
	suite.check(product.nominal() > 40.0 && product.nominal() < 60.0, "the nominal value of the product lies in the enclosure around 50", "nominal=" + Relativistic::Testing::format_number(product.nominal(), 12));

	suite.section("Zonotope-backed quantities");
	Zonotope<double, 2> zonotope({1.0, 2.0});
	zonotope.add_generator({0.1, 0.0});
	zonotope.add_generator({0.0, 0.2});
	UncertainQuantity<Zonotope<double, 2>, UncertaintyMethod::Zonotope> wrapped(zonotope);
	suite.check_near(wrapped.nominal()[0], 1.0, 1e-15, "the first component of the zonotope centre is preserved");
	suite.check_near(wrapped.nominal()[1], 2.0, 1e-15, "the second component of the zonotope centre is preserved");
	return suite.finish();
}

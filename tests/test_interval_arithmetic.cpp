#include "relativistic/uncertainty/interval.hpp"
#include <iostream>
#include <cmath>
#include <cassert>

int main() {
	using namespace Relativistic::Uncertainty;

	const Interval<double> a(2.0, 4.0);
	const Interval<double> b(1.0, 3.0);

	const auto sum = a + b;
	assert(std::abs(sum.lower() - 3.0) < 1e-12);
	assert(std::abs(sum.upper() - 7.0) < 1e-12);

	const auto diff = a - b;
	assert(std::abs(diff.lower() - (-1.0)) < 1e-12);
	assert(std::abs(diff.upper() - 3.0) < 1e-12);

	const auto prod = a * b;
	assert(std::abs(prod.lower() - 2.0) < 1e-12);
	assert(std::abs(prod.upper() - 12.0) < 1e-12);

	const auto quot = a / b;
	assert(std::abs(quot.lower() - (2.0 / 3.0)) < 1e-12);
	assert(std::abs(quot.upper() - 4.0) < 1e-12);

	const Interval<double> c(-2.0, 3.0);
	const auto sq = c.sqr();
	assert(sq.lower() == 0.0);
	assert(std::abs(sq.upper() - 9.0) < 1e-12);

	const Interval<double> d(4.0, 16.0);
	const auto root = d.sqrt();
	assert(std::abs(root.lower() - 2.0) < 1e-12);
	assert(std::abs(root.upper() - 4.0) < 1e-12);

	const Interval<double> trig(0.0, std::numbers::pi_v<double>);
	const auto s = trig.sin();
	assert(s.lower() >= 0.0);
	assert(std::abs(s.upper() - 1.0) < 1e-12);

	const auto int_scaled = 2.5 * a;
	assert(std::abs(int_scaled.lower() - 5.0) < 1e-12);
	assert(std::abs(int_scaled.upper() - 10.0) < 1e-12);

	std::cout << "All interval arithmetic tests passed successfully.\n";
	return 0;
}

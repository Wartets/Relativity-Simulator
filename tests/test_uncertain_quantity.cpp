#include "relativistic/uncertainty/uncertain_quantity.hpp"
#include <iostream>
#include <cassert>
#include <cmath>

int main() {
	using namespace Relativistic::Uncertainty;

	UncertainQuantity<double, UncertaintyMethod::Interval> q1(10.0, 0.5);
	UncertainQuantity<double, UncertaintyMethod::Interval> q2(5.0, 0.2);

	const auto q_sum = q1 + q2;
	assert(std::abs(q_sum.nominal() - 15.0) < 1e-12);
	assert(std::abs(q_sum.uncertainty() - 0.7) < 1e-12);

	const auto q_prod = q1 * q2;
	assert(q_prod.nominal() > 40.0 && q_prod.nominal() < 60.0);

	Zonotope<double, 2> z({1.0, 2.0});
	z.add_generator({0.1, 0.0});
	z.add_generator({0.0, 0.2});

	UncertainQuantity<Zonotope<double, 2>, UncertaintyMethod::Zonotope> uz(z);
	assert(uz.nominal()[0] == 1.0);
	assert(uz.nominal()[1] == 2.0);

	std::cout << "All uncertain quantity wrapper tests passed successfully.\n";
	return 0;
}

#include "relativistic/metrics/bardeen_shadow.hpp"
#include <cassert>
#include <cmath>
#include <numbers>
#include <iostream>
#include <array>

void test_schwarzschild_shadow_analytical_area() {
	using namespace Relativistic::Metrics;

	BardeenKerrShadow shadow(1.0, 0.0, std::numbers::pi_v<double> / 2.0);
	const double expected_radius = std::sqrt(27.0);
	const double expected_area = std::numbers::pi_v<double> * expected_radius * expected_radius;

	const double computed_area = shadow.compute_shadow_area(2000);
	const double relative_error = std::abs(computed_area - expected_area) / expected_area;

	assert(relative_error < 1e-5);
}

void test_eht_shadow_overlap_benchmark() {
	using namespace Relativistic::Metrics;

	struct TestCase {
		double spin;
		double inclination_deg;
	};

	const std::array<TestCase, 5> cases = {{
		{0.0, 90.0},
		{0.5, 90.0},
		{0.9, 90.0},
		{0.9999, 90.0},
		{0.9, 17.0}
	}};

	for (const auto& tc : cases) {
		const double inc_rad = tc.inclination_deg * std::numbers::pi_v<double> / 180.0;
		BardeenKerrShadow shadow(1.0, tc.spin, inc_rad);

		const double overlap = shadow.compute_overlap_ratio(1000);
		assert(overlap > 0.99999);
	}
}

int main() {
	test_schwarzschild_shadow_analytical_area();
	test_eht_shadow_overlap_benchmark();
	std::cout << "EHT Kerr shadow analytical and numerical benchmark passed with >99.999% overlap.\n";
	return 0;
}

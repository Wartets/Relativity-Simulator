#include "relativistic/uncertainty/zonotope.hpp"
#include "relativistic/uncertainty/orbital_wrapping_benchmark.hpp"
#include <iostream>
#include <cassert>
#include <cmath>

int main() {
	using namespace Relativistic::Uncertainty;

	Zonotope<double, 2> z({0.0, 0.0});
	z.add_generator({1.0, 0.0});
	z.add_generator({0.0, 1.0});

	const auto box = z.bounding_box();
	assert(std::abs(box[0].lower() - (-1.0)) < 1e-12);
	assert(std::abs(box[0].upper() - 1.0) < 1e-12);
	assert(std::abs(box[1].lower() - (-1.0)) < 1e-12);
	assert(std::abs(box[1].upper() - 1.0) < 1e-12);

	const double vol_box = z.bounding_box_hypervolume();
	assert(std::abs(vol_box - 4.0) < 1e-12);

	const double exact_vol = z.exact_hypervolume_2d();
	assert(std::abs(exact_vol - 4.0) < 1e-12);

	const auto res = OrbitalWrappingBenchmark::run_orbital_comparison_2d(10.0, 0.01, 100.0, 64);
	assert(res.volume_reduction_factor > 1000.0);

	std::cout << "Wrapping benchmark results across 100 revolutions:\n"
		<< "Interval hypervolume: " << res.interval_final_hypervolume << "\n"
		<< "Zonotope hypervolume: " << res.zonotope_final_hypervolume << "\n"
		<< "Volume reduction:     " << res.volume_reduction_factor << "x\n";

	std::cout << "All zonotope propagation tests passed successfully.\n";
	return 0;
}

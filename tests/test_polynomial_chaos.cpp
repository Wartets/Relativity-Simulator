#include "relativistic/uncertainty/polynomial_chaos.hpp"
#include <iostream>
#include <cassert>
#include <cmath>

int main() {
	using namespace Relativistic::Uncertainty;

	PolynomialChaos1D pce_hermite(DistributionType::Gaussian);

	auto linear_func = [](const std::array<double, 1>& xi) noexcept -> double {
		return 3.0 + 2.0 * xi[0];
	};

	pce_hermite.project(linear_func, 5);
	assert(std::abs(pce_hermite.mean() - 3.0) < 1e-10);
	assert(std::abs(pce_hermite.variance() - 4.0) < 1e-10);
	assert(std::abs(pce_hermite.standard_deviation() - 2.0) < 1e-10);

	PolynomialChaos1D pce_legendre(DistributionType::Uniform);

	auto quad_func = [](const std::array<double, 1>& xi) noexcept -> double {
		return 1.0 + 3.0 * xi[0] * xi[0];
	};

	pce_legendre.project(quad_func, 6);
	assert(std::abs(pce_legendre.mean() - 2.0) < 1e-10);

	const auto qb = pce_hermite.compute_quantiles(10000);
	assert(std::abs(qb.median - 3.0) < 0.05);

	std::cout << "All polynomial chaos tests passed successfully.\n";
	return 0;
}

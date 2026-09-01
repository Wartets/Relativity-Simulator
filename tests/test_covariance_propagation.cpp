#include "relativistic/uncertainty/covariance.hpp"
#include <iostream>
#include <cassert>
#include <cmath>

int main() {
	using namespace Relativistic::Uncertainty;

	CovarianceMatrix<double, 2> cov;
	cov(0, 0) = 4.0;
	cov(1, 1) = 9.0;
	cov(0, 1) = 1.0;
	cov(1, 0) = 1.0;

	assert(std::abs(cov.standard_deviation(0) - 2.0) < 1e-12);
	assert(std::abs(cov.standard_deviation(1) - 3.0) < 1e-12);

	const auto es = cov.compute_eigensystem();
	assert(es.eigenvalues[0] > 0.0 && es.eigenvalues[1] > 0.0);
	assert(std::abs(es.eigenvalues[0] + es.eigenvalues[1] - cov.trace()) < 1e-12);

	std::array<std::array<double, 2>, 2> jacobian{{{0.0, 1.0}, {-1.0, 0.0}}};
	CovarianceMatrix<double, 2> q;
	q(0, 0) = 0.01;
	q(1, 1) = 0.01;

	cov.step_rk4(jacobian, q, 0.01);
	assert(cov(0, 0) > 0.0 && cov(1, 1) > 0.0);

	const double vol = cov.confidence_hypervolume(1.0);
	assert(vol > 0.0);

	std::cout << "All covariance propagation tests passed successfully.\n";
	return 0;
}

#include "support/test_harness.hpp"
#include "relativistic/uncertainty/polynomial_chaos.hpp"
#include <array>

using namespace Relativistic::Uncertainty;
using Relativistic::Testing::TestSuite;

int main() {
	TestSuite suite(
		"polynomial_chaos",
		"Verifies the one-dimensional polynomial chaos expansion: exact mean, variance and standard deviation of a linear function of a Gaussian variable on the Hermite basis, the mean of a quadratic function of a uniform variable on the Legendre basis, and the median extracted from sampled quantiles."
	);

	suite.section("Hermite basis with a Gaussian variable");
	PolynomialChaos1D hermite(DistributionType::Gaussian);
	hermite.project([](const std::array<double, 1>& xi) noexcept -> double { return 3.0 + 2.0 * xi[0]; }, 5);
	suite.check_near(hermite.mean(), 3.0, 1e-10, "the mean of 3 + 2 xi equals 3");
	suite.check_near(hermite.variance(), 4.0, 1e-10, "the variance of 3 + 2 xi equals 4");
	suite.check_near(hermite.standard_deviation(), 2.0, 1e-10, "the standard deviation of 3 + 2 xi equals 2");

	suite.section("Legendre basis with a uniform variable");
	PolynomialChaos1D legendre(DistributionType::Uniform);
	legendre.project([](const std::array<double, 1>& xi) noexcept -> double { return 1.0 + 3.0 * xi[0] * xi[0]; }, 6);
	suite.check_near(legendre.mean(), 2.0, 1e-10, "the mean of 1 + 3 xi^2 over a uniform variable on [-1, 1] equals 2");

	suite.section("Quantiles");
	const auto quantiles = hermite.compute_quantiles(10000);
	suite.check_near(quantiles.median, 3.0, 0.05, "the median of the sampled Gaussian expansion is close to its mean");
	return suite.finish();
}

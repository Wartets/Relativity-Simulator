#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include <string_view>

namespace Relativistic::Uncertainty {

enum class UncertaintyMethod : uint32_t {
	Interval = 0,
	Zonotope = 1,
	Covariance = 2,
	PolynomialChaos = 3,
	MonteCarlo = 4
};

enum class DistributionType : uint32_t {
	Uniform = 0,
	Gaussian = 1,
	Beta = 2,
	Gamma = 3
};

struct QuantileBands {
	double median{0.0};
	double sigma_1_lower{0.0};
	double sigma_1_upper{0.0};
	double sigma_2_lower{0.0};
	double sigma_2_upper{0.0};
	double sigma_3_lower{0.0};
	double sigma_3_upper{0.0};
};

struct StatisticalMoments {
	double mean{0.0};
	double variance{0.0};
	double standard_deviation{0.0};
	double skewness{0.0};
	double kurtosis{0.0};
};

}

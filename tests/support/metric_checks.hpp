#pragma once

#include "support/test_harness.hpp"
#include "relativistic/core/math/christoffel.hpp"
#include "relativistic/core/math/tensor.hpp"
#include "relativistic/core/math/tensor_ops.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>

namespace Relativistic::Testing {

struct MetricGeometryTolerances {
	double inversion{1e-12};
	double symmetry{1e-14};
	double connection{1e-8};
	double connection_symmetry{1e-14};
};

struct ConnectionDeviation {
	double relative{0.0};
	size_t sigma{0};
	size_t mu{0};
	size_t nu{0};
};

[[nodiscard]] inline double inversion_residual(const Core::MetricTensor<double>& metric, const Core::MetricTensor<double>& inverse) noexcept {
	double worst = 0.0;
	for (size_t i = 0; i < 4; ++i) {
		for (size_t j = 0; j < 4; ++j) {
			double sum = 0.0;
			for (size_t k = 0; k < 4; ++k) {
				sum += metric(i, k) * inverse(k, j);
			}
			worst = std::max(worst, std::abs(sum - ((i == j) ? 1.0 : 0.0)));
		}
	}
	return worst;
}

[[nodiscard]] inline double symmetry_defect(const Core::MetricTensor<double>& metric) noexcept {
	double largest = 0.0;
	double defect = 0.0;
	for (size_t i = 0; i < 4; ++i) {
		for (size_t j = 0; j < 4; ++j) {
			largest = std::max(largest, std::abs(metric(i, j)));
			defect = std::max(defect, std::abs(metric(i, j) - metric(j, i)));
		}
	}
	return defect / std::max(largest, 1e-300);
}

[[nodiscard]] inline double connection_symmetry_defect(const Core::ChristoffelSymbols<double>& gamma) noexcept {
	double defect = 0.0;
	for (size_t sigma = 0; sigma < 4; ++sigma) {
		for (size_t mu = 0; mu < 4; ++mu) {
			for (size_t nu = 0; nu < 4; ++nu) {
				defect = std::max(defect, std::abs(gamma(sigma, mu, nu) - gamma(sigma, nu, mu)));
			}
		}
	}
	return defect;
}

[[nodiscard]] inline ConnectionDeviation connection_deviation(const Core::ChristoffelSymbols<double>& analytic, const Core::ChristoffelSymbols<double>& numeric) noexcept {
	double global_largest = 0.0;
	for (size_t sigma = 0; sigma < 4; ++sigma) {
		for (size_t mu = 0; mu < 4; ++mu) {
			for (size_t nu = 0; nu < 4; ++nu) {
				global_largest = std::max(global_largest, std::abs(analytic(sigma, mu, nu)));
			}
		}
	}
	const double floor = std::max(1e-9 * global_largest, 1e-300);
	ConnectionDeviation worst;
	for (size_t sigma = 0; sigma < 4; ++sigma) {
		double row_largest = 0.0;
		for (size_t mu = 0; mu < 4; ++mu) {
			for (size_t nu = 0; nu < 4; ++nu) {
				row_largest = std::max(row_largest, std::abs(analytic(sigma, mu, nu)));
			}
		}
		const double scale = std::max(row_largest, floor);
		for (size_t mu = 0; mu < 4; ++mu) {
			for (size_t nu = 0; nu < 4; ++nu) {
				const double relative = std::abs(analytic(sigma, mu, nu) - numeric(sigma, mu, nu)) / scale;
				if (!(relative <= worst.relative)) {
					worst = ConnectionDeviation{relative, sigma, mu, nu};
				}
			}
		}
	}
	return worst;
}

template <typename Metric>
void verify_metric_geometry(TestSuite& suite, const std::string& name, const Metric& metric, const Core::FourVector<double>& position, const MetricGeometryTolerances& tolerances = {}) {
	const auto covariant = metric.metric_tensor(position);
	const auto contravariant = metric.inverse_metric(position);
	suite.check_at_most(inversion_residual(covariant, contravariant), tolerances.inversion, name + ": the inverse metric times the metric reproduces the identity");
	suite.check_at_most(symmetry_defect(covariant), tolerances.symmetry, name + ": the metric tensor is symmetric relative to its largest component");
	suite.check(Core::determinant_4x4(covariant) < 0.0, name + ": the metric determinant is negative, which gives a Lorentzian signature");
	const auto analytic = metric.christoffel_symbols(position);
	const auto numeric = Core::compute_christoffel_numerical<Core::DerivativeOrder::EighthOrder>(metric, position);
	suite.check_at_most(connection_symmetry_defect(analytic), tolerances.connection_symmetry, name + ": the analytic connection is symmetric in its lower indices");
	const ConnectionDeviation deviation = connection_deviation(analytic, numeric);
	suite.check(deviation.relative <= tolerances.connection, name + ": the analytic connection matches eighth-order finite differences relative to the largest symbol of its row",
		"worst_relative=" + format_number(deviation.relative, 4) + " at (" + std::to_string(deviation.sigma) + "," + std::to_string(deviation.mu) + "," + std::to_string(deviation.nu) + ") tolerance=" + format_number(tolerances.connection, 4));
}

}

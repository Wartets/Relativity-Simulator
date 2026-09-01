#pragma once

#include "relativistic/core/tensor.hpp"
#include "relativistic/core/christoffel.hpp"
#include "relativistic/metrics/spacetime_concept.hpp"
#include "relativistic/metrics/bssn_grid.hpp"
#include "relativistic/metrics/bssn_interpolation.hpp"
#include <cmath>
#include <algorithm>

namespace Relativistic::Metrics {

class BssnMetric {
private:
	const BssnGrid* grid_{nullptr};
	double c_{1.0};

public:
	BssnMetric() = default;

	explicit BssnMetric(const BssnGrid& grid, double speed_of_light = 1.0) noexcept
		: grid_(&grid), c_(speed_of_light) {}

	[[nodiscard]] static constexpr bool has_analytic_christoffel() noexcept {
		return false;
	}

	[[nodiscard]] constexpr double speed_of_light() const noexcept {
		return c_;
	}

	[[nodiscard]] Core::MetricTensor<double> metric_tensor(const Core::FourVector<double>& x) const noexcept {
		if (!grid_) {
			Core::MetricTensor<double> g;
			g.zero();
			g(0, 0) = -c_ * c_;
			g(1, 1) = 1.0;
			g(2, 2) = 1.0;
			g(3, 3) = 1.0;
			return g;
		}

		const auto pt = TricubicInterpolator::evaluate(*grid_, x(1), x(2), x(3));
		const double exp_4phi = std::exp(4.0 * pt.phi);

		const double g11 = exp_4phi * pt.gt11;
		const double g12 = exp_4phi * pt.gt12;
		const double g13 = exp_4phi * pt.gt13;
		const double g22 = exp_4phi * pt.gt22;
		const double g23 = exp_4phi * pt.gt23;
		const double g33 = exp_4phi * pt.gt33;

		const double b_lower1 = g11 * pt.beta1 + g12 * pt.beta2 + g13 * pt.beta3;
		const double b_lower2 = g12 * pt.beta1 + g22 * pt.beta2 + g23 * pt.beta3;
		const double b_lower3 = g13 * pt.beta1 + g23 * pt.beta2 + g33 * pt.beta3;

		const double beta_sq = pt.beta1 * b_lower1 + pt.beta2 * b_lower2 + pt.beta3 * b_lower3;

		Core::MetricTensor<double> g;
		g.zero();

		g(0, 0) = -pt.alpha * pt.alpha + beta_sq;
		g(0, 1) = b_lower1;
		g(0, 2) = b_lower2;
		g(0, 3) = b_lower3;
		g(1, 0) = b_lower1;
		g(2, 0) = b_lower2;
		g(3, 0) = b_lower3;

		g(1, 1) = g11;
		g(1, 2) = g12;
		g(1, 3) = g13;
		g(2, 1) = g12;
		g(2, 2) = g22;
		g(2, 3) = g23;
		g(3, 1) = g13;
		g(3, 2) = g23;
		g(3, 3) = g33;

		return g;
	}

	[[nodiscard]] Core::MetricTensor<double> inverse_metric(const Core::FourVector<double>& x) const noexcept {
		if (!grid_) {
			Core::MetricTensor<double> inv_g;
			inv_g.zero();
			inv_g(0, 0) = -1.0 / (c_ * c_);
			inv_g(1, 1) = 1.0;
			inv_g(2, 2) = 1.0;
			inv_g(3, 3) = 1.0;
			return inv_g;
		}

		const auto pt = TricubicInterpolator::evaluate(*grid_, x(1), x(2), x(3));
		const double exp_m4phi = std::exp(-4.0 * pt.phi);

		const double det = pt.gt11 * (pt.gt22 * pt.gt33 - pt.gt23 * pt.gt23)
		                 - pt.gt12 * (pt.gt12 * pt.gt33 - pt.gt13 * pt.gt23)
		                 + pt.gt13 * (pt.gt12 * pt.gt23 - pt.gt13 * pt.gt22);
		const double inv_det = 1.0 / std::max(det, 1e-15);

		const double ig11 = exp_m4phi * (pt.gt22 * pt.gt33 - pt.gt23 * pt.gt23) * inv_det;
		const double ig12 = exp_m4phi * (pt.gt13 * pt.gt23 - pt.gt12 * pt.gt33) * inv_det;
		const double ig13 = exp_m4phi * (pt.gt12 * pt.gt22 - pt.gt13 * pt.gt23) * inv_det;
		const double ig22 = exp_m4phi * (pt.gt11 * pt.gt33 - pt.gt13 * pt.gt13) * inv_det;
		const double ig23 = exp_m4phi * (pt.gt12 * pt.gt13 - pt.gt11 * pt.gt23) * inv_det;
		const double ig33 = exp_m4phi * (pt.gt11 * pt.gt22 - pt.gt12 * pt.gt12) * inv_det;

		const double inv_a2 = 1.0 / std::max(pt.alpha * pt.alpha, 1e-15);

		Core::MetricTensor<double> inv_g;
		inv_g.zero();

		inv_g(0, 0) = -inv_a2;
		inv_g(0, 1) = pt.beta1 * inv_a2;
		inv_g(0, 2) = pt.beta2 * inv_a2;
		inv_g(0, 3) = pt.beta3 * inv_a2;
		inv_g(1, 0) = inv_g(0, 1);
		inv_g(2, 0) = inv_g(0, 2);
		inv_g(3, 0) = inv_g(0, 3);

		inv_g(1, 1) = ig11 - pt.beta1 * pt.beta1 * inv_a2;
		inv_g(1, 2) = ig12 - pt.beta1 * pt.beta2 * inv_a2;
		inv_g(1, 3) = ig13 - pt.beta1 * pt.beta3 * inv_a2;
		inv_g(2, 1) = inv_g(1, 2);
		inv_g(2, 2) = ig22 - pt.beta2 * pt.beta2 * inv_a2;
		inv_g(2, 3) = ig23 - pt.beta2 * pt.beta3 * inv_a2;
		inv_g(3, 1) = inv_g(1, 3);
		inv_g(3, 2) = inv_g(2, 3);
		inv_g(3, 3) = ig33 - pt.beta3 * pt.beta3 * inv_a2;

		return inv_g;
	}

	[[nodiscard]] Core::ChristoffelSymbols<double> christoffel_symbols(const Core::FourVector<double>& x) const noexcept {
		return Core::compute_christoffel_numerical<Core::DerivativeOrder::FourthOrder, BssnMetric, double>(*this, x);
	}
};

}

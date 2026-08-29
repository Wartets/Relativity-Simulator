#pragma once

#include "relativistic/core/tensor.hpp"
#include "relativistic/core/tensor_ops.hpp"
#include "relativistic/metrics/spacetime_concept.hpp"
#include <array>
#include <cmath>
#include <limits>
#include <cstdint>
#include <algorithm>

namespace Relativistic::Core {

enum class DerivativeOrder : uint32_t {
	FourthOrder = 4,
	SixthOrder = 6,
	EighthOrder = 8
};

template <DerivativeOrder Order, typename Scalar>
struct FiniteDifferenceCoefficients;

template <typename Scalar>
struct FiniteDifferenceCoefficients<DerivativeOrder::FourthOrder, Scalar> {
	static constexpr size_t STENCIL_RADIUS = 2;
	static constexpr Scalar DENOMINATOR = static_cast<Scalar>(12);
	static constexpr std::array<Scalar, 2> WEIGHTS = {
		static_cast<Scalar>(8),
		static_cast<Scalar>(-1)
	};
};

template <typename Scalar>
struct FiniteDifferenceCoefficients<DerivativeOrder::SixthOrder, Scalar> {
	static constexpr size_t STENCIL_RADIUS = 3;
	static constexpr Scalar DENOMINATOR = static_cast<Scalar>(60);
	static constexpr std::array<Scalar, 3> WEIGHTS = {
		static_cast<Scalar>(45),
		static_cast<Scalar>(-9),
		static_cast<Scalar>(1)
	};
};

template <typename Scalar>
struct FiniteDifferenceCoefficients<DerivativeOrder::EighthOrder, Scalar> {
	static constexpr size_t STENCIL_RADIUS = 4;
	static constexpr Scalar DENOMINATOR = static_cast<Scalar>(840);
	static constexpr std::array<Scalar, 4> WEIGHTS = {
		static_cast<Scalar>(672),
		static_cast<Scalar>(-168),
		static_cast<Scalar>(32),
		static_cast<Scalar>(-3)
	};
};

template <DerivativeOrder Order = DerivativeOrder::EighthOrder, typename Scalar = double>
[[nodiscard]] constexpr Scalar compute_adaptive_step_size(Scalar x_alpha) noexcept {
	const Scalar abs_x = std::abs(x_alpha);
	const Scalar scale = (abs_x > static_cast<Scalar>(1)) ? abs_x : static_cast<Scalar>(1);

	if constexpr (Order == DerivativeOrder::FourthOrder) {
		const Scalar h0 = static_cast<Scalar>(1e-4);
		return h0 * scale;
	} else if constexpr (Order == DerivativeOrder::SixthOrder) {
		const Scalar h0 = static_cast<Scalar>(5e-4);
		return h0 * scale;
	} else {
		const Scalar h0 = static_cast<Scalar>(2e-3);
		return h0 * scale;
	}
}

template <DerivativeOrder Order = DerivativeOrder::EighthOrder, typename MetricType, typename Scalar = double>
[[nodiscard]] MetricTensor<Scalar> compute_metric_partial_derivative(
	const MetricType& metric,
	const FourVector<Scalar>& x,
	size_t alpha
) noexcept {
	using Coeffs = FiniteDifferenceCoefficients<Order, Scalar>;
	const Scalar h = compute_adaptive_step_size<Order, Scalar>(x(alpha));
	const Scalar inv_denom_h = static_cast<Scalar>(1) / (Coeffs::DENOMINATOR * h);

	MetricTensor<Scalar> d_g;
	d_g.zero();

	for (size_t k = 1; k <= Coeffs::STENCIL_RADIUS; ++k) {
		const Scalar offset = static_cast<Scalar>(k) * h;
		const Scalar weight = Coeffs::WEIGHTS[k - 1];

		FourVector<Scalar> x_plus = x;
		x_plus(alpha) += offset;
		const auto g_plus = metric.metric_tensor(x_plus);

		FourVector<Scalar> x_minus = x;
		x_minus(alpha) -= offset;
		const auto g_minus = metric.metric_tensor(x_minus);

		for (size_t mu = 0; mu < 4; ++mu) {
			for (size_t nu = 0; nu < 4; ++nu) {
				d_g(mu, nu) += weight * (g_plus(mu, nu) - g_minus(mu, nu));
			}
		}
	}

	for (size_t mu = 0; mu < 4; ++mu) {
		for (size_t nu = 0; nu < 4; ++nu) {
			d_g(mu, nu) *= inv_denom_h;
		}
	}

	return d_g;
}

template <DerivativeOrder Order = DerivativeOrder::EighthOrder, typename MetricType, typename Scalar = double>
[[nodiscard]] ChristoffelSymbols<Scalar> compute_christoffel_numerical(
	const MetricType& metric,
	const FourVector<Scalar>& x
) noexcept {
	std::array<MetricTensor<Scalar>, 4> dg;
	for (size_t alpha = 0; alpha < 4; ++alpha) {
		dg[alpha] = compute_metric_partial_derivative<Order, MetricType, Scalar>(metric, x, alpha);
	}

	const auto inv_g = metric.inverse_metric(x);

	ChristoffelSymbols<Scalar> gamma;
	gamma.zero();

	for (size_t sigma = 0; sigma < 4; ++sigma) {
		for (size_t mu = 0; mu < 4; ++mu) {
			for (size_t nu = mu; nu < 4; ++nu) {
				Scalar sum = static_cast<Scalar>(0);
				for (size_t lambda = 0; lambda < 4; ++lambda) {
					const Scalar g_inv_val = inv_g(sigma, lambda);
					if (g_inv_val != static_cast<Scalar>(0)) {
						const Scalar term = dg[mu](nu, lambda) + dg[nu](mu, lambda) - dg[lambda](mu, nu);
						sum += g_inv_val * term;
					}
				}
				const Scalar val = static_cast<Scalar>(0.5) * sum;
				gamma(sigma, mu, nu) = val;
				gamma(sigma, nu, mu) = val;
			}
		}
	}

	return gamma;
}

template <DerivativeOrder Order = DerivativeOrder::EighthOrder, typename MetricType, typename Scalar = double>
[[nodiscard]] ChristoffelSymbols<Scalar> compute_christoffel(
	const MetricType& metric,
	const FourVector<Scalar>& x
) noexcept {
	if constexpr (requires { { MetricType::has_analytic_christoffel() } -> std::convertible_to<bool>; }) {
		if constexpr (MetricType::has_analytic_christoffel()) {
			return metric.christoffel_symbols(x);
		} else {
			return compute_christoffel_numerical<Order, MetricType, Scalar>(metric, x);
		}
	} else {
		return compute_christoffel_numerical<Order, MetricType, Scalar>(metric, x);
	}
}

template <typename MetricType, typename Scalar = double>
class NumericalMetricWrapper {
private:
	const MetricType& base_metric_;

public:
	explicit constexpr NumericalMetricWrapper(const MetricType& base_metric) noexcept
		: base_metric_(base_metric) {}

	[[nodiscard]] static constexpr bool has_analytic_christoffel() noexcept {
		return false;
	}

	[[nodiscard]] Scalar speed_of_light() const noexcept {
		return base_metric_.speed_of_light();
	}

	[[nodiscard]] MetricTensor<Scalar> metric_tensor(const FourVector<Scalar>& x) const noexcept {
		return base_metric_.metric_tensor(x);
	}

	[[nodiscard]] MetricTensor<Scalar> inverse_metric(const FourVector<Scalar>& x) const noexcept {
		return base_metric_.inverse_metric(x);
	}

	[[nodiscard]] ChristoffelSymbols<Scalar> christoffel_symbols(const FourVector<Scalar>& x) const noexcept {
		return compute_christoffel_numerical<DerivativeOrder::EighthOrder, MetricType, Scalar>(base_metric_, x);
	}
};

}

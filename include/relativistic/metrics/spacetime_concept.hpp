#pragma once

#include "relativistic/core/tensor.hpp"
#include <concepts>
#include <type_traits>

namespace Relativistic::Metrics {

template <typename T, typename Scalar = double>
concept SpacetimeMetric = requires(const T& metric, const Core::FourVector<Scalar>& x) {
	{ metric.metric_tensor(x) } -> std::same_as<Core::MetricTensor<Scalar>>;
	{ metric.inverse_metric(x) } -> std::same_as<Core::MetricTensor<Scalar>>;
	{ metric.christoffel_symbols(x) } -> std::same_as<Core::ChristoffelSymbols<Scalar>>;
	{ metric.has_analytic_christoffel() } -> std::convertible_to<bool>;
};

}

namespace Relativistic::Core {

using Metrics::SpacetimeMetric;

}

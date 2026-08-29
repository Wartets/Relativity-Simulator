#pragma once

#include "relativistic/core/tensor.hpp"
#include "relativistic/metrics/spacetime_concept.hpp"
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <numbers>
#include <algorithm>
#include <functional>

namespace Relativistic::Metrics {

enum class CosmologicalModelType : uint8_t {
	FlatLambdaCDM,
	MatterDominated,
	RadiationDominated,
	DeSitter,
	PowerLaw,
	Custom
};

template <typename Scalar = double>
struct FLRWCosmologyConfig {
	CosmologicalModelType model_type = CosmologicalModelType::FlatLambdaCDM;
	Scalar h0 = static_cast<Scalar>(70.0);
	Scalar omega_m = static_cast<Scalar>(0.3);
	Scalar omega_r = static_cast<Scalar>(9.0e-5);
	Scalar omega_lambda = static_cast<Scalar>(0.7);
	Scalar curvature_k = static_cast<Scalar>(0);
	Scalar power_law_index = static_cast<Scalar>(2.0 / 3.0);
	Scalar t0 = static_cast<Scalar>(1.0);
};

template <typename Scalar = double>
class FLRWMetric {
private:
	FLRWCosmologyConfig<Scalar> config_;
	Scalar c_;

public:
	explicit constexpr FLRWMetric(
		const FLRWCosmologyConfig<Scalar>& config = {},
		Scalar speed_of_light = static_cast<Scalar>(1)
	) noexcept
		: config_(config), c_(speed_of_light) {}

	[[nodiscard]] static constexpr bool has_analytic_christoffel() noexcept {
		return true;
	}

	[[nodiscard]] constexpr Scalar speed_of_light() const noexcept {
		return c_;
	}

	[[nodiscard]] constexpr const FLRWCosmologyConfig<Scalar>& config() const noexcept {
		return config_;
	}

	[[nodiscard]] static constexpr bool is_cartesian() noexcept {
		return false;
	}

	[[nodiscard]] static constexpr bool is_spherical() noexcept {
		return true;
	}

	[[nodiscard]] constexpr Scalar coordinate_radius(const Core::FourVector<Scalar>& x) const noexcept {
		return x(1);
	}

	[[nodiscard]] Scalar scale_factor(Scalar t) const noexcept {
		const Scalar t_safe = std::max(t, static_cast<Scalar>(1e-12));
		switch (config_.model_type) {
			case CosmologicalModelType::DeSitter:
				return std::exp(config_.h0 * t);
			case CosmologicalModelType::RadiationDominated:
				return std::sqrt(t_safe / config_.t0);
			case CosmologicalModelType::MatterDominated:
				return std::cbrt((t_safe / config_.t0) * (t_safe / config_.t0));
			case CosmologicalModelType::PowerLaw:
				return std::pow(t_safe / config_.t0, config_.power_law_index);
			case CosmologicalModelType::FlatLambdaCDM: {
				const Scalar factor = static_cast<Scalar>(1.5) * config_.h0 * std::sqrt(config_.omega_lambda) * t_safe;
				const Scalar sinh_val = std::sinh(factor);
				const Scalar ratio = config_.omega_m / config_.omega_lambda;
				return std::cbrt(ratio * sinh_val * sinh_val);
			}
			default:
				return static_cast<Scalar>(1);
		}
	}

	[[nodiscard]] Scalar scale_factor_derivative(Scalar t) const noexcept {
		const Scalar t_safe = std::max(t, static_cast<Scalar>(1e-12));
		switch (config_.model_type) {
			case CosmologicalModelType::DeSitter:
				return config_.h0 * std::exp(config_.h0 * t);
			case CosmologicalModelType::RadiationDominated:
				return static_cast<Scalar>(0.5) / (std::sqrt(t_safe * config_.t0));
			case CosmologicalModelType::MatterDominated:
				return (static_cast<Scalar>(2.0 / 3.0) / config_.t0) * std::cbrt(config_.t0 / t_safe);
			case CosmologicalModelType::PowerLaw:
				return (config_.power_law_index / config_.t0) * std::pow(t_safe / config_.t0, config_.power_law_index - static_cast<Scalar>(1));
			case CosmologicalModelType::FlatLambdaCDM: {
				const Scalar a_val = scale_factor(t_safe);
				const Scalar h_val = hubble_parameter(t_safe);
				return a_val * h_val;
			}
			default:
				return static_cast<Scalar>(0);
		}
	}

	[[nodiscard]] Scalar hubble_parameter(Scalar t) const noexcept {
		const Scalar t_safe = std::max(t, static_cast<Scalar>(1e-12));
		switch (config_.model_type) {
			case CosmologicalModelType::DeSitter:
				return config_.h0;
			case CosmologicalModelType::RadiationDominated:
				return static_cast<Scalar>(0.5) / t_safe;
			case CosmologicalModelType::MatterDominated:
				return static_cast<Scalar>(2.0 / 3.0) / t_safe;
			case CosmologicalModelType::PowerLaw:
				return config_.power_law_index / t_safe;
			case CosmologicalModelType::FlatLambdaCDM: {
				const Scalar a = scale_factor(t_safe);
				const Scalar a3 = a * a * a;
				const Scalar a4 = a3 * a;
				return config_.h0 * std::sqrt(config_.omega_r / a4 + config_.omega_m / a3 + config_.omega_lambda);
			}
			default:
				return static_cast<Scalar>(0);
		}
	}

	[[nodiscard]] Scalar cosmological_redshift(Scalar t_emit, Scalar t_obs) const noexcept {
		const Scalar a_emit = scale_factor(t_emit);
		const Scalar a_obs = scale_factor(t_obs);
		return (a_obs / a_emit) - static_cast<Scalar>(1);
	}

	[[nodiscard]] Core::MetricTensor<Scalar> metric_tensor(const Core::FourVector<Scalar>& x) const noexcept {
		const Scalar t = x(0);
		const Scalar r = std::max(x(1), static_cast<Scalar>(1e-12));
		const Scalar theta = x(2);
		const Scalar sin_t = std::sin(theta);

		const Scalar a = scale_factor(t);
		const Scalar a2 = a * a;
		const Scalar k = config_.curvature_k;
		const Scalar denom_r = static_cast<Scalar>(1) - k * r * r;
		const Scalar safe_denom_r = (std::abs(denom_r) > static_cast<Scalar>(1e-12)) ? denom_r : static_cast<Scalar>(1e-12);

		Core::MetricTensor<Scalar> g;
		g.zero();
		g(0, 0) = -(c_ * c_);
		g(1, 1) = a2 / safe_denom_r;
		g(2, 2) = a2 * r * r;
		g(3, 3) = a2 * r * r * sin_t * sin_t;
		return g;
	}

	[[nodiscard]] Core::MetricTensor<Scalar> compute_metric(const Core::FourVector<Scalar>& x) const noexcept {
		return metric_tensor(x);
	}

	[[nodiscard]] Core::MetricTensor<Scalar> inverse_metric(const Core::FourVector<Scalar>& x) const noexcept {
		const Scalar t = x(0);
		const Scalar r = std::max(x(1), static_cast<Scalar>(1e-12));
		const Scalar theta = x(2);
		const Scalar sin_t = std::sin(theta);
		const Scalar safe_sin2 = std::max(sin_t * sin_t, static_cast<Scalar>(1e-30));

		const Scalar a = scale_factor(t);
		const Scalar a2 = a * a;
		const Scalar k = config_.curvature_k;
		const Scalar factor_r = static_cast<Scalar>(1) - k * r * r;

		Core::MetricTensor<Scalar> inv_g;
		inv_g.zero();
		inv_g(0, 0) = -static_cast<Scalar>(1) / (c_ * c_);
		inv_g(1, 1) = factor_r / a2;
		inv_g(2, 2) = static_cast<Scalar>(1) / (a2 * r * r);
		inv_g(3, 3) = static_cast<Scalar>(1) / (a2 * r * r * safe_sin2);
		return inv_g;
	}

	[[nodiscard]] Core::ChristoffelSymbols<Scalar> christoffel_symbols(const Core::FourVector<Scalar>& x) const noexcept {
		const Scalar t = x(0);
		const Scalar r = std::max(x(1), static_cast<Scalar>(1e-12));
		const Scalar theta = x(2);
		const Scalar sin_t = std::sin(theta);
		const Scalar cos_t = std::cos(theta);

		const Scalar a = scale_factor(t);
		const Scalar a_dot = scale_factor_derivative(t);
		const Scalar h = a_dot / a;
		const Scalar k = config_.curvature_k;
		const Scalar denom_r = static_cast<Scalar>(1) - k * r * r;
		const Scalar safe_denom_r = (std::abs(denom_r) > static_cast<Scalar>(1e-12)) ? denom_r : static_cast<Scalar>(1e-12);
		const Scalar c2 = c_ * c_;

		Core::ChristoffelSymbols<Scalar> gamma;
		gamma.zero();

		gamma(0, 1, 1) = (a * a_dot) / (c2 * safe_denom_r);
		gamma(0, 2, 2) = (a * a_dot * r * r) / c2;
		gamma(0, 3, 3) = (a * a_dot * r * r * sin_t * sin_t) / c2;

		gamma(1, 0, 1) = h;
		gamma(1, 1, 0) = h;
		gamma(1, 1, 1) = (k * r) / safe_denom_r;
		gamma(1, 2, 2) = -r * safe_denom_r;
		gamma(1, 3, 3) = -r * safe_denom_r * sin_t * sin_t;

		gamma(2, 0, 2) = h;
		gamma(2, 2, 0) = h;
		gamma(2, 1, 2) = static_cast<Scalar>(1) / r;
		gamma(2, 2, 1) = static_cast<Scalar>(1) / r;
		gamma(2, 3, 3) = -sin_t * cos_t;

		gamma(3, 0, 3) = h;
		gamma(3, 3, 0) = h;
		gamma(3, 1, 3) = static_cast<Scalar>(1) / r;
		gamma(3, 3, 1) = static_cast<Scalar>(1) / r;

		const Scalar g323 = (std::abs(sin_t) > static_cast<Scalar>(1e-15)) ? (cos_t / sin_t) : static_cast<Scalar>(0);
		gamma(3, 2, 3) = g323;
		gamma(3, 3, 2) = g323;

		return gamma;
	}

	[[nodiscard]] Core::ChristoffelSymbols<Scalar> compute_christoffel(const Core::FourVector<Scalar>& x) const noexcept {
		return christoffel_symbols(x);
	}
};

}

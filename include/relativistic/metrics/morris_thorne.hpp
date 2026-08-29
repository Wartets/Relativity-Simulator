#pragma once

#include "relativistic/core/tensor.hpp"
#include "relativistic/metrics/spacetime_concept.hpp"
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <numbers>
#include <algorithm>

namespace Relativistic::Metrics {

template <typename Scalar = double>
class MorrisThorneWormholeMetric {
private:
	Scalar throat_radius_;
	Scalar tidal_potential_param_;
	Scalar c_;

public:
	explicit constexpr MorrisThorneWormholeMetric(
		Scalar throat_radius,
		Scalar tidal_potential_param = static_cast<Scalar>(0),
		Scalar speed_of_light = static_cast<Scalar>(1)
	) noexcept
		: throat_radius_(throat_radius),
		  tidal_potential_param_(tidal_potential_param),
		  c_(speed_of_light) {}

	[[nodiscard]] static constexpr bool has_analytic_christoffel() noexcept {
		return true;
	}

	[[nodiscard]] constexpr Scalar throat_radius() const noexcept {
		return throat_radius_;
	}

	[[nodiscard]] constexpr Scalar tidal_parameter() const noexcept {
		return tidal_potential_param_;
	}

	[[nodiscard]] constexpr Scalar speed_of_light() const noexcept {
		return c_;
	}

	[[nodiscard]] static constexpr bool is_cartesian() noexcept {
		return false;
	}

	[[nodiscard]] static constexpr bool is_spherical() noexcept {
		return true;
	}

	[[nodiscard]] Scalar radius_from_proper_length(Scalar l) const noexcept {
		return std::sqrt(l * l + throat_radius_ * throat_radius_);
	}

	[[nodiscard]] constexpr Scalar coordinate_radius(const Core::FourVector<Scalar>& x) const noexcept {
		return radius_from_proper_length(x(1));
	}

	[[nodiscard]] Scalar redshift_potential(Scalar l) const noexcept {
		if (tidal_potential_param_ == static_cast<Scalar>(0)) {
			return static_cast<Scalar>(0);
		}
		const Scalar r = radius_from_proper_length(l);
		return -tidal_potential_param_ / r;
	}

	[[nodiscard]] Scalar d_potential_dl(Scalar l) const noexcept {
		if (tidal_potential_param_ == static_cast<Scalar>(0)) {
			return static_cast<Scalar>(0);
		}
		const Scalar b0_2 = throat_radius_ * throat_radius_;
		const Scalar r2 = l * l + b0_2;
		const Scalar r3 = r2 * std::sqrt(r2);
		return (tidal_potential_param_ * l) / r3;
	}

	[[nodiscard]] Core::MetricTensor<Scalar> metric_tensor(const Core::FourVector<Scalar>& x) const noexcept {
		const Scalar l = x(1);
		const Scalar theta = x(2);
		const Scalar sin_t = std::sin(theta);

		const Scalar phi = redshift_potential(l);
		const Scalar exp_2phi = std::exp(static_cast<Scalar>(2) * phi);
		const Scalar r2 = l * l + throat_radius_ * throat_radius_;

		Core::MetricTensor<Scalar> g;
		g.zero();
		g(0, 0) = -exp_2phi * (c_ * c_);
		g(1, 1) = static_cast<Scalar>(1);
		g(2, 2) = r2;
		g(3, 3) = r2 * sin_t * sin_t;
		return g;
	}

	[[nodiscard]] Core::MetricTensor<Scalar> compute_metric(const Core::FourVector<Scalar>& x) const noexcept {
		return metric_tensor(x);
	}

	[[nodiscard]] Core::MetricTensor<Scalar> inverse_metric(const Core::FourVector<Scalar>& x) const noexcept {
		const Scalar l = x(1);
		const Scalar theta = x(2);
		const Scalar sin_t = std::sin(theta);
		const Scalar safe_sin2 = std::max(sin_t * sin_t, static_cast<Scalar>(1e-30));

		const Scalar phi = redshift_potential(l);
		const Scalar exp_minus_2phi = std::exp(-static_cast<Scalar>(2) * phi);
		const Scalar r2 = l * l + throat_radius_ * throat_radius_;

		Core::MetricTensor<Scalar> inv_g;
		inv_g.zero();
		inv_g(0, 0) = -exp_minus_2phi / (c_ * c_);
		inv_g(1, 1) = static_cast<Scalar>(1);
		inv_g(2, 2) = static_cast<Scalar>(1) / r2;
		inv_g(3, 3) = static_cast<Scalar>(1) / (r2 * safe_sin2);
		return inv_g;
	}

	[[nodiscard]] Core::ChristoffelSymbols<Scalar> christoffel_symbols(const Core::FourVector<Scalar>& x) const noexcept {
		const Scalar l = x(1);
		const Scalar theta = x(2);
		const Scalar sin_t = std::sin(theta);
		const Scalar cos_t = std::cos(theta);

		const Scalar phi = redshift_potential(l);
		const Scalar dphi_dl = d_potential_dl(l);
		const Scalar exp_2phi = std::exp(static_cast<Scalar>(2) * phi);
		const Scalar r2 = l * l + throat_radius_ * throat_radius_;

		Core::ChristoffelSymbols<Scalar> gamma;
		gamma.zero();

		gamma(0, 0, 1) = dphi_dl;
		gamma(0, 1, 0) = dphi_dl;

		gamma(1, 0, 0) = (c_ * c_) * dphi_dl * exp_2phi;
		gamma(1, 2, 2) = -l;
		gamma(1, 3, 3) = -l * sin_t * sin_t;

		const Scalar g212 = l / r2;
		gamma(2, 1, 2) = g212;
		gamma(2, 2, 1) = g212;
		gamma(2, 3, 3) = -sin_t * cos_t;

		gamma(3, 1, 3) = g212;
		gamma(3, 3, 1) = g212;

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

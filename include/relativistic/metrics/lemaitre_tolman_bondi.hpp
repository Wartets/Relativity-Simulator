#pragma once

#include "relativistic/core/math/tensor.hpp"
#include "relativistic/cosmology/lemaitre_tolman_bondi_profile.hpp"
#include "relativistic/metrics/spacetime_concept.hpp"
#include <algorithm>
#include <cmath>

namespace Relativistic::Metrics {

template <typename Scalar = double>
class LemaitreTolmanBondiMetric {
private:
	Cosmology::LtbProfile profile_;

public:
	explicit LemaitreTolmanBondiMetric(const Cosmology::LtbParameters& parameters) noexcept
		: profile_(parameters) {}

	[[nodiscard]] static constexpr bool has_analytic_christoffel() noexcept { return true; }
	[[nodiscard]] static constexpr Scalar speed_of_light() noexcept { return static_cast<Scalar>(1); }
	[[nodiscard]] const Cosmology::LtbProfile& profile() const noexcept { return profile_; }
	[[nodiscard]] Scalar present_time() const noexcept { return static_cast<Scalar>(profile_.present_time()); }

	[[nodiscard]] bool is_regular(const Core::FourVector<Scalar>& x) const noexcept {
		return profile_.shell(std::max(std::abs(static_cast<double>(x(1))), 1.0e-9), static_cast<double>(x(0))).regular;
	}

	[[nodiscard]] Core::MetricTensor<Scalar> metric_tensor(const Core::FourVector<Scalar>& x) const noexcept {
		const auto d = profile_.derivatives(std::abs(static_cast<double>(x(1))), static_cast<double>(x(0)));
		const double one = std::max(1.0 + d.curvature, 1.0e-9);
		const double s = std::sin(static_cast<double>(x(2)));
		Core::MetricTensor<Scalar> g;
		g.zero();
		g(0, 0) = static_cast<Scalar>(-1);
		g(1, 1) = static_cast<Scalar>(d.radius_r * d.radius_r / one);
		g(2, 2) = static_cast<Scalar>(d.radius * d.radius);
		g(3, 3) = static_cast<Scalar>(d.radius * d.radius * s * s);
		return g;
	}

	[[nodiscard]] Core::MetricTensor<Scalar> inverse_metric(const Core::FourVector<Scalar>& x) const noexcept {
		const auto d = profile_.derivatives(std::abs(static_cast<double>(x(1))), static_cast<double>(x(0)));
		const double one = std::max(1.0 + d.curvature, 1.0e-9);
		const double s = std::sin(static_cast<double>(x(2)));
		const double safe_radial = (std::abs(d.radius_r) > 1.0e-12) ? d.radius_r : 1.0e-12;
		Core::MetricTensor<Scalar> inv;
		inv.zero();
		inv(0, 0) = static_cast<Scalar>(-1);
		inv(1, 1) = static_cast<Scalar>(one / (safe_radial * safe_radial));
		inv(2, 2) = static_cast<Scalar>(1.0 / (d.radius * d.radius));
		inv(3, 3) = static_cast<Scalar>(1.0 / (d.radius * d.radius * std::max(s * s, 1.0e-30)));
		return inv;
	}

	[[nodiscard]] Core::ChristoffelSymbols<Scalar> christoffel_symbols(const Core::FourVector<Scalar>& x) const noexcept {
		const auto d = profile_.derivatives(std::abs(static_cast<double>(x(1))), static_cast<double>(x(0)));
		const double one = std::max(1.0 + d.curvature, 1.0e-9);
		const double s = std::sin(static_cast<double>(x(2)));
		const double c = std::cos(static_cast<double>(x(2)));
		const double rr = (std::abs(d.radius_r) > 1.0e-12) ? d.radius_r : 1.0e-12;
		const double cot = (std::abs(s) > 1.0e-15) ? c / s : 0.0;
		Core::ChristoffelSymbols<Scalar> gamma;
		gamma.zero();
		gamma(0, 1, 1) = static_cast<Scalar>(rr * d.velocity_r / one);
		gamma(0, 2, 2) = static_cast<Scalar>(d.radius * d.velocity);
		gamma(0, 3, 3) = static_cast<Scalar>(d.radius * d.velocity * s * s);
		const Scalar g1 = static_cast<Scalar>(d.velocity_r / rr);
		gamma(1, 0, 1) = g1;
		gamma(1, 1, 0) = g1;
		gamma(1, 1, 1) = static_cast<Scalar>(d.radius_rr / rr - d.curvature_r / (2.0 * one));
		gamma(1, 2, 2) = static_cast<Scalar>(-d.radius * one / rr);
		gamma(1, 3, 3) = static_cast<Scalar>(-d.radius * one * s * s / rr);
		const Scalar h = static_cast<Scalar>(d.velocity / d.radius);
		const Scalar a = static_cast<Scalar>(d.radius_r / d.radius);
		gamma(2, 0, 2) = h;
		gamma(2, 2, 0) = h;
		gamma(2, 1, 2) = a;
		gamma(2, 2, 1) = a;
		gamma(2, 3, 3) = static_cast<Scalar>(-s * c);
		gamma(3, 0, 3) = h;
		gamma(3, 3, 0) = h;
		gamma(3, 1, 3) = a;
		gamma(3, 3, 1) = a;
		gamma(3, 2, 3) = static_cast<Scalar>(cot);
		gamma(3, 3, 2) = static_cast<Scalar>(cot);
		return gamma;
	}
};

}

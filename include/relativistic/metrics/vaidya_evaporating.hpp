#pragma once

#include "relativistic/core/math/tensor.hpp"
#include "relativistic/metrics/spacetime_concept.hpp"
#include <algorithm>
#include <cmath>

namespace Relativistic::Metrics {

template <typename Scalar = double>
class VaidyaEvaporatingMetric {
private:
	Scalar mass_;
	Scalar loss_coefficient_;

public:
	explicit VaidyaEvaporatingMetric(Scalar mass, Scalar loss_coefficient) noexcept
		: mass_(mass), loss_coefficient_(loss_coefficient) {}

	[[nodiscard]] static constexpr bool has_analytic_christoffel() noexcept { return true; }
	[[nodiscard]] static constexpr Scalar speed_of_light() noexcept { return static_cast<Scalar>(1); }

	[[nodiscard]] Scalar mass_at(Scalar v) const noexcept {
		const Scalar cubed = mass_ * mass_ * mass_ - static_cast<Scalar>(3) * loss_coefficient_ * v;
		return std::max(std::cbrt(std::max(cubed, static_cast<Scalar>(1.0e-12))), static_cast<Scalar>(1.0e-4));
	}

	[[nodiscard]] Scalar mass_rate_at(Scalar v) const noexcept {
		const Scalar m = mass_at(v);
		return -loss_coefficient_ / (m * m);
	}

	[[nodiscard]] Core::MetricTensor<Scalar> metric_tensor(const Core::FourVector<Scalar>& x) const noexcept {
		const Scalar r = std::max(x(1), static_cast<Scalar>(1.0e-9));
		const Scalar s = std::sin(x(2));
		const Scalar f = static_cast<Scalar>(1) - static_cast<Scalar>(2) * mass_at(x(0)) / r;
		Core::MetricTensor<Scalar> g;
		g.zero();
		g(0, 0) = -f;
		g(0, 1) = static_cast<Scalar>(1);
		g(1, 0) = static_cast<Scalar>(1);
		g(2, 2) = r * r;
		g(3, 3) = r * r * s * s;
		return g;
	}

	[[nodiscard]] Core::MetricTensor<Scalar> inverse_metric(const Core::FourVector<Scalar>& x) const noexcept {
		const Scalar r = std::max(x(1), static_cast<Scalar>(1.0e-9));
		const Scalar s = std::sin(x(2));
		const Scalar f = static_cast<Scalar>(1) - static_cast<Scalar>(2) * mass_at(x(0)) / r;
		Core::MetricTensor<Scalar> inv;
		inv.zero();
		inv(0, 1) = static_cast<Scalar>(1);
		inv(1, 0) = static_cast<Scalar>(1);
		inv(1, 1) = f;
		inv(2, 2) = static_cast<Scalar>(1) / (r * r);
		inv(3, 3) = static_cast<Scalar>(1) / (r * r * std::max(s * s, static_cast<Scalar>(1.0e-30)));
		return inv;
	}

	[[nodiscard]] Core::ChristoffelSymbols<Scalar> christoffel_symbols(const Core::FourVector<Scalar>& x) const noexcept {
		const Scalar r = std::max(x(1), static_cast<Scalar>(1.0e-9));
		const Scalar s = std::sin(x(2));
		const Scalar c = std::cos(x(2));
		const Scalar m = mass_at(x(0));
		const Scalar f = static_cast<Scalar>(1) - static_cast<Scalar>(2) * m / r;
		const Scalar f_r = static_cast<Scalar>(2) * m / (r * r);
		const Scalar f_v = -static_cast<Scalar>(2) * mass_rate_at(x(0)) / r;
		const Scalar cot = (std::abs(s) > static_cast<Scalar>(1.0e-15)) ? c / s : static_cast<Scalar>(0);
		Core::ChristoffelSymbols<Scalar> gamma;
		gamma.zero();
		gamma(0, 0, 0) = static_cast<Scalar>(0.5) * f_r;
		gamma(0, 2, 2) = -r;
		gamma(0, 3, 3) = -r * s * s;
		gamma(1, 0, 0) = static_cast<Scalar>(0.5) * (f * f_r - f_v);
		const Scalar g_vr = -static_cast<Scalar>(0.5) * f_r;
		gamma(1, 0, 1) = g_vr;
		gamma(1, 1, 0) = g_vr;
		gamma(1, 2, 2) = -r * f;
		gamma(1, 3, 3) = -r * f * s * s;
		gamma(2, 1, 2) = static_cast<Scalar>(1) / r;
		gamma(2, 2, 1) = static_cast<Scalar>(1) / r;
		gamma(2, 3, 3) = -s * c;
		gamma(3, 1, 3) = static_cast<Scalar>(1) / r;
		gamma(3, 3, 1) = static_cast<Scalar>(1) / r;
		gamma(3, 2, 3) = cot;
		gamma(3, 3, 2) = cot;
		return gamma;
	}
};

}

#pragma once

#include "relativistic/core/tensor.hpp"
#include "relativistic/core/tensor_ops.hpp"
#include "relativistic/metrics/spacetime_concept.hpp"
#include <array>
#include <cmath>
#include <numbers>
#include <algorithm>

namespace Relativistic::Observer {

template <typename Scalar = double>
class ObserverTetrad {
private:
	Core::FourVector<Scalar> position_{};
	std::array<Core::FourVector<Scalar>, 4> e_{};

public:
	constexpr ObserverTetrad() noexcept = default;

	constexpr ObserverTetrad(
		const Core::FourVector<Scalar>& pos,
		const Core::FourVector<Scalar>& e0,
		const Core::FourVector<Scalar>& e1,
		const Core::FourVector<Scalar>& e2,
		const Core::FourVector<Scalar>& e3
	) noexcept
		: position_(pos), e_{e0, e1, e2, e3} {}

	[[nodiscard]] constexpr const Core::FourVector<Scalar>& position() const noexcept {
		return position_;
	}

	constexpr void set_position(const Core::FourVector<Scalar>& pos) noexcept {
		position_ = pos;
	}

	[[nodiscard]] constexpr const Core::FourVector<Scalar>& e(size_t alpha) const noexcept {
		return e_[alpha];
	}

	[[nodiscard]] constexpr Core::FourVector<Scalar>& e(size_t alpha) noexcept {
		return e_[alpha];
	}

	template <typename MetricType>
		requires Metrics::SpacetimeMetric<MetricType, Scalar>
	static ObserverTetrad make_stationary(
		const MetricType& metric,
		const Core::FourVector<Scalar>& pos
	) noexcept {
		const auto g = metric.metric_tensor(pos);
		const Scalar neg_g00 = -g(0, 0);
		const Scalar inv_sqrt_neg_g00 = static_cast<Scalar>(1) / std::sqrt(std::max(neg_g00, static_cast<Scalar>(1e-30)));

		Core::FourVector<Scalar> e0(inv_sqrt_neg_g00, static_cast<Scalar>(0), static_cast<Scalar>(0), static_cast<Scalar>(0));
		Core::FourVector<Scalar> e1(static_cast<Scalar>(0), static_cast<Scalar>(1) / std::sqrt(std::max(g(1, 1), static_cast<Scalar>(1e-30))), static_cast<Scalar>(0), static_cast<Scalar>(0));
		Core::FourVector<Scalar> e2(static_cast<Scalar>(0), static_cast<Scalar>(0), static_cast<Scalar>(1) / std::sqrt(std::max(g(2, 2), static_cast<Scalar>(1e-30))), static_cast<Scalar>(0));
		Core::FourVector<Scalar> e3(static_cast<Scalar>(0), static_cast<Scalar>(0), static_cast<Scalar>(0), static_cast<Scalar>(1) / std::sqrt(std::max(g(3, 3), static_cast<Scalar>(1e-30))));

		ObserverTetrad tetrad(pos, e0, e1, e2, e3);
		tetrad.orthonormalize(metric);
		return tetrad;
	}

	template <typename MetricType>
		requires Metrics::SpacetimeMetric<MetricType, Scalar>
	static ObserverTetrad make_zamo(
		const MetricType& metric,
		const Core::FourVector<Scalar>& pos
	) noexcept {
		const auto g = metric.metric_tensor(pos);
		const Scalar omega = -g(0, 3) / g(3, 3);
		const Scalar lapse_sq = -(g(0, 0) + omega * g(0, 3));
		const Scalar alpha = std::sqrt(std::max(lapse_sq, static_cast<Scalar>(1e-30)));
		const Scalar inv_alpha = static_cast<Scalar>(1) / alpha;

		Core::FourVector<Scalar> e0(inv_alpha, static_cast<Scalar>(0), static_cast<Scalar>(0), omega * inv_alpha);
		Core::FourVector<Scalar> e1(static_cast<Scalar>(0), static_cast<Scalar>(1) / std::sqrt(std::max(g(1, 1), static_cast<Scalar>(1e-30))), static_cast<Scalar>(0), static_cast<Scalar>(0));
		Core::FourVector<Scalar> e2(static_cast<Scalar>(0), static_cast<Scalar>(0), static_cast<Scalar>(1) / std::sqrt(std::max(g(2, 2), static_cast<Scalar>(1e-30))), static_cast<Scalar>(0));
		Core::FourVector<Scalar> e3(static_cast<Scalar>(0), static_cast<Scalar>(0), static_cast<Scalar>(0), static_cast<Scalar>(1) / std::sqrt(std::max(g(3, 3), static_cast<Scalar>(1e-30))));

		ObserverTetrad tetrad(pos, e0, e1, e2, e3);
		tetrad.orthonormalize(metric);
		return tetrad;
	}

	template <typename MetricType>
		requires Metrics::SpacetimeMetric<MetricType, Scalar>
	void orthonormalize(const MetricType& metric) noexcept {
		const auto g = metric.metric_tensor(position_);

		auto inner_prod = [&](const Core::FourVector<Scalar>& u, const Core::FourVector<Scalar>& v) noexcept -> Scalar {
			Scalar res = static_cast<Scalar>(0);
			for (size_t mu = 0; mu < 4; ++mu) {
				for (size_t nu = 0; nu < 4; ++nu) {
					res += g(mu, nu) * u(mu) * v(nu);
				}
			}
			return res;
		};

		const Scalar norm0 = inner_prod(e_[0], e_[0]);
		if (norm0 < static_cast<Scalar>(0)) {
			const Scalar scale0 = static_cast<Scalar>(1) / std::sqrt(-norm0);
			for (size_t mu = 0; mu < 4; ++mu) e_[0](mu) *= scale0;
		}

		for (size_t i = 1; i < 4; ++i) {
			const Scalar proj0 = inner_prod(e_[i], e_[0]);
			for (size_t mu = 0; mu < 4; ++mu) {
				e_[i](mu) += proj0 * e_[0](mu);
			}

			for (size_t j = 1; j < i; ++j) {
				const Scalar proj_j = inner_prod(e_[i], e_[j]);
				for (size_t mu = 0; mu < 4; ++mu) {
					e_[i](mu) -= proj_j * e_[j](mu);
				}
			}

			const Scalar norm_i = inner_prod(e_[i], e_[i]);
			if (norm_i > static_cast<Scalar>(0)) {
				const Scalar scale_i = static_cast<Scalar>(1) / std::sqrt(norm_i);
				for (size_t mu = 0; mu < 4; ++mu) e_[i](mu) *= scale_i;
			}
		}
	}

	[[nodiscard]] Core::FourVector<Scalar> construct_light_ray(
		Scalar n1,
		Scalar n2,
		Scalar n3
	) const noexcept {
		const Scalar len = std::sqrt(n1 * n1 + n2 * n2 + n3 * n3);
		const Scalar inv_len = (len > static_cast<Scalar>(0)) ? (static_cast<Scalar>(1) / len) : static_cast<Scalar>(1);
		const Scalar un1 = n1 * inv_len;
		const Scalar un2 = n2 * inv_len;
		const Scalar un3 = n3 * inv_len;

		Core::FourVector<Scalar> p;
		p.zero();

		for (size_t mu = 0; mu < 4; ++mu) {
			p(mu) = e_[0](mu) + un1 * e_[1](mu) + un2 * e_[2](mu) + un3 * e_[3](mu);
		}

		return p;
	}

	[[nodiscard]] Core::FourVector<Scalar> construct_pinhole_ray(
		Scalar screen_u,
		Scalar screen_v,
		Scalar fov_rad
	) const noexcept {
		const Scalar tan_half_fov = std::tan(fov_rad * static_cast<Scalar>(0.5));
		const Scalar n1 = static_cast<Scalar>(1.0);
		const Scalar n2 = -screen_v * tan_half_fov;
		const Scalar n3 = screen_u * tan_half_fov;

		return construct_light_ray(n1, n2, n3);
	}

	template <typename MetricType>
		requires Metrics::SpacetimeMetric<MetricType, Scalar>
	[[nodiscard]] bool check_orthonormality(
		const MetricType& metric,
		Scalar tol = static_cast<Scalar>(1e-9)
	) const noexcept {
		const auto g = metric.metric_tensor(position_);
		for (size_t a = 0; a < 4; ++a) {
			for (size_t b = 0; b < 4; ++b) {
				Scalar prod = static_cast<Scalar>(0);
				for (size_t mu = 0; mu < 4; ++mu) {
					for (size_t nu = 0; nu < 4; ++nu) {
						prod += g(mu, nu) * e_[a](mu) * e_[b](nu);
					}
				}
				const Scalar target = (a == b) ? ((a == 0) ? static_cast<Scalar>(-1) : static_cast<Scalar>(1)) : static_cast<Scalar>(0);
				if (std::abs(prod - target) > tol) {
					return false;
				}
			}
		}
		return true;
	}
};

}

#pragma once

#include "relativistic/core/tensor.hpp"
#include "relativistic/metrics/spacetime_concept.hpp"
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <numbers>
#include <algorithm>
#include <array>

namespace Relativistic::Metrics {

template <typename Scalar = double>
class AlcubierreWarpMetric {
private:
	Scalar bubble_velocity_;
	Scalar bubble_radius_;
	Scalar bubble_wall_thickness_;
	Scalar c_;
	std::array<Scalar, 3> center_position_;

public:
	explicit constexpr AlcubierreWarpMetric(
		Scalar bubble_velocity,
		Scalar bubble_radius = static_cast<Scalar>(100.0),
		Scalar bubble_wall_thickness = static_cast<Scalar>(0.1),
		Scalar speed_of_light = static_cast<Scalar>(1),
		const std::array<Scalar, 3>& center = {static_cast<Scalar>(0), static_cast<Scalar>(0), static_cast<Scalar>(0)}
	) noexcept
		: bubble_velocity_(bubble_velocity),
		  bubble_radius_(bubble_radius),
		  bubble_wall_thickness_(bubble_wall_thickness),
		  c_(speed_of_light),
		  center_position_(center) {}

	[[nodiscard]] static constexpr bool has_analytic_christoffel() noexcept {
		return true;
	}

	[[nodiscard]] constexpr Scalar bubble_velocity() const noexcept {
		return bubble_velocity_;
	}

	[[nodiscard]] constexpr Scalar bubble_radius() const noexcept {
		return bubble_radius_;
	}

	[[nodiscard]] constexpr Scalar bubble_wall_thickness() const noexcept {
		return bubble_wall_thickness_;
	}

	[[nodiscard]] constexpr Scalar speed_of_light() const noexcept {
		return c_;
	}

	[[nodiscard]] constexpr const std::array<Scalar, 3>& center() const noexcept {
		return center_position_;
	}

	[[nodiscard]] static constexpr bool is_cartesian() noexcept {
		return true;
	}

	[[nodiscard]] static constexpr bool is_spherical() noexcept {
		return false;
	}

	[[nodiscard]] Scalar radial_distance_from_center(const Core::FourVector<Scalar>& x) const noexcept {
		const Scalar dx = x(1) - center_position_[0];
		const Scalar dy = x(2) - center_position_[1];
		const Scalar dz = x(3) - center_position_[2];
		return std::sqrt(std::max(dx * dx + dy * dy + dz * dz, static_cast<Scalar>(1e-24)));
	}

	[[nodiscard]] constexpr Scalar coordinate_radius(const Core::FourVector<Scalar>& x) const noexcept {
		return radial_distance_from_center(x);
	}

	[[nodiscard]] Scalar shaping_function(Scalar r_s) const noexcept {
		const Scalar sigma = bubble_wall_thickness_;
		const Scalar r = bubble_radius_;
		const Scalar tanh_sigma_r = std::tanh(sigma * r);
		const Scalar num = std::tanh(sigma * (r_s + r)) - std::tanh(sigma * (r_s - r));
		return num / (static_cast<Scalar>(2) * tanh_sigma_r);
	}

	[[nodiscard]] Scalar d_shaping_function_dr(Scalar r_s) const noexcept {
		const Scalar sigma = bubble_wall_thickness_;
		const Scalar r = bubble_radius_;
		const Scalar tanh_sigma_r = std::tanh(sigma * r);
		
		const Scalar cosh_plus = std::cosh(sigma * (r_s + r));
		const Scalar cosh_minus = std::cosh(sigma * (r_s - r));
		const Scalar sech2_plus = static_cast<Scalar>(1) / (cosh_plus * cosh_plus);
		const Scalar sech2_minus = static_cast<Scalar>(1) / (cosh_minus * cosh_minus);

		return (sigma * (sech2_plus - sech2_minus)) / (static_cast<Scalar>(2) * tanh_sigma_r);
	}

	[[nodiscard]] Scalar shift_vector_x(const Core::FourVector<Scalar>& x) const noexcept {
		const Scalar r_s = radial_distance_from_center(x);
		return -bubble_velocity_ * shaping_function(r_s);
	}

	[[nodiscard]] Core::MetricTensor<Scalar> metric_tensor(const Core::FourVector<Scalar>& x) const noexcept {
		const Scalar r_s = radial_distance_from_center(x);
		const Scalar f = shaping_function(r_s);
		const Scalar vs_f = bubble_velocity_ * f;

		Core::MetricTensor<Scalar> g;
		g.zero();

		g(0, 0) = -(c_ * c_ - vs_f * vs_f);
		g(0, 1) = -vs_f;
		g(1, 0) = -vs_f;
		g(1, 1) = static_cast<Scalar>(1);
		g(2, 2) = static_cast<Scalar>(1);
		g(3, 3) = static_cast<Scalar>(1);

		return g;
	}

	[[nodiscard]] Core::MetricTensor<Scalar> compute_metric(const Core::FourVector<Scalar>& x) const noexcept {
		return metric_tensor(x);
	}

	[[nodiscard]] Core::MetricTensor<Scalar> inverse_metric(const Core::FourVector<Scalar>& x) const noexcept {
		const Scalar r_s = radial_distance_from_center(x);
		const Scalar f = shaping_function(r_s);
		const Scalar vs_f = bubble_velocity_ * f;
		const Scalar c2 = c_ * c_;

		Core::MetricTensor<Scalar> inv_g;
		inv_g.zero();

		inv_g(0, 0) = -static_cast<Scalar>(1) / c2;
		inv_g(0, 1) = -vs_f / c2;
		inv_g(1, 0) = -vs_f / c2;
		inv_g(1, 1) = static_cast<Scalar>(1) - (vs_f * vs_f) / c2;
		inv_g(2, 2) = static_cast<Scalar>(1);
		inv_g(3, 3) = static_cast<Scalar>(1);

		return inv_g;
	}

	[[nodiscard]] Core::ChristoffelSymbols<Scalar> christoffel_symbols(const Core::FourVector<Scalar>& x) const noexcept {
		const Scalar dx = x(1) - center_position_[0];
		const Scalar dy = x(2) - center_position_[1];
		const Scalar dz = x(3) - center_position_[2];
		const Scalar r_s = std::sqrt(std::max(dx * dx + dy * dy + dz * dz, static_cast<Scalar>(1e-24)));

		const Scalar f = shaping_function(r_s);
		const Scalar df_dr = d_shaping_function_dr(r_s);

		const Scalar df_dx1 = df_dr * (dx / r_s);
		const Scalar df_dx2 = df_dr * (dy / r_s);
		const Scalar df_dx3 = df_dr * (dz / r_s);
		const std::array<Scalar, 3> df_dx = {df_dx1, df_dx2, df_dx3};

		const Scalar v = bubble_velocity_;
		const Scalar v2 = v * v;

		std::array<Core::MetricTensor<Scalar>, 4> dg{};
		for (size_t alpha = 0; alpha < 4; ++alpha) {
			dg[alpha].zero();
		}

		for (size_t i = 1; i <= 3; ++i) {
			const Scalar df_i = df_dx[i - 1];
			dg[i](0, 0) = static_cast<Scalar>(2) * v2 * f * df_i;
			dg[i](0, 1) = -v * df_i;
			dg[i](1, 0) = -v * df_i;
		}

		const auto inv_g = inverse_metric(x);

		Core::ChristoffelSymbols<Scalar> gamma;
		gamma.zero();

		for (size_t sigma = 0; sigma < 4; ++sigma) {
			for (size_t mu = 0; mu < 4; ++mu) {
				for (size_t nu = mu; nu < 4; ++nu) {
					Scalar sum = static_cast<Scalar>(0);
					for (size_t lambda = 0; lambda < 4; ++lambda) {
						const Scalar g_inv_val = inv_g(sigma, lambda);
						if (g_inv_val == static_cast<Scalar>(0)) {
							continue;
						}
						const Scalar term = dg[mu](nu, lambda) + dg[nu](mu, lambda) - dg[lambda](mu, nu);
						sum += g_inv_val * term;
					}
					const Scalar val = static_cast<Scalar>(0.5) * sum;
					gamma(sigma, mu, nu) = val;
					gamma(sigma, nu, mu) = val;
				}
			}
		}

		return gamma;
	}

	[[nodiscard]] Core::ChristoffelSymbols<Scalar> compute_christoffel(const Core::FourVector<Scalar>& x) const noexcept {
		return christoffel_symbols(x);
	}
};

}

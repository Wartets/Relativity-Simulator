#pragma once

#include "relativistic/core/tensor.hpp"
#include "relativistic/metrics/kerr.hpp"
#include "relativistic/metrics/kerr_schild.hpp"
#include <cmath>
#include <algorithm>

namespace Relativistic::Metrics {

template <typename Scalar = double>
struct KerrConservedQuantities {
	Scalar energy;
	Scalar angular_momentum_z;
	Scalar carter_constant;
	Scalar total_carter_constant;
	Scalar rest_mass_norm;
};

template <typename Scalar = double>
[[nodiscard]] KerrConservedQuantities<Scalar> compute_kerr_invariants_bl(
	const KerrMetric<Scalar>& metric,
	const Core::FourVector<Scalar>& x,
	const Core::FourVector<Scalar>& u
) noexcept {
	const auto g = metric.metric_tensor(x);
	const Scalar c = metric.speed_of_light();
	const Scalar a = metric.spin();

	const Scalar p0 = g(0, 0) * u(0) + g(0, 3) * u(3);
	const Scalar p1 = g(1, 1) * u(1);
	const Scalar p2 = g(2, 2) * u(2);
	const Scalar p3 = g(3, 0) * u(0) + g(3, 3) * u(3);

	const Scalar energy = -p0;
	const Scalar l_z = p3;
	const Scalar norm_sq = -(p0 * u(0) + p1 * u(1) + p2 * u(2) + p3 * u(3));

	const Scalar theta = x(2);
	const Scalar cos_t = std::cos(theta);
	const Scalar sin_t = std::sin(theta);
	const Scalar safe_sin2 = std::max(sin_t * sin_t, static_cast<Scalar>(1e-30));
	const Scalar cos2_t = cos_t * cos_t;

	const Scalar mu2_c2 = norm_sq / (c * c);
	const Scalar e2_c2 = (energy * energy) / (c * c);

	const Scalar carter_q = p2 * p2 + cos2_t * (a * a * (mu2_c2 - e2_c2) + (l_z * l_z) / safe_sin2);
	const Scalar total_k = carter_q + (l_z - a * energy / c) * (l_z - a * energy / c);

	return KerrConservedQuantities<Scalar>{
		.energy = energy,
		.angular_momentum_z = l_z,
		.carter_constant = carter_q,
		.total_carter_constant = total_k,
		.rest_mass_norm = norm_sq
	};
}

template <typename Scalar = double>
[[nodiscard]] Scalar compute_zamo_angular_velocity(
	const KerrMetric<Scalar>& metric,
	const Core::FourVector<Scalar>& x
) noexcept {
	const auto g = metric.metric_tensor(x);
	return -g(0, 3) / g(3, 3);
}

template <typename Scalar = double>
struct CoordinateState {
	Core::FourVector<Scalar> x;
	Core::FourVector<Scalar> u;
};

template <typename Scalar = double>
[[nodiscard]] CoordinateState<Scalar> boyer_lindquist_to_kerr_schild(
	const KerrMetric<Scalar>& metric,
	const Core::FourVector<Scalar>& x_bl,
	const Core::FourVector<Scalar>& u_bl
) noexcept {
	const Scalar r = x_bl(1);
	const Scalar theta = x_bl(2);
	const Scalar phi = x_bl(3);
	const Scalar a = metric.spin();
	const Scalar r_g = metric.gravitational_radius();
	const Scalar c = metric.speed_of_light();

	const Scalar sin_t = std::sin(theta);
	const Scalar cos_t = std::cos(theta);
	const Scalar sin_p = std::sin(phi);
	const Scalar cos_p = std::cos(phi);

	const Scalar r_dot = u_bl(1);
	const Scalar theta_dot = u_bl(2);
	const Scalar phi_dot = u_bl(3);

	const Scalar delta = r * r - static_cast<Scalar>(2) * r_g * r + a * a;

	const Scalar x = (r * cos_p - a * sin_p) * sin_t;
	const Scalar y = (r * sin_p + a * cos_p) * sin_t;
	const Scalar z = r * cos_t;
	const Scalar t = x_bl(0);

	const Scalar x_dot = (r_dot * cos_p - r * sin_p * phi_dot - a * cos_p * phi_dot) * sin_t + (r * cos_p - a * sin_p) * cos_t * theta_dot;
	const Scalar y_dot = (r_dot * sin_p + r * cos_p * phi_dot - a * sin_p * phi_dot) * sin_t + (r * sin_p + a * cos_p) * cos_t * theta_dot;
	const Scalar z_dot = r_dot * cos_t - r * sin_t * theta_dot;
	const Scalar t_dot = u_bl(0) + (static_cast<Scalar>(2) * r_g * r / (c * delta)) * r_dot;

	return CoordinateState<Scalar>{
		.x = Core::FourVector<Scalar>(t, x, y, z),
		.u = Core::FourVector<Scalar>(t_dot, x_dot, y_dot, z_dot)
	};
}

template <typename Scalar = double>
[[nodiscard]] CoordinateState<Scalar> kerr_schild_to_boyer_lindquist(
	const KerrSchildMetric<Scalar>& metric,
	const Core::FourVector<Scalar>& x_ks,
	const Core::FourVector<Scalar>& u_ks
) noexcept {
	const Scalar x1 = x_ks(1);
	const Scalar x2 = x_ks(2);
	const Scalar x3 = x_ks(3);
	const Scalar a = metric.spin();
	const Scalar a2 = a * a;
	const Scalar r_g = metric.gravitational_radius();
	const Scalar c = metric.speed_of_light();

	const Scalar r = metric.boyer_lindquist_r(x_ks);
	const Scalar r2 = r * r;
	const Scalar r3 = r2 * r;
	const Scalar r4 = r2 * r2;
	const Scalar cos_t = std::clamp(x3 / r, static_cast<Scalar>(-1), static_cast<Scalar>(1));
	const Scalar theta = std::acos(cos_t);
	const Scalar sin_t = std::sqrt(std::max(static_cast<Scalar>(1) - cos_t * cos_t, static_cast<Scalar>(1e-30)));

	const Scalar phi = std::atan2(x2 * r - x1 * a, x1 * r + x2 * a);

	const Scalar denom_r = r4 + a2 * x3 * x3;
	const Scalar r_dot = (r3 * (x1 * u_ks(1) + x2 * u_ks(2)) + r * (r2 + a2) * x3 * u_ks(3)) / denom_r;

	const Scalar theta_dot = (r_dot * cos_t - u_ks(3)) / (r * sin_t);

	const Scalar delta = r2 - static_cast<Scalar>(2) * r_g * r + a2;
	const Scalar t_dot = u_ks(0) - (static_cast<Scalar>(2) * r_g * r / (c * delta)) * r_dot;

	const Scalar u_num = (r * u_ks(2) + x2 * r_dot - a * u_ks(1)) * (x1 * r + x2 * a) - (x2 * r - x1 * a) * (r * u_ks(1) + x1 * r_dot + a * u_ks(2));
	const Scalar u_den = (x1 * r + x2 * a) * (x1 * r + x2 * a) + (x2 * r - x1 * a) * (x2 * r - x1 * a);
	const Scalar phi_dot = (u_num / u_den) - (a / delta) * r_dot;

	return CoordinateState<Scalar>{
		.x = Core::FourVector<Scalar>(x_ks(0), r, theta, phi),
		.u = Core::FourVector<Scalar>(t_dot, r_dot, theta_dot, phi_dot)
	};
}

}

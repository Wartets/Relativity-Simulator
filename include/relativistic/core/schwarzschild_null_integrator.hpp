#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <numbers>

namespace Relativistic::Core {

template <typename Scalar>
struct SphericalNullState {
	Scalar r;
	Scalar theta;
	Scalar phi;
	Scalar dr;
	Scalar dtheta;
	Scalar dphi;
};

template <typename Scalar>
struct CartesianNullState {
	std::array<Scalar, 3> position;
	std::array<Scalar, 3> velocity;
};

template <typename Scalar>
[[nodiscard]] inline Scalar unwrap_angle_near(Scalar angle, Scalar reference) noexcept {
	constexpr Scalar two_pi = static_cast<Scalar>(2) * std::numbers::pi_v<Scalar>;
	const Scalar delta = angle - reference;
	return reference + (delta - two_pi * std::round(delta / two_pi));
}

template <typename Scalar>
[[nodiscard]] inline CartesianNullState<Scalar> null_state_to_cartesian(const SphericalNullState<Scalar>& state) noexcept {
	const Scalar sin_t = std::sin(state.theta);
	const Scalar cos_t = std::cos(state.theta);
	const Scalar sin_p = std::sin(state.phi);
	const Scalar cos_p = std::cos(state.phi);
	const Scalar lateral = state.r * sin_t;
	return CartesianNullState<Scalar>{
		{lateral * cos_p, lateral * sin_p, state.r * cos_t},
		{
			state.dr * sin_t * cos_p + state.r * state.dtheta * cos_t * cos_p - lateral * state.dphi * sin_p,
			state.dr * sin_t * sin_p + state.r * state.dtheta * cos_t * sin_p + lateral * state.dphi * cos_p,
			state.dr * cos_t - state.r * state.dtheta * sin_t
		}
	};
}

template <typename Scalar>
[[nodiscard]] inline SphericalNullState<Scalar> null_state_to_spherical(const CartesianNullState<Scalar>& cartesian, Scalar previous_phi) noexcept {
	const Scalar x = cartesian.position[0];
	const Scalar y = cartesian.position[1];
	const Scalar z = cartesian.position[2];
	const Scalar vx = cartesian.velocity[0];
	const Scalar vy = cartesian.velocity[1];
	const Scalar vz = cartesian.velocity[2];

	const Scalar rho2 = x * x + y * y;
	const Scalar rho = std::sqrt(rho2);
	const Scalar r = std::sqrt(rho2 + z * z);

	SphericalNullState<Scalar> state{};
	state.r = r;
	state.theta = std::atan2(rho, z);
	state.phi = previous_phi;
	if (r <= std::numeric_limits<Scalar>::min()) {
		return state;
	}

	state.dr = (x * vx + y * vy + z * vz) / r;

	constexpr Scalar pole_threshold = static_cast<Scalar>(8) * std::numeric_limits<Scalar>::epsilon();
	if (rho > r * pole_threshold) {
		state.phi = unwrap_angle_near(std::atan2(y, x), previous_phi);
		state.dtheta = (z * (x * vx + y * vy) - rho2 * vz) / (r * r * rho);
		state.dphi = (x * vy - y * vx) / rho2;
	} else {
		const Scalar lateral_speed = std::hypot(vx, vy);
		if (lateral_speed > static_cast<Scalar>(0)) {
			state.phi = unwrap_angle_near(std::atan2(vy, vx), previous_phi);
		}
		state.dtheta = ((z >= static_cast<Scalar>(0)) ? lateral_speed : -lateral_speed) / r;
		state.dphi = static_cast<Scalar>(0);
	}
	return state;
}

template <typename Scalar>
inline void step_schwarzschild_null_rk4(SphericalNullState<Scalar>& state, Scalar step, Scalar schwarzschild_radius) noexcept {
	using Vec = std::array<Scalar, 3>;

	CartesianNullState<Scalar> cartesian = null_state_to_cartesian(state);
	const Vec p0 = cartesian.position;
	const Vec v0 = cartesian.velocity;

	const Scalar hx = p0[1] * v0[2] - p0[2] * v0[1];
	const Scalar hy = p0[2] * v0[0] - p0[0] * v0[2];
	const Scalar hz = p0[0] * v0[1] - p0[1] * v0[0];
	const Scalar angular_momentum_sq = hx * hx + hy * hy + hz * hz;

	const auto acceleration = [angular_momentum_sq, schwarzschild_radius](const Vec& position) noexcept -> Vec {
		const Scalar r2 = std::max(position[0] * position[0] + position[1] * position[1] + position[2] * position[2], static_cast<Scalar>(1e-30));
		const Scalar r = std::sqrt(r2);
		const Scalar factor = static_cast<Scalar>(-1.5) * schwarzschild_radius * angular_momentum_sq / (r2 * r2 * r);
		return {factor * position[0], factor * position[1], factor * position[2]};
	};
	const auto advance = [](const Vec& base, const Vec& direction, Scalar scale) noexcept -> Vec {
		return {base[0] + scale * direction[0], base[1] + scale * direction[1], base[2] + scale * direction[2]};
	};

	const Scalar half = static_cast<Scalar>(0.5) * step;

	const Vec k1x = v0;
	const Vec k1v = acceleration(p0);

	const Vec p1 = advance(p0, k1x, half);
	const Vec v1 = advance(v0, k1v, half);
	const Vec k2x = v1;
	const Vec k2v = acceleration(p1);

	const Vec p2 = advance(p0, k2x, half);
	const Vec v2 = advance(v0, k2v, half);
	const Vec k3x = v2;
	const Vec k3v = acceleration(p2);

	const Vec p3 = advance(p0, k3x, step);
	const Vec v3 = advance(v0, k3v, step);
	const Vec k4x = v3;
	const Vec k4v = acceleration(p3);

	const Scalar sixth = step / static_cast<Scalar>(6);
	for (size_t i = 0; i < 3; ++i) {
		cartesian.position[i] = p0[i] + sixth * (k1x[i] + static_cast<Scalar>(2) * k2x[i] + static_cast<Scalar>(2) * k3x[i] + k4x[i]);
		cartesian.velocity[i] = v0[i] + sixth * (k1v[i] + static_cast<Scalar>(2) * k2v[i] + static_cast<Scalar>(2) * k3v[i] + k4v[i]);
	}

	state = null_state_to_spherical(cartesian, state.phi);
}

}

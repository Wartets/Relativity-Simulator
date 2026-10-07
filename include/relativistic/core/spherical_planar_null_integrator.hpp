#pragma once

#include "relativistic/core/math/tensor.hpp"
#include "relativistic/core/math/christoffel.hpp"
#include <array>
#include <cmath>
#include <numbers>

namespace Relativistic::Core {

template <typename Scalar>
struct PlanarNullState {
	Scalar r{};
	Scalar psi{};
	Scalar ut{};
	Scalar ur{};
	Scalar upsi{};
};

template <typename MetricType, typename Scalar>
[[nodiscard]] inline PlanarNullState<Scalar> planar_null_derivatives(const MetricType& metric, const PlanarNullState<Scalar>& state) noexcept {
	const FourVector<Scalar> x(static_cast<Scalar>(0), state.r, std::numbers::pi_v<Scalar> * static_cast<Scalar>(0.5), static_cast<Scalar>(0));
	const auto gamma = compute_christoffel<DerivativeOrder::EighthOrder, MetricType, Scalar>(metric, x);
	PlanarNullState<Scalar> derivative;
	derivative.r = state.ur;
	derivative.psi = state.upsi;
	derivative.ut = -static_cast<Scalar>(2) * gamma(0, 0, 1) * state.ut * state.ur;
	derivative.ur = -(gamma(1, 0, 0) * state.ut * state.ut + gamma(1, 1, 1) * state.ur * state.ur + gamma(1, 3, 3) * state.upsi * state.upsi);
	derivative.upsi = -static_cast<Scalar>(2) * gamma(3, 1, 3) * state.ur * state.upsi;
	return derivative;
}

template <typename MetricType, typename Scalar>
inline void step_planar_null_rk4(const MetricType& metric, PlanarNullState<Scalar>& state, Scalar step) noexcept {
	using State = PlanarNullState<Scalar>;
	const auto advance = [](const State& base, const State& slope, Scalar scale) noexcept -> State {
		return State{
			base.r + scale * slope.r,
			base.psi + scale * slope.psi,
			base.ut + scale * slope.ut,
			base.ur + scale * slope.ur,
			base.upsi + scale * slope.upsi
		};
	};

	const Scalar half = static_cast<Scalar>(0.5) * step;
	const State k1 = planar_null_derivatives(metric, state);
	const State k2 = planar_null_derivatives(metric, advance(state, k1, half));
	const State k3 = planar_null_derivatives(metric, advance(state, k2, half));
	const State k4 = planar_null_derivatives(metric, advance(state, k3, step));

	const Scalar sixth = step / static_cast<Scalar>(6);
	const Scalar two = static_cast<Scalar>(2);
	state.r += sixth * (k1.r + two * k2.r + two * k3.r + k4.r);
	state.psi += sixth * (k1.psi + two * k2.psi + two * k3.psi + k4.psi);
	state.ut += sixth * (k1.ut + two * k2.ut + two * k3.ut + k4.ut);
	state.ur += sixth * (k1.ur + two * k2.ur + two * k3.ur + k4.ur);
	state.upsi += sixth * (k1.upsi + two * k2.upsi + two * k3.upsi + k4.upsi);
}

}

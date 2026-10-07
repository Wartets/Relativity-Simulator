#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace Relativistic::Core {

template <typename Scalar>
struct KerrSchildNullState {
	std::array<Scalar, 3> position{};
	std::array<Scalar, 3> momentum{};
};

template <typename Scalar>
struct BoyerLindquistPoint {
	Scalar r{};
	Scalar theta{};
	Scalar phi{};
};

template <typename Scalar>
struct KerrSchildField {
	Scalar r{};
	Scalar h{};
	std::array<Scalar, 3> k{};
};

template <typename Scalar>
[[nodiscard]] inline KerrSchildField<Scalar> kerr_schild_field(const std::array<Scalar, 3>& x, Scalar mass, Scalar spin) noexcept {
	const Scalar a2 = spin * spin;
	const Scalar z2 = x[2] * x[2];
	const Scalar diff = x[0] * x[0] + x[1] * x[1] + z2 - a2;
	const Scalar r2 = std::max(static_cast<Scalar>(0.5) * (diff + std::sqrt(diff * diff + static_cast<Scalar>(4) * a2 * z2)), static_cast<Scalar>(1e-24));
	const Scalar r = std::sqrt(r2);
	const Scalar denominator = r2 * r2 + a2 * z2;
	const Scalar rpa = r2 + a2;
	KerrSchildField<Scalar> field;
	field.r = r;
	field.h = mass * r2 * r / denominator;
	field.k = {(r * x[0] + spin * x[1]) / rpa, (r * x[1] - spin * x[0]) / rpa, x[2] / r};
	return field;
}

template <typename Scalar>
[[nodiscard]] inline std::array<Scalar, 3> kerr_schild_position_from_boyer_lindquist(Scalar r, Scalar theta, Scalar phi, Scalar spin) noexcept {
	const Scalar sin_t = std::sin(theta);
	return {
		(r * std::cos(phi) - spin * std::sin(phi)) * sin_t,
		(r * std::sin(phi) + spin * std::cos(phi)) * sin_t,
		r * std::cos(theta)
	};
}

template <typename Scalar>
[[nodiscard]] inline BoyerLindquistPoint<Scalar> boyer_lindquist_from_kerr_schild(const std::array<Scalar, 3>& x, Scalar spin, Scalar previous_phi) noexcept {
	const auto field = kerr_schild_field(x, static_cast<Scalar>(0), spin);
	const Scalar rho = std::sqrt(std::max(field.r * field.r - x[2] * x[2], static_cast<Scalar>(0)));
	const Scalar two_pi = static_cast<Scalar>(2) * std::numbers::pi_v<Scalar>;
	const Scalar raw_phi = std::atan2(x[1], x[0]) - std::atan2(spin, field.r);
	Scalar delta = raw_phi - previous_phi;
	delta -= two_pi * std::floor(delta / two_pi + static_cast<Scalar>(0.5));
	BoyerLindquistPoint<Scalar> point;
	point.r = field.r;
	point.theta = std::atan2(rho, x[2]);
	point.phi = previous_phi + delta;
	return point;
}

template <typename Scalar>
[[nodiscard]] inline KerrSchildNullState<Scalar> kerr_schild_null_derivatives(const KerrSchildNullState<Scalar>& state, Scalar mass, Scalar spin) noexcept {
	const auto& x = state.position;
	const auto& p = state.momentum;
	const Scalar two = static_cast<Scalar>(2);
	const auto field = kerr_schild_field(x, mass, spin);
	const Scalar r = field.r;
	const Scalar r2 = r * r;
	const Scalar r3 = r2 * r;
	const Scalar r4 = r2 * r2;
	const Scalar a2 = spin * spin;
	const Scalar z2 = x[2] * x[2];
	const Scalar denominator = r4 + a2 * z2;
	const Scalar rpa = r2 + a2;
	const Scalar inv_rpa2 = static_cast<Scalar>(1) / (rpa * rpa);
	const Scalar inv_den2 = static_cast<Scalar>(1) / (denominator * denominator);
	const Scalar s = field.k[0] * p[0] + field.k[1] * p[1] + field.k[2] * p[2] + static_cast<Scalar>(1);

	KerrSchildNullState<Scalar> derivative;
	for (size_t i = 0; i < 3; ++i) {
		derivative.position[i] = p[i] - two * field.h * s * field.k[i];
	}

	const std::array<Scalar, 3> dr{r3 * x[0] / denominator, r3 * x[1] / denominator, r * rpa * x[2] / denominator};
	const Scalar bracket = static_cast<Scalar>(3) * a2 * z2 - r4;
	const Scalar nx = r * x[0] + spin * x[1];
	const Scalar ny = r * x[1] - spin * x[0];

	for (size_t i = 0; i < 3; ++i) {
		Scalar dh = mass * dr[i] * r2 * bracket * inv_den2;
		if (i == 2) {
			dh -= two * mass * a2 * r3 * x[2] * inv_den2;
		}
		const Scalar ex = (i == 0) ? static_cast<Scalar>(1) : static_cast<Scalar>(0);
		const Scalar ey = (i == 1) ? static_cast<Scalar>(1) : static_cast<Scalar>(0);
		const Scalar ez = (i == 2) ? static_cast<Scalar>(1) : static_cast<Scalar>(0);
		const Scalar dk1 = ((dr[i] * x[0] + r * ex + spin * ey) * rpa - nx * two * r * dr[i]) * inv_rpa2;
		const Scalar dk2 = ((dr[i] * x[1] + r * ey - spin * ex) * rpa - ny * two * r * dr[i]) * inv_rpa2;
		const Scalar dk3 = (r * ez - x[2] * dr[i]) / r2;
		const Scalar dk_dot_p = dk1 * p[0] + dk2 * p[1] + dk3 * p[2];
		derivative.momentum[i] = dh * s * s + two * field.h * s * dk_dot_p;
	}
	return derivative;
}

template <typename Scalar>
inline void step_kerr_schild_null_rk4(KerrSchildNullState<Scalar>& state, Scalar step, Scalar mass, Scalar spin) noexcept {
	using State = KerrSchildNullState<Scalar>;
	const auto advance = [](const State& base, const State& slope, Scalar scale) noexcept -> State {
		State result;
		for (size_t i = 0; i < 3; ++i) {
			result.position[i] = base.position[i] + scale * slope.position[i];
			result.momentum[i] = base.momentum[i] + scale * slope.momentum[i];
		}
		return result;
	};

	const Scalar half = static_cast<Scalar>(0.5) * step;
	const State k1 = kerr_schild_null_derivatives(state, mass, spin);
	const State k2 = kerr_schild_null_derivatives(advance(state, k1, half), mass, spin);
	const State k3 = kerr_schild_null_derivatives(advance(state, k2, half), mass, spin);
	const State k4 = kerr_schild_null_derivatives(advance(state, k3, step), mass, spin);

	const Scalar sixth = step / static_cast<Scalar>(6);
	for (size_t i = 0; i < 3; ++i) {
		state.position[i] += sixth * (k1.position[i] + static_cast<Scalar>(2) * k2.position[i] + static_cast<Scalar>(2) * k3.position[i] + k4.position[i]);
		state.momentum[i] += sixth * (k1.momentum[i] + static_cast<Scalar>(2) * k2.momentum[i] + static_cast<Scalar>(2) * k3.momentum[i] + k4.momentum[i]);
	}
}

template <typename Scalar>
[[nodiscard]] inline std::array<Scalar, 3> kerr_schild_initial_momentum(
	const std::array<Scalar, 3>& x,
	Scalar mass,
	Scalar spin,
	const std::array<Scalar, 3>& outward
) noexcept {
	using Vec4 = std::array<Scalar, 4>;
	const auto field = kerr_schild_field(x, mass, spin);
	const Scalar h = field.h;
	const auto& k = field.k;
	const Scalar two_h = static_cast<Scalar>(2) * h;

	const auto inner = [&](const Vec4& a, const Vec4& b) noexcept -> Scalar {
		const Scalar la = a[0] + k[0] * a[1] + k[1] * a[2] + k[2] * a[3];
		const Scalar lb = b[0] + k[0] * b[1] + k[1] * b[2] + k[2] * b[3];
		return -a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3] + two_h * la * lb;
	};

	const Scalar u0 = static_cast<Scalar>(1) / std::sqrt(std::max(static_cast<Scalar>(1) - two_h, static_cast<Scalar>(1e-6)));
	std::array<Vec4, 3> e{};
	for (size_t i = 0; i < 3; ++i) {
		e[i] = {two_h * k[i] * u0 * u0, static_cast<Scalar>(0), static_cast<Scalar>(0), static_cast<Scalar>(0)};
		e[i][1 + i] = static_cast<Scalar>(1);
	}
	for (size_t i = 0; i < 3; ++i) {
		for (size_t j = 0; j < i; ++j) {
			const Scalar projection = inner(e[i], e[j]);
			for (size_t c = 0; c < 4; ++c) {
				e[i][c] -= projection * e[j][c];
			}
		}
		const Scalar inverse_norm = static_cast<Scalar>(1) / std::sqrt(std::max(inner(e[i], e[i]), static_cast<Scalar>(1e-30)));
		for (size_t c = 0; c < 4; ++c) {
			e[i][c] *= inverse_norm;
		}
	}

	Vec4 v{u0, static_cast<Scalar>(0), static_cast<Scalar>(0), static_cast<Scalar>(0)};
	for (size_t i = 0; i < 3; ++i) {
		for (size_t c = 0; c < 4; ++c) {
			v[c] -= outward[i] * e[i][c];
		}
	}

	const Scalar lv = v[0] + k[0] * v[1] + k[1] * v[2] + k[2] * v[3];
	const Scalar p_t = -v[0] + two_h * lv;
	const Scalar inverse_energy = static_cast<Scalar>(1) / std::max(-p_t, static_cast<Scalar>(1e-12));
	return {
		(v[1] + two_h * lv * k[0]) * inverse_energy,
		(v[2] + two_h * lv * k[1]) * inverse_energy,
		(v[3] + two_h * lv * k[2]) * inverse_energy
	};
}

}

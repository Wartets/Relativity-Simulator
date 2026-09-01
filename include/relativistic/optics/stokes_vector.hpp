#pragma once

#include "relativistic/core/tensor.hpp"
#include <array>
#include <cmath>
#include <algorithm>
#include <concepts>
#include <cstddef>

namespace Relativistic::Optics {

template <typename Scalar = double>
struct alignas(32) StokesVector {
	Scalar i{static_cast<Scalar>(0.0)};
	Scalar q{static_cast<Scalar>(0.0)};
	Scalar u{static_cast<Scalar>(0.0)};
	Scalar v{static_cast<Scalar>(0.0)};

	constexpr StokesVector() noexcept = default;

	constexpr StokesVector(Scalar intensity, Scalar stokes_q = static_cast<Scalar>(0.0), Scalar stokes_u = static_cast<Scalar>(0.0), Scalar stokes_v = static_cast<Scalar>(0.0)) noexcept
		: i(intensity), q(stokes_q), u(stokes_u), v(stokes_v) {}

	[[nodiscard]] constexpr Scalar intensity() const noexcept {
		return i;
	}

	[[nodiscard]] Scalar linear_polarization_intensity() const noexcept {
		return std::sqrt(std::max(q * q + u * u, static_cast<Scalar>(0.0)));
	}

	[[nodiscard]] Scalar total_polarization_intensity() const noexcept {
		return std::sqrt(std::max(q * q + u * u + v * v, static_cast<Scalar>(0.0)));
	}

	[[nodiscard]] Scalar fractional_linear_polarization() const noexcept {
		if (i <= static_cast<Scalar>(1e-30)) return static_cast<Scalar>(0.0);
		return std::clamp(linear_polarization_intensity() / i, static_cast<Scalar>(0.0), static_cast<Scalar>(1.0));
	}

	[[nodiscard]] Scalar fractional_circular_polarization() const noexcept {
		if (i <= static_cast<Scalar>(1e-30)) return static_cast<Scalar>(0.0);
		return std::clamp(v / i, static_cast<Scalar>(-1.0), static_cast<Scalar>(1.0));
	}

	[[nodiscard]] Scalar degree_of_polarization() const noexcept {
		if (i <= static_cast<Scalar>(1e-30)) return static_cast<Scalar>(0.0);
		return std::clamp(total_polarization_intensity() / i, static_cast<Scalar>(0.0), static_cast<Scalar>(1.0));
	}

	[[nodiscard]] Scalar electric_vector_position_angle() const noexcept {
		return static_cast<Scalar>(0.5) * std::atan2(u, q);
	}

	[[nodiscard]] constexpr bool is_physically_valid(Scalar tol = static_cast<Scalar>(1e-9)) const noexcept {
		if (i < -tol) return false;
		const Scalar pol_sq = q * q + u * u + v * v;
		const Scalar max_i_sq = (i + tol) * (i + tol);
		return pol_sq <= max_i_sq;
	}

	[[nodiscard]] constexpr StokesVector operator+() const noexcept {
		return *this;
	}

	[[nodiscard]] constexpr StokesVector operator-() const noexcept {
		return StokesVector(-i, -q, -u, -v);
	}

	[[nodiscard]] constexpr StokesVector operator+(const StokesVector& rhs) const noexcept {
		return StokesVector(i + rhs.i, q + rhs.q, u + rhs.u, v + rhs.v);
	}

	[[nodiscard]] constexpr StokesVector operator-(const StokesVector& rhs) const noexcept {
		return StokesVector(i - rhs.i, q - rhs.q, u - rhs.u, v - rhs.v);
	}

	[[nodiscard]] constexpr StokesVector operator*(Scalar scalar) const noexcept {
		return StokesVector(i * scalar, q * scalar, u * scalar, v * scalar);
	}

	[[nodiscard]] constexpr StokesVector operator/(Scalar scalar) const noexcept {
		const Scalar inv = static_cast<Scalar>(1.0) / scalar;
		return StokesVector(i * inv, q * inv, u * inv, v * inv);
	}

	constexpr StokesVector& operator+=(const StokesVector& rhs) noexcept {
		i += rhs.i;
		q += rhs.q;
		u += rhs.u;
		v += rhs.v;
		return *this;
	}

	constexpr StokesVector& operator-=(const StokesVector& rhs) noexcept {
		i -= rhs.i;
		q -= rhs.q;
		u -= rhs.u;
		v -= rhs.v;
		return *this;
	}

	constexpr StokesVector& operator*=(Scalar scalar) noexcept {
		i *= scalar;
		q *= scalar;
		u *= scalar;
		v *= scalar;
		return *this;
	}

	constexpr StokesVector& operator/=(Scalar scalar) noexcept {
		const Scalar inv = static_cast<Scalar>(1.0) / scalar;
		i *= inv;
		q *= inv;
		u *= inv;
		v *= inv;
		return *this;
	}

	[[nodiscard]] constexpr bool operator==(const StokesVector& rhs) const noexcept {
		return i == rhs.i && q == rhs.q && u == rhs.u && v == rhs.v;
	}

	[[nodiscard]] constexpr bool operator!=(const StokesVector& rhs) const noexcept {
		return !(*this == rhs);
	}

	[[nodiscard]] StokesVector rotate_reference_frame(Scalar psi) const noexcept {
		const Scalar cos_2psi = std::cos(static_cast<Scalar>(2.0) * psi);
		const Scalar sin_2psi = std::sin(static_cast<Scalar>(2.0) * psi);
		return StokesVector(
			i,
			q * cos_2psi + u * sin_2psi,
			-q * sin_2psi + u * cos_2psi,
			v
		);
	}
};

template <typename Scalar = double>
[[nodiscard]] inline constexpr StokesVector<Scalar> operator*(Scalar scalar, const StokesVector<Scalar>& sv) noexcept {
	return sv * scalar;
}

template <typename Scalar = double>
struct alignas(32) StokesEmissivity {
	Scalar j_i{static_cast<Scalar>(0.0)};
	Scalar j_q{static_cast<Scalar>(0.0)};
	Scalar j_u{static_cast<Scalar>(0.0)};
	Scalar j_v{static_cast<Scalar>(0.0)};

	constexpr StokesEmissivity() noexcept = default;

	constexpr StokesEmissivity(Scalar ji, Scalar jq = static_cast<Scalar>(0.0), Scalar ju = static_cast<Scalar>(0.0), Scalar jv = static_cast<Scalar>(0.0)) noexcept
		: j_i(ji), j_q(jq), j_u(ju), j_v(jv) {}

	[[nodiscard]] constexpr StokesVector<Scalar> to_stokes_vector() const noexcept {
		return StokesVector<Scalar>(j_i, j_q, j_u, j_v);
	}
};

template <typename Scalar = double>
struct alignas(64) StokesTransferMatrix {
	Scalar alpha_i{static_cast<Scalar>(0.0)};
	Scalar alpha_q{static_cast<Scalar>(0.0)};
	Scalar alpha_u{static_cast<Scalar>(0.0)};
	Scalar alpha_v{static_cast<Scalar>(0.0)};
	Scalar rho_q{static_cast<Scalar>(0.0)};
	Scalar rho_u{static_cast<Scalar>(0.0)};
	Scalar rho_v{static_cast<Scalar>(0.0)};

	constexpr StokesTransferMatrix() noexcept = default;

	constexpr StokesTransferMatrix(
		Scalar ai, Scalar aq = static_cast<Scalar>(0.0), Scalar au = static_cast<Scalar>(0.0), Scalar av = static_cast<Scalar>(0.0),
		Scalar rq = static_cast<Scalar>(0.0), Scalar ru = static_cast<Scalar>(0.0), Scalar rv = static_cast<Scalar>(0.0)
	) noexcept
		: alpha_i(ai), alpha_q(aq), alpha_u(au), alpha_v(av), rho_q(rq), rho_u(ru), rho_v(rv) {}

	[[nodiscard]] constexpr StokesVector<Scalar> multiply(const StokesVector<Scalar>& s) const noexcept {
		return StokesVector<Scalar>(
			alpha_i * s.i + alpha_q * s.q + alpha_u * s.u + alpha_v * s.v,
			alpha_q * s.i + alpha_i * s.q + rho_v * s.u - rho_u * s.v,
			alpha_u * s.i - rho_v * s.q + alpha_i * s.u + rho_q * s.v,
			alpha_v * s.i + rho_u * s.q - rho_q * s.u + alpha_i * s.v
		);
	}

	[[nodiscard]] constexpr std::array<std::array<Scalar, 4>, 4> to_matrix_array() const noexcept {
		return {{
			{alpha_i, alpha_q, alpha_u, alpha_v},
			{alpha_q, alpha_i, rho_v, -rho_u},
			{alpha_u, -rho_v, alpha_i, rho_q},
			{alpha_v, rho_u, -rho_q, alpha_i}
		}};
	}
};

}

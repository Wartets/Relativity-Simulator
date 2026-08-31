#pragma once

#include <cstddef>
#include <cstdint>
#include <array>
#include <cmath>
#include <algorithm>

namespace Relativistic::Hydro {

template <typename Scalar = double>
struct alignas(64) PrimitiveVariables {
	Scalar rho{static_cast<Scalar>(1.0)};
	Scalar p{static_cast<Scalar>(1.0)};
	Scalar vx{static_cast<Scalar>(0.0)};
	Scalar vy{static_cast<Scalar>(0.0)};
	Scalar vz{static_cast<Scalar>(0.0)};
	Scalar bx{static_cast<Scalar>(0.0)};
	Scalar by{static_cast<Scalar>(0.0)};
	Scalar bz{static_cast<Scalar>(0.0)};
	Scalar eps{static_cast<Scalar>(1.5)};
	Scalar w{static_cast<Scalar>(1.0)};
	Scalar h{static_cast<Scalar>(2.5)};
	Scalar b2{static_cast<Scalar>(0.0)};

	constexpr PrimitiveVariables() noexcept = default;

	constexpr PrimitiveVariables(
		Scalar density,
		Scalar pressure,
		Scalar vel_x,
		Scalar vel_y = static_cast<Scalar>(0.0),
		Scalar vel_z = static_cast<Scalar>(0.0),
		Scalar b_x = static_cast<Scalar>(0.0),
		Scalar b_y = static_cast<Scalar>(0.0),
		Scalar b_z = static_cast<Scalar>(0.0)
	) noexcept
		: rho(density),
		  p(pressure),
		  vx(vel_x),
		  vy(vel_y),
		  vz(vel_z),
		  bx(b_x),
		  by(b_y),
		  bz(b_z) {
		recompute_derived();
	}

	constexpr void recompute_derived(Scalar gamma = static_cast<Scalar>(5.0 / 3.0)) noexcept {
		const Scalar v2 = vx * vx + vy * vy + vz * vz;
		const Scalar safe_v2 = (v2 < static_cast<Scalar>(1.0 - 1e-15)) ? v2 : static_cast<Scalar>(1.0 - 1e-15);
		w = static_cast<Scalar>(1.0) / std::sqrt(static_cast<Scalar>(1.0) - safe_v2);
		const Scalar b_sq = bx * bx + by * by + bz * bz;
		const Scalar bdotv = bx * vx + by * vy + bz * vz;
		b2 = b_sq / (w * w) + bdotv * bdotv;
		const Scalar rho_safe = (rho > static_cast<Scalar>(1e-15)) ? rho : static_cast<Scalar>(1e-15);
		eps = p / ((gamma - static_cast<Scalar>(1.0)) * rho_safe);
		h = static_cast<Scalar>(1.0) + (gamma * p) / ((gamma - static_cast<Scalar>(1.0)) * rho_safe);
	}

	[[nodiscard]] constexpr Scalar velocity_squared() const noexcept {
		return vx * vx + vy * vy + vz * vz;
	}

	[[nodiscard]] constexpr Scalar magnetic_squared() const noexcept {
		return bx * bx + by * by + bz * bz;
	}

	[[nodiscard]] constexpr Scalar b_dot_v() const noexcept {
		return bx * vx + by * vy + bz * vz;
	}

	[[nodiscard]] constexpr Scalar total_pressure() const noexcept {
		return p + static_cast<Scalar>(0.5) * b2;
	}

	[[nodiscard]] constexpr std::array<Scalar, 4> comoving_magnetic_vector() const noexcept {
		const Scalar bdotv = b_dot_v();
		const Scalar b0 = w * bdotv;
		return {b0, bx / w + b0 * vx, by / w + b0 * vy, bz / w + b0 * vz};
	}
};

template <typename Scalar = double>
struct alignas(64) ConservedVariables {
	Scalar d{static_cast<Scalar>(1.0)};
	Scalar sx{static_cast<Scalar>(0.0)};
	Scalar sy{static_cast<Scalar>(0.0)};
	Scalar sz{static_cast<Scalar>(0.0)};
	Scalar tau{static_cast<Scalar>(1.5)};
	Scalar bx{static_cast<Scalar>(0.0)};
	Scalar by{static_cast<Scalar>(0.0)};
	Scalar bz{static_cast<Scalar>(0.0)};

	constexpr ConservedVariables() noexcept = default;

	constexpr ConservedVariables(
		Scalar d_val,
		Scalar sx_val,
		Scalar sy_val,
		Scalar sz_val,
		Scalar tau_val,
		Scalar bx_val = static_cast<Scalar>(0.0),
		Scalar by_val = static_cast<Scalar>(0.0),
		Scalar bz_val = static_cast<Scalar>(0.0)
	) noexcept
		: d(d_val),
		  sx(sx_val),
		  sy(sy_val),
		  sz(sz_val),
		  tau(tau_val),
		  bx(bx_val),
		  by(by_val),
		  bz(bz_val) {}

	[[nodiscard]] constexpr ConservedVariables operator+(const ConservedVariables& rhs) const noexcept {
		return ConservedVariables(
			d + rhs.d, sx + rhs.sx, sy + rhs.sy, sz + rhs.sz, tau + rhs.tau,
			bx + rhs.bx, by + rhs.by, bz + rhs.bz
		);
	}

	[[nodiscard]] constexpr ConservedVariables operator-(const ConservedVariables& rhs) const noexcept {
		return ConservedVariables(
			d - rhs.d, sx - rhs.sx, sy - rhs.sy, sz - rhs.sz, tau - rhs.tau,
			bx - rhs.bx, by - rhs.by, bz - rhs.bz
		);
	}

	[[nodiscard]] constexpr ConservedVariables operator*(Scalar scalar) const noexcept {
		return ConservedVariables(
			d * scalar, sx * scalar, sy * scalar, sz * scalar, tau * scalar,
			bx * scalar, by * scalar, bz * scalar
		);
	}

	constexpr ConservedVariables& operator+=(const ConservedVariables& rhs) noexcept {
		d += rhs.d;
		sx += rhs.sx;
		sy += rhs.sy;
		sz += rhs.sz;
		tau += rhs.tau;
		bx += rhs.bx;
		by += rhs.by;
		bz += rhs.bz;
		return *this;
	}

	constexpr ConservedVariables& operator*=(Scalar scalar) noexcept {
		d *= scalar;
		sx *= scalar;
		sy *= scalar;
		sz *= scalar;
		tau *= scalar;
		bx *= scalar;
		by *= scalar;
		bz *= scalar;
		return *this;
	}
};

template <typename Scalar = double>
[[nodiscard]] inline constexpr ConservedVariables<Scalar> operator*(Scalar scalar, const ConservedVariables<Scalar>& cv) noexcept {
	return cv * scalar;
}

template <typename Scalar = double>
struct alignas(64) FluxVariables {
	Scalar fd{static_cast<Scalar>(0.0)};
	Scalar fsx{static_cast<Scalar>(0.0)};
	Scalar fsy{static_cast<Scalar>(0.0)};
	Scalar fsz{static_cast<Scalar>(0.0)};
	Scalar ftau{static_cast<Scalar>(0.0)};
	Scalar fbx{static_cast<Scalar>(0.0)};
	Scalar fby{static_cast<Scalar>(0.0)};
	Scalar fbz{static_cast<Scalar>(0.0)};

	constexpr FluxVariables() noexcept = default;

	constexpr FluxVariables(
		Scalar fd_v,
		Scalar fsx_v,
		Scalar fsy_v,
		Scalar fsz_v,
		Scalar ftau_v,
		Scalar fbx_v = static_cast<Scalar>(0.0),
		Scalar fby_v = static_cast<Scalar>(0.0),
		Scalar fbz_v = static_cast<Scalar>(0.0)
	) noexcept
		: fd(fd_v),
		  fsx(fsx_v),
		  fsy(fsy_v),
		  fsz(fsz_v),
		  ftau(ftau_v),
		  fbx(fbx_v),
		  fby(fby_v),
		  fbz(fbz_v) {}

	[[nodiscard]] constexpr FluxVariables operator+(const FluxVariables& rhs) const noexcept {
		return FluxVariables(
			fd + rhs.fd, fsx + rhs.fsx, fsy + rhs.fsy, fsz + rhs.fsz, ftau + rhs.ftau,
			fbx + rhs.fbx, fby + rhs.fby, fbz + rhs.fbz
		);
	}

	[[nodiscard]] constexpr FluxVariables operator-(const FluxVariables& rhs) const noexcept {
		return FluxVariables(
			fd - rhs.fd, fsx - rhs.fsx, fsy - rhs.fsy, fsz - rhs.fsz, ftau - rhs.ftau,
			fbx - rhs.fbx, fby - rhs.fby, fbz - rhs.fbz
		);
	}

	[[nodiscard]] constexpr FluxVariables operator*(Scalar scalar) const noexcept {
		return FluxVariables(
			fd * scalar, fsx * scalar, fsy * scalar, fsz * scalar, ftau * scalar,
			fbx * scalar, fby * scalar, fbz * scalar
		);
	}
};

template <typename Scalar = double>
struct Metric3Plus1 {
	Scalar alpha{static_cast<Scalar>(1.0)};
	std::array<Scalar, 3> beta{static_cast<Scalar>(0.0), static_cast<Scalar>(0.0), static_cast<Scalar>(0.0)};
	std::array<std::array<Scalar, 3>, 3> gamma{{{static_cast<Scalar>(1.0), 0.0, 0.0}, {0.0, static_cast<Scalar>(1.0), 0.0}, {0.0, 0.0, static_cast<Scalar>(1.0)}}};
	std::array<std::array<Scalar, 3>, 3> inv_gamma{{{static_cast<Scalar>(1.0), 0.0, 0.0}, {0.0, static_cast<Scalar>(1.0), 0.0}, {0.0, 0.0, static_cast<Scalar>(1.0)}}};
	Scalar sqrt_gamma{static_cast<Scalar>(1.0)};
	std::array<std::array<Scalar, 3>, 3> k_extrinsic{{{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}}};
};

template <typename Scalar = double>
[[nodiscard]] constexpr ConservedVariables<Scalar> prim_to_con_flat(const PrimitiveVariables<Scalar>& prim) noexcept {
	const Scalar w = prim.w;
	const Scalar d = prim.rho * w;
	const Scalar b_dot_v = prim.b_dot_v();
	const Scalar b2_raw = prim.magnetic_squared();
	const Scalar z = prim.rho * prim.h * w * w;
	const Scalar p_tot = prim.p + static_cast<Scalar>(0.5) * prim.b2;

	const Scalar sx = (z + b2_raw) * prim.vx - b_dot_v * prim.bx;
	const Scalar sy = (z + b2_raw) * prim.vy - b_dot_v * prim.by;
	const Scalar sz = (z + b2_raw) * prim.vz - b_dot_v * prim.bz;
	const Scalar tau = z + b2_raw - p_tot - d;

	return ConservedVariables<Scalar>(d, sx, sy, sz, tau, prim.bx, prim.by, prim.bz);
}

template <typename Scalar = double>
[[nodiscard]] constexpr FluxVariables<Scalar> compute_flux_1d_x(const PrimitiveVariables<Scalar>& prim) noexcept {
	const Scalar w = prim.w;
	const Scalar d = prim.rho * w;
	const Scalar w2 = w * w;
	const Scalar b_dot_v = prim.b_dot_v();
	const Scalar b2_raw = prim.magnetic_squared();
	const Scalar z = prim.rho * prim.h * w2;
	const Scalar p_tot = prim.p + static_cast<Scalar>(0.5) * prim.b2;

	const Scalar sx = (z + b2_raw) * prim.vx - b_dot_v * prim.bx;
	const Scalar sy = (z + b2_raw) * prim.vy - b_dot_v * prim.by;
	const Scalar sz = (z + b2_raw) * prim.vz - b_dot_v * prim.bz;

	const Scalar fd = d * prim.vx;
	const Scalar fsx = sx * prim.vx + p_tot - (prim.bx * prim.bx) / w2 - b_dot_v * prim.bx * prim.vx;
	const Scalar fsy = sy * prim.vx - (prim.bx * prim.by) / w2 - b_dot_v * prim.by * prim.vx;
	const Scalar fsz = sz * prim.vx - (prim.bx * prim.bz) / w2 - b_dot_v * prim.bz * prim.vx;
	const Scalar ftau = sx - fd;
	const Scalar fbx = static_cast<Scalar>(0.0);
	const Scalar fby = prim.by * prim.vx - prim.bx * prim.vy;
	const Scalar fbz = prim.bz * prim.vx - prim.bx * prim.vz;

	return FluxVariables<Scalar>(fd, fsx, fsy, fsz, ftau, fbx, fby, fbz);
}

}

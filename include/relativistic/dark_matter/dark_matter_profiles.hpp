#pragma once

#include "relativistic/core/constants.hpp"
#include <array>
#include <cmath>
#include <numbers>
#include <algorithm>
#include <concepts>

namespace Relativistic::DarkMatter {

template <typename Scalar = double>
class NFWProfile {
private:
	Scalar rho_0_{static_cast<Scalar>(1e7 * Core::PhysicalConstants<double>::SOLAR_MASS / (1000.0 * 1000.0 * 1000.0 * 3.085677581491367e16 * 3.085677581491367e16 * 3.085677581491367e16))};
	Scalar r_s_{static_cast<Scalar>(20.0 * 1000.0 * 3.085677581491367e16)};
	Scalar g_const_{static_cast<Scalar>(Core::PhysicalConstants<double>::GRAVITATIONAL_CONSTANT)};

public:
	constexpr NFWProfile() noexcept = default;

	constexpr NFWProfile(Scalar central_density, Scalar scale_radius, Scalar g = static_cast<Scalar>(Core::PhysicalConstants<double>::GRAVITATIONAL_CONSTANT)) noexcept
		: rho_0_(central_density), r_s_(scale_radius), g_const_(g) {}

	[[nodiscard]] static NFWProfile from_virial_parameters(
		Scalar m_200,
		Scalar c_200,
		Scalar h_param = static_cast<Scalar>(0.7),
		Scalar g = static_cast<Scalar>(Core::PhysicalConstants<double>::GRAVITATIONAL_CONSTANT)
	) noexcept {
		const Scalar h0 = h_param * static_cast<Scalar>(100.0 * 1000.0 / (1e6 * 3.085677581491367e16));
		const Scalar rho_crit = (static_cast<Scalar>(3.0) * h0 * h0) / (static_cast<Scalar>(8.0) * std::numbers::pi_v<Scalar> * g);
		const Scalar r_200 = std::cbrt((static_cast<Scalar>(3.0) * m_200) / (static_cast<Scalar>(800.0) * std::numbers::pi_v<Scalar> * rho_crit));
		const Scalar r_s = r_200 / c_200;
		const Scalar f_c = std::log(static_cast<Scalar>(1.0) + c_200) - (c_200 / (static_cast<Scalar>(1.0) + c_200));
		const Scalar rho_0 = (static_cast<Scalar>(200.0) / static_cast<Scalar>(3.0)) * rho_crit * (c_200 * c_200 * c_200) / f_c;
		return NFWProfile(rho_0, r_s, g);
	}

	[[nodiscard]] constexpr Scalar central_density() const noexcept { return rho_0_; }
	[[nodiscard]] constexpr Scalar scale_radius() const noexcept { return r_s_; }
	[[nodiscard]] constexpr Scalar gravitational_constant() const noexcept { return g_const_; }

	[[nodiscard]] Scalar density(Scalar r) const noexcept {
		const Scalar r_safe = std::max(r, static_cast<Scalar>(1e-12));
		const Scalar x = r_safe / r_s_;
		const Scalar denom = x * (static_cast<Scalar>(1.0) + x) * (static_cast<Scalar>(1.0) + x);
		return rho_0_ / denom;
	}

	[[nodiscard]] Scalar enclosed_mass(Scalar r) const noexcept {
		const Scalar r_safe = std::max(r, static_cast<Scalar>(1e-12));
		const Scalar x = r_safe / r_s_;
		const Scalar factor = std::log(static_cast<Scalar>(1.0) + x) - (x / (static_cast<Scalar>(1.0) + x));
		return static_cast<Scalar>(4.0) * std::numbers::pi_v<Scalar> * rho_0_ * (r_s_ * r_s_ * r_s_) * factor;
	}

	[[nodiscard]] Scalar gravitational_potential(Scalar r) const noexcept {
		const Scalar r_safe = std::max(r, static_cast<Scalar>(1e-12));
		const Scalar x = r_safe / r_s_;
		return -static_cast<Scalar>(4.0) * std::numbers::pi_v<Scalar> * g_const_ * rho_0_ * (r_s_ * r_s_ * r_s_) * std::log(static_cast<Scalar>(1.0) + x) / r_safe;
	}

	[[nodiscard]] Scalar radial_acceleration(Scalar r) const noexcept {
		const Scalar r_safe = std::max(r, static_cast<Scalar>(1e-12));
		const Scalar m_enc = enclosed_mass(r_safe);
		return -g_const_ * m_enc / (r_safe * r_safe);
	}

	[[nodiscard]] std::array<Scalar, 3> acceleration_3d(const std::array<Scalar, 3>& position) const noexcept {
		const Scalar r2 = position[0] * position[0] + position[1] * position[1] + position[2] * position[2];
		if (r2 <= static_cast<Scalar>(0.0)) {
			return {static_cast<Scalar>(0.0), static_cast<Scalar>(0.0), static_cast<Scalar>(0.0)};
		}
		const Scalar r = std::sqrt(r2);
		const Scalar a_rad = radial_acceleration(r);
		const Scalar inv_r = static_cast<Scalar>(1.0) / r;
		return {
			a_rad * position[0] * inv_r,
			a_rad * position[1] * inv_r,
			a_rad * position[2] * inv_r
		};
	}

	[[nodiscard]] Scalar circular_velocity(Scalar r) const noexcept {
		const Scalar r_safe = std::max(r, static_cast<Scalar>(1e-12));
		const Scalar m_enc = enclosed_mass(r_safe);
		return std::sqrt(g_const_ * m_enc / r_safe);
	}

	[[nodiscard]] Scalar max_circular_velocity() const noexcept {
		const Scalar r_max = static_cast<Scalar>(2.16258158706473) * r_s_;
		return circular_velocity(r_max);
	}

	[[nodiscard]] constexpr Scalar radius_of_max_velocity() const noexcept {
		return static_cast<Scalar>(2.16258158706473) * r_s_;
	}
};

template <typename Scalar = double>
class EinastoProfile {
private:
	Scalar rho_e_{static_cast<Scalar>(1e7 * Core::PhysicalConstants<double>::SOLAR_MASS / (1000.0 * 1000.0 * 1000.0 * 3.085677581491367e16 * 3.085677581491367e16 * 3.085677581491367e16))};
	Scalar r_e_{static_cast<Scalar>(20.0 * 1000.0 * 3.085677581491367e16)};
	Scalar alpha_{static_cast<Scalar>(0.16)};
	Scalar g_const_{static_cast<Scalar>(Core::PhysicalConstants<double>::GRAVITATIONAL_CONSTANT)};

	[[nodiscard]] static Scalar lower_incomplete_gamma(Scalar s, Scalar x, size_t max_terms = 120) noexcept {
		if (x <= static_cast<Scalar>(0.0)) return static_cast<Scalar>(0.0);
		if (x > static_cast<Scalar>(100.0)) {
			return std::tgamma(s);
		}

		Scalar sum = static_cast<Scalar>(1.0) / s;
		Scalar term = static_cast<Scalar>(1.0) / s;
		for (size_t n = 1; n < max_terms; ++n) {
			term *= x / (s + static_cast<Scalar>(n));
			sum += term;
			if (std::abs(term) < static_cast<Scalar>(1e-15) * std::abs(sum)) {
				break;
			}
		}
		return std::pow(x, s) * std::exp(-x) * sum;
	}

public:
	constexpr EinastoProfile() noexcept = default;

	constexpr EinastoProfile(Scalar rho_e, Scalar r_e, Scalar alpha = static_cast<Scalar>(0.16), Scalar g = static_cast<Scalar>(Core::PhysicalConstants<double>::GRAVITATIONAL_CONSTANT)) noexcept
		: rho_e_(rho_e), r_e_(r_e), alpha_(alpha), g_const_(g) {}

	[[nodiscard]] constexpr Scalar characteristic_density() const noexcept { return rho_e_; }
	[[nodiscard]] constexpr Scalar scale_radius() const noexcept { return r_e_; }
	[[nodiscard]] constexpr Scalar shape_index() const noexcept { return alpha_; }
	[[nodiscard]] constexpr Scalar gravitational_constant() const noexcept { return g_const_; }

	[[nodiscard]] Scalar density(Scalar r) const noexcept {
		const Scalar r_safe = std::max(r, static_cast<Scalar>(1e-12));
		const Scalar x = r_safe / r_e_;
		const Scalar exponent = -(static_cast<Scalar>(2.0) / alpha_) * (std::pow(x, alpha_) - static_cast<Scalar>(1.0));
		return rho_e_ * std::exp(exponent);
	}

	[[nodiscard]] Scalar enclosed_mass(Scalar r) const noexcept {
		const Scalar r_safe = std::max(r, static_cast<Scalar>(1e-12));
		const Scalar x = r_safe / r_e_;
		const Scalar d_alpha = static_cast<Scalar>(2.0) / alpha_;
		const Scalar s = static_cast<Scalar>(3.0) / alpha_;
		const Scalar y = d_alpha * std::pow(x, alpha_);
		const Scalar gamma_val = lower_incomplete_gamma(s, y);
		const Scalar factor = static_cast<Scalar>(4.0) * std::numbers::pi_v<Scalar> * rho_e_ * std::exp(d_alpha) * (r_e_ * r_e_ * r_e_) / alpha_;
		const Scalar power_d = std::pow(d_alpha, -s);
		return factor * power_d * gamma_val;
	}

	[[nodiscard]] Scalar radial_acceleration(Scalar r) const noexcept {
		const Scalar r_safe = std::max(r, static_cast<Scalar>(1e-12));
		const Scalar m_enc = enclosed_mass(r_safe);
		return -g_const_ * m_enc / (r_safe * r_safe);
	}

	[[nodiscard]] std::array<Scalar, 3> acceleration_3d(const std::array<Scalar, 3>& position) const noexcept {
		const Scalar r2 = position[0] * position[0] + position[1] * position[1] + position[2] * position[2];
		if (r2 <= static_cast<Scalar>(0.0)) {
			return {static_cast<Scalar>(0.0), static_cast<Scalar>(0.0), static_cast<Scalar>(0.0)};
		}
		const Scalar r = std::sqrt(r2);
		const Scalar a_rad = radial_acceleration(r);
		const Scalar inv_r = static_cast<Scalar>(1.0) / r;
		return {
			a_rad * position[0] * inv_r,
			a_rad * position[1] * inv_r,
			a_rad * position[2] * inv_r
		};
	}

	[[nodiscard]] Scalar circular_velocity(Scalar r) const noexcept {
		const Scalar r_safe = std::max(r, static_cast<Scalar>(1e-12));
		const Scalar m_enc = enclosed_mass(r_safe);
		return std::sqrt(g_const_ * m_enc / r_safe);
	}
};

template <typename Scalar = double>
class BurkertProfile {
private:
	Scalar rho_0_{static_cast<Scalar>(1e7 * Core::PhysicalConstants<double>::SOLAR_MASS / (1000.0 * 1000.0 * 1000.0 * 3.085677581491367e16 * 3.085677581491367e16 * 3.085677581491367e16))};
	Scalar r_0_{static_cast<Scalar>(10.0 * 1000.0 * 3.085677581491367e16)};
	Scalar g_const_{static_cast<Scalar>(Core::PhysicalConstants<double>::GRAVITATIONAL_CONSTANT)};

public:
	constexpr BurkertProfile() noexcept = default;

	constexpr BurkertProfile(Scalar rho_0, Scalar r_0, Scalar g = static_cast<Scalar>(Core::PhysicalConstants<double>::GRAVITATIONAL_CONSTANT)) noexcept
		: rho_0_(rho_0), r_0_(r_0), g_const_(g) {}

	[[nodiscard]] constexpr Scalar central_density() const noexcept { return rho_0_; }
	[[nodiscard]] constexpr Scalar core_radius() const noexcept { return r_0_; }
	[[nodiscard]] constexpr Scalar gravitational_constant() const noexcept { return g_const_; }

	[[nodiscard]] Scalar density(Scalar r) const noexcept {
		const Scalar r_safe = std::max(r, static_cast<Scalar>(1e-12));
		const Scalar x = r_safe / r_0_;
		const Scalar denom = (static_cast<Scalar>(1.0) + x) * (static_cast<Scalar>(1.0) + x * x);
		return rho_0_ / denom;
	}

	[[nodiscard]] Scalar enclosed_mass(Scalar r) const noexcept {
		const Scalar r_safe = std::max(r, static_cast<Scalar>(1e-12));
		const Scalar x = r_safe / r_0_;
		const Scalar factor = std::log(static_cast<Scalar>(1.0) + x) + static_cast<Scalar>(0.5) * std::log(static_cast<Scalar>(1.0) + x * x) - std::atan(x);
		return static_cast<Scalar>(2.0) * std::numbers::pi_v<Scalar> * rho_0_ * (r_0_ * r_0_ * r_0_) * factor;
	}

	[[nodiscard]] Scalar radial_acceleration(Scalar r) const noexcept {
		const Scalar r_safe = std::max(r, static_cast<Scalar>(1e-12));
		const Scalar m_enc = enclosed_mass(r_safe);
		return -g_const_ * m_enc / (r_safe * r_safe);
	}

	[[nodiscard]] std::array<Scalar, 3> acceleration_3d(const std::array<Scalar, 3>& position) const noexcept {
		const Scalar r2 = position[0] * position[0] + position[1] * position[1] + position[2] * position[2];
		if (r2 <= static_cast<Scalar>(0.0)) {
			return {static_cast<Scalar>(0.0), static_cast<Scalar>(0.0), static_cast<Scalar>(0.0)};
		}
		const Scalar r = std::sqrt(r2);
		const Scalar a_rad = radial_acceleration(r);
		const Scalar inv_r = static_cast<Scalar>(1.0) / r;
		return {
			a_rad * position[0] * inv_r,
			a_rad * position[1] * inv_r,
			a_rad * position[2] * inv_r
		};
	}

	[[nodiscard]] Scalar circular_velocity(Scalar r) const noexcept {
		const Scalar r_safe = std::max(r, static_cast<Scalar>(1e-12));
		const Scalar m_enc = enclosed_mass(r_safe);
		return std::sqrt(g_const_ * m_enc / r_safe);
	}
};

template <typename Scalar = double>
class HernquistProfile {
private:
	Scalar mass_{static_cast<Scalar>(1e11 * Core::PhysicalConstants<double>::SOLAR_MASS)};
	Scalar a_{static_cast<Scalar>(3.0 * 1000.0 * 3.085677581491367e16)};
	Scalar g_const_{static_cast<Scalar>(Core::PhysicalConstants<double>::GRAVITATIONAL_CONSTANT)};

public:
	constexpr HernquistProfile() noexcept = default;

	constexpr HernquistProfile(Scalar total_mass, Scalar scale_radius, Scalar g = static_cast<Scalar>(Core::PhysicalConstants<double>::GRAVITATIONAL_CONSTANT)) noexcept
		: mass_(total_mass), a_(scale_radius), g_const_(g) {}

	[[nodiscard]] constexpr Scalar total_mass() const noexcept { return mass_; }
	[[nodiscard]] constexpr Scalar scale_radius() const noexcept { return a_; }
	[[nodiscard]] constexpr Scalar gravitational_constant() const noexcept { return g_const_; }

	[[nodiscard]] Scalar density(Scalar r) const noexcept {
		const Scalar r_safe = std::max(r, static_cast<Scalar>(1e-12));
		const Scalar denom = static_cast<Scalar>(2.0) * std::numbers::pi_v<Scalar> * r_safe * (r_safe + a_) * (r_safe + a_) * (r_safe + a_);
		return (mass_ * a_) / denom;
	}

	[[nodiscard]] Scalar enclosed_mass(Scalar r) const noexcept {
		const Scalar r_safe = std::max(r, static_cast<Scalar>(1e-12));
		const Scalar ratio = r_safe / (r_safe + a_);
		return mass_ * ratio * ratio;
	}

	[[nodiscard]] Scalar gravitational_potential(Scalar r) const noexcept {
		const Scalar r_safe = std::max(r, static_cast<Scalar>(1e-12));
		return -(g_const_ * mass_) / (r_safe + a_);
	}

	[[nodiscard]] Scalar radial_acceleration(Scalar r) const noexcept {
		const Scalar r_safe = std::max(r, static_cast<Scalar>(1e-12));
		return -(g_const_ * mass_) / ((r_safe + a_) * (r_safe + a_));
	}

	[[nodiscard]] std::array<Scalar, 3> acceleration_3d(const std::array<Scalar, 3>& position) const noexcept {
		const Scalar r2 = position[0] * position[0] + position[1] * position[1] + position[2] * position[2];
		if (r2 <= static_cast<Scalar>(0.0)) {
			return {static_cast<Scalar>(0.0), static_cast<Scalar>(0.0), static_cast<Scalar>(0.0)};
		}
		const Scalar r = std::sqrt(r2);
		const Scalar a_rad = radial_acceleration(r);
		const Scalar inv_r = static_cast<Scalar>(1.0) / r;
		return {
			a_rad * position[0] * inv_r,
			a_rad * position[1] * inv_r,
			a_rad * position[2] * inv_r
		};
	}

	[[nodiscard]] Scalar circular_velocity(Scalar r) const noexcept {
		const Scalar r_safe = std::max(r, static_cast<Scalar>(1e-12));
		return std::sqrt((g_const_ * mass_ * r_safe) / ((r_safe + a_) * (r_safe + a_)));
	}
};

}

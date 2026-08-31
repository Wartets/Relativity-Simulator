#pragma once

#include <cmath>
#include <algorithm>
#include <concepts>
#include <type_traits>

namespace Relativistic::Hydro {

template <typename Scalar = double>
class IdealGasEOS {
private:
	Scalar gamma_{static_cast<Scalar>(5.0 / 3.0)};

public:
	constexpr IdealGasEOS() noexcept = default;

	explicit constexpr IdealGasEOS(Scalar adiabatic_index) noexcept
		: gamma_(adiabatic_index) {}

	[[nodiscard]] constexpr Scalar gamma() const noexcept {
		return gamma_;
	}

	[[nodiscard]] constexpr Scalar pressure(Scalar rho, Scalar eps) const noexcept {
		return (gamma_ - static_cast<Scalar>(1.0)) * rho * eps;
	}

	[[nodiscard]] constexpr Scalar specific_internal_energy(Scalar rho, Scalar p) const noexcept {
		const Scalar rho_safe = (rho > static_cast<Scalar>(1e-15)) ? rho : static_cast<Scalar>(1e-15);
		return p / ((gamma_ - static_cast<Scalar>(1.0)) * rho_safe);
	}

	[[nodiscard]] constexpr Scalar specific_enthalpy(Scalar rho, Scalar p) const noexcept {
		const Scalar rho_safe = (rho > static_cast<Scalar>(1e-15)) ? rho : static_cast<Scalar>(1e-15);
		return static_cast<Scalar>(1.0) + (gamma_ * p) / ((gamma_ - static_cast<Scalar>(1.0)) * rho_safe);
	}

	[[nodiscard]] constexpr Scalar sound_speed_squared(Scalar rho, Scalar p) const noexcept {
		const Scalar h = specific_enthalpy(rho, p);
		const Scalar rho_safe = (rho > static_cast<Scalar>(1e-15)) ? rho : static_cast<Scalar>(1e-15);
		const Scalar cs2 = (gamma_ * p) / (rho_safe * h);
		return std::clamp(cs2, static_cast<Scalar>(0.0), gamma_ - static_cast<Scalar>(1.0));
	}

	[[nodiscard]] Scalar sound_speed(Scalar rho, Scalar p) const noexcept {
		return std::sqrt(sound_speed_squared(rho, p));
	}

	[[nodiscard]] constexpr Scalar pressure_from_enthalpy(Scalar rho, Scalar h) const noexcept {
		return ((gamma_ - static_cast<Scalar>(1.0)) / gamma_) * rho * (h - static_cast<Scalar>(1.0));
	}

	[[nodiscard]] constexpr Scalar dp_drho_at_eps(Scalar, Scalar eps) const noexcept {
		return (gamma_ - static_cast<Scalar>(1.0)) * eps;
	}

	[[nodiscard]] constexpr Scalar dp_deps_at_rho(Scalar rho, Scalar) const noexcept {
		return (gamma_ - static_cast<Scalar>(1.0)) * rho;
	}
};

template <typename Scalar = double>
class SyngeEOS {
public:
	constexpr SyngeEOS() noexcept = default;

	[[nodiscard]] Scalar specific_enthalpy(Scalar rho, Scalar p) const noexcept {
		const Scalar rho_safe = (rho > static_cast<Scalar>(1e-15)) ? rho : static_cast<Scalar>(1e-15);
		const Scalar theta = p / rho_safe;
		return static_cast<Scalar>(2.5) * theta + std::sqrt(static_cast<Scalar>(2.25) * theta * theta + static_cast<Scalar>(1.0));
	}

	[[nodiscard]] Scalar pressure(Scalar rho, Scalar eps) const noexcept {
		const Scalar rho_safe = (rho > static_cast<Scalar>(1e-15)) ? rho : static_cast<Scalar>(1e-15);
		const Scalar h = static_cast<Scalar>(1.0) + eps;
		const Scalar theta = (h * h - static_cast<Scalar>(1.0)) / (static_cast<Scalar>(5.0) * h);
		return theta * rho_safe;
	}

	[[nodiscard]] Scalar sound_speed_squared(Scalar rho, Scalar p) const noexcept {
		const Scalar rho_safe = (rho > static_cast<Scalar>(1e-15)) ? rho : static_cast<Scalar>(1e-15);
		const Scalar theta = p / rho_safe;
		const Scalar h = specific_enthalpy(rho, p);
		const Scalar root_term = std::sqrt(static_cast<Scalar>(2.25) * theta * theta + static_cast<Scalar>(1.0));
		const Scalar dh_dtheta = static_cast<Scalar>(2.5) + (static_cast<Scalar>(2.25) * theta) / root_term;
		return std::clamp(theta / (h * (dh_dtheta - static_cast<Scalar>(1.0) / h)), static_cast<Scalar>(0.0), static_cast<Scalar>(1.0 / 3.0));
	}

	[[nodiscard]] Scalar sound_speed(Scalar rho, Scalar p) const noexcept {
		return std::sqrt(sound_speed_squared(rho, p));
	}
};

template <typename Scalar = double>
class PolytropicEOS {
private:
	Scalar k_{static_cast<Scalar>(100.0)};
	Scalar gamma_{static_cast<Scalar>(2.0)};

public:
	constexpr PolytropicEOS() noexcept = default;

	constexpr PolytropicEOS(Scalar polytropic_constant, Scalar gamma_index) noexcept
		: k_(polytropic_constant), gamma_(gamma_index) {}

	[[nodiscard]] constexpr Scalar gamma() const noexcept {
		return gamma_;
	}

	[[nodiscard]] Scalar pressure(Scalar rho) const noexcept {
		const Scalar rho_safe = (rho > static_cast<Scalar>(0.0)) ? rho : static_cast<Scalar>(0.0);
		return k_ * std::pow(rho_safe, gamma_);
	}

	[[nodiscard]] Scalar specific_enthalpy(Scalar rho) const noexcept {
		const Scalar rho_safe = (rho > static_cast<Scalar>(0.0)) ? rho : static_cast<Scalar>(0.0);
		return static_cast<Scalar>(1.0) + (gamma_ * k_ / (gamma_ - static_cast<Scalar>(1.0))) * std::pow(rho_safe, gamma_ - static_cast<Scalar>(1.0));
	}

	[[nodiscard]] Scalar sound_speed_squared(Scalar rho) const noexcept {
		const Scalar rho_safe = (rho > static_cast<Scalar>(1e-15)) ? rho : static_cast<Scalar>(1e-15);
		const Scalar p = pressure(rho_safe);
		const Scalar h = specific_enthalpy(rho_safe);
		return (gamma_ * p) / (rho_safe * h);
	}
};

}

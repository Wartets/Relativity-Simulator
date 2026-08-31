#pragma once

#include "relativistic/core/constants.hpp"
#include "relativistic/io/hdf5_serializer.hpp"
#include <cmath>
#include <numbers>
#include <algorithm>
#include <array>
#include <vector>
#include <string>
#include <string_view>
#include <optional>
#include <span>
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

	[[nodiscard]] constexpr Scalar energy_density(Scalar rho, Scalar eps) const noexcept {
		return rho * (static_cast<Scalar>(1.0) + eps);
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

	[[nodiscard]] constexpr Scalar gamma() const noexcept {
		return static_cast<Scalar>(4.0 / 3.0);
	}

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

	[[nodiscard]] constexpr Scalar energy_density(Scalar rho, Scalar eps) const noexcept {
		return rho * (static_cast<Scalar>(1.0) + eps);
	}

	[[nodiscard]] Scalar specific_internal_energy(Scalar rho, Scalar p) const noexcept {
		return specific_enthalpy(rho, p) - static_cast<Scalar>(1.0) - p / ((rho > static_cast<Scalar>(1e-15)) ? rho : static_cast<Scalar>(1e-15));
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

	[[nodiscard]] Scalar pressure_from_enthalpy(Scalar rho, Scalar h) const noexcept {
		const Scalar theta = (h * h - static_cast<Scalar>(1.0)) / (static_cast<Scalar>(5.0) * h);
		return theta * rho;
	}
};

template <typename Scalar = double>
class MathewsEOS {
public:
	constexpr MathewsEOS() noexcept = default;

	[[nodiscard]] constexpr Scalar gamma() const noexcept {
		return static_cast<Scalar>(5.0 / 3.0);
	}

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

	[[nodiscard]] constexpr Scalar energy_density(Scalar rho, Scalar eps) const noexcept {
		return rho * (static_cast<Scalar>(1.0) + eps);
	}

	[[nodiscard]] Scalar specific_internal_energy(Scalar rho, Scalar p) const noexcept {
		return specific_enthalpy(rho, p) - static_cast<Scalar>(1.0) - p / ((rho > static_cast<Scalar>(1e-15)) ? rho : static_cast<Scalar>(1e-15));
	}

	[[nodiscard]] Scalar sound_speed_squared(Scalar rho, Scalar p) const noexcept {
		const Scalar rho_safe = (rho > static_cast<Scalar>(1e-15)) ? rho : static_cast<Scalar>(1e-15);
		const Scalar theta = p / rho_safe;
		const Scalar h = specific_enthalpy(rho, p);
		const Scalar denom = static_cast<Scalar>(3.0) * h * std::sqrt(static_cast<Scalar>(2.25) * theta * theta + static_cast<Scalar>(1.0));
		const Scalar cs2 = (static_cast<Scalar>(5.0) * theta * (h - static_cast<Scalar>(1.0) / h)) / ((denom > static_cast<Scalar>(1e-15)) ? denom : static_cast<Scalar>(1e-15));
		return std::clamp(cs2, static_cast<Scalar>(0.0), static_cast<Scalar>(1.0 / 3.0));
	}

	[[nodiscard]] Scalar sound_speed(Scalar rho, Scalar p) const noexcept {
		return std::sqrt(sound_speed_squared(rho, p));
	}

	[[nodiscard]] Scalar pressure_from_enthalpy(Scalar rho, Scalar h) const noexcept {
		const Scalar theta = (h * h - static_cast<Scalar>(1.0)) / (static_cast<Scalar>(5.0) * h);
		return theta * rho;
	}
};

enum class FermiParticleType : uint32_t {
	Electron = 0,
	Neutron = 1,
	Proton = 2,
	Custom = 3
};

template <typename Scalar = double>
class RelativisticFermiGasEOS {
private:
	Scalar particle_mass_{static_cast<Scalar>(Core::PhysicalConstants<double>::NEUTRON_MASS)};
	Scalar mean_molecular_weight_{static_cast<Scalar>(1.0)};
	Scalar p0_{static_cast<Scalar>(1.0)};
	Scalar rho0_{static_cast<Scalar>(1.0)};
	Scalar c_{static_cast<Scalar>(Core::PhysicalConstants<double>::SPEED_OF_LIGHT)};
	Scalar hbar_{static_cast<Scalar>(Core::PhysicalConstants<double>::REDUCED_PLANCK_CONSTANT)};
	FermiParticleType particle_type_{FermiParticleType::Neutron};

	void recompute_scales() noexcept {
		const Scalar m = particle_mass_;
		const Scalar c = c_;
		const Scalar hbar = hbar_;
		const Scalar m4 = m * m * m * m;
		const Scalar c3 = c * c * c;
		const Scalar c5 = c3 * c * c;
		const Scalar hbar3 = hbar * hbar * hbar;
		const Scalar pi2 = std::numbers::pi_v<Scalar> * std::numbers::pi_v<Scalar>;

		p0_ = (m4 * c5) / (static_cast<Scalar>(24.0) * pi2 * hbar3);
		rho0_ = mean_molecular_weight_ * (m4 * c3) / (static_cast<Scalar>(3.0) * pi2 * hbar3);
	}

public:
	constexpr RelativisticFermiGasEOS() noexcept {
		recompute_scales();
	}

	explicit RelativisticFermiGasEOS(
		FermiParticleType type,
		Scalar mu_e = static_cast<Scalar>(2.0),
		Scalar speed_of_light = static_cast<Scalar>(Core::PhysicalConstants<double>::SPEED_OF_LIGHT),
		Scalar reduced_planck = static_cast<Scalar>(Core::PhysicalConstants<double>::REDUCED_PLANCK_CONSTANT)
	) noexcept
		: c_(speed_of_light), hbar_(reduced_planck), particle_type_(type) {
		if (type == FermiParticleType::Electron) {
			particle_mass_ = static_cast<Scalar>(Core::PhysicalConstants<double>::ELECTRON_MASS);
			mean_molecular_weight_ = mu_e * (static_cast<Scalar>(Core::PhysicalConstants<double>::PROTON_MASS) / particle_mass_);
		} else if (type == FermiParticleType::Proton) {
			particle_mass_ = static_cast<Scalar>(Core::PhysicalConstants<double>::PROTON_MASS);
			mean_molecular_weight_ = static_cast<Scalar>(1.0);
		} else {
			particle_mass_ = static_cast<Scalar>(Core::PhysicalConstants<double>::NEUTRON_MASS);
			mean_molecular_weight_ = static_cast<Scalar>(1.0);
		}
		recompute_scales();
	}

	[[nodiscard]] constexpr Scalar gamma() const noexcept {
		return static_cast<Scalar>(4.0 / 3.0);
	}

	[[nodiscard]] constexpr Scalar p0_scale() const noexcept {
		return p0_;
	}

	[[nodiscard]] constexpr Scalar rho0_scale() const noexcept {
		return rho0_;
	}

	[[nodiscard]] Scalar dimensionless_fermi_momentum(Scalar rho) const noexcept {
		const Scalar rho_safe = (rho > static_cast<Scalar>(0.0)) ? rho : static_cast<Scalar>(0.0);
		return std::cbrt(rho_safe / rho0_);
	}

	[[nodiscard]] Scalar pressure_from_x(Scalar x) const noexcept {
		if (x <= static_cast<Scalar>(0.0)) return static_cast<Scalar>(0.0);
		const Scalar x2 = x * x;
		const Scalar sqrt_term = std::sqrt(static_cast<Scalar>(1.0) + x2);
		const Scalar f_x = x * (static_cast<Scalar>(2.0) * x2 - static_cast<Scalar>(3.0)) * sqrt_term + static_cast<Scalar>(3.0) * std::asinh(x);
		return p0_ * f_x;
	}

	[[nodiscard]] Scalar kinetic_energy_density_from_x(Scalar x) const noexcept {
		if (x <= static_cast<Scalar>(0.0)) return static_cast<Scalar>(0.0);
		if (x < static_cast<Scalar>(0.05)) {
			const Scalar x2 = x * x;
			const Scalar x5 = x2 * x2 * x;
			return p0_ * (static_cast<Scalar>(12.0 / 5.0) * x5 * (static_cast<Scalar>(1.0) - static_cast<Scalar>(5.0 / 28.0) * x2 + static_cast<Scalar>(5.0 / 72.0) * x2 * x2));
		}
		const Scalar x2 = x * x;
		const Scalar sqrt_term = std::sqrt(static_cast<Scalar>(1.0) + x2);
		const Scalar g_x = static_cast<Scalar>(3.0) * x * (static_cast<Scalar>(2.0) * x2 + static_cast<Scalar>(1.0)) * sqrt_term - static_cast<Scalar>(3.0) * std::asinh(x);
		return p0_ * (g_x - static_cast<Scalar>(8.0) * x2 * x);
	}

	[[nodiscard]] Scalar pressure(Scalar rho) const noexcept {
		const Scalar x = dimensionless_fermi_momentum(rho);
		return pressure_from_x(x);
	}

	[[nodiscard]] Scalar pressure(Scalar rho, Scalar) const noexcept {
		return pressure(rho);
	}

	[[nodiscard]] Scalar energy_density(Scalar rho) const noexcept {
		const Scalar rho_safe = (rho > static_cast<Scalar>(0.0)) ? rho : static_cast<Scalar>(0.0);
		const Scalar x = dimensionless_fermi_momentum(rho_safe);
		return rho_safe * c_ * c_ + kinetic_energy_density_from_x(x);
	}

	[[nodiscard]] Scalar energy_density(Scalar rho, Scalar) const noexcept {
		return energy_density(rho);
	}

	[[nodiscard]] Scalar specific_enthalpy(Scalar rho, Scalar) const noexcept {
		const Scalar rho_safe = (rho > static_cast<Scalar>(1e-15)) ? rho : static_cast<Scalar>(1e-15);
		const Scalar p = pressure(rho_safe);
		const Scalar eps = energy_density(rho_safe);
		return (eps + p) / (rho_safe * c_ * c_);
	}

	[[nodiscard]] Scalar specific_internal_energy(Scalar rho, Scalar p) const noexcept {
		static_cast<void>(p);
		const Scalar rho_safe = (rho > static_cast<Scalar>(1e-15)) ? rho : static_cast<Scalar>(1e-15);
		const Scalar x = dimensionless_fermi_momentum(rho_safe);
		return kinetic_energy_density_from_x(x) / (rho_safe * c_ * c_);
	}

	[[nodiscard]] Scalar sound_speed_squared(Scalar rho, Scalar) const noexcept {
		const Scalar rho_safe = (rho > static_cast<Scalar>(1e-15)) ? rho : static_cast<Scalar>(1e-15);
		const Scalar x = dimensionless_fermi_momentum(rho_safe);
		if (x <= static_cast<Scalar>(0.0)) return static_cast<Scalar>(0.0);
		const Scalar x2 = x * x;
		const Scalar c2 = c_ * c_;
		return (c2 * x2) / (static_cast<Scalar>(3.0) * (static_cast<Scalar>(1.0) + x2));
	}

	[[nodiscard]] Scalar sound_speed(Scalar rho, Scalar p) const noexcept {
		return std::sqrt(sound_speed_squared(rho, p));
	}

	[[nodiscard]] Scalar density_from_pressure(Scalar p, Scalar tol = static_cast<Scalar>(1e-12)) const noexcept {
		if (p <= static_cast<Scalar>(0.0)) return static_cast<Scalar>(0.0);
		Scalar x = std::pow(p / p0_ * (static_cast<Scalar>(15.0) / static_cast<Scalar>(8.0)), static_cast<Scalar>(0.2));
		if (x > static_cast<Scalar>(1.0)) {
			x = std::pow(p / (static_cast<Scalar>(2.0) * p0_), static_cast<Scalar>(0.25));
		}
		for (size_t iter = 0; iter < 40; ++iter) {
			const Scalar f = pressure_from_x(x) - p;
			const Scalar df = p0_ * (static_cast<Scalar>(8.0) * x * x * x * x) / std::sqrt(static_cast<Scalar>(1.0) + x * x);
			const Scalar dx = f / ((std::abs(df) > static_cast<Scalar>(1e-30)) ? df : static_cast<Scalar>(1e-30));
			x -= dx;
			if (std::abs(dx) < tol * std::max(x, static_cast<Scalar>(1.0))) break;
		}
		x = std::max(x, static_cast<Scalar>(0.0));
		return rho0_ * x * x * x;
	}

	[[nodiscard]] Scalar pressure_from_enthalpy(Scalar, Scalar h) const noexcept {
		const Scalar safe_h = std::max(h, static_cast<Scalar>(1.0));
		const Scalar x = std::sqrt(safe_h * safe_h - static_cast<Scalar>(1.0));
		return pressure_from_x(x);
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

	[[nodiscard]] constexpr Scalar polytropic_constant() const noexcept {
		return k_;
	}

	[[nodiscard]] Scalar pressure(Scalar rho) const noexcept {
		const Scalar rho_safe = (rho > static_cast<Scalar>(0.0)) ? rho : static_cast<Scalar>(0.0);
		return k_ * std::pow(rho_safe, gamma_);
	}

	[[nodiscard]] Scalar pressure(Scalar rho, Scalar) const noexcept {
		return pressure(rho);
	}

	[[nodiscard]] Scalar energy_density(Scalar rho) const noexcept {
		const Scalar rho_safe = (rho > static_cast<Scalar>(0.0)) ? rho : static_cast<Scalar>(0.0);
		const Scalar p = pressure(rho_safe);
		return rho_safe + p / (gamma_ - static_cast<Scalar>(1.0));
	}

	[[nodiscard]] Scalar specific_enthalpy(Scalar rho, Scalar = static_cast<Scalar>(0.0)) const noexcept {
		const Scalar rho_safe = (rho > static_cast<Scalar>(0.0)) ? rho : static_cast<Scalar>(0.0);
		return static_cast<Scalar>(1.0) + (gamma_ * k_ / (gamma_ - static_cast<Scalar>(1.0))) * std::pow(rho_safe, gamma_ - static_cast<Scalar>(1.0));
	}

	[[nodiscard]] Scalar specific_internal_energy(Scalar rho, Scalar p) const noexcept {
		const Scalar rho_safe = (rho > static_cast<Scalar>(1e-15)) ? rho : static_cast<Scalar>(1e-15);
		return p / ((gamma_ - static_cast<Scalar>(1.0)) * rho_safe);
	}

	[[nodiscard]] Scalar sound_speed_squared(Scalar rho, Scalar = static_cast<Scalar>(0.0)) const noexcept {
		const Scalar rho_safe = (rho > static_cast<Scalar>(1e-15)) ? rho : static_cast<Scalar>(1e-15);
		const Scalar p = pressure(rho_safe);
		const Scalar h = specific_enthalpy(rho_safe);
		return (gamma_ * p) / (rho_safe * h);
	}

	[[nodiscard]] Scalar sound_speed(Scalar rho, Scalar p) const noexcept {
		return std::sqrt(sound_speed_squared(rho, p));
	}

	[[nodiscard]] Scalar density_from_pressure(Scalar p) const noexcept {
		if (p <= static_cast<Scalar>(0.0)) return static_cast<Scalar>(0.0);
		return std::pow(p / k_, static_cast<Scalar>(1.0) / gamma_);
	}

	[[nodiscard]] Scalar pressure_from_enthalpy(Scalar rho, Scalar h) const noexcept {
		return ((gamma_ - static_cast<Scalar>(1.0)) / gamma_) * rho * (h - static_cast<Scalar>(1.0));
	}
};

template <size_t NumPieces = 4, typename Scalar = double>
class PiecewisePolytropicEOS {
private:
	std::array<Scalar, NumPieces> gamma_{};
	std::array<Scalar, NumPieces - 1> rho_div_{};
	std::array<Scalar, NumPieces> k_{};
	std::array<Scalar, NumPieces> a_{};
	Scalar c_{static_cast<Scalar>(Core::PhysicalConstants<double>::SPEED_OF_LIGHT)};

public:
	constexpr PiecewisePolytropicEOS() noexcept = default;

	PiecewisePolytropicEOS(
		Scalar rho1,
		Scalar p1,
		std::span<const Scalar> gamma_indices,
		std::span<const Scalar> dividing_densities,
		Scalar speed_of_light = static_cast<Scalar>(Core::PhysicalConstants<double>::SPEED_OF_LIGHT)
	) : c_(speed_of_light) {
		const size_t n_pieces = std::min(gamma_indices.size(), NumPieces);
		for (size_t i = 0; i < n_pieces; ++i) {
			gamma_[i] = gamma_indices[i];
		}
		for (size_t i = 0; i < n_pieces - 1; ++i) {
			rho_div_[i] = dividing_densities[i];
		}

		const Scalar rho_0 = rho_div_[0];
		const Scalar rho_1 = (n_pieces >= 3) ? rho_div_[1] : rho1;
		const Scalar rho_2 = (n_pieces >= 4) ? rho_div_[2] : rho_1;

		k_[1] = p1 / std::pow(rho_1, gamma_[1]);
		const Scalar p0 = k_[1] * std::pow(rho_0, gamma_[1]);
		k_[0] = p0 / std::pow(rho_0, gamma_[0]);

		if (n_pieces >= 3) {
			k_[2] = p1 / std::pow(rho_1, gamma_[2]);
			if (n_pieces >= 4) {
				const Scalar p2 = k_[2] * std::pow(rho_2, gamma_[2]);
				k_[3] = p2 / std::pow(rho_2, gamma_[3]);
			}
		}

		const Scalar c2 = c_ * c_;
		a_[0] = static_cast<Scalar>(0.0);

		const Scalar term0 = (p0 / (rho_0 * c2)) * (static_cast<Scalar>(1.0) / (gamma_[0] - static_cast<Scalar>(1.0)) - static_cast<Scalar>(1.0) / (gamma_[1] - static_cast<Scalar>(1.0)));
		a_[1] = a_[0] + term0;

		if (n_pieces >= 3) {
			const Scalar term1 = (p1 / (rho_1 * c2)) * (static_cast<Scalar>(1.0) / (gamma_[1] - static_cast<Scalar>(1.0)) - static_cast<Scalar>(1.0) / (gamma_[2] - static_cast<Scalar>(1.0)));
			a_[2] = a_[1] + term1;

			if (n_pieces >= 4) {
				const Scalar p2 = k_[2] * std::pow(rho_2, gamma_[2]);
				const Scalar term2 = (p2 / (rho_2 * c2)) * (static_cast<Scalar>(1.0) / (gamma_[2] - static_cast<Scalar>(1.0)) - static_cast<Scalar>(1.0) / (gamma_[3] - static_cast<Scalar>(1.0)));
				a_[3] = a_[2] + term2;
			}
		}
	}

	[[nodiscard]] constexpr size_t find_piece(Scalar rho) const noexcept {
		size_t piece = 0;
		while (piece < NumPieces - 1 && rho > rho_div_[piece]) {
			++piece;
		}
		return piece;
	}

	[[nodiscard]] constexpr Scalar gamma() const noexcept {
		return gamma_[0];
	}

	[[nodiscard]] Scalar pressure(Scalar rho) const noexcept {
		const Scalar rho_safe = (rho > static_cast<Scalar>(0.0)) ? rho : static_cast<Scalar>(0.0);
		const size_t idx = find_piece(rho_safe);
		return k_[idx] * std::pow(rho_safe, gamma_[idx]);
	}

	[[nodiscard]] Scalar pressure(Scalar rho, Scalar) const noexcept {
		return pressure(rho);
	}

	[[nodiscard]] Scalar specific_internal_energy(Scalar rho) const noexcept {
		const Scalar rho_safe = (rho > static_cast<Scalar>(1e-15)) ? rho : static_cast<Scalar>(1e-15);
		const size_t idx = find_piece(rho_safe);
		const Scalar p = pressure(rho_safe);
		const Scalar c2 = c_ * c_;
		return a_[idx] * c2 + p / ((gamma_[idx] - static_cast<Scalar>(1.0)) * rho_safe);
	}

	[[nodiscard]] Scalar specific_internal_energy(Scalar rho, Scalar) const noexcept {
		return specific_internal_energy(rho);
	}

	[[nodiscard]] Scalar energy_density(Scalar rho) const noexcept {
		const Scalar rho_safe = (rho > static_cast<Scalar>(0.0)) ? rho : static_cast<Scalar>(0.0);
		const size_t idx = find_piece(rho_safe);
		const Scalar p = pressure(rho_safe);
		const Scalar c2 = c_ * c_;
		return (static_cast<Scalar>(1.0) + a_[idx]) * rho_safe * c2 + p / (gamma_[idx] - static_cast<Scalar>(1.0));
	}

	[[nodiscard]] Scalar energy_density(Scalar rho, Scalar) const noexcept {
		return energy_density(rho);
	}

	[[nodiscard]] Scalar specific_enthalpy(Scalar rho, Scalar = static_cast<Scalar>(0.0)) const noexcept {
		const Scalar rho_safe = (rho > static_cast<Scalar>(1e-15)) ? rho : static_cast<Scalar>(1e-15);
		const size_t idx = find_piece(rho_safe);
		const Scalar p = pressure(rho_safe);
		const Scalar c2 = c_ * c_;
		return static_cast<Scalar>(1.0) + a_[idx] + (gamma_[idx] * p) / ((gamma_[idx] - static_cast<Scalar>(1.0)) * rho_safe * c2);
	}

	[[nodiscard]] Scalar sound_speed_squared(Scalar rho, Scalar = static_cast<Scalar>(0.0)) const noexcept {
		const Scalar rho_safe = (rho > static_cast<Scalar>(1e-15)) ? rho : static_cast<Scalar>(1e-15);
		const size_t idx = find_piece(rho_safe);
		const Scalar p = pressure(rho_safe);
		const Scalar h = specific_enthalpy(rho_safe);
		return (gamma_[idx] * p) / (rho_safe * h);
	}

	[[nodiscard]] Scalar sound_speed(Scalar rho, Scalar p) const noexcept {
		return std::sqrt(sound_speed_squared(rho, p));
	}

	[[nodiscard]] Scalar density_from_pressure(Scalar p) const noexcept {
		if (p <= static_cast<Scalar>(0.0)) return static_cast<Scalar>(0.0);
		const Scalar p0 = k_[0] * std::pow(rho_div_[0], gamma_[0]);
		if (p <= p0) {
			return std::pow(p / k_[0], static_cast<Scalar>(1.0) / gamma_[0]);
		}
		const Scalar p1 = k_[1] * std::pow(rho_div_[1], gamma_[1]);
		if (p <= p1) {
			return std::pow(p / k_[1], static_cast<Scalar>(1.0) / gamma_[1]);
		}
		if constexpr (NumPieces >= 4) {
			const Scalar p2 = k_[2] * std::pow(rho_div_[2], gamma_[2]);
			if (p <= p2) {
				return std::pow(p / k_[2], static_cast<Scalar>(1.0) / gamma_[2]);
			}
			return std::pow(p / k_[3], static_cast<Scalar>(1.0) / gamma_[3]);
		} else {
			return std::pow(p / k_[2], static_cast<Scalar>(1.0) / gamma_[2]);
		}
	}

	[[nodiscard]] Scalar pressure_from_enthalpy(Scalar rho, Scalar h) const noexcept {
		const size_t idx = find_piece(rho);
		return ((gamma_[idx] - static_cast<Scalar>(1.0)) / gamma_[idx]) * rho * (h - static_cast<Scalar>(1.0));
	}
};

enum class NuclearPreset : uint32_t {
	SFHo = 0,
	Shen = 1,
	LS220 = 2,
	SLy4 = 3,
	APR4 = 4
};

template <typename Scalar = double>
class TabulatedNuclearEOS {
private:
	std::vector<Scalar> log_rho_{};
	std::vector<Scalar> log_temp_{};
	std::vector<Scalar> ye_grid_{};

	std::vector<Scalar> table_log_press_{};
	std::vector<Scalar> table_log_eps_{};
	std::vector<Scalar> table_enthalpy_{};
	std::vector<Scalar> table_cs2_{};

	size_t n_rho_{0};
	size_t n_temp_{0};
	size_t n_ye_{0};

	NuclearPreset preset_{NuclearPreset::SFHo};
	std::string model_name_{"SFHo"};
	Scalar c_{static_cast<Scalar>(Core::PhysicalConstants<double>::SPEED_OF_LIGHT)};

	[[nodiscard]] size_t index3d(size_t ir, size_t it, size_t iy) const noexcept {
		return (ir * n_temp_ + it) * n_ye_ + iy;
	}

public:
	TabulatedNuclearEOS() = default;

	void allocate_table(size_t nr, size_t nt, size_t ny) {
		n_rho_ = nr;
		n_temp_ = nt;
		n_ye_ = ny;
		const size_t total = nr * nt * ny;
		log_rho_.resize(nr);
		log_temp_.resize(nt);
		ye_grid_.resize(ny);
		table_log_press_.resize(total);
		table_log_eps_.resize(total);
		table_enthalpy_.resize(total);
		table_cs2_.resize(total);
	}

	[[nodiscard]] size_t num_rho() const noexcept { return n_rho_; }
	[[nodiscard]] size_t num_temp() const noexcept { return n_temp_; }
	[[nodiscard]] size_t num_ye() const noexcept { return n_ye_; }
	[[nodiscard]] const std::string& model_name() const noexcept { return model_name_; }
	[[nodiscard]] NuclearPreset preset() const noexcept { return preset_; }

	[[nodiscard]] constexpr Scalar gamma() const noexcept {
		return static_cast<Scalar>(2.8);
	}

	void set_grid_coordinates(
		std::span<const Scalar> l_rho,
		std::span<const Scalar> l_temp,
		std::span<const Scalar> ye
	) {
		allocate_table(l_rho.size(), l_temp.size(), ye.size());
		std::copy(l_rho.begin(), l_rho.end(), log_rho_.begin());
		std::copy(l_temp.begin(), l_temp.end(), log_temp_.begin());
		std::copy(ye.begin(), ye.end(), ye_grid_.begin());
	}

	void set_table_entry(
		size_t ir, size_t it, size_t iy,
		Scalar log_p, Scalar log_eps, Scalar h, Scalar cs2
	) noexcept {
		const size_t idx = index3d(ir, it, iy);
		table_log_press_[idx] = log_p;
		table_log_eps_[idx] = log_eps;
		table_enthalpy_[idx] = h;
		table_cs2_[idx] = cs2;
	}

	[[nodiscard]] Scalar interpolate_3d(
		const std::vector<Scalar>& table,
		Scalar log_rho,
		Scalar log_t,
		Scalar ye
	) const noexcept {
		if (n_rho_ == 0 || n_temp_ == 0 || n_ye_ == 0) return static_cast<Scalar>(0.0);

		const Scalar lr_clamped = std::clamp(log_rho, log_rho_.front(), log_rho_.back());
		const Scalar lt_clamped = std::clamp(log_t, log_temp_.front(), log_temp_.back());
		const Scalar ye_clamped = std::clamp(ye, ye_grid_.front(), ye_grid_.back());

		const Scalar d_lr = (n_rho_ > 1) ? ((log_rho_.back() - log_rho_.front()) / static_cast<Scalar>(n_rho_ - 1)) : static_cast<Scalar>(1.0);
		const Scalar norm_r = (lr_clamped - log_rho_.front()) / d_lr;
		const size_t ir0 = (n_rho_ > 1) ? std::min(static_cast<size_t>(norm_r), n_rho_ - 2) : 0;
		const size_t ir1 = (n_rho_ > 1) ? (ir0 + 1) : 0;
		const Scalar fr = (n_rho_ > 1) ? (norm_r - static_cast<Scalar>(ir0)) : static_cast<Scalar>(0.0);

		const Scalar d_lt = (n_temp_ > 1) ? ((log_temp_.back() - log_temp_.front()) / static_cast<Scalar>(n_temp_ - 1)) : static_cast<Scalar>(1.0);
		const Scalar norm_t = (lt_clamped - log_temp_.front()) / d_lt;
		const size_t it0 = (n_temp_ > 1) ? std::min(static_cast<size_t>(norm_t), n_temp_ - 2) : 0;
		const size_t it1 = (n_temp_ > 1) ? (it0 + 1) : 0;
		const Scalar ft = (n_temp_ > 1) ? (norm_t - static_cast<Scalar>(it0)) : static_cast<Scalar>(0.0);

		const Scalar d_ly = (n_ye_ > 1) ? ((ye_grid_.back() - ye_grid_.front()) / static_cast<Scalar>(n_ye_ - 1)) : static_cast<Scalar>(1.0);
		const Scalar norm_y = (ye_clamped - ye_grid_.front()) / d_ly;
		const size_t iy0 = (n_ye_ > 1) ? std::min(static_cast<size_t>(norm_y), n_ye_ - 2) : 0;
		const size_t iy1 = (n_ye_ > 1) ? (iy0 + 1) : 0;
		const Scalar fy = (n_ye_ > 1) ? (norm_y - static_cast<Scalar>(iy0)) : static_cast<Scalar>(0.0);

		const Scalar c000 = table[index3d(ir0, it0, iy0)];
		const Scalar c100 = table[index3d(ir1, it0, iy0)];
		const Scalar c010 = table[index3d(ir0, it1, iy0)];
		const Scalar c110 = table[index3d(ir1, it1, iy0)];
		const Scalar c001 = table[index3d(ir0, it0, iy1)];
		const Scalar c101 = table[index3d(ir1, it0, iy1)];
		const Scalar c011 = table[index3d(ir0, it1, iy1)];
		const Scalar c111 = table[index3d(ir1, it1, iy1)];

		const Scalar c00 = c000 * (static_cast<Scalar>(1.0) - fr) + c100 * fr;
		const Scalar c10 = c010 * (static_cast<Scalar>(1.0) - fr) + c110 * fr;
		const Scalar c01 = c001 * (static_cast<Scalar>(1.0) - fr) + c101 * fr;
		const Scalar c11 = c011 * (static_cast<Scalar>(1.0) - fr) + c111 * fr;

		const Scalar c0 = c00 * (static_cast<Scalar>(1.0) - ft) + c10 * ft;
		const Scalar c1 = c01 * (static_cast<Scalar>(1.0) - ft) + c11 * ft;

		return c0 * (static_cast<Scalar>(1.0) - fy) + c1 * fy;
	}

	[[nodiscard]] Scalar pressure(Scalar rho) const noexcept {
		if (rho <= static_cast<Scalar>(0.0) || log_rho_.empty()) return static_cast<Scalar>(0.0);
		const Scalar log_r = std::log10(std::max(rho, static_cast<Scalar>(1e-15)));
		const Scalar log_p = interpolate_3d(table_log_press_, log_r, log_temp_.front(), ye_grid_.front());
		return std::pow(static_cast<Scalar>(10.0), log_p);
	}

	[[nodiscard]] Scalar pressure(Scalar rho, Scalar) const noexcept {
		return pressure(rho);
	}

	[[nodiscard]] Scalar pressure_3d(Scalar rho, Scalar temp_mev, Scalar ye) const noexcept {
		if (rho <= static_cast<Scalar>(0.0) || log_rho_.empty()) return static_cast<Scalar>(0.0);
		const Scalar log_r = std::log10(std::max(rho, static_cast<Scalar>(1e-15)));
		const Scalar log_t = std::log10(std::max(temp_mev, static_cast<Scalar>(1e-4)));
		const Scalar log_p = interpolate_3d(table_log_press_, log_r, log_t, ye);
		return std::pow(static_cast<Scalar>(10.0), log_p);
	}

	[[nodiscard]] Scalar energy_density(Scalar rho) const noexcept {
		if (rho <= static_cast<Scalar>(0.0) || log_rho_.empty()) return static_cast<Scalar>(0.0);
		const Scalar log_r = std::log10(std::max(rho, static_cast<Scalar>(1e-15)));
		const Scalar log_eps = interpolate_3d(table_log_eps_, log_r, log_temp_.front(), ye_grid_.front());
		return std::pow(static_cast<Scalar>(10.0), log_eps);
	}

	[[nodiscard]] Scalar energy_density(Scalar rho, Scalar) const noexcept {
		return energy_density(rho);
	}

	[[nodiscard]] Scalar energy_density_3d(Scalar rho, Scalar temp_mev, Scalar ye) const noexcept {
		if (rho <= static_cast<Scalar>(0.0) || log_rho_.empty()) return static_cast<Scalar>(0.0);
		const Scalar log_r = std::log10(std::max(rho, static_cast<Scalar>(1e-15)));
		const Scalar log_t = std::log10(std::max(temp_mev, static_cast<Scalar>(1e-4)));
		const Scalar log_eps = interpolate_3d(table_log_eps_, log_r, log_t, ye);
		return std::pow(static_cast<Scalar>(10.0), log_eps);
	}

	[[nodiscard]] Scalar specific_enthalpy(Scalar rho, Scalar = static_cast<Scalar>(0.0)) const noexcept {
		if (rho <= static_cast<Scalar>(0.0) || log_rho_.empty()) return static_cast<Scalar>(1.0);
		const Scalar log_r = std::log10(std::max(rho, static_cast<Scalar>(1e-15)));
		return interpolate_3d(table_enthalpy_, log_r, static_cast<Scalar>(-2.0), static_cast<Scalar>(0.1));
	}

	[[nodiscard]] Scalar specific_internal_energy(Scalar rho, Scalar) const noexcept {
		const Scalar rho_safe = (rho > static_cast<Scalar>(1e-15)) ? rho : static_cast<Scalar>(1e-15);
		const Scalar eps_tot = energy_density(rho);
		return (eps_tot / (rho_safe * c_ * c_)) - static_cast<Scalar>(1.0);
	}

	[[nodiscard]] Scalar sound_speed_squared(Scalar rho, Scalar = static_cast<Scalar>(0.0)) const noexcept {
		if (rho <= static_cast<Scalar>(0.0) || log_rho_.empty()) return static_cast<Scalar>(0.0);
		const Scalar log_r = std::log10(std::max(rho, static_cast<Scalar>(1e-15)));
		const Scalar cs2_rel = std::clamp(interpolate_3d(table_cs2_, log_r, static_cast<Scalar>(-2.0), static_cast<Scalar>(0.1)), static_cast<Scalar>(0.0), static_cast<Scalar>(1.0));
		return cs2_rel * c_ * c_;
	}

	[[nodiscard]] Scalar sound_speed(Scalar rho, Scalar p) const noexcept {
		return std::sqrt(sound_speed_squared(rho, p));
	}

	[[nodiscard]] Scalar density_from_pressure(Scalar p) const noexcept {
		if (p <= static_cast<Scalar>(0.0) || log_rho_.empty()) return static_cast<Scalar>(0.0);
		const Scalar log_target_p = std::log10(p);

		if (log_target_p <= table_log_press_[index3d(0, 0, 0)]) {
			return std::pow(static_cast<Scalar>(10.0), log_rho_.front());
		}
		if (log_target_p >= table_log_press_[index3d(n_rho_ - 1, 0, 0)]) {
			return std::pow(static_cast<Scalar>(10.0), log_rho_.back());
		}

		size_t low_idx = 0;
		size_t high_idx = n_rho_ - 1;

		while (high_idx > low_idx + 1) {
			const size_t mid = (low_idx + high_idx) / 2;
			const Scalar mid_p = table_log_press_[index3d(mid, 0, 0)];
			if (mid_p < log_target_p) {
				low_idx = mid;
			} else {
				high_idx = mid;
			}
		}

		const Scalar p_l = table_log_press_[index3d(low_idx, 0, 0)];
		const Scalar p_h = table_log_press_[index3d(high_idx, 0, 0)];
		const Scalar frac = (p_h > p_l) ? (log_target_p - p_l) / (p_h - p_l) : static_cast<Scalar>(0.0);
		const Scalar log_r = log_rho_[low_idx] + frac * (log_rho_[high_idx] - log_rho_[low_idx]);

		return std::pow(static_cast<Scalar>(10.0), log_r);
	}

	[[nodiscard]] Scalar pressure_from_enthalpy(Scalar rho, Scalar h) const noexcept {
		return (rho * (h - static_cast<Scalar>(1.0)) * (static_cast<Scalar>(2.8) - static_cast<Scalar>(1.0))) / static_cast<Scalar>(2.8);
	}

	[[nodiscard]] IO::Hdf5Container to_hdf5_container() const {
		IO::Hdf5Container container;

		std::vector<double> r_data(log_rho_.begin(), log_rho_.end());
		std::vector<double> t_data(log_temp_.begin(), log_temp_.end());
		std::vector<double> y_data(ye_grid_.begin(), ye_grid_.end());
		std::vector<double> p_data(table_log_press_.begin(), table_log_press_.end());
		std::vector<double> e_data(table_log_eps_.begin(), table_log_eps_.end());
		std::vector<double> h_data(table_enthalpy_.begin(), table_enthalpy_.end());
		std::vector<double> cs_data(table_cs2_.begin(), table_cs2_.end());

		auto add_1d_ds = [&](std::string_view path, const std::vector<double>& vec) {
			IO::Hdf5Dataset ds;
			ds.path = std::string(path);
			ds.type = IO::Hdf5DataType::Float64;
			ds.dimensions = {vec.size()};
			ds.raw_data.resize(vec.size() * sizeof(double));
			std::memcpy(ds.raw_data.data(), vec.data(), ds.raw_data.size());
			container.add_dataset(std::move(ds));
		};

		auto add_3d_ds = [&](std::string_view path, const std::vector<double>& vec) {
			IO::Hdf5Dataset ds;
			ds.path = std::string(path);
			ds.type = IO::Hdf5DataType::Float64;
			ds.dimensions = {n_rho_, n_temp_, n_ye_};
			ds.raw_data.resize(vec.size() * sizeof(double));
			std::memcpy(ds.raw_data.data(), vec.data(), ds.raw_data.size());
			container.add_dataset(std::move(ds));
		};

		add_1d_ds("/eos/log_rho", r_data);
		add_1d_ds("/eos/log_temp", t_data);
		add_1d_ds("/eos/ye", y_data);
		add_3d_ds("/eos/log_press", p_data);
		add_3d_ds("/eos/log_eps", e_data);
		add_3d_ds("/eos/enthalpy", h_data);
		add_3d_ds("/eos/cs2", cs_data);

		return container;
	}

	[[nodiscard]] static std::optional<TabulatedNuclearEOS> from_hdf5_container(const IO::Hdf5Container& container) noexcept {
		const auto ds_r = container.get_dataset("/eos/log_rho");
		const auto ds_t = container.get_dataset("/eos/log_temp");
		const auto ds_y = container.get_dataset("/eos/ye");
		const auto ds_p = container.get_dataset("/eos/log_press");
		const auto ds_e = container.get_dataset("/eos/log_eps");
		const auto ds_h = container.get_dataset("/eos/enthalpy");
		const auto ds_cs = container.get_dataset("/eos/cs2");

		if (!ds_r || !ds_t || !ds_y || !ds_p || !ds_e || !ds_h || !ds_cs) {
			return std::nullopt;
		}

		TabulatedNuclearEOS eos;
		const auto r_span = ds_r->as_span<double>();
		const auto t_span = ds_t->as_span<double>();
		const auto y_span = ds_y->as_span<double>();

		eos.allocate_table(r_span.size(), t_span.size(), y_span.size());
		for (size_t i = 0; i < r_span.size(); ++i) eos.log_rho_[i] = static_cast<Scalar>(r_span[i]);
		for (size_t i = 0; i < t_span.size(); ++i) eos.log_temp_[i] = static_cast<Scalar>(t_span[i]);
		for (size_t i = 0; i < y_span.size(); ++i) eos.ye_grid_[i] = static_cast<Scalar>(y_span[i]);

		const auto p_span = ds_p->as_span<double>();
		const auto e_span = ds_e->as_span<double>();
		const auto h_span = ds_h->as_span<double>();
		const auto cs_span = ds_cs->as_span<double>();

		const size_t total = eos.n_rho_ * eos.n_temp_ * eos.n_ye_;
		for (size_t i = 0; i < total; ++i) {
			eos.table_log_press_[i] = static_cast<Scalar>(p_span[i]);
			eos.table_log_eps_[i] = static_cast<Scalar>(e_span[i]);
			eos.table_enthalpy_[i] = static_cast<Scalar>(h_span[i]);
			eos.table_cs2_[i] = static_cast<Scalar>(cs_span[i]);
		}

		return eos;
	}

	[[nodiscard]] static TabulatedNuclearEOS create_preset(
		NuclearPreset preset,
		size_t nr = 1600,
		size_t nt = 20,
		size_t ny = 20
	) noexcept {
		TabulatedNuclearEOS eos;
		eos.preset_ = preset;
		eos.allocate_table(nr, nt, ny);

		Scalar log_p1 = static_cast<Scalar>(34.331);
		Scalar gamma1 = static_cast<Scalar>(3.018);
		Scalar gamma2 = static_cast<Scalar>(2.898);
		Scalar gamma3 = static_cast<Scalar>(2.628);

		if (preset == NuclearPreset::SFHo) {
			eos.model_name_ = "SFHo";
			log_p1 = static_cast<Scalar>(34.392);
			gamma1 = static_cast<Scalar>(3.030);
			gamma2 = static_cast<Scalar>(2.920);
			gamma3 = static_cast<Scalar>(2.650);
		} else if (preset == NuclearPreset::Shen) {
			eos.model_name_ = "Shen";
			log_p1 = static_cast<Scalar>(34.664);
			gamma1 = static_cast<Scalar>(2.999);
			gamma2 = static_cast<Scalar>(2.766);
			gamma3 = static_cast<Scalar>(2.656);
		} else if (preset == NuclearPreset::LS220) {
			eos.model_name_ = "LS220";
			log_p1 = static_cast<Scalar>(34.490);
			gamma1 = static_cast<Scalar>(2.980);
			gamma2 = static_cast<Scalar>(2.760);
			gamma3 = static_cast<Scalar>(2.620);
		} else if (preset == NuclearPreset::APR4) {
			eos.model_name_ = "APR4";
			log_p1 = static_cast<Scalar>(34.437);
			gamma1 = static_cast<Scalar>(3.442);
			gamma2 = static_cast<Scalar>(3.256);
			gamma3 = static_cast<Scalar>(2.599);
		} else {
			eos.model_name_ = "SLy4";
			log_p1 = static_cast<Scalar>(34.384);
			gamma1 = static_cast<Scalar>(3.005);
			gamma2 = static_cast<Scalar>(2.988);
			gamma3 = static_cast<Scalar>(2.851);
		}

		const Scalar rho_0 = static_cast<Scalar>(1.9952623149688795e17);
		const Scalar rho_1 = static_cast<Scalar>(5.011872336272722e17);
		const Scalar rho_2 = static_cast<Scalar>(1.0e18);

		const Scalar p1_cgs = std::pow(static_cast<Scalar>(10.0), log_p1);
		const Scalar p1_si = p1_cgs * static_cast<Scalar>(0.1);

		const std::array<Scalar, 4> gammas = {static_cast<Scalar>(1.35692395), gamma1, gamma2, gamma3};
		const std::array<Scalar, 3> rho_divs = {rho_0, rho_1, rho_2};

		const PiecewisePolytropicEOS<4, Scalar> pp_eos(rho_1, p1_si, gammas, rho_divs, eos.c_);

		const Scalar log_r_min = static_cast<Scalar>(3.0);
		const Scalar log_r_max = static_cast<Scalar>(19.0);
		const Scalar d_lr = (log_r_max - log_r_min) / static_cast<Scalar>(nr - 1);

		for (size_t i = 0; i < nr; ++i) {
			eos.log_rho_[i] = log_r_min + static_cast<Scalar>(i) * d_lr;
		}

		const Scalar log_t_min = static_cast<Scalar>(-3.0);
		const Scalar log_t_max = static_cast<Scalar>(2.5);
		const Scalar d_lt = (log_t_max - log_t_min) / static_cast<Scalar>(nt - 1);

		for (size_t i = 0; i < nt; ++i) {
			eos.log_temp_[i] = log_t_min + static_cast<Scalar>(i) * d_lt;
		}

		const Scalar ye_min = static_cast<Scalar>(0.01);
		const Scalar ye_max = static_cast<Scalar>(0.60);
		const Scalar d_ye = (ye_max - ye_min) / static_cast<Scalar>(ny - 1);

		for (size_t i = 0; i < ny; ++i) {
			eos.ye_grid_[i] = ye_min + static_cast<Scalar>(i) * d_ye;
		}

		for (size_t ir = 0; ir < nr; ++ir) {
			const Scalar r_val = std::pow(static_cast<Scalar>(10.0), eos.log_rho_[ir]);
			const Scalar p_val = pp_eos.pressure(r_val);
			const Scalar eps_val = pp_eos.energy_density(r_val);
			const Scalar h_val = pp_eos.specific_enthalpy(r_val);
			const Scalar cs2_val = pp_eos.sound_speed_squared(r_val);

			const Scalar log_p_val = std::log10(std::max(p_val, static_cast<Scalar>(1e-30)));
			const Scalar log_eps_val = std::log10(std::max(eps_val, static_cast<Scalar>(1e-30)));

			for (size_t it = 0; it < nt; ++it) {
				const Scalar temp_mev = std::pow(static_cast<Scalar>(10.0), eos.log_temp_[it]);
				const Scalar thermal_p_ratio = static_cast<Scalar>(1.0) + static_cast<Scalar>(0.005) * temp_mev;
				const Scalar p_th = log_p_val + std::log10(thermal_p_ratio);

				for (size_t iy = 0; iy < ny; ++iy) {
					eos.set_table_entry(ir, it, iy, p_th, log_eps_val, h_val, cs2_val);
				}
			}
		}

		return eos;
	}
};

}

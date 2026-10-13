#pragma once

#include "relativistic/core/constants.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <numbers>

namespace Relativistic::Semiclassical {

using Constants = Core::PhysicalConstants<double>;

enum class EmissionSpectrum : uint32_t {
	PhotonsOnly = 0,
	MasslessStandardModel = 1,
	Custom = 2
};

enum class StressTensorModel : uint32_t {
	BlackbodyGreybody = 0,
	PolyakovSWave = 1
};

inline constexpr std::array<const char*, 3> kEmissionSpectrumNames{
	"Photons Only (2 degrees of freedom)",
	"Massless Photons, Neutrinos And Gravitons (9.25 effective degrees of freedom)",
	"Custom Effective Degrees Of Freedom"
};

inline constexpr std::array<const char*, 2> kStressTensorModelNames{
	"Greybody Blackbody Over The Horizon Area",
	"Polyakov 2D Renormalized Stress Tensor (Unruh State)"
};

struct EvaporationSettings {
	bool enabled{false};
	EmissionSpectrum spectrum{EmissionSpectrum::PhotonsOnly};
	StressTensorModel model{StressTensorModel::BlackbodyGreybody};
	double custom_degrees_of_freedom{2.0};
	double greybody_factor{1.0};
	double time_acceleration{1.0};
	double spin_down_ratio{3.0};
	double minimum_mass_kg{2.176434e-8};
	double detector_gap_ev{1.0e-6};
	double detector_coupling{1.0};

	[[nodiscard]] double degrees_of_freedom() const noexcept {
		switch (spectrum) {
			case EmissionSpectrum::PhotonsOnly: return 2.0;
			case EmissionSpectrum::MasslessStandardModel: return 2.0 + 6.0 * 7.0 / 8.0 + 2.0;
			case EmissionSpectrum::Custom:
			default: return custom_degrees_of_freedom;
		}
	}

	void sanitize() noexcept {
		const auto finite_or = [](double value, double fallback) noexcept { return std::isfinite(value) ? value : fallback; };
		spectrum = static_cast<EmissionSpectrum>(std::min<uint32_t>(static_cast<uint32_t>(spectrum), 2U));
		model = static_cast<StressTensorModel>(std::min<uint32_t>(static_cast<uint32_t>(model), 1U));
		custom_degrees_of_freedom = std::clamp(finite_or(custom_degrees_of_freedom, 2.0), 0.1, 1000.0);
		greybody_factor = std::clamp(finite_or(greybody_factor, 1.0), 0.01, 100.0);
		time_acceleration = std::clamp(finite_or(time_acceleration, 1.0), 1.0, 1.0e60);
		spin_down_ratio = std::clamp(finite_or(spin_down_ratio, 3.0), 0.0, 10.0);
		minimum_mass_kg = std::clamp(finite_or(minimum_mass_kg, 2.176434e-8), 1.0e-12, 1.0e40);
		detector_gap_ev = std::clamp(finite_or(detector_gap_ev, 1.0e-6), 1.0e-15, 1.0e12);
		detector_coupling = std::clamp(finite_or(detector_coupling, 1.0), 1.0e-6, 1.0e3);
	}
};

struct HorizonThermodynamics {
	double outer_radius_m{0.0};
	double inner_radius_m{0.0};
	double surface_gravity{0.0};
	double temperature_k{0.0};
	double area_m2{0.0};
};

class KerrHorizon {
public:
	[[nodiscard]] static HorizonThermodynamics evaluate(double mass_kg, double spin_star) noexcept {
		HorizonThermodynamics result;
		const double rg = Constants::GRAVITATIONAL_CONSTANT * mass_kg / (Constants::SPEED_OF_LIGHT * Constants::SPEED_OF_LIGHT);
		const double a = std::clamp(std::abs(spin_star), 0.0, 0.999999) * rg;
		const double root = std::sqrt(std::max(rg * rg - a * a, 0.0));
		result.outer_radius_m = rg + root;
		result.inner_radius_m = rg - root;
		const double denominator = 2.0 * (result.outer_radius_m * result.outer_radius_m + a * a);
		result.surface_gravity = (denominator > 0.0) ? (Constants::SPEED_OF_LIGHT * Constants::SPEED_OF_LIGHT * (result.outer_radius_m - result.inner_radius_m) / denominator) : 0.0;
		result.temperature_k = Constants::REDUCED_PLANCK_CONSTANT * result.surface_gravity / (2.0 * std::numbers::pi_v<double> * Constants::SPEED_OF_LIGHT * Constants::BOLTZMANN_CONSTANT);
		result.area_m2 = 4.0 * std::numbers::pi_v<double> * (result.outer_radius_m * result.outer_radius_m + a * a);
		return result;
	}
};

struct NullFluxes {
	double outgoing_at_infinity{0.0};
	double ingoing_at_horizon{0.0};
};

class RenormalizedStressTensor {
public:
	[[nodiscard]] static double thermal_flux_2d(double temperature_k) noexcept {
		const double kb = Constants::BOLTZMANN_CONSTANT;
		return std::numbers::pi_v<double> * kb * kb * temperature_k * temperature_k / (12.0 * Constants::REDUCED_PLANCK_CONSTANT);
	}

	[[nodiscard]] static double two_dimensional_luminosity(double surface_gravity) noexcept {
		const double c = Constants::SPEED_OF_LIGHT;
		return Constants::REDUCED_PLANCK_CONSTANT * surface_gravity * surface_gravity / (48.0 * std::numbers::pi_v<double> * c * c);
	}

	[[nodiscard]] static NullFluxes unruh_state_fluxes(double surface_gravity) noexcept {
		const double flux = two_dimensional_luminosity(surface_gravity);
		return NullFluxes{flux, -flux};
	}
};

class HawkingRadiation {
public:
	[[nodiscard]] static double luminosity_watts(double mass_kg, double spin_star, const EvaporationSettings& settings) noexcept {
		if (!(mass_kg > 0.0)) {
			return 0.0;
		}
		const HorizonThermodynamics horizon = KerrHorizon::evaluate(mass_kg, spin_star);
		const double dof = settings.degrees_of_freedom();
		if (settings.model == StressTensorModel::PolyakovSWave) {
			return dof * settings.greybody_factor * RenormalizedStressTensor::two_dimensional_luminosity(horizon.surface_gravity);
		}
		const double t2 = horizon.temperature_k * horizon.temperature_k;
		return 0.5 * dof * settings.greybody_factor * Constants::STEFAN_BOLTZMANN_CONSTANT * horizon.area_m2 * t2 * t2;
	}

	[[nodiscard]] static double mass_loss_rate(double mass_kg, double spin_star, const EvaporationSettings& settings) noexcept {
		return -luminosity_watts(mass_kg, spin_star, settings) / (Constants::SPEED_OF_LIGHT * Constants::SPEED_OF_LIGHT);
	}

	[[nodiscard]] static double lifetime_seconds(double mass_kg, const EvaporationSettings& settings) noexcept {
		const double rate_coefficient = -mass_loss_rate(mass_kg, 0.0, settings) * mass_kg * mass_kg;
		return (rate_coefficient > 0.0) ? (mass_kg * mass_kg * mass_kg / (3.0 * rate_coefficient)) : 0.0;
	}
};

class UnruhEffect {
public:
	[[nodiscard]] static double temperature_k(double proper_acceleration) noexcept {
		return Constants::REDUCED_PLANCK_CONSTANT * proper_acceleration / (2.0 * std::numbers::pi_v<double> * Constants::SPEED_OF_LIGHT * Constants::BOLTZMANN_CONSTANT);
	}

	[[nodiscard]] static double static_observer_acceleration(double mass_kg, double radius_m) noexcept {
		const double c2 = Constants::SPEED_OF_LIGHT * Constants::SPEED_OF_LIGHT;
		const double gm = Constants::GRAVITATIONAL_CONSTANT * mass_kg;
		const double redshift = 1.0 - 2.0 * gm / (c2 * radius_m);
		if (!(radius_m > 0.0) || redshift <= 0.0) {
			return -1.0;
		}
		return gm / (radius_m * radius_m) / std::sqrt(redshift);
	}

	[[nodiscard]] static double local_hawking_temperature_k(double horizon_temperature_k, double mass_kg, double radius_m) noexcept {
		const double redshift = 1.0 - 2.0 * Constants::GRAVITATIONAL_CONSTANT * mass_kg / (Constants::SPEED_OF_LIGHT * Constants::SPEED_OF_LIGHT * radius_m);
		return (redshift > 0.0) ? (horizon_temperature_k / std::sqrt(redshift)) : -1.0;
	}

	[[nodiscard]] static double thermal_occupation(double gap_joule, double temperature_k) noexcept {
		if (!(temperature_k > 0.0)) {
			return 0.0;
		}
		const double x = gap_joule / (Constants::BOLTZMANN_CONSTANT * temperature_k);
		return (x > 700.0) ? 0.0 : 1.0 / std::expm1(x);
	}

	[[nodiscard]] static double detector_excitation_rate(double gap_joule, double proper_acceleration, double coupling) noexcept {
		if (!(proper_acceleration > 0.0)) {
			return 0.0;
		}
		const double omega = gap_joule / Constants::REDUCED_PLANCK_CONSTANT;
		const double x = 2.0 * std::numbers::pi_v<double> * Constants::SPEED_OF_LIGHT * omega / proper_acceleration;
		return (x > 700.0) ? 0.0 : coupling * coupling * omega / (2.0 * std::numbers::pi_v<double>) / std::expm1(x);
	}
};

struct BlackHoleState {
	double mass_kg{0.0};
	double spin_star{0.0};
	bool remnant{false};
};

class EvaporatingBlackHole {
public:
	struct Rates {
		double mass{0.0};
		double spin{0.0};
	};

	[[nodiscard]] static Rates rates(double mass_kg, double spin_star, const EvaporationSettings& settings) noexcept {
		Rates result;
		result.mass = HawkingRadiation::mass_loss_rate(mass_kg, spin_star, settings);
		result.spin = (settings.spin_down_ratio - 2.0) * spin_star * result.mass / mass_kg;
		return result;
	}

	static void advance(BlackHoleState& state, double seconds, const EvaporationSettings& settings) noexcept {
		double remaining = seconds;
		for (uint32_t guard = 0; guard < 16384U && remaining > 0.0; ++guard) {
			if (state.mass_kg <= settings.minimum_mass_kg * (1.0 + 1.0e-12)) {
				state.mass_kg = settings.minimum_mass_kg;
				state.remnant = true;
				return;
			}
			const Rates k1 = rates(state.mass_kg, state.spin_star, settings);
			const double timescale = state.mass_kg / std::max(std::abs(k1.mass), 1.0e-300);
			const double h = std::min(remaining, 0.02 * timescale);
			const auto clamped = [&](double mass, double spin) noexcept {
				return Rates{std::max(mass, settings.minimum_mass_kg), std::clamp(spin, -0.999, 0.999)};
			};
			const Rates s2 = clamped(state.mass_kg + 0.5 * h * k1.mass, state.spin_star + 0.5 * h * k1.spin);
			const Rates k2 = rates(s2.mass, s2.spin, settings);
			const Rates s3 = clamped(state.mass_kg + 0.5 * h * k2.mass, state.spin_star + 0.5 * h * k2.spin);
			const Rates k3 = rates(s3.mass, s3.spin, settings);
			const Rates s4 = clamped(state.mass_kg + h * k3.mass, state.spin_star + h * k3.spin);
			const Rates k4 = rates(s4.mass, s4.spin, settings);
			state.mass_kg = std::max(state.mass_kg + h / 6.0 * (k1.mass + 2.0 * k2.mass + 2.0 * k3.mass + k4.mass), settings.minimum_mass_kg);
			state.spin_star = std::clamp(state.spin_star + h / 6.0 * (k1.spin + 2.0 * k2.spin + 2.0 * k3.spin + k4.spin), -0.999, 0.999);
			remaining -= h;
		}
	}
};

}

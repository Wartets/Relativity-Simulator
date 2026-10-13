#pragma once

#include "relativistic/cosmology/dark_energy_models.hpp"
#include "relativistic/cosmology/lemaitre_tolman_bondi_profile.hpp"
#include "relativistic/semiclassical/hawking_unruh.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unordered_map>

namespace Relativistic::Cosmology {

struct CosmologySettings {
	DarkEnergyParameters dark_energy{};
	LtbParameters ltb{};
	Semiclassical::EvaporationSettings evaporation{};

	void sanitize() noexcept {
		dark_energy.sanitize();
		ltb.sanitize();
		evaporation.sanitize();
	}

	void store(std::unordered_map<std::string, std::string>& out) const {
		const auto real = [](double value) {
			char buffer[40];
			std::snprintf(buffer, sizeof(buffer), "%.17g", value);
			return std::string(buffer);
		};
		out["cosmo_om"] = real(dark_energy.omega_matter);
		out["cosmo_or"] = real(dark_energy.omega_radiation);
		out["cosmo_pot"] = std::to_string(static_cast<uint32_t>(dark_energy.potential));
		out["cosmo_slope"] = real(dark_energy.potential_slope);
		out["cosmo_scale"] = real(dark_energy.potential_scale);
		out["cosmo_phi0"] = real(dark_energy.initial_field);
		out["cosmo_chap_a"] = real(dark_energy.chaplygin_stiffness);
		out["cosmo_chap_alpha"] = real(dark_energy.chaplygin_exponent);
		out["cosmo_ltb_h0"] = real(ltb.hubble0);
		out["cosmo_ltb_r0"] = real(ltb.void_radius);
		out["cosmo_ltb_w"] = real(ltb.void_width);
		out["cosmo_ltb_in"] = real(ltb.omega_inner);
		out["cosmo_ltb_out"] = real(ltb.omega_outer);
		out["cosmo_ev_enabled"] = evaporation.enabled ? "1" : "0";
		out["cosmo_ev_spectrum"] = std::to_string(static_cast<uint32_t>(evaporation.spectrum));
		out["cosmo_ev_model"] = std::to_string(static_cast<uint32_t>(evaporation.model));
		out["cosmo_ev_dof"] = real(evaporation.custom_degrees_of_freedom);
		out["cosmo_ev_grey"] = real(evaporation.greybody_factor);
		out["cosmo_ev_accel"] = real(evaporation.time_acceleration);
		out["cosmo_ev_spin"] = real(evaporation.spin_down_ratio);
		out["cosmo_ev_min"] = real(evaporation.minimum_mass_kg);
		out["cosmo_ev_gap"] = real(evaporation.detector_gap_ev);
		out["cosmo_ev_coupling"] = real(evaporation.detector_coupling);
	}

	void restore(const std::unordered_map<std::string, std::string>& in) {
		const auto real = [&in](const char* key, double fallback) {
			const auto it = in.find(key);
			if (it == in.end()) return fallback;
			const double parsed = std::strtod(it->second.c_str(), nullptr);
			return std::isfinite(parsed) ? parsed : fallback;
		};
		const auto integer = [&in](const char* key, uint32_t fallback) {
			const auto it = in.find(key);
			return (it != in.end()) ? static_cast<uint32_t>(std::strtoul(it->second.c_str(), nullptr, 10)) : fallback;
		};
		dark_energy.omega_matter = real("cosmo_om", dark_energy.omega_matter);
		dark_energy.omega_radiation = real("cosmo_or", dark_energy.omega_radiation);
		dark_energy.potential = static_cast<QuintessencePotential>(integer("cosmo_pot", static_cast<uint32_t>(dark_energy.potential)));
		dark_energy.potential_slope = real("cosmo_slope", dark_energy.potential_slope);
		dark_energy.potential_scale = real("cosmo_scale", dark_energy.potential_scale);
		dark_energy.initial_field = real("cosmo_phi0", dark_energy.initial_field);
		dark_energy.chaplygin_stiffness = real("cosmo_chap_a", dark_energy.chaplygin_stiffness);
		dark_energy.chaplygin_exponent = real("cosmo_chap_alpha", dark_energy.chaplygin_exponent);
		ltb.hubble0 = real("cosmo_ltb_h0", ltb.hubble0);
		ltb.void_radius = real("cosmo_ltb_r0", ltb.void_radius);
		ltb.void_width = real("cosmo_ltb_w", ltb.void_width);
		ltb.omega_inner = real("cosmo_ltb_in", ltb.omega_inner);
		ltb.omega_outer = real("cosmo_ltb_out", ltb.omega_outer);
		evaporation.enabled = integer("cosmo_ev_enabled", evaporation.enabled ? 1U : 0U) != 0U;
		evaporation.spectrum = static_cast<Semiclassical::EmissionSpectrum>(integer("cosmo_ev_spectrum", static_cast<uint32_t>(evaporation.spectrum)));
		evaporation.model = static_cast<Semiclassical::StressTensorModel>(integer("cosmo_ev_model", static_cast<uint32_t>(evaporation.model)));
		evaporation.custom_degrees_of_freedom = real("cosmo_ev_dof", evaporation.custom_degrees_of_freedom);
		evaporation.greybody_factor = real("cosmo_ev_grey", evaporation.greybody_factor);
		evaporation.time_acceleration = real("cosmo_ev_accel", evaporation.time_acceleration);
		evaporation.spin_down_ratio = real("cosmo_ev_spin", evaporation.spin_down_ratio);
		evaporation.minimum_mass_kg = real("cosmo_ev_min", evaporation.minimum_mass_kg);
		evaporation.detector_gap_ev = real("cosmo_ev_gap", evaporation.detector_gap_ev);
		evaporation.detector_coupling = real("cosmo_ev_coupling", evaporation.detector_coupling);
		sanitize();
	}
};

}

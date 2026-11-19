#pragma once

#include "relativistic/dynamics/pn/pn_body.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <string_view>
#include <unordered_map>

namespace Relativistic::UI {

enum class RandomizerDistributionLaw : uint32_t {
	Uniform = 0,
	LogUniform = 1,
	Gaussian = 2,
	PowerLaw = 3
};

inline constexpr std::array<const char*, 4> kRandomizerDistributionNames{
	"Uniform",
	"Log-Uniform",
	"Gaussian",
	"Power-Law"
};

enum class RandomizerRadiusMode : uint32_t {
	PhysicalDensity = 0,
	ExplicitBounds = 1,
	HydrostaticEquilibrium = 2
};

inline constexpr std::array<const char*, 3> kRandomizerRadiusModeNames{
	"Derived from Volumetric Density",
	"Explicit Radius Interval",
	"Relativistic Mass-Radius Profile"
};

struct RandomizerArchetypeFilter {
	bool allow_terrestrial{true};
	bool allow_gas_giant{true};
	bool allow_ice_giant{true};
	bool allow_star{true};
	bool allow_compact{true};
	bool allow_asteroid{true};
};

struct BodyRandomizerConfig {
	RandomizerRadiusMode radius_mode{RandomizerRadiusMode::PhysicalDensity};
	RandomizerDistributionLaw mass_distribution{RandomizerDistributionLaw::LogUniform};
	RandomizerDistributionLaw orbit_distribution{RandomizerDistributionLaw::LogUniform};
	RandomizerDistributionLaw eccentricity_distribution{RandomizerDistributionLaw::Uniform};

	double min_mass_ratio{1.0e-7};
	double max_mass_ratio{5.0e-3};
	double mass_power_law_alpha{2.0};

	double min_density_kg_m3{600.0};
	double max_density_kg_m3{8000.0};

	double min_explicit_radius_rg{0.02};
	double max_explicit_radius_rg{4.0};

	double min_orbit_rg{12.0};
	double max_orbit_rg{350.0};

	double min_eccentricity{0.0};
	double max_eccentricity{0.25};

	double max_inclination_deg{25.0};
	double prograde_fraction{0.92};

	double min_temperature_k{50.0};
	double max_temperature_k{6500.0};
	bool radiative_equilibrium_temperature{true};

	double min_spin_period_hours{2.0};
	double max_spin_period_hours{800.0};
	double max_axial_tilt_deg{35.0};

	double appearance_color_jitter{0.35};
	bool allow_ring_systems{true};
	double ring_system_probability{0.25};
	bool allow_atmospheres{true};
	double atmosphere_probability{0.65};

	RandomizerArchetypeFilter archetypes{};

	void sanitize() noexcept {
		min_mass_ratio = std::max(min_mass_ratio, 1e-15);
		max_mass_ratio = std::max(max_mass_ratio, min_mass_ratio * 1.001);
		mass_power_law_alpha = std::clamp(mass_power_law_alpha, 0.1, 5.0);

		min_density_kg_m3 = std::clamp(min_density_kg_m3, 50.0, 50000.0);
		max_density_kg_m3 = std::clamp(max_density_kg_m3, min_density_kg_m3, 50000.0);

		min_explicit_radius_rg = std::max(min_explicit_radius_rg, 1e-6);
		max_explicit_radius_rg = std::max(max_explicit_radius_rg, min_explicit_radius_rg * 1.01);

		min_orbit_rg = std::max(min_orbit_rg, 2.5);
		max_orbit_rg = std::max(max_orbit_rg, min_orbit_rg * 1.05);

		min_eccentricity = std::clamp(min_eccentricity, 0.0, 0.99);
		max_eccentricity = std::clamp(max_eccentricity, min_eccentricity, 0.999);

		max_inclination_deg = std::clamp(max_inclination_deg, 0.0, 90.0);
		prograde_fraction = std::clamp(prograde_fraction, 0.0, 1.0);

		min_temperature_k = std::max(min_temperature_k, 2.7);
		max_temperature_k = std::max(max_temperature_k, min_temperature_k);

		min_spin_period_hours = std::max(min_spin_period_hours, 0.05);
		max_spin_period_hours = std::max(max_spin_period_hours, min_spin_period_hours);
		max_axial_tilt_deg = std::clamp(max_axial_tilt_deg, 0.0, 180.0);

		appearance_color_jitter = std::clamp(appearance_color_jitter, 0.0, 1.0);
		ring_system_probability = std::clamp(ring_system_probability, 0.0, 1.0);
		atmosphere_probability = std::clamp(atmosphere_probability, 0.0, 1.0);
	}

	void write_settings(std::unordered_map<std::string, std::string>& kv) const {
		kv["rnd_radius_mode"] = std::to_string(static_cast<uint32_t>(radius_mode));
		kv["rnd_mass_dist"] = std::to_string(static_cast<uint32_t>(mass_distribution));
		kv["rnd_orbit_dist"] = std::to_string(static_cast<uint32_t>(orbit_distribution));
		kv["rnd_ecc_dist"] = std::to_string(static_cast<uint32_t>(eccentricity_distribution));
		kv["rnd_min_mass_ratio"] = std::to_string(min_mass_ratio);
		kv["rnd_max_mass_ratio"] = std::to_string(max_mass_ratio);
		kv["rnd_mass_alpha"] = std::to_string(mass_power_law_alpha);
		kv["rnd_min_density"] = std::to_string(min_density_kg_m3);
		kv["rnd_max_density"] = std::to_string(max_density_kg_m3);
		kv["rnd_min_radius_rg"] = std::to_string(min_explicit_radius_rg);
		kv["rnd_max_radius_rg"] = std::to_string(max_explicit_radius_rg);
		kv["rnd_min_orbit_rg"] = std::to_string(min_orbit_rg);
		kv["rnd_max_orbit_rg"] = std::to_string(max_orbit_rg);
		kv["rnd_min_ecc"] = std::to_string(min_eccentricity);
		kv["rnd_max_ecc"] = std::to_string(max_eccentricity);
		kv["rnd_max_inc"] = std::to_string(max_inclination_deg);
		kv["rnd_prograde_frac"] = std::to_string(prograde_fraction);
		kv["rnd_min_temp"] = std::to_string(min_temperature_k);
		kv["rnd_max_temp"] = std::to_string(max_temperature_k);
		kv["rnd_rad_eq_temp"] = radiative_equilibrium_temperature ? "1" : "0";
		kv["rnd_min_spin_h"] = std::to_string(min_spin_period_hours);
		kv["rnd_max_spin_h"] = std::to_string(max_spin_period_hours);
		kv["rnd_max_tilt"] = std::to_string(max_axial_tilt_deg);
		kv["rnd_color_jitter"] = std::to_string(appearance_color_jitter);
		kv["rnd_allow_rings"] = allow_ring_systems ? "1" : "0";
		kv["rnd_prob_rings"] = std::to_string(ring_system_probability);
		kv["rnd_allow_atmo"] = allow_atmospheres ? "1" : "0";
		kv["rnd_prob_atmo"] = std::to_string(atmosphere_probability);
		kv["rnd_arch_terrestrial"] = archetypes.allow_terrestrial ? "1" : "0";
		kv["rnd_arch_gas_giant"] = archetypes.allow_gas_giant ? "1" : "0";
		kv["rnd_arch_ice_giant"] = archetypes.allow_ice_giant ? "1" : "0";
		kv["rnd_arch_star"] = archetypes.allow_star ? "1" : "0";
		kv["rnd_arch_compact"] = archetypes.allow_compact ? "1" : "0";
		kv["rnd_arch_asteroid"] = archetypes.allow_asteroid ? "1" : "0";
	}

	void read_settings(const std::unordered_map<std::string, std::string>& kv) {
		auto get_dbl = [&](const char* key, double fallback) noexcept -> double {
			auto it = kv.find(key);
			return (it != kv.end()) ? std::strtod(it->second.c_str(), nullptr) : fallback;
		};
		auto get_u32 = [&](const char* key, uint32_t fallback) noexcept -> uint32_t {
			auto it = kv.find(key);
			return (it != kv.end()) ? static_cast<uint32_t>(std::strtoul(it->second.c_str(), nullptr, 10)) : fallback;
		};
		auto get_bool = [&](const char* key, bool fallback) noexcept -> bool {
			auto it = kv.find(key);
			return (it != kv.end()) ? (it->second == "1" || it->second == "true") : fallback;
		};

		radius_mode = static_cast<RandomizerRadiusMode>(get_u32("rnd_radius_mode", static_cast<uint32_t>(radius_mode)));
		mass_distribution = static_cast<RandomizerDistributionLaw>(get_u32("rnd_mass_dist", static_cast<uint32_t>(mass_distribution)));
		orbit_distribution = static_cast<RandomizerDistributionLaw>(get_u32("rnd_orbit_dist", static_cast<uint32_t>(orbit_distribution)));
		eccentricity_distribution = static_cast<RandomizerDistributionLaw>(get_u32("rnd_ecc_dist", static_cast<uint32_t>(eccentricity_distribution)));
		min_mass_ratio = get_dbl("rnd_min_mass_ratio", min_mass_ratio);
		max_mass_ratio = get_dbl("rnd_max_mass_ratio", max_mass_ratio);
		mass_power_law_alpha = get_dbl("rnd_mass_alpha", mass_power_law_alpha);
		min_density_kg_m3 = get_dbl("rnd_min_density", min_density_kg_m3);
		max_density_kg_m3 = get_dbl("rnd_max_density", max_density_kg_m3);
		min_explicit_radius_rg = get_dbl("rnd_min_radius_rg", min_explicit_radius_rg);
		max_explicit_radius_rg = get_dbl("rnd_max_radius_rg", max_explicit_radius_rg);
		min_orbit_rg = get_dbl("rnd_min_orbit_rg", min_orbit_rg);
		max_orbit_rg = get_dbl("rnd_max_orbit_rg", max_orbit_rg);
		min_eccentricity = get_dbl("rnd_min_ecc", min_eccentricity);
		max_eccentricity = get_dbl("rnd_max_ecc", max_eccentricity);
		max_inclination_deg = get_dbl("rnd_max_inc", max_inclination_deg);
		prograde_fraction = get_dbl("rnd_prograde_frac", prograde_fraction);
		min_temperature_k = get_dbl("rnd_min_temp", min_temperature_k);
		max_temperature_k = get_dbl("rnd_max_temp", max_temperature_k);
		radiative_equilibrium_temperature = get_bool("rnd_rad_eq_temp", radiative_equilibrium_temperature);
		min_spin_period_hours = get_dbl("rnd_min_spin_h", min_spin_period_hours);
		max_spin_period_hours = get_dbl("rnd_max_spin_h", max_spin_period_hours);
		max_axial_tilt_deg = get_dbl("rnd_max_tilt", max_axial_tilt_deg);
		appearance_color_jitter = get_dbl("rnd_color_jitter", appearance_color_jitter);
		allow_ring_systems = get_bool("rnd_allow_rings", allow_ring_systems);
		ring_system_probability = get_dbl("rnd_prob_rings", ring_system_probability);
		allow_atmospheres = get_bool("rnd_allow_atmo", allow_atmospheres);
		atmosphere_probability = get_dbl("rnd_prob_atmo", atmosphere_probability);
		archetypes.allow_terrestrial = get_bool("rnd_arch_terrestrial", archetypes.allow_terrestrial);
		archetypes.allow_gas_giant = get_bool("rnd_arch_gas_giant", archetypes.allow_gas_giant);
		archetypes.allow_ice_giant = get_bool("rnd_arch_ice_giant", archetypes.allow_ice_giant);
		archetypes.allow_star = get_bool("rnd_arch_star", archetypes.allow_star);
		archetypes.allow_compact = get_bool("rnd_arch_compact", archetypes.allow_compact);
		archetypes.allow_asteroid = get_bool("rnd_arch_asteroid", archetypes.allow_asteroid);
		sanitize();
	}
};

}

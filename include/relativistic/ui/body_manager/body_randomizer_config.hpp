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

	double min_friction{0.4};
	double max_friction{0.4};
	double min_restitution{0.5};
	double max_restitution{0.5};
	double min_elasticity{0.0};
	double max_elasticity{0.0};
	double min_integrity{1.0};
	double max_integrity{1.0};
	double min_youngs_modulus{1.0e9};
	double max_youngs_modulus{1.0e9};

	double min_charge{0.0};
	double max_charge{0.0};
	double min_magnetic_moment{0.0};
	double max_magnetic_moment{0.0};

	double min_quadrupole{0.0};
	double max_quadrupole{0.0};
	double min_j2{0.0};
	double max_j2{0.0};
	double min_j3{0.0};
	double max_j3{0.0};
	double min_j4{0.0};
	double max_j4{0.0};

	double min_oblateness{1.0};
	double max_oblateness{1.0};
	double min_noise_scale{2.0};
	double max_noise_scale{18.0};
	double min_roughness{0.2};
	double max_roughness{0.85};
	double min_texture_detail{0.75};
	double max_texture_detail{2.0};
	double min_specular_roughness{0.15};
	double max_specular_roughness{0.85};
	double min_emission{0.0};
	double max_emission{0.0};
	double min_atmosphere_thickness{0.05};
	double max_atmosphere_thickness{0.35};
	double min_polar_cap{0.1};
	double max_polar_cap{0.5};
	double min_night_lights{0.0};
	double max_night_lights{0.5};

	double min_source_mass_ratio{0.05};
	double max_source_mass_ratio{0.75};
	double min_source_orbit_rg{15.0};
	double max_source_orbit_rg{80.0};
	double min_source_spin{0.0};
	double max_source_spin{0.95};
	double min_source_charge{0.0};
	double max_source_charge{0.0};
	double max_source_tilt_deg{30.0};
	double source_disk_probability{0.85};
	double min_source_disk_temp_k{4000.0};
	double max_source_disk_temp_k{35000.0};
	double min_source_disk_outer_rg{18.0};
	double max_source_disk_outer_rg{45.0};
	double min_source_disk_brightness{0.2};
	double max_source_disk_brightness{1.2};

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

		min_friction = std::clamp(min_friction, 0.0, 1.0);
		max_friction = std::clamp(max_friction, min_friction, 1.0);
		min_restitution = std::clamp(min_restitution, 0.0, 1.0);
		max_restitution = std::clamp(max_restitution, min_restitution, 1.0);
		min_elasticity = std::clamp(min_elasticity, 0.0, 1.0);
		max_elasticity = std::clamp(max_elasticity, min_elasticity, 1.0);
		min_integrity = std::max(min_integrity, 0.0);
		max_integrity = std::max(max_integrity, min_integrity);
		min_youngs_modulus = std::max(min_youngs_modulus, 0.0);
		max_youngs_modulus = std::max(max_youngs_modulus, min_youngs_modulus);

		min_oblateness = std::clamp(min_oblateness, 0.1, 5.0);
		max_oblateness = std::clamp(max_oblateness, min_oblateness, 5.0);
		min_noise_scale = std::max(min_noise_scale, 0.1);
		max_noise_scale = std::max(max_noise_scale, min_noise_scale);
		min_roughness = std::clamp(min_roughness, 0.01, 1.0);
		max_roughness = std::clamp(max_roughness, min_roughness, 1.0);
		min_texture_detail = std::max(min_texture_detail, 0.05);
		max_texture_detail = std::max(max_texture_detail, min_texture_detail);
		min_specular_roughness = std::clamp(min_specular_roughness, 0.0, 1.0);
		max_specular_roughness = std::clamp(max_specular_roughness, min_specular_roughness, 1.0);
		min_emission = std::max(min_emission, 0.0);
		max_emission = std::max(max_emission, min_emission);
		min_atmosphere_thickness = std::clamp(min_atmosphere_thickness, 0.0, 1.0);
		max_atmosphere_thickness = std::clamp(max_atmosphere_thickness, min_atmosphere_thickness, 1.0);
		min_polar_cap = std::clamp(min_polar_cap, 0.0, 1.0);
		max_polar_cap = std::clamp(max_polar_cap, min_polar_cap, 1.0);
		min_night_lights = std::clamp(min_night_lights, 0.0, 2.0);
		max_night_lights = std::clamp(max_night_lights, min_night_lights, 2.0);

		min_source_mass_ratio = std::max(min_source_mass_ratio, 1e-6);
		max_source_mass_ratio = std::max(max_source_mass_ratio, min_source_mass_ratio);
		min_source_orbit_rg = std::max(min_source_orbit_rg, 4.0);
		max_source_orbit_rg = std::max(max_source_orbit_rg, min_source_orbit_rg);
		min_source_spin = std::clamp(min_source_spin, 0.0, 0.999);
		max_source_spin = std::clamp(max_source_spin, min_source_spin, 0.999);
		max_source_tilt_deg = std::clamp(max_source_tilt_deg, 0.0, 90.0);
		source_disk_probability = std::clamp(source_disk_probability, 0.0, 1.0);
		min_source_disk_temp_k = std::max(min_source_disk_temp_k, 500.0);
		max_source_disk_temp_k = std::max(max_source_disk_temp_k, min_source_disk_temp_k);
		min_source_disk_outer_rg = std::max(min_source_disk_outer_rg, 6.0);
		max_source_disk_outer_rg = std::max(max_source_disk_outer_rg, min_source_disk_outer_rg);
		min_source_disk_brightness = std::max(min_source_disk_brightness, 0.0);
		max_source_disk_brightness = std::max(max_source_disk_brightness, min_source_disk_brightness);
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
		kv["rnd_min_friction"] = std::to_string(min_friction);
		kv["rnd_max_friction"] = std::to_string(max_friction);
		kv["rnd_min_restitution"] = std::to_string(min_restitution);
		kv["rnd_max_restitution"] = std::to_string(max_restitution);
		kv["rnd_min_elasticity"] = std::to_string(min_elasticity);
		kv["rnd_max_elasticity"] = std::to_string(max_elasticity);
		kv["rnd_min_integrity"] = std::to_string(min_integrity);
		kv["rnd_max_integrity"] = std::to_string(max_integrity);
		kv["rnd_min_ym"] = std::to_string(min_youngs_modulus);
		kv["rnd_max_ym"] = std::to_string(max_youngs_modulus);
		kv["rnd_min_charge"] = std::to_string(min_charge);
		kv["rnd_max_charge"] = std::to_string(max_charge);
		kv["rnd_min_mag_mom"] = std::to_string(min_magnetic_moment);
		kv["rnd_max_mag_mom"] = std::to_string(max_magnetic_moment);
		kv["rnd_min_quad"] = std::to_string(min_quadrupole);
		kv["rnd_max_quad"] = std::to_string(max_quadrupole);
		kv["rnd_min_j2"] = std::to_string(min_j2);
		kv["rnd_max_j2"] = std::to_string(max_j2);
		kv["rnd_min_j3"] = std::to_string(min_j3);
		kv["rnd_max_j3"] = std::to_string(max_j3);
		kv["rnd_min_j4"] = std::to_string(min_j4);
		kv["rnd_max_j4"] = std::to_string(max_j4);
		kv["rnd_min_oblate"] = std::to_string(min_oblateness);
		kv["rnd_max_oblate"] = std::to_string(max_oblateness);
		kv["rnd_min_noise"] = std::to_string(min_noise_scale);
		kv["rnd_max_noise"] = std::to_string(max_noise_scale);
		kv["rnd_min_rough"] = std::to_string(min_roughness);
		kv["rnd_max_rough"] = std::to_string(max_roughness);
		kv["rnd_min_detail"] = std::to_string(min_texture_detail);
		kv["rnd_max_detail"] = std::to_string(max_texture_detail);
		kv["rnd_min_spec"] = std::to_string(min_specular_roughness);
		kv["rnd_max_spec"] = std::to_string(max_specular_roughness);
		kv["rnd_min_emit"] = std::to_string(min_emission);
		kv["rnd_max_emit"] = std::to_string(max_emission);
		kv["rnd_min_atmo"] = std::to_string(min_atmosphere_thickness);
		kv["rnd_max_atmo"] = std::to_string(max_atmosphere_thickness);
		kv["rnd_min_polar"] = std::to_string(min_polar_cap);
		kv["rnd_max_polar"] = std::to_string(max_polar_cap);
		kv["rnd_min_night"] = std::to_string(min_night_lights);
		kv["rnd_max_night"] = std::to_string(max_night_lights);
		kv["rnd_src_min_m_ratio"] = std::to_string(min_source_mass_ratio);
		kv["rnd_src_max_m_ratio"] = std::to_string(max_source_mass_ratio);
		kv["rnd_src_min_orb"] = std::to_string(min_source_orbit_rg);
		kv["rnd_src_max_orb"] = std::to_string(max_source_orbit_rg);
		kv["rnd_src_min_spin"] = std::to_string(min_source_spin);
		kv["rnd_src_max_spin"] = std::to_string(max_source_spin);
		kv["rnd_src_min_q"] = std::to_string(min_source_charge);
		kv["rnd_src_max_q"] = std::to_string(max_source_charge);
		kv["rnd_src_max_tilt"] = std::to_string(max_source_tilt_deg);
		kv["rnd_src_disk_prob"] = std::to_string(source_disk_probability);
		kv["rnd_src_min_disk_t"] = std::to_string(min_source_disk_temp_k);
		kv["rnd_src_max_disk_t"] = std::to_string(max_source_disk_temp_k);
		kv["rnd_src_min_disk_r"] = std::to_string(min_source_disk_outer_rg);
		kv["rnd_src_max_disk_r"] = std::to_string(max_source_disk_outer_rg);
		kv["rnd_src_min_disk_b"] = std::to_string(min_source_disk_brightness);
		kv["rnd_src_max_disk_b"] = std::to_string(max_source_disk_brightness);
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
		min_friction = get_dbl("rnd_min_friction", min_friction);
		max_friction = get_dbl("rnd_max_friction", max_friction);
		min_restitution = get_dbl("rnd_min_restitution", min_restitution);
		max_restitution = get_dbl("rnd_max_restitution", max_restitution);
		min_elasticity = get_dbl("rnd_min_elasticity", min_elasticity);
		max_elasticity = get_dbl("rnd_max_elasticity", max_elasticity);
		min_integrity = get_dbl("rnd_min_integrity", min_integrity);
		max_integrity = get_dbl("rnd_max_integrity", max_integrity);
		min_youngs_modulus = get_dbl("rnd_min_ym", min_youngs_modulus);
		max_youngs_modulus = get_dbl("rnd_max_ym", max_youngs_modulus);
		min_charge = get_dbl("rnd_min_charge", min_charge);
		max_charge = get_dbl("rnd_max_charge", max_charge);
		min_magnetic_moment = get_dbl("rnd_min_mag_mom", min_magnetic_moment);
		max_magnetic_moment = get_dbl("rnd_max_mag_mom", max_magnetic_moment);
		min_quadrupole = get_dbl("rnd_min_quad", min_quadrupole);
		max_quadrupole = get_dbl("rnd_max_quad", max_quadrupole);
		min_j2 = get_dbl("rnd_min_j2", min_j2);
		max_j2 = get_dbl("rnd_max_j2", max_j2);
		min_j3 = get_dbl("rnd_min_j3", min_j3);
		max_j3 = get_dbl("rnd_max_j3", max_j3);
		min_j4 = get_dbl("rnd_min_j4", min_j4);
		max_j4 = get_dbl("rnd_max_j4", max_j4);
		min_oblateness = get_dbl("rnd_min_oblate", min_oblateness);
		max_oblateness = get_dbl("rnd_max_oblate", max_oblateness);
		min_noise_scale = get_dbl("rnd_min_noise", min_noise_scale);
		max_noise_scale = get_dbl("rnd_max_noise", max_noise_scale);
		min_roughness = get_dbl("rnd_min_rough", min_roughness);
		max_roughness = get_dbl("rnd_max_rough", max_roughness);
		min_texture_detail = get_dbl("rnd_min_detail", min_texture_detail);
		max_texture_detail = get_dbl("rnd_max_detail", max_texture_detail);
		min_specular_roughness = get_dbl("rnd_min_spec", min_specular_roughness);
		max_specular_roughness = get_dbl("rnd_max_spec", max_specular_roughness);
		min_emission = get_dbl("rnd_min_emit", min_emission);
		max_emission = get_dbl("rnd_max_emit", max_emission);
		min_atmosphere_thickness = get_dbl("rnd_min_atmo", min_atmosphere_thickness);
		max_atmosphere_thickness = get_dbl("rnd_max_atmo", max_atmosphere_thickness);
		min_polar_cap = get_dbl("rnd_min_polar", min_polar_cap);
		max_polar_cap = get_dbl("rnd_max_polar", max_polar_cap);
		min_night_lights = get_dbl("rnd_min_night", min_night_lights);
		max_night_lights = get_dbl("rnd_max_night", max_night_lights);
		min_source_mass_ratio = get_dbl("rnd_src_min_m_ratio", min_source_mass_ratio);
		max_source_mass_ratio = get_dbl("rnd_src_max_m_ratio", max_source_mass_ratio);
		min_source_orbit_rg = get_dbl("rnd_src_min_orb", min_source_orbit_rg);
		max_source_orbit_rg = get_dbl("rnd_src_max_orb", max_source_orbit_rg);
		min_source_spin = get_dbl("rnd_src_min_spin", min_source_spin);
		max_source_spin = get_dbl("rnd_src_max_spin", max_source_spin);
		min_source_charge = get_dbl("rnd_src_min_q", min_source_charge);
		max_source_charge = get_dbl("rnd_src_max_q", max_source_charge);
		max_source_tilt_deg = get_dbl("rnd_src_max_tilt", max_source_tilt_deg);
		source_disk_probability = get_dbl("rnd_src_disk_prob", source_disk_probability);
		min_source_disk_temp_k = get_dbl("rnd_src_min_disk_t", min_source_disk_temp_k);
		max_source_disk_temp_k = get_dbl("rnd_src_max_disk_t", max_source_disk_temp_k);
		min_source_disk_outer_rg = get_dbl("rnd_src_min_disk_r", min_source_disk_outer_rg);
		max_source_disk_outer_rg = get_dbl("rnd_src_max_disk_r", max_source_disk_outer_rg);
		min_source_disk_brightness = get_dbl("rnd_src_min_disk_b", min_source_disk_brightness);
		max_source_disk_brightness = get_dbl("rnd_src_max_disk_b", max_source_disk_brightness);
		sanitize();
	}
};

}

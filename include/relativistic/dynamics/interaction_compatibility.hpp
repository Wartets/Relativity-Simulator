#pragma once

#include "relativistic/dynamics/interaction_config.hpp"
#include "relativistic/dynamics/pn_body.hpp"
#include <span>
#include <string_view>

namespace Relativistic::Dynamics {

[[nodiscard]] inline std::string_view magnetism_without_moments_warning(std::span<const PostNewtonianBody> bodies, const ElectromagneticInteractionConfig& cfg) noexcept {
	if (!cfg.magnetism_enabled) return {};
	for (const auto& b : bodies) {
		if (b.enabled && b.magnetic_moment != 0.0) return {};
	}
	return "Magnetism is enabled but no body currently has a non-zero magnetic moment, so no magnetic force will act until at least two such bodies exist.";
}

[[nodiscard]] inline std::string_view collisions_without_radius_warning(std::span<const PostNewtonianBody> bodies, const CollisionInteractionConfig& cfg) noexcept {
	if (!cfg.enabled) return {};
	for (const auto& b : bodies) {
		if (b.enabled && b.radius <= 0.0) {
			return "Collisions are enabled but at least one body has zero physical radius; it can never overlap another body and will pass through unaffected.";
		}
	}
	return {};
}

[[nodiscard]] inline std::string_view fragmentation_requires_source_warning(const FragmentationInteractionConfig& fragmentation_cfg, const CollisionInteractionConfig& collision_cfg) noexcept {
	if (!fragmentation_cfg.enabled) return {};
	if (!collision_cfg.enabled && !fragmentation_cfg.enable_tidal_stress) {
		return "Fragmentation is enabled but both collision impacts and tidal stress are disabled, so body integrity can never decrease.";
	}
	return {};
}

[[nodiscard]] inline std::string_view annihilation_requires_charge_warning(const AnnihilationInteractionConfig& annihilation_cfg, const ElectromagneticInteractionConfig& em_cfg) noexcept {
	if (!annihilation_cfg.enabled || !annihilation_cfg.require_opposite_charge) return {};
	if (!em_cfg.electricity_enabled) {
		return "Annihilation requires opposite charge but Coulomb electricity is disabled; charge values are still compared for annihilation purposes, this is only informational.";
	}
	return {};
}

[[nodiscard]] inline std::string_view thermodynamics_disabled_ambient_note(const ThermodynamicsInteractionConfig& cfg) noexcept {
	if (cfg.enabled && cfg.ambient_temperature_kelvin < 0.0) {
		return "Thermodynamics is enabled but ambient coupling is disabled (temperature set to -1); bodies retain heat capacity bookkeeping without radiative exchange against an environment.";
	}
	return {};
}

}

#pragma once

#include <cstdint>

namespace Relativistic::Dynamics {

struct ElectromagneticInteractionConfig {
	bool electricity_enabled{false};
	bool magnetism_enabled{false};
	double vacuum_permittivity{8.8541878128e-12};
	double vacuum_permeability{1.25663706212e-6};
};

enum class CollisionResponseModel : uint32_t {
	Elastic = 0,
	Inelastic = 1
};

struct CollisionInteractionConfig {
	bool enabled{false};
	CollisionResponseModel response_model{CollisionResponseModel::Elastic};
	bool consider_rotation{true};
	bool consider_friction{true};
	double restitution_multiplier{1.0};
	double contact_stiffness_scale{1.0e-9};
	double position_correction_factor{0.2};
	double position_correction_slop{1.0e-4};
};

struct ThermodynamicsInteractionConfig {
	bool enabled{false};
	double ambient_temperature_kelvin{-1.0};
	double radiative_coupling_scale{1.0};
};

struct FragmentationInteractionConfig {
	bool enabled{false};
	bool enable_tidal_stress{false};
	double collision_energy_to_integrity_loss{1.0e-6};
	double tidal_stress_to_integrity_loss{1.0e-6};
	double minimum_fragment_mass{1.0e-6};
	uint32_t max_fragments_per_event{2};
};

struct AnnihilationInteractionConfig {
	bool enabled{false};
	double contact_distance_scale{1.0};
	bool require_opposite_charge{true};
};

struct InteractionConfig {
	ElectromagneticInteractionConfig electromagnetic{};
	CollisionInteractionConfig collisions{};
	ThermodynamicsInteractionConfig thermodynamics{};
	FragmentationInteractionConfig fragmentation{};
	AnnihilationInteractionConfig annihilation{};
};

}

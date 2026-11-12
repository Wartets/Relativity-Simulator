#pragma once

#include "relativistic/dynamics/pn/pn_body.hpp"
#include "relativistic/ui/spacetime_source_state.hpp"
#include <array>
#include <cstdint>
#include <string_view>

namespace Relativistic::UI {

enum class BodyPresetTemplate : uint32_t {
	Custom = 0,
	Sun = 1,
	Earth = 2,
	Moon = 3,
	Jupiter = 4,
	Mars = 5,
	NeutronStar = 6,
	SupermassiveBlackHole = 7,
	StellarBlackHole = 8,
	TestParticle = 9
};

enum class BodyCatalogSortMode : uint32_t {
	CreationOrder = 0,
	Name = 1,
	Mass = 2,
	Distance = 3,
	Speed = 4
};

enum class CatalogTypeCategory : uint32_t {
	All = 0,
	Terrestrial = 1,
	GasGiant = 2,
	IceGiant = 3,
	Star = 4,
	Compact = 5,
	SmallBody = 6,
	Custom = 7
};

inline constexpr std::array<const char*, 8> kCatalogTypeCategoryNames{
	"All Object Types",
	"Terrestrial Planets",
	"Gas Giants",
	"Ice Giants",
	"Stars",
	"Neutron Stars / Pulsars",
	"Moons / Asteroids",
	"Custom / Other"
};

enum class SourcePresetTemplate : uint32_t {
	Custom = 0,
	SagittariusA = 1,
	M87 = 2,
	CygnusX1 = 3,
	GW150914Remnant = 4,
	IntermediateMass = 5,
	CompanionKerr = 6,
	SchwarzschildStatic = 7,
	ExtremalKerr = 8,
	MicroBlackHole = 9
};

inline constexpr std::array<const char*, 10> kSourceTemplateNames{
	"Custom Black Hole",
	"Sagittarius A* (Milky Way SMBH)",
	"M87* (Supermassive Black Hole)",
	"Cygnus X-1 (Stellar Black Hole)",
	"GW150914 Remnant (LIGO Binary)",
	"Intermediate-Mass Black Hole (1000 Msun)",
	"Companion Kerr Black Hole (Half Central Mass)",
	"Schwarzschild Static Black Hole (a=0)",
	"Near-Extremal Kerr Black Hole (a/M=0.998)",
	"Micro / Primordial Black Hole"
};

struct SourceTemplateSpec {
	std::string_view name;
	std::string_view description;
	bool native_units;
	double mass;
	double spin_ratio;
	double charge_ratio;
	uint32_t disk_preset_index;
	double orbit_in_central_masses;
	double inclination_rad;
};

inline constexpr std::array<SourceTemplateSpec, 9> kSourceTemplateSpecs{{
	SourceTemplateSpec{"Sagittarius A*", "Supermassive black hole at the Galactic Center (4.15M solar masses)", false, 8.26e36, 0.90, 0.0, 5, 45.0, 0.15},
	SourceTemplateSpec{"M87*", "Supermassive black hole in Virgo A galaxy (6.5B solar masses)", false, 1.29e40, 0.90, 0.0, 3, 60.0, 0.28},
	SourceTemplateSpec{"Cygnus X-1", "High-mass X-ray binary stellar black hole (21.2 solar masses)", false, 4.21e31, 0.97, 0.0, 2, 35.0, 0.08},
	SourceTemplateSpec{"GW150914 Remnant", "Post-merger ringing remnant black hole (62 solar masses)", false, 1.23e32, 0.67, 0.0, 1, 30.0, 0.0},
	SourceTemplateSpec{"Intermediate-Mass BH", "Hypothetical cluster seed black hole (1000 solar masses)", false, 1.988e33, 0.50, 0.0, 0, 40.0, 0.12},
	SourceTemplateSpec{"Companion Kerr BH", "Orbiting companion half the mass of the primary source", true, 0.5, 0.60, 0.0, 0, 32.0, 0.22},
	SourceTemplateSpec{"Schwarzschild Static BH", "Non-rotating neutral spherical black hole companion", true, 0.35, 0.0, 0.0, 1, 28.0, 0.0},
	SourceTemplateSpec{"Near-Extremal Kerr BH", "Fast-rotating black hole close to the cosmic censorship bound", true, 0.4, 0.998, 0.0, 4, 38.0, 0.05},
	SourceTemplateSpec{"Primordial Micro BH", "Sub-planetary primordial remnant (lunar mass, sub-millimeter horizon)", false, 7.35e22, 0.10, 0.0, 6, 22.0, 0.35}
}};

enum class BodyManagerTab : uint32_t {
	ObjectCatalog = 0,
	SourceCatalog = 1,
	ObjectCreation = 2,
	SourceCreation = 3,
	SystemDynamics = 4,
	Interactions = 5
};

namespace BodyEditorSection {
	inline constexpr uint32_t Identity = 1U << 0;
	inline constexpr uint32_t SourceToggle = 1U << 1;
	inline constexpr uint32_t Physical = 1U << 2;
	inline constexpr uint32_t Multipoles = 1U << 3;
	inline constexpr uint32_t Material = 1U << 4;
	inline constexpr uint32_t Surface = 1U << 5;
	inline constexpr uint32_t AccretionDisk = 1U << 6;
	inline constexpr uint32_t Complete = Identity | SourceToggle | Physical | Multipoles | Material | Surface | AccretionDisk;
	inline constexpr uint32_t ObjectCreation = Identity | Physical | Multipoles | Material | Surface;
	inline constexpr uint32_t SpacetimeSource = Identity | Physical | AccretionDisk;
}

struct BodyEditorViewState {
	bool mass_log_mode{true};
	bool radius_log_mode{true};
	bool reference_radius_log_mode{true};
	bool magnetic_moment_log_mode{false};
	bool rotation_speed_log_mode{false};
	bool lifetime_log_mode{false};
	bool heat_capacity_log_mode{false};
	bool youngs_modulus_log_mode{true};
	bool resistance_log_mode{false};
	bool guarantee_horizon{true};
	int layer_template_choice{0};
	SpacetimeSourceEditState source_parameters{true};
};

struct BodyEditResult {
	bool body_changed{false};
	bool layers_changed{false};
};

struct BodyTemplateSpec {
	std::string_view name;
	bool native_units;
	double mass;
	double radius;
	double j2;
	double j4;
	double temperature_kelvin;
	Dynamics::Body3DPreset preset;
	std::array<float, 4> color;
	std::array<float, 4> color_secondary;
	std::array<float, 4> color_tertiary;
	float noise_scale;
	float surface_roughness;
	float atmosphere_thickness;
	float emission_intensity;
	float rotation_speed;
	float polar_cap_strength;
	float night_side_light_intensity;
	bool ring_system;
	int layer_template_first;
	int layer_template_second;
};

inline constexpr std::array<BodyTemplateSpec, 9> kBodyTemplateSpecs{{
	BodyTemplateSpec{"Sun", false, 1.98847e30, 6.9634e8, 2.2e-7, 0.0, 5772.0, Dynamics::Body3DPreset::Star,
		{1.0f, 0.85f, 0.5f, 1.0f}, {1.0f, 0.55f, 0.15f, 1.0f}, {1.0f, 0.95f, 0.7f, 1.0f},
		8.0f, 0.6f, 0.0f, 2.5f, 0.05f, 0.0f, 0.0f, false, -1, -1},
	BodyTemplateSpec{"Earth", false, 5.9722e24, 6.378137e6, 1.08263e-3, 0.0, 288.0, Dynamics::Body3DPreset::TerrestrialPlanet,
		{0.10f, 0.35f, 0.65f, 1.0f}, {0.20f, 0.50f, 0.22f, 1.0f}, {0.95f, 0.95f, 0.98f, 1.0f},
		5.0f, 0.45f, 0.18f, 0.0f, 0.12f, 0.45f, 0.4f, false, 2, -1},
	BodyTemplateSpec{"Moon", false, 7.342e22, 1.7374e6, 2.0335e-4, 0.0, 250.0, Dynamics::Body3DPreset::Metallic,
		{0.55f, 0.55f, 0.58f, 1.0f}, {0.32f, 0.32f, 0.35f, 1.0f}, {0.75f, 0.75f, 0.78f, 1.0f},
		7.0f, 0.75f, 0.0f, 0.0f, 0.03f, 0.0f, 0.0f, false, 3, -1},
	BodyTemplateSpec{"Jupiter", false, 1.89813e27, 7.1492e7, 1.469657e-2, -5.86609e-4, 165.0, Dynamics::Body3DPreset::GasGiant,
		{0.82f, 0.65f, 0.45f, 1.0f}, {0.62f, 0.40f, 0.24f, 1.0f}, {0.92f, 0.85f, 0.75f, 1.0f},
		3.5f, 0.3f, 0.32f, 0.0f, 0.35f, 0.2f, 0.0f, true, 5, -1},
	BodyTemplateSpec{"Mars", false, 6.4171e23, 3.3895e6, 1.96045e-3, 0.0, 210.0, Dynamics::Body3DPreset::TerrestrialPlanet,
		{0.72f, 0.35f, 0.20f, 1.0f}, {0.48f, 0.24f, 0.15f, 1.0f}, {0.95f, 0.95f, 0.98f, 1.0f},
		6.0f, 0.65f, 0.05f, 0.0f, 0.11f, 0.0f, 0.0f, false, 0, 6},
	BodyTemplateSpec{"Neutron Star", false, 2.8e30, 12000.0, 0.0, 0.0, 1.0e6, Dynamics::Body3DPreset::NeutronStar,
		{0.85f, 0.90f, 1.0f, 1.0f}, {0.55f, 0.72f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f},
		1.5f, 0.15f, 0.0f, 3.0f, 1.8f, 0.0f, 0.0f, false, -1, -1},
	BodyTemplateSpec{"Supermassive BH", false, 8.0e36, 1.2e10, 0.0, 0.0, 0.0, Dynamics::Body3DPreset::BlackHole,
		{0.02f, 0.02f, 0.03f, 1.0f}, {0.06f, 0.05f, 0.08f, 1.0f}, {0.04f, 0.04f, 0.03f, 1.0f},
		1.0f, 0.1f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, false, -1, -1},
	BodyTemplateSpec{"Stellar Black Hole", false, 2.0e31, 30000.0, 0.0, 0.0, 0.0, Dynamics::Body3DPreset::BlackHole,
		{0.02f, 0.02f, 0.03f, 1.0f}, {0.06f, 0.05f, 0.08f, 1.0f}, {0.04f, 0.04f, 0.03f, 1.0f},
		1.0f, 0.1f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, false, -1, -1},
	BodyTemplateSpec{"Test Particle", true, 0.0, 1.0, 0.0, 0.0, 0.0, Dynamics::Body3DPreset::Asteroid,
		{1.0f, 1.0f, 1.0f, 1.0f}, {0.7f, 0.7f, 0.7f, 1.0f}, {0.85f, 0.85f, 0.85f, 1.0f},
		4.0f, 0.5f, 0.0f, 0.0f, 0.1f, 0.0f, 0.0f, false, -1, -1}
}};

struct BodyCreationArchetype {
	std::string_view label;
	double mass_ratio_min;
	double mass_ratio_max;
	double size_factor_min;
	double size_factor_max;
	double orbit_gravitational_radii_min;
	double orbit_gravitational_radii_max;
	double breakup_spin_fraction;
	double quadrupole_min;
	double quadrupole_max;
	double j2_min;
	double j2_max;
	double j3_min;
	double j3_max;
	double j4_min;
	double j4_max;
	Dynamics::Body3DPreset preset;
	double hue_center;
	double hue_spread;
	double saturation;
	double value;
	double noise_min;
	double noise_max;
	double roughness_min;
	double roughness_max;
	double atmosphere_min;
	double atmosphere_max;
	double emission_min;
	double emission_max;
	double rotation_min;
	double rotation_max;
};

inline constexpr std::array<BodyCreationArchetype, 6> kCreationArchetypes{{
	BodyCreationArchetype{"Probe", 1e-12, 1e-9, 4e-4, 2e-3, 12.0, 120.0, 0.05, 1e-12, 1e-8, 1e-10, 1e-6, 1e-12, 1e-7, 1e-12, 1e-7,
		Dynamics::Body3DPreset::Asteroid, 0.55, 0.08, 0.10, 0.75, 6.0, 14.0, 0.6, 0.95, 0.0, 0.0, 0.0, 0.0, 0.05, 0.4},
	BodyCreationArchetype{"Shard", 1e-10, 1e-7, 1e-3, 5e-3, 10.0, 150.0, 0.1, 1e-10, 1e-6, 1e-8, 1e-4, 1e-10, 1e-5, 1e-10, 1e-5,
		Dynamics::Body3DPreset::Metallic, 0.08, 0.06, 0.30, 0.55, 4.0, 10.0, 0.5, 0.85, 0.0, 0.0, 0.0, 0.0, 0.05, 0.35},
	BodyCreationArchetype{"World", 1e-7, 1e-5, 4e-3, 1.5e-2, 10.0, 200.0, 0.05, 1e-9, 1e-5, 1e-4, 3e-3, 1e-8, 1e-5, 1e-9, 1e-5,
		Dynamics::Body3DPreset::TerrestrialPlanet, 0.42, 0.14, 0.55, 0.75, 3.0, 8.0, 0.3, 0.6, 0.08, 0.3, 0.0, 0.0, 0.05, 0.2},
	BodyCreationArchetype{"Giant", 1e-5, 1e-3, 1.2e-2, 4e-2, 15.0, 250.0, 0.25, 1e-8, 1e-4, 1e-3, 2e-2, 1e-7, 1e-4, 1e-7, 1e-4,
		Dynamics::Body3DPreset::GasGiant, 0.10, 0.16, 0.45, 0.85, 2.0, 6.0, 0.2, 0.5, 0.2, 0.4, 0.0, 0.0, 0.1, 0.5},
	BodyCreationArchetype{"Compact", 1e-3, 2e-2, 4.0, 8.0, 12.0, 250.0, 0.4, 1e-12, 1e-7, 1e-10, 1e-6, 1e-12, 1e-7, 1e-12, 1e-7,
		Dynamics::Body3DPreset::NeutronStar, 0.58, 0.05, 0.08, 0.9, 1.5, 5.0, 0.15, 0.4, 0.0, 0.0, 0.5, 2.5, 0.5, 2.0},
	BodyCreationArchetype{"Astral", 2e-3, 4e-2, 1.5e-2, 4e-2, 25.0, 300.0, 0.05, 1e-8, 1e-3, 1e-7, 1e-4, 1e-7, 1e-4, 1e-7, 1e-4,
		Dynamics::Body3DPreset::Star, 0.13, 0.10, 0.35, 1.0, 3.0, 12.0, 0.4, 0.85, 0.0, 0.0, 1.5, 3.5, 0.02, 0.2}
}};

inline constexpr std::array<const char*, 10> kBodyTemplateNames{
	"Custom Body", "Sun (Solar Mass & Radius)", "Earth (Terrestrial Planet)", "Moon (Natural Satellite)",
	"Jupiter (Gas Giant)", "Mars (Telluric Planet)", "Neutron Star (Compact)", "Supermassive Black Hole",
	"Stellar Mass Black Hole", "Test Particle (Zero Mass)"
};

inline constexpr std::array<const char*, 4> kBodyGeometryNames{
	"Oblate Spheroid (Spin/J2 Deformed)", "Rigid Sphere", "Prolate Spheroid", "Triaxial Ellipsoid"
};

inline constexpr std::array<const char*, 10> kBodySurfacePresetNames{
	"Star", "Terrestrial Planet", "Gas Giant", "Ice Giant", "Metallic / Moon",
	"Asteroid", "Neutron Star", "Pulsar", "Black Hole", "Custom"
};

inline constexpr std::array<const char*, 14> kBodyTextureModeNames{
	"Procedural Noise Shader", "Solid Color", "Color Palette Blend", "Banded Gas Giant",
	"Cratered Terrestrial", "Stellar Granulation", "Accretion Flow", "Marbled Stone",
	"Ringed Gas Giant (Bands + Polar Caps)", "Icy Cracked Surface", "Volcanic Magma",
	"City Lights (Night Side)", "Nebulous Gas Cloud", "Earth Photographic Map (Blue Marble)"
};

inline constexpr std::array<const char*, 3> kEarthMapVariantNames{
	"Day Map", "Night Map", "Automatic Day/Night Blend"
};

inline constexpr std::array<const char*, 2> kEarthMapQualityNames{
	"1K (1024x512, lowest GPU cost)", "2K (2048x1024, highest detail)"
};

inline constexpr std::array<const char*, 6> kBodyAtmosphereModeNames{
	"Rayleigh Limb Shell", "Volumetric Scattering", "Off", "Thick Haze", "Volumetric Mie", "Glowing Corona"
};

}

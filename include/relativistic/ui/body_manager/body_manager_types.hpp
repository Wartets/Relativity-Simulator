#pragma once

#include "relativistic/dynamics/pn/pn_body.hpp"
#include "relativistic/ui/spacetime_source_state.hpp"
#include <array>
#include <cstdint>
#include <string_view>
#include <string>
#include <vector>
#include <cmath>
#include <numbers>
#include <algorithm>

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

enum class BulkActionScope : uint32_t {
	AllBodies = 0,
	OrdinaryObjectsOnly = 1,
	SpacetimeSourcesOnly = 2,
	ByTypeCategory = 3
};

inline constexpr std::array<const char*, 4> kBulkActionScopeNames{
	"All Bodies (Objects & Sources)",
	"Ordinary Objects Only",
	"Independent Black Holes Only",
	"By Object Type Category"
};

[[nodiscard]] inline bool matches_bulk_scope(const Dynamics::PostNewtonianBody& b, BulkActionScope scope, CatalogTypeCategory cat) noexcept {
	switch (scope) {
		case BulkActionScope::AllBodies:
			return true;
		case BulkActionScope::OrdinaryObjectsOnly:
			return !b.is_spacetime_source;
		case BulkActionScope::SpacetimeSourcesOnly:
			return b.is_spacetime_source;
		case BulkActionScope::ByTypeCategory:
			if (b.is_spacetime_source) {
				return (cat == CatalogTypeCategory::Custom || cat == CatalogTypeCategory::All);
			}
			switch (cat) {
				case CatalogTypeCategory::All: return true;
				case CatalogTypeCategory::Terrestrial: return b.preset_3d == Dynamics::Body3DPreset::TerrestrialPlanet;
				case CatalogTypeCategory::GasGiant: return b.preset_3d == Dynamics::Body3DPreset::GasGiant;
				case CatalogTypeCategory::IceGiant: return b.preset_3d == Dynamics::Body3DPreset::IceGiant;
				case CatalogTypeCategory::Star: return b.preset_3d == Dynamics::Body3DPreset::Star;
				case CatalogTypeCategory::Compact: return b.preset_3d == Dynamics::Body3DPreset::NeutronStar || b.preset_3d == Dynamics::Body3DPreset::Pulsar;
				case CatalogTypeCategory::SmallBody: return b.preset_3d == Dynamics::Body3DPreset::Metallic || b.preset_3d == Dynamics::Body3DPreset::Asteroid;
				case CatalogTypeCategory::Custom: return b.preset_3d == Dynamics::Body3DPreset::Custom || b.preset_3d == Dynamics::Body3DPreset::BlackHole;
				default: return true;
			}
		default:
			return true;
	}
}

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
	bool lock_disk_inner_to_isco{true};
	float target_eccentricity{0.0f};
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

enum class PatternGeometryType : uint32_t {
	AsteroidBelt = 0,
	DebrisSwarm = 1,
	SphericalFormation = 2
};

inline constexpr std::array<const char*, 3> kPatternGeometryNames{
	"Annular Asteroid Belt",
	"Coplanar Debris Swarm",
	"Spherical Test Formation"
};

enum class RadialDistributionLaw : uint32_t {
	Uniform = 0,
	Gaussian = 1,
	PowerLaw = 2,
	KirkwoodResonant = 3
};

inline constexpr std::array<const char*, 4> kRadialDistributionNames{
	"Uniform Radial Band",
	"Gaussian Profile",
	"Power-Law Radial Falloff",
	"Kirkwood Resonances (Gapped)"
};

enum class AngularDistributionLaw : uint32_t {
	Uniform = 0,
	ClusteredLagrange = 1,
	SpiralWave = 2,
	FibonacciLattice = 3
};

inline constexpr std::array<const char*, 4> kAngularDistributionNames{
	"Uniform Azimuthal (Full 360)",
	"Lagrangian Clustered (L4/L5 Swarm)",
	"Spiral Density Wave",
	"Fibonacci Spherical Lattice"
};

enum class DispersionDistributionLaw : uint32_t {
	Rayleigh = 0,
	Gaussian = 1,
	Uniform = 2,
	HalfGaussian = 3,
	PowerLaw = 4
};

inline constexpr std::array<const char*, 5> kDispersionDistributionNames{
	"Rayleigh Distribution",
	"Gaussian Distribution",
	"Uniform Range",
	"Half-Gaussian (Positive)",
	"Power-Law Distribution"
};

enum class VelocityInjectionMode : uint32_t {
	KeplerianCircular = 0,
	KeplerianEccentric = 1,
	RadialExpansion = 2,
	RadialCollapse = 3,
	IsotropicVirial = 4,
	SolidBodyRotation = 5,
	FixedVelocity = 6
};

inline constexpr std::array<const char*, 7> kVelocityInjectionModeNames{
	"Keplerian Circular Velocity (v = v_k)",
	"Keplerian Eccentric Orbit (Periapsis v = v_peri)",
	"Radial Blast / Expansion (Hubble Flow)",
	"Radial Collapse / Infall (Free Fall)",
	"Isotropic Virialized Dispersion",
	"Solid-Body Rotation (v = omega x r)",
	"Uniform Directional Velocity Vector"
};

enum class VelocityDistributionLaw : uint32_t {
	MaxwellBoltzmann = 0,
	Gaussian = 1,
	Uniform = 2,
	PowerLaw = 3
};

inline constexpr std::array<const char*, 4> kVelocityDistributionNames{
	"Maxwell-Boltzmann (Thermal Dispersion)",
	"Gaussian Velocity Dispersion",
	"Uniform Velocity Scatter",
	"Power-Law High-Velocity Tail"
};

enum class MassDistributionLaw : uint32_t {
	Fixed = 0,
	Uniform = 1,
	LogUniform = 2,
	PowerLaw = 3,
	DerivedFromDensity = 4
};

inline constexpr std::array<const char*, 5> kMassDistributionNames{
	"Identical Fixed Mass",
	"Uniform Interval [m_min, m_max]",
	"Log-Uniform Range [m_min, m_max]",
	"Power-Law (Salpeter / Dohnanyi)",
	"Derived from Volume & Density"
};

enum class SphericalFormationType : uint32_t {
	ThinShell = 0,
	ThickShell = 1,
	PlummerSphere = 2,
	UniformSphere = 3,
	FibonacciShell = 4,
	InfallingBlast = 5
};

inline constexpr std::array<const char*, 6> kSphericalFormationNames{
	"Thin Spherical Shell (Dyson Grid)",
	"Thick Radial Shell (Hollow Core)",
	"Plummer Polytropic Cluster",
	"Uniform Solid Density Sphere",
	"Fibonacci Geodesic Lattice Shell",
	"Infalling / Collapsing Blast Wave"
};

enum class PatternPreset : uint32_t {
	MainAsteroidBelt = 0,
	KuiperBeltAnalogue = 1,
	DenseTrojanSwarm = 2,
	NarrowPlanetaryRing = 3,
	TidalDisruptionStream = 4,
	CollisionDebrisCloud = 5,
	PlummerStarCluster = 6,
	DysonGeodesicShell = 7,
	SupernovaBlastExpansion = 8,
	Custom = 9
};

inline constexpr std::array<const char*, 10> kPatternPresetNames{
	"Main Asteroid Belt (Inner Solar System)",
	"Kuiper Belt Analogue (Cold Classical)",
	"Trojan Asteroid Swarm (Lagrangian L4/L5)",
	"Narrow Dense Planetary Ring",
	"Tidal Disruption Debris Stream",
	"High-Speed Collision Fragment Cloud",
	"Plummer Globular Star Cluster",
	"Dyson Geodesic Shell (Fibonacci Lattice)",
	"Supernova Blast Expansion Wave",
	"Custom Configuration"
};

struct GeometricPatternConfig {
	PatternGeometryType pattern_type{PatternGeometryType::AsteroidBelt};
	PatternPreset preset{PatternPreset::MainAsteroidBelt};
	int32_t object_count{150};
	int32_t parent_body_id{-2};
	std::array<double, 3> center_position{0.0, 0.0, 0.0};
	std::array<double, 3> center_velocity{0.0, 0.0, 0.0};
	double plane_inclination_deg{0.0};
	double plane_longitude_node_deg{0.0};
	double plane_argument_periapsis_deg{0.0};

	double mean_semi_major_axis{32.0};
	double radial_thickness{12.0};
	RadialDistributionLaw radial_distribution{RadialDistributionLaw::Uniform};
	double radial_gaussian_sigma{2.5};
	double radial_power_law_index{-1.5};
	bool kirkwood_gaps_enabled{false};
	int32_t kirkwood_gap_count{3};
	double kirkwood_resonance_depth{0.85};

	double vertical_scale_height{1.0};
	DispersionDistributionLaw vertical_distribution{DispersionDistributionLaw::Gaussian};

	double mean_eccentricity{0.06};
	double eccentricity_dispersion{0.035};
	DispersionDistributionLaw eccentricity_distribution{DispersionDistributionLaw::Rayleigh};

	double mean_inclination_dispersion_deg{3.5};
	DispersionDistributionLaw inclination_distribution{DispersionDistributionLaw::Rayleigh};

	AngularDistributionLaw angular_distribution{AngularDistributionLaw::Uniform};
	double angular_opening_deg{360.0};
	double angular_center_deg{0.0};
	double lagrangian_clustering_strength{0.8};

	VelocityInjectionMode velocity_mode{VelocityInjectionMode::KeplerianCircular};
	double mean_velocity_multiplier{1.0};
	double velocity_dispersion_fraction{0.03};
	VelocityDistributionLaw velocity_distribution{VelocityDistributionLaw::MaxwellBoltzmann};
	double prograde_fraction{1.0};
	std::array<double, 3> fixed_velocity_vector{0.0, 0.0, 0.0};
	double expansion_speed{0.05};

	SphericalFormationType spherical_type{SphericalFormationType::PlummerSphere};
	double thin_shell_radius{28.0};
	double plummer_scale_radius{16.0};
	double uniform_sphere_radius{25.0};
	double shell_inner_radius{14.0};
	double shell_outer_radius{42.0};
	double spherical_power_law_gamma{-2.0};
	double solid_body_rotation_speed{0.04};
	double solid_body_axis_tilt_deg{0.0};
	double solid_body_axis_azimuth_deg{0.0};

	Dynamics::Body3DPreset archetype{Dynamics::Body3DPreset::Asteroid};
	MassDistributionLaw mass_distribution{MassDistributionLaw::PowerLaw};
	double fixed_mass{1.0e-7};
	double min_mass{1.0e-9};
	double max_mass{2.0e-6};
	double mass_power_law_alpha{1.833};

	bool radius_from_density{true};
	double mean_density{2700.0};
	double density_dispersion_pct{15.0};
	double fixed_radius{0.05};
	double min_radius{0.005};
	double max_radius{0.4};

	bool temperature_equilibrium{true};
	double reference_temperature_k{250.0};
	double temperature_falloff_exponent{0.5};

	double color_jitter{0.2};
	bool randomize_appearance{true};
	std::string name_prefix{"Asteroid"};

	bool avoid_horizon_overlap{true};
	bool avoid_mutual_collisions{true};
	uint64_t seed{1337};

	void sanitize() noexcept {
		object_count = std::clamp(object_count, 2, 2500);
		mean_semi_major_axis = std::clamp(mean_semi_major_axis, 1.0, 50000.0);
		radial_thickness = std::clamp(radial_thickness, 0.01, mean_semi_major_axis * 1.95);
		radial_gaussian_sigma = std::clamp(radial_gaussian_sigma, 0.01, mean_semi_major_axis);
		radial_power_law_index = std::clamp(radial_power_law_index, -5.0, 5.0);
		kirkwood_gap_count = std::clamp(kirkwood_gap_count, 1, 8);
		kirkwood_resonance_depth = std::clamp(kirkwood_resonance_depth, 0.0, 1.0);
		vertical_scale_height = std::clamp(vertical_scale_height, 0.0, mean_semi_major_axis);
		mean_eccentricity = std::clamp(mean_eccentricity, 0.0, 0.999);
		eccentricity_dispersion = std::clamp(eccentricity_dispersion, 0.0, 0.5);
		mean_inclination_dispersion_deg = std::clamp(mean_inclination_dispersion_deg, 0.0, 90.0);
		angular_opening_deg = std::clamp(angular_opening_deg, 1.0, 360.0);
		lagrangian_clustering_strength = std::clamp(lagrangian_clustering_strength, 0.0, 1.0);
		mean_velocity_multiplier = std::clamp(mean_velocity_multiplier, 0.0, 10.0);
		velocity_dispersion_fraction = std::clamp(velocity_dispersion_fraction, 0.0, 2.0);
		prograde_fraction = std::clamp(prograde_fraction, 0.0, 1.0);
		expansion_speed = std::clamp(expansion_speed, 0.0, 10.0);
		thin_shell_radius = std::clamp(thin_shell_radius, 0.1, 50000.0);
		plummer_scale_radius = std::clamp(plummer_scale_radius, 0.1, 50000.0);
		uniform_sphere_radius = std::clamp(uniform_sphere_radius, 0.1, 50000.0);
		shell_inner_radius = std::clamp(shell_inner_radius, 0.1, 50000.0);
		shell_outer_radius = std::clamp(shell_outer_radius, shell_inner_radius * 1.01, 50000.0);
		spherical_power_law_gamma = std::clamp(spherical_power_law_gamma, -5.0, 5.0);
		solid_body_rotation_speed = std::clamp(solid_body_rotation_speed, -10.0, 10.0);
		min_mass = std::max(min_mass, 0.0);
		max_mass = std::max(max_mass, min_mass);
		fixed_mass = std::max(fixed_mass, 0.0);
		mass_power_law_alpha = std::clamp(mass_power_law_alpha, 0.1, 5.0);
		mean_density = std::clamp(mean_density, 10.0, 1.0e6);
		density_dispersion_pct = std::clamp(density_dispersion_pct, 0.0, 100.0);
		min_radius = std::clamp(min_radius, 1e-6, 1000.0);
		max_radius = std::clamp(max_radius, min_radius, 5000.0);
		fixed_radius = std::clamp(fixed_radius, 1e-6, 5000.0);
		reference_temperature_k = std::clamp(reference_temperature_k, 0.0, 1.0e6);
		temperature_falloff_exponent = std::clamp(temperature_falloff_exponent, 0.0, 2.0);
		color_jitter = std::clamp(color_jitter, 0.0, 1.0);
	}

	void apply_preset(PatternPreset p) noexcept {
		preset = p;
		switch (p) {
			case PatternPreset::MainAsteroidBelt:
				pattern_type = PatternGeometryType::AsteroidBelt;
				object_count = 150;
				mean_semi_major_axis = 32.0;
				radial_thickness = 12.0;
				radial_distribution = RadialDistributionLaw::Uniform;
				vertical_scale_height = 0.9;
				vertical_distribution = DispersionDistributionLaw::Gaussian;
				mean_eccentricity = 0.06;
				eccentricity_dispersion = 0.035;
				eccentricity_distribution = DispersionDistributionLaw::Rayleigh;
				mean_inclination_dispersion_deg = 3.5;
				inclination_distribution = DispersionDistributionLaw::Rayleigh;
				angular_distribution = AngularDistributionLaw::Uniform;
				velocity_mode = VelocityInjectionMode::KeplerianCircular;
				mean_velocity_multiplier = 1.0;
				velocity_dispersion_fraction = 0.025;
				velocity_distribution = VelocityDistributionLaw::MaxwellBoltzmann;
				prograde_fraction = 1.0;
				archetype = Dynamics::Body3DPreset::Asteroid;
				mass_distribution = MassDistributionLaw::PowerLaw;
				mass_power_law_alpha = 1.833;
				min_mass = 1.0e-9;
				max_mass = 2.0e-6;
				radius_from_density = true;
				mean_density = 2700.0;
				density_dispersion_pct = 15.0;
				temperature_equilibrium = true;
				reference_temperature_k = 250.0;
				name_prefix = "Asteroid";
				break;
			case PatternPreset::KuiperBeltAnalogue:
				pattern_type = PatternGeometryType::AsteroidBelt;
				object_count = 200;
				mean_semi_major_axis = 65.0;
				radial_thickness = 24.0;
				radial_distribution = RadialDistributionLaw::Gaussian;
				radial_gaussian_sigma = 6.0;
				vertical_scale_height = 2.2;
				vertical_distribution = DispersionDistributionLaw::Gaussian;
				mean_eccentricity = 0.08;
				eccentricity_dispersion = 0.05;
				eccentricity_distribution = DispersionDistributionLaw::Rayleigh;
				mean_inclination_dispersion_deg = 6.0;
				inclination_distribution = DispersionDistributionLaw::Rayleigh;
				angular_distribution = AngularDistributionLaw::Uniform;
				velocity_mode = VelocityInjectionMode::KeplerianCircular;
				mean_velocity_multiplier = 1.0;
				velocity_dispersion_fraction = 0.03;
				velocity_distribution = VelocityDistributionLaw::MaxwellBoltzmann;
				prograde_fraction = 1.0;
				archetype = Dynamics::Body3DPreset::IceGiant;
				mass_distribution = MassDistributionLaw::PowerLaw;
				mass_power_law_alpha = 1.95;
				min_mass = 2.0e-9;
				max_mass = 8.0e-6;
				radius_from_density = true;
				mean_density = 1500.0;
				density_dispersion_pct = 20.0;
				temperature_equilibrium = true;
				reference_temperature_k = 70.0;
				name_prefix = "KBO";
				break;
			case PatternPreset::DenseTrojanSwarm:
				pattern_type = PatternGeometryType::DebrisSwarm;
				object_count = 120;
				mean_semi_major_axis = 35.0;
				radial_thickness = 4.5;
				radial_distribution = RadialDistributionLaw::Gaussian;
				radial_gaussian_sigma = 1.2;
				vertical_scale_height = 0.6;
				vertical_distribution = DispersionDistributionLaw::Gaussian;
				mean_eccentricity = 0.04;
				eccentricity_dispersion = 0.02;
				eccentricity_distribution = DispersionDistributionLaw::Rayleigh;
				mean_inclination_dispersion_deg = 2.0;
				inclination_distribution = DispersionDistributionLaw::Rayleigh;
				angular_distribution = AngularDistributionLaw::ClusteredLagrange;
				angular_opening_deg = 40.0;
				lagrangian_clustering_strength = 0.85;
				velocity_mode = VelocityInjectionMode::KeplerianCircular;
				mean_velocity_multiplier = 1.0;
				velocity_dispersion_fraction = 0.02;
				velocity_distribution = VelocityDistributionLaw::Gaussian;
				prograde_fraction = 1.0;
				archetype = Dynamics::Body3DPreset::Asteroid;
				mass_distribution = MassDistributionLaw::PowerLaw;
				mass_power_law_alpha = 1.833;
				min_mass = 5.0e-10;
				max_mass = 1.0e-6;
				radius_from_density = true;
				mean_density = 2400.0;
				name_prefix = "Trojan";
				break;
			case PatternPreset::NarrowPlanetaryRing:
				pattern_type = PatternGeometryType::AsteroidBelt;
				object_count = 250;
				mean_semi_major_axis = 18.0;
				radial_thickness = 1.5;
				radial_distribution = RadialDistributionLaw::Uniform;
				vertical_scale_height = 0.08;
				vertical_distribution = DispersionDistributionLaw::Gaussian;
				mean_eccentricity = 0.003;
				eccentricity_dispersion = 0.002;
				eccentricity_distribution = DispersionDistributionLaw::Rayleigh;
				mean_inclination_dispersion_deg = 0.35;
				inclination_distribution = DispersionDistributionLaw::Rayleigh;
				angular_distribution = AngularDistributionLaw::Uniform;
				velocity_mode = VelocityInjectionMode::KeplerianCircular;
				mean_velocity_multiplier = 1.0;
				velocity_dispersion_fraction = 0.005;
				velocity_distribution = VelocityDistributionLaw::MaxwellBoltzmann;
				prograde_fraction = 1.0;
				archetype = Dynamics::Body3DPreset::Metallic;
				mass_distribution = MassDistributionLaw::LogUniform;
				min_mass = 1.0e-11;
				max_mass = 1.0e-8;
				radius_from_density = true;
				mean_density = 1000.0;
				name_prefix = "Ring Particle";
				break;
			case PatternPreset::TidalDisruptionStream:
				pattern_type = PatternGeometryType::DebrisSwarm;
				object_count = 180;
				mean_semi_major_axis = 22.0;
				radial_thickness = 8.0;
				radial_distribution = RadialDistributionLaw::PowerLaw;
				radial_power_law_index = -2.0;
				vertical_scale_height = 0.35;
				vertical_distribution = DispersionDistributionLaw::Gaussian;
				mean_eccentricity = 0.65;
				eccentricity_dispersion = 0.12;
				eccentricity_distribution = DispersionDistributionLaw::Gaussian;
				mean_inclination_dispersion_deg = 1.2;
				inclination_distribution = DispersionDistributionLaw::Rayleigh;
				angular_distribution = AngularDistributionLaw::Uniform;
				angular_opening_deg = 160.0;
				velocity_mode = VelocityInjectionMode::KeplerianEccentric;
				mean_velocity_multiplier = 1.0;
				velocity_dispersion_fraction = 0.05;
				velocity_distribution = VelocityDistributionLaw::Gaussian;
				prograde_fraction = 1.0;
				archetype = Dynamics::Body3DPreset::Asteroid;
				mass_distribution = MassDistributionLaw::PowerLaw;
				mass_power_law_alpha = 2.1;
				min_mass = 1.0e-10;
				max_mass = 5.0e-7;
				name_prefix = "Debris Shard";
				break;
			case PatternPreset::CollisionDebrisCloud:
				pattern_type = PatternGeometryType::DebrisSwarm;
				object_count = 100;
				mean_semi_major_axis = 25.0;
				radial_thickness = 4.0;
				radial_distribution = RadialDistributionLaw::Gaussian;
				radial_gaussian_sigma = 1.5;
				vertical_scale_height = 1.8;
				vertical_distribution = DispersionDistributionLaw::Gaussian;
				mean_eccentricity = 0.15;
				eccentricity_dispersion = 0.08;
				eccentricity_distribution = DispersionDistributionLaw::Gaussian;
				mean_inclination_dispersion_deg = 8.0;
				inclination_distribution = DispersionDistributionLaw::Rayleigh;
				angular_distribution = AngularDistributionLaw::Uniform;
				angular_opening_deg = 60.0;
				velocity_mode = VelocityInjectionMode::RadialExpansion;
				expansion_speed = 0.08;
				velocity_dispersion_fraction = 0.08;
				velocity_distribution = VelocityDistributionLaw::MaxwellBoltzmann;
				prograde_fraction = 1.0;
				archetype = Dynamics::Body3DPreset::Metallic;
				mass_distribution = MassDistributionLaw::PowerLaw;
				mass_power_law_alpha = 2.2;
				min_mass = 1.0e-10;
				max_mass = 1.0e-6;
				name_prefix = "Fragment";
				break;
			case PatternPreset::PlummerStarCluster:
				pattern_type = PatternGeometryType::SphericalFormation;
				spherical_type = SphericalFormationType::PlummerSphere;
				object_count = 160;
				plummer_scale_radius = 16.0;
				velocity_mode = VelocityInjectionMode::IsotropicVirial;
				mean_velocity_multiplier = 1.0;
				velocity_dispersion_fraction = 0.25;
				velocity_distribution = VelocityDistributionLaw::MaxwellBoltzmann;
				archetype = Dynamics::Body3DPreset::Star;
				mass_distribution = MassDistributionLaw::PowerLaw;
				mass_power_law_alpha = 2.35;
				min_mass = 1.0e-6;
				max_mass = 1.0e-3;
				radius_from_density = false;
				min_radius = 0.05;
				max_radius = 0.45;
				name_prefix = "Cluster Star";
				break;
			case PatternPreset::DysonGeodesicShell:
				pattern_type = PatternGeometryType::SphericalFormation;
				spherical_type = SphericalFormationType::FibonacciShell;
				object_count = 120;
				thin_shell_radius = 28.0;
				velocity_mode = VelocityInjectionMode::SolidBodyRotation;
				solid_body_rotation_speed = 0.035;
				velocity_dispersion_fraction = 0.0;
				archetype = Dynamics::Body3DPreset::Metallic;
				mass_distribution = MassDistributionLaw::Fixed;
				fixed_mass = 1.0e-8;
				radius_from_density = false;
				fixed_radius = 0.2;
				name_prefix = "Dyson Node";
				break;
			case PatternPreset::SupernovaBlastExpansion:
				pattern_type = PatternGeometryType::SphericalFormation;
				spherical_type = SphericalFormationType::InfallingBlast;
				object_count = 150;
				shell_inner_radius = 16.0;
				shell_outer_radius = 24.0;
				spherical_power_law_gamma = -1.0;
				velocity_mode = VelocityInjectionMode::RadialExpansion;
				expansion_speed = 0.22;
				velocity_dispersion_fraction = 0.06;
				velocity_distribution = VelocityDistributionLaw::MaxwellBoltzmann;
				archetype = Dynamics::Body3DPreset::GasGiant;
				mass_distribution = MassDistributionLaw::LogUniform;
				min_mass = 1.0e-9;
				max_mass = 5.0e-6;
				name_prefix = "Blast Knot";
				break;
			case PatternPreset::Custom:
			default:
				break;
		}
		sanitize();
	}
};

}

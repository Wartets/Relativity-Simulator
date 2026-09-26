#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include <cmath>
#include <algorithm>
#include <string_view>
#include "relativistic/render/gpu_types.hpp"

namespace Relativistic::Dynamics {

enum class Body3DGeometryModel : uint32_t {
	OblateSpheroid  = 0,
	RigidSphere     = 1,
	Sphere          = 1, // alias
	ProlateSpheroid = 2,
	TriaxialEllipsoid = 3,
	Toroid          = 4,
	Mesh            = 5
};

enum class Body3DSurfaceTextureMode : uint32_t {
	ProceduralNoise     = 0,
	SolidColor          = 1,
	ColorPalette        = 2,
	BandedGasGiant      = 3,
	CrateredTerrestrial = 4,
	StellarGranulation  = 5,
	AccretionFlow       = 6,
	MarbledStone        = 7,
	RingedGasGiant      = 8,
	IcyCracked          = 9,
	VolcanicMagma       = 10,
	CityLightsNightSide = 11,
	NebulousGas         = 12
};

enum class Body3DAtmosphereMode : uint32_t {
	RayleighLimbShell   = 0,
	VolumetricScattering = 1,
	Off                 = 2,
	None                = 2, // alias
	ThickHaze           = 3,
	VolumetricMie       = 4,
	GlowingCorona       = 5
};

enum class Body3DPreset : uint32_t {
	Star             = 0,
	Terrestrial      = 1,
	TerrestrialPlanet = 1, // alias
	GasGiant         = 2,
	IceGiant         = 3,
	Metallic         = 4,
	Moon             = 4, // alias
	Asteroid         = 5,
	NeutronStar      = 6,
	Pulsar           = 7,
	BlackHole        = 8,
	Custom           = 9
};

struct alignas(64) PostNewtonianBody {
	uint32_t id{0};
	double mass{1.0};
	double radius{1.0};
	std::array<double, 3> position{0.0, 0.0, 0.0};
	std::array<double, 3> velocity{0.0, 0.0, 0.0};
	std::array<double, 3> acceleration{0.0, 0.0, 0.0};
	std::array<double, 3> spin{0.0, 0.0, 0.0};
	bool enabled{true};
	bool is_spacetime_source{false};
	std::array<float, 4> color{0.62f, 0.75f, 1.0f, 1.0f};
	std::array<float, 4> color_secondary{0.18f, 0.30f, 0.75f, 1.0f};
	double charge{0.0};
	double magnetic_moment{0.0};
	double rotation_speed{0.0};
	double friction_coefficient{0.0};
	double restitution{0.5};
	double integrity{1.0};
	double lifetime{0.0};
	double elasticity{0.0};
	double youngs_modulus_cold{0.0};
	double youngs_modulus_hot{0.0};
	double temperature{0.0};
	double heat_capacity{0.0};
	double absorption_factor{0.0};
	double critical_temperature{0.0};
	double transmission_factor{0.0};
	double cold_resistance{0.0};
	double hot_resistance{0.0};
	std::array<char, 32> composition{};
	double quadrupole_moment{0.0};
	double j2{0.0};
	double j3{0.0};
	double j4{0.0};
	double reference_radius{0.0};
	std::array<char, 32> name{};

	Body3DGeometryModel      geometry_model{Body3DGeometryModel::OblateSpheroid};
	Body3DSurfaceTextureMode surface_texture_mode{Body3DSurfaceTextureMode::ProceduralNoise};
	Body3DAtmosphereMode     atmosphere_mode{Body3DAtmosphereMode::Off};
	Body3DPreset             preset_3d{Body3DPreset::Star};
	float surface_noise_scale{4.0f};
	float surface_roughness{0.5f};
	float atmosphere_thickness{0.15f};
	std::array<float, 4> atmosphere_color{0.58f, 0.74f, 0.95f, 0.28f};
	float emission_intensity{0.0f};
	float specular_roughness{0.3f};
	float rotation_speed_3d{0.1f};
	std::array<double, 3> rotation_axis_3d{0.0, 0.0, 1.0};
	std::array<float, 4> color_tertiary{0.9f, 0.85f, 0.6f, 1.0f};
	float texture_detail_scale{1.0f};
	float polar_cap_strength{0.0f};
	bool ring_system_enabled{false};
	float night_side_light_intensity{0.0f};

	void set_name(std::string_view new_name) noexcept {
		const size_t len = std::min(new_name.size(), name.size() - 1);
		for (size_t i = 0; i < len; ++i) { name[i] = new_name[i]; }
		name[len] = '\0';
	}

	[[nodiscard]] bool has_name() const noexcept { return name[0] != '\0'; }

	[[nodiscard]] std::string_view name_view() const noexcept {
		return std::string_view(name.data());
	}

	constexpr PostNewtonianBody() noexcept = default;

	constexpr PostNewtonianBody(
		uint32_t body_id,
		double m,
		double r,
		const std::array<double, 3>& pos,
		const std::array<double, 3>& vel,
		const std::array<double, 3>& s = {0.0, 0.0, 0.0},
		double q = 0.0,
		double j2_val = 0.0,
		double j3_val = 0.0,
		double j4_val = 0.0,
		double r_ref = 0.0
	) noexcept
		: id(body_id),
		  mass(m),
		  radius(r),
		  position(pos),
		  velocity(vel),
		  acceleration{0.0, 0.0, 0.0},
		  spin(s),
		  quadrupole_moment(q),
		  j2(j2_val),
		  j3(j3_val),
		  j4(j4_val),
		  reference_radius((r_ref > 0.0) ? r_ref : r) {}

	[[nodiscard]] double speed_squared() const noexcept {
		return velocity[0]*velocity[0] + velocity[1]*velocity[1] + velocity[2]*velocity[2];
	}
	[[nodiscard]] double speed() const noexcept { return std::sqrt(speed_squared()); }

	[[nodiscard]] double spin_magnitude_squared() const noexcept {
		return spin[0]*spin[0] + spin[1]*spin[1] + spin[2]*spin[2];
	}
	[[nodiscard]] double spin_magnitude() const noexcept { return std::sqrt(spin_magnitude_squared()); }

	[[nodiscard]] double kinetic_energy() const noexcept { return 0.5 * mass * speed_squared(); }

	[[nodiscard]] double kerr_spin_parameter() const noexcept {
		return (mass > 1e-12) ? std::clamp(spin_magnitude() / mass, 0.0, 0.999) : 0.0;
	}

	[[nodiscard]] double kerr_outer_horizon_radius() const noexcept {
		const double rg = mass;
		const double a = rg * kerr_spin_parameter();
		return rg + std::sqrt(std::max(rg * rg - a * a, 0.0));
	}

	void set_composition(std::string_view value) noexcept {
		const size_t len = std::min(value.size(), composition.size() - 1);
		for (size_t i = 0; i < len; ++i) composition[i] = value[i];
		composition[len] = '\0';
	}

	[[nodiscard]] Render::GpuBodyData to_gpu_body_data() const noexcept {
		Render::GpuBodyData gpu{};
		gpu.position = {position[0], position[1], position[2], 0.0};
		gpu.velocity = {velocity[0], velocity[1], velocity[2], 0.0};
		gpu.color_primary = {color[0], color[1], color[2], color[3]};
		gpu.color_secondary = {color_secondary[0], color_secondary[1], color_secondary[2], color_secondary[3]};
		gpu.atmosphere_color = {atmosphere_color[0], atmosphere_color[1], atmosphere_color[2], atmosphere_color[3]};
		gpu.radius = radius;
		gpu.mass = mass;
		gpu.charge = charge;
		gpu.temperature = temperature;
		gpu.noise_scale = static_cast<double>(surface_noise_scale);
		gpu.noise_roughness = static_cast<double>(surface_roughness);
		gpu.atmosphere_thickness = static_cast<double>(atmosphere_thickness);
		gpu.specular_roughness = static_cast<double>(specular_roughness);
		gpu.emission_intensity = static_cast<double>(emission_intensity);
		gpu.rotation_speed = static_cast<double>(rotation_speed_3d);
		gpu.oblateness_ratio = (std::abs(j2) > 1e-9) ? (1.0 - j2) : 1.0;
		gpu.color_tertiary = {color_tertiary[0], color_tertiary[1], color_tertiary[2], color_tertiary[3]};
		gpu.texture_detail_scale = static_cast<double>(texture_detail_scale);
		gpu.polar_cap_strength = static_cast<double>(polar_cap_strength);
		gpu.ring_system_enabled = ring_system_enabled ? 1.0 : 0.0;
		gpu.night_side_light_intensity = static_cast<double>(night_side_light_intensity);
		gpu.body_id = id;
		gpu.geometry_model = static_cast<uint32_t>(geometry_model);
		gpu.surface_texture_mode = static_cast<uint32_t>(surface_texture_mode);
		gpu.atmosphere_mode = static_cast<uint32_t>(atmosphere_mode);
		gpu.preset_3d = static_cast<uint32_t>(preset_3d);
		return gpu;
	}
};

}

namespace Relativistic {
using Dynamics::Body3DGeometryModel;
using Dynamics::Body3DSurfaceTextureMode;
using Dynamics::Body3DAtmosphereMode;
using Dynamics::Body3DPreset;
}

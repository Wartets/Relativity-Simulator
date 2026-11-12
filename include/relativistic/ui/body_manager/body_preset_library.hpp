#pragma once

#include "relativistic/dynamics/pn/pn_body.hpp"
#include <algorithm>
#include <array>
#include <cmath>

namespace Relativistic::UI {

[[nodiscard]] inline std::array<float, 3> body_hsv_to_rgb(float h, float s, float v) noexcept {
	const float c = v * s;
	const float hp = std::fmod(h * 6.0f, 6.0f);
	const float x = c * (1.0f - std::abs(std::fmod(hp, 2.0f) - 1.0f));
	float r = 0.0f, g = 0.0f, b = 0.0f;
	if (hp < 1.0f) { r = c; g = x; }
	else if (hp < 2.0f) { r = x; g = c; }
	else if (hp < 3.0f) { g = c; b = x; }
	else if (hp < 4.0f) { g = x; b = c; }
	else if (hp < 5.0f) { r = x; b = c; }
	else { r = c; b = x; }
	const float m = v - c;
	return {r + m, g + m, b + m};
}

inline void apply_body_material_preset(Dynamics::PostNewtonianBody& b, Dynamics::Body3DPreset preset) noexcept {
	double specific_heat = 900.0;
	b.friction_coefficient = 0.4;
	b.restitution = 0.5;
	b.absorption_factor = 0.7;
	b.integrity = 1.0;
	switch (preset) {
		case Dynamics::Body3DPreset::Star:
			b.temperature = 5772.0;
			b.absorption_factor = 1.0;
			b.restitution = 0.0;
			b.friction_coefficient = 0.0;
			specific_heat = 1.2e4;
			break;
		case Dynamics::Body3DPreset::TerrestrialPlanet:
			b.temperature = 288.0;
			specific_heat = 1000.0;
			break;
		case Dynamics::Body3DPreset::GasGiant:
		case Dynamics::Body3DPreset::IceGiant:
			b.temperature = 130.0;
			b.restitution = 0.1;
			specific_heat = 1.2e4;
			break;
		case Dynamics::Body3DPreset::Metallic:
			b.temperature = 220.0;
			specific_heat = 600.0;
			break;
		case Dynamics::Body3DPreset::NeutronStar:
		case Dynamics::Body3DPreset::Pulsar:
			b.temperature = 1.0e6;
			b.restitution = 0.0;
			b.friction_coefficient = 0.0;
			specific_heat = 1.0e3;
			break;
		default:
			b.temperature = 180.0;
			specific_heat = 800.0;
			break;
	}
	b.heat_capacity = std::max(b.mass, 1e-9) * specific_heat;
}

inline void apply_body_appearance_preset(Dynamics::PostNewtonianBody& b, Dynamics::Body3DPreset preset) noexcept {
	switch (preset) {
		case Dynamics::Body3DPreset::Star:
			b.atmosphere_mode = Dynamics::Body3DAtmosphereMode::GlowingCorona;
			b.surface_texture_mode = Dynamics::Body3DSurfaceTextureMode::StellarGranulation;
			b.color = {1.0f, 0.85f, 0.5f, 1.0f};
			b.color_secondary = {1.0f, 0.0f, 0.0f, 1.0f};
			b.color_tertiary = {1.0f, 0.95f, 0.7f, 1.0f};
			b.surface_noise_scale = 20.0f;
			b.surface_roughness = 0.82f;
			b.atmosphere_thickness = 0.0f;
			b.emission_intensity = 2.11f;
			b.rotation_speed_3d = 0.05f;
			b.specular_roughness = 1.0f;
			b.polar_cap_strength = 0.0f;
			b.night_side_light_intensity = 0.0f;
			b.ring_system_enabled = false;
			break;
		case Dynamics::Body3DPreset::TerrestrialPlanet:
			b.atmosphere_mode = Dynamics::Body3DAtmosphereMode::RayleighLimbShell;
			b.surface_texture_mode = Dynamics::Body3DSurfaceTextureMode::ProceduralNoise;
			b.color = {0.10f, 0.35f, 0.65f, 1.0f};
			b.color_secondary = {0.20f, 0.50f, 0.22f, 1.0f};
			b.color_tertiary = {0.95f, 0.95f, 0.98f, 1.0f};
			b.surface_noise_scale = 10.9f;
			b.surface_roughness = 0.05f;
			b.atmosphere_thickness = 0.18f;
			b.emission_intensity = 0.0f;
			b.rotation_speed_3d = 0.12f;
			b.specular_roughness = 0.2f;
			b.polar_cap_strength = 0.45f;
			b.night_side_light_intensity = 0.4f;
			b.ring_system_enabled = false;
			break;
		case Dynamics::Body3DPreset::GasGiant:
			b.atmosphere_mode = Dynamics::Body3DAtmosphereMode::ThickHaze;
			b.surface_texture_mode = Dynamics::Body3DSurfaceTextureMode::RingedGasGiant;
			b.color = {0.82f, 0.65f, 0.45f, 1.0f};
			b.color_secondary = {0.62f, 0.40f, 0.24f, 1.0f};
			b.color_tertiary = {0.58f, 0.29f, 0.0f, 1.0f};
			b.surface_noise_scale = 20.0f;
			b.surface_roughness = 0.3f;
			b.atmosphere_thickness = 0.32f;
			b.emission_intensity = 0.0f;
			b.rotation_speed_3d = 0.35f;
			b.specular_roughness = 0.5f;
			b.polar_cap_strength = 0.2f;
			b.night_side_light_intensity = 0.0f;
			b.ring_system_enabled = true;
			break;
		case Dynamics::Body3DPreset::IceGiant:
			b.atmosphere_mode = Dynamics::Body3DAtmosphereMode::VolumetricMie;
			b.surface_texture_mode = Dynamics::Body3DSurfaceTextureMode::IcyCracked;
			b.color = {0.55f, 0.75f, 0.90f, 1.0f};
			b.color_secondary = {0.35f, 0.55f, 0.80f, 1.0f};
			b.color_tertiary = {0.90f, 0.97f, 1.0f, 1.0f};
			b.surface_noise_scale = 4.0f;
			b.surface_roughness = 1.0f;
			b.atmosphere_thickness = 0.22f;
			b.emission_intensity = 0.0f;
			b.rotation_speed_3d = 0.28f;
			b.specular_roughness = 0.15f;
			b.polar_cap_strength = 0.1f;
			b.night_side_light_intensity = 0.0f;
			b.ring_system_enabled = false;
			break;
		case Dynamics::Body3DPreset::Metallic:
			b.atmosphere_mode = Dynamics::Body3DAtmosphereMode::Off;
			b.surface_texture_mode = Dynamics::Body3DSurfaceTextureMode::CrateredTerrestrial;
			b.color = {0.55f, 0.55f, 0.58f, 1.0f};
			b.color_secondary = {0.32f, 0.32f, 0.35f, 1.0f};
			b.color_tertiary = {0.75f, 0.75f, 0.78f, 1.0f};
			b.surface_noise_scale = 7.0f;
			b.surface_roughness = 0.75f;
			b.atmosphere_thickness = 0.0f;
			b.emission_intensity = 0.0f;
			b.rotation_speed_3d = 0.03f;
			b.specular_roughness = 0.35f;
			b.polar_cap_strength = 0.0f;
			b.night_side_light_intensity = 0.0f;
			b.ring_system_enabled = false;
			break;
		case Dynamics::Body3DPreset::Asteroid:
			b.atmosphere_mode = Dynamics::Body3DAtmosphereMode::Off;
			b.surface_texture_mode = Dynamics::Body3DSurfaceTextureMode::CrateredTerrestrial;
			b.color = {0.45f, 0.42f, 0.38f, 1.0f};
			b.color_secondary = {0.28f, 0.26f, 0.23f, 1.0f};
			b.color_tertiary = {0.60f, 0.56f, 0.50f, 1.0f};
			b.surface_noise_scale = 20.0f;
			b.surface_roughness = 1.0f;
			b.atmosphere_thickness = 0.0f;
			b.emission_intensity = 0.0f;
			b.rotation_speed_3d = 0.4f;
			b.specular_roughness = 0.85f;
			b.polar_cap_strength = 0.0f;
			b.night_side_light_intensity = 0.0f;
			b.ring_system_enabled = false;
			break;
		case Dynamics::Body3DPreset::NeutronStar:
			b.atmosphere_mode = Dynamics::Body3DAtmosphereMode::Off;
			b.surface_texture_mode = Dynamics::Body3DSurfaceTextureMode::StellarGranulation;
			b.color = {0.85f, 0.90f, 1.0f, 1.0f};
			b.color_secondary = {0.55f, 0.72f, 1.0f, 1.0f};
			b.color_tertiary = {1.0f, 1.0f, 1.0f, 1.0f};
			b.surface_noise_scale = 15.0f;
			b.surface_roughness = 0.15f;
			b.atmosphere_thickness = 0.0f;
			b.emission_intensity = 3.0f;
			b.rotation_speed_3d = 1.8f;
			b.specular_roughness = 1.0f;
			b.polar_cap_strength = 0.0f;
			b.night_side_light_intensity = 0.0f;
			b.ring_system_enabled = false;
			break;
		case Dynamics::Body3DPreset::Pulsar:
			b.atmosphere_mode = Dynamics::Body3DAtmosphereMode::Off;
			b.surface_texture_mode = Dynamics::Body3DSurfaceTextureMode::StellarGranulation;
			b.color = {0.65f, 0.80f, 1.0f, 1.0f};
			b.color_secondary = {0.40f, 0.55f, 1.0f, 1.0f};
			b.color_tertiary = {1.0f, 1.0f, 1.0f, 1.0f};
			b.surface_noise_scale = 1.2f;
			b.surface_roughness = 0.1f;
			b.atmosphere_thickness = 0.0f;
			b.emission_intensity = 4.0f;
			b.rotation_speed_3d = 6.0f;
			b.specular_roughness = 1.0f;
			b.polar_cap_strength = 0.0f;
			b.night_side_light_intensity = 0.0f;
			b.ring_system_enabled = false;
			break;
		case Dynamics::Body3DPreset::BlackHole:
			b.atmosphere_mode = Dynamics::Body3DAtmosphereMode::Off;
			b.surface_texture_mode = Dynamics::Body3DSurfaceTextureMode::SolidColor;
			b.color = {0.0f, 0.0f, 0.0f, 1.0f};
			b.color_secondary = {0.03f, 0.03f, 0.04f, 1.0f};
			b.color_tertiary = {0.04f, 0.04f, 0.03f, 1.0f};
			b.surface_noise_scale = 15.0f;
			b.surface_roughness = 0.79f;
			b.atmosphere_thickness = 0.0f;
			b.emission_intensity = 0.0f;
			b.rotation_speed_3d = 0.0f;
			b.specular_roughness = 0.0f;
			b.polar_cap_strength = 0.0f;
			b.night_side_light_intensity = 0.0f;
			b.ring_system_enabled = false;
			break;
		case Dynamics::Body3DPreset::Custom:
		default:
			b.preset_3d = Dynamics::Body3DPreset::Custom;
			return;
	}
	b.preset_3d = preset;
}

inline void apply_source_template_preset(Dynamics::PostNewtonianBody& b, SourcePresetTemplate preset, double mass_scale, double central_mass) noexcept {
	if (preset == SourcePresetTemplate::Custom) {
		return;
	}
	const size_t index = static_cast<size_t>(preset) - 1U;
	if (index >= kSourceTemplateSpecs.size()) {
		return;
	}
	const auto& spec = kSourceTemplateSpecs[index];
	b = Dynamics::PostNewtonianBody{};
	b.is_spacetime_source = true;
	apply_body_appearance_preset(b, Dynamics::Body3DPreset::BlackHole);
	b.mass = spec.native_units ? (spec.mass * std::max(central_mass, 1.0)) : (spec.mass / std::max(mass_scale, 1e-300));
	b.mass = std::max(b.mass, 1e-9);
	b.radius = b.kerr_outer_horizon_radius();
	b.reference_radius = b.radius;
	b.charge = spec.charge_ratio * b.mass;
	b.set_spin_state(spec.spin_ratio, {0.0, 0.0, 1.0});
	b.accretion_disk.apply_preset(static_cast<Render::AccretionDiskPreset>(spec.disk_preset_index), false);
	b.set_name(spec.name);
	b.enforce_spacetime_source_invariants();
}

}

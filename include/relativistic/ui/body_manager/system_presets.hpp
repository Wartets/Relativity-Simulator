#pragma once

#include "relativistic/dynamics/pn/pn_body.hpp"
#include "relativistic/dynamics/body_surface_layers.hpp"
#include "relativistic/optics/textures/earth_texture_image.hpp"
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/ui/body_manager/body_manager_context.hpp"
#include "relativistic/ui/body_manager/body_preset_library.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <string>
#include <vector>

namespace Relativistic::UI {

struct SystemBodyPresetSpec {
	std::string name{};
	double mass_kg{0.0};
	double radius_m{0.0};
	double semi_major_axis_m{0.0};
	double eccentricity{0.0};
	double inclination_deg{0.0};
	double longitude_node_deg{0.0};
	double argument_periapsis_deg{0.0};
	double true_anomaly_deg{0.0};
	double rotation_period_hours{24.0};
	double axial_tilt_deg{0.0};
	double temperature_k{288.0};
	Dynamics::Body3DPreset preset{Dynamics::Body3DPreset::TerrestrialPlanet};
	Dynamics::Body3DSurfaceTextureMode texture_mode{Dynamics::Body3DSurfaceTextureMode::ProceduralNoise};
	Optics::EarthMapVariant earth_variant{Optics::EarthMapVariant::Automatic};
	Optics::EarthMapQuality earth_quality{Optics::EarthMapQuality::Q2K};
	std::array<float, 4> color{0.5f, 0.5f, 0.5f, 1.0f};
	std::array<float, 4> color_secondary{0.3f, 0.3f, 0.3f, 1.0f};
	std::array<float, 4> color_tertiary{0.7f, 0.7f, 0.7f, 1.0f};
	float emission_intensity{0.0f};
	float atmosphere_thickness{0.0f};
	Dynamics::Body3DAtmosphereMode atmosphere_mode{Dynamics::Body3DAtmosphereMode::Off};
	bool ring_system{false};
	std::string parent_name{};
	int layer_template_1{-1};
	int layer_template_2{-1};
};

struct SystemPresetDefinition {
	std::string id{};
	std::string name{};
	std::string description{};
	std::string metric_name{"Flat Minkowski"};
	double central_mass_kg{1.98847e30};
	double central_radius_m{6.9634e8};
	std::vector<SystemBodyPresetSpec> bodies{};
};

class SystemPresetCatalog {
public:
	[[nodiscard]] static const std::vector<SystemPresetDefinition>& presets() noexcept {
		static const std::vector<SystemPresetDefinition> kPresets = []() {
			std::vector<SystemPresetDefinition> list;

			SystemPresetDefinition solar;
			solar.id = "solar_system_real";
			solar.name = "Complete Real Solar System (Physical Metric Scale)";
			solar.description = "The Sun and all major planets from Mercury to Neptune plus Pluto, Earth-Moon and the Galilean moons with true astrophysical masses, radii, and orbital elements.";
			solar.metric_name = "Flat Minkowski";
			solar.central_mass_kg = 1.98847e30;
			solar.central_radius_m = 6.9634e8;

			solar.bodies.push_back(SystemBodyPresetSpec{
				.name = "Sun",
				.mass_kg = 1.98847e30,
				.radius_m = 6.9634e8,
				.semi_major_axis_m = 0.0,
				.eccentricity = 0.0,
				.inclination_deg = 0.0,
				.longitude_node_deg = 0.0,
				.argument_periapsis_deg = 0.0,
				.true_anomaly_deg = 0.0,
				.rotation_period_hours = 609.12,
				.axial_tilt_deg = 7.25,
				.temperature_k = 5772.0,
				.preset = Dynamics::Body3DPreset::Star,
				.texture_mode = Dynamics::Body3DSurfaceTextureMode::StellarGranulation,
				.color = {1.0f, 0.88f, 0.55f, 1.0f},
				.color_secondary = {1.0f, 0.35f, 0.05f, 1.0f},
				.color_tertiary = {1.0f, 0.98f, 0.85f, 1.0f},
				.emission_intensity = 2.45f,
				.atmosphere_thickness = 0.0f,
				.atmosphere_mode = Dynamics::Body3DAtmosphereMode::GlowingCorona
			});

			solar.bodies.push_back(SystemBodyPresetSpec{
				.name = "Mercury",
				.mass_kg = 3.3011e23,
				.radius_m = 2.4397e6,
				.semi_major_axis_m = 5.7909e10,
				.eccentricity = 0.20563,
				.inclination_deg = 7.005,
				.longitude_node_deg = 48.331,
				.argument_periapsis_deg = 29.124,
				.true_anomaly_deg = 15.0,
				.rotation_period_hours = 1407.6,
				.axial_tilt_deg = 0.034,
				.temperature_k = 440.0,
				.preset = Dynamics::Body3DPreset::Metallic,
				.texture_mode = Dynamics::Body3DSurfaceTextureMode::CrateredTerrestrial,
				.color = {0.62f, 0.60f, 0.58f, 1.0f},
				.color_secondary = {0.35f, 0.33f, 0.32f, 1.0f},
				.color_tertiary = {0.78f, 0.76f, 0.74f, 1.0f},
				.parent_name = "Sun",
				.layer_template_1 = 3
			});

			solar.bodies.push_back(SystemBodyPresetSpec{
				.name = "Venus",
				.mass_kg = 4.8675e24,
				.radius_m = 6.0518e6,
				.semi_major_axis_m = 1.08209e11,
				.eccentricity = 0.00677,
				.inclination_deg = 3.3945,
				.longitude_node_deg = 76.680,
				.argument_periapsis_deg = 54.884,
				.true_anomaly_deg = 48.0,
				.rotation_period_hours = -5832.5,
				.axial_tilt_deg = 177.36,
				.temperature_k = 737.0,
				.preset = Dynamics::Body3DPreset::TerrestrialPlanet,
				.texture_mode = Dynamics::Body3DSurfaceTextureMode::ProceduralNoise,
				.color = {0.88f, 0.76f, 0.52f, 1.0f},
				.color_secondary = {0.72f, 0.58f, 0.36f, 1.0f},
				.color_tertiary = {0.95f, 0.85f, 0.65f, 1.0f},
				.atmosphere_thickness = 0.28f,
				.atmosphere_mode = Dynamics::Body3DAtmosphereMode::ThickHaze,
				.parent_name = "Sun",
				.layer_template_1 = 5
			});

			solar.bodies.push_back(SystemBodyPresetSpec{
				.name = "Earth",
				.mass_kg = 5.9722e24,
				.radius_m = 6.3710e6,
				.semi_major_axis_m = 1.4959787e11,
				.eccentricity = 0.01671,
				.inclination_deg = 0.000,
				.longitude_node_deg = -11.260,
				.argument_periapsis_deg = 114.207,
				.true_anomaly_deg = 100.0,
				.rotation_period_hours = 23.9344,
				.axial_tilt_deg = 23.44,
				.temperature_k = 288.15,
				.preset = Dynamics::Body3DPreset::TerrestrialPlanet,
				.texture_mode = Dynamics::Body3DSurfaceTextureMode::EarthBlueMarble,
				.earth_variant = Optics::EarthMapVariant::Automatic,
				.earth_quality = Optics::EarthMapQuality::Q2K,
				.color = {0.12f, 0.38f, 0.72f, 1.0f},
				.color_secondary = {0.22f, 0.52f, 0.25f, 1.0f},
				.color_tertiary = {0.95f, 0.95f, 0.98f, 1.0f},
				.atmosphere_thickness = 0.16f,
				.atmosphere_mode = Dynamics::Body3DAtmosphereMode::RayleighLimbShell,
				.parent_name = "Sun",
				.layer_template_1 = 2
			});

			solar.bodies.push_back(SystemBodyPresetSpec{
				.name = "Moon",
				.mass_kg = 7.342e22,
				.radius_m = 1.7374e6,
				.semi_major_axis_m = 3.844e8,
				.eccentricity = 0.0549,
				.inclination_deg = 5.145,
				.longitude_node_deg = 125.08,
				.argument_periapsis_deg = 318.15,
				.true_anomaly_deg = 45.0,
				.rotation_period_hours = 655.72,
				.axial_tilt_deg = 6.68,
				.temperature_k = 250.0,
				.preset = Dynamics::Body3DPreset::Metallic,
				.texture_mode = Dynamics::Body3DSurfaceTextureMode::CrateredTerrestrial,
				.color = {0.68f, 0.67f, 0.66f, 1.0f},
				.color_secondary = {0.42f, 0.41f, 0.40f, 1.0f},
				.color_tertiary = {0.85f, 0.84f, 0.83f, 1.0f},
				.parent_name = "Earth",
				.layer_template_1 = 3
			});

			solar.bodies.push_back(SystemBodyPresetSpec{
				.name = "Mars",
				.mass_kg = 6.4171e23,
				.radius_m = 3.3895e6,
				.semi_major_axis_m = 2.27937e11,
				.eccentricity = 0.09341,
				.inclination_deg = 1.850,
				.longitude_node_deg = 49.558,
				.argument_periapsis_deg = 286.502,
				.true_anomaly_deg = 140.0,
				.rotation_period_hours = 24.6229,
				.axial_tilt_deg = 25.19,
				.temperature_k = 210.0,
				.preset = Dynamics::Body3DPreset::TerrestrialPlanet,
				.texture_mode = Dynamics::Body3DSurfaceTextureMode::ProceduralNoise,
				.color = {0.78f, 0.38f, 0.22f, 1.0f},
				.color_secondary = {0.52f, 0.24f, 0.14f, 1.0f},
				.color_tertiary = {0.96f, 0.94f, 0.92f, 1.0f},
				.atmosphere_thickness = 0.05f,
				.atmosphere_mode = Dynamics::Body3DAtmosphereMode::RayleighLimbShell,
				.parent_name = "Sun",
				.layer_template_1 = 0,
				.layer_template_2 = 6
			});

			solar.bodies.push_back(SystemBodyPresetSpec{
				.name = "Jupiter",
				.mass_kg = 1.89813e27,
				.radius_m = 6.9911e7,
				.semi_major_axis_m = 7.78412e11,
				.eccentricity = 0.04839,
				.inclination_deg = 1.305,
				.longitude_node_deg = 100.464,
				.argument_periapsis_deg = 273.867,
				.true_anomaly_deg = 210.0,
				.rotation_period_hours = 9.925,
				.axial_tilt_deg = 3.13,
				.temperature_k = 165.0,
				.preset = Dynamics::Body3DPreset::GasGiant,
				.texture_mode = Dynamics::Body3DSurfaceTextureMode::RingedGasGiant,
				.color = {0.84f, 0.68f, 0.48f, 1.0f},
				.color_secondary = {0.64f, 0.42f, 0.26f, 1.0f},
				.color_tertiary = {0.92f, 0.86f, 0.76f, 1.0f},
				.atmosphere_thickness = 0.34f,
				.atmosphere_mode = Dynamics::Body3DAtmosphereMode::ThickHaze,
				.ring_system = true,
				.parent_name = "Sun",
				.layer_template_1 = 5
			});

			solar.bodies.push_back(SystemBodyPresetSpec{
				.name = "Io",
				.mass_kg = 8.9319e22,
				.radius_m = 1.8216e6,
				.semi_major_axis_m = 4.217e8,
				.eccentricity = 0.0041,
				.inclination_deg = 0.050,
				.longitude_node_deg = 43.0,
				.argument_periapsis_deg = 84.0,
				.true_anomaly_deg = 90.0,
				.rotation_period_hours = 42.46,
				.axial_tilt_deg = 0.0,
				.temperature_k = 130.0,
				.preset = Dynamics::Body3DPreset::TerrestrialPlanet,
				.texture_mode = Dynamics::Body3DSurfaceTextureMode::VolcanicMagma,
				.color = {0.92f, 0.84f, 0.22f, 1.0f},
				.color_secondary = {0.72f, 0.42f, 0.12f, 1.0f},
				.color_tertiary = {0.98f, 0.35f, 0.15f, 1.0f},
				.parent_name = "Jupiter"
			});

			solar.bodies.push_back(SystemBodyPresetSpec{
				.name = "Europa",
				.mass_kg = 4.800e22,
				.radius_m = 1.5608e6,
				.semi_major_axis_m = 6.709e8,
				.eccentricity = 0.009,
				.inclination_deg = 0.470,
				.longitude_node_deg = 219.0,
				.argument_periapsis_deg = 88.0,
				.true_anomaly_deg = 180.0,
				.rotation_period_hours = 85.23,
				.axial_tilt_deg = 0.1,
				.temperature_k = 102.0,
				.preset = Dynamics::Body3DPreset::IceGiant,
				.texture_mode = Dynamics::Body3DSurfaceTextureMode::IcyCracked,
				.color = {0.82f, 0.78f, 0.72f, 1.0f},
				.color_secondary = {0.52f, 0.38f, 0.28f, 1.0f},
				.color_tertiary = {0.96f, 0.94f, 0.90f, 1.0f},
				.parent_name = "Jupiter"
			});

			solar.bodies.push_back(SystemBodyPresetSpec{
				.name = "Ganymede",
				.mass_kg = 1.4819e23,
				.radius_m = 2.6341e6,
				.semi_major_axis_m = 1.0704e9,
				.eccentricity = 0.0013,
				.inclination_deg = 0.200,
				.longitude_node_deg = 63.0,
				.argument_periapsis_deg = 192.0,
				.true_anomaly_deg = 270.0,
				.rotation_period_hours = 171.7,
				.axial_tilt_deg = 0.2,
				.temperature_k = 110.0,
				.preset = Dynamics::Body3DPreset::Metallic,
				.texture_mode = Dynamics::Body3DSurfaceTextureMode::CrateredTerrestrial,
				.color = {0.65f, 0.62f, 0.58f, 1.0f},
				.color_secondary = {0.38f, 0.35f, 0.32f, 1.0f},
				.color_tertiary = {0.85f, 0.82f, 0.78f, 1.0f},
				.parent_name = "Jupiter"
			});

			solar.bodies.push_back(SystemBodyPresetSpec{
				.name = "Callisto",
				.mass_kg = 1.0759e23,
				.radius_m = 2.4103e6,
				.semi_major_axis_m = 1.8827e9,
				.eccentricity = 0.0074,
				.inclination_deg = 0.280,
				.longitude_node_deg = 298.0,
				.argument_periapsis_deg = 52.0,
				.true_anomaly_deg = 330.0,
				.rotation_period_hours = 400.5,
				.axial_tilt_deg = 0.0,
				.temperature_k = 134.0,
				.preset = Dynamics::Body3DPreset::Metallic,
				.texture_mode = Dynamics::Body3DSurfaceTextureMode::CrateredTerrestrial,
				.color = {0.48f, 0.45f, 0.42f, 1.0f},
				.color_secondary = {0.28f, 0.26f, 0.24f, 1.0f},
				.color_tertiary = {0.72f, 0.70f, 0.68f, 1.0f},
				.parent_name = "Jupiter"
			});

			solar.bodies.push_back(SystemBodyPresetSpec{
				.name = "Saturn",
				.mass_kg = 5.6834e26,
				.radius_m = 5.8232e7,
				.semi_major_axis_m = 1.43353e12,
				.eccentricity = 0.05655,
				.inclination_deg = 2.485,
				.longitude_node_deg = 113.665,
				.argument_periapsis_deg = 339.392,
				.true_anomaly_deg = 280.0,
				.rotation_period_hours = 10.656,
				.axial_tilt_deg = 26.73,
				.temperature_k = 134.0,
				.preset = Dynamics::Body3DPreset::GasGiant,
				.texture_mode = Dynamics::Body3DSurfaceTextureMode::RingedGasGiant,
				.color = {0.90f, 0.82f, 0.62f, 1.0f},
				.color_secondary = {0.75f, 0.62f, 0.42f, 1.0f},
				.color_tertiary = {0.96f, 0.90f, 0.76f, 1.0f},
				.atmosphere_thickness = 0.30f,
				.atmosphere_mode = Dynamics::Body3DAtmosphereMode::ThickHaze,
				.ring_system = true,
				.parent_name = "Sun",
				.layer_template_1 = 5
			});

			solar.bodies.push_back(SystemBodyPresetSpec{
				.name = "Titan",
				.mass_kg = 1.3452e23,
				.radius_m = 2.5747e6,
				.semi_major_axis_m = 1.22187e9,
				.eccentricity = 0.0288,
				.inclination_deg = 0.348,
				.longitude_node_deg = 168.0,
				.argument_periapsis_deg = 175.0,
				.true_anomaly_deg = 60.0,
				.rotation_period_hours = 382.7,
				.axial_tilt_deg = 0.0,
				.temperature_k = 93.7,
				.preset = Dynamics::Body3DPreset::TerrestrialPlanet,
				.texture_mode = Dynamics::Body3DSurfaceTextureMode::ProceduralNoise,
				.color = {0.86f, 0.62f, 0.28f, 1.0f},
				.color_secondary = {0.68f, 0.45f, 0.18f, 1.0f},
				.color_tertiary = {0.94f, 0.78f, 0.42f, 1.0f},
				.atmosphere_thickness = 0.26f,
				.atmosphere_mode = Dynamics::Body3DAtmosphereMode::ThickHaze,
				.parent_name = "Saturn"
			});

			solar.bodies.push_back(SystemBodyPresetSpec{
				.name = "Uranus",
				.mass_kg = 8.6810e25,
				.radius_m = 2.5362e7,
				.semi_major_axis_m = 2.87246e12,
				.eccentricity = 0.04638,
				.inclination_deg = 0.773,
				.longitude_node_deg = 74.006,
				.argument_periapsis_deg = 96.998,
				.true_anomaly_deg = 320.0,
				.rotation_period_hours = -17.24,
				.axial_tilt_deg = 97.77,
				.temperature_k = 76.0,
				.preset = Dynamics::Body3DPreset::IceGiant,
				.texture_mode = Dynamics::Body3DSurfaceTextureMode::IcyCracked,
				.color = {0.65f, 0.85f, 0.88f, 1.0f},
				.color_secondary = {0.45f, 0.68f, 0.74f, 1.0f},
				.color_tertiary = {0.85f, 0.95f, 0.98f, 1.0f},
				.atmosphere_thickness = 0.22f,
				.atmosphere_mode = Dynamics::Body3DAtmosphereMode::VolumetricMie,
				.parent_name = "Sun"
			});

			solar.bodies.push_back(SystemBodyPresetSpec{
				.name = "Neptune",
				.mass_kg = 1.02413e26,
				.radius_m = 2.4622e7,
				.semi_major_axis_m = 4.49506e12,
				.eccentricity = 0.00946,
				.inclination_deg = 1.768,
				.longitude_node_deg = 131.784,
				.argument_periapsis_deg = 273.187,
				.true_anomaly_deg = 40.0,
				.rotation_period_hours = 16.11,
				.axial_tilt_deg = 28.32,
				.temperature_k = 72.0,
				.preset = Dynamics::Body3DPreset::IceGiant,
				.texture_mode = Dynamics::Body3DSurfaceTextureMode::ProceduralNoise,
				.color = {0.25f, 0.45f, 0.88f, 1.0f},
				.color_secondary = {0.15f, 0.28f, 0.68f, 1.0f},
				.color_tertiary = {0.55f, 0.75f, 0.98f, 1.0f},
				.atmosphere_thickness = 0.25f,
				.atmosphere_mode = Dynamics::Body3DAtmosphereMode::VolumetricMie,
				.parent_name = "Sun"
			});

			solar.bodies.push_back(SystemBodyPresetSpec{
				.name = "Pluto",
				.mass_kg = 1.303e22,
				.radius_m = 1.1883e6,
				.semi_major_axis_m = 5.90638e12,
				.eccentricity = 0.24881,
				.inclination_deg = 17.160,
				.longitude_node_deg = 110.299,
				.argument_periapsis_deg = 113.834,
				.true_anomaly_deg = 15.0,
				.rotation_period_hours = -153.29,
				.axial_tilt_deg = 122.53,
				.temperature_k = 44.0,
				.preset = Dynamics::Body3DPreset::TerrestrialPlanet,
				.texture_mode = Dynamics::Body3DSurfaceTextureMode::IcyCracked,
				.color = {0.78f, 0.68f, 0.58f, 1.0f},
				.color_secondary = {0.45f, 0.35f, 0.28f, 1.0f},
				.color_tertiary = {0.92f, 0.88f, 0.82f, 1.0f},
				.parent_name = "Sun"
			});

			list.push_back(std::move(solar));

			SystemPresetDefinition inner;
			inner.id = "inner_solar_system";
			inner.name = "Inner Solar System (Terrestrial Focus)";
			inner.description = "Sun, Mercury, Venus, Earth-Moon, and Mars with physical scales optimized for inner-system dynamics and surface exploration.";
			inner.metric_name = "Flat Minkowski";
			inner.central_mass_kg = 1.98847e30;
			inner.central_radius_m = 6.9634e8;
			for (const auto& b : list.front().bodies) {
				if (b.name == "Sun" || b.name == "Mercury" || b.name == "Venus" || b.name == "Earth" || b.name == "Moon" || b.name == "Mars") {
					inner.bodies.push_back(b);
				}
			}
			list.push_back(std::move(inner));

			SystemPresetDefinition jovian;
			jovian.id = "jovian_system";
			jovian.name = "Jovian Sub-System (Jupiter & Galilean Moons)";
			jovian.description = "Jupiter positioned as the primary central attractor with Io, Europa, Ganymede, and Callisto in Laplace resonance orbits.";
			jovian.metric_name = "Schwarzschild";
			jovian.central_mass_kg = 1.89813e27;
			jovian.central_radius_m = 6.9911e7;
			jovian.bodies.push_back(SystemBodyPresetSpec{
				.name = "Jupiter",
				.mass_kg = 1.89813e27,
				.radius_m = 6.9911e7,
				.semi_major_axis_m = 0.0,
				.rotation_period_hours = 9.925,
				.axial_tilt_deg = 3.13,
				.temperature_k = 165.0,
				.preset = Dynamics::Body3DPreset::GasGiant,
				.texture_mode = Dynamics::Body3DSurfaceTextureMode::RingedGasGiant,
				.color = {0.84f, 0.68f, 0.48f, 1.0f},
				.color_secondary = {0.64f, 0.42f, 0.26f, 1.0f},
				.color_tertiary = {0.92f, 0.86f, 0.76f, 1.0f},
				.atmosphere_thickness = 0.34f,
				.atmosphere_mode = Dynamics::Body3DAtmosphereMode::ThickHaze,
				.ring_system = true
			});
			for (const auto& b : list.front().bodies) {
				if (b.parent_name == "Jupiter") {
					auto copy = b;
					copy.parent_name = "Jupiter";
					jovian.bodies.push_back(copy);
				}
			}
			list.push_back(std::move(jovian));

			SystemPresetDefinition earth_moon;
			earth_moon.id = "earth_moon_system";
			earth_moon.name = "Earth-Moon High-Precision Orbital Frame";
			earth_moon.description = "Earth at origin with Blue Marble photographic textures and terminator blending, and Moon in high-precision orbit.";
			earth_moon.metric_name = "Flat Minkowski";
			earth_moon.central_mass_kg = 5.9722e24;
			earth_moon.central_radius_m = 6.3710e6;
			earth_moon.bodies.push_back(SystemBodyPresetSpec{
				.name = "Earth",
				.mass_kg = 5.9722e24,
				.radius_m = 6.3710e6,
				.semi_major_axis_m = 0.0,
				.rotation_period_hours = 23.9344,
				.axial_tilt_deg = 23.44,
				.temperature_k = 288.15,
				.preset = Dynamics::Body3DPreset::TerrestrialPlanet,
				.texture_mode = Dynamics::Body3DSurfaceTextureMode::EarthBlueMarble,
				.earth_variant = Optics::EarthMapVariant::Automatic,
				.earth_quality = Optics::EarthMapQuality::Q2K,
				.color = {0.12f, 0.38f, 0.72f, 1.0f},
				.color_secondary = {0.22f, 0.52f, 0.25f, 1.0f},
				.color_tertiary = {0.95f, 0.95f, 0.98f, 1.0f},
				.atmosphere_thickness = 0.16f,
				.atmosphere_mode = Dynamics::Body3DAtmosphereMode::RayleighLimbShell,
				.layer_template_1 = 2
			});
			earth_moon.bodies.push_back(SystemBodyPresetSpec{
				.name = "Moon",
				.mass_kg = 7.342e22,
				.radius_m = 1.7374e6,
				.semi_major_axis_m = 3.844e8,
				.eccentricity = 0.0549,
				.inclination_deg = 5.145,
				.rotation_period_hours = 655.72,
				.axial_tilt_deg = 6.68,
				.temperature_k = 250.0,
				.preset = Dynamics::Body3DPreset::Metallic,
				.texture_mode = Dynamics::Body3DSurfaceTextureMode::CrateredTerrestrial,
				.color = {0.68f, 0.67f, 0.66f, 1.0f},
				.color_secondary = {0.42f, 0.41f, 0.40f, 1.0f},
				.color_tertiary = {0.85f, 0.84f, 0.83f, 1.0f},
				.parent_name = "Earth",
				.layer_template_1 = 3
			});
			list.push_back(std::move(earth_moon));

			return list;
		}();
		return kPresets;
	}

	[[nodiscard]] static const SystemPresetDefinition* find(std::string_view id) noexcept {
		for (const auto& preset : presets()) {
			if (preset.id == id) return &preset;
		}
		return nullptr;
	}
};

inline void apply_system_preset(BodyManagerContext& ctx, const SystemPresetDefinition& preset) noexcept {
	auto& orchestrator = ctx.orchestrator();
	auto& constants = orchestrator.constants_engine();

	constants.set_reference_mass(preset.central_mass_kg);

	const double mass_scale = ctx.mass_scale();
	const double length_scale = ctx.length_scale();
	const double speed_scale = ctx.speed_scale();
	const double g_sim = ctx.simulation_g();

	auto& params = orchestrator.parameters();
	params.mass = preset.central_mass_kg / mass_scale;
	params.render_distance_scale = 0.0;
	orchestrator.set_active_metric_name(preset.metric_name);

	ctx.clear_all_bodies();

	auto& sys = orchestrator.nbody_system();
	std::lock_guard<std::recursive_mutex> lock(sys.bodies_mutex());

	struct SpawnedRef {
		std::string name;
		uint32_t id;
		std::array<double, 3> position;
		std::array<double, 3> velocity;
		double mass;
	};
	std::vector<SpawnedRef> spawned_refs;

	for (const auto& spec : preset.bodies) {
		const double m_body = spec.mass_kg / mass_scale;
		const double r_body = std::max(spec.radius_m / length_scale, 1e-6);

		std::array<double, 3> parent_pos{0.0, 0.0, 0.0};
		std::array<double, 3> parent_vel{0.0, 0.0, 0.0};
		double parent_m = params.mass;

		if (!spec.parent_name.empty()) {
			for (const auto& ref : spawned_refs) {
				if (ref.name == spec.parent_name) {
					parent_pos = ref.position;
					parent_vel = ref.velocity;
					parent_m = ref.mass;
					break;
				}
			}
		}

		std::array<double, 3> pos_world = parent_pos;
		std::array<double, 3> vel_world = parent_vel;

		if (spec.semi_major_axis_m > 0.0) {
			const double a_sim = spec.semi_major_axis_m / length_scale;
			const double e = std::clamp(spec.eccentricity, 0.0, 0.999);
			const double inc_rad = spec.inclination_deg * (std::numbers::pi / 180.0);
			const double node_rad = spec.longitude_node_deg * (std::numbers::pi / 180.0);
			const double peri_rad = spec.argument_periapsis_deg * (std::numbers::pi / 180.0);
			const double nu_rad = spec.true_anomaly_deg * (std::numbers::pi / 180.0);

			const double r_orb = a_sim * (1.0 - e * e) / (1.0 + e * std::cos(nu_rad));
			const double mu = g_sim * (parent_m + m_body);
			const double h_orb = std::sqrt(std::max(mu * a_sim * (1.0 - e * e), 1e-18));

			const double cos_o = std::cos(node_rad);
			const double sin_o = std::sin(node_rad);
			const double cos_i = std::cos(inc_rad);
			const double sin_i = std::sin(inc_rad);
			const double cos_w = std::cos(peri_rad);
			const double sin_w = std::sin(peri_rad);

			const std::array<double, 3> P_hat{
				cos_o * cos_w - sin_o * sin_w * cos_i,
				sin_o * cos_w + cos_o * sin_w * cos_i,
				sin_w * sin_i
			};
			const std::array<double, 3> Q_hat{
				-cos_o * sin_w - sin_o * cos_w * cos_i,
				-sin_o * sin_w + cos_o * cos_w * cos_i,
				cos_w * sin_i
			};

			const double cos_nu = std::cos(nu_rad);
			const double sin_nu = std::sin(nu_rad);

			const std::array<double, 3> r_rel{
				r_orb * (cos_nu * P_hat[0] + sin_nu * Q_hat[0]),
				r_orb * (cos_nu * P_hat[1] + sin_nu * Q_hat[1]),
				r_orb * (cos_nu * P_hat[2] + sin_nu * Q_hat[2])
			};

			const std::array<double, 3> v_rel{
				(mu / h_orb) * (-sin_nu * P_hat[0] + (e + cos_nu) * Q_hat[0]),
				(mu / h_orb) * (-sin_nu * P_hat[1] + (e + cos_nu) * Q_hat[1]),
				(mu / h_orb) * (-sin_nu * P_hat[2] + (e + cos_nu) * Q_hat[2])
			};

			pos_world = {parent_pos[0] + r_rel[0], parent_pos[1] + r_rel[1], parent_pos[2] + r_rel[2]};
			vel_world = {parent_vel[0] + v_rel[0], parent_vel[1] + v_rel[1], parent_vel[2] + v_rel[2]};
		}

		Dynamics::PostNewtonianBody b(0, m_body, r_body, pos_world, vel_world, {0.0, 0.0, 0.0});
		b.reference_radius = r_body;
		b.set_name(spec.name);
		apply_body_appearance_preset(b, spec.preset);
		apply_body_material_preset(b, spec.preset);

		b.surface_texture_mode = spec.texture_mode;
		b.earth_map_variant = spec.earth_variant;
		b.earth_map_quality = spec.earth_quality;
		b.color = spec.color;
		b.color_secondary = spec.color_secondary;
		b.color_tertiary = spec.color_tertiary;
		b.emission_intensity = spec.emission_intensity;
		b.atmosphere_thickness = spec.atmosphere_thickness;
		b.atmosphere_mode = spec.atmosphere_mode;
		b.ring_system_enabled = spec.ring_system;
		b.temperature = spec.temperature_k;

		if (std::abs(spec.rotation_period_hours) > 0.01) {
			const double omega_rad_s = (2.0 * std::numbers::pi) / (std::abs(spec.rotation_period_hours) * 3600.0);
			const double omega_sim = omega_rad_s * (length_scale / speed_scale);
			b.rotation_speed_3d = static_cast<float>(spec.rotation_period_hours >= 0.0 ? omega_sim : -omega_sim);
		}
		b.set_orientation_euler_degrees(0.0, spec.axial_tilt_deg, 0.0);

		Dynamics::BodySurfaceLayerSet layers{};
		if (spec.layer_template_1 >= 0) {
			static_cast<void>(layers.add(Dynamics::SurfaceLayerDefinition::from_template(static_cast<Dynamics::SurfaceLayerTemplate>(spec.layer_template_1))));
		}
		if (spec.layer_template_2 >= 0) {
			static_cast<void>(layers.add(Dynamics::SurfaceLayerDefinition::from_template(static_cast<Dynamics::SurfaceLayerTemplate>(spec.layer_template_2))));
		}

		const uint32_t new_id = ctx.spawn_body(std::move(b), layers);
		spawned_refs.push_back(SpawnedRef{spec.name, new_id, pos_world, vel_world, m_body});
	}

	sys.update_accelerations();
	orchestrator.notify_state_changed();
}

}

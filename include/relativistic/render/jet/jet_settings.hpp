#pragma once

#include "relativistic/core/constants.hpp"
#include "relativistic/magnetosphere/blandford_znajek.hpp"
#include "relativistic/optics/radiative_processes.hpp"
#include "relativistic/render/gpu_types.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <numbers>

namespace Relativistic::Render {

enum class JetPreset : uint32_t {
	ParaboloidalCollimatedJet = 0,
	SplitMonopoleFunnel = 1,
	MonopoleReference = 2,
	MagneticallyArrestedFlux = 3,
	HotThermalFunnel = 4,
	FaintDiffuseOutflow = 5
};

inline constexpr size_t kJetPresetCount = 6;

inline constexpr std::array<const char*, kJetPresetCount> kJetPresetNames{
	"Paraboloidal Collimated Jet",
	"Split Monopole Funnel (Blandford-Znajek)",
	"Monopole Reference Field",
	"Magnetically Arrested Vertical Flux",
	"Hot Thermal Funnel",
	"Faint Diffuse Outflow"
};

inline constexpr std::array<const char*, 4> kJetGeometryNames{
	"Monopole (A = C r (1 - cos theta))",
	"Split Monopole (Mirrored Hemispheres)",
	"Paraboloidal (A = C r^nu (1 - |cos theta|))",
	"Vertical Flux (A = C r^2 sin^2 theta / 2)"
};

inline constexpr std::array<const char*, 3> kJetEmissionModelNames{
	"Non-Thermal Power Law Electrons",
	"Thermal Relativistic Maxwellian",
	"Hybrid Thermal + Non-Thermal"
};

inline constexpr std::array<const char*, 3> kJetDisplayModeNames{
	"Total Intensity",
	"Linearly Polarized Intensity",
	"Polarization Fraction Map"
};

inline constexpr std::array<const char*, 2> kJetAngularVelocityLawNames{
	"Fraction Of Horizon Angular Velocity",
	"Fixed Angular Velocity (1 / M)"
};

struct JetSettings {
	bool enabled{false};
	JetFieldGeometry geometry{JetFieldGeometry::Paraboloidal};
	float field_line_index{1.5f};
	float horizon_field_tesla{1.0e4f};
	float magnetization{50.0f};
	float magnetization_index{0.5f};
	JetAngularVelocityLaw angular_velocity_law{JetAngularVelocityLaw::HorizonFraction};
	float angular_velocity_fraction{0.5f};
	float fixed_angular_velocity{0.1f};
	float angular_velocity_variation{0.0f};
	float toroidal_field_scale{1.0f};
	float rotation_coupling{1.0f};
	float inner_radius_scale{1.1f};
	float outer_radius_mass_units{150.0f};
	float footpoint_edge_deg{75.0f};
	float footpoint_softness_deg{15.0f};
	float lorentz_factor_inner{1.3f};
	float lorentz_factor_max{6.0f};
	float acceleration_radius_mass_units{40.0f};
	float acceleration_index{1.0f};
	bool doppler_beaming_enabled{true};
	bool gravitational_redshift_enabled{true};
	JetEmissionModel emission_model{JetEmissionModel::NonThermal};
	float power_law_index{2.5f};
	float minimum_lorentz_factor{10.0f};
	float non_thermal_fraction{0.02f};
	float electron_temperature_k{3.0e10f};
	float temperature_index{0.5f};
	float observing_frequency_hz{2.3e11f};
	float absorption_scale{1.0f};
	float faraday_scale{1.0f};
	bool auto_normalization{true};
	float reference_brightness_temperature_k{1.0e10f};
	float brightness{1.0f};
	std::array<float, 3> tint{0.35f, 0.6f, 1.0f};
	JetDisplayMode display_mode{JetDisplayMode::TotalIntensity};
	float polarization_coherence{0.6f};
	float sampling_density{1.0f};
	int32_t max_samples{24};
	bool use_engine_length_scale{true};
	float meters_per_length_unit{1.477e4f};

	void sanitize() noexcept {
		geometry = static_cast<JetFieldGeometry>(std::min<uint32_t>(static_cast<uint32_t>(geometry), 3U));
		angular_velocity_law = static_cast<JetAngularVelocityLaw>(std::min<uint32_t>(static_cast<uint32_t>(angular_velocity_law), 1U));
		emission_model = static_cast<JetEmissionModel>(std::min<uint32_t>(static_cast<uint32_t>(emission_model), 2U));
		display_mode = static_cast<JetDisplayMode>(std::min<uint32_t>(static_cast<uint32_t>(display_mode), 2U));
		field_line_index = std::clamp(field_line_index, 0.25f, 4.0f);
		horizon_field_tesla = std::clamp(horizon_field_tesla, 1.0e-3f, 1.0e12f);
		magnetization = std::clamp(magnetization, 1.0e-3f, 1.0e6f);
		magnetization_index = std::clamp(magnetization_index, -2.0f, 3.0f);
		angular_velocity_fraction = std::clamp(angular_velocity_fraction, 0.0f, 2.0f);
		fixed_angular_velocity = std::clamp(fixed_angular_velocity, -1.0f, 1.0f);
		angular_velocity_variation = std::clamp(angular_velocity_variation, -1.0f, 1.0f);
		toroidal_field_scale = std::clamp(toroidal_field_scale, 0.0f, 4.0f);
		rotation_coupling = std::clamp(rotation_coupling, 0.0f, 1.0f);
		inner_radius_scale = std::clamp(inner_radius_scale, 1.001f, 10.0f);
		outer_radius_mass_units = std::clamp(outer_radius_mass_units, 5.0f, 2000.0f);
		footpoint_edge_deg = std::clamp(footpoint_edge_deg, 1.0f, 90.0f);
		footpoint_softness_deg = std::clamp(footpoint_softness_deg, 0.1f, 45.0f);
		lorentz_factor_inner = std::clamp(lorentz_factor_inner, 1.0f, 50.0f);
		lorentz_factor_max = std::clamp(lorentz_factor_max, lorentz_factor_inner, 100.0f);
		acceleration_radius_mass_units = std::clamp(acceleration_radius_mass_units, 0.1f, 2000.0f);
		acceleration_index = std::clamp(acceleration_index, 0.1f, 4.0f);
		power_law_index = std::clamp(power_law_index, 1.5f, 5.0f);
		minimum_lorentz_factor = std::clamp(minimum_lorentz_factor, 1.0f, 1.0e4f);
		non_thermal_fraction = std::clamp(non_thermal_fraction, 0.0f, 1.0f);
		electron_temperature_k = std::clamp(electron_temperature_k, 1.0e5f, 1.0e13f);
		temperature_index = std::clamp(temperature_index, -1.0f, 3.0f);
		observing_frequency_hz = std::clamp(observing_frequency_hz, 1.0e6f, 1.0e20f);
		absorption_scale = std::clamp(absorption_scale, 0.0f, 10.0f);
		faraday_scale = std::clamp(faraday_scale, 0.0f, 10.0f);
		reference_brightness_temperature_k = std::clamp(reference_brightness_temperature_k, 1.0e3f, 1.0e14f);
		brightness = std::clamp(brightness, 0.0f, 1000.0f);
		for (float& channel : tint) {
			channel = std::clamp(channel, 0.0f, 4.0f);
		}
		polarization_coherence = std::clamp(polarization_coherence, 0.0f, 1.0f);
		sampling_density = std::clamp(sampling_density, 0.1f, 8.0f);
		max_samples = std::clamp<int32_t>(max_samples, 1, 128);
		meters_per_length_unit = std::clamp(meters_per_length_unit, 1.0e-3f, 1.0e30f);
	}

	void apply_preset(JetPreset preset) noexcept {
		const bool kept_enabled = enabled;
		*this = JetSettings{};
		enabled = kept_enabled;
		switch (preset) {
			case JetPreset::SplitMonopoleFunnel:
				geometry = JetFieldGeometry::SplitMonopole;
				footpoint_edge_deg = 80.0f;
				break;
			case JetPreset::MonopoleReference:
				geometry = JetFieldGeometry::Monopole;
				footpoint_edge_deg = 85.0f;
				lorentz_factor_max = 3.0f;
				break;
			case JetPreset::MagneticallyArrestedFlux:
				geometry = JetFieldGeometry::Vertical;
				horizon_field_tesla = 3.0e4f;
				magnetization = 5.0f;
				footpoint_edge_deg = 85.0f;
				footpoint_softness_deg = 10.0f;
				angular_velocity_variation = 0.3f;
				break;
			case JetPreset::HotThermalFunnel:
				emission_model = JetEmissionModel::Hybrid;
				non_thermal_fraction = 0.1f;
				electron_temperature_k = 1.0e11f;
				temperature_index = 0.7f;
				break;
			case JetPreset::FaintDiffuseOutflow:
				magnetization = 500.0f;
				brightness = 0.4f;
				lorentz_factor_max = 2.5f;
				polarization_coherence = 0.3f;
				break;
			case JetPreset::ParaboloidalCollimatedJet:
			default:
				break;
		}
		sanitize();
	}

	[[nodiscard]] GpuJetProfile to_gpu_profile(double mass, double spin, double engine_length_scale_meters) const noexcept {
		JetSettings s = *this;
		s.sanitize();
		GpuJetProfile profile;
		profile.enabled = s.enabled ? 1.0f : 0.0f;
		if (!s.enabled) {
			return profile;
		}

		using Constants = Core::PhysicalConstants<double>;
		const double m = std::max(mass, 1e-9);
		const double a = std::clamp(spin, -0.999 * m, 0.999 * m);
		const double r_h = m + std::sqrt(std::max(m * m - a * a, 0.0));
		const double meters = s.use_engine_length_scale ? std::max(engine_length_scale_meters, 1e-30) : static_cast<double>(s.meters_per_length_unit);
		constexpr double degrees_to_radians = std::numbers::pi / 180.0;

		profile.geometry = static_cast<float>(static_cast<uint32_t>(s.geometry));
		profile.field_line_index = s.field_line_index;
		profile.horizon_field_tesla = s.horizon_field_tesla;
		profile.magnetization = s.magnetization;
		profile.magnetization_index = s.magnetization_index;
		profile.angular_velocity_law = static_cast<float>(static_cast<uint32_t>(s.angular_velocity_law));
		profile.angular_velocity_fraction = s.angular_velocity_fraction;
		profile.fixed_angular_velocity = s.fixed_angular_velocity;
		profile.angular_velocity_variation = s.angular_velocity_variation;
		profile.toroidal_field_scale = s.toroidal_field_scale;
		profile.rotation_coupling = s.rotation_coupling;
		profile.mass = static_cast<float>(m);
		profile.spin = static_cast<float>(a);
		profile.horizon_radius = static_cast<float>(r_h);
		profile.horizon_angular_velocity = static_cast<float>(a / (r_h * r_h + a * a));
		profile.inner_radius = static_cast<float>(r_h * static_cast<double>(s.inner_radius_scale));
		profile.outer_radius = static_cast<float>(std::max(static_cast<double>(s.outer_radius_mass_units) * m, r_h * static_cast<double>(s.inner_radius_scale) * 1.5));
		profile.footpoint_edge = static_cast<float>(static_cast<double>(s.footpoint_edge_deg) * degrees_to_radians);
		profile.footpoint_softness = static_cast<float>(static_cast<double>(s.footpoint_softness_deg) * degrees_to_radians);
		profile.lorentz_inner = s.lorentz_factor_inner;
		profile.lorentz_max = s.lorentz_factor_max;
		profile.acceleration_radius = static_cast<float>(static_cast<double>(s.acceleration_radius_mass_units) * m);
		profile.acceleration_index = s.acceleration_index;
		profile.doppler_enabled = s.doppler_beaming_enabled ? 1.0f : 0.0f;
		profile.redshift_enabled = s.gravitational_redshift_enabled ? 1.0f : 0.0f;
		profile.emission_model = static_cast<float>(static_cast<uint32_t>(s.emission_model));
		profile.power_law_index = s.power_law_index;
		profile.gamma_min = s.minimum_lorentz_factor;
		profile.non_thermal_fraction = s.non_thermal_fraction;
		profile.electron_temperature_k = s.electron_temperature_k;
		profile.temperature_index = s.temperature_index;
		profile.observing_frequency_hz = s.observing_frequency_hz;
		profile.meters_per_unit = static_cast<float>(meters);
		profile.polarization_fraction = static_cast<float>((static_cast<double>(s.power_law_index) + 1.0) / (static_cast<double>(s.power_law_index) + 7.0 / 3.0));
		profile.polarization_coherence = s.polarization_coherence;
		profile.absorption_scale = s.absorption_scale;
		profile.faraday_scale = s.faraday_scale;
		profile.brightness = s.brightness;
		profile.tint_r = s.tint[0];
		profile.tint_g = s.tint[1];
		profile.tint_b = s.tint[2];
		profile.display_mode = static_cast<float>(static_cast<uint32_t>(s.display_mode));
		profile.sampling_density = s.sampling_density;
		profile.max_samples = static_cast<float>(s.max_samples);

		Optics::PolarizedPlasmaState<double> reference;
		reference.electron_density = 1.0;
		reference.ion_density = 1.0;
		reference.non_thermal_fraction = 1.0;
		reference.magnetic_field_tesla = 1.0;
		reference.pitch_angle_rad = std::numbers::pi * 0.5;
		reference.power_law_index = static_cast<double>(s.power_law_index);
		reference.gamma_min = static_cast<double>(s.minimum_lorentz_factor);
		const auto emissivity = Optics::RadiativeProcessEngine<double>::non_thermal_synchrotron_emissivity(1.0, reference);
		const auto absorptivity = Optics::RadiativeProcessEngine<double>::non_thermal_synchrotron_absorptivity(1.0, reference);
		profile.log_emissivity_constant = static_cast<float>(std::log(std::max(emissivity.j_i, 1e-300)));
		profile.log_absorptivity_constant = static_cast<float>(std::log(std::max(absorptivity.alpha_i, 1e-300)));
		profile.faraday_constant = static_cast<float>((Constants::ELEMENTARY_CHARGE * Constants::ELEMENTARY_CHARGE * Constants::ELEMENTARY_CHARGE) / (2.0 * std::numbers::pi * Constants::ELECTRON_MASS * Constants::ELECTRON_MASS * Constants::VACUUM_PERMITTIVITY * Constants::SPEED_OF_LIGHT));
		profile.reserved = static_cast<float>(Constants::VACUUM_PERMITTIVITY / Constants::PROTON_MASS);

		const double nu = static_cast<double>(s.observing_frequency_hz);
		const double physical_reference = 2.0 * Constants::BOLTZMANN_CONSTANT * static_cast<double>(s.reference_brightness_temperature_k) * nu * nu / (Constants::SPEED_OF_LIGHT * Constants::SPEED_OF_LIGHT);
		profile.log_reference_intensity = static_cast<float>(std::log(std::max(physical_reference, 1e-300)));
		if (s.auto_normalization) {
			const double canonical = Magnetosphere::JetEmissionIntegrator::canonical_intensity(profile);
			if (std::isfinite(canonical) && canonical > 1e-300) {
				profile.log_reference_intensity = static_cast<float>(std::log(canonical));
			}
		}
		return profile;
	}

	template <typename Entries>
	void store(Entries& out) const {
		const auto put = [&out](const char* key, double value) {
			char buffer[40];
			std::snprintf(buffer, sizeof(buffer), "%.9g", value);
			out[key] = buffer;
		};
		put("jet_enabled", enabled ? 1.0 : 0.0);
		put("jet_geometry", static_cast<double>(static_cast<uint32_t>(geometry)));
		put("jet_field_line_index", field_line_index);
		put("jet_horizon_field_tesla", horizon_field_tesla);
		put("jet_magnetization", magnetization);
		put("jet_magnetization_index", magnetization_index);
		put("jet_angular_velocity_law", static_cast<double>(static_cast<uint32_t>(angular_velocity_law)));
		put("jet_angular_velocity_fraction", angular_velocity_fraction);
		put("jet_fixed_angular_velocity", fixed_angular_velocity);
		put("jet_angular_velocity_variation", angular_velocity_variation);
		put("jet_toroidal_field_scale", toroidal_field_scale);
		put("jet_rotation_coupling", rotation_coupling);
		put("jet_inner_radius_scale", inner_radius_scale);
		put("jet_outer_radius", outer_radius_mass_units);
		put("jet_footpoint_edge_deg", footpoint_edge_deg);
		put("jet_footpoint_softness_deg", footpoint_softness_deg);
		put("jet_lorentz_inner", lorentz_factor_inner);
		put("jet_lorentz_max", lorentz_factor_max);
		put("jet_acceleration_radius", acceleration_radius_mass_units);
		put("jet_acceleration_index", acceleration_index);
		put("jet_doppler", doppler_beaming_enabled ? 1.0 : 0.0);
		put("jet_redshift", gravitational_redshift_enabled ? 1.0 : 0.0);
		put("jet_emission_model", static_cast<double>(static_cast<uint32_t>(emission_model)));
		put("jet_power_law_index", power_law_index);
		put("jet_gamma_min", minimum_lorentz_factor);
		put("jet_non_thermal_fraction", non_thermal_fraction);
		put("jet_electron_temperature", electron_temperature_k);
		put("jet_temperature_index", temperature_index);
		put("jet_frequency", observing_frequency_hz);
		put("jet_absorption_scale", absorption_scale);
		put("jet_faraday_scale", faraday_scale);
		put("jet_auto_normalization", auto_normalization ? 1.0 : 0.0);
		put("jet_reference_temperature", reference_brightness_temperature_k);
		put("jet_brightness", brightness);
		put("jet_tint_r", tint[0]);
		put("jet_tint_g", tint[1]);
		put("jet_tint_b", tint[2]);
		put("jet_display_mode", static_cast<double>(static_cast<uint32_t>(display_mode)));
		put("jet_polarization_coherence", polarization_coherence);
		put("jet_sampling_density", sampling_density);
		put("jet_max_samples", static_cast<double>(max_samples));
		put("jet_engine_length_scale", use_engine_length_scale ? 1.0 : 0.0);
		put("jet_meters_per_unit", meters_per_length_unit);
	}

	template <typename Entries>
	void restore(const Entries& in) {
		const auto read = [&in](const char* key, double& target) {
			const auto it = in.find(key);
			if (it == in.end()) {
				return;
			}
			const double value = std::strtod(it->second.c_str(), nullptr);
			if (std::isfinite(value)) {
				target = value;
			}
		};
		const auto real = [&read](const char* key, float& target) {
			double value = static_cast<double>(target);
			read(key, value);
			target = static_cast<float>(value);
		};
		const auto flag = [&read](const char* key, bool& target) {
			double value = target ? 1.0 : 0.0;
			read(key, value);
			target = value > 0.5;
		};
		const auto index = [&read](const char* key, uint32_t current) {
			double value = static_cast<double>(current);
			read(key, value);
			return static_cast<uint32_t>(std::max(value, 0.0));
		};
		flag("jet_enabled", enabled);
		geometry = static_cast<JetFieldGeometry>(index("jet_geometry", static_cast<uint32_t>(geometry)));
		real("jet_field_line_index", field_line_index);
		real("jet_horizon_field_tesla", horizon_field_tesla);
		real("jet_magnetization", magnetization);
		real("jet_magnetization_index", magnetization_index);
		angular_velocity_law = static_cast<JetAngularVelocityLaw>(index("jet_angular_velocity_law", static_cast<uint32_t>(angular_velocity_law)));
		real("jet_angular_velocity_fraction", angular_velocity_fraction);
		real("jet_fixed_angular_velocity", fixed_angular_velocity);
		real("jet_angular_velocity_variation", angular_velocity_variation);
		real("jet_toroidal_field_scale", toroidal_field_scale);
		real("jet_rotation_coupling", rotation_coupling);
		real("jet_inner_radius_scale", inner_radius_scale);
		real("jet_outer_radius", outer_radius_mass_units);
		real("jet_footpoint_edge_deg", footpoint_edge_deg);
		real("jet_footpoint_softness_deg", footpoint_softness_deg);
		real("jet_lorentz_inner", lorentz_factor_inner);
		real("jet_lorentz_max", lorentz_factor_max);
		real("jet_acceleration_radius", acceleration_radius_mass_units);
		real("jet_acceleration_index", acceleration_index);
		flag("jet_doppler", doppler_beaming_enabled);
		flag("jet_redshift", gravitational_redshift_enabled);
		emission_model = static_cast<JetEmissionModel>(index("jet_emission_model", static_cast<uint32_t>(emission_model)));
		real("jet_power_law_index", power_law_index);
		real("jet_gamma_min", minimum_lorentz_factor);
		real("jet_non_thermal_fraction", non_thermal_fraction);
		real("jet_electron_temperature", electron_temperature_k);
		real("jet_temperature_index", temperature_index);
		real("jet_frequency", observing_frequency_hz);
		real("jet_absorption_scale", absorption_scale);
		real("jet_faraday_scale", faraday_scale);
		flag("jet_auto_normalization", auto_normalization);
		real("jet_reference_temperature", reference_brightness_temperature_k);
		real("jet_brightness", brightness);
		real("jet_tint_r", tint[0]);
		real("jet_tint_g", tint[1]);
		real("jet_tint_b", tint[2]);
		display_mode = static_cast<JetDisplayMode>(index("jet_display_mode", static_cast<uint32_t>(display_mode)));
		real("jet_polarization_coherence", polarization_coherence);
		real("jet_sampling_density", sampling_density);
		double samples = static_cast<double>(max_samples);
		read("jet_max_samples", samples);
		max_samples = static_cast<int32_t>(samples);
		flag("jet_engine_length_scale", use_engine_length_scale);
		real("jet_meters_per_unit", meters_per_length_unit);
		sanitize();
	}
};

}

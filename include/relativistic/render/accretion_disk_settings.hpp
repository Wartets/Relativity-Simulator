#pragma once

#include "relativistic/render/accretion_disk_model.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

namespace Relativistic::Render {

enum class AccretionDiskPreset : uint32_t {
	BalancedThinDisk = 0,
	SmoothNovikovThorne = 1,
	TurbulentFlow = 2,
	ConcentricRings = 3,
	GrandDesignSpiral = 4,
	HotRadiativeDisk = 5,
	CoolEmberDisk = 6
};

inline constexpr size_t kAccretionDiskPresetCount = 7;

inline constexpr std::array<const char*, kAccretionDiskPresetCount> kAccretionDiskPresetNames{
	"Balanced Thin Disk",
	"Smooth Novikov-Thorne Disk",
	"Turbulent Accretion Flow",
	"Concentric Ring Structure",
	"Grand-Design Spiral Arms",
	"Hot Radiatively Efficient Disk",
	"Cool Ember Disk"
};

struct AccretionDiskSettings {
	bool enabled{true};
	float inner_radius_scale{1.0f};
	float outer_radius_mass_units{30.0f};
	float peak_temperature_k{12546.0f};
	float floor_temperature_k{1200.0f};
	float temperature_exponent{0.75f};
	float zero_torque_strength{0.94f};
	float extra_beaming_exponent{0.0f};
	float brightness{0.45f};
	float color_saturation{1.03f};
	std::array<float, 3> tint{0.35f, 0.22f, 0.03f};
	float tint_strength{0.09f};
	float opacity_scale{1.5f};
	float rotation_speed_scale{4.0f};
	float ring_amplitude{0.15f};
	float ring_frequency{9.0f};
	float ring_sharpness{4.1f};
	int32_t spiral_arm_count{4};
	float spiral_pitch{0.83f};
	float spiral_amplitude{0.43f};
	float turbulence_amplitude{1.13f};
	float turbulence_scale{6.5f};
	int32_t turbulence_octaves{5};
	float radial_stretch{1.23f};
	float grain_amplitude{0.35f};
	float grain_scale{40.0f};
	uint32_t noise_seed{2};
	float edge_softness{4.14f};

	void sanitize() noexcept {
		inner_radius_scale = std::clamp(inner_radius_scale, 1.0f, 4.0f);
		outer_radius_mass_units = std::clamp(outer_radius_mass_units, 6.0f, 120.0f);
		peak_temperature_k = std::clamp(peak_temperature_k, 1000.0f, 200000.0f);
		floor_temperature_k = std::clamp(floor_temperature_k, 0.0f, peak_temperature_k);
		temperature_exponent = std::clamp(temperature_exponent, 0.3f, 2.0f);
		zero_torque_strength = std::clamp(zero_torque_strength, 0.0f, 1.0f);
		extra_beaming_exponent = std::clamp(extra_beaming_exponent, -2.0f, 8.0f);
		brightness = std::clamp(brightness, 0.0f, 20.0f);
		color_saturation = std::clamp(color_saturation, 0.0f, 3.0f);
		for (float& channel : tint) {
			channel = std::clamp(channel, 0.0f, 4.0f);
		}
		tint_strength = std::clamp(tint_strength, 0.0f, 1.0f);
		opacity_scale = std::clamp(opacity_scale, 0.0f, 2.0f);
		rotation_speed_scale = std::clamp(rotation_speed_scale, 0.0f, 200.0f);
		ring_amplitude = std::clamp(ring_amplitude, 0.0f, 1.0f);
		ring_frequency = std::clamp(ring_frequency, 0.0f, 64.0f);
		ring_sharpness = std::clamp(ring_sharpness, 1.0f, 12.0f);
		spiral_arm_count = std::clamp<int32_t>(spiral_arm_count, 0, 8);
		spiral_pitch = std::clamp(spiral_pitch, -6.0f, 6.0f);
		spiral_amplitude = std::clamp(spiral_amplitude, 0.0f, 1.0f);
		turbulence_amplitude = std::clamp(turbulence_amplitude, 0.0f, 2.0f);
		turbulence_scale = std::clamp(turbulence_scale, 0.5f, 40.0f);
		turbulence_octaves = std::clamp<int32_t>(turbulence_octaves, 1, 6);
		radial_stretch = std::clamp(radial_stretch, 0.25f, 16.0f);
		grain_amplitude = std::clamp(grain_amplitude, 0.0f, 1.5f);
		grain_scale = std::clamp(grain_scale, 2.0f, 400.0f);
		edge_softness = std::clamp(edge_softness, 0.05f, 6.0f);
	}

	void apply_preset(AccretionDiskPreset preset, bool preserve_thermal_basics) noexcept {
		const bool kept_enabled = enabled;
		const float kept_peak = peak_temperature_k;
		const float kept_floor = floor_temperature_k;
		const float kept_saturation = color_saturation;
		const float kept_beaming = extra_beaming_exponent;
		*this = AccretionDiskSettings{};
		enabled = kept_enabled;

		switch (preset) {
			case AccretionDiskPreset::SmoothNovikovThorne:
				ring_amplitude = 0.0f;
				spiral_arm_count = 0;
				turbulence_amplitude = 0.08f;
				grain_amplitude = 0.04f;
				break;
			case AccretionDiskPreset::TurbulentFlow:
				ring_amplitude = 0.15f;
				spiral_arm_count = 3;
				spiral_amplitude = 0.35f;
				turbulence_amplitude = 0.95f;
				turbulence_scale = 6.5f;
				turbulence_octaves = 5;
				grain_amplitude = 0.35f;
				break;
			case AccretionDiskPreset::ConcentricRings:
				ring_amplitude = 0.85f;
				ring_frequency = 16.0f;
				ring_sharpness = 5.0f;
				spiral_arm_count = 0;
				turbulence_amplitude = 0.25f;
				radial_stretch = 6.0f;
				break;
			case AccretionDiskPreset::GrandDesignSpiral:
				ring_amplitude = 0.12f;
				spiral_arm_count = 3;
				spiral_amplitude = 0.75f;
				spiral_pitch = 1.9f;
				turbulence_amplitude = 0.3f;
				break;
			case AccretionDiskPreset::HotRadiativeDisk:
				peak_temperature_k = 30000.0f;
				floor_temperature_k = 3000.0f;
				brightness = 0.4f;
				temperature_exponent = 0.75f;
				turbulence_amplitude = 0.35f;
				break;
			case AccretionDiskPreset::CoolEmberDisk:
				peak_temperature_k = 4500.0f;
				floor_temperature_k = 900.0f;
				brightness = 0.8f;
				tint = {1.0f, 0.62f, 0.35f};
				tint_strength = 0.25f;
				turbulence_amplitude = 0.7f;
				grain_amplitude = 0.3f;
				break;
			case AccretionDiskPreset::BalancedThinDisk:
			default:
				break;
		}

		if (preserve_thermal_basics) {
			peak_temperature_k = kept_peak;
			floor_temperature_k = kept_floor;
			color_saturation = kept_saturation;
			extra_beaming_exponent = kept_beaming;
		}
		sanitize();
	}

	[[nodiscard]] GpuDiskProfile to_gpu_profile() const noexcept {
		AccretionDiskSettings s = *this;
		s.sanitize();
		GpuDiskProfile profile;
		profile.enabled = s.enabled ? 1.0f : 0.0f;
		profile.inner_radius_scale = s.inner_radius_scale;
		profile.outer_radius_mass_units = s.outer_radius_mass_units;
		profile.peak_temperature_k = s.peak_temperature_k;
		profile.floor_temperature_k = s.floor_temperature_k;
		profile.temperature_exponent = s.temperature_exponent;
		profile.zero_torque_strength = s.zero_torque_strength;
		profile.extra_beaming_exponent = s.extra_beaming_exponent;
		profile.brightness = s.brightness;
		profile.color_saturation = s.color_saturation;
		profile.tint_strength = s.tint_strength;
		profile.opacity_scale = s.opacity_scale;
		profile.tint_r = s.tint[0];
		profile.tint_g = s.tint[1];
		profile.tint_b = s.tint[2];
		profile.rotation_speed_scale = s.rotation_speed_scale;
		profile.ring_amplitude = s.ring_amplitude;
		profile.ring_frequency = s.ring_frequency;
		profile.ring_sharpness = s.ring_sharpness;
		profile.spiral_arm_count = static_cast<float>(s.spiral_arm_count);
		profile.spiral_pitch = s.spiral_pitch;
		profile.spiral_amplitude = s.spiral_amplitude;
		profile.turbulence_amplitude = s.turbulence_amplitude;
		profile.turbulence_scale = s.turbulence_scale;
		profile.turbulence_octaves = static_cast<float>(s.turbulence_octaves);
		profile.radial_stretch = s.radial_stretch;
		profile.grain_amplitude = s.grain_amplitude;
		profile.grain_scale = s.grain_scale;
		profile.noise_seed = static_cast<float>(s.noise_seed & 0x00FFFFFFU);
		profile.edge_softness = s.edge_softness;
		AccretionDiskModel::finalize(profile);
		return profile;
	}
};

}

#pragma once

#include "relativistic/render/gpu_types.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

namespace Relativistic::Render {

enum class HydroDiskPreset : uint32_t {
	StandardTorus = 0,
	CompactTorus = 1,
	ExtendedTorus = 2,
	SlenderRing = 3
};

inline constexpr size_t kHydroDiskModelCount = 3;
inline constexpr size_t kHydroDiskPresetCount = 4;

inline constexpr std::array<const char*, kHydroDiskModelCount> kHydroDiskModelNames{
	"Thin Procedural Disk (Stationary)",
	"Novikov-Thorne Thin Disk (Relativistic Flux Profile)",
	"Fishbone-Moncrief Thick Torus (Hydrostatic Equilibrium)"
};

inline constexpr std::array<const char*, kHydroDiskPresetCount> kHydroDiskPresetNames{
	"Standard Torus (r_in = 6 M, r_c = 12 M)",
	"Compact Torus (r_in = 5 M, r_c = 9 M)",
	"Extended Torus (r_in = 10 M, r_c = 18 M)",
	"Slender Ring (r_in = 11 M, r_c = 14 M)"
};

struct HydroDiskSettings {
	HydroDiskModel model{HydroDiskModel::ThinProcedural};
	float accretion_rate_scale{1.0f};
	float torus_inner_radius{6.0f};
	float torus_center_radius{12.0f};
	float adiabatic_index{1.3333334f};
	float optical_depth_scale{0.35f};
	float sampling_density{2.0f};

	void sanitize() noexcept {
		model = static_cast<HydroDiskModel>(std::min<uint32_t>(static_cast<uint32_t>(model), static_cast<uint32_t>(kHydroDiskModelCount - 1)));
		accretion_rate_scale = std::clamp(accretion_rate_scale, 0.01f, 100.0f);
		torus_inner_radius = std::clamp(torus_inner_radius, 1.2f, 80.0f);
		torus_center_radius = std::clamp(torus_center_radius, torus_inner_radius * 1.05f, 160.0f);
		torus_inner_radius = std::min(torus_inner_radius, torus_center_radius / 1.05f);
		adiabatic_index = std::clamp(adiabatic_index, 1.2f, 2.0f);
		optical_depth_scale = std::clamp(optical_depth_scale, 1e-3f, 50.0f);
		sampling_density = std::clamp(sampling_density, 0.5f, 8.0f);
	}

	void apply_preset(HydroDiskPreset preset) noexcept {
		switch (preset) {
			case HydroDiskPreset::CompactTorus:
				torus_inner_radius = 5.0f;
				torus_center_radius = 9.0f;
				adiabatic_index = 1.3333334f;
				optical_depth_scale = 0.5f;
				break;
			case HydroDiskPreset::ExtendedTorus:
				torus_inner_radius = 10.0f;
				torus_center_radius = 18.0f;
				adiabatic_index = 1.3333334f;
				optical_depth_scale = 0.25f;
				break;
			case HydroDiskPreset::SlenderRing:
				torus_inner_radius = 11.0f;
				torus_center_radius = 14.0f;
				adiabatic_index = 1.3333334f;
				optical_depth_scale = 0.6f;
				break;
			case HydroDiskPreset::StandardTorus:
			default:
				torus_inner_radius = 6.0f;
				torus_center_radius = 12.0f;
				adiabatic_index = 1.3333334f;
				optical_depth_scale = 0.35f;
				break;
		}
		sampling_density = 2.0f;
		sanitize();
	}

	[[nodiscard]] GpuHydroDiskProfile to_gpu_profile() const noexcept {
		HydroDiskSettings s = *this;
		s.sanitize();
		GpuHydroDiskProfile profile;
		profile.model = static_cast<uint32_t>(s.model);
		profile.accretion_rate_scale = s.accretion_rate_scale;
		profile.torus_inner_radius = s.torus_inner_radius;
		profile.torus_center_radius = s.torus_center_radius;
		profile.adiabatic_index = s.adiabatic_index;
		profile.optical_depth_scale = s.optical_depth_scale;
		profile.sampling_density = s.sampling_density;
		return profile;
	}
};

}

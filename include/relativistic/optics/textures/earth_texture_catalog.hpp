#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace Relativistic::Optics {

inline constexpr uint32_t kPhotographicSurfaceTextureModeBase = 13U;

enum class BodyTextureQuality : uint32_t {
	Q1K = 0,
	Q2K = 1
};

inline constexpr size_t kBodyTextureQualityCount = 2;

enum class BodyTextureId : uint32_t {
	Earth = 0,
	Sun = 1,
	Mercury = 2,
	VenusAtmosphere = 3,
	VenusSurface = 4,
	Mars = 5,
	Jupiter = 6,
	Saturn = 7,
	Uranus = 8,
	Neptune = 9,
	Ceres = 10,
	Eris = 11,
	Haumea = 12,
	Makemake = 13
};

inline constexpr size_t kBodyTextureIdCount = 14;

enum class EarthMapVariant : uint32_t {
	Day = 0,
	Night = 1,
	Automatic = 2
};

using EarthMapQuality = BodyTextureQuality;

enum class EarthMapKind : uint32_t {
	Day = 0,
	Night = 1
};

inline constexpr size_t kEarthMapKindCount = 2;

struct BodyTextureDescriptor {
	std::string_view identifier;
	std::string_view display_name;
	std::string_view description;
	std::array<std::string_view, kBodyTextureQualityCount> file_paths;
	bool is_emissive_star;
};

inline constexpr std::array<BodyTextureDescriptor, kBodyTextureIdCount> kBodyTextureDescriptors{{
	{
		"earth",
		"Earth (Blue Marble)",
		"Photographic land and ocean albedo mapped from satellite observations with city lights night-side integration.",
		{{"assets/textures/solarsystemscope/1k_earth_daymap.png", "assets/textures/solarsystemscope/2k_earth_daymap.png"}},
		false
	},
	{
		"sun",
		"Sun (Photosphere)",
		"Solar photospheric granulation and limb activity texture derived from SOHO and SDO imagery.",
		{{"assets/textures/solarsystemscope/1k_sun.png", "assets/textures/solarsystemscope/2k_sun.png"}},
		true
	},
	{
		"mercury",
		"Mercury (Cratered Basalt)",
		"MESSENGER photographic basemap showing impact craters and volcanic plains.",
		{{"assets/textures/solarsystemscope/1k_mercury.png", "assets/textures/solarsystemscope/2k_mercury.png"}},
		false
	},
	{
		"venus_atmosphere",
		"Venus (Cloud Atmosphere)",
		"Thick sulfuric cloud deck and atmospheric circulation pattern in visible spectrum.",
		{{"assets/textures/solarsystemscope/1k_venus_atmosphere.png", "assets/textures/solarsystemscope/2k_venus_atmosphere.png"}},
		false
	},
	{
		"venus_surface",
		"Venus (Radar Surface)",
		"Magellan radar altimetry reconstruction of the volcanic crust, tesserae, and rift valleys.",
		{{"assets/textures/solarsystemscope/1k_venus_surface.png", "assets/textures/solarsystemscope/2k_venus_surface.png"}},
		false
	},
	{
		"mars",
		"Mars (Rusty Basalt & Deserts)",
		"Viking and MRO global mosaic exhibiting Valles Marineris, Olympus Mons, and dust regions.",
		{{"assets/textures/solarsystemscope/1k_mars.png", "assets/textures/solarsystemscope/2k_mars.png"}},
		false
	},
	{
		"jupiter",
		"Jupiter (Zonal Belts & Storms)",
		"Cassini and Juno global cylindrical mosaic of ammonia clouds and the Great Red Spot.",
		{{"assets/textures/solarsystemscope/1k_jupiter.png", "assets/textures/solarsystemscope/2k_jupiter.png"}},
		false
	},
	{
		"saturn",
		"Saturn (Atmospheric Belts)",
		"Cassini high-resolution mosaic of Saturn's upper cloud layers and banded circulation.",
		{{"assets/textures/solarsystemscope/1k_saturn.png", "assets/textures/solarsystemscope/2k_saturn.png"}},
		false
	},
	{
		"uranus",
		"Uranus (Cyan Haze)",
		"Voyager 2 true-color recreation of methane haze and upper troposphere.",
		{{"assets/textures/solarsystemscope/1k_uranus.png", "assets/textures/solarsystemscope/2k_uranus.png"}},
		false
	},
	{
		"neptune",
		"Neptune (Deep Blue Dynamic)",
		"Voyager 2 true-color mosaic highlighting fast equatorial jet streams and dark vortices.",
		{{"assets/textures/solarsystemscope/1k_neptune.png", "assets/textures/solarsystemscope/2k_neptune.png"}},
		false
	},
	{
		"ceres",
		"Ceres (Dwarf Planet)",
		"Dawn mission surface mosaic of Ceres showing Occator crater bright faculae and carbon-rich regolith.",
		{{"assets/textures/solarsystemscope/1k_ceres_fictional.png", "assets/textures/solarsystemscope/2k_ceres_fictional.png"}},
		false
	},
	{
		"eris",
		"Eris (Trans-Neptunian)",
		"Photometric albedo reconstruction of the methane-nitrogen ice surface of dwarf planet Eris.",
		{{"assets/textures/solarsystemscope/1k_eris_fictional.png", "assets/textures/solarsystemscope/2k_eris_fictional.png"}},
		false
	},
	{
		"haumea",
		"Haumea (Crystalline Ice)",
		"Surface map depicting pure water-ice mantle and crystalline spectral signatures.",
		{{"assets/textures/solarsystemscope/1k_haumea_fictional.png", "assets/textures/solarsystemscope/2k_haumea_fictional.png"}},
		false
	},
	{
		"makemake",
		"Makemake (Tholin Frost)",
		"Photometric surface reconstruction exhibiting frozen methane and red-brown tholin hydrocarbons.",
		{{"assets/textures/solarsystemscope/1k_makemake_fictional.png", "assets/textures/solarsystemscope/2k_makemake_fictional.png"}},
		false
	}
}};

inline constexpr std::array<std::string_view, kBodyTextureQualityCount> kEarthNightMapPaths{{
	"assets/textures/solarsystemscope/1k_earth_nightmap.png",
	"assets/textures/solarsystemscope/2k_earth_nightmap.png"
}};

[[nodiscard]] constexpr uint32_t body_texture_to_surface_mode(BodyTextureId id) noexcept {
	return kPhotographicSurfaceTextureModeBase + static_cast<uint32_t>(id);
}

[[nodiscard]] constexpr bool is_photographic_surface_mode(uint32_t mode) noexcept {
	return mode >= kPhotographicSurfaceTextureModeBase && mode < (kPhotographicSurfaceTextureModeBase + kBodyTextureIdCount);
}

[[nodiscard]] constexpr BodyTextureId surface_mode_to_body_texture(uint32_t mode) noexcept {
	const uint32_t idx = (mode >= kPhotographicSurfaceTextureModeBase) ? (mode - kPhotographicSurfaceTextureModeBase) : 0U;
	return (idx < kBodyTextureIdCount) ? static_cast<BodyTextureId>(idx) : BodyTextureId::Earth;
}

[[nodiscard]] constexpr BodyTextureQuality body_texture_quality_from_index(uint32_t index) noexcept {
	return (index >= 1U) ? BodyTextureQuality::Q2K : BodyTextureQuality::Q1K;
}

[[nodiscard]] constexpr EarthMapQuality earth_map_quality_from_index(uint32_t index) noexcept {
	return body_texture_quality_from_index(index);
}

[[nodiscard]] constexpr EarthMapVariant earth_map_variant_from_index(uint32_t index) noexcept {
	switch (index) {
		case 1U: return EarthMapVariant::Night;
		case 2U: return EarthMapVariant::Automatic;
		default: return EarthMapVariant::Day;
	}
}

[[nodiscard]] constexpr bool earth_variant_uses_day_map(EarthMapVariant variant) noexcept {
	return variant != EarthMapVariant::Night;
}

[[nodiscard]] constexpr bool earth_variant_uses_night_map(EarthMapVariant variant) noexcept {
	return variant != EarthMapVariant::Day;
}

[[nodiscard]] constexpr std::string_view body_texture_relative_path(BodyTextureId id, BodyTextureQuality quality) noexcept {
	return kBodyTextureDescriptors[static_cast<size_t>(id)].file_paths[static_cast<size_t>(quality)];
}

[[nodiscard]] constexpr std::string_view earth_night_relative_path(BodyTextureQuality quality) noexcept {
	return kEarthNightMapPaths[static_cast<size_t>(quality)];
}

[[nodiscard]] constexpr std::string_view earth_map_relative_path(EarthMapKind kind, BodyTextureQuality quality) noexcept {
	return (kind == EarthMapKind::Night) ? earth_night_relative_path(quality) : body_texture_relative_path(BodyTextureId::Earth, quality);
}

class EarthTextureIdleGate {
private:
	std::chrono::steady_clock::time_point last_demand_{std::chrono::steady_clock::now()};

public:
	static constexpr std::chrono::seconds kGracePeriod{10};

	[[nodiscard]] bool should_release(bool demanded) noexcept {
		const auto now = std::chrono::steady_clock::now();
		if (demanded) {
			last_demand_ = now;
			return false;
		}
		return (now - last_demand_) >= kGracePeriod;
	}
};

}

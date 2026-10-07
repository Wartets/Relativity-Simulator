#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace Relativistic::Optics {

enum class SkyPanoramaId : uint32_t {
	NightSkyHDRI001 = 0,
	NightSkyHDRI008 = 1,
	Eso0932a = 2
};

enum class SkyPanoramaQuality : uint32_t {
	Q1K = 0,
	Q2K = 1,
	Q4K = 2
};

struct SkyPanoramaVariant {
	std::string_view relative_path;
	uint32_t width;
	uint32_t height;
};

struct SkyPanoramaEntry {
	std::string_view display_name;
	bool has_quality_variants;
	std::array<SkyPanoramaVariant, 3> variants;
};

[[nodiscard]] inline const SkyPanoramaEntry& sky_panorama_catalog_entry(SkyPanoramaId id) noexcept {
	static const std::array<SkyPanoramaEntry, 3> kCatalog{{
		SkyPanoramaEntry{
			"Night Sky HDRI 001",
			true,
			{{
				{"assets/sky/ambientcg/NightSkyHDRI001_1K/NightSkyHDRI001_1K_TONEMAPPED.jpg", 1024, 512},
				{"assets/sky/ambientcg/NightSkyHDRI001_2K/NightSkyHDRI001_2K_TONEMAPPED.jpg", 2048, 1024},
				{"assets/sky/ambientcg/NightSkyHDRI001_4K/NightSkyHDRI001_4K_TONEMAPPED.jpg", 4096, 2048}
			}}
		},
		SkyPanoramaEntry{
			"Night Sky HDRI 008",
			true,
			{{
				{"assets/sky/ambientcg/NightSkyHDRI008_1K/NightSkyHDRI008_1K_TONEMAPPED.jpg", 1024, 512},
				{"assets/sky/ambientcg/NightSkyHDRI008_2K/NightSkyHDRI008_2K_TONEMAPPED.jpg", 2048, 1024},
				{"assets/sky/ambientcg/NightSkyHDRI008_4K/NightSkyHDRI008_4K_TONEMAPPED.jpg", 4096, 2048}
			}}
		},
		SkyPanoramaEntry{
			"ESO 0932a",
			false,
			{{
				{"assets/sky/eso/eso0932a.tif", 6000, 3000},
				{"assets/sky/eso/eso0932a.tif", 6000, 3000},
				{"assets/sky/eso/eso0932a.tif", 6000, 3000}
			}}
		}
	}};
	return kCatalog[static_cast<size_t>(id)];
}

[[nodiscard]] inline std::string_view sky_panorama_relative_path(SkyPanoramaId id, SkyPanoramaQuality quality) noexcept {
	const auto& entry = sky_panorama_catalog_entry(id);
	const size_t idx = entry.has_quality_variants ? static_cast<size_t>(quality) : size_t{0};
	return entry.variants[idx].relative_path;
}

}

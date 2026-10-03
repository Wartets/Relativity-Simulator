#pragma once

#include "relativistic/optics/earth_texture_catalog.hpp"
#include <algorithm>
#include <cstdint>
#include <span>

namespace Relativistic::Render {

struct EarthTextureRequirements {
	bool day{false};
	bool night{false};
	Optics::EarthMapQuality quality{Optics::EarthMapQuality::Q1K};

	[[nodiscard]] constexpr bool any() const noexcept {
		return day || night;
	}

	template <typename BodyLayout>
	[[nodiscard]] static EarthTextureRequirements gather(std::span<const BodyLayout> bodies) noexcept {
		EarthTextureRequirements result{};
		for (const auto& body : bodies) {
			if (body.surface_texture_mode != Optics::kEarthSurfaceTextureMode) {
				continue;
			}
			const auto variant = Optics::earth_map_variant_from_index(body.earth_map_variant);
			result.day = result.day || Optics::earth_variant_uses_day_map(variant);
			result.night = result.night || Optics::earth_variant_uses_night_map(variant);
			result.quality = std::max(result.quality, Optics::earth_map_quality_from_index(body.earth_map_quality));
		}
		return result;
	}
};

}

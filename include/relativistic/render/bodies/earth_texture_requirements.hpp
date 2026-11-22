#pragma once

#include "relativistic/optics/textures/earth_texture_catalog.hpp"
#include <algorithm>
#include <cstdint>
#include <span>

namespace Relativistic::Render {

struct EarthTextureRequirements {
	bool day{false};
	bool night{false};
	std::array<bool, Optics::kBodyTextureIdCount> textures{};
	Optics::BodyTextureQuality quality{Optics::BodyTextureQuality::Q1K};

	[[nodiscard]] constexpr bool any() const noexcept {
		if (day || night) return true;
		for (bool active : textures) {
			if (active) return true;
		}
		return false;
	}

	template <typename BodyLayout>
	[[nodiscard]] static EarthTextureRequirements gather(std::span<const BodyLayout> bodies) noexcept {
		EarthTextureRequirements result{};
		for (const auto& body : bodies) {
			if (!Optics::is_photographic_surface_mode(body.surface_texture_mode)) {
				continue;
			}
			const auto texture_id = Optics::surface_mode_to_body_texture(body.surface_texture_mode);
			result.quality = std::max(result.quality, Optics::body_texture_quality_from_index(body.earth_map_quality));
			if (texture_id == Optics::BodyTextureId::Earth) {
				const auto variant = Optics::earth_map_variant_from_index(body.earth_map_variant);
				const bool uses_day = Optics::earth_variant_uses_day_map(variant);
				const bool uses_night = Optics::earth_variant_uses_night_map(variant);
				result.day = result.day || uses_day;
				result.night = result.night || uses_night;
				result.textures[static_cast<size_t>(texture_id)] = result.textures[static_cast<size_t>(texture_id)] || uses_day;
			} else {
				result.textures[static_cast<size_t>(texture_id)] = true;
			}
		}
		return result;
	}
};

}

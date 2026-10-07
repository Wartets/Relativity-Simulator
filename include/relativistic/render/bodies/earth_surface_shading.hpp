#pragma once

#include "relativistic/optics/textures/earth_texture_catalog.hpp"
#include <algorithm>
#include <array>

namespace Relativistic::Render {

struct EarthSurfaceColor {
	std::array<float, 3> albedo{0.0f, 0.0f, 0.0f};
	std::array<float, 3> emissive{0.0f, 0.0f, 0.0f};
};

class EarthSurfaceShading {
public:
	[[nodiscard]] static float day_weight(float illumination, float softness) noexcept {
		const float half_width = std::max(softness * 0.5f, 0.002f);
		const float t = std::clamp((illumination + half_width) / (2.0f * half_width), 0.0f, 1.0f);
		return t * t * (3.0f - 2.0f * t);
	}

	[[nodiscard]] static EarthSurfaceColor blend(
		Optics::EarthMapVariant variant,
		float softness,
		float illumination,
		const std::array<float, 3>& day_rgb,
		const std::array<float, 3>& night_rgb
	) noexcept {
		EarthSurfaceColor result;
		switch (variant) {
			case Optics::EarthMapVariant::Night:
				result.emissive = night_rgb;
				break;
			case Optics::EarthMapVariant::Automatic: {
				const float weight = day_weight(illumination, softness);
				for (size_t c = 0; c < 3; ++c) {
					result.albedo[c] = day_rgb[c] * weight;
					result.emissive[c] = night_rgb[c] * (1.0f - weight);
				}
				break;
			}
			case Optics::EarthMapVariant::Day:
			default:
				result.albedo = day_rgb;
				break;
		}
		return result;
	}
};

}

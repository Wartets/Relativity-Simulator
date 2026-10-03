#pragma once

#include "relativistic/render/gpu_types.hpp"
#include "relativistic/optics/earth_texture_catalog.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace Relativistic::Render {

struct SurfaceShadingState {
	std::array<float, 3> color{0.0f, 0.0f, 0.0f};
	std::array<float, 3> emissive{0.0f, 0.0f, 0.0f};
	bool city_lights{false};
	float city_speckle{0.0f};
};

class BodySurfaceShading {
private:
	[[nodiscard]] static float smooth_step(float edge0, float edge1, float x) noexcept {
		const float t = std::clamp((x - edge0) / std::max(edge1 - edge0, 1e-6f), 0.0f, 1.0f);
		return t * t * (3.0f - 2.0f * t);
	}

	template <typename Noise>
	[[nodiscard]] static float pattern_value(
		uint32_t pattern,
		float px, float py, float pz,
		float theta, float phi,
		float scale, int octaves, uint32_t seed
	) noexcept {
		const float qx = px * scale + static_cast<float>(seed % 977U) * 0.731f;
		const float qy = py * scale - static_cast<float>(seed % 613U) * 0.417f;
		const float qz = pz * scale + static_cast<float>(seed % 389U) * 0.293f;
		const auto fbm = [&](float multiplier, int count, float roughness, float offset) noexcept {
			return Noise::fbm(qx * multiplier + offset, qy * multiplier + offset, qz * multiplier + offset, count, roughness);
		};

		switch (pattern) {
			case 1U: {
				const float r = 1.0f - std::abs(fbm(1.0f, octaves, 0.5f, 0.0f));
				return r * r;
			}
			case 2U:
				return std::abs(fbm(1.0f, octaves, 0.5f, 0.0f));
			case 3U:
				return 0.5f + 0.5f * std::sin(theta * scale * 2.0f + fbm(1.0f, octaves, 0.5f, 0.0f) * 2.0f);
			case 4U:
				return 0.5f + 0.5f * std::sin(phi * scale + fbm(1.0f, octaves, 0.5f, 0.0f) * 2.0f);
			case 5U: {
				const float n = 0.5f + 0.5f * fbm(4.0f, 2, 0.5f, 0.0f);
				return smooth_step(0.55f, 1.0f, n);
			}
			case 6U: {
				const float c = 0.5f + 0.5f * fbm(1.0f, octaves, 0.5f, 0.0f);
				return std::clamp(smooth_step(0.55f, 0.62f, c) - 0.6f * smooth_step(0.62f, 0.85f, c), 0.0f, 1.0f);
			}
			case 7U: {
				const float n = fbm(1.0f, octaves, 0.5f, 0.0f);
				return std::pow(std::clamp(1.0f - std::abs(n) * 2.2f, 0.0f, 1.0f), 6.0f);
			}
			case 8U:
				return 0.5f + 0.5f * std::sin((phi + theta) * scale * 0.5f + fbm(1.0f, octaves, 0.5f, 0.0f) * 6.0f);
			case 9U: {
				const float warp = fbm(0.5f, 2, 0.5f, 0.0f);
				return 0.5f + 0.5f * fbm(1.0f, octaves, 0.55f, warp * 1.5f);
			}
			default:
				return 0.5f + 0.5f * fbm(1.0f, octaves, 0.5f, 0.0f);
		}
	}

	[[nodiscard]] static float mask_value(uint32_t mask, float cos_theta, float sun_facing, float width) noexcept {
		switch (mask) {
			case 1U: return smooth_step(1.0f - width, 1.0f, std::abs(cos_theta));
			case 2U: return 1.0f - smooth_step(0.0f, width, std::abs(cos_theta));
			case 3U: return smooth_step(-0.5f * width, 0.5f * width, cos_theta);
			case 4U: return 1.0f - smooth_step(-0.5f * width, 0.5f * width, cos_theta);
			case 5U: return smooth_step(-0.5f * width, 0.5f * width, sun_facing);
			case 6U: return 1.0f - smooth_step(-0.5f * width, 0.5f * width, sun_facing);
			default: return 1.0f;
		}
	}

	[[nodiscard]] static float blend_channel(uint32_t blend, float base, float layer, float amount) noexcept {
		switch (blend) {
			case 1U:
				return base + layer * amount;
			case 2U:
				return base + (base * layer - base) * amount;
			case 3U:
				return base + ((1.0f - (1.0f - base) * (1.0f - layer)) - base) * amount;
			case 4U: {
				const float result = (base < 0.5f)
					? 2.0f * base * layer
					: 1.0f - 2.0f * (1.0f - base) * (1.0f - layer);
				return base + (result - base) * amount;
			}
			default:
				return base + (layer - base) * amount;
		}
	}

public:
	template <typename Noise>
	static void apply(
		const GpuBodyData& body,
		uint32_t texture_mode,
		float theta,
		float phi,
		float time,
		float sun_facing,
		SurfaceShadingState& state
	) noexcept {
		const float cos_theta = std::cos(theta);
		const float sin_theta = std::sin(theta);
		const float detail = static_cast<float>(std::max(body.texture_detail_scale, 0.1));
		const float noise_scale = static_cast<float>(std::max(body.noise_scale, 0.1)) * detail;
		const std::array<float, 3> tertiary{
			static_cast<float>(body.color_tertiary[0]),
			static_cast<float>(body.color_tertiary[1]),
			static_cast<float>(body.color_tertiary[2])
		};

		if (texture_mode != 8U && texture_mode != Optics::kEarthSurfaceTextureMode) {
			const float polar_strength = static_cast<float>(std::clamp(body.polar_cap_strength, 0.0, 1.0));
			if (polar_strength > 0.0f) {
				const float weight = std::pow(std::abs(cos_theta), 3.0f) * polar_strength;
				for (size_t c = 0; c < 3; ++c) {
					state.color[c] = state.color[c] * (1.0f - weight) + tertiary[c] * weight;
				}
			}
			if (body.ring_system_enabled > 0.5) {
				const float band = std::pow(std::clamp(1.0f - std::abs(theta - 1.57079633f) * 3.5f, 0.0f, 1.0f), 2.0f) * 0.35f;
				for (size_t c = 0; c < 3; ++c) {
					state.color[c] *= (1.0f - band);
				}
			}
		}

		if (texture_mode != 11U && texture_mode != Optics::kEarthSurfaceTextureMode && body.night_side_light_intensity > 0.0) {
			const float sx = sin_theta * std::cos(phi) * noise_scale;
			const float sy = sin_theta * std::sin(phi) * noise_scale;
			const float sz = cos_theta * noise_scale;
			const float speckle = Noise::fbm(sx * 2.3f + 11.0f, sy * 2.3f - 7.0f, sz * 2.3f + 3.0f, 3, 0.5f);
			state.city_lights = true;
			state.city_speckle = std::clamp((speckle + 1.0f) * 0.5f, 0.0f, 1.0f);
		}

		const uint32_t layer_total = std::min<uint32_t>(body.surface_layer_count, static_cast<uint32_t>(kMaxSurfaceLayers));
		for (uint32_t li = 0; li < layer_total; ++li) {
			const GpuSurfaceLayer& layer = body.surface_layers[li];
			if (layer.enabled == 0U || layer.opacity <= 0.0f) {
				continue;
			}
			const float layer_phi = phi + layer.rotation_factor * time;
			const float px = sin_theta * std::cos(layer_phi);
			const float py = sin_theta * std::sin(layer_phi);
			const int octaves = static_cast<int>(std::clamp<uint32_t>(layer.octaves, 1U, 6U));
			float value = pattern_value<Noise>(layer.pattern, px, py, cos_theta, theta, layer_phi, layer.scale, octaves, layer.seed);
			value = std::clamp((value - 0.5f) * layer.contrast + 0.5f, 0.0f, 1.0f);
			value = smooth_step(layer.threshold, layer.threshold + std::max(layer.softness, 0.001f), value);
			const float mask = mask_value(layer.mask, cos_theta, sun_facing, layer.mask_width);
			const float amount = std::clamp(layer.opacity * mask * value, 0.0f, 1.0f);
			for (size_t c = 0; c < 3; ++c) {
				state.color[c] = blend_channel(layer.blend, state.color[c], layer.color[c], amount);
				state.emissive[c] += layer.color[c] * amount * layer.emission;
			}
		}
	}
};

}

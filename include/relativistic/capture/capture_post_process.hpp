#pragma once

#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/render/gpu_types.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>

namespace Relativistic::Capture {

struct ColorGrade {
	float contrast{1.0f};
	float saturation{1.0f};
	float lift{0.0f};
	float inverse_gamma{1.0f};
	float gain{1.0f};
	float highlights{0.0f};
	float shadows{0.0f};
	float vignette{0.0f};

	[[nodiscard]] static ColorGrade from_parameters(const Orchestrator::PhysicalParameters& parameters) noexcept {
		ColorGrade grade;
		grade.contrast = static_cast<float>(parameters.post_contrast);
		grade.saturation = static_cast<float>(parameters.post_saturation);
		grade.lift = static_cast<float>(parameters.post_lift);
		grade.inverse_gamma = 1.0f / static_cast<float>(std::max(parameters.post_gamma, 0.01));
		grade.gain = static_cast<float>(parameters.post_gain);
		grade.highlights = static_cast<float>(parameters.post_highlights);
		grade.shadows = static_cast<float>(parameters.post_shadows);
		grade.vignette = static_cast<float>(parameters.post_vignette_strength);
		return grade;
	}

	[[nodiscard]] bool is_identity() const noexcept {
		return contrast == 1.0f && saturation == 1.0f && lift == 0.0f && inverse_gamma == 1.0f && gain == 1.0f
			&& highlights == 0.0f && shadows == 0.0f && vignette <= 0.0f;
	}

	[[nodiscard]] float grade_channel(float value) const noexcept {
		float c = std::clamp(value + lift * (1.0f - value), 0.0f, 4.0f);
		c = (c - 0.5f) * contrast + 0.5f;
		c = std::max(c, 0.0f);
		c = std::pow(c, inverse_gamma) * gain;
		if (highlights != 0.0f) {
			const float weight = std::clamp((c - 0.6f) / 0.4f, 0.0f, 1.0f);
			c += highlights * weight * (1.0f - c) * 0.5f;
		}
		if (shadows != 0.0f) {
			const float weight = std::clamp(1.0f - c / 0.4f, 0.0f, 1.0f);
			c += shadows * weight * c * 0.5f;
		}
		return std::clamp(c, 0.0f, 1.0f);
	}

	void apply(std::span<Render::GpuPixelOutput> pixels, uint32_t width, uint32_t first_row, uint32_t total_height) const noexcept {
		if (width == 0U || total_height == 0U || is_identity()) {
			return;
		}
		const float inverse_width = 1.0f / static_cast<float>(width);
		const float inverse_height = 1.0f / static_cast<float>(total_height);
		for (size_t i = 0; i < pixels.size(); ++i) {
			const uint32_t x = static_cast<uint32_t>(i % width);
			const uint32_t y = first_row + static_cast<uint32_t>(i / width);
			Render::GpuPixelOutput& pixel = pixels[i];
			float r = grade_channel(pixel.r);
			float g = grade_channel(pixel.g);
			float b = grade_channel(pixel.b);
			const float luma = 0.2126f * r + 0.7152f * g + 0.0722f * b;
			r = std::clamp(luma + (r - luma) * saturation, 0.0f, 1.0f);
			g = std::clamp(luma + (g - luma) * saturation, 0.0f, 1.0f);
			b = std::clamp(luma + (b - luma) * saturation, 0.0f, 1.0f);
			if (vignette > 0.0f) {
				const float px = (static_cast<float>(x) + 0.5f) * inverse_width - 0.5f;
				const float py = (static_cast<float>(y) + 0.5f) * inverse_height - 0.5f;
				const float distance_squared = std::min((px * px + py * py) * 2.0f, 1.0f);
				const float falloff = 1.0f - vignette * distance_squared;
				r *= falloff;
				g *= falloff;
				b *= falloff;
			}
			pixel.r = r;
			pixel.g = g;
			pixel.b = b;
		}
	}
};

struct FramePostProcess {
	ColorGrade grade{};
	float dither_strength{0.0f};

	[[nodiscard]] bool active() const noexcept {
		return !grade.is_identity() || dither_strength > 0.0f;
	}

	void apply(std::span<Render::GpuPixelOutput> pixels, uint32_t width, uint32_t first_row, uint32_t total_height) const noexcept {
		if (width == 0U || !active()) {
			return;
		}
		grade.apply(pixels, width, first_row, total_height);
		if (dither_strength <= 0.0f) {
			return;
		}
		const float amplitude = dither_strength / 255.0f;
		for (size_t i = 0; i < pixels.size(); ++i) {
			const uint32_t x = static_cast<uint32_t>(i % width);
			const uint32_t y = first_row + static_cast<uint32_t>(i / width);
			Render::GpuPixelOutput& pixel = pixels[i];
			pixel.r = std::clamp(pixel.r + amplitude * triangular_noise(x, y, 1U), 0.0f, 1.0f);
			pixel.g = std::clamp(pixel.g + amplitude * triangular_noise(x, y, 2U), 0.0f, 1.0f);
			pixel.b = std::clamp(pixel.b + amplitude * triangular_noise(x, y, 3U), 0.0f, 1.0f);
		}
	}

private:
	[[nodiscard]] static float hash_unit(uint32_t x, uint32_t y, uint32_t salt) noexcept {
		uint32_t h = (x * 0x8DA6B343U) ^ (y * 0xD8163841U) ^ (salt * 0xCB1AB31FU);
		h ^= h >> 16;
		h *= 0x7FEB352DU;
		h ^= h >> 15;
		h *= 0x846CA68BU;
		h ^= h >> 16;
		return static_cast<float>(h & 0x00FFFFFFU) * (1.0f / 16777216.0f);
	}

	[[nodiscard]] static float triangular_noise(uint32_t x, uint32_t y, uint32_t channel) noexcept {
		return hash_unit(x, y, channel * 2U) + hash_unit(x, y, channel * 2U + 1U) - 1.0f;
	}
};

}

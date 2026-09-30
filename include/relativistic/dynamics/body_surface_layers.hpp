#pragma once

#include "relativistic/render/gpu_types.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <unordered_map>

namespace Relativistic::Dynamics {

inline constexpr std::array<const char*, 10> kSurfaceLayerPatternNames{
	"Fractal Noise", "Ridged Veins", "Billow Clumps", "Latitude Bands", "Meridian Stripes",
	"Speckle Field", "Impact Craters", "Fracture Cracks", "Spiral Swirls", "Turbulent Clouds"
};

inline constexpr std::array<const char*, 5> kSurfaceLayerBlendNames{
	"Mix", "Additive", "Multiply", "Screen", "Overlay"
};

inline constexpr std::array<const char*, 7> kSurfaceLayerMaskNames{
	"Whole Surface", "Polar Caps", "Equatorial Band", "Northern Hemisphere",
	"Southern Hemisphere", "Day Side", "Night Side"
};

inline constexpr std::array<const char*, 8> kSurfaceLayerTemplateNames{
	"Polar Ice Caps", "City Lights", "Cloud Cover", "Impact Craters",
	"Lava Cracks", "Storm Swirls", "Dust Bands", "Frost Speckle"
};

enum class SurfaceLayerTemplate : uint32_t {
	PolarIceCaps = 0,
	CityLights = 1,
	CloudCover = 2,
	ImpactCraters = 3,
	LavaCracks = 4,
	StormSwirls = 5,
	DustBands = 6,
	FrostSpeckle = 7
};

struct SurfaceLayerDefinition {
	bool enabled{true};
	Render::SurfaceLayerPattern pattern{Render::SurfaceLayerPattern::Noise};
	Render::SurfaceLayerBlend blend{Render::SurfaceLayerBlend::Mix};
	Render::SurfaceLayerMask mask{Render::SurfaceLayerMask::Global};
	std::array<float, 4> color{1.0f, 1.0f, 1.0f, 1.0f};
	float opacity{0.6f};
	float scale{6.0f};
	float contrast{1.0f};
	float threshold{0.35f};
	float softness{0.3f};
	float mask_width{0.35f};
	float rotation_factor{0.0f};
	float emission{0.0f};
	uint32_t seed{1};
	uint32_t octaves{3};

	[[nodiscard]] static SurfaceLayerDefinition from_template(SurfaceLayerTemplate layer_template) noexcept {
		SurfaceLayerDefinition d;
		switch (layer_template) {
			case SurfaceLayerTemplate::PolarIceCaps:
				d.pattern = Render::SurfaceLayerPattern::Noise;
				d.blend = Render::SurfaceLayerBlend::Mix;
				d.mask = Render::SurfaceLayerMask::PolarCaps;
				d.color = {0.95f, 0.97f, 1.0f, 1.0f};
				d.opacity = 0.9f;
				d.scale = 5.0f;
				d.threshold = 0.1f;
				d.softness = 0.5f;
				d.mask_width = 0.3f;
				break;
			case SurfaceLayerTemplate::CityLights:
				d.pattern = Render::SurfaceLayerPattern::Speckle;
				d.blend = Render::SurfaceLayerBlend::Add;
				d.mask = Render::SurfaceLayerMask::NightSide;
				d.color = {1.0f, 0.85f, 0.5f, 1.0f};
				d.opacity = 0.3f;
				d.scale = 8.0f;
				d.threshold = 0.2f;
				d.softness = 0.2f;
				d.mask_width = 0.3f;
				d.emission = 1.5f;
				d.seed = 7;
				break;
			case SurfaceLayerTemplate::CloudCover:
				d.pattern = Render::SurfaceLayerPattern::Clouds;
				d.blend = Render::SurfaceLayerBlend::Mix;
				d.color = {1.0f, 1.0f, 1.0f, 1.0f};
				d.opacity = 0.75f;
				d.scale = 3.0f;
				d.threshold = 0.45f;
				d.softness = 0.4f;
				d.rotation_factor = 0.15f;
				d.octaves = 4;
				d.seed = 3;
				break;
			case SurfaceLayerTemplate::ImpactCraters:
				d.pattern = Render::SurfaceLayerPattern::Craters;
				d.blend = Render::SurfaceLayerBlend::Multiply;
				d.color = {0.45f, 0.42f, 0.4f, 1.0f};
				d.opacity = 0.8f;
				d.scale = 7.0f;
				d.threshold = 0.3f;
				d.softness = 0.3f;
				d.seed = 11;
				break;
			case SurfaceLayerTemplate::LavaCracks:
				d.pattern = Render::SurfaceLayerPattern::Cracks;
				d.blend = Render::SurfaceLayerBlend::Add;
				d.color = {1.0f, 0.35f, 0.05f, 1.0f};
				d.opacity = 1.0f;
				d.scale = 5.0f;
				d.threshold = 0.2f;
				d.softness = 0.3f;
				d.emission = 1.6f;
				d.seed = 5;
				break;
			case SurfaceLayerTemplate::StormSwirls:
				d.pattern = Render::SurfaceLayerPattern::Swirl;
				d.blend = Render::SurfaceLayerBlend::Overlay;
				d.mask = Render::SurfaceLayerMask::EquatorialBand;
				d.color = {0.9f, 0.85f, 0.8f, 1.0f};
				d.opacity = 0.6f;
				d.scale = 4.0f;
				d.threshold = 0.3f;
				d.mask_width = 0.6f;
				d.rotation_factor = 0.3f;
				d.seed = 13;
				break;
			case SurfaceLayerTemplate::DustBands:
				d.pattern = Render::SurfaceLayerPattern::Bands;
				d.blend = Render::SurfaceLayerBlend::Multiply;
				d.color = {0.7f, 0.55f, 0.4f, 1.0f};
				d.opacity = 0.6f;
				d.scale = 5.0f;
				d.threshold = 0.4f;
				break;
			case SurfaceLayerTemplate::FrostSpeckle:
				d.pattern = Render::SurfaceLayerPattern::Speckle;
				d.blend = Render::SurfaceLayerBlend::Screen;
				d.color = {0.8f, 0.9f, 1.0f, 1.0f};
				d.opacity = 0.5f;
				d.scale = 10.0f;
				d.threshold = 0.25f;
				d.seed = 17;
				break;
		}
		return d;
	}

	[[nodiscard]] Render::GpuSurfaceLayer to_gpu_layer() const noexcept {
		Render::GpuSurfaceLayer g;
		g.color = color;
		g.opacity = std::clamp(opacity, 0.0f, 1.0f);
		g.scale = std::max(scale, 0.05f);
		g.contrast = std::clamp(contrast, 0.1f, 6.0f);
		g.threshold = std::clamp(threshold, 0.0f, 0.98f);
		g.softness = std::clamp(softness, 0.001f, 1.0f);
		g.mask_width = std::clamp(mask_width, 0.01f, 1.0f);
		g.rotation_factor = rotation_factor;
		g.emission = std::max(emission, 0.0f);
		g.pattern = static_cast<uint32_t>(pattern);
		g.blend = static_cast<uint32_t>(blend);
		g.mask = static_cast<uint32_t>(mask);
		g.seed = seed;
		g.enabled = enabled ? 1U : 0U;
		g.octaves = std::clamp<uint32_t>(octaves, 1U, 6U);
		return g;
	}
};

struct BodySurfaceLayerSet {
	std::array<SurfaceLayerDefinition, Render::kMaxSurfaceLayers> layers{};
	uint32_t count{0};

	bool add(const SurfaceLayerDefinition& definition) noexcept {
		if (count >= Render::kMaxSurfaceLayers) {
			return false;
		}
		layers[count] = definition;
		++count;
		return true;
	}

	bool remove(uint32_t index) noexcept {
		if (index >= count) {
			return false;
		}
		for (uint32_t i = index; i + 1U < count; ++i) {
			layers[i] = layers[i + 1U];
		}
		layers[count - 1U] = SurfaceLayerDefinition{};
		--count;
		return true;
	}

	bool move(uint32_t index, int delta) noexcept {
		const int target = static_cast<int>(index) + delta;
		if (index >= count || target < 0 || target >= static_cast<int>(count)) {
			return false;
		}
		std::swap(layers[index], layers[static_cast<uint32_t>(target)]);
		return true;
	}
};

class BodySurfaceLayerRegistry {
private:
	mutable std::mutex mutex_;
	std::unordered_map<uint32_t, BodySurfaceLayerSet> sets_;

public:
	[[nodiscard]] BodySurfaceLayerSet get(uint32_t body_id) const {
		std::lock_guard<std::mutex> lock(mutex_);
		const auto it = sets_.find(body_id);
		return (it != sets_.end()) ? it->second : BodySurfaceLayerSet{};
	}

	void set(uint32_t body_id, const BodySurfaceLayerSet& layer_set) {
		std::lock_guard<std::mutex> lock(mutex_);
		if (layer_set.count == 0U) {
			sets_.erase(body_id);
			return;
		}
		sets_[body_id] = layer_set;
	}

	void erase(uint32_t body_id) {
		std::lock_guard<std::mutex> lock(mutex_);
		sets_.erase(body_id);
	}

	void clear() {
		std::lock_guard<std::mutex> lock(mutex_);
		sets_.clear();
	}

	void apply_to(uint32_t body_id, Render::GpuBodyData& data) const {
		std::lock_guard<std::mutex> lock(mutex_);
		const auto it = sets_.find(body_id);
		if (it == sets_.end()) {
			data.surface_layer_count = 0U;
			return;
		}
		const BodySurfaceLayerSet& layer_set = it->second;
		const uint32_t total = std::min<uint32_t>(layer_set.count, static_cast<uint32_t>(Render::kMaxSurfaceLayers));
		data.surface_layer_count = total;
		for (uint32_t i = 0; i < total; ++i) {
			data.surface_layers[i] = layer_set.layers[i].to_gpu_layer();
		}
	}
};

}

#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace Relativistic::Optics {

inline constexpr uint32_t kEarthSurfaceTextureMode = 13U;

enum class EarthMapKind : uint32_t {
	Day = 0,
	Night = 1
};

enum class EarthMapVariant : uint32_t {
	Day = 0,
	Night = 1,
	Automatic = 2
};

enum class EarthMapQuality : uint32_t {
	Q1K = 0,
	Q2K = 1
};

inline constexpr size_t kEarthMapKindCount = 2;
inline constexpr size_t kEarthMapQualityCount = 2;

inline constexpr std::array<std::array<std::string_view, kEarthMapQualityCount>, kEarthMapKindCount> kEarthMapPaths{{
	{{"assets/earth/solarsystemscope/1k_earth_daymap.png", "assets/earth/solarsystemscope/2k_earth_daymap.png"}},
	{{"assets/earth/solarsystemscope/1k_earth_nightmap.png", "assets/earth/solarsystemscope/2k_earth_nightmap.png"}}
}};

[[nodiscard]] constexpr EarthMapVariant earth_map_variant_from_index(uint32_t index) noexcept {
	switch (index) {
		case 1U: return EarthMapVariant::Night;
		case 2U: return EarthMapVariant::Automatic;
		default: return EarthMapVariant::Day;
	}
}

[[nodiscard]] constexpr EarthMapQuality earth_map_quality_from_index(uint32_t index) noexcept {
	return (index >= 1U) ? EarthMapQuality::Q2K : EarthMapQuality::Q1K;
}

[[nodiscard]] constexpr bool earth_variant_uses_day_map(EarthMapVariant variant) noexcept {
	return variant != EarthMapVariant::Night;
}

[[nodiscard]] constexpr bool earth_variant_uses_night_map(EarthMapVariant variant) noexcept {
	return variant != EarthMapVariant::Day;
}

[[nodiscard]] constexpr std::string_view earth_map_relative_path(EarthMapKind kind, EarthMapQuality quality) noexcept {
	return kEarthMapPaths[static_cast<size_t>(kind)][static_cast<size_t>(quality)];
}

class EarthTextureIdleGate {
private:
	std::chrono::steady_clock::time_point last_demand_{std::chrono::steady_clock::now()};

public:
	static constexpr std::chrono::seconds kGracePeriod{5};

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

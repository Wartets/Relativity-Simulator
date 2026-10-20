#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unordered_map>

namespace Relativistic::UI {

enum class SecondaryViewRole : uint32_t {
	FreeObserver = 0,
	ComovingProbe = 1,
	AsymptoticGlobal = 2,
	Equirectangular360 = 3
};

inline constexpr size_t kSecondaryViewRoleCount = 4;

inline constexpr std::array<const char*, kSecondaryViewRoleCount> kSecondaryViewRoleNames{
	"Free Observer (Orbital Camera)",
	"Comoving Probe (Free-Fall Cockpit)",
	"Asymptotic Global (Quasi-Orthogonal From Infinity)",
	"Equirectangular 360 Panorama"
};

inline constexpr std::array<const char*, kSecondaryViewRoleCount> kSecondaryViewRoleLabels{
	"Observer", "Probe", "Asymptotic", "Panorama"
};

enum class SecondaryTargetMode : uint32_t {
	CentralObject = 0,
	PrimaryCameraPosition = 1,
	PrimaryCameraFocus = 2,
	TrackedBody = 3,
	CustomPoint = 4
};

inline constexpr size_t kSecondaryTargetModeCount = 5;

inline constexpr std::array<const char*, kSecondaryTargetModeCount> kSecondaryTargetModeNames{
	"Central Object (Origin)",
	"Primary Camera Position",
	"Primary Camera Focus Point",
	"Tracked Body",
	"Custom Point"
};

struct SecondaryTargetSettings {
	SecondaryTargetMode mode{SecondaryTargetMode::CentralObject};
	uint32_t body_id{0};
	std::array<double, 3> custom_point{0.0, 0.0, 0.0};
};

namespace SecondaryStateIo {

using Entries = std::unordered_map<std::string, std::string>;

inline void put_real(Entries& entries, const std::string& key, double value) {
	char buffer[40];
	std::snprintf(buffer, sizeof(buffer), "%.17g", value);
	entries[key] = buffer;
}

inline void put_unsigned(Entries& entries, const std::string& key, uint64_t value) {
	entries[key] = std::to_string(value);
}

[[nodiscard]] inline double read_real(const Entries& entries, const std::string& key, double fallback) {
	const auto it = entries.find(key);
	if (it == entries.end()) {
		return fallback;
	}
	const double value = std::strtod(it->second.c_str(), nullptr);
	return std::isfinite(value) ? value : fallback;
}

[[nodiscard]] inline uint64_t read_unsigned(const Entries& entries, const std::string& key, uint64_t fallback) {
	const auto it = entries.find(key);
	if (it == entries.end()) {
		return fallback;
	}
	return static_cast<uint64_t>(std::strtoull(it->second.c_str(), nullptr, 10));
}

inline void write_target(Entries& entries, const std::string& prefix, const SecondaryTargetSettings& target) {
	put_unsigned(entries, prefix + "target_mode", static_cast<uint64_t>(target.mode));
	put_unsigned(entries, prefix + "target_body", target.body_id);
	put_real(entries, prefix + "target_x", target.custom_point[0]);
	put_real(entries, prefix + "target_y", target.custom_point[1]);
	put_real(entries, prefix + "target_z", target.custom_point[2]);
}

inline void read_target(const Entries& entries, const std::string& prefix, SecondaryTargetSettings& target) {
	const uint64_t mode = read_unsigned(entries, prefix + "target_mode", static_cast<uint64_t>(target.mode));
	target.mode = static_cast<SecondaryTargetMode>(mode < kSecondaryTargetModeCount ? mode : 0U);
	target.body_id = static_cast<uint32_t>(read_unsigned(entries, prefix + "target_body", target.body_id));
	target.custom_point[0] = read_real(entries, prefix + "target_x", target.custom_point[0]);
	target.custom_point[1] = read_real(entries, prefix + "target_y", target.custom_point[1]);
	target.custom_point[2] = read_real(entries, prefix + "target_z", target.custom_point[2]);
}

}

}

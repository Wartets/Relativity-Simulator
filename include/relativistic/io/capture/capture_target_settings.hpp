#pragma once

#include "relativistic/io/capture/capture_settings_io.hpp"
#include <algorithm>
#include <cstdint>
#include <string>

namespace Relativistic::IO {

enum class CaptureLayerMode : uint32_t {
	Live = 0,
	NoBodies = 1,
	BodiesOnly = 2
};

enum class CaptureBodyQuality : uint32_t {
	Live = 0,
	High = 1,
	Maximum = 2
};

inline constexpr int32_t kMaxProjectionOverride = 7;
inline constexpr int32_t kMaxTonemappingOverride = 3;

struct CaptureTargetSettings {
	bool use_explicit_resolution{false};
	float resolution_multiplier{1.0f};
	uint32_t explicit_width{1920};
	uint32_t explicit_height{1080};
	uint32_t supersampling{1};
	uint32_t max_ray_steps{8192};
	float step_refinement{2.0f};
	bool lock_aspect_ratio{false};
	float locked_aspect_ratio{1.7777778f};
	CaptureLayerMode layers{CaptureLayerMode::Live};
	CaptureBodyQuality body_quality{CaptureBodyQuality::Live};
	bool hide_accretion_disk{false};
	int32_t projection_override{-1};
	int32_t tonemapping_override{-1};
	float field_of_view_override_deg{0.0f};
	float exposure_offset_ev{0.0f};
	bool apply_post_processing{true};
	float dither_strength{0.0f};

	void write_settings(SettingsWriter& writer, const std::string& prefix) const {
		writer.flag(prefix + "use_explicit_resolution", use_explicit_resolution);
		writer.real(prefix + "resolution_multiplier", resolution_multiplier);
		writer.unsigned_value(prefix + "explicit_width", explicit_width);
		writer.unsigned_value(prefix + "explicit_height", explicit_height);
		writer.unsigned_value(prefix + "supersampling", supersampling);
		writer.unsigned_value(prefix + "max_ray_steps", max_ray_steps);
		writer.real(prefix + "step_refinement", step_refinement);
		writer.flag(prefix + "lock_aspect_ratio", lock_aspect_ratio);
		writer.real(prefix + "locked_aspect_ratio", locked_aspect_ratio);
		writer.enumeration(prefix + "layers", layers);
		writer.enumeration(prefix + "body_quality", body_quality);
		writer.flag(prefix + "hide_accretion_disk", hide_accretion_disk);
		writer.signed_value(prefix + "projection_override", projection_override);
		writer.signed_value(prefix + "tonemapping_override", tonemapping_override);
		writer.real(prefix + "field_of_view_override_deg", field_of_view_override_deg);
		writer.real(prefix + "exposure_offset_ev", exposure_offset_ev);
		writer.flag(prefix + "apply_post_processing", apply_post_processing);
		writer.real(prefix + "dither_strength", dither_strength);
	}

	void read_settings(const SettingsReader& reader, const std::string& prefix) {
		use_explicit_resolution = reader.flag(prefix + "use_explicit_resolution", use_explicit_resolution);
		resolution_multiplier = std::clamp(static_cast<float>(reader.real(prefix + "resolution_multiplier", resolution_multiplier)), 0.05f, 16.0f);
		explicit_width = std::clamp<uint32_t>(reader.unsigned_value(prefix + "explicit_width", explicit_width), 16U, 65535U);
		explicit_height = std::clamp<uint32_t>(reader.unsigned_value(prefix + "explicit_height", explicit_height), 16U, 65535U);
		supersampling = std::clamp<uint32_t>(reader.unsigned_value(prefix + "supersampling", supersampling), 1U, 8U);
		max_ray_steps = std::clamp<uint32_t>(reader.unsigned_value(prefix + "max_ray_steps", max_ray_steps), 64U, 65536U);
		step_refinement = std::clamp(static_cast<float>(reader.real(prefix + "step_refinement", step_refinement)), 1.0f, 32.0f);
		lock_aspect_ratio = reader.flag(prefix + "lock_aspect_ratio", lock_aspect_ratio);
		locked_aspect_ratio = std::clamp(static_cast<float>(reader.real(prefix + "locked_aspect_ratio", locked_aspect_ratio)), 0.05f, 20.0f);
		layers = reader.enumeration(prefix + "layers", layers, CaptureLayerMode::BodiesOnly);
		body_quality = reader.enumeration(prefix + "body_quality", body_quality, CaptureBodyQuality::Maximum);
		hide_accretion_disk = reader.flag(prefix + "hide_accretion_disk", hide_accretion_disk);
		projection_override = std::clamp<int32_t>(reader.signed_value(prefix + "projection_override", projection_override), -1, kMaxProjectionOverride);
		tonemapping_override = std::clamp<int32_t>(reader.signed_value(prefix + "tonemapping_override", tonemapping_override), -1, kMaxTonemappingOverride);
		field_of_view_override_deg = std::clamp(static_cast<float>(reader.real(prefix + "field_of_view_override_deg", field_of_view_override_deg)), 0.0f, 175.0f);
		exposure_offset_ev = std::clamp(static_cast<float>(reader.real(prefix + "exposure_offset_ev", exposure_offset_ev)), -6.0f, 6.0f);
		apply_post_processing = reader.flag(prefix + "apply_post_processing", apply_post_processing);
		dither_strength = std::clamp(static_cast<float>(reader.real(prefix + "dither_strength", dither_strength)), 0.0f, 4.0f);
	}
};

}

#pragma once

#include "relativistic/io/capture_settings_io.hpp"
#include <algorithm>
#include <cstdint>
#include <string>

namespace Relativistic::IO {

struct CaptureTargetSettings {
	bool use_explicit_resolution{false};
	uint32_t explicit_width{1920};
	uint32_t explicit_height{1080};
	uint32_t supersampling{1};
	uint32_t max_ray_steps{8192};
	float step_refinement{2.0f};

	void write_settings(SettingsWriter& writer, const std::string& prefix) const {
		writer.flag(prefix + "use_explicit_resolution", use_explicit_resolution);
		writer.unsigned_value(prefix + "explicit_width", explicit_width);
		writer.unsigned_value(prefix + "explicit_height", explicit_height);
		writer.unsigned_value(prefix + "supersampling", supersampling);
		writer.unsigned_value(prefix + "max_ray_steps", max_ray_steps);
		writer.real(prefix + "step_refinement", step_refinement);
	}

	void read_settings(const SettingsReader& reader, const std::string& prefix) {
		use_explicit_resolution = reader.flag(prefix + "use_explicit_resolution", use_explicit_resolution);
		explicit_width = std::clamp<uint32_t>(reader.unsigned_value(prefix + "explicit_width", explicit_width), 16U, 65535U);
		explicit_height = std::clamp<uint32_t>(reader.unsigned_value(prefix + "explicit_height", explicit_height), 16U, 65535U);
		supersampling = std::clamp<uint32_t>(reader.unsigned_value(prefix + "supersampling", supersampling), 1U, 8U);
		max_ray_steps = std::clamp<uint32_t>(reader.unsigned_value(prefix + "max_ray_steps", max_ray_steps), 64U, 65536U);
		step_refinement = std::clamp(static_cast<float>(reader.real(prefix + "step_refinement", step_refinement)), 1.0f, 32.0f);
	}
};

}

#pragma once

#include "relativistic/render/gpu_types.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>

namespace Relativistic::Render {

enum class LightSourceMode : uint32_t {
	CentralSource = 0,
	CameraHeadlight = 1,
	FixedDirection = 2,
	FixedPoint = 3,
	NearestEmissiveBody = 4,
	SpecificBody = 5,
	Unlit = 6
};

enum class LightAttenuationMode : uint32_t {
	None = 0,
	InverseSquare = 1
};

inline constexpr uint32_t kLightSourceModeCount = 7;
inline constexpr uint32_t kLightAttenuationModeCount = 2;

inline constexpr std::array<const char*, kLightSourceModeCount> kLightSourceModeNames{
	"Central Spacetime Source (Origin)",
	"Camera Headlight",
	"Fixed Direction (Distant Sun)",
	"Fixed Point In Space",
	"Nearest Emissive Body",
	"Specific Body",
	"Unlit (Uniform Full Brightness)"
};

inline constexpr std::array<const char*, kLightAttenuationModeCount> kLightAttenuationModeNames{
	"None (Constant Irradiance)",
	"Inverse Square Law"
};

struct LightSample {
	std::array<double, 3> direction{0.0, 0.0, 1.0};
	double distance{1.0e30};
	float intensity{1.0f};
	std::array<float, 3> color{1.0f, 1.0f, 1.0f};
	bool lit{true};
	bool at_origin{false};
	int source_body_index{-1};
};

class BodyLighting {
public:
	static constexpr double kEmitterThreshold = 1.0e-3;
	static constexpr double kMaxAttenuationGain = 64.0;
	static constexpr double kMaxEmitterGain = 16.0;
	static constexpr double kInfiniteDistance = 1.0e30;

	[[nodiscard]] static bool is_emitter(const GpuBodyData& body) noexcept {
		return body.preset_3d != 8U && body.emission_intensity > kEmitterThreshold;
	}

	[[nodiscard]] static std::array<double, 3> observer_position(const GpuCameraPushConstants& params) noexcept {
		const double r = params.observer_position[1];
		const double sin_theta = std::sin(params.observer_position[2]);
		return {
			r * sin_theta * std::cos(params.observer_position[3]),
			r * sin_theta * std::sin(params.observer_position[3]),
			r * std::cos(params.observer_position[2])
		};
	}

	[[nodiscard]] static double wrapped_diffuse(double n_dot_l, float softness) noexcept {
		const double wrap = std::clamp(static_cast<double>(softness), 0.0, 1.0);
		return std::clamp((n_dot_l + wrap) / (1.0 + wrap), 0.0, 1.0);
	}

	[[nodiscard]] static LightSample resolve(
		const GpuCameraPushConstants& params,
		const std::array<double, 3>& hit,
		std::span<const GpuBodyData> bodies,
		size_t self_index
	) noexcept {
		LightSample sample;
		sample.intensity = params.light_intensity;
		sample.color = {params.light_color_r, params.light_color_g, params.light_color_b};

		const auto mode = static_cast<LightSourceMode>(std::min(params.light_source_mode, kLightSourceModeCount - 1U));
		if (mode == LightSourceMode::Unlit) {
			sample.lit = false;
			return sample;
		}

		const auto place = [&](const std::array<double, 3>& position, bool at_origin) noexcept {
			const double dx = position[0] - hit[0];
			const double dy = position[1] - hit[1];
			const double dz = position[2] - hit[2];
			const double d = std::sqrt(dx * dx + dy * dy + dz * dz);
			sample.distance = d;
			sample.direction = (d > 1e-9) ? std::array<double, 3>{dx / d, dy / d, dz / d} : std::array<double, 3>{0.0, 0.0, 1.0};
			sample.at_origin = at_origin;
		};

		switch (mode) {
			case LightSourceMode::CameraHeadlight:
				place(observer_position(params), false);
				break;
			case LightSourceMode::FixedDirection: {
				const double dx = params.light_direction_x;
				const double dy = params.light_direction_y;
				const double dz = params.light_direction_z;
				const double length = std::sqrt(dx * dx + dy * dy + dz * dz);
				sample.direction = (length > 1e-9) ? std::array<double, 3>{dx / length, dy / length, dz / length} : std::array<double, 3>{0.0, 0.0, 1.0};
				sample.distance = kInfiniteDistance;
				break;
			}
			case LightSourceMode::FixedPoint:
				place({params.light_position_x, params.light_position_y, params.light_position_z}, false);
				break;
			case LightSourceMode::NearestEmissiveBody:
			case LightSourceMode::SpecificBody: {
				int found = -1;
				double best = kInfiniteDistance;
				for (size_t i = 0; i < bodies.size(); ++i) {
					if (i == self_index) continue;
					if (mode == LightSourceMode::SpecificBody) {
						if (bodies[i].body_id == params.light_source_body_id) {
							found = static_cast<int>(i);
							break;
						}
					} else if (is_emitter(bodies[i])) {
						const double dx = bodies[i].position[0] - hit[0];
						const double dy = bodies[i].position[1] - hit[1];
						const double dz = bodies[i].position[2] - hit[2];
						const double d2 = dx * dx + dy * dy + dz * dz;
						if (d2 < best) {
							best = d2;
							found = static_cast<int>(i);
						}
					}
				}
				if (found >= 0) {
					const auto& source = bodies[static_cast<size_t>(found)];
					place({source.position[0], source.position[1], source.position[2]}, false);
					sample.source_body_index = found;
					for (size_t c = 0; c < 3; ++c) {
						sample.color[c] *= static_cast<float>(source.color_primary[c]);
					}
				} else {
					place({0.0, 0.0, 0.0}, true);
				}
				break;
			}
			case LightSourceMode::CentralSource:
			default:
				place({0.0, 0.0, 0.0}, true);
				break;
		}

		if (params.light_attenuation_mode == static_cast<uint32_t>(LightAttenuationMode::InverseSquare) && sample.distance < 0.5 * kInfiniteDistance) {
			const double ratio = static_cast<double>(params.light_reference_distance) / std::max(sample.distance, 1e-6);
			sample.intensity = static_cast<float>(static_cast<double>(params.light_intensity) * std::min(ratio * ratio, kMaxAttenuationGain));
		}
		return sample;
	}

	[[nodiscard]] static std::array<float, 3> emitted_irradiance(
		const GpuCameraPushConstants& params,
		const std::array<double, 3>& hit,
		const std::array<double, 3>& normal,
		std::span<const GpuBodyData> bodies,
		size_t self_index,
		int excluded_index
	) noexcept {
		std::array<float, 3> total{0.0f, 0.0f, 0.0f};
		const double reference = static_cast<double>(params.body_emission_lighting_reference_distance);
		const double gain = static_cast<double>(params.body_emission_lighting_gain);
		for (size_t i = 0; i < bodies.size(); ++i) {
			if (i == self_index || static_cast<int>(i) == excluded_index) continue;
			const auto& emitter = bodies[i];
			if (!is_emitter(emitter)) continue;
			const double dx = emitter.position[0] - hit[0];
			const double dy = emitter.position[1] - hit[1];
			const double dz = emitter.position[2] - hit[2];
			const double d = std::sqrt(dx * dx + dy * dy + dz * dz);
			if (d < 1e-9) continue;
			const double n_dot_l = (normal[0] * dx + normal[1] * dy + normal[2] * dz) / d;
			if (n_dot_l <= 0.0) continue;
			const double effective_distance = std::max(d, std::max(emitter.radius, 1e-6));
			const double ratio = reference / effective_distance;
			const double weight = gain * emitter.emission_intensity * std::min(ratio * ratio, kMaxEmitterGain) * n_dot_l;
			for (size_t c = 0; c < 3; ++c) {
				total[c] += static_cast<float>(weight * emitter.color_primary[c]);
			}
		}
		return total;
	}
};

}

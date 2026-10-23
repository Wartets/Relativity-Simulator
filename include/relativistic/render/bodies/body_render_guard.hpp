#pragma once

#include "relativistic/render/gpu_types.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <vector>

namespace Relativistic::Render {

struct BodyGuardReport {
	size_t inspected{0};
	size_t repaired{0};
	size_t discarded{0};

	[[nodiscard]] bool clean() const noexcept {
		return repaired == 0 && discarded == 0;
	}
};

class BodyRenderGuard {
private:
	static constexpr double kMaxCoordinate = 1.0e15;
	static constexpr double kMaxColor = 64.0;

	struct Repair {
		bool applied{false};

		void real(double& value, double low, double high, double fallback) noexcept {
			const double next = std::isfinite(value) ? std::clamp(value, low, high) : fallback;
			applied = applied || !(next == value);
			value = next;
		}

		void real(float& value, float low, float high, float fallback) noexcept {
			const float next = std::isfinite(value) ? std::clamp(value, low, high) : fallback;
			applied = applied || !(next == value);
			value = next;
		}

		void index(uint32_t& value, uint32_t maximum, uint32_t fallback) noexcept {
			if (value > maximum) {
				value = fallback;
				applied = true;
			}
		}
	};

	static void repair_layer(GpuSurfaceLayer& layer, Repair& repair) noexcept {
		for (float& channel : layer.color) {
			repair.real(channel, 0.0f, 16.0f, 1.0f);
		}
		repair.real(layer.opacity, 0.0f, 1.0f, 0.0f);
		repair.real(layer.scale, 0.05f, 512.0f, 6.0f);
		repair.real(layer.contrast, 0.0f, 64.0f, 1.0f);
		repair.real(layer.threshold, 0.0f, 1.0f, 0.35f);
		repair.real(layer.softness, 0.001f, 1.0f, 0.3f);
		repair.real(layer.mask_width, 0.01f, 1.0f, 0.35f);
		repair.real(layer.rotation_factor, -1000.0f, 1000.0f, 0.0f);
		repair.real(layer.emission, 0.0f, 64.0f, 0.0f);
		repair.index(layer.pattern, 9U, 0U);
		repair.index(layer.blend, 4U, 0U);
		repair.index(layer.mask, 6U, 0U);
		const uint32_t octaves = std::clamp<uint32_t>(layer.octaves, 1U, 6U);
		if (octaves != layer.octaves) {
			layer.octaves = octaves;
			repair.applied = true;
		}
		repair.index(layer.enabled, 1U, 1U);
	}

	static void repair_disk(GpuDiskProfile& disk, Repair& repair) noexcept {
		std::array<float, sizeof(GpuDiskProfile) / sizeof(float)> raw{};
		std::memcpy(raw.data(), &disk, sizeof(disk));
		const bool finite = std::all_of(raw.begin(), raw.end(), [](float value) noexcept { return std::isfinite(value); });
		if (!finite) {
			disk = GpuDiskProfile{};
			repair.applied = true;
		}
		repair.real(disk.inner_radius_scale, 1.0f, 1000.0f, 1.0f);
		repair.real(disk.outer_radius_mass_units, 1.0f, 1.0e6f, 24.0f);
	}

public:
	[[nodiscard]] static bool sanitize(GpuBodyData& body, bool* repaired = nullptr) noexcept {
		const bool placed = std::isfinite(body.position[0]) && std::isfinite(body.position[1]) && std::isfinite(body.position[2])
			&& std::abs(body.position[0]) <= kMaxCoordinate && std::abs(body.position[1]) <= kMaxCoordinate && std::abs(body.position[2]) <= kMaxCoordinate
			&& std::isfinite(body.radius);
		if (!placed) {
			return false;
		}

		Repair repair;
		body.position[3] = 0.0;
		body.velocity[3] = 0.0;
		for (size_t axis = 0; axis < 3; ++axis) {
			repair.real(body.velocity[axis], -kMaxCoordinate, kMaxCoordinate, 0.0);
		}
		repair.real(body.radius, 1.0e-6, kMaxCoordinate, 1.0);
		repair.real(body.mass, 0.0, 1.0e60, 0.0);
		repair.real(body.charge, -1.0e60, 1.0e60, 0.0);
		repair.real(body.temperature, 0.0, 1.0e9, 5778.0);
		repair.real(body.noise_scale, 0.05, 256.0, 4.0);
		repair.real(body.noise_roughness, 0.05, 1.0, 0.5);
		repair.real(body.atmosphere_thickness, 0.0, 2.0, 0.15);
		repair.real(body.specular_roughness, 0.0, 1.0, 0.3);
		repair.real(body.emission_intensity, 0.0, 1.0e4, 0.0);
		repair.real(body.rotation_speed, -1.0e6, 1.0e6, 0.0);
		repair.real(body.oblateness_ratio, 0.1, 5.0, 1.0);
		repair.real(body.spin_parameter, -1.0, 1.0, 0.0);
		repair.real(body.texture_detail_scale, 0.1, 64.0, 1.0);
		repair.real(body.polar_cap_strength, 0.0, 1.0, 0.0);
		repair.real(body.ring_system_enabled, 0.0, 1.0, 0.0);
		repair.real(body.night_side_light_intensity, 0.0, 16.0, 0.0);
		repair.real(body.earth_terminator_softness, 0.0f, 1.0f, 0.45f);
		for (double& channel : body.color_primary) repair.real(channel, 0.0, kMaxColor, 0.0);
		for (double& channel : body.color_secondary) repair.real(channel, 0.0, kMaxColor, 0.0);
		for (double& channel : body.color_tertiary) repair.real(channel, 0.0, kMaxColor, 0.0);
		for (double& channel : body.atmosphere_color) repair.real(channel, 0.0, kMaxColor, 0.0);

		repair.index(body.geometry_model, 5U, 1U);
		repair.index(body.surface_texture_mode, 13U, 0U);
		repair.index(body.atmosphere_mode, 5U, 2U);
		repair.index(body.preset_3d, 9U, 9U);
		repair.index(body.earth_map_variant, 2U, 0U);
		repair.index(body.earth_map_quality, 1U, 0U);

		for (size_t axis = 0; axis < 3; ++axis) {
			if (!std::isfinite(body.spin_axis[axis])) {
				body.spin_axis = {0.0, 0.0, 1.0, 0.0};
				repair.applied = true;
				break;
			}
		}
		const double axis_length = std::sqrt(body.spin_axis[0] * body.spin_axis[0] + body.spin_axis[1] * body.spin_axis[1] + body.spin_axis[2] * body.spin_axis[2]);
		if (axis_length < 1.0e-9) {
			body.spin_axis = {0.0, 0.0, 1.0, 0.0};
			repair.applied = true;
		}

		if (body.surface_layer_count > kMaxSurfaceLayers) {
			body.surface_layer_count = static_cast<uint32_t>(kMaxSurfaceLayers);
			repair.applied = true;
		}
		for (GpuSurfaceLayer& layer : body.surface_layers) {
			repair_layer(layer, repair);
		}
		repair_disk(body.disk, repair);

		if (repaired != nullptr) {
			*repaired = repair.applied;
		}
		return true;
	}

	[[nodiscard]] static std::vector<GpuBodyData> sanitized_copy(std::span<const GpuBodyData> source, BodyGuardReport* report = nullptr) {
		std::vector<GpuBodyData> result;
		result.reserve(source.size());
		BodyGuardReport local;
		for (const GpuBodyData& body : source) {
			++local.inspected;
			GpuBodyData copy = body;
			bool repaired = false;
			if (!sanitize(copy, &repaired)) {
				++local.discarded;
				continue;
			}
			if (repaired) {
				++local.repaired;
			}
			result.push_back(copy);
		}
		if (report != nullptr) {
			*report = local;
		}
		return result;
	}
};

}

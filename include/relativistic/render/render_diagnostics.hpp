#pragma once

#include "relativistic/core/diagnostics/diagnostics_registry.hpp"
#include "relativistic/render/gpu_types.hpp"
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace Relativistic::Render {

enum class DispatchState : uint32_t {
	InFlight = 0,
	Completed = 1,
	Failed = 2
};

class RenderDiagnostics {
public:
	static constexpr size_t kMaximumBodyLines = 24;

	[[nodiscard]] static const char* state_name(DispatchState state) noexcept {
		switch (state) {
			case DispatchState::InFlight: return "in flight";
			case DispatchState::Completed: return "completed";
			case DispatchState::Failed: return "failed";
		}
		return "unknown";
	}

	[[nodiscard]] static std::string describe(const GpuCameraPushConstants& c, std::span<const GpuBodyData> bodies, std::string_view route, DispatchState state, double duration_ms) {
		using Core::Diagnostics::append_format;
		std::string text;
		text.reserve(8192);

		append_format(text, "  State                %s\n  Route                %.*s\n  Duration             %.3f ms\n", state_name(state), static_cast<int>(route.size()), route.data(), duration_ms);
		append_format(text, "  Frame                %ux%u, rows %u+%u, projection %u, interlace %u/%u\n", c.screen_width, c.screen_height, c.dispatch_row_offset, c.dispatch_row_count, c.projection_mode, c.interlace_mode, c.interlace_phase);
		append_format(text, "  Metric               type %u, mass %.9g, spin %.9g, charge %.9g, lambda %.6g\n", c.metric_type, c.metric_mass, c.metric_spin, c.metric_charge, c.cosmological_lambda);
		append_format(text, "  Integration          steps %u, factor %.6g, min %.6g, max %.6g, far-field %.4g, pole guard %.4g, escape %.6g\n", c.max_integration_steps, c.step_size_factor, c.min_step_size, c.max_step_size, c.far_field_step_scale, c.pole_guard_precision_scale, c.escape_radius);
		append_format(text, "  Render flags         0x%08X, space skip radius %.4g, lod threshold %.4g/%u steps\n", c.render_flags, c.space_skip_radius_scale, c.lod_distance_threshold, c.lod_reduced_steps);

		const double sin_theta = std::sin(c.observer_position[2]);
		const std::array<double, 3> eye{
			c.observer_position[1] * sin_theta * std::cos(c.observer_position[3]),
			c.observer_position[1] * sin_theta * std::sin(c.observer_position[3]),
			c.observer_position[1] * std::cos(c.observer_position[2])
		};
		append_format(text, "  Observer             r=%.9g theta=%.9g phi=%.9g, cartesian (%.9g, %.9g, %.9g), fov %.6g rad, exposure %.4g, tonemap %u\n", c.observer_position[1], c.observer_position[2], c.observer_position[3], eye[0], eye[1], eye[2], c.field_of_view_rad, c.camera_exposure, c.tonemapping_mode);
		append_format(text, "  Observer boost       (%.6g, %.6g, %.6g)\n", c.observer_boost[0], c.observer_boost[1], c.observer_boost[2]);
		append_format(text, "  Light                mode %u, attenuation %u, body %u, intensity %.4g, ambient %.4g, terminator %.4g, specular %.4g, position (%.6g, %.6g, %.6g), detail %u, shadow casters %u, emission sources %u, emission lighting %u\n", c.light_source_mode, c.light_attenuation_mode, c.light_source_body_id, static_cast<double>(c.light_intensity), static_cast<double>(c.light_ambient), static_cast<double>(c.light_terminator_softness), static_cast<double>(c.light_specular_scale), static_cast<double>(c.light_position_x), static_cast<double>(c.light_position_y), static_cast<double>(c.light_position_z), c.body_lighting_detail, c.body_shadow_caster_limit, c.body_emission_source_limit, c.body_emission_lighting_enabled);
		append_format(text, "  Sky                  source %u, panorama %u/%u (%ux%u), seed %u\n", c.sky_background_source, c.sky_panorama_id, c.sky_panorama_quality, c.sky_panorama_width, c.sky_panorama_height, c.sky_procedural_seed);
		append_format(text, "  Earth textures       day width %u, night width %u\n", c.earth_day_width, c.earth_night_width);
		append_format(text, "  Extensions           hydro model %u, jet %.0f, dark matter halos %u flags %u, cosmology table %u\n", c.hydro_disk.model, static_cast<double>(c.jet.enabled), c.dark_matter.halo_count, c.dark_matter.flags, c.cosmology.table_size);

		append_format(text, "  Bodies               %llu submitted\n", static_cast<unsigned long long>(bodies.size()));
		const size_t listed = std::min(bodies.size(), kMaximumBodyLines);
		for (size_t i = 0; i < listed; ++i) {
			const GpuBodyData& b = bodies[i];
			const double dx = b.position[0] - eye[0];
			const double dy = b.position[1] - eye[1];
			const double dz = b.position[2] - eye[2];
			const double distance = std::sqrt(dx * dx + dy * dy + dz * dz);
			const double ratio = distance / std::max(b.radius, 1.0e-9);
			append_format(text, "    [%llu] id=%u preset=%u geometry=%u texture=%u earth(variant=%u quality=%u softness=%.3f) pos=(%.9g, %.9g, %.9g) radius=%.9g mass=%.6g oblateness=%.4g layers=%u emission=%.4g camera_distance=%.9g (%.4f radii)%s\n", static_cast<unsigned long long>(i), b.body_id, b.preset_3d, b.geometry_model, b.surface_texture_mode, b.earth_map_variant, b.earth_map_quality, static_cast<double>(b.earth_terminator_softness), b.position[0], b.position[1], b.position[2], b.radius, b.mass, b.oblateness_ratio, b.surface_layer_count, b.emission_intensity, distance, ratio, (ratio < 1.0) ? " CAMERA INSIDE" : "");
		}
		if (bodies.size() > listed) {
			append_format(text, "    ... %llu more bodies\n", static_cast<unsigned long long>(bodies.size() - listed));
		}
		return text;
	}

	static void publish(const GpuCameraPushConstants& constants, std::span<const GpuBodyData> bodies, std::string_view route, DispatchState state, double duration_ms) noexcept {
		try {
			Core::Diagnostics::publish_render_snapshot(describe(constants, bodies, route, state, duration_ms));
		} catch (...) {
		}
	}
};

}

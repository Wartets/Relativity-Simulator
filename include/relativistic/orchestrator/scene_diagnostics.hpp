#pragma once

#include "relativistic/core/diagnostics/diagnostics_registry.hpp"
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <string_view>

namespace Relativistic::Orchestrator {

class SceneDiagnostics {
public:
	static constexpr size_t kMaximumBodyLines = 40;

	template <size_t Capacity>
	[[nodiscard]] static std::string describe(const SimulationOrchestrator<Capacity>& orchestrator) {
		using Core::Diagnostics::append_format;
		std::string text;
		text.reserve(8192);

		const PhysicalParameters& p = orchestrator.parameters();
		const CameraState& cam = orchestrator.camera();
		const auto clock = orchestrator.scheduler().snapshot();
		const auto& interactions = orchestrator.interaction_config();
		const auto& constants = orchestrator.constants_engine();

		append_format(text, "  Metric               %s\n  Integrator           %s\n  Scenario             %s\n  Scenario path        %s\n", orchestrator.active_metric_name().c_str(), orchestrator.active_integrator_name().c_str(), orchestrator.active_scenario_name().c_str(), orchestrator.active_scenario_path().c_str());
		append_format(text, "  Scheduler            tick %llu, time %.9g, rate %.1f Hz, dt %.6g, warp %.6g, paused %s, remaining steps %llu\n", static_cast<unsigned long long>(clock.tick_index), clock.logical_time, clock.tick_rate_hz, clock.tick_dt, clock.warp_factor, clock.is_paused ? "yes" : "no", static_cast<unsigned long long>(clock.remaining_steps));
		append_format(text, "  Primary source       mass %.9g, spin %.9g, charge %.9g, lambda %.6g, throat %.6g, warp velocity %.6g\n", p.mass, p.spin, p.charge, p.cosmological_lambda, p.wormhole_throat, p.warp_velocity);
		append_format(text, "  Camera               position (%.9g, %.9g, %.9g), pitch %.4g, yaw %.4g, roll %.4g, fov %.4g, speed %.4g, mode %u, collision %s\n", cam.position[0], cam.position[1], cam.position[2], cam.pitch, cam.yaw, cam.roll, cam.fov_deg, cam.speed, p.camera_mode, p.camera_collision_enabled ? "on" : "off");
		append_format(text, "  Render settings      scale %.3f, steps %u, preset %u, gpu compute %s, overlays 0x%08X, motion quality %u, dynamic resolution %s, interlace %s\n", p.resolution_scale, p.max_ray_steps, p.performance_preset, p.use_gpu_compute ? "on" : "off", p.visual_overlays_flags, p.motion_quality_mode, p.dynamic_resolution_enabled ? "on" : "off", p.interlace_rendering_enabled ? "on" : "off");
		append_format(text, "  Body rendering       shadows %s, low power %s, bodies only %s, lod %u px, point lod %u px, octaves %u, lighting detail %u, light mode %u\n", p.body_shadows_enabled ? "on" : "off", p.body_render_low_power_mode ? "on" : "off", p.bodies_only_render_mode ? "on" : "off", p.body_render_lod_pixel_threshold, p.body_render_point_pixel_threshold, p.body_noise_octaves, p.body_lighting_detail, p.light_source_mode);
		append_format(text, "  Constants            c %.9g, G %.9g, length scale %.9g m, mass scale %.9g kg, time scale %.9g s\n", constants.sim_speed_of_light(), constants.sim_gravitational_constant(), constants.length_scale(), constants.mass_scale(), constants.time_scale());
		append_format(text, "  Interactions         electricity %s, magnetism %s, collisions %s, thermodynamics %s, fragmentation %s, annihilation %s\n", interactions.electromagnetic.electricity_enabled ? "on" : "off", interactions.electromagnetic.magnetism_enabled ? "on" : "off", interactions.collisions.enabled ? "on" : "off", interactions.thermodynamics.enabled ? "on" : "off", interactions.fragmentation.enabled ? "on" : "off", interactions.annihilation.enabled ? "on" : "off");
		append_format(text, "  Extensions           hydro model %u, dark matter %s (%u halos)\n", static_cast<unsigned>(p.hydro_disk.model), p.dark_matter.enabled ? "on" : "off", static_cast<unsigned>(p.dark_matter.count));
		append_format(text, "  Orchestrator         commands processed %llu, state version %llu\n", static_cast<unsigned long long>(orchestrator.total_commands_processed()), static_cast<unsigned long long>(orchestrator.state_version()));

		std::lock_guard<std::recursive_mutex> lock(orchestrator.nbody_system().bodies_mutex());
		const auto bodies = orchestrator.nbody_system().bodies();
		append_format(text, "  Bodies               %llu in catalog\n", static_cast<unsigned long long>(bodies.size()));
		const size_t listed = std::min(bodies.size(), kMaximumBodyLines);
		for (size_t i = 0; i < listed; ++i) {
			const auto& b = bodies[i];
			const std::string_view name = b.has_name() ? b.name_view() : std::string_view("-");
			append_format(text, "    [%llu] id=%u name=%.*s enabled=%s source=%s mass=%.9g radius=%.9g pos=(%.9g, %.9g, %.9g) speed=%.6g preset=%u texture=%u earth(variant=%u quality=%u) integrity=%.3f temperature=%.1f\n", static_cast<unsigned long long>(i), b.id, static_cast<int>(name.size()), name.data(), b.enabled ? "yes" : "no", b.is_spacetime_source ? "yes" : "no", b.mass, b.radius, b.position[0], b.position[1], b.position[2], b.speed(), static_cast<unsigned>(b.preset_3d), static_cast<unsigned>(b.surface_texture_mode), static_cast<unsigned>(b.earth_map_variant), static_cast<unsigned>(b.earth_map_quality), b.integrity, b.temperature);
		}
		if (bodies.size() > listed) {
			append_format(text, "    ... %llu more bodies\n", static_cast<unsigned long long>(bodies.size() - listed));
		}
		return text;
	}

	template <size_t Capacity>
	static void publish(const SimulationOrchestrator<Capacity>& orchestrator) {
		Core::Diagnostics::publish_scene_snapshot(describe(orchestrator));
	}
};

}

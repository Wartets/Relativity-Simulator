#pragma once

#include "relativistic/dynamics/pn/pn_body.hpp"
#include "relativistic/orchestrator/command.hpp"
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/render/gpu_types.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <numbers>
#include <string>
#include <vector>

namespace Relativistic::Testing {

class DeterministicRandom {
private:
	uint64_t state_;

public:
	explicit constexpr DeterministicRandom(uint64_t seed) noexcept : state_(seed) {}

	[[nodiscard]] constexpr uint64_t next() noexcept {
		state_ += 0x9E3779B97F4A7C15ULL;
		uint64_t z = state_;
		z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
		z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
		return z ^ (z >> 31);
	}

	[[nodiscard]] constexpr double uniform() noexcept {
		return static_cast<double>(next() >> 11) * (1.0 / 9007199254740992.0);
	}

	[[nodiscard]] constexpr double range(double low, double high) noexcept {
		return low + (high - low) * uniform();
	}
};

struct MetricScenario {
	const char* label;
	const char* canonical_name;
	double mass;
	double spin;
	double charge;
	double lambda;
	bool supports_secondary_sources;
	bool expects_horizon_pixels;
};

inline constexpr std::array<MetricScenario, 16> kMetricScenarios{{
	{"FlatMinkowski", "FlatMinkowski", 1.0, 0.0, 0.0, 0.0, true, false},
	{"Schwarzschild", "Schwarzschild", 1.0, 0.0, 0.0, 0.0, true, true},
	{"Kerr", "Kerr", 1.0, 0.9, 0.0, 0.0, true, true},
	{"ReissnerNordstrom", "ReissnerNordstrom", 1.0, 0.0, 0.6, 0.0, true, true},
	{"KerrNewman", "KerrNewman", 1.0, 0.7, 0.4, 0.0, true, true},
	{"SchwarzschildDeSitter", "SchwarzschildDeSitter", 1.0, 0.0, 0.0, 1.0e-5, true, true},
	{"FLRW", "FLRW", 1.0, 0.0, 0.0, 1.0e-3, true, false},
	{"MorrisThorne", "MorrisThorne", 1.0, 0.0, 0.0, 0.0, false, false},
	{"Alcubierre", "Alcubierre", 1.0, 0.0, 0.0, 0.0, false, false},
	{"LemaitreTolmanBondi", "LemaitreTolmanBondi", 1.0, 0.0, 0.0, 0.0, true, false},
	{"QuintessenceCosmology", "QuintessenceCosmology", 1.0, 0.0, 0.0, 1.0e-3, true, false},
	{"ChaplyginCosmology", "ChaplyginCosmology", 1.0, 0.0, 0.0, 1.0e-3, true, false},
	{"EvaporatingBlackHole", "EvaporatingBlackHole", 1.0, 0.0, 0.0, 0.0, true, false},
	{"EddingtonFinkelstein", "EddingtonFinkelstein", 1.0, 0.0, 0.0, 0.0, true, true},
	{"PainleveGullstrand", "PainleveGullstrand", 1.0, 0.0, 0.0, 0.0, true, true},
	{"TeVeS", "TeVeS", 1.0, 0.0, 0.0, 0.0, true, false}
}};

[[nodiscard]] inline const MetricScenario* find_metric_scenario(std::string_view label) noexcept {
	for (const auto& scenario : kMetricScenarios) {
		if (label == scenario.label) {
			return &scenario;
		}
	}
	return nullptr;
}

struct ScenarioSpec {
	const MetricScenario* metric{nullptr};
	uint32_t body_count{0};
	uint32_t black_hole_count{0};
	uint32_t preset{1};
	bool space_skipping{false};
	uint32_t base_width{256};
	uint32_t max_steps_cap{0};
	uint32_t projection_mode{0};
	uint64_t seed{0xC0FFEE1234ULL};
};

struct PreparedFrame {
	std::unique_ptr<Orchestrator::SimulationOrchestrator<1024>> orchestrator{};
	Render::GpuCameraPushConstants constants{};
	std::vector<Render::GpuBodyData> bodies{};
	uint32_t width{0};
	uint32_t height{0};
	uint32_t resolved_max_steps{0};
	double resolution_scale{1.0};
};

[[nodiscard]] inline Dynamics::PostNewtonianBody make_celestial_body(uint32_t index, DeterministicRandom& random) {
	const double radial = random.range(8.0, 70.0);
	const double polar = std::acos(random.range(-1.0, 1.0));
	const double azimuth = random.range(0.0, 2.0 * std::numbers::pi_v<double>);
	const std::array<double, 3> position{
		radial * std::sin(polar) * std::cos(azimuth),
		radial * std::sin(polar) * std::sin(azimuth),
		radial * std::cos(polar)
	};
	Dynamics::PostNewtonianBody body(index + 1U, random.range(1.0e-5, 1.0e-3), random.range(0.3, 1.6), position, {0.0, 0.0, 0.0});
	body.color = {static_cast<float>(random.range(0.2, 1.0)), static_cast<float>(random.range(0.2, 1.0)), static_cast<float>(random.range(0.2, 1.0)), 1.0f};
	body.color_secondary = {static_cast<float>(random.range(0.0, 0.8)), static_cast<float>(random.range(0.0, 0.8)), static_cast<float>(random.range(0.0, 0.8)), 1.0f};
	body.geometry_model = static_cast<Dynamics::Body3DGeometryModel>(index % 4U);
	body.surface_texture_mode = static_cast<Dynamics::Body3DSurfaceTextureMode>(index % 13U);
	body.atmosphere_mode = static_cast<Dynamics::Body3DAtmosphereMode>(index % 4U);
	body.preset_3d = static_cast<Dynamics::Body3DPreset>(index % 6U);
	body.emission_intensity = (body.preset_3d == Dynamics::Body3DPreset::Star) ? 1.5f : 0.0f;
	body.j2 = (body.geometry_model == Dynamics::Body3DGeometryModel::OblateSpheroid) ? 0.002 : 0.0;
	body.rotation_speed_3d = static_cast<float>(random.range(0.02, 0.4));
	return body;
}

[[nodiscard]] inline Dynamics::PostNewtonianBody make_secondary_black_hole(uint32_t index, uint32_t total, DeterministicRandom& random) {
	const double angle = 2.0 * std::numbers::pi_v<double> * (static_cast<double>(index) + random.uniform() * 0.3) / static_cast<double>(std::max(total, 1U));
	const double radial = random.range(20.0, 60.0);
	const std::array<double, 3> position{radial * std::cos(angle), radial * std::sin(angle), random.range(-6.0, 6.0)};
	Dynamics::PostNewtonianBody body(100000U + index, random.range(0.3, 0.9), 1.0, position, {0.0, 0.0, 0.0});
	body.is_spacetime_source = true;
	body.preset_3d = Dynamics::Body3DPreset::BlackHole;
	body.set_spin_state(random.range(0.0, 0.9), {0.0, 0.0, 1.0});
	body.enforce_spacetime_source_invariants();
	return body;
}

[[nodiscard]] inline PreparedFrame prepare_frame(const ScenarioSpec& spec) {
	constexpr double kObserverRadius = 30.0;
	constexpr double kObserverPolar = 1.2;
	constexpr double kFieldOfViewDeg = 50.0;
	constexpr double kRadiansToDegrees = 180.0 / std::numbers::pi_v<double>;

	PreparedFrame frame;
	frame.orchestrator = std::make_unique<Orchestrator::SimulationOrchestrator<1024>>();
	auto& orchestrator = *frame.orchestrator;

	orchestrator.set_active_metric_name(spec.metric->canonical_name);
	static_cast<void>(orchestrator.enqueue_command(Orchestrator::Command::make_set_performance_preset(spec.preset)));
	orchestrator.process_incoming_commands();

	auto& params = orchestrator.parameters();
	params.mass = spec.metric->mass;
	params.spin = spec.metric->spin * spec.metric->mass;
	params.charge = spec.metric->charge;
	params.cosmological_lambda = spec.metric->lambda;
	params.space_skipping_enabled = spec.space_skipping;
	params.projection_mode = spec.projection_mode;
	params.camera_fov_deg = kFieldOfViewDeg;

	auto& camera = orchestrator.camera();
	camera.position = {
		kObserverRadius * std::sin(kObserverPolar),
		0.0,
		kObserverRadius * std::cos(kObserverPolar)
	};
	const double distance = std::sqrt(camera.position[0] * camera.position[0] + camera.position[1] * camera.position[1] + camera.position[2] * camera.position[2]);
	camera.yaw = std::atan2(-camera.position[1], -camera.position[0]) * kRadiansToDegrees;
	camera.pitch = std::asin(std::clamp(-camera.position[2] / distance, -1.0, 1.0)) * kRadiansToDegrees;
	camera.roll = 0.0;
	camera.fov_deg = kFieldOfViewDeg;
	camera.velocity = {0.0, 0.0, 0.0};
	camera.synchronize_spherical();
	camera.orbit_distance = camera.radius;

	frame.resolution_scale = params.resolution_scale;
	frame.width = std::clamp(static_cast<uint32_t>(std::lround(static_cast<double>(spec.base_width) * frame.resolution_scale)), 32U, 7680U);
	frame.height = std::clamp(static_cast<uint32_t>(std::lround(static_cast<double>(frame.width) * 9.0 / 16.0)), 18U, 4320U);

	DeterministicRandom random(spec.seed ^ (static_cast<uint64_t>(spec.body_count) << 20U) ^ (static_cast<uint64_t>(spec.black_hole_count) << 40U));
	const bool secondary_allowed = spec.metric->supports_secondary_sources;
	frame.bodies.reserve(static_cast<size_t>(spec.body_count) + spec.black_hole_count);
	for (uint32_t i = 0; i < spec.body_count; ++i) {
		frame.bodies.push_back(orchestrator.make_gpu_body_data(make_celestial_body(i, random)));
	}
	if (secondary_allowed) {
		for (uint32_t i = 0; i < spec.black_hole_count; ++i) {
			frame.bodies.push_back(orchestrator.make_gpu_body_data(make_secondary_black_hole(i, spec.black_hole_count, random)));
		}
	}

	frame.constants = orchestrator.build_gpu_push_constants(frame.width, frame.height, 1.0);
	const auto spherical = camera.spherical_coordinates();
	const auto basis = camera.orientation_basis();
	constexpr double kDegreesToRadians = std::numbers::pi_v<double> / 180.0;
	frame.constants.observer_position = {0.0, spherical[0], spherical[1], spherical[2]};
	frame.constants.tetrad_e0 = {1.0, 0.0, 0.0, 0.0};
	frame.constants.tetrad_e1 = {0.0, basis.forward[0], basis.forward[1], basis.forward[2]};
	frame.constants.tetrad_e2 = {0.0, basis.right[0], basis.right[1], basis.right[2]};
	frame.constants.tetrad_e3 = {0.0, basis.up[0], basis.up[1], basis.up[2]};
	frame.constants.sky_rotation_rad = params.sky_rotation_deg * kDegreesToRadians;
	frame.constants.sky_hue_shift_rad = params.sky_hue_shift_deg * kDegreesToRadians;
	frame.constants.horizon_radius = 2.0 * params.mass;
	const double configured_distance = (params.render_distance_scale > 0.0) ? (params.render_distance_scale * params.mass) : 1.0e7;
	frame.constants.escape_radius = std::max(configured_distance, spherical[0] * 2.0);
	frame.constants.body_count = static_cast<uint32_t>(frame.bodies.size());
	frame.resolved_max_steps = (spec.max_steps_cap > 0U) ? std::min(params.max_ray_steps, spec.max_steps_cap) : params.max_ray_steps;
	frame.constants.max_integration_steps = frame.resolved_max_steps;
	return frame;
}

}

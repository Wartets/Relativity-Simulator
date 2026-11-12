#pragma once

#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/dynamics/pn/pn_body.hpp"
#include "relativistic/dynamics/pn/pn_nbody_system.hpp"
#include "relativistic/dynamics/body_surface_layers.hpp"
#include "relativistic/ui/body_manager/body_manager_types.hpp"
#include "relativistic/ui/body_manager/body_preset_library.hpp"
#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <mutex>
#include <numbers>
#include <optional>
#include <random>
#include <string>
#include <string_view>

namespace Relativistic::UI {

class BodyManagerContext {
public:
	static constexpr int kCentralSourceId = -2;
	static constexpr int kNoSelection = -1;

private:
	Orchestrator::SimulationOrchestrator<1024>& orchestrator_;
	int selected_object_id_{kNoSelection};
	int selected_source_id_{kCentralSourceId};
	int tracked_body_id_{kNoSelection};
	bool tracking_enabled_{false};
	std::optional<BodyManagerTab> requested_tab_{};
	std::mt19937_64 rng_{std::random_device{}()};

	template <typename Engine>
	[[nodiscard]] static double engine_speed_scale(const Engine& engine) noexcept {
		if constexpr (requires { engine.speed_scale(); }) {
			return static_cast<double>(engine.speed_scale());
		} else if constexpr (requires { engine.velocity_scale(); }) {
			return static_cast<double>(engine.velocity_scale());
		} else if constexpr (requires { engine.time_scale(); }) {
			const double ts = static_cast<double>(engine.time_scale());
			return (ts > 0.0) ? (static_cast<double>(engine.length_scale()) / ts) : 299792458.0;
		} else if constexpr (requires { engine.speed_of_light(); }) {
			return static_cast<double>(engine.speed_of_light());
		} else if constexpr (requires { engine.c(); }) {
			return static_cast<double>(engine.c());
		} else {
			return 299792458.0;
		}
	}

public:
	explicit BodyManagerContext(Orchestrator::SimulationOrchestrator<1024>& orchestrator) noexcept
		: orchestrator_(orchestrator) {}

	[[nodiscard]] Orchestrator::SimulationOrchestrator<1024>& orchestrator() const noexcept {
		return orchestrator_;
	}

	[[nodiscard]] double mass_scale() const noexcept {
		return std::max(orchestrator_.constants_engine().mass_scale(), 1e-300);
	}

	[[nodiscard]] double length_scale() const noexcept {
		return std::max(orchestrator_.constants_engine().length_scale(), 1e-300);
	}

	[[nodiscard]] double speed_scale() const noexcept {
		return engine_speed_scale(orchestrator_.constants_engine());
	}

	[[nodiscard]] int selected_object_id() const noexcept {
		return selected_object_id_;
	}

	[[nodiscard]] int selected_source_id() const noexcept {
		return selected_source_id_;
	}

	void select_object(int id) noexcept {
		selected_object_id_ = id;
	}

	void select_source(int id) noexcept {
		selected_source_id_ = id;
	}

	void select_and_open_source(int id) noexcept {
		selected_source_id_ = id;
		requested_tab_ = BodyManagerTab::SourceCatalog;
	}

	void select_and_open_object(int id) noexcept {
		selected_object_id_ = id;
		requested_tab_ = BodyManagerTab::ObjectCatalog;
	}

	void request_tab(BodyManagerTab tab) noexcept {
		requested_tab_ = tab;
	}

	[[nodiscard]] std::optional<BodyManagerTab> take_requested_tab() noexcept {
		const std::optional<BodyManagerTab> tab = requested_tab_;
		requested_tab_.reset();
		return tab;
	}

	[[nodiscard]] bool is_tracking(int id) const noexcept {
		return tracking_enabled_ && tracked_body_id_ == id;
	}

	void set_tracking(int id, bool enabled) noexcept {
		tracking_enabled_ = enabled;
		tracked_body_id_ = enabled ? id : kNoSelection;
	}

	void toggle_tracking(int id) noexcept {
		set_tracking(id, !is_tracking(id));
	}

	void look_at(const std::array<double, 3>& target) noexcept {
		auto& camera = orchestrator_.camera();
		const double dx = target[0] - camera.position[0];
		const double dy = target[1] - camera.position[1];
		const double dz = target[2] - camera.position[2];
		const double distance = std::sqrt(dx * dx + dy * dy + dz * dz);
		if (distance <= 1e-9) return;
		camera.target = target;
		camera.yaw = std::atan2(dy, dx) * (180.0 / std::numbers::pi);
		camera.pitch = std::asin(std::clamp(dz / distance, -1.0, 1.0)) * (180.0 / std::numbers::pi);
		camera.roll = 0.0;
	}

	void update_tracking() noexcept {
		if (!tracking_enabled_) return;
		auto& sys = orchestrator_.nbody_system();
		std::lock_guard<std::recursive_mutex> tracking_lock(sys.bodies_mutex());
		if (tracked_body_id_ == kCentralSourceId) {
			look_at({0.0, 0.0, 0.0});
			return;
		}
		for (const auto& body : sys.bodies()) {
			if (body.id == static_cast<uint32_t>(tracked_body_id_) && body.enabled) {
				look_at(body.position);
				return;
			}
		}
		tracking_enabled_ = false;
		tracked_body_id_ = kNoSelection;
	}

	[[nodiscard]] double random_real(double min_val, double max_val) noexcept {
		std::uniform_real_distribution<double> dist(min_val, std::max(min_val, max_val));
		return dist(rng_);
	}

	[[nodiscard]] int random_int(int min_val, int max_val) noexcept {
		std::uniform_int_distribution<int> dist(min_val, max_val);
		return dist(rng_);
	}

	[[nodiscard]] double sample_log_uniform(double min_val, double max_val) noexcept {
		return std::pow(10.0, random_real(std::log10(min_val), std::log10(max_val)));
	}

	[[nodiscard]] double sample_signed_uniform(double min_abs, double max_abs) noexcept {
		return (random_int(0, 1) == 0 ? -1.0 : 1.0) * random_real(min_abs, max_abs);
	}

	[[nodiscard]] static std::string display_name(const Dynamics::PostNewtonianBody& body) {
		if (body.has_name()) {
			return std::string(body.name_view());
		}
		return "Body #" + std::to_string(body.id);
	}

	[[nodiscard]] static double distance_from_center(const Dynamics::PostNewtonianBody& body) noexcept {
		return std::sqrt(body.position[0] * body.position[0] + body.position[1] * body.position[1] + body.position[2] * body.position[2]);
	}

	[[nodiscard]] Dynamics::PostNewtonianBody* find_body(uint32_t id) const noexcept {
		auto& sys = orchestrator_.nbody_system();
		for (auto& body : sys.bodies()) {
			if (body.id == id) {
				return &body;
			}
		}
		return nullptr;
	}

	void count_bodies(size_t& objects, size_t& enabled_objects, size_t& sources, size_t& enabled_sources) const noexcept {
		objects = 0;
		enabled_objects = 0;
		sources = 0;
		enabled_sources = 0;
		auto& sys = orchestrator_.nbody_system();
		std::lock_guard<std::recursive_mutex> lock(sys.bodies_mutex());
		for (const auto& body : sys.bodies()) {
			if (body.is_spacetime_source) {
				++sources;
				if (body.enabled) ++enabled_sources;
			} else {
				++objects;
				if (body.enabled) ++enabled_objects;
			}
		}
	}

	[[nodiscard]] std::string unique_name(std::string_view base) const {
		std::string original = base.empty() ? "Body" : std::string(base);
		std::lock_guard<std::recursive_mutex> naming_lock(orchestrator_.nbody_system().bodies_mutex());
		const auto& bodies = orchestrator_.nbody_system().bodies();
		auto exists = [&](std::string_view name) {
			return std::any_of(bodies.begin(), bodies.end(), [&](const auto& body) { return body.name_view() == name; });
		};

		std::string stem = original;
		uint32_t start_suffix = 2;
		const size_t space_pos = original.find_last_of(' ');
		if (space_pos != std::string::npos && space_pos + 1 < original.size()) {
			const std::string_view tail = std::string_view(original).substr(space_pos + 1);
			if (!tail.empty() && std::all_of(tail.begin(), tail.end(), [](unsigned char c) { return std::isdigit(c) != 0; })) {
				const uint32_t parsed = static_cast<uint32_t>(std::strtoul(std::string(tail).c_str(), nullptr, 10));
				if (parsed > 0) {
					stem = original.substr(0, space_pos);
					start_suffix = parsed + 1;
				}
			}
		}

		if (!exists(original)) return original;
		for (uint32_t suffix = start_suffix; suffix < 1000000; ++suffix) {
			const std::string numbered = stem + " " + std::to_string(suffix);
			if (!exists(numbered)) return numbered;
		}
		return original;
	}

	[[nodiscard]] std::array<double, 3> compute_circular_orbit_velocity(const std::array<double, 3>& pos, double central_mass) const noexcept {
		const double r2 = pos[0] * pos[0] + pos[1] * pos[1] + pos[2] * pos[2];
		const double r = std::sqrt(std::max(r2, 1e-12));
		const double speed = std::sqrt(std::max(orchestrator_.physical_gravitational_constant() * central_mass, 0.0) / r);
		const std::array<double, 3> up{0.0, 0.0, 1.0};
		std::array<double, 3> tangent{
			up[1] * pos[2] - up[2] * pos[1],
			up[2] * pos[0] - up[0] * pos[2],
			up[0] * pos[1] - up[1] * pos[0]
		};
		const double t_len = std::sqrt(tangent[0] * tangent[0] + tangent[1] * tangent[1] + tangent[2] * tangent[2]);
		if (t_len < 1e-9) {
			return {0.0, speed, 0.0};
		}
		return {tangent[0] / t_len * speed, tangent[1] / t_len * speed, tangent[2] / t_len * speed};
	}

	[[nodiscard]] double primary_horizon_radius() const noexcept {
		const auto& p = orchestrator_.parameters();
		if (p.mass <= 0.0) return 0.0;
		const double a = std::clamp(p.spin, -0.999 * p.mass, 0.999 * p.mass);
		return p.mass + std::sqrt(std::max(p.mass * p.mass - a * a, 0.0));
	}

	[[nodiscard]] std::string source_overlap_warning(const Dynamics::PostNewtonianBody& b) const {
		const double horizon = b.kerr_outer_horizon_radius();
		const double primary_horizon = primary_horizon_radius();
		if (primary_horizon > 0.0 && distance_from_center(b) < primary_horizon + horizon) {
			return "This horizon overlaps the primary source horizon; overlapping sources merge when the simulation runs, and new sources are moved to a safe separation when spawned.";
		}
		auto& sys = orchestrator_.nbody_system();
		{
			std::lock_guard<std::recursive_mutex> lock(sys.bodies_mutex());
			for (const auto& other : sys.bodies()) {
				if (!other.enabled || !other.is_spacetime_source || other.id == b.id) continue;
				const double dx = b.position[0] - other.position[0];
				const double dy = b.position[1] - other.position[1];
				const double dz = b.position[2] - other.position[2];
				if (std::sqrt(dx * dx + dy * dy + dz * dz) < horizon + other.kerr_outer_horizon_radius()) {
					return "This horizon overlaps another independent black hole; overlapping sources merge when the simulation runs, and new sources are moved to a safe separation when spawned.";
				}
			}
		}
		const auto& camera = orchestrator_.camera();
		const double cx = b.position[0] - camera.position[0];
		const double cy = b.position[1] - camera.position[1];
		const double cz = b.position[2] - camera.position[2];
		if (std::sqrt(cx * cx + cy * cy + cz * cz) < horizon) {
			return "The observer is located inside this event horizon.";
		}
		return {};
	}

	void resolve_source_placement(Dynamics::PostNewtonianBody& body) const noexcept {
		constexpr double separation_margin = 1.25;
		const double horizon = body.kerr_outer_horizon_radius();
		const double primary_horizon = primary_horizon_radius();
		auto& sys = orchestrator_.nbody_system();
		std::lock_guard<std::recursive_mutex> lock(sys.bodies_mutex());
		const auto separate_from = [&body](const std::array<double, 3>& center, double required_distance) noexcept -> bool {
			const std::array<double, 3> offset{body.position[0] - center[0], body.position[1] - center[1], body.position[2] - center[2]};
			const double distance = std::sqrt(offset[0] * offset[0] + offset[1] * offset[1] + offset[2] * offset[2]);
			if (distance >= required_distance) return false;
			const std::array<double, 3> direction = (distance > 1e-9)
				? std::array<double, 3>{offset[0] / distance, offset[1] / distance, offset[2] / distance}
				: std::array<double, 3>{1.0, 0.0, 0.0};
			body.position = {center[0] + direction[0] * required_distance, center[1] + direction[1] * required_distance, center[2] + direction[2] * required_distance};
			return true;
		};
		for (int pass = 0; pass < 16; ++pass) {
			bool moved = false;
			if (primary_horizon > 0.0) {
				moved = separate_from({0.0, 0.0, 0.0}, (primary_horizon + horizon) * separation_margin);
			}
			for (const auto& other : sys.bodies()) {
				if (!other.enabled || !other.is_spacetime_source || other.id == body.id) continue;
				moved = separate_from(other.position, (other.kerr_outer_horizon_radius() + horizon) * separation_margin) || moved;
			}
			if (!moved) break;
		}
	}

	[[nodiscard]] uint32_t spawn_body(Dynamics::PostNewtonianBody body, const Dynamics::BodySurfaceLayerSet& layers) noexcept {
		auto& sys = orchestrator_.nbody_system();
		body.id = 0;
		body.acceleration = {0.0, 0.0, 0.0};
		body.set_name(unique_name(body.has_name() ? body.name_view() : std::string_view("Body")));
		if (body.is_spacetime_source) {
			body.enforce_spacetime_source_invariants();
			resolve_source_placement(body);
		}
		const uint32_t id = sys.add_body(std::move(body));
		orchestrator_.surface_layers().set(id, layers);
		sys.update_accelerations();
		orchestrator_.notify_state_changed();
		return id;
	}

	void remove_body_by_id(uint32_t id) noexcept {
		auto& sys = orchestrator_.nbody_system();
		orchestrator_.surface_layers().erase(id);
		if (sys.remove_body(id)) {
			sys.update_accelerations();
		}
		if (tracked_body_id_ == static_cast<int>(id)) {
			tracking_enabled_ = false;
			tracked_body_id_ = kNoSelection;
		}
		if (selected_object_id_ == static_cast<int>(id)) {
			selected_object_id_ = kNoSelection;
		}
		if (selected_source_id_ == static_cast<int>(id)) {
			selected_source_id_ = kNoSelection;
		}
		orchestrator_.notify_state_changed();
	}

	void clear_all_bodies() noexcept {
		orchestrator_.surface_layers().clear();
		orchestrator_.nbody_system().clear_bodies();
		selected_object_id_ = kNoSelection;
		selected_source_id_ = kNoSelection;
		if (tracked_body_id_ != kCentralSourceId) {
			tracking_enabled_ = false;
			tracked_body_id_ = kNoSelection;
		}
		orchestrator_.notify_state_changed();
	}

	void spawn_companion_source() noexcept {
		const double primary_mass = std::max(orchestrator_.parameters().mass, 1e-6);
		const double companion_mass = primary_mass * 0.5;
		const double orbit_radius = std::max(primary_mass * 40.0, 20.0);
		const double theta = random_real(std::numbers::pi * 0.35, std::numbers::pi * 0.65);
		const double phi = random_real(0.0, 2.0 * std::numbers::pi);
		const std::array<double, 3> position{
			orbit_radius * std::sin(theta) * std::cos(phi),
			orbit_radius * std::sin(theta) * std::sin(phi),
			orbit_radius * std::cos(theta)
		};
		const auto velocity = compute_circular_orbit_velocity(position, primary_mass + companion_mass);

		Dynamics::PostNewtonianBody body(
			0, companion_mass, companion_mass * 2.0,
			position, velocity,
			{0.0, 0.0, companion_mass * 0.3}
		);
		body.set_name("Companion Black Hole");
		body.is_spacetime_source = true;
		apply_body_appearance_preset(body, Dynamics::Body3DPreset::BlackHole);
		const uint32_t id = spawn_body(body, Dynamics::BodySurfaceLayerSet{});
		selected_source_id_ = static_cast<int>(id);
	}

	void populate_reference_system() noexcept {
		auto& sys = orchestrator_.nbody_system();
		orchestrator_.surface_layers().clear();
		sys.clear_bodies();

		const double central_mass = std::max(orchestrator_.parameters().mass, 1.0);
		const auto spawn_planet = [&](std::string_view name, Dynamics::Body3DPreset preset, double mass_ratio, double size_fraction, double orbit_in_central_masses) noexcept {
			const double mass = central_mass * mass_ratio;
			const double orbit_radius = central_mass * orbit_in_central_masses;
			const double radius = std::max(orbit_radius * size_fraction, 4.0 * mass);
			const std::array<double, 3> position{orbit_radius, 0.0, 0.0};
			Dynamics::PostNewtonianBody planet(0, mass, radius, position, compute_circular_orbit_velocity(position, central_mass), {0.0, 0.0, 0.0});
			apply_body_appearance_preset(planet, preset);
			apply_body_material_preset(planet, preset);
			planet.set_name(name);
			static_cast<void>(spawn_body(planet, Dynamics::BodySurfaceLayerSet{}));
		};
		spawn_planet("Inner Planet", Dynamics::Body3DPreset::TerrestrialPlanet, 3e-7, 0.006, 14.0);
		spawn_planet("Outer Planet", Dynamics::Body3DPreset::GasGiant, 4e-5, 0.02, 32.0);
		selected_object_id_ = kNoSelection;
		selected_source_id_ = kNoSelection;
	}
};

}

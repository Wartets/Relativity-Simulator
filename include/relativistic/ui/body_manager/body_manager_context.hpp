#pragma once

#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/dynamics/pn/pn_body.hpp"
#include "relativistic/dynamics/pn/pn_nbody_system.hpp"
#include "relativistic/dynamics/body_surface_layers.hpp"
#include "relativistic/ui/body_manager/body_manager_types.hpp"
#include "relativistic/ui/body_manager/body_preset_library.hpp"
#include "relativistic/units/unit_system.hpp"
#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <mutex>
#include <numbers>
#include <optional>
#include <random>
#include <string>
#include <string_view>

namespace Relativistic::UI {

struct KeplerianOrbitElements {
	bool valid{false};
	double semi_major_axis{0.0};
	double eccentricity{0.0};
	double inclination_deg{0.0};
	double period_seconds{0.0};
	double periapsis{0.0};
	double apoapsis{0.0};
	double specific_energy{0.0};
	double angular_momentum{0.0};
	bool is_bound{false};
	bool is_prograde{true};
	const char* type_name{"Unknown"};
};

struct TrajectoryHazardDiagnostic {
	bool has_hazard{false};
	bool plunge_hazard{false};
	bool roche_hazard{false};
	double periapsis{0.0};
	double horizon_radius{0.0};
	double roche_limit{0.0};
};

struct CollisionOverlapDiagnostic {
	bool has_collision{false};
	bool inside_primary_horizon{false};
	bool inside_source_horizon{false};
	bool overlaps_body{false};
	uint32_t colliding_body_id{0};
	std::string colliding_body_name{};
	double clearance_needed{0.0};
};

struct BinarySourceConfiguration {
	bool valid{false};
	double primary_mass{0.0};
	double secondary_mass{0.0};
	double mass_ratio{0.0};
	double symmetric_mass_ratio{0.0};
	double separation_distance{0.0};
	double separation_rg{0.0};
	double spin_orbit_misalignment_deg{0.0};
	double mutual_spin_angle_deg{0.0};
	double peters_coalescence_time_seconds{0.0};
};

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

	[[nodiscard]] bool tracking_active() const noexcept {
		return tracking_enabled_ && tracked_body_id_ != kNoSelection;
	}

	[[nodiscard]] int tracked_id() const noexcept {
		return tracking_enabled_ ? tracked_body_id_ : kNoSelection;
	}

	void stop_tracking() noexcept {
		tracking_enabled_ = false;
		tracked_body_id_ = kNoSelection;
	}

	[[nodiscard]] std::string tracked_target_name() const {
		if (!tracking_enabled_ || tracked_body_id_ == kNoSelection) {
			return {};
		}
		if (tracked_body_id_ == kCentralSourceId) {
			return "Central Spacetime Source";
		}
		auto& sys = orchestrator_.nbody_system();
		std::lock_guard<std::recursive_mutex> lock(sys.bodies_mutex());
		const auto* b = find_body(static_cast<uint32_t>(tracked_body_id_));
		if (b != nullptr) {
			return display_name(*b);
		}
		return "Body #" + std::to_string(tracked_body_id_);
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

	[[nodiscard]] std::array<double, 3> compute_retrograde_circular_orbit_velocity(const std::array<double, 3>& pos, double central_mass) const noexcept {
		const auto prograde = compute_circular_orbit_velocity(pos, central_mass);
		return {-prograde[0], -prograde[1], -prograde[2]};
	}

	[[nodiscard]] std::array<double, 3> compute_escape_velocity(const std::array<double, 3>& pos, const std::array<double, 3>& current_vel, double central_mass) const noexcept {
		const double r = std::sqrt(pos[0] * pos[0] + pos[1] * pos[1] + pos[2] * pos[2]);
		if (r < 1e-12) return {0.0, 0.0, 0.0};
		const double mu = std::max(orchestrator_.physical_gravitational_constant() * central_mass, 0.0);
		const double v_esc = std::sqrt(2.0 * mu / r);
		const double cur_v = std::sqrt(current_vel[0] * current_vel[0] + current_vel[1] * current_vel[1] + current_vel[2] * current_vel[2]);
		if (cur_v > 1e-9) {
			const double scale = v_esc / cur_v;
			return {current_vel[0] * scale, current_vel[1] * scale, current_vel[2] * scale};
		}
		const auto prograde = compute_circular_orbit_velocity(pos, central_mass);
		const double pro_v = std::sqrt(prograde[0] * prograde[0] + prograde[1] * prograde[1] + prograde[2] * prograde[2]);
		if (pro_v > 1e-9) {
			const double scale = v_esc / pro_v;
			return {prograde[0] * scale, prograde[1] * scale, prograde[2] * scale};
		}
		return {0.0, v_esc, 0.0};
	}

	[[nodiscard]] std::array<double, 3> compute_target_eccentricity_velocity(
		const std::array<double, 3>& pos,
		const std::array<double, 3>& current_vel,
		double central_mass,
		double target_ecc
	) const noexcept {
		const double r = std::sqrt(pos[0] * pos[0] + pos[1] * pos[1] + pos[2] * pos[2]);
		if (r < 1e-12) return current_vel;
		const double mu = std::max(orchestrator_.physical_gravitational_constant() * central_mass, 1e-14);
		const double clamped_ecc = std::max(target_ecc, 0.0);
		const double v_peri = std::sqrt(mu * (1.0 + clamped_ecc) / r);

		const double r_inv = 1.0 / r;
		const std::array<double, 3> r_hat{pos[0] * r_inv, pos[1] * r_inv, pos[2] * r_inv};
		const double v_rad = current_vel[0] * r_hat[0] + current_vel[1] * r_hat[1] + current_vel[2] * r_hat[2];
		std::array<double, 3> v_tan{current_vel[0] - v_rad * r_hat[0], current_vel[1] - v_rad * r_hat[1], current_vel[2] - v_rad * r_hat[2]};
		const double v_tan_len = std::sqrt(v_tan[0] * v_tan[0] + v_tan[1] * v_tan[1] + v_tan[2] * v_tan[2]);
		if (v_tan_len > 1e-9) {
			const double scale = v_peri / v_tan_len;
			return {v_tan[0] * scale, v_tan[1] * scale, v_tan[2] * scale};
		}
		const auto prograde = compute_circular_orbit_velocity(pos, central_mass);
		const double pro_len = std::sqrt(prograde[0] * prograde[0] + prograde[1] * prograde[1] + prograde[2] * prograde[2]);
		if (pro_len > 1e-9) {
			const double scale = v_peri / pro_len;
			return {prograde[0] * scale, prograde[1] * scale, prograde[2] * scale};
		}
		return {0.0, v_peri, 0.0};
	}

	[[nodiscard]] KeplerianOrbitElements compute_keplerian_elements(
		const std::array<double, 3>& pos,
		const std::array<double, 3>& vel,
		double central_mass
	) const noexcept {
		KeplerianOrbitElements el;
		const double r2 = pos[0] * pos[0] + pos[1] * pos[1] + pos[2] * pos[2];
		const double r = std::sqrt(r2);
		if (r < 1e-12) return el;
		const double v2 = vel[0] * vel[0] + vel[1] * vel[1] + vel[2] * vel[2];
		const double mu = std::max(orchestrator_.physical_gravitational_constant() * central_mass, 1e-14);

		const std::array<double, 3> h_vec{
			pos[1] * vel[2] - pos[2] * vel[1],
			pos[2] * vel[0] - pos[0] * vel[2],
			pos[0] * vel[1] - pos[1] * vel[0]
		};
		const double h2 = h_vec[0] * h_vec[0] + h_vec[1] * h_vec[1] + h_vec[2] * h_vec[2];
		const double h = std::sqrt(h2);
		el.angular_momentum = h;

		const double energy = 0.5 * v2 - mu / r;
		el.specific_energy = energy;

		const std::array<double, 3> v_cross_h{
			vel[1] * h_vec[2] - vel[2] * h_vec[1],
			vel[2] * h_vec[0] - vel[0] * h_vec[2],
			vel[0] * h_vec[1] - vel[1] * h_vec[0]
		};
		const std::array<double, 3> e_vec{
			v_cross_h[0] / mu - pos[0] / r,
			v_cross_h[1] / mu - pos[1] / r,
			v_cross_h[2] / mu - pos[2] / r
		};
		const double ecc = std::sqrt(e_vec[0] * e_vec[0] + e_vec[1] * e_vec[1] + e_vec[2] * e_vec[2]);
		el.eccentricity = ecc;

		if (std::abs(energy) > 1e-12) {
			el.semi_major_axis = -mu / (2.0 * energy);
		} else {
			el.semi_major_axis = 1e30;
		}

		if (ecc < 1.0) {
			el.is_bound = true;
			el.periapsis = el.semi_major_axis * (1.0 - ecc);
			el.apoapsis = el.semi_major_axis * (1.0 + ecc);
			if (el.semi_major_axis > 0.0) {
				const double sim_period = 2.0 * std::numbers::pi * std::sqrt((el.semi_major_axis * el.semi_major_axis * el.semi_major_axis) / mu);
				const double time_scale = length_scale() / speed_scale();
				el.period_seconds = sim_period * time_scale;
			}
		} else {
			el.is_bound = false;
			el.periapsis = (h2 > 1e-12) ? (h2 / (mu * (1.0 + ecc))) : std::max(r * 0.5, 1e-6);
			el.apoapsis = std::numeric_limits<double>::infinity();
			el.period_seconds = 0.0;
		}

		if (h > 1e-12) {
			const double cos_i = std::clamp(h_vec[2] / h, -1.0, 1.0);
			el.inclination_deg = std::acos(cos_i) * (180.0 / std::numbers::pi);
		} else {
			el.inclination_deg = 0.0;
		}
		el.is_prograde = (el.inclination_deg <= 90.0);

		if (ecc < 0.005) {
			el.type_name = "Circular";
		} else if (ecc < 0.999) {
			el.type_name = "Elliptic";
		} else if (ecc <= 1.001) {
			el.type_name = "Parabolic (Borderline)";
		} else {
			el.type_name = "Hyperbolic (Escape)";
		}
		el.valid = true;
		return el;
	}

	[[nodiscard]] TrajectoryHazardDiagnostic evaluate_trajectory_hazard(
		const std::array<double, 3>& pos,
		const std::array<double, 3>& vel,
		double body_mass,
		double body_radius,
		double central_mass
	) const noexcept {
		TrajectoryHazardDiagnostic diag;
		const auto el = compute_keplerian_elements(pos, vel, central_mass);
		if (!el.valid) return diag;

		diag.periapsis = el.periapsis;
		diag.horizon_radius = primary_horizon_radius();

		if (diag.horizon_radius > 0.0 && el.periapsis <= diag.horizon_radius) {
			diag.has_hazard = true;
			diag.plunge_hazard = true;
		}

		if (body_mass > 1e-15 && body_radius > 1e-9 && central_mass > 1e-12) {
			const double mass_ratio = central_mass / body_mass;
			diag.roche_limit = 1.8 * body_radius * std::cbrt(std::max(mass_ratio, 1.0));
			if (!diag.plunge_hazard && el.periapsis <= diag.roche_limit) {
				diag.has_hazard = true;
				diag.roche_hazard = true;
			}
		}
		return diag;
	}

	[[nodiscard]] BinarySourceConfiguration compute_binary_configuration(const Dynamics::PostNewtonianBody& secondary) const noexcept {
		BinarySourceConfiguration conf;
		const double m1 = std::max(orchestrator_.parameters().mass, 1e-9);
		const double m2 = std::max(secondary.mass, 1e-9);
		const double d = distance_from_center(secondary);
		if (d < 1e-9) return conf;

		conf.valid = true;
		conf.primary_mass = m1;
		conf.secondary_mass = m2;
		conf.mass_ratio = m2 / m1;
		const double m_tot = m1 + m2;
		conf.symmetric_mass_ratio = (m1 * m2) / (m_tot * m_tot);
		conf.separation_distance = d;
		conf.separation_rg = d / m1;

		const std::array<double, 3> l_vec{
			secondary.position[1] * secondary.velocity[2] - secondary.position[2] * secondary.velocity[1],
			secondary.position[2] * secondary.velocity[0] - secondary.position[0] * secondary.velocity[2],
			secondary.position[0] * secondary.velocity[1] - secondary.position[1] * secondary.velocity[0]
		};
		const double l_norm = std::sqrt(l_vec[0] * l_vec[0] + l_vec[1] * l_vec[1] + l_vec[2] * l_vec[2]);
		const std::array<double, 3> s2_hat = secondary.spin_axis_unit();

		if (l_norm > 1e-12) {
			const double dot_ls = (s2_hat[0] * l_vec[0] + s2_hat[1] * l_vec[1] + s2_hat[2] * l_vec[2]) / l_norm;
			conf.spin_orbit_misalignment_deg = std::acos(std::clamp(dot_ls, -1.0, 1.0)) * (180.0 / std::numbers::pi);
		}

		const double s1_val = orchestrator_.parameters().spin;
		const std::array<double, 3> s1_hat{0.0, 0.0, (s1_val >= 0.0) ? 1.0 : -1.0};
		const double dot_s1_s2 = s1_hat[0] * s2_hat[0] + s1_hat[1] * s2_hat[1] + s1_hat[2] * s2_hat[2];
		conf.mutual_spin_angle_deg = std::acos(std::clamp(dot_s1_s2, -1.0, 1.0)) * (180.0 / std::numbers::pi);

		const double g_sim = orchestrator_.physical_gravitational_constant();
		const double d4 = d * d * d * d;
		const double denom = 256.0 * (g_sim * g_sim * g_sim) * m1 * m2 * m_tot;
		if (denom > 1e-30) {
			const double t_sim = (5.0 * d4) / denom;
			const double time_scale = length_scale() / speed_scale();
			const auto el = compute_keplerian_elements(secondary.position, secondary.velocity, m_tot);
			const double ecc = el.valid ? std::clamp(el.eccentricity, 0.0, 0.999) : 0.0;
			const double ecc_factor = std::pow(std::max(1.0 - ecc * ecc, 1e-4), 3.5);
			conf.peters_coalescence_time_seconds = t_sim * time_scale * ecc_factor;
		}
		return conf;
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

	[[nodiscard]] CollisionOverlapDiagnostic check_placement_collision(
		const std::array<double, 3>& pos,
		double radius,
		bool is_source,
		uint32_t ignore_id = 0
	) const noexcept {
		CollisionOverlapDiagnostic diag;
		const double primary_h = primary_horizon_radius();
		const double d_center = std::sqrt(pos[0] * pos[0] + pos[1] * pos[1] + pos[2] * pos[2]);
		const double primary_margin = is_source ? 1.25 : 1.05;
		if (primary_h > 0.0 && d_center < (primary_h + radius) * primary_margin) {
			diag.has_collision = true;
			diag.inside_primary_horizon = true;
			diag.clearance_needed = (primary_h + radius) * primary_margin - d_center;
			return diag;
		}
		auto& sys = orchestrator_.nbody_system();
		std::lock_guard<std::recursive_mutex> lock(sys.bodies_mutex());
		for (const auto& other : sys.bodies()) {
			if (!other.enabled || other.id == ignore_id) continue;
			const double dx = pos[0] - other.position[0];
			const double dy = pos[1] - other.position[1];
			const double dz = pos[2] - other.position[2];
			const double dist = std::sqrt(dx * dx + dy * dy + dz * dz);
			if (other.is_spacetime_source) {
				const double other_h = other.kerr_outer_horizon_radius();
				const double margin = is_source ? 1.25 : 1.10;
				const double req = (other_h + radius) * margin;
				if (dist < req) {
					diag.has_collision = true;
					diag.inside_source_horizon = true;
					diag.colliding_body_id = other.id;
					diag.colliding_body_name = display_name(other);
					diag.clearance_needed = req - dist;
					return diag;
				}
			} else {
				const double margin = is_source ? 1.25 : 1.10;
				const double req = (other.radius + radius) * margin;
				if (dist < req) {
					diag.has_collision = true;
					diag.overlaps_body = true;
					diag.colliding_body_id = other.id;
					diag.colliding_body_name = display_name(other);
					diag.clearance_needed = req - dist;
					return diag;
				}
			}
		}
		return diag;
	}

	void resolve_safe_placement(
		std::array<double, 3>& pos,
		double radius,
		bool is_source,
		uint32_t ignore_id = 0
	) const noexcept {
		const double primary_margin = is_source ? 1.25 : 1.15;
		const double source_margin = is_source ? 1.25 : 1.15;
		const double body_margin = is_source ? 1.20 : 1.10;
		auto& sys = orchestrator_.nbody_system();
		std::lock_guard<std::recursive_mutex> lock(sys.bodies_mutex());
		const double primary_h = primary_horizon_radius();
		const auto push_away = [&pos](const std::array<double, 3>& center, double req) noexcept -> bool {
			const std::array<double, 3> offset{pos[0] - center[0], pos[1] - center[1], pos[2] - center[2]};
			const double dist = std::sqrt(offset[0] * offset[0] + offset[1] * offset[1] + offset[2] * offset[2]);
			if (dist >= req) return false;
			const std::array<double, 3> dir = (dist > 1e-9)
				? std::array<double, 3>{offset[0] / dist, offset[1] / dist, offset[2] / dist}
				: std::array<double, 3>{1.0, 0.0, 0.0};
			pos = {center[0] + dir[0] * req, center[1] + dir[1] * req, center[2] + dir[2] * req};
			return true;
		};
		for (int pass = 0; pass < 32; ++pass) {
			bool moved = false;
			if (primary_h > 0.0) {
				moved = push_away({0.0, 0.0, 0.0}, (primary_h + radius) * primary_margin) || moved;
			}
			for (const auto& other : sys.bodies()) {
				if (!other.enabled || other.id == ignore_id) continue;
				const bool other_is_source = other.is_spacetime_source;
				const double other_extent = other_is_source ? other.kerr_outer_horizon_radius() : other.radius;
				const double margin = other_is_source ? source_margin : body_margin;
				moved = push_away(other.position, (other_extent + radius) * margin) || moved;
			}
			if (!moved) break;
		}
	}

	void shift_to_center_of_mass_frame(BulkActionScope scope = BulkActionScope::AllBodies, CatalogTypeCategory category = CatalogTypeCategory::All) noexcept {
		auto& sys = orchestrator_.nbody_system();
		std::lock_guard<std::recursive_mutex> lock(sys.bodies_mutex());
		double m_tot = 0.0;
		std::array<double, 3> p_tot{0.0, 0.0, 0.0};
		for (const auto& b : sys.bodies()) {
			if (!b.enabled || !matches_bulk_scope(b, scope, category)) continue;
			m_tot += b.mass;
			p_tot[0] += b.mass * b.velocity[0];
			p_tot[1] += b.mass * b.velocity[1];
			p_tot[2] += b.mass * b.velocity[2];
		}
		if (m_tot > 1e-12) {
			const std::array<double, 3> v_cm{p_tot[0] / m_tot, p_tot[1] / m_tot, p_tot[2] / m_tot};
			for (auto& b : sys.bodies()) {
				if (!b.enabled || !matches_bulk_scope(b, scope, category)) continue;
				b.velocity[0] -= v_cm[0];
				b.velocity[1] -= v_cm[1];
				b.velocity[2] -= v_cm[2];
			}
			sys.update_accelerations();
			orchestrator_.notify_state_changed();
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

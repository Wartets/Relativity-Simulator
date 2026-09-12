#pragma once

#include "relativistic/dynamics/pn_body.hpp"
#include "relativistic/dynamics/interaction_config.hpp"
#include <span>
#include <vector>
#include <array>
#include <cmath>
#include <numbers>
#include <algorithm>
#include <cstdint>

namespace Relativistic::Dynamics {

class InteractionSolver {
private:
	static constexpr double dot3(const std::array<double, 3>& a, const std::array<double, 3>& b) noexcept {
		return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
	}

	static std::array<double, 3> sub3(const std::array<double, 3>& a, const std::array<double, 3>& b) noexcept {
		return {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
	}

public:
	static void apply_electromagnetic(std::span<PostNewtonianBody> bodies, const ElectromagneticInteractionConfig& cfg) noexcept {
		if (!cfg.electricity_enabled && !cfg.magnetism_enabled) return;
		const double ke = (cfg.vacuum_permittivity > 1e-30) ? (1.0 / (4.0 * std::numbers::pi_v<double> * cfg.vacuum_permittivity)) : 0.0;
		const double km = cfg.vacuum_permeability / (4.0 * std::numbers::pi_v<double>);

		for (size_t i = 0; i < bodies.size(); ++i) {
			if (!bodies[i].enabled) continue;
			for (size_t j = i + 1; j < bodies.size(); ++j) {
				if (!bodies[j].enabled) continue;
				const auto r_vec = sub3(bodies[i].position, bodies[j].position);
				const double r2 = dot3(r_vec, r_vec);
				if (r2 <= 1e-12) continue;
				const double r = std::sqrt(r2);
				const double inv_r3 = 1.0 / (r2 * r);

				if (cfg.electricity_enabled && bodies[i].charge != 0.0 && bodies[j].charge != 0.0) {
					const double force_mag = ke * bodies[i].charge * bodies[j].charge * inv_r3;
					for (size_t c = 0; c < 3; ++c) {
						bodies[i].acceleration[c] += (force_mag / std::max(bodies[i].mass, 1e-30)) * r_vec[c];
						bodies[j].acceleration[c] -= (force_mag / std::max(bodies[j].mass, 1e-30)) * r_vec[c];
					}
				}

				if (cfg.magnetism_enabled && bodies[i].magnetic_moment != 0.0 && bodies[j].magnetic_moment != 0.0) {
					const double dipole_factor = 3.0 * km * bodies[i].magnetic_moment * bodies[j].magnetic_moment / (r2 * r2);
					for (size_t c = 0; c < 3; ++c) {
						bodies[i].acceleration[c] += (dipole_factor / std::max(bodies[i].mass, 1e-30)) * (r_vec[c] / r);
						bodies[j].acceleration[c] -= (dipole_factor / std::max(bodies[j].mass, 1e-30)) * (r_vec[c] / r);
					}
				}
			}
		}
	}

	static void apply_thermodynamics(std::span<PostNewtonianBody> bodies, const ThermodynamicsInteractionConfig& cfg, double dt) noexcept {
		if (!cfg.enabled || cfg.ambient_temperature_kelvin < 0.0 || dt <= 0.0) return;
		constexpr double sigma = 5.670374419e-8;
		for (auto& body : bodies) {
			if (!body.enabled || body.heat_capacity <= 0.0) continue;
			const double t4 = body.temperature * body.temperature * body.temperature * body.temperature;
			const double t_amb4 = cfg.ambient_temperature_kelvin * cfg.ambient_temperature_kelvin * cfg.ambient_temperature_kelvin * cfg.ambient_temperature_kelvin;
			const double surface_area = 4.0 * std::numbers::pi_v<double> * std::max(body.radius, 1e-9) * std::max(body.radius, 1e-9);
			const double power = sigma * cfg.radiative_coupling_scale * std::max(body.absorption_factor, 0.0) * surface_area * (t4 - t_amb4);
			body.temperature = std::max(0.0, body.temperature - (power / body.heat_capacity) * dt);
		}
	}

	struct CollisionOutcome {
		std::vector<uint32_t> annihilated_body_ids{};
		std::vector<uint32_t> shattered_body_ids{};
	};

	static CollisionOutcome apply_collisions(
		std::span<PostNewtonianBody> bodies,
		const CollisionInteractionConfig& collision_cfg,
		const FragmentationInteractionConfig& fragmentation_cfg,
		const AnnihilationInteractionConfig& annihilation_cfg,
		double dt
	) noexcept {
		CollisionOutcome outcome;
		if (!collision_cfg.enabled) return outcome;

		for (size_t i = 0; i < bodies.size(); ++i) {
			if (!bodies[i].enabled) continue;
			for (size_t j = i + 1; j < bodies.size(); ++j) {
				if (!bodies[j].enabled) continue;

				const double contact_distance = bodies[i].radius + bodies[j].radius;
				const auto r_vec = sub3(bodies[i].position, bodies[j].position);
				const double r2 = dot3(r_vec, r_vec);
				if (r2 > contact_distance * contact_distance || r2 <= 1e-18) continue;
				const double r = std::sqrt(r2);
				const std::array<double, 3> normal = {r_vec[0] / r, r_vec[1] / r, r_vec[2] / r};

				if (annihilation_cfg.enabled) {
					const bool opposite_charge = bodies[i].charge * bodies[j].charge < 0.0;
					if ((!annihilation_cfg.require_opposite_charge || opposite_charge) && r <= contact_distance * annihilation_cfg.contact_distance_scale) {
						bodies[i].enabled = false;
						bodies[j].enabled = false;
						outcome.annihilated_body_ids.push_back(bodies[i].id);
						outcome.annihilated_body_ids.push_back(bodies[j].id);
						continue;
					}
				}

				const double m1 = std::max(bodies[i].mass, 1e-30);
				const double m2 = std::max(bodies[j].mass, 1e-30);
				const double overlap = contact_distance - r;

				if (overlap > collision_cfg.position_correction_slop) {
					const double youngs_i = (bodies[i].critical_temperature > 0.0 && bodies[i].temperature >= bodies[i].critical_temperature) ? bodies[i].youngs_modulus_hot : bodies[i].youngs_modulus_cold;
					const double youngs_j = (bodies[j].critical_temperature > 0.0 && bodies[j].temperature >= bodies[j].critical_temperature) ? bodies[j].youngs_modulus_hot : bodies[j].youngs_modulus_cold;
					const double youngs_eff = 0.5 * (std::max(youngs_i, 0.0) + std::max(youngs_j, 0.0));
					if (youngs_eff > 0.0 && dt > 0.0) {
						const double repulsion_force = collision_cfg.contact_stiffness_scale * youngs_eff * overlap;
						for (size_t c = 0; c < 3; ++c) {
							bodies[i].velocity[c] += (repulsion_force / m1) * dt * normal[c];
							bodies[j].velocity[c] -= (repulsion_force / m2) * dt * normal[c];
						}
					}

					const double correction_mag = (std::max(overlap - collision_cfg.position_correction_slop, 0.0) / (1.0 / m1 + 1.0 / m2)) * collision_cfg.position_correction_factor;
					for (size_t c = 0; c < 3; ++c) {
						bodies[i].position[c] += (correction_mag / m1) * normal[c];
						bodies[j].position[c] -= (correction_mag / m2) * normal[c];
					}
				}

				const auto v_rel = sub3(bodies[i].velocity, bodies[j].velocity);
				const double v_normal = dot3(v_rel, normal);
				if (v_normal >= 0.0) continue;

				const double restitution = (collision_cfg.response_model == CollisionResponseModel::Elastic)
					? std::clamp(0.5 * (bodies[i].restitution + bodies[j].restitution) * collision_cfg.restitution_multiplier, 0.0, 1.0)
					: 0.0;

				const double impulse_mag = -(1.0 + restitution) * v_normal / (1.0 / m1 + 1.0 / m2);
				for (size_t c = 0; c < 3; ++c) {
					bodies[i].velocity[c] += (impulse_mag / m1) * normal[c];
					bodies[j].velocity[c] -= (impulse_mag / m2) * normal[c];
				}

				if (collision_cfg.consider_friction) {
					const auto v_rel_post = sub3(bodies[i].velocity, bodies[j].velocity);
					const double v_normal_post = dot3(v_rel_post, normal);
					std::array<double, 3> tangential = {
						v_rel_post[0] - v_normal_post * normal[0],
						v_rel_post[1] - v_normal_post * normal[1],
						v_rel_post[2] - v_normal_post * normal[2]
					};
					const double tangential_speed = std::sqrt(dot3(tangential, tangential));
					if (tangential_speed > 1e-9) {
						for (auto& c : tangential) c /= tangential_speed;
						const double friction_coeff = 0.5 * (bodies[i].friction_coefficient + bodies[j].friction_coefficient);
						const double friction_impulse = std::min(friction_coeff * std::abs(impulse_mag), tangential_speed / (1.0 / m1 + 1.0 / m2));
						for (size_t c = 0; c < 3; ++c) {
							bodies[i].velocity[c] -= (friction_impulse / m1) * tangential[c];
							bodies[j].velocity[c] += (friction_impulse / m2) * tangential[c];
						}
						if (collision_cfg.consider_rotation) {
							bodies[i].rotation_speed += (friction_impulse * bodies[i].radius) / std::max(0.4 * m1 * bodies[i].radius * bodies[i].radius, 1e-30);
							bodies[j].rotation_speed -= (friction_impulse * bodies[j].radius) / std::max(0.4 * m2 * bodies[j].radius * bodies[j].radius, 1e-30);
						}
					}
				}

				if (fragmentation_cfg.enabled) {
					const double reduced_mass = (m1 * m2) / (m1 + m2);
					const double impact_energy = 0.5 * reduced_mass * v_normal * v_normal;
					bodies[i].integrity = std::max(0.0, bodies[i].integrity - impact_energy * fragmentation_cfg.collision_energy_to_integrity_loss);
					bodies[j].integrity = std::max(0.0, bodies[j].integrity - impact_energy * fragmentation_cfg.collision_energy_to_integrity_loss);
					if (bodies[i].integrity <= 0.0) outcome.shattered_body_ids.push_back(bodies[i].id);
					if (bodies[j].integrity <= 0.0) outcome.shattered_body_ids.push_back(bodies[j].id);
				}
			}
		}

		return outcome;
	}

	static void apply_tidal_stress(
		std::span<PostNewtonianBody> bodies,
		double central_mass,
		const FragmentationInteractionConfig& fragmentation_cfg,
		double gravitational_constant,
		double dt
	) noexcept {
		if (!fragmentation_cfg.enabled || !fragmentation_cfg.enable_tidal_stress || dt <= 0.0 || central_mass <= 0.0) return;
		for (auto& body : bodies) {
			if (!body.enabled || body.radius <= 0.0) continue;
			const double r2 = dot3(body.position, body.position);
			if (r2 <= 1e-12) continue;
			const double r = std::sqrt(r2);
			const double tidal_accel_gradient = (2.0 * gravitational_constant * central_mass) / (r2 * r);
			const double differential_accel = tidal_accel_gradient * body.radius;
			const double tidal_stress_energy = differential_accel * differential_accel * body.mass * dt;
			body.integrity = std::max(0.0, body.integrity - tidal_stress_energy * fragmentation_cfg.tidal_stress_to_integrity_loss);
		}
	}
};

}

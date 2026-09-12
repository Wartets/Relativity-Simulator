#pragma once

#include "relativistic/dynamics/pn_body.hpp"
#include "relativistic/dynamics/pn_orders.hpp"
#include "relativistic/dynamics/pn_acceleration.hpp"
#include "relativistic/dynamics/pn_spin_precession.hpp"
#include "relativistic/dynamics/pn_gravitational_waves.hpp"
#include "relativistic/dynamics/interaction_config.hpp"
#include "relativistic/dynamics/interaction_solver.hpp"
#include <vector>
#include <array>
#include <cmath>
#include <numeric>
#include <numbers>
#include <span>
#include <limits>

namespace Relativistic::Dynamics {

class PostNewtonianSystem {
private:
	std::vector<PostNewtonianBody> bodies_;
	PNOrderConfig config_;
	double time_{0.0};
	uint64_t step_count_{0};
	GravitationalWaveEmission latest_gw_emission_{};
	std::vector<PostNewtonianAccelerations> accelerations_buffer_{};
	InteractionConfig interaction_config_{};
	bool has_central_body_{false};
	bool central_body_stationary_{true};
	PostNewtonianBody central_body_{};
	uint32_t next_body_id_{1};

	[[nodiscard]] bool id_in_use(uint32_t id) const noexcept {
		return std::any_of(bodies_.begin(), bodies_.end(), [id](const PostNewtonianBody& b) { return b.id == id; });
	}

	[[nodiscard]] uint32_t allocate_id() noexcept {
		while (next_body_id_ == 0 || id_in_use(next_body_id_)) ++next_body_id_;
		return next_body_id_++;
	}

public:
	explicit PostNewtonianSystem(const PNOrderConfig& config = {}) noexcept
		: config_(config) {}

	uint32_t add_body(PostNewtonianBody body) {
		if (body.id == 0 || id_in_use(body.id)) body.id = allocate_id();
		else if (body.id >= next_body_id_) next_body_id_ = body.id + 1;
		bodies_.push_back(std::move(body));
		return bodies_.back().id;
	}

	uint32_t next_body_id() noexcept { return allocate_id(); }

	void clear_bodies() noexcept {
		bodies_.clear();
		time_ = 0.0;
		step_count_ = 0;
		next_body_id_ = 1;
	}

	void set_central_body(const PostNewtonianBody& cb, bool stationary = true) noexcept {
		central_body_ = cb;
		has_central_body_ = true;
		central_body_stationary_ = stationary;
	}

	void clear_central_body() noexcept {
		has_central_body_ = false;
	}

	[[nodiscard]] bool has_central_body() const noexcept {
		return has_central_body_;
	}

	[[nodiscard]] const PostNewtonianBody& central_body() const noexcept {
		return central_body_;
	}

	[[nodiscard]] PostNewtonianBody& central_body() noexcept {
		return central_body_;
	}

	[[nodiscard]] bool is_central_body_stationary() const noexcept {
		return central_body_stationary_;
	}

	void set_central_body_stationary(bool stationary) noexcept {
		central_body_stationary_ = stationary;
	}

	[[nodiscard]] std::span<const PostNewtonianBody> bodies() const noexcept {
		return bodies_;
	}

	[[nodiscard]] std::span<PostNewtonianBody> bodies() noexcept {
		return bodies_;
	}

	[[nodiscard]] size_t body_count() const noexcept {
		return bodies_.size();
	}

	[[nodiscard]] const PNOrderConfig& config() const noexcept {
		return config_;
	}

	void set_config(const PNOrderConfig& cfg) noexcept {
		config_ = cfg;
	}

	[[nodiscard]] const InteractionConfig& interaction_config() const noexcept {
		return interaction_config_;
	}

	[[nodiscard]] InteractionConfig& interaction_config() noexcept {
		return interaction_config_;
	}

	void set_interaction_config(const InteractionConfig& cfg) noexcept {
		interaction_config_ = cfg;
	}

	void step_interactions(double dt) noexcept {
		if (bodies_.empty()) return;

		InteractionSolver::apply_electromagnetic(bodies_, interaction_config_.electromagnetic);
		InteractionSolver::apply_thermodynamics(bodies_, interaction_config_.thermodynamics, dt);
		const auto outcome = InteractionSolver::apply_collisions(bodies_, interaction_config_.collisions, interaction_config_.fragmentation, interaction_config_.annihilation);

		if (outcome.annihilated_body_ids.empty() && outcome.shattered_body_ids.empty()) return;

		std::vector<PostNewtonianBody> next_bodies;
		next_bodies.reserve(bodies_.size());
		for (auto& body : bodies_) {
			if (!body.enabled) continue;
			const bool shattered = std::find(outcome.shattered_body_ids.begin(), outcome.shattered_body_ids.end(), body.id) != outcome.shattered_body_ids.end();
			if (!shattered) {
				next_bodies.push_back(body);
				continue;
			}
			const uint32_t fragment_count = std::max<uint32_t>(interaction_config_.fragmentation.max_fragments_per_event, 1);
			const double fragment_mass = body.mass / static_cast<double>(fragment_count);
			if (fragment_mass < interaction_config_.fragmentation.minimum_fragment_mass) {
				continue;
			}
			const double fragment_radius = body.radius / std::cbrt(static_cast<double>(fragment_count));
			for (uint32_t f = 0; f < fragment_count; ++f) {
				PostNewtonianBody fragment = body;
				fragment.id = allocate_id();
				fragment.mass = fragment_mass;
				fragment.radius = fragment_radius;
				fragment.integrity = 1.0;
				const double angle = (2.0 * std::numbers::pi_v<double> * static_cast<double>(f)) / static_cast<double>(fragment_count);
				const double offset = fragment_radius * 1.5;
				fragment.position[0] += offset * std::cos(angle);
				fragment.position[1] += offset * std::sin(angle);
				fragment.velocity[0] += 0.1 * std::cos(angle);
				fragment.velocity[1] += 0.1 * std::sin(angle);
				next_bodies.push_back(fragment);
			}
		}

		bodies_ = std::move(next_bodies);
		update_accelerations();
	}

	[[nodiscard]] double time() const noexcept {
		return time_;
	}

	[[nodiscard]] uint64_t step_count() const noexcept {
		return step_count_;
	}

	[[nodiscard]] const GravitationalWaveEmission& latest_gw_emission() const noexcept {
		return latest_gw_emission_;
	}

	void update_accelerations() noexcept {
		std::vector<size_t> active_indices;
		active_indices.reserve(bodies_.size());
		for (size_t i = 0; i < bodies_.size(); ++i) {
			if (bodies_[i].enabled) active_indices.push_back(i);
			else bodies_[i].acceleration = {0.0, 0.0, 0.0};
		}
		const size_t n = active_indices.size();
		if (n == 0) return;
		std::vector<PostNewtonianBody> active_bodies;
		active_bodies.reserve(n);
		for (size_t index : active_indices) active_bodies.push_back(bodies_[index]);

		if (!has_central_body_ && n == 2) {
			const auto& b1 = active_bodies[0];
			const auto& b2 = active_bodies[1];
			const std::array<double, 3> r_rel = {b1.position[0] - b2.position[0], b1.position[1] - b2.position[1], b1.position[2] - b2.position[2]};
			const std::array<double, 3> v_rel = {b1.velocity[0] - b2.velocity[0], b1.velocity[1] - b2.velocity[1], b1.velocity[2] - b2.velocity[2]};

			const auto acc_rel = PostNewtonianSolver::compute_binary_relative_acceleration(
				r_rel, v_rel, b1.mass, b2.mass, b1.spin, b2.spin, config_
			);

			const double m_tot = b1.mass + b2.mass;
			const double f1 = b2.mass / m_tot;
			const double f2 = -b1.mass / m_tot;

			for (size_t c = 0; c < 3; ++c) {
				bodies_[active_indices[0]].acceleration[c] = f1 * acc_rel.a_total[c];
				bodies_[active_indices[1]].acceleration[c] = f2 * acc_rel.a_total[c];
			}
		} else {
			if (accelerations_buffer_.size() != n) {
				accelerations_buffer_.resize(n);
			}
			if (n >= 2) {
				PostNewtonianSolver::compute_nbody_accelerations(active_bodies, config_, accelerations_buffer_);
				for (size_t i = 0; i < n; ++i) {
					bodies_[active_indices[i]].acceleration = accelerations_buffer_[i].a_total;
				}
			} else {
				for (size_t i = 0; i < n; ++i) {
					bodies_[active_indices[i]].acceleration = {0.0, 0.0, 0.0};
				}
			}

			if (has_central_body_ && central_body_.mass > 0.0) {
				for (size_t i = 0; i < n; ++i) {
					auto& body = bodies_[active_indices[i]];
					const std::array<double, 3> r_rel = {
						central_body_.position[0] - body.position[0],
						central_body_.position[1] - body.position[1],
						central_body_.position[2] - body.position[2]
					};
					const std::array<double, 3> v_rel = {
						central_body_.velocity[0] - body.velocity[0],
						central_body_.velocity[1] - body.velocity[1],
						central_body_.velocity[2] - body.velocity[2]
					};
					const auto acc_cen = PostNewtonianSolver::compute_binary_relative_acceleration(
						r_rel, v_rel, central_body_.mass, bodies_[i].mass,
						central_body_.spin, body.spin, config_
					);
					const double m_tot = central_body_.mass + bodies_[i].mass;
					const double f_i = (m_tot > 0.0) ? (-central_body_.mass / m_tot) : -1.0;
					for (size_t c = 0; c < 3; ++c) {
						body.acceleration[c] += f_i * acc_cen.a_total[c];
					}
				}
			}
		}

		latest_gw_emission_ = GravitationalWaveCalculator::compute_nbody_quadrupole(
			active_bodies, config_.speed_of_light, config_.gravitational_constant
		);
	}

	[[nodiscard]] double total_mass() const noexcept {
		double m = 0.0;
		for (const auto& b : bodies_) if (b.enabled) m += b.mass;
		return m;
	}

	[[nodiscard]] std::array<double, 3> center_of_mass() const noexcept {
		const double m_tot = total_mass();
		if (m_tot <= 0.0) return {0.0, 0.0, 0.0};
		std::array<double, 3> cm{0.0, 0.0, 0.0};
		for (const auto& b : bodies_) {
			if (!b.enabled) continue;
			cm[0] += b.mass * b.position[0];
			cm[1] += b.mass * b.position[1];
			cm[2] += b.mass * b.position[2];
		}
		cm[0] /= m_tot;
		cm[1] /= m_tot;
		cm[2] /= m_tot;
		return cm;
	}

	[[nodiscard]] std::array<double, 3> total_linear_momentum() const noexcept {
		std::array<double, 3> p{0.0, 0.0, 0.0};
		for (const auto& b : bodies_) {
			if (!b.enabled) continue;
			p[0] += b.mass * b.velocity[0];
			p[1] += b.mass * b.velocity[1];
			p[2] += b.mass * b.velocity[2];
		}
		return p;
	}

	[[nodiscard]] std::array<double, 3> total_angular_momentum() const noexcept {
		const size_t n = bodies_.size();
		if (n == 0) return {0.0, 0.0, 0.0};

		if (n == 2 && config_.enable_1pn && config_.speed_of_light > 0.0) {
			const auto& b1 = bodies_[0];
			const auto& b2 = bodies_[1];
			const double m1 = b1.mass;
			const double m2 = b2.mass;
			const double m_tot = m1 + m2;
			const double mu = (m1 * m2) / m_tot;
			const double eta = mu / m_tot;
			const double g = config_.gravitational_constant;
			const double c = config_.speed_of_light;
			const double c2 = c * c;

			const double dx = b1.position[0] - b2.position[0];
			const double dy = b1.position[1] - b2.position[1];
			const double dz = b1.position[2] - b2.position[2];
			const double r = std::sqrt(std::max(dx * dx + dy * dy + dz * dz, 1e-30));

			const double vx = b1.velocity[0] - b2.velocity[0];
			const double vy = b1.velocity[1] - b2.velocity[1];
			const double vz = b1.velocity[2] - b2.velocity[2];
			const double v2 = vx * vx + vy * vy + vz * vz;

			const double lx = mu * (dy * vz - dz * vy);
			const double ly = mu * (dz * vx - dx * vz);
			const double lz = mu * (dx * vy - dy * vx);

			const double factor = 1.0 + (1.0 / c2) * (0.5 * (1.0 - 3.0 * eta) * v2 + (3.0 + eta) * (g * m_tot) / r);

			return {
				lx * factor + b1.spin[0] + b2.spin[0],
				ly * factor + b1.spin[1] + b2.spin[1],
				lz * factor + b1.spin[2] + b2.spin[2]
			};
		}

		std::array<double, 3> l{0.0, 0.0, 0.0};
		for (const auto& b : bodies_) {
			l[0] += b.mass * (b.position[1] * b.velocity[2] - b.position[2] * b.velocity[1]) + b.spin[0];
			l[1] += b.mass * (b.position[2] * b.velocity[0] - b.position[0] * b.velocity[2]) + b.spin[1];
			l[2] += b.mass * (b.position[0] * b.velocity[1] - b.position[1] * b.velocity[0]) + b.spin[2];
		}
		return l;
	}

	[[nodiscard]] double compute_total_energy() const noexcept {
		const size_t n = bodies_.size();
		if (n == 0) return 0.0;

		double e_kin = 0.0;
		for (const auto& b : bodies_) {
			if (!b.enabled) continue;
			e_kin += b.kinetic_energy();
		}

		double e_pot = 0.0;
		for (size_t i = 0; i < n; ++i) {
			if (!bodies_[i].enabled) continue;
			for (size_t j = i + 1; j < n; ++j) {
				if (!bodies_[j].enabled) continue;
				const double dx = bodies_[i].position[0] - bodies_[j].position[0];
				const double dy = bodies_[i].position[1] - bodies_[j].position[1];
				const double dz = bodies_[i].position[2] - bodies_[j].position[2];
				const double r = std::sqrt(std::max(dx * dx + dy * dy + dz * dz, 1e-30));
				e_pot -= (config_.gravitational_constant * bodies_[i].mass * bodies_[j].mass) / r;
			}
		}

		double e_1pn = 0.0;
		if (config_.enable_1pn && config_.speed_of_light > 0.0) {
			const double c2 = config_.speed_of_light * config_.speed_of_light;
			const double inv_c2 = 1.0 / c2;
			const double g = config_.gravitational_constant;

			double term_v4 = 0.0;
			for (const auto& b : bodies_) {
				const double v2 = b.speed_squared();
				term_v4 += (3.0 / 8.0) * b.mass * v2 * v2;
			}

			double term_pairs = 0.0;
			for (size_t i = 0; i < n; ++i) {
				const double vi2 = bodies_[i].speed_squared();
				for (size_t j = i + 1; j < n; ++j) {
					const double vj2 = bodies_[j].speed_squared();
					const double vi_dot_vj = bodies_[i].velocity[0] * bodies_[j].velocity[0] + bodies_[i].velocity[1] * bodies_[j].velocity[1] + bodies_[i].velocity[2] * bodies_[j].velocity[2];

					const double dx = bodies_[i].position[0] - bodies_[j].position[0];
					const double dy = bodies_[i].position[1] - bodies_[j].position[1];
					const double dz = bodies_[i].position[2] - bodies_[j].position[2];
					const double r = std::sqrt(std::max(dx * dx + dy * dy + dz * dz, 1e-30));
					const double n_dot_vi = (dx * bodies_[i].velocity[0] + dy * bodies_[i].velocity[1] + dz * bodies_[i].velocity[2]) / r;
					const double n_dot_vj = (dx * bodies_[j].velocity[0] + dy * bodies_[j].velocity[1] + dz * bodies_[j].velocity[2]) / r;

					const double factor_ij = (g * bodies_[i].mass * bodies_[j].mass) / (2.0 * r);
					const double bracket = 3.0 * (vi2 + vj2) - 7.0 * vi_dot_vj - n_dot_vi * n_dot_vj + (g * (bodies_[i].mass + bodies_[j].mass)) / r;
					term_pairs += factor_ij * bracket;
				}
			}
			e_1pn = inv_c2 * (term_v4 + term_pairs);
		}

		return e_kin + e_pot + e_1pn;
	}

	void advance_time(double dt) noexcept {
		time_ += dt;
		++step_count_;
	}
};

}

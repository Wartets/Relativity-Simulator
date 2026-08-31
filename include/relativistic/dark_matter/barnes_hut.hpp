#pragma once

#include "relativistic/core/constants.hpp"
#include <array>
#include <vector>
#include <span>
#include <cmath>
#include <cstdint>
#include <algorithm>
#include <concepts>

namespace Relativistic::DarkMatter {

struct alignas(64) CollisionlessParticle {
	std::array<double, 3> position{0.0, 0.0, 0.0};
	std::array<double, 3> velocity{0.0, 0.0, 0.0};
	std::array<double, 3> acceleration{0.0, 0.0, 0.0};
	double mass{1.0};
	double softening{1.0};
	uint32_t type{0};
	uint32_t id{0};
};

struct BoundingBox3D {
	std::array<double, 3> min_corner{-1.0, -1.0, -1.0};
	std::array<double, 3> max_corner{1.0, 1.0, 1.0};
	std::array<double, 3> center{0.0, 0.0, 0.0};
	double half_size{1.0};

	constexpr BoundingBox3D() noexcept = default;

	constexpr BoundingBox3D(const std::array<double, 3>& min_c, const std::array<double, 3>& max_c) noexcept
		: min_corner(min_c), max_corner(max_c) {
		center[0] = 0.5 * (min_c[0] + max_c[0]);
		center[1] = 0.5 * (min_c[1] + max_c[1]);
		center[2] = 0.5 * (min_c[2] + max_c[2]);
		const double sx = max_c[0] - min_c[0];
		const double sy = max_c[1] - min_c[1];
		const double sz = max_c[2] - min_c[2];
		half_size = 0.5 * std::max({sx, sy, sz});
	}

	[[nodiscard]] constexpr uint32_t get_octant(const std::array<double, 3>& pos) const noexcept {
		uint32_t oct = 0;
		if (pos[0] >= center[0]) oct |= 1U;
		if (pos[1] >= center[1]) oct |= 2U;
		if (pos[2] >= center[2]) oct |= 4U;
		return oct;
	}

	[[nodiscard]] BoundingBox3D make_child_box(uint32_t octant) const noexcept {
		const double qs = 0.5 * half_size;
		std::array<double, 3> c_min{}, c_max{};
		const double cx = (octant & 1U) ? (center[0] + qs) : (center[0] - qs);
		const double cy = (octant & 2U) ? (center[1] + qs) : (center[1] - qs);
		const double cz = (octant & 4U) ? (center[2] + qs) : (center[2] - qs);
		c_min[0] = cx - qs;
		c_min[1] = cy - qs;
		c_min[2] = cz - qs;
		c_max[0] = cx + qs;
		c_max[1] = cy + qs;
		c_max[2] = cz + qs;
		return BoundingBox3D(c_min, c_max);
	}
};

struct alignas(64) OctreeNode {
	BoundingBox3D bounds{};
	std::array<double, 3> center_of_mass{0.0, 0.0, 0.0};
	std::array<std::array<double, 3>, 3> quadrupole_moment{};
	double total_mass{0.0};
	uint32_t children[8]{0, 0, 0, 0, 0, 0, 0, 0};
	uint32_t particle_index{0xFFFFFFFFU};
	uint32_t particle_count{0};
	bool is_leaf{true};
};

struct BarnesHutConfig {
	double opening_angle_theta{0.6};
	double default_softening{100.0};
	double gravitational_constant{Core::PhysicalConstants<double>::GRAVITATIONAL_CONSTANT};
	bool enable_quadrupole{true};
};

class BarnesHutOctree {
public:
	static constexpr uint32_t NULL_NODE = 0xFFFFFFFFU;

private:
	std::vector<OctreeNode> nodes_;
	BarnesHutConfig config_{};
	uint32_t root_index_{NULL_NODE};

public:
	explicit BarnesHutOctree(const BarnesHutConfig& config = {}) noexcept
		: config_(config) {}

	void clear() noexcept {
		nodes_.clear();
		root_index_ = NULL_NODE;
	}

	[[nodiscard]] size_t node_count() const noexcept {
		return nodes_.size();
	}

	[[nodiscard]] const std::vector<OctreeNode>& nodes() const noexcept {
		return nodes_;
	}

	void build(std::span<const CollisionlessParticle> particles) {
		clear();
		if (particles.empty()) {
			return;
		}

		std::array<double, 3> min_c = particles[0].position;
		std::array<double, 3> max_c = particles[0].position;

		for (const auto& p : particles) {
			for (size_t c = 0; c < 3; ++c) {
				if (p.position[c] < min_c[c]) min_c[c] = p.position[c];
				if (p.position[c] > max_c[c]) max_c[c] = p.position[c];
			}
		}

		for (size_t c = 0; c < 3; ++c) {
			const double pad = std::max((max_c[c] - min_c[c]) * 0.02, 1.0);
			min_c[c] -= pad;
			max_c[c] += pad;
		}

		nodes_.reserve(particles.size() * 4);
		root_index_ = allocate_node(BoundingBox3D(min_c, max_c));

		for (uint32_t i = 0; i < static_cast<uint32_t>(particles.size()); ++i) {
			insert_particle(root_index_, i, particles);
		}

		compute_multipoles(root_index_, particles);
	}

	[[nodiscard]] std::array<double, 3> evaluate_acceleration(
		const std::array<double, 3>& target_pos,
		double softening,
		std::span<const CollisionlessParticle> particles,
		uint32_t ignore_particle_idx = 0xFFFFFFFFU
	) const noexcept {
		static_cast<void>(particles);
		if (root_index_ == NULL_NODE || nodes_.empty()) {
			return {0.0, 0.0, 0.0};
		}

		std::array<double, 3> acc{0.0, 0.0, 0.0};
		const double theta2 = config_.opening_angle_theta * config_.opening_angle_theta;
		const double g = config_.gravitational_constant;
		const double eps2 = softening * softening;

		std::array<uint32_t, 256> stack{};
		size_t stack_ptr = 0;
		stack[stack_ptr++] = root_index_;

		while (stack_ptr > 0) {
			const uint32_t node_idx = stack[--stack_ptr];
			const auto& node = nodes_[node_idx];

			if (node.particle_count == 0 || node.total_mass <= 0.0) {
				continue;
			}

			const double dx = node.center_of_mass[0] - target_pos[0];
			const double dy = node.center_of_mass[1] - target_pos[1];
			const double dz = node.center_of_mass[2] - target_pos[2];
			const double d2 = dx * dx + dy * dy + dz * dz;

			if (node.is_leaf) {
				if (node.particle_count == 1) {
					if (node.particle_index != ignore_particle_idx) {
						const double r_eff2 = d2 + eps2;
						const double r_eff = std::sqrt(r_eff2);
						const double inv_r3 = 1.0 / (r_eff2 * r_eff);
						const double factor = g * node.total_mass * inv_r3;
						acc[0] += factor * dx;
						acc[1] += factor * dy;
						acc[2] += factor * dz;
					}
				}
				continue;
			}

			const double size = node.bounds.half_size * 2.0;
			const double size2 = size * size;

			if (size2 < theta2 * d2) {
				const double r_eff2 = d2 + eps2;
				const double r_eff = std::sqrt(r_eff2);
				const double inv_r3 = 1.0 / (r_eff2 * r_eff);
				const double factor = g * node.total_mass * inv_r3;

				acc[0] += factor * dx;
				acc[1] += factor * dy;
				acc[2] += factor * dz;

				if (config_.enable_quadrupole) {
					const double inv_r5 = inv_r3 / r_eff2;
					const double q_dot_r_x = node.quadrupole_moment[0][0] * dx + node.quadrupole_moment[0][1] * dy + node.quadrupole_moment[0][2] * dz;
					const double q_dot_r_y = node.quadrupole_moment[1][0] * dx + node.quadrupole_moment[1][1] * dy + node.quadrupole_moment[1][2] * dz;
					const double q_dot_r_z = node.quadrupole_moment[2][0] * dx + node.quadrupole_moment[2][1] * dy + node.quadrupole_moment[2][2] * dz;
					const double r_q_r = dx * q_dot_r_x + dy * q_dot_r_y + dz * q_dot_r_z;

					const double q_factor1 = g * inv_r5;
					const double q_factor2 = -2.5 * g * r_q_r * (inv_r5 / r_eff2);

					acc[0] += q_factor1 * q_dot_r_x + q_factor2 * dx;
					acc[1] += q_factor1 * q_dot_r_y + q_factor2 * dy;
					acc[2] += q_factor1 * q_dot_r_z + q_factor2 * dz;
				}
			} else {
				for (uint32_t c = 0; c < 8; ++c) {
					if (node.children[c] != NULL_NODE && stack_ptr < stack.size()) {
						stack[stack_ptr++] = node.children[c];
					}
				}
			}
		}

		return acc;
	}

	void compute_all_accelerations(std::span<CollisionlessParticle> particles) {
		build(particles);
		for (uint32_t i = 0; i < static_cast<uint32_t>(particles.size()); ++i) {
			const double softening = (particles[i].softening > 0.0) ? particles[i].softening : config_.default_softening;
			particles[i].acceleration = evaluate_acceleration(particles[i].position, softening, particles, i);
		}
	}

	void step_leapfrog(std::span<CollisionlessParticle> particles, double dt) {
		const size_t n = particles.size();
		if (n == 0) return;

		const double half_dt = 0.5 * dt;

		for (size_t i = 0; i < n; ++i) {
			for (size_t c = 0; c < 3; ++c) {
				particles[i].velocity[c] += half_dt * particles[i].acceleration[c];
				particles[i].position[c] += dt * particles[i].velocity[c];
			}
		}

		compute_all_accelerations(particles);

		for (size_t i = 0; i < n; ++i) {
			for (size_t c = 0; c < 3; ++c) {
				particles[i].velocity[c] += half_dt * particles[i].acceleration[c];
			}
		}
	}

private:
	[[nodiscard]] uint32_t allocate_node(const BoundingBox3D& bounds) {
		const uint32_t idx = static_cast<uint32_t>(nodes_.size());
		OctreeNode node{};
		node.bounds = bounds;
		node.is_leaf = true;
		node.particle_index = NULL_NODE;
		node.particle_count = 0;
		for (uint32_t& c : node.children) c = NULL_NODE;
		nodes_.push_back(node);
		return idx;
	}

	void insert_particle(uint32_t node_idx, uint32_t p_idx, std::span<const CollisionlessParticle> particles) {
		const auto& p = particles[p_idx];

		if (nodes_[node_idx].particle_count == 0) {
			nodes_[node_idx].particle_index = p_idx;
			nodes_[node_idx].particle_count = 1;
			nodes_[node_idx].center_of_mass = p.position;
			nodes_[node_idx].total_mass = p.mass;
			nodes_[node_idx].is_leaf = true;
			return;
		}

		if (nodes_[node_idx].is_leaf) {
			const uint32_t existing_p_idx = nodes_[node_idx].particle_index;
			nodes_[node_idx].is_leaf = false;
			nodes_[node_idx].particle_index = NULL_NODE;

			if (existing_p_idx != NULL_NODE) {
				const uint32_t oct_existing = nodes_[node_idx].bounds.get_octant(particles[existing_p_idx].position);
				if (nodes_[node_idx].children[oct_existing] == NULL_NODE) {
					nodes_[node_idx].children[oct_existing] = allocate_node(nodes_[node_idx].bounds.make_child_box(oct_existing));
				}
				insert_particle(nodes_[node_idx].children[oct_existing], existing_p_idx, particles);
			}
		}

		const uint32_t oct = nodes_[node_idx].bounds.get_octant(p.position);
		if (nodes_[node_idx].children[oct] == NULL_NODE) {
			nodes_[node_idx].children[oct] = allocate_node(nodes_[node_idx].bounds.make_child_box(oct));
		}
		insert_particle(nodes_[node_idx].children[oct], p_idx, particles);

		nodes_[node_idx].particle_count += 1;
	}

	void compute_multipoles(uint32_t node_idx, std::span<const CollisionlessParticle> particles) noexcept {
		if (node_idx == NULL_NODE) return;
		auto& node = nodes_[node_idx];

		if (node.is_leaf) {
			if (node.particle_count == 1 && node.particle_index != NULL_NODE) {
				const auto& p = particles[node.particle_index];
				node.total_mass = p.mass;
				node.center_of_mass = p.position;
				for (size_t i = 0; i < 3; ++i) {
					for (size_t j = 0; j < 3; ++j) {
						node.quadrupole_moment[i][j] = 0.0;
					}
				}
			}
			return;
		}

		double m_tot = 0.0;
		std::array<double, 3> cm{0.0, 0.0, 0.0};

		for (uint32_t c = 0; c < 8; ++c) {
			const uint32_t child_idx = node.children[c];
			if (child_idx != NULL_NODE) {
				compute_multipoles(child_idx, particles);
				const auto& child = nodes_[child_idx];
				m_tot += child.total_mass;
				cm[0] += child.total_mass * child.center_of_mass[0];
				cm[1] += child.total_mass * child.center_of_mass[1];
				cm[2] += child.total_mass * child.center_of_mass[2];
			}
		}

		if (m_tot > 0.0) {
			cm[0] /= m_tot;
			cm[1] /= m_tot;
			cm[2] /= m_tot;
		}

		node.total_mass = m_tot;
		node.center_of_mass = cm;

		for (size_t i = 0; i < 3; ++i) {
			for (size_t j = 0; j < 3; ++j) {
				node.quadrupole_moment[i][j] = 0.0;
			}
		}

		if (config_.enable_quadrupole) {
			for (uint32_t c = 0; c < 8; ++c) {
				const uint32_t child_idx = node.children[c];
				if (child_idx != NULL_NODE) {
					const auto& child = nodes_[child_idx];
					const double dx = child.center_of_mass[0] - cm[0];
					const double dy = child.center_of_mass[1] - cm[1];
					const double dz = child.center_of_mass[2] - cm[2];
					const std::array<double, 3> d = {dx, dy, dz};
					const double d2 = dx * dx + dy * dy + dz * dz;

					for (size_t i = 0; i < 3; ++i) {
						for (size_t j = 0; j < 3; ++j) {
							const double delta_ij = (i == j) ? 1.0 : 0.0;
							node.quadrupole_moment[i][j] += child.quadrupole_moment[i][j] + child.total_mass * (3.0 * d[i] * d[j] - delta_ij * d2);
						}
					}
				}
			}
		}
	}
};

}

#pragma once

#include "relativistic/dark_matter/dark_matter_profiles.hpp"
#include "relativistic/dark_matter/barnes_hut.hpp"
#include "relativistic/core/pcg64.hpp"
#include <vector>
#include <array>
#include <cmath>
#include <numbers>
#include <algorithm>

namespace Relativistic::DarkMatter {

struct GalaxyComponentConfig {
	double disk_mass{5e10 * Core::PhysicalConstants<double>::SOLAR_MASS};
	double disk_scale_length{3.0 * 1000.0 * 3.085677581491367e16};
	double disk_scale_height{0.3 * 1000.0 * 3.085677581491367e16};
	size_t disk_particle_count{2000};

	double bulge_mass{1e10 * Core::PhysicalConstants<double>::SOLAR_MASS};
	double bulge_scale_radius{0.8 * 1000.0 * 3.085677581491367e16};
	size_t bulge_particle_count{500};

	double halo_mass{1e12 * Core::PhysicalConstants<double>::SOLAR_MASS};
	double halo_scale_radius{25.0 * 1000.0 * 3.085677581491367e16};
	double halo_concentration{12.0};
	size_t halo_particle_count{5000};

	double default_softening{100.0 * 3.085677581491367e16};
};

class CompositeGalaxyGenerator {
public:
	static std::vector<CollisionlessParticle> generate_galaxy(
		const GalaxyComponentConfig& config,
		Core::PCG64Engine& rng,
		const std::array<double, 3>& center_pos = {0.0, 0.0, 0.0},
		const std::array<double, 3>& center_vel = {0.0, 0.0, 0.0},
		double inclination_rad = 0.0,
		double node_rad = 0.0,
		uint32_t start_id = 0
	) {
		std::vector<CollisionlessParticle> particles;
		const size_t total_count = config.disk_particle_count + config.bulge_particle_count + config.halo_particle_count;
		particles.reserve(total_count);

		const NFWProfile nfw(
			NFWProfile<double>::from_virial_parameters(config.halo_mass, config.halo_concentration)
		);
		const HernquistProfile bulge(config.bulge_mass, config.bulge_scale_radius);
		const double g = Core::PhysicalConstants<double>::GRAVITATIONAL_CONSTANT;

		auto compute_circular_velocity = [&](double r) noexcept -> double {
			const double r_safe = std::max(r, 1.0);
			const double m_halo = nfw.enclosed_mass(r_safe);
			const double m_bulge = bulge.enclosed_mass(r_safe);
			const double m_disk = config.disk_mass * (1.0 - (1.0 + r_safe / config.disk_scale_length) * std::exp(-r_safe / config.disk_scale_length));
			const double m_total = m_halo + m_bulge + m_disk;
			return std::sqrt(g * m_total / r_safe);
		};

		const double m_disk_part = (config.disk_particle_count > 0) ? (config.disk_mass / static_cast<double>(config.disk_particle_count)) : 0.0;
		for (size_t i = 0; i < config.disk_particle_count; ++i) {
			const double u1 = rng.next_uniform_double();
			const double u2 = rng.next_uniform_double();
			const double u3 = rng.next_uniform_double();

			double w = -std::log(std::max(1.0 - u1, 1e-15));
			for (int iter = 0; iter < 10; ++iter) {
				w = -std::log(std::max(1.0 - u1, 1e-15)) + std::log(std::max(1.0 + w, 1.0));
			}
			const double r = w * config.disk_scale_length;
			const double phi = 2.0 * std::numbers::pi_v<double> * u2;
			const double z = config.disk_scale_height * std::atanh(std::clamp(2.0 * u3 - 1.0, -0.999, 0.999));

			const double v_circ = compute_circular_velocity(r);
			const auto [disp_r, disp_phi] = rng.next_gaussian_pair(0.0, 0.1 * v_circ);

			const double vx = (v_circ + disp_phi) * (-std::sin(phi)) + disp_r * std::cos(phi);
			const double vy = (v_circ + disp_phi) * std::cos(phi) + disp_r * std::sin(phi);
			const double vz = rng.next_gaussian_pair(0.0, 0.05 * v_circ).first;

			CollisionlessParticle p;
			p.position = {r * std::cos(phi), r * std::sin(phi), z};
			p.velocity = {vx, vy, vz};
			p.mass = m_disk_part;
			p.softening = config.default_softening;
			p.type = 1;
			p.id = start_id + static_cast<uint32_t>(particles.size());
			particles.push_back(p);
		}

		const double m_bulge_part = (config.bulge_particle_count > 0) ? (config.bulge_mass / static_cast<double>(config.bulge_particle_count)) : 0.0;
		for (size_t i = 0; i < config.bulge_particle_count; ++i) {
			const double u = rng.next_uniform_double();
			const double r = config.bulge_scale_radius * std::sqrt(u) / (1.0 - std::sqrt(u) + 1e-6);
			const double cos_t = 2.0 * rng.next_uniform_double() - 1.0;
			const double sin_t = std::sqrt(std::max(1.0 - cos_t * cos_t, 0.0));
			const double phi = 2.0 * std::numbers::pi_v<double> * rng.next_uniform_double();

			const double v_disp = std::sqrt(g * config.bulge_mass / (config.bulge_scale_radius * 6.0));
			const auto [vx, vy] = rng.next_gaussian_pair(0.0, v_disp);
			const auto [vz, v_extra] = rng.next_gaussian_pair(0.0, v_disp);
			static_cast<void>(v_extra);

			CollisionlessParticle p;
			p.position = {r * sin_t * std::cos(phi), r * sin_t * std::sin(phi), r * cos_t};
			p.velocity = {vx, vy, vz};
			p.mass = m_bulge_part;
			p.softening = config.default_softening;
			p.type = 2;
			p.id = start_id + static_cast<uint32_t>(particles.size());
			particles.push_back(p);
		}

		const double m_halo_part = (config.halo_particle_count > 0) ? (config.halo_mass / static_cast<double>(config.halo_particle_count)) : 0.0;
		const double r_max_halo = 10.0 * config.halo_scale_radius;
		for (size_t i = 0; i < config.halo_particle_count; ++i) {
			double r = 0.0;
			while (true) {
				const double r_cand = r_max_halo * std::cbrt(rng.next_uniform_double());
				const double prob = (r_cand / config.halo_scale_radius) / std::pow(1.0 + r_cand / config.halo_scale_radius, 2);
				if (rng.next_uniform_double() < prob * 4.0) {
					r = r_cand;
					break;
				}
			}

			const double cos_t = 2.0 * rng.next_uniform_double() - 1.0;
			const double sin_t = std::sqrt(std::max(1.0 - cos_t * cos_t, 0.0));
			const double phi = 2.0 * std::numbers::pi_v<double> * rng.next_uniform_double();

			const double v_circ = compute_circular_velocity(r);
			const double v_disp = v_circ / std::numbers::sqrt2_v<double>;
			const auto [vx, vy] = rng.next_gaussian_pair(0.0, v_disp);
			const auto [vz, v_extra] = rng.next_gaussian_pair(0.0, v_disp);
			static_cast<void>(v_extra);

			CollisionlessParticle p;
			p.position = {r * sin_t * std::cos(phi), r * sin_t * std::sin(phi), r * cos_t};
			p.velocity = {vx, vy, vz};
			p.mass = m_halo_part;
			p.softening = config.default_softening * 2.0;
			p.type = 3;
			p.id = start_id + static_cast<uint32_t>(particles.size());
			particles.push_back(p);
		}

		const double cos_i = std::cos(inclination_rad);
		const double sin_i = std::sin(inclination_rad);
		const double cos_w = std::cos(node_rad);
		const double sin_w = std::sin(node_rad);

		for (auto& p : particles) {
			const double x0 = p.position[0];
			const double y0 = p.position[1];
			const double z0 = p.position[2];

			const double x1 = (cos_w * x0 - sin_w * y0);
			const double y1 = (sin_w * x0 + cos_w * y0);
			const double z1 = z0;

			const double x2 = x1;
			const double y2 = y1 * cos_i - z1 * sin_i;
			const double z2 = y1 * sin_i + z1 * cos_i;

			p.position[0] = x2 + center_pos[0];
			p.position[1] = y2 + center_pos[1];
			p.position[2] = z2 + center_pos[2];

			const double vx0 = p.velocity[0];
			const double vy0 = p.velocity[1];
			const double vz0 = p.velocity[2];

			const double vx1 = (cos_w * vx0 - sin_w * vy0);
			const double vy1 = (sin_w * vx0 + cos_w * vy0);
			const double vz1 = vz0;

			const double vx2 = vx1;
			const double vy2 = vy1 * cos_i - vz1 * sin_i;
			const double vz2 = vy1 * sin_i + vz1 * cos_i;

			p.velocity[0] = vx2 + center_vel[0];
			p.velocity[1] = vy2 + center_vel[1];
			p.velocity[2] = vz2 + center_vel[2];
		}

		return particles;
	}

	[[nodiscard]] static std::array<double, 3> compute_center_of_mass(std::span<const CollisionlessParticle> particles) noexcept {
		double m_tot = 0.0;
		std::array<double, 3> cm{0.0, 0.0, 0.0};
		for (const auto& p : particles) {
			m_tot += p.mass;
			cm[0] += p.mass * p.position[0];
			cm[1] += p.mass * p.position[1];
			cm[2] += p.mass * p.position[2];
		}
		if (m_tot > 0.0) {
			cm[0] /= m_tot;
			cm[1] /= m_tot;
			cm[2] /= m_tot;
		}
		return cm;
	}

	[[nodiscard]] static std::array<double, 3> compute_total_angular_momentum(std::span<const CollisionlessParticle> particles) noexcept {
		std::array<double, 3> l{0.0, 0.0, 0.0};
		for (const auto& p : particles) {
			l[0] += p.mass * (p.position[1] * p.velocity[2] - p.position[2] * p.velocity[1]);
			l[1] += p.mass * (p.position[2] * p.velocity[0] - p.position[0] * p.velocity[2]);
			l[2] += p.mass * (p.position[0] * p.velocity[1] - p.position[1] * p.velocity[0]);
		}
		return l;
	}
};

}

#pragma once

#include "relativistic/dynamics/pn_nbody_system.hpp"
#include <cmath>
#include <vector>
#include <algorithm>

namespace Relativistic::Dynamics {

class BulkBodyActions {
public:
	static void invert_all_velocities(PostNewtonianSystem& sys) noexcept {
		for (auto& body : sys.bodies()) {
			if (!body.enabled) continue;
			for (double& component : body.velocity) component = -component;
		}
		sys.update_accelerations();
	}

	static void zero_all_spins(PostNewtonianSystem& sys) noexcept {
		for (auto& body : sys.bodies()) {
			if (!body.enabled) continue;
			body.spin = {0.0, 0.0, 0.0};
		}
		sys.update_accelerations();
	}

	static void scatter_positions(PostNewtonianSystem& sys) noexcept {
		for (auto& body : sys.bodies()) {
			if (!body.enabled) continue;
			const double phase = static_cast<double>(body.id) * 2.399963229728653;
			const double radius = std::max(1.0, std::sqrt(body.position[0] * body.position[0] + body.position[1] * body.position[1] + body.position[2] * body.position[2]));
			body.position = {radius * std::cos(phase), radius * std::sin(phase), radius * 0.15 * std::sin(phase * 0.5)};
		}
		sys.update_accelerations();
	}

	static void snap_to_grid(PostNewtonianSystem& sys, double spacing) noexcept {
		const double safe_spacing = std::max(spacing, 1e-9);
		for (auto& body : sys.bodies()) {
			if (!body.enabled) continue;
			for (double& component : body.position) component = std::round(component / safe_spacing) * safe_spacing;
		}
		sys.update_accelerations();
	}

	static void equalize_masses(PostNewtonianSystem& sys) noexcept {
		double total_mass = 0.0;
		size_t enabled_count = 0;
		for (const auto& body : sys.bodies()) {
			if (!body.enabled) continue;
			total_mass += body.mass;
			++enabled_count;
		}
		if (enabled_count == 0) return;
		const double average_mass = total_mass / static_cast<double>(enabled_count);
		for (auto& body : sys.bodies()) {
			if (body.enabled) body.mass = average_mass;
		}
		sys.update_accelerations();
	}

	static void average_masses(PostNewtonianSystem& sys) noexcept {
		equalize_masses(sys);
	}

	static void cull_outside_radius(PostNewtonianSystem& sys, double max_radius) noexcept {
		std::vector<PostNewtonianBody> survivors;
		for (const auto& body : sys.bodies()) {
			const double r = std::sqrt(body.position[0] * body.position[0] + body.position[1] * body.position[1] + body.position[2] * body.position[2]);
			if (r <= max_radius) survivors.push_back(body);
		}
		sys.clear_bodies();
		for (auto& body : survivors) sys.add_body(body);
		sys.update_accelerations();
	}
};

}

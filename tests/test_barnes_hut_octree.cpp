#include "relativistic/dark_matter/barnes_hut.hpp"
#include "relativistic/core/pcg64.hpp"
#include <cassert>
#include <cmath>
#include <vector>

int main() {
	using namespace Relativistic::DarkMatter;

	Relativistic::Core::PCG64Engine rng(42ULL, 1ULL);
	const size_t n_particles = 300;
	std::vector<CollisionlessParticle> particles(n_particles);

	for (size_t i = 0; i < n_particles; ++i) {
		const auto [x, y] = rng.next_gaussian_pair(0.0, 50.0);
		const auto [z, vx] = rng.next_gaussian_pair(0.0, 50.0);
		const auto [vy, vz] = rng.next_gaussian_pair(0.0, 1.0);

		particles[i].position = {x, y, z};
		particles[i].velocity = {vx, vy, vz};
		particles[i].mass = 1000.0;
		particles[i].softening = 5.0;
		particles[i].id = static_cast<uint32_t>(i);
	}

	BarnesHutConfig config;
	config.opening_angle_theta = 0.5;
	config.default_softening = 5.0;
	config.gravitational_constant = 1.0;
	config.enable_quadrupole = true;

	BarnesHutOctree tree(config);
	tree.compute_all_accelerations(particles);

	assert(tree.node_count() > n_particles);

	for (size_t i = 0; i < 20; ++i) {
		std::array<double, 3> direct_acc{0.0, 0.0, 0.0};
		for (size_t j = 0; j < n_particles; ++j) {
			if (i == j) continue;
			const double dx = particles[j].position[0] - particles[i].position[0];
			const double dy = particles[j].position[1] - particles[i].position[1];
			const double dz = particles[j].position[2] - particles[i].position[2];
			const double r2 = dx * dx + dy * dy + dz * dz + 25.0;
			const double r = std::sqrt(r2);
			const double inv_r3 = 1.0 / (r2 * r);
			const double factor = config.gravitational_constant * particles[j].mass * inv_r3;
			direct_acc[0] += factor * dx;
			direct_acc[1] += factor * dy;
			direct_acc[2] += factor * dz;
		}

		const double err_x = std::abs(particles[i].acceleration[0] - direct_acc[0]);
		const double err_y = std::abs(particles[i].acceleration[1] - direct_acc[1]);
		const double err_z = std::abs(particles[i].acceleration[2] - direct_acc[2]);
		const double acc_mag = std::sqrt(direct_acc[0] * direct_acc[0] + direct_acc[1] * direct_acc[1] + direct_acc[2] * direct_acc[2]);

		if (acc_mag > 1e-6) {
			assert(err_x / acc_mag < 0.05);
			assert(err_y / acc_mag < 0.05);
			assert(err_z / acc_mag < 0.05);
		}
	}

	for (int step = 0; step < 10; ++step) {
		tree.step_leapfrog(particles, 0.01);
	}

	return 0;
}

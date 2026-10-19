#include "support/test_harness.hpp"
#include "relativistic/dark_matter/barnes_hut.hpp"
#include "relativistic/core/pcg64.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

using Relativistic::Testing::TestSuite;

int main() {
	using namespace Relativistic::DarkMatter;

	TestSuite suite(
		"barnes_hut_octree",
		"Verifies the Barnes-Hut octree on a deterministic Gaussian cloud of 300 softened particles: the tree contains more nodes than particles, the quadrupole-corrected accelerations of twenty particles agree with direct summation within 5 percent of the acceleration magnitude, and ten leapfrog steps keep every particle finite."
	);

	Relativistic::Core::PCG64Engine rng(42ULL, 1ULL);
	constexpr size_t particle_count = 300;
	std::vector<CollisionlessParticle> particles(particle_count);
	for (size_t i = 0; i < particle_count; ++i) {
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

	suite.section("Tree construction");
	tree.compute_all_accelerations(particles);
	suite.check(tree.node_count() > particle_count, "the octree holds more nodes than particles", "nodes=" + std::to_string(tree.node_count()));

	suite.section("Accuracy against direct summation");
	double worst_error = 0.0;
	size_t compared = 0;
	for (size_t i = 0; i < 20; ++i) {
		std::array<double, 3> direct{0.0, 0.0, 0.0};
		for (size_t j = 0; j < particle_count; ++j) {
			if (i == j) continue;
			const double dx = particles[j].position[0] - particles[i].position[0];
			const double dy = particles[j].position[1] - particles[i].position[1];
			const double dz = particles[j].position[2] - particles[i].position[2];
			const double r2 = dx * dx + dy * dy + dz * dz + 25.0;
			const double factor = config.gravitational_constant * particles[j].mass / (r2 * std::sqrt(r2));
			direct[0] += factor * dx;
			direct[1] += factor * dy;
			direct[2] += factor * dz;
		}
		const double magnitude = std::sqrt(direct[0] * direct[0] + direct[1] * direct[1] + direct[2] * direct[2]);
		if (magnitude > 1e-6) {
			const double error = std::max({std::abs(particles[i].acceleration[0] - direct[0]), std::abs(particles[i].acceleration[1] - direct[1]), std::abs(particles[i].acceleration[2] - direct[2])}) / magnitude;
			worst_error = std::max(worst_error, error);
			++compared;
		}
	}
	suite.check(compared == 20, "every sampled particle has a non-negligible direct acceleration", "compared=" + std::to_string(compared));
	suite.measure("worst_relative_acceleration_error", worst_error, "dimensionless");
	suite.check_at_most(worst_error, 0.05, "the tree acceleration agrees with direct summation within 5 percent of the magnitude");

	suite.section("Leapfrog integration");
	for (int step = 0; step < 10; ++step) {
		tree.step_leapfrog(particles, 0.01);
	}
	bool finite = true;
	for (const auto& particle : particles) {
		for (size_t axis = 0; axis < 3; ++axis) {
			finite = finite && std::isfinite(particle.position[axis]) && std::isfinite(particle.velocity[axis]);
		}
	}
	suite.check(finite, "every position and velocity stays finite after ten leapfrog steps");
	return suite.finish();
}

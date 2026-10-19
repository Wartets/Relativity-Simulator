#include "support/test_harness.hpp"
#include "relativistic/dark_matter/galaxy_model.hpp"
#include "relativistic/dark_matter/barnes_hut.hpp"
#include "relativistic/core/constants.hpp"
#include <array>
#include <cmath>
#include <vector>

using Relativistic::Testing::TestSuite;

int main() {
	using namespace Relativistic::DarkMatter;
	using namespace Relativistic::Core;

	TestSuite suite(
		"galaxy_collision_nbody",
		"Verifies a deterministic collision of two disk-bulge-halo galaxies of 500 particles each evolved for fifteen Barnes-Hut leapfrog steps: the centre of mass drifts by less than 1e19 m and the magnitude of the total angular momentum changes by less than 5 percent."
	);

	GalaxyComponentConfig first_config;
	first_config.disk_particle_count = 150;
	first_config.bulge_particle_count = 50;
	first_config.halo_particle_count = 300;
	GalaxyComponentConfig second_config = first_config;

	PCG64Engine rng(12345ULL, 1ULL);
	const double kpc = 1000.0 * 3.085677581491367e16;

	suite.section("Initial conditions");
	auto first = CompositeGalaxyGenerator::generate_galaxy(first_config, rng, {-50.0 * kpc, 0.0, 0.0}, {100.0e3, 50.0e3, 0.0}, 0.2, 0.0, 0);
	auto second = CompositeGalaxyGenerator::generate_galaxy(second_config, rng, {50.0 * kpc, 0.0, 0.0}, {-100.0e3, -50.0e3, 0.0}, -0.4, 0.5, static_cast<uint32_t>(first.size()));
	std::vector<CollisionlessParticle> particles;
	particles.insert(particles.end(), first.begin(), first.end());
	particles.insert(particles.end(), second.begin(), second.end());
	suite.check(particles.size() == 1000, "the two galaxies provide 1000 particles", "count=" + std::to_string(particles.size()));

	const auto initial_cm = CompositeGalaxyGenerator::compute_center_of_mass(particles);
	const auto initial_l = CompositeGalaxyGenerator::compute_total_angular_momentum(particles);

	suite.section("Collision evolution");
	BarnesHutConfig config;
	config.opening_angle_theta = 0.6;
	config.default_softening = 500.0 * 3.085677581491367e16;
	config.gravitational_constant = PhysicalConstants<double>::GRAVITATIONAL_CONSTANT;
	config.enable_quadrupole = true;
	BarnesHutOctree tree(config);
	const double dt = 5e13;
	for (int step = 0; step < 15; ++step) {
		tree.step_leapfrog(particles, dt);
	}

	const auto final_cm = CompositeGalaxyGenerator::compute_center_of_mass(particles);
	const auto final_l = CompositeGalaxyGenerator::compute_total_angular_momentum(particles);
	const double cm_drift = std::sqrt((final_cm[0] - initial_cm[0]) * (final_cm[0] - initial_cm[0]) + (final_cm[1] - initial_cm[1]) * (final_cm[1] - initial_cm[1]) + (final_cm[2] - initial_cm[2]) * (final_cm[2] - initial_cm[2]));
	const double l_initial = std::sqrt(initial_l[0] * initial_l[0] + initial_l[1] * initial_l[1] + initial_l[2] * initial_l[2]);
	const double l_final = std::sqrt(final_l[0] * final_l[0] + final_l[1] * final_l[1] + final_l[2] * final_l[2]);
	const double relative_l_change = std::abs(l_final - l_initial) / l_initial;

	suite.check_finite(cm_drift, "the centre of mass drift is finite");
	suite.check_finite(relative_l_change, "the angular momentum change is finite");
	suite.measure("centre_of_mass_drift", cm_drift, "m");
	suite.measure("angular_momentum_change", relative_l_change, "relative");
	suite.check_at_most(cm_drift, 1e19, "the centre of mass drifts by less than 1e19 m");
	suite.check_at_most(relative_l_change, 0.05, "the total angular momentum changes by less than 5 percent");
	return suite.finish();
}

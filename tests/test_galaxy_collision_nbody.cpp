#include "relativistic/dark_matter/galaxy_model.hpp"
#include "relativistic/dark_matter/barnes_hut.hpp"
#include "relativistic/core/constants.hpp"
#include <cassert>
#include <cmath>
#include <vector>

int main() {
	using namespace Relativistic::DarkMatter;
	using namespace Relativistic::Core;

	GalaxyComponentConfig cfg1;
	cfg1.disk_particle_count = 150;
	cfg1.bulge_particle_count = 50;
	cfg1.halo_particle_count = 300;

	GalaxyComponentConfig cfg2 = cfg1;

	PCG64Engine rng(12345ULL, 1ULL);

	const double kpc = 1000.0 * 3.085677581491367e16;
	const std::array<double, 3> pos1 = {-50.0 * kpc, 0.0, 0.0};
	const std::array<double, 3> vel1 = {100.0 * 1000.0, 50.0 * 1000.0, 0.0};

	const std::array<double, 3> pos2 = {50.0 * kpc, 0.0, 0.0};
	const std::array<double, 3> vel2 = {-100.0 * 1000.0, -50.0 * 1000.0, 0.0};

	auto g1 = CompositeGalaxyGenerator::generate_galaxy(cfg1, rng, pos1, vel1, 0.2, 0.0, 0);
	auto g2 = CompositeGalaxyGenerator::generate_galaxy(cfg2, rng, pos2, vel2, -0.4, 0.5, static_cast<uint32_t>(g1.size()));

	std::vector<CollisionlessParticle> system_particles;
	system_particles.insert(system_particles.end(), g1.begin(), g1.end());
	system_particles.insert(system_particles.end(), g2.begin(), g2.end());

	const auto init_cm = CompositeGalaxyGenerator::compute_center_of_mass(system_particles);
	const auto init_l = CompositeGalaxyGenerator::compute_total_angular_momentum(system_particles);

	BarnesHutConfig bh_cfg;
	bh_cfg.opening_angle_theta = 0.6;
	bh_cfg.default_softening = 500.0 * 3.085677581491367e16;
	bh_cfg.gravitational_constant = PhysicalConstants<double>::GRAVITATIONAL_CONSTANT;
	bh_cfg.enable_quadrupole = true;

	BarnesHutOctree tree(bh_cfg);

	const double dt = 5e13;
	for (int step = 0; step < 15; ++step) {
		tree.step_leapfrog(system_particles, dt);
	}

	const auto final_cm = CompositeGalaxyGenerator::compute_center_of_mass(system_particles);
	const auto final_l = CompositeGalaxyGenerator::compute_total_angular_momentum(system_particles);

	const double cm_drift = std::sqrt(
		(final_cm[0] - init_cm[0]) * (final_cm[0] - init_cm[0]) +
		(final_cm[1] - init_cm[1]) * (final_cm[1] - init_cm[1]) +
		(final_cm[2] - init_cm[2]) * (final_cm[2] - init_cm[2])
	);

	const double l_mag_init = std::sqrt(init_l[0] * init_l[0] + init_l[1] * init_l[1] + init_l[2] * init_l[2]);
	const double l_mag_final = std::sqrt(final_l[0] * final_l[0] + final_l[1] * final_l[1] + final_l[2] * final_l[2]);
	const double l_diff = std::abs(l_mag_final - l_mag_init);

	assert(cm_drift < 1e19);
	assert(l_diff / l_mag_init < 0.05);

	return 0;
}

#include "relativistic/optics/terrell_penrose.hpp"
#include <cassert>
#include <cmath>
#include <numbers>
#include <iostream>

void test_aberration_identity_and_inversion() {
	using namespace Relativistic::Optics;

	const std::array<double, 3> beta = {0.5, 0.0, 0.0};
	const std::array<double, 3> n_original = {0.0, 1.0, 0.0};

	const auto n_obs = TerrellPenroseOptics<double>::aberrate_forward(n_original, beta);
	const auto n_reconstructed = TerrellPenroseOptics<double>::aberrate_backward(n_obs, beta);

	assert(std::abs(n_reconstructed[0] - n_original[0]) < 1e-12);
	assert(std::abs(n_reconstructed[1] - n_original[1]) < 1e-12);
	assert(std::abs(n_reconstructed[2] - n_original[2]) < 1e-12);

	const std::array<double, 3> beta_diag = {0.2, 0.3, 0.4};
	const double inv_sqrt3 = 1.0 / std::sqrt(3.0);
	const std::array<double, 3> n_diag = {inv_sqrt3, inv_sqrt3, inv_sqrt3};

	const auto n_obs_diag = TerrellPenroseOptics<double>::aberrate_forward(n_diag, beta_diag);
	const auto n_reconstructed_diag = TerrellPenroseOptics<double>::aberrate_backward(n_obs_diag, beta_diag);

	assert(std::abs(n_reconstructed_diag[0] - n_diag[0]) < 1e-12);
	assert(std::abs(n_reconstructed_diag[1] - n_diag[1]) < 1e-12);
	assert(std::abs(n_reconstructed_diag[2] - n_diag[2]) < 1e-12);
}

void test_terrell_rotation_sphere_geometry() {
	using namespace Relativistic::Optics;

	const std::array<double, 3> obs_pos = {0.0, 0.0, 0.0};
	const std::array<double, 3> sphere_c0 = {0.0, 10.0, 0.0};
	const std::array<double, 3> sphere_vel = {0.8, 0.0, 0.0};
	const double radius = 1.0;
	const double t_obs = 10.0;

	const std::array<double, 3> ray_center = {0.0, 1.0, 0.0};
	const auto hit_opt = TerrellPenroseOptics<double>::intersect_moving_sphere(
		obs_pos, ray_center, t_obs, sphere_c0, sphere_vel, radius
	);

	assert(hit_opt.has_value());
	assert(hit_opt->doppler_shift > 0.0);

	const double angle_rad = TerrellPenroseOptics<double>::rotation_angle(0.8);
	assert(std::abs(std::sin(angle_rad) - 0.8) < 1e-12);
}

int main() {
	test_aberration_identity_and_inversion();
	test_terrell_rotation_sphere_geometry();
	std::cout << "Terrell-Penrose optics tests passed.\n";
	return 0;
}

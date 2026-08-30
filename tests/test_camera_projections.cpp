#include "relativistic/observer/camera_projections.hpp"
#include "relativistic/observer/observer_tetrad.hpp"
#include "relativistic/metrics/flat_minkowski.hpp"
#include <cassert>
#include <iostream>
#include <cmath>
#include <numbers>

void test_pinhole_projection() {
	using namespace Relativistic::Observer;

	const double fov = 90.0 * std::numbers::pi / 180.0;
	const auto center_ray = CameraProjector<double>::compute_pinhole_ray(0.0, 0.0, fov);
	assert(std::abs(center_ray[0] - 1.0) < 1e-14);
	assert(std::abs(center_ray[1]) < 1e-14);
	assert(std::abs(center_ray[2]) < 1e-14);

	const auto corner_ray = CameraProjector<double>::compute_pinhole_ray(1.0, 1.0, fov);
	const double norm = std::sqrt(corner_ray[0] * corner_ray[0] + corner_ray[1] * corner_ray[1] + corner_ray[2] * corner_ray[2]);
	assert(std::abs(norm - 1.0) < 1e-14);
	assert(corner_ray[2] > 0.0);
	assert(corner_ray[1] < 0.0);
}

void test_auto_zoom_compensation() {
	using namespace Relativistic::Observer;

	const double fov = 60.0 * std::numbers::pi / 180.0;
	const double gamma = 50.0;
	const double beta = std::sqrt(1.0 - 1.0 / (gamma * gamma));

	const auto ray_no_zoom = CameraProjector<double>::compute_pinhole_ray(1.0, 0.0, fov);
	const auto ray_auto_zoom = CameraProjector<double>::compute_auto_zoom_ray(1.0, 0.0, fov, gamma, beta);

	const double angle_no_zoom = std::atan2(ray_no_zoom[2], ray_no_zoom[0]);
	const double angle_auto_zoom = std::atan2(ray_auto_zoom[2], ray_auto_zoom[0]);

	assert(angle_auto_zoom < angle_no_zoom);
	const double expected_ratio = std::sqrt((1.0 - beta) / (1.0 + beta));
	const double actual_ratio = std::tan(angle_auto_zoom) / std::tan(angle_no_zoom);
	assert(std::abs(actual_ratio - expected_ratio) < 1e-7);
}

void test_stereographic_fisheye_projection() {
	using namespace Relativistic::Observer;

	const double fov_220 = 220.0 * std::numbers::pi / 180.0;
	const auto edge_ray = CameraProjector<double>::compute_fisheye_stereographic_ray(1.0, 0.0, fov_220);

	const double norm = std::sqrt(edge_ray[0] * edge_ray[0] + edge_ray[1] * edge_ray[1] + edge_ray[2] * edge_ray[2]);
	assert(std::abs(norm - 1.0) < 1e-14);

	const double theta = std::acos(edge_ray[0]);
	assert(theta > std::numbers::pi / 2.0);
	assert(std::abs(theta - 110.0 * std::numbers::pi / 180.0) < 1e-12);
}

void test_equirectangular_360_projection() {
	using namespace Relativistic::Observer;

	const auto fwd = CameraProjector<double>::compute_equirectangular_360_ray(0.0, 0.0);
	assert(std::abs(fwd[0] - 1.0) < 1e-14);
	assert(std::abs(fwd[1]) < 1e-14);
	assert(std::abs(fwd[2]) < 1e-14);

	const auto back = CameraProjector<double>::compute_equirectangular_360_ray(1.0, 0.0);
	assert(std::abs(back[0] - (-1.0)) < 1e-14);
	assert(std::abs(back[1]) < 1e-14);

	const auto top = CameraProjector<double>::compute_equirectangular_360_ray(0.0, 1.0);
	assert(std::abs(top[1] - 1.0) < 1e-14);

	const auto bottom = CameraProjector<double>::compute_equirectangular_360_ray(0.0, -1.0);
	assert(std::abs(bottom[1] - (-1.0)) < 1e-14);
}

void test_tetrad_ray_generation() {
	using namespace Relativistic;
	Metrics::FlatMinkowskiMetric<double> metric(1.0);
	Core::FourVector<double> pos(0.0, 10.0, 0.0, 0.0);
	auto tetrad = Observer::ObserverTetrad<double>::make_stationary(metric, pos);

	const auto ray_pinhole = tetrad.construct_projected_ray(Observer::ProjectionMode::Pinhole, 0.5, 0.5, 1.0);
	const double norm_pinhole = -(ray_pinhole(0) * ray_pinhole(0)) + (ray_pinhole(1) * ray_pinhole(1) + ray_pinhole(2) * ray_pinhole(2) + ray_pinhole(3) * ray_pinhole(3));
	assert(std::abs(norm_pinhole) < 1e-14);

	const auto ray_fish = tetrad.construct_projected_ray(Observer::ProjectionMode::FisheyeStereographic, 0.8, 0.8, 3.5);
	const double norm_fish = -(ray_fish(0) * ray_fish(0)) + (ray_fish(1) * ray_fish(1) + ray_fish(2) * ray_fish(2) + ray_fish(3) * ray_fish(3));
	assert(std::abs(norm_fish) < 1e-14);

	const auto ray_360 = tetrad.construct_projected_ray(Observer::ProjectionMode::Equirectangular360, 0.7, -0.3, 0.0);
	const double norm_360 = -(ray_360(0) * ray_360(0)) + (ray_360(1) * ray_360(1) + ray_360(2) * ray_360(2) + ray_360(3) * ray_360(3));
	assert(std::abs(norm_360) < 1e-14);
}

int main() {
	test_pinhole_projection();
	test_auto_zoom_compensation();
	test_stereographic_fisheye_projection();
	test_equirectangular_360_projection();
	test_tetrad_ray_generation();
	std::cout << "All camera projection tests passed successfully.\n";
	return 0;
}

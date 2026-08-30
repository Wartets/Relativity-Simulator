#include "relativistic/observer/observer_tetrad.hpp"
#include "relativistic/metrics/schwarzschild.hpp"
#include "relativistic/metrics/kerr.hpp"
#include <cassert>
#include <iostream>
#include <cmath>

int main() {
	using namespace Relativistic::Observer;
	using namespace Relativistic::Metrics;
	using namespace Relativistic::Core;

	const SchwarzschildMetric<double> s_metric(1.0);
	const FourVector<double> s_pos(0.0, 10.0, std::numbers::pi_v<double> / 2.0, 0.0);

	const auto s_tetrad = ObserverTetrad<double>::make_stationary(s_metric, s_pos);
	assert(s_tetrad.check_orthonormality(s_metric, 1e-9));

	const auto light_ray = s_tetrad.construct_light_ray(1.0, 0.0, 0.0);
	const auto g_s = s_metric.metric_tensor(s_pos);
	double norm_ray = 0.0;
	for (size_t i = 0; i < 4; ++i) {
		for (size_t j = 0; j < 4; ++j) {
			norm_ray += g_s(i, j) * light_ray(i) * light_ray(j);
		}
	}
	assert(std::abs(norm_ray) < 1e-9);

	const KerrMetric<double> k_metric(1.0, 0.9);
	const FourVector<double> k_pos(0.0, 6.0, std::numbers::pi_v<double> / 3.0, 0.0);
	const auto k_tetrad = ObserverTetrad<double>::make_zamo(k_metric, k_pos);
	assert(k_tetrad.check_orthonormality(k_metric, 1e-8));

	const auto pinhole_ray = k_tetrad.construct_pinhole_ray(0.5, -0.5, 1.0471975511965976);
	const auto g_k = k_metric.metric_tensor(k_pos);
	double norm_pinhole = 0.0;
	for (size_t i = 0; i < 4; ++i) {
		for (size_t j = 0; j < 4; ++j) {
			norm_pinhole += g_k(i, j) * pinhole_ray(i) * pinhole_ray(j);
		}
	}
	assert(std::abs(norm_pinhole) < 1e-8);

	std::cout << "test_observer_tetrad passed successfully.\n";
	return 0;
}

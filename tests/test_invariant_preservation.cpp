#include "relativistic/metrics/schwarzschild.hpp"
#include "relativistic/metrics/painleve_gullstrand.hpp"
#include "relativistic/integrators/rk45_adaptive.hpp"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <numbers>

using namespace Relativistic::Metrics;
using namespace Relativistic::Integrators;
using namespace Relativistic::Core;

int main() {
	constexpr double c = 1.0;
	constexpr double G = 1.0;
	constexpr double M = 1.0;

	SchwarzschildMetric<double> schw(M, c, G);
	const double r_s = schw.schwarzschild_radius();

	const double r_orb = 10.0 * r_s;
	const double v_circ = std::sqrt(G * M / r_orb);
	const double gamma_v = 1.0 / std::sqrt(1.0 - 3.0 * G * M / r_orb);

	GeodesicState<double> particle;
	particle.x = FourVector<double>(0.0, r_orb, std::numbers::pi_v<double> / 2.0, 0.0);
	particle.u = FourVector<double>(gamma_v, 0.0, 0.0, gamma_v * v_circ / r_orb);

	RK45Config<double> config;
	config.initial_step = 0.1;
	config.min_step = 1e-8;
	config.max_step = 10.0;
	config.rtol = 1e-10;
	config.atol = 1e-14;
	config.invariant_tolerance = 1e-12;

	RK45AdaptiveIntegrator<SchwarzschildMetric<double>, double> integrator(schw, GeodesicType::Timelike, config);

	double dt = config.initial_step;
	for (size_t step = 0; step < 50000; ++step) {
		const auto res = integrator.step(particle, dt);
		if (!res.has_value()) {
			std::cerr << "Timelike orbit integration step failed." << std::endl;
			return 1;
		}
	}

	const auto& stats = integrator.statistics();
	std::cout << "Timelike Orbit Max Invariant Error : " << std::scientific << stats.max_invariant_residual << std::endl;

	if (stats.max_invariant_residual > 1e-11) {
		std::cerr << "Timelike invariant drifted above tolerance." << std::endl;
		return 1;
	}

	PainleveGullstrandMetric<double> pg(M, c, G);
	GeodesicState<double> infalling;
	infalling.x = FourVector<double>(0.0, 4.0 * r_s, std::numbers::pi_v<double> / 2.0, 0.0);
	infalling.u = FourVector<double>(1.0, -std::sqrt(r_s / (4.0 * r_s)), 0.0, 0.0);

	RK45AdaptiveIntegrator<PainleveGullstrandMetric<double>, double> pg_integrator(pg, GeodesicType::Timelike, config);
	dt = 0.05;

	bool crossed_horizon = false;
	for (size_t step = 0; step < 20000; ++step) {
		const auto res = pg_integrator.step(infalling, dt);
		if (!res.has_value()) {
			std::cerr << "Infalling geodesic step failed." << std::endl;
			return 1;
		}
		if (infalling.x(1) < r_s) {
			crossed_horizon = true;
		}
		if (infalling.x(1) < 0.2 * r_s) {
			break;
		}
	}

	const auto& pg_stats = pg_integrator.statistics();
	std::cout << "Horizon Infall Max Invariant Error : " << std::scientific << pg_stats.max_invariant_residual << std::endl;
	std::cout << "Horizon Crossing Status            : " << (crossed_horizon ? "SUCCESS" : "FAILED") << std::endl;

	if (!crossed_horizon || pg_stats.max_invariant_residual > 1e-10) {
		std::cerr << "Horizon crossing failed or invariant violated." << std::endl;
		return 1;
	}

	std::cout << "INVARIANT PRESERVATION TEST: ALL PASSED" << std::endl;
	return 0;
}

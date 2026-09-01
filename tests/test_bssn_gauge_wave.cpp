#include "relativistic/metrics/bssn_grid.hpp"
#include "relativistic/metrics/bssn_evolution.hpp"
#include "relativistic/metrics/bssn_constraints.hpp"
#include <iostream>
#include <cmath>
#include <numbers>

using namespace Relativistic::Metrics;

int main() {
	const size_t nx = 64;
	const size_t ny = 4;
	const size_t nz = 4;
	const double dx = 1.0 / static_cast<double>(nx);
	const double dy = 1.0 / static_cast<double>(ny);
	const double dz = 1.0 / static_cast<double>(nz);

	BssnGrid grid(nx, ny, nz, dx, dy, dz);

	const double amplitude = 0.1;
	const double pi = std::numbers::pi_v<double>;

	for (size_t k = 0; k < nz; ++k) {
		for (size_t j = 0; j < ny; ++j) {
			for (size_t i = 0; i < nx; ++i) {
				const size_t idx = grid.index(static_cast<int>(i), static_cast<int>(j), static_cast<int>(k));
				const double x = static_cast<double>(i) * dx;

				const double H = 1.0 - amplitude * std::sin(2.0 * pi * x);
				const double dH_dx = -2.0 * pi * amplitude * std::cos(2.0 * pi * x);

				grid.gt11[idx] = std::pow(H, 2.0 / 3.0);
				grid.gt22[idx] = std::pow(H, -1.0 / 3.0);
				grid.gt33[idx] = std::pow(H, -1.0 / 3.0);

				grid.phi[idx] = (1.0 / 12.0) * std::log(H);
				grid.alpha[idx] = std::sqrt(H);

				grid.K[idx] = dH_dx / (2.0 * std::pow(H, 1.5));
			}
		}
	}

	BssnEvolution evolution(nx, ny, nz, dx, dy, dz);
	const double dt = 0.25 * dx;

	std::cout << "Starting BSSN Gauge Wave Evolution..." << std::endl;

	double max_H_initial = 0.0;
	auto H_init = BssnConstraints::compute_hamiltonian(grid);
	for (double val : H_init) {
		max_H_initial = std::max(max_H_initial, std::abs(val));
	}
	std::cout << "Initial Hamiltonian Constraint L_inf: " << max_H_initial << std::endl;

	for (int step = 1; step <= 1000; ++step) {
		evolution.step_rk4(grid, dt);

		if (step % 100 == 0) {
			auto H_vals = BssnConstraints::compute_hamiltonian(grid);
			double max_H = 0.0;
			for (double val : H_vals) {
				max_H = std::max(max_H, std::abs(val));
			}
			std::cout << "Step " << step << " | Hamiltonian Constraint L_inf: " << max_H << std::endl;
		}
	}

	std::cout << "BSSN Evolution completed successfully." << std::endl;
	return 0;
}

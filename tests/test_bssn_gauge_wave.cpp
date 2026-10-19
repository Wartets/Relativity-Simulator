#include "support/test_harness.hpp"
#include "relativistic/metrics/bssn/bssn_grid.hpp"
#include "relativistic/metrics/bssn/bssn_evolution.hpp"
#include "relativistic/metrics/bssn/bssn_constraints.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <string>

using namespace Relativistic::Metrics;
using Relativistic::Testing::TestSuite;

namespace {

struct ConstraintSummary {
	double maximum{0.0};
	bool finite{true};
};

[[nodiscard]] ConstraintSummary summarize(const BssnGrid& grid) {
	ConstraintSummary summary;
	for (const double value : BssnConstraints::compute_hamiltonian(grid)) {
		summary.finite = summary.finite && std::isfinite(value);
		summary.maximum = std::max(summary.maximum, std::abs(value));
	}
	return summary;
}

}

int main() {
	TestSuite suite(
		"bssn_gauge_wave",
		"Verifies the BSSN evolution on the one-dimensional gauge wave of amplitude 0.1 on a 64 by 4 by 4 grid evolved for one thousand Runge-Kutta steps of 0.25 dx: the Hamiltonian constraint stays finite at every checkpoint, the lapse stays positive, and the constraint norm grows by less than a factor of 100."
	);

	constexpr size_t nx = 64;
	constexpr size_t ny = 4;
	constexpr size_t nz = 4;
	const double dx = 1.0 / static_cast<double>(nx);
	const double dy = 1.0 / static_cast<double>(ny);
	const double dz = 1.0 / static_cast<double>(nz);
	BssnGrid grid(nx, ny, nz, dx, dy, dz);

	const double amplitude = 0.1;
	const double pi = std::numbers::pi_v<double>;
	for (size_t k = 0; k < nz; ++k) {
		for (size_t j = 0; j < ny; ++j) {
			for (size_t i = 0; i < nx; ++i) {
				const size_t index = grid.index(static_cast<int>(i), static_cast<int>(j), static_cast<int>(k));
				const double x = static_cast<double>(i) * dx;
				const double h = 1.0 - amplitude * std::sin(2.0 * pi * x);
				const double dh = -2.0 * pi * amplitude * std::cos(2.0 * pi * x);
				grid.gt11[index] = std::pow(h, 2.0 / 3.0);
				grid.gt22[index] = std::pow(h, -1.0 / 3.0);
				grid.gt33[index] = std::pow(h, -1.0 / 3.0);
				grid.phi[index] = (1.0 / 12.0) * std::log(h);
				grid.alpha[index] = std::sqrt(h);
				grid.K[index] = dh / (2.0 * std::pow(h, 1.5));
			}
		}
	}

	suite.section("Initial data");
	const auto initial = summarize(grid);
	suite.check(initial.finite, "the initial Hamiltonian constraint is finite");
	suite.measure("initial_hamiltonian_linf", initial.maximum, "dimensionless");

	suite.section("Evolution");
	BssnEvolution evolution(nx, ny, nz, dx, dy, dz);
	const double dt = 0.25 * dx;
	bool finite = true;
	double peak = initial.maximum;
	for (int step = 1; step <= 1000; ++step) {
		evolution.step_rk4(grid, dt);
		if (step % 100 == 0) {
			const auto current = summarize(grid);
			finite = finite && current.finite;
			peak = std::max(peak, current.maximum);
			suite.measure("hamiltonian_linf_step_" + std::to_string(step), current.maximum, "dimensionless");
		}
	}
	suite.check(finite, "the Hamiltonian constraint stays finite at every checkpoint");
	double minimum_lapse = 1e300;
	bool lapse_finite = true;
	for (const double value : grid.alpha) {
		lapse_finite = lapse_finite && std::isfinite(value);
		minimum_lapse = std::min(minimum_lapse, value);
	}
	suite.check(lapse_finite && minimum_lapse > 0.0, "the lapse stays finite and positive everywhere", "minimum=" + Relativistic::Testing::format_number(minimum_lapse, 6));
	suite.measure("peak_hamiltonian_linf", peak, "dimensionless");
	suite.check_at_most(peak, 100.0 * std::max(initial.maximum, 1e-6), "the constraint norm never exceeds one hundred times its initial value");
	return suite.finish();
}

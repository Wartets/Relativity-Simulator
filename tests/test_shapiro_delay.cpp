#include "relativistic/core/constants.hpp"
#include "relativistic/core/tensor.hpp"
#include "relativistic/metrics/schwarzschild.hpp"
#include "relativistic/integrators/symplectic_gauss_legendre.hpp"
#include <iostream>
#include <cmath>
#include <numbers>
#include <cassert>
#include <iomanip>
#include <algorithm>

int main() {
	using namespace Relativistic::Core;
	using namespace Relativistic::Metrics;
	using namespace Relativistic::Integrators;

	const double M = 1.0;
	const double c = 1.0;
	const double G = 1.0;
	const double r_s = 2.0 * G * M / (c * c);

	const double r0 = 1000.0;
	const double r_earth = 50000.0;
	const double r_probe = 75000.0;

	const double b_impact = r0 / std::sqrt(1.0 - r_s / r0);

	const double delta_t_shapiro_theoretical = (r_s / c) * (
		std::log((r_earth + std::sqrt(r_earth * r_earth - r0 * r0)) / r0) +
		std::log((r_probe + std::sqrt(r_probe * r_probe - r0 * r0)) / r0) +
		0.5 * std::sqrt((r_earth - r0) / (r_earth + r0)) +
		0.5 * std::sqrt((r_probe - r0) / (r_probe + r0))
	);

	const double delta_t_round_trip_theoretical = 2.0 * (r_s / c) * (
		std::log((4.0 * r_earth * r_probe) / (r0 * r0)) + 1.0
	);

	SchwarzschildMetric<double> schw(M, c, G);
	GaussLegendre6<SchwarzschildMetric<double>, double> integrator(schw);

	const double phi_earth = -std::acos(std::clamp(r0 / r_earth, -1.0, 1.0));
	const double u_t_earth = 1.0 / (1.0 - r_s / r_earth);
	const double u_phi_earth = (b_impact * c) / (r_earth * r_earth);
	const double u_r_earth_sq = c * c - (1.0 - r_s / r_earth) * (b_impact * b_impact * c * c) / (r_earth * r_earth);
	const double u_r_earth = -std::sqrt(std::max(0.0, u_r_earth_sq));

	GeodesicState<double> state;
	state.x = FourVector<double>(0.0, r_earth, std::numbers::pi_v<double> / 2.0, phi_earth);
	state.u = FourVector<double>(u_t_earth, u_r_earth, 0.0, u_phi_earth);

	const double dt_nominal = 5.0;

	while (state.u(1) < 0.0 || state.x(1) < r_probe) {
		double dt = dt_nominal;

		if (state.x(1) < r0 * 1.5) {
			dt = 0.5;
		}

		if (state.u(1) > 0.0 && (r_probe - state.x(1)) < state.u(1) * dt) {
			dt = (r_probe - state.x(1)) / state.u(1);
			const bool ok = integrator.step(state, dt);
			assert(ok);
			break;
		}

		const bool ok = integrator.step(state, dt);
		assert(ok);
	}

	for (size_t ref = 0; ref < 5 && std::abs(state.x(1) - r_probe) > 1e-9; ++ref) {
		const double fine_dt = (r_probe - state.x(1)) / state.u(1);
		const bool ok = integrator.step(state, fine_dt);
		assert(ok);
	}

	const double t_arrival = state.x(0);

	auto quad_segment_regularized = [&](double r_end) noexcept {
		const double w_max = std::sqrt(r_end - r0);
		const size_t n_panels = 500;
		const double dw = w_max / static_cast<double>(n_panels);
		double integral = 0.0;

		for (size_t p = 0; p < n_panels; ++p) {
			const double wa = static_cast<double>(p) * dw;
			const double wb = wa + dw;
			const double wm = 0.5 * (wa + wb);
			const double wr = 0.5 * dw;

			constexpr double p1 = 0.8611363115940526;
			constexpr double p2 = 0.3399810435848563;
			constexpr double w1 = 0.3478548451374538;
			constexpr double w2 = 0.6521451548625461;

			const auto eval_g = [&](double w) noexcept {
				const double r = r0 + w * w;
				const double one_minus_rs_r0 = 1.0 - r_s / r0;
				const double poly = (r + r0) - r_s * (r * r + r * r0 + r0 * r0) / (r * r0);
				const double denom = c * (1.0 - r_s / r) * std::sqrt(std::max(1e-30, poly));
				return (2.0 * r * std::sqrt(one_minus_rs_r0)) / denom;
			};

			integral += wr * (
				w1 * eval_g(wm - wr * p1) +
				w2 * eval_g(wm - wr * p2) +
				w2 * eval_g(wm + wr * p2) +
				w1 * eval_g(wm + wr * p1)
			);
		}
		return integral;
	};

	const double t_theo_exact = quad_segment_regularized(r_earth) + quad_segment_regularized(r_probe);
	const double t_flat = (std::sqrt(r_earth * r_earth - r0 * r0) + std::sqrt(r_probe * r_probe - r0 * r0)) / c;
	const double delta_t_shapiro_numerical = t_arrival - t_flat;
	const double delta_t_round_trip_numerical = 2.0 * delta_t_shapiro_numerical;
	const double relative_residual = std::abs(t_arrival - t_theo_exact) / t_arrival;

	std::cout << std::setprecision(14);
	std::cout << "Shapiro Time Delay Validation (Symplectic Gauss-Legendre Order 6):\n";
	std::cout << "  Theoretical Shapiro delay (1-way): " << delta_t_shapiro_theoretical << " GM/c^3\n";
	std::cout << "  Numerical Shapiro delay (1-way):   " << delta_t_shapiro_numerical << " GM/c^3\n";
	std::cout << "  Theoretical Shapiro delay (2-way): " << delta_t_round_trip_theoretical << " GM/c^3\n";
	std::cout << "  Numerical Shapiro delay (2-way):   " << delta_t_round_trip_numerical << " GM/c^3\n";
	std::cout << "  Exact GR coordinate flight time:   " << t_theo_exact << " GM/c^3\n";
	std::cout << "  Integrated geodesic flight time:   " << t_arrival << " GM/c^3\n";
	std::cout << "  Relative residual:                 " << relative_residual << "\n";

	assert(relative_residual < 1e-11);
	std::cout << "Scientific Validation *Shapiro Delay* PASSED.\n";

	return 0;
}

#include "relativistic/core/constants.hpp"
#include "relativistic/core/tensor.hpp"
#include "relativistic/metrics/schwarzschild.hpp"
#include "relativistic/integrators/symplectic_gauss_legendre.hpp"
#include "relativistic/integrators/cash_karp.hpp"
#include "relativistic/integrators/vernier9.hpp"
#include <iostream>
#include <cmath>
#include <numbers>
#include <cassert>
#include <iomanip>

int main() {
	using namespace Relativistic::Core;
	using namespace Relativistic::Metrics;
	using namespace Relativistic::Integrators;

	const double M = 1.0;
	const double c = 1.0;
	const double G = 1.0;
	const double r_s = 2.0 * G * M / (c * c);

	const double a = 1000.0 * r_s;
	const double e = 0.205630;
	const double r_p = a * (1.0 - e);
	const double r_a = a * (1.0 + e);

	const double num_l2 = 2.0 * G * M * r_a * r_a * r_p * r_p;
	const double den_l2 = (r_a + r_p) * r_a * r_p - r_s * (r_a * r_a + r_a * r_p + r_p * r_p);
	const double L = std::sqrt(num_l2 / den_l2);

	const double E_sq = (1.0 - r_s / r_p) * (1.0 + (L * L) / (r_p * r_p));
	const double E = std::sqrt(E_sq);

	const double u_t = E / (1.0 - r_s / r_p);
	const double u_phi = L / (r_p * r_p);

	SchwarzschildMetric<double> schw(M, c, G);
	GaussLegendre6<SchwarzschildMetric<double>, double> integrator(schw);

	GeodesicState<double> state;
	state.x = FourVector<double>(0.0, r_p, std::numbers::pi_v<double> / 2.0, 0.0);
	state.u = FourVector<double>(u_t, 0.0, 0.0, u_phi);

	const double u1 = 1.0 / r_a;
	const double u2 = 1.0 / r_p;
	const double u3 = 1.0 / r_s - u1 - u2;
	const double k_sq = (u2 - u1) / (u3 - u1);
	const double k_val = std::sqrt(k_sq);

	double agm_a = 1.0;
	double agm_g = std::sqrt(1.0 - k_val * k_val);
	for (size_t iter = 0; iter < 10; ++iter) {
		const double next_a = 0.5 * (agm_a + agm_g);
		const double next_g = std::sqrt(agm_a * agm_g);
		agm_a = next_a;
		agm_g = next_g;
	}
	const double elliptic_k = std::numbers::pi_v<double> / (2.0 * agm_a);
	const double exact_delta_phi_per_rev = (4.0 * elliptic_k) / std::sqrt(r_s * (u3 - u1)) - 2.0 * std::numbers::pi_v<double>;

	const size_t target_revolutions = 100;
	const double theoretical_total_precession = static_cast<double>(target_revolutions) * exact_delta_phi_per_rev;

	const double orbital_period_tau = 2.0 * std::numbers::pi_v<double> * std::sqrt((a * a * a) / (G * M));
	const double dt = orbital_period_tau / 1200.0;

	size_t perihelion_count = 0;
	double last_phi = 0.0;

	while (perihelion_count < target_revolutions) {
		const double dr_before = state.u(1);
		const double phi_before = state.x(3);
		const double r_before = state.x(1);

		const bool ok = integrator.step(state, dt);
		assert(ok);

		const double dr_after = state.u(1);
		const double phi_after = state.x(3);

		if (dr_before < 0.0 && dr_after >= 0.0 && r_before < a) {
			const double frac = -dr_before / (dr_after - dr_before);
			const double phi_peri = phi_before + frac * (phi_after - phi_before);
			++perihelion_count;
			last_phi = phi_peri;
		}
	}

	const double numerical_total_precession = last_phi - static_cast<double>(target_revolutions) * 2.0 * std::numbers::pi_v<double>;
	const double relative_error = std::abs(numerical_total_precession - theoretical_total_precession) / theoretical_total_precession;

	std::cout << std::setprecision(14);
	std::cout << "Mercury Precession Validation over 100 revolutions:\n";
	std::cout << "  Theoretical precession: " << theoretical_total_precession << " rad\n";
	std::cout << "  Numerical precession:   " << numerical_total_precession << " rad\n";
	std::cout << "  Relative error:         " << relative_error << "\n";

	assert(relative_error < 1e-8);
	std::cout << "Scientific Validation *Mercury Precession* PASSED.\n";

	return 0;
}

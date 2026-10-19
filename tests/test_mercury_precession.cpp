#include "support/test_harness.hpp"
#include "relativistic/core/constants.hpp"
#include "relativistic/core/math/tensor.hpp"
#include "relativistic/metrics/schwarzschild.hpp"
#include "relativistic/integrators/symplectic_gauss_legendre.hpp"
#include <cmath>
#include <numbers>

using Relativistic::Testing::TestSuite;

int main() {
	using namespace Relativistic::Core;
	using namespace Relativistic::Metrics;
	using namespace Relativistic::Integrators;

	TestSuite suite(
		"mercury_precession",
		"Verifies the perihelion advance of a Mercury-like orbit (a = 1000 r_s, e = 0.20563) integrated for one hundred revolutions with the sixth-order Gauss-Legendre scheme: the accumulated precession matches the exact elliptic-integral result of the Schwarzschild geodesic within a relative 1e-8."
	);

	const double M = 1.0;
	const double c = 1.0;
	const double G = 1.0;
	const double r_s = 2.0 * G * M / (c * c);
	const double a = 1000.0 * r_s;
	const double e = 0.205630;
	const double r_p = a * (1.0 - e);
	const double r_a = a * (1.0 + e);

	const double l_squared = 2.0 * G * M * r_a * r_a * r_p * r_p / ((r_a + r_p) * r_a * r_p - r_s * (r_a * r_a + r_a * r_p + r_p * r_p));
	const double L = std::sqrt(l_squared);
	const double E = std::sqrt((1.0 - r_s / r_p) * (1.0 + (L * L) / (r_p * r_p)));

	SchwarzschildMetric<double> schwarzschild(M, c, G);
	GaussLegendre6<SchwarzschildMetric<double>, double> integrator(schwarzschild);
	GeodesicState<double> state;
	state.x = FourVector<double>(0.0, r_p, std::numbers::pi_v<double> / 2.0, 0.0);
	state.u = FourVector<double>(E / (1.0 - r_s / r_p), 0.0, 0.0, L / (r_p * r_p));

	suite.section("Exact precession");
	const double u1 = 1.0 / r_a;
	const double u2 = 1.0 / r_p;
	const double u3 = 1.0 / r_s - u1 - u2;
	const double k = std::sqrt((u2 - u1) / (u3 - u1));
	double agm_a = 1.0;
	double agm_g = std::sqrt(1.0 - k * k);
	for (size_t iteration = 0; iteration < 10; ++iteration) {
		const double next_a = 0.5 * (agm_a + agm_g);
		const double next_g = std::sqrt(agm_a * agm_g);
		agm_a = next_a;
		agm_g = next_g;
	}
	const double elliptic_k = std::numbers::pi_v<double> / (2.0 * agm_a);
	const double exact_per_revolution = (4.0 * elliptic_k) / std::sqrt(r_s * (u3 - u1)) - 2.0 * std::numbers::pi_v<double>;
	constexpr size_t revolutions = 100;
	const double theoretical_total = static_cast<double>(revolutions) * exact_per_revolution;
	suite.check(exact_per_revolution > 0.0, "the exact precession per revolution is positive");

	suite.section("Numerical integration");
	const double period = 2.0 * std::numbers::pi_v<double> * std::sqrt((a * a * a) / (G * M));
	const double dt = period / 1200.0;
	size_t perihelion_count = 0;
	size_t step_count = 0;
	double last_phi = 0.0;
	bool steps_succeeded = true;
	while (perihelion_count < revolutions && step_count < 400000) {
		const double dr_before = state.u(1);
		const double phi_before = state.x(3);
		const double r_before = state.x(1);
		if (!integrator.step(state, dt)) {
			steps_succeeded = false;
			break;
		}
		++step_count;
		const double dr_after = state.u(1);
		if (dr_before < 0.0 && dr_after >= 0.0 && r_before < a) {
			const double fraction = -dr_before / (dr_after - dr_before);
			last_phi = phi_before + fraction * (state.x(3) - phi_before);
			++perihelion_count;
		}
	}
	suite.check(steps_succeeded, "every implicit step converges");
	suite.check(perihelion_count == revolutions, "one hundred perihelion passages are detected", "count=" + std::to_string(perihelion_count));

	suite.section("Comparison");
	const double numerical_total = last_phi - static_cast<double>(revolutions) * 2.0 * std::numbers::pi_v<double>;
	suite.measure("theoretical_precession", theoretical_total, "rad");
	suite.measure("numerical_precession", numerical_total, "rad");
	suite.check_relative(numerical_total, theoretical_total, 1e-8, "the accumulated precession matches the exact elliptic-integral result within a relative 1e-8");
	return suite.finish();
}

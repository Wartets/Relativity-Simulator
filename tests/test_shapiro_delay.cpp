#include "support/test_harness.hpp"
#include "relativistic/core/math/christoffel.hpp"
#include "relativistic/core/math/tensor.hpp"
#include "relativistic/metrics/schwarzschild.hpp"
#include "relativistic/integrators/symplectic_gauss_legendre.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>

using Relativistic::Testing::TestSuite;

int main() {
	using namespace Relativistic::Core;
	using namespace Relativistic::Metrics;
	using namespace Relativistic::Integrators;

	TestSuite suite(
		"shapiro_delay",
		"Verifies the Shapiro time delay of a radio signal grazing a Schwarzschild mass between r = 50000 and r = 75000 with closest approach 1000: the coordinate flight time integrated with the sixth-order Gauss-Legendre scheme agrees with an independent Gauss-Legendre quadrature of the exact geodesic equation within a relative 1e-11."
	);

	const double M = 1.0;
	const double c = 1.0;
	const double G = 1.0;
	const double r_s = 2.0 * G * M / (c * c);
	const double r0 = 1000.0;
	const double r_earth = 50000.0;
	const double r_probe = 75000.0;
	const double b_impact = r0 / std::sqrt(1.0 - r_s / r0);

	const double shapiro_one_way_theory = (r_s / c) * (
		std::log((r_earth + std::sqrt(r_earth * r_earth - r0 * r0)) / r0) +
		std::log((r_probe + std::sqrt(r_probe * r_probe - r0 * r0)) / r0) +
		0.5 * std::sqrt((r_earth - r0) / (r_earth + r0)) +
		0.5 * std::sqrt((r_probe - r0) / (r_probe + r0))
	);
	const double shapiro_round_trip_theory = 2.0 * (r_s / c) * (std::log((4.0 * r_earth * r_probe) / (r0 * r0)) + 1.0);

	SchwarzschildMetric<double> schwarzschild(M, c, G);
	GaussLegendre6<SchwarzschildMetric<double>, double> integrator(schwarzschild);

	suite.section("Geodesic integration");
	const double phi_earth = -std::acos(std::clamp(r0 / r_earth, -1.0, 1.0));
	const double u_t = 1.0 / (1.0 - r_s / r_earth);
	const double u_phi = (b_impact * c) / (r_earth * r_earth);
	const double u_r_squared = c * c - (1.0 - r_s / r_earth) * (b_impact * b_impact * c * c) / (r_earth * r_earth);

	GeodesicState<double> state;
	state.x = FourVector<double>(0.0, r_earth, std::numbers::pi_v<double> / 2.0, phi_earth);
	state.u = FourVector<double>(u_t, -std::sqrt(std::max(0.0, u_r_squared)), 0.0, u_phi);

	bool steps_succeeded = true;
	size_t step_count = 0;
	while ((state.u(1) < 0.0 || state.x(1) < r_probe) && step_count < 2000000) {
		double dt = (state.x(1) < r0 * 1.5) ? 0.5 : 5.0;
		const bool final_step = state.u(1) > 0.0 && (r_probe - state.x(1)) < state.u(1) * dt;
		if (final_step) {
			dt = (r_probe - state.x(1)) / state.u(1);
		}
		if (!integrator.step(state, dt)) {
			steps_succeeded = false;
			break;
		}
		++step_count;
		if (final_step) {
			break;
		}
	}
	for (size_t refinement = 0; steps_succeeded && refinement < 5 && std::abs(state.x(1) - r_probe) > 1e-9; ++refinement) {
		steps_succeeded = integrator.step(state, (r_probe - state.x(1)) / state.u(1));
	}
	suite.check(steps_succeeded, "every implicit step converges");
	suite.check_near(state.x(1), r_probe, 1e-6, "the signal reaches the probe radius");
	if (!steps_succeeded) {
		return suite.finish();
	}
	const double arrival_time = state.x(0);

	suite.section("Reference quadrature");
	const auto quadrature = [&](double r_end) noexcept {
		const double w_max = std::sqrt(r_end - r0);
		constexpr size_t panels = 500;
		const double dw = w_max / static_cast<double>(panels);
		constexpr double p1 = 0.8611363115940526;
		constexpr double p2 = 0.3399810435848563;
		constexpr double w1 = 0.3478548451374538;
		constexpr double w2 = 0.6521451548625461;
		const auto integrand = [&](double w) noexcept {
			const double r = r0 + w * w;
			const double poly = (r + r0) - r_s * (r * r + r * r0 + r0 * r0) / (r * r0);
			const double denominator = c * (1.0 - r_s / r) * std::sqrt(std::max(1e-30, poly));
			return (2.0 * r * std::sqrt(1.0 - r_s / r0)) / denominator;
		};
		double integral = 0.0;
		for (size_t panel = 0; panel < panels; ++panel) {
			const double a = static_cast<double>(panel) * dw;
			const double mid = a + 0.5 * dw;
			const double half = 0.5 * dw;
			integral += half * (w1 * integrand(mid - half * p1) + w2 * integrand(mid - half * p2) + w2 * integrand(mid + half * p2) + w1 * integrand(mid + half * p1));
		}
		return integral;
	};
	const double exact_time = quadrature(r_earth) + quadrature(r_probe);
	const double flat_time = (std::sqrt(r_earth * r_earth - r0 * r0) + std::sqrt(r_probe * r_probe - r0 * r0)) / c;
	const double numerical_delay = arrival_time - flat_time;

	suite.measure("theoretical_one_way_delay", shapiro_one_way_theory, "GM/c^3");
	suite.measure("numerical_one_way_delay", numerical_delay, "GM/c^3");
	suite.measure("theoretical_round_trip_delay", shapiro_round_trip_theory, "GM/c^3");
	suite.measure("numerical_round_trip_delay", 2.0 * numerical_delay, "GM/c^3");
	suite.measure("exact_flight_time", exact_time, "GM/c^3");
	suite.measure("integrated_flight_time", arrival_time, "GM/c^3");
	suite.check_relative(arrival_time, exact_time, 1e-11, "the integrated coordinate flight time agrees with the exact quadrature within a relative 1e-11");
	return suite.finish();
}

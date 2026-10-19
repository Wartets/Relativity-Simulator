#include "support/test_harness.hpp"
#include "relativistic/metrics/schwarzschild.hpp"
#include "relativistic/integrators/rk45_adaptive.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>

using namespace Relativistic::Metrics;
using namespace Relativistic::Integrators;
using namespace Relativistic::Core;
using Relativistic::Testing::TestSuite;

int main() {
	TestSuite suite(
		"solar_deflection",
		"Verifies the gravitational deflection of a light ray grazing the solar limb, integrated with the adaptive Dormand-Prince scheme in SI units from -1000 to +1000 solar radii: the deflection agrees with the first-order result 4GM/(c^2 R) within a relative 1e-4 and with the second-order post-Newtonian result within a relative 1e-5, while the null constraint residual stays below 1e-11."
	);

	constexpr double c = 299792458.0;
	constexpr double g_constant = 6.67430e-11;
	constexpr double solar_mass = 1.98847e30;
	constexpr double solar_radius = 6.957e8;
	constexpr double radians_to_arcseconds = (180.0 / std::numbers::pi_v<double>) * 3600.0;

	SchwarzschildMetric<double> metric(solar_mass, c, g_constant);
	const double r_s = metric.schwarzschild_radius();

	const double x_start = -1000.0 * solar_radius;
	const double x_end = 1000.0 * solar_radius;
	const double r0 = std::sqrt(x_start * x_start + solar_radius * solar_radius);
	const double theta0 = std::numbers::pi_v<double> / 2.0;
	const double phi0 = std::atan2(solar_radius, x_start);

	GeodesicState<double> photon;
	photon.x = FourVector<double>(0.0, r0, theta0, phi0);
	const double p_r = std::sin(theta0) * std::cos(phi0);
	const double p_theta = std::cos(theta0) * std::cos(phi0) / r0;
	const double p_phi = -std::sin(phi0) / (r0 * std::sin(theta0));
	photon.u = FourVector<double>(0.0, p_r, p_theta, p_phi);
	const auto g0 = metric.metric_tensor(photon.x);
	const double spatial_norm = g0(1, 1) * p_r * p_r + g0(2, 2) * p_theta * p_theta + g0(3, 3) * p_phi * p_phi;
	photon.u(0) = std::sqrt(-spatial_norm / g0(0, 0));

	RK45Config<double> config;
	config.initial_step = 1e7;
	config.min_step = 1e-4;
	config.max_step = 5e10;
	config.rtol = 1e-10;
	config.atol = 1e-14;
	config.invariant_tolerance = 1e-13;
	RK45AdaptiveIntegrator<SchwarzschildMetric<double>, double> integrator(metric, GeodesicType::Null, config);

	suite.section("Integration across the solar field");
	double dt = config.initial_step;
	uint64_t step_count = 0;
	bool integration_succeeded = true;
	bool reached_end = false;
	while (step_count < 10000000) {
		const double current_x = photon.x(1) * std::cos(photon.x(3));
		if (current_x >= x_end) {
			reached_end = true;
			break;
		}
		if (!integrator.step(photon, dt).has_value()) {
			integration_succeeded = false;
			break;
		}
		++step_count;
	}
	suite.check(integration_succeeded, "every adaptive step stays above the minimum step size");
	suite.check(reached_end, "the ray reaches the far boundary of the integration domain");
	if (!integration_succeeded || !reached_end) {
		return suite.finish();
	}

	suite.section("Deflection angle");
	const double r = photon.x(1);
	const double theta = photon.x(2);
	const double phi = photon.x(3);
	const double final_px = photon.u(1) * std::sin(theta) * std::cos(phi) + photon.u(2) * r * std::cos(theta) * std::cos(phi) - photon.u(3) * r * std::sin(theta) * std::sin(phi);
	const double final_py = photon.u(1) * std::sin(theta) * std::sin(phi) + photon.u(2) * r * std::cos(theta) * std::sin(phi) + photon.u(3) * r * std::sin(theta) * std::cos(phi);
	const double deflection_rad = std::atan2(std::abs(final_py), final_px);
	const double deflection_arcsec = deflection_rad * radians_to_arcseconds;

	const double expected_first_rad = 2.0 * r_s / solar_radius;
	const double second_order_coefficient = (15.0 * std::numbers::pi_v<double> / 4.0) - 4.0;
	const double expected_second_rad = expected_first_rad + (r_s * r_s / (4.0 * solar_radius * solar_radius)) * second_order_coefficient;
	const double expected_first_arcsec = expected_first_rad * radians_to_arcseconds;
	const double expected_second_arcsec = expected_second_rad * radians_to_arcseconds;
	const auto& stats = integrator.statistics();

	suite.measure("steps_completed", static_cast<double>(step_count), "steps");
	suite.measure("steps_rejected", static_cast<double>(stats.rejected_steps), "steps");
	suite.measure("computed_deflection", deflection_arcsec, "arcsec");
	suite.measure("first_order_deflection", expected_first_arcsec, "arcsec");
	suite.measure("second_order_deflection", expected_second_arcsec, "arcsec");
	suite.check_relative(deflection_arcsec, expected_first_arcsec, 1e-4, "the deflection matches 4GM/(c^2 R) within a relative 1e-4");
	suite.check_relative(deflection_arcsec, expected_second_arcsec, 1e-5, "the deflection matches the second-order post-Newtonian result within a relative 1e-5");

	suite.section("Null constraint");
	suite.measure("max_invariant_residual", stats.max_invariant_residual, "dimensionless");
	suite.check_at_most(stats.max_invariant_residual, 1e-11, "the null constraint residual stays below 1e-11 along the whole ray");
	return suite.finish();
}

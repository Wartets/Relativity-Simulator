#include "support/test_harness.hpp"
#include "relativistic/metrics/flat_minkowski.hpp"
#include "relativistic/metrics/schwarzschild.hpp"
#include "relativistic/metrics/schwarzschild_isotropic.hpp"
#include "relativistic/metrics/painleve_gullstrand.hpp"
#include "relativistic/metrics/eddington_finkelstein.hpp"
#include "relativistic/metrics/reissner_nordstrom.hpp"
#include "relativistic/metrics/morris_thorne.hpp"
#include "relativistic/metrics/alcubierre.hpp"
#include "relativistic/core/math/christoffel.hpp"
#include "relativistic/core/math/riemann.hpp"
#include "relativistic/core/math/tensor_ops.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <string>

using namespace Relativistic::Metrics;
using namespace Relativistic::Core;
using Relativistic::Testing::format_number;
using Relativistic::Testing::TestSuite;

namespace {

struct UnitSystem {
	const char* label;
	double speed_of_light;
	double gravitational_constant;
	double mass;
	double inversion_tolerance;
	double christoffel_tolerance;
};

constexpr UnitSystem kGeometricUnits{"geometric", 1.0, 1.0, 1.0, 1e-12, 1e-7};
constexpr UnitSystem kSiUnits{"SI", 299792458.0, 6.67430e-11, 1.98847e30, 1e-9, 1e-6};

template <typename MetricType>
double max_inversion_error(const MetricType& metric, const FourVector<double>& x) {
	const auto g = metric.metric_tensor(x);
	const auto inv_g = metric.inverse_metric(x);
	const auto product = matrix_multiply(inv_g, g);
	double worst = 0.0;
	for (size_t i = 0; i < 4; ++i) {
		for (size_t j = 0; j < 4; ++j) {
			worst = std::max(worst, std::abs(product(i, j) - ((i == j) ? 1.0 : 0.0)));
		}
	}
	return worst;
}

template <typename MetricType>
double max_metric_asymmetry(const MetricType& metric, const FourVector<double>& x) {
	const auto g = metric.metric_tensor(x);
	double scale = 1e-300;
	for (size_t i = 0; i < 4; ++i) {
		for (size_t j = 0; j < 4; ++j) {
			scale = std::max(scale, std::abs(g(i, j)));
		}
	}
	double worst = 0.0;
	for (size_t i = 0; i < 4; ++i) {
		for (size_t j = i + 1; j < 4; ++j) {
			worst = std::max(worst, std::abs(g(i, j) - g(j, i)));
		}
	}
	return worst / scale;
}

struct ChristoffelDeviation {
	double worst_relative{0.0};
	double worst_lower_asymmetry{0.0};
	size_t worst_sigma{0};
	size_t worst_mu{0};
	size_t worst_nu{0};
};

template <typename MetricType>
ChristoffelDeviation christoffel_deviation(const MetricType& metric, const FourVector<double>& x) {
	const auto analytic = metric.christoffel_symbols(x);
	const auto numerical = compute_christoffel_numerical<DerivativeOrder::EighthOrder, MetricType, double>(metric, x);
	ChristoffelDeviation result;
	for (size_t sigma = 0; sigma < 4; ++sigma) {
		double row_reference = 1e-300;
		for (size_t mu = 0; mu < 4; ++mu) {
			for (size_t nu = 0; nu < 4; ++nu) {
				row_reference = std::max(row_reference, std::abs(analytic(sigma, mu, nu)));
			}
		}
		for (size_t mu = 0; mu < 4; ++mu) {
			for (size_t nu = 0; nu < 4; ++nu) {
				const double a = analytic(sigma, mu, nu);
				const double n = numerical(sigma, mu, nu);
				const double denominator = std::max({std::abs(a), std::abs(n), 1e-9 * row_reference});
				const double relative = std::abs(a - n) / denominator;
				if (relative > result.worst_relative) {
					result.worst_relative = relative;
					result.worst_sigma = sigma;
					result.worst_mu = mu;
					result.worst_nu = nu;
				}
				result.worst_lower_asymmetry = std::max(result.worst_lower_asymmetry, std::abs(a - analytic(sigma, nu, mu)) / row_reference);
			}
		}
	}
	return result;
}

template <typename MetricType>
void run_core_checks(TestSuite& suite, const std::string& name, const MetricType& metric, const FourVector<double>& x, const UnitSystem& units) {
	const std::string prefix = name + " [" + units.label + "] ";
	suite.check_at_most(max_inversion_error(metric, x), units.inversion_tolerance, prefix + "inverse metric times metric equals identity");
	suite.check_at_most(max_metric_asymmetry(metric, x), 1e-14, prefix + "metric tensor is symmetric");
	suite.check(determinant_4x4(metric.metric_tensor(x)) < 0.0, prefix + "metric has Lorentzian signature (negative determinant)");
	const auto deviation = christoffel_deviation(metric, x);
	suite.check_at_most(deviation.worst_relative, units.christoffel_tolerance, prefix + "analytic Christoffel symbols match eighth-order finite differences");
	if (deviation.worst_relative > units.christoffel_tolerance) {
		suite.note(prefix + "largest Christoffel deviation at Gamma(" + std::to_string(deviation.worst_sigma) + "," + std::to_string(deviation.worst_mu) + "," + std::to_string(deviation.worst_nu) + ")");
	}
	suite.check_at_most(deviation.worst_lower_asymmetry, 1e-14, prefix + "Christoffel symbols are symmetric in the lower indices");
}

template <typename MetricType>
void run_vacuum_curvature_checks(TestSuite& suite, const std::string& name, const MetricType& metric, const FourVector<double>& x, double mass) {
	using Riemann = RiemannComputer<DerivativeOrder::EighthOrder, MetricType, double>;
	const double areal_radius = x(1);
	const double expected_kretschmann = 48.0 * mass * mass / std::pow(areal_radius, 6.0);
	const auto g = metric.metric_tensor(x);
	const auto inv_g = metric.inverse_metric(x);
	const auto riemann = Riemann::compute_riemann(metric, x);
	const auto ricci = Riemann::compute_ricci_tensor(riemann);
	double ricci_magnitude = 0.0;
	for (size_t i = 0; i < 4; ++i) {
		for (size_t j = 0; j < 4; ++j) {
			ricci_magnitude = std::max(ricci_magnitude, std::abs(ricci(i, j)));
		}
	}
	suite.check_at_most(ricci_magnitude / std::sqrt(expected_kretschmann), 1e-6, name + " Ricci tensor vanishes in vacuum");
	suite.check_relative(Riemann::compute_kretschmann_invariant(riemann, g, inv_g), expected_kretschmann, 1e-4, name + " Kretschmann invariant equals 48 M^2 / r^6");
	suite.check_at_most(std::abs(Riemann::compute_ricci_scalar(ricci, inv_g)) / std::sqrt(expected_kretschmann), 1e-6, name + " Ricci scalar vanishes in vacuum");
}

void run_unit_system(TestSuite& suite, const UnitSystem& units) {
	suite.section(std::string("Static spherical metrics in ") + units.label + " units");
	FlatMinkowskiMetric<double> minkowski(units.speed_of_light);
	SchwarzschildMetric<double> schwarzschild(units.mass, units.speed_of_light, units.gravitational_constant);
	SchwarzschildIsotropicMetric<double> isotropic(units.mass, units.speed_of_light, units.gravitational_constant);
	PainleveGullstrandMetric<double> painleve(units.mass, units.speed_of_light, units.gravitational_constant);
	EddingtonFinkelsteinMetric<double> eddington(units.mass, units.speed_of_light, units.gravitational_constant);

	const double rs = schwarzschild.schwarzschild_radius();
	const FourVector<double> x(0.0, 5.0 * rs, std::numbers::pi_v<double> / 3.0, 0.5);

	run_core_checks(suite, "Flat Minkowski", minkowski, x, units);
	run_core_checks(suite, "Schwarzschild", schwarzschild, x, units);
	run_core_checks(suite, "Schwarzschild isotropic", isotropic, x, units);
	run_core_checks(suite, "Painleve-Gullstrand", painleve, x, units);
	run_core_checks(suite, "Eddington-Finkelstein", eddington, x, units);

	const FourVector<double> horizon(0.0, rs, std::numbers::pi_v<double> / 2.0, 0.0);
	const double horizon_scale = std::pow(rs, 4.0) * std::pow(units.speed_of_light, 2.0);
	suite.check(std::abs(determinant_4x4(painleve.metric_tensor(horizon))) > 1e-6 * horizon_scale, std::string("Painleve-Gullstrand metric stays regular at the horizon [") + units.label + "]");
	suite.check(std::abs(determinant_4x4(eddington.metric_tensor(horizon))) > 1e-6 * horizon_scale, std::string("Eddington-Finkelstein metric stays regular at the horizon [") + units.label + "]");
}

void run_vacuum_curvature(TestSuite& suite) {
	suite.section("Vacuum curvature invariants in geometric units");
	const FourVector<double> x(0.0, 10.0, std::numbers::pi_v<double> / 3.0, 0.5);
	SchwarzschildMetric<double> schwarzschild(1.0, 1.0, 1.0);
	PainleveGullstrandMetric<double> painleve(1.0, 1.0, 1.0);
	EddingtonFinkelsteinMetric<double> eddington(1.0, 1.0, 1.0);
	run_vacuum_curvature_checks(suite, "Schwarzschild", schwarzschild, x, 1.0);
	run_vacuum_curvature_checks(suite, "Painleve-Gullstrand", painleve, x, 1.0);
	run_vacuum_curvature_checks(suite, "Eddington-Finkelstein", eddington, x, 1.0);
}

void run_reissner_nordstrom(TestSuite& suite) {
	suite.section("Reissner-Nordstrom");
	const ReissnerNordstromMetric<double> charged(1.0, 0.6, 1.0, 1.0, 1.0);
	const FourVector<double> x(0.0, 4.0, std::numbers::pi_v<double> / 3.0, 0.5);
	run_core_checks(suite, "Reissner-Nordstrom", charged, x, kGeometricUnits);
	suite.check_near(charged.outer_horizon_radius(), 1.8, 1e-12, "outer horizon radius equals M + sqrt(M^2 - Q^2)");
	suite.check_near(charged.inner_horizon_radius(), 0.2, 1e-12, "inner horizon radius equals M - sqrt(M^2 - Q^2)");
	suite.check(charged.is_subextremal() && !charged.is_extremal() && !charged.is_hyperextremal(), "Q/M = 0.6 is classified as subextremal");
	const ReissnerNordstromMetric<double> extremal(1.0, 1.0, 1.0, 1.0, 1.0);
	suite.check(extremal.is_extremal(), "Q/M = 1 is classified as extremal");
	const ReissnerNordstromMetric<double> naked(1.0, 1.2, 1.0, 1.0, 1.0);
	suite.check(naked.is_hyperextremal(), "Q/M = 1.2 is classified as hyperextremal");
	const ReissnerNordstromMetric<double> neutral(1.0, 0.0, 1.0, 1.0, 1.0);
	suite.check_near(neutral.outer_photon_sphere_radius(), 3.0, 1e-12, "uncharged outer photon sphere sits at 3 M");
	suite.check_near(neutral.outer_horizon_radius(), 2.0, 1e-12, "uncharged outer horizon sits at 2 M");
}

void run_exotic_static(TestSuite& suite) {
	suite.section("Morris-Thorne wormhole and Alcubierre bubble");
	const MorrisThorneWormholeMetric<double> wormhole(1.0, 0.3, 1.0);
	const FourVector<double> wormhole_point(0.0, 2.0, std::numbers::pi_v<double> / 3.0, 0.5);
	run_core_checks(suite, "Morris-Thorne", wormhole, wormhole_point, kGeometricUnits);
	suite.check_near(wormhole.radius_from_proper_length(0.0), 1.0, 1e-14, "areal radius at the throat equals the throat radius");
	suite.check_near(wormhole.radius_from_proper_length(3.0), std::sqrt(10.0), 1e-14, "areal radius follows sqrt(l^2 + b0^2)");

	const AlcubierreWarpMetric<double> bubble(1.5, 100.0, 0.1, 1.0);
	const FourVector<double> wall_point(0.0, 95.0, 10.0, -4.0);
	run_core_checks(suite, "Alcubierre", bubble, wall_point, kGeometricUnits);
	suite.check_near(bubble.shaping_function(0.0), 1.0, 1e-9, "shaping function is unity at the bubble centre");
	suite.check_near(bubble.shaping_function(1000.0), 0.0, 1e-9, "shaping function vanishes far from the bubble");
}

}

int main() {
	TestSuite suite(
		"static_metrics",
		"Verifies the static and exotic spacetime metrics: metric inversion, symmetry, Lorentzian signature, analytic Christoffel symbols against eighth-order finite differences in geometric and SI units, vacuum curvature invariants, and horizon structure."
	);
	run_unit_system(suite, kGeometricUnits);
	run_unit_system(suite, kSiUnits);
	run_vacuum_curvature(suite);
	run_reissner_nordstrom(suite);
	run_exotic_static(suite);
	return suite.finish();
}

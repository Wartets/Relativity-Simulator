#include "support/test_harness.hpp"
#include "support/metric_checks.hpp"
#include "relativistic/metrics/reissner_nordstrom.hpp"
#include "relativistic/metrics/kerr_newman.hpp"
#include "relativistic/metrics/schwarzschild_de_sitter.hpp"
#include "relativistic/metrics/kerr_de_sitter.hpp"
#include "relativistic/metrics/flrw.hpp"
#include <cmath>

using namespace Relativistic::Core;
using namespace Relativistic::Metrics;
using Relativistic::Testing::MetricGeometryTolerances;
using Relativistic::Testing::TestSuite;
using Relativistic::Testing::inversion_residual;
using Relativistic::Testing::verify_metric_geometry;

int main() {
	TestSuite suite(
		"vacuum_cosmo_metrics",
		"Verifies the electrovacuum and cosmological metrics: Reissner-Nordstrom and Kerr-Newman horizons and regimes, Kerr-Newman reduction to Kerr, geometric invariants of Reissner-Nordstrom, Kerr-Newman, Schwarzschild-de Sitter and Kerr-de Sitter, and the FLRW scale factor, redshift and metric inversion."
	);

	suite.section("Reissner-Nordstrom horizons");
	const ReissnerNordstromMetric<double> sub(1.0, 0.6);
	suite.check(sub.is_subextremal() && !sub.is_extremal() && !sub.is_hyperextremal(), "Q / M = 0.6 is sub-extremal");
	suite.check_near(sub.outer_horizon_radius(), 1.8, 1e-12, "the outer horizon equals M + sqrt(M^2 - Q^2) = 1.8");
	suite.check_near(sub.inner_horizon_radius(), 0.2, 1e-12, "the inner horizon equals M - sqrt(M^2 - Q^2) = 0.2");
	const ReissnerNordstromMetric<double> extremal(1.0, 1.0);
	suite.check(extremal.is_extremal(), "Q = M is extremal");
	suite.check_near(extremal.outer_horizon_radius(), 1.0, 1e-12, "the extremal outer horizon sits at M");
	suite.check_near(extremal.inner_horizon_radius(), 1.0, 1e-12, "the extremal inner horizon sits at M");
	suite.check(ReissnerNordstromMetric<double>(1.0, 1.2).is_hyperextremal(), "Q > M is hyper-extremal");
	suite.check_near(ReissnerNordstromMetric<double>(1.0, 0.0).outer_photon_sphere_radius(), 3.0, 1e-12, "the uncharged photon sphere radius equals 3 M");

	suite.section("Reissner-Nordstrom geometry");
	MetricGeometryTolerances tolerances;
	verify_metric_geometry(suite, "Reissner-Nordstrom", ReissnerNordstromMetric<double>(1.0, 0.5), FourVector<double>(0.0, 5.0, 1.2, 0.4), tolerances);

	suite.section("Kerr-Newman");
	const KerrNewmanMetric<double> kerr_newman(1.0, 0.6, 0.4);
	suite.check(kerr_newman.is_subextremal(), "a^2 + Q^2 < M^2 is sub-extremal");
	suite.check_near(kerr_newman.outer_horizon_radius(), 1.0 + std::sqrt(1.0 - 0.36 - 0.16), 1e-12, "the outer horizon equals M + sqrt(M^2 - a^2 - Q^2)");
	const KerrNewmanMetric<double> uncharged(1.0, 0.5, 0.0);
	const FourVector<double> reduction_point(0.0, 4.0, 1.0, 0.0);
	suite.check_at_most(inversion_residual(uncharged.metric_tensor(reduction_point), uncharged.inverse_metric(reduction_point)), 1e-12, "the uncharged Kerr-Newman metric inverts to the identity");
	tolerances.connection = 1e-7;
	verify_metric_geometry(suite, "Kerr-Newman", KerrNewmanMetric<double>(1.0, 0.5, 0.3), FourVector<double>(0.0, 3.5, 1.1, 0.7), tolerances);

	suite.section("De Sitter extensions");
	tolerances.connection = 1e-8;
	verify_metric_geometry(suite, "Schwarzschild-de Sitter", SchwarzschildDeSitterMetric<double>(1.0, 1e-5), FourVector<double>(0.0, 10.0, 1.3, 0.5), tolerances);
	tolerances.inversion = 1e-11;
	tolerances.connection = 1e-7;
	verify_metric_geometry(suite, "Kerr-de Sitter", KerrDeSitterMetric<double>(1.0, 0.4, 1e-4), FourVector<double>(0.0, 6.0, 1.2, 0.3), tolerances);

	suite.section("FLRW expansion");
	FLRWCosmologyConfig<double> config{.model_type = CosmologicalModelType::MatterDominated, .t0 = 1.0};
	const FLRWMetric<double> flrw(config);
	suite.check_near(flrw.scale_factor(1.0), 1.0, 1e-12, "the matter-dominated scale factor equals 1 at t0");
	suite.check_near(flrw.scale_factor(8.0), 4.0, 1e-12, "the matter-dominated scale factor grows as t^(2/3), giving 4 at t = 8");
	suite.check_near(flrw.cosmological_redshift(1.0, 8.0), 3.0, 1e-12, "the cosmological redshift between t = 1 and t = 8 equals 3");
	const FourVector<double> flrw_point(2.0, 0.5, 1.0, 0.2);
	suite.check_at_most(inversion_residual(flrw.metric_tensor(flrw_point), flrw.inverse_metric(flrw_point)), 1e-12, "the FLRW inverse metric reproduces the identity");
	return suite.finish();
}

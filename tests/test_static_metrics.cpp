#include "relativistic/metrics/flat_minkowski.hpp"
#include "relativistic/metrics/schwarzschild.hpp"
#include "relativistic/metrics/schwarzschild_isotropic.hpp"
#include "relativistic/metrics/painleve_gullstrand.hpp"
#include "relativistic/metrics/eddington_finkelstein.hpp"
#include "relativistic/core/christoffel.hpp"
#include "relativistic/core/tensor_ops.hpp"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <numbers>

using namespace Relativistic::Metrics;
using namespace Relativistic::Core;

template <typename MetricType>
bool verify_metric_inversion(const MetricType& metric, const FourVector<double>& x, double tol = 1e-6) {
	const auto g = metric.metric_tensor(x);
	const auto inv_g = metric.inverse_metric(x);
	const auto prod = matrix_multiply(inv_g, g);

	for (size_t i = 0; i < 4; ++i) {
		for (size_t j = 0; j < 4; ++j) {
			const double expected = (i == j) ? 1.0 : 0.0;
			if (std::abs(prod(i, j) - expected) > tol) {
				return false;
			}
		}
	}
	return true;
}

template <typename MetricType>
bool verify_analytic_christoffel(const MetricType& metric, const FourVector<double>& x, double tol = 1e-6) {
	const auto gamma_analytic = metric.christoffel_symbols(x);
	const auto gamma_numerical = compute_christoffel_numerical<DerivativeOrder::EighthOrder, MetricType, double>(metric, x);

	for (size_t s = 0; s < 4; ++s) {
		for (size_t m = 0; m < 4; ++m) {
			for (size_t n = 0; n < 4; ++n) {
				const double val_a = gamma_analytic(s, m, n);
				const double val_n = gamma_numerical(s, m, n);
				const double scale = std::max({1.0, std::abs(val_a), std::abs(val_n)});
				if (std::abs(val_a - val_n) / scale > tol) {
					return false;
				}
			}
		}
	}
	return true;
}

int main() {
	constexpr double c_geom = 1.0;
	constexpr double G_geom = 1.0;
	constexpr double M_geom = 1.0;

	constexpr double c_si = 299792458.0;
	constexpr double G_si = 6.67430e-11;
	constexpr double M_si = 1.98847e30;

	FlatMinkowskiMetric<double> minkowski_geom(c_geom);
	SchwarzschildMetric<double> schw_geom(M_geom, c_geom, G_geom);
	SchwarzschildIsotropicMetric<double> iso_geom(M_geom, c_geom, G_geom);
	PainleveGullstrandMetric<double> pg_geom(M_geom, c_geom, G_geom);
	EddingtonFinkelsteinMetric<double> ef_geom(M_geom, c_geom, G_geom);

	FlatMinkowskiMetric<double> minkowski_si(c_si);
	SchwarzschildMetric<double> schw_si(M_si, c_si, G_si);
	SchwarzschildIsotropicMetric<double> iso_si(M_si, c_si, G_si);
	PainleveGullstrandMetric<double> pg_si(M_si, c_si, G_si);
	EddingtonFinkelsteinMetric<double> ef_si(M_si, c_si, G_si);

	const double rs_geom = schw_geom.schwarzschild_radius();
	const FourVector<double> x_geom(0.0, 5.0 * rs_geom, std::numbers::pi_v<double> / 3.0, 0.5);

	const double rs_si = schw_si.schwarzschild_radius();
	const FourVector<double> x_si(0.0, 5.0 * rs_si, std::numbers::pi_v<double> / 3.0, 0.5);

	std::cout << "================================================================================" << std::endl;
	std::cout << "STATIC METRICS FORMAL VERIFICATION SUITE" << std::endl;
	std::cout << "================================================================================" << std::endl;

	bool all_passed = true;

	const bool minkowski_inv = verify_metric_inversion(minkowski_geom, x_geom, 1e-12) && verify_metric_inversion(minkowski_si, x_si, 1e-6);
	const bool minkowski_gamma = verify_analytic_christoffel(minkowski_geom, x_geom, 1e-10) && verify_analytic_christoffel(minkowski_si, x_si, 1e-6);
	std::cout << "Minkowski Metric Inversion          : " << (minkowski_inv ? "PASSED" : "FAILED") << std::endl;
	std::cout << "Minkowski Christoffel Symbols       : " << (minkowski_gamma ? "PASSED" : "FAILED") << std::endl;
	all_passed &= (minkowski_inv && minkowski_gamma);

	const bool schw_inv = verify_metric_inversion(schw_geom, x_geom, 1e-12) && verify_metric_inversion(schw_si, x_si, 1e-6);
	const bool schw_gamma = verify_analytic_christoffel(schw_geom, x_geom, 1e-9) && verify_analytic_christoffel(schw_si, x_si, 1e-6);
	std::cout << "Schwarzschild Standard Inversion    : " << (schw_inv ? "PASSED" : "FAILED") << std::endl;
	std::cout << "Schwarzschild Standard Christoffel  : " << (schw_gamma ? "PASSED" : "FAILED") << std::endl;
	all_passed &= (schw_inv && schw_gamma);

	const bool iso_inv = verify_metric_inversion(iso_geom, x_geom, 1e-12) && verify_metric_inversion(iso_si, x_si, 1e-6);
	const bool iso_gamma = verify_analytic_christoffel(iso_geom, x_geom, 1e-9) && verify_analytic_christoffel(iso_si, x_si, 1e-6);
	std::cout << "Schwarzschild Isotropic Inversion   : " << (iso_inv ? "PASSED" : "FAILED") << std::endl;
	std::cout << "Schwarzschild Isotropic Christoffel : " << (iso_gamma ? "PASSED" : "FAILED") << std::endl;
	all_passed &= (iso_inv && iso_gamma);

	const bool pg_inv = verify_metric_inversion(pg_geom, x_geom, 1e-12) && verify_metric_inversion(pg_si, x_si, 1e-6);
	const bool pg_gamma = verify_analytic_christoffel(pg_geom, x_geom, 1e-9) && verify_analytic_christoffel(pg_si, x_si, 1e-6);
	std::cout << "Painleve-Gullstrand Inversion       : " << (pg_inv ? "PASSED" : "FAILED") << std::endl;
	std::cout << "Painleve-Gullstrand Christoffel     : " << (pg_gamma ? "PASSED" : "FAILED") << std::endl;
	all_passed &= (pg_inv && pg_gamma);

	const bool ef_inv = verify_metric_inversion(ef_geom, x_geom, 1e-12) && verify_metric_inversion(ef_si, x_si, 1e-6);
	const bool ef_gamma = verify_analytic_christoffel(ef_geom, x_geom, 1e-9) && verify_analytic_christoffel(ef_si, x_si, 1e-6);
	std::cout << "Eddington-Finkelstein Inversion     : " << (ef_inv ? "PASSED" : "FAILED") << std::endl;
	std::cout << "Eddington-Finkelstein Christoffel   : " << (ef_gamma ? "PASSED" : "FAILED") << std::endl;
	all_passed &= (ef_inv && ef_gamma);

	const FourVector<double> x_horizon_geom(0.0, rs_geom, std::numbers::pi_v<double> / 2.0, 0.0);
	const auto g_pg_horizon_geom = pg_geom.metric_tensor(x_horizon_geom);
	const double det_pg_geom = determinant_4x4(g_pg_horizon_geom);
	const bool pg_regular_geom = (std::abs(det_pg_geom) > 1e-6);

	const FourVector<double> x_horizon_si(0.0, rs_si, std::numbers::pi_v<double> / 2.0, 0.0);
	const auto g_pg_horizon_si = pg_si.metric_tensor(x_horizon_si);
	const double det_pg_si = determinant_4x4(g_pg_horizon_si);
	const bool pg_regular_si = (std::abs(det_pg_si) > 1e-6);

	const bool pg_regular = pg_regular_geom && pg_regular_si;
	std::cout << "Painleve-Gullstrand Horizon Regular : " << (pg_regular ? "PASSED" : "FAILED") << std::endl;
	all_passed &= pg_regular;

	std::cout << "================================================================================" << std::endl;
	if (all_passed) {
		std::cout << "STATIC METRICS TEST STATUS: ALL PASSED" << std::endl;
		return 0;
	}

	std::cerr << "STATIC METRICS TEST STATUS: FAILURE" << std::endl;
	return 1;
}

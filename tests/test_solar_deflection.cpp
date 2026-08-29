#include "relativistic/metrics/schwarzschild.hpp"
#include "relativistic/integrators/rk45_adaptive.hpp"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <numbers>
#include <chrono>

using namespace Relativistic::Metrics;
using namespace Relativistic::Integrators;
using namespace Relativistic::Core;

int main() {
	constexpr double c = 299792458.0;
	constexpr double G = 6.67430e-11;
	constexpr double M_sun = 1.98847e30;
	constexpr double R_sun = 6.957e8;

	SchwarzschildMetric<double> sun_metric(M_sun, c, G);
	const double r_s = sun_metric.schwarzschild_radius();

	const double x_start = -1000.0 * R_sun;
	const double x_end = 1000.0 * R_sun;
	const double total_span_x = x_end - x_start;
	const double y_impact = R_sun;
	const double z_impact = 0.0;

	const double r0 = std::sqrt(x_start * x_start + y_impact * y_impact + z_impact * z_impact);
	const double theta0 = std::numbers::pi_v<double> / 2.0;
	const double phi0 = std::atan2(y_impact, x_start);

	GeodesicState<double> photon;
	photon.x = FourVector<double>(0.0, r0, theta0, phi0);

	const double p_x = 1.0;
	const double p_y = 0.0;
	const double p_z = 0.0;

	const double p_r = p_x * std::sin(theta0) * std::cos(phi0) + p_y * std::sin(theta0) * std::sin(phi0) + p_z * std::cos(theta0);
	const double p_theta = (p_x * std::cos(theta0) * std::cos(phi0) + p_y * std::cos(theta0) * std::sin(phi0) - p_z * std::sin(theta0)) / r0;
	const double p_phi = (-p_x * std::sin(phi0) + p_y * std::cos(phi0)) / (r0 * std::sin(theta0));

	photon.u = FourVector<double>(0.0, p_r, p_theta, p_phi);

	const auto g0 = sun_metric.metric_tensor(photon.x);
	const double spatial_norm = g0(1, 1) * p_r * p_r + g0(2, 2) * p_theta * p_theta + g0(3, 3) * p_phi * p_phi;
	photon.u(0) = std::sqrt(-spatial_norm / g0(0, 0));

	RK45Config<double> config;
	config.initial_step = 1e7;
	config.min_step = 1e-4;
	config.max_step = 5e10;
	config.rtol = 1e-10;
	config.atol = 1e-14;
	config.invariant_tolerance = 1e-13;

	RK45AdaptiveIntegrator<SchwarzschildMetric<double>, double> integrator(sun_metric, GeodesicType::Null, config);

	std::cout << "================================================================================" << std::endl;
	std::cout << "RELATIVISTIC ENGINE - FORMAL VALIDATION: SOLAR GRAVITATIONAL LIGHT DEFLECTION" << std::endl;
	std::cout << "================================================================================" << std::endl;
	std::cout << std::scientific << std::setprecision(8);
	std::cout << "Solar Mass (M_sun)           : " << M_sun << " kg" << std::endl;
	std::cout << "Solar Radius (R_sun)         : " << R_sun << " m" << std::endl;
	std::cout << "Schwarzschild Radius (r_s)   : " << r_s << " m" << std::endl;
	std::cout << "Initial Position (x, y)      : (" << x_start / R_sun << " R_sun, " << y_impact / R_sun << " R_sun)" << std::endl;
	std::cout << "Target Boundary (x)          : " << x_end / R_sun << " R_sun" << std::endl;
	std::cout << "Integrator Configuration     : RK45 Adaptive (Dormand-Prince)" << std::endl;
	std::cout << "--------------------------------------------------------------------------------" << std::endl;
	std::cout << std::setw(8) << "Step"
	          << std::setw(14) << "Progress [%]"
	          << std::setw(18) << "x [R_sun]"
	          << std::setw(18) << "r [R_sun]"
	          << std::setw(18) << "Step Size [m]"
	          << std::setw(18) << "|p.p| Error" << std::endl;
	std::cout << "--------------------------------------------------------------------------------" << std::endl;

	double dt = config.initial_step;
	uint64_t step_count = 0;
	double next_milestone_pct = 0.0;

	const auto t_start = std::chrono::high_resolution_clock::now();

	while (true) {
		const double cur_r = photon.x(1);
		const double cur_phi = photon.x(3);
		const double cur_x = cur_r * std::cos(cur_phi);

		const double progress = std::clamp((cur_x - x_start) / total_span_x, 0.0, 1.0) * 100.0;
		if (progress >= next_milestone_pct || cur_x >= x_end) {
			const auto g = sun_metric.metric_tensor(photon.x);
			const double norm_sq = g(0, 0) * photon.u(0) * photon.u(0)
			                     + g(1, 1) * photon.u(1) * photon.u(1)
			                     + g(2, 2) * photon.u(2) * photon.u(2)
			                     + g(3, 3) * photon.u(3) * photon.u(3);

			std::cout << std::setw(8) << step_count
			          << std::setw(14) << std::fixed << std::setprecision(1) << progress
			          << std::scientific << std::setprecision(4)
			          << std::setw(18) << cur_x / R_sun
			          << std::setw(18) << cur_r / R_sun
			          << std::setw(18) << dt
			          << std::setw(18) << std::abs(norm_sq) << std::endl;

			next_milestone_pct += 20.0;
		}

		if (cur_x >= x_end) {
			break;
		}

		const auto step_res = integrator.step(photon, dt);
		if (!step_res.has_value()) {
			std::cerr << "Integration error: step size dropped below minimum threshold!" << std::endl;
			return 1;
		}
		++step_count;
	}

	const auto t_end = std::chrono::high_resolution_clock::now();
	const double elapsed_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();

	const double final_r = photon.x(1);
	const double final_theta = photon.x(2);
	const double final_phi = photon.x(3);

	const double final_pr = photon.u(1);
	const double final_ptheta = photon.u(2);
	const double final_pphi = photon.u(3);

	const double final_px = final_pr * std::sin(final_theta) * std::cos(final_phi)
	                      + final_ptheta * final_r * std::cos(final_theta) * std::cos(final_phi)
	                      - final_pphi * final_r * std::sin(final_theta) * std::sin(final_phi);
	
	const double final_py = final_pr * std::sin(final_theta) * std::sin(final_phi)
	                      + final_ptheta * final_r * std::cos(final_theta) * std::sin(final_phi)
	                      + final_pphi * final_r * std::sin(final_theta) * std::cos(final_phi);

	const double deflection_angle_rad = std::atan2(std::abs(final_py), final_px);
	const double rad_to_arcsec = (180.0 / std::numbers::pi_v<double>) * 3600.0;
	const double deflection_angle_arcsec = deflection_angle_rad * rad_to_arcsec;

	const double expected_1st_rad = 2.0 * r_s / R_sun;
	const double expected_1st_arcsec = expected_1st_rad * rad_to_arcsec;

	const double second_order_coeff = (15.0 * std::numbers::pi_v<double> / 4.0) - 4.0;
	const double expected_2nd_rad = (2.0 * r_s / R_sun) + (r_s * r_s / (4.0 * R_sun * R_sun)) * second_order_coeff;
	const double expected_2nd_arcsec = expected_2nd_rad * rad_to_arcsec;

	const double rel_err_1st = std::abs(deflection_angle_arcsec - expected_1st_arcsec) / expected_1st_arcsec;
	const double rel_err_2nd = std::abs(deflection_angle_arcsec - expected_2nd_arcsec) / expected_2nd_arcsec;

	const auto& stats = integrator.statistics();

	std::cout << "================================================================================" << std::endl;
	std::cout << "VALIDATION RESULTS AND CONVERGENCE SUMMARY" << std::endl;
	std::cout << "================================================================================" << std::endl;
	std::cout << std::scientific << std::setprecision(9);
	std::cout << "Total Steps Completed        : " << step_count << " (Accepted: " << stats.accepted_steps << ", Rejected: " << stats.rejected_steps << ")" << std::endl;
	std::cout << "RHS Evaluations              : " << stats.evaluations << std::endl;
	std::cout << "Execution Elapsed Time       : " << std::fixed << std::setprecision(3) << elapsed_ms << " ms" << std::endl;
	std::cout << std::scientific << std::setprecision(9);
	std::cout << "Max Invariant |p.p| Residual : " << stats.max_invariant_residual << std::endl;
	std::cout << "--------------------------------------------------------------------------------" << std::endl;
	std::cout << "Computed Deflection Angle    : " << deflection_angle_arcsec << " arcsec (" << deflection_angle_rad << " rad)" << std::endl;
	std::cout << "First-Order Analytical (4M/b): " << expected_1st_arcsec << " arcsec (" << expected_1st_rad << " rad)" << std::endl;
	std::cout << "Second-Order 2PN Analytical  : " << expected_2nd_arcsec << " arcsec (" << expected_2nd_rad << " rad)" << std::endl;
	std::cout << "Relative Discrepancy (1st)   : " << rel_err_1st << std::endl;
	std::cout << "Relative Discrepancy (2nd)   : " << rel_err_2nd << std::endl;
	std::cout << "================================================================================" << std::endl;

	if (rel_err_1st < 1e-4 && rel_err_2nd < 1e-5 && stats.max_invariant_residual < 1e-11) {
		std::cout << "TEST STATUS: SUCCESS - Analytical deflection agreement within scientific tolerance." << std::endl;
		return 0;
	}

	std::cerr << "TEST STATUS: FAILURE - Discrepancy exceeds tolerance threshold." << std::endl;
	return 1;
}

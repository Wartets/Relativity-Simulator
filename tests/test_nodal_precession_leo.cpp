#include "relativistic/gravimetry/spherical_harmonics.hpp"
#include "relativistic/gravimetry/orbital_precession.hpp"
#include <iostream>
#include <cassert>
#include <cmath>
#include <numbers>
#include <array>
#include <vector>

void test_analytical_precession_formulas() {
	using namespace Relativistic::Gravimetry;

	const double gm = 3.986004418e14;
	const double r_ref = 6378137.0;
	const double j2 = 1.08262668355e-3;
	const double j4 = -1.61962159137e-6;

	const double altitude = 800000.0;
	const double a = r_ref + altitude;
	const double e = 0.001;

	const double sso_inc = OrbitalPrecessionAnalytic::sun_synchronous_inclination_rad(gm, r_ref, j2, a, e);
	const double sso_rate_rad_day = OrbitalPrecessionAnalytic::nodal_precession_rate_rad_day(gm, r_ref, j2, a, e, sso_inc);
	const double target_sso_rate_rad_day = (2.0 * std::numbers::pi_v<double>) / 365.2422;

	const double sso_diff = std::abs(sso_rate_rad_day - target_sso_rate_rad_day);
	assert(sso_diff < 1e-12);

	const double inc_equatorial = 0.0;
	const double rate_eq = OrbitalPrecessionAnalytic::nodal_precession_rate_j2_rad_s(gm, r_ref, j2, a, e, inc_equatorial);
	const double n = std::sqrt(gm / (a * a * a));
	const double p = a * (1.0 - e * e);
	const double expected_rate_eq = -1.5 * n * j2 * (r_ref / p) * (r_ref / p);
	assert(std::abs(rate_eq - expected_rate_eq) < 1e-15);

	const double inc_polar = std::numbers::pi_v<double> / 2.0;
	const double rate_polar = OrbitalPrecessionAnalytic::nodal_precession_rate_j2_rad_s(gm, r_ref, j2, a, e, inc_polar);
	assert(std::abs(rate_polar) < 1e-15);

	const double inc_crit = std::acos(1.0 / std::sqrt(5.0));
	const double apsidal_crit = OrbitalPrecessionAnalytic::apsidal_precession_rate_j2_rad_s(gm, r_ref, j2, a, e, inc_crit);
	assert(std::abs(apsidal_crit) < 1e-14);

	const double j2_j4_rate = OrbitalPrecessionAnalytic::nodal_precession_rate_j2_j4_rad_s(gm, r_ref, j2, j4, a, e, sso_inc);
	const double j2_only_rate = OrbitalPrecessionAnalytic::nodal_precession_rate_j2_rad_s(gm, r_ref, j2, a, e, sso_inc);
	const double delta_j4 = std::abs(j2_j4_rate - j2_only_rate);
	assert(delta_j4 > 0.0);
	assert(delta_j4 < 1e-6);
}

void test_numerical_nodal_precession_orbit() {
	using namespace Relativistic::Gravimetry;

	const double gm = 3.986004418e14;
	const double r_ref = 6378137.0;
	const double j2 = 1.08262668355e-3;

	SphericalHarmonicsGravityModel<4> earth(gm, r_ref, 2);
	earth.set_zonal_coefficient(2, j2);

	const double altitude = 700000.0;
	const double a = r_ref + altitude;
	const double e = 0.0;
	const double inc = 98.2 * std::numbers::pi_v<double> / 180.0;
	const double raan0 = 0.0;

	const double r0 = a;
	const double v0 = std::sqrt(gm / a);

	std::array<double, 3> r_pos = {r0, 0.0, 0.0};
	std::array<double, 3> v_vel = {0.0, v0 * std::cos(inc), v0 * std::sin(inc)};

	const double theoretical_rate_rad_s = OrbitalPrecessionAnalytic::nodal_precession_rate_j2_rad_s(
		gm, r_ref, j2, a, e, inc
	);

	const double n_mean = std::sqrt(gm / (a * a * a));
	const double period = 2.0 * std::numbers::pi_v<double> / n_mean;

	const double dt = 2.0;
	const size_t num_orbits = 40;
	const double total_duration = static_cast<double>(num_orbits) * period;
	const size_t steps = static_cast<size_t>(total_duration / dt);

	double prev_z = r_pos[2];
	std::vector<double> node_crossing_times;
	std::vector<double> node_crossing_raan;
	double unwrap_offset = 0.0;
	double last_raan = raan0;

	for (size_t step = 0; step < steps; ++step) {
		const auto a1 = earth.evaluate_acceleration(r_pos);
		const std::array<double, 3> r2 = {
			r_pos[0] + 0.5 * dt * v_vel[0],
			r_pos[1] + 0.5 * dt * v_vel[1],
			r_pos[2] + 0.5 * dt * v_vel[2]
		};
		const std::array<double, 3> v2 = {
			v_vel[0] + 0.5 * dt * a1[0],
			v_vel[1] + 0.5 * dt * a1[1],
			v_vel[2] + 0.5 * dt * a1[2]
		};

		const auto a2 = earth.evaluate_acceleration(r2);
		const std::array<double, 3> r3 = {
			r_pos[0] + 0.5 * dt * v2[0],
			r_pos[1] + 0.5 * dt * v2[1],
			r_pos[2] + 0.5 * dt * v2[2]
		};
		const std::array<double, 3> v3 = {
			v_vel[0] + 0.5 * dt * a2[0],
			v_vel[1] + 0.5 * dt * a2[1],
			v_vel[2] + 0.5 * dt * a2[2]
		};

		const auto a3 = earth.evaluate_acceleration(r3);
		const std::array<double, 3> r4 = {
			r_pos[0] + dt * v3[0],
			r_pos[1] + dt * v3[1],
			r_pos[2] + dt * v3[2]
		};
		const std::array<double, 3> v4 = {
			v_vel[0] + dt * a3[0],
			v_vel[1] + dt * a3[1],
			v_vel[2] + dt * a3[2]
		};

		const auto a4 = earth.evaluate_acceleration(r4);

		for (size_t c = 0; c < 3; ++c) {
			r_pos[c] += (dt / 6.0) * (v_vel[c] + 2.0 * v2[c] + 2.0 * v3[c] + v4[c]);
			v_vel[c] += (dt / 6.0) * (a1[c] + 2.0 * a2[c] + 2.0 * a3[c] + a4[c]);
		}

		if (prev_z < 0.0 && r_pos[2] >= 0.0 && v_vel[2] > 0.0) {
			const double frac = -prev_z / (r_pos[2] - prev_z + 1e-30);
			const double t_node = (static_cast<double>(step) - 1.0 + frac) * dt;

			const auto elem = OrbitalPrecessionAnalytic::cartesian_to_osculating(r_pos, v_vel, gm);
			double cur_raan = elem.raan;

			if (!node_crossing_raan.empty()) {
				if (cur_raan - last_raan < -std::numbers::pi_v<double>) {
					unwrap_offset += 2.0 * std::numbers::pi_v<double>;
				} else if (cur_raan - last_raan > std::numbers::pi_v<double>) {
					unwrap_offset -= 2.0 * std::numbers::pi_v<double>;
				}
			}
			last_raan = cur_raan;

			node_crossing_times.push_back(t_node);
			node_crossing_raan.push_back(cur_raan + unwrap_offset);
		}
		prev_z = r_pos[2];
	}

	assert(node_crossing_times.size() >= 30);

	const size_t n_nodes = node_crossing_times.size();
	double sum_t = 0.0, sum_om = 0.0, sum_tt = 0.0, sum_tom = 0.0;
	for (size_t i = 0; i < n_nodes; ++i) {
		const double t_val = node_crossing_times[i];
		const double om_val = node_crossing_raan[i];
		sum_t += t_val;
		sum_om += om_val;
		sum_tt += t_val * t_val;
		sum_tom += t_val * om_val;
	}

	const double dn = static_cast<double>(n_nodes);
	const double numerical_rate_rad_s = (dn * sum_tom - sum_t * sum_om) / (dn * sum_tt - sum_t * sum_t);

	const double rel_diff = std::abs(numerical_rate_rad_s - theoretical_rate_rad_s) / std::abs(theoretical_rate_rad_s);
	assert(rel_diff < 0.02);

	const double theoretical_rate_rad_day = theoretical_rate_rad_s * 86400.0;
	const double numerical_rate_rad_day = numerical_rate_rad_s * 86400.0;
	assert(std::abs(theoretical_rate_rad_day) > 0.0);
	assert(std::abs(numerical_rate_rad_day) > 0.0);
}

int main() {
	test_analytical_precession_formulas();
	test_numerical_nodal_precession_orbit();
	std::cout << "All LEO nodal precession tests passed.\n";
	return 0;
}

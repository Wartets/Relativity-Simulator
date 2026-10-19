#include "support/test_harness.hpp"
#include "relativistic/gravimetry/spherical_harmonics.hpp"
#include "relativistic/gravimetry/orbital_precession.hpp"
#include <array>
#include <cmath>
#include <numbers>
#include <vector>

using namespace Relativistic::Gravimetry;
using Relativistic::Testing::TestSuite;

namespace {

constexpr double gm = 3.986004418e14;
constexpr double reference_radius = 6378137.0;
constexpr double j2 = 1.08262668355e-3;

void check_analytical_formulas(TestSuite& suite) {
	suite.section("Analytical precession formulas");
	const double j4 = -1.61962159137e-6;
	const double a = reference_radius + 800000.0;
	const double e = 0.001;

	const double sso_inclination = OrbitalPrecessionAnalytic::sun_synchronous_inclination_rad(gm, reference_radius, j2, a, e);
	const double sso_rate = OrbitalPrecessionAnalytic::nodal_precession_rate_rad_day(gm, reference_radius, j2, a, e, sso_inclination);
	suite.check_near(sso_rate, (2.0 * std::numbers::pi_v<double>) / 365.2422, 1e-12, "the sun-synchronous inclination yields a nodal drift of one revolution per tropical year");

	const double mean_motion = std::sqrt(gm / (a * a * a));
	const double semi_latus = a * (1.0 - e * e);
	const double expected_equatorial = -1.5 * mean_motion * j2 * (reference_radius / semi_latus) * (reference_radius / semi_latus);
	suite.check_near(OrbitalPrecessionAnalytic::nodal_precession_rate_j2_rad_s(gm, reference_radius, j2, a, e, 0.0), expected_equatorial, 1e-15, "the equatorial nodal rate equals -3/2 n J2 (R/p)^2");
	suite.check_at_most(std::abs(OrbitalPrecessionAnalytic::nodal_precession_rate_j2_rad_s(gm, reference_radius, j2, a, e, std::numbers::pi_v<double> / 2.0)), 1e-15, "a polar orbit has no J2 nodal drift");
	suite.check_at_most(std::abs(OrbitalPrecessionAnalytic::apsidal_precession_rate_j2_rad_s(gm, reference_radius, j2, a, e, std::acos(1.0 / std::sqrt(5.0)))), 1e-14, "the apsidal drift vanishes at the critical inclination");

	const double with_j4 = OrbitalPrecessionAnalytic::nodal_precession_rate_j2_j4_rad_s(gm, reference_radius, j2, j4, a, e, sso_inclination);
	const double without_j4 = OrbitalPrecessionAnalytic::nodal_precession_rate_j2_rad_s(gm, reference_radius, j2, a, e, sso_inclination);
	const double j4_contribution = std::abs(with_j4 - without_j4);
	suite.check(j4_contribution > 0.0, "the J4 term changes the nodal rate");
	suite.check_at_most(j4_contribution, 1e-6, "the J4 correction stays small against the J2 rate");
}

void check_numerical_orbit(TestSuite& suite) {
	suite.section("Numerical nodal drift");
	SphericalHarmonicsGravityModel<4> earth(gm, reference_radius, 2);
	earth.set_zonal_coefficient(2, j2);

	const double a = reference_radius + 700000.0;
	const double inclination = 98.2 * std::numbers::pi_v<double> / 180.0;
	const double speed = std::sqrt(gm / a);
	std::array<double, 3> position = {a, 0.0, 0.0};
	std::array<double, 3> velocity = {0.0, speed * std::cos(inclination), speed * std::sin(inclination)};
	const double theoretical_rate = OrbitalPrecessionAnalytic::nodal_precession_rate_j2_rad_s(gm, reference_radius, j2, a, 0.0, inclination);

	const double mean_motion = std::sqrt(gm / (a * a * a));
	const double period = 2.0 * std::numbers::pi_v<double> / mean_motion;
	const double dt = 2.0;
	const size_t steps = static_cast<size_t>(40.0 * period / dt);

	double previous_z = position[2];
	std::vector<double> crossing_times;
	std::vector<double> crossing_raan;
	double unwrap_offset = 0.0;
	double last_raan = 0.0;

	for (size_t step = 0; step < steps; ++step) {
		const auto a1 = earth.evaluate_acceleration(position);
		const std::array<double, 3> r2 = {position[0] + 0.5 * dt * velocity[0], position[1] + 0.5 * dt * velocity[1], position[2] + 0.5 * dt * velocity[2]};
		const std::array<double, 3> v2 = {velocity[0] + 0.5 * dt * a1[0], velocity[1] + 0.5 * dt * a1[1], velocity[2] + 0.5 * dt * a1[2]};
		const auto a2 = earth.evaluate_acceleration(r2);
		const std::array<double, 3> r3 = {position[0] + 0.5 * dt * v2[0], position[1] + 0.5 * dt * v2[1], position[2] + 0.5 * dt * v2[2]};
		const std::array<double, 3> v3 = {velocity[0] + 0.5 * dt * a2[0], velocity[1] + 0.5 * dt * a2[1], velocity[2] + 0.5 * dt * a2[2]};
		const auto a3 = earth.evaluate_acceleration(r3);
		const std::array<double, 3> r4 = {position[0] + dt * v3[0], position[1] + dt * v3[1], position[2] + dt * v3[2]};
		const std::array<double, 3> v4 = {velocity[0] + dt * a3[0], velocity[1] + dt * a3[1], velocity[2] + dt * a3[2]};
		const auto a4 = earth.evaluate_acceleration(r4);
		for (size_t axis = 0; axis < 3; ++axis) {
			position[axis] += (dt / 6.0) * (velocity[axis] + 2.0 * v2[axis] + 2.0 * v3[axis] + v4[axis]);
			velocity[axis] += (dt / 6.0) * (a1[axis] + 2.0 * a2[axis] + 2.0 * a3[axis] + a4[axis]);
		}
		if (previous_z < 0.0 && position[2] >= 0.0 && velocity[2] > 0.0) {
			const double fraction = -previous_z / (position[2] - previous_z + 1e-30);
			const auto elements = OrbitalPrecessionAnalytic::cartesian_to_osculating(position, velocity, gm);
			if (!crossing_raan.empty()) {
				if (elements.raan - last_raan < -std::numbers::pi_v<double>) {
					unwrap_offset += 2.0 * std::numbers::pi_v<double>;
				} else if (elements.raan - last_raan > std::numbers::pi_v<double>) {
					unwrap_offset -= 2.0 * std::numbers::pi_v<double>;
				}
			}
			last_raan = elements.raan;
			crossing_times.push_back((static_cast<double>(step) - 1.0 + fraction) * dt);
			crossing_raan.push_back(elements.raan + unwrap_offset);
		}
		previous_z = position[2];
	}

	suite.check(crossing_times.size() >= 30, "at least thirty ascending node crossings are detected", "nodes=" + std::to_string(crossing_times.size()));
	if (crossing_times.size() < 2) {
		return;
	}
	double sum_t = 0.0;
	double sum_om = 0.0;
	double sum_tt = 0.0;
	double sum_tom = 0.0;
	for (size_t i = 0; i < crossing_times.size(); ++i) {
		sum_t += crossing_times[i];
		sum_om += crossing_raan[i];
		sum_tt += crossing_times[i] * crossing_times[i];
		sum_tom += crossing_times[i] * crossing_raan[i];
	}
	const double count = static_cast<double>(crossing_times.size());
	const double numerical_rate = (count * sum_tom - sum_t * sum_om) / (count * sum_tt - sum_t * sum_t);
	suite.measure("theoretical_nodal_rate", theoretical_rate * 86400.0, "rad/day");
	suite.measure("numerical_nodal_rate", numerical_rate * 86400.0, "rad/day");
	suite.check_relative(numerical_rate, theoretical_rate, 0.02, "the regression slope of the node longitude matches the analytic J2 rate within 2 percent");
}

}

int main() {
	TestSuite suite(
		"nodal_precession_leo",
		"Verifies the secular J2 perturbation of low Earth orbits: the sun-synchronous inclination, the equatorial and polar nodal rates, the vanishing apsidal drift at the critical inclination, the size of the J4 correction, and the nodal drift of a numerically integrated 98.2 degree orbit against the analytic rate."
	);
	check_analytical_formulas(suite);
	check_numerical_orbit(suite);
	return suite.finish();
}

#include "relativistic/io/ephemeris_types.hpp"
#include <iostream>
#include <cmath>
#include <cassert>

int main() {
	using namespace Relativistic::IO;

	const Epoch j2000 = Epoch::from_calendar(2000, 1, 1.5);
	assert(std::abs(j2000.jd - 2451545.0) < 1e-12);
	assert(std::abs(j2000.to_j2000_seconds()) < 1e-12);

	const Epoch epoch_2001 = Epoch::from_calendar(2001, 1, 1.5);
	assert(std::abs(epoch_2001.jd - 2451911.0) < 1e-12);

	KeplerianElements earth_orbit;
	earth_orbit.semi_major_axis = 149597870700.0;
	earth_orbit.eccentricity = 0.0167086;
	earth_orbit.inclination = 0.00005 * std::numbers::pi_v<double> / 180.0;
	earth_orbit.longitude_ascending_node = -11.26064 * std::numbers::pi_v<double> / 180.0;
	earth_orbit.argument_of_periapsis = 114.20783 * std::numbers::pi_v<double> / 180.0;
	earth_orbit.mean_anomaly = 358.617 * std::numbers::pi_v<double> / 180.0;
	earth_orbit.gravitational_parameter = 1.32712440018e20;

	const double e_anom = earth_orbit.eccentric_anomaly();
	const double nu = earth_orbit.true_anomaly();
	const double two_pi = 2.0 * std::numbers::pi_v<double>;
	const double diff_e = std::abs(std::remainder(e_anom - earth_orbit.mean_anomaly, two_pi));
	const double diff_nu = std::abs(std::remainder(nu - earth_orbit.mean_anomaly, two_pi));
	assert(diff_e < 0.04);
	assert(diff_nu < 0.04);

	const EphemerisStateVector state = earth_orbit.to_state_vector(j2000, NaifBodyId::EARTH, NaifBodyId::SUN);
	const double r = std::sqrt(state.position(1) * state.position(1) + state.position(2) * state.position(2) + state.position(3) * state.position(3));
	const double v = std::sqrt(state.velocity(1) * state.velocity(1) + state.velocity(2) * state.velocity(2) + state.velocity(3) * state.velocity(3));

	assert(r > 0.98 * 149597870700.0 && r < 1.02 * 149597870700.0);
	assert(v > 29000.0 && v < 31000.0);

	return 0;
}

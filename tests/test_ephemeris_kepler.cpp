#include "support/test_harness.hpp"
#include "relativistic/io/ephemeris_types.hpp"
#include <cmath>
#include <numbers>

using namespace Relativistic::IO;
using Relativistic::Testing::TestSuite;

int main() {
	TestSuite suite(
		"ephemeris_kepler",
		"Verifies the ephemeris primitives: calendar to Julian date conversion at J2000 and one year later, and the conversion of the osculating elements of the Earth into a state vector with the expected heliocentric distance and orbital speed."
	);

	suite.section("Epoch conversion");
	const Epoch j2000 = Epoch::from_calendar(2000, 1, 1.5);
	suite.check_near(j2000.jd, 2451545.0, 1e-12, "1 January 2000 at noon is the Julian date 2451545");
	suite.check_near(j2000.to_j2000_seconds(), 0.0, 1e-12, "J2000 corresponds to zero seconds past J2000");
	const Epoch next_year = Epoch::from_calendar(2001, 1, 1.5);
	suite.check_near(next_year.jd, 2451911.0, 1e-12, "1 January 2001 at noon is the Julian date 2451911");

	suite.section("Keplerian elements of the Earth");
	constexpr double astronomical_unit = 149597870700.0;
	constexpr double degrees = std::numbers::pi_v<double> / 180.0;
	KeplerianElements orbit;
	orbit.semi_major_axis = astronomical_unit;
	orbit.eccentricity = 0.0167086;
	orbit.inclination = 0.00005 * degrees;
	orbit.longitude_ascending_node = -11.26064 * degrees;
	orbit.argument_of_periapsis = 114.20783 * degrees;
	orbit.mean_anomaly = 358.617 * degrees;
	orbit.gravitational_parameter = 1.32712440018e20;

	const double two_pi = 2.0 * std::numbers::pi_v<double>;
	const double eccentric_gap = std::abs(std::remainder(orbit.eccentric_anomaly() - orbit.mean_anomaly, two_pi));
	const double true_gap = std::abs(std::remainder(orbit.true_anomaly() - orbit.mean_anomaly, two_pi));
	suite.check_at_most(eccentric_gap, 0.04, "the eccentric anomaly stays within 0.04 rad of the mean anomaly for a small eccentricity");
	suite.check_at_most(true_gap, 0.04, "the true anomaly stays within 0.04 rad of the mean anomaly for a small eccentricity");

	const EphemerisStateVector state = orbit.to_state_vector(j2000, NaifBodyId::EARTH, NaifBodyId::SUN);
	const double distance = std::sqrt(state.position(1) * state.position(1) + state.position(2) * state.position(2) + state.position(3) * state.position(3));
	const double speed = std::sqrt(state.velocity(1) * state.velocity(1) + state.velocity(2) * state.velocity(2) + state.velocity(3) * state.velocity(3));
	suite.measure("heliocentric_distance", distance, "m");
	suite.measure("orbital_speed", speed, "m/s");
	suite.check(distance > 0.98 * astronomical_unit && distance < 1.02 * astronomical_unit, "the heliocentric distance stays within 2 percent of one astronomical unit");
	suite.check(speed > 29000.0 && speed < 31000.0, "the orbital speed lies between 29 and 31 kilometres per second");
	return suite.finish();
}

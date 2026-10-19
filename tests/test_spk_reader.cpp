#include "support/test_harness.hpp"
#include "relativistic/io/spk_reader.hpp"
#include "relativistic/io/ephemeris_types.hpp"
#include <cmath>
#include <cstddef>

using namespace Relativistic::IO;
using Relativistic::Testing::TestSuite;

namespace {

[[nodiscard]] SpkChebyshevSegment make_segment(int target, int center, std::size_t coefficients_per_component) {
	SpkChebyshevSegment segment;
	segment.target_id = target;
	segment.center_id = center;
	segment.record_type = SpkRecordType::ChebyshevPositionOnly;
	segment.initial_epoch_sec = -86400.0;
	segment.final_epoch_sec = 86400.0;
	segment.initial_epoch_record = -86400.0;
	segment.interval_length_sec = 86400.0 * 2.0;
	segment.coefficients_per_component = coefficients_per_component;
	segment.record_count = 1;
	return segment;
}

}

int main() {
	TestSuite suite(
		"spk_reader",
		"Verifies the SPICE SPK Chebyshev segment evaluation: the zeroth coefficient is returned at the segment midpoint in SI units, several segments coexist in one kernel, and relative states are computed as the difference of target and centre states."
	);

	constexpr double earth_x_km = -2.607481358742887e+07;
	constexpr double earth_y_km = 1.328724185790479e+08;
	constexpr double earth_z_km = 5.759799291129994e+07;
	constexpr double earth_vx_km_day = -2.578680199182375e+06;
	constexpr double earth_vy_km_day = -4.512683935639457e+05;
	constexpr double earth_vz_km_day = -1.956698656121927e+05;

	auto earth = make_segment(NaifBodyId::EARTH, NaifBodyId::SOLAR_SYSTEM_BARYCENTER, 5);
	earth.coefficients = {
		earth_x_km, earth_vx_km_day / 2.0, 0.0, 0.0, 0.0,
		earth_y_km, earth_vy_km_day / 2.0, 0.0, 0.0, 0.0,
		earth_z_km, earth_vz_km_day / 2.0, 0.0, 0.0, 0.0
	};

	constexpr double moon_x_km = -2.915844445831969e+05;
	constexpr double moon_y_km = -2.664426543452488e+05;
	constexpr double moon_z_km = -7.608381533519894e+04;
	auto moon = make_segment(NaifBodyId::MOON, NaifBodyId::EARTH, 3);
	moon.coefficients = {moon_x_km, 10.0, 0.0, moon_y_km, -5.0, 0.0, moon_z_km, 2.0, 0.0};

	constexpr double jupiter_x_km = 5.989781600744211e+08;
	constexpr double jupiter_y_km = 4.417387224213969e+08;
	constexpr double jupiter_z_km = 1.764024823194098e+08;
	auto jupiter = make_segment(NaifBodyId::JUPITER_BARYCENTER, NaifBodyId::SOLAR_SYSTEM_BARYCENTER, 3);
	jupiter.coefficients = {jupiter_x_km, -100.0, 0.0, jupiter_y_km, 200.0, 0.0, jupiter_z_km, 50.0, 0.0};

	SpkKernel kernel;
	kernel.add_segment(earth);
	kernel.add_segment(moon);
	kernel.add_segment(jupiter);
	const Epoch j2000(Epoch::J2000_JD);

	suite.section("Absolute states at the segment midpoint");
	const auto earth_state = kernel.evaluate_body(NaifBodyId::EARTH, j2000);
	suite.check(earth_state.has_value(), "the Earth segment is found at J2000");
	if (earth_state.has_value()) {
		suite.check_relative(earth_state->position(1), earth_x_km * 1000.0, 1e-12, "the Earth x position equals the zeroth coefficient in metres");
		suite.check_relative(earth_state->position(2), earth_y_km * 1000.0, 1e-12, "the Earth y position equals the zeroth coefficient in metres");
		suite.check_relative(earth_state->position(3), earth_z_km * 1000.0, 1e-12, "the Earth z position equals the zeroth coefficient in metres");
	}
	const auto moon_state = kernel.evaluate_body(NaifBodyId::MOON, j2000);
	suite.check(moon_state.has_value(), "the Moon segment is found at J2000");
	if (moon_state.has_value()) {
		suite.check_relative(moon_state->position(1), moon_x_km * 1000.0, 1e-12, "the Moon x position equals the zeroth coefficient in metres");
	}

	suite.section("Relative states");
	const auto relative = kernel.evaluate_relative(NaifBodyId::JUPITER_BARYCENTER, NaifBodyId::EARTH, j2000);
	suite.check(relative.has_value(), "the Jupiter state relative to the Earth is available");
	if (relative.has_value()) {
		suite.check_relative(relative->position(1), (jupiter_x_km - earth_x_km) * 1000.0, 1e-12, "the relative x position equals the difference of the absolute positions");
	}
	return suite.finish();
}

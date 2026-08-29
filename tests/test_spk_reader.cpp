#include "relativistic/io/spk_reader.hpp"
#include "relativistic/io/ephemeris_types.hpp"
#include <iostream>
#include <cmath>
#include <cassert>

int main() {
	using namespace Relativistic::IO;

	SpkChebyshevSegment earth_segment;
	earth_segment.target_id = NaifBodyId::EARTH;
	earth_segment.center_id = NaifBodyId::SOLAR_SYSTEM_BARYCENTER;
	earth_segment.record_type = SpkRecordType::ChebyshevPositionOnly;
	earth_segment.initial_epoch_sec = -86400.0;
	earth_segment.final_epoch_sec = 86400.0;
	earth_segment.initial_epoch_record = -86400.0;
	earth_segment.interval_length_sec = 86400.0 * 2.0;
	earth_segment.coefficients_per_component = 5;
	earth_segment.record_count = 1;

	const double x0_km = -2.607481358742887e+07;
	const double y0_km =  1.328724185790479e+08;
	const double z0_km =  5.759799291129994e+07;

	const double vx0_km_day = -2.578680199182375e+06;
	const double vy0_km_day = -4.512683935639457e+05;
	const double vz0_km_day = -1.956698656121927e+05;

	earth_segment.coefficients = {
		x0_km, vx0_km_day / 2.0, 0.0, 0.0, 0.0,
		y0_km, vy0_km_day / 2.0, 0.0, 0.0, 0.0,
		z0_km, vz0_km_day / 2.0, 0.0, 0.0, 0.0
	};

	SpkKernel kernel;
	kernel.add_segment(earth_segment);

	const Epoch j2000(Epoch::J2000_JD);
	const auto state_opt = kernel.evaluate_body(NaifBodyId::EARTH, j2000);
	assert(state_opt.has_value());

	const double expected_x_m = x0_km * 1000.0;
	const double expected_y_m = y0_km * 1000.0;
	const double expected_z_m = z0_km * 1000.0;

	const double diff_x = std::abs(state_opt->position(1) - expected_x_m);
	const double diff_y = std::abs(state_opt->position(2) - expected_y_m);
	const double diff_z = std::abs(state_opt->position(3) - expected_z_m);

	assert(diff_x < 1e-4);
	assert(diff_y < 1e-4);
	assert(diff_z < 1e-4);

	SpkChebyshevSegment moon_segment;
	moon_segment.target_id = NaifBodyId::MOON;
	moon_segment.center_id = NaifBodyId::EARTH;
	moon_segment.record_type = SpkRecordType::ChebyshevPositionOnly;
	moon_segment.initial_epoch_sec = -86400.0;
	moon_segment.final_epoch_sec = 86400.0;
	moon_segment.initial_epoch_record = -86400.0;
	moon_segment.interval_length_sec = 86400.0 * 2.0;
	moon_segment.coefficients_per_component = 3;
	moon_segment.record_count = 1;

	const double moon_x_km = -2.915844445831969e+05;
	const double moon_y_km = -2.664426543452488e+05;
	const double moon_z_km = -7.608381533519894e+04;

	moon_segment.coefficients = {
		moon_x_km, 10.0, 0.0,
		moon_y_km, -5.0, 0.0,
		moon_z_km,  2.0, 0.0
	};
	kernel.add_segment(moon_segment);

	const auto moon_state = kernel.evaluate_body(NaifBodyId::MOON, j2000);
	assert(moon_state.has_value());
	assert(std::abs(moon_state->position(1) - moon_x_km * 1000.0) < 1e-4);

	SpkChebyshevSegment jupiter_segment;
	jupiter_segment.target_id = NaifBodyId::JUPITER_BARYCENTER;
	jupiter_segment.center_id = NaifBodyId::SOLAR_SYSTEM_BARYCENTER;
	jupiter_segment.record_type = SpkRecordType::ChebyshevPositionOnly;
	jupiter_segment.initial_epoch_sec = -86400.0;
	jupiter_segment.final_epoch_sec = 86400.0;
	jupiter_segment.initial_epoch_record = -86400.0;
	jupiter_segment.interval_length_sec = 86400.0 * 2.0;
	jupiter_segment.coefficients_per_component = 3;
	jupiter_segment.record_count = 1;

	const double jup_x_km = 5.989781600744211e+08;
	const double jup_y_km = 4.417387224213969e+08;
	const double jup_z_km = 1.764024823194098e+08;

	jupiter_segment.coefficients = {
		jup_x_km, -100.0, 0.0,
		jup_y_km,  200.0, 0.0,
		jup_z_km,   50.0, 0.0
	};
	kernel.add_segment(jupiter_segment);

	const auto jup_rel = kernel.evaluate_relative(NaifBodyId::JUPITER_BARYCENTER, NaifBodyId::EARTH, j2000);
	assert(jup_rel.has_value());

	const double expected_rel_x = (jup_x_km - x0_km) * 1000.0;
	assert(std::abs(jup_rel->position(1) - expected_rel_x) < 1e-3);

	return 0;
}

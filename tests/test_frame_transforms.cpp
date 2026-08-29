#include "relativistic/io/frame_transforms.hpp"
#include <iostream>
#include <cmath>
#include <cassert>

int main() {
	using namespace Relativistic::IO;

	const EphemerisStateVector icrf_state(
		Epoch(Epoch::J2000_JD),
		1.0e11, 2.0e11, 3.0e11,
		1000.0, 2000.0, 3000.0
	);

	const auto ecl_state = FrameTransformer<double>::transform_icrf_to_ecliptic(icrf_state);
	const auto round_trip_state = FrameTransformer<double>::transform_ecliptic_to_icrf(ecl_state);

	assert(std::abs(round_trip_state.position(1) - icrf_state.position(1)) < 1e-5);
	assert(std::abs(round_trip_state.position(2) - icrf_state.position(2)) < 1e-5);
	assert(std::abs(round_trip_state.position(3) - icrf_state.position(3)) < 1e-5);

	const auto boost_zero = FrameTransformer<double>::lorentz_boost_matrix(0.0, 0.0, 0.0);
	assert(boost_zero(0, 0) == 1.0);
	assert(boost_zero(1, 1) == 1.0);

	const double beta_v = 0.6;
	const auto boost_x = FrameTransformer<double>::lorentz_boost_matrix(beta_v, 0.0, 0.0);
	const double expected_gamma = 1.0 / std::sqrt(1.0 - beta_v * beta_v);
	assert(std::abs(boost_x(0, 0) - expected_gamma) < 1e-12);
	assert(std::abs(boost_x(0, 1) - (-expected_gamma * beta_v)) < 1e-12);

	return 0;
}

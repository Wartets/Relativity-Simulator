#include "relativistic/io/horizons_parser.hpp"
#include <iostream>
#include <cassert>

int main() {
	using namespace Relativistic::IO;

	HorizonsQueryConfig cfg;
	cfg.target_body_id = NaifBodyId::EARTH;
	cfg.center_body_id = NaifBodyId::SOLAR_SYSTEM_BARYCENTER;
	cfg.start_epoch = Epoch(2451545.0);
	cfg.stop_epoch = Epoch(2451546.0);
	cfg.step_days = 1.0;

	const std::string url = HorizonsInterface::build_query_url(cfg);
	assert(url.find("COMMAND='399'") != std::string::npos);
	assert(url.find("CENTER='@0'") != std::string::npos);
	assert(url.find("EPHEM_TYPE='VECTORS'") != std::string::npos);

	const std::string_view sample_response =
		"*******************************************************************************\n"
		"$$SOE\n"
		"2451545.000000000, A.D. 2000-Jan-01 12:00:00.0000, -2.607481358742887E+07,  1.328724185790479E+08,  5.759799291129994E+07, -2.983944679782522E+01, -5.223019370832705E+00, -2.264697518659638E+00,  4.908051214041774E+02,  1.470414967339077E+08, -2.484218671609142E-01\n"
		"2451546.000000000, A.D. 2000-Jan-02 12:00:00.0000, -2.865181745494276E+07,  1.323863412586208E+08,  5.738734267439564E+07, -2.978436573887968E+01, -6.046180370425717E+00, -2.621577903264627E+00,  4.903828975306644E+02,  1.469149021665422E+08, -4.437435160897710E-01\n"
		"$$EOE\n"
		"*******************************************************************************\n";

	const auto vectors = HorizonsInterface::parse_horizons_response(sample_response, NaifBodyId::EARTH, NaifBodyId::SOLAR_SYSTEM_BARYCENTER);
	assert(vectors.size() == 2);

	assert(std::abs(vectors[0].epoch.jd - 2451545.0) < 1e-12);
	assert(std::abs(vectors[0].position(1) - (-2.607481358742887e+10)) < 1.0);
	assert(std::abs(vectors[0].velocity(1) - (-29839.44679782522)) < 1e-3);

	assert(std::abs(vectors[1].epoch.jd - 2451546.0) < 1e-12);
	assert(std::abs(vectors[1].position(1) - (-2.865181745494276e+10)) < 1.0);

	return 0;
}

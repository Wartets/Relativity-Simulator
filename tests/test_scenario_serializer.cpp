#include "relativistic/io/scenario_serializer.hpp"
#include <iostream>
#include <cassert>

int main() {
	using namespace Relativistic::IO;

	ScenarioDefinition s_orig;
	s_orig.scenario_name = "KerrAccretionTest";
	s_orig.metric_type = "Kerr";
	s_orig.central_mass = 10.0;
	s_orig.central_spin = 0.95;
	s_orig.integrator.scheme = "Vernier9";
	s_orig.integrator.initial_step = 0.005;
	s_orig.integrator.relative_tolerance = 1e-12;
	s_orig.output.fits_enabled = true;
	s_orig.output.hdf5_enabled = true;
	s_orig.output.vtk_enabled = true;

	const std::string yaml = ScenarioSerializer::to_yaml(s_orig);
	assert(!yaml.empty());
	assert(yaml.find("scenario_name: \"KerrAccretionTest\"") != std::string::npos);
	assert(yaml.find("central_spin: 0.95") != std::string::npos);

	const auto parsed = ScenarioSerializer::from_yaml(yaml);
	assert(parsed.has_value());
	assert(parsed->scenario_name == "KerrAccretionTest");
	assert(parsed->metric_type == "Kerr");
	assert(std::abs(parsed->central_mass - 10.0) < 1e-12);
	assert(std::abs(parsed->central_spin - 0.95) < 1e-12);
	assert(parsed->integrator.scheme == "Vernier9");
	assert(std::abs(parsed->integrator.initial_step - 0.005) < 1e-12);
	assert(parsed->output.fits_enabled == true);

	return 0;
}

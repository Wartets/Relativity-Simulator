#include "support/test_harness.hpp"
#include "relativistic/io/scenario/scenario_serializer.hpp"
#include <string>

using namespace Relativistic::IO;
using Relativistic::Testing::TestSuite;

namespace {

[[nodiscard]] ScenarioDefinition make_scenario(const char* metric, double mass, double spin) {
	ScenarioDefinition scenario;
	scenario.scenario_name = "SerializerTest";
	scenario.metric_type = metric;
	scenario.central_mass = mass;
	scenario.central_spin = spin;
	return scenario;
}

void check_round_trip(TestSuite& suite) {
	suite.section("YAML round trip");
	ScenarioDefinition original = make_scenario("Kerr", 10.0, 0.95);
	original.scenario_name = "KerrAccretionTest";
	original.integrator.scheme = "Vernier9";
	original.integrator.initial_step = 0.005;
	original.integrator.relative_tolerance = 1e-12;
	original.output.fits_enabled = true;
	original.output.hdf5_enabled = false;
	original.output.vtk_enabled = true;

	const std::string yaml = ScenarioSerializer::to_yaml(original);
	suite.check(!yaml.empty(), "serialization produces a document");
	suite.check(yaml.find("scenario_name: \"KerrAccretionTest\"") != std::string::npos, "the scenario name is written as a quoted string");
	suite.check(yaml.find("central_spin: 0.95") != std::string::npos, "the spin is written with its value");

	const auto parsed = ScenarioSerializer::from_yaml(yaml);
	suite.check(parsed.has_value(), "the serialized document parses back");
	if (!parsed.has_value()) {
		return;
	}
	suite.check(parsed->scenario_name == "KerrAccretionTest", "the scenario name survives the round trip");
	suite.check(parsed->metric_type == "Kerr", "the metric type survives the round trip");
	suite.check_near(parsed->central_mass, 10.0, 1e-12, "the central mass survives the round trip");
	suite.check_near(parsed->central_spin, 0.95, 1e-12, "the central spin survives the round trip");
	suite.check(parsed->integrator.scheme == "Vernier9", "the integrator scheme survives the round trip");
	suite.check_near(parsed->integrator.initial_step, 0.005, 1e-12, "the initial step survives the round trip");
	suite.check_relative(parsed->integrator.relative_tolerance, 1e-12, 1e-9, "the relative tolerance survives the round trip");
	suite.check(parsed->output.fits_enabled && !parsed->output.hdf5_enabled && parsed->output.vtk_enabled, "the export switches survive the round trip");
}

void check_validation(TestSuite& suite) {
	suite.section("Validation rules");
	suite.check(ScenarioSerializer::validate(make_scenario("Kerr", 10.0, 0.95)).is_valid, "a sub-extremal Kerr scenario is valid");
	suite.check(ScenarioSerializer::validate(make_scenario("Schwarzschild", 1.0, 0.0)).is_valid, "a Schwarzschild scenario with positive mass is valid");

	ScenarioDefinition unnamed = make_scenario("Kerr", 1.0, 0.5);
	unnamed.scenario_name.clear();
	suite.check(!ScenarioSerializer::validate(unnamed).is_valid, "an empty scenario name is rejected");

	ScenarioDefinition no_metric = make_scenario("Kerr", 1.0, 0.5);
	no_metric.metric_type.clear();
	suite.check(!ScenarioSerializer::validate(no_metric).is_valid, "an empty metric type is rejected");

	const auto unknown = ScenarioSerializer::validate(make_scenario("NotAMetric", 1.0, 0.0));
	suite.check(!unknown.is_valid && !unknown.error_message.empty(), "an unknown metric is rejected with a message");

	suite.check(!ScenarioSerializer::validate(make_scenario("Schwarzschild", 0.0, 0.0)).is_valid, "a Schwarzschild scenario with zero mass is rejected");
	suite.check(!ScenarioSerializer::validate(make_scenario("Kerr", 1.0, 1.5)).is_valid, "a Kerr scenario exceeding the extremal spin is rejected");
	suite.check(ScenarioSerializer::validate(make_scenario("Kerr", 1.0, 1.0)).is_valid, "the extremal spin is accepted within the documented tolerance");
}

}

int main() {
	TestSuite suite(
		"scenario_serializer",
		"Verifies the declarative scenario reader and writer: field fidelity across a serialization round trip and the validation rules for names, metrics, mass and spin."
	);
	check_round_trip(suite);
	check_validation(suite);
	return suite.finish();
}
